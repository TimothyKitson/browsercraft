// Headless checks for the parts of the game you normally only reach by
// clicking: the inventory's slot rules and the crafting recipes.
//
// Run with --selftest. No window, no GL, no world -- just the logic, so
// it can be run after any change in a second.

#include "Player/Inventory.h"
#include "Game/Crafting.h"
#include "Core/KeyBindings.h"
#include "Entity/PlayerAnimation.h"
#include "Entity/PlayerModel.h"
#include "Entity/PlayerSkin.h"
#include "Entity/BoxMesh.h"
#include "Entity/Mob.h"
#include "Entity/MobModel.h"
#include "Entity/MobSkin.h"
#include "Entity/MobType.h"
#include "Entity/EntityManager.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <filesystem>

namespace
{
    int g_checks = 0;
    int g_failures = 0;

    void check(bool condition, const std::string& what)
    {
        ++g_checks;
        if (condition) return;
        ++g_failures;
        std::printf("  FAIL  %s\n", what.c_str());
    }

    void section(const char* name) { std::printf("%s\n", name); }

    // --- picking things up -------------------------------------------

    void testAdd()
    {
        section("inventory: add");

        Inventory inventory;
        check(inventory.add(Blocks::Stone, 10) == 0, "ten stone fit");
        check(inventory.countOf(Blocks::Stone) == 10, "ten stone counted");

        // A second add should top up the existing stack, not start a new one.
        inventory.add(Blocks::Stone, 60);
        check(inventory.countOf(Blocks::Stone) == 70, "stacks top up before spilling");
        check(inventory.slot(0).count == Inventory::MAX_STACK, "first slot filled to 64");
        check(inventory.slot(1).count == 6, "remainder went to the next slot");

        // Armour and the crafting grid are not storage: add() must never
        // reach them, or picking up a block could arm you with it.
        Inventory full;
        const int leftOver = full.add(Blocks::Dirt, Inventory::MAIN_SLOTS * Inventory::MAX_STACK + 64);
        check(leftOver == 64, "add stops at the end of the main inventory");
        for (int i = Inventory::ARMOR_FIRST; i < Inventory::SLOT_COUNT; ++i)
            check(full.slot(i).empty(), "slot " + std::to_string(i) + " untouched by add");
    }

    // --- clicking ------------------------------------------------------

    void testClicks()
    {
        section("inventory: clicks");

        Inventory inventory;
        inventory.slot(0) = ItemStack{ Blocks::Stone, 20 };

        inventory.leftClick(0);
        check(inventory.cursor().count == 20, "left click picks the stack up");
        check(inventory.slot(0).empty(), "slot is emptied");

        inventory.leftClick(5);
        check(inventory.slot(5).count == 20, "left click puts it down");
        check(inventory.cursor().empty(), "cursor is empty after putting down");

        inventory.rightClick(5);
        check(inventory.cursor().count == 10, "right click takes half");
        check(inventory.slot(5).count == 10, "half stays behind");

        // An odd stack rounds in the player's favour.
        Inventory odd;
        odd.slot(0) = ItemStack{ Blocks::Sand, 7 };
        odd.rightClick(0);
        check(odd.cursor().count == 4, "right click rounds the larger half up");
        check(odd.slot(0).count == 3, "the smaller half stays");

        odd.rightClick(1);
        check(odd.slot(1).count == 1, "right click places one");
        check(odd.cursor().count == 3, "cursor loses one");

        // Same block merges; a different block swaps.
        Inventory merge;
        merge.slot(0) = ItemStack{ Blocks::Stone, 60 };
        merge.cursor() = ItemStack{ Blocks::Stone, 10 };
        merge.leftClick(0);
        check(merge.slot(0).count == 64, "merge fills to the stack limit");
        check(merge.cursor().count == 6, "the overflow stays on the cursor");

        Inventory swap;
        swap.slot(0) = ItemStack{ Blocks::Stone, 5 };
        swap.cursor() = ItemStack{ Blocks::Dirt, 3 };
        swap.leftClick(0);
        check(swap.slot(0).id == Blocks::Dirt && swap.slot(0).count == 3, "different blocks swap");
        check(swap.cursor().id == Blocks::Stone && swap.cursor().count == 5, "the old stack is picked up");
    }

    void testQuickMove()
    {
        section("inventory: shift-click");

        Inventory inventory;
        inventory.slot(0) = ItemStack{ Blocks::Stone, 32 };   // hotbar
        inventory.quickMove(0);
        check(inventory.slot(0).empty(), "stack leaves the hotbar");
        check(inventory.countOf(Blocks::Stone) == 32, "and is still in the inventory");
        check(inventory.slot(Inventory::HOTBAR_SLOTS).count == 32, "it lands in storage");

        inventory.quickMove(Inventory::HOTBAR_SLOTS);
        check(inventory.slot(0).count == 32, "and comes back to the hotbar");
    }

    // --- crafting ------------------------------------------------------

    void testCrafting()
    {
        section("crafting");

        Inventory inventory;

        // One log anywhere in the grid makes four planks.
        inventory.slot(Inventory::CRAFT_FIRST) = ItemStack{ Blocks::Log, 1 };
        inventory.refreshCraftResult();
        check(inventory.slot(Inventory::CRAFT_RESULT).id == Blocks::Planks, "log makes planks");
        check(inventory.slot(Inventory::CRAFT_RESULT).count == 4, "four of them");

        check(inventory.takeCraftResult(), "the result can be taken");
        check(inventory.cursor().count == 4, "four planks on the cursor");
        check(inventory.slot(Inventory::CRAFT_FIRST).empty(), "the log was spent");
        check(inventory.slot(Inventory::CRAFT_RESULT).empty(), "and the result cleared");

        // Birch too.
        Inventory birch;
        birch.slot(Inventory::CRAFT_FIRST + 1) = ItemStack{ Blocks::BirchLog, 1 };
        birch.refreshCraftResult();
        check(birch.slot(Inventory::CRAFT_RESULT).id == Blocks::Planks, "birch log makes planks");

        // Four sand in a square makes sandstone. The 2x2 grid is the top
        // left of a 3x3 array, so this is cells 0, 1, 3 and 4.
        Inventory sand;
        sand.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Blocks::Sand, 1 };
        sand.slot(Inventory::CRAFT_FIRST + 1) = ItemStack{ Blocks::Sand, 1 };
        sand.slot(Inventory::CRAFT_FIRST + 3) = ItemStack{ Blocks::Sand, 1 };
        sand.slot(Inventory::CRAFT_FIRST + 4) = ItemStack{ Blocks::Sand, 1 };
        sand.refreshCraftResult();
        check(sand.slot(Inventory::CRAFT_RESULT).id == Blocks::Sandstone, "four sand makes sandstone");
        check(sand.slot(Inventory::CRAFT_RESULT).count == 1, "one of it");

        // Three sand is not a recipe.
        sand.slot(Inventory::CRAFT_FIRST + 4).clear();
        sand.refreshCraftResult();
        check(sand.slot(Inventory::CRAFT_RESULT).empty(), "three sand makes nothing");

        // Nonsense in the grid makes nothing.
        Inventory junk;
        junk.slot(Inventory::CRAFT_FIRST) = ItemStack{ Blocks::Obsidian, 1 };
        junk.refreshCraftResult();
        check(junk.slot(Inventory::CRAFT_RESULT).empty(), "obsidian makes nothing");

        // Shift-clicking the result crafts until the ingredients run out.
        Inventory bulk;
        bulk.slot(Inventory::CRAFT_FIRST) = ItemStack{ Blocks::Log, 5 };
        bulk.refreshCraftResult();
        bulk.quickMove(Inventory::CRAFT_RESULT);
        check(bulk.countOf(Blocks::Planks) == 20, "five logs become twenty planks");
        check(bulk.slot(Inventory::CRAFT_FIRST).empty(), "every log was spent");

        // Closing the screen must hand the grid back, not eat it.
        Inventory closing;
        closing.slot(Inventory::CRAFT_FIRST) = ItemStack{ Blocks::Dirt, 7 };
        std::vector<ItemStack> spilled;
        closing.clearCraftGrid(spilled);
        check(spilled.empty(), "nothing spilled when there was room");
        check(closing.countOf(Blocks::Dirt) == 7, "the grid came back to the inventory");
        check(closing.slot(Inventory::CRAFT_FIRST).empty(), "and the grid is empty");
    }
    // --- key bindings --------------------------------------------------

    void testKeyBindings()
    {
        section("key bindings");

        KeyBindings keys;

        // The defaults are Minecraft's, which is the whole point of them.
        check(keys.binding(Action::Forward) == Binding{ false, SDL_SCANCODE_W }, "forward is W");
        check(keys.binding(Action::Sneak) == Binding{ false, SDL_SCANCODE_LSHIFT }, "sneak is left shift");
        check(keys.binding(Action::Sprint) == Binding{ false, SDL_SCANCODE_LCTRL }, "sprint is left ctrl");
        check(keys.binding(Action::Drop) == Binding{ false, SDL_SCANCODE_Q }, "drop is Q");
        check(keys.binding(Action::Inventory) == Binding{ false, SDL_SCANCODE_E }, "inventory is E");
        check(keys.binding(Action::Attack) == Binding{ true, SDL_BUTTON_LEFT }, "attack is left click");
        check(keys.binding(Action::Use) == Binding{ true, SDL_BUTTON_RIGHT }, "use is right click");
        check(keys.binding(Action::HideHud) == Binding{ false, SDL_SCANCODE_F1 }, "hide hud is F1");
        check(keys.binding(Action::Fullscreen) == Binding{ false, SDL_SCANCODE_F11 }, "fullscreen is F11");

        // Nothing should clash out of the box.
        bool anyClash = false;
        for (int i = 0; i < static_cast<int>(Action::Count); ++i)
            if (keys.conflicts(static_cast<Action>(i))) anyClash = true;
        check(!anyClash, "no two defaults share a key");

        // Binding two things to one key is allowed, but reported.
        keys.setBinding(Action::Jump, Binding{ false, SDL_SCANCODE_W });
        check(keys.conflicts(Action::Jump), "a duplicate is reported as a clash");
        check(keys.conflicts(Action::Forward), "and so is the one it collided with");

        check(KeyBindings::describe(Binding{ false, SDL_SCANCODE_F3 }) == "F3", "F3 reads as F3");
        check(KeyBindings::describe(Binding{ true, SDL_BUTTON_MIDDLE }) == "MIDDLE CLICK",
              "the middle button has a name");
        check(KeyBindings::describe(Binding{ false, 0 }) == "NONE", "an empty binding reads as NONE");

        // Round-trip through disk, which is what makes rebinding stick.
        const std::string dir = (std::filesystem::temp_directory_path() / "bc_selftest").string();
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        keys.setBinding(Action::Forward, Binding{ false, SDL_SCANCODE_UP });
        keys.setBinding(Action::Attack, Binding{ true, SDL_BUTTON_RIGHT });
        keys.setBinding(Action::Mute, Binding{ false, 0 });
        keys.save(dir);

        KeyBindings loaded;
        loaded.load(dir);
        check(loaded.binding(Action::Forward) == Binding{ false, SDL_SCANCODE_UP }, "a rebound key survives a save");
        check(loaded.binding(Action::Attack) == Binding{ true, SDL_BUTTON_RIGHT }, "so does a mouse binding");
        check(!loaded.binding(Action::Mute).valid(), "and so does an unbound control");
        check(loaded.binding(Action::Inventory) == Binding{ false, SDL_SCANCODE_E },
              "untouched controls keep their default");

        // Saving into a folder that does not exist yet must still work:
        // the controls screen is reachable from the title, before any
        // world has been created, and this silently did nothing.
        const std::string fresh_dir = (std::filesystem::temp_directory_path() /
                                       "bc_selftest_new" / "world").string();
        std::filesystem::remove_all((std::filesystem::temp_directory_path() /
                                     "bc_selftest_new").string(), ec);
        KeyBindings unsaved;
        unsaved.setBinding(Action::Jump, Binding{ false, SDL_SCANCODE_RETURN });
        unsaved.save(fresh_dir);
        KeyBindings reloaded;
        reloaded.load(fresh_dir);
        check(reloaded.binding(Action::Jump) == Binding{ false, SDL_SCANCODE_RETURN },
              "bindings save into a folder that did not exist");
        std::filesystem::remove_all((std::filesystem::temp_directory_path() /
                                     "bc_selftest_new").string(), ec);

        // A missing file must leave the defaults alone, not wipe them.
        KeyBindings fresh;
        fresh.load((std::filesystem::temp_directory_path() / "bc_selftest_missing").string());
        check(fresh.binding(Action::Forward) == Binding{ false, SDL_SCANCODE_W },
              "no file means defaults");

        std::filesystem::remove_all(dir, ec);
    }

    // --- how a player model moves --------------------------------------

    void testPlayerAnimation()
    {
        section("players: animation");

        using namespace PlayerAnimation;

        auto about = [](float a, float b) { return std::fabs(a - b) < 0.001f; };

        check(about(wrapDegrees(0.0f), 0.0f), "zero stays zero");
        check(about(wrapDegrees(370.0f), 10.0f), "a lap and a bit folds down");
        check(about(wrapDegrees(-190.0f), 170.0f), "turning left past half a lap comes back right");
        check(about(wrapDegrees(540.0f), -180.0f), "one and a half laps lands on the boundary");

        // The arms swing opposite the legs on the same side, which is
        // what makes a walk read as a walk rather than a shuffle.
        check(legAngle(0.0f, 1.0f) > 0.0f && armAngle(0.0f, 1.0f) < 0.0f,
              "the right arm goes back as the right leg comes forward");
        check(about(legAngle(0.0f, 0.0f), 0.0f), "standing still does not swing");
        check(std::fabs(legAngle(1.0f, 0.5f)) < std::fabs(legAngle(1.0f, 1.0f)),
              "walking swings less than running");

        check(about(swingAmountFor(0.0f), 0.0f), "no speed, no swing");
        check(about(swingAmountFor(WALK_SPEED), 1.0f), "walking pace is a full swing");
        check(about(swingAmountFor(WALK_SPEED * 3.0f), 1.0f), "sprinting does not swing further");

        // The body comes round to meet the head, but never instantly,
        // and never leaves the neck twisted further than it can go.
        const float turned = followHead(0.0f, 90.0f, 1.0f / 60.0f, false);
        check(turned > 0.0f && turned < 90.0f, "the body trails the head round");
        check(std::fabs(wrapDegrees(90.0f - turned)) <= MAX_NECK_TWIST + 0.001f,
              "and never lets the neck twist too far");

        // Standing still and staring over your shoulder: the body should
        // settle exactly at the limit rather than creep past it.
        float body = 0.0f;
        for (int i = 0; i < 600; ++i) body = followHead(body, 120.0f, 1.0f / 60.0f, false);
        check(std::fabs(wrapDegrees(120.0f - body)) <= MAX_NECK_TWIST + 0.001f,
              "a long stare settles inside the limit");

        // Crossing the -180/180 seam must not send the body the long way
        // round: 170 to -170 is twenty degrees, not three hundred and forty.
        const float across = followHead(170.0f, -170.0f, 1.0f, true);
        check(std::fabs(wrapDegrees(across - 170.0f)) < 25.0f,
              "turning across the seam takes the short way");

        check(about(approach(0.0f, 1.0f, 10.0f, 1.0f), 1.0f), "a big step arrives");
        check(approach(0.0f, 1.0f, 10.0f, 1000.0f) <= 1.0f, "and never overshoots");
    }

    // --- wrapping a skin round the model -------------------------------

    void testPlayerModel()
    {
        section("players: skin layout");

        // The flat paper doll on the profile screen has been looked at;
        // the 3D model has to agree with it, or a player would be wearing
        // their own face on the back of their head. PlayerSkin names the
        // front of each body part, so compare against that.
        PlayerSkin skin;   // default: classic arms, the 64x32 layout

        struct Part { const char* name; int u, v, w, h, d; int frontX, frontY; };
        const Part PARTS[] = {
            { "head",  0, 0, 8, 8, 8,   8, 8 },
            { "body", 16, 16, 8, 12, 4, 20, 20 },
            { "arm",  40, 16, 4, 12, 4, 44, 20 },
            { "leg",   0, 16, 4, 12, 4,  4, 20 },
        };

        for (const Part& part : PARTS)
        {
            int x = 0, y = 0, w = 0, h = 0;
            PlayerModel::facePatch(PlayerModel::Front, part.u, part.v,
                                   part.w, part.h, part.d, x, y, w, h);
            check(x == part.frontX && y == part.frontY,
                  std::string(part.name) + " front sits where the doll draws it");
            check(w == part.w && h == part.h, std::string(part.name) + " front is the right size");

            // The six faces have to tile the part's own rectangle of skin
            // exactly: 2(wh + wd + hd) pixels, no overlap, nothing spilling
            // past the right-hand edge of the strip.
            int covered = 0;
            int right = part.u;
            for (int face = 0; face < 6; ++face)
            {
                PlayerModel::facePatch(face, part.u, part.v, part.w, part.h, part.d, x, y, w, h);
                covered += w * h;
                right = std::max(right, x + w);
                check(x >= part.u && y >= part.v, std::string(part.name) + " face stays in its strip");
            }
            check(covered == 2 * (part.w * part.h + part.w * part.d + part.h * part.d),
                  std::string(part.name) + " faces cover the part exactly once");
            check(right == part.u + 2 * (part.w + part.d),
                  std::string(part.name) + " strip is as wide as the part unrolled");
        }

        // The side faces are as deep as the part and the top is as deep
        // again, which is what puts the front patch below the top one.
        int x = 0, y = 0, w = 0, h = 0;
        PlayerModel::facePatch(PlayerModel::Top, 0, 0, 8, 8, 8, x, y, w, h);
        check(x == 8 && y == 0 && w == 8 && h == 8, "the top of the head is where the hair is");
        PlayerModel::facePatch(PlayerModel::Back, 0, 0, 8, 8, 8, x, y, w, h);
        check(x == 24 && y == 8, "the back of the head is the last patch along");

        // A slim skin narrows the arm and nothing else.
        check(skin.armWidth() == 4, "classic arms are four wide");
        PlayerModel::facePatch(PlayerModel::Front, 40, 16, 3, 12, 4, x, y, w, h);
        check(w == 3 && x == 44, "a slim arm is three wide in the same place");
    }

    // --- the figure itself ---------------------------------------------

    void testPlayerGeometry()
    {
        section("players: geometry");

        PlayerSkin skin;        // no GL: only the measurements are used here
        PlayerModel model;

        PlayerPose pose;
        pose.feet = glm::vec3(10.0f, 64.0f, -5.0f);
        model.build(skin, pose, 1.0f, 0.0f);

        const std::vector<float>& v = model.vertices();
        const int stride = PlayerModel::FLOATS_PER_VERTEX;

        // Head, body, two arms, two legs and the hat, each six faces of
        // two triangles.
        check(static_cast<int>(v.size()) == 7 * 36 * stride, "seven boxes of thirty-six vertices");

        glm::vec3 low(1e9f), high(-1e9f);
        bool uvInside = true;
        bool faceMarked = true;

        for (size_t i = 0; i < v.size(); i += stride)
        {
            const glm::vec3 p(v[i], v[i + 1], v[i + 2]);
            low = glm::min(low, p);
            high = glm::max(high, p);
            if (v[i + 3] < 0.0f || v[i + 3] > 1.0f || v[i + 4] < 0.0f || v[i + 4] > 1.0f)
                uvInside = false;
            // Face 6 is what tells the chunk shader this is not a cube
            // face with a normal map behind it.
            if (v[i + 8] != 6.0f) faceMarked = false;
        }

        check(uvInside, "every corner samples inside the skin");
        check(faceMarked, "every vertex is marked as needing no normal map");

        auto about = [](float a, float b) { return std::fabs(a - b) < 0.002f; };

        // Feet on the ground, and two blocks tall -- the hat layer sits
        // half a skin pixel proud of the top of the head.
        check(about(low.y, pose.feet.y), "the feet are on the ground");
        check(about(high.y - low.y, PlayerModel::HEIGHT + 0.5f / 16.0f),
              "the figure is two blocks tall");

        // Facing. Yaw 0 looks down +x, the same convention as the
        // camera, so the model must be deepest along x and widest along
        // z: shoulders across, nose forward. Getting this backwards
        // would put everyone's face on the back of their head.
        check(about(high.x - pose.feet.x, 4.5f / 16.0f), "the face points the way they are looking");
        check(about(high.z - pose.feet.z, 8.0f / 16.0f), "and the shoulders go across it");

        // Turn a quarter circle and the two swap over.
        pose.bodyYaw = 90.0f;
        pose.headYaw = 90.0f;
        model.build(skin, pose, 1.0f, 0.0f);

        glm::vec3 turnedLow(1e9f), turnedHigh(-1e9f);
        for (size_t i = 0; i < model.vertices().size(); i += stride)
        {
            const glm::vec3 p(model.vertices()[i], model.vertices()[i + 1], model.vertices()[i + 2]);
            turnedLow = glm::min(turnedLow, p);
            turnedHigh = glm::max(turnedHigh, p);
        }
        check(about(turnedHigh.z - pose.feet.z, 4.5f / 16.0f), "turning a quarter turn turns the face");
        check(about(turnedHigh.x - pose.feet.x, 8.0f / 16.0f), "and the shoulders with it");

        // Standing still, the limbs hang straight: the lowest point of
        // the model is the soles of the feet and nothing swings past them.
        check(about(turnedLow.y, pose.feet.y), "standing still, nothing swings below the feet");

        // Walking, they do not. Phase zero is the top of the cycle, with
        // one leg as far forward as it goes and the other as far back.
        PlayerPose walking = pose;
        walking.limbAmount = 1.0f;
        walking.limbPhase = 0.0f;
        model.build(skin, walking, 1.0f, 0.0f);
        float walkLow = 1e9f, walkFront = -1e9f, walkBack = 1e9f;
        for (size_t i = 0; i < model.vertices().size(); i += stride)
        {
            walkLow = std::min(walkLow, model.vertices()[i + 1]);
            walkFront = std::max(walkFront, model.vertices()[i + 2]);
            walkBack = std::min(walkBack, model.vertices()[i + 2]);
        }
        check(walkLow > pose.feet.y, "at full stride the feet have left the ground");
        check(walkFront > turnedHigh.z, "one leg has swung out in front");
        check(walkBack < turnedLow.z, "and the other out behind");
    }

    // --- mobs -----------------------------------------------------------

    void testMobTypes()
    {
        section("mobs: species");

        check(mobTypeCount() == 8, "eight species");

        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobId id = static_cast<MobId>(i);
            const MobType& type = mobType(id);
            const std::string who = type.name;

            check(type.name && *type.name, "species " + std::to_string(i) + " is named");
            check(type.maxHealth > 0, who + " can be killed");
            check(type.width > 0.0f && type.height > 0.0f, who + " has a collision box");
            check(type.walkSpeed > 0.0f, who + " can walk");
            check(!type.model.empty(), who + " has a model");

            // A model that does not match its collision box leaves the
            // mob either floating or sunk into the floor.
            check(std::fabs(modelTop(type) - type.height) < 0.07f,
                  who + " is drawn the height it collides at");

            // Feet on the ground: something has to touch y = 0.
            int lowest = 1000;
            for (const MobBox& b : type.model) lowest = std::min(lowest, b.origin.y);
            check(lowest == 0, who + " stands on the ground");

            // Every species must fit its sheet, or its texture coordinates
            // silently wrap onto another limb.
            std::vector<MobBox> boxes = type.model;
            check(MobSkin::pack(boxes, MobSkin::SHEET), who + " packs onto one sheet");

            for (const MobBox& b : boxes)
            {
                const int w = 2 * (b.size.x + b.size.z);
                const int h = b.size.y + b.size.z;
                check(b.u >= 0 && b.v >= 0 && b.u + w <= MobSkin::SHEET && b.v + h <= MobSkin::SHEET,
                      who + " keeps every patch on the sheet");
            }
        }
    }

    void testMobSpawnRules()
    {
        section("mobs: spawning");

        // Monsters in the dark, animals in the light, and neither in mid-air.
        check(EntityManager::canSpawnOn(MobId::Zombie, Blocks::Stone, 0), "zombies spawn in the dark");
        check(!EntityManager::canSpawnOn(MobId::Zombie, Blocks::Stone, 12), "and not in daylight");
        check(EntityManager::canSpawnOn(MobId::Sheep, Blocks::Grass, 14), "sheep spawn on lit grass");
        check(!EntityManager::canSpawnOn(MobId::Sheep, Blocks::Grass, 2), "and not in the dark");
        check(!EntityManager::canSpawnOn(MobId::Sheep, Blocks::Stone, 14), "nor on bare stone");
        check(!EntityManager::canSpawnOn(MobId::Zombie, Blocks::Air, 0), "nothing spawns in mid-air");
    }

    void testMobGeometry()
    {
        section("mobs: geometry");

        MobSkin skin;
        skin.layout(MobId::Cow);        // layout only: no GL in a headless run
        check(!skin.boxes().empty(), "the cow laid out");

        Mob cow(MobId::Cow, glm::vec3(4.0f, 70.0f, -2.0f), 12345u);
        MobModel model;
        model.build(skin, cow, 1.0f, 0.0f);

        const std::vector<float>& v = model.vertices();
        const int stride = BoxMesh::FLOATS_PER_VERTEX;
        check(!v.empty(), "and built some geometry");
        check(static_cast<int>(v.size()) ==
                  static_cast<int>(skin.boxes().size()) * 36 * stride,
              "six faces of two triangles per box");

        float lowY = 1e9f, highY = -1e9f;
        bool uvInside = true;
        for (size_t i = 0; i < v.size(); i += stride)
        {
            lowY = std::min(lowY, v[i + 1]);
            highY = std::max(highY, v[i + 1]);
            if (v[i + 3] < 0.0f || v[i + 3] > 1.0f || v[i + 4] < 0.0f || v[i + 4] > 1.0f)
                uvInside = false;
        }

        check(uvInside, "every corner samples inside the hide");
        check(std::fabs(lowY - 70.0f) < 0.002f, "the cow's feet are on the ground");
        check(std::fabs((highY - lowY) - mobType(MobId::Cow).height) < 0.02f,
              "and it stands its full height");
    }

    void testMobDamage()
    {
        section("mobs: damage");

        Mob pig(MobId::Pig, glm::vec3(0.0f), 7u);
        const int full = mobType(MobId::Pig).maxHealth;
        check(pig.health() == full, "a pig starts at full health");
        check(pig.alive(), "and alive");
        check(pig.sounds().empty(), "and quiet");

        pig.damage(3, glm::vec3(1.0f, 0.0f, 0.0f));
        check(pig.health() == full - 3, "a hit takes health off");
        check(pig.hurtFlash() > 0.0f, "and makes it flash");
        check(pig.sounds().size() == 1 && pig.sounds()[0].id == Sound::MobHurt,
              "and makes it squeal");
        pig.sounds().clear();

        pig.damage(1000, glm::vec3(1.0f, 0.0f, 0.0f));
        check(pig.health() == 0, "enough damage kills it");
        check(!pig.alive(), "and it stops being alive");
        check(pig.sounds().size() == 1 && pig.sounds()[0].id == Sound::MobDeath,
              "with a death noise, not another squeal");
        pig.sounds().clear();

        // A corpse lingers a moment before it is dropped, so the death is
        // visible rather than an instant disappearance.
        check(!pig.finished(), "a fresh corpse is not finished with");
        check(pig.deathFade() > 0.0f, "and has not faded yet");

        pig.damage(5, glm::vec3(1.0f, 0.0f, 0.0f));
        check(pig.sounds().empty(), "hitting a corpse does nothing");

        // Species keep their own voices.
        check(mobType(MobId::Creeper).voice == Sound::MobHiss, "creepers hiss");
        check(mobType(MobId::Chicken).voice == Sound::MobCluck, "chickens cluck");
        check(mobType(MobId::Cow).voicePitch < mobType(MobId::Pig).voicePitch,
              "a cow is lower than a pig");
    }
}

int runSelfTest()
{
    std::printf("\n=== self test ===\n");
    testAdd();
    testClicks();
    testQuickMove();
    testCrafting();
    testKeyBindings();
    testPlayerAnimation();
    testPlayerModel();
    testPlayerGeometry();
    testMobTypes();
    testMobSpawnRules();
    testMobGeometry();
    testMobDamage();

    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
