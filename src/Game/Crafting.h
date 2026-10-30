#pragma once
#include "World/Block.h"
#include "Items.h"
#include <vector>

struct ItemStack;

// What a crafting grid currently produces.
struct CraftOutput
{
    StackId id = Blocks::Air;
    int count = 0;

    bool valid() const { return id != Blocks::Air && count > 0; }
};

namespace Crafting
{
    // `grid` is always 9 entries (a 3x3 array); `size` says whether only
    // the top-left 2x2 of it is in play. Shaped recipes are matched after
    // trimming empty rows and columns, so a recipe works wherever you put
    // it in the grid, exactly as in Minecraft.
    CraftOutput match(const ItemStack* grid, int size);

    // Removes one of each ingredient used. Call after taking the result.
    void consume(ItemStack* grid, int size);

    // Everything the recipe book can make, as a flat list. The audit
    // needs this: without it there is no way to tell a block that is
    // crafted from one that nothing in the game can produce at all.
    std::vector<StackId> everyOutput();
}
