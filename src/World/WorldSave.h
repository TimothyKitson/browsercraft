#pragma once
#include <array>
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
    float air = 15.0f;
    // Experience, and the fraction of a point the furnace still owes.
    // Appended at the very end of the file, so a level written before
    // experience existed reads back as a fresh start at level nought.
    int experience = 0;
    int experienceOwedHundredths = 0;

    // What was running on the player: one amplifier and one time left
    // per effect kind, in Effects::Kind order. Written at the very end,
    // after the container dimensions, so an older file simply runs out
    // and the player comes back with nothing on them.
    std::vector<int32_t> effectAmplifiers;
    std::vector<float> effectSeconds;

    // Which world each chest and furnace stands in, in the same order
    // as the lists above. Written at the very end of the file, so a
    // level saved before containers knew about dimensions reads back
    // with all of them in the overworld -- which is what they were.
    std::vector<uint8_t> chestDimensions;
    std::vector<uint8_t> furnaceDimensions;
    // Which of the three machines each furnace is, in that same order
    // and written after everything else for the same reason: a level
    // from before the smoker existed runs out here and reads back as
    // all furnaces, which is what it held.
    std::vector<uint8_t> furnaceKinds;

    // Every sign with something written on it: where it stands, which
    // world it stands in, and its four lines. Written after the
    // furnace kinds for the same reason they were written after the
    // dimensions -- a level from before signs existed runs out of
    // file here and reads back with none.
    struct SavedSign
    {
        int x = 0, y = 0, z = 0;
        uint8_t dimension = 0;
        std::array<std::string, 4> lines;
    };
    std::vector<SavedSign> signs;

    // Every item frame holding something: where it hangs, which world
    // it hangs in, and the one stack in it. Written after the signs,
    // for the same reason they were written after everything else.
    struct SavedFrame
    {
        int x = 0, y = 0, z = 0;
        uint8_t dimension = 0;
        uint16_t item = 0;
        uint16_t count = 0;
        uint16_t damage = 0;
        uint32_t enchantments = 0;
    };
    std::vector<SavedFrame> frames;

    struct SavedBeacon
    {
        int x = 0, y = 0, z = 0;
        uint8_t dimension = 0;
        int8_t primary = -1;
        int8_t secondary = -1;
    };
    std::vector<SavedBeacon> beacons;
    uint16_t itemBase = 4096;
    // One packed word per slot, in the same order as the inventory
    // above. Empty on a level written before enchanting existed, which
    // reads back as nothing enchanted.
    std::vector<uint32_t> enchantments;
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

    // Every chest with something in it. Twenty-seven slots each, written
    // after the furnaces for the same reason they were written last.
    struct SavedChest
    {
        int x = 0, y = 0, z = 0;
        std::vector<std::pair<uint16_t, uint16_t>> slots;
    };
    std::vector<SavedChest> chests;

    // Where dying puts you back. False on a level written before this was
    // saved at all, which means fall back to asking the generator.
    glm::vec3 spawn{ 0.0f };
    bool hasSpawn = false;

    bool hasAnchor = false;
    int anchorX = 0, anchorY = 0, anchorZ = 0;

    struct SavedPot
    {
        int x = 0, y = 0, z = 0;
        int8_t dimension = 0;
        uint32_t sides = 0;
    };
    std::vector<SavedPot> pots;
    int nextMapId = 1;
    std::vector<std::vector<std::pair<uint8_t, uint8_t>>> bannerDesigns;
    struct SavedBanner
    {
        int x = 0, y = 0, z = 0;
        int8_t dimension = 0;
        uint16_t design = 0;
    };
    std::vector<SavedBanner> banners;
    struct SavedCommand
    {
        int x = 0, y = 0, z = 0;
        int8_t dimension = 0;
        uint8_t flags = 0;
        std::string command;
        std::string output;
    };
    std::vector<SavedCommand> commands;
    struct SavedStructure
    {
        int x = 0, y = 0, z = 0;
        int8_t dimension = 0;
        std::string name;
        int offset[3] = { 0, 1, 0 };
        int size[3] = { 5, 5, 5 };
    };
    std::vector<SavedStructure> structures;
    std::vector<std::string> itemNames;
    std::vector<std::array<int, 4>> lodestones;
    bool hasDeathPoint = false;
    float deathPoint[3] = { 0.0f, 0.0f, 0.0f };
    int8_t deathDimension = 0;
    struct SavedBundleStack
    {
        uint16_t id = 0;
        uint8_t count = 0;
        uint16_t damage = 0;
        uint32_t enchantments = 0;
    };
    std::vector<std::vector<SavedBundleStack>> bundles;
    struct SavedBook
    {
        std::string title;
        std::string author;
        std::vector<std::string> pages;
    };
    std::vector<SavedBook> books;
    struct SavedLectern
    {
        int32_t x = 0, y = 0, z = 0;
        int8_t dimension = 0;
        SavedBundleStack stack;
    };
    std::vector<SavedLectern> lecterns;
    struct SavedVehicle
    {
        int8_t dimension = 0;
        uint16_t type = 0;
        int16_t variant = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f;
        float fuel = 0.0f;
        std::string command;
        std::vector<std::pair<uint8_t, SavedBundleStack>> contents;
    };
    std::vector<SavedVehicle> vehicles;
    struct SavedCage
    {
        int8_t dimension = 0;
        int32_t x = 0, y = 0, z = 0;
        uint16_t mob = 0;
    };
    std::vector<SavedCage> cages;
    struct SavedExplosion
    {
        uint8_t shape = 0;
        uint8_t flags = 0;
        std::vector<uint8_t> colours;
        std::vector<uint8_t> fades;
    };
    std::vector<SavedExplosion> fireworkStars;
    struct SavedRocket
    {
        uint8_t flight = 1;
        std::vector<uint16_t> explosions;
    };
    std::vector<SavedRocket> fireworkRockets;
    std::vector<uint16_t> crossbowRockets;
    std::vector<SavedLectern> jukeboxes;
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
