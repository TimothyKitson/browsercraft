#pragma once
#include "World/Block.h"

// Where sugar cane may stand, and when it grows.
//
// The rules read several blocks around the one in question -- the
// ground beneath, the four columns beside that ground, the stack above
// -- so rather than take a World they take the smallest interface that
// answers them, the way Portal and Projectiles do. --selftest can then
// build a waterside out of nothing and check that cane stands where it
// should and nowhere else.
namespace Cane
{
    // Three is as tall as it gets, as in Minecraft.
    constexpr int MAX_HEIGHT = 3;

    struct Blocks
    {
        virtual ~Blocks() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
    };

    // Whether that ground could have cane planted on it: sand, dirt or
    // grass with water in one of the four columns beside it. The water
    // has to be level with the ground, not the cane, which is why this
    // takes the ground and not the cane's own cell.
    bool groundIsWatered(const Blocks& blocks, int x, int y, int z);

    // Whether a cane may occupy this cell: standing on watered ground,
    // or on another cane with room left in the stack.
    bool canStandAt(const Blocks& blocks, int x, int y, int z);

    // How tall the stack of cane ending at this cell is, counting this
    // one. Zero if there is no cane here.
    int stackHeight(const Blocks& blocks, int x, int y, int z);

    // Whether the cane here would put out another above it if ticked.
    bool canGrowAt(const Blocks& blocks, int x, int y, int z);
}
