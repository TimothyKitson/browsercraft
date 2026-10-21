#pragma once
#include "Structures.h"
#include <cstdint>
#include <vector>

// The stronghold, and the end portal in it.
//
// Minecraft places these on rings round the origin rather than on the
// grid everything else uses, for one reason: an eye of ender has to be
// able to point at one from anywhere, and a structure that might simply
// not exist nearby cannot be pointed at. So there are always some, they
// are always findable, and none of them is on top of spawn.
//
// The layout is a pure function of the site and the seed, like the
// village and the chamber, so every chunk works out the whole plan for
// itself and stamps its own part of it.
namespace Stronghold
{
    constexpr int ROOM = 9;      // a room and the wall round it
    constexpr int GRID = 3;      // rooms across
    constexpr int MAX_RADIUS = (ROOM * GRID) / 2 + 2;
    constexpr int HEADROOM = 5;

    // Deep, but not so deep that the way down is a mine shaft. It is
    // the one structure the game expects you to tunnel to.
    constexpr int FLOOR_MIN = 12;
    constexpr int FLOOR_MAX = 30;

    struct Site
    {
        bool exists = false;
        int x = 0;
        int z = 0;
        int y = 0;
    };

    // The nth stronghold in the world, and the one nearest a place.
    Site byIndex(int index, uint32_t seed);
    Site nearest(int blockX, int blockZ, uint32_t seed);

    // Whether any stronghold reaches into this box, and which one.
    bool reaches(const Site& site, int minX, int minZ, int maxX, int maxZ);
    Site touching(int minX, int minZ, int maxX, int maxZ, uint32_t seed);

    enum class Kind : uint8_t
    {
        // The room the whole thing exists for: a ring of twelve frames
        // over a pool of lava, with some of the eyes already in it.
        Portal,
        // Bookshelves, which is where a stronghold's loot is.
        Library,
        // A room with a chest in it.
        Store,
        // Floor, walls, and whatever wandered in.
        Hall,
    };

    const char* name(Kind kind);

    struct Room
    {
        Kind kind = Kind::Hall;
        int x = 0, z = 0;
        int width = 0, depth = 0;
        int gx = 0, gz = 0;
    };

    std::vector<Room> plan(const Site& site, uint32_t seed);

    // How many frames of the twelve already hold an eye. Minecraft
    // leaves some in and some out, so finishing the portal still costs
    // pearls -- and the count is rolled from the site, so the same
    // stronghold is the same job every time it is found.
    int eyesAlreadySet(const Site& site, uint32_t seed);
    bool eyeInFrame(const Site& site, uint32_t seed, int frameIndex);
}
