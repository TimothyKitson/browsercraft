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
    if (!isCrop(crop)) return out;

    // What this crop is sown from. For the roots it is the food
    // itself, which is why pulling one up early costs nothing.
    const StackId seed = isWheat(crop)     ? Items::WheatSeeds
                       : isCarrot(crop)    ? Items::Carrot
                       : isPotato(crop)    ? Items::Potato
                       : isBeetroot(crop)  ? Items::BeetrootSeeds
                                           : Items::MelonSeeds;

    // A stem is never ripe in the sense the others are: what it grows
    // stands beside it. Broken, it gives back its seed whatever stage
    // it had reached.
    if (isMelonStem(crop))
    {
        out.first = Items::MelonSeeds;
        out.firstCount = 1;
        return out;
    }

    if (!isCropRipe(crop))
    {
        // Pulled up early: you get your seed back and nothing else.
        out.first = seed;
        out.firstCount = 1;
        return out;
    }

    if (isWheat(crop))
    {
        out.first = Items::Wheat;
        out.firstCount = 1;
        out.second = Items::WheatSeeds;
        out.secondCount = 1 + static_cast<int>(roll % 3u);   // one to three
        return out;
    }

    // A beet is the seed and the food apart, the way wheat is.
    if (isBeetroot(crop))
    {
        out.first = Items::Beetroot;
        out.firstCount = 1;
        out.second = Items::BeetrootSeeds;
        out.secondCount = 1 + static_cast<int>(roll % 3u);
        return out;
    }

    // A root is its own seed, so a ripe one pays several of the same
    // thing: two to five, which is Minecraft's and is what makes a
    // field grow from one carrot.
    out.first = seed;
    out.firstCount = 2 + static_cast<int>(roll % 4u);
    return out;
}

BlockId Farming::cropFromSeed(StackId held)
{
    switch (held)
    {
        case Items::WheatSeeds:    return Blocks::Wheat0;
        case Items::Carrot:        return Blocks::Carrot0;
        case Items::Potato:        return Blocks::Potato0;
        case Items::BeetrootSeeds: return Blocks::Beetroot0;
        case Items::MelonSeeds:    return Blocks::MelonStem0;
        default:                   return Blocks::Air;
    }
}

bool Farming::plant(World& world, const glm::ivec3& ground, BlockId crop)
{
    if (!isCrop(crop)) return false;
    if (!isTillable(world.getBlock(ground.x, ground.y, ground.z))) return false;

    const glm::ivec3 above(ground.x, ground.y + 1, ground.z);
    const BlockId there = world.getBlock(above.x, above.y, above.z);
    if (there != Blocks::Air && there != Blocks::TallGrass) return false;

    world.setBlock(ground.x, ground.y, ground.z, Blocks::Farmland);
    world.setBlock(above.x, above.y, above.z, cropAtStage(crop, 0));
    return true;
}

bool Farming::growBambooAt(World& world, const glm::ivec3& cane)
{
    if (world.getBlock(cane.x, cane.y + 1, cane.z) != Blocks::Air) return false;

    // Count down to the ground. A cane that has reached its height
    // stops, which is what keeps a grove a grove rather than a wall
    // from the floor to the sky.
    int height = 0;
    for (int y = cane.y; y >= 0; --y)
    {
        const BlockId here = world.getBlock(cane.x, y, cane.z);
        if (here == Blocks::Bamboo) { ++height; continue; }

        // Only the ground a cane will stand on.
        if (here != Blocks::Grass && here != Blocks::Dirt &&
            here != Blocks::Sand && here != Blocks::Moss) return false;
        break;
    }

    if (height <= 0 || height >= BAMBOO_MAX) return false;

    world.setBlock(cane.x, cane.y + 1, cane.z, Blocks::Bamboo);
    return true;
}

bool Farming::growMelonBeside(World& world, const glm::ivec3& stem, uint32_t& seed)
{
    // Tilled earth under the stem, the same as any other crop.
    if (world.getBlock(stem.x, stem.y - 1, stem.z) != Blocks::Farmland) return false;

    static const int DX[] = { 1, -1, 0, 0 };
    static const int DZ[] = { 0, 0, 1, -1 };

    // Start somewhere different each time, or every stem in a field
    // would put its melon on the same side and a row of them would
    // read as a wall.
    const int from = static_cast<int>(nextRandom(seed) % 4u);

    for (int i = 0; i < 4; ++i)
    {
        const int side = (from + i) % 4;
        const int x = stem.x + DX[side];
        const int z = stem.z + DZ[side];

        if (world.getBlock(x, stem.y, z) != Blocks::Air) continue;

        const BlockId under = world.getBlock(x, stem.y - 1, z);
        if (under != Blocks::Grass && under != Blocks::Dirt &&
            under != Blocks::Farmland) continue;

        world.setBlock(x, stem.y, z, Blocks::Melon);
        return true;
    }
    return false;
}

bool Farming::canGrowAt(const World& world, const glm::ivec3& at)
{
    const BlockId crop = world.getBlock(at.x, at.y, at.z);
    if (!isCrop(crop) || isCropRipe(crop)) return false;

    // A crop needs tilled earth under it and daylight or a torch on it.
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

        // Cocoa, which ripens where it hangs. Three stages rather
        // than eight, and no farmland under it: it is a pod on a
        // trunk, not a crop in a field.
        {
            const BlockId pod = world.getBlock(x, y, z);
            if (isCocoa(pod))
            {
                if (cocoaStage(pod) < COCOA_RIPE)
                    world.setBlock(x, y, z, cocoaAt(cocoaStage(pod) + 1));
                continue;
            }
        }

        // Bamboo, which grows faster than anything in the game and
        // needs nothing but ground and light. Ticked by this pass for
        // the same reason the cane is: a second sampling pass would
        // cost the same again to find the same columns.
        if (world.getBlock(x, y, z) == Blocks::Bamboo)
        {
            growBambooAt(world, glm::ivec3(x, y, z));
            continue;
        }

        // A ripe stem does not grow any further: it puts a melon on
        // the ground beside it, and goes on doing so as long as there
        // is ground beside it to put one on. That is the whole of
        // what makes a melon different from the four crops.
        const BlockId standing = world.getBlock(x, y, z);
        if (isMelonStem(standing) && isCropRipe(standing))
        {
            growMelonBeside(world, glm::ivec3(x, y, z), seed);
            continue;
        }

        if (!canGrowAt(world, at)) continue;

        const BlockId crop = world.getBlock(x, y, z);
        world.setBlock(x, y, z, cropAtStage(crop, cropStage(crop) + 1));
    }
}
