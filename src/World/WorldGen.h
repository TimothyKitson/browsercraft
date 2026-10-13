#pragma once
#include "Chunk.h"
#include "Noise.h"
#include "Village.h"

#include "Dimension.h"
const char* dimensionFolder(Dimension dimension);

enum class Biome : uint8_t
{
    Ocean,
    Beach,
    Plains,
    Forest,
    Desert,
    Mountains,
    Snowy
};

const char* biomeName(Biome biome);

// Terrain generation. Every method is const and uses only the seeded noise
// tables, so worker threads can generate different chunks in parallel from
// a single shared instance.
class WorldGen
{
public:
    static constexpr int SEA_LEVEL = 62;
    // The lava sea the Nether is built around, and the height the End's
    // central plateau is centred on. Public because spawning somewhere
    // survivable in either dimension has to know where the ground is.
    static constexpr int NETHER_LAVA_LEVEL = 31;
    static constexpr int END_ISLAND_LEVEL = 58;

    WorldGen(uint32_t seed, Dimension dimension = Dimension::Overworld);

    Dimension dimension() const { return m_dimension; }

    void generate(Chunk& chunk) const;

    int surfaceHeight(int worldX, int worldZ) const;
    Biome biomeAt(int worldX, int worldZ) const;

    // The nearest village to a place, searching outwards a few squares.
    // Public because a dev screen and a command both want to find one,
    // and because walking to the nearest village is exactly the thing
    // a player will ask for first.
    Village::Site nearestVillage(int fromX, int fromZ, int squares = 3) const;

    // Why a site was turned down, for --villageinfo. Counting the
    // reasons is the only way to tell "the land is too steep" from
    // "there are no sites at all".
    struct VillageTally
    {
        int sites = 0;
        int wrongBiome = 0;
        int tooWet = 0;
        int tooSteep = 0;
        int built = 0;
    };
    VillageTally surveyVillages(int fromX, int fromZ, int squares) const;

    // Picks a reasonable, dry spawn point near the origin.
    void findSpawn(int& outX, int& outY, int& outZ) const;

private:
    uint32_t m_seed;
    Dimension m_dimension;
    Noise m_height;
    Noise m_hills;
    Noise m_mountains;
    Noise m_temperature;
    Noise m_humidity;
    Noise m_caves;
    Noise m_caves2;

    float temperatureAt(int x, int z) const;
    float humidityAt(int x, int z) const;

    void generateColumns(Chunk& chunk) const;

    // Villages. Each is wider than a chunk, so every chunk it reaches
    // into lays out the whole thing and stamps only its own part --
    // which works because the layout depends on nothing but the seed.
    void buildVillages(Chunk& chunk) const;
    // Whether the ground where this village would stand could carry
    // it. Decided from the site alone, so every chunk agrees.
    bool villageFits(const Village::Site& site) const;
    void stampBuilding(Chunk& chunk, const Village::Building& building) const;
    void stampInterior(Chunk& chunk, const Village::Building& building, int ground,
                       int x0, int z0, int x1, int z1, int doorX, int doorZ) const;
    void carveCaves(Chunk& chunk) const;
    void placeOres(Chunk& chunk) const;
    void decorate(Chunk& chunk) const;

    void generateNether(Chunk& chunk) const;
    void generateEnd(Chunk& chunk) const;

    void buildTree(Chunk& chunk, int worldX, int surfaceY, int worldZ, bool birch) const;
};
