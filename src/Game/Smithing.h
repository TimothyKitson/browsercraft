#pragma once
#include "Items.h"
#include "Player/Inventory.h"
#include <cstdint>
#include <vector>

namespace Smithing
{
    constexpr int TRIM_PATTERNS = 18;
    constexpr int TRIM_MATERIALS = 10;
    constexpr int TRIM_SHIFT = 24;

    struct Result
    {
        StackId id = Blocks::Air;
        int damage = 0;
        uint32_t enchantments = 0;
        bool valid() const { return id != Blocks::Air; }
    };

    Result combine(const ItemStack& pattern, const ItemStack& base, const ItemStack& addition);

    int patternOfTemplate(StackId id);
    int materialOf(StackId id);
    bool trimmable(StackId id);

    int trimPattern(uint32_t enchantments);
    int trimMaterial(uint32_t enchantments);
    uint32_t withTrim(uint32_t enchantments, int pattern, int material);

    const char* patternName(int pattern);
    unsigned int materialColour(int material);

    std::vector<StackId> everyOutput();
}
