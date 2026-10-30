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
    // Mojang measures a model downwards from a plane the renderer puts
    // somewhere above the entity's feet, and for a mob that plane is at
    // twenty-four sixteenths. A cart is not a mob and is drawn from a
    // lower one, so the base is a parameter rather than a constant.
    constexpr int MOB_BASE = 24;
    constexpr int CART_BASE = 5;
    constexpr int BOAT_BASE = 6;

    MobBox mc(Part part, int u, int v, int w, int h, int d,
              int bx, int by, int bz, int px, int py, int pz, int base = MOB_BASE)
    {
        MobBox box;
        box.part = part;
        box.size = glm::ivec3(w, h, d);
        box.origin = glm::ivec3(-(bx + px + w), base - (by + py + h), -(bz + pz + d));
        box.pivot = glm::vec3(-px, base - py, -pz);
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
            t.experience = 2;
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
            t.experience = 2;
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
            t.experience = 2;
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
            t.experience = 2;
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
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(216, 136, 136);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(242, 240, 236);
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
            t.experience = 5;
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
            // A shell bolted to a wall. It does not walk, it does not
            // chase, and it shoots at whatever comes into the room --
            // which is what makes an End city a place you clear rather
            // than a place you walk through.
            MobType& t = table[static_cast<size_t>(MobId::Shulker)];
            t.id = MobId::Shulker;
            t.experience = 5;
            t.voice = Sound::MobShulkerSay;
            t.hurtVoice = Sound::MobShulkerHurt;
            t.deathVoice = Sound::MobShulkerDeath;
            t.name = "Shulker";
            t.texture = "shulker";
            t.sheetWidth = 64;
            t.sheetHeight = 64;
            t.dimension = Dimension::End;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 30;
            t.width = 0.9f;
            t.height = 0.9f;
            t.walkSpeed = 0.0f;
            t.stationary = true;
            t.limbSwing = 0.0f;

            // Its bullet. Slow, and it comes at you across a room
            // rather than down a corridor.
            t.shootsFire = true;
            t.arrowDamage = 4;
            t.arrowRange = 16.0f;
            t.arrowInterval = 2.4f;

            t.drop = Items::ShulkerShell;
            t.dropCount = 1;

            t.bodyColour = rgb(150, 110, 150);
            t.headColour = rgb(214, 190, 214);
            t.eyeColour = rgb(60, 40, 64);

            // A box with a lid on it. The lid is filed as the head so
            // it turns towards whatever the thing is looking at.
            t.width = 1.0f;
            t.height = 1.0f;
            t.model.push_back(mc(Part::Body, 0, 28, 16, 8, 16, -8, 16, -8, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 0, 16, 8, 16, -8, -8, -8, 0, 16, 0));
        }
        {
            // A zombie that went into the water and did not come out.
            // Everything about it is the zombie's but the colour and
            // what it is carrying.
            MobType& t = table[static_cast<size_t>(MobId::Drowned)];
            t.id = MobId::Drowned;
            t.experience = 5;
            t.voice = Sound::MobDrownedSay;
            t.hurtVoice = Sound::MobDrownedHurt;
            t.deathVoice = Sound::MobDrownedDeath;
            t.name = "Drowned";
            t.texture = "drowned";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.7f;
            t.aquatic = true;
            t.attackDamage = 3;

            // It does not burn: it is standing in water, and a mob
            // that caught fire in the sea would be the one thing in
            // the game nobody could take seriously.
            t.burnsInSunlight = false;

            t.bodyColour = rgb(44, 96, 104);
            t.headColour = rgb(62, 128, 118);
            t.eyeColour = rgb(140, 200, 190);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(44, 96, 86);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 4, 12, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 4, 12, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }
        {
            // The frog. It does nothing to anybody and is only in the
            // game for one reason: it eats a magma cube, and a
            // froglight is what is left.
            MobType& t = table[static_cast<size_t>(MobId::Frog)];
            t.id = MobId::Frog;
            t.experience = 1;
            t.voice = Sound::MobFrogSay;
            t.hurtVoice = Sound::MobFrogHurt;
            t.deathVoice = Sound::MobFrogDeath;
            t.name = "Frog";
            t.texture = "frog";
            t.sheetWidth = 64;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.5f;
            t.height = 0.5f;
            t.walkSpeed = 1.6f;
            t.limbSwing = 0.6f;

            // Magma cream, which is what a magma cube leaves. A frog
            // following the thing its dinner is made of is as close
            // as this gets to Minecraft leading one on a slimeball.
            t.breedingFood = Items::MagmaCream;

            t.bodyColour = rgb(140, 170, 92);
            t.headColour = rgb(164, 196, 108);
            t.eyeColour = rgb(226, 150, 60);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(96, 128, 60);

            // A body with a head on the front of it and four stubby
            // legs: eight pixels tall in all, which is half a block.
            t.model.push_back(mc(Part::Body, 0, 0, 8, 4, 8, -4, 18, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 14, 6, 3, 4, -3, 17, 2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 0, 24, 2, 2, 2, 2, 22, 2, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 24, 2, 2, 2, -4, 22, 2, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 24, 2, 2, 2, 2, 22, -4, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 24, 2, 2, 2, -4, 22, -4, 0, 0, 0)));
        }
        {
            // The squid. It swims about and does nothing at all, which
            // is the point: an ocean with nothing alive in it reads as
            // a texture rather than a place.
            MobType& t = table[static_cast<size_t>(MobId::Squid)];
            t.id = MobId::Squid;
            t.experience = 1;
            t.voice = Sound::MobSquidSay;
            t.hurtVoice = Sound::MobSquidHurt;
            t.deathVoice = Sound::MobSquidDeath;
            t.name = "Squid";
            t.texture = "squid";
            t.sheetWidth = 64;
            t.sheetHeight = 32;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.8f;
            t.height = 0.8f;
            t.walkSpeed = 1.2f;
            t.limbSwing = 0.4f;
            t.aquatic = true;

            // It swims rather than bobs: held off the sea bed the way
            // a guardian is, not held at the waterline the way a boat
            // is. A squid lying on the surface is a dead squid.
            t.hoverHeight = 1.6f;

            t.drop = Items::InkSac;
            t.dropCount = 1;

            t.bodyColour = rgb(44, 70, 92);
            t.headColour = rgb(58, 88, 112);
            t.eyeColour = rgb(22, 30, 40);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(30, 50, 68);

            // A bell with four arms under it, which is the whole of
            // what a squid looks like from any angle.
            t.model.push_back(mc(Part::Body, 0, 0, 8, 6, 8, -4, 11, -4, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 0, 16, 2, 7, 2, -1, 0, -1, 3, 17, 3));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 16, 2, 7, 2, -1, 0, -1, -3, 17, 3)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 2, 7, 2, -1, 0, -1, 3, 17, -3));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 2, 7, 2, -1, 0, -1, -3, 17, -3)));
        }
        {
            // The ghast. It hangs in the air, it is enormous, and it
            // shoots from further off than anything else in the game.
            // A Nether with nothing above the floor is a corridor.
            MobType& t = table[static_cast<size_t>(MobId::Ghast)];
            t.id = MobId::Ghast;
            t.experience = 5;
            t.voice = Sound::MobGhastSay;
            t.hurtVoice = Sound::MobGhastHurt;
            t.deathVoice = Sound::MobGhastDeath;
            t.name = "Ghast";
            t.texture = "ghast";
            t.sheetWidth = 64;
            t.sheetHeight = 64;
            t.dimension = Dimension::Nether;
            t.spawnClass = SpawnClass::Hostile;

            // Ten half-hearts and an enormous target: Minecraft's
            // ghast dies to almost anything that reaches it, and
            // reaching it is the whole of the fight.
            t.maxHealth = 10;
            t.width = 3.0f;
            t.height = 3.0f;
            t.walkSpeed = 1.4f;
            t.limbSwing = 0.3f;
            t.hoverHeight = 7.0f;
            t.fireproof = true;

            // Drawn twice the size the boxes say, like the dragon: a
            // ghast at box size is a balloon, and the whole of the
            // thing is that it fills the sky.
            t.modelScale = 2.0f;

            t.shootsFire = true;
            t.arrowDamage = 6;
            t.arrowRange = 28.0f;
            t.arrowInterval = 3.2f;

            t.drop = Items::GhastTear;
            t.dropCount = 1;
            t.secondDrop = Items::Gunpowder;
            t.secondDropCount = 1;

            t.bodyColour = rgb(228, 228, 228);
            t.headColour = rgb(240, 240, 240);
            t.eyeColour = rgb(40, 20, 30);

            // A box with tentacles under it. Nine in Minecraft; four
            // here, because the other five are behind the first four
            // from every angle anybody sees one from.
            t.model.push_back(mc(Part::Body, 0, 0, 16, 16, 16, -8, -8, -8, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 0, 34, 2, 16, 2, 4, 8, 4, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 34, 2, 16, 2, -6, 8, 4, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 34, 2, 16, 2, 4, 8, -6, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 34, 2, 16, 2, -6, 8, -6, 0, 0, 0)));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Skeleton)];
            t.id = MobId::Skeleton;
            t.experience = 5;
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
            t.experience = 5;
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
            t.experience = 5;
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
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(120, 30, 28);
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
            // at. The teleporting is still missing; the looking is not,
            // and it is the half that matters -- an End full of things
            // that attack on sight killed the player within seconds of
            // arriving.
            MobType& t = table[static_cast<size_t>(MobId::Enderman)];
            t.id = MobId::Enderman;
            t.experience = 5;
            t.voice = Sound::MobEndermanSay;
            t.hurtVoice = Sound::MobEndermanHurt;
            t.deathVoice = Sound::MobEndermanDeath;
            t.name = "Enderman";
            t.texture = "enderman";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.neutral = true;
            t.maxHealth = 40;
            t.width = 0.6f;
            t.height = 2.9f;
            t.walkSpeed = 2.6f;
            t.attackDamage = 5;
            t.attackInterval = 1.1f;
            // An ender pearl, which is what one is actually for.
            // Gunpowder was a stand-in from before there was a pearl,
            // and it is the creeper that drops gunpowder.
            t.drop = Items::EnderPearl;
            t.dropCount = 1;
            t.bodyColour = rgb(18, 18, 22);
            t.headColour = rgb(24, 24, 28);
            t.eyeColour = rgb(214, 108, 240);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(92, 48, 118);
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
            t.experience = 5;
            t.voice = Sound::MobPigmanSay;
            t.hurtVoice = Sound::MobPigmanHurt;
            t.deathVoice = Sound::MobPigmanDeath;
            t.name = "Zombified Piglin";
            t.texture = "zombie_pigman";
            t.sheetHeight = 64;
            // Fire is nothing to it: it has stood in lava its whole
            // existence.
            t.fireproof = true;
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

        {
            MobType& t = table[static_cast<size_t>(MobId::Villager)];
            t.id = MobId::Villager;
            // Killing one is worth nothing. Minecraft gives no
            // experience for a villager, and it should not: the whole
            // point of one is that it is worth more alive.
            t.experience = 0;
            t.voice = Sound::MobVillagerSay;
            t.hurtVoice = Sound::MobVillagerHurt;
            t.deathVoice = Sound::MobVillagerDeath;
            t.name = "Villager";
            t.texture = "villager";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.4f;
            t.bodyColour = rgb(98, 70, 54);
            t.headColour = rgb(188, 146, 118);
            t.eyeColour = rgb(40, 50, 70);

            // The head is two pixels taller than a human's, and the
            // nose is its own box hung off the front of it. The nose is
            // filed as part of the head so it turns with it rather than
            // staying put while the face looks away.
            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 8, -4, -10, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 24, 0, 2, 4, 2, -1, -5, -6, 0, 0, 0));

            // The robe, which is what makes the shape read as a
            // villager rather than as a tall zombie: a body six deep
            // instead of four, and skirted below the waist.
            t.model.push_back(mc(Part::Body, 16, 20, 8, 12, 6, -4, 0, -3, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 38, 8, 6, 6, -4, 12, -3, 0, 0, 0));

            // Arms filed as the front pair of legs, the way every other
            // biped here is, so they swing against the back pair.
            t.model.push_back(mc(Part::LegFrontRight, 44, 22, 4, 8, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 44, 22, 4, 8, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 22, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 22, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }
        {
            // The wolf: wild until it is given a bone, and then it
            // follows you about. Neutral rather than hostile, so a
            // pack met in a forest is a thing to walk past rather than
            // a thing to run from -- until one of them is hit.
            MobType& t = table[static_cast<size_t>(MobId::Wolf)];
            t.id = MobId::Wolf;
            t.experience = 1;
            t.voice = Sound::MobWolfSay;
            t.hurtVoice = Sound::MobWolfHurt;
            t.deathVoice = Sound::MobWolfDeath;
            t.name = "Wolf";
            t.texture = "wolf";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 8;
            t.width = 0.6f;
            t.height = 0.85f;
            t.walkSpeed = 3.6f;      // it keeps up with you, which is the point
            t.voicePitch = 1.0f;
            t.bodyColour = rgb(214, 210, 203);
            t.headColour = rgb(226, 223, 216);

            // Head, snout, body, tail, four legs. A wolf is a small
            // long box on short legs with a big square head.
            t.model.push_back(mc(Part::Head, 0, 0, 6, 6, 4, -3, -3, -4, 0, 13, -7));
            t.model.push_back(mc(Part::Head, 0, 10, 3, 3, 4, -1, 0, -7, 0, 13, -7));
            t.model.push_back(turned(mc(Part::Body, 18, 14, 6, 9, 6, -3, -5, -4, 0, 12, 1), HALF_PI));
            t.model.push_back(mc(Part::Body, 9, 18, 2, 8, 2, -1, 0, 0, 0, 12, 8));
            addLegs(t.model, 0, 18, 2, 8, 2, -1, 0, -1, 2, 7, -4);
        }
        {
            // The horse: tamed with wheat, and then something to get on
            // the back of. Taller than anything else that walks, which
            // is most of why riding one feels different from walking.
            MobType& t = table[static_cast<size_t>(MobId::Horse)];
            t.id = MobId::Horse;
            t.experience = 3;
            t.voice = Sound::MobHorseSay;
            t.hurtVoice = Sound::MobHorseHurt;
            t.deathVoice = Sound::MobHorseDeath;
            t.breedingFood = Items::Wheat;
            t.name = "Horse";
            t.texture = "horse";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 22;
            t.width = 1.3f;
            t.height = 1.6f;
            t.walkSpeed = 6.4f;      // faster than you can run, which is the point
            t.voicePitch = 0.9f;
            t.rideable = true;
            t.saddleHeight = 1.5f;
            t.bodyColour = rgb(141, 103, 69);
            t.headColour = rgb(152, 114, 78);

            // A long body on tall legs, with the head carried out in
            // front and up, which is what reads as a horse rather than
            // as a very large cow.
            t.sheetHeight = 64;   // it does not fit on half a sheet

            t.model.push_back(mc(Part::Head, 0, 0, 6, 9, 7, -3, -7, -7, 0, 2, -10));
            t.model.push_back(mc(Part::Head, 26, 0, 4, 4, 4, -2, -2, -10, 0, 2, -10));
            t.model.push_back(turned(mc(Part::Body, 20, 20, 10, 22, 10, -5, -11, -8, 0, 3, 2), HALF_PI));
            t.model.push_back(mc(Part::Body, 44, 0, 2, 12, 2, -1, -1, 0, 0, 3, 10));
            addLegs(t.model, 0, 36, 4, 14, 4, -2, 0, -2, 4, 9, -8);
        }
        {
            // The boat, on Mojang's own layout. Every patch below was
            // read off oak.png rather than recalled: a hull of 28x16x3
            // at (0,0), two ends of 18x6x2 at (0,19) and (0,27), two
            // sides of 28x6x2 at (0,35) and (0,43). It is a mob because
            // being a mob is what gets a thing drawn, carried along and
            // sat on here; it has no wits and no voice of its own.
            MobType& t = table[static_cast<size_t>(MobId::Boat)];
            t.id = MobId::Boat;
            t.experience = 0;
            t.voice = Sound::DigWood;
            t.hurtVoice = Sound::DigWood;
            t.deathVoice = Sound::DigWood;
            t.name = "Boat";
            t.texture = "boat";
            t.sheetWidth = 128;
            t.sheetHeight = 64;
            // Without a sheet it wears the pack's own planks instead of
            // a colour picked to look like them.
            t.blockTexture = "oak_planks";
            t.blockTextureAlt = "planks_oak";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 4;
            t.width = 1.4f;
            t.height = 0.55f;
            t.walkSpeed = 5.2f;
            t.rideable = true;
            t.saddleHeight = 0.35f;
            t.inanimate = true;
            t.floats = true;
            t.modelYaw = HALF_PI;
            t.drop = Items::Boat;
            t.dropCount = 1;
            t.bodyColour = rgb(142, 106, 64);
            t.headColour = rgb(142, 106, 64);

            t.model.push_back(turned(mc(Part::Body, 0, 0, 28, 16, 3,
                                        -14, -9, -3, 0, 3, 1, BOAT_BASE), HALF_PI));
            t.model.push_back(turned(mc(Part::Body, 0, 19, 18, 6, 2,
                                        -9, -7, -1, -14, 4, 0, BOAT_BASE),
                                     0.0f, HALF_PI * 3.0f));
            t.model.push_back(turned(mc(Part::Body, 0, 27, 18, 6, 2,
                                        -9, -7, -1, 14, 4, 0, BOAT_BASE),
                                     0.0f, HALF_PI));
            t.model.push_back(turned(mc(Part::Body, 0, 35, 28, 6, 2,
                                        -14, -7, -1, 0, 4, -8, BOAT_BASE),
                                     0.0f, HALF_PI * 2.0f));
            t.model.push_back(mc(Part::Body, 0, 43, 28, 6, 2,
                                 -14, -7, -1, 0, 4, 8, BOAT_BASE));
        }
        {
            // The minecart, on Mojang's own layout: a floor with four
            // walls, every wall off the same patch of the sheet. It is
            // drawn from a lower plane than a mob is, which is what the
            // cart base is for.
            MobType& t = table[static_cast<size_t>(MobId::Minecart)];
            t.id = MobId::Minecart;
            t.experience = 0;
            t.voice = Sound::DigStone;
            t.hurtVoice = Sound::DigStone;
            t.deathVoice = Sound::DigStone;
            t.name = "Minecart";
            t.texture = "minecart";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 4;
            t.width = 0.98f;
            t.height = 0.7f;
            t.walkSpeed = 8.0f;
            t.rideable = true;
            t.saddleHeight = 0.3f;
            t.inanimate = true;
            t.ridesRails = true;
            t.modelYaw = HALF_PI;
            t.drop = Items::Minecart;
            t.dropCount = 1;
            t.bodyColour = rgb(120, 124, 132);
            t.headColour = rgb(120, 124, 132);

            t.model.push_back(turned(mc(Part::Body, 0, 10, 20, 16, 2,
                                        -10, -8, -1, 0, 4, 0, CART_BASE), HALF_PI));
            t.model.push_back(turned(mc(Part::Body, 0, 0, 16, 8, 2,
                                        -8, -9, -1, -9, 4, 0, CART_BASE), 0.0f, HALF_PI * 3.0f));
            t.model.push_back(turned(mc(Part::Body, 0, 0, 16, 8, 2,
                                        -8, -9, -1, 9, 4, 0, CART_BASE), 0.0f, HALF_PI));
            t.model.push_back(turned(mc(Part::Body, 0, 0, 16, 8, 2,
                                        -8, -9, -1, 0, 4, -7, CART_BASE), 0.0f, HALF_PI * 2.0f));
            t.model.push_back(mc(Part::Body, 0, 0, 16, 8, 2,
                                 -8, -9, -1, 0, 4, 7, CART_BASE));
        }
        {
            // The iron golem, on Mojang's own layout, read off its own
            // sheet: a head of 8x10x6 at (0,0) with a nose at (24,0), a
            // body of 18x12x11 at (0,40) with a skirt of 9x5x6 at
            // (0,70), two arms of 4x30x6 and two legs of 6x16x5.
            //
            // It is the village's own answer to what comes out at
            // night: slow, enormously tough, and worth nothing to kill.
            MobType& t = table[static_cast<size_t>(MobId::IronGolem)];
            t.id = MobId::IronGolem;
            t.experience = 0;
            t.voice = Sound::MobGolemSay;
            t.hurtVoice = Sound::MobGolemHurt;
            t.deathVoice = Sound::MobGolemDeath;
            t.name = "Iron Golem";
            t.texture = "iron_golem";
            t.sheetWidth = 128;
            t.sheetHeight = 128;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 100;
            t.width = 1.4f;
            t.height = 2.7f;
            t.walkSpeed = 1.5f;
            t.voicePitch = 0.6f;
            t.limbSwing = 0.6f;

            // It hits like a falling anvil, but only once it has been
            // given a reason: a golem that started fights would be a
            // village nobody could walk into.
            t.neutral = true;
            t.attackDamage = 11;
            t.attackReach = 1.4f;

            t.drop = Items::IronIngot;
            t.dropCount = 4;
            t.secondDrop = Blocks::FlowerRed;
            t.secondDropCount = 1;

            t.bodyColour = rgb(198, 198, 190);
            t.headColour = rgb(206, 206, 198);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 6, -4, -12, -6, 0, -7, -2));
            t.model.push_back(mc(Part::Head, 24, 0, 2, 4, 2, -1, -5, -8, 0, -7, -2));

            t.model.push_back(mc(Part::Body, 0, 40, 18, 12, 11, -9, -2, -6, 0, -7, 0));
            t.model.push_back(mc(Part::Body, 0, 70, 9, 5, 6, -4, 10, -3, 0, -7, 0));

            t.model.push_back(mc(Part::LegFrontRight, 60, 21, 4, 30, 6, -13, -2, -3, 0, -7, 0));
            t.model.push_back(mc(Part::LegFrontLeft, 60, 58, 4, 30, 6, 9, -2, -3, 0, -7, 0));

            t.model.push_back(mc(Part::LegBackRight, 37, 0, 6, 16, 5, -3, -3, -3, -4, 11, 0));
            t.model.push_back(mc(Part::LegBackLeft, 60, 0, 6, 16, 5, -3, -3, -3, 5, 11, 0));
        }

        {
            // The blaze, on Mojang's layout: a head of 8x8x8 at (0,0)
            // and twelve rods of 2x8x2, all reading the one patch at
            // (0,16). The rods sit in three rings -- nine pixels out
            // near the head, seven below that, five at the bottom --
            // which is what gives it a wide top and a narrow waist.
            //
            // It hangs in the air rather than standing. Vanilla draws
            // the model floating inside its own box and that is why it
            // has no feet to put on the floor.
            MobType& t = table[static_cast<size_t>(MobId::Blaze)];
            t.id = MobId::Blaze;
            t.experience = 10;
            t.voice = Sound::MobBlazeSay;
            t.hurtVoice = Sound::MobBlazeHurt;
            t.deathVoice = Sound::MobBlazeDeath;
            t.name = "Blaze";
            t.texture = "blaze";
            t.dimension = Dimension::Nether;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.8f;
            t.walkSpeed = 1.6f;
            t.hoverHeight = 1.2f;
            t.fireproof = true;

            // Three charges at a time in Minecraft, which comes out as
            // one every few seconds on average; it is the range that
            // makes it dangerous, not the rate.
            t.shootsFire = true;
            t.arrowDamage = 5;
            t.arrowRange = 18.0f;
            t.arrowInterval = 3.0f;

            t.drop = Items::BlazeRod;
            t.dropCount = 1;

            t.bodyColour = rgb(246, 180, 44);
            t.headColour = rgb(255, 198, 54);
            t.eyeColour = 0xFFFFF4C0;
            t.pattern = MobType::Pattern::Patches;
            t.pattern = MobType::Pattern::Belly;

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -4, -4, 0, 0, 0));

            const int ringX[12] = { 9, 5, -4, -9,  5, -1, -7, -6,  4, 0, -4, -5 };
            const int ringZ[12] = { 0, 8,  8,  1,  5,  7,  2, -4,  2, 5,  3, -2 };
            const int ringY[12] = { -2, -2, -2, -2,  2, 2, 2, 2,  11, 11, 11, 11 };
            for (int i = 0; i < 12; ++i)
                t.model.push_back(mc(Part::Body, 0, 16, 2, 8, 2, 0, 0, 0,
                                     ringX[i], ringY[i], ringZ[i]));
        }
        {
            // The wither skeleton is the skeleton's own model at 1.2,
            // which is Mojang's: same boxes, same patches, a fifth
            // bigger. Scaling the figure rather than the boxes is what
            // keeps every texture patch the size the sheet says.
            MobType& t = table[static_cast<size_t>(MobId::WitherSkeleton)];
            t.id = MobId::WitherSkeleton;
            t.experience = 5;
            t.voice = Sound::MobWitherSkeletonSay;
            t.hurtVoice = Sound::MobWitherSkeletonHurt;
            t.deathVoice = Sound::MobWitherSkeletonDeath;
            t.name = "Wither Skeleton";
            t.texture = "wither_skeleton";
            t.dimension = Dimension::Nether;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.99f;
            t.modelScale = 1.2f;
            t.walkSpeed = 2.1f;
            t.voicePitch = 0.8f;
            t.fireproof = true;

            // It swings a sword instead of shooting, which is the whole
            // difference in a fight: there is no keeping away from it.
            t.attackDamage = 8;
            t.attackReach = 1.3f;

            t.drop = Items::Coal;
            t.dropCount = 1;

            // The skull, which is the only way to get one and the
            // only reason to hunt these rather than avoid them.
            // Minecraft drops it rarely; the drop here is the second
            // one, which every kill gives, so three kills is three
            // skulls rather than a grind.
            t.secondDrop = Blocks::WitherSkull;
            t.secondDropCount = 1;

            t.bodyColour = rgb(52, 56, 52);
            t.headColour = rgb(58, 62, 58);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 2, 12, 2, -1, -2, -1, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 2, 12, 2, -1, -2, -1, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 2, 12, 2, -1, 0, -1, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 2, 12, 2, -1, 0, -1, 2, 12, 0)));
        }
        {
            // The magma cube: one box, which is what a slime is. Mojang
            // draws it as eight stacked slices that come apart when it
            // jumps, and they all read the same strip of the sheet --
            // so a single cube over that strip comes out the same
            // creature standing still.
            MobType& t = table[static_cast<size_t>(MobId::MagmaCube)];
            t.id = MobId::MagmaCube;
            t.experience = 4;
            t.voice = Sound::MobMagmaCubeSay;
            t.hurtVoice = Sound::MobMagmaCubeHurt;
            t.deathVoice = Sound::MobMagmaCubeDeath;
            t.name = "Magma Cube";
            t.texture = "magmacube";
            t.dimension = Dimension::Nether;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 16;
            t.width = 0.52f;
            t.height = 0.52f;
            t.modelScale = 2.0f;
            t.walkSpeed = 2.6f;
            t.limbSwing = 0.0f;
            t.voicePitch = 0.9f;
            t.fireproof = true;

            t.attackDamage = 4;
            t.attackReach = 1.0f;

            t.drop = Items::MagmaCream;
            t.dropCount = 1;

            t.bodyColour = rgb(52, 24, 16);
            t.headColour = rgb(52, 24, 16);

            t.model.push_back(mc(Part::Body, 0, 0, 8, 8, 8, -4, 16, -4, 0, 0, 0));
        }

        {
            // The breeze, off its own sheet: a head of 8x8x8 at (0,0)
            // and two rods of 2x8x2 at (0,16), which is the whole of
            // what the file has room for -- it is a 32x32 sheet, the
            // smallest in the game.
            //
            // Vanilla draws a body of moving air between the head and
            // the legs from a second sheet. There is no second sheet
            // here, so there is a gap where the wind would be, which
            // is closer to right than filling it in with something
            // solid would be.
            MobType& t = table[static_cast<size_t>(MobId::Breeze)];
            t.id = MobId::Breeze;
            t.experience = 10;
            t.voice = Sound::MobBreezeSay;
            t.hurtVoice = Sound::MobBreezeHurt;
            t.deathVoice = Sound::MobBreezeDeath;
            t.name = "Breeze";
            t.texture = "breeze";
            t.sheetWidth = 32;
            t.sheetHeight = 32;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 30;
            t.width = 0.6f;
            t.height = 1.77f;
            t.walkSpeed = 3.2f;
            t.limbSwing = 0.4f;

            // It never comes to you: it keeps its distance and throws
            // wind charges, and the charges do almost no damage. What
            // they do is move you, which is how it kills you.
            t.shootsWind = true;
            t.arrowDamage = 1;
            t.arrowRange = 20.0f;
            t.arrowInterval = 2.4f;

            t.drop = Items::BreezeRod;
            t.dropCount = 1;
            t.secondDrop = Items::WindCharge;
            t.secondDropCount = 2;

            t.bodyColour = rgb(166, 206, 222);
            t.headColour = rgb(196, 228, 240);
            t.eyeColour = 0xFF20C0E0;

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -4, -4, 0, 0, 0));

            // Four rods in two pairs, one above the other and turned a
            // quarter between them, so the body is a column rather than
            // two legs with a hole where the wind should be. The first
            // one had the head floating a block clear of its own feet
            // and read as two separate things.
            t.model.push_back(mc(Part::LegFrontRight, 0, 16, 2, 8, 2, 0, 0, 0, 1, 16, 0));
            t.model.push_back(mc(Part::LegFrontLeft, 0, 16, 2, 8, 2, 0, 0, 0, -3, 16, 0));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 2, 8, 2, 0, 0, 0, -1, 8, 1));
            t.model.push_back(mc(Part::LegBackLeft, 0, 16, 2, 8, 2, 0, 0, 0, -1, 8, -3));
        }

        {
            // The thing in the monument. A cube with one eye and a
            // tail, which hovers rather than walks: it has no legs to
            // walk on, and a guardian sitting on the floor of the sea
            // reads as a dropped block.
            MobType& t = table[static_cast<size_t>(MobId::Guardian)];
            t.id = MobId::Guardian;
            t.experience = 10;
            t.voice = Sound::MobGuardianSay;
            t.hurtVoice = Sound::MobGuardianHurt;
            t.deathVoice = Sound::MobGuardianDeath;
            t.name = "Guardian";
            t.texture = "guardian";
            t.sheetWidth = 64;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 30;
            t.width = 0.85f;
            t.height = 0.85f;
            t.walkSpeed = 2.4f;
            t.aquatic = true;
            t.limbSwing = 0.0f;
            t.hoverHeight = 1.4f;

            // The laser. It is not a projectile in Minecraft either --
            // it is a beam that takes a few seconds to charge and then
            // hits whatever it was pointed at -- but the slow interval
            // and the long reach are what make it feel like one.
            t.shootsFire = true;
            t.arrowDamage = 6;
            t.arrowRange = 15.0f;
            t.arrowInterval = 3.0f;

            t.drop = Blocks::Prismarine;
            t.dropCount = 1;

            t.bodyColour = rgb(92, 148, 140);
            t.headColour = rgb(106, 170, 158);
            t.eyeColour = 0xFFE8D070;
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(60, 104, 100);

            // One body cube and the eye on the front of it, then the
            // tail behind. The eye is a Head so it tracks the player,
            // which is the whole of a guardian's menace.
            t.model.push_back(mc(Part::Body, 0, 0, 12, 12, 12, -6, -6, -6, 0, 15, 0));
            t.model.push_back(mc(Part::Head, 0, 24, 4, 4, 1, -2, -2, 0, 0, 15, -6));
            t.model.push_back(mc(Part::LegBackLeft, 40, 0, 2, 2, 6, -1, -1, 0, 0, 15, 6));
            t.model.push_back(mc(Part::LegBackRight, 40, 8, 2, 2, 5, -1, -1, 0, 0, 15, 12));

            // The spikes, which are what makes it dangerous to hit.
            t.model.push_back(mc(Part::WingLeft, 0, 16, 2, 8, 2, -1, 0, -1, -7, 15, 0));
            t.model.push_back(mc(Part::WingRight, 0, 16, 2, 8, 2, -1, 0, -1, 7, 15, 0));
        }

        {
            // The warden. Blind, enormous, and the only thing in the
            // game you are meant to run from rather than fight.
            MobType& t = table[static_cast<size_t>(MobId::Warden)];
            t.id = MobId::Warden;
            t.experience = 30;
            t.voice = Sound::MobWardenSay;
            t.hurtVoice = Sound::MobWardenHurt;
            t.deathVoice = Sound::MobWardenDeath;
            t.name = "Warden";
            t.texture = "warden";
            // Minecraft draws a warden on 64x64. These boxes are not
            // Minecraft's layout, and at 64 the arms ran off the edge
            // of the sheet -- which the model test caught.
            t.sheetWidth = 128;
            t.sheetHeight = 128;
            t.spawnClass = SpawnClass::Hostile;

            // Minecraft's has 500 health and hits for 30. Those are its
            // numbers, and they are the point of it: a warden is not a
            // fight you win, it is a fight you leave.
            t.maxHealth = 500;
            t.attackDamage = 30;
            t.width = 0.9f;
            t.height = 2.9f;
            t.walkSpeed = 2.0f;
            t.limbSwing = 0.5f;

            t.drop = Items::Emerald;
            t.dropCount = 1;

            t.bodyColour = rgb(14, 54, 58);
            t.headColour = rgb(18, 64, 68);
            t.eyeColour = 0xFF20D8D0;

            // Stacked legs, body, head, with the arms hanging from
            // the shoulder down past the hip. The first version had
            // the arms standing up over the head, because the box
            // offsets were read the wrong way round: origin.y here is
            // the bottom of a box in pixels off the floor, not its
            // top.
            t.model.push_back(mc(Part::LegBackRight, 64, 40, 6, 16, 6, -3, 0, -3, -4, 8, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 64, 40, 6, 16, 6, -3, 0, -3, 4, 8, 0)));
            t.model.push_back(mc(Part::Body, 0, 32, 14, 21, 10, -7, 0, -5, 0, -13, 0));
            t.model.push_back(mc(Part::WingRight, 64, 0, 8, 26, 8, -4, 0, -4, -11, -13, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 64, 0, 8, 26, 8, -4, 0, -4, 11, -13, 0)));
            t.model.push_back(mc(Part::Head, 0, 0, 16, 14, 10, -8, 0, -5, 0, -27, 0));
        }

        {
            // The witch. Villager-shaped, because that is what she is
            // under the hat, with the hat and the nose on top of it.
            MobType& t = table[static_cast<size_t>(MobId::Witch)];
            t.id = MobId::Witch;
            t.experience = 5;
            t.voice = Sound::MobWitchSay;
            t.hurtVoice = Sound::MobWitchHurt;
            t.deathVoice = Sound::MobWitchDeath;
            t.name = "Witch";
            t.texture = "witch";
            t.sheetWidth = 64;
            t.sheetHeight = 128;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 26;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 2.4f;
            t.limbSwing = 0.5f;

            // She throws rather than closes, which is the whole of how
            // a witch fights: splash potions from a distance. The
            // damage is small on purpose -- what lands on you is.
            t.throwsPotions = true;
            t.arrowDamage = 3;
            t.arrowRange = 12.0f;
            t.arrowInterval = 2.6f;

            t.drop = Items::GlassBottle;
            t.dropCount = 1;
            t.secondDrop = Items::Sugar;
            t.secondDropCount = 1;

            t.bodyColour = rgb(60, 48, 80);
            t.headColour = rgb(142, 118, 96);
            t.eyeColour = 0xFF203040;

            t.model.push_back(mc(Part::LegBackRight, 0, 22, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 22, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
            t.model.push_back(mc(Part::Body, 16, 20, 8, 12, 6, -4, 0, -3, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 44, 22, 4, 12, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 44, 22, 4, 12, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 8, -4, -10, -4, 0, 0, 0));
            // The nose, which is the one thing everybody draws a witch
            // by, and the brim and point of the hat over it.
            t.model.push_back(mc(Part::WingRight, 24, 0, 2, 4, 2, -1, -6, -6, 0, 0, 0));
            // One pixel lower than it looks like it wants to be: the
            // brim at -12 put the model's top at exactly the height
            // the model test allows, which is no margin at all.
            t.model.push_back(mc(Part::WingLeft, 0, 64, 10, 2, 10, -5, -11, -5, 0, 0, 0));
        }

        {
            // The dragon. The one boss in the game, and the reason the
            // End exists at all: the strongholds, the eyes and the
            // portal all lead here and used to arrive at an empty
            // island.
            MobType& t = table[static_cast<size_t>(MobId::EnderDragon)];
            t.id = MobId::EnderDragon;
            t.experience = 500;
            t.voice = Sound::MobDragonSay;
            t.hurtVoice = Sound::MobDragonHurt;
            t.deathVoice = Sound::MobDragonDeath;
            t.name = "Ender Dragon";
            t.texture = "enderdragon";
            t.sheetWidth = 128;
            t.sheetHeight = 128;
            t.spawnClass = SpawnClass::Hostile;
            t.boss = true;

            // Minecraft's numbers. The health is the whole shape of
            // the fight: you cannot out-damage the crystals healing
            // it, so the crystals are what you go for first.
            t.maxHealth = 200;
            t.attackDamage = 10;
            t.width = 4.0f;
            t.height = 2.6f;
            t.walkSpeed = 7.0f;
            t.limbSwing = 0.6f;

            // It never lands. Everything about fighting one is that it
            // is in the air and you are not.
            t.hoverHeight = 9.0f;
            t.fireproof = true;

            // Drawn three times the size the boxes say. The hitbox
            // stays what it is -- a dragon you could never hit would
            // not be a fight -- but a dragon drawn at box size is a
            // bird, and the whole of the thing is that it is enormous.
            t.modelScale = 3.0f;

            t.bodyColour = rgb(26, 20, 32);
            t.headColour = rgb(34, 26, 42);
            t.eyeColour = 0xFFD050F0;

            // Head and jaw out front, a long body, and wings that
            // reach well past it either side -- the silhouette is the
            // whole of how you recognise one coming.
            // Laid out along Z: head out front, body, then a tail
            // that is most of its length. The first version was a
            // compact blob with wings and read as a bat -- a dragon
            // is recognised by being long.
            //
            // bz is what puts a box in front of or behind the middle,
            // because origin.z = -(bz + pz + depth).
            t.model.push_back(mc(Part::Head, 0, 0, 10, 8, 16, -5, -4, -34, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 32, 16, 12, 28, -8, -6, -14, 0, 0, 0));

            // The tail rides on Body rather than on a leg. Put on
            // LegBack* it was animated as a limb and swung back and
            // forth across the body, so the dragon came apart at close
            // range: a wing adrift on one side and the tail out at an
            // angle on the other.
            t.model.push_back(mc(Part::Body, 64, 0, 8, 8, 20, -4, -4, 14, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 64, 32, 5, 5, 20, -2, -2, 34, 0, 0, 0));

            // Wings reaching well past the body either side, which is
            // the whole of the silhouette. Pivoted at the shoulder
            // rather than at the middle of the mob: swung about the
            // centre, a wing this long travels half the island.
            t.model.push_back(mc(Part::WingLeft, 0, 80, 26, 3, 18, 0, -1, -12, 8, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingRight, 0, 80, 26, 3, 18,
                                          -26, -1, -12, -8, 0, 0)));
        }

        {
            // The wither: the boss you build rather than the one you
            // find. Three heads, which is the whole of its look, and
            // a rattle instead of a roar.
            MobType& t = table[static_cast<size_t>(MobId::Wither)];
            t.id = MobId::Wither;
            t.experience = 500;
            t.voice = Sound::MobWitherSay;
            t.hurtVoice = Sound::MobWitherHurt;
            t.deathVoice = Sound::MobWitherDeath;
            t.name = "Wither";
            t.texture = "wither";
            // Minecraft fits one on 64 square with its own layout.
            // These ribs are eighteen wide, which needs 66 across, so
            // the sheet is the wider of the two allowed shapes.
            t.sheetWidth = 128;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.boss = true;

            // Minecraft's numbers. Less health than the dragon and
            // more dangerous, because nothing heals it and it shoots
            // from wherever it likes.
            t.maxHealth = 300;

            // It throws and does not swing. Every mob in this game is
            // one or the other -- a test has said so since before any
            // of this -- and a wither that closed to melee would not
            // be a wither anyway: the whole of fighting one is that it
            // stays out of reach.
            t.attackDamage = 0;
            t.width = 0.9f;
            t.height = 3.5f;
            t.walkSpeed = 4.0f;
            t.limbSwing = 0.0f;
            t.hoverHeight = 4.0f;
            t.fireproof = true;
            t.modelScale = 1.4f;

            // It never closes: it throws skulls. Fast and often, which
            // is what makes a wither a fight you take cover from.
            t.arrowDamage = 5;
            t.arrowRange = 24.0f;
            t.arrowInterval = 1.1f;

            t.drop = Items::NetherStar;
            t.dropCount = 1;

            t.bodyColour = rgb(38, 38, 40);
            t.headColour = rgb(52, 52, 54);
            t.eyeColour = 0xFF404040;

            // A ribcage hanging under three heads. The side heads are
            // on the wing parts because those are the ones that do not
            // swing like a limb -- the dragon's tail taught that.
            t.model.push_back(mc(Part::Head, 0, 0, 10, 10, 10, -5, -10, -5, 0, -2, 0));
            t.model.push_back(mc(Part::WingLeft, 0, 20, 6, 6, 6, -3, -8, -3, -8, -2, 0));
            t.model.push_back(mirrored(mc(Part::WingRight, 0, 20, 6, 6, 6, -3, -8, -3, 8, -2, 0)));

            // The spine, and four ribs tapering down it.
            t.model.push_back(mc(Part::Body, 0, 36, 4, 14, 4, -2, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 24, 36, 18, 3, 3, -9, 1, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 24, 44, 14, 3, 3, -7, 5, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 24, 52, 10, 3, 3, -5, 9, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 70, 36, 6, 3, 3, -3, 13, -2, 0, 0, 0));
        }

        {
            // The pillager: the one that shoots. A crossbow keeps it
            // across the yard from you, which is what makes an outpost
            // a place you approach rather than walk into.
            MobType& t = table[static_cast<size_t>(MobId::Pillager)];
            t.id = MobId::Pillager;
            t.experience = 5;
            t.voice = Sound::MobPillagerSay;
            t.hurtVoice = Sound::MobPillagerHurt;
            t.deathVoice = Sound::MobPillagerDeath;
            t.name = "Pillager";
            t.texture = "pillager";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 24;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 2.2f;
            t.limbSwing = 0.5f;

            // A crossbow hits harder than a bow and reloads slower,
            // which is the whole difference between the two.
            t.arrowDamage = 5;
            t.arrowRange = 16.0f;
            t.arrowInterval = 2.8f;

            t.drop = Items::Arrow;
            t.dropCount = 2;
            t.secondDrop = Items::Emerald;
            t.secondDropCount = 1;

            t.bodyColour = rgb(72, 76, 80);
            t.headColour = rgb(150, 156, 152);
            t.eyeColour = rgb(60, 70, 86);

            // The villager's own frame, which is what Mojang draws an
            // illager on too: the same robe and the same long head,
            // and only the colours say which one you are looking at.
            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 8, -4, -10, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 24, 0, 2, 4, 2, -1, -5, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 20, 8, 12, 6, -4, 0, -3, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 38, 8, 6, 6, -4, 12, -3, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 44, 22, 4, 8, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 44, 22, 4, 8, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 22, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 22, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The vindicator: the one that closes. It carries an axe
            // and it is the reason a mansion corridor is frightening
            // rather than merely dark.
            MobType& t = table[static_cast<size_t>(MobId::Vindicator)];
            t.id = MobId::Vindicator;
            t.experience = 5;
            t.voice = Sound::MobVindicatorSay;
            t.hurtVoice = Sound::MobVindicatorHurt;
            t.deathVoice = Sound::MobVindicatorDeath;
            t.name = "Vindicator";
            t.texture = "vindicator";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 24;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 2.6f;
            t.limbSwing = 0.6f;

            // The hardest hand in the game short of a ravager: an axe
            // in Minecraft's hands does thirteen, and that is a death
            // in two swings to anybody in iron.
            t.attackDamage = 7;
            t.attackInterval = 1.1f;

            t.drop = Items::Emerald;
            t.dropCount = 1;
            t.secondDrop = Items::IronAxe;
            t.secondDropCount = 1;

            t.bodyColour = rgb(60, 80, 78);
            t.headColour = rgb(150, 156, 152);
            t.eyeColour = rgb(60, 70, 86);

            // The villager's own frame, which is what Mojang draws an
            // illager on too: the same robe and the same long head,
            // and only the colours say which one you are looking at.
            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 8, -4, -10, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 24, 0, 2, 4, 2, -1, -5, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 20, 8, 12, 6, -4, 0, -3, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 38, 8, 6, 6, -4, 12, -3, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 44, 22, 4, 8, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 44, 22, 4, 8, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 22, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 22, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The evoker: what a mansion is actually for. It is the
            // only thing in the game that leaves a totem, and a totem
            // is the only second chance there is.
            MobType& t = table[static_cast<size_t>(MobId::Evoker)];
            t.id = MobId::Evoker;
            t.experience = 10;
            t.voice = Sound::MobEvokerSay;
            t.hurtVoice = Sound::MobEvokerHurt;
            t.deathVoice = Sound::MobEvokerDeath;
            t.name = "Evoker";
            t.texture = "evoker";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 24;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.8f;
            t.limbSwing = 0.4f;

            // It keeps its distance and throws its magic rather than
            // closing: fangs out of the floor are a thing this engine
            // has no room for, and a thrown spell carries the same
            // idea -- stay back, and something lands on you anyway.
            t.throwsPotions = true;
            t.arrowDamage = 4;
            t.arrowRange = 14.0f;
            t.arrowInterval = 3.0f;

            t.drop = Items::TotemOfUndying;
            t.dropCount = 1;
            t.secondDrop = Items::Emerald;
            t.secondDropCount = 2;

            t.bodyColour = rgb(226, 226, 220);
            t.headColour = rgb(150, 156, 152);
            t.eyeColour = rgb(60, 70, 86);

            // The villager's own frame, which is what Mojang draws an
            // illager on too: the same robe and the same long head,
            // and only the colours say which one you are looking at.
            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 8, -4, -10, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 24, 0, 2, 4, 2, -1, -5, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 20, 8, 12, 6, -4, 0, -3, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 38, 8, 6, 6, -4, 12, -3, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 44, 22, 4, 8, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 44, 22, 4, 8, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 22, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 22, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The ravager: the thing the illagers bring with them. It
            // is slow, it is enormous, and nothing else in the
            // Overworld hits as hard.
            MobType& t = table[static_cast<size_t>(MobId::Ravager)];
            t.id = MobId::Ravager;
            t.experience = 20;
            t.voice = Sound::MobRavagerSay;
            t.hurtVoice = Sound::MobRavagerHurt;
            t.deathVoice = Sound::MobRavagerDeath;
            t.name = "Ravager";
            t.texture = "ravager";
            t.sheetWidth = 128;
            t.sheetHeight = 128;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 100;
            t.width = 1.95f;
            t.height = 2.2f;
            t.walkSpeed = 2.0f;
            t.limbSwing = 0.5f;

            t.attackDamage = 12;
            t.attackReach = 1.6f;
            t.attackInterval = 1.6f;

            // Minecraft gives up a saddle, which this engine has no
            // use for: nothing here needs one to be ridden. Hide and
            // the emeralds it was carrying instead.
            t.drop = Items::Leather;
            t.dropCount = 3;
            t.secondDrop = Items::Emerald;
            t.secondDropCount = 2;

            t.bodyColour = rgb(76, 72, 66);
            t.headColour = rgb(92, 86, 78);
            t.eyeColour = rgb(200, 60, 40);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(54, 50, 46);

            // A barrel on four legs with a head slung off the front of
            // it, horns included. Thirty-five pixels tall at the back,
            // which is the two and a bit blocks it collides at.
            t.model.push_back(mc(Part::Body, 0, 55, 20, 16, 32, -10, -9, -16, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 0, 16, 20, 16, -8, -13, -22, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 68, 0, 8, 8, 4, -4, -5, -26, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 104, 0, 2, 5, 2, -7, -15, -18, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::Head, 104, 0, 2, 5, 2, 5, -15, -18, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 96, 40, 8, 17, 8, -4, 0, -4, -6, 7, -11));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 96, 40, 8, 17, 8, -4, 0, -4, 6, 7, -11)));
            t.model.push_back(mc(Part::LegBackRight, 96, 40, 8, 17, 8, -4, 0, -4, -6, 7, 11));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 96, 40, 8, 17, 8, -4, 0, -4, 6, 7, 11)));
        }

        {
            // The slime. Minecraft's is a cube with a face in it, and
            // so is this: there is nothing else to a slime.
            MobType& t = table[static_cast<size_t>(MobId::Slime)];
            t.id = MobId::Slime;
            t.experience = 4;
            t.voice = Sound::MobSlimeSay;
            t.hurtVoice = Sound::MobSlimeHurt;
            t.deathVoice = Sound::MobSlimeDeath;
            t.name = "Slime";
            t.texture = "slime";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 16;
            t.width = 1.04f;
            t.height = 1.04f;
            t.walkSpeed = 1.4f;
            t.limbSwing = 0.0f;
            t.attackDamage = 4;
            t.attackInterval = 1.0f;

            // Nothing: a big slime splits into small ones, and it is
            // the small ones that leave the slimeballs. Killing the
            // big one and getting nothing is the point of the split.
            t.bodyColour = rgb(104, 182, 90);
            t.headColour = rgb(124, 198, 108);
            t.eyeColour = rgb(40, 60, 36);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(74, 150, 64);

            t.model.push_back(mc(Part::Body, 0, 0, 16, 16, 16, -8, 8, -8, 0, 0, 0));
        }

        {
            // What a slime splits into, and where the slimeballs come
            // from. Half the size and a quarter of the trouble.
            MobType& t = table[static_cast<size_t>(MobId::SlimeSmall)];
            t.id = MobId::SlimeSmall;
            t.experience = 1;
            t.voice = Sound::MobSlimeSmallSay;
            t.hurtVoice = Sound::MobSlimeSmallHurt;
            t.deathVoice = Sound::MobSlimeSmallDeath;
            t.name = "Small Slime";
            t.texture = "slime";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 4;
            t.width = 0.52f;
            t.height = 0.52f;
            t.walkSpeed = 1.8f;
            t.limbSwing = 0.0f;
            t.voicePitch = 1.5f;
            t.attackDamage = 2;
            t.attackInterval = 0.9f;

            t.drop = Items::Slimeball;
            t.dropCount = 1;

            t.bodyColour = rgb(104, 182, 90);
            t.headColour = rgb(124, 198, 108);
            t.eyeColour = rgb(40, 60, 36);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(74, 150, 64);

            t.model.push_back(mc(Part::Body, 0, 0, 8, 8, 8, -4, 16, -4, 0, 0, 0));
        }

        {
            // The bat. It does nothing, it is worth nothing, and a cave
            // without one is a cave nothing lives in.
            MobType& t = table[static_cast<size_t>(MobId::Bat)];
            t.id = MobId::Bat;
            t.experience = 0;
            t.voice = Sound::MobBatSay;
            t.hurtVoice = Sound::MobBatHurt;
            t.deathVoice = Sound::MobBatDeath;
            t.name = "Bat";
            t.texture = "bat";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 6;
            t.width = 0.5f;
            t.height = 0.9f;
            t.walkSpeed = 2.0f;
            t.limbSwing = 0.0f;
            t.voicePitch = 1.3f;

            // It flies: held off whatever is under it rather than
            // walking on it, the way a blaze is.
            t.hoverHeight = 2.4f;

            t.ambientMinSeconds = 10.0f;
            t.ambientMaxSeconds = 30.0f;

            t.bodyColour = rgb(68, 54, 44);
            t.headColour = rgb(84, 66, 52);
            t.eyeColour = rgb(190, 60, 50);
            t.pattern = MobType::Pattern::Belly;

            t.model.push_back(mc(Part::Head, 0, 0, 4, 4, 4, -2, 8, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 10, 4, 6, 4, -2, 12, -2, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 20, 0, 10, 8, 1, -12, 11, -1, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 20, 0, 10, 8, 1, 2, 11, -1, 0, 0, 0)));
        }

        {
            // The cave spider: a spider at half the size with poison in
            // it. The mineshafts have been empty since they were dug.
            MobType& t = table[static_cast<size_t>(MobId::CaveSpider)];
            t.id = MobId::CaveSpider;
            t.experience = 5;
            t.voice = Sound::MobCaveSpiderSay;
            t.hurtVoice = Sound::MobCaveSpiderHurt;
            t.deathVoice = Sound::MobCaveSpiderDeath;
            t.name = "Cave Spider";
            t.texture = "cave_spider";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 12;
            t.width = 1.4f;
            t.height = 0.9f;
            // Half again smaller than a spider, which is what lets one
            // through a gap you thought was safe.
            t.modelScale = 0.55f;
            t.walkSpeed = 2.8f;
            t.limbSwing = 0.3f;
            t.voicePitch = 1.7f;
            t.attackDamage = 2;
            t.attackInterval = 0.8f;
            t.attackReach = 1.0f;

            t.drop = Items::StringItem;
            t.dropCount = 2;

            t.bodyColour = rgb(22, 60, 72);
            t.headColour = rgb(30, 74, 88);
            t.eyeColour = rgb(200, 60, 50);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(14, 40, 50);

            t.model.push_back(mc(Part::Head, 32, 4, 8, 8, 8, -4, -4, -8, 0, 15, -3));
            t.model.push_back(mc(Part::Body, 0, 0, 6, 6, 6, -3, -3, -3, 0, 15, 0));
            t.model.push_back(mc(Part::Body, 0, 12, 10, 8, 12, -5, -4, -6, 0, 15, 9));
            addSpiderLeg(t.model, Part::LegBackRight,   2, LEG_WIDE,   LEG_WIDE);
            addSpiderLeg(t.model, Part::LegBackLeft,    1, LEG_NARROW, LEG_TURN);
            addSpiderLeg(t.model, Part::LegFrontRight,  0, LEG_NARROW, -LEG_TURN);
            addSpiderLeg(t.model, Part::LegFrontLeft,  -1, LEG_WIDE,   -LEG_WIDE);
        }

        {
            // The silverfish: the small thing in the walls of a
            // stronghold. It is barely worth killing and it is the
            // reason the walls of a stronghold are frightening.
            MobType& t = table[static_cast<size_t>(MobId::Silverfish)];
            t.id = MobId::Silverfish;
            t.experience = 5;
            t.voice = Sound::MobSilverfishSay;
            t.hurtVoice = Sound::MobSilverfishHurt;
            t.deathVoice = Sound::MobSilverfishDeath;
            t.name = "Silverfish";
            t.texture = "silverfish";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 8;
            t.width = 0.4f;
            t.height = 0.3f;
            t.walkSpeed = 2.6f;
            t.limbSwing = 0.2f;
            t.voicePitch = 1.6f;
            t.attackDamage = 1;
            t.attackInterval = 0.7f;
            t.attackReach = 0.8f;

            t.bodyColour = rgb(112, 112, 116);
            t.headColour = rgb(140, 140, 146);
            t.eyeColour = rgb(40, 40, 44);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(76, 76, 80);

            // Three segments on the floor, the middle one tallest, so
            // it reads as a thing rather than as a brick.
            t.model.push_back(mc(Part::Body, 0, 0, 5, 4, 4, -2, 20, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 10, 3, 3, 3, -1, 21, -5, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 18, 4, 3, 4, -2, 21, 2, 0, 0, 0));
        }

        {
            // The piglin: what a zombified one used to be. Neutral,
            // like Minecraft's -- it minds its own business until you
            // give it a reason not to, which is what makes a bastion
            // somewhere you can walk through if you are careful.
            MobType& t = table[static_cast<size_t>(MobId::Piglin)];
            t.id = MobId::Piglin;
            t.experience = 5;
            t.voice = Sound::MobPiglinSay;
            t.hurtVoice = Sound::MobPiglinHurt;
            t.deathVoice = Sound::MobPiglinDeath;
            t.name = "Piglin";
            t.texture = "piglin";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.neutral = true;
            t.dimension = Dimension::Nether;
            // Minecraft's piglins and hoglins burn like anything else.
            // Here nothing teaches a mob to walk round a lava lake, so
            // one that was not fireproof would be one that walked into
            // the nearest lake and died before you ever met it.
            t.fireproof = true;
            t.maxHealth = 16;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 2.3f;
            t.limbSwing = 0.5f;
            t.attackDamage = 5;

            t.drop = Items::GoldNugget;
            t.dropCount = 2;

            t.bodyColour = rgb(96, 70, 56);
            t.headColour = rgb(234, 168, 160);
            t.eyeColour = rgb(40, 32, 28);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            // The snout, which is what tells a piglin from a man at
            // any distance worth looking from.
            t.model.push_back(mc(Part::Head, 31, 1, 6, 3, 2, -3, -5, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 4, 12, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 4, 12, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The brute, which is not neutral about anything. It
            // guards a bastion and it does not care what you are
            // wearing.
            MobType& t = table[static_cast<size_t>(MobId::PiglinBrute)];
            t.id = MobId::PiglinBrute;
            t.experience = 20;
            t.voice = Sound::MobPiglinBruteSay;
            t.hurtVoice = Sound::MobPiglinBruteHurt;
            t.deathVoice = Sound::MobPiglinBruteDeath;
            t.name = "Piglin Brute";
            t.texture = "piglin_brute";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.dimension = Dimension::Nether;
            // Minecraft's piglins and hoglins burn like anything else.
            // Here nothing teaches a mob to walk round a lava lake, so
            // one that was not fireproof would be one that walked into
            // the nearest lake and died before you ever met it.
            t.fireproof = true;
            t.maxHealth = 50;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 2.4f;
            t.limbSwing = 0.5f;
            t.attackDamage = 9;
            t.attackInterval = 1.1f;

            t.drop = Items::GoldNugget;
            t.dropCount = 4;

            t.bodyColour = rgb(66, 50, 44);
            t.headColour = rgb(214, 150, 142);
            t.eyeColour = rgb(40, 32, 28);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(44, 32, 28);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            // The snout, which is what tells a piglin from a man at
            // any distance worth looking from.
            t.model.push_back(mc(Part::Head, 31, 1, 6, 3, 2, -3, -5, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 4, 12, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 4, 12, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The hoglin: the Nether's own livestock, in the sense
            // that it is made of meat and would rather you were.
            MobType& t = table[static_cast<size_t>(MobId::Hoglin)];
            t.id = MobId::Hoglin;
            t.experience = 5;
            t.voice = Sound::MobHoglinSay;
            t.hurtVoice = Sound::MobHoglinHurt;
            t.deathVoice = Sound::MobHoglinDeath;
            t.name = "Hoglin";
            t.texture = "hoglin";
            t.sheetWidth = 128;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.dimension = Dimension::Nether;
            // Minecraft's piglins and hoglins burn like anything else.
            // Here nothing teaches a mob to walk round a lava lake, so
            // one that was not fireproof would be one that walked into
            // the nearest lake and died before you ever met it.
            t.fireproof = true;
            t.maxHealth = 40;
            t.width = 1.4f;
            t.height = 1.4f;
            t.walkSpeed = 2.4f;
            t.limbSwing = 0.5f;
            t.attackDamage = 6;
            t.attackReach = 1.4f;

            t.drop = Items::RawPorkchop;
            t.dropCount = 3;
            t.secondDrop = Items::Leather;
            t.secondDropCount = 1;

            t.bodyColour = rgb(142, 86, 70);
            t.headColour = rgb(156, 96, 78);
            t.eyeColour = rgb(40, 28, 24);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(106, 60, 48);

            t.model.push_back(mc(Part::Body, 0, 0, 14, 12, 20, -7, 2, -10, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 70, 0, 10, 8, 10, -5, 4, -20, 0, 0, 0));
            // The tusks, which are the whole of why you run.
            t.model.push_back(mc(Part::Head, 70, 20, 2, 3, 2, -6, 8, -20, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::Head, 70, 20, 2, 3, 2, 4, 8, -20, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 0, 34, 6, 10, 6, -3, 0, -3, -4, 14, -6));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 34, 6, 10, 6, -3, 0, -3, 4, 14, -6)));
            t.model.push_back(mc(Part::LegBackRight, 0, 34, 6, 10, 6, -3, 0, -3, -4, 14, 6));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 34, 6, 10, 6, -3, 0, -3, 4, 14, 6)));
        }

        {
            // The strider: the one way across a lava sea. It attacks
            // nobody, nothing in the Nether attacks it, and it walks
            // on the surface of the lava the way a boat sits on water.
            MobType& t = table[static_cast<size_t>(MobId::Strider)];
            t.id = MobId::Strider;
            t.experience = 1;
            t.voice = Sound::MobStriderSay;
            t.hurtVoice = Sound::MobStriderHurt;
            t.deathVoice = Sound::MobStriderDeath;
            t.name = "Strider";
            t.texture = "strider";
            t.spawnClass = SpawnClass::Passive;
            t.dimension = Dimension::Nether;
            t.fireproof = true;
            t.lavaWalker = true;
            t.maxHealth = 20;
            t.width = 0.9f;
            t.height = 1.7f;
            t.walkSpeed = 1.6f;
            t.limbSwing = 0.7f;

            t.rideable = true;
            t.saddleHeight = 1.6f;

            t.drop = Items::StringItem;
            t.dropCount = 2;

            t.bodyColour = rgb(148, 48, 54);
            t.headColour = rgb(170, 60, 64);
            t.eyeColour = rgb(226, 214, 200);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(102, 30, 36);

            // A tall slab on two legs, which is the whole of it.
            t.model.push_back(mc(Part::Body, 0, 0, 16, 14, 8, -8, -4, -4, 0, 0, 0));
            t.model.push_back(mc(Part::LegBackRight, 48, 0, 4, 14, 4, -2, 0, -2, -4, 10, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 48, 0, 4, 14, 4, -2, 0, -2, 4, 10, 0)));
        }

        // The six in the sea. All of them live in the water the way a
        // squid does, and none of them can be put anywhere else.
        {
            // The cod: the ordinary fish, and the one you eat.
            MobType& t = table[static_cast<size_t>(MobId::Cod)];
            t.id = MobId::Cod;
            t.experience = 1;
            t.voice = Sound::MobCodSay;
            t.hurtVoice = Sound::MobCodHurt;
            t.deathVoice = Sound::MobCodDeath;
            t.name = "Cod";
            t.texture = "cod";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 3;
            t.width = 0.5f;
            t.height = 0.3f;
            t.walkSpeed = 2.2f;
            t.limbSwing = 0.0f;
            t.voicePitch = 1.6f;
            t.aquatic = true;
            t.hoverHeight = 1.8f;

            t.drop = Items::RawCod;
            t.dropCount = 1;

            t.bodyColour = rgb(186, 160, 118);
            t.headColour = rgb(206, 182, 140);
            t.eyeColour = rgb(30, 30, 36);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(238, 226, 198);

            // A body, a tail and a fin: at half a block long there is
            // nothing else to put on one. Long along z, because that
            // is the way everything here faces -- the first pair of
            // fish were built across their own direction of travel
            // and swam through the sea sideways.
            t.model.push_back(mc(Part::Body, 0, 0, 3, 5, 8, -1, 19, -4, 0, 0, 0));
            t.model.push_back(mc(Part::LegBackRight, 0, 14, 1, 5, 4, 0, 19, 4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 12, 14, 3, 2, 3, -1, 17, -2, 0, 0, 0));
        }

        {
            // The salmon: bigger, redder, and worth more cooked.
            MobType& t = table[static_cast<size_t>(MobId::Salmon)];
            t.id = MobId::Salmon;
            t.experience = 1;
            t.voice = Sound::MobSalmonSay;
            t.hurtVoice = Sound::MobSalmonHurt;
            t.deathVoice = Sound::MobSalmonDeath;
            t.name = "Salmon";
            t.texture = "salmon";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 3;
            t.width = 0.7f;
            t.height = 0.4f;
            t.walkSpeed = 2.4f;
            t.limbSwing = 0.0f;
            t.voicePitch = 1.4f;
            t.aquatic = true;
            t.hoverHeight = 2.0f;

            t.drop = Items::RawSalmon;
            t.dropCount = 1;

            t.bodyColour = rgb(168, 78, 62);
            t.headColour = rgb(188, 104, 86);
            t.eyeColour = rgb(30, 30, 36);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(226, 182, 162);

            t.model.push_back(mc(Part::Body, 0, 0, 4, 6, 10, -2, 18, -5, 0, 0, 0));
            t.model.push_back(mc(Part::LegBackRight, 0, 18, 1, 6, 5, 0, 18, 5, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 14, 18, 4, 2, 4, -2, 16, -2, 0, 0, 0));
        }

        {
            // The pufferfish: the one thing in the sea you should not
            // eat, and the only food in the game that is a mistake.
            MobType& t = table[static_cast<size_t>(MobId::Pufferfish)];
            t.id = MobId::Pufferfish;
            t.experience = 1;
            t.voice = Sound::MobPufferfishSay;
            t.hurtVoice = Sound::MobPufferfishHurt;
            t.deathVoice = Sound::MobPufferfishDeath;
            t.name = "Pufferfish";
            t.texture = "pufferfish";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 3;
            t.width = 0.6f;
            t.height = 0.6f;
            t.walkSpeed = 1.4f;
            t.limbSwing = 0.0f;
            t.voicePitch = 1.8f;
            t.aquatic = true;
            t.hoverHeight = 1.6f;

            t.drop = Items::Pufferfish;
            t.dropCount = 1;

            t.bodyColour = rgb(220, 180, 60);
            t.headColour = rgb(234, 200, 96);
            t.eyeColour = rgb(30, 30, 36);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(150, 120, 36);

            t.model.push_back(mc(Part::Body, 0, 0, 8, 8, 8, -4, 16, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 20, 2, 2, 2, -1, 19, -6, 0, 0, 0));
        }

        {
            // The dolphin: the one thing in the sea that plays. It is
            // not food and it is not a fight; it is the reason an
            // ocean is worth crossing on the surface.
            MobType& t = table[static_cast<size_t>(MobId::Dolphin)];
            t.id = MobId::Dolphin;
            t.experience = 3;
            t.voice = Sound::MobDolphinSay;
            t.hurtVoice = Sound::MobDolphinHurt;
            t.deathVoice = Sound::MobDolphinDeath;
            t.name = "Dolphin";
            t.texture = "dolphin";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.9f;
            t.height = 0.6f;
            t.walkSpeed = 3.6f;
            t.limbSwing = 0.0f;
            t.aquatic = true;
            t.hoverHeight = 2.6f;

            t.drop = Items::RawCod;
            t.dropCount = 1;

            t.bodyColour = rgb(96, 128, 150);
            t.headColour = rgb(116, 148, 170);
            t.eyeColour = rgb(24, 26, 32);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(226, 236, 240);

            t.model.push_back(mc(Part::Body, 0, 0, 7, 7, 13, -4, 16, -7, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 22, 5, 4, 5, -3, 18, -11, 0, 0, 0));
            t.model.push_back(mc(Part::LegBackRight, 24, 22, 6, 1, 5, -3, 20, 6, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 48, 22, 2, 4, 3, -1, 13, -3, 0, 0, 0));
        }

        {
            // The turtle: it swims, it comes ashore, and it is the one
            // thing in the sea that is not in a hurry.
            MobType& t = table[static_cast<size_t>(MobId::Turtle)];
            t.id = MobId::Turtle;
            t.experience = 1;
            t.voice = Sound::MobTurtleSay;
            t.hurtVoice = Sound::MobTurtleHurt;
            t.deathVoice = Sound::MobTurtleDeath;
            t.name = "Turtle";
            t.texture = "turtle";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 30;
            t.width = 1.2f;
            t.height = 0.4f;
            t.walkSpeed = 0.8f;
            t.limbSwing = 0.5f;
            t.voicePitch = 0.8f;
            // It swims, but it walks on the bottom and on the beach
            // rather than hanging in the water the way a fish does.
            t.aquatic = true;

            t.breedingFood = Items::WheatSeeds;

            t.drop = Blocks::Seagrass;
            t.dropCount = 1;

            t.bodyColour = rgb(72, 108, 66);
            t.headColour = rgb(118, 158, 96);
            t.eyeColour = rgb(30, 30, 36);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(44, 78, 44);

            t.model.push_back(mc(Part::Body, 0, 0, 14, 4, 12, -7, 20, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 20, 5, 4, 5, -2, 20, -10, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 22, 20, 3, 1, 4, -8, 23, -5, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 22, 20, 3, 1, 4, 5, 23, -5, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 38, 20, 3, 1, 4, -8, 23, 2, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 38, 20, 3, 1, 4, 5, 23, 2, 0, 0, 0)));
        }

        {
            // The axolotl: it does nothing useful and it is the best
            // thing in the lush caves.
            MobType& t = table[static_cast<size_t>(MobId::Axolotl)];
            t.id = MobId::Axolotl;
            t.experience = 1;
            t.voice = Sound::MobAxolotlSay;
            t.hurtVoice = Sound::MobAxolotlHurt;
            t.deathVoice = Sound::MobAxolotlDeath;
            t.name = "Axolotl";
            t.texture = "axolotl";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 14;
            t.width = 0.75f;
            t.height = 0.42f;
            t.walkSpeed = 1.6f;
            t.limbSwing = 0.4f;
            t.voicePitch = 1.4f;
            t.aquatic = true;
            t.hoverHeight = 1.4f;

            t.breedingFood = Items::RawCod;

            t.bodyColour = rgb(236, 174, 196);
            t.headColour = rgb(244, 192, 210);
            t.eyeColour = rgb(40, 32, 40);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(252, 226, 236);

            t.model.push_back(mc(Part::Body, 0, 0, 5, 5, 8, -2, 19, -3, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 14, 4, 3, 5, -2, 19, -8, 0, 0, 0));
            // The frills, which are the whole of why anybody wants one.
            t.model.push_back(mc(Part::WingRight, 0, 24, 3, 2, 3, -5, 19, -7, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 0, 24, 3, 2, 3, 2, 19, -7, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 14, 24, 2, 1, 4, -3, 23, -1, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 14, 24, 2, 1, 4, 1, 23, -1, 0, 0, 0)));
        }

        // The country animals. Four species covered the whole of the
        // overworld before these: a sheep, a pig, a cow and a chicken,
        // in every biome whatever the ground was made of.
        {
            // The rabbit: the small thing that runs away, and the one
            // animal in a desert worth eating.
            MobType& t = table[static_cast<size_t>(MobId::Rabbit)];
            t.id = MobId::Rabbit;
            t.experience = 1;
            t.voice = Sound::MobRabbitSay;
            t.hurtVoice = Sound::MobRabbitHurt;
            t.deathVoice = Sound::MobRabbitDeath;
            t.name = "Rabbit";
            t.texture = "rabbit";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 3;
            t.width = 0.4f;
            t.height = 0.5f;
            t.walkSpeed = 3.0f;
            t.limbSwing = 0.8f;
            t.voicePitch = 1.4f;
            t.breedingFood = Items::Carrot;

            t.drop = Items::RawRabbit;
            t.dropCount = 1;

            t.bodyColour = rgb(166, 140, 112);
            t.headColour = rgb(186, 162, 134);
            t.eyeColour = rgb(180, 70, 70);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(226, 212, 190);

            t.model.push_back(mc(Part::Body, 0, 0, 4, 5, 8, -2, 19, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 15, 5, 4, 5, -2, 17, -8, 0, 0, 0));
            // The ears, which are the whole of what reads as a rabbit.
            t.model.push_back(mc(Part::WingRight, 24, 15, 2, 5, 1, -2, 12, -6, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 24, 15, 2, 5, 1, 0, 12, -6, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 0, 26, 2, 3, 2, -2, 21, -5, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 26, 2, 3, 2, 0, 21, -5, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 32, 15, 2, 4, 4, -2, 20, 1, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 32, 15, 2, 4, 4, 0, 20, 1, 0, 0, 0)));
        }

        {
            // The fox: the thing in the trees that is not a wolf.
            MobType& t = table[static_cast<size_t>(MobId::Fox)];
            t.id = MobId::Fox;
            t.experience = 1;
            t.voice = Sound::MobFoxSay;
            t.hurtVoice = Sound::MobFoxHurt;
            t.deathVoice = Sound::MobFoxDeath;
            t.name = "Fox";
            t.texture = "fox";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.6f;
            t.height = 0.7f;
            t.walkSpeed = 3.2f;
            t.limbSwing = 0.7f;
            t.voicePitch = 1.2f;
            t.breedingFood = Items::GlowBerries;

            t.bodyColour = rgb(212, 124, 50);
            t.headColour = rgb(226, 150, 76);
            t.eyeColour = rgb(40, 32, 28);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(238, 232, 222);

            t.model.push_back(mc(Part::Body, 0, 0, 6, 6, 11, -3, 17, -5, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 20, 8, 6, 6, -4, 15, -11, 0, 0, 0));
            // The snout and the brush, which is all a fox is.
            t.model.push_back(mc(Part::Head, 32, 20, 4, 2, 3, -2, 19, -14, 0, 0, 0));
            t.model.push_back(mc(Part::LegBackRight, 48, 20, 2, 6, 2, -3, 18, 6, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 36, 0, 2, 7, 2, -3, 17, -4, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 36, 0, 2, 7, 2, 1, 17, -4, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackLeft, 46, 0, 2, 7, 2, -3, 17, 3, 0, 0, 0));
        }

        {
            // The goat: what a mountain is missing. It is the only
            // animal in the game that would rather be somewhere steep.
            MobType& t = table[static_cast<size_t>(MobId::Goat)];
            t.id = MobId::Goat;
            t.experience = 1;
            t.voice = Sound::MobGoatSay;
            t.hurtVoice = Sound::MobGoatHurt;
            t.deathVoice = Sound::MobGoatDeath;
            t.name = "Goat";
            t.texture = "goat";
            t.sheetWidth = 128;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.9f;
            t.height = 1.3f;
            t.walkSpeed = 2.6f;
            t.limbSwing = 0.6f;
            t.breedingFood = Items::Wheat;

            t.drop = Items::RawMutton;
            t.dropCount = 1;

            t.bodyColour = rgb(226, 220, 210);
            t.headColour = rgb(236, 232, 224);
            t.eyeColour = rgb(50, 44, 40);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(190, 180, 166);

            t.model.push_back(mc(Part::Body, 0, 0, 9, 11, 16, -5, 6, -8, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 50, 0, 5, 7, 10, -3, 2, -16, 0, 0, 0));
            // The horns, swept back the way a goat's are.
            t.model.push_back(mc(Part::Head, 50, 18, 2, 5, 2, -4, 0, -12, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::Head, 50, 18, 2, 5, 2, 2, 0, -12, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 0, 34, 3, 7, 3, -2, 0, -2, -3, 17, -5));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 34, 3, 7, 3, -2, 0, -2, 3, 17, -5)));
            t.model.push_back(mc(Part::LegBackRight, 0, 34, 3, 7, 3, -2, 0, -2, -3, 17, 5));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 34, 3, 7, 3, -2, 0, -2, 3, 17, 5)));
        }

        {
            // The panda: the one thing a jungle has that nowhere else
            // does, and the only reason to go into one on purpose.
            MobType& t = table[static_cast<size_t>(MobId::Panda)];
            t.id = MobId::Panda;
            t.experience = 1;
            t.voice = Sound::MobPandaSay;
            t.hurtVoice = Sound::MobPandaHurt;
            t.deathVoice = Sound::MobPandaDeath;
            t.name = "Panda";
            t.texture = "panda";
            t.sheetWidth = 64;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 20;
            t.width = 1.3f;
            t.height = 1.25f;
            t.walkSpeed = 1.6f;
            t.limbSwing = 0.5f;
            t.voicePitch = 0.8f;
            t.breedingFood = Blocks::Bamboo;

            t.bodyColour = rgb(226, 222, 214);
            t.headColour = rgb(234, 232, 226);
            t.eyeColour = rgb(36, 32, 32);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(36, 34, 34);

            t.model.push_back(mc(Part::Body, 0, 0, 12, 10, 16, -6, 8, -8, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 30, 9, 9, 9, -5, 6, -15, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 40, 30, 2, 3, 2, -6, 3, -12, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 40, 30, 2, 3, 2, 4, 3, -12, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 40, 44, 4, 6, 4, -2, 0, -2, -4, 18, -5));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 44, 4, 6, 4, -2, 0, -2, 4, 18, -5)));
            t.model.push_back(mc(Part::LegBackRight, 40, 44, 4, 6, 4, -2, 0, -2, -4, 18, 5));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 40, 44, 4, 6, 4, -2, 0, -2, 4, 18, 5)));
        }

        {
            // The bee. It flies, it minds its own business until you
            // hit it, and the wax it leaves is what a candle is made
            // of -- which used to be wool, for want of a bee.
            MobType& t = table[static_cast<size_t>(MobId::Bee)];
            t.id = MobId::Bee;
            t.experience = 1;
            t.voice = Sound::MobBeeSay;
            t.hurtVoice = Sound::MobBeeHurt;
            t.deathVoice = Sound::MobBeeDeath;
            t.name = "Bee";
            t.texture = "bee";
            // Passive is how it is spawned, not how it behaves:
            // the daylight rule that lets an animal be put down at
            // all asks the spawn class, and a bee put down only in
            // the dark is a bee nobody ever meets. What it does when
            // you hit it is the neutral flag below, which is how the
            // iron golem works too.
            t.spawnClass = SpawnClass::Passive;
            t.neutral = true;
            t.maxHealth = 10;
            t.width = 0.7f;
            t.height = 0.6f;
            t.walkSpeed = 2.2f;
            t.limbSwing = 0.0f;
            t.voicePitch = 1.2f;
            t.attackDamage = 2;
            t.hoverHeight = 2.0f;

            t.ambientMinSeconds = 3.0f;
            t.ambientMaxSeconds = 9.0f;

            t.drop = Items::Honeycomb;
            t.dropCount = 1;

            t.bodyColour = rgb(228, 186, 60);
            t.headColour = rgb(76, 62, 44);
            t.eyeColour = rgb(30, 26, 24);
            t.pattern = MobType::Pattern::Stripes;
            t.markColour = rgb(60, 48, 32);

            t.model.push_back(mc(Part::Body, 0, 0, 7, 7, 10, -3, 16, -5, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 20, 5, 5, 4, -2, 17, -8, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 20, 20, 9, 1, 6, -10, 14, -3, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 20, 20, 9, 1, 6, 1, 14, -3, 0, 0, 0)));
        }

        {
            // The cat: it follows you about and it does nothing at
            // all, which is the whole of what a cat is for.
            MobType& t = table[static_cast<size_t>(MobId::Cat)];
            t.id = MobId::Cat;
            t.experience = 1;
            t.voice = Sound::MobCatSay;
            t.hurtVoice = Sound::MobCatHurt;
            t.deathVoice = Sound::MobCatDeath;
            t.name = "Cat";
            t.texture = "cat";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.6f;
            t.height = 0.7f;
            t.walkSpeed = 3.0f;
            t.limbSwing = 0.6f;
            t.voicePitch = 1.3f;
            t.breedingFood = Items::RawCod;

            t.bodyColour = rgb(150, 126, 102);
            t.headColour = rgb(168, 146, 120);
            t.eyeColour = rgb(200, 190, 70);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(96, 78, 60);

            t.model.push_back(mc(Part::Body, 0, 0, 5, 5, 12, -2, 17, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 20, 5, 4, 5, -2, 17, -11, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 24, 20, 1, 1, 2, -2, 15, -9, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::Head, 24, 20, 1, 1, 2, 1, 15, -9, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 34, 20, 1, 8, 1, 0, 16, 5, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 42, 20, 2, 6, 2, -2, 18, -4, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 42, 20, 2, 6, 2, 0, 18, -4, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackLeft, 52, 20, 2, 6, 2, -2, 18, 2, 0, 0, 0));
        }

        {
            // The llama: what a savanna is missing, and the only
            // animal in the game that is taller than it is long.
            MobType& t = table[static_cast<size_t>(MobId::Llama)];
            t.id = MobId::Llama;
            t.experience = 1;
            t.voice = Sound::MobLlamaSay;
            t.hurtVoice = Sound::MobLlamaHurt;
            t.deathVoice = Sound::MobLlamaDeath;
            t.name = "Llama";
            t.texture = "llama";
            t.sheetWidth = 128;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 20;
            t.width = 0.9f;
            t.height = 1.87f;
            t.walkSpeed = 2.2f;
            t.limbSwing = 0.5f;
            t.breedingFood = Items::Wheat;

            t.drop = Items::Leather;
            t.dropCount = 1;

            t.bodyColour = rgb(196, 176, 148);
            t.headColour = rgb(214, 196, 170);
            t.eyeColour = rgb(40, 34, 30);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(146, 126, 102);

            t.model.push_back(mc(Part::Body, 0, 0, 12, 18, 10, -6, 2, -5, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 50, 0, 4, 4, 9, -2, -8, -14, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 50, 14, 4, 10, 4, -2, -2, -11, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 80, 0, 1, 3, 2, -3, -10, -10, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::Head, 80, 0, 1, 3, 2, 2, -10, -10, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 0, 30, 4, 14, 4, -2, 0, -2, -3, 10, -4));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 30, 4, 14, 4, -2, 0, -2, 3, 10, -4)));
            t.model.push_back(mc(Part::LegBackRight, 0, 30, 4, 14, 4, -2, 0, -2, -3, 10, 4));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 30, 4, 14, 4, -2, 0, -2, 3, 10, 4)));
        }

        {
            // The polar bear: the only thing in the snow that is not
            // either a sheep or a skeleton, and the only animal in the
            // game you should walk away from.
            MobType& t = table[static_cast<size_t>(MobId::PolarBear)];
            t.id = MobId::PolarBear;
            t.experience = 5;
            t.voice = Sound::MobPolarBearSay;
            t.hurtVoice = Sound::MobPolarBearHurt;
            t.deathVoice = Sound::MobPolarBearDeath;
            t.name = "Polar Bear";
            t.texture = "polar_bear";
            t.sheetWidth = 128;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.neutral = true;
            t.maxHealth = 30;
            t.width = 1.4f;
            t.height = 1.4f;
            t.walkSpeed = 2.4f;
            t.limbSwing = 0.5f;
            t.voicePitch = 0.8f;
            t.attackDamage = 6;
            t.attackReach = 1.4f;

            t.bodyColour = rgb(238, 240, 240);
            t.headColour = rgb(244, 246, 246);
            t.eyeColour = rgb(36, 32, 32);

            t.model.push_back(mc(Part::Body, 0, 0, 14, 14, 20, -7, 1, -10, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 70, 0, 9, 8, 10, -5, 3, -19, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 70, 20, 5, 3, 4, -3, 8, -22, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 0, 36, 5, 9, 6, -3, 0, -3, -4, 15, -6));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 36, 5, 9, 6, -3, 0, -3, 4, 15, -6)));
            t.model.push_back(mc(Part::LegBackRight, 0, 36, 5, 9, 6, -3, 0, -3, -4, 15, 6));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 36, 5, 9, 6, -3, 0, -3, 4, 15, 6)));
        }

        // What a particular night has. The same five monsters turned
        // up everywhere before these, whatever the ground was made of
        // and whatever the weather had been doing.
        {
            // The stray: a skeleton that has been out in the cold. It
            // keeps to the snow and it shoots harder than its cousin.
            MobType& t = table[static_cast<size_t>(MobId::Stray)];
            t.id = MobId::Stray;
            t.experience = 5;
            t.voice = Sound::MobStraySay;
            t.hurtVoice = Sound::MobStrayHurt;
            t.deathVoice = Sound::MobStrayDeath;
            t.name = "Stray";
            t.texture = "stray";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 2.0f;
            t.burnsInSunlight = true;
            t.voicePitch = 0.9f;

            t.arrowDamage = 4;
            t.arrowRange = 17.0f;
            t.arrowInterval = 2.0f;

            t.drop = Items::Bone;
            t.dropCount = 2;
            t.secondDrop = Items::Arrow;
            t.secondDropCount = 1;

            t.bodyColour = rgb(172, 194, 196);
            t.headColour = rgb(196, 214, 216);
            t.eyeColour = rgb(20, 20, 20);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(140, 182, 196);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 2, 12, 2, -1, -2, -1, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 2, 12, 2, -1, -2, -1, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 2, 12, 2, -1, 0, -1, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 2, 12, 2, -1, 0, -1, 2, 12, 0)));
        }

        {
            // The husk: a zombie dried out. The sun is nothing to it,
            // which is the whole of what makes a desert dangerous by
            // day as well as by night.
            MobType& t = table[static_cast<size_t>(MobId::Husk)];
            t.id = MobId::Husk;
            t.experience = 5;
            t.voice = Sound::MobHuskSay;
            t.hurtVoice = Sound::MobHuskHurt;
            t.deathVoice = Sound::MobHuskDeath;
            t.name = "Husk";
            t.texture = "husk";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.9f;
            t.burnsInSunlight = false;
            t.voicePitch = 0.85f;
            t.attackDamage = 4;

            t.bodyColour = rgb(132, 116, 78);
            t.headColour = rgb(156, 138, 94);
            t.eyeColour = rgb(30, 26, 20);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(104, 90, 60);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 4, 12, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 4, 12, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The bogged: a skeleton that has been in the water, and
            // 1.21's own. It is softer than a skeleton and it shoots
            // more often, which is what makes a swamp at night a
            // place you leave rather than fight through.
            MobType& t = table[static_cast<size_t>(MobId::Bogged)];
            t.id = MobId::Bogged;
            t.experience = 5;
            t.voice = Sound::MobBoggedSay;
            t.hurtVoice = Sound::MobBoggedHurt;
            t.deathVoice = Sound::MobBoggedDeath;
            t.name = "Bogged";
            t.texture = "bogged";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 16;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.9f;
            t.burnsInSunlight = true;
            t.voicePitch = 1.1f;

            t.arrowDamage = 3;
            t.arrowRange = 15.0f;
            t.arrowInterval = 1.5f;

            t.drop = Items::Bone;
            t.dropCount = 2;

            t.bodyColour = rgb(118, 132, 96);
            t.headColour = rgb(140, 152, 112);
            t.eyeColour = rgb(20, 24, 18);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(74, 96, 58);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 8, 8, -4, -8, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 16, 8, 12, 4, -4, 0, -2, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 40, 16, 2, 12, 2, -1, -2, -1, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 40, 16, 2, 12, 2, -1, -2, -1, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 16, 2, 12, 2, -1, 0, -1, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 16, 2, 12, 2, -1, 0, -1, 2, 12, 0)));
        }

        {
            // The phantom: the thing in the sky. It is the only
            // monster in the game that comes to you rather than
            // waiting to be found.
            MobType& t = table[static_cast<size_t>(MobId::Phantom)];
            t.id = MobId::Phantom;
            t.experience = 5;
            t.voice = Sound::MobPhantomSay;
            t.hurtVoice = Sound::MobPhantomHurt;
            t.deathVoice = Sound::MobPhantomDeath;
            t.name = "Phantom";
            t.texture = "phantom";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.9f;
            t.height = 0.5f;
            t.walkSpeed = 3.8f;
            t.limbSwing = 0.0f;
            t.attackDamage = 4;
            t.attackReach = 1.2f;

            // High up, which is where it comes from and the reason you
            // hear one before you see it.
            t.hoverHeight = 5.0f;

            t.ambientMinSeconds = 4.0f;
            t.ambientMaxSeconds = 12.0f;

            t.bodyColour = rgb(68, 96, 108);
            t.headColour = rgb(84, 116, 128);
            t.eyeColour = rgb(140, 220, 90);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(140, 220, 90);

            t.model.push_back(mc(Part::Body, 0, 8, 5, 3, 9, -2, 19, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 0, 5, 3, 3, -2, 19, -7, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 24, 0, 6, 2, 9, -8, 20, -4, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 24, 0, 6, 2, 9, 2, 20, -4, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 22, 2, 2, 6, -1, 20, 5, 0, 0, 0));
        }

        {
            // A villager that did not get away. It is worth nothing
            // and it is the one monster that is somebody.
            MobType& t = table[static_cast<size_t>(MobId::ZombieVillager)];
            t.id = MobId::ZombieVillager;
            t.experience = 5;
            t.voice = Sound::MobZombieVillagerSay;
            t.hurtVoice = Sound::MobZombieVillagerHurt;
            t.deathVoice = Sound::MobZombieVillagerDeath;
            t.name = "Zombie Villager";
            t.texture = "zombie_villager";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.8f;
            t.burnsInSunlight = true;
            t.attackDamage = 3;

            t.bodyColour = rgb(78, 96, 76);
            t.headColour = rgb(96, 128, 76);
            t.eyeColour = rgb(24, 32, 24);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(56, 72, 54);

            // The villager's long head and nose on a zombie's body,
            // which is exactly what one is.
            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 8, -4, -10, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 24, 0, 2, 4, 2, -1, -5, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 20, 8, 12, 6, -4, 0, -3, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 44, 22, 4, 8, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 44, 22, 4, 8, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 22, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 22, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The endermite: what the End has in its walls, the way a
            // stronghold has silverfish in its.
            MobType& t = table[static_cast<size_t>(MobId::Endermite)];
            t.id = MobId::Endermite;
            t.experience = 3;
            t.voice = Sound::MobEndermiteSay;
            t.hurtVoice = Sound::MobEndermiteHurt;
            t.deathVoice = Sound::MobEndermiteDeath;
            t.name = "Endermite";
            t.texture = "endermite";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 8;
            t.width = 0.4f;
            t.height = 0.3f;
            t.walkSpeed = 2.4f;
            t.limbSwing = 0.2f;
            t.voicePitch = 1.5f;
            t.attackDamage = 2;
            t.attackReach = 0.8f;

            t.bodyColour = rgb(52, 36, 64);
            t.headColour = rgb(72, 50, 88);
            t.eyeColour = rgb(200, 90, 230);
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(140, 70, 170);

            t.model.push_back(mc(Part::Body, 0, 0, 4, 4, 3, -2, 20, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 10, 3, 3, 3, -1, 20, -5, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 18, 3, 3, 3, -1, 21, 1, 0, 0, 0));
        }

        {
            // The vex: what an evoker sends at you. Small, fast and
            // through walls, and there is nothing to kill but the
            // thing that made it.
            MobType& t = table[static_cast<size_t>(MobId::Vex)];
            t.id = MobId::Vex;
            t.experience = 3;
            t.voice = Sound::MobVexSay;
            t.hurtVoice = Sound::MobVexHurt;
            t.deathVoice = Sound::MobVexDeath;
            t.name = "Vex";
            t.texture = "vex";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 14;
            t.width = 0.4f;
            t.height = 0.8f;
            t.walkSpeed = 4.0f;
            t.limbSwing = 0.3f;
            t.voicePitch = 1.4f;
            t.attackDamage = 5;
            t.attackReach = 1.0f;
            t.hoverHeight = 2.2f;

            t.bodyColour = rgb(138, 156, 172);
            t.headColour = rgb(206, 218, 226);
            t.eyeColour = rgb(220, 60, 50);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(226, 236, 244);

            // A small person with wings, at half a villager's scale.
            t.model.push_back(mc(Part::Head, 0, 0, 6, 6, 6, -3, 10, -3, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 14, 5, 7, 3, -2, 16, -1, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 0, 32, 1, 8, 6, -4, 13, 0, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 0, 32, 1, 8, 6, 3, 13, 0, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 32, 14, 2, 5, 2, -1, 0, -1, -3, 17, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 32, 14, 2, 5, 2, -1, 0, -1, 3, 17, 0)));
        }

        {
            // The snow golem: the one mob in the game you build. Two
            // snow blocks and a pumpkin on top, which is Minecraft's
            // and the reason a pumpkin is worth keeping.
            MobType& t = table[static_cast<size_t>(MobId::SnowGolem)];
            t.id = MobId::SnowGolem;
            t.experience = 0;
            t.voice = Sound::MobSnowGolemSay;
            t.hurtVoice = Sound::MobSnowGolemHurt;
            t.deathVoice = Sound::MobSnowGolemDeath;
            t.name = "Snow Golem";
            t.texture = "snow_golem";
            t.sheetWidth = 128;
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            // Neutral like the iron golem: it is on your side and it
            // throws snow at what comes for you, and if you break it
            // up it will throw snow at you instead. Minecraft's never
            // turns on the player; this one does, which is one line
            // against a passive mob that can somehow fight.
            t.neutral = true;
            t.maxHealth = 4;
            t.width = 0.7f;
            t.height = 1.9f;
            t.walkSpeed = 2.0f;
            t.limbSwing = 0.0f;

            // It throws snow. In Minecraft that barely hurts anything
            // -- what it does is knock them back and keep them off
            // you, which is the whole of what one is for.
            t.arrowDamage = 1;
            t.arrowRange = 14.0f;
            t.arrowInterval = 1.2f;

            t.drop = Items::Snowball;
            t.dropCount = 4;

            t.bodyColour = rgb(242, 246, 250);
            t.headColour = rgb(224, 150, 44);
            t.eyeColour = rgb(40, 34, 30);

            // Two snowballs and a pumpkin, which is what it is made
            // of and what it looks like.
            t.model.push_back(mc(Part::Body, 0, 0, 12, 10, 12, -6, 14, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 24, 10, 9, 10, -5, 5, -5, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 48, 24, 10, 10, 10, -5, -5, -5, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 90, 0, 2, 8, 2, -1, 0, -1, -6, 6, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 90, 0, 2, 8, 2, -1, 0, -1, 6, 6, 0)));
        }

        {
            // The elder guardian: what an ocean monument is for. A
            // guardian four times the size, and the only thing in the
            // sea worth drowning over.
            MobType& t = table[static_cast<size_t>(MobId::ElderGuardian)];
            t.id = MobId::ElderGuardian;
            t.experience = 40;
            t.voice = Sound::MobElderGuardianSay;
            t.hurtVoice = Sound::MobElderGuardianHurt;
            t.deathVoice = Sound::MobElderGuardianDeath;
            t.name = "Elder Guardian";
            t.texture = "elder_guardian";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 80;
            // A guardian at twice the size, which is both the drawing
            // and the box: the numbers here are the model's own.
            t.modelScale = 2.0f;
            t.width = 0.95f;
            t.height = 0.95f;
            t.walkSpeed = 1.8f;
            t.limbSwing = 0.3f;
            t.voicePitch = 0.6f;
            t.aquatic = true;
            t.hoverHeight = 2.2f;

            t.shootsFire = true;
            t.arrowDamage = 8;
            t.arrowRange = 18.0f;
            t.arrowInterval = 3.4f;

            t.drop = Blocks::Prismarine;
            t.dropCount = 3;
            t.secondDrop = Blocks::SeaLantern;
            t.secondDropCount = 1;

            t.bodyColour = rgb(170, 168, 142);
            t.headColour = rgb(192, 190, 164);
            t.eyeColour = 0xFFE8D070;
            t.pattern = MobType::Pattern::Speckled;
            t.markColour = rgb(120, 118, 96);

            t.model.push_back(mc(Part::Body, 0, 0, 14, 14, 14, -7, -7, -7, 0, 15, 0));
            t.model.push_back(mc(Part::Head, 0, 28, 5, 5, 1, -2, -2, 0, 0, 15, -7));
            t.model.push_back(mc(Part::LegBackLeft, 44, 0, 2, 2, 7, -1, -1, 0, 0, 15, 7));
            t.model.push_back(mc(Part::WingLeft, 0, 18, 2, 9, 2, -1, 0, -1, -8, 15, 0));
            t.model.push_back(mc(Part::WingRight, 0, 18, 2, 9, 2, -1, 0, -1, 8, 15, 0));
        }

        {
            // The wandering trader: a villager with no village, who
            // turns up where you are and sells what a village would.
            MobType& t = table[static_cast<size_t>(MobId::WanderingTrader)];
            t.id = MobId::WanderingTrader;
            t.experience = 0;
            t.voice = Sound::MobTraderSay;
            t.hurtVoice = Sound::MobTraderHurt;
            t.deathVoice = Sound::MobTraderDeath;
            t.name = "Wandering Trader";
            t.texture = "wandering_trader";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.95f;
            t.walkSpeed = 1.6f;
            t.voicePitch = 1.1f;

            t.bodyColour = rgb(44, 76, 132);
            t.headColour = rgb(188, 146, 118);
            t.eyeColour = rgb(40, 50, 70);

            t.model.push_back(mc(Part::Head, 0, 0, 8, 10, 8, -4, -10, -4, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 24, 0, 2, 4, 2, -1, -5, -6, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 16, 20, 8, 12, 6, -4, 0, -3, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 38, 8, 6, 6, -4, 12, -3, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 44, 22, 4, 8, 4, -3, -2, -2, -5, 2, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 44, 22, 4, 8, 4, -1, -2, -2, 5, 2, 0)));
            t.model.push_back(mc(Part::LegBackRight, 0, 22, 4, 12, 4, -2, 0, -2, -2, 12, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 22, 4, 12, 4, -2, 0, -2, 2, 12, 0)));
        }

        {
            // The camel: the tallest thing you can sit on, and what a
            // desert has instead of a horse.
            MobType& t = table[static_cast<size_t>(MobId::Camel)];
            t.id = MobId::Camel;
            t.experience = 1;
            t.voice = Sound::MobCamelSay;
            t.hurtVoice = Sound::MobCamelHurt;
            t.deathVoice = Sound::MobCamelDeath;
            t.name = "Camel";
            t.texture = "camel";
            t.sheetWidth = 128;
            t.sheetHeight = 128;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 32;
            t.width = 1.7f;
            t.height = 2.375f;
            t.walkSpeed = 2.4f;
            t.limbSwing = 0.5f;
            t.voicePitch = 0.9f;
            t.breedingFood = Blocks::Cactus;

            t.rideable = true;
            t.saddleHeight = 2.1f;

            t.bodyColour = rgb(222, 186, 124);
            t.headColour = rgb(232, 200, 142);
            t.eyeColour = rgb(50, 40, 32);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(244, 220, 176);

            t.model.push_back(mc(Part::Body, 0, 0, 12, 18, 23, -6, -6, -11, 0, 0, 0));
            // The hump, which is the whole of what reads as a camel.
            t.model.push_back(mc(Part::Body, 76, 0, 9, 6, 11, -4, -12, -5, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 76, 20, 7, 12, 9, -3, -18, -20, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 76, 44, 5, 5, 8, -2, -10, -26, 0, 0, 0));
            t.model.push_back(mc(Part::LegFrontRight, 0, 44, 4, 21, 4, -2, 0, -2, -4, 3, -8));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 0, 44, 4, 21, 4, -2, 0, -2, 4, 3, -8)));
            t.model.push_back(mc(Part::LegBackRight, 0, 44, 4, 21, 4, -2, 0, -2, -4, 3, 8));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 0, 44, 4, 21, 4, -2, 0, -2, 4, 3, 8)));
        }

        {
            // The parrot: the one thing in a jungle that is the right
            // colour for it.
            MobType& t = table[static_cast<size_t>(MobId::Parrot)];
            t.id = MobId::Parrot;
            t.experience = 1;
            t.voice = Sound::MobParrotSay;
            t.hurtVoice = Sound::MobParrotHurt;
            t.deathVoice = Sound::MobParrotDeath;
            t.name = "Parrot";
            t.texture = "parrot";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 6;
            t.width = 0.5f;
            t.height = 0.9f;
            t.walkSpeed = 2.6f;
            t.limbSwing = 0.3f;
            t.voicePitch = 1.6f;
            t.breedingFood = Items::WheatSeeds;
            t.hoverHeight = 3.0f;

            t.drop = Items::Feather;
            t.dropCount = 1;

            t.bodyColour = rgb(206, 48, 44);
            t.headColour = rgb(60, 112, 196);
            t.eyeColour = rgb(30, 28, 26);
            t.pattern = MobType::Pattern::Patches;
            t.markColour = rgb(242, 196, 56);

            t.model.push_back(mc(Part::Body, 0, 0, 4, 7, 3, -2, 13, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 0, 12, 4, 4, 4, -2, 9, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Head, 20, 12, 2, 2, 2, -1, 11, -4, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 0, 22, 1, 5, 4, -3, 13, -2, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 0, 22, 1, 5, 4, 2, 13, -2, 0, 0, 0)));
            t.model.push_back(mc(Part::LegBackRight, 14, 22, 1, 2, 1, -1, 22, 0, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegBackLeft, 14, 22, 1, 2, 1, 0, 22, 0, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 20, 22, 2, 6, 1, -1, 18, 2, 0, 0, 0));
        }

        {
            // The allay: a small blue thing that follows you about.
            // Minecraft's fetches what you show it; this one has no
            // hands for that yet and is simply alive.
            MobType& t = table[static_cast<size_t>(MobId::Allay)];
            t.id = MobId::Allay;
            t.experience = 0;
            t.voice = Sound::MobAllaySay;
            t.hurtVoice = Sound::MobAllayHurt;
            t.deathVoice = Sound::MobAllayDeath;
            t.name = "Allay";
            t.texture = "allay";
            t.sheetHeight = 64;
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 20;
            t.width = 0.35f;
            t.height = 0.6f;
            t.walkSpeed = 3.0f;
            t.limbSwing = 0.2f;
            t.voicePitch = 1.5f;
            t.hoverHeight = 1.8f;

            t.ambientMinSeconds = 5.0f;
            t.ambientMaxSeconds = 14.0f;

            t.bodyColour = rgb(74, 150, 226);
            t.headColour = rgb(140, 206, 246);
            t.eyeColour = rgb(226, 240, 252);
            t.pattern = MobType::Pattern::Belly;
            t.markColour = rgb(206, 240, 252);

            t.model.push_back(mc(Part::Head, 0, 0, 5, 5, 5, -2, 11, -2, 0, 0, 0));
            t.model.push_back(mc(Part::Body, 0, 12, 3, 4, 2, -1, 16, -1, 0, 0, 0));
            t.model.push_back(mc(Part::WingRight, 0, 20, 1, 6, 5, -3, 13, -1, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::WingLeft, 0, 20, 1, 6, 5, 2, 13, -1, 0, 0, 0)));
            t.model.push_back(mc(Part::LegFrontRight, 16, 20, 1, 4, 1, -1, 20, 0, 0, 0, 0));
            t.model.push_back(mirrored(mc(Part::LegFrontLeft, 16, 20, 1, 4, 1, 0, 20, 0, 0, 0, 0)));
        }

        return table;
    }
}

MobId cageSpecies(Dimension where)
{
    switch (where)
    {
        case Dimension::Nether: return MobId::Blaze;
        case Dimension::End:    return MobId::Enderman;
        default:                return MobId::Spider;
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
