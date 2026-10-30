#include "Redstone.h"
#include <array>
#include <queue>
#include <vector>

namespace
{
    // The six cells a signal can step to. Dust climbs nothing: in
    // Minecraft it runs up and down the side of a block it is laid
    // against, and that is a shape this engine's dust does not have.
    constexpr int DX[6] = { 1, -1, 0, 0, 0, 0 };
    constexpr int DY[6] = { 0, 0, 1, -1, 0, 0 };
    constexpr int DZ[6] = { 0, 0, 0, 0, 1, -1 };

    constexpr int SPAN = Redstone::REACH * 2 + 1;

    inline int indexOf(int dx, int dy, int dz)
    {
        return ((dx + Redstone::REACH) * SPAN + (dy + Redstone::REACH)) * SPAN +
               (dz + Redstone::REACH);
    }
}

namespace Redstone
{
    bool isSource(BlockId id)
    {
        // An observer that has just seen something is a source for
        // as long as its pulse lasts, which is what makes it useful:
        // it turns a change into a signal.
        if (id == Blocks::ObserverLit) return true;
        return id == Blocks::RedstoneTorch || id == Blocks::RedstoneBlock ||
               id == Blocks::LeverOn || id == Blocks::ButtonOn;
    }

    bool isWire(BlockId id)
    {
        return id == Blocks::RedstoneDust || id == Blocks::RedstoneDustLit;
    }

    bool isSink(BlockId id)
    {
        return id == Blocks::RedstoneLamp || id == Blocks::RedstoneLampLit ||
               isNoteBlock(id) || isDoor(id);
    }

    BlockId lit(BlockId id)
    {
        if (id == Blocks::RedstoneDust) return Blocks::RedstoneDustLit;
        if (id == Blocks::RedstoneLamp) return Blocks::RedstoneLampLit;
        if (id == Blocks::NoteBlock) return Blocks::NoteBlockLit;
        if (id == Blocks::Observer) return Blocks::ObserverLit;
        if (isDoor(id) && !doorIsOpen(id))
            return doorPart(blockFacing(id), doorIsTop(id), true);
        return id;
    }

    BlockId unlit(BlockId id)
    {
        if (id == Blocks::NoteBlockLit) return Blocks::NoteBlock;
        if (id == Blocks::ObserverLit) return Blocks::Observer;
        if (id == Blocks::RedstoneDustLit) return Blocks::RedstoneDust;
        if (id == Blocks::RedstoneLampLit) return Blocks::RedstoneLamp;
        if (isDoor(id) && doorIsOpen(id))
            return doorPart(blockFacing(id), doorIsTop(id), false);
        return id;
    }

    BlockId toggled(BlockId id)
    {
        switch (id)
        {
            case Blocks::LeverOff:  return Blocks::LeverOn;
            case Blocks::LeverOn:   return Blocks::LeverOff;
            case Blocks::ButtonOff: return Blocks::ButtonOn;
            case Blocks::ButtonOn:  return Blocks::ButtonOff;
            default: return id;
        }
    }

    bool carries(const Cells& cells, int x, int y, int z)
    {
        return isSolid(cells.at(x, y, z)) && !isWire(cells.at(x, y, z)) &&
               isSource(cells.at(x, y + 1, z));
    }

    bool poweredAt(const Cells& cells, int x, int y, int z)
    {
        for (int k = 0; k < 6; ++k)
        {
            const int nx = x + DX[k], ny = y + DY[k], nz = z + DZ[k];
            const BlockId near = cells.at(nx, ny, nz);
            if (isSource(near)) return true;
            if (near == Blocks::RedstoneDustLit) return true;
            if (carries(cells, nx, ny, nz)) return true;
        }
        return false;
    }

    // Every wire in reach, relit from scratch. Starting from the sources
    // rather than from the change is what makes this one rule rather
    // than two: breaking the last torch in a line and laying a new one
    // both come out of the same walk.
    void update(Cells& cells, int x, int y, int z)
    {
        std::vector<uint8_t> reached(static_cast<size_t>(SPAN) * SPAN * SPAN, 0);
        std::queue<glm::ivec4> open;   // x, y, z, steps used

        // Dust next to a source is lit, and the walk goes on from there.
        for (int dx = -REACH; dx <= REACH; ++dx)
            for (int dy = -REACH; dy <= REACH; ++dy)
                for (int dz = -REACH; dz <= REACH; ++dz)
                {
                    const int cx = x + dx, cy = y + dy, cz = z + dz;

                    // A source, or the block one is standing on: a
                    // lever on a cobble has no dust of its own to
                    // touch, and the cobble is what the line is laid
                    // against.
                    if (!isSource(cells.at(cx, cy, cz)) && !carries(cells, cx, cy, cz))
                        continue;

                    for (int k = 0; k < 6; ++k)
                    {
                        const int nx = cx + DX[k], ny = cy + DY[k], nz = cz + DZ[k];
                        const int ox = nx - x, oy = ny - y, oz = nz - z;
                        if (std::abs(ox) > REACH || std::abs(oy) > REACH ||
                            std::abs(oz) > REACH)
                            continue;
                        if (!isWire(cells.at(nx, ny, nz))) continue;
                        if (reached[indexOf(ox, oy, oz)]) continue;

                        reached[indexOf(ox, oy, oz)] = 1;
                        open.push(glm::ivec4(nx, ny, nz, 1));
                    }
                }

        while (!open.empty())
        {
            const glm::ivec4 here = open.front();
            open.pop();
            if (here.w >= MAX_RUN) continue;

            for (int k = 0; k < 6; ++k)
            {
                const int nx = here.x + DX[k], ny = here.y + DY[k], nz = here.z + DZ[k];
                const int ox = nx - x, oy = ny - y, oz = nz - z;
                if (std::abs(ox) > REACH || std::abs(oy) > REACH || std::abs(oz) > REACH)
                    continue;
                if (!isWire(cells.at(nx, ny, nz))) continue;
                if (reached[indexOf(ox, oy, oz)]) continue;

                reached[indexOf(ox, oy, oz)] = 1;
                open.push(glm::ivec4(nx, ny, nz, here.w + 1));
            }
        }

        // Write the wires back first: what a lamp or a door sees next
        // depends on which dust ended up lit.
        for (int dx = -REACH; dx <= REACH; ++dx)
            for (int dy = -REACH; dy <= REACH; ++dy)
                for (int dz = -REACH; dz <= REACH; ++dz)
                {
                    const int cx = x + dx, cy = y + dy, cz = z + dz;
                    const BlockId here = cells.at(cx, cy, cz);
                    if (!isWire(here)) continue;

                    const BlockId want = reached[indexOf(dx, dy, dz)] ? lit(here) : unlit(here);
                    if (want != here) cells.set(cx, cy, cz, want);
                }

        for (int dx = -REACH; dx <= REACH; ++dx)
            for (int dy = -REACH; dy <= REACH; ++dy)
                for (int dz = -REACH; dz <= REACH; ++dz)
                {
                    const int cx = x + dx, cy = y + dy, cz = z + dz;
                    const BlockId here = cells.at(cx, cy, cz);
                    if (!isSink(here)) continue;

                    // A door is two blocks and swings as one, so each
                    // half answers to what either half can see.
                    bool powered = poweredAt(cells, cx, cy, cz);
                    if (isDoor(here) && !powered)
                    {
                        const int other = doorIsTop(here) ? -1 : 1;
                        if (isDoor(cells.at(cx, cy + other, cz)))
                            powered = poweredAt(cells, cx, cy + other, cz);
                    }

                    const BlockId want = powered ? lit(here) : unlit(here);
                    if (want != here) cells.set(cx, cy, cz, want);
                }
    }
}
