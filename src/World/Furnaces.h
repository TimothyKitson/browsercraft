#pragma once
#include "Game/Smelting.h"
#include "Player/Inventory.h"
#include <glm/glm.hpp>
#include <array>
#include <map>
#include <vector>

// Every furnace that has ever been lit or loaded, by where it stands.
//
// A block in this engine is a bare id with nowhere to put contents, so
// a furnace's three slots live here instead, keyed by position. They are
// saved with the level, and a furnace whose block is gone is forgotten
// along with whatever was inside it.
class Furnaces
{
public:
    struct State
    {
        ItemStack input;
        ItemStack fuel;
        ItemStack output;
        float burnLeft = 0.0f;
        float burnTotal = 0.0f;
        float cooked = 0.0f;

        bool lit() const { return burnLeft > 0.0f; }
        bool idle() const
        {
            return input.empty() && fuel.empty() && output.empty() && burnLeft <= 0.0f;
        }
    };

    // The dimension is part of the key, not just the coordinates. It
    // has to be: a chest in the Nether at the same x, y and z as one in
    // the overworld is a different chest, and keyed on position alone
    // the two shared their contents -- so a chest built at matching
    // coordinates in the Nether opened holding the overworld's loot.
    void setDimension(int dimension) { m_dimension = dimension; }

    // The furnace at `block`, made if it is not there yet.
    State& at(const glm::ivec3& block);

    // Whether one is on record there at all, without making one.
    bool has(const glm::ivec3& block) const;

    void remove(const glm::ivec3& block);
    void clear() { m_furnaces.clear(); }

    // Runs every furnace forward. Returns the ones whose lit state
    // changed, so the caller can swap the block between the two ids.
    // Furnaces that have gone cold and empty are dropped as it goes.
    // `except` is the one the player has open: its slots are the
    // inventory's while the screen is up, so it is run from there
    // instead and skipped here.
    std::vector<glm::ivec3> tick(float deltaTime, const glm::ivec3* except = nullptr);

    // For saving. The order is stable, which keeps the file diffable.
    const std::map<std::array<int, 4>, State>& all() const { return m_furnaces; }
    void put(const glm::ivec3& block, const State& state);
    void putIn(int dimension, const glm::ivec3& block, const State& state);

private:
    std::array<int, 4> key(const glm::ivec3& block) const
    {
        return { m_dimension, block.x, block.y, block.z };
    }

    int m_dimension = 0;
    std::map<std::array<int, 4>, State> m_furnaces;
};
