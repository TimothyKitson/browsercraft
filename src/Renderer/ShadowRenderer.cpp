#include "ShadowRenderer.h"
#include "Core/GLFunctions.h"
#include "Shader.h"
#include "World/World.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include "Core/Packs.h"

namespace
{
    const std::vector<int> SHADOW_LAYOUT = { 3, 2, 1 };
    constexpr float LIFT = 0.015625f;
    constexpr float REACH = 2.0f;
}

ShadowRenderer::ShadowRenderer() = default;

ShadowRenderer::~ShadowRenderer()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

void ShadowRenderer::cast(const World& world, const glm::vec3& feet, float radius)
{
    const int x0 = static_cast<int>(std::floor(feet.x - radius));
    const int x1 = static_cast<int>(std::floor(feet.x + radius));
    const int z0 = static_cast<int>(std::floor(feet.z - radius));
    const int z1 = static_cast<int>(std::floor(feet.z + radius));
    const int yTop = static_cast<int>(std::floor(feet.y));
    const int yLow = static_cast<int>(std::floor(feet.y - REACH));

    for (int bx = x0; bx <= x1; ++bx)
        for (int bz = z0; bz <= z1; ++bz)
            for (int by = yTop; by >= yLow; --by)
            {
                if (!isOpaque(world.getBlock(bx, by, bz))) continue;
                if (isOpaque(world.getBlock(bx, by + 1, bz))) break;
                const float top = static_cast<float>(by + 1);
                const float drop = feet.y - top;
                if (drop < -0.01f || drop > REACH) break;
                const float alpha = (1.0f - drop / REACH) * 0.5f;
                if (alpha <= 0.01f) break;

                const float y = top + LIFT;
                const float xs[4] = { static_cast<float>(bx), static_cast<float>(bx + 1),
                                      static_cast<float>(bx + 1), static_cast<float>(bx) };
                const float zs[4] = { static_cast<float>(bz), static_cast<float>(bz),
                                      static_cast<float>(bz + 1), static_cast<float>(bz + 1) };
                const int order[6] = { 0, 2, 1, 0, 3, 2 };
                for (int k : order)
                {
                    m_vertices.push_back(xs[k]);
                    m_vertices.push_back(y);
                    m_vertices.push_back(zs[k]);
                    m_vertices.push_back(0.5f + (xs[k] - feet.x) / (2.0f * radius));
                    m_vertices.push_back(0.5f + (zs[k] - feet.z) / (2.0f * radius));
                    m_vertices.push_back(alpha);
                }
                break;
            }
}

void ShadowRenderer::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (m_vertices.empty()) return;
    if (!m_loaded)
    {
        m_loaded = true;
        m_shader = std::make_unique<Shader>("assets/shaders/shadow.vert", "assets/shaders/shadow.frag");
        int width = 0, height = 0, channels = 0;
        unsigned char* data = Packs::load("assets/textures/misc/shadow.png", &width, &height, &channels, 4);
        std::vector<unsigned char> painted;
        if (!data)
        {
            width = height = 32;
            painted.assign(static_cast<size_t>(width) * height * 4, 0);
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    const float dx = (x + 0.5f) / width - 0.5f, dy = (y + 0.5f) / height - 0.5f;
                    const float d = std::sqrt(dx * dx + dy * dy) * 2.0f;
                    painted[(static_cast<size_t>(y) * width + x) * 4 + 3] =
                        static_cast<unsigned char>(std::clamp(1.0f - d, 0.0f, 1.0f) * 255.0f);
                }
        }
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     data ? data : painted.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if (data) stbi_image_free(data);
    }
    if (!m_texture || !m_shader) return;

    m_shader->bind();
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", projection);
    m_shader->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    m_mesh.upload(m_vertices, SHADOW_LAYOUT, true);
    m_mesh.draw(GL_TRIANGLES);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
