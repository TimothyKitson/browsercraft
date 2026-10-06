#pragma once
#include "Chunk.h"
#include "Noise.h"

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

    explicit WorldGen(uint32_t seed);

    void generate(Chunk& chunk) const;

    int surfaceHeight(int worldX, int worldZ) const;
    Biome biomeAt(int worldX, int worldZ) const;

    // Picks a reasonable, dry spawn point near the origin.
    void findSpawn(int& outX, int& outY, int& outZ) const;

private:
    uint32_t m_seed;
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

    void buildTree(Chunk& chunk, int worldX, int surfaceY, int worldZ, bool birch) const;
};
