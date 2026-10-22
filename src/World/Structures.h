#pragma once
#include <cstdint>

// Where structures stand.
//
// Minecraft places almost everything it builds by one of three rules,
// and this is those three rules rather than a scheme of our own:
//
//   A grid. The world is cut into squares `spacing` chunks across, one
//   structure is tried in each, and it is placed somewhere in the first
//   (spacing - separation) chunks of its square. That last part is what
//   the separation is for: two structures in neighbouring squares can
//   never end up closer than `separation` chunks, however the dice
//   fall. Villages, temples, outposts, monuments, fortresses and the
//   trial chambers all go down this way.
//
//   A spread. A flat chance for every chunk, with nothing stopping two
//   from landing side by side. Mineshafts and buried treasure.
//
//   Rings. Used for one thing only: the strongholds sit on circles
//   round the origin, so there are always some to find and they are
//   never where a village is.
//
// The spacings and separations below are Minecraft's own, in chunks, so
// a world here is as crowded as a world there. The dice are not: Java's
// generator is not something to imitate from memory, so the rules are
// Minecraft's and the rolls are this engine's hash.
namespace Structures
{
    constexpr int CHUNK = 16;

    struct Grid
    {
        int spacing;        // chunks between one square and the next
        int separation;     // chunks no two of them may be closer than
        uint32_t salt;      // what keeps two kinds of structure apart
    };

    // Minecraft's own numbers, which is the whole point of the table.
    constexpr Grid VILLAGE        { 34, 8, 10387312u };
    constexpr Grid TRIAL_CHAMBER  { 34, 12, 94251327u };
    constexpr Grid DESERT_PYRAMID { 32, 8, 14357617u };
    constexpr Grid JUNGLE_TEMPLE  { 32, 8, 14357619u };
    constexpr Grid SWAMP_HUT      { 32, 8, 14357620u };
    constexpr Grid IGLOO          { 32, 8, 14357618u };
    constexpr Grid PILLAGER_POST  { 32, 8, 165745296u };
    constexpr Grid OCEAN_MONUMENT { 32, 5, 10387313u };
    constexpr Grid RUINED_PORTAL  { 40, 15, 34222645u };
    constexpr Grid SHIPWRECK      { 24, 4, 165745295u };
    constexpr Grid NETHER_FORTRESS{ 27, 4, 30084232u };
    constexpr Grid ANCIENT_CITY   { 24, 8, 20083232u };
    constexpr Grid WOODLAND_MANSION{ 80, 20, 10387319u };
    constexpr Grid END_CITY       { 20, 11, 10387313u };
    constexpr Grid BASTION        { 27, 4, 30084232u };

    // A chunk a structure of this kind stands in, if its square has one.
    struct Spot
    {
        bool exists = false;
        int chunkX = 0;
        int chunkZ = 0;
    };

    // Which square a chunk falls in. Rounds towards negative infinity,
    // because plain division rounds towards zero and that would make
    // the squares either side of the origin twice as wide.
    int squareOf(int chunk, int spacing);

    // The structure in this square, in chunks. Always exists: a grid
    // structure's rarity is its spacing, not a coin flip. Whether the
    // land there suits it is the caller's business -- that is how
    // Minecraft does it too, and why deserts have pyramids in them and
    // oceans do not.
    Spot inSquare(const Grid& grid, int squareX, int squareZ, uint32_t seed);

    // The nearest one to a place, searching out this many squares.
    Spot nearest(const Grid& grid, int blockX, int blockZ, uint32_t seed, int squares = 3);

    // Buried treasure is the one chest in the game you need a map to
    // find, and Minecraft places it by the spread rule rather than the
    // grid: a flat one-in-a-hundred for each chunk, which is why some
    // beaches have two and most have none.
    constexpr uint32_t BURIED_TREASURE_SALT = 10387320u;
    constexpr float    BURIED_TREASURE_CHANCE = 0.01f;

    // A desert well is not a structure in Minecraft at all -- it is a
    // decorator, one in a thousand chunks of desert, which is why you
    // can cross three deserts without seeing one. Same rule here.
    constexpr uint32_t DESERT_WELL_SALT = 10387321u;
    constexpr float    DESERT_WELL_CHANCE = 0.001f;

    // The other rule: a flat chance for each chunk on its own.
    bool spreadAt(int chunkX, int chunkZ, uint32_t seed, uint32_t salt, float chance);

    // And the strongholds, which are the exception to everything: a
    // fixed count on each of a few rings round the origin, spaced
    // evenly round the circle with a little wobble. There are always
    // some, they are always findable, and none of them is near spawn.
    constexpr int STRONGHOLD_RINGS = 3;
    constexpr int STRONGHOLD_IN_RING[STRONGHOLD_RINGS] = { 3, 6, 10 };
    constexpr int STRONGHOLD_RING_BLOCKS[STRONGHOLD_RINGS] = { 1408, 3328, 5248 };

    int strongholdCount();

    // Where the nth stronghold is, in blocks.
    void strongholdAt(int index, uint32_t seed, int& blockX, int& blockZ);

    // The one nearest a place, which is what an eye of ender needs to
    // know and what the debug screen reports.
    void nearestStronghold(int blockX, int blockZ, uint32_t seed, int& outX, int& outZ);
}
