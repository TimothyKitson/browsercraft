#include "PaintingRenderer.h"
#include "Core/GLFunctions.h"
#include "Shader.h"
#include "World/World.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include "Core/Packs.h"

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    uint32_t mix(uint32_t h)
    {
        h ^= h >> 16;
        h *= 0x7feb352du;
        h ^= h >> 15;
        h *= 0x846ca68bu;
        h ^= h >> 16;
        return h;
    }

    unsigned int upload(const uint8_t* pixels, int width, int height)
    {
        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return texture;
    }

    void quad(std::vector<float>& out, const glm::vec3 corner[4], const float u[4], const float v[4],
              float shade, float sky, float block)
    {
        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        for (int k : order)
        {
            out.push_back(corner[k].x);
            out.push_back(corner[k].y);
            out.push_back(corner[k].z);
            out.push_back(u[k]);
            out.push_back(v[k]);
            out.push_back(shade);
            out.push_back(sky);
            out.push_back(block);
            out.push_back(6.0f);
        }
    }
}

PaintingRenderer::~PaintingRenderer()
{
    for (unsigned int texture : m_textures)
        if (texture) glDeleteTextures(1, &texture);
    if (m_back) glDeleteTextures(1, &m_back);
}

std::vector<uint8_t> PaintingRenderer::paintVariant(int variant, int& width, int& height)
{
    const Paintings::Variant& v = Paintings::variants()[static_cast<size_t>(std::clamp(variant, 0, Paintings::count() - 1))];
    width = v.width * 16;
    height = v.height * 16;
    std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 4), 255);
    uint32_t seed = 2166136261u;
    for (const char* c = v.name; *c; ++c) seed = (seed ^ static_cast<uint8_t>(*c)) * 16777619u;
    auto hue = [&](uint32_t salt) {
        const uint32_t h = mix(seed ^ salt);
        return glm::vec3(static_cast<float>(h & 0xFFu), static_cast<float>((h >> 8) & 0xFFu),
                         static_cast<float>((h >> 16) & 0xFFu)) / 255.0f;
    };
    const glm::vec3 sky = glm::mix(hue(1u), glm::vec3(0.55f, 0.7f, 0.85f), 0.5f);
    const glm::vec3 ground = glm::mix(hue(2u), glm::vec3(0.35f, 0.45f, 0.25f), 0.5f);
    const glm::vec3 subject = hue(3u);
    const float horizon = 0.45f + static_cast<float>(mix(seed ^ 4u) % 30u) / 100.0f;
    const glm::vec2 centre(0.3f + static_cast<float>(mix(seed ^ 5u) % 40u) / 100.0f,
                           0.3f + static_cast<float>(mix(seed ^ 6u) % 30u) / 100.0f);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            const float fx = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            const float fy = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
            glm::vec3 colour = fy < horizon ? glm::mix(sky, sky * 0.7f, fy) : ground * (0.8f + 0.2f * fy);
            const glm::vec2 d((fx - centre.x) * static_cast<float>(v.width), (fy - centre.y) * static_cast<float>(v.height));
            if (glm::length(d) < 0.32f * static_cast<float>(std::min(v.width, v.height))) colour = subject;
            const float grain = static_cast<float>(mix(seed ^ static_cast<uint32_t>(x * 977 + y * 131)) % 100u) / 1000.0f;
            colour = glm::clamp(colour * (0.95f + grain), glm::vec3(0.0f), glm::vec3(1.0f));
            const bool frame = x == 0 || y == 0 || x == width - 1 || y == height - 1;
            if (frame) colour = glm::vec3(0.36f, 0.25f, 0.14f);
            uint8_t* p = &pixels[static_cast<size_t>((y * width + x) * 4)];
            p[0] = static_cast<uint8_t>(colour.r * 255.0f);
            p[1] = static_cast<uint8_t>(colour.g * 255.0f);
            p[2] = static_cast<uint8_t>(colour.b * 255.0f);
            p[3] = 255;
        }
    return pixels;
}

std::vector<uint8_t> PaintingRenderer::paintBack()
{
    std::vector<uint8_t> pixels(16 * 16 * 4, 255);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            const int n = static_cast<int>(mix(static_cast<uint32_t>(x * 31 + y * 7)) % 18u);
            const bool seam = x % 4 == 0;
            uint8_t* p = &pixels[static_cast<size_t>((y * 16 + x) * 4)];
            p[0] = static_cast<uint8_t>((seam ? 120 : 154) + n);
            p[1] = static_cast<uint8_t>((seam ? 88 : 116) + n);
            p[2] = static_cast<uint8_t>((seam ? 52 : 72) + n / 2);
        }
    return pixels;
}

unsigned int PaintingRenderer::textureFor(int variant)
{
    if (variant < 0 || variant >= Paintings::count()) return 0;
    if (m_textures.empty())
    {
        m_textures.assign(static_cast<size_t>(Paintings::count()), 0u);
        m_tried.assign(static_cast<size_t>(Paintings::count()), false);
    }
    const size_t i = static_cast<size_t>(variant);
    if (m_tried[i]) return m_textures[i];
    m_tried[i] = true;

    const Paintings::Variant& v = Paintings::variants()[i];
    int width = 0, height = 0, channels = 0;
    const std::string path = Paintings::texturePath(variant);
    unsigned char* data = Packs::load(path.c_str(), &width, &height, &channels, 4);
    if (data && width > 0 && height > 0 && width * v.height == height * v.width)
        m_textures[i] = upload(data, width, height);
    else
    {
        const std::vector<uint8_t> painted = paintVariant(variant, width, height);
        m_textures[i] = upload(painted.data(), width, height);
    }
    if (data) stbi_image_free(data);
    return m_textures[i];
}

unsigned int PaintingRenderer::backTexture()
{
    if (m_backTried) return m_back;
    m_backTried = true;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load("assets/textures/painting/back.png", &width, &height, &channels, 4);
    if (data && width > 0 && width == height) m_back = upload(data, width, height);
    else
    {
        const std::vector<uint8_t> painted = paintBack();
        m_back = upload(painted.data(), 16, 16);
    }
    if (data) stbi_image_free(data);
    return m_back;
}

void PaintingRenderer::draw(unsigned int texture, std::vector<float>& vertices)
{
    if (vertices.empty() || !texture) return;
    m_mesh.upload(vertices, CHUNK_LAYOUT, true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    m_mesh.draw(GL_TRIANGLES);
}

void PaintingRenderer::render(Shader& shader, const World& world, const std::vector<Paintings::Placed>& paintings)
{
    if (paintings.empty()) return;

    constexpr float WALL = 0.5f;
    constexpr float FACE = 0.4375f;
    constexpr float EDGE = 1.0f / 16.0f;

    std::vector<float> backs;
    std::vector<std::vector<float>> fronts(static_cast<size_t>(Paintings::count()));

    for (const Paintings::Placed& painting : paintings)
    {
        if (painting.variant < 0 || painting.variant >= Paintings::count()) continue;
        const Paintings::Variant& v = Paintings::variants()[static_cast<size_t>(painting.variant)];
        const glm::vec3 o(Paintings::out(painting.facing));
        const glm::vec3 r(Paintings::right(painting.facing));
        const glm::vec3 up(0.0f, 1.0f, 0.0f);
        const int kMin = -((v.width - 1) / 2), kMax = v.width / 2;
        const int jMin = -((v.height - 1) / 2), jMax = v.height / 2;
        const float frontShade = std::fabs(o.x) > 0.5f ? 0.8f : 0.9f;

        for (int k = kMin; k <= kMax; ++k)
            for (int j = jMin; j <= jMax; ++j)
            {
                const glm::ivec3 cell = painting.at + Paintings::right(painting.facing) * k + glm::ivec3(0, j, 0);
                const float sky = static_cast<float>(world.skyLight(cell.x, cell.y, cell.z)) / 15.0f;
                const float block = static_cast<float>(world.blockLightAt(cell.x, cell.y, cell.z)) / 15.0f;
                const glm::vec3 centre = glm::vec3(cell) + glm::vec3(0.5f);
                const glm::vec3 front = centre - o * FACE;
                const glm::vec3 back = centre - o * WALL;

                const float u0 = static_cast<float>(k - kMin) / static_cast<float>(v.width);
                const float u1 = static_cast<float>(k - kMin + 1) / static_cast<float>(v.width);
                const float v0 = static_cast<float>(jMax - j) / static_cast<float>(v.height);
                const float v1 = static_cast<float>(jMax - j + 1) / static_cast<float>(v.height);

                {
                    const glm::vec3 c[4] = { front - r * 0.5f - up * 0.5f, front + r * 0.5f - up * 0.5f,
                                             front + r * 0.5f + up * 0.5f, front - r * 0.5f + up * 0.5f };
                    const float u[4] = { u0, u1, u1, u0 };
                    const float vv[4] = { v1, v1, v0, v0 };
                    quad(fronts[static_cast<size_t>(painting.variant)], c, u, vv, frontShade, sky, block);
                }
                {
                    const glm::vec3 c[4] = { back + r * 0.5f - up * 0.5f, back - r * 0.5f - up * 0.5f,
                                             back - r * 0.5f + up * 0.5f, back + r * 0.5f + up * 0.5f };
                    const float u[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
                    const float vv[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
                    quad(backs, c, u, vv, frontShade * 0.8f, sky, block);
                }
                if (j == jMax)
                {
                    const glm::vec3 top = up * 0.5f;
                    const glm::vec3 c[4] = { back - r * 0.5f + top, back + r * 0.5f + top, front + r * 0.5f + top, front - r * 0.5f + top };
                    const float u[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
                    const float vv[4] = { EDGE, EDGE, 0.0f, 0.0f };
                    quad(backs, c, u, vv, 1.0f, sky, block);
                }
                if (j == jMin)
                {
                    const glm::vec3 bottom = -up * 0.5f;
                    const glm::vec3 c[4] = { front - r * 0.5f + bottom, front + r * 0.5f + bottom, back + r * 0.5f + bottom, back - r * 0.5f + bottom };
                    const float u[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
                    const float vv[4] = { EDGE, EDGE, 0.0f, 0.0f };
                    quad(backs, c, u, vv, 0.5f, sky, block);
                }
                if (k == kMin)
                {
                    const glm::vec3 side = r * -0.5f;
                    const glm::vec3 c[4] = { back + side - up * 0.5f, front + side - up * 0.5f, front + side + up * 0.5f, back + side + up * 0.5f };
                    const float u[4] = { 0.0f, EDGE, EDGE, 0.0f };
                    const float vv[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
                    quad(backs, c, u, vv, 0.7f, sky, block);
                }
                if (k == kMax)
                {
                    const glm::vec3 side = r * 0.5f;
                    const glm::vec3 c[4] = { back + side - up * 0.5f, front + side - up * 0.5f, front + side + up * 0.5f, back + side + up * 0.5f };
                    const float u[4] = { 0.0f, EDGE, EDGE, 0.0f };
                    const float vv[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
                    quad(backs, c, u, vv, 0.7f, sky, block);
                }
            }
    }

    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);
    draw(backTexture(), backs);
    for (int i = 0; i < Paintings::count(); ++i)
        if (!fronts[static_cast<size_t>(i)].empty()) draw(textureFor(i), fronts[static_cast<size_t>(i)]);
    glEnable(GL_CULL_FACE);
}
