#include "FireworkRenderer.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>

void FireworkRenderer::spark(const glm::vec3& at, float size, unsigned int rgb, float alpha)
{
    const unsigned int a = static_cast<unsigned int>(std::clamp(alpha, 0.0f, 1.0f) * 7.0f + 0.5f);
    const unsigned int key = ((rgb >> 4) & 0x0F0F0Fu) | (a << 24);
    m_batches[key].push_back(Spark{ at, size });
}

void FireworkRenderer::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraRight,
                              const glm::vec3& cameraUp)
{
    if (m_batches.empty()) return;
    if (!m_shader) m_shader = std::make_unique<Shader>("assets/shaders/line.vert", "assets/shaders/line.frag");
    m_shader->bind();
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", projection);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    for (const auto& batch : m_batches)
    {
        m_vertices.clear();
        for (const Spark& spark : batch.second)
        {
            const glm::vec3 r = cameraRight * spark.size;
            const glm::vec3 u = cameraUp * spark.size;
            const glm::vec3 corners[4] = { spark.at - r - u, spark.at + r - u, spark.at + r + u, spark.at - r + u };
            const int order[6] = { 0, 1, 2, 0, 2, 3 };
            for (int k : order)
            {
                m_vertices.push_back(corners[k].x);
                m_vertices.push_back(corners[k].y);
                m_vertices.push_back(corners[k].z);
            }
        }
        const unsigned int key = batch.first;
        const float red = static_cast<float>((key >> 16) & 0xFu) / 15.0f;
        const float green = static_cast<float>((key >> 8) & 0xFu) / 15.0f;
        const float blue = static_cast<float>(key & 0xFu) / 15.0f;
        const float alpha = static_cast<float>((key >> 24) & 0x7u) / 7.0f;
        m_shader->setVec4("uColor", glm::vec4(red, green, blue, alpha));
        m_mesh.upload(m_vertices, { 3 }, true);
        m_mesh.draw(GL_TRIANGLES);
    }
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_batches.clear();
}
