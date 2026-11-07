#pragma once
#include "Game/Items.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

class ArmourTextures
{
public:
    ~ArmourTextures();

    unsigned int layer(StackId piece, bool leggings);
    unsigned int leatherOverlay(bool leggings);
    unsigned int trim(int pattern, int material, StackId armour, bool leggings);
    unsigned int trimIcon(int slot, int material);

    static const char* tierName(StackId id);
    static unsigned int fallbackColour(StackId id);
    static bool swap(std::vector<uint8_t>& rgba, const std::vector<uint8_t>& key, const std::vector<uint8_t>& palette);

private:
    unsigned int upload(const std::vector<uint8_t>& rgba, int width, int height);
    bool loadRgba(const std::string& path, std::vector<uint8_t>& rgba, int& width, int& height);
    bool palette(int material, bool darker, std::vector<uint8_t>& out);

    std::map<std::string, unsigned int> m_cache;
};
