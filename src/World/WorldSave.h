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
    int hunger = 20;
    float saturation = 5.0f;
    bool creative = false;
    int selectedSlot = 0;
    uint8_t gameMode = 0;
    bool dead = false;   // a hardcore run that has ended
    bool cheats = false; // whether the commands work in this world
    // One entry per slot: what it holds and how many. The id is sixteen
    // bits because a slot can hold an item as well as a block, and the
    // items begin past everything a block id can say.
    std::vector<std::pair<uint16_t, uint16_t>> inventory;
    // How worn each slot's tool is, in the same order. Empty on a level
    // written before tools existed, which reads back as all new.
    std::vector<uint16_t> damage;

    // Every furnace with something in it: where it is, its three slots
    // and how far along it is. Appended last, so a level written before
    // furnaces existed simply has none.
    struct SavedFurnace
    {
        int x = 0, y = 0, z = 0;
        uint16_t input = 0, inputCount = 0;
        uint16_t fuel = 0, fuelCount = 0;
        uint16_t output = 0, outputCount = 0;
        float burnLeft = 0.0f, burnTotal = 0.0f, cooked = 0.0f;
    };
    std::vector<SavedFurnace> furnaces;

    // Where dying puts you back. False on a level written before this was
    // saved at all, which means fall back to asking the generator.
    glm::vec3 spawn{ 0.0f };
    bool hasSpawn = false;
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
