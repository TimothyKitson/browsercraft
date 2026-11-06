#include "BookRenderer.h"
#include "Core/GLFunctions.h"
#include "Entity/BoxMesh.h"
#include "Shader.h"
#include "World/World.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <algorithm>
#include <cmath>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };
    constexpr float PI = 3.14159265f;
    constexpr float TICK = 0.05f;
    constexpr int SCAN = 12;

    float wrap(float angle)
    {
        while (angle >= PI) angle -= PI * 2.0f;
        while (angle < -PI) angle += PI * 2.0f;
        return angle;
    }

    BoxMesh::Box part(glm::vec3 pivot, glm::vec3 from, glm::vec3 to, int u, int v, float yaw)
    {
        BoxMesh::Box box;
        box.min = pivot + from;
        box.max = pivot + to;
        box.pivot = pivot;
        box.u = u;
        box.v = v;
        box.yaw = yaw;
        return box;
    }
}

BookRenderer::~BookRenderer()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

const BookRenderer::Book* BookRenderer::bookAt(const glm::ivec3& at) const
{
    const auto found = m_books.find({ at.x, at.y, at.z });
    return found == m_books.end() ? nullptr : &found->second;
}

void BookRenderer::step(Book& book, bool playerNear, float dx, float dz, uint32_t& seed)
{
    auto next = [&](int bound) {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return static_cast<int>(seed % static_cast<uint32_t>(bound));
    };

    book.lastRotation = book.rotation;
    book.lastOpen = book.open;
    book.lastFlip = book.flip;

    if (playerNear)
    {
        book.target = std::atan2(dz, dx);
        book.open += 0.1f;
        if (book.open < 0.5f || next(40) == 0)
        {
            const float before = book.flipRandom;
            do
            {
                book.flipRandom += static_cast<float>(next(4) - next(4));
            } while (before == book.flipRandom);
        }
    }
    else
    {
        book.target += 0.02f;
        book.open -= 0.1f;
    }

    book.rotation = wrap(book.rotation);
    book.target = wrap(book.target);
    book.rotation += wrap(book.target - book.rotation) * 0.4f;
    book.open = std::clamp(book.open, 0.0f, 1.0f);
    ++book.ticks;

    const float pull = std::clamp((book.flipRandom - book.flip) * 0.4f, -0.2f, 0.2f);
    book.flipTurn += (pull - book.flipTurn) * 0.9f;
    book.flip += book.flipTurn;
}

void BookRenderer::update(float deltaTime, const World& world, const glm::vec3& player)
{
    m_player = player;
    m_scanTimer -= deltaTime;
    if (m_scanTimer <= 0.0f)
    {
        m_scanTimer = 2.0f;
        const glm::ivec3 centre = glm::ivec3(glm::floor(player));
        for (auto it = m_books.begin(); it != m_books.end();)
        {
            const auto& [x, y, z] = it->first;
            const bool far = std::abs(x - centre.x) > SCAN + 4 || std::abs(z - centre.z) > SCAN + 4 ||
                             std::abs(y - centre.y) > SCAN + 4;
            if (far || world.getBlock(x, y, z) != Blocks::EnchantingTable) it = m_books.erase(it);
            else ++it;
        }
        for (int dx = -SCAN; dx <= SCAN; ++dx)
            for (int dy = -6; dy <= 6; ++dy)
                for (int dz = -SCAN; dz <= SCAN; ++dz)
                {
                    const glm::ivec3 at = centre + glm::ivec3(dx, dy, dz);
                    if (world.getBlock(at.x, at.y, at.z) != Blocks::EnchantingTable) continue;
                    m_books.try_emplace({ at.x, at.y, at.z }, Book{});
                }
    }

    m_clock += deltaTime;
    while (m_clock >= TICK)
    {
        m_clock -= TICK;
        for (auto& [key, book] : m_books)
        {
            const auto& [x, y, z] = key;
            const float dx = player.x - (x + 0.5f);
            const float dy = player.y - (y + 0.5f);
            const float dz = player.z - (z + 0.5f);
            const bool near = dx * dx + dy * dy + dz * dz < 9.0f;
            step(book, near, dx, dz, m_seed);
        }
    }
}

void BookRenderer::render(Shader& shader, const World& world)
{
    if (m_books.empty()) return;
    if (!m_loaded)
    {
        m_loaded = true;
        int width = 0, height = 0, channels = 0;
        unsigned char* data = stbi_load("assets/textures/entity/enchanting_table_book.png", &width, &height, &channels, 4);
        if (data)
        {
            glGenTextures(1, &m_texture);
            glBindTexture(GL_TEXTURE_2D, m_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            stbi_image_free(data);
        }
    }
    if (!m_texture) return;

    const float partial = m_clock / TICK;
    m_vertices.clear();
    for (const auto& [key, book] : m_books)
    {
        const auto& [x, y, z] = key;
        const float rotation = book.lastRotation + wrap(book.rotation - book.lastRotation) * partial;
        const float open = book.lastOpen + (book.open - book.lastOpen) * partial;
        const float flip = book.lastFlip + (book.flip - book.lastFlip) * partial;
        const float time = static_cast<float>(book.ticks) + partial;

        const float left = std::clamp((flip + 0.25f - std::floor(flip + 0.25f)) * 1.6f - 0.3f, 0.0f, 1.0f);
        const float right = std::clamp((flip + 0.75f - std::floor(flip + 0.75f)) * 1.6f - 0.3f, 0.0f, 1.0f);
        const float spread = (std::sin(time * 0.02f) * 0.1f + 1.25f) * open;
        const float shift = std::sin(spread);

        std::vector<BoxMesh::Box> parts;
        parts.push_back(part({ 0, 0, -1 }, { -6, -5, -0.005f }, { 0, 5, 0.005f }, 0, 0, PI + spread));
        parts.push_back(part({ 0, 0, 1 }, { 0, -5, -0.005f }, { 6, 5, 0.005f }, 16, 0, -spread));
        parts.push_back(part({ 0, 0, 0 }, { -1, -5, 0 }, { 1, 5, 0.005f }, 12, 0, PI * 0.5f));
        parts.push_back(part({ shift, 0, 0 }, { 0, -4, -0.99f }, { 5, 4, 0.01f }, 0, 10, spread));
        parts.push_back(part({ shift, 0, 0 }, { 0, -4, -0.01f }, { 5, 4, 0.99f }, 12, 10, -spread));
        parts.push_back(part({ shift, 0, 0 }, { 0, -4, 0 }, { 5, 4, 0.005f }, 24, 10, spread - spread * 2.0f * left));
        parts.push_back(part({ shift, 0, 0 }, { 0, -4, 0 }, { 5, 4, 0.005f }, 24, 10, spread - spread * 2.0f * right));

        BoxMesh::Frame frame;
        frame.textureWidth = 64.0f;
        frame.textureHeight = 32.0f;
        frame.sky = static_cast<float>(world.skyLight(x, y + 1, z)) / 15.0f;
        frame.blockLight = std::max(static_cast<float>(world.blockLightAt(x, y + 1, z)) / 15.0f, 0.45f);

        glm::mat4 place(1.0f);
        place = glm::translate(place, glm::vec3(x + 0.5f, y + 0.85f + std::sin(time * 0.1f) * 0.01f, z + 0.5f));
        place = glm::rotate(place, -rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        place = glm::rotate(place, glm::radians(80.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        const size_t start = m_vertices.size();
        BoxMesh::appendAll(m_vertices, parts, frame);
        for (size_t i = start; i < m_vertices.size(); i += BoxMesh::FLOATS_PER_VERTEX)
        {
            const glm::vec3 model(m_vertices[i + 2], m_vertices[i + 1], m_vertices[i]);
            const glm::vec3 world4 = glm::vec3(place * glm::vec4(model, 1.0f));
            m_vertices[i] = world4.x;
            m_vertices[i + 1] = world4.y;
            m_vertices[i + 2] = world4.z;
        }
    }

    if (m_vertices.empty()) return;
    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);
    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    m_mesh.draw(GL_TRIANGLES);
    glEnable(GL_CULL_FACE);
}
