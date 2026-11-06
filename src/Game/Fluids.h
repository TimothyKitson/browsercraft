#pragma once
#include "World/Block.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Fluids
{
    struct Cells
    {
        virtual ~Cells() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
        virtual void set(int x, int y, int z, BlockId block) = 0;
    };

    int tickDelay(BlockId fluid, bool nether);
    int dropPerBlock(BlockId fluid, bool nether);
    int searchDepth(BlockId fluid, bool nether);
    bool washesAway(BlockId id);

    void update(Cells& cells, int x, int y, int z, bool nether);

    class Scheduler
    {
    public:
        void wake(const Cells& cells, int x, int y, int z, long long now, bool nether);
        void schedule(int x, int y, int z, long long due);
        int run(Cells& cells, long long now, bool nether, int budget);
        void clear() { m_due.clear(); }
        size_t pending() const { return m_due.size(); }

    private:
        std::unordered_map<uint64_t, long long> m_due;
        std::vector<std::pair<long long, uint64_t>> m_ready;
    };
}
