#include "Experience.h"
#include "Items.h"
#include "Food.h"
#include <algorithm>

namespace Experience
{
    const int ORB_VALUES[11] = { 2477, 1237, 617, 307, 149, 73, 37, 17, 7, 3, 1 };

    int toNextLevel(int level)
    {
        if (level < 0) return 0;
        if (level < 16) return 2 * level + 7;
        if (level < 31) return 5 * level - 38;
        return 9 * level - 158;
    }

    // The closed forms are the sums of the three brackets above. They
    // are written with halves cleared out -- 2.5L^2 - 40.5L + 360
    // becomes (5L^2 - 81L + 720) / 2 -- so the arithmetic stays exact
    // in integers rather than drifting a point at a time through
    // floating point.
    int totalForLevel(int level)
    {
        if (level <= 0) return 0;
        if (level <= 16) return level * level + 6 * level;
        if (level <= 31) return (5 * level * level - 81 * level + 720) / 2;
        return (9 * level * level - 325 * level + 4440) / 2;
    }

    int levelFor(int total)
    {
        if (total <= 0) return 0;

        // Walk up a level at a time rather than inverting the
        // quadratics: the inverse needs a square root, and a root that
        // lands a hair under a whole number reports the level below the
        // one actually earned. Levels are small enough that stepping is
        // not worth outsmarting.
        int level = 0;
        int spent = 0;
        while (spent + toNextLevel(level) <= total)
        {
            spent += toNextLevel(level);
            ++level;
        }
        return level;
    }

    float progress(int total)
    {
        if (total <= 0) return 0.0f;
        const int level = levelFor(total);
        const int into = total - totalForLevel(level);
        const int cost = toNextLevel(level);
        if (cost <= 0) return 0.0f;
        return static_cast<float>(into) / static_cast<float>(cost);
    }

    int orbValue(int amount)
    {
        for (int value : ORB_VALUES)
            if (value <= amount) return value;
        return 0;
    }

    std::vector<int> splitIntoOrbs(int amount)
    {
        std::vector<int> orbs;
        int left = std::max(0, amount);
        while (left > 0)
        {
            const int value = orbValue(left);
            if (value <= 0) break;
            orbs.push_back(value);
            left -= value;
        }
        return orbs;
    }

    Range forMining(StackId block)
    {
        switch (block)
        {
        case Blocks::CoalOre:    return { 0, 2 };
        case Blocks::DiamondOre: return { 3, 7 };
        case Blocks::EmeraldOre: return { 3, 7 };
        default: break;
        }
        return {};
    }

    int smeltingHundredths(StackId result)
    {
        switch (result)
        {
        case Items::IronIngot:    return 70;
        case Items::GoldIngot:    return 100;
        case Items::Diamond:      return 100;
        case Items::Coal:         return 10;
        case Blocks::Glass:       return 10;
        case Blocks::Stone:       return 10;
        default: break;
        }

        // Anything else worth smelting is food, and all cooked food
        // pays the same in Minecraft.
        if (Food::valueOf(result).hunger > 0) return 35;
        return 0;
    }
}
