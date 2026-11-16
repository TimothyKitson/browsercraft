#pragma once
#include "Mesh.h"
#include <glm/glm.hpp>
#include <vector>

class Shader;
class World;

class EndCrystalRenderer
{
public:
    ~EndCrystalRenderer();
    void render(Shader& shader, const World& world, const std::vector<glm::ivec3>& crystals, float ticks);
    static float yOffset(float age);

private:
    void load();
    void cuboid(const glm::mat4& transform, glm::vec3 from, glm::vec3 size, int u, int v, float light);

    unsigned int m_texture = 0;
    bool m_loaded = false;
    Mesh m_mesh;
    std::vector<float> m_vertices;
};
