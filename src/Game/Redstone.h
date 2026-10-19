#pragma once
#include "World/Block.h"
#include <glm/glm.hpp>

// Power, and what carries it.
//
// Minecraft's dust carries a number from fifteen down to one and fades a
// step every block. This runs it as on or off with the same fifteen-block
// reach, because every id here is a block and a block with sixteen
// strengths is sixteen ids -- and nothing in this game yet asks how
// strong a signal is, only whether there is one.
//
// The rule knows nothing about the world. It reads and writes through
// Cells, so --selftest can lay a circuit out in a plain array and switch
// it on with no window, no chunks and no renderer.
namespace Redstone
{
    // How far dust carries before it runs out, which is Minecraft's.
    constexpr int MAX_RUN = 15;

    // How far either side of a change is reconsidered. A signal cannot
    // reach further than its run plus the block that sent it, so this is
    // the whole of what a change can possibly affect.
    constexpr int REACH = MAX_RUN + 2;

    struct Cells
    {
        virtual ~Cells() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
        virtual void set(int x, int y, int z, BlockId block) = 0;
    };

    // Gives power out for nothing: a torch, a block of redstone, a lever
    // thrown on, a button held down.
    bool isSource(BlockId id);

    // Carries it along the floor.
    bool isWire(BlockId id);

    // Answers to it: a lamp, a door.
    bool isSink(BlockId id);

    // The same block in its other state. Anything that is not two-faced
    // comes back unchanged.
    BlockId lit(BlockId id);
    BlockId unlit(BlockId id);

    // What a lever or a button does when it is used.
    BlockId toggled(BlockId id);

    // Recomputes every wire and every sink within REACH of (x, y, z) and
    // writes back whatever changed. Call it after anything is placed,
    // broken or switched.
    void update(Cells& cells, int x, int y, int z);

    // Whether this cell is powered right now: next to a source, next to
    // dust that is lit, or next to a block that a source is standing on.
    // What a sink asks.
    bool poweredAt(const Cells& cells, int x, int y, int z);

    // A solid block with a switch or a torch standing on it carries the
    // power into whatever is beside it. This is Minecraft's strong
    // power, cut down to the one case this game can make: everything
    // here sits on top of something rather than on the side of it.
    bool carries(const Cells& cells, int x, int y, int z);
}
