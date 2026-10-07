#pragma once
#include "Shader.h"
#include "Mesh.h"
#include "Font.h"
#include "Atlas.h"
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

    // An arbitrary four-cornered patch, given clockwise from the top-left, so
    // the UI can draw sheared faces as well as rectangles.
    void texturedQuad(unsigned int texture, const glm::vec2 corners[4],
                      float u0, float v0, float u1, float v1, const glm::vec4& color);

    // One block drawn as an isometric cube rather than a flat face, the way
    // Minecraft's inventory shows them. Tiles come from the block atlas, so no
    // separate icon art is needed. `size` is the icon's full width.
    void blockIcon(unsigned int atlasTexture, const TileUV& top, const TileUV& left,
                   const TileUV& right, float x, float y, float size);

    void text(const std::string& value, float x, float y, float scale, const glm::vec4& color);
    void textWithShadow(const std::string& value, float x, float y, float scale, const glm::vec4& color);
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

    void useTexture(unsigned int texture);
    void flush();
    void pushQuad(float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1, const glm::vec4& color);
    void pushQuad(const glm::vec2 corners[4],
                  float u0, float v0, float u1, float v1, const glm::vec4& color);
};
