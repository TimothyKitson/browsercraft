#pragma once
#include "Player/Inventory.h"
#include <glm/glm.hpp>
#include <array>
#include <map>
#include <vector>

// What every item frame in the world is holding, by where it hangs.
//
// The same arrangement as the chests and the signs, and for the same
// reason: a block here is a bare id with nowhere to keep a stack. One
// slot rather than twenty-seven, because a frame holds one thing.
class Frames
{
public:
    void setDimension(int dimension) { m_dimension = dimension; }

    ItemStack& at(const glm::ivec3& block);
    bool has(const glm::ivec3& block) const;
    const ItemStack* find(const glm::ivec3& block) const;

    void remove(const glm::ivec3& block);
    void clear() { m_frames.clear(); }

    const std::map<std::array<int, 4>, ItemStack>& all() const { return m_frames; }
    void putIn(int dimension, const glm::ivec3& block, const ItemStack& held);

    // Every frame in this dimension within `reach`, for the pass that
    // draws what they hold. There are only ever a handful.
    struct Near
    {
        glm::ivec3 block;
        ItemStack held;
    };
    std::vector<Near> near(const glm::vec3& around, float reach) const;

private:
    std::array<int, 4> key(const glm::ivec3& block) const
    {
        return { m_dimension, block.x, block.y, block.z };
    }

    int m_dimension = 0;
    std::map<std::array<int, 4>, ItemStack> m_frames;
};
