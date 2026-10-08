#pragma once
#include <array>
#include <cstdint>

enum class GuiSprite : uint8_t
{
    HeartFull,
    HeartHalf,
    HeartContainer,
    HeartHardcoreFull,
    HeartHardcoreHalf,
    HeartHardcoreContainer,
    Hotbar,
    HotbarSelection,
    // Faint silhouettes shown in an empty armour slot.
    ArmorHelmet,
    ArmorChestplate,
    ArmorLeggings,
    ArmorBoots,
    Count
};

// HUD sprites, loaded from a resource pack's gui folder when one is
// installed. Anything missing is drawn procedurally instead, so the HUD
// always has real hearts even with no pack present.
class GuiTextures
{
public:
    GuiTextures();
    ~GuiTextures();

    GuiTextures(const GuiTextures&) = delete;
    GuiTextures& operator=(const GuiTextures&) = delete;

    unsigned int texture(GuiSprite sprite) const
    {
        return m_textures[static_cast<size_t>(sprite)];
    }

    bool fromPack(GuiSprite sprite) const
    {
        return m_fromPack[static_cast<size_t>(sprite)];
    }

private:
    std::array<unsigned int, static_cast<size_t>(GuiSprite::Count)> m_textures{};
    std::array<bool, static_cast<size_t>(GuiSprite::Count)> m_fromPack{};

    bool loadFromFile(GuiSprite sprite, const char* fileName);
    void createHeart(GuiSprite sprite, bool half, bool empty, bool hardcore);
    void createArmorIcon(GuiSprite sprite, int piece);
    void upload(GuiSprite sprite, const unsigned char* rgba, int width, int height);
};
