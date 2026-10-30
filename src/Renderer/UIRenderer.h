#pragma once
#include "Shader.h"
#include "Mesh.h"
#include "Font.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

// How big the interface is drawn, Minecraft's way.
//
// Minecraft does not lay its interface out in screen pixels. It lays it
// out on a small canvas -- never less than 320 by 240 of them -- and
// then blows that canvas up by a whole number, so the art stays sharp
// and the panels keep the same share of the screen whatever the window
// is doing. Everything in the game is written in those canvas pixels:
// a slot is eighteen of them, a heart nine, the hotbar a hundred and
// eighty-two.
//
// A pure function of the window, so --selftest can check the rule at
// every size without a window to look at.
namespace Gui
{
    constexpr int MIN_WIDTH = 320;
    constexpr int MIN_HEIGHT = 240;

    // `preferred` is the setting: 1, 2, 3, 4, and 0 for automatic,
    // which takes the largest that fits. The answer is never below 1,
    // however small the window is.
    int scaleFor(int windowWidth, int windowHeight, int preferred = 0);
}

// Immediate-mode 2D renderer for the HUD: coloured quads, atlas tiles and
// bitmap text, all batched and flushed whenever the texture changes.
//
// Coordinates are in interface pixels with (0,0) at the top-left, and
// setScale decides how many screen pixels one of those is.
class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    void begin(int screenWidth, int screenHeight);
    void end();

    // How many screen pixels one interface pixel is. Everything drawn
    // after this is multiplied by it.
    void setScale(float scale) { m_scale = scale > 0.0f ? scale : 1.0f; }
    float scale() const { return m_scale; }

    // The canvas, in interface pixels: what the width and height of the
    // window come to once the scale is taken off them.
    float canvasWidth() const { return static_cast<float>(m_width) / m_scale; }
    float canvasHeight() const { return static_cast<float>(m_height) / m_scale; }

    // Draw by inverting what is already there rather than painting
    // over it. Minecraft's crosshair is the only thing in the game
    // drawn this way, and it is why it stays legible on snow and on
    // coal without an outline round it.
    void setInvert(bool on);

    void quad(float x, float y, float w, float h, const glm::vec4& color);
    void texturedQuad(unsigned int texture, float x, float y, float w, float h,
                      float u0, float v0, float u1, float v1, const glm::vec4& color);

    // Four corners given outright, rather than a rectangle. Everything
    // else on the HUD is axis-aligned; a block drawn as a cube is three
    // parallelograms, and none of them is.
    //
    // Corners run round the shape, and uv[i] belongs to corner[i].
    // `light` is where the light comes from, in the face's own tangent
    // space. Left at zero the shape is drawn flat, which is what every
    // panel, slot and glyph wants; a block icon's faces pass one, and
    // the normal map then gives them the same relief the world has.
    // The pack's normal atlas, for the block icons. Zero draws them
    // flat, which is what happens when no pack is installed.
    void setNormalAtlas(unsigned int texture) { m_normalAtlas = texture; }

    void texturedShape(unsigned int texture, const glm::vec2 corner[4],
                       const glm::vec2 uv[4], const glm::vec4& color,
                       const glm::vec3& light = glm::vec3(0.0f));

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
    unsigned int m_normalAtlas = 0;
    unsigned int m_currentTexture = 0;
    std::vector<float> m_vertices;
    int m_width = 1;
    int m_height = 1;

    float m_scale = 1.0f;
    bool m_invert = false;
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
