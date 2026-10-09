#pragma once
#include <glm/glm.hpp>
#include <vector>

// Finding the inside of a nether portal.
//
// A frame is a ring of obsidian with a hole in it, standing in a plane.
// Working out whether the hole is closed -- and which blocks are in it
// -- is a flood fill bounded by the frame, which is pure arithmetic over
// a question anyone can answer: what is at this block?
//
// So --selftest builds frames out of nothing, lights them, and checks
// the ones that should take and the ones that should not, with no world
// to quarry obsidian from.
namespace Portal
{
    // The biggest hole that still counts. Minecraft allows up to 21x21;
    // this is the same spirit and keeps the fill bounded.
    constexpr int MAX_SPAN = 21;
    constexpr int MAX_CELLS = MAX_SPAN * MAX_SPAN;

    // What the world looks like to the search.
    struct Blocks
    {
        virtual ~Blocks() = default;

        // Obsidian, or whatever else a frame may be built from.
        virtual bool isFrame(int x, int y, int z) const = 0;

        // Air, or an existing portal -- something the fire can fill.
        virtual bool isFillable(int x, int y, int z) const = 0;
    };

    struct Found
    {
        bool valid = false;
        std::vector<glm::ivec3> inside;
    };

    // Looks for a closed hole containing `at`, in the plane that runs
    // along x or along z. Returns the blocks to fill, or nothing if the
    // hole leaks, is too big, or has no frame round it.
    Found findFrame(const Blocks& blocks, const glm::ivec3& at);
}
