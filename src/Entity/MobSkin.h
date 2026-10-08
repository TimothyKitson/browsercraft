#pragma once
#include "MobType.h"
#include <cstdint>
#include <string>
#include <vector>

// A mob's hide.
//
// Two sources, the same arrangement PlayerSkin uses. If a sheet is
// sitting in assets/skins/mob it is used as it is, so a real texture
// pack gives you real-looking animals; otherwise one is painted in code.
// Mojang's mob textures are Mojang's and cannot be redistributed, so the
// repository holds the painting and never the art -- the art arrives
// through tools/install-mob-textures.ps1 on your own machine.
//
// Either way the model is unwrapped at Mojang's own texture offsets, so
// the painted hide and the real one land on exactly the same pixels.
class MobSkin
{
public:
    MobSkin() = default;
    ~MobSkin();

    MobSkin(const MobSkin&) = delete;
    MobSkin& operator=(const MobSkin&) = delete;

    // Loads or paints the species and uploads its sheets.
    void build(MobId id);

    // Just the model: resolves the boxes without painting a pixel or
    // touching GL, so --selftest can measure the species.
    void layout(MobId id);

    unsigned int textureId() const { return m_texture; }
    // A second sheet drawn over the first. Zero for every species but
    // the sheep, whose wool is its own layer.
    unsigned int overlayTextureId() const { return m_overlayTexture; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    // True when the pixels came from a file rather than from code.
    bool fromFile() const { return m_fromFile; }

    const std::vector<MobBox>& boxes() const { return m_boxes; }

    // Where a species' sheets are looked for.
    static std::string texturePath(const char* name);

    // Whether every patch of a model lands inside a sheet of this size.
    // A box that runs off the edge silently wraps onto another limb,
    // which is why --selftest checks it.
    static bool fits(const std::vector<MobBox>& boxes, int sheetWidth, int sheetHeight);

private:
    unsigned int m_texture = 0;
    unsigned int m_overlayTexture = 0;
    int m_width = 64;
    int m_height = 32;
    bool m_fromFile = false;
    std::vector<MobBox> m_boxes;
    std::vector<uint8_t> m_pixels;

    bool loadFile(const std::string& path, std::vector<uint8_t>& pixels, int& w, int& h);
    void paint(const MobType& type);
    unsigned int upload(const std::vector<uint8_t>& pixels, int w, int h, unsigned int into);
};
