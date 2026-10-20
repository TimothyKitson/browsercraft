#pragma once
#include "World/Block.h"
#include <cstdint>

// Copper going green.
//
// The one thing in this world that changes on its own after it has been
// put down. The rule is arithmetic over a block, a position and a clock,
// so --selftest can age a wall a thousand years in a loop.
namespace Copper
{
    // How often a block gets the chance, and how likely it is to take
    // it. Minecraft's is a random tick with a long tail; this is the
    // same shape with numbers that mean a wall built at dawn has gone
    // patchy by dusk and green in a few days, rather than in a year
    // nobody will play.
    constexpr float AGE_INTERVAL = 8.0f;
    constexpr float AGE_CHANCE = 0.08f;

    // The next stage along, or the same block when there is nowhere
    // further to go. Oxidised is the end of it.
    BlockId next(BlockId id);

    // Whether this block has anywhere further to go.
    bool ages(BlockId id);

    // Whether this one takes its step now. Rolled from where it stands
    // and which tick it is, so two blocks laid together do not weather
    // in lockstep.
    bool ticks(int x, int y, int z, uint32_t seed, int tick);
}
