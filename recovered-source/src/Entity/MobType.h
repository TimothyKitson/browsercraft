#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

// One box of a mob's model, in block units relative to the mob's feet.
// Mobs are built from a handful of these, the way Minecraft models are, so a
// sheep is a body box plus a head box plus four legs rather than a mesh file.
struct ModelBox
{
    glm::vec3 center;  // offset from feet position
    glm::vec3 halfSize;
    int tile = 0;      // atlas tile index to texture it with
};

enum class MobId : uint8_t
{
    Sheep = 0,
    Pig,
    Cow,
    Chicken,
    Zombie,
    Skeleton,
    Creeper,
    Spider,
    Count
};

// Where a mob is allowed to appear. Passive animals want daylight on grass;
// hostiles want the dark.
enum class SpawnClass : uint8_t
{
    Passive,
    Hostile
};

// Everything that distinguishes one species from another, including the names
// its sounds are filed under.
//
// The sound fields are the paths the engine will ask for, minus the numbered
// variant and the extension, matching how the existing sounds are addressed:
// "mob/sheep/say" resolves to assets/sounds/mob/sheep/say1.ogg and friends.
// A mob with no sound for an event leaves that field empty.
struct MobType
{
    MobId id = MobId::Sheep;
    const char* name = "";

    SpawnClass spawnClass = SpawnClass::Passive;
    int maxHealth = 10;
    float width = 0.9f;        // collision box, matching Minecraft proportions
    float height = 1.3f;
    float walkSpeed = 2.3f;    // blocks per second

    // How often an idle sound plays, in seconds, picked randomly in range.
    float ambientMinSeconds = 6.0f;
    float ambientMaxSeconds = 18.0f;

    const char* soundAmbient = ""; // idle noise
    const char* soundHurt = "";
    const char* soundDeath = "";
    const char* soundStep = "";    // per footfall

    int ambientVariants = 0;       // how many numbered files exist
    int hurtVariants = 0;
    int deathVariants = 0;
    int stepVariants = 0;

    std::vector<ModelBox> model;
};

// The species table. Indexable by MobId.
const MobType& mobType(MobId id);

int mobTypeCount();
