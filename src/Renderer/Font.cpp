#include "Font.h"
#include "FontPaint.h"
#include "Core/GLFunctions.h"
#include "Core/Packs.h"
#include "Core/Utf8.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

namespace
{
    constexpr int FACES = static_cast<int>(FontFace::Count);

    struct Cell
    {
        int face = 0;
        uint32_t codepoint = 0;
        int w = 0;
        int h = 0;
        float scale = 1.0f;
        int top = 0;
        int measured = 0;
        int x = 0;
        int y = 0;
        std::vector<uint8_t> rgba;
    };

    struct Atlas
    {
        bool built = false;
        int w = 1;
        int h = 1;
        std::vector<uint8_t> rgba;
        std::unordered_map<uint32_t, Glyph> glyphs[FACES];
        int painted[FACES] = {};
        Glyph missing;
    };

    Atlas& atlas()
    {
        static Atlas instance;
        return instance;
    }

    Cell paintedCell(int face, uint32_t codepoint, const FontPaint::Bitmap& bitmap)
    {
        Cell cell;
        cell.face = face;
        cell.codepoint = codepoint;
        cell.w = bitmap.w;
        cell.h = bitmap.h;
        cell.top = bitmap.top;
        cell.measured = bitmap.inkWidth();
        cell.rgba.assign(static_cast<size_t>(cell.w * cell.h * 4), 0);
        for (int y = 0; y < cell.h; ++y)
            for (int x = 0; x < cell.w; ++x)
                if (bitmap.at(x, y))
                {
                    uint8_t* p = &cell.rgba[static_cast<size_t>((y * cell.w + x) * 4)];
                    p[0] = p[1] = p[2] = p[3] = 255;
                }
        return cell;
    }

    void build(Atlas& a)
    {
        a = Atlas();
        std::vector<Cell> cells;
        std::unordered_set<uint64_t> seen;
        auto key = [](int face, uint32_t codepoint) { return (static_cast<uint64_t>(face) << 32) | codepoint; };

        for (const FontMaps::Sheet& sheet : FontMaps::sheets())
        {
            const int face = static_cast<int>(sheet.face);
            int iw = 0, ih = 0, channels = 0;
            unsigned char* image = Packs::load(sheet.file, &iw, &ih, &channels, 4);
            const int cw = image ? iw / 16 : 0;
            const int ch = image ? ih / sheet.rows : 0;
            const bool usable = cw > 0 && ch > 0;
            for (int row = 0; row < sheet.rows; ++row)
                for (int col = 0; col < 16; ++col)
                {
                    const uint32_t codepoint = sheet.chars[row * 16 + col];
                    if (!codepoint || seen.count(key(face, codepoint))) continue;
                    if (usable)
                    {
                        Cell cell;
                        cell.face = face;
                        cell.codepoint = codepoint;
                        cell.w = cw;
                        cell.h = ch;
                        cell.scale = static_cast<float>(sheet.height) / static_cast<float>(ch);
                        cell.top = 7 - sheet.ascent;
                        cell.rgba.assign(static_cast<size_t>(cw * ch * 4), 0);
                        for (int y = 0; y < ch; ++y)
                            for (int x = 0; x < cw; ++x)
                            {
                                const size_t from = (static_cast<size_t>(row * ch + y) * static_cast<size_t>(iw) + static_cast<size_t>(col * cw + x)) * 4;
                                std::copy(image + from, image + from + 4, &cell.rgba[static_cast<size_t>((y * cw + x) * 4)]);
                                if (image[from + 3] != 0) cell.measured = std::max(cell.measured, x + 1);
                            }
                        cells.push_back(std::move(cell));
                    }
                    else
                    {
                        FontPaint::Bitmap bitmap;
                        if (!FontPaint::paint(sheet.face, codepoint, bitmap)) continue;
                        cells.push_back(paintedCell(face, codepoint, bitmap));
                        ++a.painted[face];
                    }
                    seen.insert(key(face, codepoint));
                }
            if (image) stbi_image_free(image);
        }

        FontPaint::Bitmap box;
        FontPaint::missing(box);
        cells.push_back(paintedCell(-1, 0, box));

        long long area = 0;
        int widest = 1;
        for (const Cell& cell : cells)
        {
            area += static_cast<long long>(cell.w + 1) * (cell.h + 1);
            widest = std::max(widest, cell.w + 1);
        }
        int width = 256;
        while (width < 8192 && (static_cast<long long>(width) * width < area * 5 / 4 || width < widest)) width *= 2;

        int x = 0, y = 0, rowHeight = 0;
        for (Cell& cell : cells)
        {
            if (x + cell.w + 1 > width)
            {
                x = 0;
                y += rowHeight;
                rowHeight = 0;
            }
            cell.x = x;
            cell.y = y;
            x += cell.w + 1;
            rowHeight = std::max(rowHeight, cell.h + 1);
        }
        a.w = width;
        a.h = std::max(1, y + rowHeight);
        a.rgba.assign(static_cast<size_t>(a.w) * static_cast<size_t>(a.h) * 4, 0);

        for (const Cell& cell : cells)
        {
            for (int row = 0; row < cell.h; ++row)
                std::copy(cell.rgba.begin() + static_cast<long>(row * cell.w * 4), cell.rgba.begin() + static_cast<long>((row + 1) * cell.w * 4),
                          a.rgba.begin() + static_cast<long>((static_cast<size_t>(cell.y + row) * static_cast<size_t>(a.w) + static_cast<size_t>(cell.x)) * 4));
            Glyph glyph;
            glyph.u0 = static_cast<float>(cell.x) / static_cast<float>(a.w);
            glyph.v0 = static_cast<float>(cell.y) / static_cast<float>(a.h);
            glyph.u1 = static_cast<float>(cell.x + cell.measured) / static_cast<float>(a.w);
            glyph.v1 = static_cast<float>(cell.y + cell.h) / static_cast<float>(a.h);
            glyph.width = static_cast<float>(cell.measured) * cell.scale;
            glyph.height = static_cast<float>(cell.h) * cell.scale;
            glyph.top = static_cast<float>(cell.top);
            glyph.advance = static_cast<float>(static_cast<int>(0.5f + static_cast<float>(cell.measured) * cell.scale) + 1);
            glyph.ink = cell.measured > 0;
            if (cell.face < 0)
                a.missing = glyph;
            else
                a.glyphs[cell.face][cell.codepoint] = glyph;
        }

        for (int face = 0; face < FACES; ++face)
        {
            Glyph space;
            space.advance = static_cast<float>(Font::SPACE_ADVANCE);
            a.glyphs[face][' '] = space;
            a.glyphs[face][0x200C] = Glyph();
        }
        a.built = true;
    }

    Atlas& ready()
    {
        Atlas& a = atlas();
        if (!a.built) build(a);
        return a;
    }
}

void Font::rebuild()
{
    build(atlas());
}

const Glyph& Font::glyph(char32_t codepoint, FontFace face)
{
    static const Glyph CONTROL;
    if (codepoint < 32) return CONTROL;
    const Atlas& a = ready();
    const auto& map = a.glyphs[std::clamp(static_cast<int>(face), 0, FACES - 1)];
    const auto found = map.find(static_cast<uint32_t>(codepoint));
    return found != map.end() ? found->second : a.missing;
}

bool Font::has(char32_t codepoint, FontFace face)
{
    const Atlas& a = ready();
    return a.glyphs[std::clamp(static_cast<int>(face), 0, FACES - 1)].count(static_cast<uint32_t>(codepoint)) != 0;
}

float Font::width(const std::string& utf8, FontFace face)
{
    float total = 0.0f;
    size_t at = 0;
    while (at < utf8.size())
    {
        const char32_t c = Utf8::next(utf8, at);
        if (isFontSwitch(c))
            face = static_cast<FontFace>(c - FONT_SWITCH);
        else
            total += advanceOf(c, face);
    }
    return total;
}

int Font::glyphCount(FontFace face)
{
    return static_cast<int>(ready().glyphs[std::clamp(static_cast<int>(face), 0, FACES - 1)].size());
}

int Font::paintedCount(FontFace face)
{
    return ready().painted[std::clamp(static_cast<int>(face), 0, FACES - 1)];
}

Font::Font()
{
    rebuild();
    const Atlas& a = atlas();
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, a.w, a.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, a.rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

Font::~Font()
{
    if (m_id) glDeleteTextures(1, &m_id);
}
