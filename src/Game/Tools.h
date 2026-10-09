#pragma once
#include "Items.h"
#include "World/Block.h"

// Tools: what they are made of, what they are for, how fast they work
// and how long they last.
//
// Everything here is a pure function of an item id and a block id, so
// --selftest can mine the whole block table with every tool in the game
// and never open a window. Application keeps the timer; this decides
// what the timer should count to.
namespace Tools
{
    enum class Kind { None, Pickaxe, Axe, Shovel, Sword, Hoe };

    // In Minecraft's order, which is also the order of their harvest
    // levels apart from gold -- gold is fast and fragile and mines no
    // more than wood can.
    enum class Tier { None, Wood, Stone, Iron, Gold, Diamond };

    Kind kindOf(StackId id);
    Tier tierOf(StackId id);

    inline bool isTool(StackId id) { return kindOf(id) != Kind::None; }

    // How many blocks it breaks before it is used up.
    int maxDurability(Tier tier);

    // What a tier can get a drop out of: stone wants wood or better,
    // iron and gold ore want stone, diamond wants iron, obsidian wants
    // diamond. Bare hands are level zero.
    int harvestLevel(Tier tier);
    int requiredLevel(BlockId block);

    // Whether mining `block` with `tool` leaves anything behind. Blocks
    // that need no particular tool always drop.
    bool canHarvest(StackId tool, BlockId block);

    // Seconds to break `block` holding `tool`. The bare hand is the same
    // as holding nothing, which is what Blocks::Air means here.
    float breakSeconds(StackId tool, BlockId block);

    // Whether using the tool on this block should cost it a point of
    // durability: breaking any block does, but swinging at air does not.
    bool wearsOnBlock(StackId tool, BlockId block);

    // The damage a mob takes from being hit with it. A sword hits
    // hardest, an axe nearly as hard, a bare hand barely at all.
    int attackDamage(StackId tool);
}
