#pragma once
#include "Chunk.h"
#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

// Everything that isn't terrain: where the player is, what they're
// carrying, what time it is.
struct LevelState
{
    uint32_t seed = 0;
    glm::vec3 playerPosition{ 0.0f };
    float yaw = -90.0f;
    float pitch = 0.0f;
    float timeOfDay = 0.25f;
    int health = 20;
    bool creative = false;
    int selectedSlot = 0;
    // 36 entries: 9 hotbar slots followed by 27 storage slots.
    std::vector<std::pair<uint8_t, uint16_t>> inventory;
};

namespace WorldSave
{
    void ensureDirectories(const std::string& saveDirectory);

    // Chunks are only written once the player has changed them -- untouched
    // terrain is cheaper to regenerate from the seed than to store.
    bool loadChunk(const std::string& saveDirectory, Chunk& chunk);
    void saveChunk(const std::string& saveDirectory, const Chunk& chunk);

    bool loadLevel(const std::string& saveDirectory, LevelState& out);
    void saveLevel(const std::string& saveDirectory, const LevelState& state);
}
