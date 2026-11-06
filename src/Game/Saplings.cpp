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

    bool roomy(BlockId id) { return id == Blocks::Air || isLeaves(id) || isSapling(id) || id == Blocks::TallGrass; }

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

    bool big(Saplings::Cells& cells, const glm::ivec3& corner, int wood, uint32_t& seed)
    {
        const BlockId log = logOfWood(wood);
        const BlockId leaves = leavesOfWood(wood);
        const bool jungle = wood == 3;
        const int height = jungle ? 12 + static_cast<int>(next(seed) % 8u) : 6 + static_cast<int>(next(seed) % 3u);
        if (!columnClear(cells, corner.x, corner.y, corner.z, height + 2, 2)) return false;

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
           ground == Blocks::Farmland;
}

bool Saplings::needsSquare(int wood) { return wood == 5; }

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
    if (wood < 0) return Blocks::Air;
    const uint32_t chance = wood == 3 ? 40u : 20u;
    if (roll % chance == 0u) return static_cast<BlockId>(Blocks::OakSapling + wood);
    return Blocks::Air;
}
