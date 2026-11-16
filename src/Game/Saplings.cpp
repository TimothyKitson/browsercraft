#include "Saplings.h"
#include "Items.h"
#include "Trees.h"
#include <algorithm>
#include <cstdlib>

namespace
{
    uint32_t next(uint32_t& seed)
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return seed;
    }

    bool roomy(BlockId id) { return id == Blocks::Air || isLeaves(id) || isSapling(id) || id == Blocks::TallGrass || isWater(id); }

    struct TreeCells : Trees::Cells
    {
        Saplings::Cells& cells;
        explicit TreeCells(Saplings::Cells& c) : cells(c) {}
        BlockId at(int x, int y, int z) const override { return cells.at(x, y, z); }
        void set(int x, int y, int z, BlockId id) override { cells.set(x, y, z, id); }
    };

    struct CellGround : Trees::Ground
    {
        const Saplings::Cells& cells;
        explicit CellGround(const Saplings::Cells& c) : cells(c) {}
        bool open(int x, int y, int z) const override { return Trees::replaceable(cells.at(x, y, z)); }
        bool muddy(int x, int y, int z) const override
        {
            const BlockId here = cells.at(x, y, z);
            return here == Blocks::Mud || here == Blocks::MuddyMangroveRoots || here == Blocks::MangroveRoots;
        }
    };

    bool growShape(Saplings::Cells& cells, Trees::Shape shape, const glm::ivec3& at, uint32_t seed, float bees, bool force)
    {
        CellGround ground(cells);
        const Trees::Plan plan = Trees::plan(shape, at, seed, bees, &ground);
        TreeCells adapter(cells);
        if (!plan.ok) return false;
        if (!force && !Trees::fits(shape, plan, adapter)) return false;
        Trees::write(plan, adapter);
        return true;
    }

    bool flowersNear(const Saplings::Cells& cells, const glm::ivec3& at)
    {
        for (int dx = -2; dx <= 2; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dz = -2; dz <= 2; ++dz)
                {
                    const BlockId here = cells.at(at.x + dx, at.y + dy, at.z + dz);
                    if (isSmallFlower(here) || here == Blocks::FloweringAzalea || here == Blocks::FloweringAzaleaLeaves) return true;
                    if (isDoublePlant(here) && doublePlantKind(here) != DoublePlant::TallGrass && doublePlantKind(here) != DoublePlant::LargeFern)
                        return true;
                }
        return false;
    }
}

bool Saplings::canPlantOn(BlockId ground)
{
    return ground == Blocks::Grass || ground == Blocks::Dirt || ground == Blocks::SnowGrass ||
           isFarmland(ground) || ground == Blocks::Mud || ground == Blocks::MuddyMangroveRoots ||
           ground == Blocks::Clay || isNylium(ground) || ground == Blocks::SoulSand;
}

bool Saplings::needsSquare(int wood) { return wood == 5; }

bool Saplings::bigTree(Cells& cells, const glm::ivec3& corner, int wood, uint32_t seed, bool force)
{
    if (seed == 0) seed = 0x9E3779B9u;
    const Trees::Shape shape = wood == 5 ? Trees::Shape::DarkOak : wood == 3 ? Trees::Shape::MegaJungle : Trees::Shape::MegaSpruce;
    return growShape(cells, shape, corner, seed, 0.0f, force);
}

bool Saplings::fancyOak(Cells& cells, const glm::ivec3& at, uint32_t seed, bool force)
{
    if (seed == 0) seed = 0x51F1A7u;
    return growShape(cells, Trees::Shape::FancyOak, at, seed, 0.0f, force);
}

bool Saplings::hugeFungus(Cells& cells, const glm::ivec3& at, bool warped, uint32_t seed)
{
    if (seed == 0) seed = 0x51ED27u;
    const BlockId stem = warped ? Blocks::WarpedStem : Blocks::CrimsonStem;
    const BlockId wart = warped ? Blocks::WarpedWartBlock : Blocks::NetherWartBlock;
    const int height = 4 + static_cast<int>(next(seed) % 9u);
    for (int dy = 1; dy < height + 2; ++dy)
        if (!roomy(cells.at(at.x, at.y + dy, at.z)) && !isNetherPlant(cells.at(at.x, at.y + dy, at.z))) return false;

    for (int i = 0; i < height; ++i) cells.set(at.x, at.y + i, at.z, stem);
    const int top = at.y + height - 1;
    const int radius = height > 8 ? 3 : 2;
    const int hat = std::min(height / 2 + 1, 5);
    auto wartAt = [&](int x, int y, int z) {
        const BlockId here = cells.at(x, y, z);
        if (here == Blocks::Air || isNetherPlant(here))
            cells.set(x, y, z, next(seed) % 14u == 0u ? Blocks::Shroomlight : wart);
    };
    for (int dx = -(radius - 1); dx <= radius - 1; ++dx)
        for (int dz = -(radius - 1); dz <= radius - 1; ++dz) wartAt(at.x + dx, top + 1, at.z + dz);
    for (int layer = 0; layer < hat; ++layer)
    {
        const int y = top - layer;
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
            {
                const bool edge = std::max(std::abs(dx), std::abs(dz)) == radius;
                const bool corner = std::abs(dx) == radius && std::abs(dz) == radius;
                if (corner && next(seed) % 2u == 0u) continue;
                if (edge || (layer == 0 && next(seed) % 3u == 0u)) wartAt(at.x + dx, y, at.z + dz);
            }
    }
    if (!warped)
    {
        const int bottom = top - hat;
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
            {
                if (cells.at(at.x + dx, bottom + 1, at.z + dz) != wart) continue;
                if (next(seed) % 4u != 0u) continue;
                const int length = 1 + static_cast<int>(next(seed) % 3u);
                for (int k = 0; k < length; ++k)
                {
                    if (cells.at(at.x + dx, bottom - k, at.z + dz) != Blocks::Air) break;
                    cells.set(at.x + dx, bottom - k, at.z + dz,
                              k == length - 1 ? Blocks::WeepingVines : Blocks::WeepingVinesPlant);
                }
            }
    }
    return true;
}

bool Saplings::findSquare(const Cells& cells, const glm::ivec3& at, glm::ivec3& corner)
{
    const BlockId kind = cells.at(at.x, at.y, at.z);
    for (int ox = -1; ox <= 0; ++ox)
        for (int oz = -1; oz <= 0; ++oz)
        {
            bool whole = true;
            for (int dx = 0; dx < 2 && whole; ++dx)
                for (int dz = 0; dz < 2 && whole; ++dz)
                    if (cells.at(at.x + ox + dx, at.y, at.z + oz + dz) != kind) whole = false;
            if (whole)
            {
                corner = glm::ivec3(at.x + ox, at.y, at.z + oz);
                return true;
            }
        }
    return false;
}

bool Saplings::grow(Cells& cells, const glm::ivec3& at, uint32_t seed)
{
    const BlockId here = cells.at(at.x, at.y, at.z);
    if (!isSapling(here)) return false;
    if (!canPlantOn(cells.at(at.x, at.y - 1, at.z))) return false;
    const int wood = saplingWood(here);
    if (isFungus(here))
    {
        const bool warped = here == Blocks::WarpedFungus;
        if (cells.at(at.x, at.y - 1, at.z) != (warped ? Blocks::WarpedNylium : Blocks::CrimsonNylium)) return false;
        cells.set(at.x, at.y, at.z, Blocks::Air);
        if (hugeFungus(cells, at, warped, seed)) return true;
        cells.set(at.x, at.y, at.z, here);
        return false;
    }
    if (seed == 0) seed = 0x9E3779B9u;
    uint32_t roll = seed;
    const float pick = static_cast<float>(next(roll) >> 8) / 16777216.0f;

    glm::ivec3 corner;
    const bool square = (wood == 1 || wood == 3 || wood == 5) && findSquare(cells, at, corner);
    if (square)
    {
        const Trees::Shape shape = wood == 5   ? Trees::Shape::DarkOak
                                   : wood == 3 ? Trees::Shape::MegaJungle
                                   : pick < 0.5f ? Trees::Shape::MegaPine
                                                 : Trees::Shape::MegaSpruce;
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz)
                cells.set(corner.x + dx, corner.y, corner.z + dz, Blocks::Air);
        if (growShape(cells, shape, corner, seed, 0.0f, false)) return true;
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz)
                cells.set(corner.x + dx, corner.y, corner.z + dz, here);
        return false;
    }
    if (needsSquare(wood)) return false;

    const bool flowers = flowersNear(cells, at);
    Trees::Shape shape = Trees::Shape::Oak;
    float bees = 0.0f;
    switch (wood)
    {
        case 0: shape = pick < 0.1f ? Trees::Shape::FancyOak : Trees::Shape::Oak; bees = flowers ? 0.05f : 0.0f; break;
        case 1: shape = Trees::Shape::Spruce; break;
        case 2: shape = Trees::Shape::Birch; bees = flowers ? 0.05f : 0.0f; break;
        case 3: shape = Trees::Shape::JungleNoVine; break;
        case 4: shape = Trees::Shape::Acacia; break;
        case 6: shape = Trees::Shape::Cherry; bees = flowers ? 0.05f : 0.0f; break;
        case 7: shape = pick < 0.85f ? Trees::Shape::TallMangrove : Trees::Shape::Mangrove; break;
        default: break;
    }
    cells.set(at.x, at.y, at.z, Blocks::Air);
    if (growShape(cells, shape, at, seed, bees, false)) return true;
    cells.set(at.x, at.y, at.z, here);
    return false;
}

BlockId Saplings::leafDrop(BlockId leaves, uint32_t roll)
{
    const int wood = woodOfLeaves(leaves);
    if (wood < 0 || wood == 7) return Blocks::Air;
    const uint32_t chance = wood == 3 ? 40u : 20u;
    if (roll % chance == 0u) return static_cast<BlockId>(Blocks::OakSapling + wood);
    return Blocks::Air;
}

bool Saplings::hugeMushroom(Cells& cells, const glm::ivec3& at, bool red, uint32_t seed, bool force)
{
    const BlockId ground = cells.at(at.x, at.y - 1, at.z);
    if (!force && ground != Blocks::Mycelium && ground != Blocks::Podzol && ground != Blocks::Dirt &&
        ground != Blocks::Grass && ground != Blocks::CoarseDirt && ground != Blocks::RootedDirt)
        return false;
    const int height = 4 + static_cast<int>(seed % 3u) + (red ? 0 : 1);
    const int reach = red ? 2 : 3;
    auto clear = [&](int x, int y, int z) {
        const BlockId here = cells.at(x, y, z);
        return here == Blocks::Air || isLeaves(here) || isWildPlant(here);
    };
    for (int dy = 1; dy <= height && !force; ++dy)
        if (!clear(at.x, at.y + dy, at.z)) return false;
    for (int dx = -reach; dx <= reach && !force; ++dx)
        for (int dz = -reach; dz <= reach; ++dz)
            for (int dy = red ? height - 3 : height; dy <= height; ++dy)
                if (!clear(at.x + dx, at.y + dy, at.z + dz)) return false;

    for (int dy = 0; dy < height; ++dy) cells.set(at.x, at.y + dy, at.z, Blocks::MushroomStem);
    const BlockId cap = red ? Blocks::RedMushroomBlock : Blocks::BrownMushroomBlock;
    if (red)
    {
        for (int dx = -1; dx <= 1; ++dx)
            for (int dz = -1; dz <= 1; ++dz) cells.set(at.x + dx, at.y + height, at.z + dz, cap);
        for (int dy = height - 3; dy < height; ++dy)
            for (int dx = -2; dx <= 2; ++dx)
                for (int dz = -2; dz <= 2; ++dz)
                {
                    const bool rim = std::abs(dx) == 2 || std::abs(dz) == 2;
                    const bool corner = std::abs(dx) == 2 && std::abs(dz) == 2;
                    if (rim && !corner) cells.set(at.x + dx, at.y + dy, at.z + dz, cap);
                }
    }
    else
    {
        for (int dx = -3; dx <= 3; ++dx)
            for (int dz = -3; dz <= 3; ++dz)
                if (!(std::abs(dx) == 3 && std::abs(dz) == 3)) cells.set(at.x + dx, at.y + height, at.z + dz, cap);
    }
    return true;
}

bool Saplings::azaleaTree(Cells& cells, const glm::ivec3& at, uint32_t seed, bool force)
{
    if (!force && !canPlantOn(cells.at(at.x, at.y - 1, at.z))) return false;
    const BlockId was = cells.at(at.x, at.y, at.z);
    cells.set(at.x, at.y, at.z, Blocks::Air);
    if (growShape(cells, Trees::Shape::Azalea, at, seed, 0.0f, force)) return true;
    cells.set(at.x, at.y, at.z, was);
    return false;
}
