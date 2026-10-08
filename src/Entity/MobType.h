#pragma once
#include "Audio/AudioEngine.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

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

// Where a mob is allowed to appear. Animals want daylight on grass;
// the rest want the dark.
enum class SpawnClass : uint8_t
{
    Passive,
    Hostile
};

// What a box is for, which decides both how it animates and what colour
// it is painted. Legs are named by corner because a quadruped's diagonal
// pairs swing together.
enum class Part : uint8_t
{
    Body,
    Head,
    LegFrontLeft,
    LegFrontRight,
    LegBackLeft,
    LegBackRight,
    Count
};

// One box of a mob, in texture pixels measured from its feet, sixteen to
// the block -- the same units the player model uses, so both go through
// the same unwrapping.
struct MobBox
{
    Part part = Part::Body;
    glm::ivec3 size{ 8, 8, 8 };
    glm::ivec3 origin{ 0, 0, 0 };   // the box's minimum corner; +z is forward
    int u = 0, v = 0;               // filled in when the hide is packed
};

// Everything that distinguishes one species from another.
struct MobType
{
    MobId id = MobId::Sheep;
    const char* name = "";

    SpawnClass spawnClass = SpawnClass::Passive;
    int maxHealth = 10;
    float width = 0.9f;         // collision box
    float height = 1.3f;
    float walkSpeed = 2.3f;     // blocks per second

    // How often an idle noise plays, in seconds, picked randomly in range.
    float ambientMinSeconds = 6.0f;
    float ambientMaxSeconds = 18.0f;

    Sound voice = Sound::MobGrunt;
    float voicePitch = 1.0f;    // shifts every sound this species makes

    // Painted, not loaded: a hide is three colours and a face, the same
    // arrangement PlayerSkin uses and for the same reason.
    uint32_t bodyColour = 0xFFFFFFFF;
    uint32_t headColour = 0xFFFFFFFF;
    uint32_t eyeColour = 0xFF202020;

    std::vector<MobBox> model;
};

const MobType& mobType(MobId id);
int mobTypeCount();

// The tallest point of the model, in blocks. The collision box and the
// drawing have to agree or a mob sinks into the floor, so --selftest
// checks this against height.
float modelTop(const MobType& type);
