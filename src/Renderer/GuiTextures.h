#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

enum class GuiSprite : uint8_t
{
    HeartFull,
    HeartHalf,
    HeartContainer,
    HeartHardcoreFull,
    HeartHardcoreHalf,
    HeartHardcoreContainer,
    FoodFull,
    FoodHalf,
    FoodContainer,
    Hotbar,
    HotbarSelection,
    // The armour row above the hearts.
    ArmorFull,
    ArmorHalf,
    ArmorEmpty,
    // Faint silhouettes shown in an empty armour slot.
    ArmorHelmet,
    ArmorChestplate,
    ArmorLeggings,
    ArmorBoots,
    Count
};

struct GuiImage
{
    unsigned int texture = 0;
    int width = 0;
    int height = 0;
    int frames = 1;
    int sliceWidth = 0;
    int sliceHeight = 0;
    int borderLeft = 0;
    int borderTop = 0;
    int borderRight = 0;
    int borderBottom = 0;
    bool tiled = false;
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

    const GuiImage* image(const std::string& path);
    bool has(const std::string& path);

private:
    std::unordered_map<std::string, GuiImage> m_images;
    void readMeta(const std::string& file, GuiImage& entry);
    std::array<unsigned int, static_cast<size_t>(GuiSprite::Count)> m_textures{};
    std::array<bool, static_cast<size_t>(GuiSprite::Count)> m_fromPack{};

    bool loadFromFile(GuiSprite sprite, const char* fileNames);
    void createHeart(GuiSprite sprite, bool half, bool empty, bool hardcore);
    void createDrumstick(GuiSprite sprite, bool half, bool empty);
    void createArmorPlate(GuiSprite sprite, bool half, bool empty);
    void createArmorIcon(GuiSprite sprite, int piece);
    void upload(GuiSprite sprite, const unsigned char* rgba, int width, int height);
};
