#include "Atlas.h"
#include "stb_image.h"
#include <cstdio>
#include <string>
#include <cstring>
#include "Core/GLFunctions.h"
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

TileUV tileUV(int tileIndex)
{
    constexpr float step = 1.0f / Atlas::TILES_PER_ROW;
    // Half-texel inset stops neighbouring tiles bleeding in at the seams.
    constexpr float inset = 0.5f / Atlas::SIZE;

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
        static constexpr int N = Atlas::TILE_PIXELS;

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

    struct TileEntry
    {
        int index;
        TileFn fn;
    };

    void paintNetherrack(Tile& t)
    {
        t.fill(Color{ 97, 38, 38 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float n = hash2(x, y, 307);
                if (n > 0.78f) t.set(x, y, Color{ 126, 52, 52 });
                else if (n < 0.2f) t.set(x, y, Color{ 72, 26, 28 });
            }
    }

    void paintSoulSand(Tile& t)
    {
        t.fill(Color{ 82, 62, 51 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 311) > 0.72f)
                    t.set(x, y, Color{ 62, 45, 37 });
    }

    void paintEndStone(Tile& t)
    {
        t.fill(Color{ 221, 223, 165 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float n = hash2(x, y, 313);
                if (n > 0.8f) t.set(x, y, Color{ 234, 236, 180 });
                else if (n < 0.18f) t.set(x, y, Color{ 198, 200, 144 });
            }
    }

    void paintNetherPortal(Tile& t)
    {
        // Swirling violet: a couple of offset sine bands plus noise reads as
        // movement even though the tile itself does not animate.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float fx = static_cast<float>(x) / Tile::N;
                const float fy = static_cast<float>(y) / Tile::N;
                const float swirl = std::sin(fx * 9.0f + fy * 5.0f) * 0.5f + 0.5f;
                const float grain = hash2(x, y, 317);
                const int level = static_cast<int>(70.0f + swirl * 90.0f + grain * 40.0f);
                t.set(x, y, Color{ clampByte(level / 2 + 40), clampByte(level / 5), clampByte(level + 60), 190 });
            }
    }

    const TileEntry TILE_PAINTERS[] = {
        { Tiles::Blank, paintBlank },
        { Tiles::Netherrack, paintNetherrack },
        { Tiles::SoulSand, paintSoulSand },
        { Tiles::EndStone, paintEndStone },
        { Tiles::NetherPortal, paintNetherPortal },
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
        { Tiles::Torch, paintTorch },
        { Tiles::BirchLogSide, paintBirchLogSide },
        { Tiles::BirchLogTop, paintBirchLogTop },
        { Tiles::BirchLeaves, paintBirchLeaves },
        { Tiles::SnowGrassSide, paintSnowGrassSide },
        { Tiles::Lava, paintLava },
        { Tiles::Glowstone, paintGlowstone },
    };
}


namespace
{
    // Vanilla Minecraft texture names for each atlas tile. A tile with no name
    // here keeps whatever the procedural painter produced.
    const char* packFileForTile(int tile)
    {
        switch (tile)
        {
            case Tiles::Stone:          return "stone";
            case Tiles::Dirt:           return "dirt";
            case Tiles::GrassSide:      return "grass_block_side";
            case Tiles::GrassTop:       return "grass_block_top";
            case Tiles::Cobblestone:    return "cobblestone";
            case Tiles::Planks:         return "oak_planks";
            case Tiles::Bedrock:        return "bedrock";
            case Tiles::Water:          return "water_still";
            case Tiles::Sand:           return "sand";
            case Tiles::Gravel:         return "gravel";
            case Tiles::GoldOre:        return "gold_ore";
            case Tiles::IronOre:        return "iron_ore";
            case Tiles::CoalOre:        return "coal_ore";
            case Tiles::DiamondOre:     return "diamond_ore";
            case Tiles::LogSide:        return "oak_log";
            case Tiles::LogTop:         return "oak_log_top";
            case Tiles::Leaves:         return "oak_leaves";
            case Tiles::Glass:          return "glass";
            case Tiles::SandstoneSide:  return "sandstone";
            case Tiles::SandstoneTop:   return "sandstone_top";
            case Tiles::SnowTile:       return "snow";
            case Tiles::Ice:            return "ice";
            case Tiles::CactusSide:     return "cactus_side";
            case Tiles::CactusTop:      return "cactus_top";
            case Tiles::Bricks:         return "bricks";
            case Tiles::Obsidian:       return "obsidian";
            case Tiles::TallGrass:      return "short_grass";
            case Tiles::FlowerRed:      return "poppy";
            case Tiles::FlowerYellow:   return "dandelion";
            case Tiles::MossyCobble:    return "mossy_cobblestone";
            case Tiles::Clay:           return "clay";
            case Tiles::PumpkinSide:    return "pumpkin_side";
            case Tiles::PumpkinTop:     return "pumpkin_top";
            case Tiles::Wool:           return "white_wool";
            case Tiles::Torch:          return "torch";
            case Tiles::BirchLogSide:   return "birch_log";
            case Tiles::BirchLogTop:    return "birch_log_top";
            case Tiles::BirchLeaves:    return "birch_leaves";
            case Tiles::SnowGrassSide:  return "grass_block_snow";
            case Tiles::Lava:           return "lava_still";
            case Tiles::Glowstone:      return "glowstone";
            case Tiles::Netherrack:     return "netherrack";
            case Tiles::SoulSand:       return "soul_sand";
            case Tiles::EndStone:       return "end_stone";
            default: break;
        }

        if (tile >= Tiles::CrackFirst && tile < Tiles::CrackFirst + Tiles::CrackStages)
        {
            static char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "destroy_stage_%d", tile - Tiles::CrackFirst);
            return buffer;
        }

        return nullptr;
    }

    // Grass and foliage ship greyscale in a resource pack and are tinted by
    // biome colour when drawn. Nothing here knows about biomes, so bake a
    // temperate tint in at load time; untinted they come out stone grey.
    bool foliageTint(int tile, float& r, float& g, float& b)
    {
        switch (tile)
        {
            case Tiles::GrassTop:
            case Tiles::TallGrass:
                r = 0.57f; g = 0.74f; b = 0.35f; return true;
            case Tiles::Leaves:
                r = 0.42f; g = 0.63f; b = 0.20f; return true;
            case Tiles::BirchLeaves:
                r = 0.50f; g = 0.66f; b = 0.29f; return true;
            default:
                return false;
        }
    }

    std::string packPath(const std::string& name)
    {
        return "assets/textures/block/" + name + ".png";
    }

    // Uploads one RGBA atlas and returns its texture name.
    unsigned int uploadAtlas(const uint8_t* rgba, int pixels)
    {
        unsigned int id = 0;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pixels, pixels, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glBindTexture(GL_TEXTURE_2D, 0);
        return id;
    }
}

int Atlas::detectPackTileSize()
{
    // Every tile in a pack shares one resolution, so the first readable file
    // decides the atlas size. Animation strips are square frames stacked
    // vertically, so the width is the tile size either way.
    for (int tile = 0; tile < Tiles::TileCount; ++tile)
    {
        const char* name = packFileForTile(tile);
        if (!name) continue;

        int w = 0, h = 0, channels = 0;
        if (stbi_info(packPath(name).c_str(), &w, &h, &channels) && w > 0)
            return w;
    }
    return 0;
}

int Atlas::loadPackTiles(uint8_t* atlasRgba, int atlasPixels, const char* suffix)
{
    int loaded = 0;
    const int tilePixels = m_tilePixels;
    const int frameBytes = tilePixels * tilePixels * 4;

    for (int tile = 0; tile < TILES_PER_ROW * TILES_PER_ROW; ++tile)
    {
        const char* name = packFileForTile(tile);
        if (!name) continue;

        int w = 0, h = 0, channels = 0;
        uint8_t* data = stbi_load(packPath(std::string(name) + suffix).c_str(), &w, &h, &channels, 4);
        if (!data) continue;

        if (w != tilePixels || h < tilePixels)
        {
            stbi_image_free(data);
            continue;
        }

        // Albedo only: a normal or specular map must not be tinted.
        float tr = 1.0f, tg = 1.0f, tb = 1.0f;
        const bool tinted = (*suffix == '\0') && foliageTint(tile, tr, tg, tb);
        if (tinted)
        {
            for (int i = 0; i < w * h; ++i)
            {
                data[i * 4 + 0] = static_cast<uint8_t>(data[i * 4 + 0] * tr);
                data[i * 4 + 1] = static_cast<uint8_t>(data[i * 4 + 1] * tg);
                data[i * 4 + 2] = static_cast<uint8_t>(data[i * 4 + 2] * tb);
            }
        }

        const int col = tile % TILES_PER_ROW;
        const int row = tile / TILES_PER_ROW;

        // Copy the first frame into the atlas.
        for (int y = 0; y < tilePixels; ++y)
        {
            uint8_t* dst = atlasRgba + (static_cast<size_t>(row * tilePixels + y) * atlasPixels + col * tilePixels) * 4;
            std::memcpy(dst, data + static_cast<size_t>(y) * w * 4, static_cast<size_t>(tilePixels) * 4);
        }

        // Taller than one tile means an animation strip; keep the frames so
        // update() can cycle them.
        const int frames = h / tilePixels;
        if (frames > 1)
        {
            Animation animation;
            animation.tile = tile;
            animation.frameCount = frames;
            animation.frameSeconds = 0.1f; // Minecraft's default two ticks
            animation.frames.resize(static_cast<size_t>(frames) * frameBytes);
            std::memcpy(animation.frames.data(), data, animation.frames.size());
            m_animations.push_back(std::move(animation));
        }

        stbi_image_free(data);
        ++loaded;
    }

    return loaded;
}

Atlas::Atlas()
{
    std::vector<Color> pixels(SIZE * SIZE, Color{ 0, 0, 0, 0 });

    auto blit = [&pixels](int tileIndex, const Tile& tile) {
        int col = tileIndex % TILES_PER_ROW;
        int row = tileIndex / TILES_PER_ROW;
        for (int y = 0; y < TILE_PIXELS; ++y)
            for (int x = 0; x < TILE_PIXELS; ++x)
                pixels[(row * TILE_PIXELS + y) * SIZE + (col * TILE_PIXELS + x)] = tile.get(x, y);
    };

    for (const TileEntry& entry : TILE_PAINTERS)
    {
        Tile tile;
        entry.fn(tile);
        blit(entry.index, tile);
    }

    for (int stage = 0; stage < Tiles::CrackStages; ++stage)
    {
        Tile tile;
        paintCrack(tile, stage);
        blit(Tiles::CrackFirst + stage, tile);
    }

    // `pixels` now holds the procedurally painted atlas at FALLBACK_TILE_PIXELS.
    // A resource pack, if present, overrides individual tiles and may be at a
    // higher resolution, in which case the whole atlas is rebuilt at that size
    // and any tile the pack does not supply is point-scaled up from the
    // painted one. That way a partial pack still produces a complete atlas.
    m_tilePixels = detectPackTileSize();
    if (m_tilePixels <= 0) m_tilePixels = FALLBACK_TILE_PIXELS;

    const int atlasPixels = TILES_PER_ROW * m_tilePixels;
    std::vector<Color> atlas(static_cast<size_t>(atlasPixels) * atlasPixels, Color{ 0, 0, 0, 0 });

    const int scale = m_tilePixels / FALLBACK_TILE_PIXELS;
    for (int tile = 0; tile < TILES_PER_ROW * TILES_PER_ROW; ++tile)
    {
        const int col = tile % TILES_PER_ROW;
        const int row = tile / TILES_PER_ROW;
        for (int y = 0; y < m_tilePixels; ++y)
        {
            for (int x = 0; x < m_tilePixels; ++x)
            {
                const int sx = (scale > 0) ? (x / scale) : 0;
                const int sy = (scale > 0) ? (y / scale) : 0;
                const int srcX = col * FALLBACK_TILE_PIXELS + (sx < FALLBACK_TILE_PIXELS ? sx : FALLBACK_TILE_PIXELS - 1);
                const int srcY = row * FALLBACK_TILE_PIXELS + (sy < FALLBACK_TILE_PIXELS ? sy : FALLBACK_TILE_PIXELS - 1);
                atlas[static_cast<size_t>(row * m_tilePixels + y) * atlasPixels + (col * m_tilePixels + x)] =
                    pixels[static_cast<size_t>(srcY) * SIZE + srcX];
            }
        }
    }

    m_loadedFromPack = loadPackTiles(reinterpret_cast<uint8_t*>(atlas.data()), atlasPixels, "");
    m_id = uploadAtlas(reinterpret_cast<const uint8_t*>(atlas.data()), atlasPixels);

    // LabPBR companions. Defaults stand in for the tiles a pack leaves out --
    // transparent and emissive blocks usually ship neither -- so the shader
    // always samples something sane: a flat outward normal, and a surface that
    // is neither smooth, metallic nor emissive.
    const size_t texels = static_cast<size_t>(atlasPixels) * atlasPixels;

    std::vector<Color> normalAtlas(texels, Color{ 128, 128, 255, 255 });
    const int normalCount = loadPackTiles(reinterpret_cast<uint8_t*>(normalAtlas.data()), atlasPixels, "_n");
    m_normalId = uploadAtlas(reinterpret_cast<const uint8_t*>(normalAtlas.data()), atlasPixels);

    std::vector<Color> specularAtlas(texels, Color{ 0, 0, 0, 0 });
    const int specularCount = loadPackTiles(reinterpret_cast<uint8_t*>(specularAtlas.data()), atlasPixels, "_s");
    m_specularId = uploadAtlas(reinterpret_cast<const uint8_t*>(specularAtlas.data()), atlasPixels);

    std::printf("[Atlas] %dpx tiles | %d albedo, %d normal, %d specular from the pack\n",
                m_tilePixels, m_loadedFromPack, normalCount, specularCount);
}

Atlas::~Atlas()
{
    if (m_id) glDeleteTextures(1, &m_id);
    if (m_normalId) glDeleteTextures(1, &m_normalId);
    if (m_specularId) glDeleteTextures(1, &m_specularId);
}

void Atlas::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glActiveTexture(GL_TEXTURE0 + unit + 1);
    glBindTexture(GL_TEXTURE_2D, m_normalId);
    glActiveTexture(GL_TEXTURE0 + unit + 2);
    glBindTexture(GL_TEXTURE_2D, m_specularId);
    glActiveTexture(GL_TEXTURE0 + unit);
}

// The atlas built above is the procedurally painted fallback, so its tiles are
// always FALLBACK_TILE_PIXELS square and nothing is animated yet. These three
// round out the interface Atlas.h declares; they stay correct once a resource
// pack loader starts filling m_animations and m_tilePixels.

int Atlas::pixelSize()
{
    return SIZE;
}

void Atlas::uploadTile(int tile, const uint8_t* rgba)
{
    if (!m_id || !rgba) return;

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
    const int frameBytes = m_tilePixels * m_tilePixels * 4;

    for (Animation& animation : m_animations)
    {
        if (animation.frameCount < 2) continue;

        animation.timer += deltaTime;
        if (animation.timer < animation.frameSeconds) continue;

        animation.timer -= animation.frameSeconds;
        animation.current = (animation.current + 1) % animation.frameCount;

        const size_t offset = static_cast<size_t>(animation.current) * frameBytes;
        if (offset + frameBytes <= animation.frames.size())
            uploadTile(animation.tile, animation.frames.data() + offset);
    }
}
