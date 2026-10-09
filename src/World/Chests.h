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

    Contents& at(const glm::ivec3& block) { return m_chests[key(block)]; }
    bool has(const glm::ivec3& block) const { return m_chests.count(key(block)) > 0; }
    void remove(const glm::ivec3& block) { m_chests.erase(key(block)); }
    void clear() { m_chests.clear(); }

    // Drops the record if nothing is left in it. Called when a chest is
    // shut, so an emptied one does not sit in the save for ever.
    void forgetIfEmpty(const glm::ivec3& block);

    const std::map<std::array<int, 3>, Contents>& all() const { return m_chests; }
    void put(const glm::ivec3& block, const Contents& contents) { m_chests[key(block)] = contents; }

private:
    static std::array<int, 3> key(const glm::ivec3& block)
    {
        return { block.x, block.y, block.z };
    }

    std::map<std::array<int, 3>, Contents> m_chests;
};
