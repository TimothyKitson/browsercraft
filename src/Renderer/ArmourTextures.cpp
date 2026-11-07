#include "ArmourTextures.h"
#include "Core/GLFunctions.h"
#include "Game/Armour.h"
#include "Game/Smithing.h"
#include <stb_image.h>
#include <algorithm>
#include <cstdlib>

ArmourTextures::~ArmourTextures()
{
    for (const auto& entry : m_cache)
        if (entry.second) glDeleteTextures(1, &entry.second);
}

const char* ArmourTextures::tierName(StackId id)
{
    if (id == Items::TurtleHelmet) return "turtle";
    switch (Armour::tierOf(id))
    {
        case Armour::Tier::Leather: return "leather";
        case Armour::Tier::Gold: return "gold";
        case Armour::Tier::Iron: return "iron";
        case Armour::Tier::Diamond: return "diamond";
        case Armour::Tier::Netherite: return "netherite";
        case Armour::Tier::Chainmail: return "chainmail";
        default: return nullptr;
    }
}

unsigned int ArmourTextures::fallbackColour(StackId id)
{
    if (id == Items::TurtleHelmet) return 0x47BF4Au;
    static const unsigned int COLOURS[] = { 0xA06540u, 0xF4D23Cu, 0xD8D8D8u, 0x5DECE0u, 0x4A4144u, 0x8C8C8Cu };
    return COLOURS[std::clamp(static_cast<int>(Armour::tierOf(id)) - 1, 0, 5)];
}

unsigned int ArmourTextures::upload(const std::vector<uint8_t>& rgba, int width, int height)
{
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}

bool ArmourTextures::loadRgba(const std::string& path, std::vector<uint8_t>& rgba, int& width, int& height)
{
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) return false;
    rgba.assign(data, data + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    stbi_image_free(data);
    return true;
}

bool ArmourTextures::swap(std::vector<uint8_t>& rgba, const std::vector<uint8_t>& key, const std::vector<uint8_t>& palette)
{
    if (key.size() < 8 * 4 || palette.size() < 8 * 4) return false;
    for (size_t i = 0; i + 3 < rgba.size(); i += 4)
    {
        if (rgba[i + 3] == 0) continue;
        int best = 0, bestDistance = 1 << 30;
        for (int k = 0; k < 8; ++k)
        {
            const int d = std::abs(static_cast<int>(rgba[i]) - static_cast<int>(key[static_cast<size_t>(k * 4)])) +
                          std::abs(static_cast<int>(rgba[i + 1]) - static_cast<int>(key[static_cast<size_t>(k * 4 + 1)])) +
                          std::abs(static_cast<int>(rgba[i + 2]) - static_cast<int>(key[static_cast<size_t>(k * 4 + 2)]));
            if (d < bestDistance)
            {
                bestDistance = d;
                best = k;
            }
        }
        rgba[i] = palette[static_cast<size_t>(best * 4)];
        rgba[i + 1] = palette[static_cast<size_t>(best * 4 + 1)];
        rgba[i + 2] = palette[static_cast<size_t>(best * 4 + 2)];
    }
    return true;
}

bool ArmourTextures::palette(int material, bool darker, std::vector<uint8_t>& out)
{
    int w = 0, h = 0;
    if (loadRgba(Smithing::palettePath(material, darker), out, w, h) && w >= 8) return true;
    const unsigned int c = Smithing::materialColour(material);
    out.assign(8 * 4, 255);
    for (int k = 0; k < 8; ++k)
    {
        const float shade = 1.15f - static_cast<float>(k) * 0.1f - (darker ? 0.15f : 0.0f);
        out[static_cast<size_t>(k * 4)] = static_cast<uint8_t>(std::clamp(static_cast<float>((c >> 16) & 255u) * shade, 0.0f, 255.0f));
        out[static_cast<size_t>(k * 4 + 1)] = static_cast<uint8_t>(std::clamp(static_cast<float>((c >> 8) & 255u) * shade, 0.0f, 255.0f));
        out[static_cast<size_t>(k * 4 + 2)] = static_cast<uint8_t>(std::clamp(static_cast<float>(c & 255u) * shade, 0.0f, 255.0f));
    }
    return true;
}

unsigned int ArmourTextures::layer(StackId piece, bool leggings)
{
    const char* tier = tierName(piece);
    if (!tier) return 0;
    const std::string name = std::string(tier) + (leggings ? "_layer_2" : "_layer_1");
    const auto found = m_cache.find(name);
    if (found != m_cache.end()) return found->second;
    std::vector<uint8_t> rgba;
    int w = 0, h = 0;
    if (!loadRgba("assets/textures/models/armor/" + name + ".png", rgba, w, h))
    {
        w = 64;
        h = 32;
        const unsigned int c = fallbackColour(piece);
        rgba.assign(static_cast<size_t>(w * h * 4), 255);
        for (size_t i = 0; i < rgba.size(); i += 4)
        {
            rgba[i] = static_cast<uint8_t>((c >> 16) & 255u);
            rgba[i + 1] = static_cast<uint8_t>((c >> 8) & 255u);
            rgba[i + 2] = static_cast<uint8_t>(c & 255u);
        }
    }
    else if (Armour::tierOf(piece) == Armour::Tier::Leather)
    {
        for (size_t i = 0; i < rgba.size(); i += 4)
        {
            rgba[i] = static_cast<uint8_t>(rgba[i] * 0xA0 / 255);
            rgba[i + 1] = static_cast<uint8_t>(rgba[i + 1] * 0x65 / 255);
            rgba[i + 2] = static_cast<uint8_t>(rgba[i + 2] * 0x40 / 255);
        }
    }
    return m_cache[name] = upload(rgba, w, h);
}

unsigned int ArmourTextures::leatherOverlay(bool leggings)
{
    const std::string name = leggings ? "leather_layer_2_overlay" : "leather_layer_1_overlay";
    const auto found = m_cache.find(name);
    if (found != m_cache.end()) return found->second;
    std::vector<uint8_t> rgba;
    int w = 0, h = 0;
    if (!loadRgba("assets/textures/models/armor/" + name + ".png", rgba, w, h)) return m_cache[name] = 0;
    return m_cache[name] = upload(rgba, w, h);
}

unsigned int ArmourTextures::trim(int pattern, int material, StackId armour, bool leggings)
{
    if (pattern < 0 || material < 0) return 0;
    const bool darker = Smithing::darkerOn(material, armour);
    const std::string key = "trim:" + std::to_string(pattern) + ":" + std::to_string(material) + (darker ? "d" : "") + (leggings ? "l" : "");
    const auto found = m_cache.find(key);
    if (found != m_cache.end()) return found->second;
    std::vector<uint8_t> rgba, keys, colours;
    int w = 0, h = 0, kw = 0, kh = 0;
    if (!loadRgba(Smithing::trimPath(pattern, leggings), rgba, w, h)) return m_cache[key] = 0;
    if (!loadRgba(Smithing::palettePath(-1, false), keys, kw, kh))
    {
        keys.assign(8 * 4, 255);
        for (int k = 0; k < 8; ++k)
            keys[static_cast<size_t>(k * 4)] = keys[static_cast<size_t>(k * 4 + 1)] = keys[static_cast<size_t>(k * 4 + 2)] =
                static_cast<uint8_t>(224 - k * 32);
    }
    palette(material, darker, colours);
    swap(rgba, keys, colours);
    return m_cache[key] = upload(rgba, w, h);
}

unsigned int ArmourTextures::trimIcon(int slot, int material)
{
    if (slot < 0 || slot > 3 || material < 0) return 0;
    const std::string key = "icon:" + std::to_string(slot) + ":" + std::to_string(material);
    const auto found = m_cache.find(key);
    if (found != m_cache.end()) return found->second;
    std::vector<uint8_t> rgba, keys, colours;
    int w = 0, h = 0, kw = 0, kh = 0;
    if (!loadRgba(Smithing::trimIconPath(slot), rgba, w, h)) return m_cache[key] = 0;
    if (!loadRgba(Smithing::palettePath(-1, false), keys, kw, kh)) return m_cache[key] = 0;
    palette(material, false, colours);
    swap(rgba, keys, colours);
    return m_cache[key] = upload(rgba, w, h);
}
