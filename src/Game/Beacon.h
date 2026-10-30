#pragma once
#include "World/Block.h"

// The beacon's pyramid, and what it is worth.
//
// Minecraft counts the complete layers of a pyramid under a beacon and
// gives you an effect whose reach and strength come off that count. A
// beacon on the ground does nothing at all, which is the whole reason
// the pyramid is interesting: it is the one build in the game whose
// only purpose is to be large.
//
// Pure arithmetic over a function that answers "what block is at this
// point", so --selftest can measure a pyramid without a world.
namespace Beacon
{
    // The most layers a pyramid can usefully have, which is Minecraft's.
    constexpr int MAX_TIERS = 4;

    // How far the effect reaches at each tier, in blocks.
    constexpr int RANGE_BASE = 20;
    constexpr int RANGE_PER_TIER = 10;

    // Whatever can answer what is where. The beacon code asks nothing
    // else of the world.
    struct Ground
    {
        virtual ~Ground() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
    };

    // How many complete layers sit under a beacon at this point.
    //
    // Layer n is a solid square of beacon-grade blocks, 2n+1 across,
    // n blocks below the beacon. A layer with one block missing does
    // not count, and nor does anything above it -- which is why a
    // half-built pyramid is worth exactly as much as no pyramid.
    int tiersUnder(const Ground& ground, int x, int y, int z);

    // How far it reaches. Zero tiers is zero reach: a beacon standing
    // on nothing is a lamp.
    int rangeFor(int tiers);

    // Whether a beacon here can see the sky. Minecraft wants a clear
    // column above it, which is what stops one working in a cellar.
    bool seesSky(const Ground& ground, int x, int y, int z, int worldTop);
}
