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
        case Kind::Fountain: return "Fountain room";
        case Kind::Cells:   return "Prison cells";
        case Kind::Stairs:  return "Stairwell";
        case Kind::Corridor: return "Corridor";
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

            static const Kind LAYOUT[GRID][GRID] = {
                { Kind::Library,  Kind::Corridor, Kind::Store,    Kind::Corridor, Kind::Cells },
                { Kind::Corridor, Kind::Fountain, Kind::Corridor, Kind::Hall,     Kind::Corridor },
                { Kind::Stairs,   Kind::Corridor, Kind::Portal,   Kind::Corridor, Kind::Store },
                { Kind::Corridor, Kind::Hall,     Kind::Corridor, Kind::Cells,    Kind::Corridor },
                { Kind::Store,    Kind::Corridor, Kind::Hall,     Kind::Corridor, Kind::Library },
            };
            const int turn = static_cast<int>(hashCoords(site.x, 18, site.z, seed + 66002) & 3u);
            int rx = gx, rz = gz;
            for (int i = 0; i < turn; ++i)
            {
                const int was = rx;
                rx = -rz;
                rz = was;
            }
            room.kind = LAYOUT[rz + HALF][rx + HALF];

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
