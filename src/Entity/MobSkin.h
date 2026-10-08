#pragma once
#include "MobType.h"
#include <cstdint>
#include <vector>

// A mob's hide, painted in code.
//
// Same reasoning as PlayerSkin: Mojang's mob textures are Mojang's and
// cannot ship in a public build, and a painted hide costs nothing to
// download. Each species gets one sheet, laid out by unwrapping its own
// boxes and packing them, so adding a box to a model needs no hand-drawn
// texture to go with it.
class MobSkin
{
public:
    static constexpr int SHEET = 64;

    MobSkin() = default;
    ~MobSkin();

    MobSkin(const MobSkin&) = delete;
    MobSkin& operator=(const MobSkin&) = delete;

    // Packs and paints the species, filling in each box's (u, v).
    void build(MobId id);

    // Just the packing: resolves every box's (u, v) without painting a
    // pixel or touching GL, so --selftest can measure the models.
    void layout(MobId id);

    unsigned int textureId() const { return m_texture; }
    int width() const { return SHEET; }
    int height() const { return SHEET; }

    // The model with its texture coordinates resolved.
    const std::vector<MobBox>& boxes() const { return m_boxes; }

    // Lays out a model on a sheet of the given size, without painting or
    // touching GL. Returns false if it will not fit, which --selftest
    // uses to check every species packs.
    static bool pack(std::vector<MobBox>& boxes, int sheet);

private:
    unsigned int m_texture = 0;
    std::vector<MobBox> m_boxes;
    std::vector<uint8_t> m_pixels;

    void paint(const MobType& type);
    void upload();
};
