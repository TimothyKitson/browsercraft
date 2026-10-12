#pragma once
#include "Player/Inventory.h"
#include <glm/glm.hpp>
#include <array>
#include <map>

// What every chest in the world holds, by where it stands.
//
// Same arrangement as the furnaces: a block is a bare id with nowhere to
// keep contents, so they live beside the world and are saved with the
// level. A chest emptied of everything is forgotten, so the table holds
// only chests with something in them.
class Chests
{
public:
    static constexpr int SLOTS = 27;

    using Contents = std::array<ItemStack, SLOTS>;

    // The dimension is part of the key, not just the coordinates. It
    // has to be: a chest in the Nether at the same x, y and z as one in
    // the overworld is a different chest, and keyed on position alone
    // the two shared their contents -- so a chest built at matching
    // coordinates in the Nether opened holding the overworld's loot.
    void setDimension(int dimension) { m_dimension = dimension; }

    Contents& at(const glm::ivec3& block) { return m_chests[key(block)]; }
    bool has(const glm::ivec3& block) const { return m_chests.count(key(block)) > 0; }
    void remove(const glm::ivec3& block) { m_chests.erase(key(block)); }
    void clear() { m_chests.clear(); }

    // Drops the record if nothing is left in it. Called when a chest is
    // shut, so an emptied one does not sit in the save for ever.
    void forgetIfEmpty(const glm::ivec3& block);

    // For saving. Each key is dimension, x, y, z.
    const std::map<std::array<int, 4>, Contents>& all() const { return m_chests; }
    void put(const glm::ivec3& block, const Contents& contents) { m_chests[key(block)] = contents; }

    // Puts one back where it came from, which need not be the dimension
    // currently loaded: everything in the level file is read at once,
    // whichever world the player happens to be standing in.
    void putIn(int dimension, const glm::ivec3& block, const Contents& contents)
    {
        m_chests[{ dimension, block.x, block.y, block.z }] = contents;
    }

private:
    std::array<int, 4> key(const glm::ivec3& block) const
    {
        return { m_dimension, block.x, block.y, block.z };
    }

    int m_dimension = 0;
    std::map<std::array<int, 4>, Contents> m_chests;
};
