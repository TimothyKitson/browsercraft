#pragma once
#include "Redstone.h"
#include "World/Block.h"
#include <glm/glm.hpp>
#include <utility>
#include <vector>

namespace Piston
{
    constexpr int MAX_PUSH = 12;

    struct Moved
    {
        glm::ivec3 direction{ 0 };
        std::vector<glm::ivec3> cells;
        std::vector<std::pair<glm::ivec3, BlockId>> broken;
    };

    bool movable(BlockId id);
    bool replaceable(BlockId id);
    bool fragile(BlockId id);
    bool sticky(BlockId id);
    bool sticksTo(BlockId a, BlockId b);

    int lineLength(const Redstone::Cells& cells, int x, int y, int z, int facing);

    bool extend(Redstone::Cells& cells, int x, int y, int z, Moved* moved = nullptr);
    bool retract(Redstone::Cells& cells, int x, int y, int z, Moved* moved = nullptr);

    glm::ivec3 offset(int facing);
    void step(int facing, int& dx, int& dz);
}
