#pragma once
#include "Items.h"
#include "VillageLoot.h"
#include "World/Block.h"
#include "World/WorldGen.h"
#include <vector>

// What is in a chest that was generated rather than placed.
//
// The village already had one of these; this is the same idea for
// everything else the generator leaves a chest in. Rolled from where
// the chest stands rather than stored, so it holds the same thing
// however many times the chunk round it is thrown away and remade, two
// players in the same world find the same loot in it, and nothing goes
// into the save until somebody actually takes something out.
//
// The tables are Minecraft's in character rather than to the item:
// a pyramid is gold and bones and a thing you did not want, buried
// treasure is the one that is always worth the dig, and a shipwreck is
// somebody's supplies. Matching Minecraft's weights exactly would mean
// copying a file this engine has no equivalent of.
namespace StructureLoot
{
    using Stack = VillageLoot::Stack;

    constexpr int MIN_STACKS = 2;
    constexpr int MAX_STACKS = 6;

    // Empty for Hoard::None, which is a chest the player built.
    std::vector<Stack> rollFor(WorldGen::Hoard hoard, int x, int y, int z, uint32_t seed);

    // What this kind of place is known for: the one thing a chest of
    // this kind always holds, so a pyramid is never a disappointment
    // and buried treasure is never a joke.
    Stack centrepiece(WorldGen::Hoard hoard, int x, int y, int z, uint32_t seed);
}
