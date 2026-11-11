#pragma once
#include "Shader.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <vector>

class EntityShadowRenderer
{
public:
    struct Spot
    {
        glm::vec3 centre{ 0.0f };
        float radius = 0.5f;
        float alpha = 0.5f;
    };

    EntityShadowRenderer();
    ~EntityShadowRenderer();

    EntityShadowRenderer(const EntityShadowRenderer&) = delete;
    EntityShadowRenderer& operator=(const EntityShadowRenderer&) = delete;

    static std::vector<float> build(const std::vector<Spot>& spots);
    void render(const glm::mat4& view, const glm::mat4& projection, const std::vector<Spot>& spots);

private:
    Shader m_shader;
    Mesh m_mesh;
    unsigned int m_texture = 0;
    bool m_loaded = false;

    void load();
};
