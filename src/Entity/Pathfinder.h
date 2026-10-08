#pragma once
#include <glm/glm.hpp>
#include <vector>

// Finding a way round things.
//
// The search knows nothing about chunks or blocks: it asks whatever it
// is given whether a creature could stand in a cell, and that is the
// whole of its contact with the world. The game hands it the world;
// --selftest hands it a map drawn in a string, which is what makes a
// pathfinder testable without a window or a terrain generator.
namespace Pathfinding
{
    struct Ground
    {
        virtual ~Ground() = default;
        // Could something stand with its feet in this cell: solid floor
        // beneath, and room enough above to be there.
        virtual bool standable(const glm::ivec3& feet) const = 0;
    };

    struct Limits
    {
        // The search stops when it has looked at this many cells. A mob
        // that cannot find a way within the budget wanders instead,
        // which is better than forty-eight of them each searching the
        // world every frame.
        int maxNodes = 240;
        int stepUp = 1;      // how far it will climb in one move
        int stepDown = 3;    // and how far it will drop
        int range = 24;      // give up beyond this far from the start
    };

    // The cells to walk through, the start excluded and the goal last.
    // Empty when there is no way inside the limits.
    std::vector<glm::ivec3> find(const Ground& ground, const glm::ivec3& start,
                                 const glm::ivec3& goal, const Limits& limits = Limits{});
}
