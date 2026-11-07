#include "BeamRenderer.h"
#include "Core/GLFunctions.h"
#include "Shader.h"
#include <stb_image.h>
#include <cmath>
#include <string>
#include <algorithm>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    struct Source
    {
        const char* path;
        unsigned int tint;
        float alpha;
    };

    const Source SOURCES[] = {
        { "assets/textures/entity/beacon_beam.png", 0xFFFFFF, 1.0f },
        { "assets/textures/entity/beacon_beam.png", 0xFFFFFF, 0.125f },
        { "assets/textures/entity/end_gateway_beam.png", 0xC090FF, 1.0f },
        { "assets/textures/entity/end_crystal/end_crystal_beam.png", 0xFFFFFF, 1.0f },
        { "assets/textures/entity/guardian_beam.png", 0x402080, 1.0f },
        { "assets/textures/entity/guardian_beam.png", 0x9C6E70, 1.0f },
        { "assets/textures/entity/guardian_beam.png", 0xE0B050, 1.0f },
        { "assets/textures/entity/guardian_beam.png", 0xFFE040, 1.0f },
        { "assets/textures/entity/beacon_beam.png", 0xE8C8FF, 0.6f },
        { nullptr, 0xFFFFFF, 1.0f },
        { nullptr, 0x262626, 1.0f },
        { nullptr, 0x6E4C2E, 1.0f },
    };

    unsigned int whiteTexture(unsigned int tint)
    {
        const unsigned char WHITE[4] = { static_cast<unsigned char>((tint >> 16) & 255u), static_cast<unsigned char>((tint >> 8) & 255u),
                                         static_cast<unsigned char>(tint & 255u), 255 };
        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, WHITE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return texture;
    }
}

BeamRenderer::~BeamRenderer()
{
    for (unsigned int texture : m_textures)
        if (texture) glDeleteTextures(1, &texture);
}

void BeamRenderer::load()
{
    m_loaded = true;
    for (int i = 0; i < static_cast<int>(Look::Count); ++i)
    {
        const Source& source = SOURCES[i];
        int width = 0, height = 0, channels = 0;
        unsigned char* data = source.path ? stbi_load(source.path, &width, &height, &channels, 4) : nullptr;
        if (!data)
        {
            m_textures[i] = whiteTexture(source.tint);
            continue;
        }
        const unsigned int r = (source.tint >> 16) & 0xFF, g = (source.tint >> 8) & 0xFF, b = source.tint & 0xFF;
        for (int p = 0; p < width * height; ++p)
        {
            data[p * 4 + 0] = static_cast<unsigned char>(data[p * 4 + 0] * r / 255);
            data[p * 4 + 1] = static_cast<unsigned char>(data[p * 4 + 1] * g / 255);
            data[p * 4 + 2] = static_cast<unsigned char>(data[p * 4 + 2] * b / 255);
            data[p * 4 + 3] = static_cast<unsigned char>(std::max(26.0f, data[p * 4 + 3] * source.alpha));
        }
        glGenTextures(1, &m_textures[i]);
        glBindTexture(GL_TEXTURE_2D, m_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stbi_image_free(data);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BeamRenderer::clear()
{
    for (auto& batch : m_batches) batch.clear();
}

bool BeamRenderer::empty() const
{
    for (const auto& batch : m_batches)
        if (!batch.empty()) return false;
    return true;
}

void BeamRenderer::beam(Look look, const glm::vec3& from, const glm::vec3& to, float radius, float scroll, float spin)
{
    const glm::vec3 span = to - from;
    const float length = glm::length(span);
    if (length < 0.001f) return;
    const glm::vec3 axis = span / length;
    const glm::vec3 reference = std::abs(axis.y) > 0.95f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 u = glm::normalize(glm::cross(axis, reference));
    glm::vec3 v = glm::normalize(glm::cross(axis, u));
    const float c = std::cos(spin), s = std::sin(spin);
    const glm::vec3 su = u * c + v * s;
    const glm::vec3 sv = v * c - u * s;

    const glm::vec3 corner[4] = {
        (su + sv) * radius, (su - sv) * radius, (-su - sv) * radius, (-su + sv) * radius,
    };

    std::vector<float>& out = m_batches[static_cast<int>(look)];
    auto push = [&](const glm::vec3& p, float tu, float tv) {
        out.push_back(p.x);
        out.push_back(p.y);
        out.push_back(p.z);
        out.push_back(tu);
        out.push_back(tv);
        out.push_back(1.0f);
        out.push_back(0.0f);
        out.push_back(1.0f);
        out.push_back(6.0f);
    };

    const float v0 = -scroll;
    const float v1 = length - scroll;
    for (int side = 0; side < 4; ++side)
    {
        const glm::vec3 a = corner[side];
        const glm::vec3 b = corner[(side + 1) & 3];
        const glm::vec3 a0 = from + a, b0 = from + b, a1 = to + a, b1 = to + b;
        push(a0, 0.0f, v1);
        push(b0, 1.0f, v1);
        push(b1, 1.0f, v0);
        push(a0, 0.0f, v1);
        push(b1, 1.0f, v0);
        push(a1, 0.0f, v0);
    }
}

void BeamRenderer::render(Shader& shader)
{
    if (empty()) return;
    if (!m_loaded) load();

    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);

    for (int pass = 0; pass < 2; ++pass)
    {
        if (pass == 1)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        for (int i = 0; i < static_cast<int>(Look::Count); ++i)
        {
            const bool haze = static_cast<Look>(i) == Look::BeaconHaze || static_cast<Look>(i) == Look::Ray;
            if (haze != (pass == 1)) continue;
            if (m_batches[i].empty()) continue;
            m_mesh.upload(m_batches[i], CHUNK_LAYOUT, true);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_textures[i]);
            m_mesh.draw(GL_TRIANGLES);
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}
