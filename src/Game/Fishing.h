#pragma once
#include "Items.h"
#include "Player/Inventory.h"
#include <cstdint>

namespace Fishing
{
    enum class Category { Fish, Junk, Treasure };

    struct Catch
    {
        ItemStack stack;
        Category category = Category::Fish;
    };

    constexpr int ROD_DURABILITY = 64;
    constexpr int CARROT_STICK_DURABILITY = 25;
    constexpr int FUNGUS_STICK_DURABILITY = 100;
    constexpr int BOOST_COST = 7;
    constexpr int FUNGUS_BOOST_COST = 1;

    int durabilityOf(StackId id);
    Catch roll(uint32_t seed, bool openWater, bool jungle);
    float waitSeconds(uint32_t seed, bool openSky, bool raining);
    float approachSeconds(uint32_t seed);
    float biteSeconds(uint32_t seed);
    int experienceFor(uint32_t seed);
}
