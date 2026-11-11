#include "Saplings.h"
#include "Items.h"
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

    void leaf(Saplings::Cells& cells, int x, int y, int z, BlockId leaves)
    {
        const BlockId here = cells.at(x, y, z);
        if (here == Blocks::Air || here == Blocks::TallGrass) cells.set(x, y, z, leaves);
    }

    void blob(Saplings::Cells& cells, int cx, int y, int cz, int radius, BlockId leaves, uint32_t& seed, bool round)
    {
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
            {
                if (round && std::abs(dx) == radius && std::abs(dz) == radius && (next(seed) % 3u) != 0u) continue;
                if (dx * dx + dz * dz > radius * radius + radius) continue;
                leaf(cells, cx + dx, y, cz + dz, leaves);
            }
    }

    bool columnClear(const Saplings::Cells& cells, int x, int y, int z, int height, int width)
    {
        for (int dy = 1; dy < height; ++dy)
            for (int dx = 0; dx < width; ++dx)
                for (int dz = 0; dz < width; ++dz)
                    if (!roomy(cells.at(x + dx, y + dy, z + dz))) return false;
        return true;
    }

    bool small(Saplings::Cells& cells, const glm::ivec3& at, int wood, uint32_t& seed)
    {
        const BlockId log = logOfWood(wood);
        const BlockId leaves = leavesOfWood(wood);
        int height = 4 + static_cast<int>(next(seed) % 3u);
        if (wood == 2) height += 1;
        if (wood == 1) height += 2;
        if (wood == 3) height += static_cast<int>(next(seed) % 4u);
        if (!columnClear(cells, at.x, at.y, at.z, height + 2, 1)) return false;

        int topX = at.x, topZ = at.z;
        if (wood == 4)
        {
            const int way = static_cast<int>(next(seed) % 4u);
            const int sx = way == 0 ? 1 : way == 1 ? -1 : 0;
            const int sz = way == 2 ? 1 : way == 3 ? -1 : 0;
            for (int i = 0; i < height; ++i)
            {
                const int step = (height - i <= 2) ? (height - i == 2 ? 1 : 2) : 0;
                cells.set(at.x + sx * step, at.y + i, at.z + sz * step, log);
            }
            topX = at.x + sx * 2;
            topZ = at.z + sz * 2;
            const int top = at.y + height;
            blob(cells, topX, top - 1, topZ, 3, leaves, seed, true);
            blob(cells, topX, top, topZ, 2, leaves, seed, true);
            return true;
        }

        if (wood == 7)
        {
            const int lift = 2;
            if (!columnClear(cells, at.x, at.y + lift, at.z, height + 2, 1)) return false;
            auto root = [&](int x, int y, int z) {
                const BlockId here = cells.at(x, y, z);
                if (here == Blocks::Air || isWater(here) || here == Blocks::TallGrass || isSapling(here))
                    cells.set(x, y, z, Blocks::MangroveRoots);
            };
            root(at.x, at.y, at.z);
            root(at.x, at.y + 1, at.z);
            static const int LEGS[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
            for (const auto& leg : LEGS)
            {
                if (next(seed) % 4u == 0u) continue;
                root(at.x + leg[0], at.y + 1, at.z + leg[1]);
                for (int y = at.y; y >= at.y - 3; --y)
                {
                    const BlockId below = cells.at(at.x + leg[0] * 2, y, at.z + leg[1] * 2);
                    if (below != Blocks::Air && !isWater(below) && below != Blocks::TallGrass) break;
                    root(at.x + leg[0] * 2, y, at.z + leg[1] * 2);
                }
            }
            for (int i = 0; i < height; ++i) cells.set(at.x, at.y + lift + i, at.z, log);
            const int top = at.y + lift + height - 1;
            blob(cells, at.x, top - 1, at.z, 3, leaves, seed, true);
            blob(cells, at.x, top, at.z, 3, leaves, seed, true);
            blob(cells, at.x, top + 1, at.z, 2, leaves, seed, true);
            for (int dx = -3; dx <= 3; ++dx)
                for (int dz = -3; dz <= 3; ++dz)
                    if (cells.at(at.x + dx, top - 1, at.z + dz) == leaves &&
                        cells.at(at.x + dx, top - 2, at.z + dz) == Blocks::Air && next(seed) % 7u == 0u)
                        cells.set(at.x + dx, top - 2, at.z + dz, Blocks::MangrovePropaguleHanging);
            return true;
        }

        if (wood == 6)
        {
            for (int i = 0; i < height; ++i) cells.set(at.x, at.y + i, at.z, log);
            const int top = at.y + height - 1;
            const int ways = 1 + static_cast<int>(next(seed) % 2u);
            for (int branch = 0; branch < ways; ++branch)
            {
                const int way = static_cast<int>(next(seed) % 4u);
                const int dx = way == 0 ? 1 : way == 1 ? -1 : 0;
                const int dz = way == 2 ? 1 : way == 3 ? -1 : 0;
                for (int k = 1; k <= 3; ++k) cells.set(at.x + dx * k, top - 1 + k, at.z + dz * k, log);
                const int tx = at.x + dx * 3, ty = top + 2, tz = at.z + dz * 3;
                blob(cells, tx, ty - 1, tz, 3, leaves, seed, true);
                blob(cells, tx, ty, tz, 3, leaves, seed, true);
                blob(cells, tx, ty + 1, tz, 2, leaves, seed, true);
            }
            blob(cells, at.x, top, at.z, 3, leaves, seed, true);
            blob(cells, at.x, top + 1, at.z, 2, leaves, seed, true);
            return true;
        }

        for (int i = 0; i < height; ++i) cells.set(at.x, at.y + i, at.z, log);
        const int top = at.y + height - 1;
        if (wood == 1)
        {
            for (int dy = -height + 2; dy <= 1; ++dy)
            {
                const int radius = dy >= 1 ? 0 : ((dy & 1) ? 1 : 2);
                if (top + dy <= at.y + 1) continue;
                blob(cells, at.x, top + dy, at.z, radius, leaves, seed, false);
            }
            leaf(cells, at.x, top + 2, at.z, leaves);
            return true;
        }
        blob(cells, at.x, top - 1, at.z, 2, leaves, seed, true);
        blob(cells, at.x, top, at.z, 2, leaves, seed, true);
        blob(cells, at.x, top + 1, at.z, 1, leaves, seed, false);
        leaf(cells, at.x, top + 2, at.z, leaves);
        return true;
    }

    bool big(Saplings::Cells& cells, const glm::ivec3& corner, int wood, uint32_t& seed, bool force = false)
    {
        const BlockId log = logOfWood(wood);
        const BlockId leaves = leavesOfWood(wood);
        const bool jungle = wood == 3;
        const bool spruce = wood == 1;
        const int height = jungle ? 12 + static_cast<int>(next(seed) % 8u)
                         : spruce ? 14 + static_cast<int>(next(seed) % 8u)
                                  : 6 + static_cast<int>(next(seed) % 3u);
        if (!force && !columnClear(cells, corner.x, corner.y, corner.z, height + 2, 2)) return false;
        if (spruce)
        {
            for (int i = 0; i < height; ++i)
                for (int dx = 0; dx < 2; ++dx)
                    for (int dz = 0; dz < 2; ++dz)
                        cells.set(corner.x + dx, corner.y + i, corner.z + dz, log);
            const int top = corner.y + height;
            const int crown = height * 2 / 3;
            for (int layer = 0; layer < crown; ++layer)
            {
                const int y = top - layer;
                const int radius = std::min(4, 1 + layer / 3 - (layer % 3 == 2 ? 1 : 0));
                for (int dx = -radius; dx <= radius + 1; ++dx)
                    for (int dz = -radius; dz <= radius + 1; ++dz)
                    {
                        const float ox = static_cast<float>(dx) - 0.5f, oz = static_cast<float>(dz) - 0.5f;
                        if (ox * ox + oz * oz > static_cast<float>(radius * radius) + 0.8f) continue;
                        leaf(cells, corner.x + dx, y, corner.z + dz, leaves);
                    }
            }
            return true;
        }

        for (int i = 0; i < height; ++i)
            for (int dx = 0; dx < 2; ++dx)
                for (int dz = 0; dz < 2; ++dz)
                    cells.set(corner.x + dx, corner.y + i, corner.z + dz, log);

        const int top = corner.y + height - 1;
        const int cx = corner.x, cz = corner.z;
        if (jungle)
        {
            for (int dy = -2; dy <= 1; ++dy)
            {
                const int radius = dy == 1 ? 2 : dy == 0 ? 4 : 3;
                blob(cells, cx, top + dy, cz, radius, leaves, seed, true);
                blob(cells, cx + 1, top + dy, cz + 1, radius, leaves, seed, true);
            }
            for (int branch = 0; branch < 3; ++branch)
            {
                const int y = corner.y + height / 2 + static_cast<int>(next(seed) % static_cast<uint32_t>(height / 3 + 1));
                const int way = static_cast<int>(next(seed) % 4u);
                const int bx = way == 0 ? 2 : way == 1 ? -1 : 0;
                const int bz = way == 2 ? 2 : way == 3 ? -1 : 0;
                if (y >= top - 2) continue;
                cells.set(cx + bx, y, cz + bz, log);
                blob(cells, cx + bx * 2, y + 1, cz + bz * 2, 2, leaves, seed, true);
            }
            return true;
        }

        for (int dy = -1; dy <= 1; ++dy)
        {
            const int radius = dy == 1 ? 2 : 3;
            blob(cells, cx, top + dy, cz, radius, leaves, seed, true);
            blob(cells, cx + 1, top + dy, cz + 1, radius, leaves, seed, true);
        }
        for (int dx = -1; dx <= 2; ++dx)
            for (int dz = -1; dz <= 2; ++dz)
                leaf(cells, cx + dx, top + 2, cz + dz, leaves);
        return true;
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
    return big(cells, corner, wood, seed, force);
}

bool Saplings::fancyOak(Cells& cells, const glm::ivec3& at, uint32_t seed, bool force)
{
    if (seed == 0) seed = 0x51F1A7u;
    const BlockId log = logOfWood(0);
    const BlockId leaves = leavesOfWood(0);
    const int height = 6 + static_cast<int>(next(seed) % 6u);
    if (!force && !columnClear(cells, at.x, at.y, at.z, height + 3, 1)) return false;
    for (int i = 0; i < height; ++i) cells.set(at.x, at.y + i, at.z, log);
    auto ball = [&](int cx, int cy, int cz) {
        for (int dy = 0; dy < 4; ++dy)
        {
            const int radius = dy == 0 || dy == 3 ? 2 : 3;
            blob(cells, cx, cy + dy, cz, radius - (dy == 3 ? 1 : 0), leaves, seed, true);
        }
    };
    const int top = at.y + height - 1;
    ball(at.x, top - 1, at.z);
    const int branches = 2 + static_cast<int>(next(seed) % 3u);
    static const int WAYS[8][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }, { 1, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 } };
    for (int b = 0; b < branches; ++b)
    {
        const int way = static_cast<int>(next(seed) % 8u);
        const int from = at.y + height / 2 + static_cast<int>(next(seed) % static_cast<uint32_t>(std::max(1, height / 2 - 1)));
        const int length = 2 + static_cast<int>(next(seed) % 3u);
        int x = at.x, y = from, z = at.z;
        for (int k = 1; k <= length; ++k)
        {
            x = at.x + WAYS[way][0] * k;
            z = at.z + WAYS[way][1] * k;
            y = from + k / 2;
            cells.set(x, y, z, log);
        }
        ball(x, y, z);
    }
    return true;
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

    glm::ivec3 corner;
    const bool square = (wood == 3 || wood == 5) && findSquare(cells, at, corner);
    if (square)
    {
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz)
                cells.set(corner.x + dx, corner.y, corner.z + dz, Blocks::Air);
        if (big(cells, corner, wood, seed)) return true;
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz)
                cells.set(corner.x + dx, corner.y, corner.z + dz, here);
        return false;
    }
    if (needsSquare(wood)) return false;

    cells.set(at.x, at.y, at.z, Blocks::Air);
    if (small(cells, at, wood, seed)) return true;
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
    const int height = 4 + static_cast<int>(seed % 2u);
    auto open = [&](int x, int y, int z) {
        const BlockId here = cells.at(x, y, z);
        return here == Blocks::Air || isLeaves(here) || isWildPlant(here);
    };
    if (!force)
    {
        if (!canPlantOn(cells.at(at.x, at.y - 1, at.z))) return false;
        for (int dy = 1; dy <= height + 1; ++dy)
            if (!open(at.x, at.y + dy, at.z)) return false;
    }
    cells.set(at.x, at.y - 1, at.z, Blocks::RootedDirt);
    for (int dy = 0; dy < height; ++dy) cells.set(at.x, at.y + dy, at.z, Blocks::Log);
    uint32_t roll = seed * 2654435761u + 7u;
    auto next = [&roll]() { roll = roll * 1664525u + 1013904223u; return (roll >> 8) & 0xFFFFu; };
    for (int dy = height - 2; dy <= height; ++dy)
    {
        const int reach = dy == height ? 1 : 2;
        for (int dx = -reach; dx <= reach; ++dx)
            for (int dz = -reach; dz <= reach; ++dz)
            {
                if (std::abs(dx) == reach && std::abs(dz) == reach && next() % 3u != 0u) continue;
                const glm::ivec3 p(at.x + dx, at.y + dy, at.z + dz);
                if (!open(p.x, p.y, p.z)) continue;
                if (dx == 0 && dz == 0 && dy < height) continue;
                cells.set(p.x, p.y, p.z, next() % 4u == 0u ? Blocks::FloweringAzaleaLeaves : Blocks::AzaleaLeaves);
            }
    }
    return true;
}
