#pragma once
#include "Biomes.h"
#include <cstdint>

namespace BiomeColours
{
    uint32_t fromMap(bool foliage, float temperature, float downfall);
    uint32_t grass(Biomes::Kind kind, int x, int z);
    uint32_t foliage(Biomes::Kind kind);
    uint32_t water(Biomes::Kind kind);
    uint32_t average(const uint32_t* colours, int count);
    void reload();
    bool fromPack();
}
