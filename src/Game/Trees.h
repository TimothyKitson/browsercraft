#pragma once
#include "World/Block.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace Trees
{
    enum class Shape : uint8_t
    {
        Oak, FancyOak, Birch, SuperBirch, Spruce, Pine, MegaSpruce, MegaPine, Jungle, JungleNoVine, MegaJungle,
        JungleBush, Acacia, DarkOak, Cherry, Mangrove, TallMangrove, SwampOak, Azalea
    };

    struct Placed
    {
        glm::ivec3 at;
        BlockId id;
    };

    struct Plan
    {
        std::vector<Placed> logs;
        std::vector<Placed> leaves;
        std::vector<Placed> roots;
        std::vector<Placed> extras;
        std::vector<glm::ivec3> dirt;
        std::vector<glm::ivec3> podzol;
        BlockId dirtBlock = Blocks::Dirt;
        bool forceDirt = false;
        glm::ivec3 base{ 0 };
        int height = 0;
        bool ok = true;
    };

    struct Ground
    {
        virtual ~Ground() = default;
        virtual bool open(int x, int y, int z) const = 0;
        virtual bool muddy(int x, int y, int z) const = 0;
    };

    struct Cells
    {
        virtual ~Cells() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
        virtual void set(int x, int y, int z, BlockId id) = 0;
    };

    Plan plan(Shape shape, const glm::ivec3& origin, uint32_t seed, float bees, const Ground* ground = nullptr);
    bool giant(Shape shape);
    bool replaceable(BlockId id);
    bool fits(Shape shape, const Plan& plan, const Cells& cells);
    void write(const Plan& plan, Cells& cells);
}
