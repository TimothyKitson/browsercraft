#pragma once
#include "Shader.h"
#include "Mesh.h"
#include "Font.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Immediate-mode 2D renderer for the HUD: coloured quads, atlas tiles and
// bitmap text, all batched and flushed whenever the texture changes.
// Coordinates are in pixels with (0,0) at the top-left.
class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    void begin(int screenWidth, int screenHeight);
    void end();

    void quad(float x, float y, float w, float h, const glm::vec4& color);
    void texturedQuad(unsigned int texture, float x, float y, float w, float h,
                      float u0, float v0, float u1, float v1, const glm::vec4& color);

    void text(const std::string& value, float x, float y, float scale, const glm::vec4& color);
    void textWithShadow(const std::string& value, float x, float y, float scale, const glm::vec4& color);
    // Same, but tilted about the middle of the line. Minecraft's splash
    // text is the only thing that needs it, and it needs it badly: the
    // tilt is most of what makes it read as a splash rather than a
    // caption.
    void textWithShadowRotated(const std::string& value, float x, float y, float scale,
                               float radians, const glm::vec4& color);
    static float textWidth(const std::string& value, float scale);
    static float textHeight(float scale) { return Font::GLYPH_H * scale; }

    unsigned int fontTexture() const { return m_font.textureId(); }
    unsigned int whiteTexture() const { return m_white; }

private:
    Shader m_shader;
    Mesh m_mesh;
    Font m_font;
    unsigned int m_white = 0;
    unsigned int m_currentTexture = 0;
    std::vector<float> m_vertices;
    int m_width = 1;
    int m_height = 1;

    float m_rotation = 0.0f;
    float m_pivotX = 0.0f;
    float m_pivotY = 0.0f;

    // Applied to every vertex until cleared. Kept as renderer state
    // rather than a parameter on each call so the glyph loop in text()
    // does not have to know anything about it.
    void setRotation(float radians, float pivotX, float pivotY);
    void clearRotation() { m_rotation = 0.0f; }

    void useTexture(unsigned int texture);
    void flush();
    void pushQuad(float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1, const glm::vec4& color);
};
