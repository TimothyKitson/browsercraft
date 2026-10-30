#pragma once
#include "Items.h"

// The composter: what goes in it, how likely each thing is to raise
// the level, and what comes out at the top.
//
// Pure arithmetic over an item id and a level, so --selftest can fill
// one a hundred times and count what it paid without a world to stand
// it in.
namespace Composting
{
    // What a full composter pays, and how many levels there are. Eight
    // is full; emptying it takes it back to nothing.
    constexpr int BONE_MEAL = 1;

    // The chance one of these raises the level, 0 if it will not go in
    // at all. Minecraft's own four tiers: leaves and seeds are poor,
    // crops are fair, bread and a cooked meal are good, and a cake is
    // certain. Nothing here is a meat or a mineral.
    float chanceOf(StackId id);

    inline bool composts(StackId id) { return chanceOf(id) > 0.0f; }

    // Whether this throw raises the level. `roll` is any number; the
    // caller supplies the dice so this stays pure.
    bool raises(StackId id, uint32_t roll);
}
