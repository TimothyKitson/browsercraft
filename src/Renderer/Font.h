#pragma once
#include "FontMaps.h"
#include <cstdint>
#include <string>

struct Glyph
{
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float top = 0.0f;
    float advance = 0.0f;
    bool ink = false;
};

class Font
{
public:
    static constexpr int GLYPH_H = 8;
    static constexpr int LINE_H = 9;
    static constexpr int SPACE_ADVANCE = 4;

    Font();
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    unsigned int textureId() const { return m_id; }

    static const Glyph& glyph(char32_t codepoint, FontFace face = FontFace::Default);
    static bool has(char32_t codepoint, FontFace face = FontFace::Default);
    static float advanceOf(char32_t codepoint, FontFace face = FontFace::Default) { return glyph(codepoint, face).advance; }
    static float width(const std::string& utf8, FontFace face = FontFace::Default);
    static int glyphCount(FontFace face);
    static int paintedCount(FontFace face);
    static void rebuild();

private:
    unsigned int m_id = 0;
};
