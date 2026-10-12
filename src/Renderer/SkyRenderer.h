#pragma once
#include "Shader.h"
#include "Mesh.h"
#include <glm/glm.hpp>

// Draws the sky as a single full-screen triangle: the fragment shader turns
// each pixel back into a world-space view ray and shades it analytically
// (gradient, sun, moon, stars, drifting clouds). No skybox texture needed.
class SkyRenderer
{
public:
    SkyRenderer();

    // `overcast` is 0 under a clear sky and 1 in full rain.
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPosition, const glm::vec3& sunDirection,
                float dayFactor, float timeSeconds, float overcast = 0.0f);

    // Shared with the chunk shader so distance fog matches the horizon.
    static glm::vec3 horizonColor(float dayFactor, const glm::vec3& sunDirection,
                                  float overcast = 0.0f);

private:
    Shader m_shader;
    Mesh m_mesh;
};
