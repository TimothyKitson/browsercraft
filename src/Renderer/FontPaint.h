#pragma once
#include "FontMaps.h"
#include <cstdint>
#include <vector>

namespace FontPaint
{
    struct Bitmap
    {
        int w = 0;
        int h = 0;
        int top = 0;
        std::vector<uint8_t> ink;

        bool at(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h && ink[static_cast<size_t>(y * w + x)] != 0; }
        void set(int x, int y) { if (x >= 0 && y >= 0 && x < w && y < h) ink[static_cast<size_t>(y * w + x)] = 1; }
        int inkWidth() const;
    };

    bool paint(FontFace face, uint32_t codepoint, Bitmap& out);
    void missing(Bitmap& out);
}
