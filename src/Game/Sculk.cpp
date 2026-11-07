#include "Sculk.h"
#include "World/Noise.h"

bool Sculk::replaceable(BlockId id)
{
    switch (id)
    {
        case Blocks::Stone: case Blocks::Deepslate: case Blocks::CobbledDeepslate: case Blocks::Tuff:
        case Blocks::Andesite: case Blocks::Granite: case Blocks::Diorite: case Blocks::Calcite:
        case Blocks::Dirt: case Blocks::Grass: case Blocks::SnowGrass: case Blocks::CoarseDirt: case Blocks::RootedDirt:
        case Blocks::Podzol: case Blocks::Mycelium: case Blocks::Mud: case Blocks::Clay:
        case Blocks::Sand: case Blocks::RedSand: case Blocks::Gravel: case Blocks::Terracotta:
        case Blocks::Netherrack: case Blocks::SoulSand: case Blocks::SoulSoil: case Blocks::EndStone:
            return true;
        default:
            return false;
    }
}

namespace
{
    bool open(BlockId id) { return id == Blocks::Air || isSculkVein(id); }

    int groundAt(const Sculk::Cells& cells, int x, int top, int bottom, int z)
    {
        for (int y = top; y >= bottom; --y)
            if (isOpaque(cells.at(x, y, z)) && open(cells.at(x, y + 1, z))) return y;
        return -1000;
    }
}

int Sculk::spread(Cells& cells, const glm::ivec3& centre, int charges, uint32_t seed)
{
    static const int DX[4] = { 1, -1, 0, 0 };
    static const int DZ[4] = { 0, 0, 1, -1 };
    int changed = 0;
    for (int k = 0; k < charges; ++k)
    {
        const int x = centre.x + static_cast<int>(hashToFloat(hashCoords(k, 1, centre.z, seed)) * 7.0f) - 3;
        const int z = centre.z + static_cast<int>(hashToFloat(hashCoords(k, 2, centre.x, seed)) * 7.0f) - 3;
        const int y = groundAt(cells, x, centre.y + 2, centre.y - 3, z);
        if (y < -999) continue;
        const BlockId ground = cells.at(x, y, z);
        if (replaceable(ground))
        {
            cells.set(x, y, z, Blocks::Sculk);
            ++changed;
            if (isSculkVein(cells.at(x, y + 1, z))) cells.set(x, y + 1, z, Blocks::Air);
        }
        else if (ground != Blocks::Sculk)
            continue;
        for (int d = 0; d < 4; ++d)
        {
            const int nx = x + DX[d], nz = z + DZ[d];
            const int ny = groundAt(cells, nx, y + 1, y - 1, nz);
            if (ny < -999) continue;
            const BlockId next = cells.at(nx, ny, nz);
            if (next == Blocks::Sculk || cells.at(nx, ny + 1, nz) != Blocks::Air) continue;
            if (hashToFloat(hashCoords(nx, ny + k, nz, seed + 17u)) < 0.6f)
            {
                cells.set(nx, ny + 1, nz, sculkVeinOn(0));
                ++changed;
            }
        }
    }
    return changed;
}
