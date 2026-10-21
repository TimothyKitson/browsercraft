#include "WorldGen.h"
#include "Game/EndPortal.h"
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

    // fbm2D divides by the sum of its octave amplitudes, so the octaves
    // average each other out and a four-octave field only ever covers
    // about a third of [-1, 1]. Every threshold below is written against
    // the full range, which is why the world came out as a pancake at
    // sea level with no mountains, no desert and no snow in it.
    //
    // tanh rather than a multiply and a clamp: it is smooth the whole way
    // out, so stretching the field leaves no plateaus where the peaks and
    // troughs would otherwise have been flattened off.
    float spread(float v, float strength)
    {
        return std::tanh(v * strength);
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
        case Biome::Jungle: return "Jungle";
        case Biome::Savanna: return "Savanna";
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
    return spread(m_temperature.fbm2D(x * 0.0016f + 500.0f, z * 0.0016f - 200.0f, 3), 3.0f);
}

float WorldGen::humidityAt(int x, int z) const
{
    return spread(m_humidity.fbm2D(x * 0.0019f - 800.0f, z * 0.0019f + 350.0f, 3), 3.0f);
}

int WorldGen::surfaceHeight(int worldX, int worldZ) const
{
    const float fx = static_cast<float>(worldX);
    const float fz = static_cast<float>(worldZ);

    // Large-scale shape: decides ocean basins vs. continents. Lifted so
    // that rather more of the world is land than sea, the way an
    // overworld is -- centred on sea level it came out as half ocean.
    float continent = spread(m_height.fbm2D(fx * 0.0013f, fz * 0.0013f, 4), 3.0f) + 0.30f;
    // Medium rolling hills.
    float hills = spread(m_hills.fbm2D(fx * 0.0085f, fz * 0.0085f, 4), 2.6f);
    // Sharp ridges, only allowed to show up well inland. ridged2D sits
    // high across most of its range, so only its upper part is taken:
    // left whole it raised every inland block by twenty-odd and made one
    // flat plateau of the continents.
    float ridges = m_mountains.ridged2D(fx * 0.0035f, fz * 0.0035f, 4);
    ridges = std::max(0.0f, ridges - 0.55f) / 0.45f;
    float mountainMask = smoothstep(0.10f, 0.70f, continent);

    float height = SEA_LEVEL
                 + continent * 22.0f
                 + hills * 8.0f
                 + mountainMask * ridges * 52.0f;

    return std::clamp(static_cast<int>(height), 1, MAX_HEIGHT - 12);
}

Biome WorldGen::biomeAt(int worldX, int worldZ) const
{
    int height = surfaceHeight(worldX, worldZ);
    float temperature = temperatureAt(worldX, worldZ);
    float humidity = humidityAt(worldX, worldZ);

    if (height < SEA_LEVEL - 1) return Biome::Ocean;
    if (height <= SEA_LEVEL) return Biome::Beach;
    if (height > SEA_LEVEL + 34) return Biome::Mountains;
    if (temperature < -0.45f) return Biome::Snowy;

    // Everything hot, sorted by how wet it is: rainforest, then the dry
    // grassland, then sand. The desert test used to take the whole of
    // the hot end and the rest came out as forest.
    if (temperature > 0.30f)
    {
        if (humidity > 0.22f) return Biome::Jungle;
        if (humidity < -0.05f) return Biome::Desert;
        return Biome::Savanna;
    }

    if (humidity > 0.05f) return Biome::Forest;
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
    layDeepslate(chunk);
    placeOres(chunk);
    dressCaves(chunk);
    decorate(chunk);
    buildVillages(chunk);
    buildTrialChambers(chunk);
    buildStrongholds(chunk);
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
                {
                    chunk.setBlockRaw(lx, y, lz, Blocks::SoulSand);

                    // Wart grows on it, here and there. This is the
                    // only place it comes from, so the flats are worth
                    // walking once brewing matters to you.
                    if (hashToFloat(hashCoords(wx, y, wz, m_seed + 74)) < 0.10f)
                        chunk.setBlockRaw(lx, y + 1, lz, Blocks::NetherWart);
                }
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
                            // Patches of gravel among the sand. Drawn from
                            // smooth noise rather than a per-column coin
                            // flip, which scattered single blocks of gravel
                            // through the seabed like static.
                            if (depth == 0)
                                block = m_humidity.fbm2D(wx * 0.045f, wz * 0.045f, 2) > 0.22f
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
                        case Biome::Jungle:
                        case Biome::Savanna:
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

WorldGen::CaveKind WorldGen::caveKindAt(int worldX, int worldZ) const
{
    // Low enough in frequency that a cave is one sort of cave for a
    // few hundred blocks: a tunnel that changed character every twenty
    // would read as noise rather than as somewhere.
    const float field = m_humidity.fbm2D(worldX * 0.0032f + 410.0f,
                                         worldZ * 0.0032f - 260.0f, 2);

    if (field > 0.26f) return CaveKind::Lush;
    if (field < -0.26f) return CaveKind::Dripstone;
    return CaveKind::Plain;
}

// A cave with something in it. Caves were a grey tube with ores in the
// walls; these are the two sorts Minecraft hollows out of the same
// rock, picked by where you are rather than by what the tunnel looks
// like, so one goes on being itself however far you follow it.
void WorldGen::dressCaves(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int lx = 0; lx < Chunk::SX; ++lx)
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;

            const CaveKind kind = caveKindAt(wx, wz);
            if (kind == CaveKind::Plain) continue;

            const int top = std::min(surfaceHeight(wx, wz) - 3, Chunk::SY - 2);

            for (int y = 5; y <= top; ++y)
            {
                if (chunk.getBlock(lx, y, lz) != Blocks::Air) continue;

                const BlockId under = chunk.getBlock(lx, y - 1, lz);
                const BlockId over = chunk.getBlock(lx, y + 1, lz);
                const bool floor = isSolid(under) && !isLiquid(under);
                const bool roof = isSolid(over) && !isLiquid(over);
                if (!floor && !roof) continue;

                const float roll = hashToFloat(hashCoords(wx, y, wz, m_seed + 7700));

                if (kind == CaveKind::Dripstone)
                {
                    // The rock itself turns to dripstone around the
                    // cave, and spikes hang and stand in it.
                    if (floor && roll < 0.42f)
                        chunk.setBlockRaw(lx, y - 1, lz, Blocks::DripstoneBlock);
                    if (roof && roll > 0.58f)
                        chunk.setBlockRaw(lx, y + 1, lz, Blocks::DripstoneBlock);

                    if (roof && roll > 0.86f)
                        chunk.setBlockRaw(lx, y, lz, Blocks::DripstoneDown);
                    else if (floor && roll < 0.12f)
                        chunk.setBlockRaw(lx, y, lz, Blocks::DripstoneUp);
                }
                else
                {
                    // Moss underfoot, azalea overhead, and the berries
                    // that are the only light down here that nobody
                    // had to carry in.
                    if (floor && roll < 0.55f)
                        chunk.setBlockRaw(lx, y - 1, lz, Blocks::Moss);
                    if (roof && roll > 0.62f)
                        chunk.setBlockRaw(lx, y + 1, lz, Blocks::AzaleaLeaves);

                    if (roof && roll > 0.90f)
                        chunk.setBlockRaw(lx, y, lz, Blocks::GlowBerries);
                }
            }
        }
}

// Everything below the deepslate line turns to deepslate, with a ragged
// edge so the change of rock is not a flat ceiling across the world, and
// tuff and calcite seaming through it. Done before the ores so a seam
// found down there sits in deepslate rather than in a pocket of stone.
void WorldGen::layDeepslate(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int lx = 0; lx < Chunk::SX; ++lx)
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;

            // A few blocks of wobble either way, so the boundary reads
            // as a change of rock rather than as a line someone drew.
            const int wobble =
                static_cast<int>(hashToFloat(hashCoords(wx, 0, wz, m_seed + 4411)) * 7.0f) - 3;
            const int top = DEEPSLATE_TOP + wobble;

            for (int y = 1; y <= top && y < Chunk::SY; ++y)
            {
                if (chunk.getBlock(lx, y, lz) != Blocks::Stone) continue;

                BlockId rock = Blocks::Deepslate;
                const float roll = hashToFloat(hashCoords(wx, y, wz, m_seed + 4412));
                if (roll < 0.055f)      rock = Blocks::Tuff;
                else if (roll < 0.068f) rock = Blocks::Calcite;

                chunk.setBlockRaw(lx, y, lz, rock);
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

    // chancePerCell is rolled for every block in the range, so these
    // numbers are small on purpose: at three in a hundred, coal was an
    // eighth of all the stone in the world and diamond was one block in
    // a hundred of the deep rock. Mining anything was a formality, and
    // the tool tiers it is supposed to gate meant nothing.
    //
    // --oreinfo prints what these actually produce, per thousand stone
    // blocks, by height. Tune against that rather than by eye.
    const OreSpec ORES[] = {
        { Blocks::CoalOre,    5, 110, 0.0030f, 12, 1001 },
        { Blocks::IronOre,    5,  64, 0.0026f,  9, 1002 },
        { Blocks::GoldOre,    5,  32, 0.00040f, 7, 1003 },
        { Blocks::DiamondOre, 5,  16, 0.00038f, 6, 1004 },
        // The rarest thing in the ground, and the only one that comes
        // in ones rather than seams -- a vein size of 1, as in
        // Minecraft, which is what makes finding one worth saying out
        // loud. Tuned against --oreinfo like the rest.
        { Blocks::EmeraldOre, 5,  32, 0.00011f, 1, 1007 },
        // Commoner than iron and deeper: in Minecraft redstone is the
        // one seam you find more of than you can use, and the whole
        // point of it is that there is plenty.
        { Blocks::RedstoneOre, 5,  16, 0.0030f,  8, 1011 },
        // Copper sits high and comes in fat seams: in Minecraft it is
        // the one ore you find more of than you know what to do with.
        { Blocks::CopperOre,  40, 110, 0.0024f, 16, 1012 },
        // The upper rock is not one grey block. These are deliberately
        // fatter and commoner than any seam: they are scenery, not a
        // find, and at a vein of thirty they read as the blotches
        // vanilla cuts through its stone rather than as lumps.
        { Blocks::Granite,   DEEPSLATE_TOP, 110, 0.0016f, 30, 1008 },
        { Blocks::Diorite,   DEEPSLATE_TOP, 110, 0.0016f, 30, 1009 },
        { Blocks::Andesite,  DEEPSLATE_TOP, 110, 0.0016f, 30, 1010 },
        { Blocks::Gravel,    10, 100, 0.0035f, 14, 1005 },
        { Blocks::Clay,      40,  66, 0.0030f, 10, 1006 },
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
                        if (isStoneFamily(chunk.getBlock(px, py, pz)))
                            chunk.setBlockRaw(px, py, pz, ore.block);
                    }
                }
            }
        }
    }
}

void WorldGen::buildTree(Chunk& chunk, int worldX, int surfaceY, int worldZ, TreeKind kind) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    BlockId logBlock = Blocks::Log;
    BlockId leafBlock = Blocks::Leaves;
    int extraHeight = 0;
    int canopyRadius = 2;

    switch (kind)
    {
        case TreeKind::Birch:
            logBlock = Blocks::BirchLog;  leafBlock = Blocks::BirchLeaves;
            extraHeight = 1;
            break;
        case TreeKind::Spruce:
            logBlock = Blocks::SpruceLog; leafBlock = Blocks::SpruceLeaves;
            // Tall and narrow, which is what a conifer is.
            extraHeight = 3;
            canopyRadius = 1;
            break;
        case TreeKind::DarkOak:
            logBlock = Blocks::DarkOakLog; leafBlock = Blocks::DarkOakLeaves;
            // Short and broad, so a dark oak wood has a closed canopy.
            extraHeight = -1;
            canopyRadius = 3;
            break;
        case TreeKind::Jungle:
            logBlock = Blocks::JungleLog; leafBlock = Blocks::JungleLeaves;
            // The tallest thing that grows: a jungle is a roof you walk
            // about underneath.
            extraHeight = 6;
            canopyRadius = 3;
            break;
        case TreeKind::Acacia:
            logBlock = Blocks::AcaciaLog; leafBlock = Blocks::AcaciaLeaves;
            // Bare trunk and then a flat plate of leaves on top, which
            // is the one tree you can name from its silhouette alone.
            extraHeight = 2;
            canopyRadius = 3;
            break;
        case TreeKind::Oak:
            break;
    }

    uint32_t h = hashCoords(worldX, 0, worldZ, m_seed + 5150);
    const int trunkHeight = 4 + static_cast<int>(hashToFloat(h) * 3.0f) + extraHeight;
    const int topY = surfaceY + trunkHeight;

    // An acacia leans: the trunk goes up straight and then steps two
    // blocks to one side, and the plate of leaves sits over where it
    // ended up rather than over its roots. Without the lean a flat
    // canopy on a straight trunk is a mushroom.
    int leanX = 0, leanZ = 0;
    if (kind == TreeKind::Acacia)
    {
        const int way = static_cast<int>(
            hashToFloat(hashCoords(worldX, 1, worldZ, m_seed + 5151)) * 4.0f) & 3;
        if (way == 0) leanX = 1;
        else if (way == 1) leanX = -1;
        else if (way == 2) leanZ = 1;
        else leanZ = -1;
    }
    const int crownX = worldX + leanX * 2;
    const int crownZ = worldZ + leanZ * 2;

    // Canopy first so the trunk overwrites any leaves in its column. A
    // conifer carries its needles further down the trunk than a
    // broadleaf carries its leaves, which is the whole silhouette.
    // An acacia carries its leaves in a plate two blocks deep; a
    // conifer carries them most of the way down the trunk.
    const int canopyDrop = (kind == TreeKind::Spruce)  ? 5
                         : (kind == TreeKind::Acacia) ? 1
                                                      : 2;
    for (int dy = -canopyDrop; dy <= 1; ++dy)
    {
        int y = topY + dy;
        if (y < 0 || y >= Chunk::SY) continue;

        // Flat on top and flat underneath: an acacia's plate does not
        // taper the way a broadleaf's crown does.
        int radius = (kind == TreeKind::Acacia)
                         ? canopyRadius
                         : ((dy >= 1) ? std::max(1, canopyRadius - 1) : canopyRadius);

        for (int dx = -radius; dx <= radius; ++dx)
        {
            for (int dz = -radius; dz <= radius; ++dz)
            {
                if (std::abs(dx) == radius && std::abs(dz) == radius)
                {
                    // Round the corners off, with a little randomness.
                    if (hashToFloat(hashCoords(crownX + dx, y, crownZ + dz, m_seed + 61)) < 0.6f)
                        continue;
                }
                int lx = crownX + dx - baseX;
                int lz = crownZ + dz - baseZ;
                if (!Chunk::inBounds(lx, y, lz)) continue;
                if (chunk.getBlock(lx, y, lz) == Blocks::Air)
                    chunk.setBlockRaw(lx, y, lz, leafBlock);
            }
        }
    }

    for (int i = 0; i < trunkHeight; ++i)
    {
        int y = surfaceY + 1 + i;

        // The last two blocks of an acacia's trunk walk out to meet its
        // crown; everything else goes straight up.
        const int step = (trunkHeight - i <= 2) ? (trunkHeight - i == 2 ? 1 : 2) : 0;
        int lx = worldX + leanX * step - baseX;
        int lz = worldZ + leanZ * step - baseZ;
        if (Chunk::inBounds(lx, y, lz))
            chunk.setBlockRaw(lx, y, lz, logBlock);
    }
}

namespace
{
    // Writes a world-space block, ignoring anything outside this chunk.
    // Every village stamp goes through this: a chunk only ever writes
    // its own cells, and the parts that fall elsewhere are written by
    // the chunk they belong to when its turn comes.
    void put(Chunk& chunk, int worldX, int y, int worldZ, BlockId block)
    {
        const int lx = worldX - chunk.position().x * Chunk::SX;
        const int lz = worldZ - chunk.position().z * Chunk::SZ;
        if (Chunk::inBounds(lx, y, lz)) chunk.setBlockRaw(lx, y, lz, block);
    }
}

WorldGen::Palette WorldGen::paletteFor(Biome biome) const
{
    Palette palette;

    switch (biome)
    {
        case Biome::Desert:
            // Sand country builds in sand, and roofs flat: there is
            // nothing to shed, and the flat top is shade.
            palette.style = RoofStyle::Flat;
            palette.wall = Blocks::Sandstone;
            palette.footing = Blocks::Sandstone;
            palette.corner = Blocks::Sandstone;
            palette.floor = Blocks::Sandstone;
            palette.roof = Blocks::Sandstone;
            palette.roofStairs = Blocks::SandstoneStairsNorth;
            palette.post = Blocks::Sandstone;
            break;

        case Biome::Snowy:
            // Pale birch walls with dark corners. Cobblestone roofs
            // over cobblestone courses made the houses read as one
            // grey mass with the snow behind them.
            palette.wall = Blocks::BirchLog;
            palette.footing = Blocks::Cobblestone;
            palette.corner = Blocks::Log;
            palette.floor = Blocks::Planks;
            palette.roof = Blocks::Planks;
            palette.roofStairs = Blocks::PlankStairsNorth;
            palette.post = Blocks::BirchLog;
            break;

        case Biome::Savanna:
            // Acacia, which is the one wood nobody mistakes for another.
            palette.wall = Blocks::AcaciaLog;
            palette.footing = Blocks::Cobblestone;
            palette.corner = Blocks::AcaciaLog;
            palette.floor = Blocks::AcaciaPlanks;
            palette.roof = Blocks::AcaciaPlanks;
            palette.roofStairs = Blocks::PlankStairsNorth;
            palette.post = Blocks::AcaciaLog;
            break;

        case Biome::Jungle:
            palette.wall = Blocks::JungleLog;
            palette.footing = Blocks::Cobblestone;
            palette.corner = Blocks::JungleLog;
            palette.floor = Blocks::JunglePlanks;
            palette.roof = Blocks::JunglePlanks;
            palette.roofStairs = Blocks::PlankStairsNorth;
            palette.post = Blocks::JungleLog;
            break;

        case Biome::Forest:
            // Timber country: logs for the walls, not just the corners.
            palette.wall = Blocks::Log;
            palette.footing = Blocks::Cobblestone;
            palette.corner = Blocks::Log;
            palette.floor = Blocks::Planks;
            palette.roof = Blocks::Planks;
            palette.roofStairs = Blocks::PlankStairsNorth;
            palette.post = Blocks::Log;
            break;

        default:
            break;   // the plains build the way the first ones did
    }

    return palette;
}

bool WorldGen::villageFits(const Village::Site& site) const
{
    if (!site.exists) return false;

    // Anywhere people could live. Mountains are out because a village
    // there would be half cliff, and so are the ocean and the beach.
    // Plains and desert alone left four villages in five with nowhere
    // to stand: this world is mostly forest.
    const Biome biome = biomeAt(site.x, site.z);
    if (biome != Biome::Plains && biome != Biome::Desert &&
        biome != Biome::Forest && biome != Biome::Snowy &&
        biome != Biome::Savanna)
        return false;

    // Sample the whole footprint, not just the middle: a village laid
    // across a slope ends up with some houses buried and some on
    // stilts, and the middle alone cannot tell you that.
    int lowest = Chunk::SY;
    int highest = 0;
    for (int dz = -Village::MAX_RADIUS; dz <= Village::MAX_RADIUS; dz += 7)
        for (int dx = -Village::MAX_RADIUS; dx <= Village::MAX_RADIUS; dx += 7)
        {
            const int here = surfaceHeight(site.x + dx, site.z + dz);
            lowest = std::min(lowest, here);
            highest = std::max(highest, here);
        }

    if (lowest <= SEA_LEVEL + 1) return false;   // one foot in the sea

    // Each building levels its own patch, so the village only has to
    // avoid genuine hillsides rather than find a billiard table.
    return (highest - lowest) <= 12;
}

void WorldGen::stampBuilding(Chunk& chunk, const Village::Building& building,
                             const Palette& palette) const
{
    // Each building finds its own level, from the ground under its own
    // middle. Levelling the whole village to one height instead meant
    // nothing could be built on anything but a billiard table -- and
    // this is just as safe across chunk borders, because the height is
    // still a pure function of where the building is.
    const int ground = surfaceHeight(building.x + building.width / 2,
                                     building.z + building.depth / 2);

    const int x0 = building.x;
    const int z0 = building.z;
    const int x1 = x0 + building.width - 1;
    const int z1 = z0 + building.depth - 1;

    if (building.kind == Village::Kind::Lamp)
    {
        for (int y = ground + 1; y <= ground + 3; ++y)
            put(chunk, x0, y, z0, palette.post);
        put(chunk, x0, ground + 4, z0, Blocks::Torch);
        return;
    }

    if (building.kind == Village::Kind::Farm)
    {
        // A fenced field with a channel down the middle of it. The
        // channel is what keeps the soil wet, and in Minecraft it is
        // also the thing that makes a village field read as a field
        // rather than a patch of dirt.
        const int midZ = (z0 + z1) / 2;

        for (int x = x0; x <= x1; ++x)
            for (int z = z0; z <= z1; ++z)
            {
                for (int y = ground + 1; y <= ground + 6; ++y)
                    put(chunk, x, y, z, Blocks::Air);

                const bool rim = (x == x0 || x == x1 || z == z0 || z == z1);
                if (rim)
                {
                    // An oak log frame round the bed and nothing above
                    // it, which is how Minecraft edges a field. The
                    // fence that used to stand on it walled the crop
                    // off from the village it belongs to.
                    put(chunk, x, ground, z, Blocks::Log);
                    continue;
                }

                if (z == midZ)
                {
                    put(chunk, x, ground, z, Blocks::Water);
                    continue;
                }

                put(chunk, x, ground, z, Blocks::Farmland);

                // Mixed growth, so a field looks worked rather than
                // planted all on one morning.
                const int stage = static_cast<int>(
                    hashCoords(x, 31, z, m_seed + 55040) % 8u);
                put(chunk, x, ground + 1, z, wheatAtStage(stage));
            }

        return;
    }

    // --- a house ---------------------------------------------------

    // Four courses to a storey, so a two-storey house stands twice as
    // tall and its windows come in two rows.
    const int wallHeight = 4 * std::max(1, building.storeys);
    const int eaves = ground + wallHeight;
    const int roofBase = eaves + 1;

    // Level the ground under it and clear the air above, so a house on
    // a slight rise is neither buried nor standing on nothing.
    for (int x = x0; x <= x1; ++x)
        for (int z = z0; z <= z1; ++z)
        {
            for (int y = ground - 3; y <= ground; ++y)
                put(chunk, x, y, z, (y == ground) ? palette.floor : Blocks::Dirt);
            for (int y = ground + 1; y <= roofBase + 12; ++y)
                put(chunk, x, y, z, Blocks::Air);
        }

    // Walls: a cobblestone course at the bottom, logs at the corners,
    // planks between. The stone footing is most of what stops a house
    // reading as a shed.
    for (int y = ground + 1; y <= eaves; ++y)
        for (int x = x0; x <= x1; ++x)
            for (int z = z0; z <= z1; ++z)
            {
                const bool edgeX = (x == x0 || x == x1);
                const bool edgeZ = (z == z0 || z == z1);
                if (!edgeX && !edgeZ) continue;

                BlockId block = palette.wall;
                if (edgeX && edgeZ) block = palette.corner;
                else if (y == ground + 1) block = palette.footing;
                // A band at each floor line, which is what reads as a
                // storey from outside.
                else if (((y - ground) % 4) == 1) block = palette.footing;

                // A window in the middle of each wall, at head height
                // on every storey.
                if (((y - ground) % 4) == 3 && !(edgeX && edgeZ))
                {
                    const bool midOfX = (x == (x0 + x1) / 2);
                    const bool midOfZ = (z == (z0 + z1) / 2);
                    if ((edgeZ && midOfX) || (edgeX && midOfZ)) block = Blocks::Glass;
                }

                put(chunk, x, y, z, block);
            }

    // --- the roof ---------------------------------------------------
    //
    // A gable of stairs rather than the flat plank lid this used to
    // have, which read as a table rather than a house. The slope runs
    // across the shorter side, and the stairs face their own eave --
    // "facing" is the low side, the way you walk up.

    const int rx0 = x0 - 1, rx1 = x1 + 1;
    const int rz0 = z0 - 1, rz1 = z1 + 1;

    if (palette.style == RoofStyle::Flat)
    {
        // A flat lid with a parapet standing a block proud of it, which
        // is what a desert village looks like and the one case where a
        // flat roof is right rather than lazy.
        for (int x = rx0; x <= rx1; ++x)
            for (int z = rz0; z <= rz1; ++z)
                put(chunk, x, roofBase, z, palette.roof);

        for (int x = rx0; x <= rx1; ++x)
        {
            put(chunk, x, roofBase + 1, rz0, palette.wall);
            put(chunk, x, roofBase + 1, rz1, palette.wall);
        }
        for (int z = rz0; z <= rz1; ++z)
        {
            put(chunk, rx0, roofBase + 1, z, palette.wall);
            put(chunk, rx1, roofBase + 1, z, palette.wall);
        }
    }
    else
    {
        const bool ridgeAlongX = (rz1 - rz0) <= (rx1 - rx0);

        const int lowEave = ridgeAlongX ? rz0 : rx0;
        const int highEave = ridgeAlongX ? rz1 : rx1;
        const int acrossFrom = ridgeAlongX ? rx0 : rz0;
        const int acrossTo = ridgeAlongX ? rx1 : rz1;

        const int span = highEave - lowEave + 1;
        const int steps = span / 2;

        const BlockId lowStair = static_cast<BlockId>(palette.roofStairs + (ridgeAlongX ? 0 : 3));
        const BlockId highStair = static_cast<BlockId>(palette.roofStairs + (ridgeAlongX ? 2 : 1));

        for (int i = 0; i < steps; ++i)
        {
            const int y = roofBase + i;
            for (int across = acrossFrom; across <= acrossTo; ++across)
            {
                if (ridgeAlongX)
                {
                    put(chunk, across, y, lowEave + i, lowStair);
                    put(chunk, across, y, highEave - i, highStair);
                }
                else
                {
                    put(chunk, lowEave + i, y, across, lowStair);
                    put(chunk, highEave - i, y, across, highStair);
                }
            }
        }

        // An odd span leaves one row in the middle with no pair: that is
        // the ridge, and it is a solid block rather than a stair.
        if (span % 2 == 1)
        {
            const int y = roofBase + steps;
            for (int across = acrossFrom; across <= acrossTo; ++across)
            {
                if (ridgeAlongX) put(chunk, across, y, lowEave + steps, palette.roof);
                else             put(chunk, lowEave + steps, y, across, palette.roof);
            }
        }

        // The gable ends: the triangle between the wall top and the
        // slope. Without it you can see straight into the roof from
        // either end.
        for (int along = (ridgeAlongX ? z0 : x0); along <= (ridgeAlongX ? z1 : x1); ++along)
        {
            const int fromEave = std::min(along - lowEave, highEave - along);
            const int top = roofBase + std::min(fromEave, steps);

            for (int y = eaves + 1; y < top; ++y)
            {
                if (ridgeAlongX)
                {
                    put(chunk, x0, y, along, palette.wall);
                    put(chunk, x1, y, along, palette.wall);
                }
                else
                {
                    put(chunk, along, y, z0, palette.wall);
                    put(chunk, along, y, z1, palette.wall);
                }
            }
        }
    }

    // --- the doorway -------------------------------------------------

    int doorX = (x0 + x1) / 2;
    int doorZ = (z0 + z1) / 2;
    switch (building.facing)
    {
        case 0: doorZ = z0; break;   // looking north
        case 1: doorX = x1; break;
        case 2: doorZ = z1; break;
        default: doorX = x0; break;
    }

    put(chunk, doorX, ground + 1, doorZ, doorPart(building.facing, false, false));
    put(chunk, doorX, ground + 2, doorZ, doorPart(building.facing, true, false));

    // --- what is inside ----------------------------------------------

    stampInterior(chunk, building, ground, x0, z0, x1, z1, doorX, doorZ, palette);
}

// The furniture. Each trade gets the blocks it works with and a chest,
// and the chest is what --villageinfo and the loot table hang off: the
// kind of the building it stands in decides what is in it.
void WorldGen::stampInterior(Chunk& chunk, const Village::Building& building, int ground,
                             int x0, int z0, int x1, int z1, int doorX, int doorZ,
                             const Palette& palette) const
{
    // Corners, in a fixed order, skipping any that the door opens onto.
    const int cornerX[4] = { x0 + 1, x1 - 1, x0 + 1, x1 - 1 };
    const int cornerZ[4] = { z0 + 1, z0 + 1, z1 - 1, z1 - 1 };

    auto free = [&](int i)
    {
        return !(std::abs(cornerX[i] - doorX) <= 1 && std::abs(cornerZ[i] - doorZ) <= 1);
    };

    int slot = 0;
    auto nextCorner = [&](int& outX, int& outZ)
    {
        while (slot < 4 && !free(slot)) ++slot;
        if (slot >= 4) { outX = x0 + 1; outZ = z0 + 1; return; }
        outX = cornerX[slot];
        outZ = cornerZ[slot];
        ++slot;
    };

    int cx = 0, cz = 0;

    // Light first, in a corner, so every house has some whatever else
    // ends up in it.
    nextCorner(cx, cz);
    put(chunk, cx, ground + 1, cz, Blocks::Torch);

    // Then a bed. Every house has one, because a village counts its
    // beds: that is what decides how many people can live in it and
    // whether there is room for another.
    nextCorner(cx, cz);
    put(chunk, cx, ground + 1, cz, Blocks::Bed);

    switch (building.kind)
    {
        case Village::Kind::Blacksmith:
        {
            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::Furnace);
            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::Anvil);
            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::Chest);
            break;
        }

        case Village::Kind::Library:
        {
            // Shelves down both long walls, with the table in the middle
            // where the shelves can reach it.
            for (int x = x0 + 1; x <= x1 - 1; ++x)
            {
                if (x == doorX) continue;
                put(chunk, x, ground + 1, z0 + 1, Blocks::Bookshelf);
                put(chunk, x, ground + 1, z1 - 1, Blocks::Bookshelf);
            }

            put(chunk, (x0 + x1) / 2, ground + 1, (z0 + z1) / 2, Blocks::EnchantingTable);

            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::Chest);
            break;
        }

        case Village::Kind::Butcher:
        {
            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::Furnace);
            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::Chest);
            break;
        }

        default:
        {
            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::CraftingTable);
            nextCorner(cx, cz);
            put(chunk, cx, ground + 1, cz, Blocks::Chest);
            break;
        }
    }
}

WorldGen::VillageTally WorldGen::surveyVillages(int fromX, int fromZ, int squares) const
{
    VillageTally tally;

    const int centreX = Village::regionOf(fromX);
    const int centreZ = Village::regionOf(fromZ);

    for (int rz = centreZ - squares; rz <= centreZ + squares; ++rz)
        for (int rx = centreX - squares; rx <= centreX + squares; ++rx)
        {
            const Village::Site site = Village::siteFor(rx, rz, m_seed);
            if (!site.exists) continue;
            ++tally.sites;

            const Biome biome = biomeAt(site.x, site.z);
            if (biome != Biome::Plains && biome != Biome::Desert &&
                biome != Biome::Forest && biome != Biome::Snowy &&
                biome != Biome::Savanna)
            { ++tally.wrongBiome; continue; }

            int lowest = Chunk::SY;
            int highest = 0;
            for (int dz = -Village::MAX_RADIUS; dz <= Village::MAX_RADIUS; dz += 7)
                for (int dx = -Village::MAX_RADIUS; dx <= Village::MAX_RADIUS; dx += 7)
                {
                    const int here = surfaceHeight(site.x + dx, site.z + dz);
                    lowest = std::min(lowest, here);
                    highest = std::max(highest, here);
                }

            if (lowest <= SEA_LEVEL + 1) { ++tally.tooWet; continue; }
            if (highest - lowest > 12) { ++tally.tooSteep; continue; }
            ++tally.built;
        }

    return tally;
}

Village::Site WorldGen::nearestVillage(int fromX, int fromZ, int squares) const
{
    Village::Site best;
    long long bestDistance = 0;

    const int centreX = Village::regionOf(fromX);
    const int centreZ = Village::regionOf(fromZ);

    for (int rz = centreZ - squares; rz <= centreZ + squares; ++rz)
        for (int rx = centreX - squares; rx <= centreX + squares; ++rx)
        {
            const Village::Site site = Village::siteFor(rx, rz, m_seed);
            if (!villageFits(site)) continue;

            const long long dx = site.x - fromX;
            const long long dz = site.z - fromZ;
            const long long distance = dx * dx + dz * dz;

            if (!best.exists || distance < bestDistance)
            {
                best = site;
                bestDistance = distance;
            }
        }

    return best;
}

// The paths. They run along the gaps between plots, which is where the
// houses already leave room, so a village reads as streets with
// buildings on them rather than buildings on a field. Laid before the
// buildings, so anything that stands on a path simply covers it.
void WorldGen::layPaths(Chunk& chunk, const Village::Site& site,
                        int fromX, int fromZ, int toX, int toZ) const
{
    const int half = Village::GRID / 2;
    const int reach = Village::PLOT * half + Village::PLOT / 2;

    auto lay = [&](int x, int z)
    {
        if (x < fromX || x > toX || z < fromZ || z > toZ) return;

        const int top = surfaceHeight(x, z);

        const int lx = x - chunk.position().x * Chunk::SX;
        const int lz = z - chunk.position().z * Chunk::SZ;
        if (!Chunk::inBounds(lx, top, lz)) return;

        // Only over ground somebody would walk on. A path laid across
        // water or a stone outcrop read as a bug rather than a road.
        const BlockId here = chunk.getBlock(lx, top, lz);
        if (here != Blocks::Grass && here != Blocks::Dirt &&
            here != Blocks::Sand && here != Blocks::SnowGrass)
            return;

        put(chunk, x, top, z, Blocks::DirtPath);
        put(chunk, x, top + 1, z, Blocks::Air);
    };

    // One line down each boundary between plot columns, and one across
    // each boundary between plot rows. Two blocks wide, which is what
    // reads as a street rather than a line somebody scratched.
    for (int g = -half; g <= half; ++g)
    {
        const int lineX = site.x + g * Village::PLOT + Village::PLOT / 2;
        const int lineZ = site.z + g * Village::PLOT + Village::PLOT / 2;

        for (int along = -reach; along <= reach; ++along)
        {
            lay(lineX, site.z + along);
            lay(lineX - 1, site.z + along);

            lay(site.x + along, lineZ);
            lay(site.x + along, lineZ - 1);
        }
    }

    // And a square round the well, so the middle of the village is a
    // place rather than a crossing.
    for (int dx = -4; dx <= 4; ++dx)
        for (int dz = -4; dz <= 4; ++dz)
            lay(site.x + dx, site.z + dz);

    // A spur from every door out to the street. Without it the doors
    // open onto grass and the paths read as laid around the village
    // rather than through it.
    for (const Village::Building& building : Village::plan(site, m_seed))
    {
        if (!Village::isHouse(building.kind) && building.kind != Village::Kind::Farm)
            continue;

        const int bx0 = building.x;
        const int bz0 = building.z;
        const int bx1 = bx0 + building.width - 1;
        const int bz1 = bz0 + building.depth - 1;

        int doorX = (bx0 + bx1) / 2;
        int doorZ = (bz0 + bz1) / 2;
        int stepX = 0, stepZ = 0;

        switch (building.facing)
        {
            case 0:  doorZ = bz0; stepZ = -1; break;   // the door looks north
            case 1:  doorX = bx1; stepX = 1;  break;
            case 2:  doorZ = bz1; stepZ = 1;  break;
            default: doorX = bx0; stepX = -1; break;
        }

        // Far enough to reach the street in the gap, and no further.
        for (int out = 1; out <= Village::PLOT / 2 + 1; ++out)
            lay(doorX + stepX * out, doorZ + stepZ * out);
    }
}

// A chamber is cut into the rock rather than built on it: the rooms are
// hollowed out of whatever is already there and lined with tuff brick,
// so one under a mountain and one under a plain are the same chamber.
void WorldGen::buildTrialChambers(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;
    const int maxX = baseX + Chunk::SX - 1;
    const int maxZ = baseZ + Chunk::SZ - 1;

    const int reach = TrialChamber::MAX_RADIUS + 1;
    const int firstX = TrialChamber::regionOf(baseX - reach);
    const int lastX = TrialChamber::regionOf(maxX + reach);
    const int firstZ = TrialChamber::regionOf(baseZ - reach);
    const int lastZ = TrialChamber::regionOf(maxZ + reach);

    for (int rz = firstZ; rz <= lastZ; ++rz)
        for (int rx = firstX; rx <= lastX; ++rx)
        {
            const TrialChamber::Site site = TrialChamber::siteFor(rx, rz, m_seed);
            if (!TrialChamber::reaches(site, baseX - 1, baseZ - 1, maxX + 1, maxZ + 1))
                continue;

            for (const TrialChamber::Room& room : TrialChamber::plan(site, m_seed))
                stampChamberRoom(chunk, site, room);

            // The way down. A chamber nobody can find is a chamber that
            // may as well not be there, so the middle room carries a
            // shaft to the surface with a ladder up one wall.
            const int shaftX = site.x;
            const int shaftZ = site.z;
            const int lx = shaftX - baseX;
            const int lz = shaftZ - baseZ;
            if (!Chunk::inBounds(lx, 0, lz)) continue;

            // Under the sea the shaft would open into the water and
            // flood nothing in particular, since nothing here flows --
            // but a ladder coming out of the sea bed is still wrong. A
            // chamber under water is simply sealed, and found the way
            // anything else underground is found.
            const int top = surfaceHeight(shaftX, shaftZ);
            if (top <= SEA_LEVEL) continue;

            // The shaft is one block across, so the rock around it is
            // the wall the ladder hangs on.
            for (int y = site.y + 1; y <= top && y < Chunk::SY; ++y)
                chunk.setBlockRaw(lx, y, lz, Blocks::LadderNorth);
        }
}

void WorldGen::stampChamberRoom(Chunk& chunk, const TrialChamber::Site& site,
                                const TrialChamber::Room& room) const
{
    using namespace TrialChamber;

    const int floor = site.y - 1;
    const int ceiling = site.y + HEADROOM;

    // Lined, then hollowed. Cutting the room first and lining it after
    // would leave the lining inside the room.
    for (int x = room.x - 1; x <= room.x + room.width; ++x)
        for (int z = room.z - 1; z <= room.z + room.depth; ++z)
            for (int y = floor; y <= ceiling; ++y)
                put(chunk, x, y, z, Blocks::TuffBricks);

    for (int x = room.x; x < room.x + room.width; ++x)
        for (int z = room.z; z < room.z + room.depth; ++z)
            for (int y = site.y; y < ceiling; ++y)
                put(chunk, x, y, z, Blocks::Air);

    // A doorway through every wall that has a room on the other side.
    // Cutting all four would open the outermost rooms into solid rock.
    constexpr int HALF = GRID / 2;
    const int midX = room.x + room.width / 2;
    const int midZ = room.z + room.depth / 2;

    auto doorway = [&](int x, int z, bool alongX) {
        for (int step = -1; step <= 1; ++step)
            for (int y = site.y; y <= site.y + 2; ++y)
                put(chunk, alongX ? x : x + step, y, alongX ? z + step : z, Blocks::Air);
    };

    if (room.gx > -HALF) doorway(room.x - 1, midZ, true);
    if (room.gx < HALF)  doorway(room.x + room.width, midZ, true);
    if (room.gz > -HALF) doorway(midX, room.z - 1, false);
    if (room.gz < HALF)  doorway(midX, room.z + room.depth, false);

    // Light, so a chamber is somewhere you can see what is coming.
    for (int cx = 0; cx < 2; ++cx)
        for (int cz = 0; cz < 2; ++cz)
            put(chunk, room.x + (cx ? room.width - 1 : 0), site.y + HEADROOM - 1,
                room.z + (cz ? room.depth - 1 : 0), Blocks::Glowstone);

    switch (room.kind)
    {
        case Kind::Spawner:
            put(chunk, midX, site.y, midZ, Blocks::TrialSpawner);
            break;

        case Kind::Vault:
            // Against the north wall, which is the furthest thing from
            // the way in however you came. The ominous one stands
            // between the two plain ones: you can see what you are
            // missing from the first time you walk in.
            put(chunk, midX - 2, site.y, room.z, Blocks::Vault);
            put(chunk, midX, site.y, room.z, Blocks::OminousVault);
            put(chunk, midX + 2, site.y, room.z, Blocks::Vault);
            break;

        case Kind::Supply:
            put(chunk, midX, site.y, midZ, Blocks::Chest);
            break;

        case Kind::Entrance:
        case Kind::Empty:
            break;
    }
}

TrialChamber::Site WorldGen::nearestChamber(int fromX, int fromZ, int squares) const
{
    TrialChamber::Site best;
    long long bestDistance = 0;

    const int centreX = TrialChamber::regionOf(fromX);
    const int centreZ = TrialChamber::regionOf(fromZ);

    for (int rz = centreZ - squares; rz <= centreZ + squares; ++rz)
        for (int rx = centreX - squares; rx <= centreX + squares; ++rx)
        {
            const TrialChamber::Site site = TrialChamber::siteFor(rx, rz, m_seed);
            if (!site.exists) continue;

            const long long dx = site.x - fromX;
            const long long dz = site.z - fromZ;
            const long long distance = dx * dx + dz * dz;

            if (!best.exists || distance < bestDistance)
            {
                best = site;
                bestDistance = distance;
            }
        }

    return best;
}

Stronghold::Site WorldGen::nearestStronghold(int fromX, int fromZ) const
{
    return Stronghold::nearest(fromX, fromZ, m_seed);
}

// The one structure the game is built round finding. Cut into the rock
// like a trial chamber, with a ring of end portal frames in the middle
// room over a pool of lava -- some of the eyes already set, the rest
// yours to find.
void WorldGen::buildStrongholds(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;
    const int maxX = baseX + Chunk::SX - 1;
    const int maxZ = baseZ + Chunk::SZ - 1;

    const Stronghold::Site site =
        Stronghold::touching(baseX - 1, baseZ - 1, maxX + 1, maxZ + 1, m_seed);
    if (!site.exists) return;

    for (const Stronghold::Room& room : Stronghold::plan(site, m_seed))
        stampStrongholdRoom(chunk, site, room);

    // A shaft up, the same as a chamber's: a stronghold nobody can
    // find is the End nobody can reach.
    const int lx = site.x - baseX;
    const int lz = site.z - baseZ;
    if (!Chunk::inBounds(lx, 0, lz)) return;

    const int top = surfaceHeight(site.x, site.z);
    if (top <= SEA_LEVEL) return;

    // Clear of the portal itself, so the shaft does not come down
    // through the middle of the ring.
    const int shaftX = site.x + 3;
    const int sx = shaftX - baseX;
    if (!Chunk::inBounds(sx, 0, lz)) return;

    const int shaftTop = surfaceHeight(shaftX, site.z);
    if (shaftTop <= SEA_LEVEL) return;

    for (int y = site.y + 1; y <= shaftTop && y < Chunk::SY; ++y)
        chunk.setBlockRaw(sx, y, lz, Blocks::LadderNorth);
}

void WorldGen::stampStrongholdRoom(Chunk& chunk, const Stronghold::Site& site,
                                   const Stronghold::Room& room) const
{
    using namespace Stronghold;

    const int floor = site.y - 1;
    const int ceiling = site.y + HEADROOM;

    for (int x = room.x - 1; x <= room.x + room.width; ++x)
        for (int z = room.z - 1; z <= room.z + room.depth; ++z)
            for (int y = floor; y <= ceiling; ++y)
                put(chunk, x, y, z, Blocks::Cobblestone);

    for (int x = room.x; x < room.x + room.width; ++x)
        for (int z = room.z; z < room.z + room.depth; ++z)
            for (int y = site.y; y < ceiling; ++y)
                put(chunk, x, y, z, Blocks::Air);

    constexpr int HALF = GRID / 2;
    const int midX = room.x + room.width / 2;
    const int midZ = room.z + room.depth / 2;

    auto doorway = [&](int x, int z, bool alongX) {
        for (int step = -1; step <= 1; ++step)
            for (int y = site.y; y <= site.y + 2; ++y)
                put(chunk, alongX ? x : x + step, y, alongX ? z + step : z, Blocks::Air);
    };

    if (room.gx > -HALF) doorway(room.x - 1, midZ, true);
    if (room.gx < HALF)  doorway(room.x + room.width, midZ, true);
    if (room.gz > -HALF) doorway(midX, room.z - 1, false);
    if (room.gz < HALF)  doorway(midX, room.z + room.depth, false);

    for (int cx = 0; cx < 2; ++cx)
        for (int cz = 0; cz < 2; ++cz)
            put(chunk, room.x + (cx ? room.width - 1 : 0), site.y + HEADROOM - 1,
                room.z + (cz ? room.depth - 1 : 0), Blocks::Torch);

    switch (room.kind)
    {
        case Kind::Portal:
        {
            // Lava under the ring, which is Minecraft's, and the
            // twelve frames round the three by three above it.
            for (int dx = -2; dx <= 2; ++dx)
                for (int dz = -2; dz <= 2; ++dz)
                    put(chunk, midX + dx, site.y - 1, midZ + dz, Blocks::Lava);

            int offsets[EndPortal::FRAME_COUNT][2];
            EndPortal::frameOffsets(offsets);

            for (int i = 0; i < EndPortal::FRAME_COUNT; ++i)
            {
                const BlockId frame = Stronghold::eyeInFrame(site, m_seed, i)
                                          ? Blocks::EndPortalFrameFilled
                                          : Blocks::EndPortalFrame;
                put(chunk, midX + offsets[i][0], site.y, midZ + offsets[i][1], frame);
            }
            break;
        }

        case Kind::Library:
            for (int dx = -2; dx <= 2; ++dx)
                for (int dy = 0; dy < 2; ++dy)
                {
                    put(chunk, midX + dx, site.y + dy, room.z, Blocks::Bookshelf);
                    put(chunk, midX + dx, site.y + dy, room.z + room.depth - 1,
                        Blocks::Bookshelf);
                }
            break;

        case Kind::Store:
            put(chunk, midX, site.y, midZ, Blocks::Chest);
            break;

        case Kind::Hall:
            break;
    }
}

void WorldGen::buildVillages(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;
    const int maxX = baseX + Chunk::SX - 1;
    const int maxZ = baseZ + Chunk::SZ - 1;

    // Every square whose village could reach in here. The roof hangs a
    // block past the walls, so the search is widened by one.
    const int reach = Village::MAX_RADIUS + 1;
    const int firstX = Village::regionOf(baseX - reach);
    const int lastX = Village::regionOf(maxX + reach);
    const int firstZ = Village::regionOf(baseZ - reach);
    const int lastZ = Village::regionOf(maxZ + reach);

    for (int rz = firstZ; rz <= lastZ; ++rz)
        for (int rx = firstX; rx <= lastX; ++rx)
        {
            const Village::Site site = Village::siteFor(rx, rz, m_seed);
            if (!Village::reaches(site, baseX - 1, baseZ - 1, maxX + 1, maxZ + 1)) continue;
            if (!villageFits(site)) continue;

            // Clear the site first. decorate() has already scattered
            // trees over it without knowing a village was coming, and
            // a house with an oak growing through the roof is not a
            // house. Only this chunk's share of the footprint, since
            // the rest is some other chunk's to clear.
            const int clearX0 = std::max(baseX, site.x - Village::MAX_RADIUS);
            const int clearX1 = std::min(maxX, site.x + Village::MAX_RADIUS);
            const int clearZ0 = std::max(baseZ, site.z - Village::MAX_RADIUS);
            const int clearZ1 = std::min(maxZ, site.z + Village::MAX_RADIUS);

            for (int x = clearX0; x <= clearX1; ++x)
                for (int z = clearZ0; z <= clearZ1; ++z)
                {
                    const int top = surfaceHeight(x, z);
                    for (int y = top + 1; y <= top + 9; ++y)
                        put(chunk, x, y, z, Blocks::Air);
                }

            // One palette for the whole village, from the biome at its
            // middle: a village with half its houses in sandstone and
            // half in oak would read as two villages that collided.
            const Palette palette = paletteFor(biomeAt(site.x, site.z));

            layPaths(chunk, site, clearX0, clearZ0, clearX1, clearZ1);

            for (const Village::Building& building : Village::plan(site, m_seed))
                stampBuilding(chunk, building, palette);
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
                // Thicker than anywhere else, and a savanna is nearly
                // bare: the two read as what they are from the density
                // of the trees before you are close enough to see one.
                case Biome::Jungle: treeChance = 0.085f; break;
                case Biome::Savanna: treeChance = 0.010f; break;
                default: break;
            }
            if (treeChance <= 0.0f) continue;

            if (hashToFloat(hashCoords(wx, 7, wz, m_seed + 4242)) >= treeChance) continue;

            const int surface = surfaceHeight(wx, wz);
            if (surface <= SEA_LEVEL) continue; // no trees in water

            // Which tree grows here is the biome's business. Spruce in
            // the cold and on the heights, dark oak in the thick of the
            // forest, oak and birch everywhere else.
            const Biome here = biomeAt(wx, wz);
            const float roll = hashToFloat(hashCoords(wx, 8, wz, m_seed + 4243));

            TreeKind kind = TreeKind::Oak;
            if (here == Biome::Jungle)
                kind = (roll < 0.85f) ? TreeKind::Jungle : TreeKind::Oak;
            else if (here == Biome::Savanna)
                kind = TreeKind::Acacia;
            else if (here == Biome::Snowy || here == Biome::Mountains)
                kind = (roll < 0.80f) ? TreeKind::Spruce : TreeKind::Oak;
            else if (here == Biome::Forest)
                kind = (roll < 0.30f) ? TreeKind::DarkOak
                     : (roll < 0.55f) ? TreeKind::Birch
                                      : TreeKind::Oak;
            else
                kind = (roll < 0.35f) ? TreeKind::Birch : TreeKind::Oak;

            buildTree(chunk, wx, surface, wz, kind);
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

            // Sugar cane grows at the waterline, and only where it
            // can reach the water: one of the four neighbouring
            // columns has to stand below sea level. Checked through
            // surfaceHeight rather than by reading blocks, because the
            // neighbour may be in a chunk that does not exist yet.
            if (ground == Blocks::Sand || ground == Blocks::Grass || ground == Blocks::Dirt)
            {
                const bool besideWater =
                    surfaceHeight(wx - 1, wz) < SEA_LEVEL ||
                    surfaceHeight(wx + 1, wz) < SEA_LEVEL ||
                    surfaceHeight(wx, wz - 1) < SEA_LEVEL ||
                    surfaceHeight(wx, wz + 1) < SEA_LEVEL;

                if (besideWater && surface <= SEA_LEVEL + 1 &&
                    hashToFloat(hashCoords(wx, 5, wz, m_seed + 991)) < 0.26f)
                {
                    const int canes = 1 + static_cast<int>(
                        hashToFloat(hashCoords(wx, 6, wz, m_seed + 992)) * 3.0f);
                    for (int i = 0; i < canes && above + i < Chunk::SY; ++i)
                        chunk.setBlockRaw(lx, above + i, lz, Blocks::SugarCane);
                    continue;
                }
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
    // Perlin noise is exactly zero at every lattice point, the origin
    // among them, so the block at 0,0 is the same height in every world
    // ever generated. Spiralling out from there handed every seed an
    // identical starting hill, so the search starts somewhere the seed
    // chooses instead.
    // Stirred rather than sliced: the top half of a small seed is all
    // zeroes, so taking the halves straight gave every seed under four
    // thousand the same starting line of longitude.
    uint32_t h = m_seed * 0x9E3779B9u;
    h ^= h >> 16; h *= 0x7FEB352Du;
    h ^= h >> 15; h *= 0x846CA68Bu;
    h ^= h >> 16;

    const int startX = static_cast<int>(h % 4096u) - 2048;
    const int startZ = static_cast<int>((h >> 16) % 4096u) - 2048;

    // Spiral outwards from there until we find dry land.
    for (int radius = 0; radius < 400; radius += 8)
    {
        for (int angleStep = 0; angleStep < 16; ++angleStep)
        {
            float angle = angleStep * 0.3927f; // 2*pi/16
            int x = startX + static_cast<int>(std::cos(angle) * radius);
            int z = startZ + static_cast<int>(std::sin(angle) * radius);
            int height = surfaceHeight(x, z);
            Biome biome = biomeAt(x, z);
            if (height <= SEA_LEVEL + 1 || biome == Biome::Ocean) continue;

            // One dry block is not a place to start: a sand bar a block
            // wide passes that test, and the first world it picked put
            // the player on one in the middle of an ocean. Ask for land
            // all around as well, out far enough to be somewhere.
            bool roomToStand = true;
            for (int dz = -24; dz <= 24 && roomToStand; dz += 12)
                for (int dx = -24; dx <= 24 && roomToStand; dx += 12)
                    if (surfaceHeight(x + dx, z + dz) <= SEA_LEVEL) roomToStand = false;

            if (roomToStand)
            {
                outX = x;
                outY = height + 2;
                outZ = z;
                return;
            }
        }
    }

    outX = startX;
    outY = SEA_LEVEL + 4;
    outZ = startZ;
}
