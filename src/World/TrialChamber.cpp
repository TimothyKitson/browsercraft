#include "TrialChamber.h"
#include "Noise.h"
#include <algorithm>

int TrialChamber::regionOf(int world)
{
    const int chunk = (world >= 0) ? (world / Structures::CHUNK)
                                   : -(((-world) + Structures::CHUNK - 1) / Structures::CHUNK);
    return Structures::squareOf(chunk, Structures::TRIAL_CHAMBER.spacing);
}

TrialChamber::Site TrialChamber::siteFor(int regionX, int regionZ, uint32_t seed)
{
    const Structures::Spot spot =
        Structures::inSquare(Structures::TRIAL_CHAMBER, regionX, regionZ, seed);

    const float deep = hashToFloat(hashCoords(regionX, 214, regionZ, seed + 77004));

    Site site;
    site.exists = true;
    site.x = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
    site.z = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
    site.y = FLOOR_MIN + static_cast<int>(deep * static_cast<float>(FLOOR_MAX - FLOOR_MIN));
    return site;
}

bool TrialChamber::reaches(const Site& site, int minX, int minZ, int maxX, int maxZ)
{
    if (!site.exists) return false;
    return site.x + MAX_RADIUS >= minX && site.x - MAX_RADIUS <= maxX &&
           site.z + MAX_RADIUS >= minZ && site.z - MAX_RADIUS <= maxZ;
}

const char* TrialChamber::name(Kind kind)
{
    switch (kind)
    {
        case Kind::Entrance: return "Entrance";
        case Kind::Spawner:  return "Spawner room";
        case Kind::Vault:    return "Vault room";
        case Kind::Supply:   return "Supply room";
        case Kind::Empty:    break;
    }
    return "Empty room";
}

std::vector<TrialChamber::Room> TrialChamber::plan(const Site& site, uint32_t seed)
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
            room.width = ROOM - 2;   // the wall takes a block either side
            room.depth = ROOM - 2;
            room.x = site.x + gx * ROOM - room.width / 2;
            room.z = site.z + gz * ROOM - room.depth / 2;

            // The middle is the way in; the corners hold the vaults,
            // because what the keys open should be the furthest thing
            // from where you come in. Everything between is a fight or
            // a chest, settled by the seed so no two chambers have the
            // same shape to them.
            const bool middle = (gx == 0 && gz == 0);
            const bool corner = (gx != 0 && gz != 0);
            const float roll = hashToFloat(hashCoords(site.x + gx, 215, site.z + gz, seed + 77005));

            room.kind = middle   ? Kind::Entrance
                      : corner   ? Kind::Vault
                      : (roll < 0.72f) ? Kind::Spawner
                                       : Kind::Supply;

            rooms.push_back(room);
        }

    return rooms;
}

bool TrialChamber::roomAt(const Site& site, uint32_t seed, int x, int z, Room& out)
{
    for (const Room& room : plan(site, seed))
    {
        if (x < room.x || x >= room.x + room.width) continue;
        if (z < room.z || z >= room.z + room.depth) continue;
        out = room;
        return true;
    }
    return false;
}

int TrialChamber::spawnerCount(const Site& site, uint32_t seed)
{
    int count = 0;
    for (const Room& room : plan(site, seed))
        if (room.kind == Kind::Spawner) ++count;
    return count;
}

int TrialChamber::vaultCount(const Site& site, uint32_t seed)
{
    int count = 0;
    for (const Room& room : plan(site, seed))
        if (room.kind == Kind::Vault) ++count;
    return count;
}
