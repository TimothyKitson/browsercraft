#include "GuiTextures.h"
#include "Core/GLFunctions.h"
#include <stb_image.h> // implementation lives in Atlas.cpp
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <algorithm>

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

    // The little shield that marks a point of armour.
    const char PLATE[9][10] = {
        ".1111111.",
        "113333311",
        "133333331",
        "133333331",
        "133333331",
        "133333331",
        ".13333331",
        "..133331.",
        "...1331.."
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
        { GuiSprite::HeartFull,              "sprites/hud/heart/full|heart_full.png",              false, false, false },
        { GuiSprite::HeartHalf,              "sprites/hud/heart/half|heart_half.png",              true,  false, false },
        { GuiSprite::HeartContainer,         "sprites/hud/heart/container|heart_container.png",         false, true,  false },
        { GuiSprite::HeartHardcoreFull,      "sprites/hud/heart/hardcore_full|heart_hardcore_full.png",     false, false, true  },
        { GuiSprite::HeartHardcoreHalf,      "sprites/hud/heart/hardcore_half|heart_hardcore_half.png",     true,  false, true  },
        { GuiSprite::HeartHardcoreContainer, "sprites/hud/heart/container_hardcore|heart_hardcore_container.png",false, true,  true  },
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
        { GuiSprite::FoodFull,      "sprites/hud/food_full|food_full.png",  false, false },
        { GuiSprite::FoodHalf,      "sprites/hud/food_half|food_half.png",  true,  false },
        { GuiSprite::FoodContainer, "sprites/hud/food_empty|food_empty.png", false, true  },
    };

    for (const FoodEntry& entry : FOOD)
    {
        if (!loadFromFile(entry.sprite, entry.file))
            createDrumstick(entry.sprite, entry.half, entry.empty);
    }

    // No procedural stand-in for these: the HUD falls back to flat panels.
    loadFromFile(GuiSprite::Hotbar, "sprites/hud/hotbar|hotbar.png");
    loadFromFile(GuiSprite::HotbarSelection, "sprites/hud/hotbar_selection|hotbar_selection.png");

    // The armour row above the hearts.
    struct PlateEntry
    {
        GuiSprite sprite;
        const char* file;
        bool half;
        bool empty;
    };

    const PlateEntry PLATES[] = {
        { GuiSprite::ArmorFull,  "sprites/hud/armor_full|armor_full.png",  false, false },
        { GuiSprite::ArmorHalf,  "sprites/hud/armor_half|armor_half.png",  true,  false },
        { GuiSprite::ArmorEmpty, "sprites/hud/armor_empty|armor_empty.png", false, true  },
    };

    for (const PlateEntry& entry : PLATES)
    {
        if (!loadFromFile(entry.sprite, entry.file))
            createArmorPlate(entry.sprite, entry.half, entry.empty);
    }

    // Armour slot ghosts. Vanilla packs name these files, so a pack that
    // has them wins; otherwise the silhouettes above are drawn.
    const char* ARMOR_FILES[4] = {
        "sprites/container/slot/helmet|empty_armor_slot_helmet.png",
        "sprites/container/slot/chestplate|empty_armor_slot_chestplate.png",
        "sprites/container/slot/leggings|empty_armor_slot_leggings.png",
        "sprites/container/slot/boots|empty_armor_slot_boots.png",
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
    for (auto& entry : m_images)
        if (entry.second.texture) glDeleteTextures(1, &entry.second.texture);
}

bool GuiTextures::loadFromFile(GuiSprite sprite, const char* fileNames)
{
    const std::string names(fileNames);
    const size_t bar = names.find('|');
    const std::string vanilla = bar == std::string::npos ? std::string() : names.substr(0, bar);
    const std::string flat = bar == std::string::npos ? names : names.substr(bar + 1);

    int width = 0, height = 0, channels = 0;
    unsigned char* data = nullptr;
    if (!vanilla.empty())
    {
        data = stbi_load((std::string(GUI_DIRECTORY) + "/" + vanilla + ".png").c_str(), &width, &height, &channels, 4);
    }
    if (!data)
        data = stbi_load((std::string(GUI_DIRECTORY) + "/" + flat).c_str(), &width, &height, &channels, 4);
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

void GuiTextures::createArmorPlate(GuiSprite sprite, bool half, bool empty)
{
    constexpr int SIZE = 9;
    std::vector<unsigned char> pixels(SIZE * SIZE * 4, 0);

    struct Colour { unsigned char r, g, b, a; };
    const Colour outline{ 18, 18, 22, 255 };
    const Colour face = empty ? Colour{ 58, 58, 62, 200 } : Colour{ 206, 206, 214, 255 };

    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
        {
            const char cell = PLATE[y][x];
            if (cell == '.') continue;

            // The half plate is the left side of the shield only, the
            // same way a half heart is.
            if (half && !empty && x > SIZE / 2) continue;

            const Colour colour = (cell == '1') ? outline : face;
            const size_t i = (static_cast<size_t>(y) * SIZE + x) * 4;
            pixels[i] = colour.r;
            pixels[i + 1] = colour.g;
            pixels[i + 2] = colour.b;
            pixels[i + 3] = colour.a;
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

const GuiImage* GuiTextures::image(const std::string& path)
{
    auto found = m_images.find(path);
    if (found != m_images.end()) return found->second.texture ? &found->second : nullptr;

    GuiImage& entry = m_images[path];
    const std::string file = std::string(GUI_DIRECTORY) + "/" + path + ".png";
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(file.c_str(), &width, &height, &channels, 4);
    if (!data) return nullptr;

    std::vector<unsigned char> pixels(data, data + static_cast<size_t>(width) * height * 4);
    stbi_image_free(data);
    if (path.find("vignette") != std::string::npos)
    {
        for (size_t i = 0; i + 3 < pixels.size(); i += 4)
        {
            const int light = (pixels[i] * 3 + pixels[i + 1] * 4 + pixels[i + 2]) / 8;
            pixels[i] = pixels[i + 1] = pixels[i + 2] = 0;
            pixels[i + 3] = static_cast<unsigned char>(light);
        }
    }
    for (size_t i = 0; i + 3 < pixels.size(); i += 4)
        if (pixels[i + 3] == 0) pixels[i] = pixels[i + 1] = pixels[i + 2] = 0;

    entry.width = width;
    entry.height = height;
    readMeta(file + ".mcmeta", entry);

    if (entry.frames > 1 && height >= width * entry.frames)
    {
        height = width;
        pixels.resize(static_cast<size_t>(width) * height * 4);
        entry.height = height;
    }

    int uploadW = width, uploadH = height;
    while ((uploadW > 1024 || uploadH > 1024) && uploadW % 2 == 0 && uploadH % 2 == 0)
    {
        const int halfW = uploadW / 2, halfH = uploadH / 2;
        std::vector<unsigned char> half(static_cast<size_t>(halfW) * halfH * 4);
        for (int y = 0; y < halfH; ++y)
            for (int x = 0; x < halfW; ++x)
                for (int c = 0; c < 4; ++c)
                {
                    int sum = 0;
                    for (int dy = 0; dy < 2; ++dy)
                        for (int dx = 0; dx < 2; ++dx)
                            sum += pixels[((static_cast<size_t>(y) * 2 + dy) * uploadW + x * 2 + dx) * 4 + c];
                    half[(static_cast<size_t>(y) * halfW + x) * 4 + c] = static_cast<unsigned char>(sum / 4);
                }
        pixels.swap(half);
        uploadW = halfW;
        uploadH = halfH;
    }

    const bool smooth = uploadW >= 512 || uploadH >= 512 ||
                        (entry.sliceWidth > 0 && uploadW > entry.sliceWidth * 2);

    glGenTextures(1, &entry.texture);
    glBindTexture(GL_TEXTURE_2D, entry.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, uploadW, uploadH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    if (smooth && entry.width > 256 * 2)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return &entry;
}

bool GuiTextures::has(const std::string& path)
{
    return image(path) != nullptr;
}

void GuiTextures::readMeta(const std::string& file, GuiImage& entry)
{
    std::FILE* in = std::fopen(file.c_str(), "rb");
    if (!in) return;
    std::string text;
    char buffer[512];
    size_t got = 0;
    while ((got = std::fread(buffer, 1, sizeof(buffer), in)) > 0) text.append(buffer, got);
    std::fclose(in);

    auto number = [&](const char* key, size_t from, int fallback) {
        const size_t at = text.find(std::string("\"") + key + "\"", from);
        if (at == std::string::npos) return fallback;
        const size_t colon = text.find(':', at);
        if (colon == std::string::npos) return fallback;
        return std::atoi(text.c_str() + colon + 1);
    };

    const size_t scaling = text.find("\"scaling\"");
    if (scaling != std::string::npos)
    {
        if (text.find("nine_slice", scaling) != std::string::npos)
        {
            entry.sliceWidth = number("width", scaling, 0);
            entry.sliceHeight = number("height", scaling, 0);
            const size_t border = text.find("\"border\"", scaling);
            if (border != std::string::npos)
            {
                const size_t colon = text.find(':', border);
                size_t at = colon + 1;
                while (at < text.size() && (text[at] == ' ' || text[at] == '\n' || text[at] == '\r' || text[at] == '\t')) ++at;
                if (at < text.size() && text[at] == '{')
                {
                    entry.borderLeft = number("left", at, 0);
                    entry.borderTop = number("top", at, 0);
                    entry.borderRight = number("right", at, 0);
                    entry.borderBottom = number("bottom", at, 0);
                }
                else
                {
                    const int all = std::atoi(text.c_str() + colon + 1);
                    entry.borderLeft = entry.borderTop = entry.borderRight = entry.borderBottom = all;
                }
            }
        }
        else if (text.find("stretch", scaling) == std::string::npos && text.find("tile", scaling) != std::string::npos)
        {
            entry.tiled = true;
            entry.sliceWidth = number("width", scaling, 0);
            entry.sliceHeight = number("height", scaling, 0);
        }
    }

    if (text.find("\"animation\"") != std::string::npos && entry.width > 0)
        entry.frames = std::max(1, entry.height / entry.width);
}
