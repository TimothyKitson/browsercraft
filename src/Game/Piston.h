#pragma once
#include "Redstone.h"
#include "World/Block.h"

// What a piston can push and what happens when it does.
//
// It reads and writes through Redstone::Cells, the same as the power
// rule, so a piston can be driven in --selftest with no world: lay the
// blocks out in a map, switch it on, and look at what moved.
namespace Piston
{
    // How long a line it will shift. Minecraft's is twelve.
    constexpr int MAX_PUSH = 12;

    // Whether a block can be shoved along. Bedrock and the things a
    // chamber is locked with stay where they are; so does another
    // piston with its head out, which would otherwise be torn in two.
    bool movable(BlockId id);

    // Whether a block is simply not there as far as pushing goes: air
    // and anything a pushed block would crush.
    bool replaceable(BlockId id);

    // How many blocks stand in front of this piston, counting from the
    // cell its head would go into. -1 when the line cannot be pushed at
    // all -- too long, or stopped by something immovable.
    int lineLength(const Redstone::Cells& cells, int x, int y, int z, int facing);

    // Push. Moves the line one along, puts the head in front and marks
    // the body as out. Returns false, and changes nothing, when the
    // line will not go.
    bool extend(Redstone::Cells& cells, int x, int y, int z);

    // Pull the head back in. A sticky piston brings whatever was
    // against its head with it; a plain one leaves it where it is.
    bool retract(Redstone::Cells& cells, int x, int y, int z);

    // One step along a facing, which is the only geometry in here.
    void step(int facing, int& dx, int& dz);
}
