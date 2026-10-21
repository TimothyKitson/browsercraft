#pragma once
#include "Structures.h"
#include <cstdint>
#include <vector>

// Where a trial chamber is and what is in it.
//
// The same arrangement as a village, for the same reason: the layout is
// a pure function of the seed and the place, nothing here touches a
// chunk, and every chunk the chamber reaches into works out the whole
// plan for itself and stamps only its own part. A chamber is wider than
// a chunk, and two chunks disagreeing by a block would leave a seam
// through a wall.
//
// It is also why --selftest can lay out every chamber in a world and
// check none of them overlap without generating any terrain.
namespace TrialChamber
{
    // Minecraft's own grid for them: thirty-four chunks to a square
    // with twelve of separation, so a chamber is as far from the next
    // one as it is there. REGION is that in blocks.
    constexpr int REGION = Structures::TRIAL_CHAMBER.spacing * Structures::CHUNK;

    constexpr int ROOM = 11;     // a room and the wall round it
    constexpr int GRID = 3;      // rooms across, centred on the middle
    constexpr int MAX_RADIUS = (ROOM * GRID) / 2 + 2;

    // Inside the room: the floor, the air above it, and the ceiling.
    constexpr int HEADROOM = 5;

    // How deep the floor sits. Deep enough to be underground anywhere
    // in the world, shallow enough that the ores around it are worth
    // the walk.
    constexpr int FLOOR_MIN = 16;
    constexpr int FLOOR_MAX = 34;

    static_assert(GRID % 2 == 1, "the chamber needs a middle room to centre on");

    struct Site
    {
        bool exists = false;
        int x = 0;
        int z = 0;
        int y = 0;       // the floor of every room in it
    };

    // Whether this square has a chamber, and where its middle is. The
    // answer depends on nothing but the seed and the square.
    Site siteFor(int regionX, int regionZ, uint32_t seed);

    // Which square a world position falls in. Separate because the
    // division has to round towards negative infinity.
    int regionOf(int world);

    enum class Kind : uint8_t
    {
        // The way in, under a shaft to the surface.
        Entrance,
        // A room with a trial spawner in the middle of it.
        Spawner,
        // A room with vaults along one wall, which is what the keys
        // the spawners give out are for.
        Vault,
        // A room with a chest in it and nothing to fight.
        Supply,
        // Floor and walls and nothing else.
        Empty,
    };

    const char* name(Kind kind);

    struct Room
    {
        Kind kind = Kind::Empty;
        int x = 0, z = 0;        // north-west corner of the floor
        int width = 0, depth = 0;

        // Where it sits on the grid, counted from the middle. The
        // stamp needs this to know which of its walls have a room on
        // the other side and so want a doorway through them.
        int gx = 0, gz = 0;
    };

    // Every room, in world coordinates. Empty when the site does not
    // exist. Rooms are laid on a grid and joined by doorways cut
    // through the walls between them.
    std::vector<Room> plan(const Site& site, uint32_t seed);

    // Which room, if any, covers this block.
    bool roomAt(const Site& site, uint32_t seed, int x, int z, Room& out);

    // Whether any part of the chamber reaches into the given box.
    bool reaches(const Site& site, int minX, int minZ, int maxX, int maxZ);

    // How many spawners and how many vaults the plan holds. A chamber
    // with no spawner is a cellar, and one with no vault has nothing
    // for the keys they give out.
    int spawnerCount(const Site& site, uint32_t seed);
    int vaultCount(const Site& site, uint32_t seed);
}
