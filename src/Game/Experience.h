#pragma once
#include "Items.h"
#include <vector>

// Levels, and the experience that buys them.
//
// Minecraft's curve is not one formula but three, bolted together at
// level 16 and level 32, and every one of its awkward numbers matters:
// the first sixteen levels come cheap, the next fifteen cost more each
// time, and past that the price climbs steeply enough that the last
// levels are a grind. Getting a bracket's constants wrong moves a level
// boundary by hundreds of points without ever looking wrong on screen,
// so all of it is pure arithmetic here and --selftest walks the whole
// curve: every level's cost, the running total, and the round trip back
// from a total to the level it buys.
namespace Experience
{
    // What it costs to go from this level to the next.
    int toNextLevel(int level);

    // Running total needed to have reached this level at all. Level 0
    // is free; the rest is the sum of toNextLevel below it, in closed
    // form so a level in the thousands costs no more to work out than
    // level one.
    int totalForLevel(int level);

    // The level a running total has bought, and how far along the bar
    // sits towards the next one, from 0 at the start to just under 1.
    int levelFor(int total);
    float progress(int total);

    // Experience arrives in lumps, not one point at a time: a kill
    // worth five drops as one orb of three and one of one and one more
    // of one. These are the sizes an orb comes in, largest first.
    extern const int ORB_VALUES[11];

    // The largest orb that fits in this much experience.
    int orbValue(int amount);

    // That much experience broken into the orbs it drops as.
    std::vector<int> splitIntoOrbs(int amount);

    // Mining gives experience for the ores whose drop is the ore
    // itself; the ones you have to smelt pay out at the furnace
    // instead, so coal and diamond pay here and iron and gold do not.
    // Inclusive, and equal when a block always gives the same.
    struct Range
    {
        int low = 0;
        int high = 0;
        bool any() const { return high > 0; }
    };

    Range forMining(StackId block);

    // What comes out of a furnace is worth a fraction of a point each,
    // which Minecraft accumulates and pays in whole points as it
    // crosses them. Scaled by a hundred so it stays in integers.
    int smeltingHundredths(StackId result);
}
