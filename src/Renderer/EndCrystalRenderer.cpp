#include "EndCrystalRenderer.h"
#include "Core/GLFunctions.h"
#include "Core/Packs.h"
#include "Shader.h"
#include "World/World.h"
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };
    constexpr float SHEET_W = 64.0f;
    constexpr float SHEET_H = 32.0f;

    float grain(int x, int y)
    {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return static_cast<float>((h ^ (h >> 16)) & 0xFFu) / 255.0f;
    }

    std::vector<uint8_t> paintSheet()
    {
        constexpr int W = 64, H = 32;
        std::vector<uint8_t> pixels(W * H * 4, 0);
        auto put = [&](int x, int y, int r, int g, int b, int a) {
            uint8_t* p = &pixels[(y * W + x) * 4];
            p[0] = static_cast<uint8_t>(std::clamp(r, 0, 255));
            p[1] = static_cast<uint8_t>(std::clamp(g, 0, 255));
            p[2] = static_cast<uint8_t>(std::clamp(b, 0, 255));
            p[3] = static_cast<uint8_t>(std::clamp(a, 0, 255));
        };
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 32; ++x)
            {
                const int fx = x % 8, fy = y % 8;
                const bool rim = fx == 0 || fx == 7 || fy == 0 || fy == 7;
                if (rim) put(x, y, 214, 196, 236, 255);
                else if ((fx + fy) % 5 == 0) put(x, y, 240, 232, 250, 150);
            }
        for (int y = 0; y < 16; ++y)
            for (int x = 32; x < 64; ++x)
            {
                const int n = static_cast<int>(grain(x, y) * 60.0f);
                put(x, y, 196 + n / 2, 84 + n, 196 + n / 2, 255);
            }
        for (int y = 16; y < 32; ++y)
            for (int x = 0; x < 48; ++x)
            {
                const int n = static_cast<int>(grain(x, y) * 70.0f);
                put(x, y, 60 + n, 60 + n, 62 + n, 255);
            }
        return pixels;
    }

    glm::mat4 turnAboutDiagonal()
    {
        const float s45 = std::sin(3.14159265f / 4.0f);
        return glm::rotate(glm::mat4(1.0f), 3.14159265f / 3.0f, glm::normalize(glm::vec3(s45, 0.0f, s45)));
    }
}

EndCrystalRenderer::~EndCrystalRenderer()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

float EndCrystalRenderer::yOffset(float age)
{
    float g = std::sin(age * 0.2f) / 2.0f + 0.5f;
    g = (g * g + g) * 0.4f;
    return g - 1.4f;
}

void EndCrystalRenderer::load()
{
    m_loaded = true;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load("assets/textures/entity/end_crystal/end_crystal.png", &width, &height, &channels, 4);
    std::vector<uint8_t> painted;
    if (!data || width * 32 != height * 64)
    {
        if (data) stbi_image_free(data);
        data = nullptr;
        painted = paintSheet();
        width = 64;
        height = 32;
    }
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data ? data : painted.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (data) stbi_image_free(data);
}

void EndCrystalRenderer::cuboid(const glm::mat4& transform, glm::vec3 from, glm::vec3 size, int u, int v, float light)
{
    const glm::vec3 lo = from / 16.0f;
    const glm::vec3 hi = (from + size) / 16.0f;
    const glm::vec3 p[8] = { { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z }, { hi.x, hi.y, lo.z }, { lo.x, hi.y, lo.z },
                             { lo.x, lo.y, hi.z }, { hi.x, lo.y, hi.z }, { hi.x, hi.y, hi.z }, { lo.x, hi.y, hi.z } };
    const float dx = size.x, dy = size.y, dz = size.z;
    const float j = static_cast<float>(u), k = u + dz, l = u + dz + dx, m = u + dz + dx + dx, n = u + dz + dx + dz, o = u + dz + dx + dz + dx;
    const float pv = static_cast<float>(v), q = v + dz, r = v + dz + dy;
    struct Face { int c[4]; float u1, v1, u2, v2; float shade; };
    const Face faces[6] = {
        { { 5, 4, 0, 1 }, k, pv, l, q, 0.5f },
        { { 2, 3, 7, 6 }, l, q, m, pv, 1.0f },
        { { 0, 4, 7, 3 }, j, q, k, r, 0.6f },
        { { 1, 0, 3, 2 }, k, q, l, r, 0.8f },
        { { 5, 1, 2, 6 }, l, q, n, r, 0.6f },
        { { 4, 5, 6, 7 }, n, q, o, r, 0.8f },
    };
    for (const Face& f : faces)
    {
        const float us[4] = { f.u2, f.u1, f.u1, f.u2 };
        const float vs[4] = { f.v1, f.v1, f.v2, f.v2 };
        glm::vec3 w[4];
        for (int c = 0; c < 4; ++c) w[c] = glm::vec3(transform * glm::vec4(p[f.c[c]], 1.0f));
        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        for (int idx : order)
        {
            m_vertices.push_back(w[idx].x);
            m_vertices.push_back(w[idx].y);
            m_vertices.push_back(w[idx].z);
            m_vertices.push_back(us[idx] / SHEET_W);
            m_vertices.push_back(vs[idx] / SHEET_H);
            m_vertices.push_back(f.shade);
            m_vertices.push_back(light);
            m_vertices.push_back(light);
            m_vertices.push_back(6.0f);
        }
    }
}

void EndCrystalRenderer::render(Shader& shader, const World& world, const std::vector<glm::ivec3>& crystals, float ticks)
{
    if (crystals.empty()) return;
    if (!m_loaded) load();
    if (!m_texture) return;

    m_vertices.clear();
    const glm::mat4 diagonal = turnAboutDiagonal();
    for (const glm::ivec3& at : crystals)
    {
        uint32_t seed = static_cast<uint32_t>(at.x) * 73856093u ^ static_cast<uint32_t>(at.y) * 19349663u ^ static_cast<uint32_t>(at.z) * 83492791u;
        seed = (seed ^ (seed >> 15)) * 2246822519u;
        const float age = static_cast<float>(seed % 100000u) + ticks;
        const float h = yOffset(age);
        const float spin = age * 3.0f;
        const float sky = static_cast<float>(world.skyLight(at.x, at.y, at.z)) / 15.0f;
        const float light = std::max(sky, static_cast<float>(world.blockLightAt(at.x, at.y, at.z)) / 15.0f);

        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(at.x + 0.5f, static_cast<float>(at.y), at.z + 0.5f));
        m = glm::scale(m, glm::vec3(2.0f));
        m = glm::translate(m, glm::vec3(0.0f, -0.5f, 0.0f));
        cuboid(m, { -6.0f, 0.0f, -6.0f }, { 12.0f, 4.0f, 12.0f }, 0, 16, light);
        m = glm::rotate(m, glm::radians(spin), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::translate(m, glm::vec3(0.0f, 1.5f + h / 2.0f, 0.0f));
        m = m * diagonal;
        cuboid(m, { -4.0f, -4.0f, -4.0f }, { 8.0f, 8.0f, 8.0f }, 0, 0, light);
        m = glm::scale(m, glm::vec3(0.875f));
        m = m * diagonal;
        m = glm::rotate(m, glm::radians(spin), glm::vec3(0.0f, 1.0f, 0.0f));
        cuboid(m, { -4.0f, -4.0f, -4.0f }, { 8.0f, 8.0f, 8.0f }, 0, 0, light);
        m = glm::scale(m, glm::vec3(0.875f));
        m = m * diagonal;
        m = glm::rotate(m, glm::radians(spin), glm::vec3(0.0f, 1.0f, 0.0f));
        cuboid(m, { -4.0f, -4.0f, -4.0f }, { 8.0f, 8.0f, 8.0f }, 32, 0, light);
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
