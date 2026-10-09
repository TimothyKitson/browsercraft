#pragma once
#include "AtlasTiles.h"
#include <vector>
#include <cstdint>
#include <string>

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
    // Highest per-tile resolution we'll build an atlas at. Higher-resolution
    // packs are box-filtered down to this. 256 means three 4096x4096
    // textures (albedo + normal + specular), roughly 270 MB of VRAM with
    // mipmaps; drop to 64 or 128 on a weaker GPU.
#ifdef __EMSCRIPTEN__
    static constexpr int MAX_TILE_PIXELS = 64; // keep the web download small
#else
    static constexpr int MAX_TILE_PIXELS = 256;
#endif

    Atlas();
    ~Atlas();

    Atlas(const Atlas&) = delete;
    Atlas& operator=(const Atlas&) = delete;

    // Advances animated tiles (flowing water, lava).
    void update(float deltaTime);

    // Binds albedo, normal and specular atlases to three texture units.
    void bind(unsigned int albedoUnit = 0, unsigned int normalUnit = 1, unsigned int specularUnit = 2) const;
    unsigned int textureId() const { return m_id; }

    int tilePixels() const { return m_tilePixels; }
    int loadedFromPack() const { return m_loadedFromPack; }
    // Whoever made the installed pack, read from assets/textures/CREDIT.txt.
    // Empty when no pack is installed or the pack came with no credit.
    // Most packs are free to redistribute only if they are credited, and
    // the repository never contains the pack itself, so it cannot name
    // one: the credit has to arrive with the art it belongs to.
    const std::string& packCredit() const { return m_packCredit; }
    // True when the pack supplied LabPBR normal/specular maps.
    bool hasPbrMaps() const { return m_hasPbrMaps; }

    // Atlas width/height in pixels; used for the UV half-texel inset.
    static int pixelSize();

    // One stage of the block-breaking overlay as RGBA, square and
    // crackOverlaySize() on a side. Exposed only so --selftest can check
    // that the stages build on one another instead of each being its own
    // picture.
    static std::vector<uint8_t> crackOverlay(int stage);
    static int crackOverlaySize();

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

    unsigned int m_id = 0;         // albedo
    unsigned int m_normalId = 0;   // LabPBR _n: normal XY, AO, height
    unsigned int m_specularId = 0; // LabPBR _s: smoothness, F0, porosity, emission
    int m_tilePixels = FALLBACK_TILE_PIXELS;
    int m_loadedFromPack = 0;
    std::string m_packCredit;
    bool m_hasPbrMaps = false;
    // True when the maps came from a Bedrock pack and were converted.
    bool m_bedrockMaps = false;
    std::vector<Animation> m_animations;

    void uploadTile(int tile, const uint8_t* rgba);
};
