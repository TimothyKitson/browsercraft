#pragma once
#include "World/Block.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace Saplings
{
    struct Cells
    {
        virtual ~Cells() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
        virtual void set(int x, int y, int z, BlockId id) = 0;
    };

    bool canPlantOn(BlockId ground);
    bool needsSquare(int wood);
    bool findSquare(const Cells& cells, const glm::ivec3& at, glm::ivec3& corner);
    bool grow(Cells& cells, const glm::ivec3& at, uint32_t seed);
    bool hugeFungus(Cells& cells, const glm::ivec3& at, bool warped, uint32_t seed);
    BlockId leafDrop(BlockId leaves, uint32_t roll);
    bool hugeMushroom(Cells& cells, const glm::ivec3& at, bool red, uint32_t seed, bool force = false);
    bool azaleaTree(Cells& cells, const glm::ivec3& at, uint32_t seed, bool force = false);
}
