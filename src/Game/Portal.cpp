#include "Portal.h"
#include <algorithm>
#include <array>

namespace
{
    // One flood fill, in a single plane. `alongX` says whether the
    // portal's face runs east-west or north-south; a portal stands
    // upright, so up and down are always one of the two directions.
    Portal::Found fillPlane(const Portal::Blocks& blocks, const glm::ivec3& at, bool alongX)
    {
        Portal::Found found;

        const glm::ivec3 across = alongX ? glm::ivec3(1, 0, 0) : glm::ivec3(0, 0, 1);
        const glm::ivec3 up(0, 1, 0);

        std::vector<glm::ivec3> open;
        open.push_back(at);

        std::vector<glm::ivec3> seen;
        seen.reserve(Portal::MAX_CELLS);

        auto alreadySeen = [&seen](const glm::ivec3& p) {
            return std::find(seen.begin(), seen.end(), p) != seen.end();
        };

        while (!open.empty())
        {
            const glm::ivec3 here = open.back();
            open.pop_back();

            if (alreadySeen(here)) continue;

            // Leaked out of the plane, or grown past anything sensible.
            if (!blocks.isFillable(here.x, here.y, here.z)) continue;
            if (seen.size() >= Portal::MAX_CELLS) return Portal::Found{};

            seen.push_back(here);

            const glm::ivec3 steps[4] = { across, -across, up, -up };
            for (const glm::ivec3& step : steps)
            {
                const glm::ivec3 next = here + step;

                // The frame is the wall; anything else means the hole is
                // open and the fire has nothing to catch on.
                if (blocks.isFrame(next.x, next.y, next.z)) continue;
                if (!blocks.isFillable(next.x, next.y, next.z)) return Portal::Found{};

                if (!alreadySeen(next)) open.push_back(next);
            }
        }

        if (seen.empty()) return Portal::Found{};

        // A portal is at least two across and three high, as in
        // Minecraft: a single hole in a wall is not a doorway.
        int minAcross = 1 << 30, maxAcross = -(1 << 30);
        int minUp = 1 << 30, maxUp = -(1 << 30);

        for (const glm::ivec3& p : seen)
        {
            const int a = alongX ? p.x : p.z;
            minAcross = std::min(minAcross, a);
            maxAcross = std::max(maxAcross, a);
            minUp = std::min(minUp, p.y);
            maxUp = std::max(maxUp, p.y);
        }

        if (maxAcross - minAcross + 1 < 2) return Portal::Found{};
        if (maxUp - minUp + 1 < 3) return Portal::Found{};

        found.valid = true;
        found.inside = std::move(seen);
        return found;
    }
}

Portal::Found Portal::findFrame(const Blocks& blocks, const glm::ivec3& at)
{
    if (!blocks.isFillable(at.x, at.y, at.z)) return Found{};

    // Try the two uprights a portal can stand in. The first that closes
    // is the one that was meant.
    for (bool alongX : { true, false })
    {
        Found found = fillPlane(blocks, at, alongX);
        if (found.valid) return found;
    }

    return Found{};
}
