#pragma once
#include "Items.h"
#include "World/Block.h"
#include <cstdint>

namespace Archaeology
{
    enum Table : int
    {
        DesertPyramid = 0,
        DesertWell,
        WarmOceanRuin,
        TrailCommon,
        TrailRare,
        ColdOceanRuin,
        TableCount
    };

    constexpr int SHERD_COUNT = 23;
    constexpr float SECONDS_PER_STAGE = 0.5f;
    constexpr int BRUSH_DURABILITY = 64;

    StackId sherd(int index);
    int sherdIndex(StackId id);
    const char* sherdPattern(int index);

    struct Find
    {
        StackId id = Blocks::Air;
        int count = 0;
    };
    Find roll(int table, int x, int y, int z, uint32_t seed);

    BlockId settled(BlockId suspicious);

    uint32_t packPot(const int sides[4]);
    int potSide(uint32_t packed, int side);
}
