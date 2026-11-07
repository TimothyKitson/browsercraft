#include "FangRenderer.h"
#include "Core/GLFunctions.h"
#include "Entity/BoxMesh.h"
#include "Shader.h"
#include "World/World.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "Core/Packs.h"

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };
    constexpr float PI = 3.14159265f;

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
        auto put = [&](int x, int y, int r, int g, int b) {
            uint8_t* p = &pixels[(y * W + x) * 4];
            p[0] = static_cast<uint8_t>(std::clamp(r, 0, 255));
            p[1] = static_cast<uint8_t>(std::clamp(g, 0, 255));
            p[2] = static_cast<uint8_t>(std::clamp(b, 0, 255));
            p[3] = 255;
        };
        for (int y = 0; y < 22; ++y)
            for (int x = 0; x < 40; ++x)
            {
                const int n = static_cast<int>(grain(x, y) * 30.0f);
                put(x, y, 92 + n, 96 + n, 88 + n);
            }
        for (int y = 0; y < 22; ++y)
            for (int x = 40; x < 64; ++x)
            {
                const int n = static_cast<int>(grain(x, y) * 24.0f);
                const bool tooth = y > 8 && ((x + y / 3) % 3 != 0);
                if (tooth) put(x, y, 222 + n / 2, 214 + n / 2, 188 + n / 2);
                else put(x, y, 104 + n, 110 + n, 98 + n);
            }
        return pixels;
    }

    BoxMesh::Box box(glm::vec3 min, glm::vec3 size, glm::vec3 pivot, int u, int v, float yaw, float roll)
    {
        BoxMesh::Box out;
        out.min = min;
        out.max = min + size;
        out.pivot = pivot;
        out.u = u;
        out.v = v;
        out.yaw = yaw;
        out.roll = roll;
        return out;
    }
}

FangRenderer::~FangRenderer()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

void FangRenderer::load()
{
    m_loaded = true;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load("assets/textures/entity/illager/evoker_fangs.png", &width, &height, &channels, 4);
    std::vector<uint8_t> painted;
    if (!data || width != 64 || height != 32)
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

void FangRenderer::render(Shader& shader, const World& world, const std::vector<Fangs::Fang>& fangs, float partial)
{
    if (fangs.empty()) return;
    if (!m_loaded) load();
    if (!m_texture) return;

    m_vertices.clear();
    for (const Fangs::Fang& fang : fangs)
    {
        const float h = Fangs::progress(fang, partial);
        if (h <= 0.0f) continue;

        float shrink = 1.0f;
        if (h > 0.9f) shrink = (1.0f - h) / 0.1f;
        if (shrink <= 0.0f) continue;

        float open = std::min(h * 2.0f, 1.0f);
        open = 1.0f - open * open * open;
        const float swing = open * 0.35f * PI;
        const float rise = (h + std::sin(h * 2.7f)) * 0.6f * 12.0f;
        const float level = 20.032f - (24.0f - rise);

        std::vector<BoxMesh::Box> parts;
        parts.push_back(box({ -5.0f, level - 12.0f, -5.0f }, { 10.0f, 12.0f, 10.0f }, { 0.0f, level, 0.0f }, 0, 0, 0.0f, 0.0f));
        const glm::vec3 upper(-1.5f, level, -4.0f);
        parts.push_back(box(upper, { 4.0f, 14.0f, 8.0f }, upper, 40, 0, 0.0f, -swing));
        const glm::vec3 lower(1.5f, level, 4.0f);
        parts.push_back(box(lower, { 4.0f, 14.0f, 8.0f }, lower, 40, 0, PI, swing));

        const int lx = static_cast<int>(std::floor(fang.at.x));
        const int ly = static_cast<int>(std::floor(fang.at.y + 0.5f));
        const int lz = static_cast<int>(std::floor(fang.at.z));

        BoxMesh::Frame frame;
        frame.feet = fang.at;
        frame.bodyYaw = 90.0f - glm::degrees(fang.yaw);
        frame.textureWidth = 64.0f;
        frame.textureHeight = 32.0f;
        frame.scale = shrink;
        frame.sky = static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f;
        frame.blockLight = std::max(static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f, 0.2f);
        BoxMesh::appendAll(m_vertices, parts, frame);
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
