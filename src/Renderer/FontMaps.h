#pragma once
#include <cstdint>
#include <vector>

enum class FontFace : uint8_t { Default, Galactic, Illager, Count };

constexpr char32_t FONT_SWITCH = 0xE000;
constexpr bool isFontSwitch(char32_t c) { return c >= FONT_SWITCH && c < FONT_SWITCH + static_cast<char32_t>(FontFace::Count); }

namespace FontMaps
{
    using Face = FontFace;

    struct Sheet
    {
        Face face;
        const char* file;
        int height;
        int ascent;
        int rows;
        const uint32_t* chars;
    };

    const std::vector<Sheet>& sheets();
}
