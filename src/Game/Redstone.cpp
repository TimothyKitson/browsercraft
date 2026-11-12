#include "Redstone.h"
#include <algorithm>
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
        return isDustBlock(id);
    }

    bool isSink(BlockId id)
    {
        return id == Blocks::RedstoneLamp || id == Blocks::RedstoneLampLit ||
               isNoteBlock(id) || isDoor(id) || isTrapdoor(id) || isFenceGate(id) ||
               specialRailFamily(id) == 1 || specialRailFamily(id) == 3;
    }

    BlockId lit(BlockId id)
    {
        if (isDustBlock(id)) return Blocks::RedstoneDustLit;
        if (id == Blocks::RedstoneLamp) return Blocks::RedstoneLampLit;
        if (id == Blocks::NoteBlock) return Blocks::NoteBlockLit;
        if (isObserver(id) && !observerLit(id)) return id == Blocks::Observer ? Blocks::ObserverLit : observerFacingTo(sixFacing(id), true);
        if (specialRailFamily(id) == 1 || specialRailFamily(id) == 3)
            return specialRailWithPower(id, true);
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
        if (isDustBlock(id)) return Blocks::RedstoneDust;
        if (id == Blocks::RedstoneLampLit) return Blocks::RedstoneLamp;
        if (specialRailFamily(id) == 1 || specialRailFamily(id) == 3)
            return specialRailWithPower(id, false);
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
        if (isWire(near)) return dustPower(near) > 0;
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

    int levelInto(const Cells& cells, int x, int y, int z, int step)
    {
        const BlockId near = cells.at(x, y, z);
        if (isWire(near)) return dustPower(near);
        if (isComparator(near)) return comparatorLit(near) && stepOf(diodeOut(near)) == step ? cells.comparatorLevel(x, y, z) : 0;
        return feeds(cells, x, y, z, step) ? 15 : 0;
    }

    int levelAt(const Cells& cells, int x, int y, int z)
    {
        int best = 0;
        for (int k = 0; k < 6; ++k)
            best = std::max(best, levelInto(cells, x + DX[k], y + DY[k], z + DZ[k], k ^ 1));
        return best;
    }

    int diodeInputLevel(const Cells& cells, int x, int y, int z)
    {
        const BlockId self = cells.at(x, y, z);
        if (!isDiode(self)) return 0;
        const int forward = stepOf(diodeOut(self));
        const int back = forward ^ 1;
        return levelInto(cells, x + DX[back], y + DY[back], z + DZ[back], forward);
    }

    int diodeSideLevel(const Cells& cells, int x, int y, int z)
    {
        const BlockId self = cells.at(x, y, z);
        if (!isDiode(self)) return 0;
        const int forward = stepOf(diodeOut(self));
        int best = 0;
        for (int k = 0; k < 6; ++k)
        {
            if (DY[k] != 0 || k == forward || k == (forward ^ 1)) continue;
            const BlockId near = cells.at(x + DX[k], y, z + DZ[k]);
            if (!isWire(near) && !isDiode(near) && near != Blocks::RedstoneBlock) continue;
            best = std::max(best, levelInto(cells, x + DX[k], y, z + DZ[k], k ^ 1));
        }
        return best;
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
        std::array<std::vector<glm::ivec3>, 16> buckets;

        auto offer = [&](int nx, int ny, int nz, int power) {
            const int ox = nx - x, oy = ny - y, oz = nz - z;
            if (power <= 0 || std::abs(ox) > REACH || std::abs(oy) > REACH || std::abs(oz) > REACH) return;
            if (!isWire(cells.at(nx, ny, nz))) return;
            uint8_t& level = reached[indexOf(ox, oy, oz)];
            if (level >= power) return;
            level = static_cast<uint8_t>(power);
            buckets[static_cast<size_t>(power)].push_back(glm::ivec3(nx, ny, nz));
        };

        for (int dx = -REACH; dx <= REACH; ++dx)
            for (int dy = -REACH; dy <= REACH; ++dy)
                for (int dz = -REACH; dz <= REACH; ++dz)
                {
                    const int cx = x + dx, cy = y + dy, cz = z + dz;
                    const BlockId start = cells.at(cx, cy, cz);
                    const bool aimed = isDiode(start) && diodeLit(start);
                    const bool watcher = isObserver(start) && observerLit(start);
                    if (!isSource(start) && !aimed && !carries(cells, cx, cy, cz))
                        continue;
                    const int power = aimed && isComparator(start) ? cells.comparatorLevel(cx, cy, cz) : MAX_RUN;

                    for (int k = 0; k < 6; ++k)
                    {
                        if (aimed && k != stepOf(diodeOut(start))) continue;
                        if (watcher && k != observerBack(start)) continue;
                        offer(cx + DX[k], cy + DY[k], cz + DZ[k], power);
                    }
                }

        for (int power = MAX_RUN; power >= 2; --power)
        {
            std::vector<glm::ivec3> batch;
            batch.swap(buckets[static_cast<size_t>(power)]);
            for (const glm::ivec3& here : batch)
            {
                const int ox = here.x - x, oy = here.y - y, oz = here.z - z;
                if (reached[indexOf(ox, oy, oz)] != power) continue;
                for (int k = 0; k < 6; ++k)
                    offer(here.x + DX[k], here.y + DY[k], here.z + DZ[k], power - 1);
                for (int k = 0; k < 6; ++k)
                {
                    if (DY[k] != 0) continue;
                    const int sx = here.x + DX[k], sz = here.z + DZ[k];
                    for (int dy = -1; dy <= 1; dy += 2)
                    {
                        if (dy > 0 && isSolid(cells.at(here.x, here.y + 1, here.z))) continue;
                        if (dy < 0 && isSolid(cells.at(sx, here.y, sz))) continue;
                        offer(sx, here.y + dy, sz, power - 1);
                    }
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

                    const BlockId want = dustOfPower(reached[indexOf(dx, dy, dz)]);
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
