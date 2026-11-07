#include "SkyRenderer.h"
#include "Core/GLFunctions.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include "Core/Packs.h"

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

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

    m_sunTexture = loadTexture("assets/textures/environment/sun.png", false);
    m_moonTexture = loadTexture("assets/textures/environment/moon_phases.png", false);
    m_cloudTexture = loadTexture("assets/textures/environment/clouds.png", true);
    m_endTexture = loadTexture("assets/textures/environment/end_sky.png", true);
}

SkyRenderer::~SkyRenderer()
{
    for (unsigned int texture : { m_sunTexture, m_moonTexture, m_cloudTexture, m_endTexture })
        if (texture) glDeleteTextures(1, &texture);
}

unsigned int SkyRenderer::loadTexture(const char* path, bool repeat)
{
    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load(path, &width, &height, &channels, 4);
    if (!data) return 0;
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return texture;
}

glm::vec3 SkyRenderer::horizonColor(float dayFactor, const glm::vec3& sunDirection,
                                    float overcast)
{
    const glm::vec3 dayHorizon(0.70f, 0.84f, 0.98f);
    const glm::vec3 nightHorizon(0.06f, 0.08f, 0.17f);
    glm::vec3 color = glm::mix(nightHorizon, dayHorizon, dayFactor);

    // The same grey the shader mixes towards, so the distance fog goes
    // on matching the horizon when it rains. The two are kept in step
    // by hand: a fog that stayed blue under a grey sky would draw a
    // line across the world at the render distance.
    const glm::vec3 wetHorizon = glm::mix(glm::vec3(0.13f, 0.14f, 0.16f),
                                          glm::vec3(0.62f, 0.64f, 0.67f), dayFactor);
    color = glm::mix(color, wetHorizon, std::clamp(overcast, 0.0f, 1.0f));

    // Warm the horizon while the sun is near it, which cloud puts a
    // stop to.
    const float sunset = std::clamp(1.0f - std::fabs(sunDirection.y) * 4.0f, 0.0f, 1.0f) *
                         dayFactor * (1.0f - std::clamp(overcast, 0.0f, 1.0f));
    return glm::mix(color, glm::vec3(0.96f, 0.56f, 0.28f), sunset * 0.75f);
}

void SkyRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                         const glm::vec3& cameraPosition, const glm::vec3& sunDirection,
                         float dayFactor, float timeSeconds, float overcast,
                         int moonPhase, bool endSky)
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
    m_shader.setFloat("uOvercast", std::clamp(overcast, 0.0f, 1.0f));
    m_shader.setInt("uMoonPhase", ((moonPhase % 8) + 8) % 8);
    m_shader.setInt("uEnd", endSky && m_endTexture ? 1 : 0);
    m_shader.setInt("uHasSun", m_sunTexture ? 1 : 0);
    m_shader.setInt("uHasMoon", m_moonTexture ? 1 : 0);
    m_shader.setInt("uHasClouds", m_cloudTexture ? 1 : 0);
    const unsigned int units[4] = { m_sunTexture, m_moonTexture, m_cloudTexture, m_endTexture };
    const char* names[4] = { "uSunTex", "uMoonTex", "uCloudTex", "uEndTex" };
    for (int i = 0; i < 4; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + 4 + i);
        glBindTexture(GL_TEXTURE_2D, units[i]);
        m_shader.setInt(names[i], 4 + i);
    }
    glActiveTexture(GL_TEXTURE0);
    m_mesh.draw(GL_TRIANGLES);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}
