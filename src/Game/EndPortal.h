#pragma once
#include "World/Block.h"

// The way into the End: twelve frames in a ring, an eye set in each,
// and the three-by-three inside them becomes the portal.
//
// The rule is a shape in the world rather than a property of one block,
// so it takes the smallest interface that can answer questions about
// the cells around it -- the same arrangement Portal and Cane use -- and
// --selftest can lay out a ring, leave a gap in it, and check it stays
// shut.
namespace EndPortal
{
    // The ring is the twelve cells round a three-by-three, which is to
    // say the five-by-five square with its own corners left out.
    constexpr int INTERIOR = 3;
    constexpr int FRAME_COUNT = 12;

    struct Blocks
    {
        virtual ~Blocks() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
    };

    // Whether every one of the twelve frames round this centre is there
    // and has an eye in it.
    bool ringComplete(const Blocks& blocks, int centreX, int y, int centreZ);

    // Looks for a complete ring whose frame includes the cell given --
    // which is what an eye has just been placed into. Writes the centre
    // and returns true when it finds one.
    bool findRingAround(const Blocks& blocks, int x, int y, int z,
                        int& outCentreX, int& outCentreZ);

    // The twelve frame positions round a centre, as offsets. Exposed so
    // the test and the builder agree on what the shape is rather than
    // each having its own idea of it.
    void frameOffsets(int out[FRAME_COUNT][2]);
}
