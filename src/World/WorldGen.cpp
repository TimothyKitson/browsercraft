#include "WorldGen.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int MAX_HEIGHT = Chunk::SY - 1;

    float smoothstep(float edge0, float edge1, float x)
    {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
}

const char* biomeName(Biome biome)
{
    switch (biome)
    {
        case Biome::Ocean: return "Ocean";
        case Biome::Beach: return "Beach";
        case Biome::Plains: return "Plains";
        case Biome::Forest: return "Forest";
        case Biome::Desert: return "Desert";
        case Biome::Mountains: return "Mountains";
        case Biome::Snowy: return "Snowy";
    }
    return "Unknown";
}

const char* dimensionName(Dimension dimension)
{
    switch (dimension)
    {
        case Dimension::Nether: return "THE NETHER";
        case Dimension::End:    return "THE END";
        default:                return "OVERWORLD";
    }
}

const char* dimensionFolder(Dimension dimension)
{
    switch (dimension)
    {
        case Dimension::Nether: return "DIM-1";
        case Dimension::End:    return "DIM1";
        default:                return "";
    }
}

WorldGen::WorldGen(uint32_t seed, Dimension dimension)
    : m_seed(seed)
    , m_dimension(dimension)
    , m_height(seed + 1)
    , m_hills(seed + 2)
    , m_mountains(seed + 3)
    , m_temperature(seed + 4)
    , m_humidity(seed + 5)
    , m_caves(seed + 6)
    , m_caves2(seed + 7)
{
}

float WorldGen::temperatureAt(int x, int z) const
{
    return m_temperature.fbm2D(x * 0.0016f + 500.0f, z * 0.0016f - 200.0f, 3);
}

float WorldGen::humidityAt(int x, int z) const
{
    return m_humidity.fbm2D(x * 0.0019f - 800.0f, z * 0.0019f + 350.0f, 3);
}

int WorldGen::surfaceHeight(int worldX, int worldZ) const
{
    const float fx = static_cast<float>(worldX);
    const float fz = static_cast<float>(worldZ);

    // Large-scale shape: decides ocean basins vs. continents.
    float continent = m_height.fbm2D(fx * 0.0013f, fz * 0.0013f, 4);
    // Medium rolling hills.
    float hills = m_hills.fbm2D(fx * 0.0085f, fz * 0.0085f, 4);
    // Sharp ridges, only allowed to show up well inland.
    float ridges = m_mountains.ridged2D(fx * 0.0035f, fz * 0.0035f, 4);
    float mountainMask = smoothstep(0.15f, 0.55f, continent);

    float height = SEA_LEVEL
                 + continent * 20.0f
                 + hills * 7.0f
                 + mountainMask * ridges * 46.0f;

    return std::clamp(static_cast<int>(height), 1, MAX_HEIGHT - 12);
}

Biome WorldGen::biomeAt(int worldX, int worldZ) const
{
    int height = surfaceHeight(worldX, worldZ);
    float temperature = temperatureAt(worldX, worldZ);
    float humidity = humidityAt(worldX, worldZ);

    if (height < SEA_LEVEL - 1) return Biome::Ocean;
    if (height <= SEA_LEVEL + 1) return Biome::Beach;
    if (height > SEA_LEVEL + 38) return Biome::Mountains;
    if (temperature < -0.33f) return Biome::Snowy;
    if (temperature > 0.28f && humidity < 0.0f) return Biome::Desert;
    if (humidity > 0.08f) return Biome::Forest;
    return Biome::Plains;
}

void WorldGen::generate(Chunk& chunk) const
{
    switch (m_dimension)
    {
        case Dimension::Nether:
            generateNether(chunk);
            return;
        case Dimension::End:
            generateEnd(chunk);
            return;
        default:
            break;
    }

    generateColumns(chunk);
    carveCaves(chunk);
    placeOres(chunk);
    decorate(chunk);
}

// A closed cavern system rather than a landscape: solid netherrack between a
// bedrock floor and ceiling, hollowed out by 3D noise, with lava filling
// everything below the sea.
void WorldGen::generateNether(Chunk& chunk) const
{
    constexpr int ROOF = 120;
    constexpr int LAVA_LEVEL = NETHER_LAVA_LEVEL;

    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int lx = 0; lx < Chunk::SX; ++lx)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;

            for (int y = 0; y < ROOF; ++y)
            {
                BlockId block = Blocks::Netherrack;

                if (y <= 1 || y >= ROOF - 2)
                {
                    // Ragged bedrock shell top and bottom.
                    const int fromEdge = (y <= 1) ? y : (ROOF - 1 - y);
                    if (fromEdge == 0 || hashToFloat(hashCoords(wx, y, wz, m_seed + 71)) < 0.55f)
                        block = Blocks::Bedrock;
                }
                else
                {
                    // Two noise fields: one carves the big caverns, the other
                    // keeps the ceiling from being a flat lid.
                    const float cavern = m_caves.fbm3D(wx * 0.021f, y * 0.034f, wz * 0.021f, 3);
                    const float roofFade = static_cast<float>(ROOF - y) / 26.0f;
                    const float threshold = 0.06f - std::min(0.35f, std::max(0.0f, 1.0f - roofFade) * 0.3f);

                    if (cavern > threshold)
                        block = (y <= LAVA_LEVEL) ? Blocks::Lava : Blocks::Air;
                }

                chunk.setBlockRaw(lx, y, lz, block);
            }

            // Soul sand collects in the low flats, glowstone hangs from the roof.
            for (int y = LAVA_LEVEL + 1; y < ROOF - 3; ++y)
            {
                if (chunk.getBlock(lx, y, lz) != Blocks::Netherrack) continue;
                if (chunk.getBlock(lx, y + 1, lz) != Blocks::Air) continue;

                if (y < LAVA_LEVEL + 9 &&
                    hashToFloat(hashCoords(wx, y, wz, m_seed + 72)) < 0.22f)
                    chunk.setBlockRaw(lx, y, lz, Blocks::SoulSand);
                break;
            }

            for (int y = ROOF - 4; y > LAVA_LEVEL; --y)
            {
                if (chunk.getBlock(lx, y, lz) != Blocks::Netherrack) continue;
                if (chunk.getBlock(lx, y - 1, lz) != Blocks::Air) continue;

                if (hashToFloat(hashCoords(wx, y, wz, m_seed + 73)) < 0.035f)
                    chunk.setBlockRaw(lx, y - 1, lz, Blocks::Glowstone);
                break;
            }
        }
    }
}

// Islands of end stone floating in the void: one large central plateau at the
// origin, and scattered outer islands beyond it.
void WorldGen::generateEnd(Chunk& chunk) const
{
    constexpr int CENTRE_Y = END_ISLAND_LEVEL;
    constexpr float MAIN_RADIUS = 76.0f;
    constexpr float INNER_VOID = 420.0f; // gap before the outer islands begin

    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int lx = 0; lx < Chunk::SX; ++lx)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;
            const float distance = std::sqrt(static_cast<float>(wx) * wx + static_cast<float>(wz) * wz);

            // How solid this column wants to be, 0 at the rim and 1 at the core.
            float density;
            if (distance < MAIN_RADIUS)
            {
                density = 1.0f - (distance / MAIN_RADIUS);
            }
            else if (distance > INNER_VOID)
            {
                const float islands = m_height.fbm2D(wx * 0.0055f, wz * 0.0055f, 3);
                density = islands - 0.22f;
            }
            else
            {
                continue; // the ring of empty space around the main island
            }

            if (density <= 0.0f) continue;

            const int halfHeight = static_cast<int>(4.0f + density * 22.0f);
            for (int y = CENTRE_Y - halfHeight; y <= CENTRE_Y + halfHeight / 3; ++y)
            {
                if (y < 1 || y >= Chunk::SY) continue;

                // Erode the underside so islands taper rather than ending flat.
                const float erosion = m_caves.fbm3D(wx * 0.04f, y * 0.06f, wz * 0.04f, 2);
                const float depth = static_cast<float>(CENTRE_Y - y) / (halfHeight + 1.0f);
                if (erosion > 0.42f - depth * 0.3f) continue;

                chunk.setBlockRaw(lx, y, lz, Blocks::EndStone);
            }
        }
    }
}

void WorldGen::generateColumns(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int lx = 0; lx < Chunk::SX; ++lx)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;
            const int surface = surfaceHeight(wx, wz);
            const Biome biome = biomeAt(wx, wz);

            for (int y = 0; y <= surface; ++y)
            {
                BlockId block = Blocks::Stone;

                if (y == 0)
                {
                    block = Blocks::Bedrock;
                }
                else if (y <= 2 && hashToFloat(hashCoords(wx, y, wz, m_seed + 99)) < 0.55f)
                {
                    block = Blocks::Bedrock; // ragged bedrock floor
                }
                else
                {
                    const int depth = surface - y;
                    switch (biome)
                    {
                        case Biome::Ocean:
                            if (depth == 0) block = (hashToFloat(hashCoords(wx, 0, wz, m_seed + 12)) < 0.3f)
                                                        ? Blocks::Gravel : Blocks::Sand;
                            else if (depth <= 3) block = Blocks::Sand;
                            break;

                        case Biome::Beach:
                            if (depth <= 3) block = Blocks::Sand;
                            else if (depth <= 6) block = Blocks::Sandstone;
                            break;

                        case Biome::Desert:
                            if (depth <= 4) block = Blocks::Sand;
                            else if (depth <= 8) block = Blocks::Sandstone;
                            break;

                        case Biome::Snowy:
                            if (depth == 0) block = Blocks::SnowGrass;
                            else if (depth <= 4) block = Blocks::Dirt;
                            break;

                        case Biome::Mountains:
                            if (y > 96)
                            {
                                if (depth == 0) block = Blocks::Snow;
                            }
                            else if (depth == 0 && y < SEA_LEVEL + 48)
                            {
                                block = Blocks::Grass;
                            }
                            else if (depth <= 3 && y < SEA_LEVEL + 46)
                            {
                                block = Blocks::Dirt;
                            }
                            break;

                        case Biome::Plains:
                        case Biome::Forest:
                        default:
                            if (depth == 0) block = (surface < SEA_LEVEL) ? Blocks::Dirt : Blocks::Grass;
                            else if (depth <= 3) block = Blocks::Dirt;
                            break;
                    }
                }

                chunk.setBlockRaw(lx, y, lz, block);
            }

            // Fill oceans/lakes up to sea level.
            for (int y = surface + 1; y <= SEA_LEVEL; ++y)
            {
                bool freezes = (biome == Biome::Snowy) && (y == SEA_LEVEL);
                chunk.setBlockRaw(lx, y, lz, freezes ? Blocks::Ice : Blocks::Water);
            }
        }
    }
}

void WorldGen::carveCaves(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int lx = 0; lx < Chunk::SX; ++lx)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;
            const int surface = surfaceHeight(wx, wz);

            // Never carve directly under an ocean: that would drain it.
            const int ceiling = (surface < SEA_LEVEL) ? (SEA_LEVEL - 4) : (surface - 2);

            for (int y = 4; y <= ceiling; ++y)
            {
                const float fx = static_cast<float>(wx);
                const float fy = static_cast<float>(y);
                const float fz = static_cast<float>(wz);

                // Two independent tunnel fields: a value near zero means the
                // point sits on a noise "sheet", which traced through 3D
                // space makes a winding tunnel.
                float tunnelA = m_caves.fbm3D(fx * 0.017f, fy * 0.033f, fz * 0.017f, 2);
                float tunnelB = m_caves2.fbm3D(fx * 0.015f + 90.0f, fy * 0.030f, fz * 0.015f - 40.0f, 2);

                bool carve = (std::fabs(tunnelA) < 0.055f) && (std::fabs(tunnelB) < 0.075f);

                // Occasional larger caverns deeper down.
                if (!carve && y < 40)
                {
                    float cavern = m_caves.fbm3D(fx * 0.028f, fy * 0.045f, fz * 0.028f, 3);
                    carve = cavern > 0.56f;
                }

                if (carve)
                {
                    BlockId current = chunk.getBlock(lx, y, lz);
                    if (current != Blocks::Bedrock && current != Blocks::Water)
                        chunk.setBlockRaw(lx, y, lz, Blocks::Air);
                }
            }
        }
    }
}

void WorldGen::placeOres(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    struct OreSpec
    {
        BlockId block;
        int minY, maxY;
        float chancePerCell;
        int maxVeinSize;
        uint32_t salt;
    };

    const OreSpec ORES[] = {
        { Blocks::CoalOre,    5, 110, 0.030f, 9, 1001 },
        { Blocks::IronOre,    5,  64, 0.022f, 7, 1002 },
        { Blocks::GoldOre,    5,  32, 0.010f, 5, 1003 },
        { Blocks::DiamondOre, 5,  16, 0.006f, 4, 1004 },
        { Blocks::Gravel,    10, 100, 0.012f, 12, 1005 },
        { Blocks::Clay,      40,  66, 0.008f, 8, 1006 },
    };

    for (const OreSpec& ore : ORES)
    {
        for (int y = ore.minY; y <= ore.maxY && y < Chunk::SY; ++y)
        {
            for (int lx = 0; lx < Chunk::SX; ++lx)
            {
                for (int lz = 0; lz < Chunk::SZ; ++lz)
                {
                    const int wx = baseX + lx;
                    const int wz = baseZ + lz;
                    if (hashToFloat(hashCoords(wx, y, wz, m_seed + ore.salt)) >= ore.chancePerCell)
                        continue;

                    // Grow a small blob around the seed point.
                    int veinSize = 2 + static_cast<int>(hashToFloat(hashCoords(wx, y + 1, wz, m_seed + ore.salt)) * ore.maxVeinSize);
                    for (int i = 0; i < veinSize; ++i)
                    {
                        uint32_t h = hashCoords(wx + i, y, wz - i, m_seed + ore.salt + 77);
                        int dx = static_cast<int>(hashToFloat(h) * 3.0f) - 1;
                        int dy = static_cast<int>(hashToFloat(h >> 8) * 3.0f) - 1;
                        int dz = static_cast<int>(hashToFloat(h >> 16) * 3.0f) - 1;
                        int px = lx + dx, py = y + dy, pz = lz + dz;
                        if (chunk.getBlock(px, py, pz) == Blocks::Stone)
                            chunk.setBlockRaw(px, py, pz, ore.block);
                    }
                }
            }
        }
    }
}

void WorldGen::buildTree(Chunk& chunk, int worldX, int surfaceY, int worldZ, bool birch) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    const BlockId logBlock = birch ? Blocks::BirchLog : Blocks::Log;
    const BlockId leafBlock = birch ? Blocks::BirchLeaves : Blocks::Leaves;

    uint32_t h = hashCoords(worldX, 0, worldZ, m_seed + 5150);
    const int trunkHeight = 4 + static_cast<int>(hashToFloat(h) * 3.0f) + (birch ? 1 : 0);
    const int topY = surfaceY + trunkHeight;

    // Canopy first so the trunk overwrites any leaves in its column.
    for (int dy = -2; dy <= 1; ++dy)
    {
        int y = topY + dy;
        if (y < 0 || y >= Chunk::SY) continue;
        int radius = (dy >= 1) ? 1 : 2;

        for (int dx = -radius; dx <= radius; ++dx)
        {
            for (int dz = -radius; dz <= radius; ++dz)
            {
                if (std::abs(dx) == radius && std::abs(dz) == radius)
                {
                    // Round the corners off, with a little randomness.
                    if (hashToFloat(hashCoords(worldX + dx, y, worldZ + dz, m_seed + 61)) < 0.6f)
                        continue;
                }
                int lx = worldX + dx - baseX;
                int lz = worldZ + dz - baseZ;
                if (!Chunk::inBounds(lx, y, lz)) continue;
                if (chunk.getBlock(lx, y, lz) == Blocks::Air)
                    chunk.setBlockRaw(lx, y, lz, leafBlock);
            }
        }
    }

    for (int i = 0; i < trunkHeight; ++i)
    {
        int y = surfaceY + 1 + i;
        int lx = worldX - baseX;
        int lz = worldZ - baseZ;
        if (Chunk::inBounds(lx, y, lz))
            chunk.setBlockRaw(lx, y, lz, logBlock);
    }
}

void WorldGen::decorate(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    // Trees are placed from a deterministic function of world position, so
    // we can look at columns just outside this chunk and draw the parts of
    // their canopies that overhang into it. That keeps trees seamless
    // across chunk borders without any cross-chunk writes.
    for (int wx = baseX - 3; wx < baseX + Chunk::SX + 3; ++wx)
    {
        for (int wz = baseZ - 3; wz < baseZ + Chunk::SZ + 3; ++wz)
        {
            const Biome biome = biomeAt(wx, wz);
            float treeChance = 0.0f;
            switch (biome)
            {
                case Biome::Forest: treeChance = 0.055f; break;
                case Biome::Plains: treeChance = 0.004f; break;
                case Biome::Snowy: treeChance = 0.018f; break;
                case Biome::Mountains: treeChance = 0.006f; break;
                default: break;
            }
            if (treeChance <= 0.0f) continue;

            if (hashToFloat(hashCoords(wx, 7, wz, m_seed + 4242)) >= treeChance) continue;

            const int surface = surfaceHeight(wx, wz);
            if (surface <= SEA_LEVEL) continue; // no trees in water

            bool birch = hashToFloat(hashCoords(wx, 8, wz, m_seed + 4243)) < 0.35f;
            buildTree(chunk, wx, surface, wz, birch);
        }
    }

    // Small single-block decorations only need this chunk's own columns.
    for (int lx = 0; lx < Chunk::SX; ++lx)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;
            const int surface = surfaceHeight(wx, wz);
            const int above = surface + 1;
            if (above >= Chunk::SY) continue;
            if (chunk.getBlock(lx, above, lz) != Blocks::Air) continue;

            const BlockId ground = chunk.getBlock(lx, surface, lz);
            const Biome biome = biomeAt(wx, wz);
            const float roll = hashToFloat(hashCoords(wx, 3, wz, m_seed + 777));

            if (biome == Biome::Desert && ground == Blocks::Sand)
            {
                if (roll < 0.004f)
                {
                    int cactusHeight = 1 + static_cast<int>(hashToFloat(hashCoords(wx, 4, wz, m_seed + 778)) * 3.0f);
                    for (int i = 0; i < cactusHeight && above + i < Chunk::SY; ++i)
                        chunk.setBlockRaw(lx, above + i, lz, Blocks::Cactus);
                }
                continue;
            }

            if (ground != Blocks::Grass && ground != Blocks::SnowGrass) continue;

            if (roll < 0.14f)
                chunk.setBlockRaw(lx, above, lz, Blocks::TallGrass);
            else if (roll < 0.155f)
                chunk.setBlockRaw(lx, above, lz, Blocks::FlowerRed);
            else if (roll < 0.168f)
                chunk.setBlockRaw(lx, above, lz, Blocks::FlowerYellow);
            else if (roll < 0.1695f && biome != Biome::Snowy)
                chunk.setBlockRaw(lx, above, lz, Blocks::Pumpkin);
        }
    }
}

void WorldGen::findSpawn(int& outX, int& outY, int& outZ) const
{
    // Spiral outwards from the origin until we find dry land.
    for (int radius = 0; radius < 400; radius += 8)
    {
        for (int angleStep = 0; angleStep < 16; ++angleStep)
        {
            float angle = angleStep * 0.3927f; // 2*pi/16
            int x = static_cast<int>(std::cos(angle) * radius);
            int z = static_cast<int>(std::sin(angle) * radius);
            int height = surfaceHeight(x, z);
            Biome biome = biomeAt(x, z);
            if (height > SEA_LEVEL + 1 && biome != Biome::Ocean)
            {
                outX = x;
                outY = height + 2;
                outZ = z;
                return;
            }
        }
    }

    outX = 0;
    outY = SEA_LEVEL + 4;
    outZ = 0;
}
