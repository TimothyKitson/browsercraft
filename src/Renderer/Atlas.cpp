#include "Atlas.h"
#include "Core/GLFunctions.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>
#include <iterator>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

namespace
{
    // Set once while the atlas is built, before any chunk meshing starts,
    // so tileUV() can be called from worker threads without locking.
    int g_atlasPixelSize = Atlas::TILES_PER_ROW * Atlas::FALLBACK_TILE_PIXELS;
}

int Atlas::pixelSize() { return g_atlasPixelSize; }

TileUV tileUV(int tileIndex)
{
    constexpr float step = 1.0f / Atlas::TILES_PER_ROW;
    // Half-texel inset stops neighbouring tiles bleeding in at the seams.
    const float inset = 0.5f / static_cast<float>(g_atlasPixelSize);

    int col = tileIndex % Atlas::TILES_PER_ROW;
    int row = tileIndex / Atlas::TILES_PER_ROW;

    TileUV uv;
    uv.u0 = col * step + inset;
    uv.u1 = (col + 1) * step - inset;
    uv.vTop = row * step + inset;
    uv.vBottom = (row + 1) * step - inset;
    return uv;
}

namespace
{
    struct Color
    {
        uint8_t r = 0, g = 0, b = 0, a = 255;
    };

    uint8_t clampByte(int v) { return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v)); }

    Color shade(Color c, int delta)
    {
        return Color{ clampByte(c.r + delta), clampByte(c.g + delta), clampByte(c.b + delta), c.a };
    }

    Color mixColor(Color a, Color b, float t)
    {
        return Color{
            clampByte(static_cast<int>(a.r + (b.r - a.r) * t)),
            clampByte(static_cast<int>(a.g + (b.g - a.g) * t)),
            clampByte(static_cast<int>(a.b + (b.b - a.b) * t)),
            clampByte(static_cast<int>(a.a + (b.a - a.a) * t))
        };
    }

    // Deterministic pseudo-random value in [0,1) from an integer coordinate.
    float hash2(int x, int y, int salt)
    {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u
                   + static_cast<uint32_t>(y) * 668265263u
                   + static_cast<uint32_t>(salt) * 2246822519u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return static_cast<float>(h & 0xFFFFFF) / static_cast<float>(0x1000000);
    }

    // One tile being painted: 16x16 RGBA.
    class Tile
    {
    public:
        static constexpr int N = Atlas::FALLBACK_TILE_PIXELS;

        void fill(Color c)
        {
            for (auto& p : px) p = c;
        }

        void set(int x, int y, Color c)
        {
            if (x < 0 || y < 0 || x >= N || y >= N) return;
            px[y * N + x] = c;
        }

        Color get(int x, int y) const
        {
            x = std::clamp(x, 0, N - 1);
            y = std::clamp(y, 0, N - 1);
            return px[y * N + x];
        }

        // Adds per-pixel brightness variation.
        void speckle(int amount, int salt)
        {
            for (int y = 0; y < N; ++y)
                for (int x = 0; x < N; ++x)
                {
                    int delta = static_cast<int>((hash2(x, y, salt) - 0.5f) * 2.0f * amount);
                    set(x, y, shade(get(x, y), delta));
                }
        }

        std::vector<Color> px = std::vector<Color>(N * N);
    };

    using TileFn = void (*)(Tile&);

    // ---------- individual tile painters ----------

    void paintBlank(Tile& t) { t.fill(Color{ 0, 0, 0, 0 }); }

    void paintStone(Tile& t)
    {
        t.fill(Color{ 126, 126, 126 });
        t.speckle(13, 101);
        for (int i = 0; i < 5; ++i)
        {
            int cx = static_cast<int>(hash2(i, 0, 7) * Tile::N);
            int cy = static_cast<int>(hash2(i, 1, 7) * Tile::N);
            for (int y = cy - 2; y <= cy + 2; ++y)
                for (int x = cx - 2; x <= cx + 2; ++x)
                    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= 4)
                        t.set(x, y, shade(t.get(x, y), -14));
        }
    }

    void paintDirt(Tile& t)
    {
        t.fill(Color{ 134, 96, 67 });
        t.speckle(18, 202);
        for (int i = 0; i < 10; ++i)
        {
            int x = static_cast<int>(hash2(i, 3, 11) * Tile::N);
            int y = static_cast<int>(hash2(i, 4, 11) * Tile::N);
            t.set(x, y, shade(t.get(x, y), -28));
        }
    }

    void paintGrassTop(Tile& t)
    {
        t.fill(Color{ 106, 170, 64 });
        t.speckle(16, 303);
        for (int i = 0; i < 8; ++i)
        {
            int x = static_cast<int>(hash2(i, 5, 13) * Tile::N);
            int y = static_cast<int>(hash2(i, 6, 13) * Tile::N);
            t.set(x, y, shade(t.get(x, y), 18));
        }
    }

    // Shared helper: dirt with a ragged band of `top` colour along the top edge.
    void paintSideWithCap(Tile& t, Color cap, int baseDepth, int salt)
    {
        paintDirt(t);
        for (int x = 0; x < Tile::N; ++x)
        {
            int depth = baseDepth + static_cast<int>(hash2(x, 0, salt) * 3.0f);
            for (int y = 0; y < depth; ++y)
            {
                int delta = static_cast<int>((hash2(x, y, salt + 1) - 0.5f) * 24.0f);
                t.set(x, y, shade(cap, delta));
            }
        }
    }

    void paintGrassSide(Tile& t) { paintSideWithCap(t, Color{ 101, 161, 60 }, 3, 404); }
    void paintSnowGrassSide(Tile& t) { paintSideWithCap(t, Color{ 245, 249, 255 }, 4, 414); }

    void paintCobble(Tile& t)
    {
        t.fill(Color{ 102, 102, 102 }); // mortar
        for (int cy = 0; cy < 4; ++cy)
            for (int cx = 0; cx < 4; ++cx)
            {
                int tint = static_cast<int>((hash2(cx, cy, 21) - 0.5f) * 44.0f);
                Color stone = shade(Color{ 135, 135, 135 }, tint);
                for (int y = 0; y < 4; ++y)
                    for (int x = 0; x < 4; ++x)
                    {
                        bool corner = (x == 0 || x == 3) && (y == 0 || y == 3);
                        bool edge = (x == 0 || y == 0);
                        if (corner || edge) continue; // leave mortar gaps
                        int px = cx * 4 + x;
                        int py = cy * 4 + y;
                        int delta = static_cast<int>((hash2(px, py, 22) - 0.5f) * 18.0f);
                        t.set(px, py, shade(stone, delta));
                    }
            }
    }

    void paintMossyCobble(Tile& t)
    {
        paintCobble(t);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 33) > 0.55f)
                    t.set(x, y, mixColor(t.get(x, y), Color{ 78, 120, 56 }, 0.55f));
    }

    void paintPlanks(Tile& t)
    {
        for (int y = 0; y < Tile::N; ++y)
        {
            int row = y / 4;
            int rowTint = static_cast<int>((hash2(row, 0, 41) - 0.5f) * 26.0f);
            Color wood = shade(Color{ 162, 130, 78 }, rowTint);
            for (int x = 0; x < Tile::N; ++x)
            {
                Color c = wood;
                if (hash2(x, row * 7, 42) > 0.80f) c = shade(c, -22); // grain
                if (y % 4 == 3) c = shade(c, -40);                     // plank seam
                int joint = static_cast<int>(hash2(row, 1, 43) * Tile::N);
                if (x == joint) c = shade(c, -40);                     // end joint
                t.set(x, y, c);
            }
        }
        t.speckle(6, 44);
    }

    void paintBedrock(Tile& t)
    {
        t.fill(Color{ 85, 85, 85 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                float n = hash2(x / 2, y / 2, 51);
                int delta = n > 0.6f ? 34 : (n < 0.35f ? -38 : 0);
                t.set(x, y, shade(t.get(x, y), delta));
            }
        t.speckle(8, 52);
    }

    void paintWater(Tile& t)
    {
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                float wave = std::sin((x + y * 0.6f) * 0.9f) * 0.5f + 0.5f;
                Color c = mixColor(Color{ 44, 94, 186, 190 }, Color{ 72, 128, 214, 190 }, wave);
                t.set(x, y, c);
            }
        t.speckle(5, 61);
    }

    void paintLava(Tile& t)
    {
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                float n = hash2(x / 2, y / 2, 71);
                Color c = mixColor(Color{ 196, 72, 12 }, Color{ 255, 176, 48 }, n);
                t.set(x, y, c);
            }
        t.speckle(10, 72);
    }

    // --- the other dimensions ---

    void paintNetherrack(Tile& t)
    {
        // Rough, veined and unmistakably not stone: the Nether reads as
        // hostile mostly through this one texture.
        t.fill(Color{ 112, 48, 48 });
        t.speckle(16, 601);
        for (int i = 0; i < 14; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 602) * Tile::N);
            const int y = static_cast<int>(hash2(i, 1, 602) * Tile::N);
            const int length = 2 + static_cast<int>(hash2(i, 2, 602) * 3.0f);
            for (int k = 0; k < length; ++k)
                t.set(x, (y + k) % Tile::N, shade(t.get(x, (y + k) % Tile::N), -26));
        }
    }

    void paintSoulSand(Tile& t)
    {
        t.fill(Color{ 84, 64, 51 });
        t.speckle(12, 611);
        // Three sunken hollows, which is what makes it read as soul sand
        // rather than as dirt.
        for (int i = 0; i < 3; ++i)
        {
            const int cx = 3 + static_cast<int>(hash2(i, 0, 612) * 10.0f);
            const int cy = 3 + static_cast<int>(hash2(i, 1, 612) * 10.0f);
            for (int y = cy - 2; y <= cy + 2; ++y)
                for (int x = cx - 2; x <= cx + 2; ++x)
                {
                    const int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
                    if (d <= 4) t.set(x, y, shade(t.get(x, y), d <= 1 ? -30 : -16));
                }
        }
    }

    void paintEndStone(Tile& t)
    {
        t.fill(Color{ 221, 223, 165 });
        t.speckle(10, 621);
        for (int i = 0; i < 18; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 622) * Tile::N);
            const int y = static_cast<int>(hash2(i, 1, 622) * Tile::N);
            t.set(x, y, shade(t.get(x, y), -34));
        }
    }

    void paintNetherPortal(Tile& t)
    {
        // A swirl rather than flat purple, so a portal still reads as a
        // portal when it is only one block wide.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = (x - 7.5f) / 8.0f;
                const float dy = (y - 7.5f) / 8.0f;
                const float swirl = std::sin((dx * dx + dy * dy) * 6.0f +
                                             std::atan2(dy, dx) * 2.0f);
                const int lift = static_cast<int>(swirl * 34.0f);
                t.set(x, y, Color{ clampByte(126 + lift), clampByte(38 + lift / 2),
                                   clampByte(176 + lift), 190 });
            }
        t.speckle(8, 631);
    }

    void paintSand(Tile& t)
    {
        t.fill(Color{ 219, 207, 163 });
        t.speckle(11, 81);
    }

    void paintGravel(Tile& t)
    {
        t.fill(Color{ 128, 124, 122 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                float n = hash2(x / 2, y / 2, 91);
                t.set(x, y, shade(t.get(x, y), static_cast<int>((n - 0.5f) * 60.0f)));
            }
        t.speckle(9, 92);
    }

    void paintOre(Tile& t, Color oreColor, int salt)
    {
        paintStone(t);
        for (int i = 0; i < 5; ++i)
        {
            int cx = 2 + static_cast<int>(hash2(i, 0, salt) * 12.0f);
            int cy = 2 + static_cast<int>(hash2(i, 1, salt) * 12.0f);
            int radius = hash2(i, 2, salt) > 0.5f ? 2 : 1;
            for (int y = cy - radius; y <= cy + radius; ++y)
                for (int x = cx - radius; x <= cx + radius; ++x)
                {
                    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) > radius * radius) continue;
                    int delta = static_cast<int>((hash2(x, y, salt + 5) - 0.5f) * 40.0f);
                    t.set(x, y, shade(oreColor, delta));
                }
        }
    }

    void paintGoldOre(Tile& t) { paintOre(t, Color{ 247, 206, 73 }, 111); }
    void paintIronOre(Tile& t) { paintOre(t, Color{ 203, 160, 122 }, 112); }
    void paintCoalOre(Tile& t) { paintOre(t, Color{ 38, 38, 38 }, 113); }
    void paintDiamondOre(Tile& t) { paintOre(t, Color{ 96, 231, 226 }, 114); }

    void paintLogSide(Tile& t)
    {
        for (int x = 0; x < Tile::N; ++x)
        {
            int tint = static_cast<int>((hash2(x, 0, 121) - 0.5f) * 36.0f);
            Color bark = shade(Color{ 104, 78, 47 }, tint);
            for (int y = 0; y < Tile::N; ++y)
            {
                Color c = bark;
                if (hash2(x, y / 3, 122) > 0.82f) c = shade(c, -24);
                t.set(x, y, c);
            }
        }
    }

    void paintRings(Tile& t, Color inner, Color outer, Color bark, int salt)
    {
        const float cx = 7.5f, cy = 7.5f;
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                float d = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                int ring = static_cast<int>(d * 1.5f);
                Color c = (ring % 2 == 0) ? inner : outer;
                if (d > 6.6f) c = bark;
                int delta = static_cast<int>((hash2(x, y, salt) - 0.5f) * 16.0f);
                t.set(x, y, shade(c, delta));
            }
    }

    void paintLogTop(Tile& t) { paintRings(t, Color{ 167, 136, 84 }, Color{ 146, 115, 67 }, Color{ 104, 78, 47 }, 131); }
    void paintBirchLogTop(Tile& t) { paintRings(t, Color{ 216, 202, 166 }, Color{ 196, 180, 145 }, Color{ 221, 221, 212 }, 132); }

    void paintBirchLogSide(Tile& t)
    {
        t.fill(Color{ 221, 221, 212 });
        t.speckle(9, 141);
        for (int i = 0; i < 6; ++i)
        {
            int y = static_cast<int>(hash2(i, 0, 142) * Tile::N);
            int x = static_cast<int>(hash2(i, 1, 142) * 12.0f);
            int len = 2 + static_cast<int>(hash2(i, 2, 142) * 3.0f);
            for (int k = 0; k < len; ++k) t.set(x + k, y, Color{ 60, 58, 54 });
        }
    }

    void paintLeavesBase(Tile& t, Color base, int salt)
    {
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                float n = hash2(x, y, salt);
                if (n > 0.86f)
                {
                    t.set(x, y, Color{ 0, 0, 0, 0 }); // gap, lets light through
                    continue;
                }
                int delta = static_cast<int>((hash2(x, y, salt + 1) - 0.5f) * 44.0f);
                t.set(x, y, shade(base, delta));
            }
    }

    void paintLeaves(Tile& t) { paintLeavesBase(t, Color{ 60, 118, 42 }, 151); }
    void paintBirchLeaves(Tile& t) { paintLeavesBase(t, Color{ 108, 152, 70 }, 152); }

    void paintGlass(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        Color frame{ 205, 229, 238, 150 };
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, 0, frame);
            t.set(i, Tile::N - 1, frame);
            t.set(0, i, frame);
            t.set(Tile::N - 1, i, frame);
        }
        for (int i = 3; i < 8; ++i) t.set(i, i, Color{ 255, 255, 255, 90 }); // highlight streak
    }

    void paintSandstoneSide(Tile& t)
    {
        t.fill(Color{ 217, 206, 160 });
        t.speckle(8, 161);
        for (int y = 0; y < Tile::N; ++y)
            if (y % 5 == 0)
                for (int x = 0; x < Tile::N; ++x)
                    t.set(x, y, shade(t.get(x, y), -22));
        for (int x = 0; x < Tile::N; ++x)
            for (int y = 0; y < 3; ++y)
                t.set(x, y, shade(t.get(x, y), 12));
    }

    void paintSandstoneTop(Tile& t)
    {
        t.fill(Color{ 222, 211, 165 });
        t.speckle(7, 162);
    }

    void paintSnow(Tile& t)
    {
        t.fill(Color{ 246, 250, 255 });
        t.speckle(6, 171);
    }

    void paintIce(Tile& t)
    {
        t.fill(Color{ 150, 190, 238, 200 });
        t.speckle(8, 181);
        for (int i = 0; i < 4; ++i)
        {
            int x = static_cast<int>(hash2(i, 0, 182) * Tile::N);
            int y = static_cast<int>(hash2(i, 1, 182) * Tile::N);
            for (int k = 0; k < 5; ++k)
                t.set(x + k, y + (k / 2), Color{ 200, 226, 252, 210 });
        }
    }

    void paintCactusSide(Tile& t)
    {
        t.fill(Color{ 85, 125, 55 });
        t.speckle(9, 191);
        for (int x = 0; x < Tile::N; ++x)
        {
            if (x % 5 == 0)
                for (int y = 0; y < Tile::N; ++y) t.set(x, y, shade(t.get(x, y), -26));
            if (x % 5 == 2)
                for (int y = 0; y < Tile::N; ++y) t.set(x, y, shade(t.get(x, y), 14));
        }
        for (int i = 0; i < 6; ++i)
        {
            int x = static_cast<int>(hash2(i, 0, 192) * Tile::N);
            int y = static_cast<int>(hash2(i, 1, 192) * Tile::N);
            t.set(x, y, Color{ 226, 226, 200 }); // spine
        }
    }

    void paintCactusTop(Tile& t)
    {
        t.fill(Color{ 100, 143, 64 });
        t.speckle(8, 193);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                float d = std::sqrt((x - 7.5f) * (x - 7.5f) + (y - 7.5f) * (y - 7.5f));
                if (d > 5.0f && d < 6.4f) t.set(x, y, shade(t.get(x, y), -24));
            }
    }

    void paintBricks(Tile& t)
    {
        t.fill(Color{ 166, 157, 150 }); // mortar
        for (int row = 0; row < 4; ++row)
        {
            int offset = (row % 2) * 4;
            for (int y = 0; y < 3; ++y)
                for (int x = 0; x < Tile::N; ++x)
                {
                    int bx = (x + offset) % Tile::N;
                    if ((bx % 8) == 7) continue; // vertical mortar joint
                    int brick = (x + offset) / 8 + row * 3;
                    int delta = static_cast<int>((hash2(brick, row, 201) - 0.5f) * 26.0f);
                    t.set(x, row * 4 + y, shade(Color{ 150, 73, 57 }, delta));
                }
        }
    }

    void paintObsidian(Tile& t)
    {
        t.fill(Color{ 21, 16, 30 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 211) > 0.84f)
                    t.set(x, y, Color{ 62, 44, 92 });
        t.speckle(5, 212);
    }

    void paintClay(Tile& t)
    {
        t.fill(Color{ 159, 164, 177 });
        t.speckle(8, 221);
    }

    void paintPumpkinSide(Tile& t)
    {
        t.fill(Color{ 198, 118, 24 });
        t.speckle(8, 231);
        for (int x = 0; x < Tile::N; ++x)
            if (x % 4 == 0)
                for (int y = 0; y < Tile::N; ++y) t.set(x, y, shade(t.get(x, y), -30));
    }

    void paintPumpkinTop(Tile& t)
    {
        t.fill(Color{ 190, 112, 22 });
        t.speckle(7, 232);
        for (int y = 6; y <= 9; ++y)
            for (int x = 6; x <= 9; ++x)
                t.set(x, y, Color{ 126, 95, 40 }); // stem
    }

    void paintWool(Tile& t)
    {
        t.fill(Color{ 233, 233, 233 });
        t.speckle(12, 241);
    }

    void paintGlowstone(Tile& t)
    {
        t.fill(Color{ 180, 150, 86 });
        t.speckle(10, 251);
        for (int i = 0; i < 7; ++i)
        {
            int cx = 2 + static_cast<int>(hash2(i, 0, 252) * 12.0f);
            int cy = 2 + static_cast<int>(hash2(i, 1, 252) * 12.0f);
            for (int y = cy - 1; y <= cy + 1; ++y)
                for (int x = cx - 1; x <= cx + 1; ++x)
                    t.set(x, y, Color{ 255, 232, 150 });
        }
    }

    void paintTallGrass(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int blade = 0; blade < 7; ++blade)
        {
            int x = 1 + static_cast<int>(hash2(blade, 0, 261) * 14.0f);
            int height = 7 + static_cast<int>(hash2(blade, 1, 261) * 7.0f);
            Color green = shade(Color{ 84, 140, 52 }, static_cast<int>((hash2(blade, 2, 261) - 0.5f) * 40.0f));
            for (int k = 0; k < height; ++k)
            {
                int y = Tile::N - 1 - k;
                int bend = static_cast<int>(k * 0.18f * (hash2(blade, 3, 261) > 0.5f ? 1 : -1));
                t.set(x + bend, y, green);
            }
        }
    }

    void paintFlower(Tile& t, Color petal, int salt)
    {
        paintTallGrass(t);
        // Clear the top half so the bloom reads clearly, keep a stem.
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < Tile::N; ++x)
                t.set(x, y, Color{ 0, 0, 0, 0 });
        for (int y = 6; y < Tile::N; ++y) t.set(7, y, Color{ 76, 124, 48 });
        t.set(6, 9, Color{ 76, 124, 48 });
        t.set(8, 11, Color{ 76, 124, 48 });

        for (int y = 2; y <= 6; ++y)
            for (int x = 5; x <= 9; ++x)
            {
                float d = std::sqrt((x - 7.0f) * (x - 7.0f) + (y - 4.0f) * (y - 4.0f));
                if (d <= 2.4f)
                {
                    int delta = static_cast<int>((hash2(x, y, salt) - 0.5f) * 30.0f);
                    t.set(x, y, shade(petal, delta));
                }
            }
        t.set(7, 4, Color{ 250, 232, 120 }); // centre
    }

    void paintFlowerRed(Tile& t) { paintFlower(t, Color{ 198, 52, 52 }, 271); }
    void paintFlowerYellow(Tile& t) { paintFlower(t, Color{ 226, 198, 54 }, 272); }

    void paintTorch(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 6; y < Tile::N; ++y)
        {
            t.set(7, y, Color{ 122, 90, 52 });
            t.set(8, y, Color{ 98, 72, 42 });
        }
        for (int y = 3; y <= 6; ++y)
            for (int x = 6; x <= 9; ++x)
            {
                float d = std::sqrt((x - 7.5f) * (x - 7.5f) + (y - 5.0f) * (y - 5.0f));
                if (d <= 2.2f) t.set(x, y, Color{ 255, 196, 72 });
                if (d <= 1.1f) t.set(x, y, Color{ 255, 240, 170 });
            }
    }

    void paintCrack(Tile& t, int stage)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        int lines = 1 + stage;
        uint8_t alpha = static_cast<uint8_t>(90 + stage * 14);
        for (int i = 0; i < lines; ++i)
        {
            float angle = hash2(i, stage, 281) * 6.2831853f;
            float x = 8.0f, y = 8.0f;
            float dx = std::cos(angle), dy = std::sin(angle);
            int length = 4 + static_cast<int>(hash2(i, stage, 282) * 6.0f) + stage;
            for (int k = 0; k < length; ++k)
            {
                t.set(static_cast<int>(x), static_cast<int>(y), Color{ 15, 15, 15, alpha });
                x += dx;
                y += dy;
                // Wobble so cracks don't look like clean rays.
                dx += (hash2(i * 31 + k, stage, 283) - 0.5f) * 0.6f;
                dy += (hash2(i * 17 + k, stage, 284) - 0.5f) * 0.6f;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.001f) { dx /= len; dy /= len; }
            }
        }
    }

    // ---------- resource pack loading ----------

    const char* PACK_DIRECTORY = "assets/textures/block";
    const char* CREDIT_FILE = "assets/textures/CREDIT.txt";

    // First non-blank, non-comment line of the credit file, trimmed and
    // capped to something that fits across the bottom of the screen.
    std::string readPackCredit()
    {
        std::FILE* file = std::fopen(CREDIT_FILE, "rb");
        if (!file) return {};

        std::string credit;
        char line[256];
        while (std::fgets(line, sizeof(line), file))
        {
            std::string text(line);
            while (!text.empty() && (text.back() == '\n' || text.back() == '\r' ||
                                     text.back() == ' ' || text.back() == '\t'))
                text.pop_back();

            size_t start = 0;
            while (start < text.size() && (text[start] == ' ' || text[start] == '\t')) ++start;
            text = text.substr(start);

            if (text.empty() || text[0] == '#') continue;
            credit = text.size() > 96 ? text.substr(0, 96) : text;
            break;
        }

        std::fclose(file);
        return credit;
    }

    struct Image
    {
        int width = 0;
        int height = 0;
        std::vector<uint8_t> rgba;

        bool valid() const { return width > 0 && height > 0; }

        Color at(int x, int y) const
        {
            const size_t i = (static_cast<size_t>(y) * width + x) * 4;
            return Color{ rgba[i], rgba[i + 1], rgba[i + 2], rgba[i + 3] };
        }

        void set(int x, int y, Color c)
        {
            const size_t i = (static_cast<size_t>(y) * width + x) * 4;
            rgba[i] = c.r; rgba[i + 1] = c.g; rgba[i + 2] = c.b; rgba[i + 3] = c.a;
        }
    };

    Image loadPNG(const std::string& path)
    {
        Image image;
        int channels = 0;
        // No vertical flip: this atlas is stored top-down, matching PNG order.
        unsigned char* data = stbi_load(path.c_str(), &image.width, &image.height, &channels, 4);
        if (!data)
        {
            image.width = image.height = 0;
            return image;
        }
        image.rgba.assign(data, data + static_cast<size_t>(image.width) * image.height * 4);
        stbi_image_free(data);
        return image;
    }

    Image loadFirstAvailable(const char* const* candidates)
    {
        for (int i = 0; candidates[i] != nullptr; ++i)
        {
            Image image = loadPNG(std::string(PACK_DIRECTORY) + "/" + candidates[i] + ".png");
            if (image.valid()) return image;
        }
        return Image{};
    }

    // ---------- Bedrock "texture set" PBR ----------
    //
    // Java packs ship LabPBR: "<name>_n" packs the normal plus AO and a
    // height map, "<name>_s" packs smoothness, F0, porosity and emission.
    // Bedrock RTX / Vibrant Visuals packs (Prizma Visuals and friends)
    // split the same information up differently: "<name>_normal" or
    // "<name>_heightmap" for shape, and "<name>_mer" holding metalness,
    // emissive and roughness in R, G and B. Everything downstream -- the
    // atlas layers, the chunk shader -- speaks LabPBR, so Bedrock maps get
    // converted here on load instead of branching in the shader.

    bool fileExists(const std::string& path)
    {
        std::FILE* file = std::fopen(path.c_str(), "rb");
        if (!file) return false;
        std::fclose(file);
        return true;
    }

    // Bedrock packs declare their maps in "<name>.texture_set.json", but the
    // files themselves always follow the suffix convention, so we only need
    // to know which dialect the pack speaks. Probing a few blocks that every
    // pack touches is enough, and costs nothing beside decoding PNGs.
    bool packIsBedrockStyle()
    {
        const char* probes[] = { "stone", "dirt", "cobblestone", "oak_log", "sand", "gravel" };
        for (const char* name : probes)
        {
            const std::string base = std::string(PACK_DIRECTORY) + "/" + name;
            if (fileExists(base + ".texture_set.json")) return true;
            if (fileExists(base + "_mer.png")) return true;
            if (fileExists(base + "_mers.png")) return true;
        }
        return false;
    }

    uint8_t encodeAxis(float value)
    {
        return static_cast<uint8_t>(std::clamp((value * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
    }

    // Bedrock normals are an ordinary tangent-space RGB map. LabPBR keeps X
    // and Y where they are but reuses blue for ambient occlusion and alpha
    // for height, so Z is dropped and the extra channels are filled in from
    // the heightmap (or left at "no effect").
    Image bedrockNormalToLab(const Image& source, const Image& heightmap)
    {
        const bool haveHeight = heightmap.valid() &&
                                heightmap.width == source.width &&
                                heightmap.height == source.height;
        Image out = source;
        for (int y = 0; y < out.height; ++y)
            for (int x = 0; x < out.width; ++x)
            {
                const Color c = source.at(x, y);
                const uint8_t height = haveHeight ? heightmap.at(x, y).r : 255;
                out.set(x, y, Color{ c.r, c.g, 255, height });
            }
        return out;
    }

    // Some Bedrock packs ship only a heightmap. Sobel it into a normal so
    // those blocks still catch the light instead of rendering dead flat.
    Image heightmapToLabNormal(const Image& heightmap, float strength)
    {
        const int w = heightmap.width;
        const int h = heightmap.height;
        auto sample = [&](int x, int y) {
            return heightmap.at(((x % w) + w) % w, ((y % h) + h) % h).r / 255.0f;
        };

        Image out = heightmap;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                // Rows run downwards, so a surface rising towards the bottom
                // of the tile tilts its normal towards +Y in LabPBR's
                // green-up convention.
                const float dx = sample(x + 1, y) - sample(x - 1, y);
                const float dy = sample(x, y + 1) - sample(x, y - 1);

                float nx = -dx * strength;
                float ny = dy * strength;
                const float inv = 1.0f / std::sqrt(nx * nx + ny * ny + 1.0f);
                nx *= inv;
                ny *= inv;

                out.set(x, y, Color{ encodeAxis(nx), encodeAxis(ny), 255,
                                     heightmap.at(x, y).r });
            }
        return out;
    }

    // MER -> LabPBR specular. Metalness becomes F0 (which the shader adds
    // straight onto the base 0.04 reflectance), roughness is inverted into
    // perceptual smoothness, and emissive moves into alpha -- where 255
    // means "not emissive", so a black emissive channel has to become 255.
    Image merToLabSpecular(const Image& mer)
    {
        Image out = mer;
        for (int y = 0; y < out.height; ++y)
            for (int x = 0; x < out.width; ++x)
            {
                const Color c = mer.at(x, y);
                const uint8_t smoothness = static_cast<uint8_t>(255 - c.b);
                const uint8_t emission = (c.g == 0) ? 255 : std::min<uint8_t>(254, c.g);
                out.set(x, y, Color{ smoothness, c.r, 0, emission });
            }
        return out;
    }

    Image scaleNearest(const Image& source, int width, int height)
    {
        if (!source.valid()) return Image{};
        if (source.width == width && source.height == height) return source;

        Image out;
        out.width = width;
        out.height = height;
        out.rgba.resize(static_cast<size_t>(width) * height * 4);

        const bool shrinking = (source.width > width || source.height > height);

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                if (!shrinking)
                {
                    // Upscaling keeps hard pixel edges, as pixel art should.
                    const int sx = std::min(source.width - 1, x * source.width / width);
                    const int sy = std::min(source.height - 1, y * source.height / height);
                    out.set(x, y, source.at(sx, sy));
                    continue;
                }

                // Downscaling averages the whole source block instead of
                // dropping pixels, so a 256x pack reduced to 64x keeps its
                // detail instead of turning into noise.
                const int x0 = x * source.width / width;
                const int x1 = std::max(x0 + 1, (x + 1) * source.width / width);
                const int y0 = y * source.height / height;
                const int y1 = std::max(y0 + 1, (y + 1) * source.height / height);

                int alphaTotal = 0, red = 0, green = 0, blue = 0, weight = 0, samples = 0;
                for (int sy = y0; sy < y1 && sy < source.height; ++sy)
                    for (int sx = x0; sx < x1 && sx < source.width; ++sx)
                    {
                        const Color c = source.at(sx, sy);
                        alphaTotal += c.a;
                        red += c.r * c.a;
                        green += c.g * c.a;
                        blue += c.b * c.a;
                        weight += c.a;
                        ++samples;
                    }

                Color averaged;
                if (samples > 0) averaged.a = static_cast<uint8_t>(alphaTotal / samples);
                if (weight > 0)
                {
                    averaged.r = static_cast<uint8_t>(red / weight);
                    averaged.g = static_cast<uint8_t>(green / weight);
                    averaged.b = static_cast<uint8_t>(blue / weight);
                }
                out.set(x, y, averaged);
            }
        return out;
    }

    // Takes one frame out of a vertical animation strip.
    Image extractFrame(const Image& strip, int frame, int frameHeight)
    {
        Image out;
        out.width = strip.width;
        out.height = frameHeight;
        out.rgba.resize(static_cast<size_t>(out.width) * out.height * 4);
        for (int y = 0; y < frameHeight; ++y)
            for (int x = 0; x < strip.width; ++x)
                out.set(x, y, strip.at(x, frame * frameHeight + y));
        return out;
    }

    // Minecraft ships grass/foliage textures in greyscale and tints them per
    // biome at runtime. A pack that bakes the colour in should be left alone,
    // hence the saturation test.
    float averageSaturation(const Image& image)
    {
        double total = 0.0;
        int counted = 0;
        for (int y = 0; y < image.height; ++y)
            for (int x = 0; x < image.width; ++x)
            {
                const Color c = image.at(x, y);
                if (c.a < 16) continue;
                const int high = std::max(c.r, std::max(c.g, c.b));
                const int low = std::min(c.r, std::min(c.g, c.b));
                if (high > 0) total += static_cast<double>(high - low) / high;
                ++counted;
            }
        return counted > 0 ? static_cast<float>(total / counted) : 0.0f;
    }

    void tintImage(Image& image, Color tint)
    {
        for (int y = 0; y < image.height; ++y)
            for (int x = 0; x < image.width; ++x)
            {
                Color c = image.at(x, y);
                c.r = static_cast<uint8_t>(c.r * tint.r / 255);
                c.g = static_cast<uint8_t>(c.g * tint.g / 255);
                c.b = static_cast<uint8_t>(c.b * tint.b / 255);
                image.set(x, y, c);
            }
    }

    void compositeOver(Image& base, const Image& overlay)
    {
        if (!overlay.valid() || overlay.width != base.width || overlay.height != base.height) return;
        for (int y = 0; y < base.height; ++y)
            for (int x = 0; x < base.width; ++x)
            {
                const Color top = overlay.at(x, y);
                if (top.a == 0) continue;
                const Color bottom = base.at(x, y);
                const float a = top.a / 255.0f;
                base.set(x, y, Color{
                    clampByte(static_cast<int>(top.r * a + bottom.r * (1.0f - a))),
                    clampByte(static_cast<int>(top.g * a + bottom.g * (1.0f - a))),
                    clampByte(static_cast<int>(top.b * a + bottom.b * (1.0f - a))),
                    bottom.a });
            }
    }

    void setOpacity(Image& image, uint8_t alpha)
    {
        for (int y = 0; y < image.height; ++y)
            for (int x = 0; x < image.width; ++x)
            {
                Color c = image.at(x, y);
                if (c.a > alpha) c.a = alpha;
                image.set(x, y, c);
            }
    }

    // Biome colours Minecraft multiplies its greyscale foliage art by.
    const Color GRASS_TINT{ 145, 189, 89, 255 };
    const Color FOLIAGE_TINT{ 119, 171, 47, 255 };
    const Color BIRCH_TINT{ 128, 167, 85, 255 };
    const Color WATER_TINT{ 63, 118, 228, 255 };

    // --- farming ---------------------------------------------------------

    void paintFarmlandTop(Tile& t)
    {
        // Damp, raked earth: dirt darkened and combed into furrows.
        t.fill(Color{ 94, 66, 42 });
        t.speckle(9, 311);
        for (int y = 0; y < Tile::N; ++y)
            if (y % 4 == 1)
                for (int x = 0; x < Tile::N; ++x) t.set(x, y, shade(t.get(x, y), -22));
    }

    void paintFarmlandSide(Tile& t)
    {
        // The side is plain dirt until the top two rows, which are the
        // tilled surface seen edge-on.
        t.fill(Color{ 122, 86, 54 });
        t.speckle(10, 312);
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < Tile::N; ++x) t.set(x, y, shade(t.get(x, y), -30));
    }

    // One stage of wheat: a row of stalks that start short and green and
    // end tall and golden.
    void paintWheatStage(Tile& t, int stage)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const float ripeness = static_cast<float>(stage) / 7.0f;
        const Color green{ 96, 142, 62 };
        const Color gold{ 206, 182, 86 };
        const Color stalk{
            static_cast<uint8_t>(green.r + (gold.r - green.r) * ripeness),
            static_cast<uint8_t>(green.g + (gold.g - green.g) * ripeness),
            static_cast<uint8_t>(green.b + (gold.b - green.b) * ripeness) };

        const int height = 3 + static_cast<int>(ripeness * 11.0f);
        const int bottom = Tile::N - 1;

        for (int column = 0; column < 4; ++column)
        {
            const int x = 2 + column * 4;
            for (int y = bottom - height; y <= bottom; ++y)
            {
                t.set(x, y, stalk);
                t.set(x + 1, y, shade(stalk, -25));
            }

            // Ears, once it is far enough along to have any.
            if (stage < 4) continue;
            const int ears = stage - 3;
            for (int e = 0; e < ears; ++e)
            {
                const int y = bottom - height + 1 + e * 2;
                t.set(x - 1, y, shade(stalk, 18));
                t.set(x + 2, y + 1, shade(stalk, 18));
            }
        }
    }

    void paintWheat0(Tile& t) { paintWheatStage(t, 0); }
    void paintWheat1(Tile& t) { paintWheatStage(t, 1); }
    void paintWheat2(Tile& t) { paintWheatStage(t, 2); }
    void paintWheat3(Tile& t) { paintWheatStage(t, 3); }
    void paintWheat4(Tile& t) { paintWheatStage(t, 4); }
    void paintWheat5(Tile& t) { paintWheatStage(t, 5); }
    void paintWheat6(Tile& t) { paintWheatStage(t, 6); }
    void paintWheat7(Tile& t) { paintWheatStage(t, 7); }

    // --- items -----------------------------------------------------------
    //
    // Items are sprites on a transparent tile rather than block faces, so
    // they are drawn rather than textured: a shape, a darker outline, and
    // a highlight down one side. At sixteen pixels that is as much as
    // reads anyway.

    // Rounds off a blob by only keeping pixels inside an ellipse.
    void paintBlob(Tile& t, int cx, int cy, int rx, int ry, Color fill, Color edge)
    {
        for (int y = cy - ry; y <= cy + ry; ++y)
            for (int x = cx - rx; x <= cx + rx; ++x)
            {
                const float dx = static_cast<float>(x - cx) / std::max(1, rx);
                const float dy = static_cast<float>(y - cy) / std::max(1, ry);
                const float d = dx * dx + dy * dy;
                if (d > 1.0f) continue;
                t.set(x, y, d > 0.55f ? edge : fill);
            }
    }

    // One stalk of grain: a stem with grains stepped out either side.
    void paintStalk(Tile& t, int x, int top, int bottom, Color stem, Color grain)
    {
        for (int y = top; y <= bottom; ++y) t.set(x, y, stem);
        for (int y = top + 1; y < bottom - 1; y += 2)
        {
            t.set(x - 1, y, grain);
            t.set(x + 1, y + 1, grain);
        }
    }

    void paintItemWheatSeeds(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color seed{ 142, 176, 74 };
        const Color dark{ 104, 134, 54 };
        for (int i = 0; i < 6; ++i)
        {
            const int cx = 4 + static_cast<int>(hash2(i, 0, 401) * 8.0f);
            const int cy = 4 + static_cast<int>(hash2(i, 1, 401) * 8.0f);
            paintBlob(t, cx, cy, 1, 1, seed, dark);
        }
    }

    void paintItemWheat(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color stem{ 150, 128, 48 };
        const Color grain{ 222, 196, 96 };
        paintStalk(t, 5, 3, 13, stem, grain);
        paintStalk(t, 8, 2, 13, stem, grain);
        paintStalk(t, 11, 4, 13, stem, grain);
    }

    void paintItemLeather(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        paintBlob(t, 8, 8, 6, 5, Color{ 166, 118, 72 }, Color{ 124, 84, 50 });
        t.speckle(8, 402);
    }

    // Raw meat: a pale fatty edge around a red middle, with a bone for
    // the cuts that have one.
    void paintMeat(Tile& t, Color flesh, Color fat, bool bone, int salt)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        paintBlob(t, 8, 9, 6, 5, flesh, shade(flesh, -40));
        for (int x = 3; x <= 13; ++x)
            if (hash2(x, 0, salt) > 0.45f) t.set(x, 5 + static_cast<int>(hash2(x, 1, salt) * 2.0f), fat);
        if (bone)
        {
            for (int y = 2; y <= 6; ++y) t.set(8, y, Color{ 232, 228, 212 });
            t.set(7, 2, Color{ 232, 228, 212 });
            t.set(9, 2, Color{ 232, 228, 212 });
        }
        t.speckle(6, salt + 1);
    }

    void paintItemRawBeef(Tile& t) { paintMeat(t, Color{ 186, 70, 64 }, Color{ 226, 182, 170 }, false, 410); }
    void paintItemRawPork(Tile& t) { paintMeat(t, Color{ 226, 140, 136 }, Color{ 242, 212, 206 }, false, 420); }
    void paintItemRawChicken(Tile& t) { paintMeat(t, Color{ 226, 176, 150 }, Color{ 240, 216, 196 }, true, 430); }
    void paintItemRawMutton(Tile& t) { paintMeat(t, Color{ 198, 96, 86 }, Color{ 232, 196, 184 }, true, 440); }

    void paintItemFeather(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color quill{ 196, 190, 176 };
        const Color vane{ 240, 240, 236 };
        for (int i = 0; i < 11; ++i)
        {
            const int x = 4 + i;
            const int y = 12 - i;
            t.set(x, y, quill);
            if (i > 1 && i < 9)
            {
                t.set(x - 1, y, vane);
                t.set(x, y + 1, vane);
                if (i % 2 == 0) t.set(x - 2, y + 1, vane);
            }
        }
    }

    void paintItemBone(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color bone{ 238, 236, 222 };
        const Color edge{ 198, 194, 178 };
        for (int i = 3; i <= 12; ++i) { t.set(i, 8, bone); t.set(i, 9, edge); }
        for (int end = 0; end < 2; ++end)
        {
            const int x = end == 0 ? 2 : 13;
            paintBlob(t, x, 7, 1, 1, bone, edge);
            paintBlob(t, x, 10, 1, 1, bone, edge);
        }
    }

    void paintItemString(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color thread{ 236, 236, 236 };
        for (int y = 2; y <= 13; ++y)
        {
            const int x = 8 + static_cast<int>(std::sin(y * 0.9f) * 3.0f);
            t.set(x, y, thread);
            t.set(x + 1, y, shade(thread, -40));
        }
    }

    void paintItemGunpowder(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        paintBlob(t, 8, 9, 5, 4, Color{ 108, 108, 108 }, Color{ 72, 72, 72 });
        for (int i = 0; i < 10; ++i)
        {
            const int x = 4 + static_cast<int>(hash2(i, 0, 450) * 9.0f);
            const int y = 6 + static_cast<int>(hash2(i, 1, 450) * 7.0f);
            t.set(x, y, Color{ 54, 54, 54 });
        }
    }

    struct TileEntry
    {
        int index;
        TileFn fn;
    };

    const TileEntry TILE_PAINTERS[] = {
        { Tiles::Blank, paintBlank },
        { Tiles::Stone, paintStone },
        { Tiles::Dirt, paintDirt },
        { Tiles::GrassSide, paintGrassSide },
        { Tiles::GrassTop, paintGrassTop },
        { Tiles::Cobblestone, paintCobble },
        { Tiles::Planks, paintPlanks },
        { Tiles::Bedrock, paintBedrock },
        { Tiles::Water, paintWater },
        { Tiles::Sand, paintSand },
        { Tiles::Gravel, paintGravel },
        { Tiles::GoldOre, paintGoldOre },
        { Tiles::IronOre, paintIronOre },
        { Tiles::CoalOre, paintCoalOre },
        { Tiles::DiamondOre, paintDiamondOre },
        { Tiles::LogSide, paintLogSide },
        { Tiles::LogTop, paintLogTop },
        { Tiles::Leaves, paintLeaves },
        { Tiles::Glass, paintGlass },
        { Tiles::SandstoneSide, paintSandstoneSide },
        { Tiles::SandstoneTop, paintSandstoneTop },
        { Tiles::SnowTile, paintSnow },
        { Tiles::Ice, paintIce },
        { Tiles::CactusSide, paintCactusSide },
        { Tiles::CactusTop, paintCactusTop },
        { Tiles::Bricks, paintBricks },
        { Tiles::Obsidian, paintObsidian },
        { Tiles::TallGrass, paintTallGrass },
        { Tiles::FlowerRed, paintFlowerRed },
        { Tiles::FlowerYellow, paintFlowerYellow },
        { Tiles::MossyCobble, paintMossyCobble },
        { Tiles::Clay, paintClay },
        { Tiles::PumpkinSide, paintPumpkinSide },
        { Tiles::PumpkinTop, paintPumpkinTop },
        { Tiles::Wool, paintWool },
        { Tiles::FarmlandTop, paintFarmlandTop },
        { Tiles::FarmlandSide, paintFarmlandSide },
        { Tiles::WheatStage0, paintWheat0 },
        { Tiles::WheatStage1, paintWheat1 },
        { Tiles::WheatStage2, paintWheat2 },
        { Tiles::WheatStage3, paintWheat3 },
        { Tiles::WheatStage4, paintWheat4 },
        { Tiles::WheatStage5, paintWheat5 },
        { Tiles::WheatStage6, paintWheat6 },
        { Tiles::WheatStage7, paintWheat7 },
        { Tiles::ItemWheatSeeds, paintItemWheatSeeds },
        { Tiles::ItemWheat, paintItemWheat },
        { Tiles::ItemLeather, paintItemLeather },
        { Tiles::ItemRawBeef, paintItemRawBeef },
        { Tiles::ItemRawPork, paintItemRawPork },
        { Tiles::ItemRawChicken, paintItemRawChicken },
        { Tiles::ItemRawMutton, paintItemRawMutton },
        { Tiles::ItemFeather, paintItemFeather },
        { Tiles::ItemBone, paintItemBone },
        { Tiles::ItemString, paintItemString },
        { Tiles::ItemGunpowder, paintItemGunpowder },
        { Tiles::Torch, paintTorch },
        { Tiles::BirchLogSide, paintBirchLogSide },
        { Tiles::BirchLogTop, paintBirchLogTop },
        { Tiles::BirchLeaves, paintBirchLeaves },
        { Tiles::SnowGrassSide, paintSnowGrassSide },
        { Tiles::Lava, paintLava },
        { Tiles::Glowstone, paintGlowstone },
        { Tiles::Netherrack, paintNetherrack },
        { Tiles::SoulSand, paintSoulSand },
        { Tiles::EndStone, paintEndStone },
        { Tiles::NetherPortal, paintNetherPortal },
    };

    enum class Tinting
    {
        None,
        Grass,   // greyscale art multiplied by the biome grass colour
        Foliage,
        Birch,
        Water
    };

    struct PackTile
    {
        int tile;
        // Vanilla file names, newest first; older packs use the later ones.
        const char* candidates[4];
        Tinting tint;
        uint8_t maxAlpha;     // 0 = keep the texture's own alpha
        bool animated;        // vertical strip of frames
    };

    const PackTile PACK_TILES[] = {
        { Tiles::Stone,         { "stone", nullptr },                                  Tinting::None,    0,   false },
        { Tiles::Dirt,          { "dirt", nullptr },                                   Tinting::None,    0,   false },
        { Tiles::GrassSide,     { "grass_block_side", "grass_side", nullptr },         Tinting::None,    0,   false },
        { Tiles::GrassTop,      { "grass_block_top", "grass_top", nullptr },           Tinting::Grass,   0,   false },
        { Tiles::Cobblestone,   { "cobblestone", nullptr },                            Tinting::None,    0,   false },
        { Tiles::Planks,        { "oak_planks", "planks_oak", nullptr },               Tinting::None,    0,   false },
        { Tiles::Bedrock,       { "bedrock", nullptr },                                Tinting::None,    0,   false },
        { Tiles::Water,         { "water_still", nullptr },                            Tinting::Water, 200,   true  },
        { Tiles::Sand,          { "sand", nullptr },                                   Tinting::None,    0,   false },
        { Tiles::Gravel,        { "gravel", nullptr },                                 Tinting::None,    0,   false },
        { Tiles::GoldOre,       { "gold_ore", nullptr },                               Tinting::None,    0,   false },
        { Tiles::IronOre,       { "iron_ore", nullptr },                               Tinting::None,    0,   false },
        { Tiles::CoalOre,       { "coal_ore", nullptr },                               Tinting::None,    0,   false },
        { Tiles::DiamondOre,    { "diamond_ore", nullptr },                            Tinting::None,    0,   false },
        { Tiles::LogSide,       { "oak_log", "log_oak", nullptr },                     Tinting::None,    0,   false },
        { Tiles::LogTop,        { "oak_log_top", "log_oak_top", nullptr },             Tinting::None,    0,   false },
        { Tiles::Leaves,        { "oak_leaves", "leaves_oak", nullptr },               Tinting::Foliage, 0,   false },
        { Tiles::Glass,         { "glass", nullptr },                                  Tinting::None,    0,   false },
        { Tiles::SandstoneSide, { "sandstone", "sandstone_normal", nullptr },          Tinting::None,    0,   false },
        { Tiles::SandstoneTop,  { "sandstone_top", nullptr },                          Tinting::None,    0,   false },
        { Tiles::SnowTile,      { "snow", nullptr },                                   Tinting::None,    0,   false },
        { Tiles::Ice,           { "ice", nullptr },                                    Tinting::None,  210,   false },
        { Tiles::CactusSide,    { "cactus_side", nullptr },                            Tinting::None,    0,   false },
        { Tiles::CactusTop,     { "cactus_top", nullptr },                             Tinting::None,    0,   false },
        { Tiles::Bricks,        { "bricks", "brick", nullptr },                        Tinting::None,    0,   false },
        { Tiles::Obsidian,      { "obsidian", nullptr },                               Tinting::None,    0,   false },
        { Tiles::TallGrass,     { "short_grass", "grass", "tallgrass", nullptr },      Tinting::Grass,   0,   false },
        { Tiles::FlowerRed,     { "poppy", "flower_rose", nullptr },                   Tinting::None,    0,   false },
        { Tiles::FlowerYellow,  { "dandelion", "flower_dandelion", nullptr },          Tinting::None,    0,   false },
        { Tiles::MossyCobble,   { "mossy_cobblestone", "cobblestone_mossy", nullptr }, Tinting::None,    0,   false },
        { Tiles::Clay,          { "clay", nullptr },                                   Tinting::None,    0,   false },
        { Tiles::PumpkinSide,   { "pumpkin_side", nullptr },                           Tinting::None,    0,   false },
        { Tiles::PumpkinTop,    { "pumpkin_top", nullptr },                            Tinting::None,    0,   false },
        { Tiles::Wool,          { "white_wool", "wool_colored_white", nullptr },       Tinting::None,    0,   false },
        { Tiles::Torch,         { "torch", "torch_on", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BirchLogSide,  { "birch_log", "log_birch", nullptr },                 Tinting::None,    0,   false },
        { Tiles::BirchLogTop,   { "birch_log_top", "log_birch_top", nullptr },         Tinting::None,    0,   false },
        { Tiles::BirchLeaves,   { "birch_leaves", "leaves_birch", nullptr },           Tinting::Birch,   0,   false },
        { Tiles::SnowGrassSide, { "grass_block_snow", "grass_side_snowed", nullptr },  Tinting::None,    0,   false },
        { Tiles::Lava,          { "lava_still", nullptr },                             Tinting::None,    0,   true  },
        { Tiles::Glowstone,     { "glowstone", nullptr },                              Tinting::None,    0,   false },
    };

    void applyTint(Image& image, Tinting tinting)
    {
        if (tinting == Tinting::None) return;
        // Leave packs that already baked the colour in alone.
        if (averageSaturation(image) > 0.18f) return;

        switch (tinting)
        {
            case Tinting::Grass:   tintImage(image, GRASS_TINT); break;
            case Tinting::Foliage: tintImage(image, FOLIAGE_TINT); break;
            case Tinting::Birch:   tintImage(image, BIRCH_TINT); break;
            case Tinting::Water:   tintImage(image, WATER_TINT); break;
            default: break;
        }
    }
}

Atlas::Atlas()
{
    // Tile size follows the pack: load one reference texture and match it.
    // Without a pack we stay at the procedural 16x16.
    {
        const char* reference[] = { "stone", nullptr };
        const Image probe = loadFirstAvailable(reference);
        if (probe.valid())
        {
            // Cap the tile size so a very high-resolution pack can't blow up
            // VRAM: the atlas is 16 tiles square per layer, times three
            // layers, so 256px tiles already means three 4096x4096 textures.
            m_tilePixels = std::clamp(probe.width, 8, MAX_TILE_PIXELS);
            m_loadedFromPack = 1;
        }
    }

    g_atlasPixelSize = TILES_PER_ROW * m_tilePixels;
    const int atlasSize = g_atlasPixelSize;
    const int tilePixels = m_tilePixels;

    std::vector<Color> pixels(static_cast<size_t>(atlasSize) * atlasSize, Color{ 0, 0, 0, 0 });
    // LabPBR defaults for anything without maps: flat normal pointing
    // straight out, full ambient occlusion, no smoothness, no emission
    // (alpha 255 means "not emissive" in the LabPBR spec).
    std::vector<Color> normalPixels(static_cast<size_t>(atlasSize) * atlasSize, Color{ 128, 128, 255, 255 });
    std::vector<Color> specularPixels(static_cast<size_t>(atlasSize) * atlasSize, Color{ 0, 0, 0, 255 });

    auto blitInto = [&](std::vector<Color>& target, int tileIndex, const Image& image) {
        const int col = tileIndex % TILES_PER_ROW;
        const int row = tileIndex / TILES_PER_ROW;
        for (int y = 0; y < tilePixels; ++y)
            for (int x = 0; x < tilePixels; ++x)
                target[static_cast<size_t>(row * tilePixels + y) * atlasSize + (col * tilePixels + x)] =
                    image.at(x, y);
    };

    auto blitImage = [&](int tileIndex, const Image& image) { blitInto(pixels, tileIndex, image); };

    // Procedural tiles are painted at 16x16, then scaled up to match.
    auto blitPainted = [&](int tileIndex, const Tile& tile) {
        Image image;
        image.width = Tile::N;
        image.height = Tile::N;
        image.rgba.resize(static_cast<size_t>(Tile::N) * Tile::N * 4);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                image.set(x, y, tile.get(x, y));
        blitImage(tileIndex, scaleNearest(image, tilePixels, tilePixels));
    };

    // Start from the procedural set so anything the pack is missing still
    // has a sensible texture.
    for (const TileEntry& entry : TILE_PAINTERS)
    {
        Tile tile;
        entry.fn(tile);
        blitPainted(entry.index, tile);
    }
    for (int stage = 0; stage < Tiles::CrackStages; ++stage)
    {
        Tile tile;
        paintCrack(tile, stage);
        blitPainted(Tiles::CrackFirst + stage, tile);
    }

    if (m_loadedFromPack)
    {
        int loaded = 0;

        // Which PBR dialect the pack speaks. Decided once: the Bedrock
        // suffixes collide with real Java texture names ("sandstone_normal"
        // is a block, not a normal map), so they are only trusted when the
        // pack has already identified itself as Bedrock.
        const bool bedrockPack = packIsBedrockStyle();

        for (const PackTile& entry : PACK_TILES)
        {
            Image image = loadFirstAvailable(entry.candidates);
            if (!image.valid()) continue;

            int frameCount = 1;
            if (entry.animated && image.height > image.width && image.width > 0 &&
                image.height % image.width == 0)
            {
                frameCount = image.height / image.width;
            }

            const int frameHeight = (frameCount > 1) ? image.height / frameCount : image.height;

            auto prepare = [&](const Image& raw) {
                Image frame = scaleNearest(raw, tilePixels, tilePixels);
                applyTint(frame, entry.tint);
                if (entry.maxAlpha > 0) setOpacity(frame, entry.maxAlpha);
                return frame;
            };

            if (frameCount > 1)
            {
                Animation animation;
                animation.tile = entry.tile;
                animation.frameCount = frameCount;
                animation.frameSeconds = (entry.tile == Tiles::Lava) ? 0.15f : 0.08f;
                animation.frames.resize(static_cast<size_t>(frameCount) * tilePixels * tilePixels * 4);

                for (int f = 0; f < frameCount; ++f)
                {
                    const Image frame = prepare(extractFrame(image, f, frameHeight));
                    std::copy(frame.rgba.begin(), frame.rgba.end(),
                              animation.frames.begin() + static_cast<size_t>(f) * tilePixels * tilePixels * 4);
                    if (f == 0) blitImage(entry.tile, frame);
                }
                m_animations.push_back(std::move(animation));
            }
            else
            {
                Image frame = prepare(image);

                // Grass sides ship as plain dirt plus a greyscale overlay
                // that gets the biome colour; composite them here.
                if (entry.tile == Tiles::GrassSide)
                {
                    const char* overlayNames[] = { "grass_block_side_overlay", "grass_side_overlay", nullptr };
                    Image overlay = loadFirstAvailable(overlayNames);
                    if (overlay.valid())
                    {
                        overlay = scaleNearest(overlay, tilePixels, tilePixels);
                        applyTint(overlay, Tinting::Grass);
                        compositeOver(frame, overlay);
                    }
                }

                blitImage(entry.tile, frame);
            }
            ++loaded;

            // LabPBR companions: "<name>_n" holds the normal map (plus AO and
            // a height map), "<name>_s" holds smoothness, reflectance and
            // emission. Minecraft itself ignores these without a shader pack;
            // our chunk shader reads them directly.
            bool foundMaps = false;
            for (int i = 0; entry.candidates[i] != nullptr; ++i)
            {
                const std::string base = entry.candidates[i];

                const Image normalMap = loadPNG(std::string(PACK_DIRECTORY) + "/" + base + "_n.png");
                if (normalMap.valid())
                {
                    const Image source = (frameCount > 1) ? extractFrame(normalMap, 0, normalMap.height / frameCount)
                                                          : normalMap;
                    blitInto(normalPixels, entry.tile, scaleNearest(source, tilePixels, tilePixels));
                    m_hasPbrMaps = true;
                }

                const Image specularMap = loadPNG(std::string(PACK_DIRECTORY) + "/" + base + "_s.png");
                if (specularMap.valid())
                {
                    const Image source = (frameCount > 1) ? extractFrame(specularMap, 0, specularMap.height / frameCount)
                                                          : specularMap;
                    blitInto(specularPixels, entry.tile, scaleNearest(source, tilePixels, tilePixels));
                    m_hasPbrMaps = true;
                }

                if (normalMap.valid() || specularMap.valid()) { foundMaps = true; break; }
            }

            // Bedrock texture sets, converted to LabPBR on the way in.
            for (int i = 0; !foundMaps && bedrockPack && entry.candidates[i] != nullptr; ++i)
            {
                const std::string base = std::string(PACK_DIRECTORY) + "/" + entry.candidates[i];

                auto firstFrame = [&](const Image& map) {
                    return (frameCount > 1 && map.height >= frameCount)
                               ? extractFrame(map, 0, map.height / frameCount)
                               : map;
                };

                Image heightmap = loadPNG(base + "_heightmap.png");
                if (!heightmap.valid()) heightmap = loadPNG(base + "_height.png");
                if (heightmap.valid()) heightmap = firstFrame(heightmap);

                Image normalMap = loadPNG(base + "_normal.png");
                if (normalMap.valid())
                {
                    normalMap = bedrockNormalToLab(firstFrame(normalMap), heightmap);
                }
                else if (heightmap.valid())
                {
                    normalMap = heightmapToLabNormal(heightmap, 4.0f);
                }

                Image mer = loadPNG(base + "_mer.png");
                if (!mer.valid()) mer = loadPNG(base + "_mers.png");
                if (mer.valid()) mer = merToLabSpecular(firstFrame(mer));

                if (normalMap.valid())
                {
                    blitInto(normalPixels, entry.tile, scaleNearest(normalMap, tilePixels, tilePixels));
                    m_hasPbrMaps = true;
                    m_bedrockMaps = true;
                }
                if (mer.valid())
                {
                    blitInto(specularPixels, entry.tile, scaleNearest(mer, tilePixels, tilePixels));
                    m_hasPbrMaps = true;
                    m_bedrockMaps = true;
                }

                if (normalMap.valid() || mer.valid()) break;
            }
        }

        // Mining crack overlays.
        for (int stage = 0; stage < Tiles::CrackStages; ++stage)
        {
            const std::string name = "destroy_stage_" + std::to_string(stage);
            const char* candidates[] = { name.c_str(), nullptr };
            Image image = loadFirstAvailable(candidates);
            if (!image.valid()) continue;
            blitImage(Tiles::CrackFirst + stage, scaleNearest(image, tilePixels, tilePixels));
        }

        m_packCredit = readPackCredit();
        if (!m_packCredit.empty())
            std::printf("Textures: pack credited as \"%s\"\n", m_packCredit.c_str());
        else
            std::printf("Textures: no %s, so nothing is credited on the title screen\n", CREDIT_FILE);

        std::printf("Textures: loaded %d of %d block textures from %s (%dpx tiles)%s\n",
                    loaded, static_cast<int>(std::size(PACK_TILES)), PACK_DIRECTORY, tilePixels,
                    !m_hasPbrMaps      ? ""
                    : m_bedrockMaps      ? ", with Bedrock texture-set maps converted to LabPBR"
                                         : ", with LabPBR normal/specular maps");
    }
    else
    {
        std::printf("Textures: no pack in %s, using built-in procedural textures\n", PACK_DIRECTORY);
    }

    // Mip levels are built per tile: each tile is shrunk using only its own
    // pixels, so neighbouring tiles can never bleed into one another the way
    // they would with glGenerateMipmap on an atlas. Without this, distant
    // terrain shimmers badly -- and the higher the pack's resolution, the
    // worse it gets.
    auto downsamplePerTile = [](const std::vector<Color>& source, int sourceTilePixels) {
        const int destTilePixels = sourceTilePixels / 2;
        const int sourceAtlas = sourceTilePixels * Atlas::TILES_PER_ROW;
        const int destAtlas = destTilePixels * Atlas::TILES_PER_ROW;
        std::vector<Color> dest(static_cast<size_t>(destAtlas) * destAtlas);

        for (int tileY = 0; tileY < Atlas::TILES_PER_ROW; ++tileY)
            for (int tileX = 0; tileX < Atlas::TILES_PER_ROW; ++tileX)
                for (int y = 0; y < destTilePixels; ++y)
                    for (int x = 0; x < destTilePixels; ++x)
                    {
                        const int sx = tileX * sourceTilePixels + x * 2;
                        const int sy = tileY * sourceTilePixels + y * 2;

                        int alphaTotal = 0, red = 0, green = 0, blue = 0, weight = 0;
                        for (int dy = 0; dy < 2; ++dy)
                            for (int dx = 0; dx < 2; ++dx)
                            {
                                const Color c = source[static_cast<size_t>(sy + dy) * sourceAtlas + (sx + dx)];
                                alphaTotal += c.a;
                                // Weight colour by alpha so transparent pixels
                                // don't drag a dark halo into the average.
                                red += c.r * c.a;
                                green += c.g * c.a;
                                blue += c.b * c.a;
                                weight += c.a;
                            }

                        Color out;
                        out.a = static_cast<uint8_t>(alphaTotal / 4);
                        if (weight > 0)
                        {
                            out.r = static_cast<uint8_t>(red / weight);
                            out.g = static_cast<uint8_t>(green / weight);
                            out.b = static_cast<uint8_t>(blue / weight);
                        }
                        dest[static_cast<size_t>(tileY * destTilePixels + y) * destAtlas +
                             (tileX * destTilePixels + x)] = out;
                    }
        return dest;
    };

    // Shrinking a cut-out texture averages its alpha, so a leaf tile that
    // was 60% solid drifts towards fully solid (or fully gone) in the lower
    // mips and distant trees stop looking like foliage. Rescale each tile's
    // alpha per level so the share of texels passing the shader's cut-off
    // stays the same as at full resolution.
    constexpr int ALPHA_CUTOFF = 26; // matches the `texel.a < 0.1` discard

    auto tileCoverage = [ALPHA_CUTOFF](const std::vector<Color>& level, int levelTilePixels, int tile, float alphaScale) {
        const int levelAtlas = levelTilePixels * Atlas::TILES_PER_ROW;
        const int originX = (tile % Atlas::TILES_PER_ROW) * levelTilePixels;
        const int originY = (tile / Atlas::TILES_PER_ROW) * levelTilePixels;

        int passing = 0;
        for (int y = 0; y < levelTilePixels; ++y)
            for (int x = 0; x < levelTilePixels; ++x)
            {
                const int alpha = static_cast<int>(
                    level[static_cast<size_t>(originY + y) * levelAtlas + (originX + x)].a * alphaScale);
                if (alpha >= ALPHA_CUTOFF) ++passing;
            }
        return static_cast<float>(passing) / static_cast<float>(levelTilePixels * levelTilePixels);
    };

    auto restoreCoverage = [&](std::vector<Color>& level, int levelTilePixels, const std::vector<float>& targets) {
        const int levelAtlas = levelTilePixels * Atlas::TILES_PER_ROW;

        for (int tile = 0; tile < Atlas::TILES_PER_ROW * Atlas::TILES_PER_ROW; ++tile)
        {
            const float target = targets[tile];
            if (target <= 0.001f || target >= 0.999f) continue; // fully clear or fully solid

            // Binary search the alpha multiplier that reproduces the target.
            float low = 0.25f, high = 6.0f, scale = 1.0f;
            for (int step = 0; step < 12; ++step)
            {
                scale = (low + high) * 0.5f;
                if (tileCoverage(level, levelTilePixels, tile, scale) < target) low = scale;
                else high = scale;
            }

            const int originX = (tile % Atlas::TILES_PER_ROW) * levelTilePixels;
            const int originY = (tile / Atlas::TILES_PER_ROW) * levelTilePixels;
            for (int y = 0; y < levelTilePixels; ++y)
                for (int x = 0; x < levelTilePixels; ++x)
                {
                    Color& texel = level[static_cast<size_t>(originY + y) * levelAtlas + (originX + x)];
                    texel.a = clampByte(static_cast<int>(texel.a * scale));
                }
        }
    };

    auto createTexture = [&](unsigned int& id, const std::vector<Color>& data, bool preserveCoverage) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Nearest magnification keeps blocks crisp up close; mipmapped
        // minification stops distant terrain from fizzing.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasSize, atlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

        std::vector<float> coverageTargets;
        if (preserveCoverage)
        {
            coverageTargets.resize(TILES_PER_ROW * TILES_PER_ROW);
            for (int tile = 0; tile < TILES_PER_ROW * TILES_PER_ROW; ++tile)
                coverageTargets[tile] = tileCoverage(data, tilePixels, tile, 1.0f);
        }

        std::vector<Color> level = data;
        int levelTilePixels = tilePixels;
        int levelIndex = 0;
        while (levelTilePixels > 1)
        {
            level = downsamplePerTile(level, levelTilePixels);
            levelTilePixels /= 2;
            ++levelIndex;
            if (preserveCoverage) restoreCoverage(level, levelTilePixels, coverageTargets);
            const int levelSize = levelTilePixels * TILES_PER_ROW;
            glTexImage2D(GL_TEXTURE_2D, levelIndex, GL_RGBA8, levelSize, levelSize, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, level.data());
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levelIndex);

        glBindTexture(GL_TEXTURE_2D, 0);
    };

    // Only the albedo atlas is alpha-tested, so only it needs its cut-out
    // coverage protecting through the mip chain.
    createTexture(m_id, pixels, true);
    createTexture(m_normalId, normalPixels, false);
    createTexture(m_specularId, specularPixels, false);
}

Atlas::~Atlas()
{
    if (m_id) glDeleteTextures(1, &m_id);
    if (m_normalId) glDeleteTextures(1, &m_normalId);
    if (m_specularId) glDeleteTextures(1, &m_specularId);
}

void Atlas::uploadTile(int tile, const uint8_t* rgba)
{
    const int col = tile % TILES_PER_ROW;
    const int row = tile / TILES_PER_ROW;
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    col * m_tilePixels, row * m_tilePixels,
                    m_tilePixels, m_tilePixels,
                    GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Atlas::update(float deltaTime)
{
    const size_t frameBytes = static_cast<size_t>(m_tilePixels) * m_tilePixels * 4;

    for (Animation& animation : m_animations)
    {
        animation.timer += deltaTime;
        if (animation.timer < animation.frameSeconds) continue;

        animation.timer = 0.0f;
        animation.current = (animation.current + 1) % animation.frameCount;
        uploadTile(animation.tile, animation.frames.data() + animation.current * frameBytes);
    }
}

void Atlas::bind(unsigned int albedoUnit, unsigned int normalUnit, unsigned int specularUnit) const
{
    glActiveTexture(GL_TEXTURE0 + albedoUnit);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glActiveTexture(GL_TEXTURE0 + normalUnit);
    glBindTexture(GL_TEXTURE_2D, m_normalId);
    glActiveTexture(GL_TEXTURE0 + specularUnit);
    glBindTexture(GL_TEXTURE_2D, m_specularId);
    glActiveTexture(GL_TEXTURE0);
}
