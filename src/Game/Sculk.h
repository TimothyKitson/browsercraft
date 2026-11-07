#pragma once
#include "World/Block.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace Sculk
{
    struct Cells
    {
        virtual ~Cells() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
        virtual void set(int x, int y, int z, BlockId block) = 0;
    };

    bool replaceable(BlockId id);
    int spread(Cells& cells, const glm::ivec3& centre, int charges, uint32_t seed);
}
