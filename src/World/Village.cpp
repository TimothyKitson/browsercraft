#include "Village.h"
#include "Noise.h"
#include <algorithm>

namespace
{
    // One in so many squares has a village. Sparse enough that finding
    // one is worth something, close enough that walking turns one up.
    constexpr float VILLAGE_CHANCE = 0.62f;
    constexpr float HOUSE_CHANCE = 0.55f;
}

int Village::regionOf(int world)
{
    // Rounds towards negative infinity: plain division rounds towards
    // zero, which would make the squares either side of the origin
    // twice as wide as the rest and put two villages in one of them.
    return (world >= 0) ? (world / REGION) : -(((-world) + REGION - 1) / REGION);
}

Village::Site Village::siteFor(int regionX, int regionZ, uint32_t seed)
{
    Site site;
    if (hashToFloat(hashCoords(regionX, 101, regionZ, seed + 55001)) >= VILLAGE_CHANCE)
        return site;

    // Somewhere inside the square, but never so near an edge that the
    // village could spill into the next one and meet its neighbour.
    // The margin is MAX_RADIUS and a little: two sites pushed as close
    // as the squares allow would otherwise sit exactly 2 * MAX_RADIUS
    // apart, and their roofs overhang a block past that.
    constexpr int MARGIN = MAX_RADIUS + 2;
    const int room = REGION - MARGIN * 2;
    const float alongX = hashToFloat(hashCoords(regionX, 102, regionZ, seed + 55002));
    const float alongZ = hashToFloat(hashCoords(regionX, 103, regionZ, seed + 55003));

    site.exists = true;
    site.x = regionX * REGION + MARGIN + static_cast<int>(alongX * static_cast<float>(room));
    site.z = regionZ * REGION + MARGIN + static_cast<int>(alongZ * static_cast<float>(room));
    return site;
}

bool Village::reaches(const Site& site, int minX, int minZ, int maxX, int maxZ)
{
    if (!site.exists) return false;
    return site.x + MAX_RADIUS >= minX && site.x - MAX_RADIUS <= maxX &&
           site.z + MAX_RADIUS >= minZ && site.z - MAX_RADIUS <= maxZ;
}

std::vector<Village::Building> Village::plan(const Site& site, uint32_t seed)
{
    std::vector<Building> out;
    if (!site.exists) return out;

    const int half = GRID / 2;

    for (int gz = -half; gz <= half; ++gz)
    {
        for (int gx = -half; gx <= half; ++gx)
        {
            // The middle plot is the well, which is what the rest of
            // the village is arranged around.
            if (gx == 0 && gz == 0)
            {
                Building well;
                well.kind = Kind::Well;
                well.width = well.depth = 5;
                well.x = site.x - 2;
                well.z = site.z - 2;
                out.push_back(well);
                continue;
            }

            const uint32_t roll = hashCoords(site.x + gx, 7, site.z + gz, seed + 55010);
            if (hashToFloat(roll) >= HOUSE_CHANCE)
            {
                // An empty plot gets a lamp at its corner often enough
                // that the paths between houses are lit.
                if (hashToFloat(hashCoords(site.x + gx, 9, site.z + gz, seed + 55011)) < 0.35f)
                {
                    Building lamp;
                    lamp.kind = Kind::Lamp;
                    lamp.width = lamp.depth = 1;
                    lamp.x = site.x + gx * PLOT;
                    lamp.z = site.z + gz * PLOT;
                    out.push_back(lamp);
                }
                continue;
            }

            // Houses are inset inside their plot, so the gap between
            // two of them is always at least the inset twice over.
            // That gap is the village's paths: nothing has to lay them.
            Building house;
            house.kind = Kind::Home;
            house.width = 7;
            house.depth = (hashToFloat(hashCoords(site.x + gx, 11, site.z + gz,
                                                  seed + 55012)) < 0.5f) ? 7 : 6;

            const int plotX = site.x + gx * PLOT - PLOT / 2;
            const int plotZ = site.z + gz * PLOT - PLOT / 2;
            house.x = plotX + (PLOT - house.width) / 2;
            house.z = plotZ + (PLOT - house.depth) / 2;

            // The door looks towards the middle, along whichever axis
            // the house is further out on, so every door faces a path.
            if (std::abs(gx) >= std::abs(gz)) house.facing = (gx > 0) ? 3 : 1;
            else                              house.facing = (gz > 0) ? 0 : 2;

            out.push_back(house);
        }
    }

    // One smith and one library at most, picked from the houses that
    // were laid out rather than from a fixed plot -- a village whose
    // smithy was always in the north-west corner would read as a
    // stamped copy the second time you found one.
    std::vector<size_t> houses;
    for (size_t i = 0; i < out.size(); ++i)
        if (isHouse(out[i].kind)) houses.push_back(i);

    size_t smith = out.size();
    if (houses.size() >= 2 &&
        hashToFloat(hashCoords(site.x, 21, site.z, seed + 55020)) < 0.70f)
    {
        smith = houses[hashCoords(site.x, 22, site.z, seed + 55021) % houses.size()];
        out[smith].kind = Kind::Blacksmith;
    }

    if (houses.size() >= 3 &&
        hashToFloat(hashCoords(site.x, 23, site.z, seed + 55022)) < 0.60f)
    {
        // Walked forward from a drawn spot until it lands on a house
        // that is still a home, so the library never takes the smithy's
        // plot and the choice stays spread across the village.
        const size_t from = hashCoords(site.x, 24, site.z, seed + 55023) % houses.size();
        for (size_t step = 0; step < houses.size(); ++step)
        {
            const size_t candidate = houses[(from + step) % houses.size()];
            if (candidate == smith) continue;
            out[candidate].kind = Kind::Library;
            break;
        }
    }

    // A butcher here and there among what is left.
    for (size_t index : houses)
    {
        if (out[index].kind != Kind::Home) continue;
        if (hashToFloat(hashCoords(out[index].x, 25, out[index].z, seed + 55024)) < 0.25f)
            out[index].kind = Kind::Butcher;
    }

    return out;
}

const char* Village::name(Kind kind)
{
    switch (kind)
    {
        case Kind::Home:       return "House";
        case Kind::Blacksmith: return "Blacksmith";
        case Kind::Library:    return "Library";
        case Kind::Butcher:    return "Butcher";
        case Kind::Well:       return "Well";
        case Kind::Lamp:       return "Lamp";
    }
    return "";
}

bool Village::buildingAt(const Site& site, uint32_t seed, int x, int z, Building& out)
{
    for (const Building& building : plan(site, seed))
    {
        if (x < building.x || x >= building.x + building.width) continue;
        if (z < building.z || z >= building.z + building.depth) continue;
        out = building;
        return true;
    }
    return false;
}
