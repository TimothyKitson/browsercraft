#include "SkyRenderer.h"
#include "Core/GLFunctions.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>

SkyRenderer::SkyRenderer()
    : m_shader("assets/shaders/sky.vert", "assets/shaders/sky.frag")
{
    // One oversized triangle covering the whole screen.
    const std::vector<float> vertices = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };
    m_mesh.upload(vertices, { 2 });
}

glm::vec3 SkyRenderer::horizonColor(float dayFactor, const glm::vec3& sunDirection)
{
    const glm::vec3 dayHorizon(0.70f, 0.84f, 0.98f);
    const glm::vec3 nightHorizon(0.06f, 0.08f, 0.17f);
    glm::vec3 color = glm::mix(nightHorizon, dayHorizon, dayFactor);

    // Warm the horizon while the sun is near it.
    const float sunset = std::clamp(1.0f - std::fabs(sunDirection.y) * 4.0f, 0.0f, 1.0f) * dayFactor;
    return glm::mix(color, glm::vec3(0.96f, 0.56f, 0.28f), sunset * 0.75f);
}

void SkyRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                         const glm::vec3& cameraPosition, const glm::vec3& sunDirection,
                         float dayFactor, float timeSeconds)
{
    // Strip the translation: only the view direction matters for the sky.
    glm::mat4 rotationOnlyView = view;
    rotationOnlyView[3][0] = 0.0f;
    rotationOnlyView[3][1] = 0.0f;
    rotationOnlyView[3][2] = 0.0f;

    const glm::mat4 invViewProj = glm::inverse(projection * rotationOnlyView);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    m_shader.bind();
    m_shader.setMat4("uInvViewProj", invViewProj);
    m_shader.setVec3("uSunDirection", glm::normalize(sunDirection));
    m_shader.setVec3("uCameraPos", cameraPosition);
    m_shader.setFloat("uDayFactor", dayFactor);
    m_shader.setFloat("uTime", timeSeconds);
    m_mesh.draw(GL_TRIANGLES);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}
