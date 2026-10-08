#include "Pathfinder.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>

namespace
{
    // Eight ways out of a cell. The diagonals are last so that when two
    // routes cost the same the straight one is found first, which keeps
    // mobs off the corners of walls.
    const int STEP_X[8] = {  1, -1,  0,  0,  1,  1, -1, -1 };
    const int STEP_Z[8] = {  0,  0,  1, -1,  1, -1,  1, -1 };

    // Cells are keyed by a single number so the open and closed sets can
    // be plain hash maps. The offsets keep negative coordinates positive.
    uint64_t keyOf(const glm::ivec3& cell)
    {
        const uint64_t x = static_cast<uint64_t>(cell.x + 0x200000) & 0x3FFFFF;
        const uint64_t y = static_cast<uint64_t>(cell.y + 0x200) & 0x7FF;
        const uint64_t z = static_cast<uint64_t>(cell.z + 0x200000) & 0x3FFFFF;
        return (x << 33) | (z << 11) | y;
    }

    struct Node
    {
        glm::ivec3 cell{ 0 };
        int cost = 0;        // steps taken to get here
        int estimate = 0;    // cost plus the guess at what is left
        uint64_t parent = 0;
        bool hasParent = false;
    };

    // Octile distance: the true length of a shortest path on a grid that
    // allows diagonals, which never overestimates and so never makes the
    // search take a longer way round.
    int heuristic(const glm::ivec3& from, const glm::ivec3& to)
    {
        const int dx = std::abs(from.x - to.x);
        const int dz = std::abs(from.z - to.z);
        const int dy = std::abs(from.y - to.y);
        const int straight = std::abs(dx - dz);
        const int diagonal = std::min(dx, dz);
        return (straight + diagonal) * 10 + dy * 4;
    }
}

std::vector<glm::ivec3> Pathfinding::find(const Ground& ground, const glm::ivec3& start,
                                          const glm::ivec3& goal, const Limits& limits)
{
    std::vector<glm::ivec3> path;
    if (start == goal) return path;

    std::unordered_map<uint64_t, Node> seen;
    std::vector<uint64_t> open;          // keys, kept as a heap by estimate
    seen.reserve(static_cast<size_t>(limits.maxNodes) * 2);

    auto worseThan = [&](uint64_t a, uint64_t b) {
        return seen[a].estimate > seen[b].estimate;   // a max-heap of the worst
    };

    Node first;
    first.cell = start;
    first.estimate = heuristic(start, goal);
    seen[keyOf(start)] = first;
    open.push_back(keyOf(start));

    uint64_t bestKey = keyOf(start);
    int bestEstimate = first.estimate;
    int examined = 0;

    while (!open.empty() && examined < limits.maxNodes)
    {
        std::pop_heap(open.begin(), open.end(), worseThan);
        const uint64_t key = open.back();
        open.pop_back();

        const Node here = seen[key];
        ++examined;

        if (here.cell == goal)
        {
            bestKey = key;
            bestEstimate = -1;   // an exact arrival beats every near miss
            break;
        }

        // Remember the closest thing seen, so a search that runs out of
        // budget still walks in the right direction instead of giving up.
        const int remaining = heuristic(here.cell, goal);
        if (remaining < bestEstimate)
        {
            bestEstimate = remaining;
            bestKey = key;
        }

        for (int i = 0; i < 8; ++i)
        {
            const int nx = here.cell.x + STEP_X[i];
            const int nz = here.cell.z + STEP_Z[i];

            if (std::abs(nx - start.x) > limits.range || std::abs(nz - start.z) > limits.range)
                continue;

            // A diagonal is only allowed when both of the straight moves
            // beside it are: otherwise mobs cut through the corner where
            // two walls meet.
            const bool diagonal = (STEP_X[i] != 0 && STEP_Z[i] != 0);

            // Find the floor near this column: level first, then up, then
            // down, which is the order that keeps paths flat.
            glm::ivec3 next(nx, here.cell.y, nz);
            bool found = ground.standable(next);

            for (int up = 1; !found && up <= limits.stepUp; ++up)
            {
                next.y = here.cell.y + up;
                found = ground.standable(next);
            }
            for (int down = 1; !found && down <= limits.stepDown; ++down)
            {
                next.y = here.cell.y - down;
                found = ground.standable(next);
            }
            if (!found) continue;

            if (diagonal)
            {
                const glm::ivec3 sideA(nx, next.y, here.cell.z);
                const glm::ivec3 sideB(here.cell.x, next.y, nz);
                if (!ground.standable(sideA) && !ground.standable(sideB)) continue;
            }

            // Diagonals cost what they really are; climbing costs extra
            // so a mob prefers to walk round a step than over it.
            const int move = (diagonal ? 14 : 10) + std::abs(next.y - here.cell.y) * 4;
            const int cost = here.cost + move;

            const uint64_t nextKey = keyOf(next);
            auto found_it = seen.find(nextKey);
            if (found_it != seen.end() && found_it->second.cost <= cost) continue;

            Node node;
            node.cell = next;
            node.cost = cost;
            node.estimate = cost + heuristic(next, goal);
            node.parent = key;
            node.hasParent = true;
            seen[nextKey] = node;

            open.push_back(nextKey);
            std::push_heap(open.begin(), open.end(), worseThan);
        }
    }

    // Walk the parents back from wherever the search got to.
    for (uint64_t key = bestKey;;)
    {
        const Node& node = seen[key];
        path.push_back(node.cell);
        if (!node.hasParent) break;
        key = node.parent;
    }

    std::reverse(path.begin(), path.end());
    if (!path.empty()) path.erase(path.begin());   // the start is where we are
    return path;
}
