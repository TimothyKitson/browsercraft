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

// Which tiles a pack does not supply, so they fall back to a painted
// one. Pure filesystem work -- no GL, no window -- so --packcheck can
// ask it without starting the game.
//
// This exists because a pack name with a typo in it is invisible: the
// tile just quietly comes out painted, and the only way to notice is
// to recognise a hand-drawn block in a screenshot full of real ones.
struct PackGap
{
    std::string what;              // the tile or item, by name
    std::vector<std::string> tried;
};

std::vector<PackGap> packGaps();
int packTileCount();

// The block texture atlas: a 16x16 grid of square tiles.
//
// Tiles come from a resource pack in assets/textures/block (vanilla
// Minecraft file names, any resolution), and any tile the pack doesn't
// provide falls back to a procedurally painted one, so the game always
// runs even with no art files present.
class Atlas
{
public:
    // Twenty-four across, so five hundred and seventy-six tiles.
    // Sixteen was 256 and the blocks filled it; twenty was 400 and the
    // crops filled that.
    //
    // This time the grid grew and the atlas got smaller. Twenty-four
    // tiles at 96 pixels is 2304 square, against twenty at 128 which
    // was 2560 -- about a fifth less texture memory for nearly half as
    // many tiles again. Nothing is lost by it: the packs in use are 16
    // and 32 pixels a tile, so 96 was already far more room than any
    // of their art needs.
    static constexpr int TILES_PER_ROW = 24;
    static constexpr int FALLBACK_TILE_PIXELS = 16;
    // Highest per-tile resolution we'll build an atlas at. Higher-resolution
    // packs are box-filtered down to this.
    //
    // Twenty tiles across at 128 is three 2560-square textures, about
    // 105 MB of VRAM with mipmaps -- less than the sixteen-across grid
    // cost at 256, which was nearer 270. The grid grew and the ceiling
    // came down, so the memory went down with it. A pack finer than
    // 128 is reduced to it; the two in use here are 16 and 32.
#ifdef __EMSCRIPTEN__
    static constexpr int MAX_TILE_PIXELS = 64; // keep the web download small
#else
    static constexpr int MAX_TILE_PIXELS = 96;
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
    // For the HUD, which lights its block icons off the same maps the
    // world does so a block in the hotbar matches the one underfoot.
    unsigned int normalTextureId() const { return m_normalId; }

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

    // Whether anything knows how to draw this tile. Exposed only so
    // --selftest can sweep every block and item and catch one that was
    // given a tile number nobody ever paints: the atlas leaves those
    // transparent, and a block with an unpainted tile is invisible in
    // the world without anything going wrong anywhere else.
    static bool hasPainter(int tileIndex);

    // Stretches a tile's LabPBR height channel (the normal map's alpha)
    // to use the full range it was drawn in. Packs put height in a
    // narrow band near the top, which parallax and the crevice shading
    // both read as a flat surface. A tile with no variation is left
    // alone: sand really is flat.
    static void stretchHeightRange(std::vector<uint8_t>& rgba);

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
