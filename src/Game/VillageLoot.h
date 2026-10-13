#pragma once
#include "Items.h"
#include "World/Block.h"
#include "World/Village.h"
#include <vector>

// What is in the chest in a village building.
//
// Rolled from where the chest stands rather than stored, so a chest
// holds the same thing however many times it is found, two players in
// the same world find the same loot in it, and nothing goes into the
// save until somebody actually takes something out.
//
// It is arithmetic over a position and a seed, so --selftest can open
// every chest in a village without generating one.
namespace VillageLoot
{
    struct Stack
    {
        StackId id = Blocks::Air;
        int count = 0;
    };

    // How many different things a chest holds. The spread is the point:
    // a chest that always held four of everything would be a vending
    // machine rather than a find.
    constexpr int MIN_STACKS = 2;
    constexpr int MAX_STACKS = 6;

    // What this kind of building keeps in its chest. Empty for anything
    // that is not a house.
    std::vector<Stack> rollFor(Village::Kind kind, int x, int y, int z, uint32_t seed);
}
