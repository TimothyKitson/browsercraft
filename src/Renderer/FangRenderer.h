#pragma once
#include "Mesh.h"
#include "Game/Fangs.h"
#include <vector>

class Shader;
class World;

class FangRenderer
{
public:
    ~FangRenderer();
    void render(Shader& shader, const World& world, const std::vector<Fangs::Fang>& fangs, float partial);

private:
    void load();

    unsigned int m_texture = 0;
    bool m_loaded = false;
    Mesh m_mesh;
    std::vector<float> m_vertices;
};
