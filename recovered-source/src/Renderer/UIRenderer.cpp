#include "UIRenderer.h"
#include "Core/GLFunctions.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

namespace
{
    const std::vector<int> UI_LAYOUT = { 2, 2, 4 }; // position, uv, colour
}

UIRenderer::UIRenderer()
    : m_shader("assets/shaders/ui.vert", "assets/shaders/ui.frag")
{
    // 1x1 opaque white pixel, so untextured quads can share the same shader.
    const uint8_t white[4] = { 255, 255, 255, 255 };
    glGenTextures(1, &m_white);
    glBindTexture(GL_TEXTURE_2D, m_white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glBindTexture(GL_TEXTURE_2D, 0);
}

UIRenderer::~UIRenderer()
{
    if (m_white) glDeleteTextures(1, &m_white);
}

void UIRenderer::begin(int screenWidth, int screenHeight)
{
    m_width = screenWidth > 0 ? screenWidth : 1;
    m_height = screenHeight > 0 ? screenHeight : 1;
    m_vertices.clear();
    m_currentTexture = 0;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.bind();
    m_shader.setMat4("uProjection",
                     glm::ortho(0.0f, static_cast<float>(m_width),
                                static_cast<float>(m_height), 0.0f));
    m_shader.setInt("uTexture", 0);
}

void UIRenderer::end()
{
    flush();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void UIRenderer::useTexture(unsigned int texture)
{
    if (texture != m_currentTexture)
    {
        flush();
        m_currentTexture = texture;
    }
}

void UIRenderer::flush()
{
    if (m_vertices.empty() || m_currentTexture == 0) { m_vertices.clear(); return; }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_currentTexture);

    m_mesh.upload(m_vertices, UI_LAYOUT, true);
    m_mesh.draw(GL_TRIANGLES);
    m_vertices.clear();
}

void UIRenderer::pushQuad(float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1, const glm::vec4& color)
{
    const float x0 = x, y0 = y, x1 = x + w, y1 = y + h;
    const float corners[6][4] = {
        { x0, y0, u0, v0 },
        { x0, y1, u0, v1 },
        { x1, y1, u1, v1 },
        { x0, y0, u0, v0 },
        { x1, y1, u1, v1 },
        { x1, y0, u1, v0 },
    };

    for (const auto& c : corners)
    {
        m_vertices.push_back(c[0]);
        m_vertices.push_back(c[1]);
        m_vertices.push_back(c[2]);
        m_vertices.push_back(c[3]);
        m_vertices.push_back(color.r);
        m_vertices.push_back(color.g);
        m_vertices.push_back(color.b);
        m_vertices.push_back(color.a);
    }
}

void UIRenderer::quad(float x, float y, float w, float h, const glm::vec4& color)
{
    useTexture(m_white);
    pushQuad(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

void UIRenderer::texturedQuad(unsigned int texture, float x, float y, float w, float h,
                              float u0, float v0, float u1, float v1, const glm::vec4& color)
{
    useTexture(texture);
    pushQuad(x, y, w, h, u0, v0, u1, v1, color);
}

void UIRenderer::text(const std::string& value, float x, float y, float scale, const glm::vec4& color)
{
    useTexture(m_font.textureId());

    float cursor = x;
    for (char c : value)
    {
        if (c == ' ')
        {
            cursor += (Font::GLYPH_W + 1) * scale;
            continue;
        }
        float u0, v0, u1, v1;
        Font::glyphUV(c, u0, v0, u1, v1);
        pushQuad(cursor, y, Font::GLYPH_W * scale, Font::GLYPH_H * scale, u0, v0, u1, v1, color);
        cursor += (Font::GLYPH_W + 1) * scale;
    }
}

void UIRenderer::textWithShadow(const std::string& value, float x, float y, float scale, const glm::vec4& color)
{
    text(value, x + scale, y + scale, scale, glm::vec4(0.0f, 0.0f, 0.0f, color.a * 0.65f));
    text(value, x, y, scale, color);
}

float UIRenderer::textWidth(const std::string& value, float scale)
{
    if (value.empty()) return 0.0f;
    return value.size() * (Font::GLYPH_W + 1) * scale - scale;
}
