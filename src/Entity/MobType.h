#pragma once
#include "Audio/AudioEngine.h"
#include "Game/Items.h"
#include "World/Dimension.h"
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
    Enderman,
    ZombiePigman,
    Villager,
    Wolf,
    Horse,
    Boat,
    Minecart,
    IronGolem,
    Blaze,
    WitherSkeleton,
    MagmaCube,
    Breeze,
    Guardian,
    Warden,
    Witch,
    EnderDragon,
    Wither,

    // A box with a shell on it, bolted to an End city and shooting at
    // whatever comes near. The only mob in the game that never moves.
    Shulker,

    // A zombie that drowned, and the one thing in the sea that fights
    // back outside a monument.
    Drowned,

    // What the sea is missing. It does nothing to anybody and is the
    // only black in the game.
    Squid,

    // The thing everyone remembers about the Nether: huge, slow, and
    // it shoots fire from across the room.
    Ghast,

    // What a swamp is missing. It eats a magma cube and leaves a
    // light behind, which is the only way to a froglight.
    Frog,

    // The illagers: villagers who left. They are what the outpost and
    // the mansion were built for, and both stood empty without them.
    Pillager,
    Vindicator,
    Evoker,
    Ravager,

    // The cave crowd. Underground was empty: the spawner only ever
    // looked down from the sky, so it never found a cave floor, and
    // every monster in the game was a monster you met outdoors.
    Slime,
    SlimeSmall,
    Bat,
    CaveSpider,
    Silverfish,

    // The Nether's own, which it has been short of: the zombified
    // ones were there, and nothing they used to be.
    Piglin,
    PiglinBrute,
    Hoglin,
    Strider,

    // The sea, which had a squid in it and nothing else. Six more:
    // two to eat, one that poisons you, one that plays, one that
    // lays eggs and one that is only there to be looked at.
    Cod,
    Salmon,
    Pufferfish,
    Dolphin,
    Turtle,
    Axolotl,

    // The country. Four animals in four biomes was the whole of the
    // overworld's wildlife: a sheep, a pig, a cow and a chicken,
    // wherever you went and whatever the ground was made of.
    Rabbit,
    Fox,
    Goat,
    Panda,
    Bee,
    Cat,
    Llama,
    PolarBear,

    // The night, which was the same five monsters everywhere. These
    // are the ones a particular place has: the snow, the desert and
    // the swamp each keep their own, and the sky has its own reason
    // to come after you.
    Stray,
    Husk,
    Bogged,
    Phantom,
    ZombieVillager,
    Endermite,
    Vex,

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

    // Bolted where it stands. A shulker is the only one: it does not
    // walk, it does not chase, and asking it to is the difference
    // between a turret and a monster. Named rather than inferred from
    // a zero walk speed, so a species that is simply missing one is
    // still caught.
    bool stationary = false;
    float limbSwing = 1.0f;     // how far its legs reach at a full run

    // Half a heart each. Zero for anything that does not fight back.
    // Which world it belongs in. A zombie in the Nether and a pigman in
    // a meadow are both wrong, and spawning only went by light level.
    Dimension dimension = Dimension::Overworld;

    // A mob that blows up instead of fighting. Zero means it does not.
    float fuseSeconds = 0.0f;
    float blastRadius = 0.0f;
    int blastDamage = 0;

    // A mob that shoots keeps its distance and looses arrows instead of
    // swinging. Zero means it fights with its hands.
    int arrowDamage = 0;
    float arrowRange = 16.0f;
    float arrowInterval = 2.0f;

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

    // What killing one is worth. Minecraft pays far more for the things
    // that fight back than for livestock, which is most of the reason
    // to go looking for a fight at all.
    int experience = 0;

    // Animals follow and breed for this; zero means nothing tempts it.
    StackId breedingFood = Blocks::Air;

    // How often an idle noise plays, in seconds, picked randomly in range.
    float ambientMinSeconds = 6.0f;
    float ambientMaxSeconds = 18.0f;

    // A neutral mob is hostile only once it has been provoked: it
    // carries a hostile's health, reach and damage, and none of them
    // matter until you hit it or stare at it. Minecraft's enderman is
    // the one that works this way, and without it the End is a room
    // full of things that kill you the moment you arrive.
    bool neutral = false;

    // Something you can get on the back of. The saddle height is where
    // the rider sits above its feet, which is what the camera is put
    // at: a rider's eyes are not the mob's eyes.
    bool rideable = false;
    float saddleHeight = 1.0f;

    // Not alive. A boat has no wits to wander with, makes no noise of
    // its own and is worth nothing to kill -- it is a mob only because
    // being a mob is what gets a thing drawn, moved and sat on here.
    bool inanimate = false;
    // Rides on top of water rather than sinking through it.
    bool floats = false;
    // Walks on lava the way a boat sits on water. The strider is the
    // only one, and it is the only way across a lava sea.
    bool lavaWalker = false;
    // Lives in the water rather than on top of it: water is not a wall
    // to be spawned outside of, which is what it is to everything else.
    // Without this a guardian could not be put in its own monument and
    // a drowned could not be put in the sea -- the spawn refused the
    // only square either of them belongs in, so neither was ever seen.
    bool aquatic = false;
    // Runs on rails, and only on rails: steered by the track rather
    // than by whoever is sitting in it.
    bool ridesRails = false;

    // The undead catch fire in open daylight. Spiders and creepers do
    // not, which is why they are the ones you still meet at noon.
    bool burnsInSunlight = false;

    // Born in the Nether. Lava, fire and a blaze's own charges are
    // nothing to it, and it is the reason a magma cube can sit in a
    // lava lake and wait.
    bool fireproof = false;

    // Hangs in the air this far above whatever is under it, instead of
    // walking. A blaze has no legs to stand on and vanilla draws it
    // floating inside its own box, so the model's feet are allowed off
    // the floor for anything with this set.
    float hoverHeight = 0.0f;

    // What it shoots. An arrow drops and sticks; a fire charge flies
    // flat and sets what it hits alight; a wind charge flies flat and
    // bursts, shoving whatever is near it.
    bool shootsFire = false;

    // Throws splash potions, which is how a witch fights: the damage
    // is not the point, what lands on you is.
    bool throwsPotions = false;
    bool shootsWind = false;

    // The whole figure, scaled. Mojang renders several mobs off a
    // shared model at a different size -- a wither skeleton is a
    // skeleton at 1.2 -- and this is both the drawing and the hitbox,
    // so width and height below are the model's own and get multiplied.
    float modelScale = 1.0f;

    // A boss: it has a bar across the top of the screen, it does not
    // despawn, and nothing else in the world is allowed to be one.
    bool boss = false;

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

    // A block texture out of the resource pack to clothe the model in,
    // rather than a flat colour. A boat is planks, so it wears the same
    // planks the world is built from and matches it exactly -- and at
    // the pack's own resolution, not sixteen pixels to the block.
    // The whole model turned about its own axis. A cart is laid out
    // along x the way Mojang draws it and turned a quarter here, the
    // same as its own renderer does.
    float modelYaw = 0.0f;

    const char* blockTexture = "";
    // What older packs call the same block, the way the atlas carries an
    // alias for every tile. Without it a pack from before the rename
    // would texture the world's planks and leave the boat flat.
    const char* blockTextureAlt = "";
    int sheetWidth = 64;
    int sheetHeight = 32;

    // Painted, not loaded: a hide is three colours and a face, the same
    // arrangement PlayerSkin uses and for the same reason.
    uint32_t bodyColour = 0xFFFFFFFF;
    uint32_t headColour = 0xFFFFFFFF;
    uint32_t eyeColour = 0xFF202020;

    std::vector<MobBox> model;
};

// What a monster cage sends out. A block here is an id and nothing
// else -- there is nowhere in it to write down which mob it keeps --
// so the dimension it stands in decides: a cage in a fortress makes
// blazes, one in the rock makes spiders.
MobId cageSpecies(Dimension where);

// What lives in a monument and what lives in an ancient city. Neither
// spawns the way everything else does -- they belong to one structure
// each and are put there by it.

// How many of its own kind a cage will have out at once, and how long
// it waits between them. A cage that never stopped would empty the
// mob cap into one corridor.
constexpr int CAGE_LIMIT = 4;
constexpr float CAGE_INTERVAL = 6.0f;
constexpr float CAGE_RANGE = 12.0f;

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
