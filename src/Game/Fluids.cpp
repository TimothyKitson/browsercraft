#include "Fluids.h"
#include <algorithm>

namespace Fluids
{
    namespace
    {
        constexpr int SIDES[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
        constexpr int SPAN = 1 << 25;

        uint64_t keyOf(int x, int y, int z)
        {
            return (static_cast<uint64_t>(x + SPAN) << 38) |
                   (static_cast<uint64_t>(z + SPAN) << 12) |
                   static_cast<uint64_t>(y & 0xFFF);
        }

        void unkey(uint64_t key, int& x, int& y, int& z)
        {
            x = static_cast<int>((key >> 38) & 0x3FFFFFF) - SPAN;
            z = static_cast<int>((key >> 12) & 0x3FFFFFF) - SPAN;
            y = static_cast<int>(key & 0xFFF);
        }

        bool passable(BlockId id, BlockId base)
        {
            if (id == Blocks::Air || washesAway(id)) return true;
            return fluidBase(id) == base && !isFluidSource(id);
        }

        bool opensBelow(const Cells& cells, int x, int y, int z, BlockId base)
        {
            const BlockId below = cells.at(x, y - 1, z);
            return fluidBase(below) == base || below == Blocks::Air || washesAway(below);
        }

        int holeDistance(const Cells& cells, int x, int y, int z, BlockId base,
                         int depth, int from, int limit)
        {
            int best = 1000;
            for (int d = 0; d < 4; ++d)
            {
                if (d == from) continue;
                const int nx = x + SIDES[d][0];
                const int nz = z + SIDES[d][1];
                if (!passable(cells.at(nx, y, nz), base)) continue;
                if (opensBelow(cells, nx, y, nz, base)) return depth;
                if (depth < limit)
                    best = std::min(best, holeDistance(cells, nx, y, nz, base, depth + 1, (d + 2) & 3, limit));
            }
            return best;
        }

        int sourcesBeside(const Cells& cells, int x, int y, int z, BlockId base)
        {
            int count = 0;
            for (const auto& s : SIDES)
                if (cells.at(x + s[0], y, z + s[1]) == base) ++count;
            return count;
        }

        BlockId settledState(const Cells& cells, int x, int y, int z, BlockId base, bool nether)
        {
            int strongest = 0;
            int sources = 0;
            for (const auto& s : SIDES)
            {
                const BlockId next = cells.at(x + s[0], y, z + s[1]);
                if (fluidBase(next) != base) continue;
                if (isFluidSource(next)) ++sources;
                strongest = std::max(strongest, fluidAmount(next));
            }

            if (base == Blocks::Water && sources >= 2)
            {
                const BlockId below = cells.at(x, y - 1, z);
                if (blockInfo(below).collides || below == Blocks::Water) return Blocks::Water;
            }

            if (fluidBase(cells.at(x, y + 1, z)) == base) return flowingFluid(base, 8, true);

            const int amount = strongest - dropPerBlock(base, nether);
            if (amount <= 0) return Blocks::Air;
            return flowingFluid(base, amount, false);
        }

        void flowSideways(Cells& cells, int x, int y, int z, BlockId here, bool nether)
        {
            const BlockId base = fluidBase(here);
            const int amount = isFluidFalling(here) ? 7 : fluidAmount(here) - dropPerBlock(base, nether);
            if (amount <= 0) return;

            constexpr int BLOCKED = 1 << 20;
            int distance[4];
            int best = BLOCKED;
            for (int d = 0; d < 4; ++d)
            {
                distance[d] = BLOCKED;
                const int nx = x + SIDES[d][0];
                const int nz = z + SIDES[d][1];
                if (!passable(cells.at(nx, y, nz), base)) continue;
                distance[d] = opensBelow(cells, nx, y, nz, base)
                                  ? 0
                                  : holeDistance(cells, nx, y, nz, base, 1, (d + 2) & 3, searchDepth(base, nether));
                best = std::min(best, distance[d]);
            }

            const BlockId flowing = flowingFluid(base, amount, false);
            for (int d = 0; d < 4; ++d)
            {
                if (distance[d] != best || best >= BLOCKED) continue;
                const int nx = x + SIDES[d][0];
                const int nz = z + SIDES[d][1];
                const BlockId there = cells.at(nx, y, nz);
                if (fluidBase(there) == base)
                {
                    if (isFluidFalling(there) || fluidAmount(there) >= amount) continue;
                }
                else if (there != Blocks::Air && !washesAway(there))
                {
                    continue;
                }
                cells.set(nx, y, nz, flowing);
            }
        }
    }

    int tickDelay(BlockId fluid, bool nether)
    {
        if (isLava(fluid)) return nether ? 10 : 30;
        return 5;
    }

    int dropPerBlock(BlockId fluid, bool nether)
    {
        if (isLava(fluid)) return nether ? 1 : 2;
        return 1;
    }

    int searchDepth(BlockId fluid, bool nether)
    {
        if (isLava(fluid)) return nether ? 4 : 2;
        return 4;
    }

    bool washesAway(BlockId id)
    {
        if (id == Blocks::Air) return false;
        if (isFluid(id) || isWaterPlant(id) || id == Blocks::SugarCane) return false;
        return isCross(id) && !blockInfo(id).collides;
    }

    void update(Cells& cells, int x, int y, int z, bool nether)
    {
        BlockId here = cells.at(x, y, z);
        if (!isFluid(here)) return;
        const BlockId base = fluidBase(here);

        if (base == Blocks::Lava)
        {
            const int around[5][3] = { { 0, 0, -1 }, { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 } };
            for (const auto& a : around)
            {
                if (!isWater(cells.at(x + a[0], y + a[1], z + a[2]))) continue;
                cells.set(x, y, z, isFluidSource(here) ? Blocks::Obsidian : Blocks::Cobblestone);
                return;
            }
        }

        if (!isFluidSource(here))
        {
            const BlockId want = settledState(cells, x, y, z, base, nether);
            if (want != here)
            {
                cells.set(x, y, z, want);
                if (!isFluid(want)) return;
                here = want;
            }
        }

        const BlockId below = cells.at(x, y - 1, z);
        if (base == Blocks::Lava && isWater(below))
        {
            cells.set(x, y - 1, z, Blocks::Stone);
            return;
        }

        const bool downOpen = below == Blocks::Air || washesAway(below) ||
                              (fluidBase(below) == base && !isFluidSource(below));
        if (downOpen)
        {
            const BlockId falling = flowingFluid(base, 8, true);
            if (below != falling) cells.set(x, y - 1, z, falling);
            if (sourcesBeside(cells, x, y, z, base) >= 3) flowSideways(cells, x, y, z, here, nether);
        }
        else if (isFluidSource(here) || fluidBase(below) != base)
        {
            flowSideways(cells, x, y, z, here, nether);
        }
    }

    void Scheduler::schedule(int x, int y, int z, long long due)
    {
        const uint64_t key = keyOf(x, y, z);
        auto found = m_due.find(key);
        if (found == m_due.end()) m_due.emplace(key, due);
        else if (due < found->second) found->second = due;
    }

    void Scheduler::wake(const Cells& cells, int x, int y, int z, long long now, bool nether)
    {
        const int around[7][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 },
                                   { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
        for (const auto& a : around)
        {
            const BlockId id = cells.at(x + a[0], y + a[1], z + a[2]);
            if (!isFluid(id)) continue;
            schedule(x + a[0], y + a[1], z + a[2], now + tickDelay(id, nether));
        }
    }

    int Scheduler::run(Cells& cells, long long now, bool nether, int budget)
    {
        m_ready.clear();
        for (const auto& entry : m_due)
            if (entry.second <= now) m_ready.emplace_back(entry.second, entry.first);
        if (m_ready.empty()) return 0;

        std::sort(m_ready.begin(), m_ready.end());
        if (static_cast<int>(m_ready.size()) > budget) m_ready.resize(static_cast<size_t>(budget));

        for (const auto& ready : m_ready) m_due.erase(ready.second);
        for (const auto& ready : m_ready)
        {
            int x = 0, y = 0, z = 0;
            unkey(ready.second, x, y, z);
            update(cells, x, y, z, nether);
        }
        return static_cast<int>(m_ready.size());
    }
}
