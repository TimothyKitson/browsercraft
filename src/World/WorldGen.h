#pragma once
#include "Chunk.h"
#include "Noise.h"

// Which world a generator is building. Each has its own terrain shape, and
// chunks are saved under their own folder so the three never collide.
enum class Dimension : uint8_t
{
    Overworld,
    Nether,
    End
};

const char* dimensionName(Dimension dimension);
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
    void carveCaves(Chunk& chunk) const;
    void placeOres(Chunk& chunk) const;
    void decorate(Chunk& chunk) const;

    void generateNether(Chunk& chunk) const;
    void generateEnd(Chunk& chunk) const;

    void buildTree(Chunk& chunk, int worldX, int surfaceY, int worldZ, bool birch) const;
};
