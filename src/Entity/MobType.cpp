#include "MobType.h"
#include "Game/Explosion.h"
#include "Game/Items.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float HALF_PI = 1.5707964f;

    // The splay and droop of a spider's legs, straight out of the vanilla
    // model.
    constexpr float LEG_WIDE = 0.7853982f;
    constexpr float LEG_NARROW = 0.5811946f;
    constexpr float LEG_TURN = 0.3926991f;

    constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
    {
        return 0xFF000000u | (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(g) << 8) | r;
    }

    // A box copied out of a vanilla model, in Mojang's own numbers: the
    // texture offset and size of its addBox, the corner that box starts
    // at, and the offset its part is posed at.
    //
    // Mojang's axes run the other way from ours and their origin sits at
    // the top of a two-block figure, so (x, y, z) becomes (-x, 24 - y, -z)
    // and a box's minimum corner becomes its maximum. Doing that here, in
    // one place, is what lets the rest of the table be transcribed from
    // the vanilla models without a second thought.
    MobBox mc(Part part, int u, int v, int w, int h, int d,
              int bx, int by, int bz, int px, int py, int pz)
    {
        MobBox box;
        box.part = part;
        box.size = glm::ivec3(w, h, d);
        box.origin = glm::ivec3(-(bx + px + w), 24 - (by + py + h), -(bz + pz + d));
        box.pivot = glm::vec3(-px, 24 - py, -pz);
        box.u = u;
        box.v = v;
        return box;
    }

    MobBox mirrored(MobBox box)
    {
        box.mirror = true;
        return box;
    }

    MobBox turned(MobBox box, float pitch, float yaw = 0.0f, float roll = 0.0f)
    {
        box.pitch = pitch;
        box.yaw = yaw;
        box.roll = roll;
        return box;
    }

    MobBox over(MobBox box, float inflate)
    {
        box.inflate = inflate;
        box.layer = 1;
        return box;
    }

    // The four corners a quadruped stands on. Mojang gives each leg the
    // same box and moves it about, so this does too.
    void addLegs(std::vector<MobBox>& out, int u, int v, int w, int h, int d,
                 int bx, int by, int bz, int spread, int backZ, int frontZ)
    {
        out.push_back(mc(Part::LegBackRight,  u, v, w, h, d, bx, by, bz, -spread, 24 - h, backZ));
        out.push_back(mc(Part::LegBackLeft,   u, v, w, h, d, bx, by, bz,  spread, 24 - h, backZ));
        out.push_back(mc(Part::LegFrontRight, u, v, w, h, d, bx, by, bz, -spread, 24 - h, frontZ));
        out.push_back(mc(Part::LegFrontLeft,  u, v, w, h, d, bx, by, bz,  spread, 24 - h, frontZ));
    }

    // One of a spider's eight. The right side is drawn on the sheet and
    // the left mirrors it, and each pair sits at its own angle so the
    // legs fan out instead of sticking straight through each other.
    void addSpiderLeg(std::vector<MobBox>& out, Part part, int z, float droop, float turn)
    {
        out.push_back(turned(mc(part, 18, 0, 16, 2, 2, -15, -1, -1, -4, 15, z),
                             0.0f, turn, -droop));
        out.push_back(turned(mirrored(mc(part, 18, 0, 16, 2, 2, -1, -1, -1, 4, 15, z)),
                             0.0f, -turn, droop));
    }

    std::vector<MobType> buildTable()
    {
        std::vector<MobType> table(static_cast<size_t>(MobId::Count));

        {
            MobType& t = table[static_cast<size_t>(MobId::Sheep)];
            t.id = MobId::Sheep;
            t.voice = Sound::MobSheepSay;
            t.hurtVoice = Sound::MobSheepHurt;
            t.deathVoice = Sound::MobSheepDeath;
            t.breedingFood = Items::Wheat;
            t.name = "Sheep";
            t.texture = "sheep";
            t.overlay = "sheep_fur";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 8;
            t.width = 0.9f;
            t.height = 1.3f;
            t.walkSpeed = 1.8f;
            t.drop = Items::RawMutton;
            t.dropCount = 1;
            t.secondDrop = Blocks::Wool;
            t.secondDropCount = 1;
            t.bodyColour = rgb(228, 228, 222);
            t.headColour = rgb(222, 212, 198);
            t.model.push_back(mc(Part::Head, 0, 0, 6, 6, 8, -3, -4, -6, 0, 6, -8));
            t.model.push_back(turned(mc(Part::Body, 28, 8, 8, 16, 6, -4, -10, -6, 0, 5, 2), HALF_PI));
            addLegs(t.model, 0, 16, 4, 12, 4, -2, 0, -2, 3, 7, -5);

            // The wool, which is the whole of what a sheep looks like.
            // It is a second sheet over the first, sitting just outside
            // the hide, exactly as Mojang draws it.
            t.model.push_back(over(mc(Part::Head, 0, 0, 6, 6, 6, -3, -4, -4, 0, 6, -8), 0.6f));
            t.model.push_back(over(turned(mc(Part::Body, 28, 8, 8, 16, 6, -4, -10, -6, 0, 5, 2),
                                          HALF_PI), 1.75f));
            std::vector<MobBox> woolLegs;
            addLegs(woolLegs, 0, 16, 4, 6, 4, -2, 0, -2, 3, 7, -5);
            for (MobBox& leg : woolLegs)
            {
                // The wool stops at the knee, so its legs are posed from
                // the hip of the longer ones underneath.
                leg.origin.y = 6;
                leg.pivot.y = 12.0f;
                t.model.push_back(over(leg, 0.5f));
            }
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Pig)];
            t.id = MobId::Pig;
            t.voice = Sound::MobPigSay;
            t.hurtVoice = Sound::MobPigHurt;
            t.deathVoice = Sound::MobPigDeath;
            t.breedingFood = Items::Wheat;
            t.name = "Pig";
            t.texture = "pig";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.9f;
            t.height = 0.9f;
            t.walkSpeed = 2.0f;
            t.voicePitch = 1.25f;
            t.drop = Items::RawPorkchop;
            t.dropCount = 2;
            t.bodyColour = rgb(238, 145, 145);
            t.headColour = rgb(238, 145, 145);
            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -4, -8, 0, 12, -6));
            t.model.push_back(mc(Part::Head, 16, 16, 4, 3, 1, -2, 0, -9, 0, 12, -6));
            t.model.push_back(turned(mc(Part::Body, 28, 8, 10, 16, 8, -5, -10, -7, 0, 11, 2), HALF_PI));
            addLegs(t.model, 0, 16, 4, 6, 4, -2, 0, -2, 3, 7, -5);
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Cow)];
            t.id = MobId::Cow;
            t.voice = Sound::MobCowSay;
            t.hurtVoice = Sound::MobCowHurt;
            t.deathVoice = Sound::MobCowDeath;
            t.breedingFood = Items::Wheat;
            t.name = "Cow";
            t.texture = "cow";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.9f;
            t.height = 1.4f;
            t.walkSpeed = 1.8f;
            t.voicePitch = 0.7f;
            t.drop = Items::RawBeef;
            t.dropCount = 2;
            t.secondDrop = Items::Leather;
            t.secondDropCount = 1;
            t.bodyColour = rgb(74, 54, 42);
            t.headColour = rgb(60, 44, 36);
            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 6, -4, -4, -6, 0, 4, -8));
            t.model.push_back(mc(Part::Head, 22, 0, 1, 3, 1, -5, -5, -4, 0, 4, -8));
            t.model.push_back(mc(Part::Head, 22, 0, 1, 3, 1, 4, -5, -4, 0, 4, -8));
            t.model.push_back(turned(mc(Part::Body, 18, 4, 12, 18, 10, -6, -10, -7, 0, 5, 2), HALF_PI));
            t.model.push_back(turned(mc(Part::Body, 52, 0, 4, 6, 1, -2, 2, -8, 0, 5, 2), HALF_PI));
            addLegs(t.model, 0, 16, 4, 12, 4, -2, 0, -2, 3, 7, -5);
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Chicken)];
            t.id = MobId::Chicken;
            t.voice = Sound::MobChickenSay;
            t.hurtVoice = Sound::MobChickenHurt;
            t.deathVoice = Sound::MobChickenDeath;
            t.breedingFood = Items::Wheat;
            t.name = "Chicken";
            t.texture = "chicken";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 4;
            t.width = 0.4f;
            t.height = 0.7f;
            t.walkSpeed = 1.5f;
            t.drop = Items::RawChicken;
            t.dropCount = 1;
            t.secondDrop = Items::Feather;
            t.secondDropCount = 1;
            t.bodyColour = rgb(234, 234, 230);
            t.headColour = rgb(234, 234, 230);
            t.eyeColour = rgb(190, 60, 40);
            t.model.push_back(mc(Part::Head, 0, 0, 4, 6, 3, -2, -6, -2, 0, 15, -4));
            t.model.push_back(mc(Part::Head, 14, 0, 4, 2, 2, -2, -4, -4, 0, 15, -4));
            t.model.push_back(mc(Part::Head, 14, 4, 2, 2, 2, -1, -2, -3, 0, 15, -4));
            t.model.push_back(turned(mc(Part::Body, 0, 9, 6, 8, 6, -3, -4, -3, 0, 16, 0), HALF_PI));
            t.model.push_back(mc(Part::LegBackRight, 26, 0, 3, 5, 3, -1, 0, -3, -2, 19, 1));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 26, 0, 3, 5, 3, -1, 0, -3, 2, 19, 1)));
            t.model.push_back(mc(Part::WingRight, 24, 13, 1, 4, 6, 0, 0, -3, -4, 13, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 24, 13, 1, 4, 6, -1, 0, -3, 4, 13, 0)));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Zombie)];
            t.id = MobId::Zombie;
            t.voice = Sound::MobZombieSay;
            t.hurtVoice = Sound::MobZombieHurt;
            t.deathVoice = Sound::MobZombieDeath;
            t.name = "Zombie";
            t.texture = "zombie";
            // The file is twice as tall as it needs to be and the bottom
            // half is empty, which is how Mojang ships it.
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.9f;
            t.burnsInSunlight = true;
            t.attackDamage = 3;
            t.bodyColour = rgb(58, 92, 132);
            t.headColour = rgb(88, 132, 72);
            t.eyeColour = rgb(24, 32, 24);
            // A biped: the arms are filed as the front pair of legs, so
            // they swing with the opposite leg the way an animal's do.
            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 4, 12, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 4, 12, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Skeleton)];
            t.id = MobId::Skeleton;
            t.voice = Sound::MobSkeletonSay;
            t.hurtVoice = Sound::MobSkeletonHurt;
            t.deathVoice = Sound::MobSkeletonDeath;
            t.name = "Skeleton";
            t.texture = "skeleton";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.99f;
            t.walkSpeed = 2.1f;
            t.burnsInSunlight = true;
            // A skeleton shoots rather than swings, which is the whole
            // difference between meeting one and meeting a zombie.
            t.arrowDamage = 3;
            t.arrowRange = 16.0f;
            t.arrowInterval = 2.0f;
            t.attackDamage = 0;
            t.drop = Items::Bone;
            t.dropCount = 2;
            t.bodyColour = rgb(200, 200, 196);
            t.headColour = rgb(214, 214, 210);
            t.eyeColour = rgb(20, 20, 20);
            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 2, 12, 2, -1, -2, -1, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 2, 12, 2, -1, -2, -1, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 2, 12, 2, -1, 0, -1, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 2, 12, 2, -1, 0, -1, 2, 12, 0)));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Creeper)];
            t.id = MobId::Creeper;
            t.voice = Sound::MobCreeperSay;
            t.hurtVoice = Sound::MobCreeperHurt;
            t.deathVoice = Sound::MobCreeperDeath;
            t.name = "Creeper";
            t.texture = "creeper";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.7f;
            t.walkSpeed = 2.0f;
            // It blows up, which is the only thing a creeper does. It
            // lights its fuse when it gets close and goes off whether or
            // not you are still there.
            t.fuseSeconds = 1.5f;
            t.blastRadius = Explosion::CREEPER_RADIUS;
            t.blastDamage = Explosion::CREEPER_DAMAGE;
            t.attackDamage = 0;
            t.drop = Items::Gunpowder;
            t.dropCount = 1;
            t.bodyColour = rgb(78, 158, 62);
            t.headColour = rgb(88, 172, 70);
            t.eyeColour = rgb(18, 24, 18);
            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 6, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 6, 0));
            addLegs(t.model, 0, 16, 4, 6, 4, -2, 0, -2, 2, 4, -4);
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Spider)];
            t.id = MobId::Spider;
            t.voice = Sound::MobSpiderSay;
            t.hurtVoice = Sound::MobSpiderHurt;
            t.deathVoice = Sound::MobSpiderDeath;
            t.name = "Spider";
            t.texture = "spider";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 16;
            t.width = 1.4f;
            t.height = 0.9f;
            t.walkSpeed = 2.4f;
            // Eight legs that start out splayed cannot sweep as far as
            // four that hang straight down.
            t.limbSwing = 0.3f;
            t.voicePitch = 1.4f;
            t.attackDamage = 2;
            t.attackInterval = 0.8f;
            t.attackReach = 1.3f;
            t.drop = Items::StringItem;
            t.dropCount = 2;
            t.bodyColour = rgb(44, 38, 38);
            t.headColour = rgb(56, 48, 46);
            t.eyeColour = rgb(190, 40, 40);
            t.model.push_back(mc(Part::Head, 32, 4, 8, 8, 8, -4, -4, -8, 0, 15, -3));
            t.model.push_back(mc(Part::Body, 0, 0, 6, 6, 6, -3, -3, -3, 0, 15, 0));
            t.model.push_back(mc(Part::Body, 0, 12, 10, 8, 12, -5, -4, -6, 0, 15, 9));
            addSpiderLeg(t.model, Part::LegBackRight,   2, LEG_WIDE,   LEG_WIDE);
            addSpiderLeg(t.model, Part::LegBackLeft,    1, LEG_NARROW, LEG_TURN);
            addSpiderLeg(t.model, Part::LegFrontRight,  0, LEG_NARROW, -LEG_TURN);
            addSpiderLeg(t.model, Part::LegFrontLeft,  -1, LEG_WIDE,   -LEG_WIDE);
        }
        {
            // The Enderman: too tall to meet indoors, and it hits hard.
            // Minecraft's teleports and only turns on you when looked
            // at; this one is simply hostile, which is the half of it
            // that needs no new machinery.
            MobType& t = table[static_cast<size_t>(MobId::Enderman)];
            t.id = MobId::Enderman;
            t.voice = Sound::MobEndermanSay;
            t.hurtVoice = Sound::MobEndermanHurt;
            t.deathVoice = Sound::MobEndermanDeath;
            t.name = "Enderman";
            t.texture = "enderman";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 40;
            t.width = 0.6f;
            t.height = 2.9f;
            t.walkSpeed = 2.6f;
            t.attackDamage = 5;
            t.attackInterval = 1.1f;
            t.drop = Items::Gunpowder;
            t.dropCount = 1;
            t.bodyColour = rgb(18, 18, 22);
            t.headColour = rgb(24, 24, 28);
            t.eyeColour = rgb(214, 108, 240);
            // The same biped as a zombie, stretched: long limbs, small
            // body, which is what makes the silhouette unmistakable.
            // Stacked so the feet land on zero: legs from 0 to 30, body
            // 30 to 42, head 42 to 50. mc() measures down from 24, so
            // each box's offsets are whatever puts it at that height.
            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -26, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, -18, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 2, 30, 2, -5, -6, -1, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 2, 30, 2, 3, -6, -1, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 2, 30, 2, -2, -6, -1, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 2, 30, 2, 0, -6, -1, 0, 0, 0)));
        }
        {
            // The Nether's own. It looks like a zombie because it is
            // one, and it belongs down there rather than in a meadow.
            MobType& t = table[static_cast<size_t>(MobId::ZombiePigman)];
            t.id = MobId::ZombiePigman;
            t.voice = Sound::MobPigmanSay;
            t.hurtVoice = Sound::MobPigmanHurt;
            t.deathVoice = Sound::MobPigmanDeath;
            t.name = "Zombified Piglin";
            t.texture = "zombie_pigman";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.dimension = Dimension::Nether;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 2.0f;
            // Already dead and already burning; the sun is nothing to it.
            t.burnsInSunlight = false;
            t.attackDamage = 4;
            t.drop = Items::RawPorkchop;
            t.dropCount = 1;
            t.bodyColour = rgb(78, 148, 140);
            t.headColour = rgb(226, 150, 148);
            t.eyeColour = rgb(24, 32, 24);
            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 4, 12, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 4, 12, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        return table;
    }
}

const MobType& mobType(MobId id)
{
    static const std::vector<MobType> table = buildTable();
    const size_t index = static_cast<size_t>(id);
    return table[index < table.size() ? index : 0];
}

int mobTypeCount() { return static_cast<int>(MobId::Count); }

void modelBounds(const MobType& type, float& bottom, float& top)
{
    float low = 0.0f, high = 0.0f;
    bool any = false;

    for (const MobBox& b : type.model)
    {
        const glm::vec3 lo = glm::vec3(b.origin) - glm::vec3(b.inflate);
        const glm::vec3 hi = glm::vec3(b.origin + b.size) + glm::vec3(b.inflate);

        for (int corner = 0; corner < 8; ++corner)
        {
            glm::vec3 p((corner & 1) ? hi.x : lo.x,
                        (corner & 2) ? hi.y : lo.y,
                        (corner & 4) ? hi.z : lo.z);
            p -= b.pivot;

            const float cp = std::cos(b.pitch), sp = std::sin(b.pitch);
            p = glm::vec3(p.x, p.y * cp - p.z * sp, p.y * sp + p.z * cp);
            const float cy = std::cos(b.yaw), sy = std::sin(b.yaw);
            p = glm::vec3(p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy);
            const float cr = std::cos(b.roll), sr = std::sin(b.roll);
            p = glm::vec3(p.x * cr - p.y * sr, p.x * sr + p.y * cr, p.z);

            const float y = p.y + b.pivot.y;
            low = any ? std::min(low, y) : y;
            high = any ? std::max(high, y) : y;
            any = true;
        }
    }

    bottom = low / 16.0f;
    top = high / 16.0f;
}

float modelTop(const MobType& type)
{
    float bottom = 0.0f, top = 0.0f;
    modelBounds(type, bottom, top);
    return top;
}
