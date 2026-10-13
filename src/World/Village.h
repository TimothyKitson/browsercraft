#pragma once
#include <cstdint>
#include <vector>

// Where a village stands and what is in it.
//
// The layout is a pure function of the seed and the place, and nothing
// here touches a chunk. That matters more than usual: a village is
// wider than a chunk, so every chunk it reaches into works out the
// whole plan for itself and stamps only the part that lands in it. Two
// chunks that disagreed by one block would leave a seam through
// somebody's wall.
//
// It also means --selftest can lay out every village in a world and
// check none of them overlap without generating any terrain.
namespace Village
{
    // The world is cut into squares with at most one village to a
    // square, which is what stops two of them growing into each other.
    // A village reaches no further than MAX_RADIUS from its middle, so
    // a chunk need only look at the squares within that.
    constexpr int REGION = 352;
    constexpr int PLOT = 11;         // one building's share of the ground
    constexpr int GRID = 5;          // plots across, centred on the middle
    constexpr int MAX_RADIUS = (PLOT * GRID) / 2 + 2;

    static_assert(GRID % 2 == 1, "the village needs a middle plot to centre on");

    struct Site
    {
        bool exists = false;
        int x = 0;
        int z = 0;
    };

    // Whether this square has a village, and where its middle is. The
    // answer depends on nothing but the seed and the square, so every
    // chunk gets the same one.
    Site siteFor(int regionX, int regionZ, uint32_t seed);

    // Which square a world position falls in. Separate because the
    // division has to round towards negative infinity, and the plain
    // one does not.
    int regionOf(int world);

    enum class Kind : uint8_t
    {
        // Houses, which differ inside and in what their chest holds.
        // A village has at most one smith and one library; the rest
        // are homes and butchers.
        Home,
        Blacksmith,
        Library,
        Butcher,

        Lamp,
        // A fenced field of wheat with a water channel down it. Not a
        // house: nobody lives in it, so it has no bed and no chest.
        Farm,
    };

    inline bool isHouse(Kind kind)
    {
        return kind == Kind::Home || kind == Kind::Blacksmith ||
               kind == Kind::Library || kind == Kind::Butcher;
    }

    const char* name(Kind kind);

    struct Building
    {
        Kind kind = Kind::Home;
        int x = 0, z = 0;        // north-west corner, in world blocks
        int width = 0, depth = 0;
        // 0 north, 1 east, 2 south, 3 west -- which way the door looks,
        // which is always towards the middle of the village.
        int facing = 0;

        // One or two. A village of identical boxes reads as a housing
        // estate; Minecraft's have small, medium and large among them.
        int storeys = 1;
    };

    // Everything in the village, in world coordinates. Empty when the
    // site does not exist.
    std::vector<Building> plan(const Site& site, uint32_t seed);

    // How many beds the village has, which is how many people can
    // live in it: one to a house. Breeding counts against this.
    int bedCount(const Site& site, uint32_t seed);

    // Which building, if any, stands on this block. Used to work out
    // what a chest found in a village should hold, without storing
    // anything: the answer is the same every time it is asked.
    bool buildingAt(const Site& site, uint32_t seed, int x, int z, Building& out);

    // Whether any part of the village reaches into the given box.
    bool reaches(const Site& site, int minX, int minZ, int maxX, int maxZ);
}
