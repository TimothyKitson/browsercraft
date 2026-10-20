#include "TrialChamber.h"
#include "Noise.h"
#include <algorithm>

namespace
{
    // Rarer than a village: one square in three has one, and a square
    // is bigger. A chamber is meant to be a trip rather than a thing
    // you stumble into on the way somewhere else.
    constexpr float CHAMBER_CHANCE = 0.34f;
}

int TrialChamber::regionOf(int world)
{
    return (world >= 0) ? (world / REGION) : -(((-world) + REGION - 1) / REGION);
}

TrialChamber::Site TrialChamber::siteFor(int regionX, int regionZ, uint32_t seed)
{
    Site site;
    if (hashToFloat(hashCoords(regionX, 211, regionZ, seed + 77001)) >= CHAMBER_CHANCE)
        return site;

    // Never near an edge, so two chambers in neighbouring squares
    // cannot grow into each other.
    constexpr int MARGIN = MAX_RADIUS + 2;
    const int room = REGION - MARGIN * 2;
    const float alongX = hashToFloat(hashCoords(regionX, 212, regionZ, seed + 77002));
    const float alongZ = hashToFloat(hashCoords(regionX, 213, regionZ, seed + 77003));
    const float deep = hashToFloat(hashCoords(regionX, 214, regionZ, seed + 77004));

    site.exists = true;
    site.x = regionX * REGION + MARGIN + static_cast<int>(alongX * static_cast<float>(room));
    site.z = regionZ * REGION + MARGIN + static_cast<int>(alongZ * static_cast<float>(room));
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
