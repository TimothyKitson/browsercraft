#pragma once
#include "Items.h"
#include "World/Block.h"
#include <cstdint>
#include <vector>

namespace BlockDrops
{
    struct Drop
    {
        StackId id = Blocks::Air;
        int count = 0;
    };

    struct Breaker
    {
        StackId tool = Blocks::Air;
        bool silk = false;
        int fortune = 0;
    };

    bool handles(BlockId id);
    std::vector<Drop> roll(BlockId id, const Breaker& breaker, uint32_t seed);

    int oreDrops(int count, int fortune, uint32_t& state);
    int uniformBonus(int count, int fortune, int multiplier, uint32_t& state);
    int binomialBonus(int count, int fortune, int extra, float probability, uint32_t& state);
}
