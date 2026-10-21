#include "Stronghold.h"
#include "Noise.h"
#include <algorithm>
#include <cmath>

Stronghold::Site Stronghold::byIndex(int index, uint32_t seed)
{
    Site site;
    if (index < 0 || index >= Structures::strongholdCount()) return site;

    int x = 0, z = 0;
    Structures::strongholdAt(index, seed, x, z);

    site.exists = true;
    site.x = x;
    site.z = z;
    site.y = FLOOR_MIN +
             static_cast<int>(hashToFloat(hashCoords(x, 17, z, seed + 66001)) *
                              static_cast<float>(FLOOR_MAX - FLOOR_MIN));
    return site;
}

Stronghold::Site Stronghold::nearest(int blockX, int blockZ, uint32_t seed)
{
    Site best;
    long long bestDistance = 0;

    for (int i = 0; i < Structures::strongholdCount(); ++i)
    {
        const Site site = byIndex(i, seed);
        const long long dx = static_cast<long long>(site.x) - blockX;
        const long long dz = static_cast<long long>(site.z) - blockZ;
        const long long distance = dx * dx + dz * dz;

        if (!best.exists || distance < bestDistance)
        {
            best = site;
            bestDistance = distance;
        }
    }

    return best;
}

bool Stronghold::reaches(const Site& site, int minX, int minZ, int maxX, int maxZ)
{
    if (!site.exists) return false;
    return site.x + MAX_RADIUS >= minX && site.x - MAX_RADIUS <= maxX &&
           site.z + MAX_RADIUS >= minZ && site.z - MAX_RADIUS <= maxZ;
}

Stronghold::Site Stronghold::touching(int minX, int minZ, int maxX, int maxZ, uint32_t seed)
{
    for (int i = 0; i < Structures::strongholdCount(); ++i)
    {
        const Site site = byIndex(i, seed);
        if (reaches(site, minX, minZ, maxX, maxZ)) return site;
    }
    return Site{};
}

const char* Stronghold::name(Kind kind)
{
    switch (kind)
    {
        case Kind::Portal:  return "Portal room";
        case Kind::Library: return "Library";
        case Kind::Store:   return "Store room";
        case Kind::Hall:    break;
    }
    return "Hall";
}

std::vector<Stronghold::Room> Stronghold::plan(const Site& site, uint32_t seed)
{
    std::vector<Room> rooms;
    if (!site.exists) return rooms;

    constexpr int HALF = GRID / 2;

    for (int gz = -HALF; gz <= HALF; ++gz)
        for (int gx = -HALF; gx <= HALF; ++gx)
        {
            Room room;
            room.gx = gx;
            room.gz = gz;
            room.width = ROOM - 2;
            room.depth = ROOM - 2;
            room.x = site.x + gx * ROOM - room.width / 2;
            room.z = site.z + gz * ROOM - room.depth / 2;

            // The portal is in the middle, because it is the thing the
            // stronghold is for and the thing you should walk into
            // rather than hunt for. A library in one corner, stores in
            // the others, halls between.
            const bool middle = (gx == 0 && gz == 0);
            const bool corner = (gx != 0 && gz != 0);
            const float roll =
                hashToFloat(hashCoords(site.x + gx, 18, site.z + gz, seed + 66002));

            room.kind = middle ? Kind::Portal
                      : corner ? (roll < 0.34f ? Kind::Library : Kind::Store)
                               : Kind::Hall;

            rooms.push_back(room);
        }

    return rooms;
}

bool Stronghold::eyeInFrame(const Site& site, uint32_t seed, int frameIndex)
{
    return hashToFloat(hashCoords(site.x + frameIndex, 19, site.z, seed + 66003)) < 0.22f;
}

int Stronghold::eyesAlreadySet(const Site& site, uint32_t seed)
{
    int count = 0;
    for (int i = 0; i < 12; ++i)
        if (eyeInFrame(site, seed, i)) ++count;
    return count;
}
