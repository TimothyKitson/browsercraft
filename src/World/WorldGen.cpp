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
        case Biome::Swamp:   return "Swamp";
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

    // The swamp is tested before the water is, which is the only way
    // it can work: a swamp has to own both sides of the waterline.
    // Tested after it, Ocean and Beach take everything at or below sea
    // level and a swamp is left as the one row of columns a block
    // above -- dry land with trees on it, which photographs as an
    // ordinary forest. The depth floor is what keeps swamps out of the
    // middle of the sea: only the shallows are allowed to go swampy.
    if (height >= SEA_LEVEL - 4 && height <= SEA_LEVEL + 2 &&
        temperature > -0.10f && humidity > 0.14f)
        return Biome::Swamp;

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
            buildNetherFortresses(chunk);
            buildBastions(chunk);
            return;
        case Dimension::End:
            generateEnd(chunk);
            buildEndPillars(chunk);
            buildEndCities(chunk);
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
    buildMineshafts(chunk);
    buildLandmarks(chunk);
    buildDesertWells(chunk);
    buildGeodes(chunk);
    buildAncientCities(chunk);
    dressSeabed(chunk);
    buildOceanStructures(chunk);
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

                        case Biome::Swamp:
                            // Mud where the water stands over it,
                            // grass on the tussocks that are clear of
                            // it. A swamp that was grass all through
                            // read as a flooded meadow.
                            if (depth == 0)
                                block = (surface <= SEA_LEVEL) ? Blocks::Mud : Blocks::Grass;
                            else if (depth <= 2) block = Blocks::Mud;
                            else if (depth <= 4) block = Blocks::Dirt;
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

    // Every chest in a landmark goes down through here, off the one
    // table that also says where to look for them again.
    void putChests(Chunk& chunk, WorldGen::Landmark kind,
                   int midX, int ground, int midZ)
    {
        WorldGen::ChestSpot spots[2];
        const int many = WorldGen::landmarkChests(kind, spots);
        for (int i = 0; i < many; ++i)
            put(chunk, midX + spots[i].dx, ground + spots[i].dy, midZ + spots[i].dz,
                Blocks::Chest);
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

        case Biome::Swamp:
            // Spruce, dark and damp-looking, which is what a building
            // standing in water wants to be made of.
            palette.wall = Blocks::SprucePlanks;
            palette.footing = Blocks::MossyCobblestone;
            palette.corner = Blocks::SpruceLog;
            palette.floor = Blocks::SprucePlanks;
            palette.roof = Blocks::SprucePlanks;
            palette.post = Blocks::SpruceLog;
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

                // One crop per bed rather than per column, so each half
                // of a field reads as a planted thing. Minecraft sows a
                // village farm the same way: a bed at a time.
                static const BlockId CROPS[] = { Blocks::Wheat0, Blocks::Carrot0,
                                                 Blocks::Potato0 };
                const BlockId crop = CROPS[hashCoords(x0, (z < midZ) ? 0 : 1, z0,
                                                      m_seed + 55041) % 3u];
                put(chunk, x, ground + 1, z, cropAtStage(crop, stage));
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

const char* WorldGen::landmarkName(Landmark kind)
{
    switch (kind)
    {
        case Landmark::DesertPyramid:   return "Desert pyramid";
        case Landmark::JungleTemple:    return "Jungle temple";
        case Landmark::Igloo:           return "Igloo";
        case Landmark::RuinedPortal:    return "Ruined portal";
        case Landmark::PillagerOutpost: return "Pillager outpost";
        case Landmark::WoodlandMansion: return "Woodland mansion";
        case Landmark::SwampHut:        return "Swamp hut";
        case Landmark::Count:           break;
    }
    return "";
}

const Structures::Grid& WorldGen::landmarkGrid(Landmark kind)
{
    switch (kind)
    {
        case Landmark::JungleTemple:    return Structures::JUNGLE_TEMPLE;
        case Landmark::Igloo:           return Structures::IGLOO;
        case Landmark::RuinedPortal:    return Structures::RUINED_PORTAL;
        case Landmark::PillagerOutpost: return Structures::PILLAGER_POST;
        case Landmark::WoodlandMansion: return Structures::WOODLAND_MANSION;
        case Landmark::SwampHut:        return Structures::SWAMP_HUT;
        default:                        return Structures::DESERT_PYRAMID;
    }
}

// Which land each one belongs on. Minecraft decides this after it has
// chosen the place rather than before, which is why a desert with no
// pyramid in it is a desert that drew one somewhere there was no sand.
bool WorldGen::landmarkSuits(Landmark kind, Biome biome)
{
    switch (kind)
    {
        case Landmark::DesertPyramid:   return biome == Biome::Desert;
        case Landmark::JungleTemple:    return biome == Biome::Jungle;
        case Landmark::Igloo:           return biome == Biome::Snowy;
        case Landmark::PillagerOutpost:
            return biome == Biome::Plains || biome == Biome::Savanna ||
                   biome == Biome::Desert;
        case Landmark::SwampHut:
            return biome == Biome::Swamp;
        case Landmark::WoodlandMansion:
            // Minecraft puts these in a dark forest, which is a biome
            // this world does not have. The ordinary forest is the
            // nearest thing to it, and the grid is Minecraft's own --
            // 80 apart, which is why finding one is a journey.
            return biome == Biome::Forest;
        case Landmark::RuinedPortal:
            // Anywhere with ground to stand on, which is the point of
            // it: a ruined portal is the one you find before you know
            // what you are looking at.
            return biome != Biome::Ocean;
        case Landmark::Count: break;
    }
    return false;
}

// Whether a landmark of this kind can actually stand here.
//
// This has to be the one test both the search and the stamp use. When
// they were two tests they disagreed: --structureinfo walked to the
// nearest ruined portal on two seeds out of four and found nothing
// there, because the search only asked about the biome and the stamp
// also refused to build below the sea. A search that points at a place
// nothing was built is worse than no search.
bool WorldGen::landmarkStands(Landmark kind, int midX, int midZ) const
{
    if (!landmarkSuits(kind, biomeAt(midX, midZ))) return false;
    return surfaceHeight(midX, midZ) > SEA_LEVEL;
}

Structures::Spot WorldGen::nearestLandmark(Landmark kind, int blockX, int blockZ,
                                           int squares) const
{
    const Structures::Grid& grid = landmarkGrid(kind);
    const int chunkX = (blockX >= 0) ? blockX / Structures::CHUNK
                                     : -(((-blockX) + Structures::CHUNK - 1) / Structures::CHUNK);
    const int chunkZ = (blockZ >= 0) ? blockZ / Structures::CHUNK
                                     : -(((-blockZ) + Structures::CHUNK - 1) / Structures::CHUNK);

    const int centreX = Structures::squareOf(chunkX, grid.spacing);
    const int centreZ = Structures::squareOf(chunkZ, grid.spacing);

    Structures::Spot best;
    long long bestDistance = 0;

    for (int sz = centreZ - squares; sz <= centreZ + squares; ++sz)
        for (int sx = centreX - squares; sx <= centreX + squares; ++sx)
        {
            const Structures::Spot spot = Structures::inSquare(grid, sx, sz, m_seed);
            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            if (!landmarkStands(kind, midX, midZ)) continue;

            const long long dx = static_cast<long long>(midX) - blockX;
            const long long dz = static_cast<long long>(midZ) - blockZ;
            const long long distance = dx * dx + dz * dz;

            if (!best.exists || distance < bestDistance)
            {
                best = spot;
                bestDistance = distance;
            }
        }

    return best;
}

void WorldGen::buildLandmarks(Chunk& chunk) const
{
    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    // None of them reaches more than a chunk or so past its middle.
    constexpr int REACH_CHUNKS = 2;

    for (int k = 0; k < static_cast<int>(Landmark::Count); ++k)
    {
        const Landmark kind = static_cast<Landmark>(k);
        const Structures::Grid& grid = landmarkGrid(kind);

        const int firstX = Structures::squareOf(chunkX - REACH_CHUNKS, grid.spacing);
        const int lastX = Structures::squareOf(chunkX + REACH_CHUNKS, grid.spacing);
        const int firstZ = Structures::squareOf(chunkZ - REACH_CHUNKS, grid.spacing);
        const int lastZ = Structures::squareOf(chunkZ + REACH_CHUNKS, grid.spacing);

        for (int sz = firstZ; sz <= lastZ; ++sz)
            for (int sx = firstX; sx <= lastX; ++sx)
            {
                const Structures::Spot spot = Structures::inSquare(grid, sx, sz, m_seed);
                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

                if (std::abs(spot.chunkX - chunkX) > REACH_CHUNKS) continue;
                if (std::abs(spot.chunkZ - chunkZ) > REACH_CHUNKS) continue;
                if (!landmarkStands(kind, midX, midZ)) continue;

                stampLandmark(chunk, kind, midX, midZ);
            }
    }
}

int WorldGen::landmarkChests(Landmark kind, ChestSpot out[2])
{
    switch (kind)
    {
        case Landmark::DesertPyramid:
            out[0] = { -2, -2, -2 };
            out[1] = {  2, -2,  2 };
            return 2;
        case Landmark::JungleTemple:
            out[0] = {  0,  1,  3 };
            out[1] = { -3,  1,  3 };
            return 2;
        case Landmark::Igloo:
            out[0] = { -2,  0,  2 };
            return 1;
        case Landmark::RuinedPortal:
            out[0] = {  3,  0,  1 };
            return 1;
        case Landmark::PillagerOutpost:
            out[0] = {  0,  OUTPOST_TALL - 3,  0 };
            return 1;
        case Landmark::WoodlandMansion:
            out[0] = { -4,  1,  4 };
            out[1] = {  4,  7, -4 };
            return 2;
        case Landmark::SwampHut:
            out[0] = {  1,  5,  1 };
            return 1;
        case Landmark::Count:
            break;
    }
    return 0;
}

void WorldGen::stampLandmark(Chunk& chunk, Landmark kind, int midX, int midZ) const
{
    const int ground = surfaceHeight(midX, midZ);
    if (ground <= SEA_LEVEL) return;

    switch (kind)
    {
        case Landmark::DesertPyramid:
        {
            // Stepped sandstone, and a room cut into the ground under
            // it with the chests in it -- which is where everybody
            // digs, and why everybody knows the floor is a trap.
            constexpr int HALF = 9;
            for (int step = 0; step <= HALF; ++step)
            {
                const int radius = HALF - step;
                const int y = ground + step;
                for (int dx = -radius; dx <= radius; ++dx)
                    for (int dz = -radius; dz <= radius; ++dz)
                        put(chunk, midX + dx, y, midZ + dz, Blocks::Sandstone);
            }

            for (int dx = -3; dx <= 3; ++dx)
                for (int dz = -3; dz <= 3; ++dz)
                    for (int dy = 1; dy <= 4; ++dy)
                    {
                        const bool shell = std::abs(dx) == 3 || std::abs(dz) == 3 || dy == 1;
                        put(chunk, midX + dx, ground - dy, midZ + dz,
                            shell ? Blocks::Sandstone : Blocks::Air);
                    }

            putChests(chunk, Landmark::DesertPyramid, midX, ground, midZ);
            // The thing under the floor. Everybody finds it once.
            put(chunk, midX, ground - 4, midZ, Blocks::Tnt);
            break;
        }

        case Landmark::JungleTemple:
        {
            constexpr int HALF = 5;
            for (int dx = -HALF; dx <= HALF; ++dx)
                for (int dz = -HALF; dz <= HALF; ++dz)
                    for (int dy = 0; dy <= 6; ++dy)
                    {
                        const bool shell = std::abs(dx) == HALF || std::abs(dz) == HALF ||
                                           dy == 0 || dy == 6;
                        const bool door = std::abs(dx) <= 1 && dz == -HALF &&
                                          dy >= 1 && dy <= 2;
                        put(chunk, midX + dx, ground + dy, midZ + dz,
                            (shell && !door) ? Blocks::Cobblestone : Blocks::Air);
                    }

            putChests(chunk, Landmark::JungleTemple, midX, ground, midZ);
            // Levers on the wall, because a jungle temple is a puzzle
            // even when the puzzle is only a story.
            put(chunk, midX + 2, ground + 2, midZ + HALF - 1, Blocks::LeverOff);
            put(chunk, midX + 3, ground + 2, midZ + HALF - 1, Blocks::LeverOff);
            break;
        }

        case Landmark::Igloo:
        {
            constexpr int R = 4;
            for (int dx = -R; dx <= R; ++dx)
                for (int dy = 0; dy <= R; ++dy)
                    for (int dz = -R; dz <= R; ++dz)
                    {
                        const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));
                        if (d > R + 0.5f) continue;

                        const bool shell = d > R - 0.8f;
                        const bool door = dz == -R + 1 && std::abs(dx) <= 1 && dy <= 1;
                        put(chunk, midX + dx, ground + dy, midZ + dz,
                            (shell && !door) ? Blocks::Snow : Blocks::Air);
                    }

            put(chunk, midX + 1, ground, midZ + 1, Blocks::Bed);
            putChests(chunk, Landmark::Igloo, midX, ground, midZ);
            put(chunk, midX - 2, ground, midZ, Blocks::Furnace);
            break;
        }

        case Landmark::RuinedPortal:
        {
            // A frame with pieces missing, standing in a scatter of
            // netherrack. The gaps are the whole character of it: you
            // know what it is and you know it does not work.
            for (int dx = -2; dx <= 2; ++dx)
                for (int dy = 0; dy <= 4; ++dy)
                {
                    const bool frame = std::abs(dx) == 2 || dy == 0 || dy == 4;
                    if (!frame) continue;
                    if (hashToFloat(hashCoords(midX + dx, dy, midZ, m_seed + 9100)) < 0.3f)
                        continue;
                    put(chunk, midX + dx, ground + dy, midZ, Blocks::Obsidian);
                }

            for (int dx = -4; dx <= 4; ++dx)
                for (int dz = -4; dz <= 4; ++dz)
                {
                    if (hashToFloat(hashCoords(midX + dx, 1, midZ + dz, m_seed + 9101)) > 0.35f)
                        continue;
                    const int y = surfaceHeight(midX + dx, midZ + dz);
                    put(chunk, midX + dx, y, midZ + dz, Blocks::Netherrack);
                }

            putChests(chunk, Landmark::RuinedPortal, midX, ground, midZ);
            break;
        }

        case Landmark::PillagerOutpost:
        {
            // A tower of dark oak with a roof and a way up inside it.
            constexpr int HALF = 3;
            constexpr int TALL = OUTPOST_TALL;

            for (int dy = 0; dy <= TALL; ++dy)
            {
                // The top storey hangs a block out past the tower,
                // which is the shape anybody recognises one by. A
                // plain box of planks read as a slab.
                const int half = (dy >= TALL - 4) ? HALF + 1 : HALF;

                for (int dx = -half; dx <= half; ++dx)
                    for (int dz = -half; dz <= half; ++dz)
                    {
                        const bool wall = std::abs(dx) == half || std::abs(dz) == half;
                        const bool floor = dy == 0 || dy == TALL || dy == TALL - 4;
                        const bool door = wall && dz == -half && std::abs(dx) <= 1 &&
                                          dy >= 1 && dy <= 2;

                        // Slots to watch from, and to be shot through.
                        const bool window = wall && dy >= TALL - 3 && dy <= TALL - 2 &&
                                            ((std::abs(dx) % 2) == 0) &&
                                            ((std::abs(dz) % 2) == 0);

                        put(chunk, midX + dx, ground + dy, midZ + dz,
                            ((wall || floor) && !door && !window) ? Blocks::DarkOakPlanks
                                                                  : Blocks::Air);
                    }
            }

            // The ladder up, and the chest at the top of it.
            for (int dy = 1; dy < TALL; ++dy)
                put(chunk, midX + HALF - 1, ground + dy, midZ + HALF - 1, Blocks::LadderNorth);

            putChests(chunk, Landmark::PillagerOutpost, midX, ground, midZ);
            break;
        }

        case Landmark::WoodlandMansion:
        {
            // The big one. Minecraft's is enormous and randomly laid
            // out room by room; this is a manor of the same materials
            // and the same shape -- dark oak frame, cobblestone infill,
            // three floors and a roof -- rather than that generator
            // rebuilt from memory.
            constexpr int HALF = MANSION_HALF;
            constexpr int STOREY = 5;
            constexpr int STOREYS = 3;
            constexpr int TALL = STOREY * STOREYS;

            for (int dy = 0; dy <= TALL + 3; ++dy)
            {
                // The roof is a pyramid on top of the box, which is
                // most of what you recognise a mansion by from across
                // a forest.
                const bool roofing = dy > TALL;
                const int half = roofing ? HALF - (dy - TALL) : HALF;
                if (half < 1) continue;

                for (int dx = -half; dx <= half; ++dx)
                    for (int dz = -half; dz <= half; ++dz)
                    {
                        const int wx = midX + dx;
                        const int wz = midZ + dz;
                        const int y = ground + dy;

                        if (roofing)
                        {
                            const bool edge = std::abs(dx) == half || std::abs(dz) == half;
                            put(chunk, wx, y, wz, edge ? Blocks::DarkOakPlanks : Blocks::Air);
                            continue;
                        }

                        const bool outer = std::abs(dx) == half || std::abs(dz) == half;
                        const bool floor = (dy % STOREY) == 0;

                        // The inner walls that make it rooms rather
                        // than one hall, with doorways through them.
                        const bool spine = (dx == 0 || dz == 0) && !floor;
                        const bool doorway = spine && (dy % STOREY) <= 2 &&
                                             (std::abs(dx) + std::abs(dz)) % 7 < 3;

                        BlockId block = Blocks::Air;

                        if (outer)
                        {
                            // Dark oak at the corners and every fourth
                            // course, cobblestone between: the timbered
                            // look is what makes it a mansion and not a
                            // warehouse.
                            const bool corner = std::abs(dx) == half && std::abs(dz) == half;
                            const bool post = (std::abs(dx) % 4) == 0 || (std::abs(dz) % 4) == 0;
                            const bool window = !corner && !post &&
                                                (dy % STOREY) >= 2 && (dy % STOREY) <= 3;
                            const bool door = dz == -half && std::abs(dx) <= 1 &&
                                              dy >= 1 && dy <= 2;

                            if (door) block = Blocks::Air;
                            else if (window) block = Blocks::Glass;
                            else if (corner || post || floor) block = Blocks::DarkOakLog;
                            else block = Blocks::Cobblestone;
                        }
                        else if (floor)
                        {
                            block = Blocks::DarkOakPlanks;
                        }
                        else if (spine && !doorway)
                        {
                            block = Blocks::DarkOakPlanks;
                        }

                        put(chunk, wx, y, wz, block);
                    }
            }

            // A way between the floors, and a lamp on each landing so
            // the inside is not pitch dark.
            for (int dy = 1; dy < TALL; ++dy)
                put(chunk, midX + HALF - 1, ground + dy, midZ + HALF - 1,
                    Blocks::LadderNorth);

            for (int storey = 0; storey < STOREYS; ++storey)
            {
                const int y = ground + storey * STOREY + 2;
                put(chunk, midX + HALF - 2, y, midZ + HALF - 2, Blocks::Torch);
                put(chunk, midX - HALF + 2, y, midZ - HALF + 2, Blocks::Torch);
            }

            putChests(chunk, Landmark::WoodlandMansion, midX, ground, midZ);
            break;
        }

        case Landmark::SwampHut:
            stampSwampHut(chunk, midX, midZ, ground);
            break;

        case Landmark::Count:
            break;
    }
}

// The witch's hut: a spruce shack up on stilts over the water, with an
// open porch on one side. Minecraft's has a cauldron and a crafting
// table in it; there is no cauldron here, so the brewing stand stands
// in for it -- which is what a witch would be using it for anyway.
void WorldGen::stampSwampHut(Chunk& chunk, int midX, int midZ, int ground) const
{
    constexpr int HALF = 3;
    constexpr int STILTS = 4;      // how far the floor stands clear
    constexpr int WALL = 4;

    // The floor sits above the water rather than on the mud, which is
    // the whole look of the thing.
    const int floorY = std::max(ground, SEA_LEVEL) + STILTS;

    // The legs it stands on, down into whatever is below -- water or
    // mud, both of which a stilt has to reach through.
    for (int dx = -HALF; dx <= HALF; dx += HALF * 2)
        for (int dz = -HALF; dz <= HALF; dz += HALF * 2)
            for (int y = floorY - 1; y > ground - 2; --y)
                put(chunk, midX + dx, y, midZ + dz, Blocks::SpruceLog);

    for (int dx = -HALF; dx <= HALF; ++dx)
        for (int dz = -HALF; dz <= HALF; ++dz)
        {
            put(chunk, midX + dx, floorY, midZ + dz, Blocks::SprucePlanks);

            for (int dy = 1; dy <= WALL; ++dy)
            {
                const bool wall = std::abs(dx) == HALF || std::abs(dz) == HALF;
                const bool roof = dy == WALL;

                // The porch: one side left open, which is how you get
                // in and what makes it read as a hut rather than a box.
                const bool porch = dz == HALF && std::abs(dx) <= 1 && dy < WALL;
                const bool corner = std::abs(dx) == HALF && std::abs(dz) == HALF;

                BlockId block = Blocks::Air;
                if (roof) block = Blocks::SprucePlanks;
                else if (porch) block = Blocks::Air;
                else if (corner) block = Blocks::SpruceLog;
                else if (wall) block = Blocks::SprucePlanks;

                put(chunk, midX + dx, floorY + dy, midZ + dz, block);
            }
        }

    // What is inside: the stand she brews at, a table, and the chest.
    put(chunk, midX - 1, floorY + 1, midZ - 1, Blocks::BrewingStand);
    put(chunk, midX - 1, floorY + 1, midZ + 1, Blocks::CraftingTable);
    putChests(chunk, Landmark::SwampHut, midX, floorY - STILTS, midZ);
}

// The desert well, which is not a structure in Minecraft at all: it is
// a decorator, one chunk of desert in a thousand. That rarity is the
// whole character of the thing -- you can cross three deserts without
// seeing one, and the one you do see is a landmark because of it.
void WorldGen::buildDesertWells(Chunk& chunk) const
{
    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    if (!Structures::spreadAt(chunkX, chunkZ, m_seed,
                              Structures::DESERT_WELL_SALT,
                              Structures::DESERT_WELL_CHANCE))
        return;

    const int midX = chunkX * Chunk::SX + Chunk::SX / 2;
    const int midZ = chunkZ * Chunk::SZ + Chunk::SZ / 2;

    if (biomeAt(midX, midZ) != Biome::Desert) return;

    const int ground = surfaceHeight(midX, midZ);
    if (ground <= SEA_LEVEL) return;

    // A sandstone slab with water in the middle of it, four pillars at
    // the corners and a roof across the top.
    for (int dx = -2; dx <= 2; ++dx)
        for (int dz = -2; dz <= 2; ++dz)
            put(chunk, midX + dx, ground, midZ + dz, Blocks::Sandstone);

    put(chunk, midX, ground, midZ, Blocks::Water);

    for (int dy = 1; dy <= 3; ++dy)
    {
        put(chunk, midX - 1, ground + dy, midZ - 1, Blocks::Sandstone);
        put(chunk, midX + 1, ground + dy, midZ - 1, Blocks::Sandstone);
        put(chunk, midX - 1, ground + dy, midZ + 1, Blocks::Sandstone);
        put(chunk, midX + 1, ground + dy, midZ + 1, Blocks::Sandstone);
    }

    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            put(chunk, midX + dx, ground + 4, midZ + dz, Blocks::Sandstone);
}

// Mineshafts go down by the spread rule rather than the grid: a flat
// chance for each chunk with nothing keeping two apart, which is why
// two of them sometimes run into each other. It is also where the
// first rails anybody sees come from.
void WorldGen::buildMineshafts(Chunk& chunk) const
{
    constexpr float CHANCE = MINESHAFT_CHANCE;
    constexpr uint32_t SALT = MINESHAFT_SALT;

    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    // A corridor runs past the end of its own chunk, so a chunk has to
    // ask its neighbours as well as itself.
    for (int dz = -2; dz <= 2; ++dz)
        for (int dx = -2; dx <= 2; ++dx)
        {
            const int cx = chunkX + dx;
            const int cz = chunkZ + dz;
            if (!Structures::spreadAt(cx, cz, m_seed, SALT, CHANCE)) continue;

            const int baseX = cx * Chunk::SX;
            const int baseZ = cz * Chunk::SZ;

            const int y = MINESHAFT_MIN +
                static_cast<int>(hashToFloat(hashCoords(cx, 23, cz, m_seed + SALT)) *
                                 static_cast<float>(MINESHAFT_MAX - MINESHAFT_MIN));

            const bool alongX = hashToFloat(hashCoords(cx, 24, cz, m_seed + SALT)) < 0.5f;
            const int length = Chunk::SX * 2;

            for (int step = 0; step < length; ++step)
            {
                const int x = alongX ? baseX + step : baseX + Chunk::SX / 2;
                const int z = alongX ? baseZ + Chunk::SZ / 2 : baseZ + step;

                for (int across = -1; across <= 1; ++across)
                {
                    const int ax = alongX ? x : x + across;
                    const int az = alongX ? z + across : z;

                    for (int up = 0; up <= 2; ++up) put(chunk, ax, y + up, az, Blocks::Air);

                    // Something underfoot, so a corridor cut through a
                    // cave is still a corridor.
                    put(chunk, ax, y - 1, az, Blocks::Planks);
                }

                // Supports every few blocks, which is what a mineshaft
                // looks like from down the corridor.
                if ((step % 5) == 0)
                    for (int side = -1; side <= 1; side += 2)
                        for (int up = 0; up <= 2; ++up)
                            put(chunk, alongX ? x : x + side, y + up,
                                alongX ? z + side : z,
                                (up == 2) ? Blocks::Planks : Blocks::OakFence);

                put(chunk, x, y, z, alongX ? Blocks::RailEastWest : Blocks::RailNorthSouth);
            }

            // A chest at the end of one shaft in three, and a cage in
            // one in three -- which is the dungeon a mineshaft turns
            // into when you are not paying attention.
            if (hashToFloat(hashCoords(cx, 25, cz, m_seed + SALT)) < 0.34f)
                put(chunk, alongX ? baseX + length - 2 : baseX + Chunk::SX / 2, y,
                    alongX ? baseZ + Chunk::SZ / 2 : baseZ + length - 2, Blocks::Chest);

            if (hashToFloat(hashCoords(cx, 26, cz, m_seed + SALT)) < 0.34f)
                put(chunk, baseX + Chunk::SX / 2 + 2, y, baseZ + Chunk::SZ / 2 + 2,
                    Blocks::MonsterSpawner);
        }
}

// A fortress: two walkways crossing over the lava with a keep where
// they meet, all of it nether brick, and a blaze cage in the keep --
// which is where blazes come from and the only reason to go.
void WorldGen::buildNetherFortresses(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    constexpr int ARM = 34;      // how far a walkway runs from the middle
    constexpr int WIDTH = 2;     // half the walkway's width
    constexpr int KEEP = 7;      // half the keep

    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    // Every square whose fortress could reach into this chunk.
    const int reachChunks = (ARM + KEEP) / Chunk::SX + 1;
    for (int sz = Structures::squareOf(chunkZ - reachChunks,
                                       Structures::NETHER_FORTRESS.spacing);
         sz <= Structures::squareOf(chunkZ + reachChunks,
                                    Structures::NETHER_FORTRESS.spacing); ++sz)
        for (int sx = Structures::squareOf(chunkX - reachChunks,
                                           Structures::NETHER_FORTRESS.spacing);
             sx <= Structures::squareOf(chunkX + reachChunks,
                                        Structures::NETHER_FORTRESS.spacing); ++sx)
        {
            const Structures::Spot spot =
                Structures::inSquare(Structures::NETHER_FORTRESS, sx, sz, m_seed);

            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            const int floor = FORTRESS_FLOOR;

            auto walkway = [&](int x, int z) {
                // Deck, and a rail of brick either side of it so you
                // can see where the edge is before you find it.
                put(chunk, x, floor, z, Blocks::NetherBricks);
                put(chunk, x, floor + 1, z, Blocks::Air);
                put(chunk, x, floor + 2, z, Blocks::Air);
            };

            for (int along = -ARM; along <= ARM; ++along)
                for (int across = -WIDTH; across <= WIDTH; ++across)
                {
                    walkway(midX + along, midZ + across);
                    walkway(midX + across, midZ + along);

                    if (std::abs(across) == WIDTH)
                    {
                        put(chunk, midX + along, floor + 1, midZ + across,
                            Blocks::NetherBricks);
                        put(chunk, midX + across, floor + 1, midZ + along,
                            Blocks::NetherBricks);
                    }
                }

            // The keep: a walled room where the two cross, roofed, with
            // the cage in the middle of it.
            for (int dx = -KEEP; dx <= KEEP; ++dx)
                for (int dz = -KEEP; dz <= KEEP; ++dz)
                {
                    const bool wall = std::abs(dx) == KEEP || std::abs(dz) == KEEP;
                    for (int dy = 0; dy <= 5; ++dy)
                    {
                        const bool roof = (dy == 5);
                        const bool solid = wall || roof || dy == 0;
                        const bool doorway = wall && std::abs(dx) <= 1 && dy >= 1 && dy <= 2;
                        const bool doorwayZ = wall && std::abs(dz) <= 1 && dy >= 1 && dy <= 2;

                        put(chunk, midX + dx, floor + dy, midZ + dz,
                            (solid && !(doorway || doorwayZ)) ? Blocks::NetherBricks
                                                              : Blocks::Air);
                    }
                }

            put(chunk, midX, floor + 1, midZ, Blocks::MonsterSpawner);

            // Nether wart on soul sand in the corners, which is what a
            // fortress is really worth: brewing starts here.
            for (int cx = -1; cx <= 1; cx += 2)
                for (int cz = -1; cz <= 1; cz += 2)
                {
                    const int wx = midX + cx * (KEEP - 2);
                    const int wz = midZ + cz * (KEEP - 2);
                    put(chunk, wx, floor, wz, Blocks::SoulSand);
                    put(chunk, wx, floor + 1, wz, Blocks::NetherWart);
                }
        }
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

// Vines down the outside of a canopy. Hung off the tree after it is
// built rather than built into it, so every wood keeps the shape it
// already had and only a swamp gets these.
void WorldGen::hangVines(Chunk& chunk, int worldX, int ground, int worldZ) const
{
    // Three, to match the margin the tree loop uses. A vine hung
    // further out than the loop reaches would appear on one side of a
    // chunk border and not the other.
    constexpr int CANOPY = 3;
    constexpr int LONGEST = 5;

    for (int dx = -CANOPY; dx <= CANOPY; ++dx)
        for (int dz = -CANOPY; dz <= CANOPY; ++dz)
        {
            if (std::abs(dx) + std::abs(dz) < 2) continue;   // the trunk

            const int wx = worldX + dx;
            const int wz = worldZ + dz;

            // One column in ten, not one in four. At 0.28 a single
            // tree hung twenty-odd curtains and the swamp came out as
            // a solid green wall you could not see through.
            if (hashToFloat(hashCoords(wx, 11, wz, m_seed + 7801)) > 0.10f) continue;

            // Find the leaves to hang from, which is the only block a
            // vine can start under.
            for (int y = ground + 9; y > ground + 2; --y)
            {
                if (!isLeaves(chunk.getBlock(wx - chunk.position().x * Chunk::SX,
                                             y,
                                             wz - chunk.position().z * Chunk::SZ)))
                    continue;

                const int drop = 2 + static_cast<int>(
                    hashToFloat(hashCoords(wx, 12, wz, m_seed + 7802)) * (LONGEST - 1));

                for (int k = 1; k <= drop; ++k)
                    put(chunk, wx, y - k, wz, Blocks::Vine);
                break;
            }
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
                // Scattered rather than thick, which is what makes a
                // swamp read as open water with trees standing in it.
                case Biome::Swamp: treeChance = 0.020f; break;
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

            // Vines off the canopy, which is most of what makes a
            // swamp look like one from the ground. Hung after the
            // tree rather than built into buildTree, so every wood
            // keeps the shape it had.
            if (here == Biome::Swamp) hangVines(chunk, wx, surface, wz);
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

// ---------------------------------------------------------------------
// The sea.
//
// Three things live out there, and Minecraft places each by a different
// rule: the shipwreck and the monument by their own grids, and buried
// treasure by the spread -- a flat one-in-a-hundred per chunk, which is
// why a map is the only sensible way to find one.
// ---------------------------------------------------------------------

namespace
{
    // Walking the grid's squares outwards from a place, skipping the
    // ones the structure cannot use.
    //
    // The other grids do not need this: a pyramid takes whatever land
    // its square offers. The two sea ones do, because most squares of
    // a monument or shipwreck grid are dry, and "the nearest spot the
    // grid offers" is almost never one with water in it. Asking for a
    // wider search without this returns the same unusable spot again.
    template <typename Fits>
    Structures::Spot nearestUsable(const Structures::Grid& grid, int blockX, int blockZ,
                                   uint32_t seed, int squares, Fits fits)
    {
        const int hereX = Structures::squareOf(
            Structures::squareOf(blockX, Structures::CHUNK), grid.spacing);
        const int hereZ = Structures::squareOf(
            Structures::squareOf(blockZ, Structures::CHUNK), grid.spacing);

        Structures::Spot best;
        long long bestDistance = 0;

        for (int sz = hereZ - squares; sz <= hereZ + squares; ++sz)
            for (int sx = hereX - squares; sx <= hereX + squares; ++sx)
            {
                const Structures::Spot spot = Structures::inSquare(grid, sx, sz, seed);
                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
                if (!fits(midX, midZ)) continue;

                const long long dx = midX - blockX;
                const long long dz = midZ - blockZ;
                const long long distance = dx * dx + dz * dz;
                if (!best.exists || distance < bestDistance)
                {
                    best = spot;
                    bestDistance = distance;
                }
            }

        return best;
    }
}

Structures::Spot WorldGen::nearestShipwreck(int blockX, int blockZ, int squares) const
{
    return nearestUsable(Structures::SHIPWRECK, blockX, blockZ, m_seed, squares,
                         [this](int x, int z) { return shipwreckFits(x, z); });
}

Structures::Spot WorldGen::nearestMonument(int blockX, int blockZ, int squares) const
{
    return nearestUsable(Structures::OCEAN_MONUMENT, blockX, blockZ, m_seed, squares,
                         [this](int x, int z) { return monumentFits(x, z); });
}

bool WorldGen::shipwreckFits(int midX, int midZ) const
{
    const int floorY = surfaceHeight(midX, midZ);
    return floorY <= SEA_LEVEL + 1 && floorY >= 24;
}

bool WorldGen::monumentFits(int midX, int midZ) const
{
    // Deep enough to be covered, and no hill poking up through the
    // middle of it. Probing the corners rather than every column: a
    // monument is checked for every chunk around it, and surfaceHeight
    // is noise rather than a lookup.
    constexpr int HALF = MONUMENT_HALF;
    for (int dz = -HALF; dz <= HALF; dz += HALF)
        for (int dx = -HALF; dx <= HALF; dx += HALF)
        {
            const int floorY = surfaceHeight(midX + dx, midZ + dz);
            if (floorY > SEA_LEVEL - MONUMENT_DEPTH) return false;
        }
    return true;
}

int WorldGen::monumentHeight(int midX, int midZ) const
{
    // As tall as the water over it allows, so the roof finishes a
    // block under the surface rather than standing out of it.
    const int base = surfaceHeight(midX, midZ) + 1;
    const int room = SEA_LEVEL - base - 1;
    return std::clamp(room, MONUMENT_MIN_TALL, MONUMENT_MAX_TALL);
}

void WorldGen::buildOceanStructures(Chunk& chunk) const
{
    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    // The monument is the wide one, so it is the reach that matters.
    constexpr int REACH_CHUNKS = 2;

    // Buried treasure is a single block inside its own chunk, so it
    // needs no reach at all. Asking the twenty-four chunks round it as
    // well was twenty-five times the work for the same one block.
    stampBuriedTreasure(chunk, chunkX, chunkZ);

    const Structures::Grid* grids[2] = { &Structures::SHIPWRECK,
                                         &Structures::OCEAN_MONUMENT };

    for (int which = 0; which < 2; ++which)
    {
        const Structures::Grid& grid = *grids[which];

        const int firstX = Structures::squareOf(chunkX - REACH_CHUNKS, grid.spacing);
        const int lastX = Structures::squareOf(chunkX + REACH_CHUNKS, grid.spacing);
        const int firstZ = Structures::squareOf(chunkZ - REACH_CHUNKS, grid.spacing);
        const int lastZ = Structures::squareOf(chunkZ + REACH_CHUNKS, grid.spacing);

        for (int sz = firstZ; sz <= lastZ; ++sz)
            for (int sx = firstX; sx <= lastX; ++sx)
            {
                const Structures::Spot spot = Structures::inSquare(grid, sx, sz, m_seed);
                if (std::abs(spot.chunkX - chunkX) > REACH_CHUNKS) continue;
                if (std::abs(spot.chunkZ - chunkZ) > REACH_CHUNKS) continue;

                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

                if (which == 0) stampShipwreck(chunk, midX, midZ);
                else if (monumentFits(midX, midZ)) stampMonument(chunk, midX, midZ);
            }
    }
}

void WorldGen::stampBuriedTreasure(Chunk& chunk, int chunkX, int chunkZ) const
{
    if (!Structures::spreadAt(chunkX, chunkZ, m_seed,
                              Structures::BURIED_TREASURE_SALT,
                              Structures::BURIED_TREASURE_CHANCE))
        return;

    // Minecraft buries it at the ninth block of the chunk rather than
    // in the middle, and a map drawn for one points at that corner.
    const int x = chunkX * Structures::CHUNK + 9;
    const int z = chunkZ * Structures::CHUNK + 9;

    const Biome biome = biomeAt(x, z);
    if (biome != Biome::Beach && biome != Biome::Ocean) return;

    const int floorY = surfaceHeight(x, z);
    if (floorY < 20) return;

    // Under the sand, not on it: the whole point is that nothing shows
    // on the surface.
    const int y = floorY - 2;
    put(chunk, x, y, z, Blocks::Chest);
}

void WorldGen::stampShipwreck(Chunk& chunk, int midX, int midZ) const
{
    const int floorY = surfaceHeight(midX, midZ);

    // Sunk, or run aground on a beach. Anything further inland is a
    // ship in a field, which is a different game.
    if (floorY > SEA_LEVEL + 1) return;
    if (floorY < 24) return;

    // Which way she lies. A fleet all pointing north looks placed.
    const bool alongX = hashToFloat(hashCoords(midX, 7, midZ, m_seed + 5501)) < 0.5f;

    constexpr int LENGTH = 21;  // bow to stern
    constexpr int BEAM = 7;     // across
    constexpr int DEPTH = 5;    // keel to gunwale

    const int halfLong = LENGTH / 2;
    const int halfWide = BEAM / 2;
    const int keel = floorY + 1;

    const bool sunk = floorY < SEA_LEVEL - 1;

    for (int along = -halfLong; along <= halfLong; ++along)
    {
        // The hull narrows towards both ends, so she has a bow and a
        // stern rather than being a crate.
        const float taper = 1.0f - static_cast<float>(std::abs(along)) /
                                   static_cast<float>(halfLong + 1);
        const int wide = std::max(1, static_cast<int>(halfWide * (0.35f + 0.65f * taper)));

        for (int across = -wide; across <= wide; ++across)
            for (int dy = 0; dy < DEPTH; ++dy)
            {
                const int wx = alongX ? midX + along : midX + across;
                const int wz = alongX ? midZ + across : midZ + along;
                const int y = keel + dy;

                const bool hull = std::abs(across) == wide || dy == 0 ||
                                  std::abs(along) == halfLong;
                // The deck, with a hatch amidships so she is not sealed.
                const bool deck = dy == DEPTH - 1 && std::abs(along) > 2;
                const bool broken = hull && dy >= DEPTH - 2 &&
                                    hashToFloat(hashCoords(wx, y, wz, m_seed + 5503)) < 0.22f;

                BlockId block = Blocks::Air;
                if ((hull || deck) && !broken) block = Blocks::Planks;
                else if (sunk && y <= SEA_LEVEL) block = Blocks::Water;

                put(chunk, wx, y, wz, block);
            }
    }

    // The mast, and the two chests she went down with. A shipwreck
    // without a chest in it is scenery.
    for (int dy = 0; dy < 7; ++dy)
    {
        const int y = keel + DEPTH + dy;
        if (sunk && y > SEA_LEVEL) break;
        put(chunk, midX, y, midZ, Blocks::Log);
    }

    const int holdY = keel + 1;
    for (int i = 0; i < 2; ++i)
    {
        const int along = (i == 0) ? -halfLong + 3 : halfLong - 3;
        const int wx = alongX ? midX + along : midX;
        const int wz = alongX ? midZ : midZ + along;
        put(chunk, wx, holdY, wz, Blocks::Chest);
    }
}

void WorldGen::stampMonument(Chunk& chunk, int midX, int midZ) const
{
    constexpr int HALF = MONUMENT_HALF;

    const int floorY = surfaceHeight(midX, midZ);
    const int base = floorY + 1;
    const int TALL = monumentHeight(midX, midZ);

    for (int dz = -HALF; dz <= HALF; ++dz)
        for (int dx = -HALF; dx <= HALF; ++dx)
        {
            const int wx = midX + dx;
            const int wz = midZ + dz;
            const int far = std::max(std::abs(dx), std::abs(dz));

            for (int dy = 0; dy <= TALL; ++dy)
            {
                const int y = base + dy;

                // The roof steps in twice, which is the silhouette that
                // makes a monument recognisable from a long way off.
                const int roofAt = (far <= HALF - 8) ? TALL
                                 : (far <= HALF - 4) ? TALL - TALL / 4
                                                     : TALL - TALL / 2;
                if (dy > roofAt) continue;

                const bool outerWall = far == HALF;
                const bool roof = dy == roofAt;
                const bool floorSlab = dy == 0;

                // The inner walls: one ring of rooms round a hall.
                const bool innerWall = (far == HALF - 4 || far == HALF - 8) &&
                                       dy <= roofAt &&
                                       !(((std::abs(dx) <= 1) && std::abs(dz) >= std::abs(dx)) ||
                                         ((std::abs(dz) <= 1) && std::abs(dx) >= std::abs(dz)));

                BlockId block = Blocks::Air;

                if (floorSlab)
                {
                    block = ((dx + dz) & 1) ? Blocks::Prismarine : Blocks::PrismarineBricks;
                }
                else if (outerWall || innerWall || roof)
                {
                    // Dark prismarine up the corners and along the
                    // course at eye height; a lantern every eighth
                    // block, which is what lights the place.
                    const bool corner = std::abs(dx) == far && std::abs(dz) == far;
                    const bool lantern = outerWall && !corner && dy == roofAt - 2 &&
                                         (((dx + dz) % 8) == 0);

                    if (lantern) block = Blocks::SeaLantern;
                    else if (corner || dy == roofAt - 4) block = Blocks::DarkPrismarine;
                    else if (roof) block = Blocks::Prismarine;
                    else block = Blocks::PrismarineBricks;
                }
                else if (y <= SEA_LEVEL)
                {
                    // Flooded, as it should be: a monument full of air
                    // would be a bunker.
                    block = Blocks::Water;
                }

                put(chunk, wx, y, wz, block);
            }
        }

    // The treasure room in the middle: dark prismarine round it, a
    // sponge in the corner, and the chest Minecraft keeps gold in.
    for (int dz = -2; dz <= 2; ++dz)
        for (int dx = -2; dx <= 2; ++dx)
            for (int dy = 1; dy <= 4; ++dy)
            {
                const bool shell = std::abs(dx) == 2 || std::abs(dz) == 2 || dy == 4;
                put(chunk, midX + dx, base + dy, midZ + dz,
                    shell ? Blocks::DarkPrismarine
                          : ((base + dy <= SEA_LEVEL) ? Blocks::Water : Blocks::Air));
            }

    put(chunk, midX, base + 1, midZ, Blocks::Chest);
    put(chunk, midX + 1, base + 1, midZ + 1, Blocks::WetSponge);
    put(chunk, midX - 1, base + 1, midZ - 1, Blocks::SeaLantern);
}

// ---------------------------------------------------------------------
// What a chest found in the world belongs to.
//
// Nothing records where the chests went: a chunk is generated, meshed
// and thrown away many times over in a session, and a table of every
// chest in a world would have to survive all of that. So this asks the
// same question the generator asked -- which structures reach this
// place, and does one of them keep a chest exactly here.
// ---------------------------------------------------------------------
WorldGen::Hoard WorldGen::hoardAt(int x, int y, int z) const
{
    // The landmarks, each off the one chest table its stamp uses.
    for (int k = 0; k < static_cast<int>(Landmark::Count); ++k)
    {
        const Landmark kind = static_cast<Landmark>(k);
        const Structures::Spot spot = nearestLandmark(kind, x, z, 1);
        if (!spot.exists) continue;

        const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
        const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
        if (!landmarkStands(kind, midX, midZ)) continue;

        const int ground = surfaceHeight(midX, midZ);

        ChestSpot spots[2];
        const int many = landmarkChests(kind, spots);
        for (int i = 0; i < many; ++i)
            if (midX + spots[i].dx == x && ground + spots[i].dy == y &&
                midZ + spots[i].dz == z)
            {
                switch (kind)
                {
                    case Landmark::DesertPyramid:   return Hoard::DesertPyramid;
                    case Landmark::JungleTemple:    return Hoard::JungleTemple;
                    case Landmark::Igloo:           return Hoard::Igloo;
                    case Landmark::RuinedPortal:    return Hoard::RuinedPortal;
                    case Landmark::PillagerOutpost: return Hoard::PillagerOutpost;
                    case Landmark::WoodlandMansion: return Hoard::WoodlandMansion;
                    case Landmark::SwampHut:        return Hoard::SwampHut;
                    case Landmark::Count:           break;
                }
            }
    }

    // Buried treasure, which is at the ninth block of its own chunk and
    // so needs no search at all.
    {
        const int chunkX = Structures::squareOf(x, Structures::CHUNK);
        const int chunkZ = Structures::squareOf(z, Structures::CHUNK);
        if (x == chunkX * Structures::CHUNK + 9 && z == chunkZ * Structures::CHUNK + 9 &&
            Structures::spreadAt(chunkX, chunkZ, m_seed,
                                 Structures::BURIED_TREASURE_SALT,
                                 Structures::BURIED_TREASURE_CHANCE) &&
            y == surfaceHeight(x, z) - 2)
            return Hoard::BuriedTreasure;
    }

    // The shipwreck's two, which depend on which way she lies.
    {
        const Structures::Spot spot = nearestShipwreck(x, z, 1);
        if (spot.exists)
        {
            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            const int floorY = surfaceHeight(midX, midZ);

            if (floorY <= SEA_LEVEL + 1 && floorY >= 24 && y == floorY + 2)
            {
                const bool alongX =
                    hashToFloat(hashCoords(midX, 7, midZ, m_seed + 5501)) < 0.5f;
                constexpr int HALF_LONG = 21 / 2;

                for (int i = 0; i < 2; ++i)
                {
                    const int along = (i == 0) ? -HALF_LONG + 3 : HALF_LONG - 3;
                    const int wx = alongX ? midX + along : midX;
                    const int wz = alongX ? midZ : midZ + along;
                    if (wx == x && wz == z) return Hoard::Shipwreck;
                }
            }
        }
    }

    // The monument's one, in the room in the middle.
    {
        const Structures::Spot spot = nearestMonument(x, z, 1);
        if (spot.exists)
        {
            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            if (midX == x && midZ == z && y == surfaceHeight(midX, midZ) + 2)
                return Hoard::Monument;
        }
    }

    // A mineshaft's chest, which sits at the far end of its corridor.
    // Same spread, same salt, same roll as the stamp: the numbers live
    // in MINESHAFT_SALT so the two cannot drift apart.
    for (int dz = -2; dz <= 2; ++dz)
        for (int dx = -2; dx <= 2; ++dx)
        {
            const int cx = Structures::squareOf(x, Structures::CHUNK) + dx;
            const int cz = Structures::squareOf(z, Structures::CHUNK) + dz;
            if (!Structures::spreadAt(cx, cz, m_seed, MINESHAFT_SALT, MINESHAFT_CHANCE))
                continue;

            const int baseX = cx * Chunk::SX;
            const int baseZ = cz * Chunk::SZ;
            const int shaftY = MINESHAFT_MIN +
                static_cast<int>(hashToFloat(hashCoords(cx, 23, cz, m_seed + MINESHAFT_SALT)) *
                                 static_cast<float>(MINESHAFT_MAX - MINESHAFT_MIN));
            if (y != shaftY) continue;
            if (hashToFloat(hashCoords(cx, 25, cz, m_seed + MINESHAFT_SALT)) >= 0.34f)
                continue;

            const bool alongX =
                hashToFloat(hashCoords(cx, 24, cz, m_seed + MINESHAFT_SALT)) < 0.5f;
            const int length = Chunk::SX * 2;
            const int chestX = alongX ? baseX + length - 2 : baseX + Chunk::SX / 2;
            const int chestZ = alongX ? baseZ + Chunk::SZ / 2 : baseZ + length - 2;
            if (chestX == x && chestZ == z) return Hoard::Mineshaft;
        }

    // An ancient city's two, either side of the frame in the middle.
    {
        const Structures::Spot spot = nearestAncientCity(x, z, 1);
        if (spot.exists && y == CITY_FLOOR + 1)
        {
            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            if ((x == midX - 6 && z == midZ + 6) || (x == midX + 6 && z == midZ - 6))
                return Hoard::AncientCity;
        }
    }

    // An End city's chest, on its top floor, and a bastion's two.
    // Neither of these is in the overworld, so neither can collide
    // with anything above: hoardAt is asked per dimension.
    {
        const Structures::Spot spot = nearestEndCity(x, z, 1);
        if (spot.exists)
        {
            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            const int ground = endGroundAt(midX, midZ);
            if (ground >= 0 && x == midX - 2 && z == midZ + 2 && y == ground + 2 + 21)
                return Hoard::EndCity;
        }
    }

    {
        const Structures::Spot spot = nearestBastion(x, z, 1);
        if (spot.exists && y == NETHER_LAVA_LEVEL + 7)
        {
            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            if ((x == midX - 4 && z == midZ + 4) || (x == midX + 4 && z == midZ - 4))
                return Hoard::Bastion;
        }
    }

    // A stronghold's store room.
    {
        const Stronghold::Site site = Stronghold::touching(x, z, x, z, m_seed);
        if (site.exists && y == site.y)
            for (const Stronghold::Room& room : Stronghold::plan(site, m_seed))
                if (room.kind == Stronghold::Kind::Store &&
                    x == room.x + room.width / 2 && z == room.z + room.depth / 2)
                    return Hoard::Stronghold;
    }

    // A trial chamber's supply room.
    {
        const TrialChamber::Site site =
            TrialChamber::siteFor(TrialChamber::regionOf(x), TrialChamber::regionOf(z), m_seed);
        TrialChamber::Room room;
        if (site.exists && y == site.y && TrialChamber::roomAt(site, m_seed, x, z, room) &&
            room.kind == TrialChamber::Kind::Supply &&
            x == room.x + room.width / 2 && z == room.z + room.depth / 2)
            return Hoard::TrialChamber;
    }

    return Hoard::None;
}

bool WorldGen::reefWater(int worldX, int worldZ) const
{
    return temperatureAt(worldX, worldZ) > CORAL_WARMTH;
}

// What grows on the sea floor.
//
// Every column of every chunk passes through here, land included, so
// the first test is the cheap one: anything at or above the sea is out
// before a single hash is drawn. On a land chunk this is one compare
// per column and nothing else.
void WorldGen::dressSeabed(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int lx = 0; lx < Chunk::SX; ++lx)
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;
            const int floorY = surfaceHeight(wx, wz);

            const int depth = SEA_LEVEL - floorY;

            // Lily pads float on the shallows of a swamp, which is the
            // one place in this world where water stands two blocks
            // deep over mud rather than running out to sea. Done here
            // because this loop has already paid for the column.
            if (depth >= 1 && depth <= 3 && biomeAt(wx, wz) == Biome::Swamp &&
                chunk.getBlock(lx, SEA_LEVEL, lz) == Blocks::Water &&
                hashToFloat(hashCoords(wx, 9, wz, m_seed + 7803)) < 0.09f)
            {
                put(chunk, wx, SEA_LEVEL + 1, wz, Blocks::LilyPad);
                continue;
            }

            // Dry land, or a puddle too shallow for anything to stand
            // up in. This is the test that keeps the cost off the
            // three quarters of the world that is not sea.
            if (depth < 2) continue;

            const BlockId ground = chunk.getBlock(lx, floorY, lz);
            if (ground != Blocks::Sand && ground != Blocks::Gravel &&
                ground != Blocks::Dirt && ground != Blocks::Stone)
                continue;

            // Nothing grows where something is already standing: a
            // monument floor does not sprout seagrass.
            if (chunk.getBlock(lx, floorY + 1, lz) != Blocks::Water) continue;

            const float roll = hashToFloat(hashCoords(wx, 1, wz, m_seed + 7701));

            // Kelp, which is the one you see from the surface: a stem
            // most of the way up with a frond on the end of it.
            if (depth >= 5 && roll < 0.055f)
            {
                const float reach =
                    0.45f + hashToFloat(hashCoords(wx, 2, wz, m_seed + 7702)) * 0.45f;
                const int tall = std::max(2, static_cast<int>((depth - 1) * reach));

                for (int k = 0; k < tall; ++k)
                    put(chunk, wx, floorY + 1 + k, wz,
                        (k == tall - 1) ? Blocks::KelpTop : Blocks::Kelp);
                continue;
            }

            // A reef, in the warm water only. The roll that decides
            // whether there is coral here is drawn off a two-block
            // cell rather than off the block, so a reef comes out as
            // blobs: a per-block roll gives a field of lone pillars
            // each a different colour, which is not what a reef looks
            // like. Costs nothing -- it is the same hash with the low
            // bit of the position thrown away.
            const int cellX = wx & ~1;
            const int cellZ = wz & ~1;
            const bool reefCell =
                depth >= 3 && reefWater(wx, wz) &&
                hashToFloat(hashCoords(cellX, 6, cellZ, m_seed + 7706)) < 0.05f;

            if (reefCell && roll < 0.85f)
            {
                static const BlockId CORALS[5] = {
                    Blocks::TubeCoralBlock, Blocks::BrainCoralBlock,
                    Blocks::BubbleCoralBlock, Blocks::FireCoralBlock,
                    Blocks::HornCoralBlock,
                };
                static const BlockId FANS[5] = {
                    Blocks::TubeCoral, Blocks::BrainCoral,
                    Blocks::BubbleCoral, Blocks::FireCoral, Blocks::HornCoral,
                };

                const int which = static_cast<int>(
                    hashCoords(cellX, 3, cellZ, m_seed + 7703) % 5u);

                // A lump standing on the floor rather than set into
                // it. Laid flush with the seabed, a coral block shows
                // nothing but its top face and reads as a coloured
                // square painted on the sand.
                const int lump = 1 + static_cast<int>(
                    hashToFloat(hashCoords(cellX, 5, cellZ, m_seed + 7705)) * 1.99f);
                const int room = std::min(lump, depth - 2);
                if (room < 1) continue;

                for (int k = 0; k < room; ++k)
                    put(chunk, wx, floorY + 1 + k, wz, CORALS[which]);

                const float crown = hashToFloat(hashCoords(wx, 4, wz, m_seed + 7704));
                if (crown < 0.55f)
                    put(chunk, wx, floorY + 1 + room, wz, FANS[which]);
                else if (crown < 0.68f)
                    put(chunk, wx, floorY + 1 + room, wz, Blocks::SeaPickle);
                continue;
            }

            // And seagrass, which is most of what is down there.
            if (roll < 0.26f)
                put(chunk, wx, floorY + 1, wz, Blocks::Seagrass);
        }
}

// ---------------------------------------------------------------------
// The ancient city.
//
// The one thing here that lives below the ores: Minecraft puts it at
// the bottom of the deep dark, and this world's floor is y=0, so it
// sits just above the bedrock. On Minecraft's own grid, 24 apart with
// 8 between.
//
// Not Minecraft's layout, which is assembled room by room from a set
// of pieces this engine has no equivalent of. What it is instead is
// the thing anybody recognises one by: a sunken floor of deepslate
// tiles, pillars holding up nothing, sculk grown over all of it, and
// the frame of reinforced deepslate in the middle that goes nowhere.
// ---------------------------------------------------------------------

Structures::Spot WorldGen::nearestAncientCity(int blockX, int blockZ, int squares) const
{
    return Structures::nearest(Structures::ANCIENT_CITY, blockX, blockZ, m_seed, squares);
}

void WorldGen::buildAncientCities(Chunk& chunk) const
{
    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    // A city is forty blocks across, so it reaches two chunks past the
    // one it is centred in.
    constexpr int REACH_CHUNKS = 2;
    const Structures::Grid& grid = Structures::ANCIENT_CITY;

    const int firstX = Structures::squareOf(chunkX - REACH_CHUNKS, grid.spacing);
    const int lastX = Structures::squareOf(chunkX + REACH_CHUNKS, grid.spacing);
    const int firstZ = Structures::squareOf(chunkZ - REACH_CHUNKS, grid.spacing);
    const int lastZ = Structures::squareOf(chunkZ + REACH_CHUNKS, grid.spacing);

    for (int sz = firstZ; sz <= lastZ; ++sz)
        for (int sx = firstX; sx <= lastX; ++sx)
        {
            const Structures::Spot spot = Structures::inSquare(grid, sx, sz, m_seed);
            if (std::abs(spot.chunkX - chunkX) > REACH_CHUNKS) continue;
            if (std::abs(spot.chunkZ - chunkZ) > REACH_CHUNKS) continue;

            stampAncientCity(chunk,
                             spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2,
                             spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2);
        }
}

void WorldGen::stampAncientCity(Chunk& chunk, int midX, int midZ) const
{
    constexpr int HALF = CITY_HALF;
    constexpr int TALL = CITY_TALL;
    const int floorY = CITY_FLOOR;

    for (int dx = -HALF; dx <= HALF; ++dx)
        for (int dz = -HALF; dz <= HALF; ++dz)
        {
            const int wx = midX + dx;
            const int wz = midZ + dz;
            const int far = std::max(std::abs(dx), std::abs(dz));

            // Round rather than square: a city is a bowl cut out of
            // the rock, and the corners of a square one read as a
            // building rather than as a place.
            const float reach = std::sqrt(static_cast<float>(dx * dx + dz * dz));
            if (reach > HALF + 0.5f) continue;

            for (int dy = 0; dy <= TALL; ++dy)
            {
                const int y = floorY + dy;

                if (dy == 0)
                {
                    // The floor. Deepslate tiles with sculk grown
                    // through it, heavier towards the middle: the
                    // sculk is what has taken the place over.
                    const float sculkiness = 1.0f - reach / static_cast<float>(HALF);
                    const bool grown =
                        hashToFloat(hashCoords(wx, y, wz, m_seed + 8801)) <
                            0.25f + sculkiness * 0.5f;
                    put(chunk, wx, y, wz, grown ? Blocks::Sculk : Blocks::DeepslateTiles);
                    continue;
                }

                // Everything above the floor is hollowed out, which is
                // the whole point: a city you cannot walk through is a
                // seam of deepslate.
                BlockId block = Blocks::Air;

                // The pillars, on a grid of eight, holding up nothing.
                const bool pillar = ((dx + HALF) % 8) == 0 && ((dz + HALF) % 8) == 0 &&
                                    far < HALF - 2;
                if (pillar && dy <= TALL - 2)
                    block = (dy % 4 == 3) ? Blocks::DeepslateTiles : Blocks::DeepslateBricks;

                // A lantern hung on each pillar. Minecraft lights a
                // city this way, and it is also the only reason the
                // place is visible at all: lit from the ceiling alone
                // it is a black screen with a hum.
                const bool beside = (((dx + HALF) % 8) == 1 && ((dz + HALF) % 8) == 0) ||
                                    (((dx + HALF) % 8) == 0 && ((dz + HALF) % 8) == 1);
                if (beside && far < HALF - 2 && (dy == 4 || dy == 9))
                    block = Blocks::SoulLantern;

                // The wall round the bowl, so it is a room rather than
                // a disc of floor in open rock.
                if (reach > HALF - 1.5f) block = Blocks::DeepslateBricks;

                // And a ceiling over the whole of it.
                if (dy == TALL) block = Blocks::DeepslateBricks;

                put(chunk, wx, y, wz, block);
            }
        }

    // What grows on the floor, and what listens. A sensor every so
    // often, a shrieker beside some of them, and a catalyst where the
    // sculk is thickest.
    for (int dx = -HALF + 2; dx <= HALF - 2; ++dx)
        for (int dz = -HALF + 2; dz <= HALF - 2; ++dz)
        {
            const int wx = midX + dx;
            const int wz = midZ + dz;
            const float reach = std::sqrt(static_cast<float>(dx * dx + dz * dz));
            if (reach > HALF - 2.5f) continue;

            const float roll = hashToFloat(hashCoords(wx, 3, wz, m_seed + 8802));

            if (roll < 0.012f)
                put(chunk, wx, floorY + 1, wz, Blocks::SculkSensor);
            else if (roll < 0.020f)
                put(chunk, wx, floorY + 1, wz, Blocks::SculkShrieker);
            else if (roll < 0.024f)
                put(chunk, wx, floorY + 1, wz, Blocks::SculkCatalyst);
        }

    // The frame in the middle. Reinforced deepslate, in the shape of a
    // portal, with nothing in it -- which is the thing everybody
    // remembers about an ancient city.
    for (int dx = -2; dx <= 2; ++dx)
        for (int dy = 0; dy <= 5; ++dy)
        {
            if (std::abs(dx) != 2 && dy != 0 && dy != 5) continue;
            put(chunk, midX + dx, floorY + 1 + dy, midZ, Blocks::ReinforcedDeepslate);
        }

    // Lit, just enough to see it by. An ancient city in the dark is a
    // black screen with a hum.
    for (int dx = -HALF + 4; dx <= HALF - 4; dx += 8)
        for (int dz = -HALF + 4; dz <= HALF - 4; dz += 8)
        {
            if (dx * dx + dz * dz > (HALF - 4) * (HALF - 4)) continue;
            put(chunk, midX + dx, floorY + TALL - 1, midZ + dz, Blocks::SoulLantern);
        }

    // And the chests it is worth the walk down for.
    put(chunk, midX - 6, floorY + 1, midZ + 6, Blocks::Chest);
    put(chunk, midX + 6, floorY + 1, midZ - 6, Blocks::Chest);
}

// ---------------------------------------------------------------------
// The End's city, and the Nether's bastion.
//
// Both are on Minecraft's own grids, and both have to ask about the
// ground as they go rather than taking whatever their square offers:
// most of the End is open void, and a bastion needs somewhere in the
// Nether that is neither lava nor solid rock. That is the same problem
// the monument had, and it is solved the same way.
// ---------------------------------------------------------------------

int WorldGen::endGroundAt(int worldX, int worldZ) const
{
    // The End is generated by density rather than by a height
    // function, so the only honest way to ask how high the island is
    // here is to generate the column and look.
    Chunk probe(ChunkPos{ Structures::squareOf(worldX, Chunk::SX),
                          Structures::squareOf(worldZ, Chunk::SZ) });
    generateEnd(probe);

    const int lx = worldX - probe.position().x * Chunk::SX;
    const int lz = worldZ - probe.position().z * Chunk::SZ;

    for (int y = Chunk::SY - 1; y > 0; --y)
        if (probe.getBlock(lx, y, lz) == Blocks::EndStone) return y;

    return -1;
}

// The same thing again: the search has to refuse every place the stamp
// refuses. The main-island rule lived only in the stamp, so on one seed
// in four the search pointed at a city on the island you arrive on,
// where none is built.
bool WorldGen::endCityStands(int midX, int midZ) const
{
    // Only on the outer islands. The main island is where you arrive
    // and where the dragon is; Minecraft puts no city on it either.
    const long long away = static_cast<long long>(midX) * midX +
                           static_cast<long long>(midZ) * midZ;
    if (away < END_CITY_VOID * END_CITY_VOID) return false;

    // An island wide enough to stand a city on, not a spur of one.
    if (endGroundAt(midX, midZ) < 0) return false;
    return endGroundAt(midX + 8, midZ) >= 0 && endGroundAt(midX - 8, midZ) >= 0 &&
           endGroundAt(midX, midZ + 8) >= 0 && endGroundAt(midX, midZ - 8) >= 0;
}

Structures::Spot WorldGen::nearestEndCity(int blockX, int blockZ, int squares) const
{
    return nearestUsable(Structures::END_CITY, blockX, blockZ, m_seed, squares,
                         [this](int x, int z) { return endCityStands(x, z); });
}

void WorldGen::buildEndCities(Chunk& chunk) const
{
    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    constexpr int REACH_CHUNKS = 1;
    const Structures::Grid& grid = Structures::END_CITY;

    const int firstX = Structures::squareOf(chunkX - REACH_CHUNKS, grid.spacing);
    const int lastX = Structures::squareOf(chunkX + REACH_CHUNKS, grid.spacing);
    const int firstZ = Structures::squareOf(chunkZ - REACH_CHUNKS, grid.spacing);
    const int lastZ = Structures::squareOf(chunkZ + REACH_CHUNKS, grid.spacing);

    for (int sz = firstZ; sz <= lastZ; ++sz)
        for (int sx = firstX; sx <= lastX; ++sx)
        {
            const Structures::Spot spot = Structures::inSquare(grid, sx, sz, m_seed);
            if (std::abs(spot.chunkX - chunkX) > REACH_CHUNKS) continue;
            if (std::abs(spot.chunkZ - chunkZ) > REACH_CHUNKS) continue;

            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

            if (!endCityStands(midX, midZ)) continue;

            stampEndCity(chunk, midX, midZ, endGroundAt(midX, midZ));
        }
}

void WorldGen::stampEndCity(Chunk& chunk, int midX, int midZ, int ground) const
{
    constexpr int HALF = 5;
    constexpr int TALL = END_CITY_TALL;
    constexpr int STOREY = 7;

    // A tower of three storeys with a pillared top, which is what an
    // End city reads as from across the void: tall and thin and
    // purple, standing on a yellow island.
    for (int dy = 0; dy <= TALL; ++dy)
        for (int dx = -HALF - 1; dx <= HALF + 1; ++dx)
            for (int dz = -HALF - 1; dz <= HALF + 1; ++dz)
            {
                const int wx = midX + dx;
                const int wz = midZ + dz;
                const int y = ground + 1 + dy;

                // The top storey hangs a block out past the tower, as
                // Minecraft's does. A plain column of purpur reads as
                // a chimney.
                const int half = (dy >= TALL - 5) ? HALF + 1 : HALF;
                if (std::abs(dx) > half || std::abs(dz) > half) continue;

                const bool wall = std::abs(dx) == half || std::abs(dz) == half;
                const bool floor = (dy % STOREY) == 0;
                const bool corner = std::abs(dx) == half && std::abs(dz) == half;

                // A way in at the bottom, and windows up the sides.
                const bool door = wall && dz == -HALF && std::abs(dx) <= 1 &&
                                  dy >= 1 && dy <= 2;
                // Every other block, not the whole course. Run along
                // the full width they read as a gap in the wall
                // rather than as windows.
                const bool window = wall && !corner &&
                                    (dy % STOREY) >= 3 && (dy % STOREY) <= 4 &&
                                    ((std::abs(dx) + std::abs(dz)) % 2) == 0;

                BlockId block = Blocks::Air;
                if (corner) block = Blocks::PurpurPillar;
                else if (door || window) block = Blocks::Air;
                else if (wall) block = Blocks::PurpurBlock;
                else if (floor) block = Blocks::EndStoneBricks;

                put(chunk, wx, y, wz, block);
            }

    // The way up, and the chests on the top floor -- which is where
    // an End city keeps what you came for.
    for (int dy = 1; dy < TALL; ++dy)
        put(chunk, midX + HALF - 1, ground + 1 + dy, midZ + HALF - 1, Blocks::LadderNorth);

    put(chunk, midX - 2, ground + 2 + STOREY * 3, midZ + 2, Blocks::Chest);

    // And a chorus plant on the island beside it, because an End city
    // without one standing near it looks dropped there.
    for (int dy = 0; dy < 4; ++dy)
        put(chunk, midX + HALF + 3, ground + 1 + dy, midZ,
            (dy == 3) ? Blocks::ChorusFlower : Blocks::ChorusPlant);
}

// Minecraft keeps a bastion and a fortress out of each other's way by
// giving them the same salt, so the two land on the same square and
// only one is built. Here they are told apart by a coin flip on the
// square -- and the flip has to live in one place, or the search walks
// to a square the stamp decided was a fortress and finds nothing
// there, which is exactly what the first one did.
bool WorldGen::bastionWins(int chunkX, int chunkZ) const
{
    return hashToFloat(hashCoords(chunkX, 77, chunkZ, m_seed + 6601)) >= 0.5f;
}

Structures::Spot WorldGen::nearestBastion(int blockX, int blockZ, int squares) const
{
    const uint32_t seed = m_seed;
    const Structures::Grid& grid = Structures::BASTION;

    // The predicate is asked about the middle of a square rather than
    // about a chunk, so the flip is recovered from the block position.
    return nearestUsable(grid, blockX, blockZ, seed, squares,
                         [this](int x, int z)
                         {
                             return bastionWins(Structures::squareOf(x, Structures::CHUNK),
                                                Structures::squareOf(z, Structures::CHUNK));
                         });
}

void WorldGen::buildBastions(Chunk& chunk) const
{
    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    constexpr int REACH_CHUNKS = 2;
    const Structures::Grid& grid = Structures::BASTION;

    const int firstX = Structures::squareOf(chunkX - REACH_CHUNKS, grid.spacing);
    const int lastX = Structures::squareOf(chunkX + REACH_CHUNKS, grid.spacing);
    const int firstZ = Structures::squareOf(chunkZ - REACH_CHUNKS, grid.spacing);
    const int lastZ = Structures::squareOf(chunkZ + REACH_CHUNKS, grid.spacing);

    for (int sz = firstZ; sz <= lastZ; ++sz)
        for (int sx = firstX; sx <= lastX; ++sx)
        {
            const Structures::Spot spot = Structures::inSquare(grid, sx, sz, m_seed);
            if (std::abs(spot.chunkX - chunkX) > REACH_CHUNKS) continue;
            if (std::abs(spot.chunkZ - chunkZ) > REACH_CHUNKS) continue;

            if (!bastionWins(spot.chunkX, spot.chunkZ)) continue;

            stampBastion(chunk,
                         spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2,
                         spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2);
        }
}

void WorldGen::stampBastion(Chunk& chunk, int midX, int midZ) const
{
    constexpr int HALF = BASTION_HALF;
    constexpr int TALL = 16;

    // A bastion stands over the lava rather than on the floor, which
    // is most of what makes one frightening to approach.
    const int base = NETHER_LAVA_LEVEL + 6;

    // Room round it first. The Nether is a closed cavern system, so a
    // bastion dropped into it without this is a bastion inside a
    // mountain: the first one built fine and could not be seen from
    // anywhere, because forty blocks of netherrack stood in front of
    // it. Minecraft's sits in the open too.
    constexpr int MARGIN = 10;
    for (int dx = -HALF - MARGIN; dx <= HALF + MARGIN; ++dx)
        for (int dz = -HALF - MARGIN; dz <= HALF + MARGIN; ++dz)
            for (int dy = -1; dy <= TALL + MARGIN; ++dy)
                put(chunk, midX + dx, base + dy, midZ + dz, Blocks::Air);

    for (int dx = -HALF; dx <= HALF; ++dx)
        for (int dz = -HALF; dz <= HALF; ++dz)
        {
            const int wx = midX + dx;
            const int wz = midZ + dz;
            const int far = std::max(std::abs(dx), std::abs(dz));

            for (int dy = 0; dy <= TALL; ++dy)
            {
                const int y = base + dy;
                const bool outer = far == HALF;
                const bool floor = dy == 0;
                const bool rampart = outer && dy <= TALL;

                BlockId block = Blocks::Air;

                if (floor)
                {
                    // Blackstone with basalt through it, and the gold
                    // that is the whole reason to go.
                    const float roll = hashToFloat(hashCoords(wx, y, wz, m_seed + 6602));
                    block = (roll < 0.04f) ? Blocks::GildedBlackstone
                          : (roll < 0.30f) ? Blocks::Basalt
                                           : Blocks::Blackstone;
                }
                else if (rampart)
                {
                    // Crenellations along the top, which is the
                    // silhouette of a fortification.
                    const bool notch = dy == TALL && ((dx + dz) % 2) == 0;
                    block = notch ? Blocks::Air : Blocks::PolishedBlackstoneBricks;
                }
                else if (dy == 1 && far < HALF - 2 && ((dx % 6) == 0 || (dz % 6) == 0))
                {
                    block = Blocks::PolishedBlackstoneBricks;
                }

                put(chunk, wx, y, wz, block);
            }
        }

    // The pillars the thing stands on, down into the lava.
    for (int dx = -HALF + 2; dx <= HALF - 2; dx += 6)
        for (int dz = -HALF + 2; dz <= HALF - 2; dz += 6)
            for (int y = base - 1; y > NETHER_LAVA_LEVEL - 4; --y)
                put(chunk, midX + dx, y, midZ + dz, Blocks::Blackstone);

    put(chunk, midX - 4, base + 1, midZ + 4, Blocks::Chest);
    put(chunk, midX + 4, base + 1, midZ - 4, Blocks::Chest);
}

// ---------------------------------------------------------------------
// The obsidian pillars, and the crystals on them.
//
// Ten of them in a ring round the middle of the main island, which is
// Minecraft's count. Their heights vary so the ring does not read as a
// fence; the tallest carry a cage round the crystal, which is what
// stops you simply shooting them all from the ground.
//
// Where they stand is arithmetic over the index rather than anything
// stored, so the generator and the dragon agree about where the
// crystals are without either having to tell the other.
// ---------------------------------------------------------------------
void WorldGen::endPillar(int index, int& outX, int& outZ, int& outTall)
{
    const float turn = 6.2831853f * static_cast<float>(index) /
                       static_cast<float>(END_PILLARS);

    outX = static_cast<int>(std::cos(turn) * END_PILLAR_RING);
    outZ = static_cast<int>(std::sin(turn) * END_PILLAR_RING);

    // Between 12 and 22, alternating rather than random: a ring whose
    // heights were rolled came out looking like a broken fence.
    outTall = 12 + (index % 5) * 2 + ((index % 2) ? 2 : 0);
}

void WorldGen::buildEndPillars(Chunk& chunk) const
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    // The ring is within sixty blocks of the origin, so only the
    // chunks near it need to think about this at all.
    if (std::abs(baseX) > END_PILLAR_RING + 40 ||
        std::abs(baseZ) > END_PILLAR_RING + 40)
        return;

    for (int i = 0; i < END_PILLARS; ++i)
    {
        int px = 0, pz = 0, tall = 0;
        endPillar(i, px, pz, tall);

        const int radius = 2 + (i % 3);
        const int top = END_ISLAND_LEVEL + tall;

        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
            {
                if (dx * dx + dz * dz > radius * radius) continue;

                // Down into the island rather than resting on it, so a
                // pillar does not end in mid-air where the ground
                // happens to be low.
                for (int y = END_ISLAND_LEVEL - 12; y <= top; ++y)
                    put(chunk, px + dx, y, pz + dz, Blocks::Obsidian);
            }

        // The crystal, on a slab of bedrock on the cap.
        put(chunk, px, top + 1, pz, Blocks::EndCrystalBase);
        put(chunk, px, top + 2, pz, Blocks::EndCrystal);

        // A cage of iron bars round the taller ones. There are no bars
        // here, so obsidian with gaps in it does the same job: it
        // means the tall crystals have to be reached rather than shot.
        if (tall < 18) continue;

        for (int dy = 1; dy <= 3; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                for (int dz = -1; dz <= 1; ++dz)
                {
                    if (dx == 0 && dz == 0) continue;
                    if (std::abs(dx) == 1 && std::abs(dz) == 1) continue;
                    if (dy == 2) continue;   // the gap you climb through
                    put(chunk, px + dx, top + dy, pz + dz, Blocks::Obsidian);
                }
    }
}

// ---------------------------------------------------------------------
// Geodes.
//
// A hollow ball of amethyst in a shell of calcite and smooth basalt,
// buried in the rock. Laid down by the spread rather than a grid --
// Minecraft scatters them the same way -- and the whole thing is built
// out of one distance test, which is what makes a ball a ball rather
// than a box with the corners filed off.
//
// The shells are what make it worth cutting into: you hit basalt,
// which is odd enough underground to make you keep going, then
// calcite, which tells you what you have found before you see it.
// ---------------------------------------------------------------------
void WorldGen::buildGeodes(Chunk& chunk) const
{
    const int chunkX = chunk.position().x;
    const int chunkZ = chunk.position().z;

    // A geode is about a dozen blocks across, so it can reach one
    // chunk past the one it is centred in.
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx)
        {
            const int cx = chunkX + dx;
            const int cz = chunkZ + dz;
            if (!Structures::spreadAt(cx, cz, m_seed, GEODE_SALT, GEODE_CHANCE)) continue;

            const int midX = cx * Chunk::SX + Chunk::SX / 2;
            const int midZ = cz * Chunk::SZ + Chunk::SZ / 2;

            const int midY = GEODE_MIN +
                static_cast<int>(hashToFloat(hashCoords(cx, 31, cz, m_seed + GEODE_SALT)) *
                                 static_cast<float>(GEODE_MAX - GEODE_MIN));

            // Between five and seven across the middle, which is
            // Minecraft's range and big enough to stand up in.
            const int inner = 4 +
                static_cast<int>(hashToFloat(hashCoords(cx, 32, cz, m_seed + GEODE_SALT)) * 3.0f);
            const int outer = inner + 3;

            for (int ox = -outer; ox <= outer; ++ox)
                for (int oy = -outer; oy <= outer; ++oy)
                    for (int oz = -outer; oz <= outer; ++oz)
                    {
                        const float away =
                            std::sqrt(static_cast<float>(ox * ox + oy * oy + oz * oz));
                        if (away > outer + 0.5f) continue;

                        const int wx = midX + ox;
                        const int wy = midY + oy;
                        const int wz = midZ + oz;
                        if (wy < 2 || wy >= Chunk::SY) continue;

                        BlockId block;
                        if (away > inner + 1.5f)      block = Blocks::SmoothBasalt;
                        else if (away > inner + 0.4f) block = Blocks::Calcite;
                        else if (away > inner - 0.8f)
                        {
                            // The lining. One block in eight is
                            // budding, which is what makes a geode
                            // worth coming back to rather than
                            // stripping once.
                            block = hashToFloat(hashCoords(wx, wy, wz, m_seed + 5601)) < 0.12f
                                        ? Blocks::BuddingAmethyst
                                        : Blocks::AmethystBlock;
                        }
                        else
                        {
                            block = Blocks::Air;
                        }

                        put(chunk, wx, wy, wz, block);
                    }

            // Clusters growing off the lining into the hollow. Done
            // after the shell, or the shell would overwrite them.
            for (int ox = -inner; ox <= inner; ++ox)
                for (int oy = -inner; oy <= inner; ++oy)
                    for (int oz = -inner; oz <= inner; ++oz)
                    {
                        const float away =
                            std::sqrt(static_cast<float>(ox * ox + oy * oy + oz * oz));
                        if (away < inner - 2.0f || away > inner - 0.8f) continue;
                        if (hashToFloat(hashCoords(midX + ox, midY + oy, midZ + oz,
                                                   m_seed + 5602)) > 0.22f)
                            continue;

                        put(chunk, midX + ox, midY + oy, midZ + oz, Blocks::AmethystCluster);
                    }
        }
}
