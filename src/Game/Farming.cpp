#include "Saplings.h"
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
                       : isPumpkinStem(crop) ? Items::PumpkinSeeds
                                           : Items::MelonSeeds;

    // A stem is never ripe in the sense the others are: what it grows
    // stands beside it. Broken, it gives back its seed whatever stage
    // it had reached.
    if (isMelonStem(crop) || isPumpkinStem(crop))
    {
        out.first = seed;
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
    if (isPotato(crop) && ((roll * 2654435761u) >> 20) % 50u == 0u)
    {
        out.second = Items::PoisonousPotato;
        out.secondCount = 1;
    }
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
        case Items::PumpkinSeeds:  return Blocks::PumpkinStem0;
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
        if (!bambooCanRootIn(here)) return false;
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

    static const int DX[] = { 0, 1, 0, -1 };
    static const int DZ[] = { -1, 0, 1, 0 };
    const bool pumpkin = isPumpkinStem(world.getBlock(stem.x, stem.y, stem.z));

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

        world.setBlock(x, stem.y, z, pumpkin ? Blocks::Pumpkin : Blocks::Melon);
        world.setBlock(stem.x, stem.y, stem.z, attachedStemOf(pumpkin, side));
        return true;
    }
    return false;
}

bool Farming::unhookStem(World& world, const glm::ivec3& stem)
{
    const BlockId here = world.getBlock(stem.x, stem.y, stem.z);
    if (!isAttachedStem(here)) return false;
    static const int DX[] = { 0, 1, 0, -1 };
    static const int DZ[] = { -1, 0, 1, 0 };
    const int f = attachedStemFacing(here);
    const BlockId fruit = world.getBlock(stem.x + DX[f], stem.y, stem.z + DZ[f]);
    const bool pumpkin = attachedStemIsPumpkin(here);
    if (fruit == (pumpkin ? Blocks::Pumpkin : Blocks::Melon)) return false;
    world.setBlock(stem.x, stem.y, stem.z, pumpkin ? Blocks::PumpkinStem7 : Blocks::MelonStem7);
    return true;
}

void Farming::unhookStemsAround(World& world, const glm::ivec3& fruit)
{
    static const int DX[] = { 0, 1, 0, -1 };
    static const int DZ[] = { -1, 0, 1, 0 };
    for (int k = 0; k < 4; ++k) unhookStem(world, glm::ivec3(fruit.x + DX[k], fruit.y, fruit.z + DZ[k]));
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

bool Farming::coralIsWet(const World& world, int x, int y, int z)
{
    static const int AROUND[6][3] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    for (const auto& d : AROUND)
        if (isWater(world.getBlock(x + d[0], y + d[1], z + d[2]))) return true;
    return false;
}

void Farming::grow(World& world, const glm::vec3& around, uint32_t& seed, std::vector<glm::ivec3>* hatched,
                   std::vector<glm::ivec3>* snifflings, std::vector<glm::ivec3>* tadpoles)
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

        {
            const BlockId reef = world.getBlock(x, y, z);
            if (isLivingCoral(reef))
            {
                if (!coralIsWet(world, x, y, z)) world.setBlock(x, y, z, deadCoralOf(reef));
                continue;
            }
            if (reef == Blocks::Frogspawn)
            {
                if (!isWater(world.getBlock(x, y - 1, z)))
                {
                    world.setBlock(x, y, z, Blocks::Air);
                    continue;
                }
                if (nextRandom(seed) % 9u != 0u) continue;
                world.setBlock(x, y, z, Blocks::Air);
                if (tadpoles)
                {
                    const int brood = 2 + static_cast<int>(nextRandom(seed) % 5u);
                    for (int k = 0; k < brood; ++k) tadpoles->push_back(glm::ivec3(x, y - 1, z));
                }
                continue;
            }
            if (isSnifferEgg(reef))
            {
                const bool moss = world.getBlock(x, y - 1, z) == Blocks::Moss;
                if (nextRandom(seed) % (moss ? 11u : 23u) != 0u) continue;
                if (reef != Blocks::SnifferEgg2) world.setBlock(x, y, z, static_cast<BlockId>(reef + 1));
                else
                {
                    world.setBlock(x, y, z, Blocks::Air);
                    if (snifflings) snifflings->push_back(glm::ivec3(x, y, z));
                }
                continue;
            }
            if (isTurtleEgg(reef))
            {
                if (world.getBlock(x, y - 1, z) != Blocks::Sand || nextRandom(seed) % 3u != 0u) continue;
                if (turtleEggHatch(reef) < 2)
                    world.setBlock(x, y, z, turtleEggOf(turtleEggCount(reef), turtleEggHatch(reef) + 1));
                else
                {
                    world.setBlock(x, y, z, Blocks::Air);
                    if (hatched)
                        for (int k = 0; k < turtleEggCount(reef); ++k) hatched->push_back(glm::ivec3(x, y, z));
                }
                continue;
            }
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

        {
            const BlockId sprout = world.getBlock(x, y, z);
            if (isTorchflowerCrop(sprout) || (isPitcherCrop(sprout) && !pitcherCropUpper(sprout)))
            {
                if (world.getBlock(x, y - 1, z) != Blocks::Farmland) continue;
                if (nextRandom(seed) % 3u != 0u) continue;
                if (sprout == Blocks::TorchflowerCrop0) world.setBlock(x, y, z, Blocks::TorchflowerCrop1);
                else if (sprout == Blocks::TorchflowerCrop1) world.setBlock(x, y, z, Blocks::Torchflower);
                else
                {
                    const int stage = pitcherCropStage(sprout);
                    if (stage >= 4) continue;
                    const BlockId overhead = world.getBlock(x, y + 1, z);
                    if (stage + 1 >= 3 && overhead != Blocks::Air && !pitcherCropUpper(overhead)) continue;
                    world.setBlock(x, y, z, pitcherCropOf(stage + 1, false));
                    if (stage + 1 >= 3) world.setBlock(x, y + 1, z, pitcherCropOf(stage + 1, true));
                }
                continue;
            }
        }

        {
            const BlockId vine = world.getBlock(x, y, z);
            if (caveVineHead(vine))
            {
                if (nextRandom(seed) % 6u == 0u && world.getBlock(x, y - 1, z) == Blocks::Air && y > 2)
                {
                    world.setBlock(x, y, z, caveVineOf(false, caveVineLit(vine)));
                    world.setBlock(x, y - 1, z, caveVineOf(true, nextRandom(seed) % 100u < 11u));
                }
                continue;
            }
        }

        {
            const BlockId spore = world.getBlock(x, y, z);
            if (isMushroom(spore))
            {
                if (nextRandom(seed) % 25u != 0u) continue;
                const int light = std::max<int>(world.skyLight(x, y, z), world.blockLightAt(x, y, z));
                if (light > 12) continue;
                int near = 0;
                for (int dx = -4; dx <= 4; ++dx)
                    for (int dy = -1; dy <= 1; ++dy)
                        for (int dz = -4; dz <= 4; ++dz)
                            if (isMushroom(world.getBlock(x + dx, y + dy, z + dz))) ++near;
                if (near >= 5) continue;
                const int nx = x + static_cast<int>(nextRandom(seed) % 3u) - 1;
                const int ny = y + static_cast<int>(nextRandom(seed) % 3u) - 1;
                const int nz = z + static_cast<int>(nextRandom(seed) % 3u) - 1;
                if (world.getBlock(nx, ny, nz) == Blocks::Air && isOpaque(world.getBlock(nx, ny - 1, nz)) &&
                    std::max<int>(world.skyLight(nx, ny, nz), world.blockLightAt(nx, ny, nz)) <= 12)
                    world.setBlock(nx, ny, nz, spore);
                continue;
            }
        }

        if (isFungus(world.getBlock(x, y, z))) continue;
        if (isSapling(world.getBlock(x, y, z)))
        {
            if (nextRandom(seed) % 4u == 0u)
            {
                struct WorldCells : Saplings::Cells
                {
                    World* world = nullptr;
                    BlockId at(int cx, int cy, int cz) const override { return world->getBlock(cx, cy, cz); }
                    void set(int cx, int cy, int cz, BlockId id) override { world->setBlock(cx, cy, cz, id); }
                } cells;
                cells.world = &world;
                Saplings::grow(cells, at, nextRandom(seed));
            }
            continue;
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
        if (world.getBlock(x, y, z) == Blocks::BambooShoot)
        {
            if (nextRandom(seed) % 3u == 0u && world.getBlock(x, y + 1, z) == Blocks::Air) world.setBlock(x, y, z, Blocks::Bamboo);
            continue;
        }

        if (world.getBlock(x, y, z) == Blocks::BuddingAmethyst)
        {
            if (nextRandom(seed) % 5u == 0u)
            {
                static const int AWAY[6][3] = { { 0, 1, 0 }, { 0, -1, 0 }, { 1, 0, 0 },
                                                { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
                const int* d = AWAY[nextRandom(seed) % 6u];
                const int gx = x + d[0], gy = y + d[1], gz = z + d[2];
                const BlockId there = world.getBlock(gx, gy, gz);
                if (there == Blocks::Air || there == Blocks::Water)
                    world.setBlock(gx, gy, gz, Blocks::SmallAmethystBud);
                else if (isAmethystGrowth(there) && there != Blocks::AmethystCluster)
                    world.setBlock(gx, gy, gz, amethystAtStage(amethystStage(there) + 1));
            }
            continue;
        }

        {
            const BlockId wart = world.getBlock(x, y, z);
            if (wart == Blocks::NetherWart0 || wart == Blocks::NetherWart1)
            {
                if (nextRandom(seed) % 3u == 0u)
                    world.setBlock(x, y, z, wart == Blocks::NetherWart0 ? Blocks::NetherWart1
                                                                     : Blocks::NetherWart);
                continue;
            }
        }

        // A ripe stem does not grow any further: it puts a melon on
        // the ground beside it, and goes on doing so as long as there
        // is ground beside it to put one on. That is the whole of
        // what makes a melon different from the four crops.
        const BlockId standing = world.getBlock(x, y, z);
        if ((isMelonStem(standing) || isPumpkinStem(standing)) && isCropRipe(standing))
        {
            growMelonBeside(world, glm::ivec3(x, y, z), seed);
            continue;
        }
        if (isAttachedStem(standing))
        {
            unhookStem(world, glm::ivec3(x, y, z));
            continue;
        }
        if (isSweetBerryBush(standing))
        {
            if (sweetBerryStage(standing) < 3 && nextRandom(seed) % 5u == 0u &&
                world.skyLight(x, y + 1, z) >= LIGHT_TO_GROW)
                world.setBlock(x, y, z, static_cast<BlockId>(standing + 1));
            continue;
        }

        if (!canGrowAt(world, at)) continue;

        const BlockId crop = world.getBlock(x, y, z);
        world.setBlock(x, y, z, cropAtStage(crop, cropStage(crop) + 1));
    }
}
