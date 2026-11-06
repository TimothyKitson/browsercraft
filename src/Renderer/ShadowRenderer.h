#pragma once
#include "Mesh.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Shader;
class World;

class ShadowRenderer
{
public:
    ShadowRenderer();
    ~ShadowRenderer();

    void clear() { m_vertices.clear(); }
    void cast(const World& world, const glm::vec3& feet, float radius);
    void render(const glm::mat4& view, const glm::mat4& projection);
    size_t quads() const { return m_vertices.size() / 36; }

private:
    std::unique_ptr<Shader> m_shader;
    unsigned int m_texture = 0;
    bool m_loaded = false;
    std::vector<float> m_vertices;
    Mesh m_mesh;
};
