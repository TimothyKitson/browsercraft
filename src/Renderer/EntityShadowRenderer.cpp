#include "EntityShadowRenderer.h"
#include "Core/GLFunctions.h"
#include "Core/Packs.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>

namespace
{
    const std::vector<int> LAYOUT = { 3, 2 };
    constexpr int PAINTED = 16;
}

EntityShadowRenderer::EntityShadowRenderer()
    : m_shader("assets/shaders/border.vert", "assets/shaders/border.frag")
{
}

EntityShadowRenderer::~EntityShadowRenderer()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

void EntityShadowRenderer::load()
{
    m_loaded = true;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load("assets/textures/misc/shadow.png", &width, &height, &channels, 4);
    std::vector<unsigned char> painted;
    if (!data)
    {
        width = height = PAINTED;
        painted.assign(static_cast<size_t>(PAINTED * PAINTED * 4), 0);
        for (int y = 0; y < PAINTED; ++y)
            for (int x = 0; x < PAINTED; ++x)
            {
                const float dx = (static_cast<float>(x) + 0.5f) / PAINTED * 2.0f - 1.0f;
                const float dy = (static_cast<float>(y) + 0.5f) / PAINTED * 2.0f - 1.0f;
                const float edge = std::sqrt(dx * dx + dy * dy);
                unsigned char* p = &painted[static_cast<size_t>((y * PAINTED + x) * 4)];
                p[0] = p[1] = p[2] = 0;
                p[3] = edge >= 1.0f ? 0 : static_cast<unsigned char>(std::lround(std::min(1.0f, (1.0f - edge) * 2.5f) * 200.0f));
            }
    }
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data ? data : painted.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (data) stbi_image_free(data);
}

std::vector<float> EntityShadowRenderer::build(const std::vector<Spot>& spots)
{
    std::vector<float> out;
    out.reserve(spots.size() * 30);
    for (const Spot& spot : spots)
    {
        if (spot.alpha <= 0.0f || spot.radius <= 0.0f) continue;
        const float x0 = spot.centre.x - spot.radius, x1 = spot.centre.x + spot.radius;
        const float z0 = spot.centre.z - spot.radius, z1 = spot.centre.z + spot.radius;
        const float y = spot.centre.y + 0.002f;
        const float corners[6][5] = { { x0, y, z0, 0.0f, 0.0f }, { x1, y, z0, 1.0f, 0.0f }, { x1, y, z1, 1.0f, 1.0f },
                                      { x0, y, z0, 0.0f, 0.0f }, { x1, y, z1, 1.0f, 1.0f }, { x0, y, z1, 0.0f, 1.0f } };
        for (const auto& c : corners)
        {
            out.push_back(c[0]);
            out.push_back(c[1]);
            out.push_back(c[2]);
            out.push_back(c[3]);
            out.push_back(c[4]);
        }
    }
    return out;
}

void EntityShadowRenderer::render(const glm::mat4& view, const glm::mat4& projection, const std::vector<Spot>& spots)
{
    if (spots.empty()) return;
    if (!m_loaded) load();
    if (!m_texture) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    m_shader.bind();
    m_shader.setMat4("uView", view);
    m_shader.setMat4("uProjection", projection);
    m_shader.setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    for (const Spot& spot : spots)
    {
        const std::vector<float> one = build({ spot });
        if (one.empty()) continue;
        m_shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, std::clamp(spot.alpha, 0.0f, 1.0f)));
        m_mesh.upload(one, LAYOUT, true);
        m_mesh.draw(GL_TRIANGLES);
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
