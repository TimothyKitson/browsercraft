#pragma once
#include "Audio/AudioEngine.h"
#include "Game/Items.h"
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
    WingLeft,
    WingRight,
    Count
};

// One box of a mob, in texture pixels measured from its feet, sixteen to
// the block -- the same units the player model uses, so both go through
// the same unwrapping.
//
// (u, v) is the box's offset on the sheet, and it is Mojang's: these are
// the numbers out of the vanilla models, so a mob wearing a real texture
// has every patch land where the artist drew it. Our axes are turned
// half a circle from theirs -- the model faces +z here and -z there --
// so a vanilla box at (x, y, z) sits at (-x, 24 - y, -z) below, which is
// the whole of the translation.
struct MobBox
{
    Part part = Part::Body;
    glm::ivec3 size{ 8, 8, 8 };
    glm::ivec3 origin{ 0, 0, 0 };   // the box's minimum corner; +z is forward
    glm::vec3 pivot{ 0.0f };        // what it swings about
    int u = 0, v = 0;

    // The old 64x32 layout draws one arm and one leg and mirrors them
    // for the other side. Every mob sheet is that layout, zombies
    // included -- theirs is a 64x64 file with the bottom half empty.
    bool mirror = false;

    // A rest pose, for the parts that do not hang straight down: a
    // spider's legs splay out and drop away from its body.
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;

    float inflate = 0.0f;           // wool sits just outside the hide it covers
    int layer = 0;                  // 1 draws from the overlay sheet
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
    float limbSwing = 1.0f;     // how far its legs reach at a full run

    // Half a heart each. Zero for anything that does not fight back.
    int attackDamage = 0;
    float attackReach = 1.1f;   // from edge to edge, on the flat
    float attackInterval = 1.0f;

    // What it leaves behind. Most of Minecraft's mob drops are items --
    // leather, bone, string, gunpowder -- and this engine has only
    // blocks so far, so most species drop nothing until it has items.
    StackId drop = Blocks::Air;
    int dropCount = 0;
    // A cow gives beef and leather, a sheep mutton and wool.
    StackId secondDrop = Blocks::Air;
    int secondDropCount = 0;

    // Animals follow and breed for this; zero means nothing tempts it.
    StackId breedingFood = Blocks::Air;

    // How often an idle noise plays, in seconds, picked randomly in range.
    float ambientMinSeconds = 6.0f;
    float ambientMaxSeconds = 18.0f;

    // The undead catch fire in open daylight. Spiders and creepers do
    // not, which is why they are the ones you still meet at noon.
    bool burnsInSunlight = false;

    Sound voice = Sound::MobPigSay;
    Sound hurtVoice = Sound::MobPigHurt;
    Sound deathVoice = Sound::MobPigDeath;
    float voicePitch = 1.0f;    // shifts every sound this species makes

    // assets/skins/mob/<texture>.png when it is there, and a hide painted
    // from the colours below when it is not. Mojang's mob textures are
    // Mojang's, so the repository holds the second of those and never the
    // first -- tools/install-mob-textures.ps1 puts the first in place on
    // your own machine.
    const char* texture = "";
    const char* overlay = "";       // a second sheet over the top: a sheep's wool
    int sheetWidth = 64;
    int sheetHeight = 32;

    // Painted, not loaded: a hide is three colours and a face, the same
    // arrangement PlayerSkin uses and for the same reason.
    uint32_t bodyColour = 0xFFFFFFFF;
    uint32_t headColour = 0xFFFFFFFF;
    uint32_t eyeColour = 0xFF202020;

    std::vector<MobBox> model;
};

const MobType& mobType(MobId id);
int mobTypeCount();

// The lowest and tallest points of the model, in blocks, with every box
// put through the rotation the renderer gives it -- a body lying on its
// side and a spider's legs angled into the ground are both a quarter
// turn away from the numbers in the table. The drawing and the collision
// box have to roughly agree or a mob floats or sinks, so --selftest
// checks these.
void modelBounds(const MobType& type, float& bottom, float& top);
float modelTop(const MobType& type);
