#include "BorderRenderer.h"
#include "World/WorldBorder.h"
#include "Core/GLFunctions.h"
#include "Core/Packs.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>

#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif

namespace
{
    const std::vector<int> LAYOUT = { 3, 2 };
    constexpr int PAINTED = 32;
}

BorderRenderer::BorderRenderer()
    : m_shader("assets/shaders/border.vert", "assets/shaders/border.frag")
{
}

BorderRenderer::~BorderRenderer()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

void BorderRenderer::load()
{
    m_loaded = true;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load("assets/textures/misc/forcefield.png", &width, &height, &channels, 4);
    std::vector<unsigned char> painted;
    if (data)
        m_fromPack = true;
    else
    {
        width = height = PAINTED;
        painted.assign(static_cast<size_t>(PAINTED * PAINTED * 4), 0);
        for (int y = 0; y < PAINTED; ++y)
            for (int x = 0; x < PAINTED; ++x)
            {
                const int band = (x + y) % 8;
                const int alpha = band == 0 ? 255 : (band == 1 || band == 7) ? 110 : 0;
                unsigned char* p = &painted[static_cast<size_t>((y * PAINTED + x) * 4)];
                p[0] = p[1] = p[2] = 255;
                p[3] = static_cast<unsigned char>(alpha);
            }
    }
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data ? data : painted.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (data) stbi_image_free(data);
}

float BorderRenderer::alphaFor(const WorldBorder& border, double x, double z, double reach)
{
    if (reach <= 0.0) return 0.0f;
    const double faded = std::pow(1.0 - border.distanceInside(x, z) / reach, 4.0);
    return static_cast<float>(std::clamp(faded, 0.0, 1.0));
}

std::vector<float> BorderRenderer::build(const WorldBorder& border, const glm::dvec3& camera, double reach, double height, float scroll)
{
    std::vector<float> out;
    const double west = border.west(), east = border.east(), north = border.north(), south = border.south();
    if (camera.x < east - reach && camera.x > west + reach && camera.z < south - reach && camera.z > north + reach) return out;

    const float n = static_cast<float>(-(camera.y * 0.5 - std::floor(camera.y * 0.5)));
    const float o = n + static_cast<float>(height);
    const float m = scroll;
    const float bottom = static_cast<float>(camera.y - height);
    const float top = static_cast<float>(camera.y + height);

    auto quad = [&](glm::vec3 a, glm::vec3 b, float u0, float u1) {
        const glm::vec3 a1(a.x, top, a.z), b1(b.x, top, b.z);
        const float corners[6][5] = {
            { a.x, bottom, a.z, u0, m + o }, { b.x, bottom, b.z, u1, m + o }, { b1.x, top, b1.z, u1, m + n },
            { a.x, bottom, a.z, u0, m + o }, { b1.x, top, b1.z, u1, m + n }, { a1.x, top, a1.z, u0, m + n },
        };
        for (const auto& corner : corners) out.insert(out.end(), corner, corner + 5);
    };

    double from = std::max(std::floor(camera.z - reach), north);
    double to = std::min(std::ceil(camera.z + reach), south);
    const float startZ = static_cast<float>(static_cast<long long>(std::floor(from)) & 1) * 0.5f;
    if (camera.x > east - reach)
    {
        float s = startZ;
        for (double t = from; t < to; t += 1.0, s += 0.5f)
        {
            const double width = std::min(1.0, to - t);
            const float v = static_cast<float>(width) * 0.5f;
            quad(glm::vec3(static_cast<float>(east), 0.0f, static_cast<float>(t)), glm::vec3(static_cast<float>(east), 0.0f, static_cast<float>(t + width)), m - s, m - (v + s));
        }
    }
    if (camera.x < west + reach)
    {
        float s = startZ;
        for (double t = from; t < to; t += 1.0, s += 0.5f)
        {
            const double width = std::min(1.0, to - t);
            const float v = static_cast<float>(width) * 0.5f;
            quad(glm::vec3(static_cast<float>(west), 0.0f, static_cast<float>(t)), glm::vec3(static_cast<float>(west), 0.0f, static_cast<float>(t + width)), m + s, m + v + s);
        }
    }

    from = std::max(std::floor(camera.x - reach), west);
    to = std::min(std::ceil(camera.x + reach), east);
    const float startX = static_cast<float>(static_cast<long long>(std::floor(from)) & 1) * 0.5f;
    if (camera.z > south - reach)
    {
        float s = startX;
        for (double t = from; t < to; t += 1.0, s += 0.5f)
        {
            const double width = std::min(1.0, to - t);
            const float v = static_cast<float>(width) * 0.5f;
            quad(glm::vec3(static_cast<float>(t), 0.0f, static_cast<float>(south)), glm::vec3(static_cast<float>(t + width), 0.0f, static_cast<float>(south)), m + s, m + v + s);
        }
    }
    if (camera.z < north + reach)
    {
        float s = startX;
        for (double t = from; t < to; t += 1.0, s += 0.5f)
        {
            const double width = std::min(1.0, to - t);
            const float v = static_cast<float>(width) * 0.5f;
            quad(glm::vec3(static_cast<float>(t), 0.0f, static_cast<float>(north)), glm::vec3(static_cast<float>(t + width), 0.0f, static_cast<float>(north)), m - s, m - (v + s));
        }
    }
    return out;
}

void BorderRenderer::render(const glm::mat4& view, const glm::mat4& projection, const WorldBorder& border, const glm::dvec3& camera,
                            double reach, double height, float scroll)
{
    const std::vector<float> vertices = build(border, camera, reach, height, scroll);
    if (vertices.empty()) return;
    if (!m_loaded) load();

    const unsigned int rgb = border.color();
    const glm::vec4 color(static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f, static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f,
                          static_cast<float>(rgb & 0xFFu) / 255.0f, alphaFor(border, camera.x, camera.z, reach));

    m_mesh.upload(vertices, LAYOUT, true);
    m_shader.bind();
    m_shader.setMat4("uView", view);
    m_shader.setMat4("uProjection", projection);
    m_shader.setVec4("uColor", color);
    m_shader.setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-3.0f, -3.0f);
    m_mesh.draw(GL_TRIANGLES);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}
