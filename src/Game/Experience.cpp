#include "Experience.h"
#include "RecipeBook.h"
#include <map>
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
        if (isBlockStack(block)) block = oreBase(static_cast<BlockId>(block));
        if (isBlockStack(block))
        {
            const BlockId b = static_cast<BlockId>(block);
            if (isSculkSensor(b) || isCalibratedSensor(b)) return { 1, 1 };
            if (b == Blocks::SculkCatalystBloom) return { 5, 5 };
        }
        switch (block)
        {
        case Blocks::MonsterSpawner: return { 15, 43 };
        case Blocks::Sculk: return { 1, 1 };
        case Blocks::SculkCatalyst: return { 5, 5 };
        case Blocks::SculkShrieker: return { 5, 5 };
        case Blocks::CoalOre:    return { 0, 2 };
        case Blocks::DiamondOre: return { 3, 7 };
        case Blocks::EmeraldOre: return { 3, 7 };
        case Blocks::NetherQuartzOre: return { 2, 5 };
        case Blocks::NetherGoldOre: return { 0, 1 };
        case Blocks::RedstoneOre: return { 1, 5 };
        case Blocks::LapisOre: return { 2, 5 };
        default: break;
        }
        return {};
    }

    int smeltingHundredths(StackId result)
    {
        static const std::pair<const char*, int> TABLE[] = {
            { "baked_potato", 35 }, { "brick", 30 }, { "charcoal", 15 }, { "coal", 10 }, { "cooked_beef", 35 },
            { "cooked_chicken", 35 }, { "cooked_cod", 35 }, { "cooked_mutton", 35 }, { "cooked_porkchop", 35 },
            { "cooked_rabbit", 35 }, { "cooked_salmon", 35 }, { "copper_ingot", 70 }, { "cracked_deepslate_bricks", 10 },
            { "cracked_deepslate_tiles", 10 }, { "cracked_nether_bricks", 10 }, { "cracked_polished_blackstone_bricks", 10 },
            { "cracked_stone_bricks", 10 }, { "deepslate", 10 }, { "diamond", 100 }, { "dried_kelp", 10 }, { "emerald", 100 },
            { "glass", 10 }, { "gold_ingot", 100 }, { "gold_nugget", 10 }, { "green_dye", 100 }, { "iron_ingot", 70 },
            { "iron_nugget", 10 }, { "lapis_lazuli", 20 }, { "lime_dye", 10 }, { "nether_brick", 10 }, { "netherite_scrap", 200 },
            { "popped_chorus_fruit", 10 }, { "quartz", 20 }, { "redstone", 70 }, { "smooth_basalt", 10 }, { "smooth_quartz", 10 },
            { "smooth_red_sandstone", 10 }, { "smooth_sandstone", 10 }, { "smooth_stone", 10 }, { "sponge", 15 }, { "stone", 10 },
            { "terracotta", 35 },
        };
        static std::map<StackId, int> byStack;
        if (byStack.empty())
            for (const auto& entry : TABLE)
            {
                const StackId id = RecipeBook::stackNamed(entry.first);
                if (id != Blocks::Air) byStack[id] = entry.second;
            }
        const auto found = byStack.find(result);
        if (found != byStack.end()) return found->second;
        if (isBlockStack(result) && isGlazedTerracotta(asBlock(result))) return 10;
        return 0;
    }
}
