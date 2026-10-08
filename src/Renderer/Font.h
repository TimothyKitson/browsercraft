#pragma once

// A 5x7 bitmap font covering ASCII 32..95 (space through underscore).
// Lowercase letters are folded to uppercase when drawing. The glyphs are
// baked into a small texture at startup; see UIRenderer::text.
class Font
{
public:
    static constexpr int FIRST_CHAR = 32;
    static constexpr int LAST_CHAR = 95;
    static constexpr int GLYPH_W = 5;
    static constexpr int GLYPH_H = 7;
    static constexpr int CELL = 8;          // padded cell size in the texture
    static constexpr int COLUMNS = 16;      // glyphs per texture row
    static constexpr int ROWS = 4;
    static constexpr int TEX_W = COLUMNS * CELL; // 128
    static constexpr int TEX_H = ROWS * CELL;    // 32

    Font();
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    unsigned int textureId() const { return m_id; }

    // UV rect of one character's 5x7 glyph box.
    static void glyphUV(char c, float& u0, float& v0, float& u1, float& v1);

private:
    unsigned int m_id = 0;
};
