#include "GuiTextures.h"
#include "Core/GLFunctions.h"
#include <stb_image.h> // implementation lives in Atlas.cpp
#include <vector>
#include <string>
#include <cstdio>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

namespace
{
    const char* GUI_DIRECTORY = "assets/textures/gui";

    // Classic 9x9 Minecraft heart silhouette.
    //   0 = transparent, 1 = outline, 2 = body, 3 = highlight
    // Empty-armour-slot silhouettes, 12x12.
    //   0 = transparent, 1 = outline, 2 = body
    const char ARMOR[4][12][13] = {
        {   // helmet
            "...111111...",
            "..12222221..",
            ".1222222221.",
            "122222222221",
            "122222222221",
            "122222222221",
            "122111111221",
            "122......221",
            "122......221",
            "122......221",
            ".11......11.",
            "............",
        },
        {   // chestplate: two shoulder straps over a torso. The straps
            //  have to start full width or the icon reads as a bucket.
            "1221....1221",
            "1221....1221",
            "122122221221",
            "122222222221",
            "122222222221",
            "122222222221",
            "122222222221",
            "122222222221",
            "122222222221",
            ".1222222221.",
            "..11111111..",
            "............",
        },
        {   // leggings
            "111111111111",
            "122222222221",
            "122222222221",
            "122222222221",
            "122211112221",
            "1221....1221",
            "1221....1221",
            "1221....1221",
            "1221....1221",
            "1221....1221",
            "1111....1111",
            "............",
        },
        {   // boots
            "............",
            "............",
            "1221....1221",
            "1221....1221",
            "1221....1221",
            "1221....1221",
            "1221....1221",
            "1221....1221",
            "12221..12221",
            "12221..12221",
            "11111..11111",
            "............",
        },
    };

    const char DRUMSTICK[9][10] = {
        "....111..",
        "..113331.",
        ".13333331",
        ".13333331",
        ".11333331",
        "..1133311",
        "...11221.",
        "..12221..",
        "..1221..."
    };

    const char HEART[9][10] = {
        "..11.11..",
        ".1221221.",
        "132222221",
        "132222221",
        "122222221",
        ".12222221",
        "..122221.",
        "...1221..",
        "....11..."
    };
}

GuiTextures::GuiTextures()
{
    struct Entry
    {
        GuiSprite sprite;
        const char* file;
        bool half;
        bool empty;
        bool hardcore;
    };

    const Entry ENTRIES[] = {
        { GuiSprite::HeartFull,              "heart_full.png",              false, false, false },
        { GuiSprite::HeartHalf,              "heart_half.png",              true,  false, false },
        { GuiSprite::HeartContainer,         "heart_container.png",         false, true,  false },
        { GuiSprite::HeartHardcoreFull,      "heart_hardcore_full.png",     false, false, true  },
        { GuiSprite::HeartHardcoreHalf,      "heart_hardcore_half.png",     true,  false, true  },
        { GuiSprite::HeartHardcoreContainer, "heart_hardcore_container.png",false, true,  true  },
    };

    for (const Entry& entry : ENTRIES)
    {
        if (!loadFromFile(entry.sprite, entry.file))
            createHeart(entry.sprite, entry.half, entry.empty, entry.hardcore);
    }

    struct FoodEntry
    {
        GuiSprite sprite;
        const char* file;
        bool half;
        bool empty;
    };

    const FoodEntry FOOD[] = {
        { GuiSprite::FoodFull,      "food_full.png",  false, false },
        { GuiSprite::FoodHalf,      "food_half.png",  true,  false },
        { GuiSprite::FoodContainer, "food_empty.png", false, true  },
    };

    for (const FoodEntry& entry : FOOD)
    {
        if (!loadFromFile(entry.sprite, entry.file))
            createDrumstick(entry.sprite, entry.half, entry.empty);
    }

    // No procedural stand-in for these: the HUD falls back to flat panels.
    loadFromFile(GuiSprite::Hotbar, "hotbar.png");
    loadFromFile(GuiSprite::HotbarSelection, "hotbar_selection.png");

    // Armour slot ghosts. Vanilla packs name these files, so a pack that
    // has them wins; otherwise the silhouettes above are drawn.
    const char* ARMOR_FILES[4] = {
        "empty_armor_slot_helmet.png",
        "empty_armor_slot_chestplate.png",
        "empty_armor_slot_leggings.png",
        "empty_armor_slot_boots.png",
    };
    for (int piece = 0; piece < 4; ++piece)
    {
        const GuiSprite sprite =
            static_cast<GuiSprite>(static_cast<int>(GuiSprite::ArmorHelmet) + piece);
        if (!loadFromFile(sprite, ARMOR_FILES[piece]))
            createArmorIcon(sprite, piece);
    }
}

GuiTextures::~GuiTextures()
{
    for (unsigned int id : m_textures)
        if (id) glDeleteTextures(1, &id);
}

bool GuiTextures::loadFromFile(GuiSprite sprite, const char* fileName)
{
    const std::string path = std::string(GUI_DIRECTORY) + "/" + fileName;

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) return false;

    upload(sprite, data, width, height);
    stbi_image_free(data);
    m_fromPack[static_cast<size_t>(sprite)] = true;
    return true;
}

void GuiTextures::createArmorIcon(GuiSprite sprite, int piece)
{
    constexpr int SIZE = 12;
    std::vector<unsigned char> pixels(SIZE * SIZE * 4, 0);

    // Grey and semi-transparent: it has to read as "nothing here yet",
    // not as an item sitting in the slot.
    struct Colour { unsigned char r, g, b, a; };
    const Colour outline{ 52, 53, 58, 190 };
    const Colour body{ 112, 114, 122, 170 };

    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
        {
            const char cell = ARMOR[piece][y][x];
            if (cell == '.') continue;

            const Colour colour = (cell == '1') ? outline : body;
            const size_t index = (static_cast<size_t>(y) * SIZE + x) * 4;
            pixels[index + 0] = colour.r;
            pixels[index + 1] = colour.g;
            pixels[index + 2] = colour.b;
            pixels[index + 3] = colour.a;
        }

    upload(sprite, pixels.data(), SIZE, SIZE);
}

void GuiTextures::createDrumstick(GuiSprite sprite, bool half, bool empty)
{
    constexpr int SIZE = 9;
    std::vector<unsigned char> pixels(SIZE * SIZE * 4, 0);

    struct Colour { unsigned char r, g, b, a; };
    const Colour outline{ 20, 12, 12, 255 };
    const Colour meat = empty ? Colour{ 62, 62, 62, 255 } : Colour{ 150, 86, 48, 255 };
    const Colour highlight = empty ? Colour{ 92, 92, 92, 255 } : Colour{ 196, 128, 72, 255 };
    const Colour bone = empty ? Colour{ 78, 78, 78, 255 } : Colour{ 228, 220, 196, 255 };

    for (int y = 0; y < SIZE; ++y)
    {
        for (int x = 0; x < SIZE; ++x)
        {
            const char cell = DRUMSTICK[y][x];
            if (cell == '.') continue;

            Colour colour = outline;
            if (cell == '2') colour = bone;
            else if (cell == '3') colour = meat;

            if (cell == '3' && x + y < 7) colour = highlight;
            if (half && x > SIZE / 2) continue;

            const size_t i = (static_cast<size_t>(y) * SIZE + x) * 4;
            pixels[i] = colour.r;
            pixels[i + 1] = colour.g;
            pixels[i + 2] = colour.b;
            pixels[i + 3] = colour.a;
        }
    }

    upload(sprite, pixels.data(), SIZE, SIZE);
}

void GuiTextures::createHeart(GuiSprite sprite, bool half, bool empty, bool hardcore)
{
    constexpr int SIZE = 9;
    std::vector<unsigned char> pixels(SIZE * SIZE * 4, 0);

    struct Colour { unsigned char r, g, b, a; };
    const Colour outline{ 20, 12, 12, 255 };
    const Colour body = empty ? Colour{ 62, 62, 62, 255 }
                              : (hardcore ? Colour{ 140, 40, 24, 255 } : Colour{ 214, 36, 36, 255 });
    const Colour highlight = empty ? Colour{ 92, 92, 92, 255 }
                                   : (hardcore ? Colour{ 196, 86, 58, 255 } : Colour{ 255, 128, 128, 255 });
    const Colour hollow{ 46, 46, 46, 255 };

    for (int y = 0; y < SIZE; ++y)
    {
        for (int x = 0; x < SIZE; ++x)
        {
            const char cell = HEART[y][x];
            if (cell == '.') continue;

            Colour colour = outline;
            if (cell == '2') colour = body;
            else if (cell == '3') colour = highlight;

            // A half heart keeps its left side filled and hollows the right.
            if (half && x > 4 && cell != '1') colour = hollow;

            const size_t index = (static_cast<size_t>(y) * SIZE + x) * 4;
            pixels[index + 0] = colour.r;
            pixels[index + 1] = colour.g;
            pixels[index + 2] = colour.b;
            pixels[index + 3] = colour.a;
        }
    }

    upload(sprite, pixels.data(), SIZE, SIZE);
}

void GuiTextures::upload(GuiSprite sprite, const unsigned char* rgba, int width, int height)
{
    unsigned int& id = m_textures[static_cast<size_t>(sprite)];
    if (id == 0) glGenTextures(1, &id);

    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Hearts are pixel art: keep the edges hard.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, 0);
}
