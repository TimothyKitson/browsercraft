#include "Farming.h"
#include "Cane.h"
#include "World/World.h"
#include <cmath>

namespace
{
    uint32_t nextRandom(uint32_t& state)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
}

bool Farming::isTillable(BlockId id)
{
    return id == Blocks::Dirt || id == Blocks::Grass || id == Blocks::SnowGrass;
}

// The cane rules take the smallest interface that answers them rather
// than a World, so --selftest can run them on a made-up waterside. This
// is the one that reads the real world.
namespace
{
    struct WorldCane : Cane::Blocks
    {
        const World* world = nullptr;
        BlockId at(int x, int y, int z) const override
        {
            return world->getBlock(x, y, z);
        }
    };
}

Farming::Harvest Farming::harvestOf(BlockId crop, uint32_t roll)
{
    Harvest out;
    if (!isWheat(crop)) return out;

    if (crop != WHEAT_RIPE)
    {
        // Pulled up early: you get your seed back and nothing else.
        out.first = Items::WheatSeeds;
        out.firstCount = 1;
        return out;
    }

    out.first = Items::Wheat;
    out.firstCount = 1;
    out.second = Items::WheatSeeds;
    out.secondCount = 1 + static_cast<int>(roll % 3u);   // one to three
    return out;
}

bool Farming::plant(World& world, const glm::ivec3& ground)
{
    if (!isTillable(world.getBlock(ground.x, ground.y, ground.z))) return false;

    const glm::ivec3 above(ground.x, ground.y + 1, ground.z);
    const BlockId there = world.getBlock(above.x, above.y, above.z);
    if (there != Blocks::Air && there != Blocks::TallGrass) return false;

    world.setBlock(ground.x, ground.y, ground.z, Blocks::Farmland);
    world.setBlock(above.x, above.y, above.z, Blocks::Wheat0);
    return true;
}

bool Farming::canGrowAt(const World& world, const glm::ivec3& at)
{
    const BlockId crop = world.getBlock(at.x, at.y, at.z);
    if (!isWheat(crop) || crop == WHEAT_RIPE) return false;

    // Wheat needs tilled earth under it and daylight or a torch on it.
    if (world.getBlock(at.x, at.y - 1, at.z) != Blocks::Farmland) return false;

    const int light = std::max<int>(world.skyLight(at.x, at.y, at.z),
                                    world.blockLightAt(at.x, at.y, at.z));
    return light >= LIGHT_TO_GROW;
}

void Farming::grow(World& world, const glm::vec3& around, uint32_t& seed)
{
    const int centreX = static_cast<int>(std::floor(around.x));
    const int centreY = static_cast<int>(std::floor(around.y));
    const int centreZ = static_cast<int>(std::floor(around.z));

    for (int i = 0; i < TICKS_PER_PASS; ++i)
    {
        // A column near the player, and a height near their own: crops
        // grow where people are, which is where anyone can see them.
        const int x = centreX + static_cast<int>(nextRandom(seed) % (TICK_RADIUS * 2u)) - TICK_RADIUS;
        const int z = centreZ + static_cast<int>(nextRandom(seed) % (TICK_RADIUS * 2u)) - TICK_RADIUS;
        const int y = centreY + static_cast<int>(nextRandom(seed) % (TICK_HEIGHT * 2u + 1u)) - TICK_HEIGHT;
        if (y < 1) continue;

        const glm::ivec3 at(x, y, z);

        // Cane is ticked by the same pass rather than one of its own:
        // it grows where people are for the same reason wheat does, and
        // a second sampling pass would cost the same again for nothing.
        if (world.getBlock(x, y, z) == Blocks::SugarCane)
        {
            WorldCane around;
            around.world = &world;
            if (Cane::canGrowAt(around, x, y, z))
                world.setBlock(x, y + 1, z, Blocks::SugarCane);
            continue;
        }

        if (!canGrowAt(world, at)) continue;

        world.setBlock(x, y, z, wheatAtStage(wheatStage(world.getBlock(x, y, z)) + 1));
    }
}
