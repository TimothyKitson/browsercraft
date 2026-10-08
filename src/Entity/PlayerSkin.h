#pragma once
#include <cstdint>
#include <string>
#include <vector>

// A player skin in the classic layout.
//
// Two sources. If a PNG is sitting in assets/skins it is used as-is, so
// you can drop in whatever skin you like on your own machine. Otherwise
// one is painted in code: Mojang's own skins are theirs, so they cannot
// be redistributed in a public build, and a generated skin also costs
// nothing to download -- which matters for a game that has to arrive
// over a school's wifi.
class PlayerSkin
{
public:
    enum class Style
    {
        Classic,  // four-pixel arms
        Slim      // three-pixel arms
    };

    // A rectangle of the skin texture, in UV space, ready to draw.
    struct Patch
    {
        float u0, v0, u1, v1;
    };

    PlayerSkin() = default;
    ~PlayerSkin();

    PlayerSkin(const PlayerSkin&) = delete;
    PlayerSkin& operator=(const PlayerSkin&) = delete;

    // How many painted characters the picker can offer.
    static int variantCount();
    static const char* variantName(int variant);

    // Paints the chosen character, or loads assets/skins/custom.png when
    // `variant` is CUSTOM_VARIANT. Safe to call again to change either.
    static constexpr int CUSTOM_VARIANT = -1;
    void build(Style style, int variant = 0);
    // True when assets/skins/custom.png exists and is a usable skin.
    static bool customAvailable();

    int variant() const { return m_variant; }

    unsigned int textureId() const { return m_texture; }
    Style style() const { return m_style; }
    int armWidth() const { return m_style == Style::Slim ? 3 : 4; }
    // True when the pixels came from a file rather than from code.
    bool fromFile() const { return m_fromFile; }

    // Front faces, which is all a flat paper doll needs. Coordinates are
    // the same in the 64x32 and 64x64 layouts; only the divisor changes.
    Patch headFront() const { return patch(8, 8, 8, 8); }
    Patch bodyFront() const { return patch(20, 20, 8, 12); }
    Patch armFront() const { return patch(44, 20, armWidth(), 12); }
    Patch legFront() const { return patch(4, 20, 4, 12); }

    Patch patch(int x, int y, int w, int h) const
    {
        return Patch{ x / static_cast<float>(m_width), y / static_cast<float>(m_height),
                      (x + w) / static_cast<float>(m_width), (y + h) / static_cast<float>(m_height) };
    }

private:
    unsigned int m_texture = 0;
    Style m_style = Style::Classic;
    int m_variant = 0;
    int m_width = 64;
    int m_height = 32;
    bool m_fromFile = false;
    std::vector<uint8_t> m_pixels;

    bool loadFile(const std::string& path);
    void paint(int variant);
    void upload();
};
