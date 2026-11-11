#include "Redstone.h"
#include <array>
#include <queue>
#include <vector>

namespace
{
    constexpr int DX[6] = { 1, -1, 0, 0, 0, 0 };
    constexpr int DY[6] = { 0, 0, 1, -1, 0, 0 };
    constexpr int DZ[6] = { 0, 0, 0, 0, 1, -1 };

    constexpr int SPAN = Redstone::REACH * 2 + 1;

    inline int observerBack(BlockId id)
    {
        static const int BACK[6] = { 4, 1, 5, 0, 3, 2 };
        return BACK[sixFacing(id)];
    }

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
        if (isObserver(id) && observerLit(id)) return true;
        if (isTrappedChest(id) && chestState(id).open) return true;
        if (lightningRodPowered(id)) return true;
        if (daylightLit(id) || id == Blocks::TargetLit || hookPowered(id)) return true;
        if (sculkSensorLit(id)) return true;
        if (buttonPowered(id) || platePowered(id) || (isFamilyLever(id) && leverOn(id))) return true;
        if (isWallTorch(id) && wallTorchKind(id) == 2) return true;
        if (specialRailFamily(id) == 2 && specialRailPowered(id)) return true;
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
               isNoteBlock(id) || isDoor(id) || isTrapdoor(id) || isFenceGate(id) ||
               specialRailFamily(id) == 1 || specialRailFamily(id) == 3;
    }

    BlockId lit(BlockId id)
    {
        if (id == Blocks::RedstoneDust) return Blocks::RedstoneDustLit;
        if (id == Blocks::RedstoneLamp) return Blocks::RedstoneLampLit;
        if (id == Blocks::NoteBlock) return Blocks::NoteBlockLit;
        if (isObserver(id) && !observerLit(id)) return id == Blocks::Observer ? Blocks::ObserverLit : observerFacingTo(sixFacing(id), true);
        if (specialRailFamily(id) == 1 || specialRailFamily(id) == 3)
            return specialRailOf(specialRailFamily(id), specialRailEastWest(id), true);
        if (isDoor(id) && !doorIsOpen(id))
            return doorPart(doorBase(id), blockFacing(id), doorIsTop(id), true);
        if (isTrapdoor(id) && !trapdoorIsOpen(id))
            return trapdoorPart(trapdoorBase(id), trapdoorFacing(id), trapdoorIsTop(id), true);
        if (isFenceGate(id) && !gateOpen(id)) return gateOf(gateWood(id), true, gateFacing(id));
        return id;
    }

    BlockId unlit(BlockId id)
    {
        if (id == Blocks::NoteBlockLit) return Blocks::NoteBlock;
        if (isObserver(id) && observerLit(id)) return id == Blocks::ObserverLit ? Blocks::Observer : observerFacingTo(sixFacing(id), false);
        if (id == Blocks::RedstoneDustLit) return Blocks::RedstoneDust;
        if (id == Blocks::RedstoneLampLit) return Blocks::RedstoneLamp;
        if (specialRailFamily(id) == 1 || specialRailFamily(id) == 3)
            return specialRailOf(specialRailFamily(id), specialRailEastWest(id), false);
        if (isDoor(id) && doorIsOpen(id))
            return doorPart(doorBase(id), blockFacing(id), doorIsTop(id), false);
        if (isTrapdoor(id) && trapdoorIsOpen(id))
            return trapdoorPart(trapdoorBase(id), trapdoorFacing(id), trapdoorIsTop(id), false);
        if (isFenceGate(id) && gateOpen(id)) return gateOf(gateWood(id), false, gateFacing(id));
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

    int stepOf(int facing)
    {
        switch (facing & 3)
        {
            case 0: return 5;
            case 1: return 0;
            case 2: return 4;
            default: return 1;
        }
    }

    bool carries(const Cells& cells, int x, int y, int z)
    {
        const BlockId self = cells.at(x, y, z);
        if (!isSolid(self) || isWire(self) || isDiode(self)) return false;
        const BlockId above = cells.at(x, y + 1, z);
        if (isSource(above) && !isObserver(above)) return true;
        for (int k = 0; k < 6; ++k)
        {
            const BlockId near = cells.at(x + DX[k], y + DY[k], z + DZ[k]);
            if (isObserver(near) && observerLit(near) && observerBack(near) == (k ^ 1)) return true;
        }
        for (int k = 0; k < 6; ++k)
        {
            if (DY[k] != 0) continue;
            const BlockId near = cells.at(x + DX[k], y, z + DZ[k]);
            if (isDiode(near) && diodeLit(near) && stepOf(diodeOut(near)) == (k ^ 1)) return true;
            if (hookPowered(near) && stepOf(hookFacing(near)) == k) return true;
        }
        return false;
    }

    bool feeds(const Cells& cells, int x, int y, int z, int step)
    {
        const BlockId near = cells.at(x, y, z);
        if (isObserver(near)) return (observerLit(near) && observerBack(near) == step) || carries(cells, x, y, z);
        if (isSource(near)) return true;
        if (near == Blocks::RedstoneDustLit) return true;
        if (isDiode(near)) return diodeLit(near) && stepOf(diodeOut(near)) == step;
        return carries(cells, x, y, z);
    }

    bool poweredAt(const Cells& cells, int x, int y, int z)
    {
        for (int k = 0; k < 6; ++k)
            if (feeds(cells, x + DX[k], y + DY[k], z + DZ[k], k ^ 1)) return true;
        return false;
    }

    bool diodeInput(const Cells& cells, int x, int y, int z)
    {
        const BlockId self = cells.at(x, y, z);
        if (!isDiode(self)) return false;
        const int forward = stepOf(diodeOut(self));
        const int back = forward ^ 1;
        return feeds(cells, x + DX[back], y + DY[back], z + DZ[back], forward);
    }

    bool diodeSide(const Cells& cells, int x, int y, int z)
    {
        const BlockId self = cells.at(x, y, z);
        if (!isDiode(self)) return false;
        const int forward = stepOf(diodeOut(self));
        for (int k = 0; k < 6; ++k)
        {
            if (DY[k] != 0 || k == forward || k == (forward ^ 1)) continue;
            const BlockId near = cells.at(x + DX[k], y, z + DZ[k]);
            if (!isWire(near) && !isDiode(near) && near != Blocks::RedstoneBlock) continue;
            if (feeds(cells, x + DX[k], y, z + DZ[k], k ^ 1)) return true;
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
                    const BlockId start = cells.at(cx, cy, cz);
                    const bool aimed = isDiode(start) && diodeLit(start);
                    const bool watcher = isObserver(start) && observerLit(start);
                    if (!isSource(start) && !aimed && !carries(cells, cx, cy, cz))
                        continue;

                    for (int k = 0; k < 6; ++k)
                    {
                        if (aimed && k != stepOf(diodeOut(start))) continue;
                        if (watcher && k != observerBack(start)) continue;
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

            for (int k = 0; k < 6; ++k)
            {
                if (DY[k] != 0) continue;
                const int sx = here.x + DX[k], sz = here.z + DZ[k];
                for (int dy = -1; dy <= 1; dy += 2)
                {
                    if (dy > 0 && isSolid(cells.at(here.x, here.y + 1, here.z))) continue;
                    if (dy < 0 && isSolid(cells.at(sx, here.y, sz))) continue;
                    const int ny = here.y + dy;
                    const int ox = sx - x, oy = ny - y, oz = sz - z;
                    if (std::abs(ox) > REACH || std::abs(oy) > REACH || std::abs(oz) > REACH)
                        continue;
                    if (!isWire(cells.at(sx, ny, sz))) continue;
                    if (reached[indexOf(ox, oy, oz)]) continue;

                    reached[indexOf(ox, oy, oz)] = 1;
                    open.push(glm::ivec4(sx, ny, sz, here.w + 1));
                }
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
