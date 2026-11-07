#pragma once
#include "Shader.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <vector>

class WorldBorder;

class BorderRenderer
{
public:
    BorderRenderer();
    ~BorderRenderer();

    BorderRenderer(const BorderRenderer&) = delete;
    BorderRenderer& operator=(const BorderRenderer&) = delete;

    static float alphaFor(const WorldBorder& border, double x, double z, double reach);
    static std::vector<float> build(const WorldBorder& border, const glm::dvec3& camera, double reach, double height, float scroll);

    void render(const glm::mat4& view, const glm::mat4& projection, const WorldBorder& border, const glm::dvec3& camera,
                double reach, double height, float scroll);
    bool fromPack() const { return m_fromPack; }

private:
    Shader m_shader;
    Mesh m_mesh;
    unsigned int m_texture = 0;
    bool m_loaded = false;
    bool m_fromPack = false;

    void load();
};
