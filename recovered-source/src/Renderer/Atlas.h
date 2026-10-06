#pragma once
#include "AtlasTiles.h"
#include <vector>
#include <cstdint>

// UV rectangle of one tile inside the atlas. vTop/vBottom are named rather
// than v0/v1 because the atlas is stored top-down: the visually-upper edge
// of a tile has the *smaller* v.
struct TileUV
{
    float u0, u1, vTop, vBottom;
};

// Pure function -- safe to call from mesher worker threads.
TileUV tileUV(int tileIndex);

// The block texture atlas: a 16x16 grid of square tiles.
//
// Tiles come from a resource pack in assets/textures/block (vanilla
// Minecraft file names, any resolution), and any tile the pack doesn't
// provide falls back to a procedurally painted one, so the game always
// runs even with no art files present.
class Atlas
{
public:
    static constexpr int TILES_PER_ROW = 16;
    static constexpr int FALLBACK_TILE_PIXELS = 16;

    Atlas();
    ~Atlas();

    Atlas(const Atlas&) = delete;
    Atlas& operator=(const Atlas&) = delete;

    // Advances animated tiles (flowing water, lava).
    void update(float deltaTime);

    void bind(unsigned int unit = 0) const;
    unsigned int textureId() const { return m_id; }

    int tilePixels() const { return m_tilePixels; }
    int loadedFromPack() const { return m_loadedFromPack; }

    // Atlas width/height in pixels; used for the UV half-texel inset.
    static int pixelSize();

private:
    // One multi-frame tile (Minecraft stores these as a vertical strip).
    struct Animation
    {
        int tile = 0;
        int frameCount = 0;
        float frameSeconds = 0.1f;
        float timer = 0.0f;
        int current = 0;
        std::vector<uint8_t> frames; // frameCount * tilePixels * tilePixels * 4
    };

    unsigned int m_id = 0;
    int m_tilePixels = FALLBACK_TILE_PIXELS;
    int m_loadedFromPack = 0;
    std::vector<Animation> m_animations;

    void uploadTile(int tile, const uint8_t* rgba);
};
