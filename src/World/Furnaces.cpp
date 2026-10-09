#include "Furnaces.h"
#include <array>

Furnaces::State& Furnaces::at(const glm::ivec3& block)
{
    return m_furnaces[key(block)];
}

bool Furnaces::has(const glm::ivec3& block) const
{
    return m_furnaces.find(key(block)) != m_furnaces.end();
}

void Furnaces::remove(const glm::ivec3& block)
{
    m_furnaces.erase(key(block));
}

void Furnaces::put(const glm::ivec3& block, const State& state)
{
    m_furnaces[key(block)] = state;
}

std::vector<glm::ivec3> Furnaces::tick(float deltaTime, const glm::ivec3* except)
{
    std::vector<glm::ivec3> changed;
    const std::array<int, 3> skip = except ? key(*except) : std::array<int, 3>{ 0, 0, 0 };

    for (auto it = m_furnaces.begin(); it != m_furnaces.end();)
    {
        if (except && it->first == skip) { ++it; continue; }

        State& state = it->second;
        const bool wasLit = state.lit();

        Furnace furnace;
        furnace.input = &state.input;
        furnace.fuel = &state.fuel;
        furnace.output = &state.output;
        furnace.burnLeft = state.burnLeft;
        furnace.burnTotal = state.burnTotal;
        furnace.cooked = state.cooked;

        furnace.tick(deltaTime);

        state.burnLeft = furnace.burnLeft;
        state.burnTotal = furnace.burnTotal;
        state.cooked = furnace.cooked;

        const glm::ivec3 where(it->first[0], it->first[1], it->first[2]);
        if (state.lit() != wasLit) changed.push_back(where);

        // A cold, empty furnace is just a block again, and keeping a row
        // for every one ever placed would grow without bound.
        if (state.idle() && state.cooked <= 0.0f) it = m_furnaces.erase(it);
        else ++it;
    }

    return changed;
}
