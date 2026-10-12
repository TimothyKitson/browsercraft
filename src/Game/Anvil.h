#pragma once
#include "Enchanting.h"
#include "Items.h"
#include "World/Block.h"
#include <cstdint>

// The anvil: mending worn gear and moving enchantments between things.
//
// This is the other half of the enchanting table. The table puts a
// random set on a bare item for levels; the anvil is how you choose
// what ends up where -- a book onto a pickaxe, two half-worn swords
// into one whole one, an ingot into a chipped axe.
//
// Like the table, all of it is arithmetic over ids, wear and the packed
// enchantment word, so --selftest can run every pairing in the game
// without a window or a world.
namespace Anvil
{
    // One of the two slots. The count matters: three ingots mend more
    // than one does, and the result says how many it actually took.
    struct Input
    {
        StackId id = Blocks::Air;
        int count = 0;
        int damage = 0;
        Enchanting::Set enchantments = 0;
    };

    struct Result
    {
        bool valid = false;
        // Set when the pairing made sense but costs more than an anvil
        // will do. The UI shows the price and refuses the click, which
        // is the only way the player learns the ceiling exists.
        bool tooExpensive = false;

        StackId id = Blocks::Air;
        int damage = 0;
        Enchanting::Set enchantments = 0;

        int levelCost = 0;
        // How many of the right-hand stack it consumes. Always 1 except
        // when mending with raw material.
        int used = 0;
    };

    // Minecraft's ceiling, and the reason a tool cannot be built up
    // forever. Anything at or above this is refused.
    constexpr int TOO_EXPENSIVE = 40;

    // What one unit of the right material mends, as a share of the
    // item's full life.
    constexpr int REPAIR_PERCENT = 25;

    // What mending this item takes: planks for a wooden pickaxe, iron
    // for an iron one, leather for a leather cap. Air when nothing
    // mends it.
    StackId repairMaterial(StackId item);

    // How long the item lasts when new, or 0 for something that does
    // not wear out at all and so has nothing to mend.
    int maxDurability(StackId item);

    inline bool wearsOut(StackId item) { return maxDurability(item) > 0; }

    // An enchanted book: the one thing whose enchantments are the whole
    // point of it, and which the anvil empties onto something useful.
    inline bool isEnchantedBook(const Input& in)
    {
        return in.id == Items::Book && Enchanting::any(in.enchantments);
    }

    // What comes out, and what it costs. An invalid result means the
    // two do not go together and the output slot stays empty.
    Result combine(const Input& left, const Input& right);
}
