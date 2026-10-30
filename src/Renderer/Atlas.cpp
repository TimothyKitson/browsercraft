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

    // The stone family. All the same rock with a different cast and a
    // different coarseness, which is what they are: granite is pink and
    // blotchy, diorite near-white, andesite grey-green and fine,
    // deepslate dark and streaky, tuff dull and pitted, calcite almost
    // chalk.
    void paintRock(Tile& t, Color base, int speckleAmount, uint32_t salt,
                   int blotches, int blotchRadius, int blotchShade)
    {
        t.fill(base);
        t.speckle(speckleAmount, salt);
        for (int i = 0; i < blotches; ++i)
        {
            const int cx = static_cast<int>(hash2(i, 0, salt) * Tile::N);
            const int cy = static_cast<int>(hash2(i, 1, salt) * Tile::N);
            const int rr = blotchRadius * blotchRadius;
            for (int y = cy - blotchRadius; y <= cy + blotchRadius; ++y)
                for (int x = cx - blotchRadius; x <= cx + blotchRadius; ++x)
                    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= rr)
                        t.set(x, y, shade(t.get(x, y), blotchShade));
        }
    }

    void paintGranite(Tile& t)  { paintRock(t, Color{ 154, 108,  92 }, 15, 211, 6, 3, -16); }
    void paintDiorite(Tile& t)  { paintRock(t, Color{ 207, 207, 209 }, 17, 223, 7, 3, -22); }
    void paintAndesite(Tile& t) { paintRock(t, Color{ 136, 139, 136 }, 11, 227, 4, 2, -12); }
    void paintTuff(Tile& t)     { paintRock(t, Color{ 108, 109, 101 }, 14, 233, 8, 2, -14); }
    void paintCalcite(Tile& t)  { paintRock(t, Color{ 223, 223, 218 },  8, 239, 3, 2,  -9); }

    // Deepslate is streaked rather than blotched: the dark rock below
    // zero reads as layered, which is what tells you how deep you are.
    void paintDeepslate(Tile& t)
    {
        t.fill(Color{ 80, 80, 84 });
        t.speckle(12, 241);
        const int n = Tile::N;
        for (int y = 0; y < n; ++y)
        {
            const int band = (y * 5) / n;
            const int step = (band % 2 == 0) ? -7 : 5;
            for (int x = 0; x < n; ++x)
                if ((x + band * 3) % 7 < 4) t.set(x, y, shade(t.get(x, y), step));
        }
    }

    void paintDeepslateTop(Tile& t)
    {
        paintDeepslate(t);
        const int n = Tile::N;
        for (int x = 0; x < n; ++x)
            for (int y = 0; y < n; ++y)
                if ((x * 3 + y * 5) % 11 < 3) t.set(x, y, shade(t.get(x, y), 6));
    }

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

    void paintPlanksIn(Tile& t, Color wood, uint32_t salt)
    {
        for (int y = 0; y < Tile::N; ++y)
        {
            int row = y / 4;
            int rowTint = static_cast<int>((hash2(row, 0, salt) - 0.5f) * 26.0f);
            Color plank = shade(wood, rowTint);
            for (int x = 0; x < Tile::N; ++x)
            {
                Color c = plank;
                if (hash2(x, row * 7, salt + 1) > 0.80f) c = shade(c, -22); // grain
                if (y % 4 == 3) c = shade(c, -40);                     // plank seam
                int joint = static_cast<int>(hash2(row, 1, salt + 2) * Tile::N);
                if (x == joint) c = shade(c, -40);                     // end joint
                t.set(x, y, c);
            }
        }
        t.speckle(6, 44);
    }

    // Oak, which is what everything that just says "planks" means.
    void paintPlanks(Tile& t) { paintPlanksIn(t, Color{ 162, 130, 78 }, 41); }

    // The bench: planks with a grid of tools scored into the top, and a
    // row of them hanging on the side, which is how you tell it from a
    // plain plank block at a glance.
    void paintCraftingTop(Tile& t)
    {
        paintPlanks(t);

        const Color line{ 92, 68, 38 };
        const int third = Tile::N / 3;
        for (int i = 1; i < 3; ++i)
            for (int k = 0; k < Tile::N; ++k)
            {
                t.set(i * third, k, line);
                t.set(k, i * third, line);
            }

        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(0, k, line); t.set(Tile::N - 1, k, line);
            t.set(k, 0, line); t.set(k, Tile::N - 1, line);
        }
    }

    void paintCraftingSide(Tile& t)
    {
        paintPlanks(t);

        // A saw and a hammer hung on the side, drawn at whatever size the
        // tile happens to be so a 16x and a 32x pack both come out right.
        const Color steel{ 176, 176, 186 };
        const Color handle{ 104, 72, 40 };
        const int n = Tile::N;

        for (int y = n / 4; y < n / 2; ++y)
            for (int x = n / 6; x < n / 2; ++x)
                if (x - n / 6 >= y - n / 4) t.set(x, y, steel);

        for (int y = n / 2; y < n - n / 5; ++y)
            for (int x = n / 4; x < n / 4 + std::max(1, n / 12); ++x)
                t.set(x, y, handle);

        for (int y = n / 2; y < n / 2 + std::max(2, n / 8); ++y)
            for (int x = n / 2 + n / 12; x < n - n / 6; ++x)
                t.set(x, y, steel);

        for (int y = n / 2; y < n - n / 5; ++y)
            for (int x = n / 2 + n / 5; x < n / 2 + n / 5 + std::max(1, n / 12); ++x)
                t.set(x, y, handle);
    }

    // The furnace. No block in this engine has a facing, so every side
    // is the front: a furnace that looked like plain stone from three
    // sides would be worse than one with four mouths.
    void paintFurnaceStone(Tile& t)
    {
        t.fill(Color{ 116, 116, 116 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float n = hash2(x / 2, y / 2, 61);
                t.set(x, y, shade(t.get(x, y), n > 0.62f ? 20 : (n < 0.34f ? -22 : 0)));
            }
        t.speckle(7, 62);
    }

    void paintFurnaceTop(Tile& t)
    {
        paintFurnaceStone(t);

        // A rim, so the top reads as the top.
        const Color rim{ 88, 88, 88 };
        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(k, 0, rim); t.set(k, Tile::N - 1, rim);
            t.set(0, k, rim); t.set(Tile::N - 1, k, rim);
        }
    }

    void paintFurnaceFace(Tile& t, bool lit)
    {
        paintFurnaceStone(t);

        const int n = Tile::N;
        const int x0 = n / 8, x1 = n - n / 8 - 1;
        const int y0 = n / 2 - n / 16, y1 = n - n / 8;

        // The mouth is nearly black so that it reads as an opening from
        // across a field. Dark grey on grey stone, at the size a block
        // actually appears on screen, looked like one more patch of the
        // noise rather than a hole with a fire in it.
        const Color frame{ 168, 168, 172 };
        const Color inner{ 54, 54, 58 };
        const Color mouth = lit ? Color{ 252, 170, 48 } : Color{ 14, 14, 16 };
        const Color ember = lit ? Color{ 255, 228, 138 } : Color{ 28, 28, 32 };

        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                const bool edge = (x == x0 || x == x1 || y == y0 || y == y1);
                if (edge) { t.set(x, y, frame); continue; }

                const bool rim = (x == x0 + 1 || x == x1 - 1 || y == y0 + 1 || y == y1 - 1);
                if (rim) { t.set(x, y, inner); continue; }

                t.set(x, y, hash2(x, y, 63) > 0.6f ? ember : mouth);
            }

        // The vent slots across the top, which is the other half of what
        // makes a furnace look like a furnace.
        const Color vent{ 58, 58, 62 };
        const Color ventLip{ 150, 150, 154 };
        const int ventTop = n / 6;
        const int ventBottom = ventTop + std::max(1, n / 10);

        for (int x = x0 + 1; x <= x1 - 1; ++x)
        {
            if (((x - x0) / std::max(1, n / 16)) % 2 == 0) continue;
            for (int y = ventTop; y <= ventBottom; ++y) t.set(x, y, vent);
            t.set(x, ventBottom + 1, ventLip);
        }
    }

    void paintFurnaceFront(Tile& t) { paintFurnaceFace(t, false); }
    void paintFurnaceLit(Tile& t)   { paintFurnaceFace(t, true); }

    // The chest. Like the furnace it has no facing to orient it by, so
    // the latch goes on every side and it reads as a chest whichever way
    // you come at it.
    void paintChestWood(Tile& t)
    {
        t.fill(Color{ 150, 110, 58 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float n = hash2(x, y / 3, 71);
                t.set(x, y, shade(t.get(x, y), n > 0.72f ? -20 : (n < 0.3f ? 14 : 0)));
            }

        const Color edge{ 96, 68, 34 };
        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(k, 0, edge); t.set(k, Tile::N - 1, edge);
            t.set(0, k, edge); t.set(Tile::N - 1, k, edge);
        }
    }

    void paintChestTop(Tile& t)
    {
        paintChestWood(t);

        // The band across the lid.
        const Color band{ 104, 74, 38 };
        for (int x = 1; x < Tile::N - 1; ++x)
            for (int y = Tile::N / 2 - 1; y <= Tile::N / 2; ++y)
                t.set(x, y, band);
    }

    void paintChestFace(Tile& t, bool front)
    {
        paintChestWood(t);

        const int n = Tile::N;
        const Color band{ 104, 74, 38 };
        const Color iron{ 92, 92, 98 };
        const Color ironLit{ 148, 148, 154 };

        // The seam between lid and body, a third of the way down.
        const int seam = n / 3;
        for (int x = 1; x < n - 1; ++x)
        {
            t.set(x, seam, band);
            t.set(x, seam + 1, shade(band, -18));
        }

        if (!front) return;

        // The latch, straddling the seam.
        const int x0 = n / 2 - std::max(1, n / 16);
        const int x1 = n / 2 + std::max(1, n / 16) - 1;
        for (int y = seam - std::max(1, n / 16); y <= seam + std::max(2, n / 8); ++y)
            for (int x = x0; x <= x1; ++x)
                t.set(x, y, (y <= seam) ? ironLit : iron);

        // The keyhole.
        t.set(n / 2 - 1, seam + std::max(1, n / 12), Color{ 32, 32, 36 });
    }

    void paintChestFront(Tile& t) { paintChestFace(t, true); }
    void paintChestSide(Tile& t)  { paintChestFace(t, false); }

    // The bed. Minecraft's is two blocks long and turned to face the way
    // you placed it; nothing here has a facing, so this is one block
    // with the pillow at one end, which still reads as a bed from above.
    void paintBedTop(Tile& t)
    {
        const int n = Tile::N;
        const Color cloth{ 176, 46, 46 };
        const Color clothDark{ 138, 32, 32 };
        const Color pillow{ 232, 232, 236 };
        const Color pillowShade{ 198, 198, 204 };
        const Color frame{ 118, 84, 44 };

        t.fill(cloth);
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                if (hash2(x, y, 81) > 0.72f) t.set(x, y, clothDark);

        // The pillow across the top quarter.
        for (int y = 1; y < n / 4 + 1; ++y)
            for (int x = 2; x < n - 2; ++x)
                t.set(x, y, hash2(x, y, 82) > 0.7f ? pillowShade : pillow);

        // The blanket's turned-down edge below it.
        for (int x = 2; x < n - 2; ++x) t.set(x, n / 4 + 1, pillowShade);

        for (int k = 0; k < n; ++k)
        {
            t.set(k, 0, frame); t.set(k, n - 1, frame);
            t.set(0, k, frame); t.set(n - 1, k, frame);
        }
    }

    void paintBedSide(Tile& t)
    {
        const int n = Tile::N;
        const Color cloth{ 176, 46, 46 };
        const Color clothDark{ 138, 32, 32 };
        const Color wood{ 134, 96, 52 };
        const Color woodDark{ 98, 70, 36 };

        // Cloth on top, the wooden frame below it. The block is opaque,
        // so the legs are painted on rather than cut out -- holes in an
        // opaque block show the inside of the world through them.
        const int clothDepth = n * 5 / 8;

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const bool upper = y < clothDepth;
                Color c = upper ? cloth : wood;
                if (hash2(x, y, 83) > 0.72f) c = upper ? clothDark : woodDark;
                t.set(x, y, c);
            }

        // The seam where the mattress meets the frame.
        for (int x = 0; x < n; ++x) t.set(x, clothDepth, shade(wood, -30));

        // Legs at the corners, dark against the lighter frame between.
        const Color shadow = shade(woodDark, -26);
        for (int y = clothDepth + 1; y < n; ++y)
            for (int x = n / 5; x < n - n / 5; ++x)
                t.set(x, y, shadow);
    }

    // TNT: a bundle of red sticks with a band round the middle.
    void paintTntBody(Tile& t, bool primed)
    {
        const int n = Tile::N;
        const Color red = primed ? Color{ 236, 236, 236 } : Color{ 186, 56, 44 };
        const Color redDark = primed ? Color{ 208, 208, 208 } : Color{ 150, 40, 32 };
        const Color band{ 232, 232, 226 };
        const Color bandEdge{ 170, 170, 164 };
        const Color text{ 40, 40, 44 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, (x / std::max(1, n / 8)) % 2 == 0 ? red : redDark);

        // The white band across the middle, with the lettering on it.
        const int top = n / 2 - n / 8;
        const int bottom = n / 2 + n / 8;
        for (int y = top; y <= bottom; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, (y == top || y == bottom) ? bandEdge : band);

        // Three dark marks standing in for T N T, scaled to the tile.
        const int mark = std::max(1, n / 16);
        for (int i = 0; i < 3; ++i)
            for (int y = top + 2; y < bottom - 1; ++y)
                for (int x = 0; x < mark * 2; ++x)
                    t.set(n / 6 + i * (n / 4) + x, y, text);
    }

    void paintTntSide(Tile& t)       { paintTntBody(t, false); }
    void paintTntPrimedSide(Tile& t) { paintTntBody(t, true); }

    void paintTntTop(Tile& t)
    {
        const int n = Tile::N;
        t.fill(Color{ 196, 64, 50 });
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                if (hash2(x / 2, y / 2, 91) > 0.6f)
                    t.set(x, y, shade(t.get(x, y), -22));

        // The fuse, poking out of the middle.
        const Color fuse{ 226, 214, 180 };
        for (int i = -1; i <= 1; ++i)
            for (int j = -1; j <= 1; ++j)
                t.set(n / 2 + i, n / 2 + j, fuse);
    }

    void paintTntBottom(Tile& t)
    {
        t.fill(Color{ 126, 100, 76 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 92) > 0.7f) t.set(x, y, shade(t.get(x, y), -18));
    }

    // A ladder: two rails and the rungs between them, everything else
    // left clear so the wall shows through.
    // A door, in two halves: planks with a frame round them, a window
    // in the upper one and a handle on both.
    void paintDoorHalf(Tile& t, bool upper)
    {
        const int n = Tile::N;
        const Color wood{ 156, 118, 68 };
        const Color woodDark{ 118, 86, 46 };
        const Color frame{ 96, 70, 38 };
        const Color iron{ 188, 188, 194 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, ((x / std::max(1, n / 8)) % 2 == 0) ? wood : woodDark);

        // The frame: all round, bar the edge where the two halves meet.
        for (int k = 0; k < n; ++k)
        {
            t.set(0, k, frame);
            t.set(n - 1, k, frame);
            if (upper) t.set(k, 0, frame);
            else t.set(k, n - 1, frame);
        }

        if (upper)
        {
            // A window, with a bar across it.
            const int lo = n / 4, hi = n - n / 4 - 1;
            for (int y = lo; y <= hi; ++y)
                for (int x = lo; x <= hi; ++x)
                {
                    const bool edge = (x == lo || x == hi || y == lo || y == hi);
                    t.set(x, y, edge ? frame : Color{ 150, 196, 226, 220 });
                }
            for (int x = lo; x <= hi; ++x) t.set(x, (lo + hi) / 2, frame);
        }

        // The handle, on the side away from the hinge.
        const int hy = upper ? n - n / 6 : n / 6;
        for (int y = hy - 1; y <= hy + 1; ++y)
            for (int x = n - n / 4; x < n - n / 8; ++x)
                t.set(x, y, iron);
    }

    void paintDoorLower(Tile& t) { paintDoorHalf(t, false); }
    void paintDoorUpper(Tile& t) { paintDoorHalf(t, true); }

    void paintLadderTile(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const int n = Tile::N;
        const Color wood{ 150, 112, 64 };
        const Color woodDark{ 110, 80, 42 };
        const int thick = std::max(1, n / 16);

        // The two rails.
        for (int y = 0; y < n; ++y)
            for (int k = 0; k < thick; ++k)
            {
                t.set(n / 8 + k, y, (y % 4 < 2) ? wood : woodDark);
                t.set(n - n / 8 - 1 - k, y, (y % 4 < 2) ? wood : woodDark);
            }

        // Rungs every quarter of the way up.
        for (int step = 0; step < 4; ++step)
        {
            const int y = n / 8 + step * (n / 4);
            for (int k = 0; k < thick; ++k)
                for (int x = n / 8; x < n - n / 8; ++x)
                    t.set(x, y + k, wood);
        }
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

    void paintLogSideIn(Tile& t, Color bark, uint32_t salt)
    {
        for (int x = 0; x < Tile::N; ++x)
        {
            int tint = static_cast<int>((hash2(x, 0, salt) - 0.5f) * 36.0f);
            Color grain = shade(bark, tint);
            for (int y = 0; y < Tile::N; ++y)
            {
                Color c = grain;
                if (hash2(x, y / 3, salt + 1) > 0.82f) c = shade(c, -24);
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

    void paintLogSide(Tile& t) { paintLogSideIn(t, Color{ 104, 78, 47 }, 121); }

    // Spruce is a darker, redder wood under near-black bark; dark oak
    // is darker still, which is the whole of what makes it dark oak.
    void paintSprucePlanks(Tile& t)  { paintPlanksIn(t, Color{ 114,  84,  48 }, 51); }
    void paintSpruceLogSide(Tile& t) { paintLogSideIn(t, Color{  58,  39,  22 }, 131); }
    void paintDarkOakPlanks(Tile& t)  { paintPlanksIn(t, Color{  79,  53,  28 }, 61); }
    void paintDarkOakLogSide(Tile& t) { paintLogSideIn(t, Color{  52,  37,  22 }, 141); }

    // Jungle is the darkest red wood of the six; acacia is the orange
    // one nothing else looks anything like.
    void paintJunglePlanks(Tile& t)  { paintPlanksIn(t, Color{ 150, 110,  78 }, 71); }
    void paintJungleLogSide(Tile& t) { paintLogSideIn(t, Color{  86,  68,  38 }, 151); }
    void paintAcaciaPlanks(Tile& t)  { paintPlanksIn(t, Color{ 173,  93,  50 }, 81); }
    void paintAcaciaLogSide(Tile& t) { paintLogSideIn(t, Color{  87,  79,  70 }, 161); }

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
    void paintSpruceLeaves(Tile& t) { paintLeavesBase(t, Color{ 44, 82, 56 }, 153); }
    void paintDarkOakLeaves(Tile& t) { paintLeavesBase(t, Color{ 42, 90, 34 }, 155); }
    void paintSpruceLogTop(Tile& t)
    { paintRings(t, Color{ 124, 94, 56 }, Color{ 104, 76, 44 }, Color{ 64, 44, 26 }, 133); }
    void paintDarkOakLogTop(Tile& t)
    { paintRings(t, Color{ 104, 76, 44 }, Color{ 86, 62, 34 }, Color{ 54, 38, 22 }, 143); }
    void paintBirchLeaves(Tile& t) { paintLeavesBase(t, Color{ 108, 152, 70 }, 152); }
    void paintJungleLeaves(Tile& t) { paintLeavesBase(t, Color{ 48, 118, 28 }, 157); }
    void paintAcaciaLeaves(Tile& t) { paintLeavesBase(t, Color{ 104, 144, 48 }, 159); }
    void paintJungleLogTop(Tile& t)
    { paintRings(t, Color{ 170, 128, 90 }, Color{ 150, 110, 78 }, Color{ 86, 68, 38 }, 153); }
    void paintAcaciaLogTop(Tile& t)
    { paintRings(t, Color{ 186, 102, 54 }, Color{ 166,  88, 44 }, Color{ 87, 79, 70 }, 163); }

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

    void paintNetherBricks(Tile& t)
    {
        t.fill(Color{ 22, 11, 14 });
        for (int row = 0; row < 4; ++row)
        {
            const int offset = (row % 2) * 4;
            for (int y = 0; y < 3; ++y)
                for (int x = 0; x < Tile::N; ++x)
                {
                    const int bx = (x + offset) % Tile::N;
                    if ((bx % 8) == 7) continue;
                    const int brick = (x + offset) / 8 + row * 3;
                    const int delta = static_cast<int>((hash2(brick, row, 231) - 0.5f) * 20.0f);
                    t.set(x, row * 4 + y, shade(Color{ 48, 24, 28 }, delta));
                }
        }
        t.speckle(6, 232);
    }

    // Redstone. The ore is stone with dull red specks; the dust is a
    // line painted on the floor, dark until something powers it; the
    // lamp goes from a dead grey-brown to the colour of a lit filament.
    void paintRedstoneOre(Tile& t)
    {
        paintStone(t);
        for (int i = 0; i < 16; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 251) * (Tile::N - 1));
            const int y = static_cast<int>(hash2(i, 1, 251) * (Tile::N - 1));
            for (int dy = 0; dy <= 1; ++dy)
                for (int dx = 0; dx <= 1; ++dx)
                    t.set(x + dx, y + dy, Color{ 172, 26, 18 });
        }
    }

    // A cross rather than a line. Minecraft draws dust from three
    // sprites -- a dot and two lines -- and picks between them by what
    // the cell is joined to; this engine's dust is one block with no
    // idea of its neighbours, and a cross is the shape that reads as a
    // run whichever way the run goes.
    void paintDustIn(Tile& t, Color core, Color edge)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int mid = Tile::N / 2;
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, mid - 1, edge);
            t.set(i, mid, core);
            t.set(mid - 1, i, edge);
            t.set(mid, i, core);
        }
        t.set(mid, mid, core);
        t.set(mid - 1, mid - 1, core);
    }

    void paintRedstoneDust(Tile& t)    { paintDustIn(t, Color{ 124, 16, 10 }, Color{ 82, 10, 6 }); }
    void paintRedstoneDustLit(Tile& t) { paintDustIn(t, Color{ 252, 56, 32 }, Color{ 188, 28, 16 }); }

    void paintTorchIn(Tile& t, Color head)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int mid = Tile::N / 2;
        for (int y = 6; y < Tile::N; ++y)
        {
            t.set(mid - 1, y, Color{ 110, 82, 48 });
            t.set(mid, y, Color{ 150, 112, 66 });
        }
        for (int y = 3; y <= 5; ++y)
            for (int x = mid - 1; x <= mid; ++x)
                t.set(x, y, head);
    }

    void paintRedstoneTorch(Tile& t)    { paintTorchIn(t, Color{ 255, 70, 40 }); }
    void paintRedstoneTorchOff(Tile& t) { paintTorchIn(t, Color{ 108, 34, 26 }); }

    void paintLeverIn(Tile& t, bool thrown)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // The cobble base it is screwed to, and the handle, which lies
        // one way or the other depending on which way it is thrown.
        for (int y = 11; y < Tile::N; ++y)
            for (int x = 5; x <= 10; ++x)
                t.set(x, y, shade(Color{ 122, 122, 122 },
                                  static_cast<int>((hash2(x, y, 253) - 0.5f) * 30.0f)));

        for (int i = 0; i < 7; ++i)
        {
            const int x = thrown ? 11 - i : 4 + i;
            t.set(x, 10 - i, Color{ 124, 92, 54 });
            t.set(x + 1, 10 - i, Color{ 92, 66, 38 });
        }
    }

    void paintLever(Tile& t)   { paintLeverIn(t, false); }
    void paintLeverOn(Tile& t) { paintLeverIn(t, true); }

    void paintButtonIn(Tile& t, bool pressed)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int top = pressed ? 8 : 7;

        // A shadow under it and along its sides, because a grey button
        // on a grey cobble is a button nobody can see -- the first one
        // vanished into the block it was screwed to, and the second
        // was a black diamond the size of a dinner plate.
        for (int y = top; y <= 12; ++y)
        {
            t.set(4, y, Color{ 58, 58, 60 });
            t.set(11, y, Color{ 58, 58, 60 });
        }
        for (int x = 4; x <= 11; ++x) t.set(x, 12, Color{ 58, 58, 60 });

        for (int y = top; y <= 11; ++y)
            for (int x = 5; x <= 10; ++x)
                t.set(x, y, shade(Color{ 158, 158, 160 },
                                  (y == top ? 34 : 0) +
                                  static_cast<int>((hash2(x, y, 254) - 0.5f) * 24.0f)));
    }

    void paintButton(Tile& t)   { paintButtonIn(t, false); }
    void paintButtonOn(Tile& t) { paintButtonIn(t, true); }

    void paintRedstoneLampIn(Tile& t, Color base, Color stud)
    {
        t.fill(base);
        t.speckle(8, 255);
        for (int y = 2; y < Tile::N; y += 5)
            for (int x = 2; x < Tile::N; x += 5)
            {
                t.set(x, y, stud);
                t.set(x + 1, y, stud);
                t.set(x, y + 1, stud);
                t.set(x + 1, y + 1, stud);
            }
    }

    void paintRedstoneLamp(Tile& t)
    { paintRedstoneLampIn(t, Color{ 96, 62, 36 }, Color{ 64, 40, 22 }); }
    void paintRedstoneLampLit(Tile& t)
    { paintRedstoneLampIn(t, Color{ 226, 166, 86 }, Color{ 255, 214, 136 }); }

    void paintRedstoneBlock(Tile& t)
    {
        t.fill(Color{ 172, 26, 18 });
        t.speckle(14, 256);
        for (int i = 0; i < 20; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 257) * Tile::N);
            const int y = static_cast<int>(hash2(i, 1, 257) * Tile::N);
            t.set(x, y, Color{ 212, 48, 34 });
        }
    }

    // The trial chamber's stone, and the two things in it. The spawner
    // is a dark cage with a glow inside that comes up when its wave is
    // out; the vault is a locked face with a keyhole that opens.
    void paintTuffBricks(Tile& t)
    {
        t.fill(Color{ 92, 94, 88 });
        for (int row = 0; row < 4; ++row)
        {
            const int offset = (row % 2) * 4;
            for (int y = 0; y < 3; ++y)
                for (int x = 0; x < Tile::N; ++x)
                {
                    const int bx = (x + offset) % Tile::N;
                    if ((bx % 8) == 7) continue;
                    const int brick = (x + offset) / 8 + row * 3;
                    const int delta = static_cast<int>((hash2(brick, row, 281) - 0.5f) * 26.0f);
                    t.set(x, row * 4 + y, shade(Color{ 116, 116, 108 }, delta));
                }
        }
        t.speckle(7, 282);
    }

    // The sea's stone. Prismarine is not a flat colour in Minecraft --
    // it shifts between a green and a blue across the face, and that
    // drift is most of what makes a monument wall read as prismarine
    // rather than as green concrete.
    void paintPrismarineIn(Tile& t, Color low, Color high, int grain)
    {
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float drift = (hash2(x / 2, y / 2, 341) + hash2(x, y, 347)) * 0.5f;
                t.set(x, y, Color{
                    static_cast<uint8_t>(low.r + (high.r - low.r) * drift),
                    static_cast<uint8_t>(low.g + (high.g - low.g) * drift),
                    static_cast<uint8_t>(low.b + (high.b - low.b) * drift) });
            }
        t.speckle(grain, 342);
    }

    void paintPrismarine(Tile& t)
    {
        paintPrismarineIn(t, Color{ 86, 146, 136 }, Color{ 110, 183, 166 }, 6);
    }

    void paintPrismarineBricks(Tile& t)
    {
        paintPrismarineIn(t, Color{ 92, 160, 148 }, Color{ 118, 190, 174 }, 4);
        // Four courses of bricks cut into it, offset row by row.
        for (int row = 0; row < 4; ++row)
            for (int x = 0; x < Tile::N; ++x)
            {
                const int offset = (row % 2) * 4;
                t.set(x, row * 4, shade(Color{ 70, 128, 118 }, 0));
                if (((x + offset) % 8) == 7)
                    for (int y = 0; y < 4; ++y)
                        t.set(x, row * 4 + y, shade(Color{ 70, 128, 118 }, 0));
            }
    }

    void paintDarkPrismarine(Tile& t)
    {
        paintPrismarineIn(t, Color{ 39, 67, 57 }, Color{ 56, 90, 78 }, 5);
    }

    void paintSeaLantern(Tile& t)
    {
        t.fill(Color{ 172, 199, 190 });
        // The cells in it, which are the bright part: a lantern that
        // was one even glow looked like a block of white wool.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float cell = hash2(x / 4, y / 4, 351);
                if (cell > 0.55f) t.set(x, y, Color{ 226, 244, 235 });
                else if (cell < 0.22f) t.set(x, y, Color{ 134, 166, 158 });
            }
        t.speckle(5, 352);
    }

    void paintSponge(Tile& t)
    {
        t.fill(Color{ 195, 192, 74 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 361) > 0.74f)
                    t.set(x, y, Color{ 148, 146, 48 });
        t.speckle(7, 362);
    }

    void paintWetSponge(Tile& t)
    {
        t.fill(Color{ 134, 139, 56 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 361) > 0.74f)
                    t.set(x, y, Color{ 98, 104, 36 });
        t.speckle(7, 363);
    }

    // What grows in the water. Kelp is a ribbon rather than a blade --
    // it is the one plant in the game that is wider at the top than
    // the bottom, and drawn as grass it reads as pondweed.
    void paintKelpIn(Tile& t, bool crown)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int frond = 0; frond < 3; ++frond)
        {
            const int x = 4 + frond * 4;
            const int wide = crown ? 2 : 1;
            for (int y = 0; y < Tile::N; ++y)
            {
                const int sway = static_cast<int>(std::sin(y * 0.45f + frond) * 1.6f);
                for (int w = -wide; w <= wide; ++w)
                {
                    if (!crown && std::abs(w) == wide && ((y + frond) % 3) == 0) continue;
                    t.set(x + sway + w, y,
                          shade(Color{ 44, 108, 46 },
                                static_cast<int>((hash2(x + w, y, 371) - 0.5f) * 30.0f)));
                }
            }
        }
    }

    void paintKelp(Tile& t)    { paintKelpIn(t, false); }
    void paintKelpTop(Tile& t) { paintKelpIn(t, true); }

    void paintSeagrass(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int blade = 0; blade < 6; ++blade)
        {
            const int x = 2 + static_cast<int>(hash2(blade, 0, 373) * 12.0f);
            const int tall = 4 + static_cast<int>(hash2(blade, 1, 373) * 6.0f);
            for (int k = 0; k < tall; ++k)
            {
                const int bend = static_cast<int>(k * 0.22f *
                                                  (hash2(blade, 3, 373) > 0.5f ? 1 : -1));
                t.set(x + bend, Tile::N - 1 - k,
                      shade(Color{ 54, 124, 58 },
                            static_cast<int>((hash2(blade, 2, 373) - 0.5f) * 36.0f)));
            }
        }
    }

    void paintSeaPickle(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int pickle = 0; pickle < 3; ++pickle)
        {
            const int x = 4 + pickle * 4;
            const int tall = 4 + static_cast<int>(hash2(pickle, 0, 375) * 3.0f);
            for (int k = 0; k < tall; ++k)
                for (int w = -1; w <= 1; ++w)
                    t.set(x + w, Tile::N - 1 - k,
                          (k == tall - 1) ? Color{ 182, 196, 92 } : Color{ 106, 126, 54 });
        }
    }

    // A reef's solid blocks: five colours, which is what makes a warm
    // ocean floor read as a reef rather than as one dyed boulder.
    void paintCoralBlockIn(Tile& t, Color body, int salt)
    {
        t.fill(body);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float lump = hash2(x / 2, y / 2, salt);
                if (lump > 0.72f) t.set(x, y, shade(body, 26));
                else if (lump < 0.26f) t.set(x, y, shade(body, -30));
            }
        t.speckle(5, salt + 1);
    }

    // And the fans that grow off them, in the same five colours.
    void paintCoralFanIn(Tile& t, Color body, int salt)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int arm = 0; arm < 5; ++arm)
        {
            const int root = 5 + static_cast<int>(hash2(arm, 0, salt) * 7.0f);
            const int tall = 6 + static_cast<int>(hash2(arm, 1, salt) * 7.0f);
            const int lean = (arm % 2) ? 1 : -1;
            for (int k = 0; k < tall; ++k)
            {
                const int x = root + static_cast<int>(k * 0.35f) * lean;
                t.set(x, Tile::N - 1 - k, body);
                // The branches, which is what makes it a fan.
                if (k > 2 && (k % 3) == 0)
                {
                    t.set(x - 1, Tile::N - 1 - k, shade(body, -18));
                    t.set(x + 1, Tile::N - 1 - k, shade(body, -18));
                }
            }
        }
    }

    void paintTubeCoralBlock(Tile& t)   { paintCoralBlockIn(t, Color{  49,  87, 206 }, 381); }
    void paintBrainCoralBlock(Tile& t)  { paintCoralBlockIn(t, Color{ 207,  91, 155 }, 383); }
    void paintBubbleCoralBlock(Tile& t) { paintCoralBlockIn(t, Color{ 165,  26, 163 }, 385); }
    void paintFireCoralBlock(Tile& t)   { paintCoralBlockIn(t, Color{ 166,  33,  42 }, 387); }
    void paintHornCoralBlock(Tile& t)   { paintCoralBlockIn(t, Color{ 216, 199,  78 }, 389); }

    void paintTubeCoral(Tile& t)   { paintCoralFanIn(t, Color{  58, 104, 224 }, 391); }
    void paintBrainCoral(Tile& t)  { paintCoralFanIn(t, Color{ 220, 110, 170 }, 393); }
    void paintBubbleCoral(Tile& t) { paintCoralFanIn(t, Color{ 186,  40, 184 }, 395); }
    void paintFireCoral(Tile& t)   { paintCoralFanIn(t, Color{ 192,  44,  52 }, 397); }
    void paintHornCoral(Tile& t)   { paintCoralFanIn(t, Color{ 232, 216,  96 }, 399); }

    // The deep dark. Sculk is near-black with a blue-green glow in
    // it, which is the only colour down there and so the only thing
    // that makes the place read as somewhere rather than as a cave
    // with the lights off.
    void paintSculk(Tile& t)
    {
        t.fill(Color{ 14, 20, 26 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float blob = hash2(x / 2, y / 2, 401);
                if (blob > 0.86f) t.set(x, y, Color{ 22, 92, 92 });
                else if (blob > 0.74f) t.set(x, y, Color{ 18, 56, 60 });
                else if (blob < 0.18f) t.set(x, y, Color{ 8, 12, 16 });
            }
        t.speckle(4, 402);
    }

    void paintDeepslateBricks(Tile& t)
    {
        t.fill(Color{ 54, 54, 58 });
        for (int row = 0; row < 4; ++row)
        {
            const int offset = (row % 2) * 4;
            for (int y = 0; y < 3; ++y)
                for (int x = 0; x < Tile::N; ++x)
                {
                    const int bx = (x + offset) % Tile::N;
                    if ((bx % 8) == 7) continue;
                    const int brick = (x + offset) / 8 + row * 3;
                    const int delta = static_cast<int>((hash2(brick, row, 405) - 0.5f) * 22.0f);
                    t.set(x, row * 4 + y, shade(Color{ 70, 70, 76 }, delta));
                }
        }
        t.speckle(5, 406);
    }

    void paintDeepslateTiles(Tile& t)
    {
        t.fill(Color{ 46, 46, 50 });
        // Four square tiles rather than courses of bricks, which is
        // what tells the two apart at a glance.
        for (int ty = 0; ty < 2; ++ty)
            for (int tx = 0; tx < 2; ++tx)
            {
                const int delta = static_cast<int>((hash2(tx, ty, 407) - 0.5f) * 20.0f);
                for (int y = 1; y < 7; ++y)
                    for (int x = 1; x < 7; ++x)
                        t.set(tx * 8 + x, ty * 8 + y, shade(Color{ 62, 62, 68 }, delta));
            }
        t.speckle(5, 408);
    }

    // The two that listen, and the one that spreads. Each is sculk
    // with something on top of it, so each is the sculk tile with its
    // own mark drawn over.
    void paintSculkMark(Tile& t, Color mark, Color glow, int salt)
    {
        paintSculk(t);
        for (int i = 0; i < 9; ++i)
        {
            const int x = 3 + static_cast<int>(hash2(i, 0, salt) * 10.0f);
            const int y = 3 + static_cast<int>(hash2(i, 1, salt) * 10.0f);
            t.set(x, y, mark);
            t.set(x + 1, y, glow);
            t.set(x, y + 1, glow);
        }
    }

    void paintSculkSensorTop(Tile& t)
    {
        paintSculkMark(t, Color{ 28, 110, 110 }, Color{ 20, 78, 84 }, 411);
        // The three tendrils, which is the thing you recognise a
        // sensor by from above.
        for (int arm = 0; arm < 3; ++arm)
            for (int k = 0; k < 6; ++k)
                t.set(8 + static_cast<int>(std::cos(arm * 2.1f) * k),
                      8 + static_cast<int>(std::sin(arm * 2.1f) * k),
                      Color{ 46, 150, 148 });
    }

    void paintSculkSensorTopLit(Tile& t)
    {
        paintSculkSensorTop(t);
        for (int arm = 0; arm < 3; ++arm)
            for (int k = 0; k < 6; ++k)
                t.set(8 + static_cast<int>(std::cos(arm * 2.1f) * k),
                      8 + static_cast<int>(std::sin(arm * 2.1f) * k),
                      Color{ 120, 230, 220 });
    }

    void paintSculkSensorSide(Tile& t) { paintSculkMark(t, Color{ 26, 96, 100 }, Color{ 18, 66, 70 }, 413); }

    void paintSculkShriekerTop(Tile& t)
    {
        paintSculk(t);
        // A ring with a hole in the middle: the thing that screams.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - 7.5f;
                const float dy = y - 7.5f;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d < 2.4f) t.set(x, y, Color{ 6, 10, 12 });
                else if (d < 4.6f) t.set(x, y, Color{ 196, 186, 150 });
                else if (d < 5.6f) t.set(x, y, Color{ 132, 124, 96 });
            }
    }

    void paintSculkShriekerSide(Tile& t) { paintSculkMark(t, Color{ 150, 142, 112 }, Color{ 96, 90, 70 }, 415); }

    void paintSculkCatalystTop(Tile& t)
    {
        paintSculk(t);
        // The bone-white core, which is the one bright thing in an
        // ancient city.
        for (int y = 4; y < 12; ++y)
            for (int x = 4; x < 12; ++x)
                t.set(x, y, shade(Color{ 206, 204, 190 },
                                  static_cast<int>((hash2(x, y, 417) - 0.5f) * 30.0f)));
    }

    void paintSculkCatalystSide(Tile& t)
    {
        paintSculk(t);
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 419) > 0.35f)
                    t.set(x, y, Color{ 188, 186, 172 });
    }

    void paintSoulLantern(Tile& t)
    {
        t.fill(Color{ 58, 56, 52 });
        // The soul fire inside it, which is the cold blue the Nether
        // and the deep dark share.
        for (int y = 3; y < 13; ++y)
            for (int x = 4; x < 12; ++x)
                t.set(x, y, (hash2(x, y, 421) > 0.4f) ? Color{ 68, 216, 222 }
                                                      : Color{ 36, 150, 170 });
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 0, Color{ 44, 42, 40 });
            t.set(x, Tile::N - 1, Color{ 44, 42, 40 });
        }
    }

    void paintReinforcedDeepslateTop(Tile& t)
    {
        t.fill(Color{ 50, 52, 50 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (((x / 4) + (y / 4)) % 2 == 0)
                    t.set(x, y, Color{ 64, 68, 60 });
        t.speckle(6, 423);
    }

    void paintReinforcedDeepslateSide(Tile& t)
    {
        t.fill(Color{ 46, 48, 46 });
        for (int y = 2; y < Tile::N; y += 5)
            for (int x = 0; x < Tile::N; ++x)
                t.set(x, y, Color{ 92, 96, 84 });
        t.speckle(6, 424);
    }

    // The End's city. Purpur is a pale violet with a coarse grain in
    // it, which is the only colour out there that is not end stone.
    void paintPurpurIn(Tile& t, Color body, int salt)
    {
        t.fill(body);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float grain = hash2(x, y, salt);
                if (grain > 0.80f) t.set(x, y, shade(body, 22));
                else if (grain < 0.24f) t.set(x, y, shade(body, -24));
            }
        t.speckle(5, salt + 1);
    }

    void paintPurpur(Tile& t) { paintPurpurIn(t, Color{ 169, 125, 169 }, 431); }

    void paintPurpurPillarTop(Tile& t)
    {
        paintPurpurIn(t, Color{ 176, 134, 176 }, 433);
        // A ring, so the top of a pillar is not the side of one.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - 7.5f, dy = y - 7.5f;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > 4.6f && d < 6.2f) t.set(x, y, Color{ 134, 96, 134 });
            }
    }

    void paintPurpurPillarSide(Tile& t)
    {
        paintPurpurIn(t, Color{ 172, 128, 172 }, 435);
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 0, Color{ 132, 94, 132 });
            t.set(x, 1, Color{ 148, 108, 148 });
            t.set(x, Tile::N - 2, Color{ 148, 108, 148 });
            t.set(x, Tile::N - 1, Color{ 132, 94, 132 });
        }
    }

    void paintEndBricks(Tile& t)
    {
        t.fill(Color{ 206, 212, 154 });
        // Small bricks, four courses, which is what tells them from
        // plain end stone across a room.
        for (int row = 0; row < 4; ++row)
        {
            const int offset = (row % 2) * 4;
            for (int x = 0; x < Tile::N; ++x)
            {
                t.set(x, row * 4, Color{ 170, 176, 118 });
                if (((x + offset) % 8) == 7)
                    for (int y = 0; y < 4; ++y)
                        t.set(x, row * 4 + y, Color{ 170, 176, 118 });
            }
        }
        t.speckle(6, 437);
    }

    void paintChorusPlant(Tile& t)
    {
        t.fill(Color{ 94, 60, 94 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 439) > 0.70f) t.set(x, y, Color{ 122, 82, 122 });
        t.speckle(6, 440);
    }

    void paintChorusFlower(Tile& t)
    {
        t.fill(Color{ 198, 190, 198 });
        for (int y = 5; y < 11; ++y)
            for (int x = 5; x < 11; ++x)
                t.set(x, y, Color{ 120, 94, 120 });
        t.speckle(6, 441);
    }

    // The Nether's bastion. Blackstone is darker than netherrack and
    // has no red in it, which is the whole of how you tell a bastion
    // from the rock it stands in.
    void paintBlackstone(Tile& t)
    {
        t.fill(Color{ 36, 32, 38 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float grain = hash2(x, y, 443);
                if (grain > 0.82f) t.set(x, y, Color{ 56, 50, 58 });
                else if (grain < 0.20f) t.set(x, y, Color{ 22, 20, 24 });
            }
        t.speckle(5, 444);
    }

    void paintPolishedBlackstone(Tile& t)
    {
        t.fill(Color{ 44, 40, 46 });
        for (int row = 0; row < 4; ++row)
        {
            const int offset = (row % 2) * 4;
            for (int x = 0; x < Tile::N; ++x)
            {
                t.set(x, row * 4, Color{ 26, 24, 28 });
                if (((x + offset) % 8) == 7)
                    for (int y = 0; y < 4; ++y)
                        t.set(x, row * 4 + y, Color{ 26, 24, 28 });
            }
        }
        t.speckle(4, 445);
    }

    void paintGildedBlackstone(Tile& t)
    {
        paintBlackstone(t);
        // The gold in it, in flecks rather than a seam: a bastion's
        // gold is what you chip out of the walls.
        for (int i = 0; i < 14; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 447) * Tile::N);
            const int y = static_cast<int>(hash2(i, 1, 447) * Tile::N);
            t.set(x, y, Color{ 226, 186, 72 });
            if (x + 1 < Tile::N) t.set(x + 1, y, Color{ 180, 144, 56 });
        }
    }

    void paintBasaltTop(Tile& t)
    {
        t.fill(Color{ 80, 78, 84 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float d = hash2(x / 3, y / 3, 449);
                if (d > 0.72f) t.set(x, y, Color{ 98, 96, 102 });
                else if (d < 0.26f) t.set(x, y, Color{ 60, 58, 64 });
            }
        t.speckle(5, 450);
    }

    void paintBasaltSide(Tile& t)
    {
        t.fill(Color{ 66, 64, 70 });
        // Columns, because basalt comes in them.
        for (int x = 0; x < Tile::N; ++x)
        {
            const int delta = static_cast<int>((hash2(x / 2, 0, 451) - 0.5f) * 30.0f);
            for (int y = 0; y < Tile::N; ++y)
                t.set(x, y, shade(Color{ 72, 70, 78 }, delta));
        }
        t.speckle(5, 452);
    }

    // The swamp. A lily pad is drawn flat on the water, so it is the
    // one tile here that is only ever seen from above.
    void paintLilyPad(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - 7.5f, dy = y - 7.5f;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > 7.2f) continue;
                // The notch cut out of one side, which is the shape.
                if (dx > 0.0f && std::abs(dy) < 1.6f) continue;
                t.set(x, y, shade(Color{ 42, 108, 44 },
                                  static_cast<int>((hash2(x, y, 461) - 0.5f) * 26.0f)));
            }
    }

    void paintVine(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int strand = 0; strand < 5; ++strand)
        {
            const int x = 1 + static_cast<int>(hash2(strand, 0, 463) * 13.0f);
            const int tall = 8 + static_cast<int>(hash2(strand, 1, 463) * 8.0f);
            for (int y = 0; y < tall; ++y)
            {
                const int drift = static_cast<int>(std::sin(y * 0.4f + strand) * 1.4f);
                t.set(x + drift, y,
                      shade(Color{ 48, 92, 40 },
                            static_cast<int>((hash2(strand, y, 464) - 0.5f) * 34.0f)));
                if ((y % 4) == 0 && x + drift + 1 < Tile::N)
                    t.set(x + drift + 1, y, Color{ 38, 76, 32 });
            }
        }
    }

    void paintMud(Tile& t)
    {
        t.fill(Color{ 60, 50, 46 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float wet = hash2(x, y, 465);
                if (wet > 0.80f) t.set(x, y, Color{ 78, 66, 60 });
                else if (wet < 0.22f) t.set(x, y, Color{ 44, 36, 34 });
            }
        t.speckle(6, 466);
    }

    // The crystal on top of a pillar. Pale violet glass with a core
    // in it, which is the one bright thing on a dark island.
    void paintEndCrystal(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - 7.5f, dy = y - 7.5f;
                const float d = std::abs(dx) + std::abs(dy);   // a diamond, not a disc
                if (d > 7.5f) continue;
                if (d < 2.5f) t.set(x, y, Color{ 240, 226, 250 });
                else if (d < 5.0f) t.set(x, y, Color{ 196, 150, 220 });
                else t.set(x, y, Color{ 134, 92, 168 });
            }
    }

    // The skull you build a wither out of. Charcoal-grey bone with
    // two sockets, which is the whole of how you know it from an
    // ordinary skeleton's.
    void paintWitherSkull(Tile& t)
    {
        t.fill(Color{ 46, 46, 46 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 471) > 0.78f) t.set(x, y, Color{ 62, 62, 62 });

        for (int y = 5; y < 9; ++y)
            for (int x = 3; x < 7; ++x)
            {
                t.set(x, y, Color{ 10, 10, 10 });
                t.set(x + 6, y, Color{ 10, 10, 10 });
            }

        // The jaw.
        for (int x = 4; x < 12; ++x) t.set(x, 12, Color{ 24, 24, 24 });
        t.speckle(5, 472);
    }

    void paintBeacon(Tile& t)
    {
        t.fill(Color{ 28, 60, 64 });
        // The core, which is the bright part: a beacon is the one
        // block in the game that is a light you built.
        for (int y = 3; y < 13; ++y)
            for (int x = 3; x < 13; ++x)
            {
                const float d = std::max(std::abs(x - 7.5f), std::abs(y - 7.5f));
                t.set(x, y, d < 2.5f ? Color{ 236, 252, 250 }
                          : d < 4.0f ? Color{ 140, 226, 220 }
                                     : Color{ 70, 150, 160 });
            }
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, 0, Color{ 18, 40, 44 });
            t.set(i, Tile::N - 1, Color{ 18, 40, 44 });
            t.set(0, i, Color{ 18, 40, 44 });
            t.set(Tile::N - 1, i, Color{ 18, 40, 44 });
        }
    }

    // The storage blocks. Each is its metal in a solid face with the
    // corners knocked off, which is how Minecraft draws all of them.
    void paintStorageBlock(Tile& t, Color body, int salt)
    {
        t.fill(body);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float edge = std::min(std::min(x, Tile::N - 1 - x),
                                            std::min(y, Tile::N - 1 - y));
                if (edge < 1.5f) t.set(x, y, shade(body, -34));
                else if (edge < 2.5f) t.set(x, y, shade(body, 20));
                else if (hash2(x, y, salt) > 0.86f) t.set(x, y, shade(body, 16));
            }
        t.speckle(4, salt + 1);
    }

    void paintIronBlock(Tile& t)    { paintStorageBlock(t, Color{ 220, 220, 220 }, 481); }
    void paintGoldBlock(Tile& t)    { paintStorageBlock(t, Color{ 246, 208,  62 }, 483); }
    void paintDiamondBlock(Tile& t) { paintStorageBlock(t, Color{  98, 219, 214 }, 485); }
    void paintEmeraldBlock(Tile& t) { paintStorageBlock(t, Color{  66, 200,  96 }, 487); }
    void paintCoalBlock(Tile& t)    { paintStorageBlock(t, Color{  24,  24,  26 }, 489); }

    // A note block is a drum: planks with a dark skin stretched over
    // the face and studs round it.
    void paintNoteBlock(Tile& t)
    {
        t.fill(Color{ 104, 74, 48 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float edge = std::min(std::min(x, Tile::N - 1 - x),
                                            std::min(y, Tile::N - 1 - y));
                if (edge >= 3.0f) t.set(x, y, Color{ 62, 42, 30 });
            }
        for (int i = 2; i < Tile::N; i += 4)
        {
            t.set(i, 1, Color{ 176, 148, 96 });
            t.set(i, Tile::N - 2, Color{ 176, 148, 96 });
            t.set(1, i, Color{ 176, 148, 96 });
            t.set(Tile::N - 2, i, Color{ 176, 148, 96 });
        }
        t.speckle(5, 491);
    }

    // An observer: a face with two eyes, and a lamp on it when it has
    // just seen something.
    void paintObserverTopIn(Tile& t, Color eye)
    {
        t.fill(Color{ 58, 58, 60 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 493) > 0.82f) t.set(x, y, Color{ 76, 76, 78 });

        for (int y = 6; y < 10; ++y)
            for (int x = 3; x < 6; ++x)
            {
                t.set(x, y, eye);
                t.set(x + 7, y, eye);
            }
        t.speckle(4, 494);
    }

    void paintObserverTop(Tile& t)    { paintObserverTopIn(t, Color{ 150, 150, 154 }); }
    void paintObserverTopLit(Tile& t) { paintObserverTopIn(t, Color{ 236, 64, 48 }); }

    void paintObserverSide(Tile& t)
    {
        t.fill(Color{ 50, 50, 52 });
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 5, Color{ 96, 96, 100 });
            t.set(x, 10, Color{ 96, 96, 100 });
        }
        t.speckle(5, 495);
    }

    // A hopper: a funnel seen from above, and a tapering box from the
    // side. Drawn rather than modelled, because a hopper here is a
    // cube like everything else.
    void paintHopperTop(Tile& t)
    {
        t.fill(Color{ 60, 60, 62 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float edge = std::min(std::min(x, Tile::N - 1 - x),
                                            std::min(y, Tile::N - 1 - y));
                if (edge < 1.5f) t.set(x, y, Color{ 92, 92, 96 });
                else if (edge > 4.5f) t.set(x, y, Color{ 26, 26, 28 });
            }
        t.speckle(4, 501);
    }

    void paintHopperSide(Tile& t)
    {
        t.fill(Color{ 52, 52, 54 });
        // The taper: wide at the top, narrow at the bottom.
        for (int y = 0; y < Tile::N; ++y)
        {
            const int inset = (y < 6) ? 0 : (y - 5);
            for (int x = 0; x < inset && x < Tile::N; ++x)
            {
                t.set(x, y, Color{ 30, 30, 32 });
                t.set(Tile::N - 1 - x, y, Color{ 30, 30, 32 });
            }
        }
        t.speckle(4, 502);
    }

    // The two that throw: cobble with a face on the front. A dropper's
    // mouth is square and a dispenser's has a bow in it, which is the
    // only way to tell them apart at a glance.
    void paintThrowerFace(Tile& t, bool barred)
    {
        paintCobble(t);
        for (int y = 4; y < 12; ++y)
            for (int x = 4; x < 12; ++x)
                t.set(x, y, Color{ 32, 32, 32 });

        if (!barred) return;
        for (int y = 5; y < 11; ++y)
            if ((y % 2) == 1)
                for (int x = 5; x < 11; ++x)
                    t.set(x, y, Color{ 128, 128, 128 });
    }

    void paintDropperFront(Tile& t)   { paintThrowerFace(t, false); }
    void paintDispenserFront(Tile& t) { paintThrowerFace(t, true); }

    // Amethyst. Violet with facets in it, which is the only colour
    // underground that is not a shade of grey or brown.
    void paintAmethystIn(Tile& t, Color body, bool budding)
    {
        t.fill(body);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float facet = hash2(x / 3, y / 3, 511);
                if (facet > 0.72f) t.set(x, y, shade(body, 30));
                else if (facet < 0.28f) t.set(x, y, shade(body, -28));
            }

        // Budding amethyst has the buds showing on its faces, which
        // is how you tell the block worth keeping from the one worth
        // mining.
        if (budding)
            for (int i = 0; i < 5; ++i)
            {
                const int x = 2 + static_cast<int>(hash2(i, 0, 513) * 11.0f);
                const int y = 2 + static_cast<int>(hash2(i, 1, 513) * 11.0f);
                for (int d = 0; d < 3; ++d)
                {
                    t.set(x, y + d, Color{ 226, 186, 250 });
                    t.set(x + 1, y + d, Color{ 186, 138, 222 });
                }
            }

        t.speckle(5, 512);
    }

    void paintAmethyst(Tile& t) { paintAmethystIn(t, Color{ 134, 98, 178 }, false); }
    void paintBudding(Tile& t)  { paintAmethystIn(t, Color{ 120, 86, 164 }, true); }

    void paintCluster(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int spike = 0; spike < 5; ++spike)
        {
            const int x = 2 + spike * 3;
            const int tall = 6 + static_cast<int>(hash2(spike, 0, 515) * 7.0f);
            for (int k = 0; k < tall; ++k)
            {
                const int y = Tile::N - 1 - k;
                const int wide = (k < tall - 3) ? 1 : 0;
                for (int w = -wide; w <= wide; ++w)
                    t.set(x + w, y, (k > tall - 4) ? Color{ 232, 198, 252 }
                                                   : Color{ 170, 124, 212 });
            }
        }
    }

    void paintSmoothBasalt(Tile& t)
    {
        t.fill(Color{ 62, 60, 66 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (hash2(x, y, 517) > 0.80f) t.set(x, y, Color{ 74, 72, 80 });
        t.speckle(3, 518);
    }

    // A lantern: the same cage as the soul one with an ordinary
    // flame in it, so the two read as a pair.
    void paintLantern(Tile& t)
    {
        t.fill(Color{ 58, 56, 52 });
        for (int y = 3; y < 13; ++y)
            for (int x = 4; x < 12; ++x)
                t.set(x, y, (hash2(x, y, 531) > 0.4f) ? Color{ 252, 214, 118 }
                                                      : Color{ 238, 160, 60 });
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 0, Color{ 44, 42, 40 });
            t.set(x, Tile::N - 1, Color{ 44, 42, 40 });
        }
        // The bars across the glass.
        for (int y = 3; y < 13; ++y)
        {
            t.set(5, y, Color{ 72, 68, 62 });
            t.set(10, y, Color{ 72, 68, 62 });
        }
    }

    void paintBarrelTop(Tile& t)
    {
        t.fill(Color{ 126, 94, 56 });
        // The lid, with a ring of hoop round it.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float edge = std::min(std::min(x, Tile::N - 1 - x),
                                            std::min(y, Tile::N - 1 - y));
                if (edge < 1.5f) t.set(x, y, Color{ 76, 62, 44 });
                else if (edge < 2.5f) t.set(x, y, Color{ 152, 116, 72 });
            }
        for (int x = 3; x < 13; ++x) t.set(x, 8, Color{ 86, 68, 46 });
        t.speckle(5, 533);
    }

    void paintBarrelSide(Tile& t)
    {
        t.fill(Color{ 118, 88, 52 });
        // Staves down it, and two hoops round it.
        for (int x = 0; x < Tile::N; ++x)
            if ((x % 4) == 3)
                for (int y = 0; y < Tile::N; ++y) t.set(x, y, Color{ 94, 70, 42 });
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 2, Color{ 62, 56, 48 });
            t.set(x, 3, Color{ 78, 70, 58 });
            t.set(x, Tile::N - 4, Color{ 78, 70, 58 });
            t.set(x, Tile::N - 3, Color{ 62, 56, 48 });
        }
        t.speckle(5, 534);
    }

    void paintCampfireTop(Tile& t)
    {
        t.fill(Color{ 72, 52, 34 });
        // Logs crossed, with the fire between them.
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, 4, Color{ 104, 76, 48 });
            t.set(i, 11, Color{ 104, 76, 48 });
        }
        for (int y = 5; y < 11; ++y)
            for (int x = 3; x < 13; ++x)
                t.set(x, y, (hash2(x, y, 535) > 0.45f) ? Color{ 250, 176, 56 }
                                                       : Color{ 226, 104, 36 });
        t.speckle(5, 536);
    }

    void paintCampfireLog(Tile& t)
    {
        t.fill(Color{ 92, 66, 42 });
        for (int y = 0; y < Tile::N; ++y)
            if ((y % 5) == 2)
                for (int x = 0; x < Tile::N; ++x) t.set(x, y, Color{ 68, 48, 32 });
        // Embers glowing underneath.
        for (int x = 0; x < Tile::N; ++x)
            if (hash2(x, 0, 537) > 0.6f) t.set(x, Tile::N - 1, Color{ 232, 120, 44 });
        t.speckle(5, 538);
    }

    // --- the workstations -------------------------------------------

    // The smoker and the blast furnace are the furnace in a different
    // shell: the mouth is painted the same way so that all three read
    // as the same machine, which is what they are.
    void paintKilnBody(Tile& t, Color base, int salt)
    {
        t.fill(base);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float n = hash2(x / 2, y / 2, salt);
                t.set(x, y, shade(t.get(x, y), n > 0.62f ? 16 : (n < 0.34f ? -18 : 0)));
            }
        t.speckle(6, salt + 1);
    }

    void paintKilnFace(Tile& t, Color base, int salt, bool lit, Color frame)
    {
        paintKilnBody(t, base, salt);

        const int n = Tile::N;
        const int x0 = n / 8, x1 = n - n / 8 - 1;
        const int y0 = n / 2 - n / 16, y1 = n - n / 8;

        const Color inner{ 44, 40, 38 };
        const Color mouth = lit ? Color{ 252, 170, 48 } : Color{ 14, 14, 16 };
        const Color ember = lit ? Color{ 255, 228, 138 } : Color{ 28, 28, 32 };

        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                const bool edge = (x == x0 || x == x1 || y == y0 || y == y1);
                if (edge) { t.set(x, y, frame); continue; }

                const bool rim = (x == x0 + 1 || x == x1 - 1 || y == y0 + 1 || y == y1 - 1);
                if (rim) { t.set(x, y, inner); continue; }

                t.set(x, y, hash2(x, y, salt + 2) > 0.6f ? ember : mouth);
            }
    }

    // Logs round the mouth, because a smoker is a furnace with wood
    // built round it and that is the only way to tell them apart.
    const Color SMOKER_WOOD{ 94, 70, 44 };
    const Color BLAST_STONE{ 66, 66, 70 };

    void paintSmokerFace(Tile& t, bool lit)
    {
        paintKilnFace(t, SMOKER_WOOD, 571, lit, Color{ 148, 112, 68 });
        // Grain down the staves either side of the mouth.
        for (int y = 0; y < Tile::N; ++y)
            if ((y % 5) == 1)
                for (int x = 0; x < 2; ++x)
                {
                    t.set(x, y, Color{ 74, 54, 34 });
                    t.set(Tile::N - 1 - x, y, Color{ 74, 54, 34 });
                }
    }

    void paintSmokerFront(Tile& t) { paintSmokerFace(t, false); }
    void paintSmokerFrontLit(Tile& t) { paintSmokerFace(t, true); }

    void paintSmokerTop(Tile& t)
    {
        paintKilnBody(t, SMOKER_WOOD, 572);
        // The hole the smoke leaves by.
        for (int y = 5; y < 11; ++y)
            for (int x = 5; x < 11; ++x)
                t.set(x, y, (x == 5 || x == 10 || y == 5 || y == 10)
                                ? Color{ 64, 46, 30 } : Color{ 26, 24, 22 });
    }

    void paintSmokerSide(Tile& t)
    {
        paintKilnBody(t, SMOKER_WOOD, 573);
        for (int y = 0; y < Tile::N; ++y)
            if ((y % 5) == 2)
                for (int x = 0; x < Tile::N; ++x) t.set(x, y, Color{ 70, 52, 32 });
    }

    void paintBlastFace(Tile& t, bool lit)
    {
        paintKilnFace(t, BLAST_STONE, 575, lit, Color{ 118, 118, 124 });
        // The two iron straps a blast furnace is bound with.
        for (int y = 1; y < 4; ++y)
            for (int x = 2; x < Tile::N - 2; ++x)
                if (y == 2) t.set(x, y, Color{ 176, 176, 182 });
    }

    void paintBlastFurnaceFront(Tile& t) { paintBlastFace(t, false); }
    void paintBlastFurnaceFrontLit(Tile& t) { paintBlastFace(t, true); }

    void paintBlastFurnaceTop(Tile& t)
    {
        paintKilnBody(t, BLAST_STONE, 576);
        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(k, 0, Color{ 44, 44, 48 }); t.set(k, Tile::N - 1, Color{ 44, 44, 48 });
            t.set(0, k, Color{ 44, 44, 48 }); t.set(Tile::N - 1, k, Color{ 44, 44, 48 });
        }
        for (int y = 6; y < 10; ++y)
            for (int x = 6; x < 10; ++x) t.set(x, y, Color{ 24, 22, 24 });
    }

    void paintBlastFurnaceSide(Tile& t)
    {
        paintKilnBody(t, BLAST_STONE, 577);
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 3, Color{ 150, 150, 156 });
            t.set(x, Tile::N - 4, Color{ 150, 150, 156 });
        }
    }

    // The composter: a slatted wooden box with whatever is rotting in
    // it showing over the rim.
    void paintComposterWood(Tile& t, int salt)
    {
        t.fill(Color{ 128, 100, 60 });
        for (int x = 0; x < Tile::N; ++x)
            if ((x % 4) == 3)
                for (int y = 0; y < Tile::N; ++y) t.set(x, y, Color{ 96, 72, 42 });
        t.speckle(6, salt);
    }

    void paintComposterSide(Tile& t)
    {
        paintComposterWood(t, 581);
        // The slats, which are what a composter is: a box with gaps.
        for (int y = 0; y < Tile::N; ++y)
            if ((y % 6) == 4)
                for (int x = 0; x < Tile::N; ++x) t.set(x, y, Color{ 74, 56, 34 });
    }

    void paintComposterBottom(Tile& t) { paintComposterWood(t, 582); }

    // The top is the rim with a hole in it; the fills put more and
    // more compost into that hole.
    void paintComposterIn(Tile& t, int level)
    {
        paintComposterWood(t, 583);
        for (int y = 2; y < Tile::N - 2; ++y)
            for (int x = 2; x < Tile::N - 2; ++x)
                t.set(x, y, Color{ 46, 34, 22 });

        if (level <= 0) return;

        // Three heights of compost, and a fourth that is ready: the
        // ready one is green, because that is the one worth opening.
        const int top = (level >= 4) ? 3 : (Tile::N - 2) - level * 3;
        const Color muck = (level >= 4) ? Color{ 112, 156, 62 } : Color{ 92, 72, 36 };
        const Color fleck = (level >= 4) ? Color{ 148, 192, 86 } : Color{ 122, 98, 50 };

        for (int y = std::max(3, top); y < Tile::N - 2; ++y)
            for (int x = 3; x < Tile::N - 3; ++x)
                t.set(x, y, hash2(x, y, 584 + level) > 0.55f ? fleck : muck);
    }

    void paintComposterTop(Tile& t)   { paintComposterIn(t, 0); }
    void paintComposterFill1(Tile& t) { paintComposterIn(t, 1); }
    void paintComposterFill2(Tile& t) { paintComposterIn(t, 2); }
    void paintComposterFill3(Tile& t) { paintComposterIn(t, 3); }
    void paintComposterReady(Tile& t) { paintComposterIn(t, 4); }

    // The grindstone: a wheel between two posts.
    void paintGrindstoneSide(Tile& t)
    {
        t.fill(Color{ 118, 92, 58 });
        for (int y = 0; y < Tile::N; ++y)
            if ((y % 5) == 2)
                for (int x = 0; x < Tile::N; ++x) t.set(x, y, Color{ 92, 70, 42 });

        // The wheel seen edge-on, which is the whole of what it does.
        for (int y = 2; y < Tile::N - 2; ++y)
            for (int x = 5; x < 11; ++x)
                t.set(x, y, hash2(x, y, 586) > 0.5f ? Color{ 136, 136, 140 }
                                                    : Color{ 108, 108, 112 });
        t.speckle(5, 587);
    }

    void paintGrindstoneFace(Tile& t)
    {
        t.fill(Color{ 118, 92, 58 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - 7.5f, dy = y - 7.5f;
                const float r = std::sqrt(dx * dx + dy * dy);
                if (r > 6.4f) continue;
                if (r < 1.6f) { t.set(x, y, Color{ 74, 56, 34 }); continue; }
                t.set(x, y, hash2(x, y, 588) > 0.5f ? Color{ 142, 142, 146 }
                                                    : Color{ 110, 110, 114 });
            }
        t.speckle(5, 589);
    }

    void paintGrindstonePivot(Tile& t)
    {
        t.fill(Color{ 118, 92, 58 });
        for (int y = 6; y < 10; ++y)
            for (int x = 0; x < Tile::N; ++x) t.set(x, y, Color{ 86, 64, 38 });
        for (int y = 5; y < 11; ++y)
            for (int x = 5; x < 11; ++x) t.set(x, y, Color{ 128, 128, 132 });
        t.speckle(5, 590);
    }

    // The lectern: a stand with a book open on it.
    void paintLecternBase(Tile& t)
    {
        t.fill(Color{ 120, 94, 58 });
        for (int x = 0; x < Tile::N; ++x)
            if ((x % 4) == 1)
                for (int y = 0; y < Tile::N; ++y) t.set(x, y, Color{ 94, 72, 44 });
        t.speckle(5, 592);
    }

    void paintLecternSide(Tile& t)
    {
        paintLecternBase(t);
        // The sloped desk across the top of it.
        for (int x = 0; x < Tile::N; ++x)
        {
            const int top = 2 + x / 3;
            for (int y = top; y < top + 3 && y < Tile::N; ++y)
                t.set(x, y, Color{ 150, 118, 74 });
        }
    }

    void paintLecternFront(Tile& t)
    {
        paintLecternBase(t);
        for (int y = 2; y < 6; ++y)
            for (int x = 1; x < Tile::N - 1; ++x)
                t.set(x, y, Color{ 152, 120, 76 });
    }

    void paintLecternTop(Tile& t)
    {
        t.fill(Color{ 134, 104, 64 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (((x + y) % 7) == 0) t.set(x, y, Color{ 118, 90, 54 });
        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(k, 0, Color{ 100, 76, 46 }); t.set(k, Tile::N - 1, Color{ 100, 76, 46 });
            t.set(0, k, Color{ 100, 76, 46 }); t.set(Tile::N - 1, k, Color{ 100, 76, 46 });
        }
        t.speckle(4, 594);
    }

    // The same desk with a book open on it. Minecraft draws that book
    // as a model and so has no texture for it; this engine draws cubes
    // and so needs one.
    void paintLecternTopBook(Tile& t)
    {
        t.fill(Color{ 134, 104, 64 });
        // A book open on the desk, spine down the middle.
        for (int y = 2; y < Tile::N - 2; ++y)
            for (int x = 2; x < Tile::N - 2; ++x)
                t.set(x, y, Color{ 232, 224, 198 });
        for (int y = 2; y < Tile::N - 2; ++y)
        {
            t.set(7, y, Color{ 148, 54, 44 });
            t.set(8, y, Color{ 148, 54, 44 });
        }
        for (int y = 4; y < Tile::N - 4; y += 2)
            for (int x = 3; x < 7; ++x)
            {
                t.set(x, y, Color{ 176, 168, 148 });
                t.set(Tile::N - 1 - x, y, Color{ 176, 168, 148 });
            }
        t.speckle(4, 593);
    }

    // The cauldron: an iron pot, empty or with water standing in it.
    void paintCauldronIron(Tile& t, int salt)
    {
        t.fill(Color{ 72, 72, 76 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float n = hash2(x / 2, y / 2, salt);
                t.set(x, y, shade(t.get(x, y), n > 0.6f ? 14 : (n < 0.35f ? -14 : 0)));
            }
    }

    void paintCauldronSide(Tile& t)
    {
        paintCauldronIron(t, 595);
        // The band round the top, and the feet under it.
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 0, Color{ 108, 108, 114 });
            t.set(x, 1, Color{ 96, 96, 102 });
        }
        for (int y = Tile::N - 3; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (x > 2 && x < Tile::N - 3) t.set(x, y, Color{ 40, 40, 44 });
    }

    void paintCauldronBottom(Tile& t) { paintCauldronIron(t, 596); }

    void paintCauldronIn(Tile& t, int water)
    {
        paintCauldronIron(t, 597);
        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(k, 0, Color{ 104, 104, 110 }); t.set(k, Tile::N - 1, Color{ 104, 104, 110 });
            t.set(0, k, Color{ 104, 104, 110 }); t.set(Tile::N - 1, k, Color{ 104, 104, 110 });
        }

        // Inside it: the dark of an empty pot, or water at one of three
        // depths. Depth is the only thing a cauldron has to say, and
        // this is the face it says it on.
        const Color empty{ 28, 28, 30 };
        for (int y = 2; y < Tile::N - 2; ++y)
            for (int x = 2; x < Tile::N - 2; ++x)
                t.set(x, y, empty);

        if (water <= 0) return;

        const int inset = 3 - water;        // deeper water fills more of it
        const Color top{ 60, 110, 200 };
        const Color deep{ 42, 86, 170 };
        for (int y = 2 + inset; y < Tile::N - 2 - inset; ++y)
            for (int x = 2 + inset; x < Tile::N - 2 - inset; ++x)
                t.set(x, y, hash2(x, y, 598 + water) > 0.55f ? top : deep);
    }

    void paintCauldronTop(Tile& t)    { paintCauldronIn(t, 0); }
    void paintCauldronWater1(Tile& t) { paintCauldronIn(t, 1); }
    void paintCauldronWater2(Tile& t) { paintCauldronIn(t, 2); }
    void paintCauldronWater3(Tile& t) { paintCauldronIn(t, 3); }

    // --- the other plants ---------------------------------------------

    // Painted off the same shape the carrot uses, which is defined
    // further down beside the other two crops.
    void paintRootCropStage(Tile& t, int stage, Color leaf, Color root);

    // Beetroot: a low leafy crop with the red crown showing once it
    // is grown. A root crop is a root crop.
    void paintBeetrootStage(Tile& t, int stage)
    {
        paintRootCropStage(t, stage, Color{ 66, 128, 48 }, Color{ 164, 42, 56 });
        if (stage >= 5)
            for (int i = 0; i < Tile::N; i += 4)
                t.set(i + 1, Tile::N - 3, Color{ 196, 60, 74 });
    }

    void paintBeetroot0(Tile& t) { paintBeetrootStage(t, 0); }
    void paintBeetroot1(Tile& t) { paintBeetrootStage(t, 1); }
    void paintBeetroot2(Tile& t) { paintBeetrootStage(t, 2); }
    void paintBeetroot3(Tile& t) { paintBeetrootStage(t, 3); }
    void paintBeetroot4(Tile& t) { paintBeetrootStage(t, 4); }
    void paintBeetroot5(Tile& t) { paintBeetrootStage(t, 5); }
    void paintBeetroot6(Tile& t) { paintBeetrootStage(t, 6); }
    void paintBeetroot7(Tile& t) { paintBeetrootStage(t, 7); }

    void paintMelonSide(Tile& t)
    {
        t.fill(Color{ 118, 150, 42 });
        // The stripes down it, which are the whole of what tells a
        // melon from a block of moss at the size it is seen.
        for (int x = 0; x < Tile::N; ++x)
        {
            const int band = (x / 3) % 2;
            for (int y = 0; y < Tile::N; ++y)
                if (band == 0) t.set(x, y, Color{ 86, 118, 32 });
        }
        t.speckle(6, 601);
    }

    void paintMelonTop(Tile& t)
    {
        t.fill(Color{ 128, 160, 50 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - 7.5f, dy = y - 7.5f;
                if (dx * dx + dy * dy > 42.0f) continue;
                t.set(x, y, Color{ 96, 128, 36 });
            }
        t.speckle(6, 602);
    }

    // The stem. Young it is a green curl; ripe it is the brown one
    // that has a melon on the end of it.
    void paintMelonStem(Tile& t, Color vine)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 4; y < Tile::N; ++y)
        {
            const int x = 7 + static_cast<int>(2.0f * std::sin((y - 4) * 0.55f));
            t.set(x, y, vine);
            t.set(x + 1, y, shade(vine, -18));
        }
        for (int i = 0; i < 4; ++i)
        {
            t.set(4 + i, 6 + i / 2, vine);
            t.set(11 - i, 9 + i / 2, vine);
        }
    }

    void paintMelonStemYoung(Tile& t) { paintMelonStem(t, Color{ 96, 150, 54 }); }
    void paintMelonStemRipe(Tile& t)  { paintMelonStem(t, Color{ 150, 134, 48 }); }

    void paintBamboo(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A thin cane up the middle, with a knuckle every few blocks.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 6; x < 10; ++x)
                t.set(x, y, (x < 7) ? Color{ 108, 150, 46 } : Color{ 86, 126, 36 });
        for (int y = 2; y < Tile::N; y += 6)
            for (int x = 5; x < 11; ++x)
                t.set(x, y, Color{ 142, 176, 70 });
        // A leaf either side, so a cane is not a green stick.
        for (int i = 0; i < 4; ++i)
        {
            t.set(5 - i / 2, 4 + i, Color{ 84, 140, 48 });
            t.set(10 + i / 2, 9 + i, Color{ 84, 140, 48 });
        }
    }

    void paintItemBeetroot(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 5; y < 14; ++y)
            for (int x = 4; x < 12; ++x)
            {
                const float dx = (x - 7.5f) / 4.2f;
                const float dy = (y - 9.0f) / 4.6f;
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, (dx + dy < -0.4f) ? Color{ 186, 58, 74 }
                                              : Color{ 142, 30, 48 });
            }
        for (int i = 0; i < 4; ++i)
        {
            t.set(7 - i / 2, 4 - i / 2 + 1, Color{ 72, 140, 56 });
            t.set(9 + i / 2, 3 + i / 2, Color{ 96, 170, 72 });
        }
    }

    void paintSeedsIn(Tile& t, Color seed)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int i = 0; i < 9; ++i)
        {
            const int x = 4 + (i % 3) * 3 + ((i / 3) % 2);
            const int y = 5 + (i / 3) * 3;
            t.set(x, y, seed);
            t.set(x + 1, y, shade(seed, -24));
            t.set(x, y + 1, shade(seed, -24));
        }
    }

    void paintItemBeetrootSeeds(Tile& t) { paintSeedsIn(t, Color{ 196, 148, 96 }); }
    void paintItemMelonSeeds(Tile& t)    { paintSeedsIn(t, Color{ 214, 206, 160 }); }

    void paintBowlIn(Tile& t, bool full)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 8; y < 14; ++y)
            for (int x = 2; x < 14; ++x)
            {
                const float dx = (x - 7.5f) / 6.0f;
                const float dy = (y - 8.0f) / 5.6f;
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, Color{ 158, 108, 72 });
            }
        for (int x = 3; x < 13; ++x)
        {
            t.set(x, 7, full ? Color{ 176, 46, 60 } : Color{ 112, 74, 48 });
            t.set(x, 8, full ? Color{ 148, 34, 48 } : Color{ 128, 88, 56 });
        }
        t.speckle(5, full ? 605 : 606);
    }

    void paintItemBowl(Tile& t)         { paintBowlIn(t, false); }
    void paintItemBeetrootSoup(Tile& t) { paintBowlIn(t, true); }

    void paintItemMelonSlice(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A wedge: rind at the bottom, flesh above it, pips in it.
        for (int y = 3; y < 14; ++y)
        {
            const int half = (y - 2);
            for (int x = 8 - half; x <= 7 + half; ++x)
            {
                if (x < 0 || x >= Tile::N) continue;
                t.set(x, y, (y >= 12) ? Color{ 88, 128, 36 }
                                      : Color{ 226, 74, 82 });
            }
        }
        for (int i = 0; i < 4; ++i)
            t.set(5 + i * 2, 8 + (i % 2) * 2, Color{ 44, 36, 30 });
    }

    // --- what is made rather than grown ---------------------------------

    void paintCocoaStage(Tile& t, int stage)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A pod hung off the trunk, bigger and browner as it ripens.
        const int radius = 3 + stage;
        const Color skin = (stage == 0) ? Color{ 128, 164, 70 }
                         : (stage == 1) ? Color{ 162, 122, 52 }
                                        : Color{ 142, 78, 36 };

        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = (x - 7.5f) / radius;
                const float dy = (y - 6.0f) / (radius + 1.0f);
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, (dx + dy < -0.3f) ? shade(skin, 26) : skin);
            }

        // The stalk it hangs by.
        for (int y = 0; y < 3; ++y) t.set(7, y, Color{ 92, 70, 44 });
        t.speckle(5, 611 + stage);
    }

    void paintCocoa0(Tile& t) { paintCocoaStage(t, 0); }
    void paintCocoa1(Tile& t) { paintCocoaStage(t, 1); }
    void paintCocoa2(Tile& t) { paintCocoaStage(t, 2); }

    void paintCakeTop(Tile& t)
    {
        t.fill(Color{ 244, 240, 232 });
        // Icing, with the cherries Minecraft puts on it.
        for (int y = 1; y < Tile::N - 1; ++y)
            for (int x = 1; x < Tile::N - 1; ++x)
                t.set(x, y, Color{ 236, 230, 220 });
        for (int i = 0; i < 4; ++i)
        {
            const int x = 3 + (i % 2) * 8;
            const int y = 3 + (i / 2) * 8;
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                    t.set(x + dx, y + dy, Color{ 192, 44, 48 });
        }
        t.speckle(4, 615);
    }

    void paintCakeSide(Tile& t)
    {
        t.fill(Color{ 166, 112, 70 });
        // Icing over the top third, sponge below it.
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < Tile::N; ++x)
                t.set(x, y, (y < 4) ? Color{ 236, 230, 220 } : Color{ 206, 198, 184 });
        t.speckle(6, 616);
    }

    void paintCakeBottom(Tile& t)
    {
        t.fill(Color{ 140, 92, 56 });
        t.speckle(6, 617);
    }

    // The cut face: sponge all the way up, with no icing on it. This
    // is what tells a cake somebody has started from a whole one.
    void paintCakeInner(Tile& t)
    {
        t.fill(Color{ 196, 140, 88 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if (((x + y) % 5) == 0) t.set(x, y, Color{ 172, 118, 70 });
        t.speckle(6, 618);
    }

    void paintCandleIn(Tile& t, bool lit)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 6; y < Tile::N; ++y)
            for (int x = 6; x < 10; ++x)
                t.set(x, y, (x < 7) ? Color{ 238, 232, 214 } : Color{ 214, 206, 186 });

        // The wick, and the flame on top of it when it is alight.
        t.set(7, 5, Color{ 60, 52, 44 });
        t.set(8, 5, Color{ 60, 52, 44 });
        if (!lit) return;

        for (int y = 1; y < 5; ++y)
            for (int x = 6; x < 10; ++x)
            {
                const bool core = (y > 2 && x > 6 && x < 9);
                t.set(x, y, core ? Color{ 255, 238, 170 } : Color{ 248, 170, 54 });
            }
    }

    void paintCandle(Tile& t)    { paintCandleIn(t, false); }
    void paintCandleLit(Tile& t) { paintCandleIn(t, true); }

    void paintScaffoldSide(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // Two uprights and the rungs between them: you see through it,
        // which is most of what makes scaffolding read as scaffolding.
        const Color pole{ 196, 160, 94 };
        const Color dark{ 158, 124, 68 };
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < 3; ++x)
            {
                t.set(x, y, (x == 0) ? dark : pole);
                t.set(Tile::N - 1 - x, y, (x == 0) ? dark : pole);
            }
        for (int y = 1; y < Tile::N; y += 7)
            for (int x = 3; x < Tile::N - 3; ++x)
            {
                t.set(x, y, pole);
                t.set(x, y + 1, dark);
            }
    }

    void paintScaffoldTop(Tile& t)
    {
        t.fill(Color{ 186, 150, 88 });
        for (int y = 3; y < Tile::N - 3; ++y)
            for (int x = 3; x < Tile::N - 3; ++x)
                t.set(x, y, Color{ 152, 118, 64 });
        t.speckle(5, 621);
    }

    void paintScaffoldBottom(Tile& t)
    {
        paintScaffoldSide(t);
    }

    void paintItemEgg(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 3; y < 14; ++y)
            for (int x = 4; x < 12; ++x)
            {
                const float dx = (x - 7.5f) / 3.8f;
                const float dy = (y - 9.0f) / 5.4f;
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, (dx + dy < -0.4f) ? Color{ 246, 240, 226 }
                                              : Color{ 214, 204, 184 });
            }
        t.speckle(4, 623);
    }

    void paintItemMilkBucket(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 4; y < 14; ++y)
        {
            const int inset = (y - 4) / 5;
            for (int x = 3 + inset; x < 13 - inset; ++x)
                t.set(x, y, (x < 5) ? Color{ 186, 190, 196 } : Color{ 150, 154, 162 });
        }
        for (int x = 4; x < 12; ++x)
        {
            t.set(x, 4, Color{ 248, 248, 244 });
            t.set(x, 5, Color{ 234, 234, 230 });
        }
        t.speckle(4, 624);
    }

    void paintItemCake(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 5; y < 13; ++y)
            for (int x = 2; x < 14; ++x)
                t.set(x, y, (y < 8) ? Color{ 236, 230, 220 } : Color{ 174, 118, 72 });
        for (int i = 0; i < 3; ++i)
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                    t.set(4 + i * 4 + dx, 5 + dy, Color{ 192, 44, 48 });
        t.speckle(4, 625);
    }

    void paintItemCookie(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 3; y < 14; ++y)
            for (int x = 3; x < 14; ++x)
            {
                const float dx = x - 8.0f, dy = y - 8.5f;
                if (dx * dx + dy * dy > 28.0f) continue;
                t.set(x, y, Color{ 186, 138, 84 });
            }
        for (int i = 0; i < 5; ++i)
            t.set(5 + (i * 3) % 8, 6 + (i * 2) % 6, Color{ 78, 52, 34 });
        t.speckle(4, 626);
    }

    void paintItemCocoaBeans(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int i = 0; i < 5; ++i)
        {
            const int x = 4 + (i % 3) * 3;
            const int y = 5 + (i / 3) * 4;
            for (int dy = 0; dy < 3; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                    t.set(x + dx, y + dy, (dx == 0) ? Color{ 142, 78, 36 }
                                                    : Color{ 108, 58, 26 });
        }
    }

    // A plank board. Minecraft draws its signs from an entity sheet
    // rather than a block texture, so there is nothing in a pack to
    // read: this is what a sign is made of, and the words go over it.
    void paintSignBoard(Tile& t)
    {
        t.fill(Color{ 166, 134, 86 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
                if ((y % 5) == 1) t.set(x, y, Color{ 142, 112, 70 });
        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(k, 0, Color{ 118, 92, 56 }); t.set(k, Tile::N - 1, Color{ 118, 92, 56 });
            t.set(0, k, Color{ 118, 92, 56 }); t.set(Tile::N - 1, k, Color{ 118, 92, 56 });
        }
        t.speckle(5, 631);
    }

    // The frame: a wooden border with nothing in the middle, so what
    // is in it shows through.
    void paintItemFrameBorder(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const int edge = std::min(std::min(x, Tile::N - 1 - x),
                                          std::min(y, Tile::N - 1 - y));
                if (edge >= 3) continue;
                t.set(x, y, (edge == 0) ? Color{ 108, 84, 52 }
                          : (edge == 1) ? Color{ 186, 154, 102 }
                                        : Color{ 146, 116, 72 });
            }
    }

    void paintItemElytra(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // Two folded wings, pale and veined, which is what Minecraft's
        // look like in a slot.
        for (int y = 2; y < 14; ++y)
        {
            const int wide = 1 + (y - 2) / 3;
            for (int o = 0; o <= wide; ++o)
            {
                const Color shade = (o == wide) ? Color{ 128, 118, 128 }
                                                : Color{ 196, 186, 196 };
                t.set(6 - o, y, shade);
                t.set(9 + o, y, shade);
            }
        }
        for (int y = 3; y < 13; y += 3)
        {
            t.set(5, y, Color{ 104, 96, 108 });
            t.set(10, y, Color{ 104, 96, 108 });
        }
        for (int y = 2; y < 14; ++y)
        {
            t.set(7, y, Color{ 92, 86, 96 });
            t.set(8, y, Color{ 92, 86, 96 });
        }
    }

    void paintShulkerBox(Tile& t)
    {
        t.fill(Color{ 148, 112, 158 });
        // The lid, and the seam round the middle where it opens.
        for (int y = 0; y < 6; ++y)
            for (int x = 0; x < Tile::N; ++x)
                t.set(x, y, Color{ 186, 152, 196 });
        for (int x = 0; x < Tile::N; ++x)
        {
            t.set(x, 6, Color{ 96, 68, 108 });
            t.set(x, 7, Color{ 118, 88, 130 });
        }
        for (int k = 0; k < Tile::N; ++k)
        {
            t.set(0, k, Color{ 108, 78, 118 });
            t.set(Tile::N - 1, k, Color{ 108, 78, 118 });
        }
        t.speckle(5, 645);
    }

    void paintItemShulkerShell(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A domed shell, pale purple, with the ridges across it.
        for (int y = 4; y < 14; ++y)
            for (int x = 3; x < 13; ++x)
            {
                const float dx = (x - 7.5f) / 4.6f;
                const float dy = (y - 13.0f) / 9.0f;
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, (dx + dy < -0.5f) ? Color{ 226, 206, 230 }
                                              : Color{ 184, 156, 196 });
            }
        for (int y = 6; y < 14; y += 3)
            for (int x = 4; x < 12; ++x)
                if (t.get(x, y).a > 0) t.set(x, y, Color{ 142, 112, 156 });
        t.speckle(4, 641);
    }

    void paintItemTrident(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A shaft up the diagonal with three prongs at the top of it.
        for (int i = 0; i < 12; ++i)
            t.set(3 + i, 13 - i, Color{ 86, 118, 120 });
        for (int i = 0; i < 12; ++i)
            t.set(4 + i, 13 - i, Color{ 58, 86, 92 });

        const int tipX = 12, tipY = 3;
        for (int k = 0; k < 4; ++k)
        {
            t.set(tipX - 3 + k, tipY + 3 - k, Color{ 162, 200, 200 });
            t.set(tipX, tipY - 1 + k, Color{ 162, 200, 200 });
            t.set(tipX + 2, tipY + 1 + k, Color{ 162, 200, 200 });
        }
        for (int x = tipX - 3; x <= tipX + 2; ++x) t.set(x, tipY + 4, Color{ 110, 150, 152 });
    }

    void paintItemSign(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 2; y < 10; ++y)
            for (int x = 2; x < 14; ++x)
                t.set(x, y, (y < 4) ? Color{ 182, 148, 98 } : Color{ 160, 128, 82 });
        for (int y = 5; y < 8; ++y)
            for (int x = 4; x < 12; ++x)
                if ((y % 2) == 1) t.set(x, y, Color{ 120, 94, 58 });
        for (int y = 10; y < Tile::N - 1; ++y)
            for (int x = 7; x < 9; ++x) t.set(x, y, Color{ 128, 100, 62 });
        t.speckle(4, 632);
    }

    void paintSpawnerIn(Tile& t, Color bars, Color inside)
    {
        t.fill(inside);
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, 0, bars);
            t.set(i, Tile::N - 1, bars);
            t.set(0, i, bars);
            t.set(Tile::N - 1, i, bars);
        }
        // A cage, drawn as a grid rather than as a texture: it is the
        // one block you are meant to read from across a room.
        for (int i = 0; i < Tile::N; i += 3)
            for (int k = 0; k < Tile::N; ++k)
            {
                t.set(i, k, bars);
                t.set(k, i, bars);
            }
    }

    void paintTrialSpawnerSide(Tile& t)
    { paintSpawnerIn(t, Color{ 46, 50, 54 }, Color{ 26, 30, 34 }); }
    void paintTrialSpawnerSideLit(Tile& t)
    { paintSpawnerIn(t, Color{ 56, 62, 66 }, Color{ 196, 108, 32 }); }
    void paintTrialSpawnerTop(Tile& t)
    { paintSpawnerIn(t, Color{ 52, 56, 60 }, Color{ 32, 36, 40 }); }
    void paintTrialSpawnerTopLit(Tile& t)
    { paintSpawnerIn(t, Color{ 62, 68, 72 }, Color{ 230, 148, 44 }); }

    void paintVaultIn(Tile& t, Color face, Color keyhole)
    {
        t.fill(Color{ 54, 58, 62 });
        t.speckle(8, 285);
        for (int y = 2; y <= 13; ++y)
            for (int x = 2; x <= 13; ++x)
                t.set(x, y, shade(face, static_cast<int>((hash2(x, y, 286) - 0.5f) * 20.0f)));

        // The keyhole, which is the whole of what tells the two apart.
        for (int y = 6; y <= 10; ++y)
            for (int x = 6; x <= 9; ++x)
            {
                const int d = (x - 7) * (x - 7) + (y - 7) * (y - 7);
                if (d <= 4) t.set(x, y, keyhole);
            }
    }

    void paintVaultSide(Tile& t)     { paintVaultIn(t, Color{ 72, 78, 82 }, Color{ 18, 20, 22 }); }
    void paintVaultSideOpen(Tile& t) { paintVaultIn(t, Color{ 86, 94, 98 }, Color{ 248, 206, 92 }); }
    void paintVaultTop(Tile& t)
    {
        t.fill(Color{ 66, 70, 74 });
        t.speckle(9, 287);
    }

    // A piston reads by its faces: planks and iron down the sides, a
    // pale face where the head comes out, a slime-green one if it is
    // sticky, and a ring on the back where the body sits once it has
    // pushed.
    void paintPistonSide(Tile& t)
    {
        paintPlanksIn(t, Color{ 148, 118, 72 }, 91);
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, 0, Color{ 122, 122, 126 });
            t.set(i, Tile::N - 1, Color{ 102, 102, 106 });
        }
    }

    void paintPistonTop(Tile& t)
    {
        t.fill(Color{ 118, 118, 122 });
        t.speckle(8, 92);
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, 0, Color{ 142, 142, 146 });
            t.set(0, i, Color{ 142, 142, 146 });
        }
    }

    void paintPistonFaceIn(Tile& t, Color pad)
    {
        t.fill(Color{ 128, 108, 72 });
        t.speckle(7, 93);
        // The plate, inset, with a crack across the middle of it.
        for (int y = 2; y <= 13; ++y)
            for (int x = 2; x <= 13; ++x)
                t.set(x, y, shade(pad, static_cast<int>((hash2(x, y, 94) - 0.5f) * 22.0f)));
        for (int x = 2; x <= 13; ++x) t.set(x, 7, shade(pad, -40));
    }

    void paintPistonFace(Tile& t)       { paintPistonFaceIn(t, Color{ 166, 162, 150 }); }
    void paintStickyPistonFace(Tile& t) { paintPistonFaceIn(t, Color{ 138, 186, 110 }); }

    void paintPistonBack(Tile& t)
    {
        t.fill(Color{ 112, 112, 116 });
        t.speckle(9, 95);
        for (int y = 5; y <= 10; ++y)
            for (int x = 5; x <= 10; ++x)
                t.set(x, y, Color{ 84, 84, 88 });
    }

    void paintPistonArm(Tile& t)
    {
        t.fill(Color{ 128, 108, 72 });
        t.speckle(6, 96);
        // The shaft down the middle of the arm.
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 6; x <= 9; ++x)
                t.set(x, y, shade(Color{ 150, 150, 154 },
                                  static_cast<int>((hash2(x, y, 97) - 0.5f) * 20.0f)));
    }

    // The crafter: a bench with a grid cut into its top, lit when it
    // has just fired.
    void paintCrafterTopIn(Tile& t, Color hole)
    {
        t.fill(Color{ 104, 88, 64 });
        t.speckle(8, 98);
        for (int gy = 0; gy < 3; ++gy)
            for (int gx = 0; gx < 3; ++gx)
                for (int y = 0; y < 3; ++y)
                    for (int x = 0; x < 3; ++x)
                        t.set(3 + gx * 5 + x, 3 + gy * 5 + y, hole);
    }

    void paintCrafterTop(Tile& t)    { paintCrafterTopIn(t, Color{ 44, 38, 30 }); }
    void paintCrafterTopLit(Tile& t) { paintCrafterTopIn(t, Color{ 236, 158, 60 }); }

    void paintCrafterSide(Tile& t)
    {
        paintPlanksIn(t, Color{ 118, 96, 66 }, 99);
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(0, i, Color{ 88, 88, 92 });
            t.set(Tile::N - 1, i, Color{ 88, 88, 92 });
        }
    }

    // Copper, through its four ages: the new metal, a patchy brown as
    // the shine goes, a green creeping over it, and the green all the
    // way. Each stage is the last one with more of the next colour
    // blotched across it, which is what weathering looks like.
    void paintCopperIn(Tile& t, Color metal, Color patch, float coverage, uint32_t salt)
    {
        t.fill(metal);
        t.speckle(10, salt);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float n = hash2(x / 2, y / 2, salt + 1) * 0.6f +
                                hash2(x, y, salt + 2) * 0.4f;
                if (n < coverage)
                    t.set(x, y, shade(patch, static_cast<int>((hash2(x, y, salt + 3) - 0.5f) * 26.0f)));
            }
    }

    void paintCopperBlock(Tile& t)
    { paintCopperIn(t, Color{ 192, 107, 73 }, Color{ 210, 126, 88 }, 0.22f, 301); }
    void paintExposedCopper(Tile& t)
    { paintCopperIn(t, Color{ 161, 124, 100 }, Color{ 136, 150, 118 }, 0.34f, 305); }
    void paintWeatheredCopper(Tile& t)
    { paintCopperIn(t, Color{ 108, 155, 118 }, Color{ 134, 126, 104 }, 0.40f, 309); }
    void paintOxidizedCopper(Tile& t)
    { paintCopperIn(t, Color{ 82, 162, 132 }, Color{ 104, 176, 146 }, 0.34f, 313); }

    void paintCopperOre(Tile& t)
    {
        paintStone(t);
        for (int i = 0; i < 14; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 317) * (Tile::N - 2));
            const int y = static_cast<int>(hash2(i, 1, 317) * (Tile::N - 2));
            for (int dy = 0; dy <= 1; ++dy)
                for (int dx = 0; dx <= 1; ++dx)
                    t.set(x + dx, y + dy, Color{ 192, 107, 73 });
        }
    }

    void paintCopperBulbIn(Tile& t, Color core, Color frame)
    {
        t.fill(frame);
        t.speckle(8, 321);
        // A window in each face with the filament behind it.
        for (int y = 3; y <= 12; ++y)
            for (int x = 3; x <= 12; ++x)
                t.set(x, y, shade(core, static_cast<int>((hash2(x, y, 322) - 0.5f) * 24.0f)));
        for (int i = 3; i <= 12; ++i)
        {
            t.set(i, 3, frame);
            t.set(i, 12, frame);
            t.set(3, i, frame);
            t.set(12, i, frame);
        }
    }

    void paintCopperBulb(Tile& t)
    { paintCopperBulbIn(t, Color{ 92, 74, 58 }, Color{ 176, 104, 72 }); }
    void paintCopperBulbLit(Tile& t)
    { paintCopperBulbIn(t, Color{ 255, 226, 150 }, Color{ 198, 122, 82 }); }

    // The two sorts of cave: one that drips and one that grows.
    void paintDripstoneBlock(Tile& t)
    {
        t.fill(Color{ 134, 107, 92 });
        t.speckle(12, 331);
        for (int i = 0; i < 20; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 332) * Tile::N);
            const int y = static_cast<int>(hash2(i, 1, 332) * Tile::N);
            t.set(x, y, shade(Color{ 134, 107, 92 }, -28));
        }
    }

    void paintDripstoneSpike(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A cone, point down, which is what one looks like from the
        // side -- and the cross it is drawn on shows two of them.
        for (int y = 0; y < Tile::N; ++y)
        {
            const int half = (Tile::N - y) / 4;
            for (int x = 7 - half; x <= 8 + half; ++x)
            {
                if (x < 0 || x >= Tile::N) continue;
                t.set(x, y, shade(Color{ 146, 116, 99 },
                                  static_cast<int>((hash2(x, y, 333) - 0.5f) * 30.0f)));
            }
        }
    }

    void paintMoss(Tile& t)
    {
        t.fill(Color{ 88, 110, 54 });
        t.speckle(16, 334);
        for (int i = 0; i < 26; ++i)
        {
            const int x = static_cast<int>(hash2(i, 0, 335) * Tile::N);
            const int y = static_cast<int>(hash2(i, 1, 335) * Tile::N);
            t.set(x, y, shade(Color{ 70, 94, 44 }, 0));
        }
    }

    void paintAzaleaLeaves(Tile& t)
    {
        paintLeavesBase(t, Color{ 86, 118, 52 }, 336);
        // The flowers, which are the whole of why anybody looks twice
        // at an azalea.
        for (int i = 0; i < 9; ++i)
        {
            const int x = 1 + static_cast<int>(hash2(i, 0, 337) * 13.0f);
            const int y = 1 + static_cast<int>(hash2(i, 1, 337) * 13.0f);
            t.set(x, y, Color{ 222, 160, 206 });
            t.set(x + 1, y, Color{ 196, 128, 180 });
            t.set(x, y + 1, Color{ 196, 128, 180 });
        }
    }

    void paintGlowBerries(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A vine down the middle with the berries hung off it.
        for (int y = 0; y < Tile::N; ++y)
        {
            t.set(7, y, Color{ 86, 104, 50 });
            t.set(8, y, Color{ 68, 86, 40 });
        }
        for (int i = 0; i < 5; ++i)
        {
            const int y = 2 + i * 3;
            const int x = (i % 2) ? 9 : 5;
            for (int dy = 0; dy <= 1; ++dy)
                for (int dx = 0; dx <= 1; ++dx)
                    t.set(x + dx, y + dy, Color{ 255, 196, 86 });
        }
    }

    // The ominous vault: the same lock in a colder metal, with the
    // keyhole glowing a different colour. It has to read as a vault
    // and as not the same vault.
    void paintOminousVaultSide(Tile& t)     { paintVaultIn(t, Color{ 60, 66, 84 }, Color{ 14, 16, 24 }); }
    void paintOminousVaultSideOpen(Tile& t) { paintVaultIn(t, Color{ 74, 82, 104 }, Color{ 142, 212, 248 }); }

    // A spawner is a cage you can see into, which is the whole of how
    // you know to break it before whatever is inside gets out.
    void paintSpawner(Tile& t)
    {
        t.fill(Color{ 26, 30, 32, 215 });
        for (int i = 0; i < Tile::N; ++i)
            for (int k = 0; k < Tile::N; ++k)
                if ((i % 4) == 0 || (k % 4) == 0)
                    t.set(i, k, Color{ 72, 80, 84 });
        for (int i = 0; i < Tile::N; ++i)
        {
            t.set(i, 0, Color{ 86, 94, 98 });
            t.set(i, Tile::N - 1, Color{ 86, 94, 98 });
            t.set(0, i, Color{ 86, 94, 98 });
            t.set(Tile::N - 1, i, Color{ 86, 94, 98 });
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

    // A block breaking is one crack spreading, not ten unrelated ones
    // shown in turn. Nothing here is seeded on the stage: crack i follows
    // the same path at every stage and simply reaches further, and stage s
    // draws cracks 0..s, so each overlay strictly contains the one before.
    void paintCrack(Tile& t, int stage)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const uint8_t alpha = static_cast<uint8_t>(110 + stage * 12);

        for (int i = 0; i <= stage; ++i)
        {
            const float angle = hash2(i, 0, 281) * 6.2831853f;
            float x = 8.0f, y = 8.0f;
            float dx = std::cos(angle), dy = std::sin(angle);

            const int length = 3 + static_cast<int>(hash2(i, 1, 282) * 3.0f) + (stage - i);
            for (int k = 0; k < length; ++k)
            {
                t.set(static_cast<int>(x), static_cast<int>(y), Color{ 15, 15, 15, alpha });
                x += dx;
                y += dy;
                dx += (hash2(i * 31 + k, 0, 283) - 0.5f) * 0.6f;
                dy += (hash2(i * 17 + k, 0, 284) - 0.5f) * 0.6f;
                const float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.001f) { dx /= len; dy /= len; }
            }
        }
    }

    // The same overlay as a plain buffer, so --selftest can check that
    // each stage really does contain the one before it.
    std::vector<uint8_t> crackStagePixels(int stage)
    {
        Tile tile;
        paintCrack(tile, stage);

        std::vector<uint8_t> out(static_cast<size_t>(Tile::N) * Tile::N * 4, 0);
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const Color c = tile.get(x, y);
                const size_t i = (static_cast<size_t>(y) * Tile::N + x) * 4;
                out[i] = c.r; out[i + 1] = c.g; out[i + 2] = c.b; out[i + 3] = c.a;
            }
        return out;
    }

    // ---------- resource pack loading ----------

    // How much of the height scale a tile has to use before it counts
    // as having any relief at all. Eight of 255 is about three percent.
    const int HEIGHT_MIN_SPAN = 8;

    const char* PACK_DIRECTORY = "assets/textures/block";
    // Items live in their own folder in a pack, and are plain flat icons
    // -- no tinting, no animation, no normal maps to go with them.
    const char* ITEM_PACK_DIRECTORY = "assets/textures/item";
    const char* CREDIT_FILE = "assets/textures/CREDIT.txt";
    const char* MOB_CREDIT_FILE = "assets/skins/mob/CREDIT.txt";

    // First non-blank, non-comment line of a credit file, trimmed and
    // capped to something that fits across the bottom of the screen.
    std::string readCreditFile(const char* path)
    {
        std::FILE* file = std::fopen(path, "rb");
        if (!file) return {};

        std::string credit;
        char line[256];
        bool first = true;
        while (std::fgets(line, sizeof(line), file))
        {
            std::string text(line);

            // Windows PowerShell writes UTF-8 with a byte-order mark, and
            // three stray bytes at the front of the file came out as
            // mojibake in front of the pack's name on the title screen.
            if (first && text.size() >= 3 &&
                static_cast<unsigned char>(text[0]) == 0xEF &&
                static_cast<unsigned char>(text[1]) == 0xBB &&
                static_cast<unsigned char>(text[2]) == 0xBF)
                text.erase(0, 3);
            first = false;
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

    // Blocks and mobs are installed separately and can come from
    // different people, so both are named -- on one line, because the
    // title screen has room for one.
    std::string readPackCredit()
    {
        const std::string blocks = readCreditFile(CREDIT_FILE);
        const std::string mobs = readCreditFile(MOB_CREDIT_FILE);

        if (blocks.empty()) return mobs;
        if (mobs.empty() || mobs == blocks) return blocks;
        return blocks + " / " + mobs;
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
    const Color REDSTONE_OFF_TINT{ 118, 16, 10, 255 };
    const Color REDSTONE_ON_TINT{ 252, 58, 34, 255 };

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

    // Carrots and potatoes. Both are leaf above and root below, so
    // they share a painter: what changes is the colour of the crown
    // and whether the root shows at the end.
    void paintRootCropStage(Tile& t, int stage, Color leaf, Color root)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const float grown = static_cast<float>(stage) / 7.0f;
        const int tall = 3 + static_cast<int>(grown * 9.0f);
        const int bottom = Tile::N - 1;

        for (int column = 0; column < 4; ++column)
        {
            const int x = 2 + column * 4;
            for (int y = bottom - tall; y <= bottom; ++y)
            {
                t.set(x, y, leaf);
                t.set(x + 1, y, shade(leaf, -28));
            }

            // Leaves spreading as it comes on.
            const int spread = static_cast<int>(grown * 2.0f);
            for (int w = 1; w <= spread; ++w)
            {
                t.set(x - w, bottom - tall + w, shade(leaf, 14));
                t.set(x + 1 + w, bottom - tall + w, shade(leaf, -14));
            }

            // And the root showing at the bottom once it is ripe,
            // which is how you know it is worth pulling.
            if (stage < 6) continue;
            for (int y = bottom - 2; y <= bottom; ++y)
            {
                t.set(x, y, root);
                t.set(x + 1, y, shade(root, -20));
            }
        }
    }

    void paintCarrotStage(Tile& t, int stage)
    {
        paintRootCropStage(t, stage, Color{ 74, 134, 54 }, Color{ 226, 132, 38 });
    }

    void paintPotatoStage(Tile& t, int stage)
    {
        paintRootCropStage(t, stage, Color{ 88, 140, 64 }, Color{ 198, 168, 102 });
    }

    void paintCarrot0(Tile& t) { paintCarrotStage(t, 0); }
    void paintCarrot1(Tile& t) { paintCarrotStage(t, 1); }
    void paintCarrot2(Tile& t) { paintCarrotStage(t, 2); }
    void paintCarrot3(Tile& t) { paintCarrotStage(t, 3); }
    void paintCarrot4(Tile& t) { paintCarrotStage(t, 4); }
    void paintCarrot5(Tile& t) { paintCarrotStage(t, 5); }
    void paintCarrot6(Tile& t) { paintCarrotStage(t, 6); }
    void paintCarrot7(Tile& t) { paintCarrotStage(t, 7); }
    void paintPotato0(Tile& t) { paintPotatoStage(t, 0); }
    void paintPotato1(Tile& t) { paintPotatoStage(t, 1); }
    void paintPotato2(Tile& t) { paintPotatoStage(t, 2); }
    void paintPotato3(Tile& t) { paintPotatoStage(t, 3); }
    void paintPotato4(Tile& t) { paintPotatoStage(t, 4); }
    void paintPotato5(Tile& t) { paintPotatoStage(t, 5); }
    void paintPotato6(Tile& t) { paintPotatoStage(t, 6); }
    void paintPotato7(Tile& t) { paintPotatoStage(t, 7); }

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

    // ---------- tools ----------
    //
    // One painter for all twenty-five: a handle on the diagonal with a
    // head drawn in the tier's metal. The heads are given as the pixels
    // they occupy on a 16x16 tile, read off the vanilla shapes, which is
    // what makes a pickaxe read as a pickaxe at a glance.

    const Color TIER_COLOR[5] = {
        { 150, 110,  62 },   // wood
        { 128, 128, 128 },   // stone
        { 216, 216, 216 },   // iron
        { 246, 208,  70 },   // gold
        {  94, 219, 208 },   // diamond
    };

    // Rows of the head, top to bottom, as "fill from x0 to x1 on row y".
    struct Span { int y, x0, x1; };

    const Span PICKAXE_HEAD[] = {
        { 2, 4, 12 }, { 3, 3, 5 }, { 3, 7, 8 }, { 3, 11, 13 },
        { 4, 2, 3 }, { 4, 13, 14 },
    };
    const Span AXE_HEAD[] = {
        { 2, 6, 10 }, { 3, 5, 11 }, { 4, 5, 11 }, { 5, 6, 10 }, { 6, 7, 9 },
    };
    const Span SHOVEL_HEAD[] = {
        { 2, 8, 11 }, { 3, 8, 11 }, { 4, 8, 11 }, { 5, 9, 10 },
    };
    const Span SWORD_HEAD[] = {
        { 1, 10, 12 }, { 2, 9, 12 }, { 3, 8, 11 }, { 4, 7, 10 },
        { 5, 6, 9 }, { 6, 5, 8 }, { 7, 5, 7 },
    };
    const Span HOE_HEAD[] = {
        { 2, 7, 12 }, { 3, 7, 9 }, { 3, 11, 12 }, { 4, 7, 8 },
    };

    struct HeadShape { const Span* spans; int count; };

    const HeadShape TOOL_HEADS[5] = {
        { PICKAXE_HEAD, static_cast<int>(sizeof(PICKAXE_HEAD) / sizeof(Span)) },
        { AXE_HEAD,     static_cast<int>(sizeof(AXE_HEAD) / sizeof(Span)) },
        { SHOVEL_HEAD,  static_cast<int>(sizeof(SHOVEL_HEAD) / sizeof(Span)) },
        { SWORD_HEAD,   static_cast<int>(sizeof(SWORD_HEAD) / sizeof(Span)) },
        { HOE_HEAD,     static_cast<int>(sizeof(HOE_HEAD) / sizeof(Span)) },
    };

    void paintToolTile(Tile& t, int kind, int tier)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const Color metal = TIER_COLOR[tier];
        const Color dark = shade(metal, -46);
        const Color handle{ 139, 104, 60 };
        const Color handleDark{ 102, 74, 40 };

        // The handle runs corner to corner on every one of them, and a
        // sword's is short because its blade takes up the diagonal.
        const int from = (kind == 3) ? 8 : 5;
        for (int i = from; i <= 13; ++i)
        {
            t.set(i, i, handle);
            if (i < 13) t.set(i + 1, i, handleDark);
        }

        const HeadShape& head = TOOL_HEADS[kind];
        for (int i = 0; i < head.count; ++i)
        {
            const Span& span = head.spans[i];
            for (int x = span.x0; x <= span.x1; ++x)
            {
                // A sword's blade is drawn along the other diagonal, so
                // its spans are read as a mirror of everything else.
                t.set(x, span.y, (x == span.x0 || span.y <= 2) ? metal : dark);
            }
        }

        // One highlight, so the head does not read as a flat blob.
        for (int i = 0; i < head.count && i < 2; ++i)
            t.set(head.spans[i].x0 + 1, head.spans[i].y, shade(metal, 40));
    }

    // Cooked meat is the raw cut, browned. Rather than four more hand
    // drawn icons, the raw painter is run and the result darkened and
    // warmed, which is what cooking looks like.
    void cookTile(Tile& t)
    {
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                Color c = t.get(x, y);
                if (c.a == 0) continue;
                c.r = static_cast<uint8_t>(std::min(255, c.r * 86 / 100 + 26));
                c.g = static_cast<uint8_t>(c.g * 62 / 100 + 12);
                c.b = static_cast<uint8_t>(c.b * 48 / 100);
                t.set(x, y, c);
            }
    }

    void paintItemSteak(Tile& t)         { paintItemRawBeef(t); cookTile(t); }
    void paintItemCookedPork(Tile& t)    { paintItemRawPork(t); cookTile(t); }
    void paintItemCookedChicken(Tile& t) { paintItemRawChicken(t); cookTile(t); }
    void paintItemCookedMutton(Tile& t)  { paintItemRawMutton(t); cookTile(t); }

    // ---------- armour ----------
    //
    // One painter for all sixteen, the same way the tools are done: the
    // silhouette of the piece in the tier's material.

    const Color ARMOUR_COLOR[4] = {
        { 166, 104,  66 },   // leather
        { 246, 208,  70 },   // gold
        { 216, 216, 216 },   // iron
        {  94, 219, 208 },   // diamond
    };

    const Span HELMET[] = {
        { 3, 4, 11 }, { 4, 3, 12 }, { 5, 3, 12 }, { 6, 3, 4 }, { 6, 11, 12 },
        { 7, 3, 4 }, { 7, 11, 12 }, { 8, 3, 4 }, { 8, 11, 12 },
    };
    const Span CHESTPLATE[] = {
        { 3, 3, 5 }, { 3, 10, 12 },
        { 4, 2, 13 }, { 5, 2, 13 }, { 6, 2, 13 },
        { 7, 3, 12 }, { 8, 3, 12 }, { 9, 3, 12 }, { 10, 4, 11 }, { 11, 4, 11 },
    };
    const Span LEGGINGS[] = {
        { 3, 3, 12 }, { 4, 3, 12 }, { 5, 3, 12 },
        { 6, 3, 6 }, { 6, 9, 12 }, { 7, 3, 6 }, { 7, 9, 12 },
        { 8, 3, 6 }, { 8, 9, 12 }, { 9, 3, 6 }, { 9, 9, 12 },
        { 10, 4, 6 }, { 10, 9, 11 },
    };
    const Span BOOTS[] = {
        { 6, 3, 6 }, { 6, 9, 12 },
        { 7, 3, 6 }, { 7, 9, 12 },
        { 8, 2, 7 }, { 8, 9, 13 },
        { 9, 2, 7 }, { 9, 9, 13 },
    };

    const HeadShape ARMOUR_SHAPES[4] = {
        { HELMET,     static_cast<int>(sizeof(HELMET) / sizeof(Span)) },
        { CHESTPLATE, static_cast<int>(sizeof(CHESTPLATE) / sizeof(Span)) },
        { LEGGINGS,   static_cast<int>(sizeof(LEGGINGS) / sizeof(Span)) },
        { BOOTS,      static_cast<int>(sizeof(BOOTS) / sizeof(Span)) },
    };

    void paintArmourTile(Tile& t, int piece, int tier)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const Color body = ARMOUR_COLOR[tier];
        const Color dark = shade(body, -44);
        const Color lit = shade(body, 38);

        const HeadShape& shape = ARMOUR_SHAPES[piece];
        for (int i = 0; i < shape.count; ++i)
        {
            const Span& span = shape.spans[i];
            for (int x = span.x0; x <= span.x1; ++x)
            {
                // Lit along the top, shaded down the sides, so the piece
                // has some shape to it rather than being a flat cut-out.
                const bool top = (i < 2);
                const bool edge = (x == span.x0 || x == span.x1);
                t.set(x, span.y, top ? lit : (edge ? dark : body));
            }
        }
    }

    // The bucket, and the two things it can be full of. One shape, with
    // whatever it holds painted into the bowl of it.
    void paintBucket(Tile& t, bool full, Color contents)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const Color iron{ 196, 196, 202 };
        const Color ironDark{ 132, 132, 138 };
        const Color ironLit{ 232, 232, 238 };

        // The body: a tapering tub.
        for (int y = 6; y <= 13; ++y)
        {
            const int inset = (y - 6) / 4;
            for (int x = 3 + inset; x <= 12 - inset; ++x)
            {
                const bool edge = (x == 3 + inset || x == 12 - inset || y == 13);
                t.set(x, y, edge ? ironDark : iron);
            }
        }

        // What it holds, sitting in the top of the tub.
        if (full)
            for (int y = 7; y <= 9; ++y)
                for (int x = 5; x <= 10; ++x)
                    t.set(x, y, (x + y) % 4 == 0 ? shade(contents, 30) : contents);

        // The rim and the handle over it.
        for (int x = 3; x <= 12; ++x) t.set(x, 5, ironLit);
        for (int x = 5; x <= 10; ++x) t.set(x, 3, ironDark);
        t.set(4, 4, ironDark);
        t.set(11, 4, ironDark);
    }

    void paintItemBucket(Tile& t)      { paintBucket(t, false, Color{}); }
    void paintItemWaterBucket(Tile& t) { paintBucket(t, true, Color{ 58, 106, 206 }); }
    void paintItemLavaBucket(Tile& t)  { paintBucket(t, true, Color{ 226, 110, 30 }); }

    void paintItemFlintAndSteel(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color steel{ 198, 198, 204 };
        const Color steelDark{ 134, 134, 140 };
        const Color stone{ 62, 58, 60 };
        const Color handle{ 140, 100, 56 };

        // The steel: a hooked bar down the left.
        for (int y = 3; y <= 11; ++y) { t.set(4, y, steel); t.set(5, y, steelDark); }
        for (int x = 4; x <= 8; ++x) t.set(x, 11, steel);
        for (int x = 4; x <= 7; ++x) t.set(x, 3, steelDark);

        // The handle it is set into.
        for (int y = 5; y <= 9; ++y)
            for (int x = 2; x <= 3; ++x)
                t.set(x, y, handle);

        // The flint, struck against it.
        for (int y = 5; y <= 9; ++y)
            for (int x = 9; x <= 13; ++x)
                if ((x + y) % 7 != 0) t.set(x, y, stone);
    }

    void paintItemFlint(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color stone{ 58, 54, 56 };
        const Color lit{ 96, 92, 94 };

        // A struck flake: broad at one end, tapering to a point.
        for (int y = 4; y <= 11; ++y)
        {
            const int inset = (y <= 5) ? 4 : (y >= 10 ? 3 : 1);
            for (int x = 2 + inset; x <= 13 - inset; ++x)
                t.set(x, y, (x + y) % 5 == 0 ? lit : stone);
        }
    }

    void paintItemBow(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color wood{ 142, 100, 54 };
        const Color woodDark{ 104, 72, 36 };
        const Color string{ 226, 226, 220 };

        // The stave, bowed from top-right round to bottom-right.
        const int arc[12][2] = {
            { 10, 2 }, { 12, 3 }, { 13, 5 }, { 13, 7 }, { 13, 8 }, { 13, 10 },
            { 12, 12 }, { 10, 13 }, { 8, 13 }, { 6, 12 }, { 5, 10 }, { 5, 5 },
        };
        for (const auto& p2 : arc)
        {
            t.set(p2[0], p2[1], wood);
            t.set(p2[0] - 1, p2[1], woodDark);
        }
        for (int y = 3; y <= 12; ++y) t.set(4, y, wood);

        // The string, drawn straight across the back of it.
        for (int y = 3; y <= 13; ++y) t.set(3, y, string);
        t.set(10, 2, string);
        t.set(10, 13, string);
    }

    void paintItemArrow(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color shaft{ 160, 120, 68 };
        const Color head{ 96, 92, 94 };
        const Color fletch{ 228, 228, 224 };

        // Point at the top-right, fletching at the bottom-left.
        for (int i = 4; i <= 11; ++i) t.set(i, 15 - i, shaft);

        t.set(12, 3, head); t.set(13, 2, head); t.set(12, 2, head); t.set(11, 3, head);
        t.set(13, 3, head); t.set(12, 4, head);

        t.set(3, 12, fletch); t.set(2, 13, fletch); t.set(4, 12, fletch);
        t.set(3, 13, fletch); t.set(2, 12, fletch); t.set(3, 11, fletch);
    }

    // The arrow as it flies: pointing right, filling the width, with
    // the shaft on the middle rows so a long thin quad shows all of it.
    void paintFireCharge(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int mid = Tile::N / 2;
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = (x - (mid - 1)) / 6.0f;
                const float dy = (y - (mid - 1)) / 5.0f;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > 1.0f) continue;

                // White at the heart, orange at the edge, with the
                // outermost ring left ragged so it reads as flame
                // rather than as a ball.
                if (d > 0.72f && hash2(x, y, 241) > 0.55f) continue;
                t.set(x, y, mixColor(Color{ 255, 248, 208 }, Color{ 226, 88, 16 },
                                     std::min(1.0f, d * 1.15f)));
            }
    }

    // A wind charge is a ring of moving air, so it is drawn as one:
    // pale, open in the middle, and nothing like the fire charge it
    // shares a quad with.
    void paintWindCharge(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const float mid = Tile::N / 2.0f - 0.5f;
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - mid, dy = y - mid;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > 6.5f || d < 2.6f) continue;
                if (hash2(x, y, 271) > 0.78f) continue;
                t.set(x, y, mixColor(Color{ 236, 248, 255 }, Color{ 150, 196, 214 },
                                     std::min(1.0f, (d - 2.6f) / 3.9f)));
            }
    }

    void paintArrowInFlight(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const int n = Tile::N;
        const int mid = n / 2;
        const Color shaft{ 168, 126, 72 };
        const Color shaftDark{ 124, 92, 50 };
        const Color head{ 148, 148, 154 };
        const Color fletch{ 236, 236, 232 };

        // Shaft down the middle, two rows thick.
        for (int x = 2; x < n - 3; ++x)
        {
            t.set(x, mid - 1, shaft);
            t.set(x, mid, shaftDark);
        }

        // The head, a wedge at the right-hand end.
        for (int i = 0; i < 4; ++i)
            for (int y = mid - 1 - i; y <= mid + i; ++y)
                t.set(n - 4 + i - 1, y, head);

        // Fletching at the left, three vanes.
        for (int i = 0; i < 4; ++i)
        {
            t.set(2 + i, mid - 2 - (3 - i) / 2, fletch);
            t.set(2 + i, mid + 1 + (3 - i) / 2, fletch);
        }
    }

    // An experience orb: a round green mote, pale at the top left where
    // the light falls and deepening to the far edge, with a white
    // highlight. Drawn as a disc rather than Minecraft's dithered blob,
    // because at this tile size dithering turns to mush.
    void paintExperienceOrb(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const int n = Tile::N;
        const float mid = (n - 1) * 0.5f;
        const float radius = n * 0.46f;

        for (int y = 0; y < n; ++y)
        {
            for (int x = 0; x < n; ++x)
            {
                const float dx = (x - mid) / radius;
                const float dy = (y - mid) / radius;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > 1.0f) continue;

                // Lit from the top left, the same corner everything
                // else in the atlas is lit from.
                const float lit = std::clamp(1.0f - (dx * 0.45f + dy * 0.45f) - d * 0.35f,
                                             0.0f, 1.0f);

                Color c;
                c.r = static_cast<unsigned char>(120 + 125 * lit);
                c.g = static_cast<unsigned char>(190 + 65 * lit);
                c.b = static_cast<unsigned char>(40 + 70 * lit);
                c.a = 255;
                t.set(x, y, c);
            }
        }

        // A hard white glint, so it reads as something that shines
        // rather than a flat green pip.
        const int gx = static_cast<int>(mid - radius * 0.38f);
        const int gy = static_cast<int>(mid - radius * 0.38f);
        t.set(gx, gy, Color{ 255, 255, 230 });
        t.set(gx + 1, gy, Color{ 240, 255, 200 });
        t.set(gx, gy + 1, Color{ 240, 255, 200 });
    }

    // Sugar cane: three tall reeds, pale green, each drifting sideways
    // as it climbs so a stand of it does not read as a picket fence.
    void paintSugarCane(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const int n = Tile::N;
        const Color stem{ 142, 198, 118 };
        const Color stemDark{ 110, 166, 92 };
        const Color stemPale{ 170, 216, 146 };

        const int roots[3] = { n / 5, n / 2, (n * 4) / 5 };
        const int lean[3] = { -2, 0, 2 };

        for (int r = 0; r < 3; ++r)
        {
            for (int y = 0; y < n; ++y)
            {
                const int x = roots[r] + (lean[r] * (n - 1 - y)) / n;
                if (x < 0 || x >= n - 1) continue;
                t.set(x, y, (r == 1) ? stemPale : stem);
                t.set(x + 1, y, stemDark);
            }
        }
    }

    // A bookshelf's side: planks above and below, with two rows of
    // spines between them in assorted colours and widths so it does not
    // read as striped wallpaper.
    void paintBookshelfSide(Tile& t)
    {
        const int n = Tile::N;
        const Color plank{ 160, 128, 80 };
        const Color plankDark{ 136, 106, 64 };
        const Color shadow{ 60, 44, 28 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, ((x + y) % 7 == 0) ? plankDark : plank);

        const Color spines[5] = {
            { 170, 60, 52 }, { 70, 100, 160 }, { 190, 160, 70 },
            { 80, 140, 80 }, { 150, 90, 150 },
        };

        const int shelfTop[2] = { n / 6, (n * 4) / 7 };
        const int shelfHeight = n / 3;

        for (int shelf = 0; shelf < 2; ++shelf)
        {
            const int top = shelfTop[shelf];
            const int end = (top + shelfHeight < n) ? top + shelfHeight : n;

            for (int y = top; y < end; ++y)
                for (int x = 1; x < n - 1; ++x)
                    t.set(x, y, shadow);

            int x = 1;
            int pick = shelf * 3;
            while (x < n - 1)
            {
                const int width = 1 + ((pick * 7 + shelf) % 2);
                // Every other spine stands a pixel shorter, which is
                // what stops the row looking like a solid band.
                const int spineTop = top + ((pick % 2 == 0) ? 0 : 1);
                for (int y = spineTop; y < end; ++y)
                    for (int k = 0; k < width && x + k < n - 1; ++k)
                        t.set(x + k, y, spines[pick % 5]);
                x += width + 1;
                ++pick;
            }
        }
    }

    void paintItemPaper(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color sheet{ 238, 238, 230 };
        const Color edge{ 198, 198, 190 };

        const int lo = n / 5;
        const int hi = (n * 4) / 5;

        for (int y = lo; y < hi; ++y)
            for (int x = lo; x < hi; ++x)
                t.set(x, y, sheet);

        // A ragged bottom edge and a shaded right one, so it reads as a
        // sheet rather than a white square.
        for (int x = lo; x < hi; ++x)
            t.set(x, hi - 1 - (x % 2), edge);
        for (int y = lo; y < hi; ++y)
            t.set(hi - 1, y, edge);
    }

    void paintItemBook(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color cover{ 150, 60, 52 };
        const Color coverDark{ 112, 42, 38 };
        const Color pages{ 238, 236, 220 };
        const Color band{ 196, 170, 90 };

        const int left = n / 5;
        const int right = (n * 4) / 5;
        const int top = n / 6;
        const int bottom = (n * 5) / 6;

        for (int y = top; y < bottom; ++y)
            for (int x = left; x < right; ++x)
                t.set(x, y, cover);

        // The page block, set in from the spine on the left.
        for (int y = top + 1; y < bottom - 1; ++y)
            for (int x = left + 3; x < right - 1; ++x)
                t.set(x, y, pages);

        for (int y = top; y < bottom; ++y)
        {
            t.set(left, y, coverDark);
            t.set(left + 1, y, cover);
        }
        for (int x = left; x < right; ++x)
            t.set(x, (top + bottom) / 2, band);
    }

    // The enchanting table's top: an open book on dark stone, which is
    // the whole of what you see of it from standing height.
    // The anvil, seen from above: the flat face you work on, with the
    // horn standing proud at one end.
    // Trodden earth: dirt, rubbed pale and smooth on top.
    // A rail: two iron strips on wooden sleepers, running top to bottom
    // of the tile so the mesher can turn it a quarter and get the other
    // straight for nothing.
    void paintRail(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color iron{ 164, 164, 170 };
        const Color ironDark{ 118, 118, 126 };
        const Color sleeper{ 104, 78, 48 };
        const Color sleeperDark{ 82, 60, 36 };

        // Sleepers across, every four sixteenths.
        const int step = std::max(2, n / 4);
        const int thick = std::max(1, n / 16);
        for (int y = step / 2; y < n; y += step)
            for (int j = 0; j < thick * 2; ++j)
                for (int x = n / 8; x < n - n / 8; ++x)
                    t.set(x, y + j, ((x + y) % 7 == 0) ? sleeperDark : sleeper);

        // Two rails down it.
        for (int side = 0; side < 2; ++side)
        {
            const int x0 = side == 0 ? n / 4 : n - n / 4 - thick;
            for (int y = 0; y < n; ++y)
                for (int i = 0; i < thick; ++i)
                    t.set(x0 + i, y, (i == 0) ? iron : ironDark);
        }
    }

    // The same, bent through a right angle: in along the top edge and
    // out along the right one.
    void paintRailCorner(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color iron{ 164, 164, 170 };
        const Color ironDark{ 118, 118, 126 };
        const Color sleeper{ 104, 78, 48 };

        const int thick = std::max(1, n / 16);

        // Two arcs about the far corner, one inside the other.
        for (int side = 0; side < 2; ++side)
        {
            const float radius = (side == 0 ? 0.25f : 0.75f) * n;
            for (int step = 0; step <= n * 2; ++step)
            {
                const float angle = (step / static_cast<float>(n * 2)) * 1.5707963f;
                const int x = static_cast<int>(n - radius * std::cos(angle));
                const int y = static_cast<int>(n - radius * std::sin(angle));
                for (int i = 0; i < thick; ++i)
                    for (int j = 0; j < thick; ++j)
                        t.set(x + i, y + j, (i == 0) ? iron : ironDark);
            }
        }

        // Sleepers fanned between them.
        for (int step = 0; step <= 6; ++step)
        {
            const float angle = (step / 6.0f) * 1.5707963f;
            for (float radius = 0.3f * n; radius < 0.72f * n; radius += 0.5f)
            {
                const int x = static_cast<int>(n - radius * std::cos(angle));
                const int y = static_cast<int>(n - radius * std::sin(angle));
                if (t.get(x, y).a == 0) t.set(x, y, sleeper);
            }
        }
    }

    // A minecart seen from the side and a little above: an open iron
    // tub on two wheels.
    void paintItemMinecart(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color iron{ 142, 146, 154 };
        const Color ironDark{ 98, 102, 110 };
        const Color inside{ 70, 74, 80 };
        const Color wheel{ 60, 60, 66 };

        const int left = n / 8;
        const int right = n - n / 8;
        const int top = n / 3;
        const int floorY = n - n / 4;

        for (int y = top; y < floorY; ++y)
            for (int x = left; x < right; ++x)
            {
                const bool wall = x < left + n / 8 || x >= right - n / 8 || y >= floorY - n / 8;
                const bool lip = y < top + n / 12;
                t.set(x, y, lip ? iron : (wall ? ironDark : inside));
            }

        // Two wheels under it, just proud of the tub.
        for (int side = 0; side < 2; ++side)
        {
            const int cx = side == 0 ? left + n / 5 : right - n / 5;
            const int cy = floorY + n / 12;
            const int r = std::max(1, n / 10);
            for (int dy = -r; dy <= r; ++dy)
                for (int dx = -r; dx <= r; ++dx)
                    if (dx * dx + dy * dy <= r * r) t.set(cx + dx, cy + dy, wheel);
        }
    }

    void paintDirtPathTop(Tile& t)
    {
        const int n = Tile::N;
        const Color worn{ 150, 125, 88 };
        const Color wornDark{ 128, 105, 72 };
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, ((x * 5 + y * 3) % 9 < 4) ? wornDark : worn);
    }

    void paintDirtPathSide(Tile& t)
    {
        paintDirt(t);
        const int n = Tile::N;
        const Color worn{ 150, 125, 88 };
        for (int x = 0; x < n; ++x)
        {
            t.set(x, 0, worn);
            if ((x & 1) == 0) t.set(x, 1, worn);
        }
    }

    void paintNetherWartTile(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color wart{ 150, 30, 44 };
        const Color wartDark{ 96, 18, 28 };

        // A low clump: wider at the bottom, ragged at the top.
        for (int y = n / 2; y < n; ++y)
        {
            const int inset = (y < (n * 5) / 8) ? n / 3 : n / 5;
            for (int x = inset; x < n - inset; ++x)
                t.set(x, y, ((x * 5 + y * 7) % 9 < 4) ? wartDark : wart);
        }
    }

    void paintBrewingStandTile(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color iron{ 128, 128, 134 };
        const Color ironDark{ 80, 80, 86 };
        const Color flame{ 236, 150, 48 };

        // A post with a wide foot and three bottles hanging off it.
        for (int y = n / 5; y < n; ++y)
            t.set(n / 2, y, iron);
        for (int x = n / 4; x < n - n / 4; ++x)
            t.set(x, n - 1, ironDark);
        for (int x = n / 3; x < n - n / 3; ++x)
            t.set(x, n - 2, iron);

        for (int y = (n * 2) / 3; y < n - 2; ++y)
        {
            t.set(n / 4, y, iron);
            t.set(n - 1 - n / 4, y, iron);
        }
        t.set(n / 2, n / 5, flame);
        t.set(n / 2 - 1, n / 5 + 1, flame);
        t.set(n / 2 + 1, n / 5 + 1, flame);
    }

    void paintAnvilTop(Tile& t)
    {
        const int n = Tile::N;
        const Color iron{ 74, 74, 78 };
        const Color ironDark{ 54, 54, 58 };
        const Color ironLight{ 104, 104, 110 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, ((x * 7 + y * 3) % 13 == 0) ? ironDark : iron);

        // The working face, a lighter rectangle inset from the edges.
        const int inset = n / 8;
        for (int y = inset; y < n - inset; ++y)
            for (int x = inset * 2; x < n - inset * 2; ++x)
                t.set(x, y, ironLight);

        // Hammered dents along it.
        for (int x = inset * 2 + 1; x < n - inset * 2; x += 3)
            t.set(x, n / 2, ironDark);
    }

    // Its side: the wide top, the waist pinched in, the foot spread out.
    void paintAnvilSide(Tile& t)
    {
        const int n = Tile::N;
        const Color iron{ 68, 68, 72 };
        const Color ironDark{ 44, 44, 48 };
        const Color ironLight{ 98, 98, 104 };
        const Color gap{ 30, 30, 34 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                // Rows near the middle are the waist, and narrower.
                const bool waist = (y > n / 3) && (y < (n * 2) / 3);
                const int margin = waist ? n / 4 : 0;
                if (x < margin || x >= n - margin) t.set(x, y, gap);
                else t.set(x, y, ((x * 5 + y * 7) % 11 == 0) ? ironDark : iron);
            }

        // A highlight along the top edge, where the light catches it.
        for (int x = 0; x < n; ++x) t.set(x, 0, ironLight);
    }

    void paintEnchantTableTop(Tile& t)
    {
        const int n = Tile::N;
        const Color stone{ 46, 38, 52 };
        const Color stoneDark{ 34, 28, 40 };
        const Color cover{ 150, 40, 40 };
        const Color pages{ 236, 232, 214 };
        const Color pagesDim{ 206, 200, 180 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, ((x * 3 + y * 5) % 11 == 0) ? stoneDark : stone);

        // The book, open, lying across the middle.
        const int top = n / 4;
        const int bottom = (n * 3) / 4;
        const int left = n / 6;
        const int right = (n * 5) / 6;
        const int spine = n / 2;

        for (int y = top; y < bottom; ++y)
            for (int x = left; x < right; ++x)
                t.set(x, y, (x < spine) ? pages : pagesDim);

        // The covers showing round the edges of the leaves.
        for (int x = left; x < right; ++x)
        {
            t.set(x, top, cover);
            t.set(x, bottom - 1, cover);
        }
        for (int y = top; y < bottom; ++y)
        {
            t.set(left, y, cover);
            t.set(right - 1, y, cover);
            t.set(spine, y, cover);
        }
    }

    // Its side: obsidian below, a band of the same dark stone above.
    void paintEnchantTableSide(Tile& t)
    {
        const int n = Tile::N;
        const Color stone{ 46, 38, 52 };
        const Color stoneDark{ 34, 28, 40 };
        const Color rim{ 86, 70, 96 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                t.set(x, y, ((x * 5 + y * 3) % 9 == 0) ? stoneDark : stone);

        // A lit rim along the top edge, so the slab reads as a table
        // with a surface rather than a block cut in half.
        for (int x = 0; x < n; ++x)
        {
            t.set(x, 0, rim);
            t.set(x, 1, rim);
        }
    }

    // Emerald ore: green crystal in grey stone, the same shape of
    // speckle the other ores use so a wall of mixed ore reads as one
    // family of blocks.
    void paintEmeraldOre(Tile& t)
    {
        paintStone(t);

        const int n = Tile::N;
        const Color gem{ 52, 200, 100 };
        const Color gemPale{ 120, 236, 160 };
        const Color gemDark{ 28, 140, 70 };

        // Three lumps, placed by hand rather than scattered: a random
        // sprinkle at this size reads as noise rather than as ore.
        const int spots[3][2] = { { n / 5, n / 4 }, { (n * 3) / 5, n / 2 },
                                  { n / 3, (n * 3) / 4 } };

        for (const auto& spot : spots)
        {
            const int sx = spot[0];
            const int sy = spot[1];
            for (int dy = 0; dy < 3; ++dy)
                for (int dx = 0; dx < 3; ++dx)
                {
                    if (dx == 2 && dy == 2) continue;
                    const int x = sx + dx;
                    const int y = sy + dy;
                    if (x >= n || y >= n) continue;
                    t.set(x, y, (dx == 0 && dy == 0) ? gemPale
                                                     : ((dx + dy >= 3) ? gemDark : gem));
                }
        }
    }

    // A bottle: narrow neck, round body. The potion is the same shape
    // with the body filled in, so the two are painted by one function
    // and differ only in what goes inside.
    void paintBottle(Tile& t, Color inside, bool filled)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color glass{ 190, 206, 214 };
        const Color glassDark{ 128, 146, 156 };
        const Color cork{ 150, 112, 68 };

        const int neckLeft = (n * 7) / 16;
        const int neckRight = (n * 9) / 16;
        const int shoulder = (n * 6) / 16;

        for (int y = n / 8; y < shoulder; ++y)
            for (int x = neckLeft; x < neckRight; ++x)
                t.set(x, y, glass);

        for (int x = neckLeft - 1; x <= neckRight; ++x)
            t.set(x, n / 8, cork);

        const float mid = (n - 1) * 0.5f;
        for (int y = shoulder; y < n - n / 10; ++y)
        {
            const float dy = (y - shoulder) / static_cast<float>(n - shoulder);
            const int half = static_cast<int>((n * 0.16f) + dy * (n * 0.18f));
            for (int x = static_cast<int>(mid) - half; x <= static_cast<int>(mid) + half; ++x)
            {
                if (x < 0 || x >= n) continue;
                const bool edge = (x <= static_cast<int>(mid) - half + 1) ||
                                  (x >= static_cast<int>(mid) + half - 1);
                if (filled && !edge) t.set(x, y, inside);
                else t.set(x, y, edge ? glassDark : glass);
            }
        }
    }

    // A shield: a plank face banded with iron, narrowing to a point.
    void paintItemShield(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color wood{ 142, 106, 64 };
        const Color woodDark{ 110, 80, 46 };
        const Color iron{ 176, 176, 182 };

        const float mid = (n - 1) * 0.5f;
        for (int y = n / 8; y < n - n / 8; ++y)
        {
            // Square at the top, tapering to a point at the bottom.
            const float down = (y - n / 8) / static_cast<float>(n - n / 4);
            const int half = static_cast<int>((n * 0.34f) * (1.0f - down * down * 0.85f));

            for (int x = static_cast<int>(mid) - half; x <= static_cast<int>(mid) + half; ++x)
            {
                if (x < 0 || x >= n) continue;
                t.set(x, y, ((x + y) % 5 == 0) ? woodDark : wood);
            }
        }

        // The boss, and a band across the middle.
        for (int y = n / 2 - 1; y <= n / 2 + 1; ++y)
            for (int x = n / 3; x < n - n / 3; ++x)
                if (t.get(x, y).a > 0) t.set(x, y, iron);
    }

    void paintItemGlassBottle(Tile& t) { paintBottle(t, Color{ 0, 0, 0, 0 }, false); }
    void paintItemPotion(Tile& t) { paintBottle(t, Color{ 220, 60, 80 }, true); }

    // A scatter of grains, for the three powders. Each gets its own
    // colour and the same shape, which is what they look like anyway.
    void paintPowder(Tile& t, Color bright, Color dark)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const float mid = (n - 1) * 0.5f;
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const float dx = (x - mid) / (n * 0.34f);
                const float dy = (y - mid * 1.15f) / (n * 0.26f);
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, ((x * 5 + y * 3) % 7 < 3) ? bright : dark);
            }
    }

    void paintItemSugar(Tile& t) { paintPowder(t, Color{ 246, 246, 250 }, Color{ 202, 206, 214 }); }
    // A boat seen from above and a little to one side: a hull with a
    // bench across it, which is what the item looks like in the hand.
    void paintItemBoat(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color wood{ 142, 106, 64 };
        const Color rim{ 110, 80, 46 };
        const Color inside{ 92, 68, 40 };

        const float mid = (n - 1) * 0.5f;
        const int top = n / 4;
        const int bottom = n - n / 6;

        for (int y = top; y < bottom; ++y)
        {
            // Square across the stern and drawn in to a point at the bow.
            const float along = (y - top) / static_cast<float>(bottom - top);
            const int half = static_cast<int>((n * 0.40f) * (1.0f - along * along * 0.75f));

            for (int x = static_cast<int>(mid) - half; x <= static_cast<int>(mid) + half; ++x)
            {
                if (x < 0 || x >= n) continue;
                const bool edge = x <= static_cast<int>(mid) - half + n / 16 ||
                                  x >= static_cast<int>(mid) + half - n / 16 ||
                                  y < top + n / 16 || y >= bottom - n / 16;
                t.set(x, y, edge ? rim : (along > 0.3f && along < 0.5f ? wood : inside));
            }
        }
    }

    void paintItemBlazePowder(Tile& t) { paintPowder(t, Color{ 252, 196, 70 }, Color{ 204, 110, 26 }); }
    void paintItemGlowstoneDust(Tile& t) { paintPowder(t, Color{ 248, 226, 150 }, Color{ 198, 160, 84 }); }

    // Nether wart: a dark red knot of a thing.
    void paintItemNetherWart(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color wart{ 150, 30, 44 };
        const Color wartDark{ 96, 18, 28 };
        const float mid = (n - 1) * 0.5f;
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const float dx = (x - mid) / (n * 0.33f);
                const float dy = (y - mid) / (n * 0.33f);
                const float d = dx * dx + dy * dy;
                if (d > 1.0f) continue;
                t.set(x, y, ((x * 7 + y * 5) % 9 < 4) ? wartDark : wart);
            }
    }

    void paintItemEmerald(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const Color gem{ 52, 200, 100 };
        const Color gemPale{ 140, 244, 176 };
        const Color gemDark{ 26, 132, 66 };

        // A cut stone: a tall hexagon with a lit top-left facet.
        const float mid = (n - 1) * 0.5f;
        for (int y = 0; y < n; ++y)
        {
            for (int x = 0; x < n; ++x)
            {
                const float dx = (x - mid) / (n * 0.30f);
                const float dy = (y - mid) / (n * 0.42f);
                if (std::fabs(dx) + std::fabs(dy) > 1.0f) continue;

                const bool lit = (dx + dy) < -0.35f;
                const bool shade = (dx + dy) > 0.45f;
                t.set(x, y, lit ? gemPale : (shade ? gemDark : gem));
            }
        }
    }

    // The end portal frame: pale end stone with a dark socket cut in
    // the top of it, empty until an eye is set in it.
    void paintEndFrameTop(Tile& t)
    {
        paintEndStone(t);
        const int n = Tile::N;
        const Color socket{ 48, 44, 60 };
        const Color rim{ 92, 96, 86 };

        for (int y = n / 4; y < (n * 3) / 4; ++y)
            for (int x = n / 4; x < (n * 3) / 4; ++x)
                t.set(x, y, socket);

        for (int i = n / 4; i < (n * 3) / 4; ++i)
        {
            t.set(i, n / 4, rim);
            t.set(n / 4, i, rim);
        }
    }

    // The same with the eye in it: a green pupil that reads as lit.
    void paintEndFrameEye(Tile& t)
    {
        paintEndFrameTop(t);
        const int n = Tile::N;
        const Color eye{ 62, 172, 128 };
        const Color eyePale{ 142, 230, 190 };

        const float mid = (n - 1) * 0.5f;
        const float radius = n * 0.22f;
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const float dx = x - mid;
                const float dy = y - mid;
                if (dx * dx + dy * dy > radius * radius) continue;
                t.set(x, y, (dx + dy < -radius * 0.4f) ? eyePale : eye);
            }
    }

    void paintEndFrameSide(Tile& t)
    {
        paintEndStone(t);
        const int n = Tile::N;
        const Color band{ 108, 112, 100 };
        for (int x = 0; x < n; ++x)
        {
            t.set(x, 0, band);
            t.set(x, 1, band);
        }
    }

    // The portal itself: near black with scattered lights in it, which
    // is as close as a flat tile gets to Minecraft's starfield.
    void paintEndPortalTile(Tile& t)
    {
        const int n = Tile::N;
        t.fill(Color{ 10, 8, 20 });

        for (int y = 0; y < n; ++y)
        {
            for (int x = 0; x < n; ++x)
            {
                // A fixed scatter rather than a random one: the tile is
                // drawn once and seen a thousand times, so it wants to
                // look deliberate.
                const int h = (x * 73 + y * 151 + x * y * 17) % 97;
                if (h < 4)
                    t.set(x, y, Color{ 196, 226, 236 });
                else if (h < 11)
                    t.set(x, y, Color{ 86, 120, 150 });
                else if (h < 22)
                    t.set(x, y, Color{ 30, 34, 62 });
            }
        }
    }

    void paintItemEnderPearl(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const int n = Tile::N;
        const float mid = (n - 1) * 0.5f;
        const float radius = n * 0.36f;

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const float dx = (x - mid) / radius;
                const float dy = (y - mid) / radius;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > 1.0f) continue;

                const float lit = std::clamp(1.0f - (dx * 0.5f + dy * 0.5f) - d * 0.3f,
                                             0.0f, 1.0f);
                Color c;
                c.r = static_cast<unsigned char>(18 + 70 * lit);
                c.g = static_cast<unsigned char>(70 + 130 * lit);
                c.b = static_cast<unsigned char>(72 + 110 * lit);
                c.a = 255;
                t.set(x, y, c);
            }
    }

    void paintItemEyeOfEnder(Tile& t)
    {
        paintItemEnderPearl(t);
        const int n = Tile::N;
        const float mid = (n - 1) * 0.5f;
        const float radius = n * 0.17f;
        const Color pupil{ 220, 240, 200 };
        const Color ring{ 120, 200, 150 };

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const float dx = x - mid;
                const float dy = y - mid;
                const float d2 = dx * dx + dy * dy;
                if (d2 > radius * radius * 2.4f) continue;
                t.set(x, y, (d2 <= radius * radius) ? pupil : ring);
            }
    }

    // A raindrop, drawn down the whole height of its tile: the quad it
    // goes on is long and thin, and a drop shaped like a drop would be
    // stretched into a smear on it.
    void paintRainDrop(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const int n = Tile::N;
        const int mid = n / 2;

        for (int y = 0; y < n; ++y)
        {
            // Brightest in the middle of its fall and thinner at both
            // ends, so a column of them does not read as a row of bars.
            const float along = static_cast<float>(y) / static_cast<float>(n - 1);
            const float taper = 1.0f - std::fabs(along - 0.5f) * 1.6f;
            if (taper <= 0.0f) continue;

            const unsigned char alpha = static_cast<unsigned char>(90 + 150 * taper);
            Color c{ 190, 214, 240, alpha };
            t.set(mid, y, c);
            t.set(mid - 1, y, Color{ 160, 186, 220, static_cast<unsigned char>(alpha * 2 / 3) });
        }
    }

    void paintSnowFlake(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const int n = Tile::N;
        const float mid = (n - 1) * 0.5f;
        const float radius = n * 0.3f;

        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const float dx = x - mid;
                const float dy = y - mid;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > radius) continue;

                const float edge = 1.0f - d / radius;
                t.set(x, y, Color{ 248, 250, 255,
                                   static_cast<unsigned char>(120 + 135 * edge) });
            }
    }

    void paintItemBlazeRod(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color core{ 252, 206, 48 };
        const Color edge{ 196, 126, 16 };
        for (int i = 3; i <= 12; ++i)
        {
            t.set(i, i, core);
            t.set(i + 1, i, core);
            t.set(i + 2, i, edge);
        }
        for (int i = 3; i <= 12; i += 3)
            t.set(i + 1, i, Color{ 255, 248, 176 });
    }

    void paintItemBreezeRod(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int i = 3; i <= 12; ++i)
        {
            t.set(i, i, Color{ 206, 230, 238 });
            t.set(i + 1, i, Color{ 142, 176, 192 });
            t.set(i + 2, i, Color{ 96, 126, 146 });
        }
    }

    void paintItemWindCharge(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const float mid = Tile::N / 2.0f - 0.5f;
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - mid, dy = y - mid;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > 5.6f || d < 2.2f) continue;
                t.set(x, y, mixColor(Color{ 240, 250, 255 }, Color{ 128, 178, 200 },
                                     std::min(1.0f, (d - 2.2f) / 3.4f)));
            }
    }

    void paintItemCopperIngot(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color body{ 196, 114, 78 };
        for (int y = 6; y <= 10; ++y)
            for (int x = 3 + (y - 6) / 2; x <= 12 - (y - 6) / 2; ++x)
                t.set(x, y, shade(body, y == 6 ? 34 : (y == 10 ? -30 : 0)));
    }

    void paintItemGlowBerries(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int i = 0; i < 7; ++i)
        {
            const int cx = 4 + static_cast<int>(hash2(i, 0, 338) * 8.0f);
            const int cy = 4 + static_cast<int>(hash2(i, 1, 338) * 8.0f);
            for (int dy = 0; dy <= 1; ++dy)
                for (int dx = 0; dx <= 1; ++dx)
                    t.set(cx + dx, cy + dy, Color{ 255, 190, 80 });
        }
    }

    void paintItemOminousBottle(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A bottle, the same shape as the potions, with something
        // darker in it.
        for (int y = 5; y <= 13; ++y)
        {
            const int half = (y < 7) ? 2 : 4;
            for (int x = 8 - half; x <= 7 + half; ++x)
                t.set(x, y, mixColor(Color{ 48, 40, 78 }, Color{ 110, 96, 168 },
                                     (y - 5) / 8.0f));
        }
        for (int y = 2; y <= 4; ++y)
            for (int x = 6; x <= 9; ++x)
                t.set(x, y, Color{ 196, 190, 214 });
    }

    void paintItemApple(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 3; y < 15; ++y)
            for (int x = 3; x < 13; ++x)
            {
                const float dx = (x - 7.5f) / 5.0f;
                const float dy = (y - 9.5f) / 5.5f;
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, (dx < -0.3f && dy < -0.2f) ? Color{ 236, 108, 96 }
                                                       : Color{ 194, 48, 44 });
            }

        // The stalk and one leaf, which is what makes it an apple
        // rather than a red circle.
        t.set(8, 2, Color{ 96, 64, 38 });
        t.set(8, 3, Color{ 96, 64, 38 });
        t.set(9, 3, Color{ 72, 140, 56 });
        t.set(10, 3, Color{ 72, 140, 56 });
    }

    void paintItemBoneMeal(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        // A heap of powder rather than a bone: it is what a bone
        // becomes, and the two have to be told apart in a hotbar.
        for (int y = 6; y < 14; ++y)
        {
            const int wide = 2 + (y - 6);
            for (int x = 8 - wide; x <= 7 + wide && x < Tile::N; ++x)
                if (x >= 0)
                    t.set(x, y, (y < 8) ? Color{ 248, 248, 240 }
                                        : Color{ 222, 222, 210 });
        }
        t.speckle(6, 521);
    }

    // The star a wither leaves. A four-pointed white burst, which is
    // the one item in the game that is not a thing but a trophy.
    // A carrot: the root laid along the tile's diagonal with its
    // leaves at the top. Both crops are the seed and the food at
    // once, so these two icons are what you plant as well as eat.
    void paintItemCarrot(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int i = 0; i < 11; ++i)
        {
            const int x = 3 + i;
            const int y = 13 - i;
            const int wide = (10 - i) / 3;
            for (int o = -wide; o <= wide; ++o)
            {
                const int px = x + o;
                const int py = y + o;
                if (px < 0 || px >= Tile::N || py < 0 || py >= Tile::N) continue;
                t.set(px, py, (o < 0) ? Color{ 246, 160, 60 }
                                      : Color{ 198, 106, 24 });
            }
        }
        for (int i = 0; i < 4; ++i)
        {
            t.set(12 - i, 3 + i / 2, Color{ 72, 148, 56 });
            t.set(13 - i / 2, 2 + i, Color{ 96, 176, 72 });
        }
    }

    // The common body of both potatoes: a lumpy oval with eyes in it.
    void paintPotatoBody(Tile& t, Color light, Color dark, Color eye)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 3; y < 14; ++y)
            for (int x = 2; x < 14; ++x)
            {
                const float dx = (x - 7.5f) / 5.6f;
                const float dy = (y - 8.5f) / 5.2f;
                if (dx * dx + dy * dy > 1.0f) continue;
                t.set(x, y, (dx + dy < -0.4f) ? light : dark);
            }
        t.set(5, 7, eye);
        t.set(9, 6, eye);
        t.set(7, 11, eye);
        t.set(11, 9, eye);
    }

    void paintItemPotato(Tile& t)
    {
        paintPotatoBody(t, Color{ 214, 174, 112 }, Color{ 174, 134, 76 },
                        Color{ 126, 94, 50 });
        t.speckle(5, 733);
    }

    // Out of the furnace: darker, with the skin split along its back.
    void paintItemBakedPotato(Tile& t)
    {
        paintPotatoBody(t, Color{ 198, 140, 70 }, Color{ 150, 96, 40 },
                        Color{ 104, 62, 26 });
        for (int i = 0; i < 6; ++i)
            t.set(5 + i, 9 - i / 2, Color{ 246, 218, 150 });
        t.speckle(5, 734);
    }

    void paintItemNetherStar(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = std::abs(x - 7.5f);
                const float dy = std::abs(y - 7.5f);
                const float arm = std::min(dx, dy);
                const float reach = std::max(dx, dy);
                if (arm * 2.4f + reach > 9.0f) continue;

                t.set(x, y, (arm < 1.2f && reach > 4.0f) ? Color{ 214, 220, 236 }
                          : (reach < 2.6f)               ? Color{ 255, 255, 255 }
                                                         : Color{ 236, 240, 250 });
            }
    }

    void paintItemOminousKey(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color cold{ 158, 206, 240 };
        const Color dark{ 92, 130, 176 };
        for (int y = 2; y <= 6; ++y)
            for (int x = 5; x <= 9; ++x)
            {
                const int d = (x - 7) * (x - 7) + (y - 4) * (y - 4);
                if (d <= 4 && d >= 1) t.set(x, y, cold);
            }
        for (int y = 7; y <= 13; ++y)
        {
            t.set(7, y, cold);
            t.set(8, y, dark);
        }
        t.set(9, 11, cold);
        t.set(9, 13, cold);
    }

    void paintItemTrialKey(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color gold{ 236, 194, 88 };
        const Color dark{ 168, 128, 44 };

        // A ring at the top and a shaft with two teeth, which is what
        // a key is from across a room.
        for (int y = 2; y <= 6; ++y)
            for (int x = 5; x <= 9; ++x)
            {
                const int d = (x - 7) * (x - 7) + (y - 4) * (y - 4);
                if (d <= 4 && d >= 1) t.set(x, y, gold);
            }
        for (int y = 7; y <= 13; ++y)
        {
            t.set(7, y, gold);
            t.set(8, y, dark);
        }
        t.set(9, 11, gold);
        t.set(9, 13, gold);
    }

    void paintItemMace(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        // A haft up the diagonal with a heavy head on the end of it.
        for (int i = 2; i <= 9; ++i)
        {
            t.set(i, 15 - i, Color{ 108, 80, 46 });
            t.set(i + 1, 15 - i, Color{ 78, 56, 32 });
        }
        for (int y = 2; y <= 6; ++y)
            for (int x = 9; x <= 13; ++x)
            {
                const int d = (x - 11) * (x - 11) + (y - 4) * (y - 4);
                if (d > 6) continue;
                t.set(x, y, shade(Color{ 92, 88, 96 }, d <= 1 ? 40 : 0));
            }
    }

    void paintItemRedstone(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int i = 0; i < 24; ++i)
        {
            const int x = 3 + static_cast<int>(hash2(i, 0, 261) * 10.0f);
            const int y = 3 + static_cast<int>(hash2(i, 1, 261) * 10.0f);
            t.set(x, y, shade(Color{ 196, 30, 20 },
                              static_cast<int>((hash2(i, 2, 261) - 0.5f) * 70.0f)));
        }
    }

    void paintItemNetherBrick(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        for (int y = 6; y <= 10; ++y)
            for (int x = 3; x <= 12; ++x)
            {
                const int delta = static_cast<int>((hash2(x, y, 233) - 0.5f) * 22.0f);
                t.set(x, y, shade(Color{ 60, 30, 36 }, delta));
            }
        for (int x = 3; x <= 12; ++x) t.set(x, 6, shade(Color{ 60, 30, 36 }, 30));
    }

    void paintItemMagmaCream(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const float radius = 5.0f;
        for (int y = 0; y < Tile::N; ++y)
            for (int x = 0; x < Tile::N; ++x)
            {
                const float dx = x - 7.5f, dy = y - 8.0f;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d > radius) continue;
                t.set(x, y, mixColor(Color{ 255, 170, 40 }, Color{ 74, 36, 22 },
                                     std::min(1.0f, d / radius)));
            }
    }

    void paintItemStick(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color wood{ 150, 112, 66 };
        const Color dark{ 110, 80, 44 };
        for (int i = 3; i <= 12; ++i)
        {
            t.set(i, i, wood);
            t.set(i + 1, i, dark);
        }
    }

    // A lump, in whatever the material is. Coal is nearly black, a
    // diamond is bright, the ingots are bars rather than lumps.
    void paintLump(Tile& t, Color body)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color edge = shade(body, -50);
        const Color lit = shade(body, 46);

        for (int y = 4; y <= 11; ++y)
        {
            const int inset = (y <= 4 || y >= 11) ? 3 : (y == 5 || y == 10) ? 2 : 1;
            for (int x = 3 + inset; x <= 12 - inset; ++x)
                t.set(x, y, (y <= 6) ? lit : body);
        }
        for (int x = 6; x <= 9; ++x) { t.set(x, 3, edge); t.set(x, 12, edge); }
        for (int y = 5; y <= 10; ++y) { t.set(3, y, edge); t.set(12, y, edge); }
    }

    void paintItemRawCopper(Tile& t) { paintLump(t, Color{ 186, 112, 76 }); }

    void paintItemCoal(Tile& t)    { paintLump(t, Color{ 44, 44, 48 }); }
    void paintItemDiamond(Tile& t) { paintLump(t, Color{ 94, 219, 208 }); }

    void paintIngot(Tile& t, Color body)
    {
        t.fill(Color{ 0, 0, 0, 0 });
        const Color edge = shade(body, -50);
        const Color lit = shade(body, 40);

        for (int y = 6; y <= 10; ++y)
        {
            const int inset = (y == 6 || y == 10) ? 1 : 0;
            for (int x = 3 + inset; x <= 12 - inset; ++x)
                t.set(x, y, y <= 7 ? lit : body);
        }
        for (int x = 4; x <= 11; ++x) { t.set(x, 5, edge); t.set(x, 11, edge); }
        for (int y = 6; y <= 10; ++y) { t.set(2, y, edge); t.set(13, y, edge); }
    }

    void paintItemIronIngot(Tile& t) { paintIngot(t, Color{ 216, 216, 216 }); }
    void paintItemGoldIngot(Tile& t) { paintIngot(t, Color{ 246, 208, 70 }); }

    void paintItemBread(Tile& t)
    {
        t.fill(Color{ 0, 0, 0, 0 });

        const Color crust{ 158, 98, 44 };
        const Color edge{ 112, 66, 28 };
        const Color top{ 206, 148, 80 };
        const Color score{ 128, 76, 34 };

        for (int y = 5; y <= 11; ++y)
        {
            const int inset = (y <= 5 || y >= 11) ? 3 : (y == 6 || y == 10) ? 2 : 1;
            for (int x = 1 + inset; x <= 14 - inset; ++x)
                t.set(x, y, (y <= 7) ? top : crust);
        }

        for (int x = 3; x <= 13; ++x)
        {
            t.set(x, 4, edge);
            t.set(x, 12, edge);
        }
        for (int y = 5; y <= 11; ++y)
        {
            t.set(2, y, edge);
            t.set(14, y, edge);
        }

        for (int x = 5; x <= 11; x += 3)
            for (int y = 5; y <= 7; ++y) t.set(x + (y - 6), y, score);

        t.speckle(5, 470);
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
        { Tiles::SpruceLogTop,     paintSpruceLogTop },
        { Tiles::SpruceLogSide,    paintSpruceLogSide },
        { Tiles::SpruceLeaves,     paintSpruceLeaves },
        { Tiles::SprucePlanks,     paintSprucePlanks },
        { Tiles::DarkOakLogTop,    paintDarkOakLogTop },
        { Tiles::DarkOakLogSide,   paintDarkOakLogSide },
        { Tiles::DarkOakLeaves,    paintDarkOakLeaves },
        { Tiles::DarkOakPlanks,    paintDarkOakPlanks },
        { Tiles::JungleLogTop,     paintJungleLogTop },
        { Tiles::JungleLogSide,    paintJungleLogSide },
        { Tiles::JungleLeaves,     paintJungleLeaves },
        { Tiles::JunglePlanks,     paintJunglePlanks },
        { Tiles::AcaciaLogTop,     paintAcaciaLogTop },
        { Tiles::AcaciaLogSide,    paintAcaciaLogSide },
        { Tiles::AcaciaLeaves,     paintAcaciaLeaves },
        { Tiles::AcaciaPlanks,     paintAcaciaPlanks },
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
        { Tiles::NetherBricks, paintNetherBricks },
        { Tiles::RedstoneOre, paintRedstoneOre },
        { Tiles::RedstoneDust, paintRedstoneDust },
        { Tiles::RedstoneDustLit, paintRedstoneDustLit },
        { Tiles::RedstoneTorch, paintRedstoneTorch },
        { Tiles::RedstoneTorchOff, paintRedstoneTorchOff },
        { Tiles::LeverTile, paintLever },
        { Tiles::LeverOnTile, paintLeverOn },
        { Tiles::ButtonTile, paintButton },
        { Tiles::ButtonOnTile, paintButtonOn },
        { Tiles::RedstoneLamp, paintRedstoneLamp },
        { Tiles::RedstoneLampLit, paintRedstoneLampLit },
        { Tiles::RedstoneBlockTile, paintRedstoneBlock },
        { Tiles::TuffBricks, paintTuffBricks },
        { Tiles::TrialSpawnerTop, paintTrialSpawnerTop },
        { Tiles::TrialSpawnerTopLit, paintTrialSpawnerTopLit },
        { Tiles::TrialSpawnerSide, paintTrialSpawnerSide },
        { Tiles::TrialSpawnerSideLit, paintTrialSpawnerSideLit },
        { Tiles::VaultTop, paintVaultTop },
        { Tiles::VaultSide, paintVaultSide },
        { Tiles::VaultSideOpen, paintVaultSideOpen },
        { Tiles::PistonTop, paintPistonTop },
        { Tiles::PistonSide, paintPistonSide },
        { Tiles::PistonFace, paintPistonFace },
        { Tiles::StickyPistonFace, paintStickyPistonFace },
        { Tiles::PistonBack, paintPistonBack },
        { Tiles::PistonArm, paintPistonArm },
        { Tiles::CrafterTop, paintCrafterTop },
        { Tiles::CrafterTopLit, paintCrafterTopLit },
        { Tiles::CrafterSide, paintCrafterSide },
        { Tiles::CopperOre, paintCopperOre },
        { Tiles::CopperBlock, paintCopperBlock },
        { Tiles::ExposedCopper, paintExposedCopper },
        { Tiles::WeatheredCopper, paintWeatheredCopper },
        { Tiles::OxidizedCopper, paintOxidizedCopper },
        { Tiles::CopperBulbTile, paintCopperBulb },
        { Tiles::CopperBulbLitTile, paintCopperBulbLit },
        { Tiles::DripstoneBlock, paintDripstoneBlock },
        { Tiles::DripstoneSpike, paintDripstoneSpike },
        { Tiles::MossTile, paintMoss },
        { Tiles::AzaleaLeavesTile, paintAzaleaLeaves },
        { Tiles::GlowBerriesTile, paintGlowBerries },
        { Tiles::OminousVaultSide, paintOminousVaultSide },
        { Tiles::OminousVaultSideOpen, paintOminousVaultSideOpen },
        { Tiles::SpawnerTile, paintSpawner },
        { Tiles::PrismarineTile, paintPrismarine },
        { Tiles::PrismarineBricksTile, paintPrismarineBricks },
        { Tiles::DarkPrismarineTile, paintDarkPrismarine },
        { Tiles::SeaLanternTile, paintSeaLantern },
        { Tiles::SpongeTile, paintSponge },
        { Tiles::WetSpongeTile, paintWetSponge },
        { Tiles::KelpTile, paintKelp },
        { Tiles::KelpTopTile, paintKelpTop },
        { Tiles::SeagrassTile, paintSeagrass },
        { Tiles::SeaPickleTile, paintSeaPickle },
        { Tiles::TubeCoralBlockTile, paintTubeCoralBlock },
        { Tiles::BrainCoralBlockTile, paintBrainCoralBlock },
        { Tiles::BubbleCoralBlockTile, paintBubbleCoralBlock },
        { Tiles::FireCoralBlockTile, paintFireCoralBlock },
        { Tiles::HornCoralBlockTile, paintHornCoralBlock },
        { Tiles::TubeCoralTile, paintTubeCoral },
        { Tiles::BrainCoralTile, paintBrainCoral },
        { Tiles::BubbleCoralTile, paintBubbleCoral },
        { Tiles::FireCoralTile, paintFireCoral },
        { Tiles::HornCoralTile, paintHornCoral },
        { Tiles::DeepslateBricksTile, paintDeepslateBricks },
        { Tiles::DeepslateTilesTile, paintDeepslateTiles },
        { Tiles::SculkTile, paintSculk },
        { Tiles::SculkSensorTop, paintSculkSensorTop },
        { Tiles::SculkSensorTopLit, paintSculkSensorTopLit },
        { Tiles::SculkSensorSide, paintSculkSensorSide },
        { Tiles::SculkShriekerTop, paintSculkShriekerTop },
        { Tiles::SculkShriekerSide, paintSculkShriekerSide },
        { Tiles::SculkCatalystTop, paintSculkCatalystTop },
        { Tiles::SculkCatalystSide, paintSculkCatalystSide },
        { Tiles::SoulLanternTile, paintSoulLantern },
        { Tiles::ReinforcedDeepslateTop, paintReinforcedDeepslateTop },
        { Tiles::ReinforcedDeepslateSide, paintReinforcedDeepslateSide },
        { Tiles::PurpurTile, paintPurpur },
        { Tiles::PurpurPillarTop, paintPurpurPillarTop },
        { Tiles::PurpurPillarSide, paintPurpurPillarSide },
        { Tiles::EndBricksTile, paintEndBricks },
        { Tiles::ChorusPlantTile, paintChorusPlant },
        { Tiles::ChorusFlowerTile, paintChorusFlower },
        { Tiles::BlackstoneTile, paintBlackstone },
        { Tiles::PolishedBlackstoneTile, paintPolishedBlackstone },
        { Tiles::GildedBlackstoneTile, paintGildedBlackstone },
        { Tiles::BasaltTop, paintBasaltTop },
        { Tiles::BasaltSide, paintBasaltSide },
        { Tiles::LilyPadTile, paintLilyPad },
        { Tiles::VineTile, paintVine },
        { Tiles::MudTile, paintMud },
        { Tiles::EndCrystalTile, paintEndCrystal },
        { Tiles::WitherSkullTile, paintWitherSkull },
        { Tiles::BeaconTile, paintBeacon },
        { Tiles::IronBlockTile, paintIronBlock },
        { Tiles::GoldBlockTile, paintGoldBlock },
        { Tiles::DiamondBlockTile, paintDiamondBlock },
        { Tiles::EmeraldBlockTile, paintEmeraldBlock },
        { Tiles::CoalBlockTile, paintCoalBlock },
        { Tiles::NoteBlockTile, paintNoteBlock },
        { Tiles::ObserverTop, paintObserverTop },
        { Tiles::ObserverTopLit, paintObserverTopLit },
        { Tiles::ObserverSide, paintObserverSide },
        { Tiles::HopperTop, paintHopperTop },
        { Tiles::HopperSide, paintHopperSide },
        { Tiles::DropperFront, paintDropperFront },
        { Tiles::DispenserFront, paintDispenserFront },
        { Tiles::AmethystTile, paintAmethyst },
        { Tiles::BuddingTile, paintBudding },
        { Tiles::ClusterTile, paintCluster },
        { Tiles::SmoothBasaltTile, paintSmoothBasalt },
        { Tiles::LanternTile, paintLantern },
        { Tiles::BarrelTop, paintBarrelTop },
        { Tiles::BarrelSide, paintBarrelSide },
        { Tiles::CampfireTop, paintCampfireTop },
        { Tiles::SmokerFront, paintSmokerFront },
        { Tiles::SmokerFrontLit, paintSmokerFrontLit },
        { Tiles::SmokerTop, paintSmokerTop },
        { Tiles::SmokerSide, paintSmokerSide },
        { Tiles::BlastFurnaceFront, paintBlastFurnaceFront },
        { Tiles::BlastFurnaceFrontLit, paintBlastFurnaceFrontLit },
        { Tiles::BlastFurnaceTop, paintBlastFurnaceTop },
        { Tiles::BlastFurnaceSide, paintBlastFurnaceSide },
        { Tiles::ComposterSide, paintComposterSide },
        { Tiles::ComposterBottom, paintComposterBottom },
        { Tiles::ComposterTop, paintComposterTop },
        { Tiles::ComposterFill1, paintComposterFill1 },
        { Tiles::ComposterFill2, paintComposterFill2 },
        { Tiles::ComposterFill3, paintComposterFill3 },
        { Tiles::ComposterReady, paintComposterReady },
        { Tiles::GrindstoneSide, paintGrindstoneSide },
        { Tiles::GrindstoneFace, paintGrindstoneFace },
        { Tiles::GrindstonePivot, paintGrindstonePivot },
        { Tiles::LecternTop, paintLecternTop },
        { Tiles::LecternTopBook, paintLecternTopBook },
        { Tiles::LecternSide, paintLecternSide },
        { Tiles::LecternFront, paintLecternFront },
        { Tiles::LecternBase, paintLecternBase },
        { Tiles::CauldronTop, paintCauldronTop },
        { Tiles::CauldronSide, paintCauldronSide },
        { Tiles::CauldronBottom, paintCauldronBottom },
        { Tiles::CauldronWater1, paintCauldronWater1 },
        { Tiles::CauldronWater2, paintCauldronWater2 },
        { Tiles::CauldronWater3, paintCauldronWater3 },
        { Tiles::BeetrootStage0, paintBeetroot0 },
        { Tiles::BeetrootStage1, paintBeetroot1 },
        { Tiles::BeetrootStage2, paintBeetroot2 },
        { Tiles::BeetrootStage3, paintBeetroot3 },
        { Tiles::BeetrootStage4, paintBeetroot4 },
        { Tiles::BeetrootStage5, paintBeetroot5 },
        { Tiles::BeetrootStage6, paintBeetroot6 },
        { Tiles::BeetrootStage7, paintBeetroot7 },
        { Tiles::MelonSide, paintMelonSide },
        { Tiles::MelonTop, paintMelonTop },
        { Tiles::MelonStemYoung, paintMelonStemYoung },
        { Tiles::MelonStemRipe, paintMelonStemRipe },
        { Tiles::BambooTile, paintBamboo },
        { Tiles::CocoaStage0, paintCocoa0 },
        { Tiles::CocoaStage1, paintCocoa1 },
        { Tiles::CocoaStage2, paintCocoa2 },
        { Tiles::CakeTop, paintCakeTop },
        { Tiles::CakeSide, paintCakeSide },
        { Tiles::CakeBottom, paintCakeBottom },
        { Tiles::CakeInner, paintCakeInner },
        { Tiles::CandleTile, paintCandle },
        { Tiles::CandleLitTile, paintCandleLit },
        { Tiles::ScaffoldTop, paintScaffoldTop },
        { Tiles::ScaffoldSide, paintScaffoldSide },
        { Tiles::ScaffoldBottom, paintScaffoldBottom },
        { Tiles::ItemEgg, paintItemEgg },
        { Tiles::ItemMilkBucket, paintItemMilkBucket },
        { Tiles::ItemCake, paintItemCake },
        { Tiles::ItemCookie, paintItemCookie },
        { Tiles::ItemCocoaBeans, paintItemCocoaBeans },
        { Tiles::SignTile, paintSignBoard },
        { Tiles::ItemSign, paintItemSign },
        { Tiles::ItemFrameTile, paintItemFrameBorder },
        { Tiles::ItemItemFrame, paintItemFrameBorder },
        { Tiles::ItemElytra, paintItemElytra },
        { Tiles::ShulkerBoxTile, paintShulkerBox },
        { Tiles::ItemShulkerShell, paintItemShulkerShell },
        { Tiles::ItemTrident, paintItemTrident },
        { Tiles::ItemBowl, paintItemBowl },
        { Tiles::ItemBeetroot, paintItemBeetroot },
        { Tiles::ItemBeetrootSeeds, paintItemBeetrootSeeds },
        { Tiles::ItemBeetrootSoup, paintItemBeetrootSoup },
        { Tiles::ItemMelonSlice, paintItemMelonSlice },
        { Tiles::ItemMelonSeeds, paintItemMelonSeeds },
        { Tiles::CampfireLog, paintCampfireLog },
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
        { Tiles::CarrotStage0, paintCarrot0 },
        { Tiles::CarrotStage1, paintCarrot1 },
        { Tiles::CarrotStage2, paintCarrot2 },
        { Tiles::CarrotStage3, paintCarrot3 },
        { Tiles::CarrotStage4, paintCarrot4 },
        { Tiles::CarrotStage5, paintCarrot5 },
        { Tiles::CarrotStage6, paintCarrot6 },
        { Tiles::CarrotStage7, paintCarrot7 },
        { Tiles::PotatoStage0, paintPotato0 },
        { Tiles::PotatoStage1, paintPotato1 },
        { Tiles::PotatoStage2, paintPotato2 },
        { Tiles::PotatoStage3, paintPotato3 },
        { Tiles::PotatoStage4, paintPotato4 },
        { Tiles::PotatoStage5, paintPotato5 },
        { Tiles::PotatoStage6, paintPotato6 },
        { Tiles::PotatoStage7, paintPotato7 },
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
        { Tiles::CraftingTop,   paintCraftingTop },
        { Tiles::FurnaceTop,    paintFurnaceTop },
        { Tiles::BedTop,        paintBedTop },
        { Tiles::LadderTile,    paintLadderTile },
        { Tiles::DoorLower,     paintDoorLower },
        { Tiles::DoorUpper,     paintDoorUpper },
        { Tiles::TntTop,        paintTntTop },
        { Tiles::TntSide,       paintTntSide },
        { Tiles::TntBottom,     paintTntBottom },
        { Tiles::TntPrimedSide, paintTntPrimedSide },
        { Tiles::BedSide,       paintBedSide },
        { Tiles::ChestTop,      paintChestTop },
        { Tiles::ChestFront,    paintChestFront },
        { Tiles::ChestSide,     paintChestSide },
        { Tiles::FurnaceFront,  paintFurnaceFront },
        { Tiles::FurnaceLit,    paintFurnaceLit },
        { Tiles::ItemSteak,         paintItemSteak },
        { Tiles::ItemCookedPork,    paintItemCookedPork },
        { Tiles::ItemCookedChicken, paintItemCookedChicken },
        { Tiles::ItemCookedMutton,  paintItemCookedMutton },
        { Tiles::CraftingSide,  paintCraftingSide },
        { Tiles::ItemStick,     paintItemStick },
        { Tiles::ItemCoal,      paintItemCoal },
        { Tiles::ItemFlint,     paintItemFlint },
        { Tiles::ItemFlintAndSteel, paintItemFlintAndSteel },
        { Tiles::ItemBucket,      paintItemBucket },
        { Tiles::ItemWaterBucket, paintItemWaterBucket },
        { Tiles::ItemLavaBucket,  paintItemLavaBucket },
        { Tiles::ItemBow,       paintItemBow },
        { Tiles::ItemArrow,     paintItemArrow },
        { Tiles::ArrowInFlight, paintArrowInFlight },
        { Tiles::FireCharge, paintFireCharge },
        { Tiles::WindCharge, paintWindCharge },
        { Tiles::ExperienceOrb, paintExperienceOrb },
        { Tiles::SugarCane,     paintSugarCane },
        { Tiles::BookshelfSide, paintBookshelfSide },
        { Tiles::AnvilTop,         paintAnvilTop },
        { Tiles::Granite,          paintGranite },
        { Tiles::Diorite,          paintDiorite },
        { Tiles::Andesite,         paintAndesite },
        { Tiles::Deepslate,        paintDeepslate },
        { Tiles::DeepslateTop,     paintDeepslateTop },
        { Tiles::Tuff,             paintTuff },
        { Tiles::Calcite,          paintCalcite },
        { Tiles::RailTile,         paintRail },
        { Tiles::RailCorner,       paintRailCorner },
        { Tiles::DirtPathTop,      paintDirtPathTop },
        { Tiles::DirtPathSide,     paintDirtPathSide },
        { Tiles::NetherWartTile,   paintNetherWartTile },
        { Tiles::BrewingStandTile, paintBrewingStandTile },
        { Tiles::AnvilSide,        paintAnvilSide },
        { Tiles::EnchantTableTop,  paintEnchantTableTop },
        { Tiles::EnchantTableSide, paintEnchantTableSide },
        { Tiles::EmeraldOre,    paintEmeraldOre },
        { Tiles::ItemEmerald,   paintItemEmerald },
        { Tiles::ItemShield,        paintItemShield },
        { Tiles::ItemGlassBottle,   paintItemGlassBottle },
        { Tiles::ItemPotion,        paintItemPotion },
        { Tiles::ItemNetherWart,    paintItemNetherWart },
        { Tiles::ItemSugar,         paintItemSugar },
        { Tiles::ItemBlazePowder,   paintItemBlazePowder },
        { Tiles::ItemBoat,          paintItemBoat },
        { Tiles::ItemMinecart,      paintItemMinecart },
        { Tiles::ItemBlazeRod,      paintItemBlazeRod },
        { Tiles::ItemNetherBrick,   paintItemNetherBrick },
        { Tiles::ItemRedstone,      paintItemRedstone },
        { Tiles::ItemBreezeRod,     paintItemBreezeRod },
        { Tiles::ItemWindCharge,    paintItemWindCharge },
        { Tiles::ItemMace,          paintItemMace },
        { Tiles::ItemTrialKey,      paintItemTrialKey },
        { Tiles::ItemRawCopper,     paintItemRawCopper },
        { Tiles::ItemCopperIngot,   paintItemCopperIngot },
        { Tiles::ItemGlowBerries,   paintItemGlowBerries },
        { Tiles::ItemOminousBottle, paintItemOminousBottle },
        { Tiles::ItemOminousKey,    paintItemOminousKey },
        { Tiles::ItemNetherStar,    paintItemNetherStar },
        { Tiles::ItemApple,         paintItemApple },
        { Tiles::ItemBoneMeal,      paintItemBoneMeal },
        { Tiles::ItemCarrot,        paintItemCarrot },
        { Tiles::ItemPotato,        paintItemPotato },
        { Tiles::ItemBakedPotato,   paintItemBakedPotato },
        { Tiles::ItemMagmaCream,    paintItemMagmaCream },
        { Tiles::ItemGlowstoneDust, paintItemGlowstoneDust },
        { Tiles::EndFrameTop,   paintEndFrameTop },
        { Tiles::EndFrameEye,   paintEndFrameEye },
        { Tiles::EndFrameSide,  paintEndFrameSide },
        { Tiles::EndPortalTile, paintEndPortalTile },
        { Tiles::RainDrop,      paintRainDrop },
        { Tiles::SnowFlake,     paintSnowFlake },
        { Tiles::ItemEnderPearl, paintItemEnderPearl },
        { Tiles::ItemEyeOfEnder, paintItemEyeOfEnder },
        { Tiles::ItemPaper,     paintItemPaper },
        { Tiles::ItemBook,      paintItemBook },
        { Tiles::ItemDiamond,   paintItemDiamond },
        { Tiles::ItemIronIngot, paintItemIronIngot },
        { Tiles::ItemGoldIngot, paintItemGoldIngot },
        { Tiles::ItemLeather, paintItemLeather },
        { Tiles::ItemRawBeef, paintItemRawBeef },
        { Tiles::ItemRawPork, paintItemRawPork },
        { Tiles::ItemRawChicken, paintItemRawChicken },
        { Tiles::ItemRawMutton, paintItemRawMutton },
        { Tiles::ItemFeather, paintItemFeather },
        { Tiles::ItemBone, paintItemBone },
        { Tiles::ItemString, paintItemString },
        { Tiles::ItemGunpowder, paintItemGunpowder },
        { Tiles::ItemBread,     paintItemBread },
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
        Water,
        RedstoneOff,
        RedstoneOn
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

    // Item icons a pack can supply. Vanilla names, newest first.
    struct PackItem
    {
        int tile;
        const char* candidates[3];
    };

    const PackItem PACK_ITEMS[] = {
        { Tiles::ItemWheatSeeds, { "wheat_seeds", "seeds_wheat", nullptr } },
        { Tiles::ItemWheat,      { "wheat", nullptr, nullptr } },
        { Tiles::ItemLeather,    { "leather", nullptr, nullptr } },
        { Tiles::ItemRawBeef,    { "beef", "beef_raw", nullptr } },
        { Tiles::ItemRawPork,    { "porkchop", "porkchop_raw", nullptr } },
        { Tiles::ItemRawChicken, { "chicken", "chicken_raw", nullptr } },
        { Tiles::ItemRawMutton,  { "mutton", "mutton_raw", nullptr } },
        { Tiles::ItemFeather,    { "feather", nullptr, nullptr } },
        { Tiles::ItemBone,       { "bone", nullptr, nullptr } },
        { Tiles::ItemString,     { "string", nullptr, nullptr } },
        { Tiles::ItemGunpowder,  { "gunpowder", nullptr, nullptr } },
        { Tiles::ItemBread,      { "bread", nullptr, nullptr } },

        { Tiles::ItemStick,      { "stick", nullptr, nullptr } },
        { Tiles::ItemCoal,       { "coal", nullptr, nullptr } },
        { Tiles::ItemFlint,      { "flint", nullptr, nullptr } },
        { Tiles::ItemFlintAndSteel, { "flint_and_steel", nullptr, nullptr } },
        { Tiles::ItemBucket,      { "bucket", nullptr, nullptr } },
        { Tiles::ItemWaterBucket, { "water_bucket", nullptr, nullptr } },
        { Tiles::ItemLavaBucket,  { "lava_bucket", nullptr, nullptr } },
        { Tiles::ItemBow,        { "bow", "bow_standby", nullptr } },
        { Tiles::ItemArrow,      { "arrow", nullptr, nullptr } },
        { Tiles::ItemDiamond,    { "diamond", nullptr, nullptr } },
        { Tiles::ItemIronIngot,  { "iron_ingot", nullptr, nullptr } },
        { Tiles::ItemGoldIngot,  { "gold_ingot", nullptr, nullptr } },

        { Tiles::ItemPaper,      { "paper", nullptr, nullptr } },
        { Tiles::ItemBook,       { "book", nullptr, nullptr } },
        { Tiles::ItemEmerald,    { "emerald", nullptr, nullptr } },
        { Tiles::ItemEnderPearl, { "ender_pearl", nullptr, nullptr } },
        // Vanilla has always called the eye "ender_eye", not
        // "eye_of_ender", whatever the item is called in the game.
        { Tiles::ItemEyeOfEnder, { "ender_eye", nullptr, nullptr } },
        { Tiles::ItemGlassBottle,   { "glass_bottle", nullptr, nullptr } },
        { Tiles::ItemPotion,        { "potion", "potion_bottle_drinkable", nullptr } },
        { Tiles::ItemNetherWart,    { "nether_wart", nullptr, nullptr } },
        { Tiles::ItemSugar,         { "sugar", nullptr, nullptr } },
        { Tiles::ItemBlazePowder,   { "blaze_powder", nullptr, nullptr } },
        { Tiles::ItemBoat,          { "oak_boat", nullptr, nullptr } },
        { Tiles::ItemGlowstoneDust, { "glowstone_dust", nullptr, nullptr } },

        { Tiles::ItemToolFirst +  0, { "wooden_pickaxe", "wood_pickaxe", nullptr } },
        { Tiles::ItemToolFirst +  1, { "wooden_axe", "wood_axe", nullptr } },
        { Tiles::ItemToolFirst +  2, { "wooden_shovel", "wood_shovel", nullptr } },
        { Tiles::ItemToolFirst +  3, { "wooden_sword", "wood_sword", nullptr } },
        { Tiles::ItemToolFirst +  4, { "wooden_hoe", "wood_hoe", nullptr } },
        { Tiles::ItemToolFirst +  5, { "stone_pickaxe", nullptr, nullptr } },
        { Tiles::ItemToolFirst +  6, { "stone_axe", nullptr, nullptr } },
        { Tiles::ItemToolFirst +  7, { "stone_shovel", nullptr, nullptr } },
        { Tiles::ItemToolFirst +  8, { "stone_sword", nullptr, nullptr } },
        { Tiles::ItemToolFirst +  9, { "stone_hoe", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 10, { "iron_pickaxe", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 11, { "iron_axe", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 12, { "iron_shovel", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 13, { "iron_sword", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 14, { "iron_hoe", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 15, { "golden_pickaxe", "gold_pickaxe", nullptr } },
        { Tiles::ItemToolFirst + 16, { "golden_axe", "gold_axe", nullptr } },
        { Tiles::ItemToolFirst + 17, { "golden_shovel", "gold_shovel", nullptr } },
        { Tiles::ItemToolFirst + 18, { "golden_sword", "gold_sword", nullptr } },
        { Tiles::ItemToolFirst + 19, { "golden_hoe", "gold_hoe", nullptr } },
        { Tiles::ItemToolFirst + 20, { "diamond_pickaxe", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 21, { "diamond_axe", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 22, { "diamond_shovel", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 23, { "diamond_sword", nullptr, nullptr } },
        { Tiles::ItemToolFirst + 24, { "diamond_hoe", nullptr, nullptr } },
        { Tiles::ItemBlazeRod,   { "blaze_rod", nullptr, nullptr } },
        { Tiles::ItemNetherBrick, { "nether_brick", nullptr, nullptr } },
        { Tiles::ItemMagmaCream, { "magma_cream", nullptr, nullptr } },
        { Tiles::ItemRedstone,   { "redstone", nullptr, nullptr } },
        { Tiles::ItemBreezeRod,  { "breeze_rod", nullptr, nullptr } },
        { Tiles::ItemWindCharge, { "wind_charge", nullptr, nullptr } },
        { Tiles::ItemMace,       { "mace", nullptr, nullptr } },
        { Tiles::ItemTrialKey,   { "trial_key", nullptr, nullptr } },
        { Tiles::ItemRawCopper,  { "raw_copper", nullptr, nullptr } },
        { Tiles::ItemCopperIngot, { "copper_ingot", nullptr, nullptr } },
        { Tiles::ItemGlowBerries, { "glow_berries", nullptr, nullptr } },
        { Tiles::ItemOminousBottle, { "ominous_bottle", nullptr, nullptr } },
        { Tiles::ItemOminousKey, { "ominous_trial_key", nullptr, nullptr } },
        { Tiles::ItemNetherStar, { "nether_star", nullptr, nullptr } },
        { Tiles::ItemApple, { "apple", nullptr, nullptr } },
        { Tiles::ItemBoneMeal, { "bone_meal", nullptr, nullptr } },
        { Tiles::ItemCarrot, { "carrot", nullptr, nullptr } },
        { Tiles::ItemPotato, { "potato", nullptr, nullptr } },
        { Tiles::ItemBakedPotato, { "baked_potato", nullptr, nullptr } },
        { Tiles::ItemBowl, { "bowl", nullptr, nullptr } },
        { Tiles::ItemBeetroot, { "beetroot", nullptr, nullptr } },
        { Tiles::ItemBeetrootSeeds, { "beetroot_seeds", nullptr, nullptr } },
        { Tiles::ItemBeetrootSoup, { "beetroot_soup", nullptr, nullptr } },
        { Tiles::ItemMelonSlice, { "melon_slice", nullptr, nullptr } },
        { Tiles::ItemMelonSeeds, { "melon_seeds", nullptr, nullptr } },
        { Tiles::ItemEgg, { "egg", nullptr, nullptr } },
        { Tiles::ItemMilkBucket, { "milk_bucket", nullptr, nullptr } },
        { Tiles::ItemCake, { "cake", nullptr, nullptr } },
        { Tiles::ItemCookie, { "cookie", nullptr, nullptr } },
        { Tiles::ItemCocoaBeans, { "cocoa_beans", nullptr, nullptr } },
        { Tiles::ItemSign, { "oak_sign", nullptr, nullptr } },
        { Tiles::ItemItemFrame, { "item_frame", nullptr, nullptr } },
        { Tiles::ItemElytra, { "elytra", nullptr, nullptr } },
        { Tiles::ItemShulkerShell, { "shulker_shell", nullptr, nullptr } },
        { Tiles::ItemTrident, { "trident", nullptr, nullptr } },
    };

    const PackTile PACK_TILES[] = {
        { Tiles::Stone,         { "stone", nullptr },                                  Tinting::None,    0,   false },
        { Tiles::Dirt,          { "dirt", nullptr },                                   Tinting::None,    0,   false },
        { Tiles::GrassSide,     { "grass_block_side", "grass_side", nullptr },         Tinting::None,    0,   false },
        { Tiles::GrassTop,      { "grass_block_top", "grass_top", nullptr },           Tinting::Grass,   0,   false },
        { Tiles::Cobblestone,   { "cobblestone", nullptr },                            Tinting::None,    0,   false },
        { Tiles::Planks,        { "oak_planks", "planks_oak", nullptr },               Tinting::None,    0,   false },
        { Tiles::SpruceLogTop,  { "spruce_log_top", "log_spruce_top", nullptr },       Tinting::None,    0,   false },
        { Tiles::SpruceLogSide, { "spruce_log", "log_spruce", nullptr },               Tinting::None,    0,   false },
        { Tiles::SpruceLeaves,  { "spruce_leaves", "leaves_spruce", nullptr },         Tinting::None,    0,   false },
        { Tiles::SprucePlanks,  { "spruce_planks", "planks_spruce", nullptr },         Tinting::None,    0,   false },
        { Tiles::DarkOakLogTop, { "dark_oak_log_top", "log_big_oak_top", nullptr },    Tinting::None,    0,   false },
        { Tiles::DarkOakLogSide,{ "dark_oak_log", "log_big_oak", nullptr },            Tinting::None,    0,   false },
        { Tiles::DarkOakLeaves, { "dark_oak_leaves", "leaves_big_oak", nullptr },      Tinting::Foliage, 0,   false },
        { Tiles::DarkOakPlanks, { "dark_oak_planks", "planks_big_oak", nullptr },      Tinting::None,    0,   false },
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
        { Tiles::NetherBricks,  { "nether_bricks", "nether_brick", nullptr },          Tinting::None,    0,   false },
        { Tiles::JungleLogTop,  { "jungle_log_top", "log_jungle_top", nullptr },       Tinting::None,    0,   false },
        { Tiles::JungleLogSide, { "jungle_log", "log_jungle", nullptr },               Tinting::None,    0,   false },
        { Tiles::JungleLeaves,  { "jungle_leaves", "leaves_jungle", nullptr },         Tinting::Foliage, 0,   false },
        { Tiles::JunglePlanks,  { "jungle_planks", "planks_jungle", nullptr },         Tinting::None,    0,   false },
        { Tiles::AcaciaLogTop,  { "acacia_log_top", "log_acacia_top", nullptr },       Tinting::None,    0,   false },
        { Tiles::AcaciaLogSide, { "acacia_log", "log_acacia", nullptr },               Tinting::None,    0,   false },
        { Tiles::AcaciaLeaves,  { "acacia_leaves", "leaves_acacia", nullptr },         Tinting::Foliage, 0,   false },
        { Tiles::AcaciaPlanks,  { "acacia_planks", "planks_acacia", nullptr },         Tinting::None,    0,   false },
        { Tiles::RedstoneOre,   { "redstone_ore", nullptr },                           Tinting::None,    0,   false },
        // Dust is painted here rather than taken from a pack. A pack
        // ships three sprites -- a dot and two lines -- and Minecraft
        // picks between them by what each cell is joined to; this
        // engine's dust is one block that knows nothing of its
        // neighbours, so a pack's straight line comes out as a row of
        // dashes lying across the run.
        { Tiles::RedstoneTorch, { "redstone_torch", nullptr },                         Tinting::None,    0,   false },
        { Tiles::RedstoneTorchOff, { "redstone_torch_off", nullptr },                  Tinting::None,    0,   false },
        { Tiles::RedstoneLamp,  { "redstone_lamp", nullptr },                          Tinting::None,    0,   false },
        { Tiles::RedstoneLampLit, { "redstone_lamp_on", nullptr },                     Tinting::None,    0,   false },
        { Tiles::RedstoneBlockTile, { "redstone_block", nullptr },                     Tinting::None,    0,   false },
        { Tiles::TuffBricks,    { "tuff_bricks", nullptr },                            Tinting::None,    0,   false },
        { Tiles::TrialSpawnerTop, { "trial_spawner_top_inactive", nullptr },           Tinting::None,    0,   false },
        { Tiles::TrialSpawnerTopLit, { "trial_spawner_top_active", nullptr },          Tinting::None,    0,   false },
        { Tiles::TrialSpawnerSide, { "trial_spawner_side_inactive", nullptr },         Tinting::None,    0,   false },
        { Tiles::TrialSpawnerSideLit, { "trial_spawner_side_active", nullptr },        Tinting::None,    0,   false },
        { Tiles::VaultTop,      { "vault_top", nullptr },                              Tinting::None,    0,   false },
        { Tiles::VaultSide,     { "vault_front_off", nullptr },                        Tinting::None,    0,   false },
        { Tiles::VaultSideOpen, { "vault_front_on", nullptr },                         Tinting::None,    0,   false },
        { Tiles::OminousVaultSide, { "vault_front_off_ominous", nullptr },             Tinting::None,    0,   false },
        { Tiles::OminousVaultSideOpen, { "vault_front_on_ominous", nullptr },          Tinting::None,    0,   false },
        { Tiles::SpawnerTile,   { "spawner", "mob_spawner", nullptr },                 Tinting::None,    0,   false },
        { Tiles::PrismarineTile, { "prismarine", nullptr },                            Tinting::None,    0,   false },
        { Tiles::PrismarineBricksTile, { "prismarine_bricks", nullptr },               Tinting::None,    0,   false },
        { Tiles::DarkPrismarineTile, { "dark_prismarine", nullptr },                   Tinting::None,    0,   false },
        { Tiles::SeaLanternTile, { "sea_lantern", nullptr },                           Tinting::None,    0,   false },
        { Tiles::SpongeTile,    { "sponge", nullptr },                                 Tinting::None,    0,   false },
        { Tiles::WetSpongeTile, { "wet_sponge", nullptr },                             Tinting::None,    0,   false },
        { Tiles::KelpTile,      { "kelp_plant", nullptr },                             Tinting::None,    0,   false },
        { Tiles::KelpTopTile,   { "kelp", nullptr },                                   Tinting::None,    0,   false },
        { Tiles::SeagrassTile,  { "seagrass", "short_grass", nullptr },                Tinting::None,    0,   false },
        { Tiles::SeaPickleTile, { "sea_pickle", nullptr },                             Tinting::None,    0,   false },
        { Tiles::TubeCoralBlockTile, { "tube_coral_block", nullptr },                  Tinting::None,    0,   false },
        { Tiles::BrainCoralBlockTile, { "brain_coral_block", nullptr },                Tinting::None,    0,   false },
        { Tiles::BubbleCoralBlockTile, { "bubble_coral_block", nullptr },              Tinting::None,    0,   false },
        { Tiles::FireCoralBlockTile, { "fire_coral_block", nullptr },                  Tinting::None,    0,   false },
        { Tiles::HornCoralBlockTile, { "horn_coral_block", nullptr },                  Tinting::None,    0,   false },
        { Tiles::TubeCoralTile, { "tube_coral", nullptr },                             Tinting::None,    0,   false },
        { Tiles::BrainCoralTile, { "brain_coral", nullptr },                           Tinting::None,    0,   false },
        { Tiles::BubbleCoralTile, { "bubble_coral", nullptr },                         Tinting::None,    0,   false },
        { Tiles::FireCoralTile, { "fire_coral", nullptr },                             Tinting::None,    0,   false },
        { Tiles::HornCoralTile, { "horn_coral", nullptr },                             Tinting::None,    0,   false },
        { Tiles::DeepslateBricksTile, { "deepslate_bricks", nullptr },                 Tinting::None,    0,   false },
        { Tiles::DeepslateTilesTile, { "deepslate_tiles", nullptr },                   Tinting::None,    0,   false },
        { Tiles::SculkTile,     { "sculk", nullptr },                                  Tinting::None,    0,   false },
        { Tiles::SculkSensorTop, { "sculk_sensor_top", nullptr },                      Tinting::None,    0,   false },
        { Tiles::SculkSensorTopLit, { "sculk_sensor_top_active", "sculk_sensor_top", nullptr }, Tinting::None, 0, false },
        { Tiles::SculkSensorSide, { "sculk_sensor_side", nullptr },                    Tinting::None,    0,   false },
        { Tiles::SculkShriekerTop, { "sculk_shrieker_top", nullptr },                  Tinting::None,    0,   false },
        { Tiles::SculkShriekerSide, { "sculk_shrieker_side", nullptr },                Tinting::None,    0,   false },
        { Tiles::SculkCatalystTop, { "sculk_catalyst_top", nullptr },                  Tinting::None,    0,   false },
        { Tiles::SculkCatalystSide, { "sculk_catalyst_side", nullptr },                Tinting::None,    0,   false },
        { Tiles::SoulLanternTile, { "soul_lantern", nullptr },                         Tinting::None,    0,   false },
        { Tiles::ReinforcedDeepslateTop, { "reinforced_deepslate_top", nullptr },      Tinting::None,    0,   false },
        { Tiles::ReinforcedDeepslateSide, { "reinforced_deepslate_side", nullptr },    Tinting::None,    0,   false },
        { Tiles::PurpurTile,    { "purpur_block", nullptr },                           Tinting::None,    0,   false },
        { Tiles::PurpurPillarTop, { "purpur_pillar_top", nullptr },                    Tinting::None,    0,   false },
        { Tiles::PurpurPillarSide, { "purpur_pillar", nullptr },                       Tinting::None,    0,   false },
        { Tiles::EndBricksTile, { "end_stone_bricks", nullptr },                       Tinting::None,    0,   false },
        { Tiles::ChorusPlantTile, { "chorus_plant", nullptr },                         Tinting::None,    0,   false },
        { Tiles::ChorusFlowerTile, { "chorus_flower", nullptr },                       Tinting::None,    0,   false },
        { Tiles::BlackstoneTile, { "blackstone", nullptr },                            Tinting::None,    0,   false },
        { Tiles::PolishedBlackstoneTile, { "polished_blackstone_bricks", nullptr },    Tinting::None,    0,   false },
        { Tiles::GildedBlackstoneTile, { "gilded_blackstone", nullptr },               Tinting::None,    0,   false },
        { Tiles::BasaltTop,     { "basalt_top", nullptr },                             Tinting::None,    0,   false },
        { Tiles::BasaltSide,    { "basalt_side", nullptr },                            Tinting::None,    0,   false },
        { Tiles::LilyPadTile,   { "lily_pad", nullptr },                               Tinting::Foliage, 0,   false },
        { Tiles::VineTile,      { "vine", nullptr },                                   Tinting::Foliage, 0,   false },
        { Tiles::MudTile,       { "mud", nullptr },                                    Tinting::None,    0,   false },
        { Tiles::BeaconTile,    { "beacon", nullptr },                                 Tinting::None,    0,   false },
        { Tiles::IronBlockTile, { "iron_block", nullptr },                             Tinting::None,    0,   false },
        { Tiles::GoldBlockTile, { "gold_block", nullptr },                             Tinting::None,    0,   false },
        { Tiles::DiamondBlockTile, { "diamond_block", nullptr },                       Tinting::None,    0,   false },
        { Tiles::EmeraldBlockTile, { "emerald_block", nullptr },                       Tinting::None,    0,   false },
        { Tiles::CoalBlockTile, { "coal_block", nullptr },                             Tinting::None,    0,   false },
        { Tiles::NoteBlockTile, { "note_block", nullptr },                             Tinting::None,    0,   false },
        { Tiles::ObserverTop,   { "observer_front", nullptr },                         Tinting::None,    0,   false },
        { Tiles::ObserverTopLit, { "observer_front", nullptr },                        Tinting::None,    0,   false },
        { Tiles::ObserverSide,  { "observer_side", nullptr },                          Tinting::None,    0,   false },
        { Tiles::HopperTop,     { "hopper_top", nullptr },                             Tinting::None,    0,   false },
        { Tiles::HopperSide,    { "hopper_outside", nullptr },                         Tinting::None,    0,   false },
        { Tiles::DropperFront,  { "dropper_front", nullptr },                          Tinting::None,    0,   false },
        { Tiles::DispenserFront, { "dispenser_front", nullptr },                       Tinting::None,    0,   false },
        { Tiles::AmethystTile,  { "amethyst_block", nullptr },                         Tinting::None,    0,   false },
        { Tiles::BuddingTile,   { "budding_amethyst", nullptr },                       Tinting::None,    0,   false },
        { Tiles::ClusterTile,   { "amethyst_cluster", nullptr },                       Tinting::None,    0,   false },
        { Tiles::SmoothBasaltTile, { "smooth_basalt", nullptr },                       Tinting::None,    0,   false },
        // The crops. Minecraft draws carrots and potatoes in four
        // stages and spreads them over eight block states, which is
        // the mapping below; wheat gets one texture per stage. All
        // three were painted-only until now, which is why a field
        // looked hand-drawn next to the ground it stood in.
        { Tiles::WheatStage0, { "wheat_stage0", nullptr },                          Tinting::None,    0,   false },
        { Tiles::WheatStage1, { "wheat_stage1", nullptr },                          Tinting::None,    0,   false },
        { Tiles::WheatStage2, { "wheat_stage2", nullptr },                          Tinting::None,    0,   false },
        { Tiles::WheatStage3, { "wheat_stage3", nullptr },                          Tinting::None,    0,   false },
        { Tiles::WheatStage4, { "wheat_stage4", nullptr },                          Tinting::None,    0,   false },
        { Tiles::WheatStage5, { "wheat_stage5", nullptr },                          Tinting::None,    0,   false },
        { Tiles::WheatStage6, { "wheat_stage6", nullptr },                          Tinting::None,    0,   false },
        { Tiles::WheatStage7, { "wheat_stage7", nullptr },                          Tinting::None,    0,   false },
        { Tiles::CarrotStage0, { "carrots_stage0", nullptr },                      Tinting::None,    0,   false },
        { Tiles::CarrotStage1, { "carrots_stage0", nullptr },                      Tinting::None,    0,   false },
        { Tiles::CarrotStage2, { "carrots_stage1", nullptr },                      Tinting::None,    0,   false },
        { Tiles::CarrotStage3, { "carrots_stage1", nullptr },                      Tinting::None,    0,   false },
        { Tiles::CarrotStage4, { "carrots_stage2", nullptr },                      Tinting::None,    0,   false },
        { Tiles::CarrotStage5, { "carrots_stage2", nullptr },                      Tinting::None,    0,   false },
        { Tiles::CarrotStage6, { "carrots_stage2", nullptr },                      Tinting::None,    0,   false },
        { Tiles::CarrotStage7, { "carrots_stage3", nullptr },                      Tinting::None,    0,   false },
        { Tiles::PotatoStage0, { "potatoes_stage0", nullptr },                     Tinting::None,    0,   false },
        { Tiles::PotatoStage1, { "potatoes_stage0", nullptr },                     Tinting::None,    0,   false },
        { Tiles::PotatoStage2, { "potatoes_stage1", nullptr },                     Tinting::None,    0,   false },
        { Tiles::PotatoStage3, { "potatoes_stage1", nullptr },                     Tinting::None,    0,   false },
        { Tiles::PotatoStage4, { "potatoes_stage2", nullptr },                     Tinting::None,    0,   false },
        { Tiles::PotatoStage5, { "potatoes_stage2", nullptr },                     Tinting::None,    0,   false },
        { Tiles::PotatoStage6, { "potatoes_stage2", nullptr },                     Tinting::None,    0,   false },
        { Tiles::PotatoStage7, { "potatoes_stage3", nullptr },                     Tinting::None,    0,   false },
        { Tiles::LanternTile,   { "lantern", nullptr },                                Tinting::None,    0,   false },
        { Tiles::BarrelTop,     { "barrel_top", nullptr },                             Tinting::None,    0,   false },
        { Tiles::BarrelSide,    { "barrel_side", nullptr },                            Tinting::None,    0,   false },
        { Tiles::CampfireTop,   { "campfire_fire", nullptr },                          Tinting::None,    0,   false },
        { Tiles::CampfireLog,   { "campfire_log", nullptr },                           Tinting::None,    0,   false },
        { Tiles::SmokerFront,   { "smoker_front", nullptr },                           Tinting::None,    0,   false },
        { Tiles::SmokerFrontLit,{ "smoker_front_on", nullptr },                        Tinting::None,    0,   false },
        { Tiles::SmokerTop,     { "smoker_top", nullptr },                             Tinting::None,    0,   false },
        { Tiles::SmokerSide,    { "smoker_side", nullptr },                            Tinting::None,    0,   false },
        { Tiles::BlastFurnaceFront,    { "blast_furnace_front", nullptr },             Tinting::None,    0,   false },
        { Tiles::BlastFurnaceFrontLit, { "blast_furnace_front_on", nullptr },          Tinting::None,    0,   false },
        { Tiles::BlastFurnaceTop,      { "blast_furnace_top", nullptr },               Tinting::None,    0,   false },
        { Tiles::BlastFurnaceSide,     { "blast_furnace_side", nullptr },              Tinting::None,    0,   false },
        { Tiles::ComposterSide, { "composter_side", nullptr },                         Tinting::None,    0,   false },
        { Tiles::ComposterBottom, { "composter_bottom", nullptr },                     Tinting::None,    0,   false },
        { Tiles::ComposterTop,  { "composter_top", nullptr },                          Tinting::None,    0,   false },
        // The pack has one compost texture and one for when it is
        // ready, where this has three heights of fill. The three all
        // take the compost; only the last is the green one.
        { Tiles::ComposterFill1, { "composter_compost", nullptr },                     Tinting::None,    0,   false },
        { Tiles::ComposterFill2, { "composter_compost", nullptr },                     Tinting::None,    0,   false },
        { Tiles::ComposterFill3, { "composter_compost", nullptr },                     Tinting::None,    0,   false },
        { Tiles::ComposterReady, { "composter_ready", nullptr },                       Tinting::None,    0,   false },
        { Tiles::GrindstoneSide, { "grindstone_side", nullptr },                       Tinting::None,    0,   false },
        { Tiles::GrindstoneFace, { "grindstone_round", nullptr },                      Tinting::None,    0,   false },
        { Tiles::GrindstonePivot, { "grindstone_pivot", nullptr },                     Tinting::None,    0,   false },
        { Tiles::LecternTop,    { "lectern_top", nullptr },                            Tinting::None,    0,   false },
        { Tiles::LecternSide,   { "lectern_sides", nullptr },                          Tinting::None,    0,   false },
        { Tiles::LecternFront,  { "lectern_front", nullptr },                          Tinting::None,    0,   false },
        { Tiles::LecternBase,   { "lectern_base", nullptr },                           Tinting::None,    0,   false },
        { Tiles::CauldronSide,  { "cauldron_side", nullptr },                          Tinting::None,    0,   false },
        { Tiles::CauldronBottom,{ "cauldron_bottom", nullptr },                        Tinting::None,    0,   false },
        { Tiles::CauldronTop,   { "cauldron_inner", nullptr },                         Tinting::None,    0,   false },
        // Minecraft draws the water in a cauldron as a layer over the
        // inner wall. There is no layer here, so the three levels take
        // the water texture itself and the painted versions keep the
        // depth the level means.
        { Tiles::CauldronWater1, { "water_still", nullptr },                           Tinting::Water,   0,   true  },
        { Tiles::CauldronWater2, { "water_still", nullptr },                           Tinting::Water,   0,   true  },
        { Tiles::CauldronWater3, { "water_still", nullptr },                           Tinting::Water,   0,   true  },
        // Four pictures over eight states, which is Minecraft's own
        // mapping: a beet looks the same for two stages at a time.
        { Tiles::BeetrootStage0, { "beetroots_stage0", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BeetrootStage1, { "beetroots_stage0", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BeetrootStage2, { "beetroots_stage1", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BeetrootStage3, { "beetroots_stage1", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BeetrootStage4, { "beetroots_stage2", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BeetrootStage5, { "beetroots_stage2", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BeetrootStage6, { "beetroots_stage2", nullptr },                      Tinting::None,    0,   false },
        { Tiles::BeetrootStage7, { "beetroots_stage3", nullptr },                      Tinting::None,    0,   false },
        { Tiles::MelonSide,     { "melon_side", nullptr },                             Tinting::None,    0,   false },
        { Tiles::MelonTop,      { "melon_top", nullptr },                              Tinting::None,    0,   false },
        { Tiles::MelonStemYoung,{ "melon_stem", nullptr },                             Tinting::None,    0,   false },
        { Tiles::MelonStemRipe, { "attached_melon_stem", nullptr },                    Tinting::None,    0,   false },
        // The leaves rather than the stalk. Minecraft tiles the stalk
        // texture onto a 2px column and hangs the leaves off it as
        // separate sprites; this engine draws one cross, and a fully
        // opaque stalk texture on it came out as a solid green slab
        // the width of the block.
        { Tiles::BambooTile,    { "bamboo_large_leaves", "bamboo_stalk", nullptr },    Tinting::None,    0,   false },
        { Tiles::CocoaStage0,   { "cocoa_stage0", nullptr },                           Tinting::None,    0,   false },
        { Tiles::CocoaStage1,   { "cocoa_stage1", nullptr },                           Tinting::None,    0,   false },
        { Tiles::CocoaStage2,   { "cocoa_stage2", nullptr },                           Tinting::None,    0,   false },
        { Tiles::CakeTop,       { "cake_top", nullptr },                               Tinting::None,    0,   false },
        { Tiles::CakeSide,      { "cake_side", nullptr },                              Tinting::None,    0,   false },
        { Tiles::CakeBottom,    { "cake_bottom", nullptr },                            Tinting::None,    0,   false },
        { Tiles::CakeInner,     { "cake_inner", nullptr },                             Tinting::None,    0,   false },
        { Tiles::CandleTile,    { "candle", nullptr },                                 Tinting::None,    0,   false },
        { Tiles::CandleLitTile, { "candle_lit", nullptr },                             Tinting::None,    0,   false },
        { Tiles::ScaffoldTop,   { "scaffolding_top", nullptr },                        Tinting::None,    0,   false },
        { Tiles::ScaffoldSide,  { "scaffolding_side", nullptr },                       Tinting::None,    0,   false },
        { Tiles::ScaffoldBottom,{ "scaffolding_bottom", nullptr },                     Tinting::None,    0,   false },
        // A sign is a plank board. Minecraft draws its signs off an
        // entity sheet, so a pack has no block texture to give: the
        // planks are what it is made of, and the words go over it.
        { Tiles::SignTile,      { "oak_planks", "planks_oak", nullptr },               Tinting::None,    0,   false },
        { Tiles::ItemFrameTile, { "item_frame", nullptr },                             Tinting::None,    0,   false },
        { Tiles::ShulkerBoxTile,{ "shulker_box", nullptr },                            Tinting::None,    0,   false },
        { Tiles::PistonTop,     { "piston_side", nullptr },                            Tinting::None,    0,   false },
        { Tiles::PistonSide,    { "piston_side", nullptr },                            Tinting::None,    0,   false },
        { Tiles::PistonFace,    { "piston_top", "piston_top_normal", nullptr },         Tinting::None,    0,   false },
        { Tiles::StickyPistonFace, { "piston_top_sticky", nullptr },                   Tinting::None,    0,   false },
        { Tiles::PistonBack,    { "piston_bottom", nullptr },                          Tinting::None,    0,   false },
        { Tiles::PistonArm,     { "piston_inner", nullptr },                           Tinting::None,    0,   false },
        { Tiles::CrafterTop,    { "crafter_top", nullptr },                            Tinting::None,    0,   false },
        { Tiles::CrafterTopLit, { "crafter_top_crafting", nullptr },                   Tinting::None,    0,   false },
        { Tiles::CrafterSide,   { "crafter_west", "crafter_south", nullptr },          Tinting::None,    0,   false },
        { Tiles::CopperOre,     { "copper_ore", nullptr },                             Tinting::None,    0,   false },
        { Tiles::CopperBlock,   { "copper_block", nullptr },                           Tinting::None,    0,   false },
        { Tiles::ExposedCopper, { "exposed_copper", nullptr },                         Tinting::None,    0,   false },
        { Tiles::WeatheredCopper, { "weathered_copper", nullptr },                     Tinting::None,    0,   false },
        { Tiles::OxidizedCopper, { "oxidized_copper", nullptr },                       Tinting::None,    0,   false },
        { Tiles::CopperBulbTile, { "copper_bulb", nullptr },                           Tinting::None,    0,   false },
        { Tiles::CopperBulbLitTile, { "copper_bulb_lit", nullptr },                    Tinting::None,    0,   false },
        { Tiles::DripstoneBlock, { "dripstone_block", nullptr },                       Tinting::None,    0,   false },
        { Tiles::DripstoneSpike, { "pointed_dripstone_down_tip", "pointed_dripstone_down_base", nullptr },
                                                                                       Tinting::None,    0,   false },
        { Tiles::MossTile,      { "moss_block", nullptr },                             Tinting::None,    0,   false },
        { Tiles::AzaleaLeavesTile, { "flowering_azalea_leaves", "azalea_leaves", nullptr },
                                                                                       Tinting::None,    0,   false },
        { Tiles::GlowBerriesTile, { "cave_vines_plant_lit", "cave_vines_lit", nullptr },
                                                                                       Tinting::None,    0,   false },

        // Everything added since the pack table was last gone over.
        // A tile with no row here is drawn procedurally however good a
        // pack is installed, which is not wrong but is a waste of the
        // pack -- and it shows, because the block next to it is not.
        { Tiles::EmeraldOre,    { "emerald_ore", nullptr },                            Tinting::None,    0,   false },
        { Tiles::BookshelfSide, { "bookshelf", nullptr },                              Tinting::None,    0,   false },
        { Tiles::AnvilTop,         { "anvil_top", nullptr },                           Tinting::None,    0,   false },
        { Tiles::NetherWartTile,   { "nether_wart_stage2", "nether_wart", nullptr },  Tinting::None,    0,   false },
        { Tiles::BrewingStandTile, { "brewing_stand", nullptr },                       Tinting::None,    0,   false },
        { Tiles::Granite,          { "granite", nullptr, nullptr },                  Tinting::None,    0,   false },
        { Tiles::Diorite,          { "diorite", nullptr, nullptr },                  Tinting::None,    0,   false },
        { Tiles::Andesite,         { "andesite", nullptr, nullptr },                 Tinting::None,    0,   false },
        { Tiles::Deepslate,        { "deepslate", nullptr, nullptr },                Tinting::None,    0,   false },
        { Tiles::DeepslateTop,     { "deepslate_top", "deepslate", nullptr },        Tinting::None,    0,   false },
        { Tiles::Tuff,             { "tuff", nullptr, nullptr },                     Tinting::None,    0,   false },
        { Tiles::Calcite,          { "calcite", nullptr, nullptr },                  Tinting::None,    0,   false },
        { Tiles::RailTile,         { "rail", "rail_normal", nullptr },               Tinting::None,    0,   false },
        { Tiles::RailCorner,       { "rail_corner", "rail_normal_turned", nullptr }, Tinting::None,    0,   false },
        { Tiles::DirtPathTop,      { "dirt_path_top", "grass_path_top", nullptr },    Tinting::None,    0,   false },
        { Tiles::DirtPathSide,     { "dirt_path_side", "grass_path_side", nullptr },  Tinting::None,    0,   false },
        { Tiles::AnvilSide,        { "anvil", nullptr },                               Tinting::None,    0,   false },
        { Tiles::EnchantTableTop,  { "enchanting_table_top", nullptr },                Tinting::None,    0,   false },
        { Tiles::EnchantTableSide, { "enchanting_table_side", nullptr },               Tinting::None,    0,   false },
        { Tiles::EndFrameTop,   { "end_portal_frame_top", nullptr },                   Tinting::None,    0,   false },
        { Tiles::EndFrameEye,   { "end_portal_frame_eye", nullptr },                   Tinting::None,    0,   false },
        { Tiles::EndFrameSide,  { "end_portal_frame_side", nullptr },                  Tinting::None,    0,   false },

        // Cane is a crossed-quad plant, so it keeps whatever alpha the
        // pack gives it the way the flowers and the grass do.
        { Tiles::SugarCane,     { "sugar_cane", nullptr },                             Tinting::Grass,   0,   false },
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
            case Tinting::RedstoneOff: tintImage(image, REDSTONE_OFF_TINT); break;
            case Tinting::RedstoneOn:  tintImage(image, REDSTONE_ON_TINT); break;
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

    // The twenty-five tools are one shape in one of five metals, so they
    // are painted from the offset rather than listed one by one.
    for (int i = 0; i < 25; ++i)
    {
        Tile tile;
        paintToolTile(tile, i % 5, i / 5);
        blitPainted(Tiles::ItemToolFirst + i, tile);
    }

    for (int i = 0; i < 16; ++i)
    {
        Tile tile;
        paintArmourTile(tile, i % 4, i / 4);
        blitPainted(Tiles::ItemArmourFirst + i, tile);
    }

    // Item icons out of the pack, over the painted ones. These are their
    // own folder and have none of the block machinery about them, so they
    // are a plain loop rather than another case in the one below. The
    // pack need not have a block folder for this to be worth doing.
    {
        int items = 0;
        for (const PackItem& entry : PACK_ITEMS)
        {
            Image image;
            for (int i = 0; entry.candidates[i] != nullptr && !image.valid(); ++i)
                image = loadPNG(std::string(ITEM_PACK_DIRECTORY) + "/" + entry.candidates[i] + ".png");

            // A pack may hold an animated item as a column of frames; the
            // first one is the icon.
            if (!image.valid()) continue;
            if (image.height > image.width && image.width > 0 && image.height % image.width == 0)
                image = extractFrame(image, 0, image.width);

            blitImage(entry.tile, scaleNearest(image, tilePixels, tilePixels));
            ++items;
        }

        if (items > 0)
            std::printf("Textures: %d of %d item icons from %s\n",
                        items, static_cast<int>(std::size(PACK_ITEMS)), ITEM_PACK_DIRECTORY);
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
                    Image tile = scaleNearest(source, tilePixels, tilePixels);
                    Atlas::stretchHeightRange(tile.rgba);
                    blitInto(normalPixels, entry.tile, tile);
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
                    Image tile = scaleNearest(normalMap, tilePixels, tilePixels);
                    Atlas::stretchHeightRange(tile.rgba);
                    blitInto(normalPixels, entry.tile, tile);
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

std::vector<uint8_t> Atlas::crackOverlay(int stage)
{
    return crackStagePixels(std::clamp(stage, 0, Tiles::CrackStages - 1));
}

int Atlas::crackOverlaySize()
{
    return Atlas::FALLBACK_TILE_PIXELS;
}

void Atlas::stretchHeightRange(std::vector<uint8_t>& rgba)
{
    uint8_t low = 255, high = 0;
    for (size_t i = 3; i < rgba.size(); i += 4)
    {
        low = std::min(low, rgba[i]);
        high = std::max(high, rgba[i]);
    }

    // Under this the pack is saying the surface is flat, not that it
    // drew something very subtle.
    const int span = static_cast<int>(high) - static_cast<int>(low);
    if (span < HEIGHT_MIN_SPAN) return;

    for (size_t i = 3; i < rgba.size(); i += 4)
        rgba[i] = static_cast<uint8_t>(((static_cast<int>(rgba[i]) - low) * 255) / span);
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

bool Atlas::hasPainter(int tileIndex)
{
    for (const TileEntry& entry : TILE_PAINTERS)
        if (entry.index == tileIndex) return true;

    // Three families are painted by a loop rather than by a row each,
    // because their tiles differ only by which kind and which tier they
    // are: the twenty-five tools, the sixteen pieces of armour, and the
    // crack overlays, which are built from one another rather than
    // drawn at all. None of them is missing for having no row.
    if (tileIndex >= Tiles::ItemToolFirst && tileIndex < Tiles::ItemToolEnd) return true;
    if (tileIndex >= Tiles::ItemArmourFirst && tileIndex < Tiles::ItemArmourEnd) return true;
    if (tileIndex >= Tiles::CrackFirst &&
        tileIndex < Tiles::CrackFirst + Tiles::CrackStages)
        return true;

    return false;
}


// ---------------------------------------------------------------------
// Which tiles the pack does not cover.
//
// Walks the same two tables the loader walks and asks only whether a
// file is there, so it needs no GL and no window. A name with a typo
// in it is otherwise invisible: the tile comes out painted and nothing
// says so.
// ---------------------------------------------------------------------
int packTileCount()
{
    return static_cast<int>(std::size(PACK_TILES)) + static_cast<int>(std::size(PACK_ITEMS));
}

std::vector<PackGap> packGaps()
{
    std::vector<PackGap> gaps;

    for (const PackTile& entry : PACK_TILES)
    {
        PackGap gap;
        bool found = false;
        for (int i = 0; i < 4 && entry.candidates[i]; ++i)
        {
            gap.tried.push_back(entry.candidates[i]);
            if (fileExists(std::string(PACK_DIRECTORY) + "/" + entry.candidates[i] + ".png"))
                found = true;
        }
        if (!found && !gap.tried.empty())
        {
            gap.what = "block tile " + std::to_string(entry.tile);
            gaps.push_back(gap);
        }
    }

    for (const PackItem& entry : PACK_ITEMS)
    {
        PackGap gap;
        bool found = false;
        for (int i = 0; i < 3 && entry.candidates[i]; ++i)
        {
            gap.tried.push_back(entry.candidates[i]);
            if (fileExists(std::string(ITEM_PACK_DIRECTORY) + "/" + entry.candidates[i] + ".png"))
                found = true;
        }
        if (!found && !gap.tried.empty())
        {
            gap.what = "item tile " + std::to_string(entry.tile);
            gaps.push_back(gap);
        }
    }

    return gaps;
}
