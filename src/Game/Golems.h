#pragma once
#include "World/Block.h"
#include "Entity/MobType.h"

// The one thing in the game you build rather than find. Minecraft
// makes both golems the same way -- a pumpkin on top of a body -- and
// the body is what says which golem you get.
//
// All of it is a question about four blocks, so --selftest can build
// every shape there is without a world to build it in.
namespace Golems
{
    // Where the blocks sit, as offsets from the pumpkin the player has
    // just put down. The body hangs below it; the arms are either side
    // of the upper body block, on whichever axis they were laid out.
    struct Shape
    {
        BlockId upper = Blocks::Air;    // one below the pumpkin
        BlockId lower = Blocks::Air;    // two below it
        BlockId armA = Blocks::Air;     // beside the upper block
        BlockId armB = Blocks::Air;     // and on its other side
    };

    // What this shape makes, or MobId::Count for nothing at all. Two
    // snow blocks under a pumpkin make a snow golem whatever is beside
    // them; iron wants the arms as well, which is what keeps a tower
    // of iron blocks from turning into a golem by accident.
    MobId madeBy(const Shape& shape);

    // Whether this block is the one that finishes a golem. Asked
    // before the arithmetic so the common case -- putting down a
    // plank -- costs nothing.
    inline bool isTrigger(BlockId placed) { return placed == Blocks::Pumpkin; }
}
