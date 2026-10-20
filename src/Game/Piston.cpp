#include "Piston.h"

void Piston::step(int facing, int& dx, int& dz)
{
    switch (facing & 3)
    {
        case 0:  dx = 0;  dz = -1; break;   // north
        case 1:  dx = 1;  dz = 0;  break;   // east
        case 2:  dx = 0;  dz = 1;  break;   // south
        default: dx = -1; dz = 0;  break;   // west
    }
}

bool Piston::replaceable(BlockId id)
{
    return id == Blocks::Air || isLiquid(id);
}

bool Piston::movable(BlockId id)
{
    if (replaceable(id)) return false;
    if (blockInfo(id).hardness < 0.0f) return false;   // bedrock, the vaults, the spawners

    // A piston mid-push is two blocks pretending to be one. Shoving
    // either half would leave the other behind.
    if (pistonOut(id) || isPistonHead(id)) return false;

    // Things that are really somewhere else: a chest's contents and a
    // door's other half live outside the block, and moving the block
    // without them loses them.
    if (id == Blocks::Chest || isDoor(id) || id == Blocks::Bed) return false;
    if (id == Blocks::Furnace || id == Blocks::FurnaceLit) return false;

    return true;
}

int Piston::lineLength(const Redstone::Cells& cells, int x, int y, int z, int facing)
{
    int dx = 0, dz = 0;
    step(facing, dx, dz);

    int count = 0;
    for (int i = 1; i <= MAX_PUSH + 1; ++i)
    {
        const BlockId ahead = cells.at(x + dx * i, y, z + dz * i);
        if (replaceable(ahead)) return count;
        if (!movable(ahead)) return -1;
        if (++count > MAX_PUSH) return -1;
    }
    return -1;
}

bool Piston::extend(Redstone::Cells& cells, int x, int y, int z)
{
    const BlockId body = cells.at(x, y, z);
    if (!isPistonBody(body) || pistonOut(body)) return false;

    const int facing = pistonFacing(body);
    const int length = lineLength(cells, x, y, z, facing);
    if (length < 0) return false;

    int dx = 0, dz = 0;
    step(facing, dx, dz);

    // From the far end back, so each block is written into a cell that
    // has already been read.
    for (int i = length; i >= 1; --i)
    {
        const BlockId moving = cells.at(x + dx * i, y, z + dz * i);
        cells.set(x + dx * (i + 1), y, z + dz * (i + 1), moving);
    }

    cells.set(x + dx, y, z + dz, pistonHead(facing, pistonSticky(body)));
    cells.set(x, y, z, pistonBody(facing, pistonSticky(body), true));
    return true;
}

bool Piston::retract(Redstone::Cells& cells, int x, int y, int z)
{
    const BlockId body = cells.at(x, y, z);
    if (!isPistonBody(body) || !pistonOut(body)) return false;

    const int facing = pistonFacing(body);
    int dx = 0, dz = 0;
    step(facing, dx, dz);

    const bool sticky = pistonSticky(body);

    // The head comes back whatever happens.
    cells.set(x + dx, y, z + dz, Blocks::Air);
    cells.set(x, y, z, pistonBody(facing, sticky, false));

    if (sticky)
    {
        const BlockId stuck = cells.at(x + dx * 2, y, z + dz * 2);
        if (movable(stuck))
        {
            cells.set(x + dx * 2, y, z + dz * 2, Blocks::Air);
            cells.set(x + dx, y, z + dz, stuck);
        }
    }

    return true;
}
