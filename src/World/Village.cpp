#include "Village.h"
#include "Noise.h"
#include <algorithm>

namespace
{
    constexpr float HOUSE_CHANCE = 0.55f;
}

int Village::regionOf(int world)
{
    const int chunk = (world >= 0) ? (world / Structures::CHUNK)
                                   : -(((-world) + Structures::CHUNK - 1) / Structures::CHUNK);
    return Structures::squareOf(chunk, Structures::VILLAGE.spacing);
}

Village::Site Village::siteFor(int regionX, int regionZ, uint32_t seed)
{
    // One to a square, in the middle of whichever chunk the grid
    // picked. Whether the ground there suits a village is settled
    // elsewhere, which is Minecraft's order too: it chooses the place
    // first and asks about the land after.
    const Structures::Spot spot =
        Structures::inSquare(Structures::VILLAGE, regionX, regionZ, seed);

    Site site;
    site.exists = true;
    site.x = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
    site.z = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
    return site;
}

bool Village::reaches(const Site& site, int minX, int minZ, int maxX, int maxZ)
{
    if (!site.exists) return false;
    return site.x + MAX_RADIUS >= minX && site.x - MAX_RADIUS <= maxX &&
           site.z + MAX_RADIUS >= minZ && site.z - MAX_RADIUS <= maxZ;
}

namespace
{
    // Puts a building in its plot so that the side its door is on sits
    // a fixed step back from the street, and leaves the other axis
    // centred. Centring both ways instead left every frontage at a
    // different distance from the path, and a street of houses none of
    // which lined up read as scattered however tidy the grid was.
    void placeInPlot(const Village::Site& site, int gx, int gz, Village::Building& building)
    {
        using namespace Village;

        const int plotX = site.x + gx * PLOT - PLOT / 2;
        const int plotZ = site.z + gz * PLOT - PLOT / 2;
        const int plotEndX = plotX + PLOT - 1;
        const int plotEndZ = plotZ + PLOT - 1;

        constexpr int SETBACK = 1;   // between the frontage and the plot edge

        building.x = plotX + (PLOT - building.width) / 2;
        building.z = plotZ + (PLOT - building.depth) / 2;

        switch (building.facing)
        {
            case 0:  building.z = plotZ + SETBACK; break;                        // front north
            case 1:  building.x = plotEndX - SETBACK - (building.width - 1); break;
            case 2:  building.z = plotEndZ - SETBACK - (building.depth - 1); break;
            default: building.x = plotX + SETBACK; break;
        }
    }
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
            // The middle plot is left open. It used to hold a well,
            // which read as a shed in the middle of the square; the
            // paved square the paths lay there says "middle of the
            // village" better than a building did.
            if (gx == 0 && gz == 0) continue;

            const uint32_t roll = hashCoords(site.x + gx, 7, site.z + gz, seed + 55010);
            if (hashToFloat(roll) >= HOUSE_CHANCE)
            {
                // A plot with no house on it is either a field or a
                // lit corner of the path. A village of nothing but
                // houses has nobody growing anything.
                const float spare = hashToFloat(hashCoords(site.x + gx, 9, site.z + gz,
                                                           seed + 55011));

                if (spare < 0.40f)
                {
                    Building farm;
                    farm.kind = Kind::Farm;
                    farm.width = 9;
                    farm.depth = 7;

                    if (std::abs(gx) >= std::abs(gz)) farm.facing = (gx > 0) ? 3 : 1;
                    else                              farm.facing = (gz > 0) ? 0 : 2;

                    placeInPlot(site, gx, gz, farm);
                    out.push_back(farm);
                }
                else if (spare < 0.65f)
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
            // A handful of footprints rather than one. Nothing wider
            // than nine, so two in neighbouring plots still have a gap
            // between them for the path.
            struct Shape { int width, depth, storeys; };
            static const Shape SHAPES[] = {
                { 5, 5, 1 },   // a cottage
                { 7, 5, 1 },
                { 7, 7, 1 },
                { 9, 6, 1 },   // wide
                { 6, 9, 1 },   // deep
                { 7, 6, 2 },   // two storeys
                { 9, 7, 2 },   // the big one
            };
            constexpr int SHAPE_COUNT = static_cast<int>(sizeof(SHAPES) / sizeof(SHAPES[0]));

            const Shape& shape = SHAPES[hashCoords(site.x + gx, 11, site.z + gz,
                                                   seed + 55012) % SHAPE_COUNT];

            Building house;
            house.kind = Kind::Home;
            house.width = shape.width;
            house.depth = shape.depth;
            house.storeys = shape.storeys;

            // The door looks towards the middle, along whichever axis
            // the house is further out on, so every door faces a path.
            if (std::abs(gx) >= std::abs(gz)) house.facing = (gx > 0) ? 3 : 1;
            else                              house.facing = (gz > 0) ? 0 : 2;

            placeInPlot(site, gx, gz, house);
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
        case Kind::Lamp:       return "Lamp";
        case Kind::Farm:       return "Farm";
    }
    return "";
}

int Village::bedCount(const Site& site, uint32_t seed)
{
    int beds = 0;
    for (const Building& building : plan(site, seed))
        if (isHouse(building.kind)) ++beds;
    return beds;
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
