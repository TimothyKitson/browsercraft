#pragma once
#include "Mesh.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <vector>

class FireworkRenderer
{
public:
    void clear() { m_batches.clear(); }
    void spark(const glm::vec3& at, float size, unsigned int rgb, float alpha);
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraRight, const glm::vec3& cameraUp);
    bool empty() const { return m_batches.empty(); }

private:
    struct Spark
    {
        glm::vec3 at;
        float size;
    };
    std::map<unsigned int, std::vector<Spark>> m_batches;
    std::unique_ptr<Shader> m_shader;
    Mesh m_mesh;
    std::vector<float> m_vertices;
};
