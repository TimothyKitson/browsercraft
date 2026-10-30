#pragma once

// Minecraft's own text, as near as a bitmap can get it: printable ASCII,
// upper and lower case, eight rows tall with the descenders of g, j, p, q
// and y hanging off the bottom, and every character only as wide as it
// needs to be -- an i is one column, an m is five.
//
// The variable width is the whole difference between text that reads like
// Minecraft's and text that reads like a terminal's. Mojang measures each
// glyph off the sheet the same way this does: the width is however many
// columns the drawing actually uses, and the character after it starts one
// column later.
class Font
{
public:
    static constexpr int FIRST_CHAR = 32;   // space
    static constexpr int LAST_CHAR = 126;   // ~
    static constexpr int GLYPH_W = 6;       // the widest any glyph gets
    static constexpr int GLYPH_H = 8;       // seven for a capital, one for a tail

    // Padded cells in the baked texture, so no two glyphs share an edge.
    static constexpr int CELL_W = 8;
    static constexpr int CELL_H = 10;
    static constexpr int COLUMNS = 16;
    static constexpr int ROWS = 6;              // 96 cells for 95 glyphs
    static constexpr int TEX_W = COLUMNS * CELL_W;  // 128
    static constexpr int TEX_H = ROWS * CELL_H;     // 60

    // How wide a space is drawn as. Blank, so it has no columns of its
    // own to measure.
    static constexpr int SPACE_WIDTH = 3;

    Font();
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    unsigned int textureId() const { return m_id; }

    // How many columns this character actually draws.
    static int widthOf(char c);

    // And how far along the next one starts: one clear column after it.
    static int advanceOf(char c) { return widthOf(c) + 1; }

    // UV rect of one character's glyph box, only as wide as the
    // character is: a narrow glyph in a wide box would be drawn with
    // its neighbour's whitespace hanging off it.
    static void glyphUV(char c, float& u0, float& v0, float& u1, float& v1);

private:
    unsigned int m_id = 0;
};
