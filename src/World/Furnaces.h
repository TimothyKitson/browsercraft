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
    const std::map<std::array<int, 3>, State>& all() const { return m_furnaces; }
    void put(const glm::ivec3& block, const State& state);

private:
    static std::array<int, 3> key(const glm::ivec3& block)
    {
        return { block.x, block.y, block.z };
    }

    std::map<std::array<int, 3>, State> m_furnaces;
};
