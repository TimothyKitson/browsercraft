// Headless checks for the parts of the game you normally only reach by
// clicking: the inventory's slot rules and the crafting recipes.
//
// Run with --selftest. No window, no GL, no world -- just the logic, so
// it can be run after any change in a second.

#include "Player/Inventory.h"
#include "Game/Crafting.h"
#include "Game/Items.h"
#include "Game/Farming.h"
#include "Core/KeyBindings.h"
#include "Entity/PlayerAnimation.h"
#include "Entity/PlayerModel.h"
#include "Entity/PlayerSkin.h"
#include "Entity/BoxMesh.h"
#include "Entity/Mob.h"
#include "World/WorldSave.h"
#include "Game/Tools.h"
#include "Game/Smelting.h"
#include "Game/Armour.h"
#include "Game/Sleep.h"
#include "Game/Explosion.h"
#include "Game/Portal.h"
#include "World/Furnaces.h"
#include "World/Chests.h"
#include "Entity/Projectiles.h"
#include "Entity/MobModel.h"
#include "Entity/MobSkin.h"
#include "Entity/MobType.h"
#include "Entity/EntityManager.h"
#include "Entity/Pathfinder.h"
#include "Game/Chat.h"
#include "Game/Food.h"
#include "Renderer/Atlas.h"
#include "Player/Player.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
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

            float bottom = 0.0f, top = 0.0f;
            modelBounds(type, bottom, top);

            // A model that does not match its collision box leaves the
            // mob either floating or sunk into the floor. Minecraft's own
            // heads and horns poke out of their hitboxes, so this asks
            // only that the drawing and the box are the same animal.
            check(top >= type.height * 0.6f && top <= type.height + 0.3f,
                  who + " is drawn about the height it collides at");

            // Feet on the ground, within a pixel. A spider's legs splay
            // out and end just shy of the floor, which is vanilla too.
            check(bottom > -0.2f && bottom < 0.05f, who + " stands on the ground");

            // Every species must fit its sheet, or its texture coordinates
            // silently wrap onto another limb.
            check(type.sheetWidth == 64 && (type.sheetHeight == 32 || type.sheetHeight == 64),
                  who + " has a sheet the shape Minecraft draws on");
            check(MobSkin::fits(type.model, type.sheetWidth, type.sheetHeight),
                  who + " keeps every patch on its sheet");
        }
    }

    void testSunlightBurning()
    {
        section("mobs: daylight");

        const MobType& zombie = mobType(MobId::Zombie);
        const MobType& skeleton = mobType(MobId::Skeleton);
        const MobType& creeper = mobType(MobId::Creeper);
        const MobType& spider = mobType(MobId::Spider);
        const MobType& cow = mobType(MobId::Cow);

        check(Mob::burnsNow(zombie, 1.0f, 15, false), "a zombie caught in the open burns");
        check(Mob::burnsNow(skeleton, 1.0f, 15, false), "and so does a skeleton");
        check(!Mob::burnsNow(creeper, 1.0f, 15, false), "a creeper does not");
        check(!Mob::burnsNow(spider, 1.0f, 15, false), "nor a spider");
        check(!Mob::burnsNow(cow, 1.0f, 15, false), "and certainly not a cow");

        check(!Mob::burnsNow(zombie, 0.0f, 15, false), "nothing burns at night");
        check(!Mob::burnsNow(zombie, 0.2f, 15, false), "nor at dawn, before the sun is up");
        check(!Mob::burnsNow(zombie, 1.0f, 14, false), "a roof over its head is enough");
        check(!Mob::burnsNow(zombie, 1.0f, 0, false), "and a cave certainly is");
        check(!Mob::burnsNow(zombie, 1.0f, 15, true), "standing in water puts it out");
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

    void testBoxWinding()
    {
        section("entities: box winding");

        // Model space -- right, up, the way it faces -- is left-handed,
        // so a box wound the obvious way comes out inside-out once it is
        // placed in the world and the graphics card throws away the
        // faces you can see instead of the ones you cannot. One figure
        // on its own still looks right, which is how this went unnoticed
        // until a sheep needed its wool drawn over its hide.
        BoxMesh::Box box;
        box.min = glm::vec3(-4.0f, 0.0f, -4.0f);
        box.max = glm::vec3(4.0f, 8.0f, 4.0f);

        BoxMesh::Frame frame;
        frame.feet = glm::vec3(0.0f);
        frame.bodyYaw = 0.0f;

        std::vector<float> v;
        BoxMesh::append(v, box, frame);

        const int stride = BoxMesh::FLOATS_PER_VERTEX;
        const glm::vec3 centre(0.0f, 0.25f, 0.0f);   // the box's middle, in blocks

        bool allOutward = true;
        for (size_t i = 0; i + 2 * stride < v.size(); i += 3 * stride)
        {
            const glm::vec3 a(v[i], v[i + 1], v[i + 2]);
            const glm::vec3 b(v[i + stride], v[i + stride + 1], v[i + stride + 2]);
            const glm::vec3 c(v[i + 2 * stride], v[i + 2 * stride + 1], v[i + 2 * stride + 2]);

            // Counter-clockwise seen from outside means the cross product
            // points away from the middle of the box.
            const glm::vec3 facing = glm::cross(b - a, c - a);
            if (glm::dot(facing, (a + b + c) / 3.0f - centre) <= 0.0f) allOutward = false;
        }

        check(v.size() == 36 * static_cast<size_t>(stride), "a box is twelve triangles");
        check(allOutward, "every one of them faces out of the box");
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

        // What the table says the model measures has to be what the
        // renderer actually draws, rotated boxes and all.
        float bottom = 0.0f, top = 0.0f;
        modelBounds(mobType(MobId::Cow), bottom, top);
        check(std::fabs(lowY - (70.0f + bottom)) < 0.002f, "the cow's feet are on the ground");
        check(std::fabs(highY - (70.0f + top)) < 0.002f,
              "and it is drawn the height the model claims");

        // Mojang's numbers, turned round to ours: a zombie's head is the
        // top eight pixels of a two-block figure, and its right arm
        // hangs off the right-hand side of its chest. If the conversion
        // in MobType ever drifts, every mob wears its texture crooked.
        const MobType& zombie = mobType(MobId::Zombie);
        const MobBox& head = zombie.model[0];
        check(head.u == 0 && head.v == 0, "the zombie's head reads from the top-left of its sheet");
        check(head.origin == glm::ivec3(-4, 24, -4) && head.size == glm::ivec3(8, 8, 8),
              "and sits on top of a two-block figure");

        const MobBox& rightArm = zombie.model[2];
        check(rightArm.u == 40 && rightArm.v == 16, "its right arm reads from Mojang's arm patch");
        check(rightArm.origin.x == 4 && !rightArm.mirror, "and hangs on its right, unmirrored");
        check(zombie.model[3].mirror, "while the left arm is the same patch, mirrored");
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
        check(pig.sounds().size() == 1 && pig.sounds()[0].id == Sound::MobPigHurt,
              "and makes it squeal");
        pig.sounds().clear();

        pig.damage(1000, glm::vec3(1.0f, 0.0f, 0.0f));
        check(pig.health() == 0, "enough damage kills it");
        check(!pig.alive(), "and it stops being alive");
        check(pig.sounds().size() == 1 && pig.sounds()[0].id == Sound::MobPigDeath,
              "with a death noise, not another squeal");
        pig.sounds().clear();

        // A corpse lingers a moment before it is dropped, so the death is
        // visible rather than an instant disappearance.
        check(!pig.finished(), "a fresh corpse is not finished with");
        check(pig.deathFade() > 0.0f, "and has not faded yet");

        pig.damage(5, glm::vec3(1.0f, 0.0f, 0.0f));
        check(pig.sounds().empty(), "hitting a corpse does nothing");

        // Species keep their own voices.
        check(mobType(MobId::Creeper).voice == Sound::MobCreeperSay, "a creeper has its own voice");
        check(mobType(MobId::Chicken).voice == Sound::MobChickenSay, "and so does a chicken");

        // No two species may share a cry, or a dying sheep sounds like a
        // dying zombie the moment real audio is installed.
        bool distinct = true;
        for (int a = 0; a < mobTypeCount(); ++a)
            for (int b = a + 1; b < mobTypeCount(); ++b)
            {
                const MobType& x = mobType(static_cast<MobId>(a));
                const MobType& y = mobType(static_cast<MobId>(b));
                if (x.voice == y.voice || x.hurtVoice == y.hurtVoice ||
                    x.deathVoice == y.deathVoice) distinct = false;
            }
        check(distinct, "every species has its own say, hurt and death");
        check(mobType(MobId::Cow).voicePitch < mobType(MobId::Pig).voicePitch,
              "a cow is lower than a pig");
    }

    // --- combat ---------------------------------------------------------

    void testMobCombat()
    {
        section("mobs: combat");

        // Only the things that should fight back do.
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobType& t = mobType(static_cast<MobId>(i));
            const std::string who = t.name;
            // A hostile mob has to be able to hurt you somehow -- with
            // its hands, with a bow, or by going off. Checking only for
            // hands was what this said before skeletons could shoot, and
            // it has now caught two mobs whose way of fighting changed.
            const bool fights = t.attackDamage > 0 || t.arrowDamage > 0 || t.blastDamage > 0;

            if (t.spawnClass == SpawnClass::Hostile)
                check(fights, who + " fights back somehow");
            else
                check(!fights, who + " does not attack");

            if (t.attackDamage > 0)
                check(t.attackInterval > 0.0f, who + " cannot swing infinitely fast");

            if (t.arrowDamage > 0)
            {
                check(t.arrowInterval > 0.0f, who + " cannot shoot infinitely fast");
                check(t.arrowRange > 0.0f, who + " shoots some distance");
                check(t.attackDamage == 0,
                      who + " either shoots or swings, not both");
            }

            if (t.blastDamage > 0)
            {
                check(t.fuseSeconds > 0.0f, who + " takes a moment to go off");
                check(t.blastRadius > 0.0f, who + " goes off over some distance");
                check(t.attackDamage == 0, who + " does not also swing");

                // The fuse has to be long enough to run from. A creeper
                // that went off the instant it touched you would be an
                // unavoidable loss of health rather than a fright.
                check(t.fuseSeconds > 0.8f, who + " can be run away from");
            }
            check(t.dropCount == 0 || t.drop != Blocks::Air,
                  who + " does not drop nothing repeatedly");
        }

        // A death is reported exactly once, on the frame it happens --
        // the corpse lingers afterwards, so anything waiting for removal
        // would drop the loot in the wrong place or not at all.
        Mob sheep(MobId::Sheep, glm::vec3(3.0f, 64.0f, 3.0f), 99u);
        check(!sheep.takeDeathReport(), "a living sheep reports no death");
        sheep.damage(1000, glm::vec3(0.0f, 0.0f, 1.0f));
        check(sheep.takeDeathReport(), "a dead one reports once");
        check(!sheep.takeDeathReport(), "and only once");
        check(mobType(MobId::Sheep).drop == Items::RawMutton, "a sheep leaves mutton");
        check(mobType(MobId::Sheep).secondDrop == Blocks::Wool, "and its wool");

        // Knockback pushes away from whatever hit it, and upward.
        Mob pig(MobId::Pig, glm::vec3(0.0f, 64.0f, 0.0f), 5u);
        const glm::vec3 before = pig.position();
        pig.damage(1, glm::vec3(1.0f, 0.0f, 0.0f));
        check(pig.position() == before, "a hit does not teleport anything");
        check(pig.hurtFlash() > 0.0f, "but it does flash");

        // Who the swing lands on. No world needed: this is a ray against
        // the mobs' own boxes, and it is the only part of hitting
        // something that a screenshot cannot show.
        EntityManager herd;
        herd.mobs().emplace_back(MobId::Cow, glm::vec3(0.0f, 64.0f, 5.0f), 1u);
        herd.mobs().emplace_back(MobId::Cow, glm::vec3(0.0f, 64.0f, 12.0f), 2u);

        // Eye height is 1.62 and a cow is 1.375 tall, so looking dead
        // level really does pass over its back. Aim slightly down, the
        // way you would at an animal.
        const glm::vec3 eye(0.0f, 65.6f, 0.0f);
        const glm::vec3 ahead = glm::normalize(glm::vec3(0.0f, -0.1f, 1.0f));

        check(herd.pick(eye, glm::vec3(0.0f, 0.0f, 1.0f), 20.0f) == nullptr,
              "looking level goes over a cow's back");

        Mob* nearest = herd.pick(eye, ahead, 20.0f);
        check(nearest != nullptr, "a cow straight ahead is picked");
        if (nearest) check(nearest->position().z == 5.0f, "and it is the nearer of the two");

        check(herd.pick(eye, ahead, 3.0f) == nullptr, "one out of reach is not");
        check(herd.pick(eye, glm::vec3(0.0f, 0.0f, -1.0f), 20.0f) == nullptr,
              "nor one behind you");
        check(herd.pick(eye, glm::vec3(1.0f, 0.0f, 0.0f), 20.0f) == nullptr,
              "nor one off to the side");

        // A corpse is not a target: you cannot keep hitting it.
        herd.mobs()[0].damage(1000, ahead);
        Mob* living = herd.pick(eye, ahead, 20.0f);
        check(living != nullptr && living->position().z == 12.0f,
              "a dead one is skipped for the live one behind it");
    }

    // --- items ----------------------------------------------------------

    void testItems()
    {
        section("items");

        // The whole design rests on the two id ranges never meeting. A
        // block id is a byte, and the items begin one past the largest
        // value a byte can hold.
        check(Items::FIRST == 256, "items start past every possible block id");
        check(static_cast<int>(Blocks::Count) < Items::FIRST, "and no block reaches them");
        check(!isItem(Blocks::Air) && !isItem(Blocks::Stone), "blocks are not items");
        check(isBlockStack(Blocks::Stone), "and blocks read as blocks");
        check(isItem(Items::Wheat) && isItem(Items::Gunpowder), "items are items");
        check(!isBlockStack(Items::Wheat), "and items are not blocks");
        check(!isItem(Items::Count), "nothing past the last item is one");

        // Narrowing an item to a BlockId is exactly the mistake this
        // range is meant to make impossible to miss.
        check(static_cast<BlockId>(Items::Wheat) != Items::Wheat,
              "an item does not survive being squeezed into a block id");

        for (StackId id = Items::FIRST; id < Items::Count; ++id)
        {
            const ItemInfo& info = itemInfo(id);
            const std::string what = info.name;
            check(info.name && *info.name, "item " + std::to_string(id) + " is named");
            check(info.maxStack > 0 && info.maxStack <= Inventory::MAX_STACK,
                  what + " stacks to something sensible");
            check(info.tile >= Tiles::ItemFirst && info.tile < Tiles::ItemTileEnd,
                  what + " has a tile of its own");
        }

        // Every item needs its own sprite, or two of them are the same
        // picture in the hotbar.
        for (StackId a = Items::FIRST; a < Items::Count; ++a)
            for (StackId b = a + 1; b < Items::Count; ++b)
                check(itemInfo(a).tile != itemInfo(b).tile,
                      std::string(itemInfo(a).name) + " and " + itemInfo(b).name +
                          " do not share a sprite");

        // The three lookups that let the inventory stay ignorant of which
        // it is holding.
        check(std::string(displayName(Items::Wheat)) == "Wheat", "an item knows its name");
        check(displayName(Blocks::Stone) != nullptr, "and so does a block");
        check(atlasTileFor(Items::Bone) == Tiles::ItemBone, "an item draws its sprite");
        check(atlasTileFor(Blocks::Stone) == blockInfo(Blocks::Stone).tileTop,
              "a block still draws its top face");
        check(maxStackOf(Items::Wheat) == 64 && maxStackOf(Blocks::Stone) == 64,
              "both stack to sixty-four");

        check(isBreedingFood(Items::Wheat), "wheat is what animals want");
        check(!isBreedingFood(Items::WheatSeeds), "seeds are not");
        check(!isBreedingFood(Blocks::Grass), "nor is grass");
    }

    void testItemsInInventory()
    {
        section("items: inventory");

        Inventory inventory;
        check(inventory.add(Items::Wheat, 10) == 0, "wheat goes in");
        check(inventory.countOf(Items::Wheat) == 10, "and is counted");
        check(inventory.countOf(Blocks::Stone) == 0, "without becoming stone");

        inventory.add(Items::Wheat, 60);
        check(inventory.countOf(Items::Wheat) == 70, "a second handful tops up the stack");
        check(inventory.slot(0).count == Inventory::MAX_STACK, "to sixty-four");
        check(inventory.slot(1).count == 6, "and spills the rest");

        // An item and a block that happen to be adjacent numbers must not
        // pool: Items::WheatSeeds is 256 and would be Air at a byte wide.
        Inventory mixed;
        mixed.add(Items::WheatSeeds, 5);
        mixed.add(Blocks::Stone, 5);
        check(mixed.countOf(Items::WheatSeeds) == 5, "seeds stay seeds");
        check(mixed.countOf(Blocks::Stone) == 5, "and stone stays stone");
        check(mixed.slot(0).id != mixed.slot(1).id, "in slots of their own");
    }

    void testWheatRecipe()
    {
        section("items: crafting");

        // Seeds used to bundle into wheat, as a stand-in for farming.
        // They must not any more: wheat is grown, and leaving both in
        // would make a field pointless.
        ItemStack grid[9];
        for (int i : { 0, 1, 3 }) { grid[i].id = Items::WheatSeeds; grid[i].count = 1; }
        check(!Crafting::match(grid, 2).valid(), "seeds no longer shortcut into wheat");

        // Four planks in the player's own 2x2 make the bench, which is
        // the only way to a 3x3 grid and so to anything three wide.
        ItemStack bench[9];
        for (int i : { 0, 1, 3, 4 }) { bench[i].id = Blocks::Planks; bench[i].count = 1; }
        const CraftOutput table = Crafting::match(bench, 2);
        check(table.valid() && table.id == Blocks::CraftingTable,
              "four planks in a square make a bench");
        check(table.count == 1, "one of them");

        // Three planks in a row is not a square, so it is not a bench --
        // it makes slabs, which is a different thing in the same shape.
        ItemStack row[9];
        for (int i : { 0, 1, 2 }) { row[i].id = Blocks::Planks; row[i].count = 1; }
        const CraftOutput notABench = Crafting::match(row, 3);
        check(!notABench.valid() || notABench.id != Blocks::CraftingTable,
              "three in a row is not a bench");

        // The real recipes still work.
        ItemStack logs[9];
        logs[0].id = Blocks::Log; logs[0].count = 1;
        const CraftOutput planks = Crafting::match(logs, 2);
        check(planks.valid() && planks.id == Blocks::Planks && planks.count == 4,
              "one log still makes four planks");

        ItemStack empty[9];
        check(!Crafting::match(empty, 2).valid(), "an empty grid makes nothing");
    }

    void testBreeding()
    {
        section("items: breeding");

        Mob cow(MobId::Cow, glm::vec3(0.0f, 64.0f, 0.0f), 11u);
        check(!cow.baby(), "a spawned cow is grown");
        check(!cow.inLove(), "and not yet interested");
        check(cow.canBreed(), "but could be");

        check(!cow.feed(Blocks::Grass), "it will not take grass");
        check(!cow.feed(Items::WheatSeeds), "nor seeds");
        check(cow.feed(Items::Wheat), "it takes wheat");
        check(cow.inLove(), "and falls in love");
        check(!cow.feed(Items::Wheat), "a second helping is refused");

        // Afterwards it has to wait before it can go again, which is what
        // stops one stack of wheat filling a field.
        cow.onBred();
        check(!cow.inLove(), "breeding ends the mood");
        check(!cow.canBreed(), "and starts a cooldown");
        check(!cow.feed(Items::Wheat), "so more wheat does nothing yet");

        // A calf is half size, cannot breed, and eats to grow up faster.
        Mob calf(MobId::Cow, glm::vec3(0.0f, 64.0f, 0.0f), 12u, true);
        check(calf.baby(), "a calf is a baby");
        check(!calf.canBreed(), "and far too young for that");
        check(std::fabs(calf.scale() - Mob::BABY_SCALE) < 0.001f, "it is half size");
        check(std::fabs(calf.width() - mobType(MobId::Cow).width * Mob::BABY_SCALE) < 0.001f,
              "with a hitbox to match");
        check(calf.height() < mobType(MobId::Cow).height, "and cannot be hit as high up");
        check(calf.feed(Items::Wheat), "it will still eat");
        check(!calf.inLove(), "but that hurries it along rather than pairing it off");

        // Monsters are not tempted by anything.
        Mob zombie(MobId::Zombie, glm::vec3(0.0f, 64.0f, 0.0f), 13u);
        check(mobType(MobId::Zombie).breedingFood == Blocks::Air, "a zombie wants no food");
        check(!zombie.feed(Items::Wheat), "and refuses wheat");
    }

    void testMobDrops()
    {
        section("items: mob drops");

        struct Expected { MobId who; StackId first; StackId second; };
        const Expected DROPS[] = {
            { MobId::Cow,      Items::RawBeef,      Items::Leather },
            { MobId::Pig,      Items::RawPorkchop,  Blocks::Air },
            { MobId::Chicken,  Items::RawChicken,   Items::Feather },
            { MobId::Sheep,    Items::RawMutton,    Blocks::Wool },
            { MobId::Skeleton, Items::Bone,         Blocks::Air },
            { MobId::Creeper,  Items::Gunpowder,    Blocks::Air },
            { MobId::Spider,   Items::StringItem,   Blocks::Air },
            { MobId::Zombie,   Blocks::Air,         Blocks::Air },
        };

        for (const Expected& e : DROPS)
        {
            const MobType& t = mobType(e.who);
            const std::string who = t.name;
            check(t.drop == e.first, who + " drops what it should");
            check(t.secondDrop == e.second, who + " drops its second thing too");
            if (t.drop != Blocks::Air) check(t.dropCount > 0, who + " drops at least one");
            if (t.secondDrop != Blocks::Air)
                check(t.secondDropCount > 0, who + " drops at least one of the second");
        }
    }

    void testFarming()
    {
        section("farming");

        // The crop's whole state is its block id, which is what lets a
        // field save and load with the chunks it stands in.
        check(WHEAT_STAGES == 8, "wheat has eight stages");
        check(wheatAtStage(0) == Blocks::Wheat0, "the first is stage zero");
        check(wheatAtStage(7) == WHEAT_RIPE, "and the last is ripe");
        check(isWheat(Blocks::Wheat0) && isWheat(WHEAT_RIPE), "both read as wheat");
        check(!isWheat(Blocks::Farmland) && !isWheat(Blocks::Grass), "earth does not");

        for (int stage = 0; stage < WHEAT_STAGES; ++stage)
            check(wheatStage(wheatAtStage(stage)) == stage,
                  "stage " + std::to_string(stage) + " survives the round trip");

        // Growing past ripe would walk off the end of the ids.
        check(wheatAtStage(99) == WHEAT_RIPE, "it cannot grow past ripe");
        check(wheatAtStage(-5) == Blocks::Wheat0, "nor shrink past nothing");

        // A crop is planted, never held: it must not turn up in the
        // creative palette or drop itself when broken.
        for (int stage = 0; stage < WHEAT_STAGES; ++stage)
            check(!isObtainable(wheatAtStage(stage)), "a crop is not a block you can hold");
        check(isObtainable(Blocks::Farmland), "but tilled earth is");

        check(Farming::isTillable(Blocks::Dirt), "dirt can be tilled");
        check(Farming::isTillable(Blocks::Grass), "so can grass");
        check(!Farming::isTillable(Blocks::Stone), "stone cannot");
        check(!Farming::isTillable(Blocks::Farmland), "and tilling twice does nothing");

        // Pull it up early and you get your seed back; let it ripen and
        // you get wheat and enough seed to sow again.
        const Farming::Harvest young = Farming::harvestOf(Blocks::Wheat3, 0);
        check(young.first == Items::WheatSeeds && young.firstCount == 1,
              "an unripe crop gives back one seed");
        check(young.secondCount == 0, "and nothing else");

        bool alwaysResows = true;
        for (uint32_t roll = 0; roll < 12; ++roll)
        {
            const Farming::Harvest ripe = Farming::harvestOf(WHEAT_RIPE, roll);
            if (ripe.first != Items::Wheat || ripe.firstCount != 1) alwaysResows = false;
            if (ripe.second != Items::WheatSeeds) alwaysResows = false;
            if (ripe.secondCount < 1 || ripe.secondCount > 3) alwaysResows = false;
        }
        check(alwaysResows, "a ripe one gives a wheat and one to three seeds");

        check(Farming::harvestOf(Blocks::Grass, 0).firstCount == 0,
              "harvesting something that is not a crop gives nothing");

        // Sowing a whole field has to be worth it: one seed in, one wheat
        // and at least one seed back out, so a field never runs down.
        check(Farming::harvestOf(WHEAT_RIPE, 0).secondCount >= 1,
              "a harvest always replaces the seed it cost");
    }

    // --- pathfinding ----------------------------------------------------

    // A map drawn as text, one row per line of z. '.' is floor, '#' is
    // wall. Everything is at y = 0, which is all a flat test needs.
    struct DrawnGround : Pathfinding::Ground
    {
        std::vector<std::string> rows;

        bool standable(const glm::ivec3& feet) const override
        {
            if (feet.y != 0) return false;
            if (feet.z < 0 || feet.z >= static_cast<int>(rows.size())) return false;
            const std::string& row = rows[static_cast<size_t>(feet.z)];
            if (feet.x < 0 || feet.x >= static_cast<int>(row.size())) return false;
            return row[static_cast<size_t>(feet.x)] != '#';
        }
    };

    bool walkable(const DrawnGround& map, const std::vector<glm::ivec3>& path,
                  const glm::ivec3& start)
    {
        glm::ivec3 at = start;
        for (const glm::ivec3& step : path)
        {
            if (std::abs(step.x - at.x) > 1 || std::abs(step.z - at.z) > 1) return false;
            if (!map.standable(step)) return false;
            at = step;
        }
        return true;
    }

    void testPathfinding()
    {
        section("mobs: pathfinding");

        // Open ground: straight there.
        DrawnGround open;
        open.rows = { ".......",
                      ".......",
                      ".......",
                      ".......",
                      "......." };

        const glm::ivec3 start(0, 0, 2);
        const glm::ivec3 goal(6, 0, 2);
        std::vector<glm::ivec3> path = Pathfinding::find(open, start, goal);
        check(!path.empty(), "it finds a way across open ground");
        check(path.back() == goal, "and arrives");
        check(walkable(open, path, start), "by steps a creature could take");
        check(static_cast<int>(path.size()) == 6, "without wandering");

        // A wall with a gap in it: the old AI would walk into this and
        // re-roll its heading until it happened to miss the wall.
        DrawnGround wall;
        wall.rows = { "...#...",
                      "...#...",
                      "...#...",
                      ".......",
                      "...#..." };

        path = Pathfinding::find(wall, start, goal);
        check(!path.empty() && path.back() == goal, "it finds the gap in a wall");
        check(walkable(wall, path, start), "and the route is walkable");

        bool wentThroughGap = false;
        for (const glm::ivec3& step : path)
            if (step.x == 3 && step.z == 3) wentThroughGap = true;
        check(wentThroughGap, "by going round through the gap");

        // Walled in completely: no path, and it must say so rather than
        // searching the world.
        DrawnGround sealed;
        sealed.rows = { ".#.....",
                        "##.....",
                        ".......",
                        ".......",
                        "......." };

        path = Pathfinding::find(sealed, glm::ivec3(0, 0, 0), goal);
        check(path.empty() || path.back() != goal, "it does not invent a way out of a sealed room");

        // Hemmed in on every side: the search has nowhere to go at all.
        DrawnGround box;
        box.rows = { "###",
                     "#.#",
                     "###" };
        path = Pathfinding::find(box, glm::ivec3(1, 0, 1), glm::ivec3(1, 0, 10));
        check(path.empty(), "and nothing at all when it is boxed in");

        // Standing on the goal already.
        check(Pathfinding::find(open, goal, goal).empty(), "no steps are needed to stay put");

        // Diagonals must not cut the corner where two walls meet.
        DrawnGround corner;
        corner.rows = { "..#",
                        "##.",
                        "..." };
        path = Pathfinding::find(corner, glm::ivec3(1, 0, 0), glm::ivec3(2, 0, 1));
        check(walkable(corner, path, glm::ivec3(1, 0, 0)),
              "it does not squeeze through the corner of a wall");

        // The node budget is a promise: a hopeless search has to stop.
        DrawnGround wide;
        wide.rows.assign(40, std::string(40, '.'));
        Pathfinding::Limits tight;
        tight.maxNodes = 30;
        path = Pathfinding::find(wide, glm::ivec3(0, 0, 0), glm::ivec3(39, 0, 39), tight);
        check(static_cast<int>(path.size()) <= tight.maxNodes,
              "a capped search returns something no longer than its budget");
        check(walkable(wide, path, glm::ivec3(0, 0, 0)),
              "and what it returns is still walkable");
        check(!path.empty(), "it heads the right way even when it cannot see the end");

        // The mob side of the arrangement: a mob asks for a route, is
        // handed one, and stops asking until the route is stale.
        Mob zombie(MobId::Zombie, glm::vec3(0.5f, 64.0f, 0.5f), 7u);
        check(!zombie.wantsPath(), "a mob with nothing to chase asks for no route");
        check(!zombie.hasPath(), "and is walking nowhere in particular");

        zombie.setPath({ glm::ivec3(1, 64, 0), glm::ivec3(2, 64, 0) }, glm::ivec3(2, 64, 0));
        check(zombie.hasPath(), "once handed a route it has one");
        check(zombie.pathLength() == 2, "of the length it was given");
        check(zombie.pathGoal() == glm::ivec3(2, 64, 0), "aimed where the search was aimed");
        check(!zombie.wantsPath(), "and does not ask again straight away");

        zombie.setPath({}, glm::ivec3(2, 64, 0));
        check(!zombie.hasPath(), "an empty route leaves it walking nowhere");
    }

    void testChatCommands()
    {
        section("chat and commands");

        // Ordinary chat is not a command and never needs cheats.
        check(Chat::parse("hello", false).kind == Chat::Kind::Say, "plain text is chat");
        check(Chat::parse("hello", false).text == "hello", "and goes out as typed");
        check(Chat::parse("   ", false).kind == Chat::Kind::Nothing, "a blank line says nothing");
        check(Chat::parse("  hi  ", false).text == "hi", "surrounding space is trimmed");

        // Help works either way, and says which way it is.
        check(Chat::parse("/help", false).kind == Chat::Kind::Help, "/help works without cheats");
        check(Chat::parse("/help", true).kind == Chat::Kind::Help, "and with them");
        check(Chat::parse("/help", false).text != Chat::parse("/help", true).text,
              "and tells you which of the two you have");

        // Everything else is refused when the world was made without
        // cheats -- and refused as Denied, not as Unknown, so the player
        // is told why rather than being told it does not exist.
        const char* GATED[] = { "/gamemode creative", "/time set day", "/tp 1 2 3",
                                "/give stone 4", "/kill", "/seed" };
        bool allDenied = true, allAllowed = true;
        for (const char* line : GATED)
        {
            if (Chat::parse(line, false).kind != Chat::Kind::Denied) allDenied = false;
            const Chat::Kind kind = Chat::parse(line, true).kind;
            if (kind == Chat::Kind::Denied || kind == Chat::Kind::Unknown) allAllowed = false;
        }
        check(allDenied, "every command is refused when cheats are off");
        check(allAllowed, "and every one of them works when they are on");

        check(Chat::parse("/gamemode creative", true).mode == static_cast<int>(GameMode::Creative),
              "/gamemode creative asks for creative");
        check(Chat::parse("/gm s", true).mode == static_cast<int>(GameMode::Survival),
              "and /gm s for survival");
        check(Chat::parse("/gamemode sideways", true).kind == Chat::Kind::Unknown,
              "a mode that does not exist is turned away");

        check(Chat::parse("/time set day", true).time == 0.25f, "day is dawn");
        check(Chat::parse("/time set night", true).time == 0.75f, "night is dusk");
        check(Chat::parse("/time set", true).kind == Chat::Kind::Unknown, "with no time, nothing");

        const Chat::Command tp = Chat::parse("/tp 10 70 -5", true);
        check(tp.position == glm::vec3(10.0f, 70.0f, -5.0f), "/tp reads three coordinates");
        check(Chat::parse("/tp 10 70", true).kind == Chat::Kind::Unknown, "two is not enough");
        check(Chat::parse("/tp here there everywhere", true).kind == Chat::Kind::Unknown,
              "and they have to be numbers");

        const Chat::Command give = Chat::parse("/give wheat 12", true);
        check(give.item == Items::Wheat && give.count == 12, "/give finds an item by name");
        check(Chat::parse("/give cobblestone", true).item == Blocks::Cobblestone,
              "and a block by name");
        check(Chat::parse("/give raw_beef", true).item == Items::RawBeef,
              "with underscores for the spaces, as Minecraft writes them");
        check(Chat::parse("/give unobtainium", true).kind == Chat::Kind::Unknown,
              "something that does not exist is turned away");
        check(Chat::parse("/give wheat", true).count == 1, "no count means one");

        check(Chat::parse("/nonsense", true).kind == Chat::Kind::Unknown,
              "an unknown command is unknown even with cheats on");
        check(!Chat::parse("/nonsense", true).text.empty(), "and says so rather than failing silently");

        // The log keeps only what is worth showing.
        Chat::Log log;
        for (int i = 0; i < 200; ++i) log.add("line " + std::to_string(i));
        check(static_cast<int>(log.lines().size()) <= Chat::VISIBLE_LINES * 4,
              "the log does not grow without bound");
        check(log.lines().back().text == "line 199", "and keeps the newest");

        log.clear();
        log.add("");
        check(log.lines().empty(), "an empty line is not logged");
    }

    void testCrackStages()
    {
        section("mining: break overlay");

        const int size = Atlas::crackOverlaySize();
        std::vector<std::vector<uint8_t>> stages;
        for (int i = 0; i < Tiles::CrackStages; ++i) stages.push_back(Atlas::crackOverlay(i));

        check(static_cast<int>(stages.size()) == Tiles::CrackStages, "ten stages of cracking");
        check(stages[0].size() == static_cast<size_t>(size) * size * 4, "each one a square tile");

        auto cracked = [&](const std::vector<uint8_t>& tile) {
            int n = 0;
            for (size_t i = 3; i < tile.size(); i += 4) if (tile[i] > 0) ++n;
            return n;
        };

        check(cracked(stages[0]) > 0, "the first stage marks the block");

        // The whole point: breaking is one crack spreading. Every pixel
        // cracked at one stage stays cracked at the next, and there are
        // always more of them, so it reads as damage accumulating rather
        // than as a flicker-book of unrelated pictures.
        bool contains = true, grows = true;
        for (size_t stage = 1; stage < stages.size(); ++stage)
        {
            const std::vector<uint8_t>& before = stages[stage - 1];
            const std::vector<uint8_t>& after = stages[stage];
            for (size_t i = 3; i < before.size(); i += 4)
                if (before[i] > 0 && after[i] == 0) contains = false;
            if (cracked(after) <= cracked(before)) grows = false;
        }

        check(contains, "no crack ever heals between one stage and the next");
        check(grows, "and every stage adds more of them");
        check(cracked(stages.back()) > cracked(stages.front()) * 2,
              "the last stage is far more broken than the first");
    }

    void testFood()
    {
        section("food and hunger");

        check(Food::isEdible(Items::Bread), "bread is food");
        check(Food::isEdible(Items::RawBeef), "so is raw beef");
        check(!Food::isEdible(Items::Bone), "a bone is not");
        check(!Food::isEdible(Blocks::Stone), "nor is a block of stone");
        check(Food::valueOf(Items::Bread).hunger > Food::valueOf(Items::RawChicken).hunger,
              "a loaf fills you more than a raw chicken leg");

        Player player(glm::vec3(0.0f, 64.0f, 0.0f));
        player.mode = GameMode::Survival;
        check(player.hunger == Player::MAX_HUNGER, "you start with a full stomach");
        check(!player.canEat(), "and nothing to gain from eating");

        player.hunger = 10;
        player.saturation = 0.0f;
        const FoodValue loaf = Food::valueOf(Items::Bread);
        check(player.eat(loaf.hunger, loaf.saturation), "a hungry player eats");
        check(player.hunger == 15, "and the bread fills five of it");
        check(player.saturation <= static_cast<float>(player.hunger),
              "saturation never runs past the hunger holding it up");

        player.hunger = Player::MAX_HUNGER;
        check(!player.eat(loaf.hunger, loaf.saturation), "a full player does not eat");

        // Exhaustion spends saturation first and hunger only once that
        // is gone, which is what makes a big meal last.
        Player walker(glm::vec3(0.0f, 64.0f, 0.0f));
        walker.mode = GameMode::Survival;
        walker.saturation = 2.0f;
        walker.addExhaustion(Player::EXHAUSTION_PER_DRAIN);
        walker.updateVitals(0.0f);
        check(walker.saturation == 1.0f, "exhaustion eats into saturation first");
        check(walker.hunger == Player::MAX_HUNGER, "leaving the hunger alone");

        walker.saturation = 0.0f;
        walker.addExhaustion(Player::EXHAUSTION_PER_DRAIN);
        walker.updateVitals(0.0f);
        check(walker.hunger == Player::MAX_HUNGER - 1, "and only then into the hunger");

        // Healing is what a full stomach buys you.
        Player starving(glm::vec3(0.0f, 64.0f, 0.0f));
        starving.mode = GameMode::Survival;
        starving.health = 10;
        starving.hunger = 4;
        for (int i = 0; i < 60; ++i) starving.updateVitals(0.2f);
        check(starving.health <= 10, "an empty stomach does not heal you");

        Player fed(glm::vec3(0.0f, 64.0f, 0.0f));
        fed.mode = GameMode::Survival;
        fed.health = 10;
        fed.hunger = Player::MAX_HUNGER;
        for (int i = 0; i < 60; ++i) fed.updateVitals(0.2f);
        check(fed.health > 10, "a full one does");

        // Nothing left to spend means it starts costing health.
        Player empty(glm::vec3(0.0f, 64.0f, 0.0f));
        empty.mode = GameMode::Survival;
        empty.hunger = 0;
        empty.saturation = 0.0f;
        const int before = empty.health;
        for (int i = 0; i < 60; ++i) empty.updateVitals(0.2f);
        check(empty.health < before, "starving costs you health");

        // Holding the button down for long enough is the whole of
        // eating, and letting go before then gets you nothing.
        Food::Bite bite = Food::chew(true, Items::Bread, true, 0.0f, 0.1f);
        check(bite.chewing && !bite.swallowed, "a bite starts but does not land straight away");
        check(bite.timer > 0.0f, "and the chew is counted");

        bite = Food::chew(true, Items::Bread, true, Food::EAT_SECONDS - 0.01f, 0.1f);
        check(bite.swallowed, "held long enough, it lands");
        check(bite.timer == 0.0f, "and the next one starts from nothing");

        bite = Food::chew(false, Items::Bread, true, 1.5f, 0.1f);
        check(!bite.chewing && bite.timer == 0.0f, "letting go throws the chew away");

        bite = Food::chew(true, Items::Bone, true, 1.5f, 0.1f);
        check(!bite.chewing, "a bone is not chewed however long you hold it");

        bite = Food::chew(true, Items::Bread, false, 1.5f, 0.1f);
        check(!bite.chewing, "and a full player will not start");

        // Creative players never think about any of it.
        Player builder(glm::vec3(0.0f, 64.0f, 0.0f));
        builder.mode = GameMode::Creative;
        builder.addExhaustion(100.0f);
        builder.updateVitals(0.1f);
        check(builder.hunger == Player::MAX_HUNGER, "creative mode is never hungry");
    }

    void testPlayerImmunity()
    {
        section("player: hurt immunity");

        Player player(glm::vec3(0.0f, 64.0f, 0.0f));
        player.mode = GameMode::Survival;
        const int full = Player::MAX_HEALTH;

        check(player.damage(3), "the first blow lands");
        check(player.health == full - 3, "and takes three off");
        check(player.invulnerable(), "leaving the player briefly immune");

        // Without this a mob standing in your face deals its damage once
        // a frame and kills you before you can step back.
        check(!player.damage(3), "a second blow in the same instant does not");
        check(player.health == full - 3, "so the health is unchanged");

        check(!player.damage(0), "a blow for nothing never lands");

        // Creative players are not hurt at all.
        Player builder(glm::vec3(0.0f, 64.0f, 0.0f));
        builder.mode = GameMode::Creative;
        check(!builder.damage(5), "creative mode shrugs it off");
        check(builder.health == full, "and keeps its health");

        // Respawning clears the immunity along with everything else.
        player.respawn(glm::vec3(0.0f, 70.0f, 0.0f));
        check(player.health == full, "respawning heals");
        check(!player.invulnerable(), "and does not leave you immune");
        check(player.damage(2), "so the next blow lands again");
    }

    void testTools()
    {
        section("tools");

        using namespace Tools;

        // Kind and tier are read off the item's position in the enum, so
        // the whole block of twenty-five has to line up.
        check(kindOf(Items::WoodPickaxe) == Kind::Pickaxe, "the first tool is a pickaxe");
        check(tierOf(Items::WoodPickaxe) == Tier::Wood, "made of wood");
        check(kindOf(Items::DiamondHoe) == Kind::Hoe, "the last is a hoe");
        check(tierOf(Items::DiamondHoe) == Tier::Diamond, "made of diamond");
        check(kindOf(Items::IronShovel) == Kind::Shovel, "and an iron shovel is a shovel");
        check(tierOf(Items::IronShovel) == Tier::Iron, "made of iron");

        check(kindOf(Blocks::Stone) == Kind::None, "a block is not a tool");
        check(kindOf(Items::Bread) == Kind::None, "nor is a loaf of bread");
        check(!isTool(Blocks::Air), "nor is an empty hand");

        // Every one of the twenty-five is a real tool with a real tier,
        // which is what catches an item added in the middle of the run.
        int tools = 0;
        for (StackId id = Items::WoodPickaxe; id <= Items::DiamondHoe; ++id)
        {
            if (kindOf(id) != Kind::None && tierOf(id) != Tier::None) ++tools;
            if (maxDurability(tierOf(id)) <= 0) break;
        }
        check(tools == 25, "all twenty-five are tools");

        // Harvesting. Stone wants a pickaxe of any sort; diamond ore
        // wants iron; obsidian wants diamond.
        check(!canHarvest(Blocks::Air, Blocks::Stone), "bare hands get nothing from stone");
        check(canHarvest(Items::WoodPickaxe, Blocks::Stone), "a wooden pickaxe does");
        check(!canHarvest(Items::WoodPickaxe, Blocks::DiamondOre), "but not from diamond ore");
        check(!canHarvest(Items::StonePickaxe, Blocks::DiamondOre), "nor a stone one");
        check(canHarvest(Items::IronPickaxe, Blocks::DiamondOre), "an iron one does");
        check(canHarvest(Items::DiamondPickaxe, Blocks::Obsidian), "and diamond gets obsidian");
        check(!canHarvest(Items::IronPickaxe, Blocks::Obsidian), "where iron does not");

        // Gold is quick but mines no more than wood can, which is the
        // whole joke of it.
        check(harvestLevel(Tier::Gold) == harvestLevel(Tier::Wood), "gold mines what wood mines");
        check(breakSeconds(Items::GoldPickaxe, Blocks::Stone) <
                  breakSeconds(Items::DiamondPickaxe, Blocks::Stone),
              "but faster than diamond");
        check(maxDurability(Tier::Gold) < maxDurability(Tier::Wood), "and lasts less long");

        // The right tool, and only the right tool, speeds the work up.
        check(breakSeconds(Items::WoodPickaxe, Blocks::Stone) <
                  breakSeconds(Blocks::Air, Blocks::Stone),
              "a pickaxe is quicker through stone than a fist");
        check(breakSeconds(Items::WoodShovel, Blocks::Stone) ==
                  breakSeconds(Blocks::Air, Blocks::Stone),
              "a shovel is no quicker through it at all");
        check(breakSeconds(Items::WoodShovel, Blocks::Sand) <
                  breakSeconds(Blocks::Air, Blocks::Sand),
              "but is through sand");
        check(breakSeconds(Items::WoodAxe, Blocks::Planks) <
                  breakSeconds(Items::WoodPickaxe, Blocks::Planks),
              "and an axe through wood");

        // Dirt needs nothing in particular, so a fist still gets a drop.
        check(canHarvest(Blocks::Air, Blocks::Dirt), "dirt comes up by hand");
        check(breakSeconds(Blocks::Air, Blocks::Bedrock) < 0.0f, "bedrock never breaks");
        check(breakSeconds(Items::DiamondPickaxe, Blocks::Bedrock) < 0.0f, "not even with diamond");

        // A better tier is always at least as quick as a worse one.
        const StackId ladder[] = { Items::WoodPickaxe, Items::StonePickaxe, Items::IronPickaxe };
        for (int i = 1; i < 3; ++i)
            check(breakSeconds(ladder[i], Blocks::Stone) < breakSeconds(ladder[i - 1], Blocks::Stone),
                  "each tier is quicker than the last through stone");

        // Wear. Breaking a block costs a point; swinging at nothing costs
        // nothing; and a block that takes no time to break is free.
        check(wearsOnBlock(Items::IronPickaxe, Blocks::Stone), "stone wears a pickaxe");
        check(!wearsOnBlock(Items::IronPickaxe, Blocks::Air), "air does not");
        check(!wearsOnBlock(Items::IronPickaxe, Blocks::TallGrass), "nor does tall grass");
        check(!wearsOnBlock(Blocks::Cobblestone, Blocks::Stone), "and a block in hand has nothing to wear");

        // Damage. A sword beats an axe beats a pickaxe beats a fist.
        check(attackDamage(Blocks::Air) == 1, "a fist does one");
        check(attackDamage(Items::WoodSword) > attackDamage(Blocks::Air), "a sword does more");
        check(attackDamage(Items::DiamondSword) > attackDamage(Items::WoodSword),
              "a diamond sword more again");
        check(attackDamage(Items::IronSword) > attackDamage(Items::IronAxe), "a sword beats its own axe");
        check(attackDamage(Items::IronAxe) > attackDamage(Items::IronPickaxe), "which beats its pickaxe");
        check(attackDamage(Items::DiamondHoe) == 1, "and a hoe is no weapon at all");
    }

    void testEveryBlockIsMinable()
    {
        section("tools: every block in the game");

        using namespace Tools;

        // The sweep that would have caught birch logs, gravel, clay and
        // the flowers needing a pickaxe: blockMaterial began life as a
        // table of what a block sounds like, and falling through to
        // stone was harmless until it also decided what drops.
        //
        // Anything a pickaxe is not for must come up by hand. Rather
        // than list them, the rule is turned round: a block that needs a
        // tool has to be one a pickaxe is the tool for.
        int needTool = 0, byHand = 0;

        for (BlockId id = 1; id < Blocks::Count; ++id)
        {
            if (!isObtainable(id)) continue;
            if (blockInfo(id).hardness < 0.0f) continue;   // never breaks

            // Named, so a failure says which block rather than leaving
            // the whole table to be searched by hand.
            const std::string who = blockInfo(id).name;

            if (requiredLevel(id) == 0)
            {
                ++byHand;
                check(canHarvest(Blocks::Air, id), who + " comes up by hand");
                continue;
            }

            ++needTool;

            // Whatever wants a tool wants a pickaxe, and the best
            // pickaxe in the game must be able to get it.
            check(blockMaterial(id) == Material::Stone, who + ": only stone wants a tool");
            check(canHarvest(Items::DiamondPickaxe, id), who + ": diamond gets all of it");
            check(!canHarvest(Blocks::Air, id), who + ": a bare hand gets none");

            // And the right tool is always quicker than the wrong one.
            check(breakSeconds(Items::DiamondPickaxe, id) < breakSeconds(Blocks::Air, id),
                  who + " is quicker with it");
        }

        check(byHand > 20, "most of the block list comes up by hand");
        check(needTool > 8, "and a fair few want a pickaxe");

        // The ones that were actually wrong, named, so a regression says
        // which block rather than just a count.
        const BlockId SOFT[] = {
            Blocks::BirchLog, Blocks::BirchLeaves, Blocks::Gravel, Blocks::Clay,
            Blocks::Snow, Blocks::SnowGrass, Blocks::TallGrass, Blocks::FlowerRed,
            Blocks::FlowerYellow, Blocks::Cactus, Blocks::Pumpkin, Blocks::Wool,
            Blocks::Glass, Blocks::Ice, Blocks::SoulSand, Blocks::CraftingTable,
            // A bed is cloth and a stick of TNT is paper: neither wants
            // a pickaxe, and both did until this line was written.
            Blocks::Bed, Blocks::Tnt,
        };
        for (BlockId id : SOFT)
            check(canHarvest(Blocks::Air, id), "a bare hand gets this one");

        // An axe is for wood of either kind, a shovel for the loose
        // ground, a pickaxe for stone -- not whichever was listed first.
        check(breakSeconds(Items::IronAxe, Blocks::BirchLog) <
                  breakSeconds(Blocks::Air, Blocks::BirchLog),
              "an axe is quicker through birch too");
        check(breakSeconds(Items::IronShovel, Blocks::Gravel) <
                  breakSeconds(Blocks::Air, Blocks::Gravel),
              "and a shovel through gravel");
        check(breakSeconds(Items::IronShovel, Blocks::Clay) <
                  breakSeconds(Blocks::Air, Blocks::Clay),
              "and through clay");
        check(breakSeconds(Items::IronPickaxe, Blocks::Furnace) <
                  breakSeconds(Blocks::Air, Blocks::Furnace),
              "and a pickaxe through a furnace");
        check(!canHarvest(Blocks::Air, Blocks::Furnace), "which a bare hand cannot take");
    }

    void testToolRecipes()
    {
        section("tools: the recipes");

        // Every tool in the game can be made, and comes out as itself.
        // A missing or malformed recipe shows up here rather than as an
        // item nobody can reach.
        struct Shape { int w, h; const char* cells; };
        const Shape SHAPES[5] = {
            { 3, 3, "MMM.S..S." },   // pickaxe
            { 2, 3, "MMMS.S" },      // axe
            { 1, 3, "MSS" },         // shovel
            { 1, 3, "MMS" },         // sword
            { 2, 3, "MM.S.S" },      // hoe
        };
        const StackId HEADS[5] = { Blocks::Planks, Blocks::Cobblestone,
                                   Items::IronIngot, Items::GoldIngot, Items::Diamond };

        int made = 0;
        for (int tier = 0; tier < 5; ++tier)
            for (int kind = 0; kind < 5; ++kind)
            {
                const Shape& shape = SHAPES[kind];

                ItemStack grid[9];
                for (int y = 0; y < shape.h; ++y)
                    for (int x = 0; x < shape.w; ++x)
                    {
                        const char cell = shape.cells[y * shape.w + x];
                        if (cell == '.') continue;
                        grid[y * 3 + x].id = (cell == 'M') ? HEADS[tier] : Items::Stick;
                        grid[y * 3 + x].count = 1;
                    }

                const CraftOutput out = Crafting::match(grid, 3);
                const StackId wanted = static_cast<StackId>(Items::WoodPickaxe + tier * 5 + kind);
                if (out.valid() && out.id == wanted && out.count == 1) ++made;
            }

        check(made == 25, "every tool in the game has a recipe that makes it");

        // Slabs: three of a block in a row makes six half ones.
        struct SlabPair { StackId from, to; };
        const SlabPair SLABS[4] = {
            { Blocks::Stone, Blocks::StoneSlab },
            { Blocks::Cobblestone, Blocks::CobblestoneSlab },
            { Blocks::Planks, Blocks::PlankSlab },
            { Blocks::Sandstone, Blocks::SandstoneSlab },
        };
        for (const SlabPair& pair : SLABS)
        {
            ItemStack row2[9];
            for (int i : { 0, 1, 2 }) { row2[i].id = pair.from; row2[i].count = 1; }
            const CraftOutput cut = Crafting::match(row2, 3);
            check(cut.valid() && cut.id == pair.to, "three in a row makes slabs");
            check(cut.count == 6, "six of them");

            // A slab stands half as tall, and the block it came from
            // still stands full height.
            check(blockHeight(asBlock(pair.to)) == 0.5f, "and a slab is half a block");
            check(blockHeight(asBlock(pair.from)) == 1.0f, "where the whole one is not");
            check(isSlab(asBlock(pair.to)), "and knows itself to be one");
            check(!isSlab(asBlock(pair.from)), "and the whole one does not");

            // You can still stand on one, and it still needs the right
            // tool if the block it came from did.
            check(isSolid(asBlock(pair.to)), "a slab holds you up");
            check(Tools::requiredLevel(asBlock(pair.to)) ==
                      Tools::requiredLevel(asBlock(pair.from)),
                  "and wants what its parent wanted");
        }

        // The recipes the early game turns on. A torch you cannot make
        // is a cave you cannot enter.
        ItemStack torch[9];
        torch[0].id = Items::Coal;  torch[0].count = 1;
        torch[3].id = Items::Stick; torch[3].count = 1;
        const CraftOutput lit = Crafting::match(torch, 2);
        check(lit.valid() && lit.id == Blocks::Torch, "coal on a stick makes torches");
        check(lit.count == 4, "four of them");

        ItemStack wool[9];
        for (int i : { 0, 1, 3, 4 }) { wool[i].id = Items::StringItem; wool[i].count = 1; }
        const CraftOutput cloth = Crafting::match(wool, 2);
        check(cloth.valid() && cloth.id == Blocks::Wool, "four string makes wool");

        ItemStack bed[9];
        for (int i : { 0, 1, 2 }) { bed[i].id = Blocks::Wool; bed[i].count = 1; }
        for (int i : { 3, 4, 5 }) { bed[i].id = Blocks::Planks; bed[i].count = 1; }
        const CraftOutput sleep = Crafting::match(bed, 3);
        check(sleep.valid() && sleep.id == Blocks::Bed, "wool over planks makes a bed");

        ItemStack box[9];
        for (int i : { 0, 1, 2, 3, 5, 6, 7, 8 }) { box[i].id = Blocks::Planks; box[i].count = 1; }
        const CraftOutput chest = Crafting::match(box, 3);
        check(chest.valid() && chest.id == Blocks::Chest, "eight planks make a chest");

        ItemStack oven[9];
        for (int i : { 0, 1, 2, 3, 5, 6, 7, 8 }) { oven[i].id = Blocks::Cobblestone; oven[i].count = 1; }
        const CraftOutput furnace = Crafting::match(oven, 3);
        check(furnace.valid() && furnace.id == Blocks::Furnace, "eight cobble make a furnace");

        // Sticks, and the bench that the three-wide ones need.
        ItemStack sticks[9];
        sticks[0].id = Blocks::Planks; sticks[0].count = 1;
        sticks[3].id = Blocks::Planks; sticks[3].count = 1;
        const CraftOutput stick = Crafting::match(sticks, 2);
        check(stick.valid() && stick.id == Items::Stick, "two planks stacked make sticks");
        check(stick.count == 4, "four of them");

        // A pickaxe is three wide, so the player's own grid cannot make
        // one however the planks are arranged.
        ItemStack small[9];
        small[0].id = Blocks::Planks; small[0].count = 1;
        small[1].id = Blocks::Planks; small[1].count = 1;
        small[3].id = Items::Stick;   small[3].count = 1;
        const CraftOutput none = Crafting::match(small, 2);
        check(!none.valid() || none.id != Items::WoodPickaxe,
              "a pickaxe cannot be made without a bench");
    }

    void testSmelting()
    {
        section("smelting");

        check(Smelting::resultOf(Blocks::IronOre) == Items::IronIngot, "iron ore smelts to an ingot");
        check(Smelting::resultOf(Blocks::GoldOre) == Items::GoldIngot, "and gold ore to its own");
        check(Smelting::resultOf(Blocks::Sand) == Blocks::Glass, "sand to glass");
        check(Smelting::resultOf(Items::RawBeef) == Items::Steak, "and beef to steak");
        check(!Smelting::isSmeltable(Blocks::Dirt), "dirt makes nothing");
        check(!Smelting::isSmeltable(Items::IronIngot), "and an ingot does not smelt twice");

        // Cooking is worth doing: every cut feeds you better for it, and
        // the cooked one is edible rather than merely obtainable.
        const StackId RAW[4] = { Items::RawBeef, Items::RawPorkchop,
                                 Items::RawChicken, Items::RawMutton };
        for (StackId raw : RAW)
        {
            const StackId cooked = Smelting::resultOf(raw);
            check(Food::isEdible(cooked), "a cooked cut can be eaten");
            check(Food::valueOf(cooked).hunger > Food::valueOf(raw).hunger,
                  "and feeds you better than the raw one");
        }

        check(Smelting::isFuel(Items::Coal), "coal burns");
        check(Smelting::burnSeconds(Items::Coal) > Smelting::burnSeconds(Blocks::Planks),
              "longer than planks do");
        check(!Smelting::isFuel(Blocks::Stone), "stone does not burn");
        check(!Smelting::isFuel(Items::IronPickaxe), "nor does an iron pickaxe");
        check(Smelting::isFuel(Items::WoodPickaxe), "though a wooden one is firewood");

        // A furnace with ore and coal in it produces ingots, a tick at a
        // time, and stops when the ore runs out.
        ItemStack input, fuel, output;
        input.id = Blocks::IronOre; input.count = 2;
        fuel.id = Items::Coal; fuel.count = 1;

        Furnace furnace;
        furnace.input = &input;
        furnace.fuel = &fuel;
        furnace.output = &output;

        furnace.tick(0.1f);
        check(furnace.lit(), "putting ore and coal in lights it");
        check(fuel.empty(), "which costs the coal");
        check(output.empty(), "and produces nothing yet");

        for (int i = 0; i < 101; ++i) furnace.tick(0.1f);
        check(output.id == Items::IronIngot, "ten seconds later there is an ingot");
        check(output.count == 1, "one of them");
        check(input.count == 1, "and one ore left");

        for (int i = 0; i < 101; ++i) furnace.tick(0.1f);
        check(output.count == 2, "the second ore follows");
        check(input.empty(), "and the ore is gone");

        // One lump of coal is eighty seconds, so the fire is still in
        // after twenty seconds of work, with nothing left to do.
        check(furnace.lit(), "the fire outlives the work");

        // An empty furnace does not eat fuel keeping warm.
        ItemStack nothing, spare, out2;
        spare.id = Items::Coal; spare.count = 3;

        Furnace idle;
        idle.input = &nothing;
        idle.fuel = &spare;
        idle.output = &out2;
        for (int i = 0; i < 50; ++i) idle.tick(0.1f);
        check(!idle.lit(), "an empty furnace never lights");
        check(spare.count == 3, "and burns none of its coal");

        // A full output stops it, rather than destroying what it makes.
        ItemStack ore, coal, full;
        ore.id = Blocks::GoldOre; ore.count = 4;
        coal.id = Items::Coal; coal.count = 2;
        full.id = Items::GoldIngot; full.count = maxStackOf(Items::GoldIngot);

        Furnace blocked;
        blocked.input = &ore;
        blocked.fuel = &coal;
        blocked.output = &full;
        for (int i = 0; i < 200; ++i) blocked.tick(0.1f);
        check(full.count == maxStackOf(Items::GoldIngot), "a full output slot loses nothing");
        check(ore.count == 4, "and the ore stays where it is");
        check(coal.count == 2, "with the coal unburnt");

        // Smelting something the output cannot hold does nothing either.
        ItemStack sand, wood, held;
        sand.id = Blocks::Sand; sand.count = 1;
        wood.id = Blocks::Planks; wood.count = 1;
        held.id = Blocks::Dirt; held.count = 1;

        Furnace mismatched;
        mismatched.input = &sand;
        mismatched.fuel = &wood;
        mismatched.output = &held;
        for (int i = 0; i < 200; ++i) mismatched.tick(0.1f);
        check(held.id == Blocks::Dirt && held.count == 1, "an output holding something else blocks it");
        check(sand.count == 1, "and the sand is not smelted");
    }

    void testFurnaceStore()
    {
        section("smelting: the furnaces");

        Furnaces furnaces;
        const glm::ivec3 here(4, 70, -9);

        check(!furnaces.has(here), "nowhere has a furnace to begin with");

        Furnaces::State& state = furnaces.at(here);
        state.input.id = Blocks::Sand; state.input.count = 1;
        state.fuel.id = Items::Coal; state.fuel.count = 1;
        check(furnaces.has(here), "putting something in one records it");

        // Lighting it is a change the world has to hear about, so the
        // block can be swapped for its burning twin.
        const std::vector<glm::ivec3> lit = furnaces.tick(0.1f);
        check(lit.size() == 1 && lit[0] == here, "lighting one is reported");
        check(furnaces.at(here).lit(), "and it is alight");

        // Ten seconds of it, then the glass comes out.
        for (int i = 0; i < 101; ++i) furnaces.tick(0.1f);
        check(furnaces.at(here).output.id == Blocks::Glass, "the sand became glass");

        // An empty, cold furnace is forgotten rather than kept forever.
        Furnaces::State& done = furnaces.at(here);
        done.output.clear();
        done.burnLeft = 0.0f;
        done.cooked = 0.0f;
        furnaces.tick(0.016f);
        check(!furnaces.has(here), "a cold empty furnace is forgotten");
    }

    void testArmour()
    {
        section("armour");

        using namespace Armour;

        check(pieceOf(Items::LeatherHelmet) == Piece::Helmet, "the first piece is a helmet");
        check(tierOf(Items::LeatherHelmet) == Tier::Leather, "made of leather");
        check(pieceOf(Items::DiamondBoots) == Piece::Boots, "the last is a pair of boots");
        check(tierOf(Items::DiamondBoots) == Tier::Diamond, "made of diamond");
        check(!isArmour(Items::IronSword), "a sword is not armour");
        check(!isArmour(Blocks::Stone), "nor is a block");

        // Each piece goes in its own slot, and the four of them between
        // them cover all four slots exactly once.
        bool covered[4] = { false, false, false, false };
        for (StackId id = Items::IronHelmet; id <= Items::IronBoots; ++id)
        {
            const int slot = slotFor(id);
            check(slot >= 0 && slot < 4, "every piece has a slot");
            check(!covered[slot], "and no two share one");
            covered[slot] = true;
        }
        check(covered[0] && covered[1] && covered[2] && covered[3], "all four are covered");

        // Minecraft's own totals: leather seven, gold eleven, iron
        // fifteen, diamond twenty.
        const int LEATHER = defencePoints(Items::LeatherHelmet) + defencePoints(Items::LeatherChestplate)
                          + defencePoints(Items::LeatherLeggings) + defencePoints(Items::LeatherBoots);
        const int IRON = defencePoints(Items::IronHelmet) + defencePoints(Items::IronChestplate)
                       + defencePoints(Items::IronLeggings) + defencePoints(Items::IronBoots);
        const int DIAMOND = defencePoints(Items::DiamondHelmet) + defencePoints(Items::DiamondChestplate)
                          + defencePoints(Items::DiamondLeggings) + defencePoints(Items::DiamondBoots);

        check(LEATHER == 7, "a suit of leather is seven points");
        check(IRON == 15, "a suit of iron is fifteen");
        check(DIAMOND == 20, "a suit of diamond is twenty");

        // A chestplate is always the best piece of its tier, and a
        // better tier is always worth at least as much.
        check(defencePoints(Items::IronChestplate) > defencePoints(Items::IronHelmet),
              "the chestplate is the best piece");
        check(defencePoints(Items::DiamondChestplate) > defencePoints(Items::IronChestplate),
              "and diamond beats iron");
        check(maxDurability(Items::DiamondHelmet) > maxDurability(Items::LeatherHelmet),
              "and lasts far longer");

        // Four percent off per point, and a blow is never stopped dead.
        check(reduce(10, 0) == 10, "no armour takes nothing off");
        check(reduce(10, 20) < reduce(10, 10), "a full suit beats half a one");
        check(reduce(10, 10) < 10, "and any armour beats none");
        check(reduce(100, 20) == 20, "twenty points is eighty percent off");
        check(reduce(1, 20) == 1, "but the smallest blow still lands");
        check(reduce(10, 40) == reduce(10, 20), "and points past twenty are wasted");
        check(reduce(0, 10) == 0, "nothing stays nothing");

        // The whole point of it: a player in diamond outlasts one in
        // nothing by a long way against the same blows.
        Player bare(glm::vec3(0.0f, 64.0f, 0.0f));
        Player armoured(glm::vec3(0.0f, 64.0f, 0.0f));
        armoured.armourPoints = DIAMOND;

        int bareHits = 0, armouredHits = 0;
        while (bare.health > 0 && bareHits < 100)
        {
            bare.clearHurtCooldown();
            if (bare.damage(5)) ++bareHits;
        }
        while (armoured.health > 0 && armouredHits < 100)
        {
            armoured.clearHurtCooldown();
            if (armoured.damage(5)) ++armouredHits;
        }
        check(armouredHits > bareHits * 2, "diamond armour more than doubles what you survive");
    }

    void testArmourRecipes()
    {
        section("armour: the recipes");

        struct Shape { int w, h; const char* cells; };
        const Shape SHAPES[4] = {
            { 3, 2, "MMMM.M" },        // helmet
            { 3, 3, "M.MMMMMMM" },     // chestplate
            { 3, 3, "MMMM.MM.M" },     // leggings
            { 3, 2, "M.MM.M" },        // boots
        };
        const StackId MATERIAL[4] = { Items::Leather, Items::GoldIngot,
                                      Items::IronIngot, Items::Diamond };

        int made = 0;
        for (int tier = 0; tier < 4; ++tier)
            for (int piece = 0; piece < 4; ++piece)
            {
                const Shape& shape = SHAPES[piece];

                ItemStack grid[9];
                for (int y = 0; y < shape.h; ++y)
                    for (int x = 0; x < shape.w; ++x)
                    {
                        if (shape.cells[y * shape.w + x] != 'M') continue;
                        grid[y * 3 + x].id = MATERIAL[tier];
                        grid[y * 3 + x].count = 1;
                    }

                const CraftOutput out = Crafting::match(grid, 3);
                const StackId wanted = static_cast<StackId>(Items::LeatherHelmet + tier * 4 + piece);
                if (out.valid() && out.id == wanted && out.count == 1) ++made;
            }

        check(made == 16, "every piece of armour has a recipe that makes it");
    }

    void testWearing()
    {
        section("armour: putting it on");

        Inventory inventory;

        ItemStack helmet{ Items::IronHelmet, 1, 0 };
        check(inventory.wear(helmet), "a helmet goes on");
        check(inventory.slot(Inventory::ARMOR_FIRST).id == Items::IronHelmet,
              "into the helmet slot");
        check(helmet.empty(), "and leaves your hand");

        // Swapping one for another hands the old one back rather than
        // destroying it.
        ItemStack better{ Items::DiamondHelmet, 1, 0 };
        check(inventory.wear(better), "a better one goes on over it");
        check(inventory.slot(Inventory::ARMOR_FIRST).id == Items::DiamondHelmet, "and is worn");
        check(better.id == Items::IronHelmet, "and the old one comes back to your hand");

        ItemStack sword{ Items::IronSword, 1, 0 };
        check(!inventory.wear(sword), "a sword cannot be worn");

        // Boots cannot be put in the helmet slot by hand either.
        Inventory other;
        other.cursor() = ItemStack{ Items::IronBoots, 1, 0 };
        other.leftClick(Inventory::ARMOR_FIRST);
        check(other.slot(Inventory::ARMOR_FIRST).empty(), "boots do not go on your head");
        check(other.cursor().id == Items::IronBoots, "and stay on the cursor");

        other.leftClick(Inventory::ARMOR_FIRST + 3);
        check(other.slot(Inventory::ARMOR_FIRST + 3).id == Items::IronBoots,
              "but go on your feet");
    }

    void testPortalFrames()
    {
        section("nether portals");

        // A frame built out of nothing: obsidian wherever the test says,
        // air everywhere else.
        struct Built : Portal::Blocks
        {
            std::vector<glm::ivec3> frame;

            bool isFrame(int x, int y, int z) const override
            {
                return std::find(frame.begin(), frame.end(), glm::ivec3(x, y, z)) != frame.end();
            }
            bool isFillable(int x, int y, int z) const override
            {
                return !isFrame(x, y, z);
            }

            // The standing frame: a ring with a hole `w` across and `h`
            // high, with its bottom-left inside corner at the origin.
            void build(int w, int h, bool alongX)
            {
                frame.clear();
                for (int a = -1; a <= w; ++a)
                    for (int b = -1; b <= h; ++b)
                    {
                        const bool edge = (a == -1 || a == w || b == -1 || b == h);
                        if (!edge) continue;
                        frame.push_back(alongX ? glm::ivec3(a, b, 0) : glm::ivec3(0, b, a));
                    }
            }
        };

        // The standard doorway: two across, three high.
        Built built;
        built.build(2, 3, true);
        Portal::Found found = Portal::findFrame(built, glm::ivec3(0, 0, 0));
        check(found.valid, "a two by three frame lights");
        check(found.inside.size() == 6, "and six blocks fill it");

        // The same frame the other way round.
        built.build(2, 3, false);
        found = Portal::findFrame(built, glm::ivec3(0, 0, 0));
        check(found.valid, "and so does one facing the other way");
        check(found.inside.size() == 6, "with the same six blocks");

        // Bigger frames are allowed, as in Minecraft.
        built.build(4, 5, true);
        found = Portal::findFrame(built, glm::ivec3(1, 2, 0));
        check(found.valid, "a larger frame lights from anywhere inside it");
        check(found.inside.size() == 20, "and fills the whole hole");

        // A hole with a block missing leaks, and nothing catches.
        built.build(2, 3, true);
        built.frame.erase(std::remove(built.frame.begin(), built.frame.end(),
                                      glm::ivec3(0, -1, 0)),
                          built.frame.end());
        check(!Portal::findFrame(built, glm::ivec3(0, 0, 0)).valid,
              "a frame with a gap in it does not light");

        // Too small in either direction.
        built.build(1, 3, true);
        check(!Portal::findFrame(built, glm::ivec3(0, 0, 0)).valid,
              "one block wide is not a doorway");

        built.build(2, 2, true);
        check(!Portal::findFrame(built, glm::ivec3(0, 0, 0)).valid,
              "nor is two high");

        // Lighting the frame itself does nothing.
        built.build(2, 3, true);
        check(!Portal::findFrame(built, glm::ivec3(-1, -1, 0)).valid,
              "striking the obsidian itself lights nothing");

        // No frame at all: open air leaks forever and must be refused
        // rather than filling the sky.
        struct OpenAir : Portal::Blocks
        {
            bool isFrame(int, int, int) const override { return false; }
            bool isFillable(int, int, int) const override { return true; }
        };
        OpenAir sky;
        check(!Portal::findFrame(sky, glm::ivec3(0, 70, 0)).valid,
              "open air does not light");
    }

    void testExplosions()
    {
        section("explosions");

        const float R = Explosion::CREEPER_RADIUS;

        // Hardest at the middle, nothing at the edge, and falling off
        // the whole way between.
        check(Explosion::falloff(0.0f, R) == 1.0f, "the middle takes all of it");
        check(Explosion::falloff(R, R) == 0.0f, "the edge takes none");
        check(Explosion::falloff(R + 5.0f, R) == 0.0f, "and past it, none");
        check(Explosion::falloff(R * 0.5f, R) < 1.0f, "half way takes less than all");
        check(Explosion::falloff(R * 0.5f, R) > 0.0f, "but more than none");
        check(Explosion::falloff(1.0f, R) > Explosion::falloff(2.0f, R),
              "nearer is always worse");

        check(Explosion::damageAt(0.0f, R, 24) == 24, "a blast in the face is the full count");
        check(Explosion::damageAt(R, R, 24) == 0, "and at the edge, nothing");
        check(Explosion::damageAt(R * 0.5f, R, 24) < 24, "and between, something less");

        // What it breaks. Bedrock never, obsidian not from a creeper,
        // and water smothers it rather than being thrown about.
        check(!Explosion::destroys(Blocks::Bedrock, 0.0f, R), "bedrock survives anything");
        check(!Explosion::destroys(Blocks::Obsidian, 0.0f, R), "obsidian survives a creeper");
        check(!Explosion::destroys(Blocks::Water, 0.0f, R), "water smothers it");
        check(!Explosion::destroys(Blocks::Air, 0.0f, R), "and air is nothing to break");

        check(Explosion::destroys(Blocks::Dirt, 0.0f, R), "dirt at the centre goes");
        check(Explosion::destroys(Blocks::Stone, 0.0f, R), "and so does stone");
        check(!Explosion::destroys(Blocks::Stone, R, R), "but not at the edge");

        // Softer blocks survive further out than harder ones, which is
        // what gives a blast its ragged edge rather than a clean sphere.
        float dirtReach = 0.0f, stoneReach = 0.0f;
        for (float d = 0.0f; d < R; d += 0.05f)
        {
            if (Explosion::destroys(Blocks::Dirt, d, R)) dirtReach = d;
            if (Explosion::destroys(Blocks::Stone, d, R)) stoneReach = d;
        }
        check(dirtReach > stoneReach, "dirt goes further out than stone");

        // And the creeper's own numbers are survivable in armour but not
        // without: twenty-four at point blank is more than a player has.
        check(Explosion::CREEPER_DAMAGE > Player::MAX_HEALTH,
              "a creeper in your face kills an unarmoured player");
        check(Armour::reduce(Explosion::CREEPER_DAMAGE, 20) < Player::MAX_HEALTH,
              "but a suit of diamond sees you through it");
    }

    void testArrows()
    {
        section("bows and arrows");

        // Drawing. A bow barely pulled is worth almost nothing, which is
        // what stops it being a machine gun.
        check(Arrows::drawFraction(0.0f) == 0.0f, "an undrawn bow is nothing");
        check(Arrows::drawFraction(Arrows::DRAW_SECONDS) == 1.0f, "a full draw is one");
        check(Arrows::drawFraction(99.0f) == 1.0f, "and holding longer adds nothing");

        check(!Arrows::worthLoosing(0.05f), "a twitch looses nothing");
        check(Arrows::worthLoosing(1.0f), "a full draw does");

        check(Arrows::speedFor(1.0f) > Arrows::speedFor(0.5f), "a fuller draw flies faster");
        check(Arrows::speedFor(0.0f) >= Arrows::MIN_SPEED, "and never slower than the floor");
        check(Arrows::speedFor(1.0f) <= Arrows::MAX_SPEED, "nor faster than the ceiling");

        check(Arrows::damageFor(1.0f) > Arrows::damageFor(0.3f), "and hits harder");
        check(Arrows::damageFor(0.0f) >= 1, "but always for something");

        // Flight. An arrow fired level drops, and one fired up comes
        // back down -- the sums are the same gravity the player feels.
        struct NoTargets : Projectiles::Targets
        {
            int asked = 0;
            bool strike(const glm::vec3&, int, bool) override { ++asked; return false; }
        };

        // Open sky: nothing is solid, so the only thing stopping an
        // arrow is the arithmetic.
        struct OpenSky : Projectiles::Blocks
        {
            bool solidAt(int, int, int) const override { return false; }
        };
        OpenSky world;

        Projectiles arrows;
        NoTargets nothing;

        const glm::vec3 from(0.5f, 200.0f, 0.5f);   // well clear of any terrain
        arrows.spawn(from, glm::vec3(20.0f, 0.0f, 0.0f), 4, true);
        check(arrows.all().size() == 1, "an arrow is in the air");

        for (int i = 0; i < 10; ++i) arrows.update(0.02f, world, nothing);

        check(!arrows.all().empty(), "and still flying");
        if (!arrows.all().empty())
        {
            const Arrow& flying = arrows.all()[0];
            check(flying.position.x > from.x, "it has gone the way it was pointed");
            check(flying.position.y < from.y, "and has dropped on the way");
            check(flying.velocity.y < 0.0f, "and is still falling");
        }
        check(nothing.asked > 0, "something was asked about what it might hit");

        // A target that says yes stops it dead.
        struct Hits : Projectiles::Targets
        {
            int struck = 0;
            int damageSeen = 0;
            bool fromPlayerSeen = false;
            bool strike(const glm::vec3&, int damage, bool fromPlayer) override
            {
                ++struck;
                damageSeen = damage;
                fromPlayerSeen = fromPlayer;
                return true;
            }
        };

        Projectiles one;
        Hits target;
        one.spawn(glm::vec3(0.5f, 200.0f, 0.5f), glm::vec3(20.0f, 0.0f, 0.0f), 7, true);
        one.update(0.02f, world, target);

        check(target.struck == 1, "a target that says yes is struck once");
        check(target.damageSeen == 7, "for what the arrow was carrying");
        check(target.fromPlayerSeen, "and knows who loosed it");
        check(one.all().empty(), "and the arrow is gone");

        // A skeleton's arrow says so, which is what keeps it from
        // hitting the skeleton that fired it.
        Projectiles theirs;
        Hits mine;
        theirs.spawn(glm::vec3(0.5f, 200.0f, 0.5f), glm::vec3(0.0f, 0.0f, 20.0f), 3, false);
        theirs.update(0.02f, world, mine);
        check(!mine.fromPlayerSeen, "a skeleton's arrow is marked as theirs");

        // Arrows that hit nothing do not pile up for ever.
        Projectiles stale;
        NoTargets ignored;
        stale.spawn(glm::vec3(0.5f, 300.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), 1, true);
        for (int i = 0; i < 40; ++i) stale.update(1.0f, world, ignored);
        check(stale.all().empty(), "an arrow that hits nothing is cleared up");
    }

    void testSleep()
    {
        section("beds");

        // Noon and the afternoon are no time for bed.
        check(!Sleep::isNight(0.25f), "you cannot sleep at sunrise");
        check(!Sleep::isNight(0.5f), "nor at noon");
        check(!Sleep::isNight(0.7f), "nor in the late afternoon");

        // Dusk through midnight and round to dawn is.
        check(Sleep::isNight(0.75f), "you can sleep at sunset");
        check(Sleep::isNight(0.9f), "and through the evening");
        check(Sleep::isNight(0.0f), "and at midnight");
        check(Sleep::isNight(0.1f), "and in the small hours");
        check(!Sleep::isNight(0.23f), "but not once it is getting light");

        // The window wraps round midnight rather than stopping at it,
        // which an unwrapped comparison would get wrong.
        check(Sleep::isNight(0.99f), "just before midnight is night");
        check(Sleep::isNight(0.01f), "and just after it still is");

        // Waking puts the clock at morning, which is not itself a time
        // you could have gone to bed at -- so sleeping twice over is not
        // possible without a day passing first.
        check(!Sleep::isNight(Sleep::wakeTime()), "you wake into the day");

        // The whole day, swept, so the window is exactly one stretch
        // rather than two: counting the edges catches a rule that is
        // true in two separate places by accident.
        int openings = 0;
        bool was = Sleep::isNight(0.0f);
        for (int i = 1; i <= 1000; ++i)
        {
            const bool now = Sleep::isNight(static_cast<float>(i) / 1000.0f);
            if (now && !was) ++openings;
            was = now;
        }
        check(openings == 1, "the night opens once in a day");
    }

    void testChestStore()
    {
        section("chests");

        Chests chests;
        const glm::ivec3 here(2, 71, 8);

        check(!chests.has(here), "nowhere has a chest to begin with");

        Chests::Contents& held = chests.at(here);
        held[4].id = Blocks::Cobblestone;
        held[4].count = 30;
        check(chests.has(here), "putting something in one records it");
        check(chests.at(here)[4].count == 30, "and it is still there");

        // Emptying one forgets it, so a save does not carry a row of
        // twenty-seven nothings for every chest ever placed.
        chests.forgetIfEmpty(here);
        check(chests.has(here), "a chest with something in it is kept");

        chests.at(here)[4].clear();
        chests.forgetIfEmpty(here);
        check(!chests.has(here), "an emptied one is forgotten");

        // Two chests do not share contents, which is the bug a single
        // shared buffer would have.
        const glm::ivec3 a(0, 64, 0), b(1, 64, 0);
        chests.at(a)[0].id = Blocks::Sand;
        chests.at(a)[0].count = 1;
        check(chests.at(b)[0].empty(), "the chest next door is its own");
    }

    void testLevelRoundTrip()
    {
        section("saving: the level file");

        const std::string dir = "selftest-save";
        WorldSave::ensureDirectories(dir);

        LevelState wrote;
        wrote.seed = 12345u;
        wrote.playerPosition = glm::vec3(10.5f, 70.0f, -4.5f);
        wrote.timeOfDay = 0.6f;
        wrote.health = 14;
        wrote.hunger = 11;
        wrote.saturation = 2.5f;
        wrote.cheats = true;
        wrote.gameMode = 1;
        wrote.selectedSlot = 5;
        wrote.spawn = glm::vec3(-8.5f, 66.0f, 32.5f);
        wrote.inventory.emplace_back(static_cast<uint16_t>(Blocks::Stone), 42);

        LevelState::SavedFurnace furnace;
        furnace.x = 3; furnace.y = 64; furnace.z = -7;
        furnace.input = Blocks::IronOre; furnace.inputCount = 5;
        furnace.fuel = Items::Coal; furnace.fuelCount = 2;
        furnace.output = Items::IronIngot; furnace.outputCount = 9;
        furnace.burnLeft = 12.0f; furnace.burnTotal = 80.0f; furnace.cooked = 3.5f;
        wrote.furnaces.push_back(furnace);

        LevelState::SavedChest chest;
        chest.x = -5; chest.y = 68; chest.z = 12;
        chest.slots.resize(Chests::SLOTS, { 0, 0 });
        chest.slots[0] = { static_cast<uint16_t>(Blocks::Planks), 64 };
        chest.slots[26] = { static_cast<uint16_t>(Items::DiamondSword), 1 };
        wrote.chests.push_back(chest);

        WorldSave::saveLevel(dir, wrote);

        LevelState read;
        check(WorldSave::loadLevel(dir, read), "a level written reads back");
        check(read.seed == wrote.seed, "with its seed");
        check(read.playerPosition == wrote.playerPosition, "and where you stood");
        check(read.health == 14 && read.hunger == 11, "and how you were doing");
        check(read.cheats, "and whether the commands work");

        // Dying used to put you back at the generator's guess rather than
        // where the world actually began, because this was written and
        // never read.
        check(read.hasSpawn, "the spawn point comes back");
        check(read.spawn == wrote.spawn, "and is the one that was saved");

        check(read.inventory.size() == 1, "the inventory comes back");

        check(read.chests.size() == 1, "and the chest");
        if (read.chests.size() == 1)
        {
            const LevelState::SavedChest& c = read.chests[0];
            check(c.x == -5 && c.y == 68 && c.z == 12, "where it was left");
            check(c.slots.size() == Chests::SLOTS, "with all its slots");
            check(c.slots[0].first == Blocks::Planks && c.slots[0].second == 64,
                  "and what was in the first");
            check(c.slots[26].first == Items::DiamondSword, "and the last");
        }


        // A furnace left burning is still burning when you come back,
        // and still holds what was in it.
        check(read.furnaces.size() == 1, "the furnace comes back too");
        if (read.furnaces.size() == 1)
        {
            const LevelState::SavedFurnace& f = read.furnaces[0];
            check(f.x == 3 && f.y == 64 && f.z == -7, "where it was left");
            check(f.input == Blocks::IronOre && f.inputCount == 5, "with the ore still in it");
            check(f.fuel == Items::Coal && f.fuelCount == 2, "and the coal");
            check(f.output == Items::IronIngot && f.outputCount == 9, "and what it had made");
            check(f.burnLeft > 11.0f && f.burnLeft < 13.0f, "still alight");
        }

        check(read.inventory[0].first == Blocks::Stone, "holding what it held");
        check(read.inventory[0].second == 42, "and as much of it");

        // The test leaves nothing behind in the player's game folder.
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    void testMobHurt()
    {
        section("mobs: being hurt");

        // A blow from somewhere knocks the mob back and off its feet.
        Mob struck(MobId::Cow, glm::vec3(0.0f, 64.0f, 0.0f), 1u);
        struck.damage(1, glm::vec3(1.0f, 0.0f, 0.0f));
        check(struck.velocity().y > 0.0f, "a blow lifts the mob");
        check(struck.velocity().x > 0.0f, "and pushes it away from the blow");

        // Fire passes no direction, and lifting the mob for it had the
        // undead hopping on the spot for as long as the sun was up.
        Mob burnt(MobId::Zombie, glm::vec3(0.0f, 64.0f, 0.0f), 2u);
        const float before = burnt.velocity().y;
        burnt.damage(1, glm::vec3(0.0f));
        check(burnt.velocity().y == before, "burning does not launch it");
        check(burnt.velocity().x == 0.0f, "nor shove it sideways");

        // One yelp per blow is right; one per burn tick, from a herd, is
        // a racket. The voice is rate limited, the death cry is not.
        Mob noisy(MobId::Pig, glm::vec3(0.0f, 64.0f, 0.0f), 3u);
        noisy.sounds().clear();
        noisy.damage(1, glm::vec3(0.0f));
        check(noisy.sounds().size() == 1, "the first hurt is heard");
        noisy.sounds().clear();
        noisy.damage(1, glm::vec3(0.0f));
        check(noisy.sounds().empty(), "a second hurt straight after is not");

        Mob doomed(MobId::Chicken, glm::vec3(0.0f, 64.0f, 0.0f), 4u);
        doomed.damage(1, glm::vec3(0.0f));
        doomed.sounds().clear();
        doomed.damage(100, glm::vec3(0.0f));
        check(!doomed.alive(), "a big enough blow kills");
        check(doomed.sounds().size() == 1, "and dying is always heard");
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
    testBoxWinding();
    testMobTypes();
    testSunlightBurning();
    testMobSpawnRules();
    testMobGeometry();
    testMobDamage();
    testMobCombat();
    testItems();
    testItemsInInventory();
    testWheatRecipe();
    testBreeding();
    testMobDrops();
    testFarming();
    testPathfinding();
    testChatCommands();
    testCrackStages();
    testFood();
    testPlayerImmunity();
    testMobHurt();
    testTools();
    testEveryBlockIsMinable();
    testToolRecipes();
    testSmelting();
    testFurnaceStore();
    testArmour();
    testArmourRecipes();
    testWearing();
    testPortalFrames();
    testExplosions();
    testArrows();
    testSleep();
    testChestStore();
    testLevelRoundTrip();

    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
