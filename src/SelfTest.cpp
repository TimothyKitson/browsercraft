#include "Entity/SpriteParticles.h"
// Headless checks for the parts of the game you normally only reach by
// clicking: the inventory's slot rules and the crafting recipes.
//
// Run with --selftest. No window, no GL, no world -- just the logic, so
// it can be run after any change in a second.

#include "Player/Inventory.h"
#include "Game/Crafting.h"
#include "Game/GameRules.h"
#include "Game/Chat.h"
#include "Entity/DroppedItems.h"
#include "World/World.h"
#include "Game/Beacon.h"
#include "Game/Hopper.h"
#include "Game/Redstone.h"
#include "Game/Piston.h"
#include "Game/Copper.h"
#include "Game/Trial.h"
#include "World/TrialChamber.h"
#include "World/Structures.h"
#include "World/Stronghold.h"
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
#include "Game/Composting.h"
#include "Game/Glide.h"
#include "World/Signs.h"
#include "World/Frames.h"
#include "World/WallFacing.h"
#include "Game/Armour.h"
#include "Game/Sleep.h"
#include "Game/Explosion.h"
#include "Game/Experience.h"
#include "Game/Cane.h"
#include "Game/Fluids.h"
#include "Renderer/BookRenderer.h"
#include "Renderer/PaintingRenderer.h"
#include "Renderer/ArmorStandRenderer.h"
#include "Renderer/SignRenderer.h"
#include "Renderer/ArmourTextures.h"
#include "World/ChunkMesher.h"
#include "Game/Anvil.h"
#include "Game/Archaeology.h"
#include "Game/Stonecutting.h"
#include "Game/Smithing.h"
#include "Game/Maps.h"
#include "Game/Banners.h"
#include "Game/Sculk.h"
#include "Game/Grindstone.h"
#include "Game/Saplings.h"
#include "Game/Shield.h"
#include "Game/Effects.h"
#include "Game/Brewing.h"
#include "Game/Fireworks.h"
#include "Game/Fangs.h"
#include "Game/Paintings.h"
#include "Game/ArmorStands.h"
#include "Game/Discs.h"
#include "World/Village.h"
#include "Game/VillageLoot.h"
#include "Game/StructureLoot.h"
#include "Game/Enchanting.h"
#include "Game/Scoreboard.h"
#include "Game/Attributes.h"
#include "Game/Trading.h"
#include "Game/EndPortal.h"
#include "Game/Weather.h"
#include "Game/Portal.h"
#include "World/Furnaces.h"
#include "World/Chests.h"
#include "World/WorldGen.h"
#include "Entity/Projectiles.h"
#include "Entity/HeldItem.h"
#include "Entity/MobModel.h"
#include "Entity/MobSkin.h"
#include "Entity/MobType.h"
#include "Entity/EntityManager.h"
#include "Entity/Pathfinder.h"
#include "Game/Chat.h"
#include "Game/Fishing.h"
#include "Game/Food.h"
#include "Game/Raids.h"
#include "Game/RecipeBook.h"
#include "Game/Advancements.h"
#include "Game/Stats.h"
#include "Game/Worlds.h"
#include "Game/Servers.h"
#include "Net/Ping.h"
#include "Net/Protocol.h"
#include "Core/Packs.h"
#include "Game/Golems.h"
#include "Renderer/Atlas.h"
#include "Renderer/UIRenderer.h"
#include "Renderer/FontPaint.h"
#include "Renderer/BorderRenderer.h"
#include "Renderer/ShaderPipeline.h"
#include <stb_image.h>
#include "World/WorldBorder.h"
#include "World/BiomeColours.h"
#include "World/ChunkMesher.h"
#include "Core/Utf8.h"
#include "Entity/HeldItem.h"
#include "Audio/AudioEngine.h"
#include "Player/Player.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include <array>
#include <map>
#include <tuple>
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

    void testCaveAndSpikeFeatures()
    {
        section("ice spikes and large dripstone");
        const WorldGen generator(2024u);
        auto chunkOf = [](int v, int size) { return v >= 0 ? v / size : -((-v + size - 1) / size); };
        int spikeX = 0, spikeZ = 0;
        bool icy = false;
        for (int r = 0; r < 4000 && !icy; r += 32)
            for (int a = 0; a < 16 && !icy; ++a)
            {
                const int x = static_cast<int>(std::cos(a * 0.3927f) * r), z = static_cast<int>(std::sin(a * 0.3927f) * r);
                if (generator.kindAt(x, z) == Biomes::Kind::IceSpikes && generator.kindAt(x + 40, z) == Biomes::Kind::IceSpikes &&
                    generator.kindAt(x, z + 40) == Biomes::Kind::IceSpikes)
                {
                    spikeX = x;
                    spikeZ = z;
                    icy = true;
                }
            }
        check(icy, "there is an ice spikes plain nearby");
        if (icy)
        {
            int tallest = 0;
            for (int dz = -1; dz <= 1; ++dz)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    Chunk chunk(ChunkPos{ chunkOf(spikeX, Chunk::SX) + dx, chunkOf(spikeZ, Chunk::SZ) + dz });
                    generator.generate(chunk);
                    for (int lz = 0; lz < Chunk::SZ; ++lz)
                        for (int lx = 0; lx < Chunk::SX; ++lx)
                        {
                            const int ground = generator.surfaceHeight(chunk.position().x * Chunk::SX + lx, chunk.position().z * Chunk::SZ + lz);
                            for (int y = Chunk::SY - 1; y > ground; --y)
                                if (chunk.getBlock(lx, y, lz) == Blocks::PackedIce)
                                {
                                    tallest = std::max(tallest, y - ground);
                                    break;
                                }
                        }
                }
            check(tallest >= 6, "and its spikes stand at least six blocks off the snow");
        }

        int cones = 0;
        for (int cz = -8; cz <= 8 && cones == 0; ++cz)
            for (int cx = -8; cx <= 8 && cones == 0; ++cx)
            {
                if (generator.caveKindAt(cx * Chunk::SX + 8, cz * Chunk::SZ + 8) != WorldGen::CaveKind::Dripstone) continue;
                Chunk chunk(ChunkPos{ cx, cz });
                generator.generate(chunk);
                for (int lz = 1; lz < Chunk::SZ - 1; ++lz)
                    for (int lx = 1; lx < Chunk::SX - 1; ++lx)
                        for (int y = 6; y < Chunk::SY - 4; ++y)
                        {
                            if (chunk.getBlock(lx, y, lz) != Blocks::DripstoneBlock || chunk.getBlock(lx, y + 1, lz) != Blocks::DripstoneBlock ||
                                chunk.getBlock(lx, y + 2, lz) != Blocks::DripstoneBlock)
                                continue;
                            int open = 0;
                            for (const auto& [ox, oz] : { std::pair{ 1, 0 }, std::pair{ -1, 0 }, std::pair{ 0, 1 }, std::pair{ 0, -1 } })
                                if (chunk.getBlock(lx + ox * 2, y + 1, lz + oz * 2) == Blocks::Air) ++open;
                            if (open >= 2) ++cones;
                        }
            }
        check(cones > 0, "dripstone caves grow large dripstone columns");
    }

    void testBigTrees()
    {
        section("big trees");
        struct Grid : Saplings::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> blocks;
            BlockId at(int x, int y, int z) const override
            {
                const auto found = blocks.find({ x, y, z });
                return found == blocks.end() ? (y <= 0 ? static_cast<BlockId>(Blocks::Grass) : static_cast<BlockId>(Blocks::Air)) : found->second;
            }
            void set(int x, int y, int z, BlockId id) override { blocks[{ x, y, z }] = id; }
            int count(BlockId id) const
            {
                int n = 0;
                for (const auto& [at, block] : blocks) n += block == id ? 1 : 0;
                return n;
            }
        };
        Grid spruce;
        check(Saplings::bigTree(spruce, glm::ivec3(0, 1, 0), 1, 77u, true), "a mega spruce grows");
        check(spruce.at(0, 1, 0) == logOfWood(1) && spruce.at(1, 1, 1) == logOfWood(1) && spruce.at(0, 14, 1) == logOfWood(1),
              "on a two-by-two trunk at least fourteen tall");
        check(spruce.count(leavesOfWood(1)) > 60, "under a cone of needles");
        Grid dark;
        check(Saplings::bigTree(dark, glm::ivec3(0, 1, 0), 5, 91u, true) && dark.at(1, 1, 0) == logOfWood(5),
              "a dark oak stands on four logs");
        Grid oak;
        check(Saplings::fancyOak(oak, glm::ivec3(0, 1, 0), 123u, true), "a fancy oak grows");
        int offTrunk = 0;
        for (const auto& [at, block] : oak.blocks)
            if (block == logOfWood(0) && (std::get<0>(at) != 0 || std::get<2>(at) != 0)) ++offTrunk;
        check(offTrunk >= 2 && oak.count(leavesOfWood(0)) > 40, "with branches off its trunk and balls of leaves");
    }

    void testDripKinds()
    {
        section("drips fall and land as their own liquid");
        using K = SpriteParticles::Kind;
        check(SpriteParticles::fallOf(K::DripHangHoney) == K::DripFallHoney && SpriteParticles::landOf(K::DripFallHoney) == K::DripLandHoney,
              "honey hangs, falls and lands as honey");
        check(SpriteParticles::fallOf(K::DripHangTear) == K::DripFallTear && SpriteParticles::landOf(K::DripFallTear) == K::DripLandTear,
              "crying obsidian tears do the same");
        check(SpriteParticles::fallOf(K::DripHangWater) == K::DripWater && SpriteParticles::landOf(K::DripWater) == K::Splash,
              "water still splashes where it lands");
        check(SpriteParticles::fallOf(K::Flame) == K::Count && SpriteParticles::landOf(K::FallingSpore) == K::Count,
              "and nothing else turns into a drip");
    }

    void testSoundCategories()
    {
        section("sound categories");
        check(AudioEngine::categoryOf(Sound::MobZombieHurt) == SoundCategory::Hostile, "a zombie's hurt sound is hostile");
        check(AudioEngine::categoryOf(Sound::MobIllusionerDeath) == SoundCategory::Hostile, "and so is an illusioner's death");
        check(AudioEngine::categoryOf(Sound::MobCowSay) == SoundCategory::Neutral, "a cow is a friendly creature");
        check(AudioEngine::categoryOf(Sound::DigStone) == SoundCategory::Blocks && AudioEngine::categoryOf(Sound::StepSnow) == SoundCategory::Blocks,
              "digging and steps are block sounds");
        check(AudioEngine::categoryOf(Sound::Rain) == SoundCategory::Weather, "rain is weather");
        check(AudioEngine::categoryOf(Sound::NoteFirst) == SoundCategory::Records && AudioEngine::categoryOf(Sound::GoatHornFirst) == SoundCategory::Records,
              "note blocks and goat horns sit with the jukebox");
        check(AudioEngine::categoryOf(Sound::Hurt) == SoundCategory::Players, "your own hurt sound is a player sound");
        check(std::string(AudioEngine::categoryKey(SoundCategory::Records)) == "record" &&
                  std::string(AudioEngine::categoryKey(SoundCategory::Neutral)) == "neutral",
              "categories save under their options.txt names");
        check(!AudioEngine::soundFileExists("no/such/sound.ogg"), "a missing music file is noticed before it is streamed");
    }

    void testChestLids()
    {
        section("chest lids open");
        auto highest = [](const std::vector<float>& v) {
            float top = -1.0f;
            for (size_t i = 1; i < v.size(); i += 10) top = std::max(top, v[i]);
            return top;
        };
        auto towards = [](const std::vector<float>& v, int axis) {
            float lo = 99.0f, hi = -99.0f;
            for (size_t i = static_cast<size_t>(axis); i < v.size(); i += 10)
            {
                lo = std::min(lo, v[i]);
                hi = std::max(hi, v[i]);
            }
            return std::make_pair(lo, hi);
        };
        std::vector<float> shut, open;
        appendChestLid(chestOf(0, 0, 0), glm::vec3(0.0f), 0.0f, 1.0f, 0.0f, shut);
        appendChestLid(chestOf(0, 0, 0), glm::vec3(0.0f), 1.0f, 1.0f, 0.0f, open);
        check(!shut.empty() && std::fabs(highest(shut) - 14.0f / 16.0f) < 0.001f, "a shut lid sits fourteen pixels up");
        check(std::fabs(highest(open) - 24.0f / 16.0f) < 0.01f, "an open lid stands up from its hinge at the back, latch on top");
        check(towards(open, 2).first > 13.0f / 16.0f - 0.01f, "and leans over the back of the chest, away from its front");
        std::vector<float> east;
        appendChestLid(chestOf(0, 1, 0), glm::vec3(0.0f), 1.0f, 1.0f, 0.0f, east);
        check(towards(east, 0).second < 3.0f / 16.0f + 0.01f, "a chest facing east hinges on its west side");
        std::vector<float> lit;
        appendChestLid(chestOf(2, 0, 0), glm::vec3(5.0f, 6.0f, 7.0f), 0.5f, 0.25f, 0.75f, lit);
        check(!lit.empty() && lit[6] == 0.25f && lit[7] == 0.75f && lit[0] >= 5.0f && lit[0] <= 6.0f,
              "an ender chest lid is placed at its cell and lit by it");
        std::vector<float> still, swung;
        appendBellBody(Blocks::BellFirst, glm::vec3(0.0f), 0.0f, 0.0f, 1.0f, 0.0f, still);
        appendBellBody(Blocks::BellFirst, glm::vec3(0.0f), -0.3f, 0.0f, 1.0f, 0.0f, swung);
        float stillLow = 99.0f, swungZ = 0.0f;
        for (size_t i = 1; i < still.size(); i += 10) stillLow = std::min(stillLow, still[i]);
        for (size_t i = 2; i < swung.size(); i += 10) swungZ += swung[i];
        swungZ /= static_cast<float>(std::max<size_t>(1, swung.size() / 10));
        check(!still.empty() && std::fabs(stillLow - 4.0f / 16.0f) < 0.001f, "a bell hangs with its lip four pixels up");
        check(swungZ > 0.53f, "and a ring pushed south swings it south");
        std::vector<float> shulker;
        appendShulkerLid(Blocks::ShulkerBox, glm::vec3(0.0f), 1.0f, 1.0f, 0.0f, shulker);
        float shulkerLow = 99.0f, shulkerHigh = -99.0f;
        for (size_t i = 1; i < shulker.size(); i += 10)
        {
            shulkerLow = std::min(shulkerLow, shulker[i]);
            shulkerHigh = std::max(shulkerHigh, shulker[i]);
        }
        check(!shulker.empty() && std::fabs(shulkerLow - 1.0f) < 0.001f && std::fabs(shulkerHigh - 1.5f) < 0.001f,
              "an open shulker box lifts its lid half a block");
        setLidLifted(1, 2, 3, true);
        check(lidLifted(1, 2, 3) && !lidLifted(1, 2, 4), "a lifted lid is remembered by its cell");
        clearLiftedLids();
        check(!lidLifted(1, 2, 3), "and forgotten when the world goes");
    }

    void testInventoryShortcuts()
    {
        section("inventory: drag, double-click, number keys and Q");

        Inventory spread;
        spread.cursor() = ItemStack{ Blocks::Stone, 64 };
        spread.slot(Inventory::HOTBAR_SLOTS + 2) = ItemStack{ Blocks::Dirt, 5 };
        check(spread.spread({ Inventory::HOTBAR_SLOTS, Inventory::HOTBAR_SLOTS + 1, Inventory::HOTBAR_SLOTS + 2, 3 }, false),
              "a left drag over slots spreads the stack");
        check(spread.slot(Inventory::HOTBAR_SLOTS).count == 21 && spread.slot(Inventory::HOTBAR_SLOTS + 1).count == 21 &&
                  spread.slot(3).count == 21,
              "evenly over the slots that can take it");
        check(spread.slot(Inventory::HOTBAR_SLOTS + 2).id == Blocks::Dirt && spread.slot(Inventory::HOTBAR_SLOTS + 2).count == 5,
              "skipping a slot holding something else");
        check(spread.cursor().count == 1, "and the remainder stays on the cursor");

        Inventory ones;
        ones.cursor() = ItemStack{ Items::Diamond, 2 };
        ones.spread({ 0, 1, 2, 4 }, true);
        check(ones.slot(0).count == 1 && ones.slot(1).count == 1 && ones.slot(2).empty() && ones.cursor().empty(),
              "a right drag puts one in each slot until the cursor runs out");

        Inventory topUp;
        topUp.slot(0) = ItemStack{ Blocks::Stone, 60 };
        topUp.cursor() = ItemStack{ Blocks::Stone, 20 };
        topUp.spread({ 0, 1 }, false);
        check(topUp.slot(0).count == 64 && topUp.slot(1).count == 10 && topUp.cursor().count == 6,
              "a drag tops a stack up to its limit and keeps what does not fit");

        Inventory armour;
        armour.cursor() = ItemStack{ Blocks::Stone, 10 };
        check(!armour.canSpreadInto(Inventory::ARMOR_FIRST) && !armour.canSpreadInto(Inventory::CRAFT_RESULT) &&
                  !armour.canSpreadInto(Inventory::FURNACE_OUTPUT),
              "a drag never drops stone into armour or result slots");

        Inventory gather;
        gather.slot(0) = ItemStack{ Blocks::Stone, 64 };
        gather.slot(5) = ItemStack{ Blocks::Stone, 10 };
        gather.slot(9) = ItemStack{ Blocks::Stone, 20 };
        gather.slot(12) = ItemStack{ Blocks::Dirt, 30 };
        gather.cursor() = ItemStack{ Blocks::Stone, 4 };
        std::vector<int> all;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i) all.push_back(i);
        check(gather.gather(all) == 60, "a double-click gathers the matching stacks onto the cursor");
        check(gather.cursor().count == 64 && gather.slot(5).empty() && gather.slot(9).empty() && gather.slot(0).count == 34,
              "part stacks first, then full ones only to fill the cursor");
        check(gather.slot(12).count == 30, "and nothing of another kind");

        Inventory keys;
        keys.slot(Inventory::HOTBAR_SLOTS + 4) = ItemStack{ Blocks::Stone, 12 };
        keys.slot(2) = ItemStack{ Items::Diamond, 3 };
        check(keys.swapSlots(Inventory::HOTBAR_SLOTS + 4, 2), "a number key swaps the hovered slot with the hotbar");
        check(keys.slot(2).id == Blocks::Stone && keys.slot(Inventory::HOTBAR_SLOTS + 4).id == Items::Diamond,
              "both ways");
        keys.slot(Inventory::ARMOR_FIRST) = ItemStack{};
        check(!keys.swapSlots(Inventory::ARMOR_FIRST, 2), "and stone does not go onto your head that way");
        keys.slot(Inventory::FURNACE_OUTPUT) = ItemStack{ Items::IronIngot, 3 };
        keys.slot(7).clear();
        check(keys.swapSlots(Inventory::FURNACE_OUTPUT, 7) && keys.slot(7).count == 3 && keys.slot(Inventory::FURNACE_OUTPUT).empty(),
              "a furnace's output goes to the hotbar slot");
        keys.slot(Inventory::OFFHAND).clear();
        check(keys.swapSlots(2, Inventory::OFFHAND) && keys.slot(Inventory::OFFHAND).id == Blocks::Stone,
              "and F puts the hovered stack in the offhand");

        Inventory drop;
        drop.slot(10) = ItemStack{ Blocks::Stone, 9 };
        const ItemStack one = drop.throwFrom(10, false);
        check(one.id == Blocks::Stone && one.count == 1 && drop.slot(10).count == 8, "Q throws one item from the hovered slot");
        const ItemStack rest = drop.throwFrom(10, true);
        check(rest.count == 8 && drop.slot(10).empty(), "and Ctrl+Q the whole stack");
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
        check(birch.slot(Inventory::CRAFT_RESULT).id == Blocks::BirchPlanks, "birch log makes birch planks");

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

    // What a mob actually occupies, as against what its model is drawn
    // at. Mojang draws several of them at a scale -- a guardian's model
    // is twice its box, an elder guardian's is half of its -- so a
    // comparison between two species has to use the scaled size or it
    // is comparing drawings rather than animals.
    float standsAt(const MobType& t) { return t.height * t.modelScale; }
    float spans(const MobType& t) { return t.width * t.modelScale; }

    void testMobTypes()
    {
        section("mobs: species");

        // Counted rather than named, so adding a species does not fail
        // a test that was only ever recording how many there were.
        check(mobTypeCount() == static_cast<int>(MobId::Count), "every species has a type");
        check(mobTypeCount() >= 8, "and there are at least the original eight");

        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobId id = static_cast<MobId>(i);
            const MobType& type = mobType(id);
            const std::string who = type.name;

            check(type.name && *type.name, "species " + std::to_string(i) + " is named");
            check(type.maxHealth > 0, who + " can be killed");
            check(type.width > 0.0f && type.height > 0.0f, who + " has a collision box");
            // Everything walks but the one thing that is bolted down.
            if (type.stationary)
                check(type.walkSpeed == 0.0f,
                      who + " is bolted where it stands, so it has no walk speed");
            else
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
            //
            // Nothing that hovers has feet to put there. Vanilla draws
            // a blaze floating inside its own box, so what is asked of
            // one instead is that it stays inside the box it floats in.
            if (type.hoverHeight > 0.0f)
                check(bottom >= 0.0f && bottom < type.height * 0.5f,
                      who + " hangs inside its own box");
            else
                check(bottom > -0.2f && bottom < 0.05f, who + " stands on the ground");

            // Every species must fit its sheet, or its texture coordinates
            // silently wrap onto another limb.
            // 64x128 is on the list because that is what Minecraft
            // draws a witch and a villager on. It was missing until
            // the witch went in and this caught her.
            // Mojang's own sheet shapes, which are not all powers of
            // two square: the fox is drawn on 48x32, the frog on
            // 48x48 and the dragon on 256x256.
            const bool shaped =
                (type.sheetWidth == 16 && type.sheetHeight == 16) ||
                (type.sheetWidth == 32 && type.sheetHeight == 32) ||
                (type.sheetWidth == 48 &&
                 (type.sheetHeight == 32 || type.sheetHeight == 48)) ||
                (type.sheetWidth == 64 &&
                 (type.sheetHeight == 32 || type.sheetHeight == 64 ||
                  type.sheetHeight == 128)) ||
                (type.sheetWidth == 128 &&
                 (type.sheetHeight == 64 || type.sheetHeight == 128)) ||
                (type.sheetWidth == 192 && type.sheetHeight == 192) ||
                (type.sheetWidth == 256 && type.sheetHeight == 256);
            check(shaped, who + " has a sheet the shape Minecraft draws on");
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
        // Only the living. A boat creaks when it breaks and is silent
        // otherwise -- it has no say of its own to share with anything.
        bool distinct = true;
        for (int a = 0; a < mobTypeCount(); ++a)
            for (int b = a + 1; b < mobTypeCount(); ++b)
            {
                const MobType& x = mobType(static_cast<MobId>(a));
                const MobType& y = mobType(static_cast<MobId>(b));
                if (x.inanimate || y.inanimate) continue;
                if (std::string(x.name) == y.name) continue;
                if (x.id == MobId::TraderLlama || y.id == MobId::TraderLlama) continue;
                if (x.id == MobId::Mooshroom || y.id == MobId::Mooshroom || x.id == MobId::Giant || y.id == MobId::Giant) continue;
                if (x.voice == y.voice || x.hurtVoice == y.hurtVoice ||
                    x.deathVoice == y.deathVoice) distinct = false;
            }
        check(distinct, "every living species has its own say, hurt and death");
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

            // A hostile mob must be able to fight. A passive one must
            // not, unless it is neutral -- a golem belongs to its
            // village and spawns in daylight like any other resident,
            // but it finishes fights it did not start.
            if (t.spawnClass == SpawnClass::Hostile)
                check(fights, who + " fights back somehow");
            else if (t.neutral)
                check(fights, who + " is neutral, so it had better be able to fight");
            else
                check(!fights, who + " does not attack");

            // Whatever its class, anything neutral must hold its hand
            // until it is given a reason -- and anything that cannot
            // fight is never hostile however badly it is treated.
            if (t.neutral)
            {
                check(!Mob::hostileNow(t, false),
                      who + " leaves you alone until you start something");
                check(Mob::hostileNow(t, true), "and comes for you once you have");
            }
            else if (!fights)
            {
                check(!Mob::hostileNow(t, true),
                      who + " cannot turn on you, because it has nothing to turn with");
            }
            else
            {
                check(Mob::hostileNow(t, false), who + " needs no reason");
            }

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

            // Nothing from the Nether burns in a sun it never sees, and
            // nothing from the overworld belongs down there.
            if (t.dimension == Dimension::Nether)
                check(!t.burnsInSunlight, who + " has no sun to burn in");
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

        // The whole design rests on the two id ranges never meeting.
        // Blocks have everything below Items::FIRST and items have
        // everything above it, which is what lets one stack hold
        // either.
        //
        // This used to read "FIRST == 256", because a block id was a
        // byte and 256 was one past every value one could hold. Both
        // moved when the game ran out of blocks at the lantern. What
        // has to be true is not a particular number but that the
        // blocks have not reached the items, with room left over.
        check(static_cast<int>(Blocks::Count) < Items::FIRST,
              "the blocks have not reached the items");
        check(Items::FIRST - static_cast<int>(Blocks::Count) > 64,
              "and there is room left for more of them");
        check(!isItem(Blocks::Air) && !isItem(Blocks::Stone), "blocks are not items");
        check(isBlockStack(Blocks::Stone), "and blocks read as blocks");
        check(isItem(Items::Wheat) && isItem(Items::Gunpowder), "items are items");
        check(!isBlockStack(Items::Wheat), "and items are not blocks");
        check(!isItem(Items::Count), "nothing past the last item is one");

        // Narrowing an item to a BlockId is the mistake this range
        // exists to catch. While a block id was a byte the value
        // visibly changed, which is what this used to check; with
        // sixteen bits it survives the cast intact, so what protects
        // against it now is that the result is not a block -- the
        // table is bounds-checked and hands back air rather than
        // reading past its end.
        check(static_cast<int>(Items::Wheat) >= static_cast<int>(Blocks::Count),
              "an item narrowed to a block id is not a block");
        check(&blockInfo(static_cast<BlockId>(Items::Wheat)) == &blockInfo(Blocks::Air),
              "and asking the block table about one gives air, not whatever is there");

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

    void testVillages()
    {
        section("villages: where they stand and what is in them");

        check(Village::regionOf(0) == 0, "the origin is in square zero");
        check(Village::regionOf(Village::REGION - 1) == 0, "and so is the block before the next");
        check(Village::regionOf(Village::REGION) == 1, "which starts square one");
        check(Village::regionOf(-1) == -1, "one west of the origin is square minus one");
        check(Village::regionOf(-Village::REGION) == -1, "and so is the far side of it");
        check(Village::regionOf(-Village::REGION - 1) == -2,
              "squares either side of the origin are the same width as the rest");

        const uint32_t seed = 4242u;

        // The same square always answers the same way, whoever asks.
        for (int rz = -3; rz <= 3; ++rz)
            for (int rx = -3; rx <= 3; ++rx)
            {
                const Village::Site a = Village::siteFor(rx, rz, seed);
                const Village::Site b = Village::siteFor(rx, rz, seed);
                check(a.exists == b.exists && a.x == b.x && a.z == b.z,
                      "a square gives the same village every time it is asked");
            }

        // Minecraft's rule, which is not the one this used to use: a
        // village lands anywhere in the first (spacing - separation)
        // chunks of its square, and the separation left over at the far
        // end is what guarantees the gap. Checking it sat in the middle
        // of its square was checking a scheme of our own.
        const int window = (Structures::VILLAGE.spacing - Structures::VILLAGE.separation) *
                           Structures::CHUNK;

        int found = 0;
        std::vector<Village::Site> sites;
        for (int rz = -4; rz <= 4; ++rz)
            for (int rx = -4; rx <= 4; ++rx)
            {
                const Village::Site site = Village::siteFor(rx, rz, seed);
                if (!site.exists) continue;
                ++found;
                sites.push_back(site);

                const int lowX = rx * Village::REGION;
                check(site.x >= lowX && site.x < lowX + window,
                      "a village lands in its square's placement window");
                const int lowZ = rz * Village::REGION;
                check(site.z >= lowZ && site.z < lowZ + window, "on both axes");
            }
        check(found > 10, "a stretch of world that size holds a good few villages");

        // And the guarantee that window buys: no two can be closer than
        // the separation, however the dice fall.
        const int apart = Structures::VILLAGE.separation * Structures::CHUNK;
        for (size_t i = 0; i < sites.size(); ++i)
            for (size_t j = i + 1; j < sites.size(); ++j)
            {
                const int gapX = std::abs(sites[i].x - sites[j].x);
                const int gapZ = std::abs(sites[i].z - sites[j].z);
                check(gapX >= apart || gapZ >= apart,
                      "and no two villages are closer than the separation allows");
                check(gapX > Village::MAX_RADIUS * 2 || gapZ > Village::MAX_RADIUS * 2,
                      "so none of them ever reaches another");
            }

        // --- the layout ---------------------------------------------
        Village::Site site;
        site.exists = true;
        site.x = 1000;
        site.z = -2000;

        const std::vector<Village::Building> plan = Village::plan(site, seed);
        check(!plan.empty(), "a village has something in it");

        int houses = 0;
        for (const Village::Building& b : plan)
        {
            if (Village::isHouse(b.kind)) ++houses;

            check(b.width > 0 && b.depth > 0, "everything in it takes up room");

            // Nothing reaches past what a chunk was told to look for,
            // or a building would be cut in half at a chunk border.
            check(b.x >= site.x - Village::MAX_RADIUS, "nothing stands west of the village");
            check(b.x + b.width - 1 <= site.x + Village::MAX_RADIUS, "nor east of it");
            check(b.z >= site.z - Village::MAX_RADIUS, "nor north");
            check(b.z + b.depth - 1 <= site.z + Village::MAX_RADIUS, "nor south");
        }
        check(houses > 0, "houses in it");

        // The middle plot is left open: the paved square the paths put
        // there is the middle of the village, and the well that used to
        // stand on it read as a shed in the way.
        for (const Village::Building& b : plan)
        {
            const bool coversMiddle = site.x >= b.x && site.x < b.x + b.width &&
                                      site.z >= b.z && site.z < b.z + b.depth;
            check(!coversMiddle, "nothing is built on the middle of the village");
        }

        // Every frontage stands the same step back from its street,
        // which is what makes a row of different-sized houses line up.
        for (const Village::Building& b : plan)
        {
            if (!Village::isHouse(b.kind)) continue;

            const int awayX = b.x + b.width / 2 - site.x;
            const int awayZ = b.z + b.depth / 2 - site.z;
            const int plotX = (awayX >= 0 ? (awayX + Village::PLOT / 2) / Village::PLOT
                                          : -((-awayX + Village::PLOT / 2) / Village::PLOT));
            const int plotZ = (awayZ >= 0 ? (awayZ + Village::PLOT / 2) / Village::PLOT
                                          : -((-awayZ + Village::PLOT / 2) / Village::PLOT));

            const int edgeX = site.x + plotX * Village::PLOT - Village::PLOT / 2;
            const int edgeZ = site.z + plotZ * Village::PLOT - Village::PLOT / 2;

            int gap = -1;
            switch (b.facing)
            {
                case 0:  gap = b.z - edgeZ; break;
                case 1:  gap = (edgeX + Village::PLOT - 1) - (b.x + b.width - 1); break;
                case 2:  gap = (edgeZ + Village::PLOT - 1) - (b.z + b.depth - 1); break;
                default: gap = b.x - edgeX; break;
            }
            check(gap == 1, "every frontage is the same step back from its street");
        }

        // Nothing overlaps anything else: two buildings sharing ground
        // would leave one half-built inside the other.
        for (size_t i = 0; i < plan.size(); ++i)
            for (size_t j = i + 1; j < plan.size(); ++j)
            {
                const Village::Building& a = plan[i];
                const Village::Building& b = plan[j];
                const bool apart = a.x + a.width <= b.x || b.x + b.width <= a.x ||
                                   a.z + a.depth <= b.z || b.z + b.depth <= a.z;
                check(apart, "no two buildings stand on the same ground");
            }

        // Every door looks towards the middle, so it opens onto a path
        // rather than into the back of the next house.
        for (const Village::Building& b : plan)
        {
            if (!Village::isHouse(b.kind)) continue;
            const int cx = b.x + b.width / 2;
            const int cz = b.z + b.depth / 2;

            const bool right = (b.facing == 0 && cz > site.z) ||
                               (b.facing == 2 && cz < site.z) ||
                               (b.facing == 1 && cx < site.x) ||
                               (b.facing == 3 && cx > site.x);
            check(right, "every door looks towards the well");
        }

        // reaches() has to agree with where the buildings actually are,
        // or a chunk would skip a village that does reach into it.
        for (const Village::Building& b : plan)
            check(Village::reaches(site, b.x, b.z, b.x + b.width - 1, b.z + b.depth - 1),
                  "a village reaches every box it has a building in");

        Village::Site nowhere;
        check(Village::plan(nowhere, seed).empty(), "a square with no village plans nothing");
        check(!Village::reaches(nowhere, -10000, -10000, 10000, 10000),
              "and reaches nowhere at all");
    }

    void testVillageLife()
    {
        section("villages: beds, trades and pairing off");

        const uint32_t seed = 4242u;

        Village::Site site;
        site.exists = true;
        site.x = 500;
        site.z = 500;

        // One bed to a house, which is what a village counts.
        const std::vector<Village::Building> plan = Village::plan(site, seed);
        int houses = 0;
        for (const Village::Building& b : plan)
            if (Village::isHouse(b.kind)) ++houses;

        check(Village::bedCount(site, seed) == houses, "a village has one bed to a house");
        check(Village::bedCount(site, seed) > 0, "and at least one of them");

        Village::Site nowhere;
        check(Village::bedCount(nowhere, seed) == 0, "a village that is not there has none");

        // A villager keeps the trade it was handed, and falls back to
        // the one its seed draws when nobody handed it one.
        {
            Mob villager(MobId::Villager, glm::vec3(0.0f), 777u);
            check(villager.professionSlot() < 0, "a villager starts with no trade of its own");

            villager.setProfessionSlot(static_cast<int>(Trading::Profession::Toolsmith));
            check(villager.professionSlot() == static_cast<int>(Trading::Profession::Toolsmith),
                  "and keeps the one it is given");
        }

        // Willing without being fed, which is how villagers pair off.
        {
            Mob villager(MobId::Villager, glm::vec3(0.0f), 1u);
            check(!villager.inLove(), "a villager is not willing to begin with");
            villager.makeWilling();
            check(villager.inLove(), "and is once the village has room");

            villager.onBred();
            check(!villager.inLove(), "pairing off spends it");

            villager.makeWilling();
            check(!villager.inLove(),
                  "and the cooldown after holds, or one pair would fill a village");
        }
        {
            // A baby is nobody's parent.
            Mob calf(MobId::Villager, glm::vec3(0.0f), 2u, true);
            calf.makeWilling();
            check(!calf.inLove(), "a child does not pair off");
        }
        {
            Mob dead(MobId::Villager, glm::vec3(0.0f), 3u);
            dead.damage(1000, glm::vec3(0.0f));
            dead.makeWilling();
            check(!dead.inLove(), "nor does a dead one");
        }

        // Every village grows something, and a field is not a house:
        // nobody lives in it, so it has no bed.
        {
            int farms = 0, withFarms = 0, villages = 0;
            for (int rz = -5; rz <= 5; ++rz)
                for (int rx = -5; rx <= 5; ++rx)
                {
                    const Village::Site each = Village::siteFor(rx, rz, seed);
                    if (!each.exists) continue;
                    ++villages;

                    int here = 0;
                    for (const Village::Building& b : Village::plan(each, seed))
                        if (b.kind == Village::Kind::Farm) ++here;

                    farms += here;
                    if (here) ++withFarms;
                }

            check(farms > 0, "villages grow something");
            check(withFarms < villages, "though not every one of them does");
            check(!Village::isHouse(Village::Kind::Farm), "a field is not a house");
            check(VillageLoot::rollFor(Village::Kind::Farm, 0, 70, 0, seed).empty(),
                  "and has no chest to fill");
        }

        // The path is ordinary ground: it digs like dirt, and a village
        // that laid it over water or a stone outcrop would read as a
        // bug rather than a road.
        {
            check(blockInfo(Blocks::DirtPath).collides, "a path is something to stand on");
            check(blockInfo(Blocks::DirtPath).hardness > 0.0f, "and can be dug up");
            check(Tools::requiredLevel(Blocks::DirtPath) == 0, "by hand, like the dirt it is");
        }

        // Villagers belong to their village and turn back at the edge.
        {
            const glm::vec3 middle(100.0f, 70.0f, -40.0f);
            const float radius = static_cast<float>(Village::MAX_RADIUS);

            Mob villager(MobId::Villager, middle, 9u);
            check(!villager.hasHome(), "a villager starts belonging nowhere");
            check(!Mob::wouldTurnBack(middle, middle, 0.0f),
                  "and one with nowhere to belong never turns back");

            villager.setHome(middle, radius);
            check(villager.hasHome(), "and belongs once it is given somewhere");

            check(!Mob::wouldTurnBack(middle, middle, radius),
                  "standing in the middle, it carries on");
            check(!Mob::wouldTurnBack(middle + glm::vec3(radius - 2.0f, 0.0f, 0.0f),
                                      middle, radius),
                  "and inside the edge too");
            check(Mob::wouldTurnBack(middle + glm::vec3(radius + 5.0f, 0.0f, 0.0f),
                                     middle, radius),
                  "but turns back once past it");
            check(Mob::wouldTurnBack(middle + glm::vec3(0.0f, 0.0f, radius + 5.0f),
                                     middle, radius),
                  "on either axis");
            check(!Mob::wouldTurnBack(middle + glm::vec3(0.0f, 400.0f, 0.0f),
                                      middle, radius),
                  "and height is not distance: straight up is still home");
        }

        // Asleep at night, awake by day, and awake whatever the hour
        // once something has hit it.
        {
            Mob villager(MobId::Villager, glm::vec3(0.0f, 70.0f, 0.0f), 4u);
            check(!villager.asleep(), "a villager starts awake");

            check(!Mob::sleepsAt(MobId::Villager, 1.0f, false), "awake in broad daylight");
            check(!Mob::sleepsAt(MobId::Villager, Mob::SLEEP_BELOW, false),
                  "and still awake at the edge of the light");
            check(Mob::sleepsAt(MobId::Villager, 0.0f, false), "asleep once the light goes");
            check(!Mob::sleepsAt(MobId::Villager, 0.0f, true),
                  "but awake whatever the hour once something has hit it");

            for (MobId other : { MobId::Cow, MobId::Pig, MobId::Zombie, MobId::Skeleton })
                check(!Mob::sleepsAt(other, 0.0f, false),
                      std::string(mobType(other).name) + " does not sleep");
        }
    }

    void testVillageLoot()
    {
        section("villages: the trades and what is in their chests");

        const uint32_t seed = 4242u;

        Village::Site site;
        site.exists = true;
        site.x = 1000;
        site.z = -2000;

        // Across a spread of villages: at most one smith and one
        // library each, and never the same plot for both.
        int withSmith = 0, withLibrary = 0, villages = 0;
        for (int rz = -6; rz <= 6; ++rz)
            for (int rx = -6; rx <= 6; ++rx)
            {
                const Village::Site each = Village::siteFor(rx, rz, seed);
                if (!each.exists) continue;
                ++villages;

                int smiths = 0, libraries = 0;
                for (const Village::Building& b : Village::plan(each, seed))
                {
                    if (b.kind == Village::Kind::Blacksmith) ++smiths;
                    if (b.kind == Village::Kind::Library) ++libraries;
                }
                check(smiths <= 1, "a village has at most one smith");
                check(libraries <= 1, "and at most one library");
                if (smiths) ++withSmith;
                if (libraries) ++withLibrary;
            }

        check(villages > 20, "a good few villages to look at");
        check(withSmith > 0 && withSmith < villages,
              "some villages have a smith and some do not");
        check(withLibrary > 0 && withLibrary < villages,
              "and the same for libraries");

        // The plan is still the plan: nothing overlaps now the trades
        // have been handed out.
        const std::vector<Village::Building> plan = Village::plan(site, seed);
        for (size_t i = 0; i < plan.size(); ++i)
            for (size_t j = i + 1; j < plan.size(); ++j)
            {
                const Village::Building& a = plan[i];
                const Village::Building& b = plan[j];
                const bool apart = a.x + a.width <= b.x || b.x + b.width <= a.x ||
                                   a.z + a.depth <= b.z || b.z + b.depth <= a.z;
                check(apart, "no two buildings stand on the same ground");
            }

        for (const Village::Building& b : plan)
            check(Village::name(b.kind)[0] != '\0', "every kind of building has a name");

        // buildingAt has to agree with the plan, or a chest would be
        // filled from the wrong trade's table.
        for (const Village::Building& b : plan)
        {
            Village::Building found;
            check(Village::buildingAt(site, seed, b.x, b.z, found),
                  "a block inside a building finds it");
            check(found.kind == b.kind && found.x == b.x && found.z == b.z,
                  "and finds the right one");
        }
        {
            Village::Building found;
            check(!Village::buildingAt(site, seed, site.x + 100000, site.z, found),
                  "and a block far outside finds nothing");
        }

        // --- the loot ------------------------------------------------
        for (Village::Kind kind : { Village::Kind::Home, Village::Kind::Blacksmith,
                                    Village::Kind::Library, Village::Kind::Butcher })
        {
            const std::string who = Village::name(kind);

            int lowest = 99, highest = 0;
            bool everyStackReal = true;
            bool everyCountReal = true;
            bool everDuplicated = false;

            for (int i = 0; i < 400; ++i)
            {
                const std::vector<VillageLoot::Stack> loot =
                    VillageLoot::rollFor(kind, 100 + i * 7, 70, -300 - i * 3, seed);

                lowest = std::min(lowest, static_cast<int>(loot.size()));
                highest = std::max(highest, static_cast<int>(loot.size()));

                for (size_t a = 0; a < loot.size(); ++a)
                {
                    if (loot[a].id == Blocks::Air) everyStackReal = false;
                    if (loot[a].count <= 0) everyCountReal = false;
                    if (loot[a].count > maxStackOf(loot[a].id)) everyCountReal = false;

                    // The shuffle exists so one chest never holds the
                    // same thing twice.
                    for (size_t b = a + 1; b < loot.size(); ++b)
                        if (loot[a].id == loot[b].id) everDuplicated = true;
                }
            }

            check(everyStackReal, who + ": every stack is a real item");
            check(everyCountReal, who + ": and a count that fits in a slot");
            check(!everDuplicated, who + ": and nothing turns up twice in one chest");
            check(lowest >= VillageLoot::MIN_STACKS, who + ": never emptier than the floor");
            check(highest <= VillageLoot::MAX_STACKS, who + ": nor fuller than the ceiling");
            check(highest > lowest, who + ": and how much varies from chest to chest");
        }

        // The same chest always holds the same thing, however often it
        // is asked: that is what lets this go unsaved until it is
        // disturbed.
        for (int i = 0; i < 20; ++i)
        {
            const std::vector<VillageLoot::Stack> a =
                VillageLoot::rollFor(Village::Kind::Blacksmith, 40 + i, 71, 9, seed);
            const std::vector<VillageLoot::Stack> b =
                VillageLoot::rollFor(Village::Kind::Blacksmith, 40 + i, 71, 9, seed);
            check(a.size() == b.size(), "a chest rolls the same every time");
            for (size_t k = 0; k < a.size(); ++k)
                check(a[k].id == b[k].id && a[k].count == b[k].count,
                      "down to the last item");
        }

        // Two chests side by side are not the same chest.
        {
            const std::vector<VillageLoot::Stack> a =
                VillageLoot::rollFor(Village::Kind::Home, 10, 70, 10, seed);
            const std::vector<VillageLoot::Stack> b =
                VillageLoot::rollFor(Village::Kind::Home, 11, 70, 10, seed);
            bool same = a.size() == b.size();
            for (size_t k = 0; same && k < a.size(); ++k)
                same = (a[k].id == b[k].id && a[k].count == b[k].count);
            check(!same, "two chests in a row hold different things");
        }

        // A well has no chest, so it has no table either.
        check(VillageLoot::rollFor(Village::Kind::Lamp, 0, 70, 0, seed).empty(),
              "a lamp post holds nothing");

        // Each trade keeps its own stock: a library has no iron in it.
        bool smithHasMetal = false, libraryHasPaper = false, butcherHasMeat = false;
        bool libraryHasIron = false;
        for (int i = 0; i < 300; ++i)
        {
            for (const VillageLoot::Stack& stack :
                 VillageLoot::rollFor(Village::Kind::Blacksmith, i, 70, i * 3, seed))
                if (stack.id == Items::IronIngot || stack.id == Items::Diamond)
                    smithHasMetal = true;

            for (const VillageLoot::Stack& stack :
                 VillageLoot::rollFor(Village::Kind::Library, i, 70, i * 3, seed))
            {
                if (stack.id == Items::Paper || stack.id == Items::Book) libraryHasPaper = true;
                if (stack.id == Items::IronIngot) libraryHasIron = true;
            }

            for (const VillageLoot::Stack& stack :
                 VillageLoot::rollFor(Village::Kind::Butcher, i, 70, i * 3, seed))
                if (stack.id == Items::RawBeef || stack.id == Items::Steak)
                    butcherHasMeat = true;
        }
        check(smithHasMetal, "a smith keeps metal");
        check(libraryHasPaper, "a library keeps paper and books");
        check(butcherHasMeat, "a butcher keeps meat");
        check(!libraryHasIron, "and a library keeps no iron: each trade has its own stock");
    }

    void testBrewing()
    {
        section("brewing: what a stand makes of a bottle");

        using Brewing::Potion;

        // Water takes nether wart and nothing else.
        check(Brewing::brew(Potion::Water, Items::NetherWart) == Potion::Awkward,
              "water and nether wart make the awkward base");
        check(Brewing::brew(Potion::Water, Items::Sugar) == Potion::Mundane,
              "sugar in water makes only a mundane potion");
        check(Brewing::brew(Potion::Water, Items::BlazePowder) == Potion::Mundane,
              "and so does blaze powder");
        check(Brewing::brew(Potion::Water, Blocks::Cobblestone) == Potion::Count,
              "nor a handful of cobblestone");

        // The base takes the three that do something.
        check(Brewing::brew(Potion::Awkward, Items::Sugar) == Potion::Speed,
              "sugar makes swiftness");
        check(Brewing::brew(Potion::Awkward, Items::BlazePowder) == Potion::Strength,
              "blaze powder makes strength");
        check(Brewing::brew(Potion::Awkward, Items::GoldenCarrot) == Potion::NightVision,
              "a golden carrot makes night vision");
        check(Brewing::brew(Potion::Awkward, Items::NetherWart) == Potion::Count,
              "and a second wart does nothing to it");

        // A finished potion is finished.
        for (Potion done : { Potion::Speed, Potion::Strength, Potion::NightVision })
            for (StackId with : { Items::Sugar, Items::BlazePowder,
                                  Items::GlowstoneDust, Items::NetherWart })
                check(Brewing::brew(done, with) == Potion::Count,
                      "a finished potion takes nothing further");

        check(Brewing::isIngredient(Items::Sugar), "sugar is an ingredient");
        check(Brewing::isIngredient(Items::NetherWart), "so is wart");
        check(!Brewing::isIngredient(Items::Potion), "a potion is not");
        check(!Brewing::isIngredient(Blocks::Dirt), "nor is a block of dirt");
        check(Brewing::isIngredient(Blocks::Stone), "but stone is, for infestation");

        // Damage encoding: a potion rides in the stack's damage field.
        for (int i = 0; i < static_cast<int>(Potion::Count); ++i)
        {
            const Potion potion = static_cast<Potion>(i);
            check(Brewing::fromDamage(Brewing::toDamage(potion)) == potion,
                  std::string("potion ") + std::to_string(i) + " survives the damage field");
            check(Brewing::name(potion)[0] != '\0', "and has a name");
        }
        check(Brewing::fromDamage(-1) == Potion::Water, "a damage below the table reads as water");
        check(Brewing::fromDamage(9999) == Potion::Water, "and so does one above it");

        // What drinking them does.
        Effects::Kind kind;
        int amplifier = 0;
        float seconds = 0.0f;
        check(!Brewing::effectOf(Potion::Water, kind, amplifier, seconds),
              "water leaves nothing behind");
        check(!Brewing::effectOf(Potion::Awkward, kind, amplifier, seconds),
              "nor does the base: it is a stage, not a drink");

        check(Brewing::effectOf(Potion::Speed, kind, amplifier, seconds) &&
              kind == Effects::Kind::Speed && seconds > 0.0f,
              "swiftness leaves speed, for a while");
        check(Brewing::effectOf(Potion::Strength, kind, amplifier, seconds) &&
              kind == Effects::Kind::Strength,
              "strength leaves strength");
        check(Brewing::effectOf(Potion::NightVision, kind, amplifier, seconds) &&
              kind == Effects::Kind::NightVision,
              "and night vision leaves night vision");

        // Drinking one actually moves the player.
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            check(Effects::speedMultiplier(p.effects) == 1.0f, "unhurried to begin with");

            Brewing::effectOf(Potion::Speed, kind, amplifier, seconds);
            Effects::give(p.effects, kind, amplifier, seconds);
            check(Effects::speedMultiplier(p.effects) > 1.0f, "and quicker once drunk");

            // Three minutes is what it says on the bottle.
            for (int i = 0; i < 1790; ++i) p.updateEffects(0.1f);
            check(p.effects.has(Effects::Kind::Speed), "still going after three minutes less a moment");
            for (int i = 0; i < 40; ++i) p.updateEffects(0.1f);
            check(!p.effects.has(Effects::Kind::Speed), "and worn off just after");
        }
    }

    void testEffects()
    {
        section("status effects: the clock and what it does");

        using Effects::Kind;

        Effects::Set set;
        check(!set.any(), "a fresh player carries nothing");
        check(!set.has(Kind::Speed), "and no speed");
        check(set.amplifier(Kind::Speed) == -1,
              "an absent effect reads as -1, not as the weakest there is");

        Effects::give(set, Kind::Speed, 0, 10.0f);
        check(set.has(Kind::Speed) && set.any(), "a drink gives it");
        check(set.amplifier(Kind::Speed) == 0, "at the level it was poured");
        check(set.secondsLeft(Kind::Speed) == 10.0f, "for as long as it said");

        // Weaker and shorter: ignored outright.
        Effects::give(set, Kind::Speed, 0, 4.0f);
        check(set.secondsLeft(Kind::Speed) == 10.0f, "a shorter one of the same strength is ignored");
        Effects::give(set, Kind::Speed, 0, 25.0f);
        check(set.secondsLeft(Kind::Speed) == 25.0f, "a longer one of the same strength extends it");

        Effects::give(set, Kind::Speed, 1, 5.0f);
        check(set.amplifier(Kind::Speed) == 1, "a stronger one takes over");
        check(set.secondsLeft(Kind::Speed) == 5.0f, "even when it is shorter");
        Effects::give(set, Kind::Speed, 0, 600.0f);
        check(set.amplifier(Kind::Speed) == 1,
              "and a long weak one does not cut the strong one short");
        check(set.secondsLeft(Kind::Speed) == 5.0f, "nor extend it");

        Effects::remove(set, Kind::Speed);
        check(!set.has(Kind::Speed), "it can be taken away");
        Effects::give(set, Kind::Poison, 0, 3.0f);
        Effects::clear(set);
        check(!set.any(), "and all of it at once");

        // Zero and negative times give nothing at all.
        Effects::give(set, Kind::Speed, 0, 0.0f);
        check(!set.has(Kind::Speed), "a drink of no length does nothing");

        // --- the clock ---------------------------------------------------
        {
            Effects::Set s2;
            Effects::give(s2, Kind::Strength, 0, 1.0f);
            Effects::step(s2, 0.4f);
            check(s2.has(Kind::Strength), "it survives a step shorter than itself");
            Effects::step(s2, 0.7f);
            check(!s2.has(Kind::Strength), "and runs out when the time is up");
        }

        // Regeneration pulses rather than trickling.
        {
            Effects::Set s2;
            Effects::give(s2, Kind::Regeneration, 0, 100.0f);
            const Effects::Tick first = Effects::step(s2, Effects::REGEN_INTERVAL - 0.01f);
            check(first.healPulses == 0, "regeneration pays nothing before its interval");
            const Effects::Tick second = Effects::step(s2, 0.02f);
            check(second.healPulses == 1, "and one heart when the interval comes round");
        }
        {
            // Ten seconds of regeneration I is four pulses.
            Effects::Set s2;
            Effects::give(s2, Kind::Regeneration, 0, 100.0f);
            int total = 0;
            for (int i = 0; i < 101; ++i) total += Effects::step(s2, 0.1f).healPulses;
            check(total == 4, "ten seconds of regeneration I heals four");
        }
        {
            // The amplifier halves the wait, so twice as many.
            Effects::Set s2;
            Effects::give(s2, Kind::Regeneration, 1, 100.0f);
            int total = 0;
            for (int i = 0; i < 101; ++i) total += Effects::step(s2, 0.1f).healPulses;
            check(total == 8, "and regeneration II heals eight");
        }
        {
            Effects::Set s2;
            Effects::give(s2, Kind::Poison, 0, 100.0f);
            int total = 0;
            for (int i = 0; i < 101; ++i) total += Effects::step(s2, 0.1f).poisonPulses;
            check(total == 8, "ten seconds of poison I lands eight");
        }
        {
            // A frame longer than the effect has left must not pay out
            // pulses the player never lived through.
            Effects::Set s2;
            Effects::give(s2, Kind::Poison, 0, 1.0f);
            const Effects::Tick tick = Effects::step(s2, 60.0f);
            check(tick.poisonPulses == 0,
                  "a long frame pays only for the time the effect had left");
            check(!s2.has(Kind::Poison), "and the effect is gone after it");
        }
        {
            // Both at once, each on its own clock.
            Effects::Set s2;
            Effects::give(s2, Kind::Regeneration, 0, 100.0f);
            Effects::give(s2, Kind::Poison, 0, 100.0f);
            int healed = 0, poisoned = 0;
            for (int i = 0; i < 101; ++i)
            {
                const Effects::Tick tick = Effects::step(s2, 0.1f);
                healed += tick.healPulses;
                poisoned += tick.poisonPulses;
            }
            check(healed == 4 && poisoned == 8, "two effects keep separate clocks");
        }

        // --- what they do -------------------------------------------------
        {
            Effects::Set s2;
            check(Effects::speedMultiplier(s2) == 1.0f, "no effects, no change to speed");
            check(Effects::attackBonus(s2) == 0, "nor to a blow");
            check(Effects::jumpMultiplier(s2) == 1.0f, "nor to a jump");
            check(!Effects::fireImmune(s2) && !Effects::breathesWater(s2),
                  "and nothing is survivable that was not");
            check(Effects::nightVisionFloor(s2) == 0.0f, "and the dark stays dark");

            Effects::give(s2, Kind::Speed, 0, 10.0f);
            check(Effects::speedMultiplier(s2) > 1.0f, "speed makes you faster");
            const float speedOne = Effects::speedMultiplier(s2);
            Effects::give(s2, Kind::Speed, 1, 10.0f);
            check(Effects::speedMultiplier(s2) > speedOne, "and speed II more so");

            Effects::give(s2, Kind::Slowness, 0, 10.0f);
            check(Effects::speedMultiplier(s2) < speedOne * 1.5f,
                  "slowness pulls back against it");

            Effects::give(s2, Kind::Strength, 0, 10.0f);
            check(Effects::attackBonus(s2) == 3, "strength I adds three to a blow");
            Effects::give(s2, Kind::Weakness, 0, 10.0f);
            check(Effects::attackBonus(s2) == -1,
                  "and weakness can take it below nothing, for the caller to floor");

            Effects::give(s2, Kind::JumpBoost, 0, 10.0f);
            check(Effects::jumpMultiplier(s2) > 1.0f, "jump boost jumps higher");

            Effects::give(s2, Kind::FireResistance, 0, 10.0f);
            check(Effects::fireImmune(s2), "fire resistance keeps the fire off");
            Effects::give(s2, Kind::WaterBreathing, 0, 10.0f);
            check(Effects::breathesWater(s2), "water breathing keeps the water out");
            Effects::give(s2, Kind::NightVision, 0, 10.0f);
            check(Effects::nightVisionFloor(s2) > 0.5f, "and night vision lifts the dark");
        }

        // Every kind has a name and a side.
        int harmful = 0;
        int neutral = 0;
        for (int i = 0; i < Effects::KIND_COUNT; ++i)
        {
            const Kind kind = static_cast<Kind>(i);
            check(Effects::name(kind)[0] != '\0',
                  std::string("effect ") + std::to_string(i) + " has a name");
            if (Effects::isHarmful(kind)) ++harmful;
            if (Effects::isNeutral(kind)) ++neutral;
        }
        check(harmful == 20, "twenty of them are ones you would rather not have");
        check(neutral == 4, "and four of those are neither good nor bad");
        check(Effects::isHarmful(Kind::BadOmen),
              "bad omen among them, though it is the only one that does nothing to you");
    }

    void testEffectsOnThePlayer()
    {
        section("status effects: what they do to the player");

        using Effects::Kind;

        // Regeneration heals, and stops at full.
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.health = 10;
            Effects::give(p.effects, Kind::Regeneration, 0, 100.0f);
            for (int i = 0; i < 101; ++i) p.updateEffects(0.1f);
            check(p.health == 14, "regeneration heals a heart a pulse");

            p.health = Player::MAX_HEALTH;
            for (int i = 0; i < 101; ++i) p.updateEffects(0.1f);
            check(p.health == Player::MAX_HEALTH, "and cannot take you over full");
        }

        // Poison never kills: Minecraft leaves you on half a heart.
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.health = 4;
            Effects::give(p.effects, Kind::Poison, 0, 600.0f);
            for (int i = 0; i < 1200; ++i) p.updateEffects(0.1f);
            check(p.health == 1, "poison takes you to one and no further");
            check(p.health > 0, "so it is never the thing that kills you");
        }

        // Creative takes no poison at all.
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.mode = GameMode::Creative;
            p.health = Player::MAX_HEALTH;
            Effects::give(p.effects, Kind::Poison, 0, 100.0f);
            for (int i = 0; i < 101; ++i) p.updateEffects(0.1f);
            check(p.health == Player::MAX_HEALTH, "creative shrugs off poison");
        }

        // A dead player's effects stop.
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.health = 0;
            Effects::give(p.effects, Kind::Regeneration, 0, 100.0f);
            p.updateEffects(0.1f);
            check(!p.effects.any(), "dying clears what was running");
        }

        // Water breathing: the lungs never empty.
        {
            // Ten seconds under, which is inside the fifteen the lungs
            // hold: any longer and the player drowns, and dying puts the
            // air back, so the check would be reading a corpse.
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            for (int i = 0; i < 100; ++i) p.breathe(0.1f, true);
            check(p.air < Player::MAX_AIR_SECONDS * 0.5f,
                  "a long dive empties the lungs");
            check(p.health == Player::MAX_HEALTH, "without drowning them yet");

            Player q(glm::vec3(0.0f, 70.0f, 0.0f));
            Effects::give(q.effects, Kind::WaterBreathing, 0, 600.0f);
            bool drowned = false;
            for (int i = 0; i < 300; ++i) drowned |= q.breathe(0.1f, true);
            check(q.air == Player::MAX_AIR_SECONDS, "water breathing keeps them full");
            check(!drowned, "and nothing ever lands");
        }

        // Lava burns, and fire resistance takes all of it.
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.health = Player::MAX_HEALTH;
            const bool first = p.burn(0.1f, true);
            check(first, "stepping into lava hurts at once");
            check(p.health == Player::MAX_HEALTH - Player::LAVA_DAMAGE,
                  "for four, which is two hearts");

            // Out of the lava, nothing more.
            const int held = p.health;
            for (int i = 0; i < 50; ++i) p.burn(0.1f, false);
            check(p.health == held, "and stops the moment you are out of it");
        }
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.health = Player::MAX_HEALTH;
            Effects::give(p.effects, Kind::FireResistance, 0, 600.0f);
            for (int i = 0; i < 200; ++i)
            {
                p.clearHurtCooldown();
                p.burn(0.1f, true);
            }
            check(p.health == Player::MAX_HEALTH,
                  "fire resistance swims lava without a scratch");
        }
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.mode = GameMode::Creative;
            p.health = Player::MAX_HEALTH;
            for (int i = 0; i < 50; ++i) p.burn(0.1f, true);
            check(p.health == Player::MAX_HEALTH, "creative does not burn either");
        }

        // Lava kills a survival player who stays in it.
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.health = Player::MAX_HEALTH;
            for (int i = 0; i < 200 && p.health > 0; ++i)
            {
                p.clearHurtCooldown();
                p.burn(0.1f, true);
            }
            check(p.health <= 0, "and standing in it is fatal");
        }
    }

    void testRiding()
    {
        section("the horse, and getting on it");

        const MobType& horse = mobType(MobId::Horse);
        check(horse.rideable, "a horse can be ridden");
        check(horse.saddleHeight > 0.0f, "and a rider sits above its feet, not in them");
        check(horse.saddleHeight <= horse.height,
              "though not above its head");
        check(horse.walkSpeed > 5.0f, "it is faster than you are on foot, which is the point");
        check(horse.spawnClass == SpawnClass::Passive, "and it is not out to get you");

        // The horse and the strider, among the living. Riding a
        // creeper would be a different game; riding a strider is how
        // Minecraft gets you across a lava sea.
        int rideable = 0;
        for (int i = 0; i < static_cast<int>(MobId::Count); ++i)
        {
            const MobType& t = mobType(static_cast<MobId>(i));
            if (t.rideable && !t.inanimate) ++rideable;
        }
        check(rideable == 10, "it and its four kin, the strider, the camel, the two llamas and a saddled pig are the ones you can get on");
        check(mobType(MobId::Strider).rideable, "the strider is one of the others");
        check(mobType(MobId::Camel).rideable, "and the camel is the third");

        check(Mob::tamingFood(MobId::Horse) == Items::Wheat, "wheat tames one");

        {
            Mob mount(MobId::Horse, glm::vec3(10.0f, 70.0f, -4.0f), 21u);
            check(!mount.ridden(), "a horse starts unridden");

            // The saddle is above the feet by exactly what the type says.
            const glm::vec3 seat = mount.saddle();
            check(std::fabs(seat.y - (mount.position().y + horse.saddleHeight)) < 0.0001f,
                  "the saddle sits where the type says it does");
            check(seat.x == mount.position().x && seat.z == mount.position().z,
                  "and directly over it");

            mount.setRidden(true);
            check(mount.ridden(), "and can be got on");
            mount.setRidden(false);
            check(!mount.ridden(), "and off again");
        }

        // Steering points it where it is told, and only moves it when
        // asked: a horse that walked off on its own with a rider up
        // would be a horse nobody could steer.
        {
            Mob mount(MobId::Horse, glm::vec3(0.0f, 70.0f, 0.0f), 22u);
            mount.setRidden(true);

            mount.steer(90.0f, false, false);
            check(mount.ridden(), "still ridden after steering");

            // Steering a loose horse is harmless: it simply has a goal
            // it did not pick, and chooses another on its next pass.
            Mob loose(MobId::Horse, glm::vec3(0.0f, 70.0f, 0.0f), 23u);
            loose.steer(180.0f, true, true);
            check(!loose.ridden(), "steering does not put anybody on it");
        }

        // Getting off clears what it was told to do, or it would keep
        // walking the way the rider last pointed it.
        {
            Mob mount(MobId::Horse, glm::vec3(0.0f, 70.0f, 0.0f), 24u);
            mount.setRidden(true);
            mount.steer(45.0f, true, true);
            mount.setRidden(false);
            check(!mount.ridden(), "getting off ends the ride");
            check(!mount.hunting(), "and leaves it with nothing it was told to chase");
        }
    }

    void testBoats()
    {
        section("the boat");

        const MobType& boat = mobType(MobId::Boat);
        check(boat.rideable, "a boat can be got into");
        check(boat.inanimate, "but it is not alive");
        check(boat.floats, "and it floats");
        check(boat.drop == Items::Boat && boat.dropCount == 1,
              "break one and you get it back");
        check(boat.saddleHeight > 0.0f && boat.saddleHeight <= boat.height,
              "you sit in it, not under it and not on its mast");
        check(boat.experience == 0, "and killing it teaches you nothing");

        // Nothing inanimate may spawn on its own: a lake full of boats
        // nobody built would be the giveaway.
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobType& t = mobType(static_cast<MobId>(i));
            if (!t.inanimate) continue;
            check(t.breedingFood == Blocks::Air,
                  std::string(t.name) + " cannot be bred");
            check(t.attackDamage == 0, std::string(t.name) + " cannot hurt you");
        }

        // Who may get in what. A boat belongs to whoever finds it; a
        // horse has to be tamed and grown first.
        check(Mob::canBeRidden(boat, false, false), "anyone can get into a boat");
        check(!Mob::canBeRidden(boat, false, true), "though not a half-built one");
        check(Mob::canBeRidden(mobType(MobId::Horse), true, false),
              "a tamed horse can be ridden");
        check(!Mob::canBeRidden(mobType(MobId::Horse), false, false),
              "a wild one cannot");
        check(!Mob::canBeRidden(mobType(MobId::Horse), true, true),
              "and neither can a foal");
        for (MobId other : { MobId::Cow, MobId::Pig, MobId::Creeper, MobId::Villager })
            check(!Mob::canBeRidden(mobType(other), true, false),
                  std::string(mobType(other).name) + " is not transport");

        // A boat is planks, so it wears the pack's planks rather than a
        // colour picked to look like them: put one next to a plank wall
        // and the grain is the same grain.
        check(*boat.blockTexture != '\0', "a hull is clad in a block texture");
        check(std::string(boat.blockTexture) == "oak_planks",
              "the same planks the world is built from");
        check(*boat.blockTextureAlt != '\0',
              "and it knows what an older pack calls them");
        check(!*mobType(MobId::Cow).blockTexture,
              "while an animal is not clad in anything");

        check(maxStackOf(Items::Boat) == 1, "boats do not stack, the same as vanilla");
        check(std::string(displayName(Items::Boat)) == "Oak Boat", "and it is called an oak boat, as in Minecraft");

        // Five planks, open at the top. The shape matters: a full three
        // by two would be something else.
        {
            ItemStack hull[9];
            for (int i : { 0, 2, 3, 4, 5 }) { hull[i].id = Blocks::Planks; hull[i].count = 1; }
            const CraftOutput made = Crafting::match(hull, 3);
            check(made.valid() && made.id == Items::Boat && made.count == 1,
                  "five planks round an open top make a boat");

            // Filled in, it is six planks and not a boat.
            ItemStack solid[9];
            for (int i : { 0, 1, 2, 3, 4, 5 }) { solid[i].id = Blocks::Planks; solid[i].count = 1; }
            const CraftOutput notABoat = Crafting::match(solid, 3);
            check(!notABoat.valid() || notABoat.id != Items::Boat,
                  "but a solid block of six is not one");
        }

        // Buoyancy. Dropped in deep, it comes up; held under, it rises;
        // and left alone it settles at the waterline rather than
        // bobbing there for ever.
        const float waterline = 64.0f;

        check(Mob::verticalStep(0.0f, 62.0f, waterline, true, 0.05f) > 0.0f,
              "a hull under the surface is pushed up");
        check(Mob::verticalStep(0.0f, 66.0f, waterline, true, 0.05f) < 0.0f,
              "and one above it is pulled down");
        check(Mob::verticalStep(0.0f, 62.0f, waterline, false, 0.05f) < 0.0f,
              "while anything that does not float just falls");

        // Where it comes to rest, which is the whole point: the first
        // try added the spring on top of gravity and settled a block
        // under the surface, where the water drew over the hull and it
        // read as a blue smudge rather than a boat.
        {
            float y = 60.0f, vy = 0.0f;
            for (int step = 0; step < 400; ++step)
            {
                vy = Mob::verticalStep(vy, y, waterline, true, 0.02f);
                y += vy * 0.02f;
            }
            check(std::fabs(y - waterline) < 0.05f,
                  "and one dropped four blocks under settles at the waterline, "
                  "not below it");
            check(std::fabs(vy) < 0.05f, "and stops moving once it is there");
        }

        // The damping has to actually damp. A spring this stiff with no
        // drag would throw the boat into the sky.
        {
            float y = 60.0f, vy = 0.0f, highest = y;
            for (int step = 0; step < 400; ++step)
            {
                vy = Mob::verticalStep(vy, y, waterline, true, 0.02f);
                y += vy * 0.02f;
                highest = std::max(highest, y);
            }
            check(highest < waterline + 1.0f, "and never breaches on the way up");
        }

        // Settling must not depend on the frame rate: a boat that rode
        // high at 60fps and sank at 20 would be the same bug wearing a
        // different hat.
        for (float dt : { 0.008f, 0.016f, 0.033f, 0.05f })
        {
            float y = 61.0f, vy = 0.0f;
            for (int step = 0; step < static_cast<int>(12.0f / dt); ++step)
            {
                vy = Mob::verticalStep(vy, y, waterline, true, dt);
                y += vy * dt;
            }
            check(std::fabs(y - waterline) < 0.05f,
                  "it settles at the waterline at " + std::to_string(dt) + "s a frame");
        }
    }

    void testNether()
    {
        section("the Nether: what lives there and what it drops");

        // Everything that belongs down there is in the spawn list, and
        // nothing that belongs up here is. A zombie in the Nether was
        // the bug that put dimensions on the table in the first place.
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobType& t = mobType(static_cast<MobId>(i));
            if (t.dimension != Dimension::Nether) continue;
            // The strider is the exception, and Minecraft's is too:
            // it attacks nobody, nothing attacks it, and it is the one
            // way across a lava sea.
            check(t.spawnClass == SpawnClass::Hostile || t.neutral ||
                      t.id == MobId::Strider,
                  std::string(t.name) + " is not something you meet in the Nether and pet");
            check(t.fireproof, std::string(t.name) + " is not hurt by the fire it lives in");
        }

        const MobType& blaze = mobType(MobId::Blaze);
        check(blaze.hoverHeight > 0.0f, "a blaze hovers rather than walks");
        check(blaze.shootsFire && blaze.arrowDamage > 0, "and throws fire rather than swinging");
        check(blaze.drop == Items::BlazeRod && blaze.dropCount > 0, "and leaves a rod behind");

        // Twelve rods and a head, all reading two patches: the head's
        // at the top-left and the one rod patch at (0, 16).
        check(blaze.model.size() == 13, "a blaze is a head and twelve rods");
        int rodBoxes = 0;
        for (const MobBox& b : blaze.model)
            if (b.u == 0 && b.v == 16) ++rodBoxes;
        check(rodBoxes == 12, "and every rod reads the one patch Mojang drew");

        // The scaled species. The figure and the box scale together, or
        // a wither skeleton is drawn through its own hitbox.
        const MobType& wither = mobType(MobId::WitherSkeleton);
        const MobType& skeleton = mobType(MobId::Skeleton);
        check(wither.modelScale > 1.0f, "a wither skeleton is the bigger skeleton");
        check(wither.model.size() == skeleton.model.size(),
              "built from the same boxes as the one up top");
        for (size_t i = 0; i < wither.model.size(); ++i)
            check(wither.model[i].u == skeleton.model[i].u &&
                      wither.model[i].v == skeleton.model[i].v,
                  "and reading the same patches");
        check(wither.attackDamage > skeleton.arrowDamage,
              "it hits harder up close than the other one does at range");

        {
            Mob one(MobId::WitherSkeleton, glm::vec3(0.0f), 3u);
            check(std::fabs(one.height() - wither.height * wither.modelScale) < 0.001f,
                  "and its hitbox is scaled with it");
            check(std::fabs(one.width() - wither.width * wither.modelScale) < 0.001f,
                  "in both directions");
        }

        // Brewing's supply line, end to end: a rod off a blaze grinds
        // into powder, the powder makes strength, and the cream off a
        // magma cube is the only way to walk through fire.
        {
            ItemStack rod[9];
            rod[0].id = Items::BlazeRod;
            rod[0].count = 1;
            const CraftOutput powder = Crafting::match(rod, 2);
            check(powder.valid() && powder.id == Items::BlazePowder,
                  "a blaze rod grinds down into powder");
            check(powder.count == 2, "two of it, the way Minecraft pays");
        }
        check(Brewing::brew(Brewing::Potion::Awkward, Items::MagmaCream) ==
                  Brewing::Potion::FireResistance,
              "magma cream makes fire resistance");
        check(mobType(MobId::MagmaCube).drop == Items::MagmaCream,
              "which is what a magma cube is carrying");

        // The stand-in is gone: netherrack used to cook down into blaze
        // powder because there was no blaze to take it off.
        check(Smelting::resultOf(Blocks::Netherrack) == Items::NetherBrick,
              "netherrack fires into a nether brick");
        check(Smelting::resultOf(Blocks::Netherrack) != Items::BlazePowder,
              "and not into the powder it stood in for");
        {
            ItemStack four[9];
            for (int i : { 0, 1, 3, 4 }) { four[i].id = Items::NetherBrick; four[i].count = 1; }
            const CraftOutput block = Crafting::match(four, 3);
            check(block.valid() && block.id == Blocks::NetherBricks,
                  "and four of those make the block a fortress is built of");
        }

        // Fire itself. A charge leaves the player burning after it has
        // gone, the burn is not the blow, and water is the answer.
        {
            Player player(glm::vec3(0.0f, 64.0f, 0.0f));
            player.mode = GameMode::Survival;
            player.health = Player::MAX_HEALTH;
            player.ignite(5.0f);
            check(player.onFire(), "a charge leaves the player alight");

            // The half-second of immunity after a blow is bled off in
            // Player::update, which wants a world; a burn lands once a
            // second, which is twice that, so clearing it each step is
            // what the real game does to it by the time fire comes back
            // round.
            int hits = 0;
            for (int step = 0; step < 300; ++step)
            {
                player.clearHurtCooldown();
                if (player.burn(0.05f, false)) ++hits;
            }

            check(hits >= 4 && hits <= 6, "which costs about a heart a second while it lasts");
            check(!player.onFire(), "and goes out on its own");

            player.health = Player::MAX_HEALTH;
            player.ignite(5.0f);
            player.douse();
            check(!player.onFire(), "water puts it out at once");

            // Lava is both: it cooks you while you are in it and leaves
            // you burning when you climb out.
            player.health = Player::MAX_HEALTH;
            player.burn(0.05f, true);
            check(player.onFire(), "climbing out of lava you are still alight");
        }

        {
            Player safe(glm::vec3(0.0f, 64.0f, 0.0f));
            safe.mode = GameMode::Survival;
            safe.health = Player::MAX_HEALTH;
            Effects::give(safe.effects, Effects::Kind::FireResistance, 0, 30.0f);
            safe.ignite(5.0f);
            check(!safe.onFire(), "fire resistance means it never catches at all");
        }

        // A hovering thing settles at its own height above the floor
        // and stays there, which is the same spring the waterline uses.
        {
            const float wanted = 64.0f + blaze.hoverHeight;
            float y = 64.0f, vy = 0.0f;
            for (int step = 0; step < 600; ++step)
            {
                vy = Mob::verticalStep(vy, y, wanted, true,
                                       Mob::HOVER_PULL, Mob::HOVER_DAMPING, 0.02f);
                y += vy * 0.02f;
            }
            check(std::fabs(y - wanted) < 0.05f, "a blaze settles at its hovering height");
            check(std::fabs(vy) < 0.05f, "and hangs there");
        }
    }

    void testHeightStretch()
    {
        section("pbr: the height map a pack actually ships");

        auto tile = [](std::initializer_list<uint8_t> alphas) {
            std::vector<uint8_t> rgba;
            for (uint8_t a : alphas)
            {
                rgba.push_back(10); rgba.push_back(20); rgba.push_back(30); rgba.push_back(a);
            }
            return rgba;
        };
        auto alphaOf = [](const std::vector<uint8_t>& rgba) {
            std::vector<uint8_t> out;
            for (size_t i = 3; i < rgba.size(); i += 4) out.push_back(rgba[i]);
            return out;
        };

        // The real shape of the problem: Prime's bricks run 233 to 255,
        // which parallax and the crevice shading both read as a flat
        // wall. Stretched, the same tile uses the whole range.
        {
            std::vector<uint8_t> bricks = tile({ 233, 244, 255, 240, 233, 255 });
            Atlas::stretchHeightRange(bricks);
            const std::vector<uint8_t> a = alphaOf(bricks);
            check(a[0] == 0, "the lowest point of a shallow tile goes to the floor");
            check(a[2] == 255, "and the highest to the ceiling");
            check(a[1] > a[3] && a[3] > a[0], "and the order of everything between is kept");
        }

        // Sand is flat and must stay flat. Amplifying nothing gives
        // noise, which would be worse than leaving it alone.
        {
            std::vector<uint8_t> sand = tile({ 255, 255, 255, 255 });
            Atlas::stretchHeightRange(sand);
            const std::vector<uint8_t> a = alphaOf(sand);
            check(a[0] == 255 && a[3] == 255, "a tile with no relief is left alone");
        }

        // And so is one that barely varies: that is a pack saying flat,
        // not a pack being subtle.
        {
            std::vector<uint8_t> nearly = tile({ 250, 252, 253, 251 });
            Atlas::stretchHeightRange(nearly);
            const std::vector<uint8_t> a = alphaOf(nearly);
            check(a[0] == 250 && a[2] == 253, "and so is one that barely varies");
        }

        // The colour channels are the normal, and must not be touched.
        {
            std::vector<uint8_t> any = tile({ 100, 200, 150 });
            Atlas::stretchHeightRange(any);
            check(any[0] == 10 && any[1] == 20 && any[2] == 30,
                  "stretching the height leaves the normal alone");
        }

        // Already full-range is a no-op, so running it twice is safe.
        {
            std::vector<uint8_t> full = tile({ 0, 128, 255 });
            Atlas::stretchHeightRange(full);
            std::vector<uint8_t> twice = full;
            Atlas::stretchHeightRange(twice);
            check(full == twice, "a tile already using the whole range does not move");
        }

        // Order is never inverted, whatever the range: a crevice must
        // not come out as a ridge.
        {
            std::vector<uint8_t> ramp = tile({ 190, 200, 210, 220, 230, 240, 250 });
            Atlas::stretchHeightRange(ramp);
            const std::vector<uint8_t> a = alphaOf(ramp);
            bool rising = true;
            for (size_t i = 1; i < a.size(); ++i) if (a[i] <= a[i - 1]) rising = false;
            check(rising, "a rising slope comes out rising");
            check(a.front() == 0 && a.back() == 255, "and spans the full range");
        }
    }

    void testRails()
    {
        section("rails, and what runs on them");

        // Every shape is a rail, and only the plain one is a thing you
        // hold: the other five are it bent to meet its neighbours.
        for (BlockId id : { Blocks::RailNorthSouth, Blocks::RailEastWest,
                            Blocks::RailNorthEast, Blocks::RailSouthEast,
                            Blocks::RailSouthWest, Blocks::RailNorthWest })
        {
            check(isRail(id), std::string(blockInfo(id).name) + " is a rail");
            check(!isSolid(id), "and you walk over it, not into it");
            check(!isOpaque(id), "and it casts no shadow");
            check(Tools::canHarvest(Blocks::Air, id),
                  "and bare hands can pick it up");

            // Every rail joins exactly two ways, or a cart reaching it
            // would have nowhere to go.
            int ends = 0;
            for (int d = 0; d < 4; ++d) if (railLinks(id, d)) ++ends;
            check(ends == 2, "and it runs between exactly two of its sides");
        }

        check(isObtainable(Blocks::RailNorthSouth), "a rail is a thing you hold");
        for (BlockId id : { Blocks::RailEastWest, Blocks::RailNorthEast,
                            Blocks::RailSouthEast, Blocks::RailSouthWest,
                            Blocks::RailNorthWest })
            check(!isObtainable(id), "but its other shapes are not");

        // Shaping. A straight run beats a corner wherever both would
        // do, which is what stops a line kinking when a spur is laid
        // alongside it.
        check(railShapeFor(false, false, false, false) == Blocks::RailNorthSouth,
              "a rail on its own lies north to south");
        check(railShapeFor(true, false, true, false) == Blocks::RailNorthSouth,
              "one between two others in a line stays straight");
        check(railShapeFor(false, true, false, true) == Blocks::RailEastWest,
              "and so does one lying the other way");
        check(railShapeFor(true, true, true, true) == Blocks::RailNorthSouth,
              "a crossroads picks a straight rather than a corner");
        check(railShapeFor(false, true, false, false) == Blocks::RailEastWest,
              "one neighbour east makes an east-west rail");

        check(railShapeFor(true, true, false, false) == Blocks::RailNorthEast,
              "north and east make the north-east bend");
        check(railShapeFor(false, true, true, false) == Blocks::RailSouthEast,
              "south and east the south-east one");
        check(railShapeFor(false, false, true, true) == Blocks::RailSouthWest,
              "south and west the south-west one");
        check(railShapeFor(true, false, false, true) == Blocks::RailNorthWest,
              "and north and west the last");

        // Every corner joins the two sides its name claims.
        check(railLinks(Blocks::RailNorthEast, 0) && railLinks(Blocks::RailNorthEast, 1),
              "the north-east bend joins north to east");
        check(railLinks(Blocks::RailSouthWest, 2) && railLinks(Blocks::RailSouthWest, 3),
              "and the south-west one south to west");

        // The shaping must be reversible: whatever shape a set of
        // neighbours produces must join up with exactly those sides.
        for (int mask = 0; mask < 16; ++mask)
        {
            const bool n = (mask & 1) != 0, e = (mask & 2) != 0;
            const bool s = (mask & 4) != 0, w = (mask & 8) != 0;
            const BlockId shaped = railShapeFor(n, e, s, w);

            int joined = 0;
            for (int d = 0; d < 4; ++d) if (railLinks(shaped, d)) ++joined;
            check(joined == 2, "every shaping gives a rail with two ends");

            // Where exactly two neighbours sit, the rail must reach
            // both of them -- otherwise a laid track has a gap in it.
            const int count = (n ? 1 : 0) + (e ? 1 : 0) + (s ? 1 : 0) + (w ? 1 : 0);
            if (count != 2) continue;
            const bool reaches = (!n || railLinks(shaped, 0)) && (!e || railLinks(shaped, 1)) &&
                                 (!s || railLinks(shaped, 2)) && (!w || railLinks(shaped, 3));
            check(reaches, "and a rail with two neighbours reaches both");
        }

        // The cart.
        const MobType& cart = mobType(MobId::Minecart);
        check(cart.ridesRails, "a minecart runs on rails");
        check(cart.inanimate && cart.rideable, "and is got into, not tamed");
        check(!cart.floats, "and it is no boat");
        check(cart.drop == Items::Minecart, "break one and you get it back");
        check(cart.walkSpeed > mobType(MobId::Boat).walkSpeed,
              "and it is quicker than a boat, which is the point of track");

        // Guidance. A cart is put on the middle of its rail across the
        // travel, and sent out the end it is already pointed at.
        {
            const glm::ivec3 cell(4, 70, -9);
            glm::vec3 snapped(0.0f);
            float yaw = 0.0f;

            // Heading east down a north-south rail: it must leave by
            // one of the two ends the rail actually has, not east.
            check(Mob::railGuide(Blocks::RailNorthSouth, cell,
                                 glm::vec3(4.8f, 70.1f, -8.2f), 0.0f, snapped, yaw),
                  "a cart on a rail is guided by it");
            check(yaw == Mob::railYaw(0) || yaw == Mob::railYaw(2),
                  "and off a north-south rail it leaves north or south");
            check(std::fabs(snapped.x - 4.5f) < 0.0001f,
                  "and is pulled to the middle of the track across its travel");
            check(std::fabs(snapped.y - (70.0f + Mob::RAIL_LIFT)) < 0.0001f,
                  "and sits on the rail's own surface");

            // Pointed south, it goes south; turned round, it goes back.
            Mob::railGuide(Blocks::RailNorthSouth, cell, glm::vec3(4.5f, 70.1f, -8.5f),
                           90.0f, snapped, yaw);
            check(yaw == Mob::railYaw(2), "pointed south it carries on south");
            Mob::railGuide(Blocks::RailNorthSouth, cell, glm::vec3(4.5f, 70.1f, -8.5f),
                           -90.0f, snapped, yaw);
            check(yaw == Mob::railYaw(0), "and turned round it goes back north");

            Mob::railGuide(railSlopeOf(0, 1, false), cell, glm::vec3(4.75f, 70.5f, -8.5f), 0.0f, snapped, yaw);
            check(std::fabs(snapped.y - (70.75f + Mob::RAIL_LIFT)) < 0.0001f && yaw == Mob::railYaw(1),
                  "a cart three quarters up a slope rising east sits three quarters up");
            Mob::railGuide(railSlopeOf(2, 0, true), cell, glm::vec3(4.5f, 70.5f, -8.75f), -90.0f, snapped, yaw);
            check(std::fabs(snapped.y - (70.75f + Mob::RAIL_LIFT)) < 0.0001f, "and the same on a powered slope rising north");

            for (int high = 0; high < 4; ++high)
            {
                const BlockId plain = railSlopeOf(0, high, false);
                check(isRail(plain) && isRailSlope(plain) && railSlopeDir(plain) == high && !isSpecialRail(plain) &&
                          railLinks(plain, high) && railLinks(plain, (high + 2) & 3) && !railLinks(plain, (high + 1) & 3),
                      "a sloped rail runs up and down its own slope");
                check(blockDrop(plain) == blockDrop(Blocks::RailNorthSouth), "and breaks into a rail");
                for (int family = 1; family <= 3; ++family)
                    for (bool powered : { false, true })
                    {
                        const BlockId sloped = railSlopeOf(family, high, powered);
                        check(specialRailFamily(sloped) == family && specialRailPowered(sloped) == powered &&
                                  railSlopeDir(sloped) == high && specialRailEastWest(sloped) == ((high & 1) != 0),
                              "a sloped special rail keeps its kind, power and slope");
                        check(specialRailWithPower(sloped, !powered) == railSlopeOf(family, high, !powered),
                              "and keeps its slope when power changes");
                    }
            }
            check(Redstone::lit(railSlopeOf(1, 2, false)) == railSlopeOf(1, 2, true), "a powered rail on a slope lights up");

            // A bend sends it out the other way than it came in.
            Mob::railGuide(Blocks::RailSouthEast, cell, glm::vec3(4.5f, 70.1f, -8.5f),
                           -90.0f, snapped, yaw);
            check(yaw == Mob::railYaw(1) || yaw == Mob::railYaw(2),
                  "a bend sends a cart out one of its own two ends");

            // Anything that is not a rail guides nothing.
            check(!Mob::railGuide(Blocks::Stone, cell, glm::vec3(4.5f, 70.1f, -8.5f),
                                  0.0f, snapped, yaw),
                  "and a stone block is not a rail");
        }

        // Momentum. A cart that only moved while the key was held
        // would be a wheelbarrow.
        {
            const float top = cart.walkSpeed;

            float speed = 0.0f;
            for (int step = 0; step < 300; ++step)
                speed = Mob::railSpeedStep(speed, true, top, 0.02f);
            check(std::fabs(speed - top) < 0.0001f, "pushing gets a cart up to its top speed");
            speed = Mob::railSpeedStep(speed, true, top, 0.02f);
            check(speed <= top + 0.0001f, "and no further");

            // Coasting: it keeps going for a good while, then stops
            // dead rather than creeping backwards.
            float coasted = 0.0f;
            int steps = 0;
            while (speed > 0.0f && steps < 10000)
            {
                speed = Mob::railSpeedStep(speed, false, top, 0.02f);
                coasted += speed * 0.02f;
                ++steps;
            }
            check(speed == 0.0f, "and let go, it comes to a stop");
            check(coasted > 8.0f, "after coasting " + std::to_string(static_cast<int>(coasted)) +
                                      " blocks, which is worth building track for");

            for (int step = 0; step < 50; ++step)
                speed = Mob::railSpeedStep(speed, false, top, 0.02f);
            check(speed == 0.0f, "and a stopped cart stays stopped rather than reversing");
        }

        // Every rail shape must guide, from any heading, to one of its
        // own ends. A shape that sent a cart off the track would derail
        // it into the scenery.
        for (BlockId id : { Blocks::RailNorthSouth, Blocks::RailEastWest,
                            Blocks::RailNorthEast, Blocks::RailSouthEast,
                            Blocks::RailSouthWest, Blocks::RailNorthWest })
        {
            bool always = true;
            for (int degrees = -180; degrees < 180; degrees += 15)
            {
                glm::vec3 snapped(0.0f);
                float yaw = 0.0f;
                if (!Mob::railGuide(id, glm::ivec3(0, 64, 0), glm::vec3(0.5f, 64.1f, 0.5f),
                                    static_cast<float>(degrees), snapped, yaw))
                {
                    always = false;
                    break;
                }

                bool onTrack = false;
                for (int d = 0; d < 4; ++d)
                    if (railLinks(id, d) && std::fabs(yaw - Mob::railYaw(d)) < 0.001f)
                        onTrack = true;
                if (!onTrack) always = false;
            }
            check(always, std::string("a cart never leaves a ") + blockInfo(id).name +
                              " anywhere but its own two ends");
        }
    }

    void testWoods()
    {
        section("more than one kind of tree");

        const BlockId LOGS[] = { Blocks::Log, Blocks::BirchLog,
                                 Blocks::SpruceLog, Blocks::DarkOakLog };
        const BlockId LEAVES[] = { Blocks::Leaves, Blocks::BirchLeaves,
                                   Blocks::SpruceLeaves, Blocks::DarkOakLeaves };
        const BlockId PLANKS[] = { Blocks::Planks, Blocks::SprucePlanks,
                                   Blocks::DarkOakPlanks };

        // Every family test has to agree with itself, or a wood is a
        // log to one part of the game and stone to another -- which is
        // exactly what happened to the first two: they fell through to
        // Material::Stone and wanted a pickaxe.
        for (BlockId log : LOGS)
        {
            check(isLog(log), std::string(blockInfo(log).name) + " is a log");
            check(blockMaterial(log) == Material::Wood, "and it is made of wood");
            check(Smelting::burnSeconds(log) > 0.0f, "and it burns");
            check(std::string(blockSoundGroup(log)) == "wood", "and it sounds like wood");
        }
        for (BlockId leaf : LEAVES)
        {
            check(isLeaves(leaf), std::string(blockInfo(leaf).name) + " is leaves");
            check(blockMaterial(leaf) == Material::Plant, "and comes apart like a plant");
            check(Tools::canHarvest(Blocks::Air, leaf), "and bare hands get it");
            check(!isOpaque(leaf), "and light gets through it");
        }
        for (BlockId plank : PLANKS)
        {
            check(isPlanks(plank), std::string(blockInfo(plank).name) + " is planks");
            check(blockMaterial(plank) == Material::Wood, "and made of wood");
        }

        // A log gives up the planks of its own wood.
        for (BlockId log : LOGS)
        {
            ItemStack grid[9];
            grid[0].id = log; grid[0].count = 1;
            const CraftOutput out = Crafting::match(grid, 2);
            check(out.valid() && out.count == 4,
                  std::string(blockInfo(log).name) + " makes four planks");
            check(isBlockStack(out.id) && isPlanks(asBlock(out.id)),
                  "and what it makes is planks");
            check(out.id == planksFromLog(log), "of its own wood");
        }

        // And any plank will do wherever a recipe asks for planks: a
        // spruce bench is a bench. Without this a new wood can be cut
        // and stacked and used for nothing at all.
        for (BlockId plank : PLANKS)
        {
            ItemStack bench[9];
            for (int i : { 0, 1, 3, 4 }) { bench[i].id = plank; bench[i].count = 1; }
            const CraftOutput out = Crafting::match(bench, 2);
            check(out.valid() && out.id == Blocks::CraftingTable,
                  std::string(blockInfo(plank).name) + " makes a bench like any other");
        }

        // Mixed woods too, which is what a player's grid actually looks
        // like after chopping two kinds of tree.
        {
            ItemStack mixed[9];
            mixed[0].id = Blocks::Planks;         mixed[0].count = 1;
            mixed[1].id = Blocks::SprucePlanks;   mixed[1].count = 1;
            mixed[3].id = Blocks::DarkOakPlanks;  mixed[3].count = 1;
            mixed[4].id = Blocks::Planks;         mixed[4].count = 1;
            const CraftOutput out = Crafting::match(mixed, 2);
            check(out.valid() && out.id == Blocks::CraftingTable,
                  "and four different planks still make one bench");
        }
    }

    void testIronGolem()
    {
        section("the iron golem");

        const MobType& golem = mobType(MobId::IronGolem);
        check(golem.spawnClass == SpawnClass::Passive,
              "a golem is not a monster: it spawns in its village in daylight");
        check(golem.neutral, "but it is neutral, so it fights once provoked");
        check(golem.attackDamage > mobType(MobId::Zombie).attackDamage,
              "and it hits harder than a zombie, which is the point of one");
        check(golem.maxHealth > mobType(MobId::Villager).maxHealth * 4,
              "it takes an enormous amount of killing");
        check(golem.height > 2.0f, "and it is taller than you are");
        check(golem.walkSpeed < mobType(MobId::Villager).walkSpeed + 0.5f,
              "it is slow, which is what you can do about one");
        check(golem.experience == 0, "and killing it teaches you nothing");

        // What it leaves behind. Iron, which is the whole reason anyone
        // builds one, and the poppy it was holding.
        check(golem.drop == Items::IronIngot && golem.dropCount >= 3,
              "it drops the iron it was made of");
        check(golem.secondDrop == Blocks::FlowerRed,
              "and the poppy");

        // Provocation. It must not come for you unasked, or a village
        // would be somewhere you could not walk into.
        check(!Mob::hostileNow(golem, false), "it ignores you until you hit it");
        check(Mob::hostileNow(golem, true), "and then it does not");

        // It must not burn at noon, which anything passive avoids but
        // is worth saying out loud for the one mob built to stand in a
        // village square all day.
        check(!Mob::burnsNow(golem, 1.0f, 15, false),
              "and it stands in the sun all day without catching light");

        // Its sheet is the big one, and every patch has to land on it.
        check(golem.sheetWidth == 128 && golem.sheetHeight == 128,
              "it is drawn on a sheet of its own size");
        check(MobSkin::fits(golem.model, golem.sheetWidth, golem.sheetHeight),
              "and every patch of it lands on that sheet");
    }

    void testWolves()
    {
        section("the wolf: wild until it is given a bone");

        const MobType& wolf = mobType(MobId::Wolf);
        check(wolf.spawnClass == SpawnClass::Passive,
              "a wolf is passive: a pack in a forest is to walk past, not run from");
        check(wolf.maxHealth > 0 && wolf.width > 0.0f && wolf.height > 0.0f,
              "and is a real animal with a size and a life");
        check(!wolf.model.empty(), "and something to draw");
        check(wolf.walkSpeed > mobType(MobId::Cow).walkSpeed,
              "it keeps up with you, which a cow does not");

        check(Mob::tamingFood(MobId::Wolf) == Items::Bone, "a bone is what tames one");

        // Sitting. Only something tamed takes an order, and a horse is
        // ridden rather than told.
        check(Mob::canSit(MobId::Wolf, true), "a tamed wolf can be told to sit");
        check(!Mob::canSit(MobId::Wolf, false), "a wild one cannot");
        check(!Mob::canSit(MobId::Horse, true), "and a horse is ridden, not told");
        for (MobId other : { MobId::Cow, MobId::Pig, MobId::Villager, MobId::Zombie,
                             MobId::IronGolem })
            check(!Mob::canSit(other, true),
                  std::string(mobType(other).name) + " takes no orders");

        {
            Mob pet(MobId::Wolf, glm::vec3(0.0f, 70.0f, 0.0f), 31u);
            check(!pet.sitting(), "a wolf starts on its feet");

            // Untamed, the order does nothing at all: a wild wolf is
            // not yours to tell.
            pet.toggleSit();
            check(!pet.sitting(), "and an untamed one ignores being told");

            pet.tameWith(Items::Bone);
            pet.toggleSit();
            check(pet.sitting(), "a tamed one sits when it is told");
            pet.toggleSit();
            check(!pet.sitting(), "and gets up when it is told again");
        }
        for (MobId other : { MobId::Cow, MobId::Pig, MobId::Sheep, MobId::Villager,
                             MobId::Zombie })
            check(Mob::tamingFood(other) == Blocks::Air,
                  std::string(mobType(other).name) + " cannot be tamed");

        {
            Mob pup(MobId::Wolf, glm::vec3(0.0f, 70.0f, 0.0f), 11u);
            check(!pup.tamed(), "a wolf starts wild");
            check(!pup.tameWith(Items::Wheat), "wheat does nothing to it");
            check(!pup.tamed(), "and leaves it wild");

            check(pup.tameWith(Items::Bone), "a bone tames it");
            check(pup.tamed(), "and it stays tamed");
            check(!pup.tameWith(Items::Bone),
                  "a second bone does nothing, so one is not spent for no reason");
        }
        {
            // Nothing else takes the bone, so offering one to a cow
            // does not quietly swallow it.
            Mob cow(MobId::Cow, glm::vec3(0.0f, 70.0f, 0.0f), 12u);
            check(!cow.tameWith(Items::Bone), "a cow does not take a bone");
            check(!cow.tamed(), "and is not tamed by one");
        }
        {
            Mob dead(MobId::Wolf, glm::vec3(0.0f, 70.0f, 0.0f), 13u);
            dead.damage(1000, glm::vec3(0.0f));
            check(!dead.tameWith(Items::Bone), "a dead wolf takes nothing");
        }

        // It drops nothing: a wolf is a companion, not livestock.
        check(wolf.drop == Blocks::Air, "a wolf is not something you butcher");

        // Set on something, it goes at it rather than heeling.
        {
            Mob dog(MobId::Wolf, glm::vec3(0.0f, 70.0f, 0.0f), 14u);
            dog.tameWith(Items::Bone);
            check(!dog.hunting(), "a tamed wolf is not hunting to begin with");

            dog.huntTowards(glm::vec3(10.0f, 70.0f, 0.0f));
            check(dog.hunting(), "and is once it is set on something");

            // Pointed at where it already stands, there is nothing to
            // go at and nothing to divide by.
            Mob still(MobId::Wolf, glm::vec3(5.0f, 70.0f, 5.0f), 15u);
            still.tameWith(Items::Bone);
            still.huntTowards(glm::vec3(5.0f, 70.0f, 5.0f));
            check(!still.hunting(), "being set on its own feet sets it on nothing");
        }

        // It bites on a clock, not every frame it is in reach.
        {
            Mob dog(MobId::Wolf, glm::vec3(0.0f, 70.0f, 0.0f), 16u);
            dog.tameWith(Items::Bone);

            check(dog.tryBite(0.0f), "the first bite lands at once");
            check(!dog.tryBite(0.0f), "and the next does not");

            int bites = 0;
            for (int i = 0; i < 100; ++i) if (dog.tryBite(0.1f)) ++bites;
            check(bites >= 9 && bites <= 11,
                  "ten seconds is about ten bites, not a hundred");
        }

        check(Mob::BITE_DAMAGE > 0, "a bite is worth something");
        check(Mob::BITE_REACH < Mob::HUNT_RANGE,
              "it has to reach what it is going at before it can bite it");
    }

    void testShield()
    {
        section("the shield: what it stops");

        const glm::vec3 me(0.0f, 70.0f, 0.0f);
        const glm::vec3 north(0.0f, 0.0f, -1.0f);

        // Straight ahead is covered; behind is not.
        check(Shield::covers(north, me, me + glm::vec3(0.0f, 0.0f, -5.0f)),
              "a blow from straight ahead lands on the shield");
        check(!Shield::covers(north, me, me + glm::vec3(0.0f, 0.0f, 5.0f)),
              "one from behind does not");
        check(!Shield::covers(north, me, me + glm::vec3(5.0f, 0.0f, 0.0f)),
              "nor one from straight to the side");

        // Off to one side but still in front.
        check(Shield::covers(north, me, me + glm::vec3(2.0f, 0.0f, -5.0f)),
              "a blow a little off the front still lands on it");
        check(Shield::covers(north, me, me + glm::vec3(-2.0f, 0.0f, -5.0f)),
              "on either side of the front");

        // Height is not distance: something above you is still ahead.
        check(Shield::covers(north, me, me + glm::vec3(0.0f, 9.0f, -4.0f)),
              "something standing over you is still in front of you");
        check(!Shield::covers(north, me, me + glm::vec3(0.0f, 9.0f, 4.0f)),
              "and standing over you from behind is still behind");

        // Degenerate inputs decide nothing rather than crashing.
        check(!Shield::covers(glm::vec3(0.0f), me, me + glm::vec3(0.0f, 0.0f, -5.0f)),
              "looking nowhere covers nothing");
        check(!Shield::covers(north, me, me), "and a blow from where you stand covers nothing");

        // Turning to meet it is the whole of using one.
        {
            const glm::vec3 from = me + glm::vec3(5.0f, 0.0f, 0.0f);
            check(!Shield::covers(north, me, from), "caught side-on, it does not cover");
            check(Shield::covers(glm::vec3(1.0f, 0.0f, 0.0f), me, from),
                  "and covers once you have turned to face it");
        }

        // What gets through.
        check(Shield::damageThrough(7, true) == 0, "a blow that is blocked does nothing");
        check(Shield::damageThrough(7, false) == 7, "and one that is not comes through whole");
        check(Shield::damageThrough(0, false) == 0, "nothing stays nothing");

        // It wears out, and iron mends it.
        check(Anvil::maxDurability(Items::Shield) == Shield::MAX_DURABILITY,
              "a shield has a life to spend");
        check(Anvil::wearsOut(Items::Shield), "so the anvil knows it wears out");
        check(Anvil::repairMaterial(Items::Shield) == Items::IronIngot,
              "and that iron is what mends it");
        check(Shield::WEAR_PER_BLOCK > 0, "every blow it stops costs it something");

        // Holding one up slows you down, or it would be worn always.
        check(Shield::RAISED_SPEED > 0.0f && Shield::RAISED_SPEED < 1.0f,
              "a raised shield slows you, but does not root you");
        {
            Player walking(glm::vec3(0.0f, 70.0f, 0.0f));
            check(!walking.blocking, "and nobody starts with one up");
        }

        // One stacks to a single slot: two of them are rarely the same
        // shield, and a stack has one damage field between them.
        check(maxStackOf(Items::Shield) == 1, "a shield does not stack");
        check(displayName(Items::Shield)[0] != '\0', "and has a name");
    }

    void testAnvil()
    {
        section("the anvil: mending and moving enchantments");

        using Anvil::Input;
        using Enchanting::Kind;

        // What mends what.
        check(Anvil::repairMaterial(Items::IronPickaxe) == Items::IronIngot,
              "iron tools take iron");
        check(Anvil::repairMaterial(Items::WoodPickaxe) == Blocks::Planks,
              "wooden ones take planks");
        check(Anvil::repairMaterial(Items::StonePickaxe) == Blocks::Cobblestone,
              "stone ones take cobblestone");
        check(Anvil::repairMaterial(Items::DiamondSword) == Items::Diamond,
              "diamond gear takes diamonds");
        check(Anvil::repairMaterial(Items::LeatherBoots) == Items::Leather,
              "leather armour takes leather");
        check(Anvil::repairMaterial(Items::IronChestplate) == Items::IronIngot,
              "iron armour takes iron");
        check(Anvil::repairMaterial(Blocks::Cobblestone) == Blocks::Air,
              "a block takes nothing: it does not wear out");

        check(Anvil::wearsOut(Items::IronPickaxe), "a pickaxe wears out");
        check(Anvil::wearsOut(Items::IronChestplate), "so does armour");
        check(!Anvil::wearsOut(Blocks::Cobblestone), "cobblestone does not");
        check(!Anvil::wearsOut(Items::Stick), "nor does a stick");

        const int pickMax = Anvil::maxDurability(Items::IronPickaxe);
        check(pickMax > 0, "an iron pickaxe has a life to spend");
        const int step = std::max(1, (pickMax * Anvil::REPAIR_PERCENT + 99) / 100);

        // --- mending with raw material ---------------------------------
        {
            Input worn{ Items::IronPickaxe, 1, pickMax - 1, 0 };   // nearly dead
            Input iron{ Items::IronIngot, 64, 0, 0 };
            const Anvil::Result r = Anvil::combine(worn, iron);
            check(r.valid, "an ingot mends a worn pickaxe");
            check(r.id == Items::IronPickaxe, "and what comes out is still a pickaxe");
            check(r.damage == 0, "and with a stack to hand it comes out whole");
            check(r.levelCost == r.used, "one level an ingot");
            check(r.used > 0 && r.used <= 4,
                  "four ingots is the most a whole repair ever takes");
        }
        {
            // One ingot and no more: a quarter of its life back.
            Input worn{ Items::IronPickaxe, 1, pickMax - 1, 0 };
            Input one{ Items::IronIngot, 1, 0, 0 };
            const Anvil::Result r = Anvil::combine(worn, one);
            check(r.valid && r.used == 1, "one ingot is spent when one is all there is");
            check(r.damage == (pickMax - 1) - step, "mending a quarter of its life");
            check(r.damage > 0, "and leaving the rest of the wear behind");
        }
        {
            // Barely scratched: it should spend one ingot, not the stack.
            Input nicked{ Items::IronPickaxe, 1, 1, 0 };
            Input iron{ Items::IronIngot, 64, 0, 0 };
            const Anvil::Result r = Anvil::combine(nicked, iron);
            check(r.valid && r.used == 1, "a scratch costs one ingot, not the stack");
            check(r.damage == 0, "and it comes out whole");
        }
        {
            Input whole{ Items::IronPickaxe, 1, 0, 0 };
            Input iron{ Items::IronIngot, 64, 0, 0 };
            check(!Anvil::combine(whole, iron).valid,
                  "a pickaxe with nothing wrong with it is refused");
        }
        {
            Input worn{ Items::IronPickaxe, 1, pickMax - 1, 0 };
            Input gold{ Items::GoldIngot, 64, 0, 0 };
            check(!Anvil::combine(worn, gold).valid, "the wrong metal is refused");
        }

        // --- books ------------------------------------------------------
        {
            Input pick{ Items::IronPickaxe, 1, 0, 0 };
            Input book{ Items::Book, 1,  0,
                        Enchanting::withLevel(0u, Kind::Efficiency, 3) };
            const Anvil::Result r = Anvil::combine(pick, book);
            check(r.valid, "a book of efficiency goes onto a pickaxe");
            check(Enchanting::levelOf(r.enchantments, Kind::Efficiency) == 3,
                  "at the level the book carried");
            check(r.levelCost == 3, "costing a level a level");
            check(r.used == 1, "and the book is spent");
        }
        {
            // Power belongs on a bow and nowhere else.
            Input pick{ Items::IronPickaxe, 1, 0, 0 };
            Input book{ Items::Book, 1, 0, Enchanting::withLevel(0u, Kind::Power, 2) };
            check(!Anvil::combine(pick, book).valid,
                  "a book of power does nothing to a pickaxe");
        }
        {
            // Two of a level make the next one up.
            Input pick{ Items::IronPickaxe, 1, 0,
                        Enchanting::withLevel(0u, Kind::Efficiency, 2) };
            Input book{ Items::Book, 1, 0,
                        Enchanting::withLevel(0u, Kind::Efficiency, 2) };
            const Anvil::Result r = Anvil::combine(pick, book);
            check(r.valid, "two of the same level combine");
            check(Enchanting::levelOf(r.enchantments, Kind::Efficiency) == 3,
                  "into the next one up");
        }
        {
            // A weaker book is absorbed and gains nothing.
            Input pick{ Items::IronPickaxe, 1, 0,
                        Enchanting::withLevel(0u, Kind::Efficiency, 4) };
            Input book{ Items::Book, 1, 0,
                        Enchanting::withLevel(0u, Kind::Efficiency, 2) };
            check(!Anvil::combine(pick, book).valid,
                  "a weaker book adds nothing and is refused");
        }
        {
            // The ceiling holds.
            const int top = Enchanting::maxLevel(Kind::Efficiency);
            Input pick{ Items::IronPickaxe, 1, 0,
                        Enchanting::withLevel(0u, Kind::Efficiency, top) };
            Input book{ Items::Book, 1, 0,
                        Enchanting::withLevel(0u, Kind::Efficiency, top) };
            check(!Anvil::combine(pick, book).valid,
                  "two at the ceiling make nothing higher");
        }
        {
            // Fortune and silk touch will not share an item.
            Input pick{ Items::IronPickaxe, 1, 0,
                        Enchanting::withLevel(0u, Kind::SilkTouch, 1) };
            Input book{ Items::Book, 1, 0,
                        Enchanting::withLevel(0u, Kind::Fortune, 2) };
            const Anvil::Result r = Anvil::combine(pick, book);
            check(!r.valid, "fortune will not go onto a silk touch pickaxe");
        }
        {
            // Two different kinds both land.
            Input pick{ Items::IronPickaxe, 1, 0,
                        Enchanting::withLevel(0u, Kind::Efficiency, 2) };
            Enchanting::Set two = Enchanting::withLevel(0u, Kind::Unbreaking, 2);
            two = Enchanting::withLevel(two, Kind::Fortune, 1);
            Input book{ Items::Book, 1, 0, two };
            const Anvil::Result r = Anvil::combine(pick, book);
            check(r.valid, "a book of two enchantments lands");
            check(Enchanting::levelOf(r.enchantments, Kind::Efficiency) == 2,
                  "what was there stays");
            check(Enchanting::levelOf(r.enchantments, Kind::Unbreaking) == 2 &&
                  Enchanting::levelOf(r.enchantments, Kind::Fortune) == 1,
                  "and both of the new ones arrive");
        }

        // --- joining two of the same item -------------------------------
        {
            Input a{ Items::IronPickaxe, 1, pickMax * 3 / 4, 0 };
            Input b{ Items::IronPickaxe, 1, pickMax * 3 / 4, 0 };
            const Anvil::Result r = Anvil::combine(a, b);
            check(r.valid, "two worn pickaxes make one better one");
            check(r.damage < a.damage, "with less wear than either had");
            check(r.used == 1, "spending the second one");
        }
        {
            Input a{ Items::IronPickaxe, 1, 0, 0 };
            Input b{ Items::IronSword, 1, 0, 0 };
            check(!Anvil::combine(a, b).valid, "a sword does not mend a pickaxe");
        }
        {
            Input a{ Blocks::Cobblestone, 64, 0, 0 };
            Input b{ Blocks::Cobblestone, 64, 0, 0 };
            check(!Anvil::combine(a, b).valid, "and cobblestone is not anvil work");
        }
        {
            Input nothing{};
            Input pick{ Items::IronPickaxe, 1, 10, 0 };
            check(!Anvil::combine(nothing, pick).valid, "an empty left slot makes nothing");
            check(!Anvil::combine(pick, nothing).valid, "nor an empty right one");
        }

        // --- the ceiling on price ---------------------------------------
        {
            // Stack enough levels onto one item to pass forty.
            Enchanting::Set rich = 0;
            for (int k = 0; k < static_cast<int>(Kind::Count); ++k)
            {
                const Kind kind = static_cast<Kind>(k);
                if (!Enchanting::appliesTo(kind, Items::DiamondSword)) continue;
                if (kind == Kind::Fortune) continue;   // fights silk touch
                rich = Enchanting::withLevel(rich, kind, Enchanting::maxLevel(kind));
            }
            Input sword{ Items::DiamondSword, 1, 0, 0 };
            Input book{ Items::Book, 1, 0, rich };
            const Anvil::Result r = Anvil::combine(sword, book);
            // Either it fits under the ceiling or it is refused for price,
            // but it must never come out valid at or above it.
            check(!(r.valid && r.levelCost >= Anvil::TOO_EXPENSIVE),
                  "nothing valid ever costs forty levels or more");
            if (r.tooExpensive) check(!r.valid, "and too expensive means no result");
        }
    }

    void testEndermanStare()
    {
        section("mobs: the enderman waits to be provoked");

        const MobType& ender = mobType(MobId::Enderman);
        check(ender.neutral, "the enderman is neutral");
        check(ender.spawnClass == SpawnClass::Hostile,
              "and still carries a hostile's health and reach");

        for (MobId other : { MobId::Zombie, MobId::Skeleton, MobId::Creeper, MobId::Spider })
            check(!mobType(other).neutral, std::string(mobType(other).name) + " is not neutral");

        Mob calm(MobId::Enderman, glm::vec3(0.0f), 1u);
        check(!calm.provoked(), "it starts unprovoked");
        check(!calm.hostileNow(), "and will not come after the player");

        Mob hit(MobId::Enderman, glm::vec3(0.0f), 1u);
        hit.damage(1, glm::vec3(1.0f, 0.0f, 0.0f));
        check(hit.provoked(), "hitting it provokes it");
        check(hit.hostileNow(), "and then it comes after the player");

        // The sun passes no direction, and a mob burning in daylight has
        // nobody to go after.
        Mob burnt(MobId::Enderman, glm::vec3(0.0f), 1u);
        burnt.damage(1, glm::vec3(0.0f));
        check(!burnt.provoked(), "burning in the sun does not provoke it");

        Mob zombie(MobId::Zombie, glm::vec3(0.0f), 1u);
        check(zombie.hostileNow(), "a zombie needs no provoking");

        // Staring. The enderman stands at the origin, 2.9 tall, so its
        // head is just under 2.9 up.
        const float tall = ender.height;
        const glm::vec3 feet(0.0f, 0.0f, 0.0f);
        const glm::vec3 head(0.0f, tall - Stare::HEAD_RADIUS, 0.0f);

        const glm::vec3 eye(0.0f, 1.62f, -10.0f);
        check(Stare::atHead(eye, glm::normalize(head - eye), feet, tall),
              "looking straight at the head provokes it");
        check(!Stare::atHead(eye, glm::normalize(feet - eye), feet, tall),
              "looking at its feet does not");
        check(!Stare::atHead(eye, glm::vec3(0.0f, 0.0f, -1.0f), feet, tall),
              "looking the other way does not");

        // The crosshair has to be on the head at any range, so the same
        // small angle that hits at ten blocks misses at sixty.
        const glm::vec3 near(0.0f, 1.62f, -4.0f);
        const glm::vec3 far(0.0f, 1.62f, -50.0f);
        const glm::vec3 offBy(0.9f, 0.0f, 0.0f);
        check(!Stare::atHead(near, glm::normalize(head + offBy - near), feet, tall),
              "a head's width off to the side misses up close");
        check(!Stare::atHead(far, glm::normalize(head + offBy - far), feet, tall),
              "and misses at range too");
        check(Stare::atHead(far, glm::normalize(head - far), feet, tall),
              "dead on still counts at fifty blocks");

        // Past its range it does not care.
        const glm::vec3 miles(0.0f, 1.62f, -200.0f);
        check(!Stare::atHead(miles, glm::normalize(head - miles), feet, tall),
              "beyond sixty-four blocks it does not notice");

        check(!Stare::atHead(eye, glm::vec3(0.0f), feet, tall),
              "a zero look direction provokes nothing");
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
            { MobId::Spider,   Items::StringItem,   Items::SpiderEye },
            { MobId::Zombie,   Items::RottenFlesh,  Blocks::Air },
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

    // --- the other two crops --------------------------------------------

    // Carrots and potatoes work the way wheat does -- eight ids, one
    // per stage -- but they are their own seed, so a ripe one pays
    // several of itself rather than a crop and a seed.
    void testRootCrops()
    {
        section("carrots and potatoes");

        const BlockId FIRST[] = { Blocks::Wheat0, Blocks::Carrot0, Blocks::Potato0,
                                  Blocks::Beetroot0, Blocks::MelonStem0 };
        const char* NAMES[] = { "wheat", "carrot", "potato", "beetroot", "melon stem" };
        constexpr int KINDS = 5;

        for (int c = 0; c < KINDS; ++c)
        {
            const std::string what = NAMES[c];

            for (int stage = 0; stage < WHEAT_STAGES; ++stage)
            {
                const BlockId at = cropAtStage(FIRST[c], stage);

                check(at == static_cast<BlockId>(FIRST[c] + stage),
                      what + " stage " + std::to_string(stage) + " is its own id");
                check(isCrop(at), what + " reads as a crop");
                check(cropStage(at) == stage, "and knows which stage it is at");

                // Exactly one of the three predicates may claim it, or
                // a carrot would harvest as wheat.
                const int claims = (isWheat(at) ? 1 : 0) + (isCarrot(at) ? 1 : 0) +
                                   (isPotato(at) ? 1 : 0) + (isBeetroot(at) ? 1 : 0) +
                                   (isMelonStem(at) ? 1 : 0);
                check(claims == 1, "and only one crop claims it");

                check(!isObtainable(at), "a crop is planted, never held");
                check(blockMaterial(at) == Material::Plant, "and comes apart like a plant");
                check(Tools::canHarvest(Blocks::Air, at), "bare hands are enough");
                check(!blockInfo(at).collides, "and you walk through it");
                check(isCropRipe(at) == (stage == WHEAT_STAGES - 1),
                      "only the last stage is ripe");
            }

            // Growing past ripe would walk off the end of the ids and
            // into whatever block comes next.
            check(cropAtStage(FIRST[c], 99) == cropAtStage(FIRST[c], 7),
                  what + " cannot grow past ripe");
            check(cropAtStage(FIRST[c], -5) == FIRST[c], "nor shrink past nothing");
        }

        check(!isCrop(Blocks::Farmland) && !isCrop(Blocks::Grass),
              "earth is not a crop");
        check(!isCropRipe(Blocks::Grass), "and cannot be ripe");

        // What you plant. For the roots the seed and the food are one
        // item, which is Minecraft's and is why a carrot is worth
        // picking up.
        check(Farming::cropFromSeed(Items::WheatSeeds) == Blocks::Wheat0,
              "seeds sow wheat");
        check(Farming::cropFromSeed(Items::Carrot) == Blocks::Carrot0,
              "a carrot sows a carrot");
        check(Farming::cropFromSeed(Items::Potato) == Blocks::Potato0,
              "a potato sows a potato");
        check(Farming::cropFromSeed(Items::BeetrootSeeds) == Blocks::Beetroot0,
              "beetroot seeds sow a beet");
        check(Farming::cropFromSeed(Items::MelonSeeds) == Blocks::MelonStem0,
              "and melon seeds sow a stem, not a melon");
        check(Farming::cropFromSeed(Items::BakedPotato) == Blocks::Air,
              "a baked one does not: it has been cooked");
        check(Farming::cropFromSeed(Items::Bread) == Blocks::Air,
              "and bread sows nothing");

        // Pulled up early, a root gives back exactly what it cost.
        for (int c = 1; c < 3; ++c)
        {
            const StackId seed = (c == 1) ? Items::Carrot : Items::Potato;
            const Farming::Harvest young = Farming::harvestOf(
                cropAtStage(FIRST[c], 2), 0);
            check(young.first == seed && young.firstCount == 1,
                  std::string("an unripe ") + NAMES[c] + " gives back one");
            check(young.secondCount == 0, "and nothing else");

            bool alwaysPays = true;
            for (uint32_t roll = 0; roll < 16; ++roll)
            {
                const Farming::Harvest ripe = Farming::harvestOf(
                    cropAtStage(FIRST[c], 7), roll);
                if (ripe.first != seed) alwaysPays = false;
                if (ripe.firstCount < 2 || ripe.firstCount > 5) alwaysPays = false;
                if (ripe.secondCount != 0 && ripe.second != Items::PoisonousPotato) alwaysPays = false;
            }
            check(alwaysPays, std::string("a ripe ") + NAMES[c] + " pays two to five of itself");
        }

        // Eating. A raw potato is the worst food in the game and a
        // baked one is worth as much as bread, which is the whole
        // reason the furnace is worth walking back to.
        check(Food::isEdible(Items::Carrot), "a carrot is food");
        check(Food::isEdible(Items::Potato), "so is a raw potato");
        check(Food::isEdible(Items::BakedPotato), "and a baked one");
        check(Food::valueOf(Items::BakedPotato).hunger >
                  Food::valueOf(Items::Potato).hunger,
              "baking a potato is worth doing");
        check(Smelting::resultOf(Items::Potato) == Items::BakedPotato,
              "the furnace is what bakes it");
        check(Smelting::resultOf(Items::Carrot) == Blocks::Air,
              "a carrot does not cook");
        check(Smelting::resultOf(Items::BakedPotato) == Blocks::Air,
              "and a baked potato does not bake twice");

        // A beet is the seed and the food apart, the way wheat is, so
        // it has to pay back at least the seed it cost or a field of
        // them runs down.
        const Farming::Harvest youngBeet = Farming::harvestOf(Blocks::Beetroot4, 0);
        check(youngBeet.first == Items::BeetrootSeeds && youngBeet.firstCount == 1,
              "an unripe beet gives back one seed");
        bool beetPays = true;
        for (uint32_t roll = 0; roll < 16; ++roll)
        {
            const Farming::Harvest ripe = Farming::harvestOf(Blocks::Beetroot7, roll);
            if (ripe.first != Items::Beetroot || ripe.firstCount != 1) beetPays = false;
            if (ripe.second != Items::BeetrootSeeds) beetPays = false;
            if (ripe.secondCount < 1 || ripe.secondCount > 3) beetPays = false;
        }
        check(beetPays, "a ripe beet gives a beetroot and one to three seeds");

        // A stem is the odd one: what it ripens into stands beside it,
        // so breaking the stem at any stage gives back only the seed.
        for (int stage = 0; stage < WHEAT_STAGES; ++stage)
        {
            const Farming::Harvest cut =
                Farming::harvestOf(cropAtStage(Blocks::MelonStem0, stage), 0);
            check(cut.first == Items::MelonSeeds && cut.firstCount == 1,
                  "a melon stem gives back one seed whatever stage it is at");
            check(cut.secondCount == 0, "and nothing else");
        }

        // The items are easy to mix up in the table that names them,
        // and getting that wrong once called a nether star an apple.
        // Each must have its own name and its own icon.
        const StackId NEW_ITEMS[] = {
            Items::Carrot, Items::Potato, Items::BakedPotato, Items::Bowl,
            Items::Beetroot, Items::BeetrootSeeds, Items::BeetrootSoup,
            Items::MelonSlice, Items::MelonSeeds,
        };
        const char* WANTED[] = {
            "Carrot", "Potato", "Baked Potato", "Bowl",
            "Beetroot", "Beetroot Seeds", "Beetroot Soup",
            "Melon Slice", "Melon Seeds",
        };
        constexpr int NEW_COUNT = 9;
        for (int i = 0; i < NEW_COUNT; ++i)
        {
            check(std::string(itemInfo(NEW_ITEMS[i]).name) == WANTED[i],
                  std::string("the item table calls it a ") + WANTED[i]);
            for (int j = i + 1; j < NEW_COUNT; ++j)
                check(itemInfo(NEW_ITEMS[i]).tile != itemInfo(NEW_ITEMS[j]).tile,
                      std::string(WANTED[i]) + " and " + WANTED[j] +
                          " do not share an icon");
        }
    }

    // --- the melon and the bamboo ---------------------------------------

    // The two plants that are not crops: one grows beside its stem
    // rather than on it, and one grows straight up out of the ground.
    void testMelonAndBamboo()
    {
        section("melons and bamboo");

        check(blockDrop(Blocks::Melon) == Items::MelonSlice,
              "a melon comes apart into slices");
        check(Food::isEdible(Items::MelonSlice), "and a slice is food");
        check(isObtainable(Blocks::Melon), "a melon is a block you can hold");
        check(blockInfo(Blocks::Melon).collides, "and stand on");
        check(blockMaterial(Blocks::Melon) == Material::Plant,
              "it comes apart like a plant rather than wanting a pickaxe");
        check(Tools::canHarvest(Blocks::Air, Blocks::Melon), "so bare hands get it");

        // Nine slices back into a melon and one into a seed: the loop
        // that makes a melon worth planting rather than just eating.
        const std::vector<StackId> made = Crafting::everyOutput();
        check(std::find(made.begin(), made.end(),
                        static_cast<StackId>(Blocks::Melon)) != made.end(),
              "slices make a melon back");
        check(std::find(made.begin(), made.end(),
                        static_cast<StackId>(Items::MelonSeeds)) != made.end(),
              "and a slice makes the seed that grows another");
        check(std::find(made.begin(), made.end(),
                        static_cast<StackId>(Items::BeetrootSoup)) != made.end(),
              "and six beets in a bowl make soup");

        // Bamboo: a plant, broken by hand, and the tallest thing in
        // the game that is not a tree.
        check(Farming::BAMBOO_MAX > 8, "bamboo grows taller than a person can reach");
        check(isObtainable(Blocks::Bamboo), "a cane is a thing you hold");
        check(blockDrop(Blocks::Bamboo) == Blocks::Bamboo, "and gives back itself");
        check(blockInfo(Blocks::Bamboo).collides && !blockInfo(Blocks::Bamboo).opaque, "a grove is stalks you weave between, as in 1.21.1");
        check(blockInfo(Blocks::Bamboo).render == RenderType::Cross,
              "and it is drawn as a plant rather than a box");
        check(Tools::canHarvest(Blocks::Air, Blocks::Bamboo), "bare hands cut it");

        // The stem does not stack into the melon's own ids: an
        // off-by-one here would have a ripe stem read as a melon.
        check(!isMelonStem(Blocks::Melon), "a melon is not its own stem");
        check(!isCrop(Blocks::Melon), "nor a crop");
        check(!isCrop(Blocks::Bamboo), "and neither is bamboo");
        check(cropAtStage(Blocks::MelonStem0, 7) == Blocks::MelonStem7,
              "a stem's last stage is its own last id");
    }

    void testPumpkinsAndBees()
    {
        section("pumpkins, hay, berries and bees");

        check(Farming::cropFromSeed(Items::PumpkinSeeds) == Blocks::PumpkinStem0, "pumpkin seeds sow a pumpkin stem");
        check(isCrop(Blocks::PumpkinStem3) && cropAtStage(Blocks::PumpkinStem0, 7) == Blocks::PumpkinStem7,
              "and it ripens through eight stages");
        const Farming::Harvest stem = Farming::harvestOf(Blocks::PumpkinStem7, 0);
        check(stem.first == Items::PumpkinSeeds && stem.firstCount == 1, "a broken pumpkin stem gives back its seed");
        bool attached = true;
        for (int f = 0; f < 4; ++f)
        {
            const BlockId p = attachedStemOf(true, f), m = attachedStemOf(false, f);
            if (!isAttachedStem(p) || !attachedStemIsPumpkin(p) || attachedStemFacing(p) != f) attached = false;
            if (!isAttachedStem(m) || attachedStemIsPumpkin(m) || attachedStemFacing(m) != f) attached = false;
            if (isObtainable(p) || isObtainable(m) || isCrop(p)) attached = false;
        }
        check(attached, "an attached stem knows its fruit and which way it points, and is never carried");
        check(blockDrop(attachedStemOf(true, 1)) == Items::PumpkinSeeds && blockDrop(attachedStemOf(false, 3)) == Items::MelonSeeds,
              "and breaks into the right seed");

        bool faces = true;
        for (int f = 0; f < 4; ++f)
        {
            const BlockId carved = static_cast<BlockId>(Blocks::CarvedPumpkinFirst + f);
            const BlockId jack = static_cast<BlockId>(Blocks::JackOLanternFirst + f);
            if (blockFacing(carved) != f || blockFacing(jack) != f) faces = false;
            if (blockDrop(carved) != Blocks::CarvedPumpkinFirst + 2 || blockDrop(jack) != Blocks::JackOLanternFirst + 2) faces = false;
            if (isObtainable(carved) != (f == 2) || isObtainable(jack) != (f == 2)) faces = false;
        }
        check(faces, "a carved pumpkin and a jack o'lantern turn four ways and come back as one");
        check(lightEmission(Blocks::JackOLanternFirst) == 15 && lightEmission(Blocks::CarvedPumpkinFirst) == 0,
              "the lantern shines and the carving does not");
        check(Armour::slotFor(static_cast<StackId>(Blocks::CarvedPumpkinFirst + 2)) == Armour::slotFor(Items::IronHelmet),
              "a carved pumpkin is worn on the head");
        check(Armour::defencePoints(static_cast<StackId>(Blocks::CarvedPumpkinFirst + 2)) == 0, "and stops nothing");
        check(Inventory::belongsIn(Inventory::ARMOR_FIRST, static_cast<StackId>(Blocks::CarvedPumpkinFirst + 2)),
              "so the helmet slot takes it");

        ItemStack lantern[9];
        lantern[0].id = static_cast<StackId>(Blocks::CarvedPumpkinFirst + 2); lantern[0].count = 1;
        lantern[3].id = Blocks::Torch; lantern[3].count = 1;
        const CraftOutput jack = Crafting::match(lantern, 2);
        check(jack.valid() && jack.id == Blocks::JackOLanternFirst + 2, "a carved pumpkin over a torch makes a jack o'lantern");

        ItemStack shears[9];
        shears[1].id = Items::IronIngot; shears[1].count = 1;
        shears[3].id = Items::IronIngot; shears[3].count = 1;
        const CraftOutput cut = Crafting::match(shears, 2);
        check(cut.valid() && cut.id == Items::Shears, "two iron ingots on a slant make shears");
        check(maxStackOf(Items::Shears) == 1, "and they do not stack");

        ItemStack seeds[9];
        seeds[4].id = Blocks::Pumpkin; seeds[4].count = 1;
        const CraftOutput sown = Crafting::match(seeds, 3);
        check(sown.valid() && sown.id == Items::PumpkinSeeds && sown.count == 4, "a pumpkin makes four seeds");

        auto nine = [&](StackId what) {
            ItemStack grid[9];
            for (ItemStack& cell : grid) { cell.id = what; cell.count = 1; }
            return Crafting::match(grid, 3);
        };
        check(nine(Items::Wheat).id == Blocks::HayBale, "nine wheat bind into a hay bale");
        check(nine(Items::BoneMeal).id == Blocks::BoneBlock, "nine bone meal pack into a bone block");
        check(nine(Items::DriedKelp).id == Blocks::DriedKelpBlock, "nine dried kelp press into a block");
        check(Smelting::resultOf(Blocks::Kelp) == Items::DriedKelp, "kelp dries in a furnace");

        check(Food::isEdible(Items::SweetBerries) && Food::isEdible(Items::HoneyBottle) && Food::isEdible(Items::DriedKelp),
              "berries, honey and dried kelp are food");
        check(maxStackOf(Items::HoneyBottle) == 16, "honey bottles stack to sixteen");

        ItemStack honey[9];
        for (int i : { 0, 1, 3, 4 }) { honey[i].id = Items::HoneyBottle; honey[i].count = 1; }
        const CraftOutput block = Crafting::match(honey, 2);
        check(block.valid() && block.id == Blocks::HoneyBlock, "four honey bottles make a honey block");
        const int extra = Crafting::consume(honey, 2);
        bool bottles = extra == 0;
        for (int i : { 0, 1, 3, 4 }) if (honey[i].id != Items::GlassBottle || honey[i].count != 1) bottles = false;
        check(bottles, "and leave their bottles behind in the grid");
        check(!blockInfo(Blocks::HoneyBlock).opaque, "honey is see-through");

        bool berries = true;
        for (int s = 0; s < 4; ++s)
        {
            const BlockId bush = static_cast<BlockId>(Blocks::SweetBerryBush0 + s);
            if (!isSweetBerryBush(bush) || sweetBerryStage(bush) != s || isObtainable(bush)) berries = false;
            if (blockInfo(bush).collides) berries = false;
        }
        check(berries, "a berry bush grows through four stages, and you walk into it rather than onto it");

        bool hives = true;
        for (int f = 0; f < 4; ++f)
            for (int h = 0; h < 6; ++h)
                for (bool nest : { true, false })
                {
                    const BlockId hive = hiveOf(nest, f, h);
                    if (!isHive(hive) || isBeeNest(hive) != nest || hiveFacing(hive) != f || hiveHoney(hive) != h ||
                        blockFacing(hive) != f) hives = false;
                }
        check(hives, "a hive keeps its facing and how much honey is in it");
        check(blockDrop(hiveOf(false, 3, 5)) == hiveOf(false, 2, 0), "a beehive comes back empty");
        check(blockDrop(hiveOf(true, 1, 2)) == Blocks::Air, "a bee nest gives nothing without silk touch");
        check(nine(Blocks::Planks).id != hiveOf(false, 2, 0), "planks alone are not a hive");
        ItemStack hive[9];
        for (int i : { 0, 1, 2, 6, 7, 8 }) { hive[i].id = Blocks::Planks; hive[i].count = 1; }
        for (int i : { 3, 4, 5 }) { hive[i].id = Items::Honeycomb; hive[i].count = 1; }
        const CraftOutput built = Crafting::match(hive, 3);
        check(built.valid() && built.id == hiveOf(false, 2, 0), "planks and honeycomb make a beehive");
    }

    void testOcean()
    {
        section("corals, the conduit and turtles");

        bool corals = true;
        for (int kind = 0; kind < 5; ++kind)
        {
            const BlockId block = static_cast<BlockId>(Blocks::TubeCoralBlock + kind);
            const BlockId plant = static_cast<BlockId>(Blocks::TubeCoral + kind);
            const BlockId fan = static_cast<BlockId>(Blocks::CoralFanFirst + kind);
            if (coralKind(block) != kind || coralKind(plant) != kind || coralKind(fan) != kind) corals = false;
            if (deadCoralOf(block) != Blocks::DeadCoralBlockFirst + kind) corals = false;
            if (deadCoralOf(plant) != Blocks::DeadCoralFirst + kind) corals = false;
            if (deadCoralOf(fan) != Blocks::DeadCoralFanFirst + kind) corals = false;
            if (!isLivingCoral(block) || isLivingCoral(deadCoralOf(block))) corals = false;
            if (blockDrop(block) != Blocks::DeadCoralBlockFirst + kind) corals = false;
            if (blockDrop(plant) != Blocks::Air || blockDrop(fan) != Blocks::Air) corals = false;
            if (blockDrop(deadCoralOf(block)) != deadCoralOf(block)) corals = false;
            for (int f = 0; f < 4; ++f)
            {
                const BlockId wall = coralWallFanOf(false, kind, f);
                if (!isCoralWallFan(wall) || coralKind(wall) != kind || blockFacing(wall) != f || isObtainable(wall)) corals = false;
                if (deadCoralOf(wall) != coralWallFanOf(true, kind, f) || !isDeadCoralWallFan(deadCoralOf(wall))) corals = false;
                if (!isWaterPlant(wall) || !isWaterPlant(deadCoralOf(wall))) corals = false;
            }
            if (!isWaterPlant(fan) || !isWaterPlant(deadCoralOf(plant)) || isWaterPlant(block)) corals = false;
            if (blockInfo(fan).collides || !blockInfo(block).collides) corals = false;
        }
        check(corals, "every coral has a kind, a dead form and four wall fans, and blocks break into dead blocks");
        check(isWaterPlant(Blocks::TallSeagrassLower) && isWaterPlant(Blocks::TallSeagrassUpper), "tall seagrass stands in water");
        check(!isObtainable(Blocks::TallSeagrassUpper) && blockDrop(Blocks::TallSeagrassLower) == Blocks::Air,
              "and needs shears to take");

        bool eggs = true;
        for (int count = 1; count <= 4; ++count)
            for (int hatch = 0; hatch < 3; ++hatch)
            {
                const BlockId egg = turtleEggOf(count, hatch);
                if (!isTurtleEgg(egg) || turtleEggCount(egg) != count || turtleEggHatch(egg) != hatch) eggs = false;
                if (isObtainable(egg) != (count == 1 && hatch == 0)) eggs = false;
                if (blockDrop(egg) != Blocks::Air) eggs = false;
            }
        check(eggs, "a clutch holds one to four eggs at three stages of cracking, and breaks to nothing without silk");
        check(blockOverlaps(turtleEggOf(2, 0), 0, 0, 0, 0.4f, 0.1f, 0.4f, 0.6f, 0.3f, 0.6f, 0) &&
              !blockOverlaps(turtleEggOf(2, 0), 0, 0, 0, 0.4f, 0.6f, 0.4f, 0.6f, 0.9f, 0.6f, 0),
              "eggs are low enough to step onto");
        check(mobType(MobId::Turtle).breedingFood == Blocks::Seagrass, "turtles are bred with seagrass");

        ItemStack ring[9];
        for (int i = 0; i < 9; ++i) { ring[i].id = i == 4 ? Items::HeartOfTheSea : Items::NautilusShell; ring[i].count = 1; }
        const CraftOutput conduit = Crafting::match(ring, 3);
        check(conduit.valid() && conduit.id == Blocks::Conduit, "eight nautilus shells round a heart of the sea make a conduit");
        check(lightEmission(Blocks::Conduit) == 15 && !blockInfo(Blocks::Conduit).opaque, "a conduit shines and is small");
        check(Tools::canHarvest(Blocks::Air, Blocks::Conduit), "and comes away in the hand");

        ItemStack shell[9];
        for (int i : { 0, 1, 2, 3, 5 }) { shell[i].id = Items::TurtleScute; shell[i].count = 1; }
        const CraftOutput helmet = Crafting::match(shell, 3);
        check(helmet.valid() && helmet.id == Items::TurtleHelmet, "five scutes make a turtle shell");
        check(Armour::slotFor(Items::TurtleHelmet) == Armour::slotFor(Items::IronHelmet), "worn on the head");
        check(Armour::defencePoints(Items::TurtleHelmet) == 2 && Armour::maxDurability(Items::TurtleHelmet) == 275,
              "with two points of armour and the iron helmet's wear");
        check(Anvil::repairMaterial(Items::TurtleHelmet) == Items::TurtleScute, "and mended with a scute");

        Effects::Set power;
        Effects::give(power, Effects::Kind::ConduitPower, 0, 13.0f);
        check(Effects::breathesWater(power), "conduit power lets you breathe under water");
        check(Effects::digMultiplier(power) > 1.0f, "and dig faster");
        check(StructureLoot::centrepiece(WorldGen::Hoard::BuriedTreasure, 0, 40, 0, 7u).id == Items::HeartOfTheSea,
              "buried treasure always holds a heart of the sea");
    }

    void testIceAndSnow()
    {
        section("ice, frost walker and powder snow");

        check(isSlippery(Blocks::Ice) && isSlippery(Blocks::PackedIce) && isSlippery(Blocks::BlueIce) &&
              isSlippery(Blocks::FrostedIce2) && !isSlippery(Blocks::Snow), "the four ices are slippery and snow is not");
        check(blockDrop(Blocks::PackedIce) == Blocks::Air && blockDrop(Blocks::BlueIce) == Blocks::Air,
              "packed and blue ice need silk touch");
        check(!isObtainable(Blocks::FrostedIce0) && !isObtainable(Blocks::PowderSnow), "frosted ice and powder snow are never carried");
        check(blockInfo(Blocks::PackedIce).opaque && !blockInfo(Blocks::FrostedIce1).opaque, "packed ice is solid to see, frosted ice is clear");
        auto nine = [&](StackId what) {
            ItemStack grid[9];
            for (ItemStack& cell : grid) { cell.id = what; cell.count = 1; }
            return Crafting::match(grid, 3);
        };
        check(nine(Blocks::Ice).id == Blocks::PackedIce, "nine ice pack into packed ice");
        check(nine(Blocks::PackedIce).id == Blocks::BlueIce, "and nine packed ice into blue ice");

        using Enchanting::Kind;
        check(Enchanting::maxLevel(Kind::FrostWalker) == 2 && Enchanting::isTreasure(Kind::FrostWalker), "frost walker goes to two and is treasure");
        check(Enchanting::appliesTo(Kind::FrostWalker, Items::IronBoots) && !Enchanting::appliesTo(Kind::FrostWalker, Items::IronHelmet),
              "it goes on boots only");
        bool neverOffered = true;
        for (uint32_t roll = 0; roll < 200; ++roll)
            for (int row = 0; row < 3; ++row)
                if (Enchanting::offerFor(row, 15, Items::DiamondBoots, roll * 2654435761u).kind == Kind::FrostWalker) neverOffered = false;
        check(neverOffered, "the table never offers it");
        bool sold = false;
        for (const Trading::Deal& deal : Trading::dealsFor(Trading::Profession::Armorer))
            if (deal.offer.give == Items::IronBoots && Enchanting::has(deal.offer.giveEnchantments, Kind::FrostWalker)) sold = true;
        check(sold, "an armorer sells frost walker boots");
        const Enchanting::Set kept = Enchanting::withLevel(Enchanting::withLevel(0u, Kind::Power, 5), Kind::FrostWalker, 2);
        check(Enchanting::levelOf(kept, Kind::Power) == 5 && Enchanting::levelOf(kept, Kind::FrostWalker) == 2,
              "and it packs beside the old kinds without disturbing them");

        check(!blockInfo(Blocks::PowderSnow).collides && blockMaterial(Blocks::PowderSnow) == Material::Snow,
              "powder snow is walked into and dug with a shovel");
        check(maxStackOf(Items::PowderSnowBucket) == 1, "a powder snow bucket does not stack");
        check(Player::FREEZE_SECONDS > 0.0f, "freezing takes time");
    }

    void testWiring()
    {
        section("repeaters, comparators, hooks and the new rails");

        struct Grid : Redstone::Cells
        {
            std::map<std::array<int, 3>, BlockId> blocks;
            BlockId at(int x, int y, int z) const override
            {
                const auto it = blocks.find({ x, y, z });
                return it == blocks.end() ? Blocks::Air : it->second;
            }
            void set(int x, int y, int z, BlockId b) override { blocks[{ x, y, z }] = b; }
        };

        bool ids = true;
        for (int out = 0; out < 4; ++out)
        {
            for (int delay = 1; delay <= 4; ++delay)
                for (bool lit : { false, true })
                {
                    const BlockId r = repeaterOf(out, delay, lit);
                    if (!isRepeater(r) || repeaterOut(r) != out || repeaterDelay(r) != delay || repeaterLit(r) != lit || blockFacing(r) != out)
                        ids = false;
                    if (blockDrop(r) != repeaterOf(0, 1, false)) ids = false;
                }
            for (bool sub : { false, true })
                for (bool lit : { false, true })
                {
                    const BlockId c = comparatorOf(out, sub, lit);
                    if (!isComparator(c) || comparatorOut(c) != out || comparatorSubtracts(c) != sub || comparatorLit(c) != lit) ids = false;
                }
            for (bool att : { false, true })
                for (bool pow : { false, true })
                {
                    const BlockId h = hookOf(out, att, pow);
                    if (!isTripwireHook(h) || hookFacing(h) != out || hookAttached(h) != att || hookPowered(h) != pow) ids = false;
                    if (blockDrop(h) != Items::TripwireHook) ids = false;
                }
        }
        check(ids, "every repeater, comparator and hook knows its facing, setting and state, and breaks back to one item");

        Grid line;
        line.set(0, 0, 0, Blocks::Stone);
        line.set(1, 0, 0, Blocks::Stone);
        line.set(-1, 0, 0, Blocks::Stone);
        line.set(0, 0, 1, Blocks::Stone);
        line.set(0, 1, 0, repeaterOf(1, 1, true));
        line.set(1, 1, 0, Blocks::RedstoneDust);
        line.set(-1, 1, 0, Blocks::RedstoneDust);
        line.set(0, 1, 1, Blocks::RedstoneDust);
        Redstone::update(line, 0, 1, 0);
        check(line.at(1, 1, 0) == Blocks::RedstoneDustLit, "a lit repeater lights the dust it points at");
        check(line.at(-1, 1, 0) == Blocks::RedstoneDust && line.at(0, 1, 1) == Blocks::RedstoneDust,
              "and none behind it or beside it");

        Grid input;
        input.set(0, 0, 0, repeaterOf(1, 2, false));
        input.set(-1, 0, 0, Blocks::RedstoneBlock);
        check(Redstone::diodeInput(input, 0, 0, 0), "a repeater reads the block behind it");
        input.set(-1, 0, 0, Blocks::Air);
        input.set(0, 0, 1, Blocks::RedstoneBlock);
        check(!Redstone::diodeInput(input, 0, 0, 0), "and not one beside it");
        input.set(0, 0, 0, comparatorOf(1, true, false));
        check(Redstone::diodeSide(input, 0, 0, 0), "while a comparator feels its side");

        Grid wall;
        wall.set(0, 0, 0, Blocks::Stone);
        wall.set(1, 0, 0, hookOf(1, true, true));
        check(Redstone::carries(wall, 0, 0, 0), "a pulled hook powers the wall it hangs on");
        wall.set(1, 0, 0, hookOf(1, true, false));
        check(!Redstone::carries(wall, 0, 0, 0), "and a slack one does not");
        check(Redstone::isSource(Blocks::TargetLit) && Redstone::isSource(Blocks::DaylightDetectorLit) &&
              Redstone::isSource(Blocks::DaylightDetectorInvertedLit) && !Redstone::isSource(Blocks::DaylightDetector),
              "a struck target and a lit daylight detector give power");

        bool rails = true;
        for (int family = 1; family <= 3; ++family)
            for (bool ew : { false, true })
                for (bool lit : { false, true })
                {
                    const BlockId rail = specialRailOf(family, ew, lit);
                    if (!isRail(rail) || specialRailFamily(rail) != family || specialRailEastWest(rail) != ew || specialRailPowered(rail) != lit)
                        rails = false;
                    if (railLinks(rail, 1) != ew || railLinks(rail, 0) == ew) rails = false;
                    if (blockDrop(rail) != specialRailOf(family, false, false)) rails = false;
                }
        check(rails, "powered, detector and activator rails run straight either way");
        check(Redstone::isSink(specialRailOf(1, false, false)) && Redstone::lit(specialRailOf(1, true, false)) == specialRailOf(1, true, true),
              "a powered rail answers to power");
        check(Redstone::isSource(specialRailOf(2, false, true)), "and a detector rail with a cart on it gives power");

        check(isDoor(Blocks::IronDoorFirst) && doorBase(Blocks::IronDoorFirst + 9) == Blocks::IronDoorFirst &&
              Redstone::isSink(Blocks::IronDoorFirst + 3), "an iron door is a door that answers to power");
        check(isTrapdoor(Blocks::IronTrapdoorFirst) && trapdoorBase(Blocks::IronTrapdoorFirst + 5) == Blocks::IronTrapdoorFirst,
              "and so is an iron trapdoor");
        check(!Tools::canHarvest(Blocks::Air, Blocks::IronDoorFirst), "both want a pickaxe");

        const std::vector<StackId> made = Crafting::everyOutput();
        auto makes = [&](StackId id) { return std::find(made.begin(), made.end(), id) != made.end(); };
        check(makes(Blocks::RepeaterFirst) && makes(Blocks::ComparatorFirst) && makes(Blocks::DaylightDetector) &&
              makes(Blocks::Target) && makes(Blocks::PoweredRailFirst) && makes(Blocks::DetectorRailFirst) &&
              makes(Blocks::ActivatorRailFirst) && makes(Blocks::IronDoorFirst) && makes(Blocks::IronTrapdoorFirst),
              "every new redstone part has a recipe");
    }

    void testFireAndSoul()
    {
        section("fire, soul fire and the soul blocks");
        check(isFire(Blocks::Fire0) && isFire(Blocks::Fire3) && isFire(Blocks::SoulFire) && !isFire(Blocks::Campfire),
              "four ages of fire and soul fire");
        check(!isObtainable(Blocks::Fire2) && blockDrop(Blocks::Fire1) == Blocks::Air && !blockInfo(Blocks::Fire0).collides,
              "fire is walked into, never carried");
        check(lightEmission(Blocks::Fire0) == 15 && lightEmission(Blocks::SoulFire) == 10 && lightEmission(Blocks::SoulTorch) == 10,
              "soul light is dimmer");
        check(isFlammable(Blocks::Planks) && isFlammable(Blocks::Log) && isFlammable(Blocks::Wool) && isFlammable(Blocks::Leaves) &&
              !isFlammable(Blocks::Stone) && !isFlammable(Blocks::Glass), "wood, wool and leaves burn and stone does not");
        check(isCampfire(Blocks::SoulCampfire) && blockDrop(Blocks::SoulCampfire) == Blocks::SoulSoil, "a soul campfire breaks into soul soil");
        const std::vector<StackId> made = Crafting::everyOutput();
        auto makes = [&](StackId id) { return std::find(made.begin(), made.end(), id) != made.end(); };
        check(makes(Blocks::SoulTorch) && makes(Blocks::SoulCampfire) && makes(Blocks::IronBars) && makes(Blocks::ChainFirst) &&
              makes(Blocks::EndRodFirst) && makes(Items::IronNugget), "the soul lights, bars, chain, rod and nuggets are made");
        check(Smelting::resultOf(Items::ChorusFruit) == Items::PoppedChorusFruit, "chorus fruit pops in a furnace");
        check(blockDrop(Blocks::Cobweb) == Items::StringItem && !blockInfo(Blocks::Cobweb).collides, "a cobweb is walked into and cut for string");
        check(Tools::breakSeconds(Items::IronSword, Blocks::Cobweb) < Tools::breakSeconds(Blocks::Air, Blocks::Cobweb) / 4.0f,
              "and a sword goes through it quickly");
        bool anchors = true;
        const int LIGHT[5] = { 0, 3, 7, 11, 15 };
        for (int c = 0; c < 5; ++c)
            if (anchorCharge(anchorOf(c)) != c || lightEmission(anchorOf(c)) != LIGHT[c] || blockDrop(anchorOf(c)) != Blocks::RespawnAnchor0)
                anchors = false;
        check(anchors, "a respawn anchor holds four charges and glows brighter with each");
        check(!Tools::canHarvest(Items::IronPickaxe, Blocks::CryingObsidian) && Tools::canHarvest(Items::DiamondPickaxe, Blocks::CryingObsidian),
              "crying obsidian wants a diamond pickaxe");
        check(lightEmission(Blocks::MagmaBlock) == 3 && lightEmission(Blocks::CryingObsidian) == 10, "magma glows dimly and crying obsidian brighter");
        check(makes(Blocks::MagmaBlock) && makes(Blocks::RespawnAnchor0) && makes(Blocks::Lodestone), "magma, the anchor and the lodestone are made");
        LevelState saved;
        saved.hasAnchor = true;
        saved.anchorX = -12; saved.anchorY = 40; saved.anchorZ = 99;
        const std::string dir = (std::filesystem::temp_directory_path() / "bc_selftest_anchor").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        WorldSave::ensureDirectories(dir);
        WorldSave::saveLevel(dir, saved);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.hasAnchor && back.anchorX == -12 && back.anchorY == 40 && back.anchorZ == 99,
              "the anchor you set survives a save");
        LevelState potted;
        LevelState::SavedPot pot;
        pot.x = 5; pot.y = 64; pot.z = -7; pot.dimension = 1;
        const int sides[4] = { -1, 4, 9, 22 };
        pot.sides = Archaeology::packPot(sides);
        potted.pots.push_back(pot);
        WorldSave::saveLevel(dir, potted);
        LevelState potBack;
        check(WorldSave::loadLevel(dir, potBack) && potBack.pots.size() == 1 && potBack.pots[0].sides == pot.sides &&
              potBack.pots[0].dimension == 1 && Archaeology::potSide(potBack.pots[0].sides, 0) == -1 &&
              Archaeology::potSide(potBack.pots[0].sides, 3) == 22, "a decorated pot keeps its four sides through a save");
        ItemStack ring[9];
        ring[1].id = Items::Brick; ring[3].id = Items::SkullSherd; ring[5].id = Items::Brick; ring[7].id = Items::ArcherSherd;
        for (int i : { 1, 3, 5, 7 }) ring[i].count = 1;
        const CraftOutput potMade = Crafting::match(ring, 3);
        check(potMade.valid() && potMade.id == Blocks::DecoratedPotFirst + 2 && Archaeology::potSide(potMade.enchantments, 1) == 21 &&
              Archaeology::potSide(potMade.enchantments, 3) == 1 && Archaeology::potSide(potMade.enchantments, 0) == -1,
              "four bricks or sherds in a ring make a pot carrying them");
        check(maxStackOf(static_cast<StackId>(Blocks::DecoratedPotFirst + 2)) == 1, "and pots do not stack");
        std::filesystem::remove_all(dir, ec);
    }

    void testArchaeology()
    {
        section("brushes, suspicious sand and sherds");
        bool dust = true;
        for (int table = 0; table < Archaeology::TableCount; ++table)
            for (int stage = 0; stage < 4; ++stage)
            {
                const BlockId block = suspiciousOf(table, stage);
                if (!isSuspicious(block) || dustTable(block) != table || dustStage(block) != stage || isObtainable(block)) dust = false;
                if (blockDrop(block) != Blocks::Air) dust = false;
                if (Archaeology::settled(block) != (table < 3 ? Blocks::Sand : Blocks::Gravel)) dust = false;
            }
        check(dust, "suspicious sand and gravel keep their loot table and how far they have been dusted");
        bool sherds = true;
        for (int k = 0; k < Archaeology::SHERD_COUNT; ++k)
            if (Archaeology::sherdIndex(Archaeology::sherd(k)) != k || !isItem(Archaeology::sherd(k))) sherds = false;
        check(sherds, "twenty-three sherds, each its own item");
        bool finds = true;
        bool sawSherd[Archaeology::TableCount] = {};
        for (int table = 0; table < Archaeology::TableCount; ++table)
            for (int i = 0; i < 200; ++i)
            {
                const Archaeology::Find find = Archaeology::roll(table, i * 7, 40, -i * 3, 99u);
                if (find.id == Blocks::Air || find.count != 1) finds = false;
                if (Archaeology::sherdIndex(find.id) >= 0) sawSherd[table] = true;
            }
        check(finds, "every brushing finds something");
        check(sawSherd[Archaeology::DesertPyramid] && sawSherd[Archaeology::DesertWell] && sawSherd[Archaeology::TrailRare] &&
              !sawSherd[Archaeology::TrailCommon], "sherds come from the pyramid, the well and the rare ruins, not the common ones");
        const std::vector<StackId> made = Crafting::everyOutput();
        check(std::find(made.begin(), made.end(), static_cast<StackId>(Items::Brush)) != made.end() && maxStackOf(Items::Brush) == 1,
              "a brush is made from a feather, copper and a stick, and does not stack");

        const WorldGen generator(2024u);
        int ruinX = 0, ruinZ = 0;
        bool found = false;
        for (int cz = -6; cz <= 6 && !found; ++cz)
            for (int cx = -6; cx <= 6 && !found; ++cx)
                found = generator.trailRuinIn(cx, cz, ruinX, ruinZ);
        check(found, "there is a trail ruin within a few hundred blocks");
        if (found)
        {
            int gravel = 0;
            for (int dz = -1; dz <= 1; ++dz)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    auto chunkOf = [](int v, int size) { return v >= 0 ? v / size : -((-v + size - 1) / size); };
                    Chunk chunk(ChunkPos{ chunkOf(ruinX, Chunk::SX) + dx, chunkOf(ruinZ, Chunk::SZ) + dz });
                    generator.generate(chunk);
                    for (int y = 0; y < Chunk::SY; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                                if (isSuspiciousGravel(chunk.getBlock(lx, y, lz))) ++gravel;
                }
            check(gravel > 0, "and it is full of suspicious gravel");
        }
    }

    void testFoodsThatBite()
    {
        section("golden food, stews and the foods that bite back");
        check(Food::isEdible(Items::GoldenApple) && Food::alwaysEdible(Items::GoldenApple) && Food::alwaysEdible(Items::EnchantedGoldenApple) &&
              !Food::alwaysEdible(Items::Bread), "a golden apple can be eaten on a full stomach and bread cannot");
        check(Food::valueOf(Items::RabbitStew).hunger == 10 && Food::valueOf(Items::GoldenCarrot).hunger == 6 &&
              Food::valueOf(Items::PumpkinPie).hunger == 8, "stews and pies fill as much as Minecraft's");
        check(Food::leavesBowl(Items::MushroomStew) && Food::leavesBowl(Items::BeetrootSoup) && !Food::leavesBowl(Items::Bread),
              "a stew gives its bowl back");
        check(Food::stewEffect(Food::stewFlower(Blocks::AzureBluet)).kind == Effects::Kind::Blindness &&
              Food::stewEffect(Food::stewFlower(Blocks::OxeyeDaisy)).kind == Effects::Kind::Regeneration,
              "a suspicious stew does what its flower does");

        ItemStack stew[9];
        stew[0].id = Blocks::BrownMushroom; stew[1].id = Blocks::RedMushroom; stew[2].id = Items::Bowl; stew[3].id = Blocks::Cornflower;
        for (ItemStack& cell : stew) if (cell.id != Blocks::Air) cell.count = 1;
        const CraftOutput sus = Crafting::match(stew, 3);
        check(sus.valid() && sus.id == Items::SuspiciousStew && sus.damage == Food::stewFlower(Blocks::Cornflower),
              "both mushrooms, a bowl and a flower make a suspicious stew that remembers the flower");
        stew[3].clear();
        check(Crafting::match(stew, 3).id == Items::MushroomStew, "and without the flower, mushroom stew");
        ItemStack apple[9];
        for (int i = 0; i < 9; ++i) { apple[i].id = i == 4 ? Items::Apple : Items::GoldIngot; apple[i].count = 1; }
        check(Crafting::match(apple, 3).id == Items::GoldenApple, "eight gold ingots round an apple make a golden apple");

        Player eater(glm::vec3(0.0f));
        eater.health = 20;
        Effects::give(eater.effects, Effects::Kind::Absorption, 0, 120.0f);
        eater.absorption = 4;
        eater.damage(3);
        check(eater.health == 20 && eater.absorption == 1, "absorption hearts take a blow before your own do");
        Player starving(glm::vec3(0.0f));
        starving.mode = GameMode::Survival;
        starving.saturation = 0.0f;
        Effects::give(starving.effects, Effects::Kind::Hunger, 0, 60.0f);
        const int before = starving.hunger;
        for (int i = 0; i < 600; ++i) { starving.updateEffects(0.1f); starving.updateVitals(0.0f); }
        check(starving.hunger < before, "hunger makes you hungry");
        check(mobType(MobId::Zombie).drop == Items::RottenFlesh && mobType(MobId::Spider).secondDrop == Items::SpiderEye,
              "zombies drop rotten flesh and spiders their eyes");
        bool poisonous = false;
        for (uint32_t roll = 0; roll < 400 && !poisonous; ++roll)
            poisonous = Farming::harvestOf(static_cast<BlockId>(Blocks::Potato0 + 7), roll).second == Items::PoisonousPotato;
        check(poisonous, "now and then a potato comes up poisonous");
    }

    void testMaterials()
    {
        section("drops and materials");
        auto grid = [](std::initializer_list<StackId> cells, ItemStack* out) {
            int i = 0;
            for (StackId id : cells) { out[i].clear(); if (id != Blocks::Air) { out[i].id = id; out[i].count = 1; } ++i; }
            for (; i < 9; ++i) out[i].clear();
        };
        const StackId S = Items::PrismarineShard, C = Items::PrismarineCrystals, _ = Blocks::Air;
        ItemStack g[9];
        grid({ S, S, _, S, S, _, _, _, _ }, g);
        check(Crafting::match(g, 3).id == Blocks::Prismarine, "four shards make prismarine");
        grid({ S, S, S, S, S, S, S, S, S }, g);
        check(Crafting::match(g, 3).id == Blocks::PrismarineBricks, "nine make prismarine bricks");
        grid({ S, S, S, S, Items::DyeBlack, S, S, S, S }, g);
        check(Crafting::match(g, 3).id == Blocks::DarkPrismarine, "eight round a black dye make dark prismarine");
        grid({ S, C, S, C, C, C, S, C, S }, g);
        check(Crafting::match(g, 3).id == Blocks::SeaLantern, "shards and crystals make a sea lantern");
        grid({ Items::RabbitHide, Items::RabbitHide, _, Items::RabbitHide, Items::RabbitHide, _, _, _, _ }, g);
        check(Crafting::match(g, 3).id == Items::Leather, "four rabbit hides make leather");
        grid({ Items::Charcoal, _, _, Items::Stick, _, _, _, _, _ }, g);
        check(Crafting::match(g, 3).id == Blocks::Torch, "charcoal lights a torch like coal");
        grid({ Items::Charcoal, Items::Charcoal, Items::Charcoal, Items::Charcoal, Items::Charcoal,
               Items::Charcoal, Items::Charcoal, Items::Charcoal, Items::Charcoal }, g);
        check(Crafting::match(g, 3).id != Blocks::CoalBlock, "but nine of it are not a block of coal");
        check(Smelting::resultOf(Blocks::SpruceLog) == Items::Charcoal && Smelting::resultOf(Blocks::StrippedOakLog) == Items::Charcoal,
              "a log fired is charcoal");
        check(Smelting::resultOf(Blocks::CrimsonStem) == Blocks::Air, "but a nether stem does not burn down to it");
        check(Smelting::burnSeconds(Items::Charcoal) == Smelting::burnSeconds(Items::Coal), "and charcoal burns as long as coal");
        check(blockDrop(Blocks::SeaLantern) == Items::PrismarineCrystals, "a sea lantern breaks into crystals");
        check(mobType(MobId::Rabbit).secondDrop == Items::RabbitHide, "a rabbit leaves its hide");
        check(mobType(MobId::Phantom).drop == Items::PhantomMembrane, "a phantom its membrane");
        check(mobType(MobId::Guardian).drop == Items::PrismarineShard && mobType(MobId::ElderGuardian).secondDrop == Blocks::WetSponge,
              "guardians shards, and the elder one a wet sponge");

        Anvil::Input wings{ Items::Elytra, 1, 400, 0 };
        Anvil::Input membrane{ Items::PhantomMembrane, 2, 0, 0 };
        const Anvil::Result mended = Anvil::combine(wings, membrane);
        check(mended.valid && mended.damage < 400, "phantom membrane mends an elytra at the anvil");
        check(Brewing::brew(Brewing::Potion::Awkward, Items::RabbitFoot) == Brewing::Potion::Leaping,
              "a rabbit's foot brews leaping");
        Effects::Kind kind = Effects::Kind::Count;
        int amplifier = 0;
        float seconds = 0.0f;
        check(Brewing::effectOf(Brewing::Potion::Leaping, kind, amplifier, seconds) && kind == Effects::Kind::JumpBoost,
              "which is jump boost");

        bool bottles = false, hides = false;
        for (const Trading::Deal& d : Trading::dealsFor(Trading::Profession::Cleric))
            if (d.offer.give == Items::ExperienceBottle && d.rank == 4) bottles = true;
        for (const Trading::Deal& d : Trading::dealsFor(Trading::Profession::Leatherworker))
            if (d.offer.want == Items::RabbitHide) hides = true;
        check(bottles, "a master cleric sells bottles o' enchanting");
        check(hides, "and a leatherworker buys rabbit hide");

        bool fragment = false;
        for (int n = 0; n < 60 && !fragment; ++n)
            for (const StructureLoot::Stack& s : StructureLoot::rollFor(WorldGen::Hoard::AncientCity, n * 7, -40, n * 3, 99))
                if (s.id == Items::DiscFragment) fragment = true;
        check(fragment, "disc fragments turn up in ancient cities");

        Arrow bottle;
        bottle.kind = Arrows::Kind::XpBottle;
        check(!bottle.flies() && bottle.bursts() && !bottle.shaft(), "a thrown bottle o' enchanting arcs and breaks");
    }

    void testMobBuckets()
    {
        section("buckets of fish, axolotls and tadpoles");
        const MobId CAUGHT[] = { MobId::Cod, MobId::Salmon, MobId::Pufferfish, MobId::Axolotl, MobId::TropicalFish, MobId::Tadpole };
        bool roundTrip = true;
        for (MobId id : CAUGHT)
            if (bucketFor(id) == Blocks::Air || mobInBucket(bucketFor(id)) != id) roundTrip = false;
        check(roundTrip, "every bucketable creature has its own bucket, and the bucket remembers it");
        check(bucketFor(MobId::TropicalFishLarge) == Items::TropicalFishBucket, "both shapes of tropical fish share one bucket");
        check(bucketFor(MobId::Cow) == Blocks::Air && mobInBucket(Items::WaterBucket) == MobId::Count, "a cow does not go in a bucket");
        check(maxStackOf(Items::AxolotlBucket) == 1, "a bucket of anything stacks to one");
        bool keepsLook = true;
        for (uint32_t seed : { 7u, 99991u, 0xDEADBEEFu, 0x80000001u })
        {
            if (axolotlVariantOf(bucketSeed(MobId::Axolotl, seed)) != axolotlVariantOf(seed)) keepsLook = false;
            const TropicalFish::Look before = TropicalFish::lookFor(seed, true);
            const TropicalFish::Look after = TropicalFish::lookFor(bucketSeed(MobId::TropicalFish, seed), true);
            if (before.variety != after.variety || before.base != after.base || before.pattern != after.pattern) keepsLook = false;
        }
        check(keepsLook, "what comes out of a bucket looks like what went in, even after the bucket is saved");

        bool shapesHold = true;
        int common = 0;
        for (uint32_t seed = 1; seed <= 400; ++seed)
        {
            const TropicalFish::Look small = TropicalFish::lookFor(seed, false);
            const TropicalFish::Look large = TropicalFish::lookFor(seed, true);
            if (TropicalFish::large(small.variety) || !TropicalFish::large(large.variety)) shapesHold = false;
            if (small.base < 0 || small.base > 15 || small.pattern < 0 || small.pattern > 15) shapesHold = false;
            if (*TropicalFish::commonName(small)) ++common;
        }
        check(shapesHold, "a small tropical fish is always one of the six small kinds, and a large one a large");
        check(common > 300 && common < 400, "and nine in ten are one of Minecraft's twenty-two named fish");
        TropicalFish::Look clown;
        clown.variety = 0; clown.base = 1; clown.pattern = 0;
        check(std::string(TropicalFish::commonName(clown)) == "Clownfish", "an orange and white kob is a clownfish");
        check(std::string(TropicalFish::varietyName(11)) == "Clayfish", "and the twelfth shape is the clayfish");

        bool kinds = true;
        for (int kind = 0; kind < 3; ++kind)
            if (frogKindOf(frogSeedOfKind(kind, 12345u + static_cast<uint32_t>(kind))) != kind) kinds = false;
        check(kinds, "a tadpole can grow into whichever frog its climate asks for");

        Mob tadpole(MobId::Tadpole, glm::vec3(0.0f), 7u);
        check(!tadpole.feed(Items::Wheat) && tadpole.feed(Items::Slimeball) && tadpole.growthBoost() > 0.0f,
              "a slimeball, and nothing else, hurries a tadpole along");
        check(mobType(MobId::Frog).breedingFood == Items::Slimeball, "and is what brings two frogs together");
        check(blockDrop(Blocks::Frogspawn) == Blocks::Air && !isObtainable(Blocks::Frogspawn), "frogspawn cannot be picked up");
    }

    void testBooks()
    {
        section("books: enchanted, written and copied");
        ItemStack g[9];
        auto put = [&](std::initializer_list<ItemStack> cells) {
            int i = 0;
            for (const ItemStack& c : cells) g[i++] = c;
            for (; i < 9; ++i) g[i].clear();
        };
        put({ ItemStack{ Items::Book, 1 }, ItemStack{ Items::InkSac, 1 }, ItemStack{ Items::Feather, 1 } });
        check(Crafting::match(g, 3).id == Items::WritableBook, "a book, an ink sac and a feather make a book and quill");

        ItemStack signed_{ Items::WrittenBook, 1 };
        signed_.damage = 3;
        put({ signed_, ItemStack{ Items::WritableBook, 1 }, ItemStack{ Items::WritableBook, 1 } });
        const CraftOutput copies = Crafting::match(g, 3);
        check(copies.id == Items::WrittenBook && copies.count == 2 && copies.damage == 3 && ((copies.enchantments >> 24) & 3u) == 1u,
              "a written book beside two books and quill makes two copies of the original");
        Crafting::consume(g, 3);
        check(g[0].id == Items::WrittenBook && g[1].empty() && g[2].empty(), "and the original stays in the grid");
        ItemStack copyOfCopy = signed_;
        copyOfCopy.enchantments = 2u << 24;
        put({ copyOfCopy, ItemStack{ Items::WritableBook, 1 } });
        check(!Crafting::match(g, 3).valid(), "a copy of a copy cannot be copied again");
        check(maxStackOf(Items::WrittenBook) == 16 && maxStackOf(Items::WritableBook) == 1 && maxStackOf(Items::EnchantedBook) == 1,
              "written books stack to sixteen, the others to one");
        check(Enchanting::glints(Items::WrittenBook, 0) && Enchanting::glints(Items::EnchantedBook, 0), "written and enchanted books shine");

        check(Enchanting::offerFor(0, 15, Items::Book, 77u).valid(), "a plain book can go on the enchanting table");
        Anvil::Input book{ Items::EnchantedBook, 1, 0, Enchanting::withLevel(0, Enchanting::Kind::Sharpness, 3) };
        Anvil::Input sword{ Items::DiamondSword, 1, 0, 0 };
        const Anvil::Result sharpened = Anvil::combine(sword, book);
        check(sharpened.valid && Enchanting::levelOf(sharpened.enchantments, Enchanting::Kind::Sharpness) == 3,
              "an enchanted book gives up its enchantment at the anvil");
        Anvil::Input one{ Items::EnchantedBook, 1, 0, Enchanting::withLevel(0, Enchanting::Kind::Efficiency, 1) };
        const Anvil::Result merged = Anvil::combine(one, one);
        check(merged.valid && merged.id == Items::EnchantedBook && Enchanting::levelOf(merged.enchantments, Enchanting::Kind::Efficiency) == 2,
              "and two books of the same make one of the next level");
        const Grindstone::Result wiped = Grindstone::combine(book, Anvil::Input{});
        check(wiped.valid && wiped.id == Items::Book, "a grindstone turns an enchanted book back into a book");

        bool fishedBook = false;
        for (uint32_t seed = 0; seed < 6000 && !fishedBook; ++seed)
        {
            const Fishing::Catch c = Fishing::roll(seed, true, false);
            if (c.stack.id == Items::EnchantedBook && Enchanting::any(c.stack.enchantments)) fishedBook = true;
        }
        check(fishedBook, "a rod can bring up an enchanted book");
        bool sold = false;
        for (const Trading::Deal& d : Trading::dealsFor(Trading::Profession::Librarian))
            if (d.offer.give == Items::EnchantedBook && d.offer.giveEnchantments != 0) sold = true;
        check(sold, "and a librarian sells them");

        const std::string dir = (std::filesystem::temp_directory_path() / "bc_selftest_books").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        WorldSave::ensureDirectories(dir);
        LevelState state;
        state.seed = 5u;
        state.books.push_back(LevelState::SavedBook{ "Notes", "Alex", { "first page", std::string(600, 'x') } });
        LevelState::SavedLectern lectern;
        lectern.x = 4; lectern.y = 70; lectern.z = -9;
        lectern.stack.id = static_cast<uint16_t>(Items::WrittenBook);
        lectern.stack.count = 1;
        lectern.stack.damage = 1;
        state.lecterns.push_back(lectern);
        WorldSave::saveLevel(dir, state);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.books.size() == 1 && back.books[0].title == "Notes" &&
                  back.books[0].author == "Alex" && back.books[0].pages.size() == 2 && back.books[0].pages[1].size() == 600,
              "a written book's title, author and long pages are kept in the level");
        check(back.lecterns.size() == 1 && back.lecterns[0].stack.id == static_cast<uint16_t>(Items::WrittenBook) && back.lecterns[0].z == -9,
              "and so is the book left open on a lectern");
        std::filesystem::remove_all(dir, ec);
    }

    void testVehicles()
    {
        section("vehicles: boats of every wood, chest boats and the loaded minecarts");
        ItemStack g[9];
        for (ItemStack& cell : g) cell.clear();
        const int cells[5] = { 0, 2, 3, 4, 5 };
        bool everyWood = true;
        for (int wood = 0; wood < BOAT_WOODS; ++wood)
        {
            for (int c : cells) g[c] = ItemStack{ planksOfWood(wood), 1 };
            if (Crafting::match(g, 3).id != boatOfWood(wood, false)) everyWood = false;
        }
        check(everyWood, "planks of each wood make that wood's boat, and bamboo a raft");
        for (int c : cells) g[c] = ItemStack{ Blocks::CrimsonPlanks, 1 };
        check(!Crafting::match(g, 3).valid(), "but nether planks make no boat");
        for (ItemStack& cell : g) cell.clear();
        g[0] = ItemStack{ Items::CherryBoat, 1 };
        g[1] = ItemStack{ Blocks::Chest, 1 };
        check(Crafting::match(g, 3).id == Items::CherryChestBoat, "a boat and a chest make a boat with a chest");
        g[0] = ItemStack{ Items::Minecart, 1 };
        check(Crafting::match(g, 3).id == Items::ChestMinecart, "and a minecart and a chest a minecart with chest");
        g[1] = ItemStack{ Blocks::Hopper, 1 };
        check(Crafting::match(g, 3).id == Items::HopperMinecart, "a hopper makes a hopper minecart");
        g[1] = ItemStack{ Blocks::Tnt, 1 };
        check(Crafting::match(g, 3).id == Items::TntMinecart, "TNT a minecart with TNT");
        g[1] = ItemStack{ Blocks::Furnace, 1 };
        check(Crafting::match(g, 3).id == Items::FurnaceMinecart, "and a furnace a furnace minecart");

        bool roundTrip = true;
        for (int wood = 0; wood < BOAT_WOODS; ++wood)
            for (bool chest : { false, true })
            {
                const StackId item = boatOfWood(wood, chest);
                if (boatItemWood(item) != wood || isChestBoatItem(item) != chest || maxStackOf(item) != 1) roundTrip = false;
                const MobId kind = wood == 8 ? (chest ? MobId::ChestRaft : MobId::Raft) : (chest ? MobId::ChestBoat : MobId::Boat);
                if (vehicleItemFor(kind, wood) != item) roundTrip = false;
            }
        check(roundTrip, "a broken boat gives back the boat it was, wood and chest and all");
        bool carts = true;
        for (int kind = 0; kind <= 5; ++kind)
            if (minecartItemKind(vehicleItemFor(MobId::Minecart, kind)) != kind) carts = false;
        check(carts, "and a broken minecart the minecart it was");
        check(hasVehicleChest(MobId::ChestBoat, 3) && hasVehicleChest(MobId::Minecart, 1) && hasVehicleChest(MobId::Minecart, 3) &&
                  !hasVehicleChest(MobId::Boat, 0) && !hasVehicleChest(MobId::Minecart, 2),
              "chest boats, chest minecarts and hopper minecarts hold things");

        const std::string dir = (std::filesystem::temp_directory_path() / "bc_selftest_vehicles").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        WorldSave::ensureDirectories(dir);
        LevelState state;
        state.seed = 6u;
        LevelState::SavedVehicle boat;
        boat.dimension = 0;
        boat.type = static_cast<uint16_t>(MobId::ChestBoat);
        boat.variant = 6;
        boat.x = 10.5f; boat.y = 63.0f; boat.z = -4.5f; boat.yaw = 90.0f;
        LevelState::SavedBundleStack diamonds;
        diamonds.id = static_cast<uint16_t>(Items::Diamond);
        diamonds.count = 12;
        boat.contents.emplace_back(static_cast<uint8_t>(5), diamonds);
        state.vehicles.push_back(boat);
        LevelState::SavedVehicle cart;
        cart.dimension = 1;
        cart.type = static_cast<uint16_t>(MobId::Minecart);
        cart.variant = 5;
        cart.command = "say hello";
        state.vehicles.push_back(cart);
        WorldSave::saveLevel(dir, state);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.vehicles.size() == 2 && back.vehicles[0].variant == 6 &&
                  back.vehicles[0].contents.size() == 1 && back.vehicles[0].contents[0].first == 5 &&
                  back.vehicles[0].contents[0].second.count == 12,
              "a boat left in the world is kept with its wood and what is in its chest");
        check(back.vehicles.size() == 2 && back.vehicles[1].dimension == 1 && back.vehicles[1].command == "say hello",
              "and a cart in the Nether with its command");
        std::filesystem::remove_all(dir, ec);
    }

    void testSpawnEggs()
    {
        section("spawn eggs");
        int eggs = 0;
        bool apart = true;
        std::set<std::pair<unsigned int, unsigned int>> looks;
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            unsigned int shell = 0, spots = 0;
            const MobId id = static_cast<MobId>(i);
            const bool has = spawnEggColours(id, shell, spots);
            const MobType& type = mobType(id);
            if (type.inanimate || id == MobId::SlimeSmall || id == MobId::TropicalFishLarge || id == MobId::Illusioner || id == MobId::Giant)
            {
                if (has) apart = false;
                continue;
            }
            if (!has) apart = false;
            else
            {
                ++eggs;
                looks.insert({ shell, spots });
            }
        }
        check(apart, "every living mob has an egg, and no boat, cart, half a slime or illusioner does");
        check(eggs >= 60, "which is more than sixty eggs");
        unsigned int shell = 0, spots = 0;
        check(spawnEggColours(MobId::Pig, shell, spots) && shell == 0xF0A5A2u && spots == 0xDB635Fu, "a pig's egg is pink with darker spots");
        check(spawnEggColours(MobId::Creeper, shell, spots) && shell == 0x0DA70Bu && spots == 0u, "and a creeper's green with black");
        check(maxStackOf(Items::SpawnEgg) == 64, "eggs stack to sixty-four");
        check(Tiles::ItemSpawnEggFirst + mobTypeCount() <= Tiles::ItemSpawnEggLast + 1, "and every mob has a tile to be drawn with");
    }

    void testFireworkCrafting()
    {
        section("fireworks: stars, fades and rockets");
        Fireworks::library().clear();
        ItemStack g[9];
        auto put = [&](std::initializer_list<StackId> cells) {
            int i = 0;
            for (StackId id : cells) { g[i].clear(); if (id != Blocks::Air) { g[i].id = id; g[i].count = 1; } ++i; }
            for (; i < 9; ++i) g[i].clear();
        };
        put({ Items::Gunpowder, Items::DyeRed });
        const CraftOutput plain = Crafting::match(g, 3);
        check(plain.id == Items::FireworkStar && plain.count == 1, "gunpowder and a dye make a firework star");
        const Fireworks::Explosion* red = Fireworks::library().explosionAt(plain.damage);
        check(red && red->shape == Fireworks::Shape::SmallBall && red->colours.size() == 1 && red->colours[0] == 14,
              "a small red ball");
        check(Crafting::match(g, 3).damage == plain.damage, "and making it again does not fill the level with copies");
        put({ Items::Gunpowder, Items::DyeBlue, Items::DyeYellow, Items::GoldNugget, Items::Diamond, Items::GlowstoneDust });
        const CraftOutput fancy = Crafting::match(g, 3);
        const Fireworks::Explosion* star = Fireworks::library().explosionAt(fancy.damage);
        check(star && star->shape == Fireworks::Shape::Star && star->colours.size() == 2 && star->trail && star->twinkle,
              "a gold nugget shapes a star, a diamond adds a trail and glowstone a twinkle");
        put({ Items::Gunpowder, Items::DyeBlue, Items::FireCharge });
        check(Fireworks::library().explosionAt(Crafting::match(g, 3).damage)->shape == Fireworks::Shape::LargeBall,
              "a fire charge makes it a large ball");
        put({ Items::Gunpowder, Items::DyeBlue, Items::Feather });
        check(Fireworks::library().explosionAt(Crafting::match(g, 3).damage)->shape == Fireworks::Shape::Burst, "and a feather a burst");
        put({ Items::Gunpowder, Items::DyeBlue, Items::Feather, Items::GoldNugget });
        check(!Crafting::match(g, 3).valid(), "but not two shapes at once");
        put({ Items::Gunpowder });
        check(!Crafting::match(g, 3).valid(), "and not with no colour");

        ItemStack starStack{ Items::FireworkStar, 1, fancy.damage };
        g[0] = starStack;
        g[1] = ItemStack{ Items::DyeWhite, 1 };
        for (int i = 2; i < 9; ++i) g[i].clear();
        const CraftOutput faded = Crafting::match(g, 3);
        const Fireworks::Explosion* fading = Fireworks::library().explosionAt(faded.damage);
        check(faded.id == Items::FireworkStar && fading && fading->fades.size() == 1 && fading->fades[0] == 0 && fading->trail,
              "a star and a dye give it a colour to fade to, keeping the rest");

        g[0] = ItemStack{ Items::Paper, 1 };
        g[1] = ItemStack{ Items::Gunpowder, 1 };
        g[2] = ItemStack{ Items::Gunpowder, 1 };
        g[3] = starStack;
        for (int i = 4; i < 9; ++i) g[i].clear();
        const CraftOutput rocket = Crafting::match(g, 3);
        const Fireworks::Rocket* design = Fireworks::library().rocketAt(rocket.damage);
        check(rocket.id == Items::FireworkRocket && rocket.count == 3 && design && design->flight == 2 &&
                  design->explosions.size() == 1,
              "paper, two gunpowder and a star make three rockets of flight two");
        g[3].clear();
        g[4] = ItemStack{ Items::Gunpowder, 1 };
        g[5] = ItemStack{ Items::Gunpowder, 1 };
        check(!Crafting::match(g, 3).valid(), "four gunpowder is too much for one rocket");
        put({ Items::BlazePowder, Items::Gunpowder, Items::Charcoal });
        check(Crafting::match(g, 3).id == Items::FireCharge && Crafting::match(g, 3).count == 3,
              "blaze powder, gunpowder and charcoal make three fire charges");
        check(Fireworks::lifetimeSeconds(1, 0u) >= 1.0f && Fireworks::lifetimeSeconds(3, 0xFFFFFFFFu) > Fireworks::lifetimeSeconds(1, 0u),
              "a longer flight stays up longer");
        check(Fireworks::starColour(starStack) != 0x8A8A8Au, "and a coloured star shows its colour");
        Fireworks::library().clear();
    }

    void testMusicDiscs()
    {
        section("music discs and the jukebox");
        bool everyDisc = true;
        for (int d = 0; d < Discs::COUNT; ++d)
        {
            const StackId item = Discs::itemOf(d);
            if (Discs::discOf(item) != d || maxStackOf(item) != 1 || Discs::song(d).seconds < 60.0f) everyDisc = false;
            if (Discs::song(d).comparator < 1 || Discs::song(d).comparator > 15) everyDisc = false;
        }
        check(everyDisc, "nineteen discs, each with its song, its length and its comparator strength");
        check(std::string(Discs::song(2).key) == "blocks" && Discs::song(2).seconds == 345.0f, "blocks is the longest at 345 seconds");
        check(Discs::creeperDrops(0) && Discs::creeperDrops(11) && !Discs::creeperDrops(12) && !Discs::creeperDrops(14),
              "a creeper drops only the twelve old discs");
        ItemStack g[9];
        for (ItemStack& cell : g) cell = ItemStack{ Items::DiscFragment, 1 };
        check(Crafting::match(g, 3).id == Discs::itemOf(13), "nine disc fragments make disc 5");
        bool relic = false;
        for (int n = 0; n < 400 && !relic; ++n)
            relic = Archaeology::roll(Archaeology::TrailRare, n, 30, n * 3, 5u).id == Discs::itemOf(15);
        check(relic, "and relic is brushed out of trail ruins");
        bool pigstep = false;
        for (int n = 0; n < 80 && !pigstep; ++n)
            for (const StructureLoot::Stack& s : StructureLoot::rollFor(WorldGen::Hoard::Bastion, n * 5, 40, n * 9, 77))
                if (s.id == Discs::itemOf(14)) pigstep = true;
        check(pigstep, "pigstep is in a bastion's chests");
    }

    void testPotionForms()
    {
        section("potions: every brew, long and strong, splash and lingering");
        using Brewing::Brew;
        using Brewing::Potion;
        Brew out;
        check(Brewing::brewWith(Brew{ Potion::Awkward }, Items::GoldenCarrot, out) && out.potion == Potion::NightVision,
              "a golden carrot brews night vision, as in Minecraft");
        check(Brewing::brewWith(Brew{ Potion::Speed }, Items::GlowstoneDust, out) && out.strong && out.potion == Potion::Speed,
              "glowstone makes a potion stronger");
        check(Brewing::brewWith(Brew{ Potion::Speed }, Items::Redstone, out) && out.extended, "and redstone makes it last");
        check(!Brewing::brewWith(Brew{ Potion::FireResistance }, Items::GlowstoneDust, out), "but fire resistance has no stronger kind");
        check(Brewing::brewWith(Brew{ Potion::Speed, true, false }, Items::FermentedSpiderEye, out) &&
                  out.potion == Potion::Slowness && out.extended,
              "a fermented eye turns long swiftness into long slowness");
        check(Brewing::brewWith(Brew{ Potion::Water }, Items::FermentedSpiderEye, out) && out.potion == Potion::Weakness,
              "and water into weakness");
        check(Brewing::brewWith(Brew{ Potion::Awkward }, Items::SpiderEye, out) && out.potion == Potion::Poison, "a spider eye brews poison");
        check(Brewing::brewWith(Brew{ Potion::Awkward }, Items::Pufferfish, out) && out.potion == Potion::WaterBreathing,
              "a pufferfish water breathing");
        check(Brewing::brewWith(Brew{ Potion::Awkward }, Items::TurtleHelmet, out) && out.potion == Potion::TurtleMaster,
              "and a turtle shell the turtle master");
        check(Brewing::brewWith(Brew{ Potion::Water }, Items::Sugar, out) && out.potion == Potion::Mundane, "sugar in water is mundane");
        check(Brewing::brewWith(Brew{ Potion::Water }, Items::GlowstoneDust, out) && out.potion == Potion::Thick, "glowstone in water is thick");

        const std::vector<Brewing::Dose> strongSpeed = Brewing::dosesOf(Brew{ Potion::Speed, false, true });
        check(strongSpeed.size() == 1 && strongSpeed[0].amplifier == 1 && strongSpeed[0].seconds == 90.0f, "swiftness II is a minute and a half");
        const std::vector<Brewing::Dose> longSpeed = Brewing::dosesOf(Brew{ Potion::Speed, true, false });
        check(longSpeed.size() == 1 && longSpeed[0].seconds == 480.0f, "long swiftness eight minutes");
        const std::vector<Brewing::Dose> turtle = Brewing::dosesOf(Brew{ Potion::TurtleMaster });
        check(turtle.size() == 2 && turtle[0].kind == Effects::Kind::Slowness && turtle[0].amplifier == 3 &&
                  turtle[1].kind == Effects::Kind::Resistance && turtle[1].amplifier == 2,
              "the turtle master is slowness IV with resistance III");
        check(Brewing::decode(Brewing::encode(Brew{ Potion::Poison, false, true })).strong &&
                  Brewing::fromDamage(Brewing::encode(Brew{ Potion::Poison, true, false })) == Potion::Poison,
              "a stack keeps its potion, length and strength");
        check(Brewing::decode(static_cast<int>(Potion::Speed)).potion == Potion::Speed, "and an old saved potion still reads");
        check(Brewing::formChange(Items::Potion, Items::Gunpowder) == Items::SplashPotion &&
                  Brewing::formChange(Items::SplashPotion, Items::DragonsBreath) == Items::LingeringPotion &&
                  Brewing::formChange(Items::Potion, Items::DragonsBreath) == Blocks::Air,
              "gunpowder makes a splash potion and dragon's breath a lingering one");
        check(Brewing::fullName(Brew{ Potion::Speed }, Brewing::Form::Splash) == "Splash Potion of Swiftness" &&
                  Brewing::fullName(Brew{ Potion::Water }, Brewing::Form::Lingering) == "Lingering Water Bottle",
              "and they are called what Minecraft calls them");
        check(Brewing::brewColour(Brew{ Potion::Water }) == 0x385DC6u &&
                  Brewing::brewColour(Brew{ Potion::Poison }) != Brewing::brewColour(Brew{ Potion::Speed }),
              "every brew has its own colour");
        bool ingredients = true;
        for (StackId id : { Items::GoldenCarrot, Items::SpiderEye, Items::Pufferfish, Items::TurtleHelmet, Items::Redstone,
                            Items::FermentedSpiderEye, Items::Gunpowder, Items::DragonsBreath })
            if (!Brewing::isIngredient(id)) ingredients = false;
        check(ingredients, "and the stand takes every one of their ingredients");

        ItemStack g[9];
        for (int i = 0; i < 9; ++i) g[i] = ItemStack{ Items::Arrow, 1 };
        g[4] = ItemStack{ Items::LingeringPotion, 1, Brewing::encode(Brew{ Potion::Slowness }) };
        const CraftOutput tipped = Crafting::match(g, 3);
        check(tipped.id == Items::TippedArrow && tipped.count == 8 && tipped.damage == static_cast<int>(Effects::Kind::Slowness),
              "eight arrows round a lingering potion make eight tipped arrows");
    }

    void testArmadillo()
    {
        section("the armadillo, its scutes and wolf armour");
        const MobType& armadillo = mobType(MobId::Armadillo);
        check(armadillo.breedingFood == Items::SpiderEye && armadillo.maxHealth == 12, "an armadillo has twelve health and loves spider eyes");
        bool shell = false;
        for (const MobBox& box : armadillo.model) if (box.part == Part::Shell) shell = true;
        check(shell, "and a ball to roll up into");
        bool savanna = false, badlands = false;
        for (int k = 0; k < 200; ++k)
        {
            if (EntityManager::pickSpawn(Biomes::Kind::Savanna, Biomes::Group::Creature, static_cast<float>(k) / 200.0f) == MobId::Armadillo) savanna = true;
            if (EntityManager::pickSpawn(Biomes::Kind::Badlands, Biomes::Group::Creature, static_cast<float>(k) / 200.0f) == MobId::Armadillo) badlands = true;
        }
        check(savanna && badlands, "it lives in savannas and badlands");

        Mob ball(MobId::Armadillo, glm::vec3(0.0f), 3u);
        const int full = ball.health();
        ball.damage(2, glm::vec3(1.0f, 0.0f, 0.0f));
        check(ball.rolled() && ball.health() == full - 2, "a hit makes it roll up");
        ball.damage(5, glm::vec3(1.0f, 0.0f, 0.0f));
        check(ball.health() == full - 4, "and rolled up it takes half of a blow less one");

        Mob wolf(MobId::Wolf, glm::vec3(0.0f), 5u);
        wolf.setHorseArmour(Items::WolfArmor);
        const int before = wolf.health();
        wolf.damage(6, glm::vec3(1.0f, 0.0f, 0.0f));
        check(wolf.health() == before && wolf.armourWear() == 6, "wolf armour takes the whole blow");
        wolf.damage(60, glm::vec3(1.0f, 0.0f, 0.0f));
        check(wolf.horseArmour() == 0 && wolf.takeArmourBreak(), "until it breaks");

        ItemStack g[9];
        const int cells[6] = { 0, 3, 4, 5, 6, 8 };
        for (ItemStack& cell : g) cell.clear();
        for (int c : cells) g[c] = ItemStack{ Items::ArmadilloScute, 1 };
        check(Crafting::match(g, 3).id == Items::WolfArmor, "six scutes make wolf armour");
        for (ItemStack& cell : g) cell.clear();
        g[0] = ItemStack{ Items::WolfArmor, 1, 10 };
        g[1] = ItemStack{ Items::DyeBlue, 1 };
        const CraftOutput dyed = Crafting::match(g, 3);
        check(dyed.id == Items::WolfArmor && dyed.damage == 10 && ((dyed.enchantments >> 24) & 0x1Fu) == static_cast<uint32_t>(Items::DyeBlue - Items::DyeWhite + 1),
              "and a dye colours it, keeping its wear");
        check(!Enchanting::glints(Items::WolfArmor, dyed.enchantments), "without making it shine");
        check(itemDurability(Items::WolfArmor) == 64, "it lasts sixty-four points");
    }

    void testGlowSquid()
    {
        section("the glow squid, its ink, and signs that glow");
        const MobType& glow = mobType(MobId::GlowSquid);
        const MobType& squid = mobType(MobId::Squid);
        check(glow.drop == Items::GlowInkSac && squid.drop == Items::InkSac, "a glow squid drops glow ink where a squid drops ink");
        check(glow.aquatic && glow.hoverHeight > 0.0f && glow.model.size() == squid.model.size(), "and swims on the squid's body");
        check(std::string(glow.texture) == "glow_squid", "wearing its own skin");
        unsigned int shell = 0, spots = 0;
        check(spawnEggColours(MobId::GlowSquid, shell, spots) && shell == 0x095656u && spots == 0x85F1BCu, "with its own spawn egg");

        Mob lit(MobId::GlowSquid, glm::vec3(0.0f), 5u);
        check(lit.glowLight() >= 1.0f, "it lights itself");
        lit.damage(1, glm::vec3(0.0f));
        check(lit.glowLight() <= 0.0f && lit.darkTime() >= 4.9f, "and goes dark for five seconds when hurt");
        Mob plain(MobId::Squid, glm::vec3(0.0f), 5u);
        check(plain.glowLight() <= 0.0f, "an ordinary squid gives no light");

        Signs signs;
        const glm::ivec3 at(4, 70, -2);
        signs.at(at) = Signs::Text{ "Hello", "", "", "" };
        check(Signs::plain(signs.look(at)) && signs.look(at).colour == 15, "a sign starts with plain black text");
        signs.setLook(at, Signs::Look{ 14, true });
        check(signs.look(at).colour == 14 && signs.look(at).glow, "dye and glow ink change how it reads");
        check(Signs::textRgb(14) == 0xFF0000u && Signs::shadedRgb(Signs::Look{ 14, false }) == 0x660000u,
              "red text is drawn at four tenths until it glows");
        check(Signs::shadedRgb(Signs::Look{ 15, true }) == 0xF0EBCCu, "glowing black ink is outlined in pale cream");
        signs.setLook(at, Signs::Look{});
        check(signs.looks().empty(), "plain text keeps no row");
        signs.setLook(at, Signs::Look{ 3, false });
        signs.remove(at);
        check(signs.looks().empty(), "and a sign taken down forgets its colour");

        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_signlook_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        LevelState level;
        level.seed = 9u;
        LevelState::SavedSign sign;
        sign.x = 3; sign.y = 65; sign.z = -8;
        sign.lines[0] = "Glow";
        level.signs.push_back(sign);
        LevelState::SavedSignLook look;
        look.x = 3; look.y = 65; look.z = -8; look.colour = 9; look.glow = 1;
        level.signLooks.push_back(look);
        WorldSave::saveLevel(dir, level);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.signLooks.size() == 1 && back.signLooks[0].colour == 9 &&
                  back.signLooks[0].glow == 1 && back.signLooks[0].z == -8,
              "a sign's colour and glow are kept with the level");
        std::filesystem::remove_all(dir, ec);
    }

    void testFangsZoglinIllusioner()
    {
        section("evoker fangs, the zoglin and the illusioner");
        const glm::vec3 caster(0.0f, 64.0f, 0.0f);
        const std::vector<Fangs::Spot> line = Fangs::pattern(caster, glm::vec3(10.0f, 64.0f, 0.0f));
        check(line.size() == 16 && std::fabs(line[0].x - 1.25f) < 0.001f && std::fabs(line[15].x - 20.0f) < 0.001f &&
                  line[15].warmup == 15,
              "a far target gets sixteen fangs in a line, a tick apart");
        const std::vector<Fangs::Spot> rings = Fangs::pattern(caster, glm::vec3(2.0f, 64.0f, 0.0f));
        int inner = 0, outer = 0;
        for (const Fangs::Spot& spot : rings)
        {
            const float r = std::sqrt(spot.x * spot.x + spot.z * spot.z);
            if (std::fabs(r - 1.5f) < 0.01f && spot.warmup == 0) ++inner;
            if (std::fabs(r - 2.5f) < 0.01f && spot.warmup == 3) ++outer;
        }
        check(rings.size() == 13 && inner == 5 && outer == 8, "a near one gets a ring of five and a ring of eight");

        Fangs::Fang fang;
        fang.at = glm::vec3(0.0f, 64.0f, 0.0f);
        fang.warmup = 2;
        int ticks = 0, bite = -1, start = -1, gone = -1;
        while (gone < 0 && ticks < 100)
        {
            const int events = Fangs::tick(fang);
            ++ticks;
            if (events & Fangs::Started) start = ticks;
            if (events & Fangs::Bite) bite = ticks;
            if (events & Fangs::Gone) gone = ticks;
        }
        check(start == 3 && bite == 10 && gone == 25, "a fang rises after its warmup, bites eight ticks later and is gone after twenty-two");
        check(Fangs::bites(fang, glm::vec3(0.5f, 64.0f, 0.2f), 0.3f, 1.8f), "it catches a player standing on it");
        check(!Fangs::bites(fang, glm::vec3(1.2f, 64.0f, 0.0f), 0.3f, 1.8f), "and not one a step away");
        check(Fangs::DAMAGE == 6, "for six points of magic");

        const MobType& zoglin = mobType(MobId::Zoglin);
        check(zoglin.maxHealth == 40 && zoglin.attackDamage == 6 && zoglin.drop == Items::RottenFlesh &&
                  zoglin.spawnClass == SpawnClass::Hostile && !zoglin.burnsInSunlight,
              "a zoglin has forty health, hits for six and drops rotten flesh");
        check(std::string(zoglin.packSheet) == "hoglin/zoglin.png" && zoglin.model.size() == mobType(MobId::Hoglin).model.size(),
              "on the hoglin's body in its own skin");
        Mob hoglin(MobId::Hoglin, glm::vec3(0.0f), 2u);
        hoglin.addZombifyTime(10.0f);
        hoglin.addZombifyTime(6.0f);
        check(hoglin.zombifyTime() > EntityManager::ZOMBIFY_SECONDS, "a hoglin out of the Nether turns after fifteen seconds");
        hoglin.addZombifyTime(-1.0f);
        check(hoglin.zombifyTime() == 0.0f, "and the count starts again back home");

        const MobType& illusioner = mobType(MobId::Illusioner);
        check(illusioner.maxHealth == 32 && illusioner.castsIllusions && illusioner.arrowDamage > 0 && !illusioner.conjuresFangs,
              "an illusioner has thirty-two health and a bow");
        unsigned int shell = 0, spots = 0;
        check(!spawnEggColours(MobId::Illusioner, shell, spots), "and no spawn egg, as in 1.21.1");
        Mob trick(MobId::Illusioner, glm::vec3(0.0f), 4u);
        check(!trick.illusioned(), "it starts out as one");
        trick.shuffleMirrors();
        bool spread = true;
        for (int k = 0; k < 4; ++k)
        {
            const glm::vec3 o = trick.mirrorOffset(k);
            if (std::fabs(o.x) > 3.0f || std::fabs(o.z) > 3.0f || o.y < 0.0f || o.y > 1.0f) spread = false;
        }
        check(spread, "and its copies stand within three blocks of it");
    }

    void testPackSheetsAndLlamas()
    {
        section("every mob on its pack sheet, the trader llama and llama spit");
        bool named = true;
        bool present = true;
        const bool packHere = std::filesystem::exists("assets/textures/entity");
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobType& t = mobType(static_cast<MobId>(i));
            const std::string sheet = t.packSheet;
            if (sheet.size() < 5 || sheet.substr(sheet.size() - 4) != ".png") named = false;
            else if (packHere && !std::filesystem::exists("assets/textures/entity/" + sheet)) present = false;
        }
        check(named, "every mob names its sheet in the pack");
        check(present, "and every one of those sheets is in the pack");
        check(std::string(mobType(MobId::Tadpole).packSheet) == "tadpole/tadpole.png", "the tadpole's is in the tadpole folder");

        const MobType& llama = mobType(MobId::Llama);
        check(llama.spits && llama.neutral && llama.arrowDamage == 1 && llama.arrowRange == 20.0f && llama.arrowInterval == 2.0f,
              "a llama spits for one, from twenty blocks, every two seconds, once provoked");
        check(!Mob::hostileNow(llama, false) && Mob::hostileNow(llama, true), "and only once provoked");
        const MobType& trader = mobType(MobId::TraderLlama);
        check(trader.spits && trader.model.size() == llama.model.size() && trader.breedingFood == Blocks::Air,
              "a trader llama is a llama that cannot be bred");
        unsigned int shell = 0, spots = 0;
        check(spawnEggColours(MobId::TraderLlama, shell, spots) && shell == 0xEAA430u && spots == 0x456296u,
              "with 1.21.1's spawn egg");
        Arrow spit;
        spit.kind = Arrows::Kind::Spit;
        check(!spit.flies() && spit.bursts(), "spit arcs like an arrow and splashes rather than sticking");
    }

    void testPaintings()
    {
        section("paintings: every variant, the biggest that fits, and popping off");
        int placeable = 0, largest = 0;
        for (const Paintings::Variant& v : Paintings::variants())
        {
            if (v.placeable) ++placeable;
            largest = std::max(largest, v.width * v.height);
        }
        check(Paintings::count() == 50 && placeable == 46, "fifty paintings, forty-six of them hung by hand");
        check(largest == 16 && Paintings::find("pointer") >= 0 &&
                  Paintings::variants()[static_cast<size_t>(Paintings::find("pointer"))].width == 4,
              "from one block up to four by four");
        check(!Paintings::variants()[static_cast<size_t>(Paintings::find("earth"))].placeable, "the four elements are creative only");

        const Paintings::Placed big{ glm::ivec3(0, 10, 0), 0, Paintings::find("pointer") };
        const std::vector<glm::ivec3> cells = Paintings::cells(big);
        int minX = 99, maxX = -99, minY = 99, maxY = -99;
        for (const glm::ivec3& c : cells)
        {
            minX = std::min(minX, c.x); maxX = std::max(maxX, c.x);
            minY = std::min(minY, c.y); maxY = std::max(maxY, c.y);
        }
        check(cells.size() == 16 && minX == -1 && maxX == 2 && minY == 9 && maxY == 12, "a four by four reaches one back and two on, as Minecraft offsets it");

        struct TestWall : Paintings::Wall
        {
            int width = 1, height = 1;
            bool solid(int x, int y, int z) const override
            {
                return z == -1 && x >= 0 && x < width && y >= 10 && y < 10 + height;
            }
            bool open(int x, int y, int z) const override { (void)x; (void)y; return z >= 0; }
        };
        TestWall wall;
        wall.width = 1;
        wall.height = 1;
        const int one = Paintings::choose(glm::ivec3(0, 10, 0), 0, wall, 7u);
        check(one >= 0 && Paintings::variants()[static_cast<size_t>(one)].width == 1 &&
                  Paintings::variants()[static_cast<size_t>(one)].height == 1,
              "a one-block wall gets a one-block painting");
        wall.width = 2;
        const int wide = Paintings::choose(glm::ivec3(0, 10, 0), 0, wall, 7u);
        check(wide >= 0 && Paintings::variants()[static_cast<size_t>(wide)].width == 2 &&
                  Paintings::variants()[static_cast<size_t>(wide)].height == 1,
              "two across gets one two across");
        wall.width = 4;
        wall.height = 4;
        const int huge = Paintings::choose(glm::ivec3(1, 11, 0), 0, wall, 3u);
        check(huge >= 0 && Paintings::variants()[static_cast<size_t>(huge)].width * Paintings::variants()[static_cast<size_t>(huge)].height == 16,
              "a four by four wall gets the biggest there is");
        std::set<int> picks;
        for (uint32_t roll = 0; roll < 12; ++roll) picks.insert(Paintings::choose(glm::ivec3(1, 11, 0), 0, wall, roll));
        check(picks.size() > 1, "picked at random among the biggest");
        check(!Paintings::fits(Paintings::Placed{ glm::ivec3(1, 11, 0), 0, huge }, TestWall{}), "and it comes down when the wall goes");
        const std::vector<Paintings::Placed> hung = { Paintings::Placed{ glm::ivec3(1, 11, 0), 0, huge } };
        check(Paintings::choose(glm::ivec3(0, 10, 0), 0, wall, 1u, hung) < 0 ||
                  !Paintings::overlaps(Paintings::Placed{ glm::ivec3(0, 10, 0), 0, Paintings::choose(glm::ivec3(0, 10, 0), 0, wall, 1u, hung) }, hung[0]),
              "and two paintings never overlap");

        const Paintings::Box box = Paintings::bounds(Paintings::Placed{ glm::ivec3(0, 10, 0), 0, Paintings::find("kebab") });
        check(std::fabs(box.min.z - 0.0f) < 0.001f && std::fabs(box.max.z - 0.0625f) < 0.001f, "a sixteenth deep against the wall");
        float distance = 0.0f;
        check(Paintings::rayHits(box, glm::vec3(0.5f, 10.5f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), 5.0f, distance) &&
                  std::fabs(distance - 2.9375f) < 0.01f,
              "and a punch finds it");

        ItemStack grid[9];
        for (ItemStack& cell : grid) cell = ItemStack{ Items::Stick, 1 };
        grid[4] = ItemStack{ static_cast<StackId>(Blocks::WoolOrange + 3), 1 };
        check(Crafting::match(grid, 3).id == Items::Painting, "eight sticks round any wool make a painting");

        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_painting_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        LevelState level;
        level.seed = 12u;
        LevelState::SavedPainting saved;
        saved.x = 4; saved.y = 70; saved.z = -9; saved.facing = 3; saved.variant = "sunflowers";
        level.paintings.push_back(saved);
        WorldSave::saveLevel(dir, level);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.paintings.size() == 1 && back.paintings[0].variant == "sunflowers" &&
                  back.paintings[0].facing == 3 && back.paintings[0].z == -9,
              "and they are kept with the level");
        std::filesystem::remove_all(dir, ec);

        int w = 0, h = 0;
        const std::vector<uint8_t> painted = PaintingRenderer::paintVariant(Paintings::find("skeleton"), w, h);
        check(w == 64 && h == 48 && painted.size() == static_cast<size_t>(64 * 48 * 4), "and each has a painted stand-in the size of the real one");
    }

    void testAdvancements()
    {
        section("advancements: 1.21.1's five trees, their rules and their layout");
        const std::vector<Advancements::Def>& defs = Advancements::all();
        check(defs.size() == 122, "all 122 advancements of 1.21.1");
        int roots = 0;
        bool parents = true, titled = true;
        std::set<std::string> ids;
        for (const Advancements::Def& def : defs)
        {
            ids.insert(def.id);
            if (def.parent[0] == '\0') ++roots;
            else parents = parents && Advancements::indexOf(def.parent) >= 0;
            titled = titled && def.title[0] != '\0' && def.description[0] != '\0' && !def.criteria.empty();
        }
        check(ids.size() == defs.size() && roots == 5, "each has its own id, and there are five roots");
        check(parents && titled, "every parent exists, and each has a title, a description and something to do");
        std::string missing;
        int resolved = 0;
        for (const Advancements::Def& def : defs)
        {
            if (RecipeBook::stackNamed(def.icon) != Blocks::Air) ++resolved;
            else missing += std::string(def.icon) + " ";
        }
        if (!missing.empty()) std::printf("    icons not in the game: %s\n", missing.c_str());
        check(resolved >= 110, "nearly every icon is an item the game has");
        const Advancements::Def& stone = defs[static_cast<size_t>(Advancements::indexOf("story/mine_stone"))];
        check(std::string(stone.title) == "Stone Age" && stone.x == 1.0f && Advancements::rootOf(Advancements::Tab::Story) == 0,
              "titles are Mojang's and the trees lie as 1.21.1 lays them");

        Advancements::Tracker tracker;
        check(!tracker.tabVisible(Advancements::Tab::Story), "a new world shows nothing");
        check(tracker.grant("story/root", "crafting_table") && tracker.done(0), "a crafting table makes the first one");
        check(!tracker.grant("story/root", "crafting_table"), "and only once");
        check(tracker.takeNew().size() == 1 && tracker.takeNew().empty(), "it is reported once, for the toast");
        const int stoneAge = Advancements::indexOf("story/mine_stone");
        const int upgrade = Advancements::indexOf("story/upgrade_tools");
        const int iron = Advancements::indexOf("story/smelt_iron");
        check(tracker.tabVisible(Advancements::Tab::Story) && tracker.visible(stoneAge) && tracker.visible(upgrade) && !tracker.visible(iron),
              "and opens the tab two steps ahead, as 1.21.1 does");
        const int hidden = Advancements::indexOf("nether/all_effects");
        check(defs[static_cast<size_t>(hidden)].hidden && !tracker.visible(hidden), "a hidden one stays hidden until it is done");
        const int armour = Advancements::indexOf("story/obtain_armor");
        check(tracker.grant("story/obtain_armor", "iron_boots") && tracker.done(armour), "any one piece of iron armour suits up");
        const int hunted = Advancements::indexOf("adventure/kill_all_mobs");
        tracker.grant("adventure/kill_all_mobs", "zombie");
        tracker.grant("adventure/kill_all_mobs", "creeper");
        check(tracker.criteriaNeeded(hunted) == 36 && tracker.criteriaDone(hunted) == 2 && !tracker.done(hunted),
              "Monsters Hunted counts its thirty-six");
        check(!tracker.grant("adventure/kill_all_mobs", "pig"), "and a pig is not one of them");
        Advancements::Tracker reloaded;
        reloaded.load(tracker.lines());
        check(reloaded.done(0) && reloaded.done(armour) && reloaded.criteriaDone(hunted) == 2 && reloaded.takeNew().empty(),
              "progress is saved and read back without fresh toasts");
        check(std::string(Advancements::frameText(Advancements::Frame::Challenge)) == "Challenge Complete!", "challenges announce themselves");
    }

    void testStatistics()
    {
        section("statistics: what is counted and how it reads");
        using Stats::Custom;
        using Stats::Format;
        check(Stats::CUSTOM_COUNT == 75, "the seventy-five general stats of 1.21.1");
        std::set<std::string> ids;
        bool named = true;
        for (int i = 0; i < Stats::CUSTOM_COUNT; ++i)
        {
            ids.insert(Stats::id(static_cast<Custom>(i)));
            named = named && Stats::name(static_cast<Custom>(i))[0] != '\0';
        }
        check(ids.size() == static_cast<size_t>(Stats::CUSTOM_COUNT) && named, "each has its own id and a name");
        check(std::string(Stats::id(Custom::Jump)) == "jump" && std::string(Stats::name(Custom::Jump)) == "Jumps" &&
              std::string(Stats::name(Custom::WalkOneCm)) == "Distance Walked", "names are Mojang's");
        check(Stats::format(Custom::PlayTime) == Format::Time && Stats::format(Custom::SprintOneCm) == Format::Distance &&
              Stats::format(Custom::DamageDealt) == Format::Tenths && Stats::format(Custom::Deaths) == Format::Plain,
              "each stat reads in its own unit");
        check(Stats::formatted(Format::Plain, 1234567) == "1,234,567" && Stats::formatted(Format::Plain, 12) == "12",
              "counts are grouped by thousands");
        check(Stats::formatted(Format::Time, 20) == "1.00 s" && Stats::formatted(Format::Time, 1200) == "1.00 min" &&
              Stats::formatted(Format::Time, 114280) == "1.59 h" && Stats::formatted(Format::Time, 20 * 3600 * 24) == "1.00 d",
              "ticks read as seconds, minutes, hours and days");
        check(Stats::formatted(Format::Distance, 50) == "50 cm" && Stats::formatted(Format::Distance, 51) == "0.51 m" &&
              Stats::formatted(Format::Distance, 250000) == "2.50 km", "centimetres step up past half a unit");
        check(Stats::formatted(Format::Tenths, 3860) == "386.00", "damage is kept in tenths of a point");

        Stats::Book book;
        book.add(Custom::Jump);
        book.add(Custom::Jump, 4);
        book.set(Custom::Deaths, 2);
        book.item(Stats::ItemStat::Mined, Blocks::Stone, 3);
        book.item(Stats::ItemStat::Crafted, Items::Stick, 8);
        book.item(Stats::ItemStat::Dropped, Items::Stick, 0);
        book.killed(static_cast<int>(MobId::Zombie), 2);
        book.killedBy(static_cast<int>(MobId::Creeper));
        check(book.get(Custom::Jump) == 5 && book.get(Custom::Deaths) == 2, "general stats add and set");
        check(book.count(Stats::ItemStat::Mined, Blocks::Stone) == 3 && book.count(Stats::ItemStat::Crafted, Items::Stick) == 8 &&
              book.count(Stats::ItemStat::Dropped, Items::Stick) == 0, "item stats are kept per item");
        check(book.kills(static_cast<int>(MobId::Zombie)) == 2 && book.deathsBy(static_cast<int>(MobId::Creeper)) == 1 &&
              book.kills(static_cast<int>(MobId::Creeper)) == 0, "kills and deaths are kept per mob");
        Stats::Book reloaded;
        reloaded.load(book.lines());
        check(reloaded.get(Custom::Jump) == 5 && reloaded.count(Stats::ItemStat::Crafted, Items::Stick) == 8 &&
              reloaded.deathsBy(static_cast<int>(MobId::Creeper)) == 1 && reloaded.items().size() == 2,
              "the book is saved and read back");

        Player player(glm::vec3(0.0f, 70.0f, 0.0f));
        player.damage(4, true);
        Player::Tally tally = player.takeTally();
        check(tally.taken == 4 && tally.absorbed == 0 && player.takeTally().taken == 0, "a blow is tallied once");
        player.clearHurtCooldown();
        player.absorption = 4;
        player.damage(6, true);
        tally = player.takeTally();
        check(tally.absorbed == 4 && tally.taken == 2, "golden hearts are tallied apart from real ones");

        Inventory bench;
        bench.slot(Inventory::CRAFT_FIRST) = ItemStack{ Blocks::Log, 1 };
        bench.refreshCraftResult();
        bench.takeCraftResult();
        const auto crafted = bench.takeCrafted();
        check(crafted.size() == 1 && crafted[0].first == Blocks::Planks && crafted[0].second == 4 && bench.takeCrafted().empty(),
              "what comes off the grid is logged for the crafted column");
    }

    void testWorlds()
    {
        section("worlds: the save list, Create World and the world types");
        check(Worlds::sanitize("My: World?") == "My_ World_" && Worlds::sanitize("a/b.c") == "a_b_c" && Worlds::sanitize("CON") == "_CON_",
              "folder names lose what a file name cannot hold, as 1.21.1's do");
        const std::string saves = (std::filesystem::temp_directory_path() / "bc_selftest_saves").string();
        std::filesystem::remove_all(saves);
        std::filesystem::create_directories(saves);
        check(Worlds::uniqueFolder(saves, "   ") == "New World", "a blank name is a New World");
        Worlds::Meta first;
        first.name = "Castle";
        first.lastPlayed = 1000;
        Worlds::writeMeta(saves + "/" + Worlds::uniqueFolder(saves, "Castle"), first);
        check(Worlds::uniqueFolder(saves, "Castle") == "Castle (1)", "a taken folder gets a number");
        Worlds::writeMeta(saves + "/Castle (1)", first);
        check(Worlds::uniqueFolder(saves, "Castle (1)") == "Castle (2)", "and the number counts on");
        Worlds::Meta second;
        second.name = "Beach House";
        second.lastPlayed = 5000;
        second.mode = static_cast<int>(GameMode::Creative);
        second.cheats = true;
        second.type = Worlds::Type::Flat;
        second.structures = false;
        second.bonusChest = true;
        second.difficulty = Worlds::Difficulty::Peaceful;
        second.difficultyLocked = true;
        second.layers = "minecraft:bedrock,3*minecraft:sand";
        second.biome = "minecraft:desert";
        Worlds::writeMeta(saves + "/Beach House", second);
        const std::vector<Worlds::Entry> worlds = Worlds::list(saves);
        check(worlds.size() == 3 && worlds[0].folder == "Beach House", "the list holds every save, last played first");
        const Worlds::Meta& read = worlds[0].meta;
        check(read.name == "Beach House" && read.cheats && read.type == Worlds::Type::Flat && !read.structures && read.bonusChest &&
              read.difficulty == Worlds::Difficulty::Peaceful && read.difficultyLocked && read.layers == second.layers &&
                  read.mode == static_cast<int>(GameMode::Creative),
              "each world's settings are kept beside it");
        check(Worlds::matches(worlds[0], "beach") && !Worlds::matches(worlds[0], "castle"), "the search box matches names");
        std::filesystem::remove_all(saves);

        const std::vector<Worlds::Layer> layers = Worlds::parseLayers("minecraft:bedrock,2*minecraft:dirt,grass_block");
        check(layers.size() == 3 && layers[1].count == 2 && layers[2].block == "minecraft:grass_block" &&
              Worlds::formatLayers(layers) == "minecraft:bedrock,2*minecraft:dirt,minecraft:grass_block", "superflat layers read and write");
        std::string presetLayers, presetBiome;
        check(Worlds::parsePresetCode("minecraft:bedrock,2*minecraft:dirt,minecraft:grass_block;minecraft:plains", presetLayers, presetBiome) &&
              presetBiome == "minecraft:plains" && !Worlds::parsePresetCode(";minecraft:plains", presetLayers, presetBiome),
              "a preset code is a layer list and a biome");
        check(Worlds::flatPresets().size() == 9 && std::string(Worlds::flatPresets()[0].name) == "Classic Flat" &&
              std::string(Worlds::flatPresets()[8].name) == "The Void", "the nine 1.21.1 superflat presets");
        check(Worlds::seedFrom("12345", 7) == 12345u && Worlds::seedFrom("", 7) == 7u && Worlds::javaHash("hello") == 99162322 &&
              Worlds::seedFrom("hello", 7) == static_cast<uint32_t>(99162322), "seeds: numbers as they are, words hashed the way Java hashes them");
        check(std::string(Worlds::typeName(Worlds::Type::LargeBiomes)) == "Large Biomes" && Worlds::typeFromId("flat") == Worlds::Type::Flat,
              "the world types carry Mojang's names");

        WorldGen::Options flat;
        flat.type = WorldGen::Options::Type::Flat;
        flat.layers = { { Blocks::Bedrock, 1 }, { Blocks::Dirt, 2 }, { Blocks::Grass, 1 } };
        flat.biome = Biome::Desert;
        WorldGen flatGen(1234u, Dimension::Overworld, flat);
        Chunk chunk(ChunkPos{ 3, -2 });
        flatGen.generate(chunk);
        check(chunk.getBlock(5, 0, 5) == Blocks::Bedrock && chunk.getBlock(5, 2, 5) == Blocks::Dirt && chunk.getBlock(5, 3, 5) == Blocks::Grass &&
              chunk.getBlock(5, 4, 5) == Blocks::Air, "a superflat chunk is its layers and nothing else");
        check(flatGen.surfaceHeight(999, -999) == 3 && flatGen.biomeAt(50, 50) == Biome::Desert, "and is one height and one biome");
        int sx = 0, sy = 0, sz = 0;
        flatGen.findSpawn(sx, sy, sz);
        check(sx == 8 && sz == 8 && sy == 5, "you start above the middle of it");
        WorldGen::Options empty;
        empty.type = WorldGen::Options::Type::Flat;
        empty.layers = { { Blocks::Air, 1 } };
        WorldGen voidGen(1u, Dimension::Overworld, empty);
        Chunk origin(ChunkPos{ 0, 0 });
        voidGen.generate(origin);
        check(origin.getBlock(8, WorldGen::VOID_PLATFORM_Y, 8) == Blocks::Cobblestone && origin.getBlock(0, WorldGen::VOID_PLATFORM_Y, 0) == Blocks::Stone &&
              origin.getBlock(8, 0, 8) == Blocks::Air, "the void gets its stone platform");
        WorldGen::Options single;
        single.type = WorldGen::Options::Type::SingleBiome;
        single.biome = Biome::CherryGrove;
        WorldGen singleGen(99u, Dimension::Overworld, single);
        check(singleGen.biomeAt(0, 0) == Biome::CherryGrove && singleGen.biomeAt(4000, -9000) == Biome::CherryGrove, "a single biome is everywhere");
        WorldGen::Options tall;
        tall.type = WorldGen::Options::Type::Amplified;
        WorldGen normalGen(77u), tallGen(77u, Dimension::Overworld, tall);
        int higher = 0, checked = 0;
        for (int x = -2000; x <= 2000; x += 97)
            for (int z = -2000; z <= 2000; z += 89)
            {
                const int plain = normalGen.surfaceHeight(x, z);
                if (plain <= WorldGen::SEA_LEVEL + 4) continue;
                ++checked;
                if (tallGen.surfaceHeight(x, z) > plain) ++higher;
            }
        check(checked > 0 && higher * 10 >= checked * 9, "amplified land stands taller");
        WorldGen::Options none;
        none.structures = false;
        check(!WorldGen(5u, Dimension::Overworld, none).structuresOn() && WorldGen(5u).structuresOn(), "structures can be turned off");
        check(biomeFromId("minecraft:windswept_hills") == Biome::Mountains && std::string(biomeId(Biome::Snowy)) == "minecraft:snowy_plains",
              "biomes go by their 1.21.1 ids");

        check(Player::scaledByDifficulty(4, 0) == 0 && Player::scaledByDifficulty(4, 1) == 3 && Player::scaledByDifficulty(4, 2) == 4 &&
              Player::scaledByDifficulty(4, 3) == 6 && Player::scaledByDifficulty(1, 1) == 1, "mob blows scale with difficulty");
        Player easy(glm::vec3(0.0f, 70.0f, 0.0f));
        easy.difficulty = 1;
        easy.hunger = 0;
        easy.saturation = 0.0f;
        easy.health = 10;
        for (int i = 0; i < 40; ++i) easy.updateVitals(0.5f);
        check(easy.health == 10, "on easy hunger stops at five hearts");
        Player hard(glm::vec3(0.0f, 70.0f, 0.0f));
        hard.difficulty = 3;
        hard.hunger = 0;
        hard.saturation = 0.0f;
        hard.health = 4;
        for (int i = 0; i < 80; ++i) { hard.updateVitals(0.5f); hard.clearHurtCooldown(); }
        check(hard.health == 0, "on hard it drains everything");
        Player calm(glm::vec3(0.0f, 70.0f, 0.0f));
        calm.difficulty = 0;
        calm.hunger = 10;
        calm.health = 10;
        for (int i = 0; i < 10; ++i) calm.updateVitals(0.5f);
        check(calm.health > 10 && calm.hunger > 10, "peaceful heals and feeds you");
    }

    void testServerList()
    {
        section("server list: saved servers, addresses and ping bars");
        std::vector<Servers::Entry> servers(2);
        servers[0].name = "Home";
        servers[0].address = "192.168.1.4";
        servers[1].name = "Friends";
        servers[1].address = "play.example.net:25600";
        servers[1].packs = Servers::Packs::Enabled;
        const std::vector<Servers::Entry> read = Servers::parse(Servers::lines(servers));
        check(read.size() == 2 && read[1].name == "Friends" && read[1].address == "play.example.net:25600" &&
              read[1].packs == Servers::Packs::Enabled && read[0].packs == Servers::Packs::Prompt, "saved servers are kept with their settings");
        Servers::move(servers, 1, 0);
        check(servers[0].name == "Friends" && servers[1].name == "Home", "moving a server swaps it with its neighbour");
        Servers::move(servers, 0, 5);
        check(servers[0].name == "Friends", "and never past the ends");
        std::string host;
        uint16_t port = 0;
        check(Net::splitAddress("10.0.0.2", host, port) && host == "10.0.0.2" && port == Net::DEFAULT_PORT, "an address without a port uses the default");
        check(Net::splitAddress(" example.org:25600 ", host, port) && host == "example.org" && port == 25600, "a port after the colon is used");
        check(!Net::splitAddress("example.org:99999", host, port) && !Net::splitAddress("", host, port) &&
              !Net::splitAddress("example.org:", host, port), "a bad port or no address is refused");
        check(Net::pingBars(40) == 5 && Net::pingBars(150) == 4 && Net::pingBars(450) == 3 && Net::pingBars(800) == 2 && Net::pingBars(2000) == 1,
              "ping bars step down at 150, 300, 600 and 1000 ms, as 1.21.1's do");
    }

    void testResourcePacks()
    {
        section("resource packs: reading packs and choosing their files");
        check(Packs::descriptionFrom("{\"pack\":{\"pack_format\":34,\"description\":\"Shiny blocks\"}}") == "Shiny blocks" &&
              Packs::formatFrom("{\"pack\":{\"pack_format\":34,\"description\":\"x\"}}") == 34, "a pack's description and format are read");
        check(Packs::descriptionFrom("{\"pack\":{\"description\":[{\"text\":\"Two \"},{\"text\":\"parts\"}],\"pack_format\":15}}") == "Two parts" &&
              Packs::descriptionFrom("{\"pack\":{\"description\":\"\u00a7aGreen\"}}").find("Green") != std::string::npos,
              "text components and colour codes come out as plain text");
        const std::vector<std::string> picked = Packs::selectionFrom("file/A,file/B Pack");
        check(picked.size() == 2 && picked[1] == "file/B Pack" && Packs::selectionLine(picked) == "file/A,file/B Pack", "the chosen order is saved and read back");
        const std::string root = (std::filesystem::temp_directory_path() / "bc_selftest_pack").string();
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root + "/assets/minecraft/textures/block");
        if (std::FILE* f = std::fopen((root + "/assets/minecraft/textures/block/only_in_pack.png").c_str(), "wb")) { std::fputs("x", f); std::fclose(f); }
        Packs::setActive({ root });
        check(Packs::file("assets/textures/block/only_in_pack.png") == root + "/assets/minecraft/textures/block/only_in_pack.png",
              "a chosen pack's texture is used in place of the game's");
        check(Packs::file("assets/textures/block/stone.png") == "assets/textures/block/stone.png" &&
              Packs::file("world/saves/x/icon.png") == "world/saves/x/icon.png", "and anything it lacks falls through to the game");
        Packs::setActive({});
        check(Packs::file("assets/textures/block/only_in_pack.png") == "assets/textures/block/only_in_pack.png", "with no packs nothing changes");
        std::filesystem::remove_all(root);
    }

    void testFonts()
    {
        section("fonts: UTF-8 text");
        check(Utf8::length("Gr\u00fc\u00dfe") == 5 && Utf8::decode("\u0416")[0] == 0x416 && Utf8::encode(U"\u00e9") == "\u00e9",
              "text is read and written as UTF-8");
        std::string word = "ca\u00e9";
        Utf8::popBack(word);
        check(word == "ca", "backspace takes a whole letter, not half of one");
        word = "\u00e9t\u00e9";
        Utf8::popFront(word);
        check(word == "t\u00e9", "and so does trimming from the front");
        check(Utf8::decode(std::string("a\xC3")).back() == Utf8::REPLACEMENT, "a broken sequence reads as a replacement character");
        std::string typed;
        check(Utf8::type(typed, "\u00e7a\x01\u00a7\uE001\u0416", 10) && typed == "\u00e7a\u0416",
              "typing keeps letters from every script and drops control characters, the section sign and font marks");
        typed.clear();
        Utf8::type(typed, "\u00e9\u00e9\u00e9\u00e9", 3);
        check(Utf8::length(typed) == 3, "a field's limit counts letters, not bytes");
        std::string longLine;
        for (int i = 0; i < 150; ++i) longLine += "\u00e9";
        const Chat::Command said = Chat::parse(longLine, false);
        check(Utf8::length(said.text) == Chat::MAX_LENGTH && Utf8::decode(said.text).back() == 0xE9, "chat is cut to its length on a letter boundary");

        section("fonts: the glyph sheets");
        check(Font::has(0xE9) && Font::has(0xC9) && Font::has(0x416) && Font::has(0x3A9) && Font::has(0x142),
              "accented, Cyrillic and Greek letters all have glyphs");
        check(Font::advanceOf(' ') == 4.0f && Font::advanceOf(0x200C) == 0.0f, "a space is four wide and a zero-width non-joiner nothing");
        const Glyph& missing = Font::glyph(0x4E2D);
        check(!Font::has(0x4E2D) && missing.ink && missing.advance == 6.0f, "a character no sheet has is drawn as 1.21.1's missing box, six wide");
        check(Font::glyph(0xC9).top < 0.0f && Font::glyph('E').top == 0.0f, "an accented capital reaches above the line, as on the accented sheet");
        check(Font::has('a', FontFace::Galactic) && Font::has('Z', FontFace::Galactic) && !Font::has('1', FontFace::Galactic),
              "the galactic alphabet has letters only");
        check(Font::has('a', FontFace::Illager) && Font::has('7', FontFace::Illager) && Font::advanceOf(' ', FontFace::Illager) == 4.0f,
              "the illager runes have letters, digits and the space");
        check(UIRenderer::textWidth("abc", 1.0f, FontFace::Galactic) == Font::width("abc", FontFace::Galactic) - 1.0f,
              "text in another font is measured in that font");
        std::string marked;
        Utf8::append(marked, FONT_SWITCH + static_cast<char32_t>(FontFace::Galactic));
        marked += "abc";
        check(UIRenderer::textWidth(marked, 1.0f) == UIRenderer::textWidth("abc", 1.0f, FontFace::Galactic),
              "a font mark in the text switches the font from there on");
        const std::string trimmed = UIRenderer::trimToWidth("the elder words", 20.0f, FontFace::Galactic);
        check(!trimmed.empty() && Font::width(trimmed, FontFace::Galactic) <= 20.0f &&
              Font::width(UIRenderer::trimToWidth("the elder words", 20.0f, FontFace::Galactic) + "w", FontFace::Galactic) > 20.0f,
              "trimming keeps as much as fits and no more");

        section("fonts: painted when no pack has them");
        FontPaint::Bitmap e, eAcute, eCap, eCapAcute, zhe, omega, rune1, rune2, box;
        check(FontPaint::paint(FontFace::Default, 'e', e) && FontPaint::paint(FontFace::Default, 0xE9, eAcute) &&
              eAcute.inkWidth() == e.inkWidth() && eAcute.top == 0, "a lower-case accent sits in the space above the letter");
        check(FontPaint::paint(FontFace::Default, 'E', eCap) && FontPaint::paint(FontFace::Default, 0xC9, eCapAcute) &&
              eCapAcute.top < 0 && eCapAcute.h > eCap.h, "a capital's accent is drawn above the line, so the capital stays full height");
        FontPaint::Bitmap iAcute, ringA;
        check(FontPaint::paint(FontFace::Default, 0xED, iAcute) && !iAcute.at(0, -iAcute.top), "an accented i loses its dot");
        check(FontPaint::paint(FontFace::Default, 0x416, zhe) && FontPaint::paint(FontFace::Default, 0x3A9, omega) &&
              zhe.inkWidth() == 5 && omega.inkWidth() == 5, "Cyrillic and Greek letters are painted");
        check(FontPaint::paint(FontFace::Default, 0xE7, ringA) && ringA.h > 8, "a mark below hangs under the letter");
        check(!FontPaint::paint(FontFace::Default, 0x4E2D, box), "and a character with no painting is left to the missing box");
        int painted = 0, total = 0;
        for (const FontMaps::Sheet& sheet : FontMaps::sheets())
            for (int i = 0; i < sheet.rows * 16; ++i)
            {
                if (!sheet.chars[i]) continue;
                ++total;
                FontPaint::Bitmap any;
                if (FontPaint::paint(sheet.face, sheet.chars[i], any)) ++painted;
            }
        check(total > 2000 && painted > 1000, "over a thousand of the sheets' characters can be painted");
        check(FontPaint::paint(FontFace::Galactic, 'a', rune1) && FontPaint::paint(FontFace::Galactic, 'b', rune2) && rune1.ink != rune2.ink &&
              FontPaint::paint(FontFace::Galactic, 'A', box) && box.ink == rune1.ink, "painted runes differ by letter and ignore case");
        FontPaint::Bitmap illager;
        check(FontPaint::paint(FontFace::Illager, 'a', illager) && illager.ink != rune1.ink, "and the illager runes are not the galactic ones");

        section("fonts: /tellraw and the enchanting table");
        std::string out;
        check(Chat::textComponent("{\"text\":\"hi\",\"font\":\"minecraft:alt\"}", out) && Utf8::decode(out) == std::u32string(U"\uE001hi"),
              "a component's font is kept with its text");
        check(Chat::textComponent("[\"a \",{\"text\":\"b\",\"font\":\"illageralt\",\"extra\":[\"c\",{\"text\":\"d\",\"font\":\"default\"}]}]", out) &&
              Utf8::decode(out) == std::u32string(U"a \uE002bc\uE000d"), "children take their parent's font unless they name one");
        check(Chat::textComponent("\"plain \\u00e9\"", out) && out == "plain \u00e9" && Chat::textComponent("[1,true]", out) && out == "1true",
              "plain strings, escapes, numbers and booleans are text too");
        check(!Chat::textComponent("{\"text\":}", out) && !Chat::textComponent("{\"text\":\"x\"} junk", out), "broken JSON is refused");
        const Chat::Command raw = Chat::parse("/tellraw @a {\"text\":\"Hello  there\",\"font\":\"alt\"}", true);
        check(raw.kind == Chat::Kind::Tellraw && Utf8::decode(raw.text) == std::u32string(U"\uE001Hello  there"), "/tellraw prints its message as given");
        check(Chat::parse("/tellraw @a \"x\"", false).kind == Chat::Kind::Denied && Chat::parse("/tellraw @a", true).kind == Chat::Kind::Unknown,
              "and needs cheats and a message");
        const std::string phrase = Enchanting::phrase(1234u, 0);
        const size_t spaces = static_cast<size_t>(std::count(phrase.begin(), phrase.end(), ' '));
        check(phrase == Enchanting::phrase(1234u, 0) && phrase != Enchanting::phrase(1234u, 1) && spaces >= 2 && spaces <= 3,
              "an offer's runes are three or four words, fixed by the table's seed and the row");
    }

    void testAllBiomes()
    {
        section("biomes: all 64 of 1.21.1's");
        check(Biomes::COUNT == 64, "the game knows every biome in the 1.21.1 data");
        bool named = true, roundTrip = true;
        for (int i = 0; i < Biomes::COUNT; ++i)
        {
            const Biomes::Kind kind = static_cast<Biomes::Kind>(i);
            named = named && Biomes::info(kind).title[0] != '\0' && std::string(Biomes::info(kind).id).rfind("minecraft:", 0) == 0;
            roundTrip = roundTrip && Biomes::fromId(Biomes::info(kind).id) == kind;
        }
        check(named && roundTrip, "each has its id and English name, and is found again by its id");
        check(Biomes::fromId("birch_forest") == Biomes::Kind::BirchForest && biomeFromId("minecraft:birch_forest") == Biome::Forest &&
              biomeFromId("minecraft:ice_spikes") == Biome::Snowy && !Biomes::known("minecraft:nowhere"), "ids work with or without the namespace");
        check(Biomes::dimension(Biomes::Kind::SoulSandValley) == Dimension::Nether && Biomes::dimension(Biomes::Kind::EndHighlands) == Dimension::End &&
              Biomes::dimension(Biomes::Kind::Meadow) == Dimension::Overworld, "and belongs to its dimension");
        check(Biomes::info(Biomes::Kind::Swamp).water == 0x617B64u && Biomes::info(Biomes::Kind::Badlands).grass == 9470285 &&
              Biomes::info(Biomes::Kind::DarkForest).grassModifier == 1 && std::string(Biomes::info(Biomes::Kind::BasaltDeltas).particle) == "white_ash" &&
              Biomes::info(Biomes::Kind::FrozenOcean).frozen && !Biomes::info(Biomes::Kind::Desert).precipitation,
              "with the climate and colours from the data");

        const WorldGen generator(4242u);
        int seen[Biomes::COUNT] = {};
        bool families = true;
        for (int z = -6000; z <= 6000; z += 61)
            for (int x = -6000; x <= 6000; x += 61)
            {
                const Biomes::Kind kind = generator.kindAt(x, z);
                ++seen[static_cast<int>(kind)];
                families = families && Biomes::family(kind) == generator.biomeAt(x, z);
            }
        check(families, "a biome always sits inside the broader kind of land the terrain was built for");
        int overworld = 0;
        std::string missing;
        for (int i = 0; i < Biomes::COUNT; ++i)
            if (Biomes::dimension(static_cast<Biomes::Kind>(i)) == Dimension::Overworld)
            {
                if (seen[i] > 0) ++overworld;
                else missing += std::string(" ") + Biomes::info(static_cast<Biomes::Kind>(i)).id;
            }
        std::printf("  overworld biomes not found on the surface:%s\n", missing.c_str());
        check(overworld >= 40, "most of the overworld's biomes turn up on the surface of one world");
        check(seen[static_cast<int>(Biomes::Kind::River)] > 0 && seen[static_cast<int>(Biomes::Kind::FrozenRiver)] > 0, "rivers wind through it, frozen in the cold");
        bool riverChecked = false, riverWet = false;
        for (int z = -6000; z <= 6000 && !riverChecked; z += 61)
            for (int x = -6000; x <= 6000 && !riverChecked; x += 61)
            {
                if (generator.kindAt(x, z) != Biomes::Kind::River || generator.surfaceHeight(x, z) >= WorldGen::SEA_LEVEL) continue;
                Chunk chunk(ChunkPos{ Structures::squareOf(x, Chunk::SX), Structures::squareOf(z, Chunk::SZ) });
                generator.generate(chunk);
                const int lx = x - chunk.position().x * Chunk::SX, lz = z - chunk.position().z * Chunk::SZ;
                riverWet = chunk.getBlock(lx, WorldGen::SEA_LEVEL, lz) == Blocks::Water;
                riverChecked = true;
            }
        check(riverChecked && riverWet, "a river's bed is below sea level with water over it");
        int caves[3] = {};
        for (int z = -3000; z <= 3000; z += 97)
            for (int x = -3000; x <= 3000; x += 97)
            {
                const Biomes::Kind kind = generator.kindAt(x, 10, z);
                caves[0] += kind == Biomes::Kind::LushCaves;
                caves[1] += kind == Biomes::Kind::DripstoneCaves;
                caves[2] += kind == Biomes::Kind::DeepDark;
            }
        check(caves[0] > 0 && caves[1] > 0 && caves[2] > 0, "and lush caves, dripstone caves and the deep dark lie underneath");

        const WorldGen nether(4242u, Dimension::Nether);
        int netherSeen[Biomes::COUNT] = {};
        for (int z = -2000; z <= 2000; z += 53)
            for (int x = -2000; x <= 2000; x += 53) ++netherSeen[static_cast<int>(nether.netherKindAt(x, z))];
        check(netherSeen[static_cast<int>(Biomes::Kind::SoulSandValley)] > 0 && netherSeen[static_cast<int>(Biomes::Kind::BasaltDeltas)] > 0 &&
              netherSeen[static_cast<int>(Biomes::Kind::NetherWastes)] > 0 && netherSeen[static_cast<int>(Biomes::Kind::CrimsonForest)] > 0 &&
              netherSeen[static_cast<int>(Biomes::Kind::WarpedForest)] > 0, "the nether has all five of its biomes");
        auto netherSample = [&](Biomes::Kind wanted, BlockId a, BlockId b) {
            for (int z = -2000; z <= 2000; z += 53)
                for (int x = -2000; x <= 2000; x += 53)
                {
                    if (nether.netherKindAt(x, z) != wanted) continue;
                    Chunk chunk(ChunkPos{ Structures::squareOf(x, Chunk::SX), Structures::squareOf(z, Chunk::SZ) });
                    nether.generate(chunk);
                    int count = 0;
                    for (int y = 0; y < Chunk::SY; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                            {
                                const BlockId block = chunk.getBlock(lx, y, lz);
                                if (block == a || block == b) ++count;
                            }
                    return count;
                }
            return 0;
        };
        check(netherSample(Biomes::Kind::SoulSandValley, Blocks::SoulSand, Blocks::SoulSoil) > 200, "a soul sand valley is floored with soul sand and soil");
        check(netherSample(Biomes::Kind::BasaltDeltas, Blocks::Basalt, Blocks::Blackstone) > 200, "and the basalt deltas with basalt and blackstone");

        const WorldGen end(4242u, Dimension::End);
        int endSeen[Biomes::COUNT] = {};
        for (int z = -4000; z <= 4000; z += 101)
            for (int x = -4000; x <= 4000; x += 101) ++endSeen[static_cast<int>(end.endKindAt(x, z))];
        check(end.endKindAt(0, 0) == Biomes::Kind::TheEnd && endSeen[static_cast<int>(Biomes::Kind::EndHighlands)] > 0 &&
              endSeen[static_cast<int>(Biomes::Kind::EndMidlands)] > 0 && endSeen[static_cast<int>(Biomes::Kind::EndBarrens)] > 0 &&
              endSeen[static_cast<int>(Biomes::Kind::SmallEndIslands)] > 0, "the end is The End in the middle and four kinds of island beyond");

        WorldGen::Options single;
        single.type = WorldGen::Options::Type::SingleBiome;
        single.kind = Biomes::Kind::BirchForest;
        single.biome = Biomes::family(single.kind);
        const WorldGen birches(4242u, Dimension::Overworld, single);
        check(birches.kindAt(500, -300) == Biomes::Kind::BirchForest && birches.biomeAt(500, -300) == Biome::Forest,
              "a single-biome world is that biome everywhere");
    }

    void testBiomeColours()
    {
        section("biome colours: grass, leaves and water");
        const uint32_t swampGrass = BiomeColours::grass(Biomes::Kind::Swamp, 10, 20);
        check(swampGrass == 0x4C763Cu || swampGrass == 0x6A7039u, "swamp grass is one of 1.21.1's two swamp greens");
        check(BiomeColours::grass(Biomes::Kind::Badlands, 0, 0) == 9470285u && BiomeColours::foliage(Biomes::Kind::Badlands) == 10387789u,
              "badlands grass and leaves take the data's fixed colours");
        const uint32_t forest = BiomeColours::grass(Biomes::Kind::Forest, 0, 0);
        check(BiomeColours::grass(Biomes::Kind::DarkForest, 0, 0) == (((forest & 0xFEFEFEu) + 0x28340Au) >> 1),
              "dark forest grass is darkened the way 1.21.1 darkens it");
        check(BiomeColours::water(Biomes::Kind::Swamp) == 0x617B64u && BiomeColours::water(Biomes::Kind::WarmOcean) == 0x43D5EEu &&
              BiomeColours::water(Biomes::Kind::Plains) == 0x3F76E4u, "water takes each biome's water colour");
        check(BiomeColours::grass(Biomes::Kind::Plains, 0, 0) == BiomeColours::fromMap(false, 0.8f, 0.4f) &&
              BiomeColours::grass(Biomes::Kind::Desert, 0, 0) != BiomeColours::grass(Biomes::Kind::Jungle, 0, 0),
              "other grass comes from the colour map by temperature and rainfall");
        const uint32_t pair[2] = { 0x000000u, 0xFEFEFEu };
        check(BiomeColours::average(pair, 2) == 0x7F7F7Fu, "neighbouring columns are averaged");

        WorldGen::Options swampy;
        swampy.type = WorldGen::Options::Type::SingleBiome;
        swampy.kind = Biomes::Kind::Swamp;
        swampy.biome = Biome::Swamp;
        const WorldGen swampGen(77u, Dimension::Overworld, swampy);
        Chunk chunk(ChunkPos{ 3, -2 });
        swampGen.paintTints(chunk);
        bool allSwamp = true;
        for (uint32_t water : chunk.waterTint) allSwamp = allSwamp && water == 0x617B64u;
        check(allSwamp && chunk.foliageTint[0] == 0x6A7039u, "a chunk keeps the blended colours of its columns");

        MeshSnapshot snapshot;
        snapshot.pos = ChunkPos{ 0, 0 };
        std::fill(snapshot.grass.begin(), snapshot.grass.end(), BiomeColours::fromMap(false, 0.8f, 0.4f));
        std::fill(snapshot.foliage.begin(), snapshot.foliage.end(), 0x6A7039u);
        std::fill(snapshot.water.begin(), snapshot.water.end(), 0x617B64u);
        snapshot.blocks[static_cast<size_t>(MeshSnapshot::index(5, 60, 5))] = Blocks::Grass;
        snapshot.blocks[static_cast<size_t>(MeshSnapshot::index(8, 60, 8))] = Blocks::Water;
        snapshot.blocks[static_cast<size_t>(MeshSnapshot::index(11, 60, 11))] = Blocks::Stone;
        MeshResult mesh;
        buildChunkMesh(snapshot, mesh);
        check(mesh.opaqueVertices.size() % 10 == 0 && mesh.transparentVertices.size() % 10 == 0 && !mesh.transparentVertices.empty(),
              "every vertex carries its tint as a tenth number");
        auto unpack = [](float packed, int& r, int& g, int& b, bool& selective) {
            const uint32_t p = static_cast<uint32_t>(packed);
            r = static_cast<int>((p >> 14) & 127u);
            g = static_cast<int>((p >> 7) & 127u);
            b = static_cast<int>(p & 127u);
            selective = ((p >> 21) & 1u) != 0;
        };
        bool plainGrassTop = false, sideSelective = false, stoneNeutral = true;
        for (size_t i = 0; i < mesh.opaqueVertices.size(); i += 10)
        {
            const int tag = static_cast<int>(mesh.opaqueVertices[i + 8]);
            const int id = (tag >> 3) & 4095, face = tag & 7;
            int r = 0, g = 0, b = 0;
            bool selective = false;
            unpack(mesh.opaqueVertices[i + 9], r, g, b, selective);
            if (id == Blocks::Grass && face == 2) plainGrassTop = r == 64 && g == 64 && b == 64 && !selective;
            if (id == Blocks::Grass && face == 0) sideSelective = selective;
            if (id == Blocks::Stone) stoneNeutral = stoneNeutral && r == 64 && g == 64 && b == 64 && !selective;
        }
        check(plainGrassTop, "plains grass is left as the pack drew it");
        check(sideSelective, "a grass block's side is tinted only where the grass hangs over the dirt");
        check(stoneNeutral, "and stone is never tinted");
        int wr = 0, wg = 0, wb = 0;
        bool ws = false;
        unpack(mesh.transparentVertices[9], wr, wg, wb, ws);
        check(std::abs(wr - static_cast<int>(0x61 * 64 / 0x3F)) <= 1 && std::abs(wg - static_cast<int>(0x7B * 64 / 0x76)) <= 1 &&
              std::abs(wb - static_cast<int>(0x64 * 64 / 0xE4)) <= 1, "swamp water is turned from the default blue to the swamp's green");
    }

    void testWorldBorder()
    {
        section("world border: where it is and what it does");
        WorldBorder border;
        check(border.size() == 59999968.0 && border.east() == 29999984.0 && border.west() == -29999984.0 && border.isDefault(),
              "a new world's border is 59,999,968 blocks wide around 0, 0");
        check(border.color() == 0x20A0FFu && border.stage() == WorldBorder::Stage::Stationary, "and still, so drawn blue");
        border.setCenter(100.5, -20.5);
        border.setSize(10.0);
        check(border.west() == 95.5 && border.east() == 105.5 && border.north() == -25.5 && border.south() == -15.5, "its four sides are half its size from the centre");
        check(border.distanceInside(100.5, -20.5) == 5.0 && border.distanceInside(104.5, -20.5) == 1.0 && border.distanceInside(110.5, -20.5) == -5.0,
              "distance inside is to the nearest side, and negative outside");
        check(border.containsBlock(96, -25) && !border.containsBlock(95, -25) && border.containsBlock(105, -16) && !border.containsBlock(106, -16),
              "a block counts as inside by its corner, as in 1.21.1");
        check(border.damageFor(100.5, -20.5) == 0.0f && border.damageFor(110.0, -20.5) == 0.0f && border.damageFor(120.5, -20.5) == 2.0f &&
              border.damageFor(111.0, -20.5) == 1.0f, "no hurt within the five-block buffer, then 0.2 a block past it, at least 1");
        check(border.warning(100.5, -20.5) == 0.0f && std::abs(border.warning(104.5, -20.5) - 0.8f) < 1e-5f && border.warning(110.0, -20.5) == 1.0f,
              "the red warning starts five blocks in and is full at the wall");
        border.lerpTo(10.0, 4.0, 6.0);
        check(border.stage() == WorldBorder::Stage::Shrinking && border.color() == 0xFF3030u, "a shrinking border is red");
        border.update(3.0);
        check(std::abs(border.size() - 7.0) < 1e-9 && border.target() == 4.0 && std::abs(border.secondsLeft() - 3.0) < 1e-9, "and moves evenly over its time");
        border.update(5.0);
        check(border.size() == 4.0 && border.stage() == WorldBorder::Stage::Stationary, "then stops where it was going");
        border.lerpTo(4.0, 8.0, 2.0);
        check(border.color() == 0x40FF80u, "a growing border is green");
        border.damagePerBlock = 0.5;
        border.warningBlocks = 7;
        WorldBorder copy;
        check(copy.load(border.save()) && copy.size() == border.size() && copy.target() == 8.0 && copy.centerX() == 100.5 &&
              copy.damagePerBlock == 0.5 && copy.warningBlocks == 7 && copy.stage() == WorldBorder::Stage::Growing, "it is saved with the world, mid-move included");
        check(!copy.load("1,2,3") && !copy.load("a,b,c,d,e,f,g,h,i"), "and a damaged line is refused");
        check(border.blocks(100.5, -20.5, 0.6) == false && border.blocks(103.0, -20.5, 0.6), "it only walls in what is up against it");

        section("attributes: thirty-one of them, with modifiers and /attribute");
        {
            check(Attributes::COUNT == 31, "all thirty-one 1.21.1 attributes");
            Attributes::Kind found;
            check(Attributes::fromId("minecraft:generic.max_health", found) && found == Attributes::Kind::MaxHealth &&
                  Attributes::fromId("movement_speed", found) && found == Attributes::Kind::MovementSpeed, "found by id, with or without the prefix");
            Attributes::Set traits;
            check(traits.value(Attributes::Kind::MovementSpeed) == 0.1 && traits.value(Attributes::Kind::MaxHealth) == 20.0, "defaults are 1.21.1's");
            traits.addModifier(Attributes::Kind::MaxHealth, { "a", 10.0, Attributes::Operation::AddValue });
            traits.addModifier(Attributes::Kind::MaxHealth, { "b", 0.5, Attributes::Operation::AddMultipliedBase });
            traits.addModifier(Attributes::Kind::MaxHealth, { "c", 1.0, Attributes::Operation::AddMultipliedTotal });
            check(traits.value(Attributes::Kind::MaxHealth) == 90.0, "add, then multiply the base, then multiply the total");
            check(!traits.addModifier(Attributes::Kind::MaxHealth, { "a", 1.0, Attributes::Operation::AddValue }), "one id per modifier");
            Attributes::Set copy;
            copy.load(traits.save());
            check(copy.value(Attributes::Kind::MaxHealth) == 90.0, "and the set is saved and read back");
            traits.setBase(Attributes::Kind::MaxHealth, 5000.0);
            check(traits.base(Attributes::Kind::MaxHealth) == 1024.0, "bases are clamped to the attribute's range");
            Player tough(glm::vec3(0.0f));
            std::string told;
            std::vector<std::string> args = { "@s", "generic.max_health", "base", "set", "40" };
            check(Attributes::command(tough.attributes, args, "Alex", told) && tough.maxHealth() == 40, "/attribute base set raises max health");
            args = { "@s", "minecraft:generic.movement_speed", "modifier", "add", "fast", "0.5", "add_multiplied_base" };
            check(Attributes::command(tough.attributes, args, "Alex", told) && std::abs(tough.attributes.value(Attributes::Kind::MovementSpeed) - 0.15) < 1e-9,
                  "modifiers go on by command");
            args = { "@s", "generic.movement_speed", "modifier", "remove", "fast" };
            check(Attributes::command(tough.attributes, args, "Alex", told) && tough.attributes.value(Attributes::Kind::MovementSpeed) == 0.1, "and come off");
            check(Chat::parse("/attribute @s generic.luck get", true).kind == Chat::Kind::AttributeCommand, "/attribute parses");
            check(Armour::toughnessOf(Items::DiamondChestplate) == 2 && Armour::toughnessOf(Items::NetheriteBoots) == 3 &&
                  Armour::knockbackResistanceOf(Items::NetheriteHelmet) == 0.1f && Armour::toughnessOf(Items::IronHelmet) == 0,
                  "diamond and netherite carry toughness, netherite knockback resistance");
            check(Armour::reduce(30, 20, 12) < Armour::reduce(30, 20, 0) && Armour::reduce(2, 20, 0) == 1,
                  "toughness keeps armour working against big hits");
        }

        section("scoreboard: objectives, scores, teams, tags and triggers");
        {
            Scoreboard board;
            std::string said;
            auto run = [&](const std::string& line) {
                const Chat::Command command = Chat::parse(line, true);
                return command.kind == Chat::Kind::ScoreboardCommand && board.command(command.sub, command.args, "Alex", said);
            };
            check(run("/scoreboard objectives add kills totalKillCount \"Mob Kills\"") && board.objective("kills") &&
                  board.objective("kills")->displayName == "Mob Kills", "an objective is made with its criterion and display name");
            check(!run("/scoreboard objectives add kills dummy"), "and a second of the same name is refused");
            check(!run("/scoreboard objectives add bad nonsense"), "as is a criterion that does not exist");
            check(run("/scoreboard objectives add points dummy") && run("/scoreboard players set @s points 5") && board.score("points", "Alex") == 5,
                  "scores are set on yourself");
            check(run("/scoreboard players add #pool points 7") && run("/scoreboard players operation @s points += #pool points") &&
                  board.score("points", "Alex") == 12, "fake players hold numbers and operations combine them");
            check(run("/scoreboard players operation @s points %= #pool points") && board.score("points", "Alex") == 5, "modulo works like Minecraft's");
            board.addToCriterion("totalKillCount", "Alex", 3);
            check(board.score("kills", "Alex") == 3, "kill counts go up by themselves");
            check(run("/scoreboard objectives setdisplay sidebar points") && board.sidebar().size() == 1 && board.sidebar()[0].first == "Alex",
                  "the sidebar shows real holders and hides the #fake ones");
            check(run("/scoreboard objectives add hp health") && !run("/scoreboard players set @s hp 3"), "health is read-only");
            Scoreboard::Live live;
            live.health = 13;
            board.refreshLive("Alex", live);
            check(board.score("hp", "Alex") == 13, "and follows the player");
            check(run("/team add red") && run("/team modify red prefix [R] ") && run("/team join red @s") && board.teamOf("Alex") &&
                  board.teamOf("Alex")->prefix == "[R]", "teams take members and a prefix");
            check(run("/team leave @s") && !board.teamOf("Alex"), "and let them go");
            check(run("/tag @s add builder") && board.tagged("builder") && !run("/tag @s add builder") && run("/tag @s remove builder") &&
                  !board.tagged("builder"), "tags go on and come off");
            check(run("/scoreboard objectives add vote trigger") && !run("/trigger vote") && run("/scoreboard players enable @s vote") &&
                  run("/trigger vote add 4") && board.score("vote", "Alex") == 4 && !run("/trigger vote"),
                  "a trigger fires once each time it is enabled");
            Scoreboard copy;
            copy.load(board.save());
            check(copy.score("points", "Alex") == 5 && copy.score("points", "#pool") == 7 && copy.displayed("sidebar") &&
                  copy.objective("kills")->displayName == "Mob Kills", "everything is saved with the world");
            check(Chat::parse("/scoreboard objectives list", false).kind == Chat::Kind::Denied && Chat::parse("/trigger vote", false).kind == Chat::Kind::ScoreboardCommand,
                  "/scoreboard is a cheat but /trigger is for everyone");
        }

        section("chat: the rest of the vanilla commands");
        {
            for (const char* line : { "/advancement grant @s only story/root", "/bossbar list", "/item replace entity @s weapon.mainhand with stone",
                                      "/particle heart", "/playsound entity.player.levelup", "/stopsound", "/random value 1..6", "/recipe give @s *",
                                      "/ride @s dismount", "/spectate", "/spreadplayers 0 0 1 10 false @s", "/tick query", "/function demo:start",
                                      "/datapack list", "/reload", "/data get entity @s Health", "/loot give @s loot minecraft:entities/zombie",
                                      "/place feature oak", "/execute run say hi", "/forceload query", "/kick Steve", "/ban Steve", "/banlist",
                                      "/whitelist list", "/op Steve", "/save-all", "/stop", "/debug start" })
            {
                const Chat::Command more = Chat::parse(line, true);
                check(more.kind == Chat::Kind::MoreCommand, std::string(line) + " is a command");
            }
            check(Chat::parse("/random roll 1..6", false).kind == Chat::Kind::MoreCommand && Chat::parse("/random value 1..6", false).kind == Chat::Kind::Denied &&
                  Chat::parse("/tm hello", false).kind == Chat::Kind::MoreCommand && Chat::parse("/bossbar list", false).kind == Chat::Kind::Denied,
                  "rolling dice and team chat need no cheats; the rest do");
            const std::vector<std::string> names = Chat::suggest("/gam");
            check(std::find(names.begin(), names.end(), "gamemode") != names.end() && std::find(names.begin(), names.end(), "gamerule") != names.end() &&
                  std::find(names.begin(), names.end(), "give") == names.end(), "tab offers the commands that start with what was typed");
            const std::vector<std::string> modes = Chat::suggest("/gamemode cr");
            check(modes.size() == 1 && modes[0] == "creative", "and the arguments a command takes");
            const std::vector<std::string> swords = Chat::suggest("/give @s diamond_sw");
            check(std::find(swords.begin(), swords.end(), "diamond_sword") != swords.end(), "items for /give");
            const std::vector<std::string> groves = Chat::suggest("/locate biome cherry");
            check(groves.size() == 1 && groves[0] == "cherry_grove", "biomes for /locate biome");
            const std::vector<std::string> speeds = Chat::suggest("/effect give @s spe");
            check(std::find(speeds.begin(), speeds.end(), "speed") != speeds.end(), "effects for /effect");
            check(Chat::suggest("/weather @").size() == 5, "and target selectors wherever an @ starts a word");
            check(Chat::completeWith("/gamemode cr", "creative") == "/gamemode creative" && Chat::completeWith("/gam", "gamemode") == "/gamemode",
                  "a choice replaces the word being typed");
            const glm::vec3 here(10.0f, 64.0f, 20.0f);
            Chat::Command weather = Chat::parse("/weather thunder 60", true);
            check(weather.kind == Chat::Kind::Weather && weather.sub == "thunder" && weather.seconds == 60.0f, "/weather thunder takes a length");
            check(Chat::parse("/weather fog", true).kind == Chat::Kind::Unknown, "and refuses weather that does not exist");
            check(Chat::parse("/weather rain", false).kind == Chat::Kind::Denied, "and is a cheat");
            check(Chat::parse("/difficulty", false).kind == Chat::Kind::Difficulty && Chat::parse("/difficulty", false).mode == -1,
                  "/difficulty with nothing after it just says what it is");
            check(Chat::parse("/difficulty hard", true).mode == 3 && Chat::parse("/difficulty 0", true).mode == 0, "it takes names and numbers");
            Chat::Command xp = Chat::parse("/xp add @s 5L", true);
            check(xp.kind == Chat::Kind::Experience && xp.sub == "add" && xp.count == 5 && xp.mode == 1, "/xp 5L is five levels");
            Chat::Command points = Chat::parse("/experience set @p 30 points", true);
            check(points.sub == "set" && points.count == 30 && points.mode == 0, "/experience set takes points");
            check(Chat::parse("/xp query @s levels", true).sub == "query", "/xp query asks");
            Chat::Command clear = Chat::parse("/clear @s minecraft:dirt 3", true);
            check(clear.kind == Chat::Kind::Clear && clear.item == Blocks::Dirt && clear.count == 3, "/clear can take a kind and a count");
            check(Chat::parse("/clear", true).item == Blocks::Air && Chat::parse("/clear", true).count == -1, "and with nothing, takes everything");
            Chat::Command spawn = Chat::parse("/setworldspawn ~ ~1 ~", true, here);
            check(spawn.kind == Chat::Kind::SetWorldSpawn && spawn.position == glm::vec3(10.0f, 65.0f, 20.0f), "/setworldspawn reads ~");
            check(Chat::parse("/spawnpoint @s 1 2 3", true, here).position == glm::vec3(1.0f, 2.0f, 3.0f), "/spawnpoint skips the target");
            Chat::Command me = Chat::parse("/me waves hello", false);
            check(me.kind == Chat::Kind::Me && me.text == "waves hello", "/me is not a cheat");
            Chat::Command msg = Chat::parse("/tell Alex meet at spawn", false);
            check(msg.kind == Chat::Kind::Whisper && msg.sub == "Alex" && msg.text == "meet at spawn", "/tell whispers");
            check(Chat::parse("/list", false).kind == Chat::Kind::List, "/list lists");
            check(Chat::parse("/defaultgamemode creative", true).mode == static_cast<int>(GameMode::Creative), "/defaultgamemode");
            Chat::Command fill = Chat::parse("/fill ~ ~ ~ ~2 ~2 ~2 stone hollow", true, here);
            check(fill.kind == Chat::Kind::Fill && fill.item == Blocks::Stone && fill.sub == "hollow" &&
                  fill.position2 == glm::vec3(12.0f, 66.0f, 22.0f), "/fill reads two corners, a block and a mode");
            check(Chat::parse("/fill 0 0 0 1 1 1 air", true).item == Blocks::Air, "and can fill with air");
            check(Chat::parse("/fill 0 0 0 1 1 1 diamond", true).kind == Chat::Kind::Unknown, "but not with an item");
            Chat::Command clone = Chat::parse("/clone 0 60 0 3 63 3 10 70 10", true);
            check(clone.kind == Chat::Kind::Clone && clone.position3 == glm::vec3(10.0f, 70.0f, 10.0f), "/clone reads three corners");
            Chat::Command add = Chat::parse("/time add 1d", true);
            check(add.kind == Chat::Kind::TimeAdd && add.value == 24000.0, "/time add 1d is a day of ticks");
            check(Chat::parse("/time query daytime", true).kind == Chat::Kind::TimeQuery, "/time query");
            check(Chat::parse("/time set day", true).kind == Chat::Kind::TimeSet, "/time set still works");
            Chat::Command title = Chat::parse("/title @a title {\"text\":\"Hello\"}", true);
            check(title.kind == Chat::Kind::Title && title.sub == "title" && title.text == "Hello", "/title reads its text");
            check(Chat::parse("/title @s actionbar Watch out", true).text == "Watch out", "and plain words too");
            Chat::Command enchant = Chat::parse("/enchant @s minecraft:silk_touch", true);
            check(enchant.kind == Chat::Kind::Enchant && enchant.effect == static_cast<int>(Enchanting::Kind::SilkTouch) && enchant.amplifier == 1,
                  "/enchant finds silk touch");
            check(Chat::parse("/enchant @s sharpness 4", true).amplifier == 4, "with a level");
            check(Chat::parse("/enchant @s nonsense", true).kind == Chat::Kind::Unknown, "and refuses made-up ones");
            Chat::Command locate = Chat::parse("/locate structure minecraft:village_plains", true);
            check(locate.kind == Chat::Kind::Locate && locate.sub == "structure" && locate.text == "village_plains", "/locate structure");
            check(Chat::parse("/damage @s 4", true).count == 4, "/damage");
        }

        section("world border: /worldborder");
        WorldBorder live;
        std::string message;
        Chat::Command set = Chat::parse("/worldborder set 100", true);
        check(set.kind == Chat::Kind::WorldBorder && Chat::applyWorldBorder(set, live, message) && live.size() == 100.0 &&
              message == "Set the world border to 100.0 block(s) wide", "/worldborder set changes the size at once");
        check(!Chat::applyWorldBorder(set, live, message) && message == "Nothing changed. The world border is already that size", "and says when nothing changed");
        check(!Chat::applyWorldBorder(Chat::parse("/worldborder set 0.5", true), live, message) && message == "World border cannot be smaller than 1 block wide",
              "it cannot be under a block wide");
        check(Chat::applyWorldBorder(Chat::parse("/worldborder add -40 20", true), live, message) && live.target() == 60.0 &&
              message == "Shrinking the world border to 60.0 block(s) wide over 20 second(s)", "add with a time shrinks it over that time");
        check(Chat::applyWorldBorder(Chat::parse("/worldborder add 40 10", true), live, message) &&
              message.rfind("Growing the world border to ", 0) == 0, "or grows it");
        Chat::Command centre = Chat::parse("/worldborder center ~5 ~-3", true, glm::vec3(10.0f, 64.0f, 20.0f));
        check(Chat::applyWorldBorder(centre, live, message) && live.centerX() == 15.0 && live.centerZ() == 17.0 &&
              message == "Set the center of the world border to 15.00, 17.00", "center takes coordinates, ~ included");
        check(!Chat::applyWorldBorder(Chat::parse("/worldborder center 40000000 0", true), live, message) &&
              message == "World border cannot be further out than 29999984 blocks", "but not past the edge of the world");
        check(Chat::applyWorldBorder(Chat::parse("/worldborder damage amount 1", true), live, message) && live.damagePerBlock == 1.0 &&
              Chat::applyWorldBorder(Chat::parse("/worldborder damage buffer 2", true), live, message) && live.safeZone == 2.0 &&
              Chat::applyWorldBorder(Chat::parse("/worldborder warning distance 9", true), live, message) && live.warningBlocks == 9 &&
              Chat::applyWorldBorder(Chat::parse("/worldborder warning time 30", true), live, message) && live.warningTime == 30 &&
              message == "Set the world border warning time to 30 second(s)", "damage and warning settings are changed one by one");
        live.setSize(250.0);
        check(Chat::applyWorldBorder(Chat::parse("/worldborder get", true), live, message) && message == "The world border is currently 250 block(s) wide",
              "get reports the width");
        check(Chat::parse("/worldborder set 5", false).kind == Chat::Kind::Denied && Chat::parse("/worldborder", true).kind == Chat::Kind::Unknown &&
              Chat::parse("/worldborder set x", true).kind == Chat::Kind::Unknown && Chat::parse("/worldborder warning size 3", true).kind == Chat::Kind::Unknown,
              "it needs cheats and well-formed arguments");

        section("world border: drawn with the forcefield");
        WorldBorder near;
        near.setSize(20.0);
        const std::vector<float> wall = BorderRenderer::build(near, glm::dvec3(8.0, 70.0, 0.0), 128.0, 400.0, 0.0f);
        check(!wall.empty() && wall.size() % 30 == 0, "near the border its sides are built as quads");
        bool onSides = true;
        for (size_t i = 0; i < wall.size(); i += 5)
            onSides = onSides && (std::abs(wall[i]) == 10.0f || std::abs(wall[i + 2]) == 10.0f);
        check(onSides, "every vertex lies on one of the four sides");
        check(BorderRenderer::build(WorldBorder(), glm::dvec3(8.0, 70.0, 0.0), 128.0, 400.0, 0.0f).empty(), "far from it nothing is drawn");
        check(BorderRenderer::alphaFor(near, 0.0, 0.0, 128.0) < BorderRenderer::alphaFor(near, 9.0, 0.0, 128.0) &&
              BorderRenderer::alphaFor(near, 30.0, 0.0, 128.0) == 1.0f, "it fades in as you near it and is solid outside");

        section("effect/dither: the ordered dither");
        unsigned char painted[16];
        ShaderPipeline::paintDither(painted);
        const int BAYER[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
        bool bayer = true;
        for (int i = 0; i < 16; ++i) bayer = bayer && painted[i] == BAYER[i] * 17;
        check(bayer, "without a pack the 4x4 Bayer matrix is painted, every level from 0 to 15 once");
        int width = 0, height = 0, channels = 0;
        if (unsigned char* sheet = Packs::load("assets/textures/effect/dither.png", &width, &height, &channels, 4))
        {
            bool same = width == 4 && height == 4;
            for (int i = 0; same && i < 16; ++i) same = sheet[i * 4] == painted[i];
            stbi_image_free(sheet);
            check(same, "and it is the same matrix as the pack's effect/dither.png");
        }
    }

    void testRecipeBook()
    {
        section("recipe book: the crafting and furnace books");
        using RecipeBook::Book;
        using RecipeBook::Group;
        const std::vector<RecipeBook::Recipe>& crafting = RecipeBook::recipes(Book::Crafting);
        check(crafting.size() > 500, "the crafting book holds every recipe in the table and the families");
        int doors = 0;
        int doorFamily = -1;
        for (const RecipeBook::Recipe& recipe : crafting)
            if (recipe.result == doorBaseOf(1) || recipe.result == Blocks::DoorLowShutNorth)
            {
                ++doors;
                if (doorFamily < 0) doorFamily = recipe.family;
                else check(recipe.family == doorFamily, "wooden doors share one slot");
            }
        check(doors >= 2, "a door for each wood");

        RecipeBook::Stock stock;
        stock[Blocks::Planks] = 12;
        const RecipeBook::Recipe* table = nullptr;
        for (const RecipeBook::Recipe& recipe : crafting)
            if (recipe.result == Blocks::CraftingTable) table = &recipe;
        check(table && RecipeBook::timesCraftable(*table, stock, 64) == 3, "twelve planks make three crafting tables");
        RecipeBook::Stock mixed;
        mixed[Blocks::Planks] = 3;
        mixed[Blocks::SprucePlanks] = 2;
        std::vector<StackId> picks;
        check(table && RecipeBook::allocate(*table, mixed, 1, picks), "and any mix of planks makes one");
        check(table && RecipeBook::fits(*table, 2) && RecipeBook::cellIndex(*table, 3, 2) == 4, "it fits the little grid, laid out from the corner");

        bool furnaceFits = true;
        for (const RecipeBook::Recipe& recipe : crafting)
            if (recipe.result == Blocks::Furnace) furnaceFits = RecipeBook::fits(recipe, 2);
        check(!furnaceFits, "a furnace needs the big grid");

        check(RecipeBook::craftingGroup(Items::IronAxe) == Group::Equipment && RecipeBook::craftingGroup(Blocks::PistonNorth) == Group::Redstone &&
                  RecipeBook::craftingGroup(Blocks::StoneBricks) == Group::Building && RecipeBook::craftingGroup(Items::Bread) == Group::Misc &&
                  RecipeBook::craftingGroup(doorBaseOf(2)) == Group::Redstone,
              "each recipe sits on the tab 1.21.1 puts it on");
        check(RecipeBook::tabs(Book::Crafting).size() == 5 && RecipeBook::tabs(Book::Furnace).size() == 4 &&
                  RecipeBook::tabs(Book::BlastFurnace).size() == 3 && RecipeBook::tabs(Book::Smoker).size() == 2,
              "with the tabs each book has");

        bool iron = false, beef = false, glass = false, smokerOnlyFood = true, blastNoFood = true;
        for (const RecipeBook::Recipe& recipe : RecipeBook::recipes(Book::Furnace))
        {
            if (recipe.result == Items::IronIngot && recipe.group == Group::Misc) iron = true;
            if (recipe.result == Items::Steak && recipe.group == Group::Food) beef = true;
            if (recipe.result == Blocks::Glass && recipe.group == Group::Blocks) glass = true;
        }
        for (const RecipeBook::Recipe& recipe : RecipeBook::recipes(Book::Smoker)) smokerOnlyFood = smokerOnlyFood && recipe.group == Group::Food;
        for (const RecipeBook::Recipe& recipe : RecipeBook::recipes(Book::BlastFurnace)) blastNoFood = blastNoFood && recipe.group != Group::Food;
        check(iron && beef && glass, "the furnace book sorts into food, blocks and the rest");
        check(smokerOnlyFood && blastNoFood, "a smoker's book is all food and a blast furnace's none");

        RecipeBook::Stock some;
        some[Blocks::Log] = 1;
        const std::vector<RecipeBook::Slot> craftable = RecipeBook::slots(Book::Crafting, Group::Search, 2, "", true, some);
        bool onlyPlanks = !craftable.empty();
        for (const RecipeBook::Slot& slot : craftable)
            for (int index : slot.recipes) onlyPlanks = onlyPlanks && isPlanks(asBlock(crafting[static_cast<size_t>(index)].result));
        check(onlyPlanks, "with one log, the craftable filter shows only planks");
        const std::vector<RecipeBook::Slot> searched = RecipeBook::slots(Book::Crafting, Group::Search, 3, "crafting table", false, some);
        check(searched.size() == 1, "and the search finds a recipe by name");
        check(RecipeBook::mojangGroup("bow") == 'E' && RecipeBook::mojangGroup("oak_planks") == 'B' && RecipeBook::mojangGroup("nothing") == 0,
              "the categories are 1.21.1's own");
    }

    void testNewEffects()
    {
        section("status effects: the rest of 1.21.1's, the brews and the sources");
        using Effects::Kind;

        bool named = true, roundTrip = true, icons = true;
        for (int i = 0; i < Effects::KIND_COUNT; ++i)
        {
            const Kind kind = static_cast<Kind>(i);
            Kind back = Kind::Speed;
            named = named && Effects::name(kind)[0] != '\0';
            roundTrip = roundTrip && Effects::fromId(Effects::id(kind), back) && back == kind;
            icons = icons && std::string(Effects::icon(kind)) == std::string("../mob_effect/") + Effects::id(kind);
        }
        check(Effects::KIND_COUNT == 39, "thirty-nine effects in all, as in 1.21.1");
        check(named && roundTrip, "each has a name and comes back from its id");
        check(icons, "and its icon is the pack's mob_effect picture of the same name");
        Kind found = Kind::Speed;
        check(Effects::fromId("minecraft:unluck", found) && found == Kind::BadLuck && !Effects::fromId("flying", found),
              "ids read with or without the namespace, and nonsense is refused");
        check(Effects::colour(Kind::Wither) == 0x736156 && Effects::colour(Kind::Darkness) == 0x292721 &&
                  Effects::colour(Kind::TrialOmen) == 0x16A6A6,
              "colours are Mojang's");
        check(Effects::isNeutral(Kind::Glowing) && Effects::isNeutral(Kind::RaidOmen) && Effects::isBeneficial(Kind::SlowFalling) &&
                  Effects::isHarmful(Kind::Levitation) && !Effects::isBeneficial(Kind::Infested),
              "and each sits on the side Minecraft puts it");

        {
            Effects::Set set;
            Effects::give(set, Kind::InstantHealth, 0, Effects::INSTANT_SECONDS);
            Effects::give(set, Kind::InstantHealth, 0, Effects::INSTANT_SECONDS);
            Effects::Tick tick = Effects::step(set, 0.05f);
            check(tick.instantHeal == 8 && !set.has(Kind::InstantHealth), "two healing splashes in one moment heal twice, then are gone");
            Effects::give(set, Kind::InstantDamage, 1, Effects::INSTANT_SECONDS);
            tick = Effects::step(set, 0.05f);
            check(tick.instantHarm == 12, "harming II does twelve");
            check(Effects::instantAmount(Kind::InstantHealth, 0, 0.5f) == 2, "and half a splash heals half");
        }
        {
            Effects::Set set;
            Effects::give(set, Kind::Wither, 0, 10.0f);
            int pulses = 0;
            for (int i = 0; i < 100; ++i) pulses += Effects::step(set, 0.05f).witherPulses;
            check(pulses == 2, "wither bites once every two seconds");
            Effects::give(set, Kind::Wither, 1, 10.0f);
            pulses = 0;
            for (int i = 0; i < 100; ++i) pulses += Effects::step(set, 0.05f).witherPulses;
            check(pulses == 5, "and twice as often a level up");
        }
        {
            Effects::Set set;
            Effects::give(set, Kind::MiningFatigue, 2, 300.0f);
            check(std::fabs(Effects::digMultiplier(set) - 0.0027f) < 0.0001f, "mining fatigue III leaves digging at a crawl");
            Effects::give(set, Kind::Levitation, 1, 10.0f);
            check(Effects::levitationSpeed(set) == 2.0f, "levitation II lifts two blocks a second");
            Effects::give(set, Kind::Luck, 0, 300.0f);
            Effects::give(set, Kind::BadLuck, 1, 300.0f);
            check(Effects::luck(set) == -1, "luck and bad luck cancel");
            check(Effects::detectionScale(set, 4) == 1.0f, "a visible player is seen as far as ever");
            Effects::give(set, Kind::Invisibility, 0, 180.0f);
            check(std::fabs(Effects::detectionScale(set, 0) - 0.07f) < 0.001f && std::fabs(Effects::detectionScale(set, 4) - 0.7f) < 0.001f,
                  "an invisible one is seen at a fourteenth of the range, more for armour worn");
            check(Effects::cobwebDrag(set) == 0.25f, "a cobweb holds you at a quarter");
            Effects::give(set, Kind::Weaving, 0, 180.0f);
            check(Effects::cobwebDrag(set) == 0.5f, "and at half while weaving");
            check(std::fabs(Effects::darknessPulse(1.0f, 0.0f) - 0.45f) < 0.001f && Effects::darknessPulse(0.0f, 1.0f) == 0.0f,
                  "darkness throbs as deep as Minecraft's and only while it is on");
        }
        {
            Player p(glm::vec3(0.0f, 70.0f, 0.0f));
            p.health = 10;
            p.takeEffect(Kind::InstantHealth, 0, Effects::INSTANT_SECONDS);
            check(p.health == 14, "a potion of healing puts back two hearts");
            Effects::give(p.effects, Kind::HealthBoost, 0, 5.0f);
            check(p.maxHealth() == 24, "health boost adds two hearts");
            p.heal(40);
            check(p.health == 24, "which can be filled");
            for (int i = 0; i < 120; ++i) p.updateEffects(0.05f);
            check(p.maxHealth() == 20 && p.health == 20, "and are taken away again when it ends");
            Effects::give(p.effects, Kind::Wither, 0, 10.0f);
            for (int i = 0; i < 45; ++i) p.updateEffects(0.05f);
            check(p.health < 20, "wither eats into health");
            Player q(glm::vec3(0.0f, 70.0f, 0.0f));
            q.health = 1;
            Effects::give(q.effects, Kind::Wither, 0, 10.0f);
            for (int i = 0; i < 45; ++i) q.updateEffects(0.05f);
            check(q.health == 0, "and unlike poison it can kill");
        }

        using Brewing::Potion;
        using Brewing::Brew;
        Brew out;
        check(Brewing::brew(Potion::Awkward, Items::GlisteringMelon) == Potion::Healing &&
                  Brewing::brew(Potion::Awkward, Items::PhantomMembrane) == Potion::SlowFalling &&
                  Brewing::brew(Potion::Awkward, Items::BreezeRod) == Potion::WindCharged &&
                  Brewing::brew(Potion::Awkward, Blocks::SlimeBlock) == Potion::Oozing &&
                  Brewing::brew(Potion::Awkward, Blocks::Stone) == Potion::Infested &&
                  Brewing::brew(Potion::Awkward, Blocks::Cobweb) == Potion::Weaving,
              "the awkward base takes 1.21.1's six new ingredients");
        check(Brewing::brew(Potion::Water, Items::BreezeRod) == Potion::Mundane &&
                  Brewing::brew(Potion::Water, Items::PhantomMembrane) == Potion::Count,
              "water makes mundane of the 1.21 four but nothing of a membrane");
        check(Brewing::brewWith(Brew{ Potion::NightVision, true, false }, Items::FermentedSpiderEye, out) &&
                  out.potion == Potion::Invisibility && out.extended,
              "a spider eye spoils long night vision into long invisibility");
        check(Brewing::brewWith(Brew{ Potion::Healing, false, true }, Items::FermentedSpiderEye, out) &&
                  out.potion == Potion::Harming && out.strong,
              "and healing II into harming II");
        check(Brewing::brewWith(Brew{ Potion::Poison, true, false }, Items::FermentedSpiderEye, out) &&
                  out.potion == Potion::Harming && !out.extended,
              "and poison into harming");
        check(Brewing::canExtend(Potion::SlowFalling) && Brewing::canStrengthen(Potion::Harming) &&
                  !Brewing::canExtend(Potion::Oozing) && !Brewing::canStrengthen(Potion::Luck),
              "redstone and glowstone take where they do in 1.21.1");
        check(std::string(Brewing::name(Potion::Infested)) == "Potion of Infestation" &&
                  Brewing::fullName(Brew{ Potion::WindCharged }, Brewing::Form::Splash) == "Splash Potion of Wind Charging",
              "and the brews carry Minecraft's names");
        const int tipped = Brewing::tippedDamage(Kind::InstantHealth, 1);
        check(Brewing::tippedKind(tipped) == Kind::InstantHealth && Brewing::tippedAmplifier(tipped) == 1 &&
                  Brewing::tippedName(tipped) == "Arrow of Healing",
              "a tipped arrow keeps its level and is named for the brew");
        check(std::fabs(Brewing::tippedSeconds(Brewing::tippedDamage(Kind::Slowness, 0)) - 11.25f) < 0.01f,
              "and lasts an eighth of the drink");
        check(Food::stewEffect(Food::stewFlower(Blocks::WitherRose)).kind == Kind::Wither,
              "a wither rose in a stew withers");

        check(isUndead(MobId::Zombie) && isUndead(MobId::Phantom) && !isUndead(MobId::Cow) && !isUndead(MobId::Creeper),
              "the undead are the ones healing hurts");
        {
            Mob skeleton(MobId::WitherSkeleton, glm::vec3(0.0f, 64.0f, 0.0f), 5u);
            skeleton.giveEffect(Kind::Wither, 0, 10.0f);
            Mob pig(MobId::Pig, glm::vec3(0.0f, 64.0f, 0.0f), 6u);
            pig.giveEffect(Kind::Invisibility, 0, 30.0f);
            pig.giveEffect(Kind::Oozing, 0, 30.0f);
            check(!skeleton.effects().has(Kind::Wither), "a wither skeleton shrugs off wither");
            check(pig.invisible() && pig.effects().has(Kind::Oozing), "and anything else carries what it is given");
        }

        check(Raids::waveCount(1) == 3 && Raids::waveCount(2) == 5 && Raids::waveCount(3) == 7, "a raid has three, five or seven waves");
        bool firstWave = true;
        for (uint32_t seed = 0; seed < 64; ++seed)
        {
            const std::vector<MobId> party = Raids::party(1, 5, 2, false, seed);
            const int pillagers = static_cast<int>(std::count(party.begin(), party.end(), MobId::Pillager));
            firstWave = firstWave && pillagers >= 4 && pillagers <= 5 &&
                        std::count(party.begin(), party.end(), MobId::Ravager) == 0 &&
                        std::count(party.begin(), party.end(), MobId::Evoker) == 0;
        }
        check(firstWave, "the first wave is four or five pillagers and a vindicator at most");
        const std::vector<MobId> third = Raids::party(3, 5, 2, false, 7u);
        check(std::count(third.begin(), third.end(), MobId::Ravager) == 1, "the third brings a ravager");
        const std::vector<MobId> bonus = Raids::party(6, 5, 2, true, 7u);
        check(std::count(bonus.begin(), bonus.end(), MobId::Evoker) == 1, "and a bonus wave is the last wave again");
        check(Raids::heroPrice(10, 0) == 7 && Raids::heroPrice(1, 0) == 1 && Raids::heroPrice(32, 4) == 15,
              "a hero pays less, never nothing");
        Raids::Raid raid = Raids::begin(glm::vec3(0.0f), 2, 2);
        check(raid.groups() == 6, "an omen above the first adds a wave");
        Raids::addOmen(raid, 9);
        check(raid.omenLevel == Raids::MAX_OMEN, "and the omen tops out at five");
        check(Raids::barName(raid, 2) == "Raid - Raiders Remaining: 2" && Raids::barName(raid, 5) == "Raid", "the bar counts down the last two");
        raid.status = Raids::Status::Victory;
        check(Raids::barName(raid, 0) == "Raid - Victory", "and says how it went");

        const Chat::Command give = Chat::parse("/effect give @s minecraft:levitation 12 1", true);
        check(give.kind == Chat::Kind::EffectGive && give.effect == static_cast<int>(Kind::Levitation) && give.amplifier == 1 &&
                  give.seconds == 12.0f,
              "/effect gives any of them by name");
        check(Chat::parse("/effect clear", true).kind == Chat::Kind::EffectClear && !Chat::succeeded(Chat::parse("/effect give @s floating", true)),
              "clears them, and refuses ones that do not exist");

        int plain = 0, lucky = 0;
        for (uint32_t seed = 0; seed < 4000; ++seed)
        {
            if (Fishing::roll(seed, true, false).category == Fishing::Category::Treasure) ++plain;
            if (Fishing::roll(seed, true, false, 3).category == Fishing::Category::Treasure) ++lucky;
        }
        check(lucky > plain, "luck brings up more treasure");
    }

    void testTrimPalettes()
    {
        section("armour trims: Mojang's palettes, darker on their own metal");
        std::vector<uint8_t> key(8 * 4, 255), palette(8 * 4, 255);
        for (int k = 0; k < 8; ++k)
        {
            key[static_cast<size_t>(k * 4)] = key[static_cast<size_t>(k * 4 + 1)] = key[static_cast<size_t>(k * 4 + 2)] = static_cast<uint8_t>(224 - k * 32);
            palette[static_cast<size_t>(k * 4)] = static_cast<uint8_t>(10 + k);
            palette[static_cast<size_t>(k * 4 + 1)] = 100;
            palette[static_cast<size_t>(k * 4 + 2)] = 200;
        }
        std::vector<uint8_t> trim = { 224, 224, 224, 255, 0, 0, 0, 255, 130, 126, 129, 255, 50, 50, 50, 0 };
        check(ArmourTextures::swap(trim, key, palette) && trim[0] == 10 && trim[4] == 17 && trim[8] == 13 && trim[12] == 50,
              "each grey of a trim takes the matching colour of its material, and clear pixels stay clear");
        check(Smithing::darkerOn(2, Items::GoldChestplate) && Smithing::darkerOn(0, Items::IronBoots) &&
                  Smithing::darkerOn(5, Items::DiamondHelmet) && Smithing::darkerOn(6, Items::NetheriteLeggings),
              "gold on gold, iron on iron, diamond on diamond and netherite on netherite take the darker palette");
        check(!Smithing::darkerOn(2, Items::IronChestplate) && !Smithing::darkerOn(4, Items::DiamondBoots),
              "and nothing else does");
        check(Smithing::trimPath(10, true) == "assets/textures/trims/models/armor/silence_leggings.png" &&
                  Smithing::palettePath(8, false) == "assets/textures/trims/color_palettes/amethyst.png" &&
                  Smithing::palettePath(6, true) == "assets/textures/trims/color_palettes/netherite_darker.png" &&
                  Smithing::palettePath(-1, false) == "assets/textures/trims/color_palettes/trim_palette.png" &&
                  Smithing::trimIconPath(3) == "assets/textures/trims/items/boots_trim.png",
              "the trims, their palettes and the icon overlays come from the pack's own files");
    }

    void testMapMarks()
    {
        section("map decorations, explorer maps and maps on walls");
        bool named = true, present = true;
        const bool packHere = std::filesystem::exists("assets/textures/map/decorations");
        for (int mark = 0; mark < Maps::MarkCount; ++mark)
        {
            const std::string path = Maps::markPath(mark);
            if (path.rfind("../map/decorations/", 0) != 0) named = false;
            if (packHere && !std::filesystem::exists("assets/textures/map/decorations/" + std::string(Maps::markTexture(mark)) + ".png"))
                present = false;
        }
        check(Maps::MarkCount == 35 && named && present, "thirty-five marks, each with its picture in the pack");

        Maps::Data map;
        map.centreX = 64;
        map.centreZ = 64;
        const Maps::Placed middle = Maps::place(map, 64.0f, 64.0f, Maps::RedX, 8);
        check(middle.shown && std::fabs(middle.x - 64.0f) < 0.01f && std::fabs(middle.z - 64.0f) < 0.01f, "a mark lands where it is in the world");
        check(Maps::rotationFor(90.0f) == 0 && Maps::rotationFor(180.0f) == 4 && Maps::rotationFor(0.0f) == 12,
              "the player's arrow turns in sixteenths, nought facing south as in Minecraft");
        const Maps::Placed near = Maps::placePlayer(map, 300.0f, 64.0f, 90.0f, true);
        check(near.shown && near.mark == Maps::PlayerOffMap && near.x == 128.0f, "off the edge it waits at the border");
        const Maps::Placed far = Maps::placePlayer(map, 5000.0f, 64.0f, 90.0f, true);
        check(!far.shown, "and far off an ordinary map loses you");
        map.explorer = Maps::OceanExplorer;
        const Maps::Placed lost = Maps::placePlayer(map, 5000.0f, 64.0f, 90.0f, true);
        check(lost.shown && lost.mark == Maps::PlayerOffLimits, "where an explorer map keeps you as a dot");
        check(!Maps::placePlayer(map, 64.0f, 64.0f, 90.0f, false).shown, "and neither shows you from another dimension");

        check(Maps::toggleBanner(map, 70, 70, 14) && map.decorations.size() == 1 && map.decorations[0].mark == Maps::BannerFirst + 14,
              "using a map on a banner marks it in the banner's colour");
        check(!Maps::toggleBanner(map, 70, 70, 14) && map.decorations.empty(), "and using it again takes the mark off");
        check(!Maps::toggleBanner(map, 900, 70, 2) && map.decorations.empty(), "a banner off the map cannot be marked");

        map.decorations.push_back(Maps::Decoration{ Maps::Monument, 100, 30, 8 });
        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_mapmark_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        check(Maps::save(dir, 3, map), "a map with marks saves");
        Maps::Data back;
        check(Maps::load(dir, 3, back) && back.explorer == Maps::OceanExplorer && back.decorations.size() == 1 &&
                  back.decorations[0].mark == Maps::Monument && back.decorations[0].worldX == 100,
              "and keeps them");
        std::filesystem::remove_all(dir, ec);

        int explorers = 0;
        bool compass = true;
        for (const Trading::Deal& deal : Trading::dealsFor(Trading::Profession::Cartographer))
            if (deal.offer.give == Items::FilledMap)
            {
                ++explorers;
                if (deal.offer.also != Items::Compass || (deal.offer.giveEnchantments >> 24) == 0u) compass = false;
            }
        check(explorers == 3 && compass, "a cartographer sells the ocean, woodland and trial explorer maps for emeralds and a compass");
        check(std::string(Maps::explorerName(Maps::TrialExplorer)) == "Trial Explorer Map" && Maps::explorerMark(Maps::WoodlandExplorer) == Maps::Mansion,
              "each named for what it finds and marking it");
    }

    void testSignKinds()
    {
        section("signs: standing, hanging, on a bracket, and drawn as Mojang's models");
        bool rounds = true;
        for (int wood = 0; wood < WOOD_COUNT; ++wood)
            for (int r = 0; r < 16; ++r)
            {
                const BlockId standing = standingSignOf(wood, r);
                const BlockId hanging = hangingSignOf(wood, r);
                if (!isSign(standing) || !isStandingSign(standing) || signWood(standing) != wood || signRotation(standing) != r) rounds = false;
                if (!isSign(hanging) || !isCeilingHangingSign(hanging) || signWood(hanging) != wood || signRotation(hanging) != r) rounds = false;
                if (r < 4)
                {
                    const BlockId bracket = wallHangingSignOf(wood, r);
                    if (!isWallHangingSign(bracket) || signWood(bracket) != wood || signRotation(bracket) != r * 4 || signFacing(bracket) != r)
                        rounds = false;
                    const BlockId wall = signFacingAs(signBaseOf(wood), r);
                    if (!isWallSign(wall) || signRotation(wall) != r * 4) rounds = false;
                }
            }
        check(rounds, "every wood stands sixteen ways, hangs sixteen ways and on a bracket four ways");
        check(blockDrop(standingSignOf(3, 5)) == signItemOfWood(3) && blockDrop(hangingSignOf(9, 2)) == hangingSignItemOfWood(9) &&
                  blockDrop(wallHangingSignOf(10, 1)) == hangingSignItemOfWood(10),
              "a standing sign breaks into a sign and a hanging one into a hanging sign");
        check(itemInfo(hangingSignItemOfWood(5)).name == std::string("Dark Oak Hanging Sign") && woodOfHangingSignItem(hangingSignItemOfWood(8)) == 8,
              "eleven hanging sign items, one a wood");

        ItemStack grid[9];
        for (ItemStack& cell : grid) cell.clear();
        grid[0] = grid[2] = ItemStack{ static_cast<StackId>(Blocks::ChainFirst), 1 };
        for (int k = 3; k < 9; ++k) grid[k] = ItemStack{ static_cast<StackId>(Blocks::StrippedCherryLog), 1 };
        const CraftOutput made = Crafting::match(grid, 3);
        check(made.id == hangingSignItemOfWood(6) && made.count == 6, "two chains over six stripped logs make six hanging signs");

        check(SignRenderer::boxesFor(standingSignOf(0, 0), false).size() == 2 && SignRenderer::boxesFor(signFacingAs(signBaseOf(0), 0), false).size() == 1,
              "a standing sign is a board on a post and a wall sign is the board alone");
        check(SignRenderer::boxesFor(hangingSignOf(0, 0), false).size() == 5 && SignRenderer::boxesFor(hangingSignOf(0, 0), true).size() == 2 &&
                  SignRenderer::boxesFor(wallHangingSignOf(0, 0), false).size() == 6,
              "a hanging sign hangs on two pairs of chains, or a V when it hangs from something thin, and a bracket sign has its bar");
        const glm::vec3 south = SignRenderer::facingVector(standingSignOf(0, 0));
        const glm::vec3 west = SignRenderer::facingVector(standingSignOf(0, 4));
        check(south.z > 0.99f && west.x < -0.99f, "rotation nought faces south and four faces west, as in Minecraft");
        check(SignRenderer::textAnchor(glm::ivec3(0), standingSignOf(0, 0)).y > 0.8f &&
                  SignRenderer::textAnchor(glm::ivec3(0), hangingSignOf(0, 0)).y < 0.4f,
              "the words sit on the board, high on a post and low under chains");

        MeshSnapshot snapshot;
        snapshot.pos = ChunkPos{ 2, -1 };
        snapshot.blocks[static_cast<size_t>(MeshSnapshot::index(3, 70, 5))] = hangingSignOf(2, 7);
        snapshot.blocks[static_cast<size_t>(MeshSnapshot::index(4, 70, 5))] = Blocks::Stone;
        MeshResult result;
        buildChunkMesh(snapshot, result);
        check(result.signs.size() == 1 && result.signs[0] == glm::ivec3(2 * Chunk::SX + 3, 70, -1 * Chunk::SZ + 5),
              "the mesher leaves signs out of the chunk and says where they are");
    }

    void testShieldBanners()
    {
        section("banner patterns on shields");
        Banners::Library library;
        Banners::Library* before = Banners::active();
        Banners::setActive(&library);
        Banners::Design design;
        design.push_back(Banners::Layer{ Banners::Cross, 0 });
        const int designId = library.idOf(design);
        ItemStack grid[9];
        for (ItemStack& cell : grid) cell.clear();
        grid[0] = ItemStack{ Items::Shield, 1, 40, Enchanting::withLevel(0, Enchanting::Kind::Unbreaking, 2) };
        grid[4] = ItemStack{ static_cast<StackId>(Blocks::BannerFirst + 14 * 8), 1, designId };
        const CraftOutput shield = Crafting::match(grid, 3);
        check(shield.id == Items::Shield && shield.damage == 40 &&
                  Enchanting::levelOf(shield.enchantments, Enchanting::Kind::Unbreaking) == 2 && (shield.enchantments >> 24) != 0u,
              "a shield and a banner make the shield with that banner on it, wear and enchantments kept");
        const Banners::Design look = library.design(static_cast<int>(shield.enchantments >> 24));
        check(look.size() == 2 && look[0].pattern == Banners::Base && look[0].colour == 14 && look[1].pattern == Banners::Cross,
              "it carries the banner's colour and every layer");
        check(!Enchanting::glints(Items::Shield, shield.enchantments & 0xFF000000u), "and does not shine for it");
        grid[0] = ItemStack{ Items::Shield, 1, 40, shield.enchantments };
        check(!Crafting::match(grid, 3).valid(), "a shield that already has one takes no second");
        check(Banners::shieldTexture(Banners::Creeper) == "assets/textures/entity/shield/creeper.png" &&
                  Banners::bannerTexture(Banners::Creeper) == "assets/textures/entity/banner/creeper.png",
              "shields and banners draw their patterns from the pack's own sheets");
        Banners::setActive(before);
    }

    void testArmorStands()
    {
        section("armour stands: dressing, two hits, poses and the model");
        using namespace ArmorStands;
        check(preferredSlot(Items::DiamondHelmet) == Head && preferredSlot(Items::IronChestplate) == Chest &&
                  preferredSlot(Items::GoldLeggings) == Legs && preferredSlot(Items::LeatherBoots) == Feet,
              "armour goes where it is worn");
        check(preferredSlot(Items::Elytra) == Chest && preferredSlot(static_cast<StackId>(Blocks::CarvedPumpkinFirst)) == Head &&
                  preferredSlot(Items::DiamondSword) == MainHand,
              "an elytra on the back, a pumpkin on the head, anything else in the hand");
        check(placedYaw(0.0f) == 180.0f && placedYaw(100.0f) == 270.0f && placedYaw(-30.0f) == 135.0f,
              "it is set down facing whoever placed it, to the nearest eighth of a turn");

        Stand stand;
        Outcome put = use(stand, ItemStack{ Items::IronHelmet, 1 }, 1.0f, false);
        check(put.changed && put.hand.empty() && stand.gear[Head].id == Items::IronHelmet, "a helmet goes on its head");
        put = use(stand, ItemStack{ Items::DiamondChestplate, 3 }, 1.0f, false);
        check(put.changed && put.hand.count == 2 && stand.gear[Chest].id == Items::DiamondChestplate, "one from a stack goes on");
        put = use(stand, ItemStack{ Items::IronChestplate, 2 }, 1.0f, false);
        check(!put.changed && stand.gear[Chest].id == Items::DiamondChestplate, "but a stack will not swap for what is worn");
        put = use(stand, ItemStack{ Items::IronChestplate, 1 }, 1.0f, false);
        check(put.changed && put.hand.id == Items::DiamondChestplate && stand.gear[Chest].id == Items::IronChestplate,
              "a single one swaps for it");
        check(slotAtHeight(1.8f, stand) == Head && slotAtHeight(1.2f, stand) == Chest && slotAtHeight(0.3f, stand) == MainHand,
              "an empty hand takes back whatever is where it reaches");
        put = use(stand, ItemStack{}, 1.8f, false);
        check(put.changed && put.hand.id == Items::IronHelmet && stand.gear[Head].empty(), "and the helmet comes off");
        put = use(stand, ItemStack{ Items::GoldHelmet, 5 }, 1.0f, true);
        check(put.changed && put.hand.count == 5 && stand.gear[Head].id == Items::GoldHelmet, "in creative it is put on without being spent");

        check(!hit(stand, false), "the first blow only rocks it");
        check(hit(stand, false), "a second straight after breaks it");
        stand.sinceHit = 1.0f;
        check(!hit(stand, false), "but not once it has settled");
        check(hit(stand, true), "and a creative player breaks it at once");

        float distance = 0.0f, height = 0.0f;
        stand.feet = glm::vec3(0.5f, 64.0f, 0.5f);
        check(rayHits(stand, glm::vec3(0.5f, 65.6f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f), 5.0f, distance, height) &&
                  std::fabs(height - 1.6f) < 0.01f && std::fabs(distance - 3.25f) < 0.01f,
              "a click finds it and says how high it landed");

        check(poses().size() >= 12 && std::string(poses()[0].name) == "Default" &&
                  poses()[0].angles.leftArm == glm::vec3(-10.0f, 0.0f, -10.0f) && poses()[0].angles.rightArm == glm::vec3(-15.0f, 0.0f, 10.0f),
              "it stands in Minecraft's default pose and has a dozen to cycle through");

        Stand plain;
        const std::vector<BoxMesh::Box> boxes = ArmorStandRenderer::standBoxes(plain);
        float top = -1e9f, bottom = 1e9f;
        for (const BoxMesh::Box& box : boxes)
        {
            top = std::max(top, box.max.y);
            bottom = std::min(bottom, box.min.y);
        }
        check(boxes.size() == 10 && bottom == 0.0f && top == 30.0f, "its ten pieces run from the base plate to the top of the head");
        check(ArmorStandRenderer::armourBoxes(plain, Chest).size() == 3 && ArmorStandRenderer::armourBoxes(plain, Legs).size() == 3 &&
                  ArmorStandRenderer::armourBoxes(plain, Head).size() == 1 && ArmorStandRenderer::armourBoxes(plain, Feet).size() == 2,
              "and armour is drawn on Mojang's armour model");
        const glm::vec3 hand = handPosition(plain, true);
        check(hand.y > 0.5f && hand.y < 1.2f, "an item is held at the end of its arm");

        ItemStack grid[9];
        for (ItemStack& cell : grid) cell.clear();
        grid[0] = grid[1] = grid[2] = grid[4] = grid[6] = grid[8] = ItemStack{ Items::Stick, 1 };
        grid[7] = ItemStack{ static_cast<StackId>(Blocks::SmoothStoneSlab), 1 };
        check(Crafting::match(grid, 3).id == Items::ArmorStand, "six sticks and a smooth stone slab make one");

        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_stand_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        LevelState level;
        level.seed = 13u;
        LevelState::SavedStand saved;
        saved.x = 3.5f; saved.y = 70.0f; saved.z = -2.5f; saved.yaw = 135.0f; saved.pose = 4;
        saved.gear[Chest] = LevelState::SavedGear{ static_cast<uint16_t>(Items::DiamondChestplate), 1, 17, 5u };
        level.stands.push_back(saved);
        WorldSave::saveLevel(dir, level);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.stands.size() == 1 && back.stands[0].yaw == 135.0f && back.stands[0].pose == 4 &&
                  back.stands[0].gear[Chest].id == static_cast<uint16_t>(Items::DiamondChestplate) && back.stands[0].gear[Chest].damage == 17 &&
                  back.stands[0].gear[Chest].enchantments == 5u,
              "and they are kept with the level, dressed and posed");
        std::filesystem::remove_all(dir, ec);
    }

    void testArchery()
    {
        section("the crossbow, and spectral and tipped arrows");
        const uint32_t sharp = Enchanting::withLevel(0, Enchanting::Kind::Unbreaking, 3);
        const uint32_t loaded = Crossbows::withLoad(sharp, Crossbows::Tipped, static_cast<int>(Effects::Kind::Slowness));
        check(Crossbows::loadOf(loaded) == Crossbows::Tipped && Crossbows::effectOf(loaded) == static_cast<int>(Effects::Kind::Slowness) &&
              Enchanting::levelOf(loaded, Enchanting::Kind::Unbreaking) == 3,
              "a loaded crossbow keeps what it holds above its enchantments");
        check(Crossbows::loadOf(Crossbows::withLoad(loaded, Crossbows::Empty)) == Crossbows::Empty &&
              Enchanting::levelOf(Crossbows::withLoad(loaded, Crossbows::Empty), Enchanting::Kind::Unbreaking) == 3,
              "and firing it empties it without touching them");
        check(!Enchanting::glints(Items::Crossbow, Crossbows::withLoad(0, Crossbows::Arrow)), "a loaded crossbow does not glint for being loaded");
        check(itemDurability(Items::Crossbow) == 465 && itemDurability(Items::Trident) == 250, "a crossbow wears out at 465 uses and a trident at 250");
        ItemStack grid[9];
        const StackId parts[9] = { Items::Stick, Items::IronIngot, Items::Stick, Items::StringItem, Items::TripwireHook, Items::StringItem,
                                   Blocks::Air, Items::Stick, Blocks::Air };
        for (int i = 0; i < 9; ++i) { grid[i].id = parts[i]; grid[i].count = parts[i] == Blocks::Air ? 0 : 1; }
        check(Crafting::match(grid, 3).id == Items::Crossbow, "sticks, iron, string and a tripwire hook make a crossbow");
        ItemStack glow[9];
        for (int i : { 1, 3, 5, 7 }) { glow[i].id = Items::GlowstoneDust; glow[i].count = 1; }
        glow[4].id = Items::Arrow; glow[4].count = 1;
        const CraftOutput spectral = Crafting::match(glow, 3);
        check(spectral.id == Items::SpectralArrow && spectral.count == 2, "four glowstone dust round an arrow make two spectral arrows");
        Arrow shaft;
        shaft.kind = Arrows::Kind::Spectral;
        check(shaft.shaft() && !shaft.bursts() && !shaft.flies(), "a spectral arrow sticks in what it hits like any arrow");
        Mob target(MobId::Zombie, glm::vec3(0.0f), 4u);
        target.afflict(Mob::Affliction::Glowing, 10.0f);
        target.afflict(Mob::Affliction::Slowness, 5.0f);
        check(target.glowing() && target.slowed() && !target.poisoned(), "a struck mob glows or is slowed by what was on the arrow");

        uint32_t trident = Enchanting::withTrident(0, Enchanting::TridentKind::Loyalty, 3);
        trident = Enchanting::withTrident(trident, Enchanting::TridentKind::Impaling, 5);
        check(Enchanting::tridentLevel(trident, Enchanting::TridentKind::Loyalty) == 3 &&
              Enchanting::tridentLevel(trident, Enchanting::TridentKind::Impaling) == 5 &&
              Enchanting::tridentLevel(trident, Enchanting::TridentKind::Riptide) == 0, "a trident keeps its own enchantments apart");
        check(Enchanting::tridentConflicts(trident, Enchanting::TridentKind::Riptide) &&
              !Enchanting::tridentConflicts(trident, Enchanting::TridentKind::Channeling), "and riptide will not go with loyalty");
        check(Enchanting::glints(Items::Trident, trident) && !Enchanting::glints(Items::Trident, 0), "an enchanted trident glints");
        bool offered = false;
        for (uint32_t roll = 1; roll < 200 && !offered; ++roll)
            for (int row = 0; row < 3; ++row)
                if (Enchanting::offerFor(row, 15, Items::Trident, roll).special >= 0) offered = true;
        check(offered, "the enchanting table offers trident enchantments for a trident");

        check(Armour::tierOf(Items::ChainmailChestplate) == Armour::Tier::Chainmail &&
              Armour::pieceOf(Items::ChainmailBoots) == Armour::Piece::Boots && Armour::slotFor(Items::ChainmailHelmet) == 0,
              "chainmail is armour of its own, worn where its piece goes");
        int chain = 0;
        for (StackId piece = Items::ChainmailHelmet; piece <= Items::ChainmailBoots; ++piece) chain += Armour::defencePoints(piece);
        check(chain == 12 && Armour::maxDurability(Items::ChainmailChestplate) == 240, "and gives twelve points, wearing like iron");
        check(Enchanting::enchantability(Items::ChainmailLeggings) == 12, "it takes enchantments a little better than iron");
        bool sold = false;
        for (const Trading::Offer& offer : Trading::offersFor(Trading::Profession::Armorer))
            if (offer.give == Items::ChainmailChestplate) sold = true;
        check(sold, "armorers sell it");
    }

    void testFishing()
    {
        section("fishing, and the two sticks you steer with");
        int fish = 0, junk = 0, treasure = 0, bamboo = 0, closedTreasure = 0;
        for (uint32_t seed = 0; seed < 4000; ++seed)
        {
            const Fishing::Catch c = Fishing::roll(seed, true, false);
            if (c.category == Fishing::Category::Fish) ++fish;
            if (c.category == Fishing::Category::Junk) ++junk;
            if (c.category == Fishing::Category::Treasure) ++treasure;
            if (c.stack.id == Blocks::Bamboo) ++bamboo;
            if (Fishing::roll(seed, false, false).category == Fishing::Category::Treasure) ++closedTreasure;
        }
        check(fish > 3200 && fish < 3600 && junk > 280 && junk < 520 && treasure > 120 && treasure < 300,
              "most of what bites is fish, a tenth is junk and a twentieth treasure");
        check(bamboo == 0 && closedTreasure == 0, "bamboo only comes up in a jungle, and treasure only under open sky");
        bool jungleBamboo = false;
        for (uint32_t seed = 0; seed < 4000 && !jungleBamboo; ++seed) jungleBamboo = Fishing::roll(seed, true, true).stack.id == Blocks::Bamboo;
        check(jungleBamboo, "where it does come up");
        bool enchanted = true;
        for (uint32_t seed = 0; seed < 4000; ++seed)
        {
            const Fishing::Catch c = Fishing::roll(seed, true, false);
            if (c.category == Fishing::Category::Treasure && (c.stack.id == Items::Bow || c.stack.id == Items::Book))
                enchanted = enchanted && c.stack.enchantments != 0;
        }
        check(enchanted, "and a bow or a book fished up is always enchanted");
        const float wait = Fishing::waitSeconds(9, true, false);
        check(wait >= 5.0f && wait <= 30.0f && Fishing::waitSeconds(9, false, false) > wait, "a bite takes longer under a roof");

        ItemStack grid[9];
        grid[2].id = Items::Stick; grid[4].id = Items::Stick; grid[6].id = Items::Stick;
        grid[5].id = Items::StringItem; grid[8].id = Items::StringItem;
        for (ItemStack& cell : grid) if (cell.id != Blocks::Air) cell.count = 1;
        const CraftOutput rod = Crafting::match(grid, 3);
        check(rod.valid() && rod.id == Items::FishingRod, "three sticks and two string make a fishing rod");
        ItemStack bait[9];
        bait[0].id = Items::FishingRod; bait[0].count = 1;
        bait[4].id = Items::Carrot; bait[4].count = 1;
        const CraftOutput carrot = Crafting::match(bait, 3);
        check(carrot.valid() && carrot.id == Items::CarrotOnAStick, "a rod over a carrot makes a carrot on a stick");
        check(Fishing::durabilityOf(Items::FishingRod) == 64 && Fishing::durabilityOf(Items::CarrotOnAStick) == 25 &&
              Fishing::durabilityOf(Items::WarpedFungusOnAStick) == 100, "and they wear out at Minecraft's rates");
        check(Enchanting::appliesTo(Enchanting::Kind::Unbreaking, Items::FishingRod), "unbreaking goes on a rod");
        check(Mob::canBeRidden(mobType(MobId::Pig), false, false, true) && !Mob::canBeRidden(mobType(MobId::Pig), false, false, false) &&
              Mob::canBeRidden(mobType(MobId::Strider), false, false, true) && !Mob::canBeRidden(mobType(MobId::Pig), false, true, true),
              "a saddled pig or strider can be ridden, a bare one or a piglet cannot");
        check(Food::isEdible(Items::TropicalFish), "tropical fish can be eaten");

        const Mob first(MobId::Sheep, glm::vec3(0.0f), 7u);
        const Mob second(MobId::Sheep, glm::vec3(0.0f), 7u);
        check(first.serial() != 0 && first.serial() != second.serial(), "every mob has a number of its own, to hang a lead on");
        check(Mob::leashable(mobType(MobId::Sheep)) && Mob::leashable(mobType(MobId::Horse)) &&
              !Mob::leashable(mobType(MobId::Zombie)) && !Mob::leashable(mobType(MobId::Creeper)),
              "animals take a lead and monsters do not");
        Mob named(MobId::Pig, glm::vec3(0.0f), 3u);
        named.setCustomName("Wilbur");
        check(named.customName() == "Wilbur", "a name tag's name stays on the mob");

        ItemStack rope[9];
        rope[0].id = Items::StringItem; rope[1].id = Items::StringItem; rope[3].id = Items::StringItem;
        rope[4].id = Items::Slimeball; rope[8].id = Items::StringItem;
        for (ItemStack& cell : rope) if (cell.id != Blocks::Air) cell.count = 1;
        const CraftOutput lead = Crafting::match(rope, 3);
        check(lead.valid() && lead.id == Items::Lead && lead.count == 2, "four string and a slimeball make two leads");

        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_name_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        LevelState level;
        level.seed = 6u;
        level.itemNames = { "Shaun", "Wilbur" };
        WorldSave::saveLevel(dir, level);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.itemNames.size() == 2 && back.itemNames[1] == "Wilbur",
              "the names written on name tags are kept with the level");

        LevelState pointed;
        pointed.seed = 8u;
        pointed.lodestones.push_back({ 0, 12, 64, -7 });
        pointed.hasDeathPoint = true;
        pointed.deathPoint[0] = 100.5f; pointed.deathPoint[1] = 40.0f; pointed.deathPoint[2] = -3.5f;
        pointed.deathDimension = 1;
        WorldSave::saveLevel(dir, pointed);
        LevelState again;
        check(WorldSave::loadLevel(dir, again) && again.lodestones.size() == 1 && again.lodestones[0][3] == -7 && again.hasDeathPoint &&
              again.deathPoint[0] == 100.5f && again.deathDimension == 1,
              "lodestones a compass is bound to, and where you last died, are kept too");
        std::filesystem::remove_all(dir, ec);

        ItemStack glass[9];
        glass[1].id = Items::AmethystShard; glass[4].id = Items::CopperIngot; glass[7].id = Items::CopperIngot;
        for (ItemStack& cell : glass) if (cell.id != Blocks::Air) cell.count = 1;
        check(Crafting::match(glass, 3).id == Items::Spyglass, "an amethyst shard over two copper ingots makes a spyglass");
        ItemStack dial[9];
        for (int i : { 1, 3, 5, 7 }) { dial[i].id = Items::GoldIngot; dial[i].count = 1; }
        dial[4].id = Items::Redstone; dial[4].count = 1;
        check(Crafting::match(dial, 3).id == Items::Clock, "four gold round a redstone makes a clock");
        ItemStack echo[9];
        for (int i = 0; i < 9; ++i) { echo[i].id = i == 4 ? Items::Compass : Items::EchoShard; echo[i].count = 1; }
        check(Crafting::match(echo, 3).id == Items::RecoveryCompass, "eight echo shards round a compass make a recovery compass");

        ItemStack sack[9];
        sack[1].id = Items::StringItem; sack[4].id = Items::Leather;
        for (ItemStack& cell : sack) if (cell.id != Blocks::Air) cell.count = 1;
        check(Crafting::match(sack, 3).id == Items::Bundle, "string over leather makes a bundle");
        check(maxStackOf(Items::Bundle) == 1 && maxStackOf(Items::GoatHorn) == 1, "bundles and horns are one to a slot");
        check(EntityManager::breaksHorn(Blocks::Stone) && EntityManager::breaksHorn(Blocks::Log) && EntityManager::breaksHorn(Blocks::PackedIce) &&
              !EntityManager::breaksHorn(Blocks::Dirt) && !EntityManager::breaksHorn(Blocks::Air),
              "a goat sheds a horn ramming stone, logs, ore or packed ice, and not dirt");
        Mob goat(MobId::Goat, glm::vec3(0.0f), 50u * 3u + 7u);
        check(goat.screamingGoat() && goat.hornsLeft() == 2, "one goat in fifty screams, and each has two horns to lose");
        bool fired = false;
        for (int k = 0; k < 4000 && !fired; ++k) fired = goat.ramReady(0.1f);
        check(fired, "and every goat rams now and then");
        bool horn = false;
        for (int n = 0; n < 60 && !horn; ++n)
            for (const StructureLoot::Stack& stack : StructureLoot::rollFor(WorldGen::Hoard::PillagerOutpost, n * 7, 50, n * 11, 4242u))
                if (stack.id == Items::GoatHorn) horn = true;
        check(horn, "pillager outposts keep goat horns");
    }

    void testCreativeBlocks()
    {
        section("command blocks and the other creative-only blocks");
        const BlockId chain = commandBlockOf(1, 4, true);
        check(isCommandBlock(chain) && commandKind(chain) == 1 && commandFacing(chain) == 4 && commandConditional(chain),
              "a command block knows its kind, its facing and whether it is conditional");
        check(isCreativeOnly(chain) && isCreativeOnly(Blocks::Barrier) && isCreativeOnly(lightBlockOf(3)) &&
              isCreativeOnly(Blocks::StructureVoid) && isCreativeOnly(Blocks::JigsawFirst) && isCreativeOnly(Blocks::StructureBlockFirst + 2) &&
              !isCreativeOnly(Blocks::Stone), "and the creative-only blocks are the ones Minecraft keeps out of survival");
        check(blockDrop(commandBlockOf(0, 2, false)) == Blocks::Air && blockDrop(Blocks::Barrier) == Blocks::Air,
              "none of them drops anything");
        check(isObtainable(commandBlockOf(2, 2, false)) && !isObtainable(commandBlockOf(2, 5, false)) &&
              isObtainable(lightBlockOf(15)) && !isObtainable(lightBlockOf(4)), "only one form of each is held");
        check(isSolid(Blocks::Barrier) && !isVisible(Blocks::Barrier), "a barrier stops you and cannot be seen");
        check(!isSolid(lightBlockOf(9)) && lightEmission(lightBlockOf(9)) == 9 && lightEmission(lightBlockOf(15)) == 15,
              "a light block is walked through and shines at its level");
        check(blockInfo(Blocks::Barrier).hardness < 0.0f && blockInfo(commandBlockOf(0, 0, false)).hardness < 0.0f,
              "and none of them can be mined in survival");

        const Chat::Command say = Chat::parse("/say hello there", true);
        check(say.kind == Chat::Kind::Announce && say.text == "hello there", "/say announces what follows");
        const Chat::Command set = Chat::parse("/setblock ~1 ~-2 ~ glowstone", true, glm::vec3(10.5f, 64.2f, -3.7f));
        check(set.kind == Chat::Kind::SetBlock && set.position == glm::vec3(11.0f, 62.0f, -4.0f) && set.item == Blocks::Glowstone,
              "/setblock reads ~ as where the command runs from");
        check(Chat::parse("/setblock 0 70 0 gold_block", true).item == Blocks::GoldBlock, "and Minecraft's own names for blocks");
        check(Chat::parse("/setblock 0 70 0 air", true).kind == Chat::Kind::SetBlock, "air is a block you can set");
        check(Chat::parse("/setblock 0 70 0 wheat_seeds", true).kind == Chat::Kind::Unknown, "but an item is not");
        const Chat::Command summon = Chat::parse("/summon zombie ~ ~1 ~", true, glm::vec3(1.0f, 2.0f, 3.0f));
        check(summon.kind == Chat::Kind::Summon && summon.mob == static_cast<int>(MobId::Zombie) &&
              summon.position == glm::vec3(1.0f, 3.0f, 3.0f), "/summon names a mob and where");
        check(Chat::parse("/give loom", true).kind == Chat::Kind::Give, "and /give finds blocks past the first two hundred and fifty-six");
        check(Chat::parse("/tp ~ ~10 ~", true, glm::vec3(5.0f, 60.0f, 5.0f)).position == glm::vec3(5.0f, 70.0f, 5.0f),
              "/tp takes relative coordinates too");
        check(Chat::succeeded(say) && !Chat::succeeded(Chat::parse("/nonsense", true)), "a command knows whether it worked");

        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_command_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        LevelState level;
        level.seed = 5u;
        LevelState::SavedCommand command;
        command.x = 1; command.y = 2; command.z = 3; command.dimension = 0; command.flags = 3;
        command.command = "say hi";
        command.output = "[@] hi";
        level.commands.push_back(command);
        LevelState::SavedStructure structure;
        structure.x = -4; structure.y = 60; structure.z = 9; structure.name = "house";
        structure.offset[1] = 2; structure.size[0] = 7;
        level.structures.push_back(structure);
        WorldSave::saveLevel(dir, level);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.commands.size() == 1 && back.commands[0].command == "say hi" &&
              back.commands[0].output == "[@] hi" && back.commands[0].flags == 3 && back.structures.size() == 1 &&
              back.structures[0].name == "house" && back.structures[0].offset[1] == 2 && back.structures[0].size[0] == 7,
              "command blocks and structure blocks keep what was typed into them");
        std::filesystem::remove_all(dir, ec);
    }

    void testSculkExtras()
    {
        section("sculk veins and the calibrated sensor");
        check(sculkVeinFace(sculkVeinOn(4)) == 4 && isSculkVein(sculkVeinOn(0)) && !isSculkVein(Blocks::Sculk),
              "a vein knows which face it grows on");
        check(isObtainable(sculkVeinOn(0)) && !isObtainable(sculkVeinOn(3)), "only the floor vein is a thing you hold");
        check(blockDrop(sculkVeinOn(2)) == Blocks::Air, "and a vein gives nothing back without silk touch");
        check(!isSolid(sculkVeinOn(0)), "you walk through veins");
        check(isCalibratedSensor(calibratedOf(3, true)) && calibratedLit(calibratedOf(3, true)) && calibratedFacing(calibratedOf(3, true)) == 3 &&
              blockFacing(calibratedOf(1, false)) == 1, "a calibrated sensor faces one way and is lit or not");
        check(blockDrop(calibratedOf(0, true)) == static_cast<StackId>(calibratedOf(2, false)) && isObtainable(calibratedOf(2, false)) &&
              !isObtainable(calibratedOf(0, false)), "and comes back as the one you hold");
        check(blockDrop(Blocks::SculkSensorLit) == Blocks::SculkSensor, "a sensor broken while it rings comes back quiet");
        check(blockHeight(Blocks::SculkSensor) == 0.5f && blockHeight(calibratedOf(0, false)) == 0.5f,
              "both sensors are half a block high");
        check(Redstone::isSource(Blocks::SculkSensorLit) && Redstone::isSource(calibratedOf(2, true)) &&
              !Redstone::isSource(Blocks::SculkSensor) && !Redstone::isSource(calibratedOf(2, false)),
              "a sensor powers redstone while it rings and not otherwise");

        ItemStack grid[9];
        grid[1].id = Items::AmethystShard; grid[3].id = Items::AmethystShard; grid[5].id = Items::AmethystShard;
        grid[4].id = Blocks::SculkSensor;
        for (ItemStack& cell : grid) if (cell.id != Blocks::Air) cell.count = 1;
        const CraftOutput made = Crafting::match(grid, 3);
        check(made.valid() && isCalibratedSensor(asBlock(made.id)), "three amethyst shards round a sensor calibrate it");

        struct Grid : Sculk::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> cells;
            BlockId at(int x, int y, int z) const override
            {
                const auto found = cells.find({ x, y, z });
                if (found != cells.end()) return found->second;
                return y < 10 ? Blocks::Stone : Blocks::Air;
            }
            void set(int x, int y, int z, BlockId block) override { cells[{ x, y, z }] = block; }
        } floor;
        const int changed = Sculk::spread(floor, glm::ivec3(0, 10, 0), 8, 1234u);
        int sculk = 0, veins = 0;
        for (const auto& cell : floor.cells)
        {
            if (cell.second == Blocks::Sculk) ++sculk;
            if (isSculkVein(cell.second)) ++veins;
        }
        check(changed > 0 && sculk > 0, "a catalyst's charge turns stone around a death into sculk");
        check(veins > 0, "and leaves veins creeping out from it");
        check(Sculk::replaceable(Blocks::Deepslate) && !Sculk::replaceable(Blocks::Obsidian) && !Sculk::replaceable(Blocks::Planks),
              "sculk takes rock and earth, not obsidian or anything built");
    }

    void testBanners()
    {
        section("banners and the loom");
        bool every = true;
        for (int p = Banners::SquareBottomLeft; p < Banners::PatternCount; ++p)
        {
            int covered = 0;
            for (int y = 0; y < Banners::CLOTH_H; ++y)
                for (int x = 0; x < Banners::CLOTH_W; ++x)
                    if (Banners::coverage(p, x, y) > 0.0f) ++covered;
            if (covered == 0 || covered >= Banners::CLOTH_W * Banners::CLOTH_H) every = false;
        }
        check(every, "every pattern covers some of the cloth and not all of it");
        check(Banners::coverage(Banners::StripeLeft, 0, 20) > 0.0f && Banners::coverage(Banners::StripeLeft, 19, 20) == 0.0f,
              "a pale dexter runs down the left as you look at it");
        check(Banners::coverage(Banners::StripeTop, 10, 0) > 0.0f && Banners::coverage(Banners::StripeTop, 10, 39) == 0.0f,
              "and a chief across the top");

        check(Banners::loomChoices(Blocks::Air).size() == 34, "the loom offers the thirty-four patterns that need no item");
        const std::vector<int> flower = Banners::loomChoices(Items::FlowerBannerPattern);
        check(flower.size() == 1 && flower[0] == Banners::Flower, "and a flower pattern item offers just the flower");
        check(Banners::itemOfPattern(Banners::Guster) == Items::GusterBannerPattern && Banners::patternOfItem(Items::Paper) < 0,
              "each pattern item names its pattern");

        Banners::Design design, next;
        bool grew = true;
        for (int k = 0; k < Banners::MAX_LAYERS; ++k)
        {
            grew = grew && Banners::addLayer(design, Banners::StripeTop + k, k, next);
            design = next;
        }
        check(grew && design.size() == 6, "a banner takes six layers");
        check(!Banners::addLayer(design, Banners::Circle, 1, next), "and no seventh");
        check(!Banners::addLayer(Banners::Design{}, Banners::Base, 1, next), "and the field is not a layer of its own");

        Banners::Library library;
        Banners::Design one{ Banners::Layer{ Banners::Cross, 14 } };
        Banners::Design two{ Banners::Layer{ Banners::Cross, 11 } };
        const int a = library.idOf(one), b = library.idOf(two);
        check(a > 0 && b > 0 && a != b && library.idOf(one) == a, "the same design always gets the same number");
        check(library.idOf(Banners::Design{}) == 0 && library.design(0).empty(), "and a plain banner is number nought");
        check(Banners::key(14, one) != Banners::key(0, one), "a red banner and a white one with the same layers look different");

        Banners::Sheets none;
        std::vector<uint8_t> sheet;
        Banners::compose(14, Banners::Design{ Banners::Layer{ Banners::StripeLeft, 0 } }, none, sheet);
        const uint8_t* left = &sheet[static_cast<size_t>((21 * Banners::SHEET + 2) * 4)];
        const uint8_t* right = &sheet[static_cast<size_t>((21 * Banners::SHEET + 18) * 4)];
        check(sheet.size() == static_cast<size_t>(Banners::SHEET * Banners::SHEET * 4) && left[0] > 200 && left[2] > 180 &&
              right[0] > 120 && right[2] < 80, "without a pack the banner is painted: white stripe on a red field");

        ItemStack grid[9];
        for (int i = 0; i < 6; ++i) { grid[i].id = Blocks::WoolRed; grid[i].count = 1; }
        grid[7].id = Items::Stick; grid[7].count = 1;
        const CraftOutput red = Crafting::match(grid, 3);
        check(red.valid() && red.id == static_cast<StackId>(bannerOf(14, false, 2)), "six red wool over a stick make a red banner");
        ItemStack frame[9];
        frame[0].id = Items::StringItem; frame[1].id = Items::StringItem; frame[3].id = Blocks::Planks; frame[4].id = Blocks::Planks;
        for (ItemStack& cell : frame) if (cell.id != Blocks::Air) cell.count = 1;
        const CraftOutput loom = Crafting::match(frame, 3);
        check(loom.valid() && isLoom(asBlock(loom.id)), "two string over two planks make a loom");
        ItemStack sheetOf[9];
        sheetOf[0].id = Items::Paper; sheetOf[0].count = 1; sheetOf[4].id = Blocks::OxeyeDaisy; sheetOf[4].count = 1;
        const CraftOutput charge = Crafting::match(sheetOf, 3);
        check(charge.valid() && charge.id == Items::FlowerBannerPattern, "paper and an oxeye daisy make the flower pattern");

        Trading::Profession job = Trading::Profession::Nitwit;
        check(Trading::jobSite(Blocks::LoomFirst + 1, job) && job == Trading::Profession::Shepherd, "a loom is a shepherd's");
        check(maxStackOf(bannerOf(3, false, 2)) == 16 && maxStackOf(Items::FlowBannerPattern) == 1,
              "banners stack to sixteen and pattern items do not stack");
        check(Inventory::belongsIn(Inventory::LOOM_BANNER, bannerOf(3, false, 2)) && !Inventory::belongsIn(Inventory::LOOM_BANNER, Blocks::Wool) &&
              Inventory::belongsIn(Inventory::LOOM_DYE, Items::DyeRed) && Inventory::belongsIn(Inventory::LOOM_PATTERN, Items::SkullBannerPattern) &&
              !Inventory::belongsIn(Inventory::LOOM_PATTERN, Items::Paper), "the loom's slots take a banner, a dye and a pattern");
        check(isObtainable(bannerOf(5, false, 2)) && !isObtainable(bannerOf(5, true, 1)) && !isObtainable(bannerOf(5, false, 0)),
              "a banner is a thing you hold, and its other facings are not");
        check(blockDrop(bannerOf(9, true, 3)) == static_cast<StackId>(bannerOf(9, false, 2)), "a wall banner comes down as the banner");
        check(!isSolid(bannerOf(0, false, 0)), "and you walk through banners");

        Inventory inventory;
        ItemStack plain{ static_cast<StackId>(bannerOf(14, false, 2)), 1 };
        ItemStack fancy = plain;
        fancy.damage = a;
        inventory.addStack(plain);
        inventory.addStack(fancy);
        inventory.addStack(fancy);
        int plainSlots = 0, fancySlots = 0, fancyCount = 0;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i)
        {
            const ItemStack& slot = inventory.slot(i);
            if (slot.id != plain.id) continue;
            if (slot.damage == 0) ++plainSlots;
            else { ++fancySlots; fancyCount += slot.count; }
        }
        check(plainSlots == 1 && fancySlots == 1 && fancyCount == 2, "patterned banners stack with their own kind and not with plain ones");
        inventory.add(plain.id, 3);
        check(inventory.slot(0).damage == 0 && inventory.slot(0).count == 4 && inventory.slot(1).count == 2,
              "and plain ones topping up never land on a patterned stack");

        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_banner_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        LevelState level;
        level.seed = 9u;
        level.bannerDesigns.push_back({ { static_cast<uint8_t>(Banners::Cross), 14 }, { static_cast<uint8_t>(Banners::Border), 4 } });
        LevelState::SavedBanner hung;
        hung.x = 5; hung.y = 70; hung.z = -9; hung.dimension = 1; hung.design = 1;
        level.banners.push_back(hung);
        WorldSave::saveLevel(dir, level);
        LevelState back;
        check(WorldSave::loadLevel(dir, back) && back.bannerDesigns.size() == 1 && back.bannerDesigns[0].size() == 2 &&
              back.bannerDesigns[0][1].first == Banners::Border && back.banners.size() == 1 && back.banners[0].z == -9 &&
              back.banners[0].dimension == 1 && back.banners[0].design == 1, "banner designs and where banners hang are saved");
        std::filesystem::remove_all(dir, ec);
    }

    void testMaps()
    {
        section("maps and the cartography table");
        int cx = 0, cz = 0;
        Maps::centreFor(5, -3, 0, cx, cz);
        check(cx == 0 && cz == 0, "a map drawn near the origin is centred on it");
        Maps::centreFor(70, 130, 0, cx, cz);
        check(cx == 128 && cz == 128, "maps tile the world in squares of 128");
        Maps::centreFor(-70, 0, 1, cx, cz);
        check(cx == -192 && Maps::span(1) == 256, "a zoomed map covers twice as much from its own grid");
        check(Maps::shaded(0xFFFFFFu, 0) < Maps::shaded(0xFFFFFFu, 1) && Maps::shaded(0xFFFFFFu, 2) == 0xFFFFFFu,
              "ground facing north is drawn darker than level ground");

        ItemStack compass[9];
        compass[1].id = Items::IronIngot; compass[3].id = Items::IronIngot; compass[4].id = Items::Redstone;
        compass[5].id = Items::IronIngot; compass[7].id = Items::IronIngot;
        for (ItemStack& s : compass) if (s.id != Blocks::Air) s.count = 1;
        const CraftOutput needle = Crafting::match(compass, 3);
        check(needle.valid() && needle.id == Items::Compass, "four iron round a redstone make a compass");
        ItemStack sheet[9];
        for (int i = 0; i < 9; ++i) { sheet[i].id = i == 4 ? Items::Compass : Items::Paper; sheet[i].count = 1; }
        const CraftOutput blank = Crafting::match(sheet, 3);
        check(blank.valid() && blank.id == Items::EmptyMap, "eight paper round a compass make an empty map");
        ItemStack desk[9];
        desk[0].id = Items::Paper; desk[1].id = Items::Paper;
        for (int i : { 3, 4, 6, 7 }) desk[i].id = Blocks::Planks;
        for (ItemStack& s : desk) if (s.id != Blocks::Air) s.count = 1;
        const CraftOutput table = Crafting::match(desk, 3);
        check(table.valid() && table.id == Blocks::CartographyTable, "two paper over four planks make a cartography table");
        Trading::Profession job = Trading::Profession::Nitwit;
        check(Trading::jobSite(Blocks::CartographyTable, job) && job == Trading::Profession::Cartographer,
              "and the table is a cartographer's");
        check(maxStackOf(Items::FilledMap) == 1 && maxStackOf(Items::EmptyMap) == 64,
              "a drawn map is one of a kind and blank ones stack");
        check(Inventory::belongsIn(Inventory::CARTO_MAP, Items::FilledMap) && !Inventory::belongsIn(Inventory::CARTO_MAP, Items::Paper) &&
              Inventory::belongsIn(Inventory::CARTO_EXTRA, Blocks::GlassPane) && !Inventory::belongsIn(Inventory::CARTO_EXTRA, Blocks::Stone),
              "the table's slots take maps and what goes with them");

        const std::string dir = (std::filesystem::temp_directory_path() / "browsercraft_maps_test").string();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        Maps::Data drawn;
        drawn.centreX = 256;
        drawn.centreZ = -128;
        drawn.scale = 2;
        drawn.dimension = 1;
        drawn.locked = true;
        drawn.rgba[0] = 12; drawn.rgba[3] = 255;
        drawn.rgba[Maps::SIZE * Maps::SIZE * 4 - 1] = 200;
        check(Maps::save(dir, 7, drawn), "a map saves");
        Maps::Data back;
        check(Maps::load(dir, 7, back) && back.centreX == 256 && back.centreZ == -128 && back.scale == 2 && back.dimension == 1 &&
              back.locked && back.rgba[0] == 12 && back.rgba[3] == 255 && back.rgba[Maps::SIZE * Maps::SIZE * 4 - 1] == 200,
              "and comes back as it was drawn");
        Maps::Data none;
        check(!Maps::load(dir, 8, none), "a map that was never drawn does not load");

        LevelState level;
        level.seed = 3u;
        level.nextMapId = 42;
        WorldSave::saveLevel(dir, level);
        LevelState read;
        check(WorldSave::loadLevel(dir, read) && read.nextMapId == 42, "the level remembers which map number comes next");
        std::filesystem::remove_all(dir, ec);
    }

    void testStonecutter()
    {
        section("the stonecutter and the fletching table");
        const std::vector<Stonecutting::Option> stone = Stonecutting::optionsFor(Blocks::Stone);
        bool slab = false, bricks = false;
        for (const Stonecutting::Option& o : stone)
        {
            if (o.result == Blocks::StoneSlab && o.count == 2) slab = true;
            if (o.result == Blocks::StoneBricks && o.count == 1) bricks = true;
        }
        check(slab && bricks, "stone cuts into two slabs or one stone brick");
        check(Stonecutting::optionsFor(Items::Stick).empty() && Stonecutting::optionsFor(Blocks::Dirt).empty(),
              "and a stick or dirt cuts into nothing");
        check(Stonecutting::everyOutput().size() > 50, "the table is the 1.21 data's, as far as the blocks go");
        Trading::Profession job = Trading::Profession::Nitwit;
        check(Trading::jobSite(Blocks::Stonecutter, job) && job == Trading::Profession::Mason, "a stonecutter is a mason's");
        check(Trading::jobSite(Blocks::FletchingTable, job) && job == Trading::Profession::Fletcher, "a fletching table a fletcher's");
        check(Inventory::belongsIn(Inventory::CUT_INPUT, Blocks::Stone) && !Inventory::belongsIn(Inventory::CUT_INPUT, Items::Stick),
              "the cutter takes blocks only");
        const std::vector<StackId> made = Crafting::everyOutput();
        check(std::find(made.begin(), made.end(), static_cast<StackId>(Blocks::Stonecutter)) != made.end() &&
              std::find(made.begin(), made.end(), static_cast<StackId>(Blocks::FletchingTable)) != made.end(),
              "both are crafted");
    }

    // --- the workstations -----------------------------------------------

    // A village had a bench and a furnace in it and nothing else
    // anybody would stand at. These are the rest: two kilns, a box
    // that rots plants, a wheel that strips enchantments, a desk and
    // a pot.
    void testWorkstations()
    {
        section("the village workstations");

        // The three kilns are one machine. Everything that opens,
        // lights or breaks one asks these, so a fourth would be ids
        // and a tile and nothing else.
        const BlockId KILNS[] = { Blocks::Furnace, Blocks::FurnaceLit,
                                  Blocks::Smoker, Blocks::SmokerLit,
                                  Blocks::BlastFurnace, Blocks::BlastFurnaceLit };
        for (BlockId id : KILNS)
        {
            check(isFurnaceBlock(id), std::string(blockInfo(id).name) + " is a kiln");
            check(isFurnaceBlock(furnaceLit(id)) && isFurnaceBlock(furnaceCold(id)),
                  "and both its states are");
            check(furnaceCold(furnaceLit(id)) == furnaceCold(id),
                  "lighting it and putting it out is the same block again");
            check(Smelting::kindOf(furnaceLit(id)) == Smelting::kindOf(furnaceCold(id)),
                  "and lighting it does not change what it is");
        }
        check(!isFurnaceBlock(Blocks::CraftingTable), "a bench is not a kiln");
        check(furnaceLit(Blocks::Smoker) == Blocks::SmokerLit, "a lit smoker is a smoker");
        check(furnaceCold(Blocks::BlastFurnaceLit) == Blocks::BlastFurnace,
              "and a cold blast furnace is one too");

        check(Smelting::kindOf(Blocks::Smoker) == Smelting::Kind::Smoker, "a smoker");
        check(Smelting::kindOf(Blocks::BlastFurnaceLit) == Smelting::Kind::BlastFurnace,
              "a blast furnace");
        check(Smelting::kindOf(Blocks::Stone) == Smelting::Kind::Furnace,
              "and anything else is a plain furnace, which is the harmless answer");

        // What each will take. The whole point of the two specialists
        // is that they take less, so a smoker that smelted ore would
        // make the furnace pointless.
        check(Smelting::suits(Smelting::Kind::Furnace, Items::RawBeef), "a furnace cooks meat");
        check(Smelting::suits(Smelting::Kind::Furnace, Blocks::IronOre), "and smelts ore");
        check(Smelting::suits(Smelting::Kind::Smoker, Items::RawBeef), "a smoker cooks meat");
        check(!Smelting::suits(Smelting::Kind::Smoker, Blocks::IronOre),
              "but will not touch ore");
        check(Smelting::suits(Smelting::Kind::BlastFurnace, Blocks::IronOre),
              "a blast furnace smelts ore");
        check(Smelting::suits(Smelting::Kind::BlastFurnace, Items::RawCopper),
              "and raw metal");
        check(!Smelting::suits(Smelting::Kind::BlastFurnace, Items::RawBeef),
              "but will not cook dinner");
        check(!Smelting::suits(Smelting::Kind::Smoker, Blocks::Sand),
              "and neither makes glass");

        check(Smelting::resultIn(Smelting::Kind::Smoker, Items::Potato) == Items::BakedPotato,
              "what a smoker takes it still makes");
        check(Smelting::resultIn(Smelting::Kind::Smoker, Blocks::IronOre) == Blocks::Air,
              "and what it will not take it makes nothing of");

        check(Smelting::cookSeconds(Smelting::Kind::Smoker) <
                  Smelting::cookSeconds(Smelting::Kind::Furnace),
              "a smoker is faster than a furnace");
        check(Smelting::cookSeconds(Smelting::Kind::BlastFurnace) ==
                  Smelting::cookSeconds(Smelting::Kind::Smoker),
              "and the two specialists run at the same speed");

        // Run one of each with the same load and count what comes out.
        // Twice the speed has to mean twice the output, or the trade
        // the two specialists offer is not a trade.
        auto runFor = [](Smelting::Kind kind, StackId load, float seconds)
        {
            ItemStack in, fuel, out;
            in.id = load; in.count = 64;
            fuel.id = Items::Coal; fuel.count = 64;

            Furnace machine;
            machine.input = &in;
            machine.fuel = &fuel;
            machine.output = &out;
            machine.kind = kind;

            for (float t = 0.0f; t < seconds; t += 0.05f) machine.tick(0.05f);
            return out.count;
        };

        const int slow = runFor(Smelting::Kind::Furnace, Items::RawBeef, 61.0f);
        const int fast = runFor(Smelting::Kind::Smoker, Items::RawBeef, 61.0f);
        check(slow > 0, "a furnace cooks something in a minute");
        check(fast >= slow * 2, "and a smoker cooks at least twice as much");
        check(runFor(Smelting::Kind::Smoker, Blocks::IronOre, 61.0f) == 0,
              "a smoker given ore sits there doing nothing");

        // --- the composter ---
        check(COMPOST_FULL == 8, "a composter has eight levels");
        for (int level = 0; level <= COMPOST_FULL; ++level)
        {
            const BlockId at = composterAt(level);
            check(isComposter(at), "every level reads as a composter");
            check(compostLevel(at) == level, "and knows which level it is at");
            check(blockDrop(at) == Blocks::Composter0,
                  "and comes up empty however full it was");
            check(isObtainable(at) == (level == 0),
                  "only the empty one is a thing you hold");
        }
        check(composterAt(99) == Blocks::Composter8, "it cannot fill past full");
        check(composterAt(-3) == Blocks::Composter0, "nor empty past empty");
        check(!isComposter(Blocks::Barrel), "a barrel is not a composter");

        // What goes in it. Asked of the families, so a leaf or a crop
        // added later composts without anybody remembering to say so.
        check(Composting::composts(Items::WheatSeeds), "seeds compost");
        check(Composting::composts(Blocks::Leaves), "leaves compost");
        check(Composting::composts(Blocks::TallGrass), "grass composts");
        check(Composting::composts(Items::Carrot), "a carrot composts");
        check(Composting::composts(Items::Bread), "bread composts");
        check(!Composting::composts(Blocks::Stone), "stone does not");
        check(!Composting::composts(Items::IronIngot), "nor iron");
        check(!Composting::composts(Items::RawBeef), "nor meat");

        check(Composting::chanceOf(Items::Bread) > Composting::chanceOf(Items::Wheat),
              "bread is worth more than the wheat it was made of");
        check(Composting::chanceOf(Items::Wheat) > Composting::chanceOf(Items::WheatSeeds),
              "and wheat is worth more than its seed");

        // A hundred throws of each, so the chances mean what they say.
        for (StackId what : { static_cast<StackId>(Items::WheatSeeds),
                              static_cast<StackId>(Items::Wheat),
                              static_cast<StackId>(Items::Bread) })
        {
            int raised = 0;
            for (uint32_t roll = 0; roll < 100; ++roll)
                if (Composting::raises(what, roll)) ++raised;

            const int wanted = static_cast<int>(Composting::chanceOf(what) * 100.0f + 0.5f);
            check(raised == wanted,
                  std::string(displayName(what)) + " raises it " +
                      std::to_string(wanted) + " times in a hundred");
        }
        int neverRaises = 0;
        for (uint32_t roll = 0; roll < 100; ++roll)
            if (Composting::raises(Blocks::Stone, roll)) ++neverRaises;
        check(neverRaises == 0, "and stone never raises it at all");

        // --- the cauldron ---
        check(CAULDRON_FULL == 3, "a cauldron holds three");
        for (int level = 0; level <= CAULDRON_FULL; ++level)
        {
            const BlockId at = cauldronAt(level);
            check(isCauldron(at), "every level reads as a cauldron");
            check(cauldronLevel(at) == level, "and knows how full it is");
            check(blockDrop(at) == Blocks::Cauldron0, "and comes up empty");
            check(isObtainable(at) == (level == 0), "only the empty one is held");
        }
        check(cauldronAt(99) == Blocks::Cauldron3, "it cannot fill past full");
        check(cauldronAt(-1) == Blocks::Cauldron0, "nor empty past empty");

        // --- the lectern ---
        check(isLectern(Blocks::Lectern) && isLectern(Blocks::LecternBook),
              "both lecterns read as one");
        check(!isLectern(Blocks::Bookshelf), "a bookshelf does not");
        check(isObtainable(Blocks::Lectern), "an empty lectern is a thing you hold");
        check(!isObtainable(Blocks::LecternBook),
              "one with a book on it is the same block holding something");
        check(blockDrop(Blocks::LecternBook) == Blocks::Lectern,
              "and breaking it gives back the lectern");

        // --- the grindstone ---
        check(Enchanting::grindHundredths(0) == 0, "nothing on it is worth nothing");
        const Enchanting::Set one =
            Enchanting::withLevel(0, Enchanting::Kind::Sharpness, 1);
        const Enchanting::Set five =
            Enchanting::withLevel(0, Enchanting::Kind::Sharpness, 5);
        check(Enchanting::grindHundredths(five) > Enchanting::grindHundredths(one),
              "and a better enchantment pays better");
        check(Enchanting::grindHundredths(five) ==
                  5 * Enchanting::grindHundredths(one),
              "five levels pay five times one");

        // --- all six, as blocks ---
        const BlockId SHOP[] = { Blocks::Smoker, Blocks::BlastFurnace,
                                 Blocks::Composter0, Blocks::Grindstone,
                                 Blocks::Lectern, Blocks::Cauldron0 };
        for (BlockId id : SHOP)
        {
            check(isObtainable(id), std::string(blockInfo(id).name) + " can be held");
            check(blockInfo(id).hardness > 0.0f, "and takes work to break");
            check(blockInfo(id).collides, "and is something to stand on");
            check(blockSoundGroup(id) != nullptr, "and makes a noise when it breaks");
        }

        // Each is made at a bench, since none of them is found.
        const std::vector<StackId> made = Crafting::everyOutput();
        for (BlockId id : SHOP)
            check(std::find(made.begin(), made.end(), static_cast<StackId>(id)) != made.end(),
                  std::string(blockInfo(id).name) + " has a recipe");

        // Wood wants an axe and stone wants a pickaxe. Left to fall
        // through, every one of these would have wanted a pickaxe --
        // which is the mistake the block table has caught five times.
        check(blockMaterial(Blocks::Composter0) == Material::Wood, "a composter is wood");
        check(blockMaterial(Blocks::Lectern) == Material::Wood, "so is a lectern");
        check(blockMaterial(Blocks::Composter8) == Material::Wood,
              "and a full composter is still wood");
        check(blockMaterial(Blocks::Smoker) == Material::Stone, "a smoker is stone");
        check(blockMaterial(Blocks::Cauldron3) == Material::Stone,
              "and a cauldron is iron, which comes apart the same way");
    }

    // --- what is made rather than grown ---------------------------------

    // The cake, the cookie, the candle and the scaffolding, and the
    // two things a cake cannot be made without.
    void testMadeThings()
    {
        section("cake, cookies, candles and scaffolding");

        // A cake is carried as an item and set down as a block, and
        // from then on it is eaten where it stands. Nothing else in
        // the game works that way.
        check(isItem(Items::Cake), "a cake is carried as an item");
        check(maxStackOf(Items::Cake) == 1, "one at a time, like a bucket");
        for (int eaten = 0; eaten < CAKE_SLICES; ++eaten)
        {
            const BlockId at = static_cast<BlockId>(Blocks::Cake0 + eaten);
            check(isCake(at), "every state of it reads as a cake");
            check(cakeEaten(at) == eaten, "and knows how much is gone");
            check(blockDrop(at) == Blocks::Air,
                  "breaking one gives nothing back, which is Minecraft's");
            check(!isObtainable(at), "and none of them is a block you hold");
        }
        check(!isCake(Blocks::Candle), "a candle is not a cake");
        check(cakeEaten(Blocks::Cake6) == CAKE_SLICES - 1,
              "the last state is one slice from gone");

        // A candle is the one light you can put out again.
        check(blockInfo(Blocks::CandleLit).lightEmission > 0, "a lit candle gives light");
        check(blockInfo(Blocks::Candle).lightEmission == 0, "an unlit one does not");
        check(blockDrop(Blocks::CandleLit) == Blocks::Candle,
              "and breaking a lit one gives back a candle");
        check(isObtainable(Blocks::Candle), "a candle is a thing you hold");
        check(!isObtainable(Blocks::CandleLit), "a lit one is the same candle alight");

        // Scaffolding is climbed, which is the whole of what it is for.
        check(isClimbable(Blocks::Scaffolding), "scaffolding can be climbed");
        check(isClimbable(Blocks::LadderNorth), "so can a ladder");
        check(!isClimbable(Blocks::Stone), "stone cannot");
        check(!blockInfo(Blocks::Scaffolding).collides,
              "and you are inside it rather than on it");
        check(blockDrop(Blocks::Scaffolding) == Blocks::Scaffolding,
              "it gives back itself");

        // Cocoa: three stages, on a trunk, and the only route to a
        // cookie.
        check(COCOA_RIPE == 2, "cocoa has three stages");
        for (int stage = 0; stage <= COCOA_RIPE; ++stage)
        {
            const BlockId pod = cocoaAt(stage);
            check(isCocoa(pod), "every stage reads as cocoa");
            check(cocoaStage(pod) == stage, "and knows how ripe it is");
            check(blockDrop(pod) == Items::CocoaBeans, "and gives beans");
            check(!isObtainable(pod), "a pod is not a block anybody places");
            check(blockMaterial(pod) == Material::Plant, "and comes apart like a plant");
        }
        check(cocoaAt(99) == Blocks::Cocoa2, "it cannot ripen past ripe");
        check(cocoaAt(-1) == Blocks::Cocoa0, "nor unripen");

        // The recipes. Each of these is the only route to its result,
        // so a missing one is a thing in the game nobody can have.
        const std::vector<StackId> made = Crafting::everyOutput();
        auto craftable = [&](StackId id)
        {
            return std::find(made.begin(), made.end(), id) != made.end();
        };
        check(craftable(Items::Cake), "a cake can be made");
        check(craftable(Items::Cookie), "a cookie can be made");
        check(craftable(Blocks::Scaffolding), "scaffolding can be made");
        check(craftable(Blocks::Candle), "a candle can be made");
        check(Food::isEdible(Items::Cookie), "and a cookie is food");

        // The bucket comes back out of the grid. Without this a cake
        // cost three iron as well as three milk, which is Minecraft's
        // recipe at four times the price.
        {
            ItemStack grid[9];
            grid[0].id = Items::MilkBucket; grid[0].count = 1;
            grid[1].id = Items::Wheat;      grid[1].count = 3;

            Crafting::consume(grid, 3);
            check(grid[0].id == Items::Bucket && grid[0].count == 1,
                  "a milk bucket used in a recipe leaves the bucket behind");
            check(grid[1].count == 2, "and everything else is spent one at a time");
        }

        // Milk is the only way out of a bad potion, so it has to be
        // drinkable and it has to not be food: a full player must
        // still be able to drink one.
        check(!Food::isEdible(Items::MilkBucket),
              "milk does not go through the food table");
        check(maxStackOf(Items::MilkBucket) == 1, "and comes one to a bucket");

        // The names and the icons, which is where the item table goes
        // wrong when something is added in the middle of the enum.
        const StackId MADE[] = { Items::Egg, Items::MilkBucket, Items::Cake,
                                 Items::Cookie, Items::CocoaBeans };
        const char* NAMES[] = { "Egg", "Milk Bucket", "Cake", "Cookie", "Cocoa Beans" };
        for (int i = 0; i < 5; ++i)
        {
            check(std::string(itemInfo(MADE[i]).name) == NAMES[i],
                  std::string("the item table calls it a ") + NAMES[i]);
            for (int j = i + 1; j < 5; ++j)
                check(itemInfo(MADE[i]).tile != itemInfo(MADE[j]).tile,
                      std::string(NAMES[i]) + " and " + NAMES[j] +
                          " do not share an icon");
        }

        // A chicken lays on a clock of its own, which is the only way
        // an egg gets into the game.
        check(Mob::EGG_SECONDS > 0.0f, "a chicken lays on a clock");
    }

    // --- the End's leftovers --------------------------------------------

    // The wings, the shulker, the drowned and what it carries.
    void testTheEndAndTheSea()
    {
        section("the elytra");

        // What turns a glide on and off. Everything here is a way of
        // being in the air with a working pair of wings on.
        Glide::Conditions wings;
        wings.wearingElytra = true;
        wings.jumpPressed = true;
        check(Glide::holds(wings), "jumping in the air with wings on starts a glide");

        wings.onGround = true;
        check(!Glide::holds(wings), "the ground ends one");
        wings.onGround = false;
        wings.inWater = true;
        check(!Glide::holds(wings), "so does water");
        wings.inWater = false;
        wings.climbing = true;
        check(!Glide::holds(wings), "and a ladder");
        wings.climbing = false;
        wings.flying = true;
        check(!Glide::holds(wings), "and creative flight, which is already flying");
        wings.flying = false;

        wings.wearingElytra = false;
        wings.alreadyGliding = true;
        check(!Glide::holds(wings), "taking the wings off drops you out of one");
        wings.wearingElytra = true;
        check(Glide::holds(wings), "and a glide once started needs no more jumping");

        // Jumping off the ground must not turn the hop into a flight.
        wings.alreadyGliding = false;
        wings.velocityY = 6.0f;
        check(!Glide::holds(wings), "a glide does not start on the way up");

        // Flying it. Level, a glide loses height slowly and keeps its
        // speed; diving trades height for speed; climbing trades back.
        const glm::vec3 level(0.0f, 0.0f, -1.0f);
        const glm::vec3 dive = glm::normalize(glm::vec3(0.0f, -0.8f, -0.6f));
        const glm::vec3 climb = glm::normalize(glm::vec3(0.0f, 0.8f, -0.6f));

        auto speedOf = [](const glm::vec3& v) {
            return glm::length(glm::vec2(v.x, v.z));
        };

        glm::vec3 flat(0.0f, 0.0f, -10.0f);
        for (int i = 0; i < 20; ++i) flat = Glide::step(flat, level, 0.05f);
        check(flat.y < 0.0f, "a level glide still loses height");
        check(flat.y >= Glide::TERMINAL, "but never faster than it may");

        glm::vec3 diving(0.0f, 0.0f, -10.0f);
        for (int i = 0; i < 20; ++i) diving = Glide::step(diving, dive, 0.05f);
        check(speedOf(diving) > 10.0f, "a dive builds speed");
        check(diving.y < flat.y, "and loses height faster than a level glide");

        glm::vec3 climbing(0.0f, 0.0f, -20.0f);
        const float before = speedOf(climbing);
        for (int i = 0; i < 20; ++i) climbing = Glide::step(climbing, climb, 0.05f);
        check(speedOf(climbing) < before, "a climb spends the speed it was carrying");
        check(climbing.y > flat.y, "and buys height with it");

        // Neither end runs away: a glide cannot stall to nothing and
        // cannot build speed for ever.
        glm::vec3 fast(0.0f, 0.0f, -10.0f);
        for (int i = 0; i < 2000; ++i) fast = Glide::step(fast, dive, 0.05f);
        check(speedOf(fast) <= Glide::MAX_SPEED + 0.01f, "a dive has a top speed");

        glm::vec3 slow(0.0f, 0.0f, -10.0f);
        for (int i = 0; i < 2000; ++i) slow = Glide::step(slow, climb, 0.05f);
        check(speedOf(slow) >= Glide::MIN_SPEED - 0.01f, "and a climb cannot stall it");

        // Looking somewhere is going somewhere: a glide steers itself.
        glm::vec3 turning(0.0f, 0.0f, -10.0f);
        for (int i = 0; i < 40; ++i)
            turning = Glide::step(turning, glm::vec3(1.0f, 0.0f, 0.0f), 0.05f);
        check(turning.x > 0.0f, "turning the head turns the glide");
        check(std::fabs(turning.z) < 0.01f, "all the way round");

        // The wings go where a chestplate goes and are not armour.
        check(Armour::pieceOf(Items::Elytra) == Armour::Piece::Chestplate,
              "the wings are worn where a chestplate is");
        check(Armour::defencePoints(Items::Elytra) == 0, "and stop nothing");
        check(Armour::maxDurability(Items::Elytra) > 0, "but they wear out");
        check(Armour::slotFor(Items::Elytra) ==
                  Armour::slotFor(Items::DiamondChestplate),
              "and they share the chestplate's slot");

        // They come out of an End city, which is the only reason to
        // cross the End to find one.
        bool found = false;
        for (int n = 0; n < 40 && !found; ++n)
            for (const StructureLoot::Stack& stack : StructureLoot::rollFor(
                     WorldGen::Hoard::EndCity, n * 7, 50, n * 11, 4242u))
                if (stack.id == Items::Elytra) found = true;
        check(found, "and an End city is where they are");

        section("the shulker and the drowned");

        const MobType& shulker = mobType(MobId::Shulker);
        check(shulker.stationary, "a shulker is bolted where it stands");
        check(shulker.walkSpeed == 0.0f, "so it has no walk speed");
        check(shulker.dimension == Dimension::End, "it belongs to the End");
        check(shulker.arrowDamage > 0, "and it fights at a distance");
        check(shulker.attackDamage == 0, "rather than with its hands");
        check(shulker.drop == Items::ShulkerShell, "it leaves a shell");

        const MobType& drowned = mobType(MobId::Drowned);
        check(drowned.dimension == Dimension::Overworld, "a drowned belongs here");
        check(drowned.attackDamage > 0, "and fights with its hands");
        check(!drowned.burnsInSunlight,
              "it does not burn: it is standing in the sea");
        check(drowned.walkSpeed > 0.0f, "and it walks, unlike the shulker");

        // The trident is the hardest swing in the game short of a
        // mace at the end of a fall.
        check(Tools::attackDamage(Items::Trident) >
                  Tools::attackDamage(Items::DiamondSword),
              "a trident hits harder than a diamond sword");
        check(Tools::kindOf(Items::Trident) == Tools::Kind::Trident,
              "and it is its own kind of weapon");

        section("netherite");

        // The dig. Debris is a diamond-pickaxe block, like obsidian,
        // and the only thing it smelts into is worth having.
        check(Tools::requiredLevel(Blocks::AncientDebris) ==
                  Tools::requiredLevel(Blocks::Obsidian),
              "ancient debris wants what obsidian wants");
        check(!Tools::canHarvest(Items::IronPickaxe, Blocks::AncientDebris),
              "so iron is not enough");
        check(Tools::canHarvest(Items::DiamondPickaxe, Blocks::AncientDebris),
              "and diamond is");
        check(Smelting::resultOf(Blocks::AncientDebris) == Items::NetheriteScrap,
              "fired, it is scrap");
        check(Smelting::suits(Smelting::Kind::BlastFurnace, Blocks::AncientDebris),
              "and a blast furnace will take it, being metal");

        // The nine tools and pieces are at the end of the item list
        // rather than after the diamond ones, so every piece of the
        // arithmetic that finds a tier has to be asked by hand.
        const StackId NETHER_TOOLS[] = {
            Items::NetheritePickaxe, Items::NetheriteAxe, Items::NetheriteShovel,
            Items::NetheriteSword, Items::NetheriteHoe,
        };
        const Tools::Kind WANTED_KIND[] = {
            Tools::Kind::Pickaxe, Tools::Kind::Axe, Tools::Kind::Shovel,
            Tools::Kind::Sword, Tools::Kind::Hoe,
        };
        for (int i = 0; i < 5; ++i)
        {
            const StackId tool = NETHER_TOOLS[i];
            check(Tools::isTool(tool), "a netherite tool is a tool");
            check(Tools::kindOf(tool) == WANTED_KIND[i], "and knows which kind it is");
            check(Tools::tierOf(tool) == Tools::Tier::Netherite,
                  "and which tier it is");
        }
        check(Tools::maxDurability(Tools::Tier::Netherite) >
                  Tools::maxDurability(Tools::Tier::Diamond),
              "netherite outlasts diamond");
        check(Tools::harvestLevel(Tools::Tier::Netherite) >=
                  Tools::harvestLevel(Tools::Tier::Diamond),
              "and gets at everything diamond does");
        check(Tools::attackDamage(Items::NetheriteSword) >
                  Tools::attackDamage(Items::DiamondSword),
              "and hits harder");

        const StackId NETHER_ARMOUR[] = {
            Items::NetheriteHelmet, Items::NetheriteChestplate,
            Items::NetheriteLeggings, Items::NetheriteBoots,
        };
        const Armour::Piece WANTED_PIECE[] = {
            Armour::Piece::Helmet, Armour::Piece::Chestplate,
            Armour::Piece::Leggings, Armour::Piece::Boots,
        };
        for (int i = 0; i < 4; ++i)
        {
            const StackId piece = NETHER_ARMOUR[i];
            check(Armour::isArmour(piece), "a netherite piece is armour");
            check(Armour::pieceOf(piece) == WANTED_PIECE[i], "and knows where it goes");
            check(Armour::tierOf(piece) == Armour::Tier::Netherite,
                  "and which tier it is");
            check(Armour::slotFor(piece) ==
                      Armour::slotFor(static_cast<StackId>(Items::DiamondHelmet + i)),
                  "and shares the diamond piece's slot");
            check(Armour::maxDurability(piece) >
                      Armour::maxDurability(static_cast<StackId>(Items::DiamondHelmet + i)),
                  "and outlasts it");
        }

        int netheritePoints = 0, diamondPoints = 0;
        for (int i = 0; i < 4; ++i)
        {
            netheritePoints += Armour::defencePoints(NETHER_ARMOUR[i]);
            diamondPoints += Armour::defencePoints(
                static_cast<StackId>(Items::DiamondHelmet + i));
        }
        check(netheritePoints == diamondPoints,
              "a netherite suit stops what a diamond one stops -- it only lasts longer");

        // The upgrade. Nine pairings, and every one has to keep what
        // was on the tool: an upgrade that threw the enchantments away
        // would be a way of ruining a good pickaxe.
        for (StackId id = Items::DiamondPickaxe; id <= Items::DiamondHoe; ++id)
            check(Tools::kindOf(Tools::upgradeOf(id)) == Tools::kindOf(id),
                  "a diamond tool upgrades into the same kind of tool");
        for (StackId id = Items::DiamondHelmet; id <= Items::DiamondBoots; ++id)
            check(Armour::pieceOf(Tools::upgradeOf(id)) == Armour::pieceOf(id),
                  "and a piece of armour into the same piece");
        check(Tools::upgradeOf(Items::IronPickaxe) == Blocks::Air,
              "iron does not upgrade: it is diamond or nothing");
        check(Tools::upgradeOf(Items::Bread) == Blocks::Air, "and neither does bread");

        {
            ItemStack grid[9];
            grid[0].id = Items::DiamondPickaxe;
            grid[0].count = 1;
            grid[0].damage = 200;
            grid[0].enchantments =
                Enchanting::withLevel(0, Enchanting::Kind::Efficiency, 4);
            grid[4].id = Items::NetheriteIngot;
            grid[4].count = 1;

            check(!Crafting::match(grid, 3).valid(), "the crafting grid no longer upgrades: that is the smithing table's");
            const ItemStack plate{ Items::NetheriteUpgradeTemplate, 1 };
            const Smithing::Result made = Smithing::combine(plate, grid[0], grid[4]);
            check(made.valid(), "a template, a diamond pickaxe and an ingot make something");
            check(made.id == Items::NetheritePickaxe, "and what they make is the pickaxe");
            check(!Smithing::combine(ItemStack{ Items::CoastTemplate, 1 }, grid[0], grid[4]).valid(),
                  "and a trim template does not upgrade it");
            check(made.enchantments == grid[0].enchantments,
                  "with the enchantment still on it");
            check(made.damage > 0, "and still worn");
            check(made.damage < Tools::maxDurability(Tools::Tier::Netherite),
                  "but not worn out");

            // The wear carries as a fraction: a tool an eighth gone
            // comes out an eighth gone, not nearly new.
            const double wasFraction = 200.0 /
                Tools::maxDurability(Tools::Tier::Diamond);
            const double nowFraction = static_cast<double>(made.damage) /
                Tools::maxDurability(Tools::Tier::Netherite);
            check(std::fabs(wasFraction - nowFraction) < 0.02,
                  "and as worn as it was, not as worn as the number says");
        }

        {
            // Two ingots, or anything else in the grid, is not an
            // upgrade: it has to be the tool and the one ingot.
            const ItemStack plate{ Items::NetheriteUpgradeTemplate, 1 };
            check(!Smithing::combine(plate, ItemStack{ Items::IronSword, 1 }, ItemStack{ Items::NetheriteIngot, 1 }).valid(),
                  "iron does not upgrade");
            check(!Smithing::combine(plate, ItemStack{ Items::DiamondSword, 1 }, ItemStack{ Items::GoldIngot, 1 }).valid(),
                  "and gold is not netherite");
            ItemStack helmet{ Items::IronHelmet, 1 };
            helmet.enchantments = Enchanting::withLevel(0, Enchanting::Kind::Protection, 2);
            const Smithing::Result trimmed = Smithing::combine(ItemStack{ Items::WildTemplate, 1 }, helmet, ItemStack{ Items::Emerald, 1 });
            check(trimmed.valid() && trimmed.id == Items::IronHelmet && Smithing::trimPattern(trimmed.enchantments) == 2 &&
                  Smithing::trimMaterial(trimmed.enchantments) == 4, "a wild template and an emerald trim a helmet");
            check(Enchanting::levelOf(trimmed.enchantments, Enchanting::Kind::Protection) == 2 &&
                  Enchanting::listOf(trimmed.enchantments).size() == 1, "without touching its enchantments");
            check(!Smithing::combine(ItemStack{ Items::WildTemplate, 1 }, ItemStack{ Items::IronSword, 1 }, ItemStack{ Items::Emerald, 1 }).valid(),
                  "a sword cannot be trimmed");
            check(!Enchanting::glints(static_cast<StackId>(Blocks::DecoratedPotFirst + 2), 5u), "a patterned pot does not glint");
            check(!Enchanting::glints(Items::IronHelmet, Smithing::withTrim(0, 4, 4)), "nor does a trim alone");
        }

        section("the ghast");

        const MobType& ghast = mobType(MobId::Ghast);
        check(ghast.dimension == Dimension::Nether, "a ghast belongs to the Nether");
        check(ghast.hoverHeight > 0.0f, "and hangs in the air");
        check(ghast.arrowRange > mobType(MobId::Blaze).arrowRange,
              "and shoots from further off than anything else down there");
        check(ghast.fireproof, "lava does not trouble it");
        check(ghast.drop == Items::GhastTear, "and it leaves a tear");

        // Ten half-hearts and an enormous target: reaching one is the
        // whole of the fight, not surviving it.
        check(ghast.maxHealth < mobType(MobId::Blaze).maxHealth,
              "it dies to almost anything that reaches it");
        check(ghast.width >= 2.0f && ghast.height >= 2.0f, "but it is enormous");

        // The tear is the only route to a drink that puts hearts back.
        check(Brewing::isIngredient(Items::GhastTear), "a tear brews");
        check(Brewing::brew(Brewing::Potion::Awkward, Items::GhastTear) ==
                  Brewing::Potion::Regeneration,
              "and what it makes is regeneration");
        {
            Effects::Kind kind = Effects::Kind::Speed;
            int amplifier = 0;
            float seconds = 0.0f;
            check(Brewing::effectOf(Brewing::Potion::Regeneration, kind, amplifier, seconds),
                  "which is something you drink rather than brew with");
            check(kind == Effects::Kind::Regeneration, "and it regenerates");
            check(seconds > 0.0f && seconds < 120.0f,
                  "for a shorter while than the rest, because it is worth more");
        }
        check(Brewing::brew(Brewing::Potion::Water, Items::GhastTear) ==
                  Brewing::Potion::Mundane,
              "a tear in plain water is only mundane");

        section("frogs and froglights");

        const MobType& frog = mobType(MobId::Frog);
        check(frog.spawnClass == SpawnClass::Passive, "a frog does nobody any harm");
        check(frog.attackDamage == 0, "and does not fight");
        check(frog.breedingFood == Items::Slimeball,
              "it follows a slimeball, as Minecraft's does");
        check(frog.walkSpeed > 0.0f, "and it hops about");

        // Three lights by the climate, which is Minecraft's rule read
        // off the ground the frog is standing on.
        check(froglightFor(Biome::Jungle) == Blocks::PearlescentFroglight,
              "a frog in the heat leaves the pearl one");
        check(froglightFor(Biome::Desert) == Blocks::PearlescentFroglight,
              "and so does one in the sand");
        check(froglightFor(Biome::Snowy) == Blocks::VerdantFroglight,
              "a frog in the cold leaves the green one");
        check(froglightFor(Biome::Swamp) == Blocks::OchreFroglight,
              "and everywhere in between leaves the ochre one");
        check(froglightFor(Biome::Plains) == Blocks::OchreFroglight,
              "which is the common one, as it should be");

        for (int k = 0; k <= static_cast<int>(Biome::Swamp); ++k)
            check(isFroglight(froglightFor(static_cast<Biome>(k))),
                  "every climate leaves a froglight of some colour");

        const BlockId LIGHTS[] = { Blocks::OchreFroglight, Blocks::VerdantFroglight,
                                   Blocks::PearlescentFroglight };
        for (BlockId light : LIGHTS)
        {
            check(isFroglight(light), std::string(blockInfo(light).name) + " is a froglight");
            check(blockInfo(light).lightEmission == 15,
                  "and the brightest light there is");
            check(isObtainable(light), "and a block you can hold");
            check(blockDrop(light) == light, "which gives back itself");
        }
        check(!isFroglight(Blocks::Glowstone), "glowstone is not one");
        check(!isFroglight(Blocks::Stone), "nor is stone");

        // Two shells round a chest, and the box is a box.
        check(isContainerBlock(Blocks::ShulkerBox), "a shulker box holds things");
        check(isObtainable(Blocks::ShulkerBox), "and is a block you can hold");
        const std::vector<StackId> made = Crafting::everyOutput();
        check(std::find(made.begin(), made.end(),
                        static_cast<StackId>(Blocks::ShulkerBox)) != made.end(),
              "and the shells make one");
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

        for (int id = 1; isBlockId(id); id = nextBlockId(id))
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
            const bool snowy = blockMaterial(id) == Material::Snow;
            const StackId best = snowy ? static_cast<StackId>(Items::DiamondShovel) : static_cast<StackId>(Items::DiamondPickaxe);
            check(blockMaterial(id) == Material::Stone || snowy, who + ": only stone and snow want a tool");
            check(canHarvest(best, id), who + ": diamond gets all of it");
            check(!canHarvest(Blocks::Air, id), who + ": a bare hand gets none");

            // And the right tool is always quicker than the wrong one.
            check(breakSeconds(best, id) < breakSeconds(Blocks::Air, id),
                  who + " is quicker with it");
        }

        check(byHand > 20, "most of the block list comes up by hand");
        check(needTool > 8, "and a fair few want a pickaxe");

        // The ones that were actually wrong, named, so a regression says
        // which block rather than just a count.
        const BlockId SOFT[] = {
            Blocks::BirchLog, Blocks::BirchLeaves, Blocks::Gravel, Blocks::Clay,
            Blocks::SnowGrass, Blocks::TallGrass, Blocks::FlowerRed,
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
        check(sleep.valid() && sleep.id == bedOf(0, 0, false), "white wool over planks makes a white bed");

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
        check(reduce(10, 20) == 4 && reduce(100, 20) == 84, "twenty points take sixty percent off a ten and little off a hundred, as in 1.21.1");
        check(reduce(1, 20) == 1, "but the smallest blow still lands");
        check(reduce(10, 40) == reduce(10, 30), "and points past thirty are wasted");
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

    void testLadders()
    {
        section("ladders");

        // Four facings, each its own id, and the id carries which.
        for (int facing = 0; facing < 4; ++facing)
        {
            const BlockId id = ladderFacing(facing);
            check(isLadder(id), "every facing is a ladder");
            check(blockFacing(id) == facing, "and knows which way it points");
        }

        // Round trips: the id for a facing gives that facing back, and
        // the four are distinct rather than aliases of one another.
        check(ladderFacing(0) != ladderFacing(1), "north is not east");
        check(ladderFacing(2) != ladderFacing(3), "south is not west");
        check(ladderFacing(4) == ladderFacing(0), "and the fifth wraps round to the first");

        // Only one of them is a thing you hold: the other three are the
        // same ladder hung on a different wall, and showing all four in
        // the creative palette would be four copies of one item.
        check(isObtainable(Blocks::LadderNorth), "a ladder can be held");
        check(!isObtainable(Blocks::LadderEast), "but not one already on a wall");
        check(!isObtainable(Blocks::LadderSouth), "nor the next");
        check(!isObtainable(Blocks::LadderWest), "nor the last");

        // You walk through a ladder rather than into it.
        check(!isSolid(Blocks::LadderNorth), "a ladder does not block you");
        check(!isOpaque(Blocks::LadderNorth), "nor hide what is behind it");

        // And it comes up by hand, like the wood it is made of.
        check(Tools::requiredLevel(Blocks::LadderNorth) == 0, "a ladder needs no tool");
        check(blockMaterial(Blocks::LadderNorth) != Material::Stone, "and is not stone");
    }

    void testHandSwing()
    {
        section("the hand swinging");

        HeldItem hand;
        check(!hand.swinging(), "a hand at rest is not swinging");

        hand.swing();
        check(hand.swinging(), "and is once it has been swung");

        // It runs for SWING_SECONDS and then stops.
        hand.update(HeldItem::SWING_SECONDS * 0.5f);
        check(hand.swinging(), "still going halfway through");
        hand.update(HeldItem::SWING_SECONDS * 0.6f);
        check(!hand.swinging(), "and done once its time is up");

        // Mining holds the button down. Asking first is what lets one
        // swing finish before the next starts; calling swing() every
        // frame instead reset the timer before it could run down, and
        // the arm sat near the start of its arc looking still.
        constexpr float FRAME = 1.0f / 60.0f;
        const int frames = 300;

        HeldItem asking;
        int started = 0;
        for (int i = 0; i < frames; ++i)
        {
            if (!asking.swinging()) { asking.swing(); ++started; }
            asking.update(FRAME);
        }

        // Five seconds of mining at six ticks a swing, which is
        // Minecraft's, comes to about sixteen of them. Counted off
        // the constant rather than written down, so lengthening a
        // swing does not read as a broken one.
        const int expected = static_cast<int>(5.0f / HeldItem::SWING_SECONDS);
        check(started >= expected - 2 && started <= expected + 2,
              "mining swings over and over, " + std::to_string(started) + " times in five seconds");

        HeldItem everyFrame;
        int restarts = 0;
        for (int i = 0; i < frames; ++i)
        {
            everyFrame.swing();
            everyFrame.update(FRAME);
            if (!everyFrame.swinging()) ++restarts;
        }
        check(restarts == 0,
              "swinging every frame never lets one finish, which was the bug");
    }

    void testFirstPersonArm()
    {
        section("the hand on screen");

        // The arm box is built with the hand at its top; a skin draws an
        // arm with the sleeve at the top of its patch and the hand at
        // the bottom. So the hand end of the box has to take the LOWER
        // part of the patch. Reading the cube's own uv table straight
        // did the opposite and put the cuff on the knuckles.
        const float atShoulder = FirstPersonArm::patchV(0.0f);
        const float atHand = FirstPersonArm::patchV(1.0f);

        check(atHand > atShoulder,
              "the hand end of the arm takes the hand end of the skin");
        check(atShoulder >= 0.0f && atHand <= 1.0f, "and both stay inside the patch");

        // Halfway along the arm is halfway down the patch, so the
        // forearm does not bunch up at one end.
        check(std::fabs(FirstPersonArm::patchV(0.5f) - 0.5f) < 0.001f,
              "and the middle of the arm is the middle of the patch");

        // The Steve skin this is checked against: teal sleeve on the
        // upper third of the arm patch, bare skin below it. That is the
        // layout every Minecraft skin uses, and it is the reason the
        // rule above runs the way it does rather than the other way.
        check(atHand > 0.5f, "the hand samples the lower half of the patch");
    }

    void testLandmarks()
    {
        section("the surface landmarks");

        constexpr uint32_t SEED = 4242u;
        const WorldGen generator(SEED);

        // Each one has a name, a grid and land it belongs on. A kind
        // that suited nothing would be a kind that never appears.
        for (int k = 0; k < static_cast<int>(WorldGen::Landmark::Count); ++k)
        {
            const WorldGen::Landmark kind = static_cast<WorldGen::Landmark>(k);
            check(WorldGen::landmarkName(kind)[0] != '\0', "every landmark has a name");

            const Structures::Grid& grid = WorldGen::landmarkGrid(kind);
            check(grid.spacing > grid.separation, "and a grid with room in it");

            int suits = 0;
            for (Biome biome : { Biome::Ocean, Biome::Beach, Biome::Plains, Biome::Forest,
                                 Biome::Desert, Biome::Mountains, Biome::Snowy,
                                 Biome::Jungle, Biome::Savanna, Biome::Swamp })
                if (WorldGen::landmarkSuits(kind, biome)) ++suits;
            check(suits > 0, "and somewhere it belongs");
        }

        // The ones that are about a biome are only in that biome. A
        // pyramid in a snowfield is the sort of thing you notice.
        check(WorldGen::landmarkSuits(WorldGen::Landmark::DesertPyramid, Biome::Desert),
              "a pyramid belongs in the desert");
        check(!WorldGen::landmarkSuits(WorldGen::Landmark::DesertPyramid, Biome::Snowy),
              "and nowhere near a snowfield");
        check(WorldGen::landmarkSuits(WorldGen::Landmark::Igloo, Biome::Snowy),
              "an igloo belongs in the snow");
        check(!WorldGen::landmarkSuits(WorldGen::Landmark::Igloo, Biome::Desert),
              "and not in the sand");
        check(WorldGen::landmarkSuits(WorldGen::Landmark::JungleTemple, Biome::Jungle),
              "a temple belongs in the jungle");
        check(!WorldGen::landmarkSuits(WorldGen::Landmark::RuinedPortal, Biome::Ocean),
              "and nothing stands on the sea");

        // And they are built. Each kind is looked for where its own
        // grid says it should be, and the chunks there generated.
        struct Wanted { WorldGen::Landmark kind; BlockId block; const char* says; };
        const Wanted WANTED[] = {
            { WorldGen::Landmark::DesertPyramid,   Blocks::Sandstone,
              "a desert pyramid is built of sandstone" },
            { WorldGen::Landmark::JungleTemple,    Blocks::Cobblestone,
              "a jungle temple of cobblestone" },
            { WorldGen::Landmark::Igloo,           Blocks::Snow,
              "an igloo of snow" },
            { WorldGen::Landmark::RuinedPortal,    Blocks::Obsidian,
              "a ruined portal of obsidian" },
            { WorldGen::Landmark::PillagerOutpost, Blocks::DarkOakPlanks,
              "and an outpost of dark oak" },
            { WorldGen::Landmark::WoodlandMansion, Blocks::DarkOakLog,
              "and a mansion timbered in it" },
        };

        for (const Wanted& wanted : WANTED)
        {
            // A mansion grid is eighty chunks across, so a search wide
            // enough for the rest finds nothing: that distance is the
            // whole character of the thing.
            const int squares =
                wanted.kind == WorldGen::Landmark::WoodlandMansion ? 5 : 6;
            const Structures::Spot spot =
                generator.nearestLandmark(wanted.kind, 0, 0, squares);
            check(spot.exists,
                  std::string(WorldGen::landmarkName(wanted.kind)) +
                      ": there is one within a few squares of the origin");
            if (!spot.exists) continue;

            int material = 0, chests = 0;
            for (int cz = spot.chunkZ - 1; cz <= spot.chunkZ + 1; ++cz)
                for (int cx = spot.chunkX - 1; cx <= spot.chunkX + 1; ++cx)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    for (int y = 0; y < Chunk::SY; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                            {
                                const BlockId block = chunk.getBlock(lx, y, lz);
                                if (block == wanted.block ||
                                    (wanted.block == Blocks::Obsidian && block == Blocks::CryingObsidian)) ++material;
                                if (block == Blocks::Chest) ++chests;
                            }
                }

            check(material > 8, wanted.says);
            check(chests > 0, "with something in it worth the walk");
        }
    }

    void testOceanStructures()
    {
        section("the sea");

        constexpr uint32_t SEED = 4242u;
        const WorldGen generator(SEED);

        // The grids are Minecraft's, which is the whole point of them.
        check(Structures::SHIPWRECK.spacing == 24 && Structures::SHIPWRECK.separation == 4,
              "a shipwreck grid is 24 and 4, as Minecraft's is");
        check(Structures::OCEAN_MONUMENT.spacing == 32 &&
              Structures::OCEAN_MONUMENT.separation == 5,
              "and a monument grid 32 and 5");
        check(Structures::BURIED_TREASURE_CHANCE > 0.0f &&
              Structures::BURIED_TREASURE_CHANCE < 0.05f,
              "buried treasure is rare enough to need a map");

        // A monument needs deep water over the whole of its footprint,
        // which is the only thing keeping one out of a puddle.
        {
            const Structures::Spot spot = generator.nearestMonument(0, 0, 12);
            check(spot.exists, "there is a monument somewhere out there");

            if (spot.exists)
            {
                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
                check(generator.monumentFits(midX, midZ),
                      "and the one that was offered has water over it");

                // Every corner of it is under the sea, not just the
                // middle: a monument half out of the water would be
                // the thing you notice first about this world.
                for (int dz = -WorldGen::MONUMENT_HALF; dz <= WorldGen::MONUMENT_HALF;
                     dz += WorldGen::MONUMENT_HALF)
                    for (int dx = -WorldGen::MONUMENT_HALF; dx <= WorldGen::MONUMENT_HALF;
                         dx += WorldGen::MONUMENT_HALF)
                        check(generator.surfaceHeight(midX + dx, midZ + dz) <=
                                  WorldGen::SEA_LEVEL - WorldGen::MONUMENT_DEPTH,
                              "every corner of it stands in deep water");

                // And it finishes under the surface. A prismarine roof
                // standing proud of the waves is what you get if the
                // height is Minecraft's rather than this ocean's.
                const int tall = generator.monumentHeight(midX, midZ);
                check(generator.surfaceHeight(midX, midZ) + 1 + tall < WorldGen::SEA_LEVEL,
                      "and its roof finishes below the water, not above it");

                int prismarine = 0, lanterns = 0, chests = 0, dark = 0;
                for (int cz = spot.chunkZ - 1; cz <= spot.chunkZ + 1; ++cz)
                    for (int cx = spot.chunkX - 1; cx <= spot.chunkX + 1; ++cx)
                    {
                        Chunk chunk(ChunkPos{ cx, cz });
                        generator.generate(chunk);

                        for (int y = 0; y < Chunk::SY; ++y)
                            for (int lz = 0; lz < Chunk::SZ; ++lz)
                                for (int lx = 0; lx < Chunk::SX; ++lx)
                                    switch (chunk.getBlock(lx, y, lz))
                                    {
                                        case Blocks::Prismarine:
                                        case Blocks::PrismarineBricks: ++prismarine; break;
                                        case Blocks::DarkPrismarine:   ++dark; break;
                                        case Blocks::SeaLantern:       ++lanterns; break;
                                        case Blocks::Chest:            ++chests; break;
                                        default: break;
                                    }
                    }

                check(prismarine > 150, "a monument is a great deal of prismarine");
                check(dark > 0, "with the dark sort up its corners");
                check(lanterns > 0, "and a lantern or two lighting it");
                check(chests > 0, "and the room in the middle is not empty");
            }
        }

        // A shipwreck lies on the sea floor or run aground on a beach,
        // never in a field.
        {
            const Structures::Spot spot = generator.nearestShipwreck(0, 0, 14);
            const bool found = spot.exists;
            if (spot.exists)
            {
                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
                check(generator.surfaceHeight(midX, midZ) <= WorldGen::SEA_LEVEL + 1,
                      "a shipwreck is never left standing in a field");

                int planks = 0, chests = 0;
                for (int cz = spot.chunkZ - 1; cz <= spot.chunkZ + 1; ++cz)
                    for (int cx = spot.chunkX - 1; cx <= spot.chunkX + 1; ++cx)
                    {
                        Chunk chunk(ChunkPos{ cx, cz });
                        generator.generate(chunk);

                        for (int y = 0; y < Chunk::SY; ++y)
                            for (int lz = 0; lz < Chunk::SZ; ++lz)
                                for (int lx = 0; lx < Chunk::SX; ++lx)
                                {
                                    const BlockId block = chunk.getBlock(lx, y, lz);
                                    if (block == Blocks::Planks) ++planks;
                                    if (block == Blocks::Chest) ++chests;
                                }
                    }

                check(planks > 40, "a shipwreck is a hull's worth of planks");
                check(chests > 0, "with what she went down carrying still in her");
            }
            check(found, "there is a shipwreck in the water somewhere near the origin");
        }

        // Buried treasure goes down by the spread rather than the grid,
        // so what matters is the density rather than any one chunk.
        {
            int hits = 0;
            constexpr int LOOKED = 4000;
            for (int i = 0; i < LOOKED; ++i)
                if (Structures::spreadAt(i % 200, i / 200, SEED,
                                         Structures::BURIED_TREASURE_SALT,
                                         Structures::BURIED_TREASURE_CHANCE))
                    ++hits;

            const float rate = static_cast<float>(hits) / static_cast<float>(LOOKED);
            check(rate > Structures::BURIED_TREASURE_CHANCE * 0.4f &&
                  rate < Structures::BURIED_TREASURE_CHANCE * 2.2f,
                  "buried treasure turns up about as often as it is meant to");
        }
    }

    // Every chest the generator puts down is worth opening. They were
    // all empty when the landmarks went in, which is not something a
    // screenshot shows you.
    void testStructureLoot()
    {
        section("what is in a found chest");

        constexpr uint32_t SEED = 4242u;

        const WorldGen::Hoard EVERY[] = {
            WorldGen::Hoard::DesertPyramid,  WorldGen::Hoard::JungleTemple,
            WorldGen::Hoard::Igloo,          WorldGen::Hoard::RuinedPortal,
            WorldGen::Hoard::PillagerOutpost, WorldGen::Hoard::Shipwreck,
            WorldGen::Hoard::BuriedTreasure, WorldGen::Hoard::Monument,
            WorldGen::Hoard::Mineshaft,      WorldGen::Hoard::Stronghold,
            WorldGen::Hoard::TrialChamber,
        };

        check(StructureLoot::rollFor(WorldGen::Hoard::None, 0, 64, 0, SEED).empty(),
              "a chest the player built holds nothing but what they put in it");

        for (WorldGen::Hoard hoard : EVERY)
        {
            int lowest = 999, highest = 0;
            bool signatureEverywhere = true;

            for (int i = 0; i < 60; ++i)
            {
                const int x = 300 + i * 13;
                const int z = -700 - i * 7;
                const std::vector<StructureLoot::Stack> loot =
                    StructureLoot::rollFor(hoard, x, 48, z, SEED);

                check(!loot.empty(), "every kind of found chest holds something");
                if (loot.empty()) continue;

                lowest = std::min(lowest, static_cast<int>(loot.size()));
                highest = std::max(highest, static_cast<int>(loot.size()));

                const StructureLoot::Stack signature =
                    StructureLoot::centrepiece(hoard, x, 48, z, SEED);

                bool has = false;
                for (const StructureLoot::Stack& stack : loot)
                {
                    check(stack.count > 0, "and no empty stacks in it");
                    check(stack.count <= maxStackOf(stack.id),
                          "nor a stack bigger than the item allows");
                    if (stack.id == signature.id) has = true;
                }
                if (!has) signatureEverywhere = false;

                // Rolled from the position, so finding it twice finds
                // the same thing twice.
                const std::vector<StructureLoot::Stack> again =
                    StructureLoot::rollFor(hoard, x, 48, z, SEED);
                check(again.size() == loot.size(),
                      "and the same chest holds the same thing every time");
            }

            check(signatureEverywhere,
                  "the thing this kind of place is known for is always in it");
            check(highest > lowest,
                  "but no two of them are the same, or it is a vending machine");
        }

        // The lookup that says which structure a chest belongs to. Walk
        // to a landmark, find its chest, and ask.
        const WorldGen generator(SEED);
        struct Pair { WorldGen::Landmark kind; WorldGen::Hoard hoard; };
        const Pair PAIRS[] = {
            { WorldGen::Landmark::DesertPyramid,   WorldGen::Hoard::DesertPyramid },
            { WorldGen::Landmark::JungleTemple,    WorldGen::Hoard::JungleTemple },
            { WorldGen::Landmark::Igloo,           WorldGen::Hoard::Igloo },
            { WorldGen::Landmark::RuinedPortal,    WorldGen::Hoard::RuinedPortal },
            { WorldGen::Landmark::PillagerOutpost, WorldGen::Hoard::PillagerOutpost },
        };

        for (const Pair& pair : PAIRS)
        {
            const Structures::Spot spot = generator.nearestLandmark(pair.kind, 0, 0, 6);
            if (!spot.exists) continue;

            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            const int ground = generator.surfaceHeight(midX, midZ);

            WorldGen::ChestSpot spots[2];
            const int many = WorldGen::landmarkChests(pair.kind, spots);
            check(many > 0, "every landmark keeps a chest somewhere");

            for (int i = 0; i < many; ++i)
            {
                const WorldGen::Hoard found =
                    generator.hoardAt(midX + spots[i].dx, ground + spots[i].dy,
                                      midZ + spots[i].dz);
                check(found == pair.hoard,
                      std::string(WorldGen::landmarkName(pair.kind)) +
                          ": its chest knows what it belongs to");
            }

            // And a block of air off to one side belongs to nothing,
            // or every chest in the world would be a pyramid's.
            check(generator.hoardAt(midX + 40, ground, midZ + 40) == WorldGen::Hoard::None,
                  "while the ground beside it belongs to nothing");
        }
    }

    void testSeabed()
    {
        section("what grows on the sea floor");

        constexpr uint32_t SEED = 4242u;
        const WorldGen generator(SEED);

        int kelp = 0, kelpTops = 0, seagrass = 0, coral = 0, fans = 0, pickles = 0;
        int seaFloors = 0, landColumns = 0, plantsOnLand = 0;

        for (int cz = -4; cz <= 4; ++cz)
            for (int cx = -4; cx <= 4; ++cx)
            {
                Chunk chunk(ChunkPos{ cx, cz });
                generator.generate(chunk);

                for (int lx = 0; lx < Chunk::SX; ++lx)
                    for (int lz = 0; lz < Chunk::SZ; ++lz)
                    {
                        const int wx = cx * Chunk::SX + lx;
                        const int wz = cz * Chunk::SZ + lz;
                        const bool wet =
                            generator.surfaceHeight(wx, wz) < WorldGen::SEA_LEVEL - 1;
                        if (wet) ++seaFloors; else ++landColumns;

                        for (int y = 0; y < Chunk::SY; ++y)
                        {
                            const BlockId block = chunk.getBlock(lx, y, lz);
                            if (!isWaterPlant(block) && !isCoralBlock(block)) continue;

                            // Nothing that grows in the sea is allowed
                            // to turn up on a hillside.
                            if (!wet) ++plantsOnLand;

                            switch (block)
                            {
                                case Blocks::Kelp:      ++kelp; break;
                                case Blocks::KelpTop:   ++kelpTops; break;
                                case Blocks::Seagrass:  ++seagrass; break;
                                case Blocks::SeaPickle: ++pickles; break;
                                default:
                                    if (isCoralBlock(block)) ++coral; else ++fans;
                                    break;
                            }
                        }
                    }
            }

        check(seaFloors > 0, "there is sea in the chunks round the origin");
        check(landColumns > 0, "and land as well, or this proves nothing");

        check(seagrass > 0, "seagrass grows on the sea floor");
        check(kelp > 0, "kelp grows on the sea floor");
        check(kelpTops > 0, "and every stem of it ends in a frond");
        check(kelpTops <= kelp, "with more stem than frond, not the other way round");

        check(plantsOnLand == 0, "and none of it grows on dry land");

        // Coral is the warm-water one, so it is rarer than the rest.
        // Counting it rather than requiring it: the chunks round one
        // origin may hold no warm water at all.
        check(coral >= 0 && fans >= 0 && pickles >= 0, "coral is counted where it grows");

        // A reef only ever stands in warm water. This is the test that
        // would catch coral turning up in the sea off a snowfield.
        for (int cz = -4; cz <= 4; ++cz)
            for (int cx = -4; cx <= 4; ++cx)
            {
                Chunk chunk(ChunkPos{ cx, cz });
                generator.generate(chunk);

                for (int lx = 0; lx < Chunk::SX; ++lx)
                    for (int lz = 0; lz < Chunk::SZ; ++lz)
                        for (int y = 0; y < Chunk::SY; ++y)
                            if (isCoralBlock(chunk.getBlock(lx, y, lz)))
                            {
                                check(generator.reefWater(cx * Chunk::SX + lx,
                                                          cz * Chunk::SZ + lz),
                                      "coral only ever stands in warm water");
                                lx = Chunk::SX; lz = Chunk::SZ; y = Chunk::SY;
                            }
            }

        // The mesher rule that keeps a glassy box from forming round
        // every blade: the sea draws no face against what grows in it.
        check(isWaterPlant(Blocks::Kelp), "kelp counts as something growing in water");
        check(isWaterPlant(Blocks::Seagrass), "and so does seagrass");
        check(isWaterPlant(Blocks::TubeCoral), "and a coral fan");
        check(!isWaterPlant(Blocks::TubeCoralBlock),
              "but a coral block is solid, not a thing standing in the water");
        check(!isWaterPlant(Blocks::TallGrass),
              "and what grows on land does not count");
        check(isCoralBlock(Blocks::HornCoralBlock) && isCoralBlock(Blocks::TubeCoralBlock),
              "all five colours of coral are coral");
        check(!isCoralBlock(Blocks::Prismarine), "and prismarine is not");
    }

    void testDesertWell()
    {
        section("the desert well");

        constexpr uint32_t SEED = 4242u;
        const WorldGen generator(SEED);

        // Not a structure in Minecraft at all: a decorator, one chunk
        // of desert in a thousand. The rarity is what is being tested,
        // because it is the whole character of the thing.
        check(Structures::DESERT_WELL_CHANCE < 0.005f,
              "a desert well is rare enough to be a landmark when you find one");

        int hits = 0;
        constexpr int LOOKED = 60000;
        for (int i = 0; i < LOOKED; ++i)
            if (Structures::spreadAt(i % 400, i / 400, SEED,
                                     Structures::DESERT_WELL_SALT,
                                     Structures::DESERT_WELL_CHANCE))
                ++hits;

        const float rate = static_cast<float>(hits) / static_cast<float>(LOOKED);
        check(rate > Structures::DESERT_WELL_CHANCE * 0.3f &&
              rate < Structures::DESERT_WELL_CHANCE * 3.0f,
              "and it turns up about as often as it is meant to");

        // And one gets built, with water in it. A well without water
        // is a pile of sandstone.
        bool found = false;
        for (int ring = 0; ring <= 90 && !found; ++ring)
            for (int cz = -ring; cz <= ring && !found; ++cz)
                for (int cx = -ring; cx <= ring && !found; ++cx)
                {
                    if (ring > 0 && std::abs(cx) != ring && std::abs(cz) != ring) continue;
                    if (!Structures::spreadAt(cx, cz, SEED,
                                              Structures::DESERT_WELL_SALT,
                                              Structures::DESERT_WELL_CHANCE))
                        continue;

                    const int midX = cx * Chunk::SX + Chunk::SX / 2;
                    const int midZ = cz * Chunk::SZ + Chunk::SZ / 2;
                    if (generator.biomeAt(midX, midZ) != Biome::Desert) continue;
                    if (generator.surfaceHeight(midX, midZ) <= WorldGen::SEA_LEVEL) continue;

                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    const int ground = generator.surfaceHeight(midX, midZ);
                    const int lx = Chunk::SX / 2;
                    const int lz = Chunk::SZ / 2;

                    check(chunk.getBlock(lx, ground, lz) == Blocks::Water,
                          "a desert well has water in the middle of it");
                    check(chunk.getBlock(lx, ground + 4, lz) == Blocks::Sandstone,
                          "and a roof over the top of it");
                    check(chunk.getBlock(lx - 1, ground + 2, lz - 1) == Blocks::Sandstone,
                          "standing on four pillars");
                    found = true;
                }

        check(found, "there is a desert well somewhere in the deserts near the origin");
    }

    void testAncientCity()
    {
        section("the ancient city");

        constexpr uint32_t SEED = 4242u;
        const WorldGen generator(SEED);

        check(Structures::ANCIENT_CITY.spacing == 24 &&
              Structures::ANCIENT_CITY.separation == 8,
              "an ancient city grid is 24 and 8, as Minecraft's is");

        const Structures::Spot spot = generator.nearestAncientCity(0, 0, 6);
        check(spot.exists, "there is one within a few squares of the origin");
        if (!spot.exists) return;

        const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
        const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

        int sculk = 0, bricks = 0, sensors = 0, shriekers = 0, catalysts = 0;
        int lanterns = 0, frame = 0, chests = 0, aboveTheOres = 0;

        for (int cz = spot.chunkZ - 2; cz <= spot.chunkZ + 2; ++cz)
            for (int cx = spot.chunkX - 2; cx <= spot.chunkX + 2; ++cx)
            {
                Chunk chunk(ChunkPos{ cx, cz });
                generator.generate(chunk);

                for (int y = 0; y < Chunk::SY; ++y)
                    for (int lx = 0; lx < Chunk::SX; ++lx)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                        {
                            const BlockId block = chunk.getBlock(lx, y, lz);
                            if (block == Blocks::Chest &&
                                y == WorldGen::CITY_FLOOR + 1) ++chests;
                            if (!isDeepDark(block)) continue;

                            // The whole city lives below the ores. A
                            // block of it up at the surface would mean
                            // the floor height had got away.
                            if (y > WorldGen::CITY_FLOOR + WorldGen::CITY_TALL)
                                ++aboveTheOres;

                            switch (block)
                            {
                                case Blocks::Sculk:          ++sculk; break;
                                case Blocks::SculkShrieker:  ++shriekers; break;
                                case Blocks::SculkCatalyst:  ++catalysts; break;
                                case Blocks::SoulLantern:    ++lanterns; break;
                                case Blocks::ReinforcedDeepslate: ++frame; break;
                                default:
                                    if (isSculkSensor(block)) ++sensors; else ++bricks;
                                    break;
                            }
                        }
            }

        check(bricks > 400, "an ancient city is a great deal of deepslate");
        check(sculk > 200, "with sculk grown over the floor of it");
        check(sensors > 0, "something listening");
        check(shriekers > 0, "and something to tell");
        check(catalysts > 0, "and the thing that spread it all");
        check(lanterns > 0, "lit well enough to see it by");
        check(frame >= 12, "and the frame in the middle that goes nowhere");
        check(chests == 2, "two chests, which is what the walk down is for");
        check(aboveTheOres == 0, "and none of it anywhere near the surface");

        // The chests know what they belong to.
        check(generator.hoardAt(midX - 6, WorldGen::CITY_FLOOR + 1, midZ + 6) ==
                  WorldGen::Hoard::AncientCity,
              "a city chest knows what it belongs to");
        check(generator.hoardAt(midX, WorldGen::CITY_FLOOR + 1, midZ) ==
                  WorldGen::Hoard::None,
              "while the floor beside it belongs to nothing");

        // And the city's blocks are its own: nothing else in the world
        // is built of them, which is what makes isDeepDark a usable
        // test for "am I in one".
        check(isDeepDark(Blocks::Sculk) && isDeepDark(Blocks::ReinforcedDeepslate),
              "the deep dark's blocks are the deep dark's");
        check(!isDeepDark(Blocks::Deepslate),
              "and plain deepslate, which is everywhere, is not one of them");
    }

    void testOtherDimensionStructures()
    {
        section("the End's city and the Nether's bastion");

        constexpr uint32_t SEED = 4242u;

        check(Structures::END_CITY.spacing > Structures::END_CITY.separation,
              "an End city grid has room in it");
        check(Structures::BASTION.spacing == Structures::NETHER_FORTRESS.spacing &&
              Structures::BASTION.salt == Structures::NETHER_FORTRESS.salt,
              "a bastion shares the fortress's grid, as Minecraft's does");

        // The two are told apart by a coin flip on the square, and the
        // flip has to be the same in the search and in the stamp. It
        // was not, the first time: the search walked to a square the
        // stamp had decided was a fortress and found nothing there.
        {
            const WorldGen nether(SEED, Dimension::Nether);
            const Structures::Spot spot = nether.nearestBastion(0, 0, 6);
            check(spot.exists, "there is a bastion within a few squares of the origin");

            if (spot.exists)
            {
                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

                int blackstone = 0, gilded = 0, chests = 0;
                for (int cz = spot.chunkZ - 2; cz <= spot.chunkZ + 2; ++cz)
                    for (int cx = spot.chunkX - 2; cx <= spot.chunkX + 2; ++cx)
                    {
                        Chunk chunk(ChunkPos{ cx, cz });
                        nether.generate(chunk);

                        for (int y = 0; y < Chunk::SY; ++y)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                                for (int lz = 0; lz < Chunk::SZ; ++lz)
                                {
                                    const BlockId block = chunk.getBlock(lx, y, lz);
                                    if (block == Blocks::GildedBlackstone) ++gilded;
                                    else if (isBastionBlock(block)) ++blackstone;
                                    if (block == Blocks::Chest) ++chests;
                                }
                    }

                check(blackstone > 500, "a bastion is a great deal of blackstone");
                check(gilded > 0, "with the gold in it that is the reason to go");
                check(chests >= 2, "and chests worth the trip");

                check(nether.hoardAt(midX - 4, WorldGen::NETHER_LAVA_LEVEL + 7, midZ + 4) ==
                          WorldGen::Hoard::Bastion,
                      "a bastion chest knows what it belongs to");
            }
        }

        // The End city, which needs an island under it -- most of the
        // End is open void, so the grid alone is not enough.
        {
            const WorldGen end(SEED, Dimension::End);
            const Structures::Spot spot = end.nearestEndCity(0, 0, 40);
            check(spot.exists, "there is an End city out on the islands");

            if (spot.exists)
            {
                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

                check(end.endGroundAt(midX, midZ) >= 0,
                      "and it has an island under it rather than void");

                const long long away = static_cast<long long>(midX) * midX +
                                       static_cast<long long>(midZ) * midZ;
                check(away >= 460LL * 460LL,
                      "out past the main island, where Minecraft puts them too");

                int purpur = 0, chorus = 0, chests = 0;
                for (int cz = spot.chunkZ - 1; cz <= spot.chunkZ + 1; ++cz)
                    for (int cx = spot.chunkX - 1; cx <= spot.chunkX + 1; ++cx)
                    {
                        Chunk chunk(ChunkPos{ cx, cz });
                        end.generate(chunk);

                        for (int y = 0; y < Chunk::SY; ++y)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                                for (int lz = 0; lz < Chunk::SZ; ++lz)
                                {
                                    const BlockId block = chunk.getBlock(lx, y, lz);
                                    if (block == Blocks::ChorusPlant ||
                                        block == Blocks::ChorusFlower) ++chorus;
                                    else if (isEndCityBlock(block)) ++purpur;
                                    if (block == Blocks::Chest) ++chests;
                                }
                    }

                check(purpur > 200, "an End city is a tower's worth of purpur");
                check(chorus > 0, "with a chorus plant standing beside it");
                check(chests > 0, "and what you came out here for");
            }
        }

        // Each family belongs to its own structure and nothing else.
        check(isEndCityBlock(Blocks::PurpurBlock) && !isEndCityBlock(Blocks::EndStone),
              "purpur is the city's, plain end stone is the island's");
        check(isBastionBlock(Blocks::Blackstone) && !isBastionBlock(Blocks::NetherBricks),
              "blackstone is the bastion's, nether brick is the fortress's");
    }

    // The invariant that --structureinfo found broken on two seeds out
    // of four: if the search says a structure is here, it is here.
    //
    // This is the bug class that keeps coming back. The search and the
    // stamp each decide whether a place suits a structure, and when
    // those are two separate tests they drift apart -- the bastion's
    // coin flip, the ruined portal below the sea, the End city on the
    // main island were all the same mistake. A search that points at a
    // place nothing was built is worse than no search at all.
    void testSearchAgreesWithStamp()
    {
        section("where the search points, something stands");

        for (uint32_t seed : { 1u, 4242u, 99999u })
        {
            const WorldGen over(seed);
            const std::string which = "seed " + std::to_string(seed);

            for (int k = 0; k < static_cast<int>(WorldGen::Landmark::Count); ++k)
            {
                const WorldGen::Landmark kind = static_cast<WorldGen::Landmark>(k);
                const int squares =
                    kind == WorldGen::Landmark::WoodlandMansion ? 2 : 6;

                const Structures::Spot spot = over.nearestLandmark(kind, 0, 0, squares);
                if (!spot.exists) continue;

                const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
                const std::string who =
                    which + ", " + WorldGen::landmarkName(kind);

                // The search's own test must hold at the place it
                // points, and the stamp must agree to build there.
                check(over.landmarkStands(kind, midX, midZ),
                      who + ": the place it points at suits it");
                check(over.surfaceHeight(midX, midZ) > WorldGen::SEA_LEVEL,
                      who + ": and is out of the sea");

                // And something is actually there.
                int chests = 0;
                for (int cz = spot.chunkZ - 1; cz <= spot.chunkZ + 1; ++cz)
                    for (int cx = spot.chunkX - 1; cx <= spot.chunkX + 1; ++cx)
                    {
                        Chunk chunk(ChunkPos{ cx, cz });
                        over.generate(chunk);
                        for (int y = 0; y < Chunk::SY; ++y)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                                for (int lz = 0; lz < Chunk::SZ; ++lz)
                                    if (chunk.getBlock(lx, y, lz) == Blocks::Chest) ++chests;
                    }

                check(chests > 0, who + ": and it was built there");
            }
        }

        // The End city's main-island rule has to live in the search as
        // well as the stamp, or on some seeds it points at a city on
        // the island you arrive on, where none is built.
        for (uint32_t seed : { 4242u, 99999u })
        {
            const WorldGen end(seed, Dimension::End);
            const Structures::Spot spot = end.nearestEndCity(0, 0, 40);
            if (!spot.exists) continue;

            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

            check(end.endCityStands(midX, midZ),
                  "seed " + std::to_string(seed) +
                      ": the End city search points somewhere one stands");

            const long long away = static_cast<long long>(midX) * midX +
                                   static_cast<long long>(midZ) * midZ;
            check(away >= WorldGen::END_CITY_VOID * WorldGen::END_CITY_VOID,
                  "and out past the island you arrive on");
        }

        // Same for the bastion, whose coin flip was in two places.
        for (uint32_t seed : { 4242u, 99999u })
        {
            const WorldGen nether(seed, Dimension::Nether);
            const Structures::Spot spot = nether.nearestBastion(0, 0, 6);
            if (!spot.exists) continue;

            check(nether.bastionWins(spot.chunkX, spot.chunkZ),
                  "seed " + std::to_string(seed) +
                      ": the bastion search points at a square that drew a bastion");
        }
    }

    void testSwamp()
    {
        section("the swamp, and the hut in it");

        constexpr uint32_t SEED = 4242u;
        const WorldGen generator(SEED);

        // The biome has to own both sides of the waterline. Tested
        // after Ocean and Beach it owned only the one row of columns a
        // block above the sea -- dry land with trees on it, which is a
        // forest. So: a swamp that exists, and one with water in it.
        int swampColumns = 0, wetSwamp = 0, drySwamp = 0;
        for (int wz = -600; wz <= 600; wz += 17)
            for (int wx = -600; wx <= 600; wx += 17)
            {
                if (generator.biomeAt(wx, wz) != Biome::Swamp) continue;
                ++swampColumns;
                if (generator.surfaceHeight(wx, wz) < WorldGen::SEA_LEVEL) ++wetSwamp;
                else ++drySwamp;
            }

        check(swampColumns > 0, "there are swamps in the world");
        check(wetSwamp > 0, "with water standing in them");
        check(drySwamp > 0, "and ground to walk on as well");

        // A swamp is low. If one turned up on a hillside the height
        // band has got away.
        for (int wz = -400; wz <= 400; wz += 23)
            for (int wx = -400; wx <= 400; wx += 23)
                if (generator.biomeAt(wx, wz) == Biome::Swamp)
                    check(generator.surfaceHeight(wx, wz) <= WorldGen::SEA_LEVEL + 2,
                          "no swamp stands more than two blocks above the sea");

        // What grows in one. Walking out to a real swamp and
        // generating it rather than trusting the rules.
        bool looked = false;
        for (int ring = 0; ring <= 40 && !looked; ++ring)
            for (int cz = -ring; cz <= ring && !looked; ++cz)
                for (int cx = -ring; cx <= ring && !looked; ++cx)
                {
                    if (ring > 0 && std::abs(cx) != ring && std::abs(cz) != ring) continue;

                    const int midX = cx * Chunk::SX + 8;
                    const int midZ = cz * Chunk::SZ + 8;
                    if (generator.biomeAt(midX, midZ) != Biome::Swamp) continue;
                    if (generator.surfaceHeight(midX, midZ) >= WorldGen::SEA_LEVEL) continue;

                    int mud = 0, pads = 0, vines = 0, water = 0;
                    for (int dz = -2; dz <= 2; ++dz)
                        for (int dx = -2; dx <= 2; ++dx)
                        {
                            Chunk chunk(ChunkPos{ cx + dx, cz + dz });
                            generator.generate(chunk);

                            for (int y = 0; y < Chunk::SY; ++y)
                                for (int lx = 0; lx < Chunk::SX; ++lx)
                                    for (int lz = 0; lz < Chunk::SZ; ++lz)
                                        switch (chunk.getBlock(lx, y, lz))
                                        {
                                            case Blocks::Mud:     ++mud; break;
                                            case Blocks::LilyPad: ++pads; break;
                                            case Blocks::Vine:    ++vines; break;
                                            case Blocks::Water:   ++water; break;
                                            default: break;
                                        }
                        }

                    check(water > 0, "a swamp has water standing in it");
                    check(mud > 0, "and mud under the water");
                    check(pads > 0, "lily pads floating on it");
                    (void)vines;
                    looked = true;
                }

        check(looked, "there is a wet swamp near the origin to look at");

        // Vines are counted separately from the rest, in the dry part
        // of a swamp rather than the wet. They hang off trees, trees
        // do not grow in standing water, and a swamp is three parts
        // water to one part ground -- so looking for them beside the
        // lily pads was asking the wrong chunks.
        //
        // Walked to off the column scan above rather than by rings out
        // from the origin: for this seed the nearest swamp is fifteen
        // chunks out, and a ring search that stopped short of it
        // reported no vines in the world.
        int vinesAnywhere = 0;
        for (int wz = -900; wz <= 900 && vinesAnywhere == 0; wz += 48)
            for (int wx = -900; wx <= 900 && vinesAnywhere == 0; wx += 48)
            {
                if (generator.biomeAt(wx, wz) != Biome::Swamp) continue;
                if (generator.surfaceHeight(wx, wz) <= WorldGen::SEA_LEVEL) continue;

                const int cx = Structures::squareOf(wx, Chunk::SX);
                const int cz = Structures::squareOf(wz, Chunk::SZ);

                for (int dz = -1; dz <= 1; ++dz)
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        Chunk chunk(ChunkPos{ cx + dx, cz + dz });
                        generator.generate(chunk);

                        for (int y = 0; y < Chunk::SY; ++y)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                                for (int lz = 0; lz < Chunk::SZ; ++lz)
                                    if (isVine(chunk.getBlock(lx, y, lz)))
                                        ++vinesAnywhere;
                    }
            }

        check(vinesAnywhere > 0, "and vines hang off the trees that stand in one");

        // The hut. The last Minecraft structure that had nowhere to go.
        check(Structures::SWAMP_HUT.spacing == 32 &&
              Structures::SWAMP_HUT.separation == 8,
              "a swamp hut grid is 32 and 8, as Minecraft's is");

        const Structures::Spot spot =
            generator.nearestLandmark(WorldGen::Landmark::SwampHut, 0, 0, 8);
        check(spot.exists, "there is a hut within a few squares of the origin");

        if (spot.exists)
        {
            const int midX = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int midZ = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;

            check(generator.biomeAt(midX, midZ) == Biome::Swamp,
                  "and it stands in a swamp, which is the only place one goes");

            int spruce = 0, chests = 0, stands = 0, benches = 0;
            for (int cz = spot.chunkZ - 1; cz <= spot.chunkZ + 1; ++cz)
                for (int cx = spot.chunkX - 1; cx <= spot.chunkX + 1; ++cx)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    for (int y = 0; y < Chunk::SY; ++y)
                        for (int lx = 0; lx < Chunk::SX; ++lx)
                            for (int lz = 0; lz < Chunk::SZ; ++lz)
                                switch (chunk.getBlock(lx, y, lz))
                                {
                                    case Blocks::SprucePlanks:
                                    case Blocks::SpruceLog:     ++spruce; break;
                                    case Blocks::Chest:         ++chests; break;
                                    case Blocks::BrewingStand:  ++stands; break;
                                    case Blocks::CraftingTable: ++benches; break;
                                    default: break;
                                }
                }

            check(spruce > 40, "a hut is a shack's worth of spruce");
            check(chests > 0, "with a chest in it");
            check(stands > 0, "the stand she brews at");
            check(benches > 0, "and a bench");
        }

        // And the witch, who lives in it and nowhere else.
        const MobType& witch = mobType(MobId::Witch);
        check(std::string(witch.name) == "Witch", "the witch is in the table");
        check(witch.arrowRange > 0.0f && witch.arrowInterval > 0.0f,
              "and throws rather than closes, which is how a witch fights");
    }

    // Every block and every mob has to be reachable by somebody.
    //
    // This is the test that was missing. Everything else asks whether
    // a thing is correct; nothing asked whether it was there. The
    // three swamp blocks sat textured, named and unreachable for a
    // week because no test could tell the difference between a block
    // nobody has got round to generating and a block that is crafted.
    void testEverythingIsReachable()
    {
        section("everything in the game can be got at");

        // The sponge is the one the audit actually caught. A monument
        // has wet sponge in it and nothing dried it, so the dry block
        // existed with no route into anybody's hands.
        check(Smelting::resultOf(Blocks::WetSponge) == Blocks::Sponge,
              "a wet sponge dries into a dry one, which is Minecraft's recipe");
        check(Smelting::resultOf(Blocks::Clay) == Blocks::Terracotta && Smelting::resultOf(Items::ClayBall) == Items::Brick,
              "a clay block fires into terracotta and a clay ball into a brick");

        // The glow berries are the one the item side of the audit
        // caught. Food.cpp had listed them as edible and this file had
        // checked that they were -- but mining the vine gave you the
        // block, so the one food that grows underground could be
        // picked up and never eaten.
        check(blockDrop(Blocks::GlowBerries) == Items::GlowBerries,
              "glow berries come off the vine as something you can eat");
        check(Food::isEdible(blockDrop(Blocks::GlowBerries)),
              "and what comes off is edible, which is the whole point of them");

        // The general rule behind it: anything edible has to have a
        // way of reaching a hand. A food nobody can hold is a food
        // nobody can eat.
        for (int i = static_cast<int>(Items::FIRST); i < static_cast<int>(Items::Count); ++i)
        {
            const StackId id = static_cast<StackId>(i);
            if (!Food::isEdible(id)) continue;

            bool obtainable = false;
            for (int b = 0; isBlockId(b) && !obtainable; b = nextBlockId(b))
            {
                const BlockId block = static_cast<BlockId>(b);
                if (blockDrop(block) == id) obtainable = true;

                // A crop does not drop what it is; it drops what it
                // grew, and blockDrop cannot say that. Left out, the
                // beetroot read as a food nobody could hold -- which
                // this test caught the day it was added.
                if (!isCrop(block)) continue;
                for (uint32_t roll = 0; roll < 4 && !obtainable; ++roll)
                {
                    const Farming::Harvest cut = Farming::harvestOf(block, roll);
                    if (cut.firstCount > 0 && cut.first == id) obtainable = true;
                    if (cut.secondCount > 0 && cut.second == id) obtainable = true;
                }
            }
            for (int m = 0; m < mobTypeCount() && !obtainable; ++m)
            {
                const MobType& type = mobType(static_cast<MobId>(m));
                if (type.drop == id || type.secondDrop == id) obtainable = true;
            }
            for (StackId made : Crafting::everyOutput())
                if (made == id) obtainable = true;
            for (int k = 0; k < static_cast<int>(Items::Count) && !obtainable; ++k)
                if (Smelting::resultOf(static_cast<StackId>(k)) == id) obtainable = true;
            for (uint32_t seed = 0; seed < 2000 && !obtainable; ++seed)
                if (Fishing::roll(seed, true, true).stack.id == id) obtainable = true;

            // And what is in a chest somewhere. This was missing, and
            // the apple found it: its other route is a chance drop off
            // oak leaves, which is not something blockDrop can say, so
            // by this test's old reckoning a food you can pick up in
            // any village was unobtainable.
            for (Village::Kind kind : { Village::Kind::Home, Village::Kind::Blacksmith,
                                        Village::Kind::Library, Village::Kind::Butcher })
                for (int n = 0; n < 30 && !obtainable; ++n)
                    for (const VillageLoot::Stack& stack :
                             VillageLoot::rollFor(kind, n * 5, 70, n * 3, 4242u))
                        if (stack.id == id) obtainable = true;

            for (int h = 0; h <= static_cast<int>(WorldGen::Hoard::TrialChamber) &&
                            !obtainable; ++h)
                for (int n = 0; n < 30 && !obtainable; ++n)
                    for (const StructureLoot::Stack& stack : StructureLoot::rollFor(
                             static_cast<WorldGen::Hoard>(h), n * 7, 50, n * 11, 4242u))
                        if (stack.id == id) obtainable = true;

            check(obtainable,
                  std::string(displayName(id)) + " can be got hold of, not just eaten");
        }

        // Every mob has to be meetable. A species in the table that
        // is in no spawn list is a model nobody ever sees.
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobId id = static_cast<MobId>(i);
            const MobType& type = mobType(id);
            const std::string who = type.name;

            // Boats and minecarts ride in the mob table because they
            // are entities with models, but nothing spawns them.
            if (isBoatMob(id) || id == MobId::Minecart) continue;

            bool meetable = false;

            // The night list, the Nether's, and the End's.
            for (int k = 0; k < EntityManager::nightSpeciesCount(); ++k)
                if (EntityManager::nightSpecies(k) == id) meetable = true;

            // And the two other worlds' own lists, asked of the game
            // rather than copied into the switch below. A ghast was
            // added to the Nether's list and this test still called
            // it unmeetable, which is the test being out of date
            // rather than the game being wrong.
            for (int k = 0; k < EntityManager::netherSpeciesCount(); ++k)
                if (EntityManager::netherSpecies(k) == id) meetable = true;
            for (int k = 0; k < EntityManager::endSpeciesCount(); ++k)
                if (EntityManager::endSpecies(k) == id) meetable = true;
            for (int k = 0; k < EntityManager::caveSpeciesCount(); ++k)
                if (EntityManager::caveSpecies(k) == id) meetable = true;
            for (int k = 0; k < EntityManager::seaSpeciesCount(); ++k)
                if (EntityManager::seaSpecies(k) == id) meetable = true;

            // And what each biome has of its own, by day and after
            // dark, asked of the tables the spawner reads rather than
            // copied out of them.
            for (int biome = 0; biome < Biomes::COUNT; ++biome)
                for (int step = 0; step < 200; ++step)
                {
                    const Biomes::Kind where = static_cast<Biomes::Kind>(biome);
                    const float roll = static_cast<float>(step) / 200.0f;
                    if (EntityManager::pickSpawn(where, Biomes::Group::Creature, roll) == id) meetable = true;
                    if (EntityManager::pickSpawn(where, Biomes::Group::Monster, roll) == id) meetable = true;
                }

            // What a cage sends out.
            for (Dimension where : { Dimension::Overworld, Dimension::Nether, Dimension::End })
                if (cageSpecies(where) == id) meetable = true;

            // The ones that belong to one structure each, and the ones
            // the world puts down by hand.
            switch (id)
            {
                case MobId::Sheep: case MobId::Pig: case MobId::Cow: case MobId::Chicken:
                case MobId::Wolf:  case MobId::Horse:
                case MobId::Villager: case MobId::IronGolem: case MobId::Cat: case MobId::Camel:
                case MobId::ZombiePigman: case MobId::WitherSkeleton:
                case MobId::MagmaCube: case MobId::Blaze:
                case MobId::Breeze:
                case MobId::Guardian: case MobId::Warden: case MobId::Witch:
                // The shulker belongs to an End city and the drowned
                // to the sea, and both are put down by hand where
                // they belong rather than by the night spawner.
                case MobId::Shulker: case MobId::Drowned:
                // The frog belongs to the waterside, and is put down
                // there by hand rather than by the night spawner.
                case MobId::Frog:
                // And the squid, which belongs to open water.
                case MobId::Squid: case MobId::GlowSquid:
                case MobId::Bee:
                case MobId::Zoglin: case MobId::Illusioner: case MobId::TraderLlama:
                // A small slime is never spawned: it is what a big
                // one leaves behind when it is killed.
                case MobId::SlimeSmall:
                // The brute belongs to a bastion and is put there by
                // hand, the way a shulker belongs to an End city.
                case MobId::PiglinBrute:
                // A vex is what an evoker brings with it, so it comes
                // out of the mansion beside its master.
                case MobId::Vex:
                // A snow golem is built rather than spawned: two snow
                // blocks and a pumpkin on top.
                case MobId::SnowGolem:
                // The monument's own, and the allay in its cage at
                // the outpost.
                case MobId::ElderGuardian:
                case MobId::Allay:
                // A sniffer hatches from an egg brushed out of a warm
                // ocean ruin; nothing spawns one.
                case MobId::Sniffer:
                // A tadpole hatches from frogspawn a pair of frogs laid.
                case MobId::Tadpole:
                // A wandering trader belongs nowhere, which is why he
                // turns up anywhere in daylight.
                case MobId::WanderingTrader:
                // The illagers belong to the two buildings that were
                // built for them: the outpost and the mansion.
                case MobId::Pillager: case MobId::Vindicator:
                case MobId::Evoker:   case MobId::Ravager:
                case MobId::EnderDragon:
                case MobId::Wither:
                case MobId::Mule:
                case MobId::SkeletonHorse:
                case MobId::ZombieHorse:
                case MobId::Giant:
                    meetable = true;
                    break;
                default:
                    break;
            }

            check(meetable, who + " can be met somewhere in the world");
            check(type.voice != type.hurtVoice && type.hurtVoice != type.deathVoice,
                  who + " has three voices of its own");
        }

        // And the three that belong to a structure each really are
        // tied to one, rather than wandering in with the night.
        for (int k = 0; k < EntityManager::nightSpeciesCount(); ++k)
        {
            const MobId id = EntityManager::nightSpecies(k);
            check(id != MobId::Warden, "a warden never turns up with the night");
            check(id != MobId::Guardian, "nor a guardian");
        }
    }

    void testWitchPotions()
    {
        section("the witch throws, and what she throws breaks");

        const MobType& witch = mobType(MobId::Witch);
        check(witch.throwsPotions, "a witch throws potions");
        check(!witch.shootsFire && !witch.shootsWind,
              "and neither fire nor wind, which are somebody else's");
        check(witch.arrowDamage > 0 && witch.arrowDamage <= 4,
              "the throw itself barely hurts -- what lands on you is the point");

        // The two things a projectile can be, which used to be one
        // test. A potion arcs like an arrow and breaks like a charge;
        // while flies() answered both, a thrown potion stuck in the
        // wall like a dart and never went off.
        Arrow shaft;  shaft.kind = Arrows::Kind::Shaft;
        Arrow fire;   fire.kind = Arrows::Kind::Fire;
        Arrow wind;   wind.kind = Arrows::Kind::Wind;
        Arrow potion; potion.kind = Arrows::Kind::Potion;

        check(!shaft.flies() && !shaft.bursts(), "an arrow arcs, and sticks where it lands");
        check(fire.flies() && fire.bursts(), "a fire charge flies flat and breaks");
        check(wind.flies() && wind.bursts(), "so does a wind charge");
        check(!potion.flies(), "a potion arcs rather than flying flat");
        check(potion.bursts(), "but it breaks rather than sticking, which is the whole point");

        // And it is drawn as something, rather than as whatever tile
        // index zero happens to be.
        check(tileUV(Tiles::ItemPotion).u1 > tileUV(Tiles::ItemPotion).u0,
              "and there is a bottle to draw it with");
    }

    void testDragonFight()
    {
        section("the End island, and the dragon over it");

        constexpr uint32_t SEED = 4242u;
        const WorldGen end(SEED, Dimension::End);

        // Ten pillars in a ring, which is Minecraft's count.
        check(WorldGen::END_PILLARS == 10, "ten pillars, as Minecraft has");

        std::vector<std::pair<int,int>> where;
        for (int i = 0; i < WorldGen::END_PILLARS; ++i)
        {
            int px = 0, pz = 0, tall = 0;
            WorldGen::endPillar(i, px, pz, tall);

            check(tall >= 12 && tall <= 22, "every pillar is between twelve and twenty-two tall");

            const float out = std::sqrt(static_cast<float>(px * px + pz * pz));
            check(out > WorldGen::END_PILLAR_RING - 2.0f &&
                  out < WorldGen::END_PILLAR_RING + 2.0f,
                  "and stands on the ring rather than wherever it likes");

            // No two in the same place: a ring of ten that was really
            // six would be hard to see and easy to miss.
            for (const auto& seen : where)
                check(seen.first != px || seen.second != pz,
                      "no two pillars stand in the same place");
            where.push_back({ px, pz });
        }

        // They are not all the same height, or the ring is a fence.
        int shortest = 99, tallest = 0;
        for (int i = 0; i < WorldGen::END_PILLARS; ++i)
        {
            int px = 0, pz = 0, tall = 0;
            WorldGen::endPillar(i, px, pz, tall);
            shortest = std::min(shortest, tall);
            tallest = std::max(tallest, tall);
        }
        check(tallest > shortest, "and they are not all the same height");

        // Built, with a crystal on each. Generated for real rather
        // than trusting the arithmetic that placed them.
        int obsidian = 0, crystals = 0, bases = 0;
        for (int cz = -4; cz <= 4; ++cz)
            for (int cx = -4; cx <= 4; ++cx)
            {
                Chunk chunk(ChunkPos{ cx, cz });
                end.generate(chunk);

                for (int y = 0; y < Chunk::SY; ++y)
                    for (int lx = 0; lx < Chunk::SX; ++lx)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            switch (chunk.getBlock(lx, y, lz))
                            {
                                case Blocks::Obsidian:       ++obsidian; break;
                                case Blocks::EndCrystal:     ++crystals; break;
                                case Blocks::EndCrystalBase: ++bases; break;
                                default: break;
                            }
            }

        check(obsidian > 500, "the pillars are built, and out of obsidian");
        check(crystals == WorldGen::END_PILLARS,
              "with exactly one crystal on each -- no more, and none missing");
        check(bases == WorldGen::END_PILLARS, "each standing on its own slab");

        // The dragon itself.
        const MobType& dragon = mobType(MobId::EnderDragon);
        check(dragon.boss, "the dragon is a boss");
        check(dragon.maxHealth == 200, "with Minecraft's two hundred health");
        check(dragon.hoverHeight > 4.0f, "and it never lands");
        check(dragon.experience > 100, "it is worth the fight");
        check(standsAt(dragon) > 4.0f, "and it is drawn big");

        // Nothing else in the world is a boss, or the bar means
        // nothing.
        int bosses = 0;
        for (int i = 0; i < mobTypeCount(); ++i)
            if (mobType(static_cast<MobId>(i)).boss) ++bosses;
        check(bosses >= 1, "and it is one");

        // It is not on the night list: it belongs to the island.
        for (int k = 0; k < EntityManager::nightSpeciesCount(); ++k)
            check(EntityManager::nightSpecies(k) != MobId::EnderDragon,
                  "a dragon never turns up with the night");

        // Healing is what makes the crystals the fight. Ten of them
        // put back more than most weapons take off, so going for the
        // pillars is not a tactic -- it is the only way through.
        Mob mob(MobId::EnderDragon, glm::vec3(0.0f, 80.0f, 0.0f), 4242u);
        const int full = mob.health();
        mob.damage(60, glm::vec3(0.0f, 0.0f, 1.0f));
        check(mob.health() < full, "a dragon can be hurt");

        const int hurt = mob.health();
        mob.heal(10);
        check(mob.health() == hurt + 10, "and healed by what the crystals put back");

        mob.heal(10000);
        check(mob.health() == full, "but never past what it started with");
    }

    void testWither()
    {
        section("the wither, and the beacon it pays for");

        const MobType& wither = mobType(MobId::Wither);
        check(wither.boss, "the wither is a boss");
        check(wither.maxHealth == 300, "with Minecraft's three hundred health");
        check(wither.hoverHeight > 0.0f, "it never lands");
        check(wither.arrowDamage > 0 && wither.arrowRange > 15.0f,
              "and it throws from a long way off rather than closing");
        check(wither.drop == Items::NetherStar, "it leaves a nether star");

        // Two bosses now, and no more. The dragon test asserted there
        // was exactly one; both are checked here so neither can be
        // quietly joined by a third.
        int bosses = 0;
        for (int i = 0; i < mobTypeCount(); ++i)
            if (mobType(static_cast<MobId>(i)).boss) ++bosses;
        check(bosses == 2, "there are two bosses in the game, and only two");

        // The skull, which is the only way to build one.
        const MobType& skeleton = mobType(MobId::WitherSkeleton);
        check(skeleton.secondDrop == Blocks::WitherSkull,
              "a wither skeleton drops the skull you build it from");

        // Neither boss turns up with the night.
        for (int k = 0; k < EntityManager::nightSpeciesCount(); ++k)
        {
            check(EntityManager::nightSpecies(k) != MobId::Wither,
                  "a wither is built, never met");
            check(EntityManager::nightSpecies(k) != MobId::EnderDragon,
                  "and a dragon belongs to its island");
        }

        // The star is good for one thing, and that thing exists.
        bool makesBeacon = false;
        for (StackId made : Crafting::everyOutput())
            if (made == Blocks::Beacon) makesBeacon = true;
        check(makesBeacon, "a beacon can be made");

        bool wantsStar = false;
        for (StackId need : Crafting::everyIngredient())
            if (need == Items::NetherStar) wantsStar = true;
        check(wantsStar, "and the star is what it is made from");

        // A beacon is a light you built: if it were dark it would be
        // an expensive block of glass.
        check(lightEmission(Blocks::Beacon) > 12,
              "a beacon lights the place it stands in");

        // The skull is a block you place, so it has to be placeable
        // and breakable like one.
        check(blockInfo(Blocks::WitherSkull).hardness > 0.0f,
              "the skull can be broken once it is placed");
        check(blockDrop(Blocks::WitherSkull) == Blocks::WitherSkull,
              "and what comes up is the skull again, so a failed build is not lost");
    }

    void testBeacon()
    {
        section("the beacon, and the pyramid under it");

        // A world made of a map, so a pyramid can be measured without
        // anything to stand it in.
        struct Bench : Beacon::Ground
        {
            std::map<std::array<int,3>, BlockId> blocks;
            BlockId at(int x, int y, int z) const override
            {
                const auto it = blocks.find({ x, y, z });
                return it == blocks.end() ? Blocks::Air : it->second;
            }
            void lay(int tiers, BlockId of)
            {
                for (int tier = 1; tier <= tiers; ++tier)
                    for (int dx = -tier; dx <= tier; ++dx)
                        for (int dz = -tier; dz <= tier; ++dz)
                            blocks[{ dx, -tier, dz }] = of;
            }
        };

        // Nothing under it is worth nothing. A beacon on the ground is
        // a lamp, which is the whole reason the pyramid is interesting.
        {
            Bench bench;
            check(Beacon::tiersUnder(bench, 0, 0, 0) == 0,
                  "a beacon standing on nothing has no pyramid");
            check(Beacon::rangeFor(0) == 0, "and reaches nobody");
        }

        // Each complete layer counts.
        for (int built = 1; built <= Beacon::MAX_TIERS; ++built)
        {
            Bench bench;
            bench.lay(built, Blocks::IronBlock);
            check(Beacon::tiersUnder(bench, 0, 0, 0) == built,
                  "a pyramid of " + std::to_string(built) + " counts as " +
                      std::to_string(built));
            check(Beacon::rangeFor(built) > Beacon::rangeFor(built - 1),
                  "and reaches further than one layer smaller");
        }

        // One block missing from the top layer and the whole thing is
        // worth nothing: a pyramid is complete or it is not.
        {
            Bench bench;
            bench.lay(3, Blocks::GoldBlock);
            bench.blocks[{ 1, -1, 1 }] = Blocks::Air;
            check(Beacon::tiersUnder(bench, 0, 0, 0) == 0,
                  "one gap in the first layer and the pyramid counts for nothing");
        }

        // A gap lower down stops the count there rather than being
        // ignored -- a three-tier pyramid with no bottom is a
        // two-tier pyramid.
        {
            Bench bench;
            bench.lay(3, Blocks::DiamondBlock);
            bench.blocks[{ 0, -3, 0 }] = Blocks::Air;
            check(Beacon::tiersUnder(bench, 0, 0, 0) == 2,
                  "and a gap in the bottom layer costs that layer, not the ones above");
        }

        // Only the four metals count. Coal is a storage block and not
        // good enough, which is Minecraft's joke and this one's.
        {
            Bench bench;
            bench.lay(2, Blocks::CoalBlock);
            check(Beacon::tiersUnder(bench, 0, 0, 0) == 0,
                  "a pyramid of coal is worth nothing");
        }
        check(isBeaconBase(Blocks::IronBlock) && isBeaconBase(Blocks::GoldBlock) &&
              isBeaconBase(Blocks::DiamondBlock) && isBeaconBase(Blocks::EmeraldBlock),
              "iron, gold, diamond and emerald all build one");
        check(!isBeaconBase(Blocks::CoalBlock) && !isBeaconBase(Blocks::Stone),
              "and coal and stone do not");

        // It wants to see the sky, which is what stops one working in
        // a cellar.
        {
            Bench bench;
            bench.lay(1, Blocks::IronBlock);
            check(Beacon::seesSky(bench, 0, 0, 0, 128), "a beacon under open sky works");
            bench.blocks[{ 0, 9, 0 }] = Blocks::Stone;
            check(!Beacon::seesSky(bench, 0, 0, 0, 128),
                  "and one with a roof over it does not");
        }

        // The storage blocks pack and unpack. Nine in, one out, nine
        // back: a block that could not be opened again would be a
        // trap rather than a tidy.
        struct Pair { StackId item; BlockId block; };
        const Pair PAIRS[] = {
            { Items::IronIngot, Blocks::IronBlock },
            { Items::GoldIngot, Blocks::GoldBlock },
            { Items::Diamond,   Blocks::DiamondBlock },
            { Items::Emerald,   Blocks::EmeraldBlock },
            { Items::Coal,      Blocks::CoalBlock },
        };

        for (const Pair& pair : PAIRS)
        {
            ItemStack grid[9];
            for (ItemStack& slot : grid) { slot.id = pair.item; slot.count = 1; }

            const CraftOutput packed = Crafting::match(grid, 3);
            check(packed.id == pair.block && packed.count == 1,
                  std::string(displayName(pair.block)) + ": nine of them pack into one");

            ItemStack back[9];
            back[0].id = pair.block;
            back[0].count = 1;
            const CraftOutput loose = Crafting::match(back, 3);
            check(loose.id == pair.item && loose.count == 9,
                  std::string(displayName(pair.block)) + ": and one comes back as nine");
        }
    }

    void testRedstoneParts()
    {
        section("redstone: the note block and the observer");

        // A note block answers to power like a lamp does.
        check(Redstone::isSink(Blocks::NoteBlock), "a note block answers to power");
        check(Redstone::lit(Blocks::NoteBlock) == Blocks::NoteBlockLit,
              "and has a struck state");
        check(Redstone::unlit(Blocks::NoteBlockLit) == Blocks::NoteBlock,
              "which it comes back out of");

        // An observer is a source while it is flashing, and nothing
        // when it is not. That is the whole of what makes it useful:
        // it turns a change into a signal.
        check(Redstone::isSource(Blocks::ObserverLit),
              "an observer mid-flash gives power out");
        check(!Redstone::isSource(Blocks::Observer),
              "and an observer at rest gives none");
        check(Redstone::lit(Blocks::Observer) == Blocks::ObserverLit &&
              Redstone::unlit(Blocks::ObserverLit) == Blocks::Observer,
              "the two states are each other's opposite");

        // A flash that never ended would be a lever.
        check(Redstone::OBSERVER_PULSE > 0.0f && Redstone::OBSERVER_PULSE < 1.0f,
              "an observer's flash is brief, and it does end");

        // And it drives a circuit. Laid out in a plain array: an
        // observer, a run of dust, and a lamp at the end of it.
        {
            struct Bench : Redstone::Cells
            {
                std::map<std::array<int,3>, BlockId> blocks;
                BlockId at(int x, int y, int z) const override
                {
                    const auto it = blocks.find({ x, y, z });
                    return it == blocks.end() ? Blocks::Air : it->second;
                }
                void set(int x, int y, int z, BlockId b) override
                {
                    blocks[{ x, y, z }] = b;
                }
            } bench;

            for (int x = 0; x < 8; ++x)
            {
                bench.blocks[{ x, 0, 0 }] = Blocks::Stone;
                bench.blocks[{ x, 1, 0 }] = Blocks::RedstoneDust;
            }
            bench.blocks[{ 8, 0, 0 }] = Blocks::Stone;
            bench.blocks[{ 8, 1, 0 }] = Blocks::RedstoneLamp;

            // At rest the lamp is dark.
            bench.blocks[{ -1, 1, 0 }] = observerFacingTo(3, false);
            Redstone::update(bench, 0, 1, 0);
            check(bench.at(8, 1, 0) == Blocks::RedstoneLamp,
                  "an observer at rest lights nothing");

            bench.blocks[{ -1, 1, 0 }] = observerFacingTo(0, true);
            Redstone::update(bench, 0, 1, 0);
            check(bench.at(8, 1, 0) == Blocks::RedstoneLamp,
                  "a flashing observer turned the other way lights nothing beside it");

            // Flashing, it lights the whole run.
            bench.blocks[{ -1, 1, 0 }] = observerFacingTo(3, true);
            Redstone::update(bench, 0, 1, 0);
            check(bench.at(8, 1, 0) == Blocks::RedstoneLampLit,
                  "and an observer flashing lights a lamp eight blocks away");

            // And a note block on the same run is struck too.
            bench.blocks[{ 4, 1, 0 }] = Blocks::NoteBlock;
            Redstone::update(bench, 4, 1, 0);
            check(bench.at(4, 1, 0) == Blocks::NoteBlockLit,
                  "a note block on a live wire is struck");
        }

        {
            struct Stair : Redstone::Cells
            {
                std::map<std::array<int,3>, BlockId> blocks;
                BlockId at(int x, int y, int z) const override
                {
                    const auto it = blocks.find({ x, y, z });
                    return it == blocks.end() ? (y <= 0 ? Blocks::Stone : Blocks::Air) : it->second;
                }
                void set(int x, int y, int z, BlockId b) override { blocks[{ x, y, z }] = b; }
            } stair;

            stair.blocks[{ -1, 1, 0 }] = Blocks::RedstoneTorch;
            stair.blocks[{ 0, 1, 0 }] = Blocks::RedstoneDust;
            stair.blocks[{ 1, 1, 0 }] = Blocks::Stone;
            stair.blocks[{ 1, 2, 0 }] = Blocks::RedstoneDust;
            stair.blocks[{ 2, 2, 0 }] = Blocks::RedstoneDust;
            stair.blocks[{ 3, 1, 0 }] = Blocks::RedstoneDust;
            stair.blocks[{ 4, 1, 0 }] = Blocks::RedstoneLamp;
            Redstone::update(stair, 0, 1, 0);
            check(stair.at(1, 2, 0) == Blocks::RedstoneDustLit, "dust climbs the side of a block");
            check(stair.at(3, 1, 0) == Blocks::RedstoneDustLit, "and runs back down the far side");
            check(stair.at(4, 1, 0) == Blocks::RedstoneLampLit, "and lights what it reaches");

            stair.blocks[{ 0, 2, 0 }] = Blocks::Stone;
            stair.blocks[{ 1, 2, 0 }] = Blocks::RedstoneDust;
            stair.blocks[{ 2, 2, 0 }] = Blocks::RedstoneDust;
            stair.blocks[{ 3, 1, 0 }] = Blocks::RedstoneDust;
            Redstone::update(stair, 0, 1, 0);
            check(stair.at(1, 2, 0) == Blocks::RedstoneDust, "but not with a block over it");
        }
    }

    void testHopper()
    {
        section("hoppers, droppers and dispensers");

        // A bench: blocks in one map, their contents in another, wired
        // to the hopper rule. No world, no chunks.
        struct Bench : Hopper::Holder
        {
            std::map<std::array<int,3>, BlockId> blocks;
            std::map<std::array<int,3>, std::array<ItemStack, 27>> held;

            BlockId at(int x, int y, int z) const override
            {
                const auto it = blocks.find({ x, y, z });
                return it == blocks.end() ? Blocks::Air : it->second;
            }

            ItemStack* slotsAt(int x, int y, int z, int& count) override
            {
                count = Hopper::slotsOf(at(x, y, z));
                if (count <= 0) return nullptr;
                return held[{ x, y, z }].data();
            }

            int countOf(int x, int y, int z, StackId id)
            {
                int total = 0;
                for (const ItemStack& slot : held[{ x, y, z }])
                    if (slot.id == id) total += slot.count;
                return total;
            }
        };

        // How many slots each kind keeps. A hopper having fewer than a
        // chest is not a detail: a five-slot hopper fills and stops,
        // which is how a sorter is built.
        check(Hopper::slotsOf(Blocks::Hopper) == Hopper::HOPPER_SLOTS,
              "a hopper keeps five slots");
        check(Hopper::slotsOf(Blocks::Chest) > Hopper::slotsOf(Blocks::Hopper),
              "and a chest keeps more than a hopper");
        check(Hopper::slotsOf(Blocks::Stone) == 0, "while stone keeps nothing");

        // A chest, a hopper under it, a chest under that. One item
        // should walk all the way down.
        {
            Bench bench;
            bench.blocks[{ 0, 2, 0 }] = Blocks::Chest;
            bench.blocks[{ 0, 1, 0 }] = Blocks::Hopper;
            bench.blocks[{ 0, 0, 0 }] = Blocks::Chest;

            bench.held[{ 0, 2, 0 }][0].id = Blocks::Cobblestone;
            bench.held[{ 0, 2, 0 }][0].count = 3;

            // Pulled down one at a time, not all at once.
            check(Hopper::step(bench, 0, 1, 0), "a hopper under a full chest moves something");
            check(bench.countOf(0, 2, 0, Blocks::Cobblestone) == 2,
                  "and takes exactly one");

            // Enough steps for all three to reach the bottom.
            for (int i = 0; i < 8; ++i) Hopper::step(bench, 0, 1, 0);

            check(bench.countOf(0, 2, 0, Blocks::Cobblestone) == 0, "the top chest empties");
            check(bench.countOf(0, 0, 0, Blocks::Cobblestone) == 3,
                  "and everything in it arrives at the bottom");
        }

        {
            Bench bench;
            bench.blocks[{ 0, 0, 0 }] = hopperFacingTo(1);
            bench.blocks[{ 1, 0, 0 }] = Blocks::Chest;
            bench.held[{ 0, 0, 0 }][0].id = Blocks::Sand;
            bench.held[{ 0, 0, 0 }][0].count = 2;
            Hopper::step(bench, 0, 0, 0);
            check(bench.countOf(1, 0, 0, Blocks::Sand) == 1 && bench.countOf(0, 0, 0, Blocks::Sand) == 1,
                  "a hopper laid on its side feeds the chest it points at");
            check(isHopper(hopperFacingTo(3)) && hopperFacing(hopperFacingTo(3)) == 3 && axisBase(hopperFacingTo(2)) == Blocks::Hopper &&
                  blockDrop(hopperFacingTo(0)) == static_cast<StackId>(Blocks::Hopper), "and breaks back into a plain hopper");
        }

        {
            bool turns = true;
            for (int f = 0; f < 6; ++f)
            {
                turns = turns && isDispenser(dispenserFacingTo(f)) && sixFacing(dispenserFacingTo(f)) == f &&
                        isDropper(dropperFacingTo(f)) && sixFacing(dropperFacingTo(f)) == f &&
                        isObserver(observerFacingTo(f, false)) && !observerLit(observerFacingTo(f, false)) &&
                        observerLit(observerFacingTo(f, true)) && sixFacing(observerFacingTo(f, true)) == f;
                turns = turns && !isDropper(dispenserFacingTo(f)) && !isDispenser(dropperFacingTo(f));
            }
            check(turns, "dispensers, droppers and observers face all six ways");
            check(axisBase(dispenserFacingTo(2)) == Blocks::Dispenser && axisBase(dropperFacingTo(5)) == Blocks::Dropper &&
                  blockDrop(observerFacingTo(1, true)) == static_cast<StackId>(Blocks::Observer),
                  "and break back into the plain block");
            check(sixFacing(Blocks::Dispenser) == 4 && sixFacing(Blocks::Observer) == 4, "the old blocks still face up");
            check(Redstone::lit(observerFacingTo(3, false)) == observerFacingTo(3, true) &&
                  Redstone::unlit(observerFacingTo(3, true)) == observerFacingTo(3, false) &&
                  Redstone::isSource(observerFacingTo(0, true)) && !Redstone::isSource(observerFacingTo(0, false)),
                  "a turned observer lights and goes out facing the same way");
            check(Hopper::slotsOf(Blocks::DecoratedPotFirst) == 1, "a hopper can fill a decorated pot");
            check(Hopper::slotsOf(barrelWith(0, false)) == 27 && Hopper::slotsOf(Blocks::ShulkerBox) == 27, "and a barrel or a shulker box");
            turns = true;
            for (int wear = 0; wear < 3; ++wear)
                for (bool alongX : { false, true })
                    turns = turns && isAnvil(anvilOf(wear, alongX)) && anvilWear(anvilOf(wear, alongX)) == wear &&
                            anvilAlongX(anvilOf(wear, alongX)) == alongX && blockDrop(anvilOf(wear, alongX)) == blockDrop(anvilOf(wear, false));
            check(turns && anvilOf(3, true) == Blocks::Air, "an anvil turned either way keeps its wear and breaks past the last");
            turns = true;
            for (int face = 0; face < 3; ++face)
                for (int facing = 0; facing < 4; ++facing)
                    turns = turns && isGrindstone(grindstoneOf(face, facing)) && grindstoneFace(grindstoneOf(face, facing)) == face &&
                            grindstoneFacing(grindstoneOf(face, facing)) == facing && blockDrop(grindstoneOf(face, facing)) == static_cast<StackId>(Blocks::Grindstone);
            check(turns, "a grindstone stands, hangs or sits on a wall facing any way");
            turns = true;
            for (int facing = 0; facing < 4; ++facing)
                for (bool book : { false, true })
                    turns = turns && isLectern(lecternWith(facing, book)) && lecternFacing(lecternWith(facing, book)) == facing &&
                            lecternHasBook(lecternWith(facing, book)) == book && blockDrop(lecternWith(facing, book)) == static_cast<StackId>(Blocks::Lectern);
            check(turns, "a lectern faces any way with or without its book");
            turns = true;
            for (int facing = 0; facing < 6; ++facing)
                for (bool open : { false, true })
                    turns = turns && isBarrel(barrelWith(facing, open)) && barrelFacing(barrelWith(facing, open)) == facing &&
                            barrelOpen(barrelWith(facing, open)) == open && isContainerBlock(barrelWith(facing, open)) &&
                            blockDrop(barrelWith(facing, open)) == static_cast<StackId>(Blocks::Barrel);
            check(turns, "a barrel faces any of six ways, open or shut");
            turns = true;
            for (int facing = 0; facing < 4; ++facing)
                for (int stage = 0; stage < 3; ++stage)
                    turns = turns && isCocoa(cocoaWith(facing, stage)) && cocoaFacing(cocoaWith(facing, stage)) == facing &&
                            cocoaStage(cocoaWith(facing, stage)) == stage && blockDrop(cocoaWith(facing, stage)) == static_cast<StackId>(Items::CocoaBeans);
            check(turns, "a cocoa pod hangs off any side of a trunk at any ripeness");
            {
                const BlockId hung = doorHingeBase(Blocks::DoorLowShutNorth);
                const BlockId part = doorPart(hung, 2, true, true);
                check(isDoor(part) && doorHinged(part) && blockFacing(part) == 2 && doorIsTop(part) && doorIsOpen(part) && doorWood(part) == 0 &&
                          blockDrop(part) == blockDrop(Blocks::DoorLowShutNorth),
                      "a door can hang from its other side and still be the same door");
                check(Redstone::unlit(part) == doorPart(hung, 2, true, false), "and a powered one shuts on the same hinge");
                check(isIronDoor(doorPart(doorHingeBase(Blocks::IronDoorFirst), 1, false, false)), "an iron door hung the other way is still iron");
            }
            check(isVine(vineWith(0b10101)) && vineMask(vineWith(0b10101)) == 0b10101 && isClimbable(vineWith(1)) &&
                      vineWith(0) == Blocks::Air && axisBase(vineWith(3)) == Blocks::Vine,
                  "vines remember which faces of their cell they hang on");
            check(shaderStandIn(vineWith(2)) == Blocks::Vine && shaderStandIn(railSlopeOf(0, 1, false)) == Blocks::RailEastWest,
                  "and are shaded like the block they stand in for");
            check(pickleCount(pickleOf(3, false)) == 3 && !pickleDry(pickleOf(3, false)) && pickleDry(pickleOf(2, true)) &&
                      lightEmission(pickleOf(4, false)) == 15 && lightEmission(pickleOf(1, false)) == 6 && lightEmission(pickleOf(4, true)) == 0 &&
                      isWaterPlant(pickleOf(2, false)) && !isWaterPlant(pickleOf(2, true)) && blockDrop(pickleOf(4, true)) == static_cast<StackId>(Blocks::SeaPickle),
                  "sea pickles group up to four, glow brighter in a bunch and go dark out of the water");
            check(isGlowLichen(glowLichenWith(0b100101)) && lichenMask(glowLichenWith(0b100101)) == 0b100101 && glowLichenFace(glowLichenWith(0b100100)) == 2 &&
                      lichenMask(glowLichenOn(3)) == 8 && lightEmission(glowLichenWith(63)) == lightEmission(glowLichenOn(0)) &&
                      isSculkVein(sculkVeinWith(6)) && veinMask(sculkVeinWith(6)) == 6 && sculkVeinFace(sculkVeinWith(6)) == 1,
                  "glow lichen and sculk veins spread over several faces of one cell");
            check(vehicleItemFor(MobId::Minecart, 6) == static_cast<StackId>(Items::Minecart), "a spawner minecart breaks into a plain minecart");
            check(Chat::parse("/summon spawner_minecart ~ ~ ~", true, glm::vec3(0.0f)).mob == static_cast<int>(MobId::Minecart) &&
                      Chat::parse("/summon spawner_minecart ~ ~ ~", true, glm::vec3(0.0f)).sub.find("Variant:6") != std::string::npos,
                  "and can be summoned");
            {
                const Chat::Command egg = Chat::parse("/give @s zombie_spawn_egg 3", true);
                check(egg.kind == Chat::Kind::Give && egg.item == static_cast<StackId>(Items::SpawnEgg) &&
                          egg.amplifier == static_cast<int>(MobId::Zombie) && egg.count == 3,
                      "/give takes a target and gives a spawn egg by its vanilla name");
                const Chat::Command stone = Chat::parse("/give @p stone 5", true);
                check(stone.kind == Chat::Kind::Give && stone.item == static_cast<StackId>(Blocks::Stone) && stone.count == 5,
                      "and still gives plain blocks after a target");
            }
            check(isFrame(Blocks::FrameFloor) && isFrame(Blocks::GlowFrameCeiling) && isGlowFrame(Blocks::GlowFrameFloor) &&
                      frameLay(Blocks::FrameFloor) == 1 && frameLay(Blocks::GlowFrameCeiling) == 2 && frameLay(frameFacingAs(2)) == 0 &&
                      blockDrop(Blocks::FrameCeiling) == blockDrop(frameFacingAs(0)) && blockDrop(Blocks::GlowFrameFloor) == blockDrop(frameFacingAs(1, true)),
                  "item frames lie on floors and hang from ceilings");
            {
                Mob warden(MobId::Warden, glm::vec3(0.0f, 64.0f, 0.0f), 5u);
                const int before = warden.health();
                warden.startEmerging();
                check(warden.burrowing() && std::fabs(warden.burrowDepth() - warden.height()) < 0.001f, "a summoned warden starts under the ground");
                warden.damage(10, glm::vec3(1.0f, 0.0f, 0.0f));
                check(warden.health() == before, "and cannot be hurt while it climbs out");
            }
            check(isStonecutter(Blocks::StonecutterTurned) && blockDrop(Blocks::StonecutterTurned) == static_cast<StackId>(Blocks::Stonecutter),
                  "a turned stonecutter is still a stonecutter");
            check(shelfMask(shelfOf(1, 3)) == 7 && shelfBooks(shelfOf(1, 3)) == 3 && shelfFacing(shelfOf(1, 3)) == 1,
                  "an old bookshelf reads as its first slots filled");
            check(isChiseledBookshelf(shelfWith(2, 0b101010)) && shelfMask(shelfWith(2, 0b101010)) == 0b101010 &&
                      shelfBooks(shelfWith(2, 0b101010)) == 3 && shelfFacing(shelfWith(2, 0b101010)) == 2,
                  "a bookshelf remembers which of its six slots hold books");
            check(blockDrop(shelfWith(3, 63)) == blockDrop(shelfOf(0, 0)), "and still breaks into a bookshelf");
            check(Hopper::slotsOf(dropperFacingTo(1)) == Hopper::THROWER_SLOTS && Hopper::slotsOf(dispenserFacingTo(5)) == Hopper::THROWER_SLOTS,
                  "hoppers can fill a turned dropper or dispenser");

            Bench bench;
            bench.blocks[{ 0, 1, 0 }] = Blocks::Chest;
            bench.blocks[{ 0, 0, 0 }] = Blocks::Hopper;
            bench.blocks[{ 0, -1, 0 }] = dropperFacingTo(2);
            bench.held[{ 0, 0, 0 }][0].id = Blocks::Dirt;
            bench.held[{ 0, 0, 0 }][0].count = 1;
            Hopper::step(bench, 0, 0, 0);
            check(bench.countOf(0, -1, 0, Blocks::Dirt) == 1, "a hopper feeds a dropper laid on its side");
        }

        // Nothing is lost when the thing below is full. This is the
        // one that matters: taken before the push was checked, an item
        // would vanish every time a chest filled up.
        {
            Bench bench;
            bench.blocks[{ 0, 2, 0 }] = Blocks::Chest;
            bench.blocks[{ 0, 1, 0 }] = Blocks::Hopper;
            bench.blocks[{ 0, 0, 0 }] = Blocks::Hopper;

            bench.held[{ 0, 1, 0 }][0].id = Blocks::Dirt;
            bench.held[{ 0, 1, 0 }][0].count = 1;

            // Fill the one below so there is nowhere to push.
            for (int i = 0; i < Hopper::HOPPER_SLOTS; ++i)
            {
                bench.held[{ 0, 0, 0 }][i].id = Blocks::Stone;
                bench.held[{ 0, 0, 0 }][i].count = maxStackOf(Blocks::Stone);
            }

            Hopper::step(bench, 0, 1, 0);
            check(bench.countOf(0, 1, 0, Blocks::Dirt) == 1,
                  "a hopper over a full one keeps what it is holding");
        }

        // Stacking: a chest fed by a hopper must not fill with
        // twenty-seven piles of one.
        {
            std::array<ItemStack, 27> into{};
            for (int i = 0; i < 20; ++i)
                check(Hopper::give(into.data(), 27, Blocks::Cobblestone),
                      "a hopper keeps finding room");

            int used = 0;
            for (const ItemStack& slot : into) if (!slot.empty()) ++used;
            check(used == 1, "and twenty of a thing land in one slot, not twenty");
        }

        // Taking from an empty holder gives nothing rather than
        // inventing something.
        {
            std::array<ItemStack, 9> empty{};
            check(Hopper::take(empty.data(), 9) == Blocks::Air,
                  "nothing comes out of an empty dropper");
        }

        // And a dropper hands back what was put in it.
        {
            std::array<ItemStack, 9> loaded{};
            loaded[3].id = Items::Arrow;
            loaded[3].count = 2;

            check(Hopper::take(loaded.data(), 9) == Items::Arrow,
                  "a dropper throws what it is holding");
            check(loaded[3].count == 1, "one at a time");
        }

        // The pace is Minecraft's, and it is a pace rather than a
        // free-for-all.
        check(Hopper::INTERVAL > 0.0f && Hopper::INTERVAL < 1.0f,
              "a hopper moves at a steady rate rather than instantly");
    }

    void testGeodes()
    {
        section("geodes");

        constexpr uint32_t SEED = 4242u;
        const WorldGen generator(SEED);

        // Rare enough to be worth finding. A geode behind every wall
        // would be scenery rather than a find.
        check(WorldGen::GEODE_CHANCE > 0.0f && WorldGen::GEODE_CHANCE < 0.02f,
              "a geode is rare enough to be worth running into");

        bool looked = false;
        for (int ring = 0; ring <= 60 && !looked; ++ring)
            for (int cz = -ring; cz <= ring && !looked; ++cz)
                for (int cx = -ring; cx <= ring && !looked; ++cx)
                {
                    if (ring > 0 && std::abs(cx) != ring && std::abs(cz) != ring) continue;
                    if (!Structures::spreadAt(cx, cz, SEED, WorldGen::GEODE_SALT,
                                              WorldGen::GEODE_CHANCE))
                        continue;

                    int amethyst = 0, budding = 0, clusters = 0;
                    int calcite = 0, basalt = 0, hollow = 0;
                    int lowest = 999, highest = 0;

                    for (int dz = -1; dz <= 1; ++dz)
                        for (int dx = -1; dx <= 1; ++dx)
                        {
                            Chunk chunk(ChunkPos{ cx + dx, cz + dz });
                            generator.generate(chunk);

                            for (int y = 0; y < Chunk::SY; ++y)
                                for (int lx = 0; lx < Chunk::SX; ++lx)
                                    for (int lz = 0; lz < Chunk::SZ; ++lz)
                                    {
                                        const BlockId block = chunk.getBlock(lx, y, lz);
                                        if (!isGeode(block) && block != Blocks::Calcite)
                                            continue;

                                        // Measured off the geode's own
                                        // blocks only. Calcite turns up
                                        // as an ordinary stone variant
                                        // all through the rock, so
                                        // bounding on it said the geode
                                        // reached the bedrock.
                                        if (isGeode(block))
                                        {
                                            lowest = std::min(lowest, y);
                                            highest = std::max(highest, y);
                                        }

                                        switch (block)
                                        {
                                            case Blocks::AmethystBlock:   ++amethyst; break;
                                            case Blocks::BuddingAmethyst: ++budding; break;
                                            case Blocks::AmethystCluster: ++clusters; break;
                                            case Blocks::SmoothBasalt:    ++basalt; break;
                                            case Blocks::Calcite:         ++calcite; break;
                                            default: break;
                                        }
                                    }
                        }

                    check(amethyst > 20, "a geode is lined with amethyst");
                    check(budding > 0, "some of it budding, which is what makes it worth keeping");
                    check(budding < amethyst, "but most of it plain");
                    check(clusters > 0, "with clusters growing off the lining");
                    check(calcite > 20, "a shell of calcite round it");
                    check(basalt > 20, "and basalt round that");

                    // It is hollow. A solid ball of amethyst would be
                    // a seam, not a geode -- and you could not get in.
                    for (int dz = -1; dz <= 1; ++dz)
                        for (int dx = -1; dx <= 1; ++dx)
                        {
                            Chunk chunk(ChunkPos{ cx + dx, cz + dz });
                            generator.generate(chunk);
                            for (int y = lowest; y <= highest; ++y)
                                for (int lx = 0; lx < Chunk::SX; ++lx)
                                    for (int lz = 0; lz < Chunk::SZ; ++lz)
                                        if (chunk.getBlock(lx, y, lz) == Blocks::Air) ++hollow;
                        }
                    check(hollow > 0, "and it is hollow, so there is something to walk into");

                    // Underground, where somebody digging will meet it.
                    check(lowest >= 2, "a geode sits above the bedrock");
                    check(highest <= WorldGen::GEODE_MAX + 8,
                          "and below the surface rather than poking out of a hillside");

                    looked = true;
                }

        check(looked, "there is a geode somewhere near the origin");

        // A cluster is knocked off rather than mined, so it must not
        // want a pickaxe -- the mistake this block table has caught
        // five times now.
        check(blockMaterial(Blocks::AmethystCluster) == Material::Plant,
              "a cluster comes off by hand rather than wanting a pickaxe");
        check(isGeode(Blocks::AmethystBlock) && isGeode(Blocks::SmoothBasalt),
              "the geode blocks are the geode's");
        check(!isGeode(Blocks::Calcite),
              "while calcite is its own block and turns up elsewhere too");
    }

    void testAppleAndBoneMeal()
    {
        section("apples, and bone meal");

        // An apple is food, and modest food: it is what you eat on the
        // way somewhere, which is why it falls out of trees.
        check(Food::isEdible(Items::Apple), "an apple can be eaten");
        check(Food::valueOf(Items::Apple).hunger < Food::valueOf(Items::Steak).hunger,
              "and is worth less than a steak");
        check(Food::valueOf(Items::Apple).hunger > Food::valueOf(Items::GlowBerries).hunger,
              "but more than a glow berry");

        // Bone meal comes off a bone, three at a time, which is
        // Minecraft's rate.
        bool fromBone = false;
        for (StackId made : Crafting::everyOutput())
            if (made == Items::BoneMeal) fromBone = true;
        check(fromBone, "bone meal is made from something");

        {
            ItemStack grid[9];
            grid[0].id = Items::Bone;
            grid[0].count = 1;
            const CraftOutput meal = Crafting::match(grid, 3);
            check(meal.id == Items::BoneMeal && meal.count == 3,
                  "one bone grinds into three meal");
        }

        // The two are told apart in a hotbar. They were not, once: the
        // item table is read by enum index and these two were appended
        // to the end of it while the enum had them in the middle, so
        // every item from the apple onward came back with somebody
        // else's name and somebody else's icon.
        check(std::string(displayName(Items::Apple)) == "Apple",
              "an apple is called an apple");
        check(std::string(displayName(Items::BoneMeal)) == "Bone Meal",
              "and bone meal is called bone meal");
        check(std::string(displayName(Items::NetherStar)) == "Nether Star",
              "and the star is still the star");
        check(atlasTileFor(Items::Apple) != atlasTileFor(Items::BoneMeal),
              "and they are not drawn with the same picture");

        // The table and the enum have to stay in step. This is the
        // check that would have caught it outright.
        for (int i = static_cast<int>(Items::FIRST); i < static_cast<int>(Items::Count); ++i)
        {
            const StackId id = static_cast<StackId>(i);
            check(displayName(id)[0] != '\0',
                  "item " + std::to_string(i) + " has a name");
            check(atlasTileFor(id) != Tiles::Blank,
                  std::string(displayName(id)) + " has a picture of its own");
        }
    }

    void testFortressAndMineshaft()
    {
        section("the fortress and the mineshaft");

        constexpr uint32_t SEED = 4242u;

        // What a cage sends is decided by where it stands, because a
        // block here is an id with nowhere to write anything down.
        check(cageSpecies(Dimension::Nether) == MobId::Blaze,
              "a cage in a fortress makes blazes");
        check(cageSpecies(Dimension::Overworld) == MobId::Spider,
              "and one in the rock makes spiders");
        for (Dimension where : { Dimension::Overworld, Dimension::Nether, Dimension::End })
            check(mobType(cageSpecies(where)).spawnClass == SpawnClass::Hostile,
                  "and whatever it makes is something that fights");

        check(isMonsterSpawner(Blocks::MonsterSpawner), "a cage is a cage");
        check(!isMonsterSpawner(Blocks::TrialSpawner), "and a trial spawner is not one");
        check(!isOpaque(Blocks::MonsterSpawner), "you can see into it");
        check(CAGE_LIMIT > 0 && CAGE_LIMIT < 12, "it keeps a few out at once, not a swarm");

        // The fortress. Nether chunks, so the overworld generator will
        // not do: this is the one structure in the other dimension.
        {
            const WorldGen nether(SEED, Dimension::Nether);
            const Structures::Spot spot =
                Structures::inSquare(Structures::NETHER_FORTRESS, 0, 0, SEED);

            int brick = 0, cages = 0, wart = 0, soul = 0;
            for (int cz = spot.chunkZ - 1; cz <= spot.chunkZ + 1; ++cz)
                for (int cx = spot.chunkX - 1; cx <= spot.chunkX + 1; ++cx)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    nether.generate(chunk);

                    for (int y = WorldGen::FORTRESS_FLOOR - 1;
                         y <= WorldGen::FORTRESS_FLOOR + 6 && y < Chunk::SY; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                                switch (chunk.getBlock(lx, y, lz))
                                {
                                    case Blocks::NetherBricks: ++brick; break;
                                    case Blocks::MonsterSpawner: ++cages; break;
                                    case Blocks::NetherWart:
                                        ++wart;
                                        if (y > 0 && chunk.getBlock(lx, y - 1, lz) == Blocks::SoulSand) ++soul;
                                        break;
                                    default: break;
                                }
                }

            check(brick > 300, "a fortress is a lot of nether brick");
            check(cages == 1, "with one cage in the keep");
            check(wart == 4 && soul == 4, "and wart growing in the corners of it, on soul sand");
            check(WorldGen::FORTRESS_FLOOR > WorldGen::NETHER_LAVA_LEVEL,
                  "and the whole of it stands clear of the lava");
        }

        // The mineshaft, which goes down by the spread rule. Finding
        // one means walking until the dice say so rather than going to
        // the middle of a square.
        {
            const WorldGen generator(SEED);

            bool found = false;
            int planks = 0, rails = 0, fence = 0;

            for (int radius = 0; radius <= 30 && !found; ++radius)
                for (int cz = -radius; cz <= radius && !found; ++cz)
                    for (int cx = -radius; cx <= radius && !found; ++cx)
                    {
                        if (std::max(std::abs(cx), std::abs(cz)) != radius) continue;
                        if (!Structures::spreadAt(cx, cz, SEED, 445731u, 0.012f)) continue;

                        Chunk chunk(ChunkPos{ cx, cz });
                        generator.generate(chunk);

                        for (int y = WorldGen::MINESHAFT_MIN - 2;
                             y <= WorldGen::MINESHAFT_MAX + 3 && y < Chunk::SY; ++y)
                            for (int lz = 0; lz < Chunk::SZ; ++lz)
                                for (int lx = 0; lx < Chunk::SX; ++lx)
                                    switch (chunk.getBlock(lx, y, lz))
                                    {
                                        case Blocks::Planks: ++planks; break;
                                        case Blocks::RailEastWest:
                                        case Blocks::RailNorthSouth: ++rails; break;
                                        case Blocks::OakFence: ++fence; break;
                                        default: break;
                                    }

                        found = planks > 0;
                    }

            check(found, "there is a mineshaft within thirty chunks of the origin");
            check(rails > 0, "with rails down it");
            check(fence > 0, "and supports holding the roof up");
        }
    }

    void testStructurePlacement()
    {
        section("structures: Minecraft's own placement rules");

        constexpr uint32_t SEED = 4242u;

        // The grid rule, checked on every kind this game places by it.
        // Two guarantees: one to a square, and never closer than the
        // separation. That second one is what the separation is for,
        // and nothing else in the scheme enforces it.
        const Structures::Grid GRIDS[] = {
            Structures::VILLAGE, Structures::TRIAL_CHAMBER, Structures::DESERT_PYRAMID,
            Structures::OCEAN_MONUMENT, Structures::NETHER_FORTRESS,
            Structures::RUINED_PORTAL, Structures::SHIPWRECK,
        };

        for (const Structures::Grid& grid : GRIDS)
        {
            check(grid.separation < grid.spacing,
                  "a structure's separation leaves room inside its spacing");

            std::vector<Structures::Spot> spots;
            for (int sz = -6; sz <= 6; ++sz)
                for (int sx = -6; sx <= 6; ++sx)
                {
                    const Structures::Spot spot = Structures::inSquare(grid, sx, sz, SEED);
                    check(spot.exists, "every square on the grid has one");

                    const int lowX = sx * grid.spacing;
                    const int lowZ = sz * grid.spacing;
                    check(spot.chunkX >= lowX &&
                              spot.chunkX < lowX + grid.spacing - grid.separation,
                          "and it lands in that square's placement window");
                    check(spot.chunkZ >= lowZ &&
                              spot.chunkZ < lowZ + grid.spacing - grid.separation,
                          "on both axes");

                    spots.push_back(spot);
                }

            int tooClose = 0;
            for (size_t i = 0; i < spots.size(); ++i)
                for (size_t j = i + 1; j < spots.size(); ++j)
                {
                    const int gapX = std::abs(spots[i].chunkX - spots[j].chunkX);
                    const int gapZ = std::abs(spots[i].chunkZ - spots[j].chunkZ);
                    if (gapX < grid.separation && gapZ < grid.separation) ++tooClose;
                }
            check(tooClose == 0, "and no two of them are closer than the separation");
        }

        // Two kinds of structure must not land on top of each other
        // every time, which is what the salt is for.
        {
            int sameChunk = 0;
            for (int sz = -8; sz <= 8; ++sz)
                for (int sx = -8; sx <= 8; ++sx)
                {
                    const Structures::Spot a =
                        Structures::inSquare(Structures::DESERT_PYRAMID, sx, sz, SEED);
                    const Structures::Spot b =
                        Structures::inSquare(Structures::JUNGLE_TEMPLE, sx, sz, SEED);
                    if (a.chunkX == b.chunkX && a.chunkZ == b.chunkZ) ++sameChunk;
                }
            check(sameChunk == 0, "two kinds with different salts never share a chunk");
        }

        // Negative coordinates round the right way, or the squares
        // either side of the origin are twice as wide as the rest.
        check(Structures::squareOf(0, 32) == 0, "the origin is in square zero");
        check(Structures::squareOf(31, 32) == 0, "and so is the far end of that square");
        check(Structures::squareOf(32, 32) == 1, "the next chunk starts square one");
        check(Structures::squareOf(-1, 32) == -1, "a chunk west of the origin is square -1");
        check(Structures::squareOf(-32, 32) == -1, "and so is the far end of that one");
        check(Structures::squareOf(-33, 32) == -2, "with -2 beginning after it");

        // The spread rule: a flat chance per chunk, the same answer
        // every time, and roughly the rate it claims.
        {
            int hits = 0;
            for (int cz = 0; cz < 100; ++cz)
                for (int cx = 0; cx < 100; ++cx)
                    if (Structures::spreadAt(cx, cz, SEED, 1234u, 0.01f)) ++hits;

            check(hits > 40 && hits < 170, "a one-in-a-hundred spread comes up about that often");
            check(Structures::spreadAt(5, 7, SEED, 1234u, 0.01f) ==
                      Structures::spreadAt(5, 7, SEED, 1234u, 0.01f),
                  "and gives the same answer every time it is asked");
        }

        section("structures: the strongholds, which are the exception");

        // There are always some. Everything else in the world may or
        // may not be nearby; an eye of ender has to have something to
        // point at from anywhere at all.
        check(Structures::strongholdCount() > 0, "a world always has strongholds in it");

        std::vector<Stronghold::Site> holds;
        for (int i = 0; i < Structures::strongholdCount(); ++i)
        {
            const Stronghold::Site site = Stronghold::byIndex(i, SEED);
            check(site.exists, "every one of them exists");
            check(site.y >= Stronghold::FLOOR_MIN && site.y <= Stronghold::FLOOR_MAX,
                  "and is underground");
            holds.push_back(site);
        }

        // None of them on top of spawn, and none on top of another.
        for (const Stronghold::Site& site : holds)
        {
            const double away = std::sqrt(static_cast<double>(site.x) * site.x +
                                          static_cast<double>(site.z) * site.z);
            check(away > 800.0, "none of them is on top of spawn");
        }

        for (size_t i = 0; i < holds.size(); ++i)
            for (size_t j = i + 1; j < holds.size(); ++j)
            {
                const int gapX = std::abs(holds[i].x - holds[j].x);
                const int gapZ = std::abs(holds[i].z - holds[j].z);
                check(gapX > Stronghold::MAX_RADIUS * 2 || gapZ > Stronghold::MAX_RADIUS * 2,
                      "and no two of them are built into each other");
            }

        // The nearest one really is the nearest.
        for (int i = 0; i < 40; ++i)
        {
            const int fromX = (i - 20) * 320;
            const int fromZ = (i * 97 % 41 - 20) * 280;

            const Stronghold::Site near = Stronghold::nearest(fromX, fromZ, SEED);
            check(near.exists, "there is always one to find");

            const long long best = (long long)(near.x - fromX) * (near.x - fromX) +
                                   (long long)(near.z - fromZ) * (near.z - fromZ);
            for (const Stronghold::Site& other : holds)
            {
                const long long distance = (long long)(other.x - fromX) * (other.x - fromX) +
                                           (long long)(other.z - fromZ) * (other.z - fromZ);
                check(distance >= best, "and nothing else is nearer than the one it names");
            }
        }

        // The layout: one portal room, and a ring of twelve in it.
        {
            const Stronghold::Site site = holds.front();
            const std::vector<Stronghold::Room> rooms = Stronghold::plan(site, SEED);
            check(static_cast<int>(rooms.size()) == Stronghold::GRID * Stronghold::GRID,
                  "a stronghold is a full grid of rooms");

            int portals = 0;
            for (const Stronghold::Room& room : rooms)
                if (room.kind == Stronghold::Kind::Portal) ++portals;
            check(portals == 1, "with exactly one portal room in it");

            const int eyes = Stronghold::eyesAlreadySet(site, SEED);
            check(eyes >= 0 && eyes < EndPortal::FRAME_COUNT,
                  "some of the eyes are set and never all of them");
            check(Stronghold::eyeInFrame(site, SEED, 0) ==
                      Stronghold::eyeInFrame(site, SEED, 0),
                  "and which ones is the same every time it is asked");
        }

        // And it is built. The portal room has to hold twelve frames,
        // or the End is somewhere you can only reach by crafting.
        {
            const WorldGen generator(SEED);
            const Stronghold::Site site = generator.nearestStronghold(0, 0);

            int frames = 0, filled = 0, lava = 0, shelves = 0;
            const int chunkX = (site.x >= 0 ? site.x : site.x - Chunk::SX + 1) / Chunk::SX;
            const int chunkZ = (site.z >= 0 ? site.z : site.z - Chunk::SZ + 1) / Chunk::SZ;

            for (int cz = chunkZ - 2; cz <= chunkZ + 2; ++cz)
                for (int cx = chunkX - 2; cx <= chunkX + 2; ++cx)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    for (int y = site.y - 2; y <= site.y + Stronghold::HEADROOM; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                            {
                                if (y < 0 || y >= Chunk::SY) continue;
                                switch (chunk.getBlock(lx, y, lz))
                                {
                                    case Blocks::EndPortalFrame: ++frames; break;
                                    case Blocks::EndPortalFrameFilled: ++filled; break;
                                    case Blocks::Lava: ++lava; break;
                                    case Blocks::Bookshelf: ++shelves; break;
                                    default: break;
                                }
                            }
                }

            check(frames + filled == EndPortal::FRAME_COUNT,
                  "the portal room holds the twelve frames and no more");
            check(filled < EndPortal::FRAME_COUNT,
                  "and never arrives finished, or there would be nothing to do");
            check(lava > 0, "with lava under the ring, which is Minecraft's");
            check(shelves > 0, "and a library somewhere in it");
        }
    }

    void testCaves()
    {
        section("caves: the two sorts that are not just grey");

        const WorldGen generator(4242u);

        // Every kind is somewhere, and a kind holds for a stretch
        // rather than flickering block to block -- a cave that changed
        // character every twenty blocks would read as noise.
        int seen[3] = { 0 };
        for (int z = -3000; z <= 3000; z += 61)
            for (int x = -3000; x <= 3000; x += 61)
                ++seen[static_cast<int>(generator.caveKindAt(x, z))];

        check(seen[0] > 0, "most caves are plain rock");
        check(seen[1] > 0, "some of them drip");
        check(seen[2] > 0, "and some of them grow");
        check(seen[0] > seen[1] && seen[0] > seen[2],
              "with the plain ones commonest, so the others are a find");

        {
            // Walk a line and count how often the kind changes. A run
            // should be long: this is the difference between a cave
            // that is somewhere and a cave that is static.
            int changes = 0;
            WorldGen::CaveKind last = generator.caveKindAt(-2000, 140);
            for (int x = -2000; x <= 2000; x += 4)
            {
                const WorldGen::CaveKind here = generator.caveKindAt(x, 140);
                if (here != last) ++changes;
                last = here;
            }
            check(changes > 0, "the kind does change as you walk");
            check(changes < 60, "but not every few blocks");
        }

        // It is the same answer every time, or two chunks would dress
        // the same cave differently and leave a seam down it.
        for (int i = 0; i < 50; ++i)
            check(generator.caveKindAt(i * 37, i * -19) ==
                      generator.caveKindAt(i * 37, i * -19),
                  "a place is the same sort of cave every time it is asked");

        // And it is actually built. Generating enough chunks to find
        // both kinds would be slow, so this walks out until it finds a
        // chunk over each and checks only those.
        auto censusOf = [&](WorldGen::CaveKind wanted, int& dripstone, int& spikes,
                            int& moss, int& berries) {
            dripstone = spikes = moss = berries = 0;

            for (int radius = 0; radius <= 40; ++radius)
                for (int cz = -radius; cz <= radius; ++cz)
                    for (int cx = -radius; cx <= radius; ++cx)
                    {
                        if (std::max(std::abs(cx), std::abs(cz)) != radius) continue;
                        if (generator.caveKindAt(cx * Chunk::SX + 8, cz * Chunk::SZ + 8) != wanted)
                            continue;

                        Chunk chunk(ChunkPos{ cx, cz });
                        generator.generate(chunk);

                        for (int y = 0; y < 80 && y < Chunk::SY; ++y)
                            for (int lz = 0; lz < Chunk::SZ; ++lz)
                                for (int lx = 0; lx < Chunk::SX; ++lx)
                                    switch (chunk.getBlock(lx, y, lz))
                                    {
                                        case Blocks::DripstoneBlock: ++dripstone; break;
                                        case Blocks::DripstoneUp:
                                        case Blocks::DripstoneDown: ++spikes; break;
                                        case Blocks::Moss: ++moss; break;
                                        case Blocks::GlowBerries: ++berries; break;
                                        default: break;
                                    }

                        if (dripstone + spikes + moss + berries > 0) return true;
                    }
            return false;
        };

        {
            int dripstone = 0, spikes = 0, moss = 0, berries = 0;
            check(censusOf(WorldGen::CaveKind::Dripstone, dripstone, spikes, moss, berries),
                  "a dripstone cave turns up within a few hundred chunks");
            check(dripstone > 0, "and it is cut through dripstone");
            check(spikes > 0, "with spikes hanging and standing in it");
            check(moss == 0 && berries == 0, "and nothing growing in it");
        }

        {
            int dripstone = 0, spikes = 0, moss = 0, berries = 0;
            check(censusOf(WorldGen::CaveKind::Lush, dripstone, spikes, moss, berries),
                  "and so does a lush one");
            check(moss > 0, "with moss underfoot");
            check(dripstone == 0 && spikes == 0, "and no dripstone in it");
        }

        // Glow berries are the only light down there nobody carried in.
        check(lightEmission(Blocks::GlowBerries) > 0, "glow berries light the cave they grow in");
        check(Food::isEdible(Items::GlowBerries), "and can be eaten");
        check(Food::valueOf(Items::GlowBerries).hunger < Food::valueOf(Items::Bread).hunger,
              "though they are worth less than a loaf");

        check(isLeaves(Blocks::AzaleaLeaves), "azalea counts as leaves");
        check(blockMaterial(Blocks::AzaleaLeaves) == Material::Plant, "and tears like them");
        check(blockMaterial(Blocks::Moss) == Material::Plant, "moss comes up with a hand");
        check(blockMaterial(Blocks::DripstoneBlock) == Material::Stone,
              "and dripstone wants a pickaxe");
    }

    void testCopper()
    {
        section("copper: the one block that changes on its own");

        // The four ages, in order and ending.
        check(Copper::next(Blocks::CopperBlock) == Blocks::ExposedCopper,
              "new copper goes patchy first");
        check(Copper::next(Blocks::ExposedCopper) == Blocks::WeatheredCopper,
              "then the green starts");
        check(Copper::next(Blocks::WeatheredCopper) == Blocks::OxidizedCopper,
              "then it takes over");
        check(Copper::next(Blocks::OxidizedCopper) == Blocks::OxidizedCopper,
              "and there it stops");

        check(Copper::ages(Blocks::CopperBlock), "the new block has somewhere to go");
        check(!Copper::ages(Blocks::OxidizedCopper), "the oxidised one has not");
        check(!Copper::ages(Blocks::Stone), "and stone is not copper at all");
        check(!Copper::ages(Blocks::CopperOre), "nor is the ore it came out of");

        // A wall left standing gets all the way there, and takes a
        // while about it. A step that happened every tick would turn
        // a build green while you were still laying it.
        {
            BlockId block = Blocks::CopperBlock;
            int steps = 0;
            for (int tick = 1; tick <= 4000 && Copper::ages(block); ++tick)
                if (Copper::ticks(10, 70, -4, 4242u, tick))
                {
                    block = Copper::next(block);
                    ++steps;
                }

            check(block == Blocks::OxidizedCopper, "a wall left alone ends up green");
            check(steps == 3, "by way of both stages in between");
        }

        {
            // How long that took, in rolls. At one roll every eight
            // seconds and a chance of about one in twelve, a stage is
            // a couple of minutes and the whole way is most of an
            // hour -- long enough to be weather rather than a timer.
            int rolls = 0;
            for (int tick = 1; tick <= 4000; ++tick)
                if (Copper::ticks(10, 70, -4, 4242u, tick)) ++rolls;

            const float share = rolls / 4000.0f;
            check(share > Copper::AGE_CHANCE * 0.5f && share < Copper::AGE_CHANCE * 1.6f,
                  "the roll comes up about as often as it claims to");
        }

        // Two blocks side by side must not weather in lockstep, or a
        // wall changes colour all at once like a traffic light.
        {
            int together = 0, apart = 0;
            for (int tick = 1; tick <= 2000; ++tick)
            {
                const bool left = Copper::ticks(0, 64, 0, 4242u, tick);
                const bool right = Copper::ticks(1, 64, 0, 4242u, tick);
                if (left && right) ++together;
                if (left != right) ++apart;
            }
            check(apart > together, "neighbouring blocks weather at their own pace");
        }

        // The chain out of the ground.
        check(Smelting::resultOf(Blocks::CopperOre) == Items::CopperIngot,
              "copper ore smelts into an ingot");
        check(Smelting::resultOf(Items::RawCopper) == Items::CopperIngot,
              "and so does the raw copper it drops");

        {
            ItemStack nine[9];
            for (int i = 0; i < 9; ++i) { nine[i].id = Items::CopperIngot; nine[i].count = 1; }
            const CraftOutput packed = Crafting::match(nine, 3);
            check(packed.valid() && packed.id == Blocks::CopperBlock,
                  "nine ingots make a block");

            ItemStack back[9];
            back[0].id = Blocks::CopperBlock;
            back[0].count = 1;
            const CraftOutput loose = Crafting::match(back, 2);
            check(loose.valid() && loose.id == Items::CopperIngot && loose.count == 9,
                  "and the block gives all nine back");
        }

        // The bulb: lit and unlit are the same block to everything but
        // the light it gives and the pulse that flips it.
        check(isCopperBulb(Blocks::CopperBulb) && isCopperBulb(Blocks::CopperBulbLit),
              "both bulbs are bulbs");
        check(lightEmission(Blocks::CopperBulbLit) == 15, "a lit bulb is as bright as it gets");
        check(lightEmission(Blocks::CopperBulb) == 0, "and an unlit one gives nothing");
        check(Copper::ages(Blocks::CopperBulb) && !Copper::ages(Copper::waxed(Blocks::CopperBulb)),
              "a bulb weathers like the copper it is made of, unless it is waxed");

        {
            ItemStack bulb[9];
            bulb[1].id = Blocks::CopperBlock; bulb[1].count = 1;
            bulb[3].id = Blocks::CopperBlock; bulb[3].count = 1;
            bulb[4].id = Items::BlazeRod;    bulb[4].count = 1;
            bulb[5].id = Blocks::CopperBlock; bulb[5].count = 1;
            bulb[7].id = Items::Redstone;    bulb[7].count = 1;
            const CraftOutput made = Crafting::match(bulb, 3);
            check(made.valid() && made.id == Blocks::CopperBulb && made.count == 4,
                  "three copper blocks, a blaze rod and redstone make four bulbs");
        }

        {
            const WorldGen generator(4242u);
            int built = 0, chested = 0, caged = 0;
            for (int cz = -12; cz <= 12 && built < 2; ++cz)
                for (int cx = -12; cx <= 12 && built < 2; ++cx)
                {
                    int ox = 0, oy = 0, oz = 0;
                    if (!generator.dungeonIn(cx, cz, ox, oy, oz)) continue;
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);
                    const int lx = ox - cx * Chunk::SX, lz = oz - cz * Chunk::SZ;
                    if (chunk.getBlock(lx, oy, lz) != Blocks::MonsterSpawner) continue;
                    ++built;
                    if (generator.dungeonCage(ox, oy, oz) >= 0) ++caged;
                    static const int SPOTS[4][2] = { { 3, 0 }, { -3, 0 }, { 0, 3 }, { 0, -3 } };
                    for (const auto& spot : SPOTS)
                        if (chunk.getBlock(lx + spot[0], oy, lz + spot[1]) == Blocks::Chest &&
                            generator.hoardAt(ox + spot[0], oy, oz + spot[1]) == WorldGen::Hoard::Dungeon)
                            ++chested;
                }
            check(built > 0, "monster rooms are dug into the caves");
            check(caged == built && chested >= built, "each with a spawner of its own kind and a dungeon chest or two");
        }

        // There is copper in the ground to find, and it is up where
        // Minecraft keeps it rather than down with the diamond.
        {
            const WorldGen generator(4242u);
            long long copper = 0, stone = 0;
            int lowest = 999, highest = -1;

            for (int cz = 0; cz < 2; ++cz)
                for (int cx = 0; cx < 2; ++cx)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    for (int y = 0; y < 128 && y < Chunk::SY; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                            {
                                const BlockId block = chunk.getBlock(lx, y, lz);
                                if (block == Blocks::CopperOre)
                                {
                                    ++copper;
                                    lowest = std::min(lowest, y);
                                    highest = std::max(highest, y);
                                }
                                else if (isStoneFamily(block)) ++stone;
                            }
                }

            check(copper > 0, "there is copper in the ground");
            check(lowest >= 20, "none of it is down with the diamond");
            check(highest <= 110, "and none of it is in the clouds");
            check(copper * 1000 / std::max<long long>(stone, 1) < 40,
                  "and it is a seam rather than the rock itself");
        }
    }

    void testPistons()
    {
        section("pistons: what moves and what does not");

        struct Bench : Redstone::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> cells;
            BlockId at(int x, int y, int z) const override
            {
                auto it = cells.find({ x, y, z });
                return it == cells.end() ? Blocks::Air : it->second;
            }
            void set(int x, int y, int z, BlockId block) override { cells[{ x, y, z }] = block; }
        };

        // East is +x, which is the one direction worth spelling out: a
        // piston pointing the wrong way would still pass every test
        // that only counted blocks.
        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonEast);
            bench.set(1, 0, 0, Blocks::Cobblestone);

            check(Piston::lineLength(bench, 0, 0, 0, 1) == 1, "one block in front is a line of one");
            check(Piston::extend(bench, 0, 0, 0), "and it goes");
            check(bench.at(0, 0, 0) == Blocks::PistonOutEast, "the body is marked out");
            check(bench.at(1, 0, 0) == Blocks::PistonHeadEast, "its head is in front of it");
            check(bench.at(2, 0, 0) == Blocks::Cobblestone, "and the block has moved one along");

            check(Piston::retract(bench, 0, 0, 0), "it comes back");
            check(bench.at(0, 0, 0) == Blocks::PistonEast, "the body is in again");
            check(bench.at(1, 0, 0) == Blocks::Air, "the head is gone");
            check(bench.at(2, 0, 0) == Blocks::Cobblestone,
                  "and a plain piston leaves the block where it pushed it");
        }

        // Sticky brings it back.
        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::StickyPistonEast);
            bench.set(1, 0, 0, Blocks::Cobblestone);
            Piston::extend(bench, 0, 0, 0);
            Piston::retract(bench, 0, 0, 0);

            check(bench.at(1, 0, 0) == Blocks::Cobblestone, "a sticky piston pulls it back");
            check(bench.at(2, 0, 0) == Blocks::Air, "and leaves nothing behind it");
        }

        // A line of twelve goes; thirteen does not. Minecraft's limit,
        // and the reason it exists: without it a piston at the edge of
        // the world would push a column to the horizon.
        for (int length : { 1, 6, Piston::MAX_PUSH })
        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonEast);
            for (int i = 1; i <= length; ++i) bench.set(i, 0, 0, Blocks::Cobblestone);

            check(Piston::lineLength(bench, 0, 0, 0, 1) == length,
                  "a line of " + std::to_string(length) + " is measured right");
            check(Piston::extend(bench, 0, 0, 0),
                  "and a line of " + std::to_string(length) + " moves");
            check(bench.at(length + 1, 0, 0) == Blocks::Cobblestone,
                  "with the far end one further along");
        }

        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonEast);
            for (int i = 1; i <= Piston::MAX_PUSH + 1; ++i)
                bench.set(i, 0, 0, Blocks::Cobblestone);

            check(Piston::lineLength(bench, 0, 0, 0, 1) < 0, "thirteen is too many");
            check(!Piston::extend(bench, 0, 0, 0), "so the piston does not move");
            check(bench.at(0, 0, 0) == Blocks::PistonEast, "and nothing about it changed");
            check(bench.at(1, 0, 0) == Blocks::Cobblestone, "nor about what it was pushing");
        }

        // Bedrock stops it dead, and so does a chest -- what is inside
        // a chest lives outside the block, and moving one without its
        // contents would lose them.
        for (BlockId stubborn : { Blocks::Bedrock, Blocks::Chest, Blocks::Vault,
                                  Blocks::TrialSpawner })
        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonEast);
            bench.set(1, 0, 0, Blocks::Cobblestone);
            bench.set(2, 0, 0, stubborn);

            check(!Piston::movable(stubborn),
                  std::string(blockInfo(stubborn).name) + " does not get pushed");
            check(!Piston::extend(bench, 0, 0, 0), "so the piston behind it stays put");
            check(bench.at(1, 0, 0) == Blocks::Cobblestone, "and nothing in the line moves");
        }

        // Air and water are not in the way at all.
        check(Piston::replaceable(Blocks::Air), "air is not in the way");
        check(Piston::replaceable(Blocks::Water), "nor is water");
        check(!Piston::replaceable(Blocks::Stone), "stone is");

        // Pushing into nothing: a piston with clear air in front still
        // puts its head out.
        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonWest);
            check(Piston::lineLength(bench, 0, 0, 0, 3) == 0, "nothing in front is a line of none");
            check(Piston::extend(bench, 0, 0, 0), "and the head still comes out");
            check(bench.at(-1, 0, 0) == Blocks::PistonHeadWest, "to the west of it");
        }

        // Every facing, so none of the four is wired up backwards.
        for (int facing = 0; facing < 4; ++facing)
        {
            int dx = 0, dz = 0;
            Piston::step(facing, dx, dz);

            Bench bench;
            bench.set(0, 0, 0, pistonBody(facing, false, false));
            bench.set(dx, 0, dz, Blocks::Cobblestone);

            check(Piston::extend(bench, 0, 0, 0),
                  "a piston facing " + std::to_string(facing) + " pushes");
            check(bench.at(dx, 0, dz) == pistonHead(facing, false),
                  "its head goes where it points");
            check(bench.at(dx * 2, 0, dz * 2) == Blocks::Cobblestone,
                  "and the block goes one further the same way");
        }

        // A piston mid-push is two blocks pretending to be one, so
        // nothing shoves either half of one.
        check(!Piston::movable(Blocks::PistonOutNorth), "a piston with its head out will not budge");
        check(!Piston::movable(Blocks::PistonHeadNorth), "nor its head");
        check(Piston::movable(Blocks::PistonNorth), "one sitting idle can be pushed like anything else");

        // What the id says about it.
        for (int facing = 0; facing < 4; ++facing)
            for (bool sticky : { false, true })
            {
                const BlockId idle = pistonBody(facing, sticky, false);
                const BlockId out = pistonBody(facing, sticky, true);
                const BlockId head = pistonHead(facing, sticky);

                check(pistonFacing(idle) == facing && pistonFacing(out) == facing &&
                          pistonFacing(head) == facing,
                      "every part of a piston points the same way");
                check(pistonSticky(idle) == sticky && pistonSticky(head) == sticky,
                      "and knows whether it is sticky");
                check(pistonOut(out) && !pistonOut(idle), "and whether its head is out");
                check(blockInfo(head).tileFront >= 0, "the head has a face of its own");
            }

        for (int facing = 4; facing < 6; ++facing)
            for (bool sticky : { false, true })
            {
                const BlockId idle = pistonBody(facing, sticky, false);
                const BlockId out = pistonBody(facing, sticky, true);
                const BlockId head = pistonHead(facing, sticky);
                check(isPistonBody(idle) && isPistonBody(out) && isPistonHead(head) && !isPistonHead(idle) && !isPistonBody(head),
                      "an upright piston knows its parts");
                check(pistonFacing(idle) == facing && pistonFacing(out) == facing && pistonFacing(head) == facing &&
                          pistonSticky(idle) == sticky && pistonSticky(head) == sticky && pistonOut(out) && !pistonOut(idle),
                      "and which way it points");
                check(blockDrop(out) == static_cast<StackId>(sticky ? Blocks::StickyPistonNorth : Blocks::PistonNorth),
                      "and breaks into the right piston");
            }
        check(!pistonSticky(Blocks::SlimeBlock) && !pistonSticky(Blocks::Stone), "only pistons are sticky pistons");

        {
            Bench bench;
            bench.set(0, 0, 0, pistonBody(4, false, false));
            bench.set(0, 1, 0, Blocks::Sand);
            bench.set(0, 2, 0, Blocks::Cobblestone);
            check(Piston::extend(bench, 0, 0, 0), "a piston facing up pushes");
            check(bench.at(0, 1, 0) == pistonHead(4, false) && bench.at(0, 2, 0) == Blocks::Sand && bench.at(0, 3, 0) == Blocks::Cobblestone,
                  "and lifts the column above it");
            check(Piston::retract(bench, 0, 0, 0) && bench.at(0, 1, 0) == Blocks::Air && bench.at(0, 2, 0) == Blocks::Sand,
                  "and leaves it up when it comes back");
        }

        {
            Bench bench;
            bench.set(0, 5, 0, pistonBody(5, true, false));
            bench.set(0, 4, 0, Blocks::Stone);
            check(Piston::extend(bench, 0, 5, 0) && bench.at(0, 3, 0) == Blocks::Stone, "a sticky piston facing down pushes down");
            check(Piston::retract(bench, 0, 5, 0) && bench.at(0, 4, 0) == Blocks::Stone && bench.at(0, 3, 0) == Blocks::Air,
                  "and pulls back up");
        }

        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonEast);
            bench.set(1, 0, 0, Blocks::SlimeBlock);
            bench.set(1, 1, 0, Blocks::Stone);
            bench.set(1, 0, 1, Blocks::Planks);
            bench.set(1, -1, 0, Blocks::HoneyBlock);
            Piston::Moved moved;
            check(Piston::extend(bench, 0, 0, 0, &moved), "a piston pushes a slime block");
            check(bench.at(2, 0, 0) == Blocks::SlimeBlock && bench.at(2, 1, 0) == Blocks::Stone && bench.at(2, 0, 1) == Blocks::Planks,
                  "and everything stuck to the slime goes with it");
            check(bench.at(1, -1, 0) == Blocks::HoneyBlock && bench.at(2, -1, 0) == Blocks::Air, "but honey does not stick to slime");
            check(moved.cells.size() == 4 && moved.direction == glm::ivec3(1, 0, 0), "and the move is reported cell by cell");
        }

        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::StickyPistonEast);
            bench.set(1, 0, 0, Blocks::HoneyBlock);
            bench.set(1, 1, 0, Blocks::Cobblestone);
            Piston::extend(bench, 0, 0, 0);
            Piston::retract(bench, 0, 0, 0);
            check(bench.at(1, 0, 0) == Blocks::HoneyBlock && bench.at(1, 1, 0) == Blocks::Cobblestone && bench.at(2, 1, 0) == Blocks::Air,
                  "a sticky piston pulls a honey block back with what is on it");
        }

        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonEast);
            bench.set(1, 0, 0, Blocks::SlimeBlock);
            bench.set(1, 1, 0, Blocks::Obsidian);
            bench.set(2, 0, 0, Blocks::Obsidian);
            check(!Piston::extend(bench, 0, 0, 0), "obsidian in front stops a slime push");
            bench.set(2, 0, 0, Blocks::Air);
            check(Piston::extend(bench, 0, 0, 0) && bench.at(1, 1, 0) == Blocks::Obsidian, "obsidian stuck on the side just stays");
        }

        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::PistonEast);
            bench.set(1, 0, 0, Blocks::Stone);
            bench.set(2, 0, 0, Blocks::Torch);
            Piston::Moved moved;
            check(Piston::fragile(Blocks::Torch) && Piston::fragile(Blocks::Pumpkin) && !Piston::fragile(Blocks::Stone) && !Piston::fragile(Blocks::RailNorthSouth),
                  "torches and pumpkins break, stone and rails do not");
            check(Piston::extend(bench, 0, 0, 0, &moved) && bench.at(2, 0, 0) == Blocks::Stone, "a piston pushes stone over a torch");
            check(moved.broken.size() == 1 && moved.broken[0].second == Blocks::Torch, "and the torch is broken off");
            check(!Piston::movable(Blocks::Obsidian) && !Piston::movable(Blocks::EnchantingTable) && !Piston::movable(Blocks::Hopper),
                  "obsidian, tables and hoppers do not move");
        }

        // The crafter is a bench that answers to a pulse, so what it
        // makes is whatever the bench would make of the same grid.
        // Checked here rather than against a world: the block is a
        // trigger, and the recipe underneath it is the one every other
        // bench in the game uses.
        {
            ItemStack grid[9];
            for (int i : { 0, 1, 3, 4 }) { grid[i].id = Blocks::Planks; grid[i].count = 1; }
            const CraftOutput made = Crafting::match(grid, 3);
            check(made.valid() && made.id == Blocks::CraftingTable,
                  "a crafter loaded with four planks would make a bench");

            Crafting::consume(grid, 3);
            for (int i : { 0, 1, 3, 4 })
                check(grid[i].empty(), "and spends one of each when it fires");

            check(!Crafting::match(grid, 3).valid(), "an empty crafter makes nothing");
        }

        check(isCrafter(Blocks::Crafter) && isCrafter(Blocks::CrafterLit),
              "both crafters are crafters");
        check(lightEmission(Blocks::CrafterLit) > lightEmission(Blocks::Crafter),
              "and the one that has just fired shows it");

        {
            ItemStack bench[9];
            const StackId wanted[9] = {
                Items::IronIngot, Items::IronIngot,     Items::IronIngot,
                Items::IronIngot, Blocks::CraftingTable, Items::IronIngot,
                Items::Redstone,  Blocks::PistonNorth,   Items::Redstone };
            for (int i = 0; i < 9; ++i) { bench[i].id = wanted[i]; bench[i].count = 1; }
            const CraftOutput out = Crafting::match(bench, 3);
            check(out.valid() && out.id == Blocks::Crafter, "and a crafter can be made at all");
        }

        // Crafted, or there is no way to hold one.
        {
            ItemStack grid[9];
            const StackId want[9] = {
                Blocks::Planks,      Blocks::Planks,   Blocks::Planks,
                Blocks::Cobblestone, Items::IronIngot, Blocks::Cobblestone,
                Blocks::Cobblestone, Items::Redstone,  Blocks::Cobblestone };
            for (int i = 0; i < 9; ++i) { grid[i].id = want[i]; grid[i].count = 1; }

            const CraftOutput made = Crafting::match(grid, 3);
            check(made.valid() && made.id == Blocks::PistonNorth, "a piston can be made");

            ItemStack sticky[9];
            sticky[0].id = Blocks::PistonNorth; sticky[0].count = 1;
            sticky[1].id = Items::MagmaCream;   sticky[1].count = 1;
            const CraftOutput stuck = Crafting::match(sticky, 2);
            check(stuck.valid() && stuck.id == Blocks::StickyPistonNorth,
                  "and made sticky with what a magma cube leaves");
        }
    }

    void testTrialChambers()
    {
        section("trial chambers: where they are and what is in them");

        constexpr uint32_t SEED = 4242u;

        // Deterministic, which is the whole point of planning a
        // structure rather than storing one: every chunk the chamber
        // reaches into works the same plan out for itself.
        for (int rx = -3; rx <= 3; ++rx)
            for (int rz = -3; rz <= 3; ++rz)
            {
                const TrialChamber::Site a = TrialChamber::siteFor(rx, rz, SEED);
                const TrialChamber::Site b = TrialChamber::siteFor(rx, rz, SEED);
                check(a.exists == b.exists && a.x == b.x && a.z == b.z && a.y == b.y,
                      "a chamber is in the same place every time it is asked for");
            }

        // Negative coordinates round the right way. Plain division
        // rounds towards zero, which would make the squares either side
        // of the origin twice as wide and put two chambers in one.
        check(TrialChamber::regionOf(0) == 0, "the origin is in square zero");
        check(TrialChamber::regionOf(TrialChamber::REGION) == 1, "and one square along is one");
        check(TrialChamber::regionOf(-1) == -1, "a block west of it is in square minus one");
        check(TrialChamber::regionOf(-TrialChamber::REGION) == -1,
              "and so is the far side of that square");

        // No two chambers can meet. They are cut out of the rock, so
        // two overlapping ones would hollow each other's walls away.
        std::vector<TrialChamber::Site> sites;
        for (int rx = -6; rx <= 6; ++rx)
            for (int rz = -6; rz <= 6; ++rz)
            {
                const TrialChamber::Site site = TrialChamber::siteFor(rx, rz, SEED);
                if (site.exists) sites.push_back(site);
            }

        check(!sites.empty(), "there are chambers in the world at all");

        bool anyOverlap = false;
        for (size_t a = 0; a < sites.size(); ++a)
            for (size_t b = a + 1; b < sites.size(); ++b)
            {
                const int reach = TrialChamber::MAX_RADIUS * 2;
                if (std::abs(sites[a].x - sites[b].x) <= reach &&
                    std::abs(sites[a].z - sites[b].z) <= reach)
                    anyOverlap = true;
            }
        check(!anyOverlap, "and no two of them reach into each other");

        // Deep enough to be underground anywhere, which is what makes a
        // chamber something you go down to.
        for (const TrialChamber::Site& site : sites)
            check(site.y >= TrialChamber::FLOOR_MIN && site.y <= TrialChamber::FLOOR_MAX,
                  "every chamber is under the rock rather than on it");

        // The plan itself.
        const TrialChamber::Site site = sites.front();
        const std::vector<TrialChamber::Room> rooms = TrialChamber::plan(site, SEED);

        check(static_cast<int>(rooms.size()) == TrialChamber::GRID * TrialChamber::GRID,
              "a chamber is a full grid of rooms");
        check(TrialChamber::spawnerCount(site, SEED) > 0,
              "with something to fight in it");
        check(TrialChamber::vaultCount(site, SEED) > 0,
              "and somewhere for the keys to be spent");

        int entrances = 0;
        for (const TrialChamber::Room& room : rooms)
            if (room.kind == TrialChamber::Kind::Entrance) ++entrances;
        check(entrances == 1, "and exactly one way in");

        // Rooms must not overlap each other either, or the stamp would
        // line one room's wall through the middle of the next.
        bool roomsOverlap = false;
        for (size_t a = 0; a < rooms.size(); ++a)
            for (size_t b = a + 1; b < rooms.size(); ++b)
            {
                const TrialChamber::Room& p = rooms[a];
                const TrialChamber::Room& q = rooms[b];
                if (p.x < q.x + q.width && q.x < p.x + p.width &&
                    p.z < q.z + q.depth && q.z < p.z + p.depth)
                    roomsOverlap = true;
            }
        check(!roomsOverlap, "no two rooms stand in the same place");

        // roomAt agrees with the plan, which is what the stamp relies on.
        for (const TrialChamber::Room& room : rooms)
        {
            TrialChamber::Room found;
            const bool hit = TrialChamber::roomAt(site, SEED, room.x + room.width / 2,
                                                  room.z + room.depth / 2, found);
            check(hit && found.kind == room.kind,
                  std::string("the middle of a ") + TrialChamber::name(room.kind) +
                      " is in that room");
        }

        TrialChamber::Room outside;
        check(!TrialChamber::roomAt(site, SEED, site.x + 400, site.z, outside),
              "and a block well clear of it is in no room at all");

        section("trial chambers: the fight and the reward");

        // The breeze belongs to the chambers. Anything else in a wave
        // is something you could have met on the surface; the breeze
        // is the one thing you cannot.
        bool sendsBreeze = false;
        for (int i = 0; i < 400; ++i)
            if (Trial::speciesFor(site.x + i, site.y, site.z, SEED, i % Trial::WAVE_SIZE) ==
                MobId::Breeze)
                sendsBreeze = true;
        check(sendsBreeze, "a trial spawner is where breezes come from");

        // And the only place. It was in the night spawns while there
        // was nowhere else to meet one; there is now.
        {
            bool nightly = false;
            for (int i = 0; i < EntityManager::nightSpeciesCount(); ++i)
                if (EntityManager::nightSpecies(i) == MobId::Breeze) nightly = true;
            check(!nightly, "and not something you meet on the surface at night");
            check(EntityManager::nightSpeciesCount() > 0, "though the night still sends plenty");
        }

        for (int i = 0; i < 200; ++i)
        {
            const MobId sent = Trial::speciesFor(site.x + i, site.y, site.z, SEED, 0);
            check(mobType(sent).spawnClass == SpawnClass::Hostile,
                  "and everything it sends is something that fights");
        }

        // The same spawner always sends the same thing.
        for (int i = 0; i < Trial::WAVE_SIZE; ++i)
            check(Trial::speciesFor(site.x, site.y, site.z, SEED, i) ==
                      Trial::speciesFor(site.x, site.y, site.z, SEED, i),
                  "a spawner sends the same wave every time");

        // A vault always pays something, and pays the same thing twice.
        for (int i = 0; i < 120; ++i)
        {
            const std::vector<Trial::Stack> loot =
                Trial::vaultLoot(site.x + i * 3, site.y, site.z - i, SEED);
            check(!loot.empty(), "a vault is never empty");

            int total = 0;
            for (const Trial::Stack& stack : loot)
            {
                check(stack.count > 0, "and never holds nothing of something");
                total += stack.count;
            }
            check(total > 0, "there is something in it to take");
        }

        {
            const std::vector<Trial::Stack> once = Trial::vaultLoot(site.x, site.y, site.z, SEED);
            const std::vector<Trial::Stack> again = Trial::vaultLoot(site.x, site.y, site.z, SEED);
            check(once.size() == again.size(), "a vault holds the same thing every time");
            for (size_t i = 0; i < once.size(); ++i)
                check(once[i].id == again[i].id && once[i].count == again[i].count,
                      "down to the last item");
        }

        // And it is actually built. Planning one is half the job: the
        // stamp has to put it in the chunks, and a chamber that exists
        // only in the plan is a chamber nobody can walk into.
        {
            const WorldGen generator(SEED);
            const TrialChamber::Site built = generator.nearestChamber(0, 0, 4);
            check(built.exists, "there is a chamber within a few squares of the origin");

            int brick = 0, spawners = 0, vaults = 0, chests = 0, ladders = 0, air = 0;
            const int chunkX = (built.x >= 0 ? built.x : built.x - Chunk::SX + 1) / Chunk::SX;
            const int chunkZ = (built.z >= 0 ? built.z : built.z - Chunk::SZ + 1) / Chunk::SZ;

            for (int cz = chunkZ - 1; cz <= chunkZ + 1; ++cz)
                for (int cx = chunkX - 1; cx <= chunkX + 1; ++cx)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    for (int y = built.y - 2; y <= built.y + TrialChamber::HEADROOM + 1; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                            {
                                if (y < 0 || y >= Chunk::SY) continue;
                                switch (chunk.getBlock(lx, y, lz))
                                {
                                    case Blocks::TuffBricks: ++brick; break;
                                    case Blocks::TrialSpawner: ++spawners; break;
                                    case Blocks::Vault: ++vaults; break;
                                    case Blocks::Chest: ++chests; break;
                                    case Blocks::LadderNorth: ++ladders; break;
                                    case Blocks::Air: ++air; break;
                                    default: break;
                                }
                            }
                }

            check(brick > 200, "the chamber is lined with tuff brick");
            check(air > 200, "and hollow inside");
            check(spawners > 0, "with spawners in it");
            check(vaults > 0, "and vaults");
            (void)chests;

            // The way in, where there is one. A chamber under the sea
            // is sealed rather than opening a ladder out of the sea
            // bed, so which of the two this one is depends on what is
            // over it.
            const int over = generator.surfaceHeight(built.x, built.z);
            if (over > WorldGen::SEA_LEVEL)
                check(ladders > 0, "a chamber under land has a ladder up to the surface");
            else
                check(ladders == 0, "and one under the sea is sealed instead");

            // Whichever of the two that one was, check the other kind
            // as well: the nearest chamber to the origin is in an
            // ocean on this seed, and testing only that one would have
            // left the ladder itself unchecked.
            bool checkedLand = over > WorldGen::SEA_LEVEL;
            for (int rx = -4; rx <= 4 && !checkedLand; ++rx)
                for (int rz = -4; rz <= 4 && !checkedLand; ++rz)
                {
                    const TrialChamber::Site dry = TrialChamber::siteFor(rx, rz, SEED);
                    if (!dry.exists) continue;
                    if (generator.surfaceHeight(dry.x, dry.z) <= WorldGen::SEA_LEVEL) continue;

                    const int cx = (dry.x >= 0 ? dry.x : dry.x - Chunk::SX + 1) / Chunk::SX;
                    const int cz = (dry.z >= 0 ? dry.z : dry.z - Chunk::SZ + 1) / Chunk::SZ;
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    int rungs = 0;
                    for (int y = dry.y + 1; y < Chunk::SY; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                                if (chunk.getBlock(lx, y, lz) == Blocks::LadderNorth) ++rungs;

                    check(rungs > 0, "a chamber under dry land is reachable from the top");
                    checkedLand = true;
                }
            check(checkedLand, "and there is a chamber under dry land to check");
        }

        section("trial chambers: the ominous half");

        // The whole loop, in order: a plain vault can pay a bottle,
        // the bottle makes the next spawner ominous, an ominous
        // spawner pays an ominous key, and that key opens the vaults
        // that hold what is only in there.
        {
            bool anyBottle = false;
            for (int i = 0; i < 400 && !anyBottle; ++i)
                for (const Trial::Stack& stack :
                     Trial::vaultLoot(site.x + i * 5, site.y, site.z - i * 3, SEED))
                    if (stack.id == Items::OminousBottle) anyBottle = true;

            check(anyBottle, "a plain vault can pay an ominous bottle");
        }

        check(Trial::waveSize(true) > Trial::waveSize(false),
              "an ominous spawner sends more than a plain one");
        check(Trial::keyFor(true) == Items::OminousTrialKey, "and pays an ominous key");
        check(Trial::keyFor(false) == Items::TrialKey, "where a plain one pays a plain key");
        check(Trial::keyFor(true) != Trial::keyFor(false),
              "and the two are not the same key, which is the whole point of it");

        // An ominous vault is worth the trouble: more of everything a
        // plain one has, and one thing that is only ever in there.
        {
            int richer = 0, poorer = 0;
            bool sawExtra = false;

            for (int i = 0; i < 200; ++i)
            {
                const int x = site.x + i * 7, z = site.z - i * 11;
                int plain = 0, ominous = 0;

                for (const Trial::Stack& stack : Trial::vaultLoot(x, site.y, z, SEED))
                    plain += stack.count;
                for (const Trial::Stack& stack : Trial::ominousVaultLoot(x, site.y, z, SEED))
                {
                    ominous += stack.count;
                    if (stack.id == Items::Mace || stack.id == Items::BreezeRod ||
                        stack.id == Items::EnderPearl)
                        sawExtra = true;
                }

                if (ominous > plain) ++richer;
                else ++poorer;
            }

            check(richer > poorer * 4, "an ominous vault pays better than a plain one");
            check(sawExtra, "and holds things a plain one never does");
        }

        {
            const std::vector<Trial::Stack> once =
                Trial::ominousVaultLoot(site.x, site.y, site.z, SEED);
            const std::vector<Trial::Stack> again =
                Trial::ominousVaultLoot(site.x, site.y, site.z, SEED);
            check(once.size() == again.size(),
                  "an ominous vault holds the same thing every time, like any other");
            check(!once.empty(), "and is never empty");
        }

        // The blocks themselves.
        check(isVault(Blocks::OminousVault) && isVault(Blocks::OminousVaultOpen),
              "an ominous vault is a vault");
        check(isOminousVault(Blocks::OminousVault), "and knows which sort it is");
        check(!isOminousVault(Blocks::Vault), "where a plain one does too");
        check(blockInfo(Blocks::OminousVault).hardness < 0.0f,
              "and neither can be mined open");

        // Bad omen does nothing to the body. That is deliberate: it is
        // a flag you carry rather than a thing you feel, and if it
        // slowed you down the hard mode would be hard twice over.
        {
            Effects::Set set;
            Effects::give(set, Effects::Kind::BadOmen, 0, Trial::OMEN_SECONDS);
            check(set.has(Effects::Kind::BadOmen), "an omen is carried");
            check(Effects::speedMultiplier(set) == 1.0f, "and does not slow you down");
            check(Effects::attackBonus(set) == 0, "nor weaken what you hit with");
        }

        // One ominous vault to a vault room, with the plain ones
        // either side of it.
        {
            const WorldGen generator(SEED);
            const TrialChamber::Site built = generator.nearestChamber(0, 0, 4);

            int plain = 0, ominous = 0;
            const int chunkX = (built.x >= 0 ? built.x : built.x - Chunk::SX + 1) / Chunk::SX;
            const int chunkZ = (built.z >= 0 ? built.z : built.z - Chunk::SZ + 1) / Chunk::SZ;

            for (int cz = chunkZ - 1; cz <= chunkZ + 1; ++cz)
                for (int cx = chunkX - 1; cx <= chunkX + 1; ++cx)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);

                    for (int y = built.y - 1; y <= built.y + TrialChamber::HEADROOM; ++y)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                            for (int lx = 0; lx < Chunk::SX; ++lx)
                            {
                                if (y < 0 || y >= Chunk::SY) continue;
                                const BlockId block = chunk.getBlock(lx, y, lz);
                                if (block == Blocks::Vault) ++plain;
                                if (block == Blocks::OminousVault) ++ominous;
                            }
                }

            check(ominous > 0, "a chamber has ominous vaults in it");
            check(plain > ominous, "and more plain ones than ominous");
        }

        // The spawner's three states, and what tells them apart.
        check(isTrialSpawner(Blocks::TrialSpawner), "a spawner waiting is a spawner");
        check(isTrialSpawner(Blocks::TrialSpawnerActive), "so is one with its wave out");
        check(isTrialSpawner(Blocks::TrialSpawnerSpent), "and one that is finished with");
        check(!isTrialSpawner(Blocks::Vault), "a vault is not one");
        check(lightEmission(Blocks::TrialSpawnerActive) >
                  lightEmission(Blocks::TrialSpawnerSpent),
              "a spawner with its wave out is lit and a spent one is dark");
        check(isVault(Blocks::Vault) && isVault(Blocks::VaultOpen), "both vaults are vaults");
        check(blockInfo(Blocks::Vault).hardness < 0.0f, "and a vault cannot be mined open");
        check(blockInfo(Blocks::TrialSpawner).hardness < 0.0f,
              "nor a spawner taken home");
    }

    void testTrickyTrials()
    {
        section("1.21: the mace, the wind charge and the breeze");

        // The smash curve, which is Minecraft's: four a block for the
        // first three, two a block for the next five, one after that.
        check(Tools::maceSmashBonus(0.0f) == 0, "a mace swung standing still is just a club");
        check(Tools::maceSmashBonus(1.0f) == 4, "one block of fall is worth four");
        check(Tools::maceSmashBonus(3.0f) == 12, "three blocks is twelve");
        check(Tools::maceSmashBonus(8.0f) == 22, "eight blocks is twenty-two");
        check(Tools::maceSmashBonus(18.0f) == 32, "and after that it is a point a block");

        // It never pays less for falling further.
        int last = -1;
        for (float f = 0.0f; f <= 40.0f; f += 0.5f)
        {
            const int here = Tools::maceSmashBonus(f);
            check(here >= last, "the smash never pays less for falling further");
            last = here;
        }

        check(Tools::kindOf(Items::Mace) == Tools::Kind::Mace, "a mace is its own kind of thing");
        check(Tools::attackDamage(Items::Mace) >= Tools::attackDamage(Items::IronSword),
              "and hits at least as hard as an iron sword before the fall is counted");
        check(!Tools::canHarvest(Items::Mace, Blocks::Stone), "it is no use against stone");
        check(Tools::maxDurability(Tools::tierOf(Items::Mace)) > 0, "and it wears out like a tool");

        // The shove: hardest at the middle, nothing at all past the
        // edge, and never straight down.
        {
            const glm::vec3 burst(0.0f, 0.0f, 0.0f);
            const glm::vec3 near = Arrows::windImpulse(burst, glm::vec3(0.5f, 0.0f, 0.0f));
            const glm::vec3 far = Arrows::windImpulse(burst, glm::vec3(3.0f, 0.0f, 0.0f));
            const glm::vec3 outside = Arrows::windImpulse(burst, glm::vec3(9.0f, 0.0f, 0.0f));

            check(glm::length(near) > glm::length(far), "a wind charge shoves hardest up close");
            check(glm::length(outside) == 0.0f, "and not at all beyond its reach");
            check(near.x > 0.0f, "it pushes away from where it went off");

            // Under your own feet it throws you up, which is the whole
            // point of one.
            const glm::vec3 underfoot = Arrows::windImpulse(glm::vec3(0.0f, -0.5f, 0.0f),
                                                            glm::vec3(0.0f, 0.0f, 0.0f));
            check(underfoot.y > 0.0f, "one at your feet throws you into the air");

            const glm::vec3 above = Arrows::windImpulse(glm::vec3(0.0f, 2.0f, 0.0f),
                                                        glm::vec3(0.0f, 0.0f, 0.0f));
            check(above.y > 0.0f, "and one over your head never drives you into the floor");
        }

        // The breeze itself.
        const MobType& breeze = mobType(MobId::Breeze);
        check(breeze.shootsWind, "a breeze throws wind charges");
        check(!breeze.shootsFire, "and nothing else");
        check(breeze.arrowDamage <= 2, "they hurt almost not at all");
        check(breeze.arrowRange > 12.0f, "which is why it keeps its distance");
        check(breeze.drop == Items::BreezeRod, "it leaves a rod");
        check(breeze.secondDrop == Items::WindCharge, "and some of what it was throwing");
        check(breeze.sheetWidth == 32 && breeze.sheetHeight == 32,
              "off the smallest sheet in the game");
        check(MobSkin::fits(breeze.model, breeze.sheetWidth, breeze.sheetHeight),
              "and every patch of it lands on that sheet");

        // The chain: a breeze gives a rod, a rod gives charges or a
        // mace. Without it the mace is a thing you can never hold.
        {
            ItemStack rod[9];
            rod[0].id = Items::BreezeRod;
            rod[0].count = 1;
            const CraftOutput charges = Crafting::match(rod, 2);
            check(charges.valid() && charges.id == Items::WindCharge && charges.count == 4,
                  "a breeze rod makes four wind charges");

            ItemStack mace[9];
            mace[0].id = Items::BreezeRod;  mace[0].count = 1;
            mace[1].id = Items::IronIngot;  mace[1].count = 1;
            const CraftOutput made = Crafting::match(mace, 2);
            check(made.valid() && made.id == Items::Mace, "and a rod with iron makes the mace");
            check(itemInfo(Items::Mace).maxStack == 1, "which does not stack");
        }
    }

    void testRedstone()
    {
        section("redstone: power, and how far it goes");

        // A patch of world in a map, so a circuit can be laid out and
        // switched on with no chunks, no renderer and no window.
        struct Bench : Redstone::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> cells;

            BlockId at(int x, int y, int z) const override
            {
                auto it = cells.find({ x, y, z });
                return it == cells.end() ? Blocks::Air : it->second;
            }
            void set(int x, int y, int z, BlockId block) override
            {
                cells[{ x, y, z }] = block;
            }
        };

        // What a run of dust with a lever at one end and a lamp at the
        // other does, for a run of whatever length.
        auto runOf = [](int length, BlockId source) {
            Bench bench;
            for (int x = 0; x < length; ++x) bench.set(x, 0, 0, Blocks::RedstoneDust);
            bench.set(-1, 0, 0, source);
            bench.set(length, 0, 0, Blocks::RedstoneLamp);
            Redstone::update(bench, 0, 0, 0);
            return bench;
        };

        {
            Bench lit = runOf(4, Blocks::LeverOn);
            for (int x = 0; x < 4; ++x)
                check(lit.at(x, 0, 0) == Blocks::RedstoneDustLit,
                      "a thrown lever lights the whole run");
            check(lit.at(4, 0, 0) == Blocks::RedstoneLampLit, "and the lamp at the end of it");
        }

        {
            Bench dark = runOf(4, Blocks::LeverOff);
            for (int x = 0; x < 4; ++x)
                check(dark.at(x, 0, 0) == Blocks::RedstoneDust, "a lever off leaves it dark");
            check(dark.at(4, 0, 0) == Blocks::RedstoneLamp, "and the lamp with it");
        }

        // Switching one off again has to put the line out. Relighting
        // from the sources rather than from the change is what makes
        // this work at all: the first rule only ever spread power and
        // had no way of taking it back.
        {
            Bench bench = runOf(6, Blocks::LeverOn);
            check(bench.at(5, 0, 0) == Blocks::RedstoneDustLit, "the far end lights");

            bench.set(-1, 0, 0, Blocks::LeverOff);
            Redstone::update(bench, -1, 0, 0);
            for (int x = 0; x < 6; ++x)
                check(bench.at(x, 0, 0) == Blocks::RedstoneDust,
                      "and goes out again when the lever is thrown back");
            check(bench.at(6, 0, 0) == Blocks::RedstoneLamp, "taking the lamp with it");
        }

        // Fifteen blocks and no further, which is Minecraft's.
        {
            Bench bench;
            for (int x = 0; x < 20; ++x) bench.set(x, 0, 0, Blocks::RedstoneDust);
            bench.set(-1, 0, 0, Blocks::RedstoneBlock);
            Redstone::update(bench, 0, 0, 0);

            check(bench.at(Redstone::MAX_RUN - 1, 0, 0) == Blocks::RedstoneDustLit,
                  "dust carries fifteen blocks");
            check(bench.at(Redstone::MAX_RUN, 0, 0) == Blocks::RedstoneDust,
                  "and not a sixteenth");
        }

        // A gap stops it. Nothing jumps across a missing block.
        {
            Bench bench;
            for (int x = 0; x < 8; ++x)
                if (x != 3) bench.set(x, 0, 0, Blocks::RedstoneDust);
            bench.set(-1, 0, 0, Blocks::RedstoneTorch);
            Redstone::update(bench, 0, 0, 0);

            check(bench.at(2, 0, 0) == Blocks::RedstoneDustLit, "a torch lights what it reaches");
            check(bench.at(4, 0, 0) == Blocks::RedstoneDust, "and a gap stops it dead");
        }

        // Every source, and nothing else.
        check(Redstone::isSource(Blocks::RedstoneTorch), "a torch gives power out");
        check(Redstone::isSource(Blocks::RedstoneBlock), "so does a block of redstone");
        check(Redstone::isSource(Blocks::LeverOn), "and a lever thrown on");
        check(Redstone::isSource(Blocks::ButtonOn), "and a button held down");
        check(!Redstone::isSource(Blocks::LeverOff), "a lever off does not");
        check(!Redstone::isSource(Blocks::ButtonOff), "nor a button nobody is pressing");
        check(!Redstone::isSource(Blocks::RedstoneTorchOff), "nor a torch that is out");
        check(!Redstone::isSource(Blocks::RedstoneDustLit), "dust carries power, it does not make it");

        // A lever stays where it is put and a button comes back up.
        check(Redstone::toggled(Blocks::LeverOff) == Blocks::LeverOn, "a lever throws on");
        check(Redstone::toggled(Blocks::LeverOn) == Blocks::LeverOff, "and back off");
        check(Redstone::toggled(Blocks::ButtonOff) == Blocks::ButtonOn, "a button presses in");
        check(Redstone::toggled(Blocks::ButtonOn) == Blocks::ButtonOff, "and lets go");

        // A door swings as one thing. Powering either half opens both,
        // which is the whole of what makes a redstone door a door.
        {
            Bench bench;
            bench.set(0, 0, 0, doorPart(0, false, false));
            bench.set(0, 1, 0, doorPart(0, true, false));
            bench.set(0, 0, 0 + 1, Blocks::RedstoneDust);
            bench.set(-1, 0, 1, Blocks::LeverOn);
            Redstone::update(bench, 0, 0, 0);

            check(doorIsOpen(bench.at(0, 0, 0)), "power opens the bottom of a door");
            check(doorIsOpen(bench.at(0, 1, 0)), "and the top with it");
            check(blockFacing(bench.at(0, 0, 0)) == 0, "without turning it round");

            bench.set(-1, 0, 1, Blocks::LeverOff);
            Redstone::update(bench, -1, 0, 1);
            check(!doorIsOpen(bench.at(0, 0, 0)) && !doorIsOpen(bench.at(0, 1, 0)),
                  "and it shuts again when the power goes");
        }

        // A lever on a block, which is how anyone actually builds one:
        // the switch never touches the dust, the block it stands on
        // does. Getting this wrong made the first circuit I laid out
        // look right and do nothing.
        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::Cobblestone);
            bench.set(0, 1, 0, Blocks::LeverOn);
            for (int x = 1; x <= 5; ++x) bench.set(x, 0, 0, Blocks::RedstoneDust);
            bench.set(6, 0, 0, Blocks::RedstoneLamp);
            Redstone::update(bench, 0, 0, 0);

            check(bench.at(1, 0, 0) == Blocks::RedstoneDustLit,
                  "a lever standing on a block lights the dust beside it");
            check(bench.at(5, 0, 0) == Blocks::RedstoneDustLit, "all the way along");
            check(bench.at(6, 0, 0) == Blocks::RedstoneLampLit, "and the lamp at the end");

            bench.set(0, 1, 0, Blocks::LeverOff);
            Redstone::update(bench, 0, 1, 0);
            check(bench.at(1, 0, 0) == Blocks::RedstoneDust &&
                      bench.at(6, 0, 0) == Blocks::RedstoneLamp,
                  "and switching it off puts the whole line out");
        }

        // The block has to be carrying it, not merely be near it. A
        // plain cobble next to a line does not make a second source.
        {
            Bench bench;
            bench.set(0, 0, 0, Blocks::Cobblestone);
            bench.set(1, 0, 0, Blocks::RedstoneDust);
            Redstone::update(bench, 0, 0, 0);
            check(bench.at(1, 0, 0) == Blocks::RedstoneDust,
                  "a bare block beside dust powers nothing");
            check(!Redstone::carries(bench, 0, 0, 0), "and carries nothing itself");
        }

        // Dust is drawn flat like a rail and must never be taken for
        // one: laid beside a track it would bend into the line.
        check(isFlat(Blocks::RedstoneDust) && isFlat(Blocks::RedstoneDustLit),
              "dust lies flat on the floor");
        check(!isRail(Blocks::RedstoneDust) && !isRail(Blocks::RedstoneDustLit),
              "and is not a rail");
        check(!railLinks(Blocks::RedstoneDust, 0) && !railLinks(Blocks::RedstoneDust, 1),
              "so nothing joins onto it");

        // What the lamp is for: it is the only block here that lights
        // the room, and only when it is on.
        check(lightEmission(Blocks::RedstoneLampLit) > 0, "a lit lamp gives light");
        check(lightEmission(Blocks::RedstoneLamp) == 0, "and an unlit one gives none");
        check(lightEmission(Blocks::RedstoneTorch) > 0, "a redstone torch glows a little");

        // The recipes, end to end: a seam gives dust, dust packs into a
        // block and comes back out again.
        check(Smelting::resultOf(Blocks::RedstoneOre) == Blocks::Air,
              "redstone ore is not something you smelt");

        {
            ItemStack nine[9];
            for (int i = 0; i < 9; ++i) { nine[i].id = Items::Redstone; nine[i].count = 1; }
            const CraftOutput packed = Crafting::match(nine, 3);
            check(packed.valid() && packed.id == Blocks::RedstoneBlock,
                  "nine dust pack into a block");

            ItemStack back[9];
            back[0].id = Blocks::RedstoneBlock;
            back[0].count = 1;
            const CraftOutput loose = Crafting::match(back, 2);
            check(loose.valid() && loose.id == Items::Redstone && loose.count == 9,
                  "and the block gives all nine back");
        }

        {
            ItemStack lamp[9];
            lamp[1].id = Items::Redstone;  lamp[1].count = 1;
            lamp[3].id = Items::Redstone;  lamp[3].count = 1;
            lamp[4].id = Blocks::Glowstone; lamp[4].count = 1;
            lamp[5].id = Items::Redstone;  lamp[5].count = 1;
            lamp[7].id = Items::Redstone;  lamp[7].count = 1;
            const CraftOutput made = Crafting::match(lamp, 3);
            check(made.valid() && made.id == Blocks::RedstoneLamp,
                  "glowstone wrapped in dust makes a lamp");
        }
    }

    void testBiomes()
    {
        section("biomes: every one of them is somewhere");

        const WorldGen generator(4242u);

        // A coarse sweep over a few thousand blocks each way. A biome
        // that exists in the enum and nowhere in the world is the bug
        // worth catching: the jungle's first rule took the whole hot
        // end and the savanna never appeared at all.
        int seen[16] = { 0 };
        for (int z = -4000; z <= 4000; z += 97)
            for (int x = -4000; x <= 4000; x += 97)
                ++seen[static_cast<int>(generator.biomeAt(x, z)) & 15];

        const Biome every[] = { Biome::Ocean, Biome::Beach, Biome::Plains, Biome::Forest,
                                Biome::Desert, Biome::Mountains, Biome::Snowy,
                                Biome::Jungle, Biome::Savanna };
        for (Biome biome : every)
            check(seen[static_cast<int>(biome)] > 0,
                  std::string(biomeName(biome)) + " is somewhere in the world");

        // Named, too. A biome with no name reads as an empty line on
        // the debug screen.
        for (Biome biome : every)
            check(biomeName(biome)[0] != '\0',
                  std::string("and has a name on the debug screen"));

        // The woods. Six trees, six planks, and a log of each kind that
        // breaks down into its own plank rather than into oak.
        const BlockId logs[] = { Blocks::Log, Blocks::BirchLog, Blocks::SpruceLog,
                                 Blocks::DarkOakLog, Blocks::JungleLog, Blocks::AcaciaLog };
        for (BlockId log : logs)
        {
            check(isLog(log), std::string(blockInfo(log).name) + " counts as a log");
            check(blockMaterial(log) == Material::Wood,
                  std::string(blockInfo(log).name) + " is cut with an axe");

            const BlockId planks = planksFromLog(log);
            check(isPlanks(planks),
                  std::string(blockInfo(log).name) + " breaks down into planks");

            ItemStack grid[9];
            grid[0].id = log;
            grid[0].count = 1;
            const CraftOutput made = Crafting::match(grid, 2);
            check(made.valid() && made.id == planks && made.count == 4,
                  std::string("and four of them at the bench"));
        }

        const BlockId leaves[] = { Blocks::Leaves, Blocks::BirchLeaves, Blocks::SpruceLeaves,
                                   Blocks::DarkOakLeaves, Blocks::JungleLeaves,
                                   Blocks::AcaciaLeaves };
        for (BlockId leaf : leaves)
        {
            check(isLeaves(leaf), std::string(blockInfo(leaf).name) + " counts as leaves");
            check(blockMaterial(leaf) == Material::Plant,
                  std::string(blockInfo(leaf).name) + " tears rather than chops");
        }

        // The jungle and the savanna actually grow their own trees, and
        // the one is far thicker than the other.
        auto woodIn = [&](int cx, int cz, BlockId log) {
            Chunk chunk(ChunkPos{ cx, cz });
            generator.generate(chunk);
            int count = 0;
            for (int y = 0; y < Chunk::SY; ++y)
                for (int lz = 0; lz < Chunk::SZ; ++lz)
                    for (int lx = 0; lx < Chunk::SX; ++lx)
                        if (chunk.getBlock(lx, y, lz) == log) ++count;
            return count;
        };

        int jungleChunks = 0, savannaChunks = 0, jungleWood = 0, acaciaWood = 0;
        for (int cz = -24; cz <= 24 && (jungleChunks < 3 || savannaChunks < 3); ++cz)
            for (int cx = -24; cx <= 24 && (jungleChunks < 3 || savannaChunks < 3); ++cx)
            {
                const Biome here = generator.biomeAt(cx * Chunk::SX + 8, cz * Chunk::SZ + 8);
                if (here == Biome::Jungle && jungleChunks < 3)
                {
                    ++jungleChunks;
                    jungleWood += woodIn(cx, cz, Blocks::JungleLog);
                }
                else if (here == Biome::Savanna && savannaChunks < 3)
                {
                    ++savannaChunks;
                    acaciaWood += woodIn(cx, cz, Blocks::AcaciaLog);
                }
            }

        check(jungleChunks == 3 && savannaChunks == 3,
              "there is a jungle and a savanna within a few hundred chunks");
        check(jungleWood > 0, "the jungle grows jungle trees");
        check(acaciaWood > 0, "and the savanna grows acacias");
        check(jungleWood > acaciaWood,
              "with the jungle far the thicker of the two");
    }

    void testOreRates()
    {
        section("ore generation");

        // Four chunks is enough to see the shape without making the
        // test slow. What matters is not the exact count -- that is what
        // --oreinfo is for -- but that the ordering and the depth limits
        // hold, which is what a careless retune would break.
        const WorldGen generator(4242u);

        long long coal = 0, iron = 0, gold = 0, diamond = 0, emerald = 0, stone = 0;
        int deepestGold = 999, deepestDiamond = 999;
        int highestGold = -1, highestDiamond = -1;
        int highestEmerald = -1;

        for (int cz = 0; cz < 2; ++cz)
            for (int cx = 0; cx < 2; ++cx)
            {
                Chunk chunk(ChunkPos{ cx, cz });
                generator.generate(chunk);

                for (int y = 0; y < 128 && y < Chunk::SY; ++y)
                    for (int lz = 0; lz < Chunk::SZ; ++lz)
                        for (int lx = 0; lx < Chunk::SX; ++lx)
                        {
                            switch (chunk.getBlock(lx, y, lz))
                            {
                                case Blocks::Stone: ++stone; break;
                                case Blocks::CoalOre: ++coal; ++stone; break;
                                case Blocks::IronOre: ++iron; ++stone; break;
                                case Blocks::GoldOre:
                                    ++gold; ++stone;
                                    deepestGold = std::min(deepestGold, y);
                                    highestGold = std::max(highestGold, y);
                                    break;
                                case Blocks::DiamondOre:
                                    ++diamond; ++stone;
                                    deepestDiamond = std::min(deepestDiamond, y);
                                    highestDiamond = std::max(highestDiamond, y);
                                    break;
                                case Blocks::EmeraldOre:
                                    ++emerald; ++stone;
                                    highestEmerald = std::max(highestEmerald, y);
                                    break;
                                default: break;
                            }
                        }
            }

        check(stone > 10000, "there is stone to put ore in");
        check(coal > 0 && iron > 0, "coal and iron are there to be found");
        check(diamond > 0, "and so is diamond, somewhere");

        // Emerald is the rarest thing in the ground. Four chunks is a
        // small sample and it comes in ones rather than seams, so the
        // check is that it never outnumbers diamond rather than that
        // any turned up at all.
        check(emerald <= diamond,
              "emerald is rarer than diamond (" + std::to_string(emerald) + " to " +
                  std::to_string(diamond) + ")");
        check(highestEmerald < 48,
              "and stays deep, like the rest of what is worth digging for");

        // The ladder the whole tool progression rests on.
        check(coal > iron, "coal is commoner than iron");
        check(iron > gold, "iron is commoner than gold");
        check(gold > diamond, "and gold than diamond");

        // Rare means rare. At one in a hundred, mining was a formality
        // and the tool tiers gated nothing -- that is what these catch.
        auto perThousand = [&](long long n) {
            return 1000.0 * static_cast<double>(n) / static_cast<double>(stone);
        };

        check(perThousand(coal) < 40.0, "coal is not a tenth of the world");
        check(perThousand(iron) < 25.0, "nor is iron");
        check(perThousand(gold) < 6.0, "gold is rare");
        check(perThousand(diamond) < 3.0, "and diamond rarer still");

        // Depth. Diamond near the surface would skip the whole descent.
        if (highestDiamond >= 0) check(highestDiamond <= 20, "diamond keeps to the deep rock");
        if (highestGold >= 0) check(highestGold <= 36, "and gold stays well down");
        check(deepestDiamond >= 0, "and does not go below the world");
    }

    void testDoors()
    {
        section("doors");

        // Sixteen ids for one door: two halves, four facings, open or
        // shut. The helpers are the only thing that should know the
        // order, so this checks they agree with themselves.
        for (int facing = 0; facing < 4; ++facing)
            for (int top = 0; top < 2; ++top)
                for (int open = 0; open < 2; ++open)
                {
                    const BlockId id = doorPart(facing, top != 0, open != 0);

                    check(isDoor(id), "every part is a door");
                    check(blockFacing(id) == facing, "and faces the way it was built");
                    check(doorIsTop(id) == (top != 0), "and knows which half it is");
                    check(doorIsOpen(id) == (open != 0), "and whether it is open");
                }

        // All sixteen are distinct: an overlap would make two different
        // doors the same block.
        std::vector<BlockId> seen;
        for (int facing = 0; facing < 4; ++facing)
            for (int top = 0; top < 2; ++top)
                for (int open = 0; open < 2; ++open)
                    seen.push_back(doorPart(facing, top != 0, open != 0));

        std::sort(seen.begin(), seen.end());
        check(std::unique(seen.begin(), seen.end()) == seen.end(),
              "all sixteen parts are distinct");

        // Only the one you hold is obtainable; a door is placed as two
        // blocks from a single item.
        check(isObtainable(Blocks::DoorLowShutNorth), "a door is a thing you hold");
        check(!isObtainable(doorPart(0, true, false)), "its top half is not");
        check(!isObtainable(doorPart(1, false, false)), "nor a turned one");
        check(!isObtainable(doorPart(0, false, true)), "nor an opened one");

        // Swinging moves the panel a quarter turn, so a door that
        // blocked the way through no longer does.
        auto blocksMiddle = [](BlockId id) {
            // A probe where someone walking through would be.
            return blockOverlaps(id, 0, 0, 0, 0.3f, 0.1f, 0.0f, 0.7f, 0.9f, 0.15f);
        };

        check(blocksMiddle(doorPart(0, false, false)), "a shut door is in the way");
        check(!blocksMiddle(doorPart(0, false, true)), "an open one is not");

        // And it is somewhere else rather than nowhere: opening a door
        // moves it aside, it does not delete it.
        bool anywhere = false;
        for (int side = 0; side < 4 && !anywhere; ++side)
        {
            const float lo = (side == 1) ? 0.85f : 0.0f;
            const float hi = (side == 1) ? 1.0f : 0.15f;
            if (side == 1 || side == 3)
                anywhere = blockOverlaps(doorPart(0, false, true), 0, 0, 0,
                                         lo, 0.1f, 0.3f, hi, 0.9f, 0.7f);
        }
        check(anywhere, "an open door is still somewhere");

        // Both halves use the same tile set but not the same tile, so a
        // door does not read as two of the same panel stacked.
        check(blockInfo(doorPart(0, false, false)).tileSide !=
                  blockInfo(doorPart(0, true, false)).tileSide,
              "the two halves are drawn differently");
    }

    void testStairs()
    {
        section("stairs");

        // Four facings per material, and turning a flight round keeps
        // it the same material.
        const BlockId KINDS[4] = { Blocks::StoneStairsNorth, Blocks::CobbleStairsNorth,
                                   Blocks::PlankStairsNorth, Blocks::SandstoneStairsNorth };

        for (BlockId kind : KINDS)
        {
            for (int facing = 0; facing < 4; ++facing)
            {
                const BlockId turned = stairsFacing(kind, facing);
                check(isStairs(turned), "a turned flight is still stairs");
                check(blockFacing(turned) == facing, "and faces the way it was turned");
                check(stairsFacing(turned, 0) == kind, "and turns back to where it started");
            }

            check(isObtainable(kind), "the north flight is a thing you hold");
            check(!isObtainable(stairsFacing(kind, 1)), "the turned ones are not");
            check(Tools::requiredLevel(kind) == Tools::requiredLevel(stairsFacing(kind, 2)),
                  "and turning it does not change what mines it");
        }

        // Turning one material's stairs never yields another's.
        check(stairsFacing(Blocks::PlankStairsNorth, 3) != Blocks::StoneStairsWest,
              "oak stairs turned are still oak");

        // The shape. A flight facing north is solid in its bottom half
        // everywhere, and in its top half only at the south end.
        const BlockId north = Blocks::CobbleStairsNorth;

        auto solid = [&](float x, float y, float z) {
            // A small probe box around the point.
            return blockOverlaps(north, 0, 0, 0,
                                 x - 0.05f, y - 0.05f, z - 0.05f,
                                 x + 0.05f, y + 0.05f, z + 0.05f);
        };

        check(solid(0.5f, 0.25f, 0.25f), "the bottom half is solid at the low end");
        check(solid(0.5f, 0.25f, 0.75f), "and at the high end");
        check(solid(0.5f, 0.75f, 0.75f), "the step is solid at the high end");
        check(!solid(0.5f, 0.75f, 0.25f), "and open at the low end, which is what you walk up");

        // Turned round, the open corner moves to the other side.
        auto solidSouth = [&](float x, float y, float z) {
            return blockOverlaps(Blocks::CobbleStairsSouth, 0, 0, 0,
                                 x - 0.05f, y - 0.05f, z - 0.05f,
                                 x + 0.05f, y + 0.05f, z + 0.05f);
        };
        check(solidSouth(0.5f, 0.75f, 0.25f), "turned round, the step is at the other end");
        check(!solidSouth(0.5f, 0.75f, 0.75f), "and the open corner with it");

        // A slab is the bottom half and nothing above it; a whole block
        // is all of it. The same function answers for all three.
        check(blockOverlaps(Blocks::CobblestoneSlab, 0, 0, 0,
                            0.4f, 0.2f, 0.4f, 0.6f, 0.3f, 0.6f), "a slab fills its bottom half");
        check(!blockOverlaps(Blocks::CobblestoneSlab, 0, 0, 0,
                             0.4f, 0.7f, 0.4f, 0.6f, 0.9f, 0.6f), "and nothing above it");
        check(blockOverlaps(Blocks::Cobblestone, 0, 0, 0,
                            0.4f, 0.7f, 0.4f, 0.6f, 0.9f, 0.6f), "a whole block fills all of it");
        check(!blockOverlaps(Blocks::Air, 0, 0, 0,
                             0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f), "and air fills none of it");

        // The cell it is in is where it is: a block at the origin does
        // not reach into the one next door.
        check(!blockOverlaps(Blocks::Cobblestone, 0, 0, 0,
                             1.2f, 0.2f, 0.2f, 1.8f, 0.8f, 0.8f),
              "a block does not reach into the next cell");
        check(blockOverlaps(Blocks::Cobblestone, 1, 0, 0,
                            1.2f, 0.2f, 0.2f, 1.8f, 0.8f, 0.8f),
              "and the one that is there does");

        // The step is low enough to walk up, which is the whole point of
        // a stair and the reason the step-up height is what it is.
        check(0.5f < 0.55f, "a stair's rise is inside the step-up height");
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
        wrote.experience = 1211;
        wrote.experienceOwedHundredths = 45;
        wrote.air = 6.25f;
        wrote.effectAmplifiers.assign(Effects::KIND_COUNT, 0);
        wrote.effectSeconds.assign(Effects::KIND_COUNT, 0.0f);
        wrote.effectAmplifiers[static_cast<int>(Effects::Kind::Speed)] = 1;
        wrote.effectSeconds[static_cast<int>(Effects::Kind::Speed)] = 42.5f;
        wrote.effectAmplifiers[static_cast<int>(Effects::Kind::Poison)] = 0;
        wrote.effectSeconds[static_cast<int>(Effects::Kind::Poison)] = 7.0f;
        wrote.inventory.emplace_back(static_cast<uint16_t>(Blocks::Stone), 42);
        wrote.inventory.emplace_back(static_cast<uint16_t>(Items::DiamondPickaxe), 1);
        wrote.damage.push_back(0);
        wrote.damage.push_back(137);
        wrote.enchantments.push_back(0u);
        wrote.enchantments.push_back(
            Enchanting::withLevel(Enchanting::withLevel(0u, Enchanting::Kind::Efficiency, 5),
                                  Enchanting::Kind::Unbreaking, 3));

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

        check(read.effectAmplifiers.size() == static_cast<size_t>(Effects::KIND_COUNT),
              "effects come back, one slot a kind");
        check(read.effectSeconds[static_cast<int>(Effects::Kind::Speed)] == 42.5f,
              "with the time each had left");
        check(read.effectAmplifiers[static_cast<int>(Effects::Kind::Speed)] == 1,
              "and how strong it was");
        check(read.effectSeconds[static_cast<int>(Effects::Kind::Poison)] == 7.0f,
              "for every kind that was running");
        check(read.effectSeconds[static_cast<int>(Effects::Kind::Strength)] == 0.0f,
              "and nothing for the ones that were not");

        // Everything appended since the format was first written. Each
        // of these went in at the end precisely so an older file still
        // loads, and each is read back in the order it was written --
        // one of them out of place and every field after it is garbage.
        check(read.experience == wrote.experience, "and the experience earned");
        check(read.experienceOwedHundredths == wrote.experienceOwedHundredths,
              "and what the furnace still owed");
        check(read.air == wrote.air, "and how much air was left");
        check(read.damage.size() == wrote.damage.size(), "the wear on every slot");
        check(read.damage.size() > 1 && read.damage[1] == 137, "with the right amount on it");
        check(read.enchantments.size() == wrote.enchantments.size(),
              "and the enchantments on every slot");
        if (read.enchantments.size() > 1)
        {
            check(Enchanting::levelOf(read.enchantments[1], Enchanting::Kind::Efficiency) == 5,
                  "efficiency five survives the round trip");
            check(Enchanting::levelOf(read.enchantments[1], Enchanting::Kind::Unbreaking) == 3,
                  "and unbreaking three alongside it");
        }
        check(read.health == 14 && read.hunger == 11, "and how you were doing");
        check(read.cheats, "and whether the commands work");

        // Dying used to put you back at the generator's guess rather than
        // where the world actually began, because this was written and
        // never read.
        check(read.hasSpawn, "the spawn point comes back");
        check(read.spawn == wrote.spawn, "and is the one that was saved");

        check(read.inventory.size() == wrote.inventory.size(), "the inventory comes back");
        check(read.inventory.size() > 1 &&
                  read.inventory[1].first == static_cast<uint16_t>(Items::DiamondPickaxe),
              "with the pickaxe in the slot it was saved in");

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

    // --- experience ----------------------------------------------------

    void testExperience()
    {
        section("experience: the level curve");

        check(Experience::toNextLevel(0) == 7, "level 0 costs 7");
        check(Experience::toNextLevel(15) == 37, "level 15 costs 37");
        check(Experience::toNextLevel(16) == 42, "level 16 costs 42, the second bracket");
        check(Experience::toNextLevel(30) == 112, "level 30 costs 112");
        check(Experience::toNextLevel(31) == 121, "level 31 costs 121, the third bracket");

        // Totals Minecraft is known to use, which is what pins the
        // constants: a bracket written with the wrong offset still
        // climbs smoothly and is only caught by a number like these.
        check(Experience::totalForLevel(16) == 352, "level 16 at 352");
        check(Experience::totalForLevel(17) == 394, "level 17 at 394");
        check(Experience::totalForLevel(30) == 1395, "level 30 at 1395");
        check(Experience::totalForLevel(31) == 1507, "level 31 at 1507");
        check(Experience::totalForLevel(32) == 1628, "level 32 at 1628");

        // The closed form has to agree with adding the costs up one at a
        // time, across both joins. This is the check that catches a
        // bracket which is internally consistent but bolted on at the
        // wrong height.
        int running = 0;
        bool matched = true;
        for (int level = 0; level < 200; ++level)
        {
            if (Experience::totalForLevel(level) != running) matched = false;
            running += Experience::toNextLevel(level);
        }
        check(matched, "closed form matches the running total for 200 levels");

        bool climbing = true;
        for (int level = 1; level < 200; ++level)
            if (Experience::toNextLevel(level) < Experience::toNextLevel(level - 1))
                climbing = false;
        check(climbing, "each level costs at least as much as the last");

        section("experience: totals back to levels");

        bool roundTrip = true;
        bool justUnder = true;
        for (int level = 0; level < 200; ++level)
        {
            if (Experience::levelFor(Experience::totalForLevel(level)) != level)
                roundTrip = false;
            if (level > 0 &&
                Experience::levelFor(Experience::totalForLevel(level) - 1) != level - 1)
                justUnder = false;
        }
        check(roundTrip, "every total reads back as its own level");
        check(justUnder, "one point short of a level is the level below");

        check(Experience::levelFor(0) == 0, "nothing earned is level 0");
        check(Experience::levelFor(-5) == 0, "a negative total is level 0");

        check(Experience::progress(Experience::totalForLevel(5)) == 0.0f,
              "the bar is empty at a level boundary");
        bool inRange = true;
        for (int total = 0; total < 3000; ++total)
        {
            const float p = Experience::progress(total);
            if (p < 0.0f || p >= 1.0f) inRange = false;
        }
        check(inRange, "the bar stays between empty and full for 3000 points");

        section("experience: orbs");

        check(Experience::orbValue(1) == 1, "one point is one orb of one");
        check(Experience::orbValue(6) == 3, "six splits off a three");
        check(Experience::orbValue(2477) == 2477, "the largest orb");
        check(Experience::orbValue(0) == 0, "nothing makes no orb");

        bool exact = true;
        bool legal = true;
        bool bounded = true;
        for (int worth = 1; worth <= 4000; ++worth)
        {
            const std::vector<int> orbs = Experience::splitIntoOrbs(worth);
            int sum = 0;
            for (int one : orbs)
            {
                sum += one;
                bool known = false;
                for (int value : Experience::ORB_VALUES)
                    if (one == value) known = true;
                if (!known) legal = false;
            }
            if (sum != worth) exact = false;
            if (orbs.size() > 16) bounded = false;
        }
        check(exact, "orbs add back up to the award, for every award to 4000");
        check(legal, "every orb is one of the sizes orbs come in");
        check(bounded, "no award scatters into more than sixteen orbs");
        check(Experience::splitIntoOrbs(0).empty(), "nothing drops no orbs");
        check(Experience::splitIntoOrbs(-3).empty(), "a negative award drops no orbs");

        section("experience: what pays out");

        // Ore you can use pays at the seam; ore you must smelt pays at
        // the furnace. Paying at both would be paying twice.
        check(Experience::forMining(Blocks::CoalOre).any(), "coal pays when mined");
        check(Experience::forMining(Blocks::DiamondOre).any(), "diamond pays when mined");
        check(!Experience::forMining(Blocks::IronOre).any(), "iron does not pay when mined");
        check(!Experience::forMining(Blocks::GoldOre).any(), "gold does not pay when mined");
        check(!Experience::forMining(Blocks::Stone).any(), "stone pays nothing");
        check(Experience::smeltingHundredths(Items::IronIngot) > 0, "iron pays at the furnace");
        check(Experience::smeltingHundredths(Items::GoldIngot) > 0, "gold pays at the furnace");
        check(Experience::smeltingHundredths(Blocks::Dirt) == 0, "dirt pays nothing");

        bool ranges = true;
        for (int id = 0; id < 256; ++id)
        {
            const Experience::Range r = Experience::forMining(static_cast<StackId>(id));
            if (r.low > r.high || r.low < 0) ranges = false;
        }
        check(ranges, "every mining range is the right way round");

        section("experience: the player's running total");

        Player player(glm::vec3(0.0f));
        check(player.experience == 0, "a new player has none");
        check(!player.addExperience(0), "nothing awarded is not a level");
        check(player.experience == 0, "and adds nothing");

        check(player.addExperience(7), "seven points is the first level");
        check(Experience::levelFor(player.experience) == 1, "and reads as level 1");
        check(!player.addExperience(1), "one more is not another level");

        // The furnace pays fractions, which must not round away: ten
        // iron ingots at seven tenths of a point each is seven points.
        Player smelter(glm::vec3(0.0f));
        for (int i = 0; i < 10; ++i) smelter.addExperienceHundredths(70);
        check(smelter.experience == 7, "ten ingots at 0.7 pay exactly 7");
        check(smelter.experienceOwedHundredths == 0, "with nothing left owing");

        Player part(glm::vec3(0.0f));
        check(!part.addExperienceHundredths(35), "a third of a point is no point yet");
        check(part.experience == 0, "and is not paid early");
        part.addExperienceHundredths(35);
        part.addExperienceHundredths(35);
        check(part.experience == 1, "three of them cross a whole point");

        Player died(glm::vec3(0.0f));
        died.addExperience(500);
        died.addExperienceHundredths(50);
        died.respawn(glm::vec3(0.0f));
        check(died.experience == 0, "dying costs everything earned");
        check(died.experienceOwedHundredths == 0, "and everything owed");
    }

    // --- sugar cane ------------------------------------------------------

    // A patch of world held in a map, so the cane rules can be asked
    // about a waterside that does not exist.
    struct FakeGround : Cane::Blocks
    {
        std::map<std::array<int, 3>, BlockId> cells;

        void set(int x, int y, int z, BlockId id) { cells[{ x, y, z }] = id; }
        BlockId at(int x, int y, int z) const override
        {
            // ::Blocks, not Blocks: inside something deriving from
            // Cane::Blocks the bare name finds the base class.
            const auto it = cells.find({ x, y, z });
            return (it == cells.end()) ? ::Blocks::Air : it->second;
        }
    };

    struct FakeFluidWorld : Fluids::Cells
    {
        std::map<std::array<int, 3>, BlockId> cells;
        int floorY = 0;

        BlockId at(int x, int y, int z) const override
        {
            const auto it = cells.find({ x, y, z });
            if (it != cells.end()) return it->second;
            return y <= floorY ? ::Blocks::Stone : ::Blocks::Air;
        }
        void set(int x, int y, int z, BlockId id) override { cells[{ x, y, z }] = id; changed.push_back({ x, y, z }); }
        std::vector<std::array<int, 3>> changed;

        void settle(bool nether = false, int rounds = 400)
        {
            Fluids::Scheduler scheduler;
            long long now = 0;
            std::vector<std::array<int, 3>> wake = changed;
            changed.clear();
            for (const auto& c : wake) scheduler.wake(*this, c[0], c[1], c[2], now, nether);
            for (int i = 0; i < rounds; ++i)
            {
                ++now;
                scheduler.run(*this, now, nether, 4096);
                wake.swap(changed);
                changed.clear();
                for (const auto& c : wake) scheduler.wake(*this, c[0], c[1], c[2], now, nether);
                if (scheduler.pending() == 0) break;
            }
        }
    };

    void testFluids()
    {
        section("fluids: water spreads and settles");

        FakeFluidWorld flat;
        flat.set(0, 1, 0, Blocks::Water);
        flat.settle();
        check(flat.at(1, 1, 0) == Blocks::WaterFlow7, "water beside a source is one step down");
        check(flat.at(7, 1, 0) == Blocks::WaterFlow1, "and runs seven blocks on the flat");
        check(flat.at(8, 1, 0) == Blocks::Air, "and no further");
        check(flat.at(3, 1, 3) == Blocks::WaterFlow2 && flat.at(4, 1, 3) == Blocks::WaterFlow1 &&
              flat.at(4, 1, 4) == Blocks::Air, "it spreads round corners by steps, not as the crow flies");

        flat.set(0, 1, 0, Blocks::Air);
        flat.settle();
        bool dried = true;
        for (int x = -8; x <= 8; ++x)
            for (int z = -8; z <= 8; ++z)
                if (isFluid(flat.at(x, 1, z))) dried = false;
        check(dried, "and drains away when the source is taken");

        section("fluids: water runs downhill");
        FakeFluidWorld slope;
        slope.set(0, 1, 0, Blocks::Water);
        slope.set(3, 1, 0, Blocks::Air);
        slope.set(3, 0, 0, Blocks::Air);
        slope.settle();
        check(isWater(slope.at(3, 0, 0)), "it finds the hole three blocks away and pours in");
        check(slope.at(-1, 1, 0) == Blocks::Air, "rather than spreading the other way");
        check(isFluidFalling(slope.at(3, 0, 0)) || fluidAmount(slope.at(3, 0, 0)) > 0, "and falls into it");

        section("fluids: infinite water");
        FakeFluidWorld pool;
        pool.set(0, 1, 0, Blocks::Water);
        pool.set(2, 1, 0, Blocks::Water);
        for (int x = -1; x <= 3; ++x)
            for (int z = -1; z <= 1; ++z)
                if (!(z == 0 && x >= 0 && x <= 2)) pool.set(x, 1, z, Blocks::Stone);
        pool.settle();
        check(pool.at(1, 1, 0) == Blocks::Water, "two sources either side make a third");

        section("fluids: lava");
        FakeFluidWorld lava;
        lava.set(0, 1, 0, Blocks::Lava);
        lava.settle();
        check(lava.at(1, 1, 0) == Blocks::LavaFlow6, "lava in the overworld drops two a block");
        check(lava.at(3, 1, 0) == Blocks::LavaFlow2, "and runs three blocks");
        check(lava.at(4, 1, 0) == Blocks::Air, "and no further");

        FakeFluidWorld hot;
        hot.set(0, 1, 0, Blocks::Lava);
        hot.settle(true);
        check(hot.at(4, 1, 0) == Blocks::LavaFlow4, "but runs like water in the nether");

        section("fluids: lava meets water");
        FakeFluidWorld meet;
        meet.set(0, 1, 0, Blocks::Lava);
        meet.set(1, 1, 0, Blocks::Stone);
        meet.set(0, 1, 1, Blocks::Stone);
        meet.set(-1, 1, 0, Blocks::Stone);
        meet.set(0, 1, -1, Blocks::Stone);
        meet.set(3, 1, -1, Blocks::Water);
        meet.settle();
        meet.set(1, 1, 0, Blocks::WaterFlow3);
        meet.settle();
        check(meet.at(0, 1, 0) == Blocks::Obsidian, "water on a lava source makes obsidian");

        FakeFluidWorld cobble;
        cobble.set(0, 1, 0, Blocks::LavaFlow4);
        cobble.set(-1, 1, 0, Blocks::LavaFlow6);
        cobble.set(1, 1, 0, Blocks::WaterFlow5);
        Fluids::update(cobble, 0, 1, 0, false);
        check(cobble.at(0, 1, 0) == Blocks::Cobblestone, "water on running lava makes cobblestone");

        FakeFluidWorld fall;
        fall.set(0, 2, 0, Blocks::Lava);
        fall.set(0, 1, 0, Blocks::Water);
        Fluids::update(fall, 0, 2, 0, false);
        check(fall.at(0, 1, 0) == Blocks::Stone || fall.at(0, 2, 0) == Blocks::Obsidian,
              "lava meeting water below turns one of them to rock");

        section("the wither: its first eleven seconds");
        {
            Mob wither(MobId::Wither, glm::vec3(0.0f, 80.0f, 0.0f), 77u);
            check(wither.spawnPhase() > 10.0f, "a new wither starts in its spawn phase");
            check(wither.health() == mobType(MobId::Wither).maxHealth / 3, "at a third of its health");
            const int before = wither.health();
            wither.damage(50, glm::vec3(1.0f, 0.0f, 0.0f));
            check(wither.health() == before, "and nothing can hurt it until it wakes");
        }

        section("enchanting table: the book");
        {
            BookRenderer::Book book;
            uint32_t seed = 99u;
            for (int i = 0; i < 20; ++i) BookRenderer::step(book, true, 0.0f, 2.0f, seed);
            check(book.open > 0.99f, "the book opens when someone stands near");
            check(std::abs(book.rotation - 1.5708f) < 0.05f, "and turns to face them");
            for (int i = 0; i < 20; ++i) BookRenderer::step(book, false, 0.0f, 0.0f, seed);
            check(book.open < 0.01f, "and shuts again when they leave");
        }

        section("amethyst: buds grow in four stages");
        check(amethystAtStage(0) == Blocks::SmallAmethystBud && amethystAtStage(3) == Blocks::AmethystCluster,
              "small, medium, large, then a cluster");
        for (int stage = 0; stage < 4; ++stage)
            check(amethystStage(amethystAtStage(stage)) == stage, "each stage knows where it is");
        check(lightEmission(Blocks::SmallAmethystBud) < lightEmission(Blocks::LargeAmethystBud) &&
              lightEmission(Blocks::LargeAmethystBud) < lightEmission(Blocks::AmethystCluster),
              "and each glows brighter than the last");
        check(blockDrop(Blocks::MediumAmethystBud) == Blocks::Air, "a bud broken early gives nothing");

        section("villagers: job sites");
        {
            Trading::Profession job = Trading::Profession::Farmer;
            check(Trading::jobSite(Blocks::Smoker, job) && job == Trading::Profession::Butcher, "a smoker makes a butcher");
            check(Trading::jobSite(Blocks::BlastFurnace, job) && job == Trading::Profession::Armorer, "a blast furnace an armorer");
            check(Trading::jobSite(Blocks::BrewingStand, job) && job == Trading::Profession::Cleric, "a brewing stand a cleric");
            check(Trading::jobSite(Blocks::Barrel, job) && job == Trading::Profession::Fisherman, "a barrel a fisherman");
            check(Trading::jobSite(Blocks::Cauldron0, job) && job == Trading::Profession::Leatherworker, "a cauldron a leatherworker");
            check(Trading::jobSite(Blocks::Grindstone, job) && job == Trading::Profession::Weaponsmith, "a grindstone a weaponsmith");
            check(Trading::jobSite(Blocks::Composter0, job) && job == Trading::Profession::Farmer, "a composter a farmer");
            check(!Trading::jobSite(Blocks::Stone, job), "and stone is nobody's job");
        }

        section("fluids: what water washes away");
        check(Fluids::washesAway(Blocks::Torch) || !isCross(Blocks::Torch), "a torch is washed away if it stands like a plant");
        check(!Fluids::washesAway(Blocks::Stone), "stone is not");
        check(!Fluids::washesAway(Blocks::SugarCane), "sugar cane holds its ground, as in Minecraft");
        check(!isObtainable(Blocks::WaterFlow3) && !isObtainable(Blocks::LavaFalling), "running water is never an item");
    }

    void testSugarCane()
    {
        section("sugar cane: where it stands");

        // Sand at the water's edge: cane belongs on it.
        FakeGround shore;
        shore.set(0, 63, 0, Blocks::Sand);
        shore.set(1, 63, 0, Blocks::Water);

        check(Cane::groundIsWatered(shore, 0, 63, 0), "sand beside water is watered ground");
        check(Cane::canStandAt(shore, 0, 64, 0), "cane stands on it");

        // The same sand with no water anywhere near it will not do.
        FakeGround dry;
        dry.set(0, 63, 0, Blocks::Sand);
        check(!Cane::groundIsWatered(dry, 0, 63, 0), "dry sand is not watered ground");
        check(!Cane::canStandAt(dry, 0, 64, 0), "and cane will not stand on it");

        // Water has to be level with the ground, not with the cane:
        // a pool one block up does not count.
        FakeGround highWater;
        highWater.set(0, 63, 0, Blocks::Sand);
        highWater.set(1, 64, 0, Blocks::Water);
        check(!Cane::canStandAt(highWater, 0, 64, 0),
              "water level with the cane rather than the ground does not count");

        // All four sides work, not just the one that happened to be
        // written first.
        const int dx[4] = { -1, 1, 0, 0 };
        const int dz[4] = { 0, 0, -1, 1 };
        bool everySide = true;
        for (int i = 0; i < 4; ++i)
        {
            FakeGround side;
            side.set(0, 63, 0, Blocks::Sand);
            side.set(dx[i], 63, dz[i], Blocks::Water);
            if (!Cane::canStandAt(side, 0, 64, 0)) everySide = false;
        }
        check(everySide, "water on any of the four sides will do");

        // Dirt and grass work, stone does not.
        for (BlockId ground : { Blocks::Sand, Blocks::Dirt, Blocks::Grass })
        {
            FakeGround g;
            g.set(0, 63, 0, ground);
            g.set(1, 63, 0, Blocks::Water);
            check(Cane::canStandAt(g, 0, 64, 0),
                  std::string("cane stands on ") + blockInfo(ground).name);
        }
        FakeGround stone;
        stone.set(0, 63, 0, Blocks::Stone);
        stone.set(1, 63, 0, Blocks::Water);
        check(!Cane::canStandAt(stone, 0, 64, 0), "but not on stone, however wet");

        section("sugar cane: how tall");

        // One on the shore, then two more on top of it.
        FakeGround stack;
        stack.set(0, 63, 0, Blocks::Sand);
        stack.set(1, 63, 0, Blocks::Water);
        stack.set(0, 64, 0, Blocks::SugarCane);

        check(Cane::stackHeight(stack, 0, 64, 0) == 1, "one cane is one tall");
        check(Cane::canGrowAt(stack, 0, 64, 0), "and has room to grow");
        check(Cane::canStandAt(stack, 0, 65, 0), "a second may stand on it");

        stack.set(0, 65, 0, Blocks::SugarCane);
        check(Cane::stackHeight(stack, 0, 65, 0) == 2, "two cane is two tall");
        check(Cane::canGrowAt(stack, 0, 65, 0), "and still has room");
        check(!Cane::canGrowAt(stack, 0, 64, 0),
              "the one underneath does not grow: there is cane in the way");

        stack.set(0, 66, 0, Blocks::SugarCane);
        check(Cane::stackHeight(stack, 0, 66, 0) == 3, "three cane is three tall");
        check(!Cane::canGrowAt(stack, 0, 66, 0), "three is as tall as it goes");
        check(!Cane::canStandAt(stack, 0, 67, 0), "and a fourth may not be placed on it");

        check(Cane::stackHeight(stack, 0, 70, 0) == 0, "empty air is no cane at all");

        // Nothing here grows: it is not cane in the first place.
        FakeGround notCane;
        notCane.set(0, 63, 0, Blocks::Sand);
        notCane.set(1, 63, 0, Blocks::Water);
        notCane.set(0, 64, 0, Blocks::TallGrass);
        check(!Cane::canGrowAt(notCane, 0, 64, 0), "grass is not cane and does not grow into it");

        section("sugar cane: what it is for");

        // The chain has to work end to end, or cane is scenery: three
        // cane make paper, paper and a hide make a book, books and
        // planks make a shelf.
        ItemStack paper[9];
        for (int i : { 0, 1, 2 }) { paper[i].id = Blocks::SugarCane; paper[i].count = 1; }
        const CraftOutput sheets = Crafting::match(paper, 3);
        check(sheets.valid() && sheets.id == Items::Paper, "three cane in a row make paper");
        check(sheets.count == 3, "three of them, as in Minecraft");

        // Two cane is not enough.
        ItemStack two[9];
        for (int i : { 0, 1 }) { two[i].id = Blocks::SugarCane; two[i].count = 1; }
        check(!Crafting::match(two, 3).valid(), "two cane make nothing");

        ItemStack book[9];
        for (int i : { 0, 1, 2 }) { book[i].id = Items::Paper; book[i].count = 1; }
        book[3].id = Items::Leather; book[3].count = 1;
        const CraftOutput bound = Crafting::match(book, 3);
        check(bound.valid() && bound.id == Items::Book, "three paper and a hide make a book");

        // Paper alone does not: the hide is what binds it.
        ItemStack loose[9];
        for (int i : { 0, 1, 2 }) { loose[i].id = Items::Paper; loose[i].count = 1; }
        check(!Crafting::match(loose, 3).valid(), "three paper alone make nothing");

        ItemStack shelf[9];
        for (int i : { 0, 1, 2, 6, 7, 8 }) { shelf[i].id = Blocks::Planks; shelf[i].count = 1; }
        for (int i : { 3, 4, 5 }) { shelf[i].id = Items::Book; shelf[i].count = 1; }
        const CraftOutput made = Crafting::match(shelf, 3);
        check(made.valid() && made.id == Blocks::Bookshelf,
              "six planks round three books make a bookshelf");

        // None of the three fit a 2x2: every one of them is three wide,
        // so all of them want a bench.
        check(!Crafting::match(paper, 2).valid(), "paper needs a bench");
        check(!Crafting::match(shelf, 2).valid(), "a bookshelf needs a bench");
    }

    // --- enchanting ------------------------------------------------------

    struct FakeRoom : Enchanting::Surroundings
    {
        std::map<std::array<int, 3>, BlockId> cells;
        void set(int x, int y, int z, BlockId id) { cells[{ x, y, z }] = id; }
        BlockId at(int x, int y, int z) const override
        {
            const auto it = cells.find({ x, y, z });
            return (it == cells.end()) ? ::Blocks::Air : it->second;
        }
    };

    void testEnchanting()
    {
        using Enchanting::Kind;

        section("enchanting: packing a set into one word");

        // Every kind has to survive a round trip at every level it
        // allows, or an item saved with one comes back with another.
        bool roundTrip = true;
        bool capped = true;
        for (int k = 0; k < static_cast<int>(Kind::Count); ++k)
        {
            const Kind kind = static_cast<Kind>(k);
            for (int level = 0; level <= Enchanting::maxLevel(kind); ++level)
            {
                const Enchanting::Set set = Enchanting::withLevel(0u, kind, level);
                if (Enchanting::levelOf(set, kind) != level) roundTrip = false;
            }
            // Past its maximum it is held at the maximum, never wrapped
            // round to something small by the three bits it lives in.
            const Enchanting::Set over = Enchanting::withLevel(0u, kind, 99);
            if (Enchanting::levelOf(over, kind) != Enchanting::maxLevel(kind)) capped = false;
        }
        check(roundTrip, "every kind reads back at every level it allows");
        check(capped, "and is held at its maximum rather than wrapping");

        // The kinds must not tread on each other's bits: all of them at
        // once, and each still reads back its own level.
        Enchanting::Set everything = 0u;
        for (int k = 0; k < static_cast<int>(Kind::Count); ++k)
            everything = Enchanting::withLevel(everything, static_cast<Kind>(k),
                                               Enchanting::maxLevel(static_cast<Kind>(k)));
        bool independent = true;
        for (int k = 0; k < static_cast<int>(Kind::Count); ++k)
        {
            const Kind kind = static_cast<Kind>(k);
            if (Enchanting::levelOf(everything, kind) != Enchanting::maxLevel(kind))
                independent = false;
        }
        check(independent, "all of them at once, and none has trodden on another's bits");
        check(Enchanting::listOf(everything).size() == static_cast<size_t>(Kind::Count),
              "and all of them are listed");

        // Setting one back to nothing leaves the rest alone.
        const Enchanting::Set fewer = Enchanting::withLevel(everything, Kind::Fortune, 0);
        check(!Enchanting::has(fewer, Kind::Fortune), "one can be taken off");
        check(Enchanting::levelOf(fewer, Kind::Efficiency) ==
                  Enchanting::maxLevel(Kind::Efficiency),
              "without disturbing its neighbour");

        check(!Enchanting::any(0u), "nothing enchanted is an empty set");

        section("enchanting: the thirty that 1.21.1 added");
        {
            check(Enchanting::KIND_COUNT == 38, "thirty-eight kinds, the ones a trident keeps apart aside");
            Enchanting::Set wide = Enchanting::withLevel(0u, Kind::Mending, 1);
            wide = Enchanting::withLevel(wide, Kind::Unbreaking, 3);
            wide = Enchanting::withLevel(wide, Kind::Smite, 5) | 0x5A000000u;
            check(Enchanting::levelOf(wide, Kind::Mending) == 1 && Enchanting::levelOf(wide, Kind::Unbreaking) == 3 &&
                  Enchanting::levelOf(wide, Kind::Smite) == 5, "a sword carries mending, unbreaking and smite at once");
            const Enchanting::Set same = Enchanting::withLevel(Enchanting::withLevel(Enchanting::withLevel(0u, Kind::Smite, 5), Kind::Unbreaking, 3), Kind::Mending, 1);
            check((wide & Enchanting::KIND_BITS) == (same & Enchanting::KIND_BITS), "the same set is the same word, so stacks still compare");
            const Enchanting::Set retrimmed = Enchanting::withLevel(wide, Kind::Smite, 4);
            check((retrimmed >> 24) == 0x5Au, "and what shares the top byte survives a change");
            check(Enchanting::listOf(wide).size() == 3, "all three are listed");
            const Enchanting::Set plain = Enchanting::withLevel(wide, Kind::Mending, 0);
            const Enchanting::Set back = Enchanting::withLevel(plain, Kind::Smite, 0);
            check(back == (Enchanting::withLevel(0u, Kind::Unbreaking, 3) | 0x5A000000u), "taking the new ones off packs it back into the old word");
            const std::string saved = Enchanting::librarySave();
            const size_t shelved = Enchanting::librarySize();
            Enchanting::libraryLoad(saved);
            check(Enchanting::librarySize() == shelved && Enchanting::levelOf(wide, Kind::Smite) == 5,
                  "the shelf of wide sets is saved with the world and read back the same");
            check(std::string(Enchanting::idOf(Kind::BindingCurse)) == "binding_curse" && std::string(Enchanting::name(Kind::VanishingCurse)) == "Curse of Vanishing",
                  "the curses have their 1.21.1 ids and names");
            check(Enchanting::conflicts(Kind::Protection, Kind::BlastProtection) && Enchanting::conflicts(Kind::Sharpness, Kind::Smite) &&
                  Enchanting::conflicts(Kind::Infinity, Kind::Mending) && Enchanting::conflicts(Kind::Multishot, Kind::Piercing) &&
                  Enchanting::conflicts(Kind::DepthStrider, Kind::FrostWalker) && !Enchanting::conflicts(Kind::Smite, Kind::Looting),
                  "the 1.21.1 exclusive sets");
            check(Enchanting::appliesTo(Kind::Looting, Items::DiamondSword) && !Enchanting::appliesTo(Kind::Looting, Items::DiamondAxe) &&
                  Enchanting::appliesTo(Kind::Respiration, Items::IronHelmet) && !Enchanting::appliesTo(Kind::Respiration, Items::IronBoots) &&
                  Enchanting::appliesTo(Kind::DepthStrider, Items::IronBoots) && Enchanting::appliesTo(Kind::SwiftSneak, Items::IronLeggings) &&
                  Enchanting::appliesTo(Kind::Multishot, Items::Crossbow) && Enchanting::appliesTo(Kind::Lure, Items::FishingRod) &&
                  Enchanting::appliesTo(Kind::Density, Items::Mace) && Enchanting::appliesTo(Kind::Infinity, Items::Bow) &&
                  Enchanting::appliesTo(Kind::Mending, Items::Elytra) && Enchanting::appliesTo(Kind::BindingCurse, Items::Elytra),
                  "each goes where 1.21.1 puts it");
            check(Enchanting::isTreasure(Kind::Mending) && Enchanting::isTreasure(Kind::SoulSpeed) && !Enchanting::isTreasure(Kind::Looting),
                  "mending, soul speed and the curses never come off the table");
            bool tableClean = true;
            for (uint32_t roll = 1; roll < 200; ++roll)
                for (int row = 0; row < 3; ++row)
                {
                    const Enchanting::Offer offer = Enchanting::offerFor(row, 15, Items::DiamondSword, roll * 7919u);
                    if (offer.valid() && (Enchanting::isTreasure(offer.kind) || !Enchanting::appliesTo(offer.kind, Items::DiamondSword))) tableClean = false;
                }
            check(tableClean, "the table offers the new sword ones but never a treasure");

            const Enchanting::Set blast = Enchanting::withLevel(0u, Kind::BlastProtection, 4);
            check(Enchanting::protectionFactor(blast, Enchanting::Hurt::Blast) == 8 && Enchanting::protectionFactor(blast, Enchanting::Hurt::Plain) == 0,
                  "blast protection only counts against explosions");
            check(Enchanting::protectionFactor(Enchanting::withLevel(0u, Kind::FeatherFalling, 4), Enchanting::Hurt::Fall) == 12, "feather falling is three a level");
            check(Enchanting::reducedByFactor(20, 10) == 2 && Enchanting::reducedByFactor(40, 10) == 2, "four percent a point, capped at eighty");
            check(Enchanting::extraDamageAgainst(Enchanting::withLevel(0u, Kind::Smite, 2), true, false) == 5.0f &&
                  Enchanting::extraDamageAgainst(Enchanting::withLevel(0u, Kind::Smite, 2), false, true) == 0.0f, "smite is for the undead");

            Player suited(glm::vec3(0.0f));
            suited.armourEnchants[3] = Enchanting::withLevel(0u, Kind::FeatherFalling, 4);
            suited.cause = "fall";
            suited.damage(10);
            check(suited.health == Player::MAX_HEALTH - 5, "feather falling IV halves a fall");
            Player other(glm::vec3(0.0f));
            other.armourEnchants[3] = Enchanting::withLevel(0u, Kind::FeatherFalling, 4);
            other.cause = "generic";
            other.damage(10);
            check(other.health == Player::MAX_HEALTH - 10, "and does nothing for a punch");

            Player posed(glm::vec3(0.0f, 10.0f, 0.0f));
            check(std::abs(posed.aabb().max.y - posed.aabb().min.y - Player::HEIGHT) < 0.001f && std::abs(posed.eyePosition().y - 10.0f - Player::EYE_HEIGHT) < 0.001f, "standing is 1.8 tall");
            posed.pose = Player::Pose::Sneaking;
            check(std::abs(posed.aabb().max.y - posed.aabb().min.y - 1.5f) < 0.001f && std::abs(posed.eyePosition().y - 11.27f) < 0.001f,
                  "sneaking shrinks the box to 1.5 with the eyes at 1.27");
            posed.pose = Player::Pose::Swimming;
            check(std::abs(posed.aabb().max.y - posed.aabb().min.y - 0.6f) < 0.001f && std::abs(posed.eyePosition().y - 10.4f) < 0.001f,
                  "swimming and crawling lie down to 0.6");
            Anvil::Input cursed{ Items::DiamondSword, 1, 40, Enchanting::withLevel(Enchanting::withLevel(0u, Kind::Sharpness, 3), Kind::VanishingCurse, 1) };
            const Grindstone::Result ground = Grindstone::combine(cursed, Anvil::Input{ Blocks::Air, 0, 0, 0u });
            check(ground.valid && !Enchanting::has(ground.enchantments, Kind::Sharpness) && Enchanting::has(ground.enchantments, Kind::VanishingCurse),
                  "a grindstone takes everything off but a curse");

            Inventory worn;
            worn.slot(Inventory::ARMOR_FIRST) = ItemStack{ Items::IronHelmet, 1, 0, Enchanting::withLevel(0u, Kind::BindingCurse, 1) };
            worn.leftClick(Inventory::ARMOR_FIRST);
            check(worn.cursor().empty() && !worn.slot(Inventory::ARMOR_FIRST).empty(), "curse of binding keeps a helmet on");
            worn.bindingHolds = false;
            worn.leftClick(Inventory::ARMOR_FIRST);
            check(!worn.cursor().empty(), "except in creative");
            check(Chat::parse("/enchant @s binding_curse", true).kind == Chat::Kind::Enchant, "/enchant knows the curses by id");
            Mob archer(MobId::Skeleton, glm::vec3(0.0f), 21u);
            archer.dress(2);
            Mob brute(MobId::Vindicator, glm::vec3(0.0f), 22u);
            brute.dress(2);
            Mob trader(MobId::Piglin, glm::vec3(0.0f), 23u);
            trader.dress(2);
            check(archer.gear(Mob::MainHand) == Items::Bow && brute.gear(Mob::MainHand) == Items::IronAxe &&
                  (trader.gear(Mob::MainHand) == Items::Crossbow || trader.gear(Mob::MainHand) == Items::GoldSword) && trader.canPickUp(),
                  "mobs are dressed the way 1.21.1 dresses them");
            Mob geared(MobId::Zombie, glm::vec3(0.0f), 24u);
            geared.setGear(Mob::Head, Items::IronHelmet);
            geared.setGear(Mob::Chest, Items::IronChestplate);
            check(geared.wornArmour() == 8 && geared.dressed(), "worn armour adds up");
            const int healthy = geared.health();
            geared.damage(10, glm::vec3(0.0f));
            check(healthy - geared.health() == 6, "and takes the edge off a blow");
            bool anyArmoured = false, bare = false;
            for (uint32_t seed = 1; seed < 4000; ++seed)
            {
                Mob walker(MobId::Zombie, glm::vec3(0.0f), seed);
                walker.dress(3);
                if (walker.gear(Mob::Head) != 0) anyArmoured = true;
                else bare = true;
            }
            check(anyArmoured && bare, "some zombies spawn in armour on hard and most do not");
            check(Mob::acceptsFood(MobId::Pig, Items::Carrot, Items::Wheat) && Mob::acceptsFood(MobId::Pig, Items::Beetroot, Items::Wheat) &&
                  !Mob::acceptsFood(MobId::Pig, Items::Wheat, Items::Wheat) && Mob::acceptsFood(MobId::Chicken, Items::MelonSeeds, Items::Wheat) &&
                  Mob::acceptsFood(MobId::Horse, Items::GoldenCarrot, Items::Wheat) && !Mob::acceptsFood(MobId::Horse, Items::Wheat, Items::Wheat) &&
                  !Mob::acceptsFood(MobId::Parrot, Items::WheatSeeds, Items::WheatSeeds) && Mob::acceptsFood(MobId::Cow, Items::Wheat, Items::Wheat),
                  "each animal breeds on its 1.21.1 foods");
            Mob big(MobId::Slime, glm::vec3(0.0f), 31u);
            big.setSize(4);
            Mob medium(MobId::Slime, glm::vec3(0.0f), 32u);
            medium.setSize(2);
            check(big.health() == 16 && medium.health() == 4 && big.width() > medium.width() * 1.9f, "slimes come large and medium");
            Mob wildWolf(MobId::Wolf, glm::vec3(0.0f), 33u);
            check(!wildWolf.feed(Items::RawBeef), "a wild wolf will not breed");
            Mob armoured(MobId::Zombie, glm::vec3(0.0f), 9u);
            const int before = armoured.health();
            armoured.damage(10, glm::vec3(0.0f));
            Mob breached(MobId::Zombie, glm::vec3(0.0f), 9u);
            breached.pierceArmour(4);
            breached.damage(10, glm::vec3(0.0f));
            check(before - armoured.health() == 9 && before - breached.health() == 10, "a zombie's two points of armour, and breach cutting through them");
        }

        section("enchanting: what goes on what");

        check(Enchanting::appliesTo(Kind::Efficiency, Items::DiamondPickaxe),
              "efficiency goes on a pickaxe");
        check(!Enchanting::appliesTo(Kind::Efficiency, Items::DiamondSword),
              "but not on a sword");
        check(Enchanting::appliesTo(Kind::Sharpness, Items::DiamondSword),
              "sharpness goes on a sword");
        check(!Enchanting::appliesTo(Kind::Sharpness, Items::DiamondHelmet),
              "but not on a helmet");
        check(Enchanting::appliesTo(Kind::Protection, Items::DiamondHelmet),
              "protection goes on armour");
        check(Enchanting::appliesTo(Kind::Power, Items::Bow), "power goes on a bow");
        check(!Enchanting::appliesTo(Kind::Power, Items::DiamondSword),
              "and nowhere else");
        check(Enchanting::appliesTo(Kind::Unbreaking, Items::DiamondSword) &&
                  Enchanting::appliesTo(Kind::Unbreaking, Items::DiamondHelmet) &&
                  Enchanting::appliesTo(Kind::Unbreaking, Items::Bow),
              "unbreaking goes on anything that wears out");
        check(!Enchanting::appliesTo(Kind::Unbreaking, Blocks::Stone),
              "but not on a block");

        check(Enchanting::conflicts(Kind::Fortune, Kind::SilkTouch),
              "fortune and silk touch cannot share an item");
        check(Enchanting::conflicts(Kind::SilkTouch, Kind::Fortune),
              "whichever way round they are asked");
        check(!Enchanting::conflicts(Kind::Efficiency, Kind::Unbreaking),
              "efficiency and unbreaking sit together happily");
        check(!Enchanting::conflicts(Kind::Fortune, Kind::Fortune),
              "and nothing conflicts with itself");

        section("enchanting: how enchantable the metal is");

        // Minecraft's own ranking, which is not the one anyone expects:
        // gold takes the best by a distance and stone is the worst.
        check(Enchanting::enchantability(Items::GoldPickaxe) >
                  Enchanting::enchantability(Items::DiamondPickaxe),
              "gold takes better enchantments than diamond");
        check(Enchanting::enchantability(Items::StonePickaxe) <
                  Enchanting::enchantability(Items::WoodPickaxe),
              "and stone is worse than wood");
        check(Enchanting::enchantability(Items::GoldHelmet) >
                  Enchanting::enchantability(Items::IronHelmet),
              "gold armour likewise");
        check(Enchanting::enchantability(Blocks::Stone) == 0,
              "a block takes none at all");

        section("enchanting: counting the shelves");

        // A table alone in a field.
        FakeRoom bare;
        check(Enchanting::shelvesAround(bare, 0, 64, 0) == 0, "a bare table has no shelves");

        // One shelf two cells away, with the gap clear.
        FakeRoom one;
        one.set(2, 64, 0, ::Blocks::Bookshelf);
        check(Enchanting::shelvesAround(one, 0, 64, 0) == 1, "one shelf two cells out counts");

        // The same shelf with something standing in the gap.
        FakeRoom blocked;
        blocked.set(2, 64, 0, ::Blocks::Bookshelf);
        blocked.set(1, 64, 0, ::Blocks::Stone);
        check(Enchanting::shelvesAround(blocked, 0, 64, 0) == 0,
              "a shelf behind a wall counts for nothing");

        // Pushed right up against the table it is not on the ring at
        // all, which is the rule people trip over.
        FakeRoom touching;
        touching.set(1, 64, 0, ::Blocks::Bookshelf);
        check(Enchanting::shelvesAround(touching, 0, 64, 0) == 0,
              "a shelf against the table is too close to count");

        // A full wall of them, capped at fifteen.
        FakeRoom library;
        for (int dy = 0; dy <= 1; ++dy)
            for (int dx = -2; dx <= 2; ++dx)
                for (int dz = -2; dz <= 2; ++dz)
                    if (std::max(std::abs(dx), std::abs(dz)) == 2)
                        library.set(dx, 64 + dy, dz, ::Blocks::Bookshelf);
        check(Enchanting::shelvesAround(library, 0, 64, 0) == Enchanting::MAX_SHELVES,
              "a full library counts fifteen and no more");

        section("enchanting: what the table offers");

        // More shelves must never make the offers worse. Checked across
        // many rolls, because one roll proves nothing about a formula
        // with a random term in it.
        bool climbs = true;
        for (uint32_t roll = 0; roll < 200; ++roll)
        {
            const int poor = Enchanting::costForRow(2, 0, roll);
            const int rich = Enchanting::costForRow(2, Enchanting::MAX_SHELVES, roll);
            if (rich < poor) climbs = false;
        }
        check(climbs, "a full library never offers less than a bare table");

        // The three rows run cheapest at the top to dearest at the
        // bottom, whatever the roll.
        bool ordered = true;
        bool positive = true;
        for (uint32_t roll = 0; roll < 200; ++roll)
        {
            const int a = Enchanting::costForRow(0, 8, roll);
            const int b = Enchanting::costForRow(1, 8, roll);
            const int c = Enchanting::costForRow(2, 8, roll);
            if (!(a <= b && b <= c)) ordered = false;
            if (a <= 0) positive = false;
        }
        check(ordered, "the three rows run cheap to dear, top to bottom");
        check(positive, "and the cheapest still costs something");

        // Whatever it offers for a thing must be something that can go
        // on that thing. An offer of sharpness on a shovel would be
        // bought and then do nothing at all.
        bool sensible = true;
        bool levelled = true;
        const StackId SUBJECTS[] = { Items::DiamondPickaxe, Items::IronSword, Items::Bow,
                                     Items::GoldHelmet, Items::WoodShovel, Items::StoneAxe };
        for (StackId item : SUBJECTS)
        {
            for (uint32_t roll = 0; roll < 150; ++roll)
            {
                for (int row = 0; row < 3; ++row)
                {
                    const Enchanting::Offer offer =
                        Enchanting::offerFor(row, 10, item, roll);
                    if (!offer.valid()) continue;
                    if (!Enchanting::appliesTo(offer.kind, item)) sensible = false;
                    if (offer.level < 1 || offer.level > Enchanting::maxLevel(offer.kind))
                        levelled = false;
                }
            }
        }
        check(sensible, "every offer is something that can go on the item offered for");
        check(levelled, "and at a level that enchantment allows");

        // Nothing is offered for a thing that cannot be enchanted.
        bool nothingForBlocks = true;
        for (int row = 0; row < 3; ++row)
            for (uint32_t roll = 0; roll < 50; ++roll)
                if (Enchanting::offerFor(row, 10, Blocks::Stone, roll).valid())
                    nothingForBlocks = false;
        check(nothingForBlocks, "a block is offered nothing");

        section("enchanting: what they do");

        check(Enchanting::digSpeedMultiplier(0u) == 1.0f, "an unenchanted tool digs at its own pace");
        const Enchanting::Set eff1 = Enchanting::withLevel(0u, Kind::Efficiency, 1);
        const Enchanting::Set eff5 = Enchanting::withLevel(0u, Kind::Efficiency, 5);
        check(Enchanting::digSpeedMultiplier(eff1) > 1.0f, "efficiency digs faster");
        check(Enchanting::digSpeedMultiplier(eff5) >
                  Enchanting::digSpeedMultiplier(eff1),
              "and faster still at five");

        // Unbreaking skips wear rather than adding durability, so over
        // many uses a level-three tool should take roughly a quarter of
        // the hits. Counted rather than assumed.
        const Enchanting::Set unb3 = Enchanting::withLevel(0u, Kind::Unbreaking, 3);
        int plainWear = 0, enchantedWear = 0;
        for (uint32_t i = 0; i < 4000; ++i)
        {
            if (Enchanting::wearsThisTime(0u, i)) ++plainWear;
            if (Enchanting::wearsThisTime(unb3, i)) ++enchantedWear;
        }
        check(plainWear == 4000, "an unenchanted tool wears every single time");
        check(enchantedWear > 700 && enchantedWear < 1300,
              "unbreaking three wears about a quarter as often, " +
                  std::to_string(enchantedWear) + " in 4000");

        // Fortune never gives less than one, or mining would sometimes
        // destroy the ore outright.
        const Enchanting::Set fort3 = Enchanting::withLevel(0u, Kind::Fortune, 3);
        bool neverNothing = true;
        int total = 0;
        for (uint32_t i = 0; i < 2000; ++i)
        {
            const int got = Enchanting::fortuneDrops(fort3, i);
            if (got < 1) neverNothing = false;
            total += got;
        }
        check(neverNothing, "fortune never turns a seam into nothing");
        check(total > 2000, "and gives more than mining it bare would");
        check(Enchanting::fortuneDrops(0u, 7u) == 1, "without it a seam gives one");

        check(Enchanting::sharpnessBonus(0u) == 0, "a plain sword gets no bonus");
        check(Enchanting::sharpnessBonus(Enchanting::withLevel(0u, Kind::Sharpness, 5)) >
                  Enchanting::sharpnessBonus(Enchanting::withLevel(0u, Kind::Sharpness, 1)),
              "sharpness five hits harder than sharpness one");

        // Protection takes a slice off, but never all of it: a blow
        // always lands for at least one.
        const Enchanting::Set prot4 = Enchanting::withLevel(0u, Kind::Protection, 4);
        check(Enchanting::protectionReduced(0u, 10) == 10, "without it a blow lands whole");
        check(Enchanting::protectionReduced(prot4, 10) < 10, "with it, less gets through");
        bool alwaysHurts = true;
        for (int blow = 1; blow <= 40; ++blow)
            if (Enchanting::protectionReduced(prot4, blow) < 1) alwaysHurts = false;
        check(alwaysHurts, "and no amount of it makes you proof");

        section("enchanting: an enchanted item in a slot");

        // An enchanted tool handed back must keep what was put on it.
        Inventory pack;
        ItemStack sword;
        sword.id = Items::DiamondSword;
        sword.count = 1;
        sword.damage = 12;
        sword.enchantments = Enchanting::withLevel(0u, Kind::Sharpness, 4);

        check(pack.addStack(sword) == 0, "an enchanted sword fits in an empty pack");
        check(pack.slot(0).id == Items::DiamondSword, "and is there");
        check(pack.slot(0).damage == 12, "with its wear");
        check(Enchanting::levelOf(pack.slot(0).enchantments, Kind::Sharpness) == 4,
              "and its enchantment");

        // Clearing a slot clears the enchantments with it, or the next
        // thing in that slot inherits them.
        ItemStack used = pack.slot(0);
        used.clear();
        check(used.enchantments == 0u, "clearing a slot takes the enchantments with it");
    }

    // --- villagers and trading -------------------------------------------

    void testTrading()
    {
        using Trading::Profession;

        section("trading: professions");

        // A villager's trade comes from the seed it was born with, so
        // the same villager is always the same trade.
        bool stable = true;
        for (uint32_t seed = 1; seed < 500; ++seed)
            if (Trading::professionFor(seed) != Trading::professionFor(seed))
                stable = false;
        check(stable, "a seed always gives the same profession");

        // All four have to come up. Consecutive seeds taken straight
        // would hand them out in a repeating cycle, so this is really a
        // check that the seed is stirred.
        int seen[static_cast<int>(Profession::Count)] = { 0 };
        for (uint32_t seed = 1; seed < 2000; ++seed)
            ++seen[static_cast<int>(Trading::professionFor(seed))];

        bool allAppear = true;
        bool evenish = true;
        for (int i = 0; i < static_cast<int>(Profession::Count); ++i)
        {
            if (seen[i] == 0) allAppear = false;
            const int fair = 1999 / static_cast<int>(Profession::Count);
            if (seen[i] < fair / 2 || seen[i] > fair * 2) evenish = false;
        }
        check(allAppear, "every profession turns up");
        check(evenish, "and they come up at roughly the same rate");

        bool named = true;
        for (int i = 0; i < static_cast<int>(Profession::Count); ++i)
        {
            const char* n = Trading::name(static_cast<Profession>(i));
            if (!n || !*n) named = false;
        }
        check(named, "every profession has a name to show");

        section("trading: the offers");

        // Every offer of every profession has to be a deal that can
        // actually be struck: real items on both sides, and counts that
        // are not zero or absurd.
        bool sound = true;
        bool sane = true;
        bool stacks = true;
        int totalOffers = 0;
        for (int i = 0; i < static_cast<int>(Profession::Count); ++i)
        {
            if (static_cast<Profession>(i) == Profession::Nitwit) continue;
            const std::vector<Trading::Offer> offers =
                Trading::offersFor(static_cast<Profession>(i));
            if (offers.empty()) sound = false;
            totalOffers += static_cast<int>(offers.size());

            for (const Trading::Offer& offer : offers)
            {
                if (!offer.valid()) sound = false;

                // Nothing may ask for more than a stack of something,
                // since there would be no way to carry the price.
                if (offer.wantCount > maxStackOf(offer.want)) stacks = false;
                if (offer.also != Blocks::Air && offer.alsoCount > maxStackOf(offer.also))
                    stacks = false;
                if (offer.giveCount > maxStackOf(offer.give)) stacks = false;

                // A trade that gives back what it took is not a trade.
                if (offer.want == offer.give) sane = false;
            }
        }
        check(sound, "every offer is a deal that can be struck");
        check(sane, "and none of them swaps a thing for itself");
        check(stacks, "and no price is more than a stack");
        check(totalOffers >= 12, "there are " + std::to_string(totalOffers) + " offers in all");

        // Emeralds are the currency: every profession must have some
        // way of earning them, or the half of its list that spends them
        // is unreachable.
        bool earns = true;
        check(Trading::offersFor(Profession::Nitwit).empty(), "a nitwit has nothing to trade");
        for (int i = 0; i < static_cast<int>(Profession::Count); ++i)
        {
            if (static_cast<Profession>(i) == Profession::Nitwit) continue;
            bool sells = false;
            for (const Trading::Offer& offer :
                 Trading::offersFor(static_cast<Profession>(i)))
                if (offer.give == Items::Emerald) sells = true;
            if (!sells) earns = false;
        }
        check(earns, "every profession buys something for emeralds");

        bool spends = true;
        for (int i = 0; i < static_cast<int>(Profession::Count); ++i)
        {
            if (static_cast<Profession>(i) == Profession::Nitwit) continue;
            bool buys = false;
            for (const Trading::Offer& offer :
                 Trading::offersFor(static_cast<Profession>(i)))
                if (offer.want == Items::Emerald) buys = true;
            if (!buys) spends = false;
        }
        check(spends, "and every profession sells something for them");

        section("trading: ranks and stock");

        bool climbs = true;
        for (int r = 1; r < Trading::RANKS; ++r)
            if (Trading::xpForRank(r) <= Trading::xpForRank(r - 1)) climbs = false;
        check(climbs, "each rank needs more experience than the last");
        check(Trading::rankFor(0) == 0, "a new villager is a " + std::string(Trading::rankName(0)));
        check(Trading::rankFor(Trading::xpForRank(1)) == 1, "reaching the threshold ranks it up");
        check(Trading::rankFor(Trading::xpForRank(1) - 1) == 0, "and one short does not");
        check(Trading::rankFor(100000) == Trading::RANKS - 1, "and it tops out at " +
              std::string(Trading::rankName(Trading::RANKS - 1)));

        bool everyRank = true;
        bool stocked = true;
        bool earnsXp = true;
        for (int i = 0; i < static_cast<int>(Profession::Count); ++i)
        {
            if (static_cast<Profession>(i) == Profession::Nitwit) continue;
            const std::vector<Trading::Deal> deals = Trading::dealsFor(static_cast<Profession>(i));
            for (int r = 0; r < Trading::RANKS; ++r)
            {
                bool any = false;
                for (const Trading::Deal& d : deals) if (d.rank == r) any = true;
                if (!any) everyRank = false;
            }
            for (const Trading::Deal& d : deals)
            {
                if (d.maxUses <= 0) stocked = false;
                if (d.villagerXp <= 0 || d.playerXp <= 0) earnsXp = false;
            }
        }
        check(everyRank, "every profession has something new at every rank");
        check(stocked, "every deal can be struck at least once before it sells out");
        check(earnsXp, "and every deal teaches the villager and the player something");

        section("trading: what you can afford");

        const Trading::Offer deal{ Items::Wheat, 18, Blocks::Air, 0, Items::Emerald, 1 };
        check(!Trading::affordable(deal, 17, 0), "seventeen wheat is not enough");
        check(Trading::affordable(deal, 18, 0), "eighteen is");
        check(Trading::affordable(deal, 64, 0), "and more than enough is too");

        // A deal with a second ingredient wants both.
        const Trading::Offer pair{ Items::Emerald, 18, Items::Diamond, 1,
                                   Items::DiamondPickaxe, 1 };
        check(!Trading::affordable(pair, 18, 0), "emeralds alone will not buy it");
        check(!Trading::affordable(pair, 17, 1), "nor will the diamond alone");
        check(Trading::affordable(pair, 18, 1), "both together will");

        const Trading::Offer nonsense;
        check(!Trading::affordable(nonsense, 64, 64),
              "an empty offer cannot be afforded at any price");

        section("trading: the villager itself");

        const MobType& villager = mobType(MobId::Villager);
        check(villager.spawnClass == SpawnClass::Passive, "a villager does not fight");
        check(villager.attackDamage == 0, "and deals no damage");
        check(villager.experience == 0,
              "and is worth nothing dead, which is the point of one");
        check(!villager.model.empty(), "it has a model");
        check(villager.maxHealth > 0, "and some health");

        // The seed a villager is born with is kept apart from the one
        // it draws headings from, which is the whole reason it exists:
        // the drawing one is stirred on every call, so a profession
        // read off it would change as the villager wandered about.
        // Building one picks a heading and an idle timer already, so if
        // the two were the same number this would catch it.
        Mob one(MobId::Villager, glm::vec3(0.0f, 64.0f, 0.0f), 12345u);
        Mob two(MobId::Villager, glm::vec3(8.0f, 64.0f, 8.0f), 12345u);
        check(one.birthSeed() == 12345u, "a villager keeps the seed it was born with");
        check(one.birthSeed() == two.birthSeed(),
              "two born alike agree on it");
        check(Trading::professionFor(one.birthSeed()) ==
                  Trading::professionFor(two.birthSeed()),
              "and so follow the same trade");

        // Different seeds must be able to give different trades, or
        // every villager in the world is a farmer.
        bool differ = false;
        for (uint32_t seed = 1; seed < 40 && !differ; ++seed)
            if (Trading::professionFor(seed) != Trading::professionFor(1u)) differ = true;
        check(differ, "and villagers born differently can follow different ones");
    }

    // --- the way to the End ----------------------------------------------

    struct FakeFrames : EndPortal::Blocks
    {
        std::map<std::array<int, 3>, BlockId> cells;
        void set(int x, int y, int z, BlockId id) { cells[{ x, y, z }] = id; }
        BlockId at(int x, int y, int z) const override
        {
            const auto it = cells.find({ x, y, z });
            return (it == cells.end()) ? ::Blocks::Air : it->second;
        }

        // Lays a complete ring of eyed frames round a centre.
        void layRing(int cx, int y, int cz)
        {
            int offsets[EndPortal::FRAME_COUNT][2];
            EndPortal::frameOffsets(offsets);
            for (const auto& o : offsets)
                set(cx + o[0], y, cz + o[1], ::Blocks::EndPortalFrameFilled);
        }
    };

    void testEndPortal()
    {
        section("the end portal: the shape of the ring");

        int offsets[EndPortal::FRAME_COUNT][2];
        EndPortal::frameOffsets(offsets);

        // Twelve distinct cells, none of them a corner and none inside.
        std::set<std::pair<int, int>> seen;
        bool noCorners = true;
        bool allOutside = true;
        for (const auto& o : offsets)
        {
            seen.insert({ o[0], o[1] });
            if (std::abs(o[0]) == 2 && std::abs(o[1]) == 2) noCorners = false;
            if (std::abs(o[0]) < 2 && std::abs(o[1]) < 2) allOutside = false;
        }
        check(seen.size() == EndPortal::FRAME_COUNT, "twelve frames, all different");
        check(noCorners, "and none of them a corner");
        check(allOutside, "and none of them inside the ring");

        section("the end portal: opening it");

        FakeFrames whole;
        whole.layRing(0, 64, 0);
        check(EndPortal::ringComplete(whole, 0, 64, 0), "a full ring is complete");

        // One frame short, in every one of the twelve places.
        bool everyGapCaught = true;
        for (const auto& o : offsets)
        {
            FakeFrames gapped;
            gapped.layRing(0, 64, 0);
            gapped.set(o[0], 64, o[1], ::Blocks::Air);
            if (EndPortal::ringComplete(gapped, 0, 64, 0)) everyGapCaught = false;
        }
        check(everyGapCaught, "a gap anywhere in the twelve leaves it shut");

        // A frame with no eye in it is not enough either.
        bool everyEmptyCaught = true;
        for (const auto& o : offsets)
        {
            FakeFrames unlit;
            unlit.layRing(0, 64, 0);
            unlit.set(o[0], 64, o[1], ::Blocks::EndPortalFrame);
            if (EndPortal::ringComplete(unlit, 0, 64, 0)) everyEmptyCaught = false;
        }
        check(everyEmptyCaught, "an eyeless frame anywhere leaves it shut");

        // The ring must be found from any of its own twelve frames,
        // which is what happens when the last eye goes in -- and it
        // could be the last eye in any of the twelve.
        bool foundFromEvery = true;
        for (const auto& o : offsets)
        {
            int cx = 999, cz = 999;
            if (!EndPortal::findRingAround(whole, o[0], 64, o[1], cx, cz) ||
                cx != 0 || cz != 0)
                foundFromEvery = false;
        }
        check(foundFromEvery, "and is found from whichever frame was filled last");

        // Nothing at all nearby finds nothing.
        FakeFrames bare;
        int cx = 0, cz = 0;
        check(!EndPortal::findRingAround(bare, 0, 64, 0, cx, cz),
              "an empty field opens no portal");

        // A ring one level down must not be lit by a frame up here.
        FakeFrames below;
        below.layRing(0, 63, 0);
        check(!EndPortal::ringComplete(below, 0, 64, 0),
              "a ring on another level is a different ring");

        // Two rings side by side: filling one must not open the other.
        FakeFrames twice;
        twice.layRing(0, 64, 0);
        twice.layRing(40, 64, 0);
        int ax = 999, az = 999;
        check(EndPortal::findRingAround(twice, 2, 64, 0, ax, az) && ax == 0 && az == 0,
              "with two rings, a frame finds the one it belongs to");

        section("the end portal: the way in");

        // The whole chain has to be craftable, or the End stays
        // somewhere you can only reach from the command line.
        ItemStack eye[9];
        eye[0].id = Items::EnderPearl;  eye[0].count = 1;
        eye[1].id = Items::BlazePowder; eye[1].count = 1;
        const CraftOutput made = Crafting::match(eye, 2);
        check(made.valid() && made.id == Items::EyeOfEnder,
              "a pearl and blaze powder make an eye of ender");

        // It took gunpowder while there was no blaze to get the powder
        // off. There is one now, so the stand-in is gone.
        ItemStack old[9];
        old[0].id = Items::EnderPearl; old[0].count = 1;
        old[1].id = Items::Gunpowder;  old[1].count = 1;
        const CraftOutput notAnEye = Crafting::match(old, 2);
        check(!notAnEye.valid() || notAnEye.id != Items::EyeOfEnder,
              "and gunpowder no longer stands in for it");

        ItemStack frame[9];
        for (int i = 0; i < 9; ++i) { frame[i].id = Blocks::EndStone; frame[i].count = 1; }
        frame[4].id = Items::EnderPearl;
        const CraftOutput frames = Crafting::match(frame, 3);
        check(frames.valid() && frames.id == Blocks::EndPortalFrame,
              "end stone round a pearl makes frames");
        check(frames.count == 4, "four at a time, so a ring takes three castings");

        // An enderman has to actually give you the pearl, or none of
        // the above can be started.
        check(mobType(MobId::Enderman).drop == Items::EnderPearl,
              "an enderman drops the pearl the chain begins with");
    }

    // --- nothing missing its picture or its noise -------------------------

    void testEveryBlockHasAPicture()
    {
        section("textures: every block has a picture");

        // A block whose tile nobody paints is not an error anywhere: the
        // atlas leaves it transparent and the block is simply invisible
        // in the world. So it has to be swept for.
        std::vector<std::string> unpainted;
        for (int id = 1; isBlockId(id); id = nextBlockId(id))   // Air draws nothing
        {
            const BlockInfo& info = blockInfo(static_cast<BlockId>(id));
            if (info.render == RenderType::None) continue;

            const int faces[3] = { info.tileTop, info.tileBottom, info.tileSide };
            const char* which[3] = { "top", "bottom", "side" };
            for (int f = 0; f < 3; ++f)
            {
                if (Atlas::hasPainter(faces[f])) continue;
                unpainted.push_back(std::string(info.name) + " (" + which[f] + ")");
            }
        }

        for (const std::string& missing : unpainted)
            check(false, missing + " has no picture");
        check(unpainted.empty(),
              "all " + std::to_string(Blocks::Count - 1) + " blocks are drawn");

        section("textures: every item has a picture");

        std::vector<std::string> itemless;
        for (int id = Items::FIRST; id < Items::Count; ++id)
        {
            const StackId item = static_cast<StackId>(id);
            if (!Atlas::hasPainter(atlasTileFor(item)))
                itemless.push_back(itemInfo(item).name);
        }
        for (const std::string& missing : itemless)
            check(false, missing + " has no picture");
        check(itemless.empty(),
              "all " + std::to_string(Items::Count - Items::FIRST) + " items are drawn");

        // Every item also needs a name to show, and a stack size that
        // means something.
        bool named = true;
        bool stacks = true;
        for (int id = Items::FIRST; id < Items::Count; ++id)
        {
            const ItemInfo& info = itemInfo(static_cast<StackId>(id));
            if (!info.name || !*info.name) named = false;
            if (info.maxStack < 1 || info.maxStack > 64) stacks = false;
        }
        check(named, "every item has a name");
        check(stacks, "and stacks to somewhere between one and sixty-four");

        // And every block, or the name shown over the hotbar is blank.
        bool blockNames = true;
        for (int id = 1; isBlockId(id); id = nextBlockId(id))
        {
            const BlockInfo& info = blockInfo(static_cast<BlockId>(id));
            if (!info.name || !*info.name) blockNames = false;
        }
        check(blockNames, "every block has a name");
    }


    void testShapeFamilies()
    {
        section("blocks: the second id range and the shape families");
        check(Blocks::ExtFirst > static_cast<int>(Items::Count), "the second block range starts above the items");
        check(Blocks::ExtCount <= 65535, "and still fits a sixteen bit id");
        check(isBlockId(Blocks::Stone) && isBlockId(Blocks::SmoothQuartz) && !isBlockId(Items::Stick) && !isBlockId(Blocks::Count),
              "both ranges are block ids and nothing between them is");
        check(isBlockStack(Blocks::SmoothQuartz) && !isItem(Blocks::SmoothQuartz), "a block from the second range is a block in a slot");
        check(std::string(blockInfo(Blocks::SmoothQuartz).name) == "Smooth Quartz Block", "and has its own row");
        check(nextBlockId(Blocks::Count - 1) == Blocks::ExtFirst, "walking the ids steps from one range to the next");

        bool stairsRound = true, stairsNamed = true;
        for (int m = 0; m < STAIRS_MATERIALS; ++m)
            for (int top = 0; top < 2; ++top)
                for (int f = 0; f < 4; ++f)
                {
                    const BlockId id = stairsOf(m, top != 0, f);
                    if (!isStairs(id) || stairsMaterial(id) != m || stairsUpsideDown(id) != (top != 0) || blockFacing(id) != f)
                        stairsRound = false;
                    if (stairsFacing(id, (f + 1) & 3) != stairsOf(m, top != 0, (f + 1) & 3)) stairsRound = false;
                    if (std::string(blockInfo(id).name).find("Stairs") == std::string::npos) stairsNamed = false;
                    if (blockDrop(id) != stairsOf(m, false, 0)) stairsRound = false;
                }
        check(stairsRound, "every flight of stairs knows its material, half and facing, and drops the one you hold");
        check(stairsNamed, "and is called stairs");
        check(stairsOf(0, false, 2) == Blocks::StoneStairsSouth && stairsOf(2, false, 0) == Blocks::PlankStairsNorth,
              "the four old materials keep the ids their saves hold");
        check(STAIRS_MATERIALS == 56, "all 56 kinds of stairs in 1.21.1 exist");

        bool slabsRound = true;
        for (int m = 0; m < SLAB_MATERIALS; ++m)
            for (int half = 0; half < 3; ++half)
            {
                const BlockId id = slabOf(m, half);
                if (slabMaterial(id) != m || slabHalf(id) != half) slabsRound = false;
                if (blockDrop(id) != slabOf(m, 0)) slabsRound = false;
                if ((half == 1) != isTopSlab(id)) slabsRound = false;
                if ((half == 2) != blockInfo(id).opaque) slabsRound = false;
            }
        check(slabsRound, "every slab has a bottom, a top and a doubled form, and each drops the slab");
        check(SLAB_MATERIALS == 60, "all 60 kinds of slab in 1.21.1 exist");
        check(slabOf(0, 0) == Blocks::StoneSlab && slabOf(2, 0) == Blocks::PlankSlab && slabMaterial(Blocks::EnchantingTable) < 0,
              "the old slabs keep their ids and a half-height table is not a slab");

        bool gatesRound = true;
        for (int w = 0; w < WOOD_COUNT; ++w)
            for (int open = 0; open < 2; ++open)
                for (int f = 0; f < 4; ++f)
                {
                    const BlockId id = gateOf(w, open != 0, f);
                    if (!isFenceGate(id) || gateWood(id) != w || gateOpen(id) != (open != 0) || gateFacing(id) != f) gatesRound = false;
                    if (blockDrop(id) != gateOf(w, false, 0)) gatesRound = false;
                }
        check(gatesRound, "a gate of every wood swings both ways and drops shut");
        check(!blockOverlaps(gateOf(0, true, 0), 0, 0, 0, 0.1f, 0.1f, 0.1f, 0.9f, 1.4f, 0.9f), "an open gate lets you through");
        check(blockOverlaps(gateOf(0, false, 0), 0, 0, 0, 0.1f, 0.1f, 0.1f, 0.9f, 1.4f, 0.9f), "a shut one does not");

        bool fences = true;
        for (int w = 0; w < WOOD_COUNT; ++w)
            if (!isFence(fenceOfWood(w)) || fenceWood(fenceOfWood(w)) != w || shapeBase(fenceOfWood(w)) != planksOfWood(w)) fences = false;
        check(fences, "a fence of every wood, made of that wood");
        check(!fenceJoins(fenceOfWood(1), Blocks::FenceRunLast) && fenceJoins(fenceOfWood(1), fenceOfWood(3)),
              "wooden fences join each other but not the nether brick one");
        check(WALL_MATERIALS == 25 && isWall(Blocks::WallRunFirst) && shapeBase(Blocks::WallRunFirst) == Blocks::Cobblestone,
              "all 25 walls exist, the first of cobblestone");

        auto at = [](const std::map<std::pair<int, int>, BlockId>& cells) {
            return [&cells](int dx, int dz) {
                const auto found = cells.find({ dx, dz });
                return found == cells.end() ? static_cast<BlockId>(Blocks::Air) : found->second;
            };
        };
        const BlockId north = stairsOf(STAIRS_OLD, false, 0);
        std::map<std::pair<int, int>, BlockId> alone;
        check(stairsQuadrantsAt(north, at(alone)) == stairsSideQuadrants(2), "a lone flight is straight, its step at the back");
        std::map<std::pair<int, int>, BlockId> outer{ { { 0, 1 }, stairsOf(STAIRS_OLD, false, 3) } };
        check(stairsQuadrantsAt(north, at(outer)) == 8, "with a turned flight behind it, it is an outer corner");
        std::map<std::pair<int, int>, BlockId> inner{ { { 0, -1 }, stairsOf(STAIRS_OLD, false, 3) } };
        check(stairsQuadrantsAt(north, at(inner)) == (12 | 2), "and with one in front, an inner corner");
        std::map<std::pair<int, int>, BlockId> mixed{ { { 0, 1 }, stairsOf(STAIRS_OLD, true, 3) } };
        check(stairsQuadrantsAt(north, at(mixed)) == stairsSideQuadrants(2), "upside-down stairs do not bend upright ones");

        bool based = true;
        for (int id = Blocks::ExtFirst; id < Blocks::ExtCount; ++id)
        {
            const BlockId b = static_cast<BlockId>(id);
            if (!isStairs(b) && !isSlabBlock(b) && !isFence(b) && !isFenceGate(b) && !isWall(b)) continue;
            if (shapeBase(b) == Blocks::Air) based = false;
            if (blockMaterial(b) != blockMaterial(shapeBase(b)) && slabMaterial(b) != PETRIFIED_SLAB) based = false;
        }
        check(based, "every shape in the second range is made of a real block and sounds and mines like it");
        check(blockMaterial(slabOf(PETRIFIED_SLAB, 0)) == Material::Stone && !isFlammable(slabOf(PETRIFIED_SLAB, 0)),
              "the petrified oak slab is stone that looks like oak");
        check(isFlammable(fenceOfWood(4)) && isFlammable(stairsOf(STAIRS_OLD + 4, false, 0)) && !isFlammable(Blocks::WallRunFirst),
              "wooden shapes burn and stone ones do not");

        int made = 0, cut = 0;
        const std::vector<StackId> outputs = Crafting::everyOutput();
        const std::vector<StackId> cuts = Stonecutting::everyOutput();
        for (int m = 0; m < STAIRS_MATERIALS; ++m)
            if (std::find(outputs.begin(), outputs.end(), stairsOf(m, false, 0)) != outputs.end()) ++made;
        check(made == STAIRS_MATERIALS, "every kind of stairs can be crafted");
        for (int m = 0; m < WALL_MATERIALS; ++m)
            if (std::find(cuts.begin(), cuts.end(), static_cast<StackId>(Blocks::WallRunFirst + m)) != cuts.end()) ++cut;
        check(cut == WALL_MATERIALS, "and every wall comes off a stonecutter");

        ItemStack grid[9];
        for (int i = 0; i < 9; ++i) grid[i] = ItemStack{};
        grid[0] = grid[1] = grid[2] = ItemStack{ Blocks::SprucePlanks, 1 };
        check(Crafting::match(grid, 3).id == slabOf(SLAB_OLD + 3, 0), "three spruce planks in a row make spruce slabs");
        grid[0] = grid[2] = ItemStack{ Items::Stick, 1 };
        grid[1] = ItemStack{ Blocks::CherryPlanks, 1 };
        grid[3] = grid[5] = ItemStack{ Items::Stick, 1 };
        grid[4] = ItemStack{ Blocks::CherryPlanks, 1 };
        check(Crafting::match(grid, 3).id == gateOf(6, false, 0), "and sticks round cherry planks make a cherry gate");
        check(Smelting::burnSeconds(slabOf(SLAB_OLD + 3, 0)) > 0.0f && Smelting::burnSeconds(Blocks::WallRunFirst) == 0.0f,
              "a wooden slab is fuel and a wall is not");
    }


    void testCarpetsWoodAndInfested()
    {
        section("blocks: carpets, wood and hyphae, infested stone, polished basalt");
        ItemStack grid[9];
        auto clear = [&grid]() { for (ItemStack& cell : grid) cell = ItemStack{}; };

        bool carpets = true;
        for (int c = 0; c < 16; ++c)
        {
            const BlockId carpet = static_cast<BlockId>(Blocks::CarpetFirst + c);
            if (!isCarpet(carpet) || blockMaterial(carpet) != Material::Wool || !isFlammable(carpet) || isSolid(carpet)) carpets = false;
            clear();
            grid[0] = grid[1] = ItemStack{ woolOfColour(c), 1 };
            const CraftOutput made = Crafting::match(grid, 3);
            if (made.id != carpet || made.count != 3) carpets = false;
        }
        check(carpets, "a carpet of every colour, two wool making three");
        clear();
        for (int i = 0; i < 8; ++i) grid[i] = ItemStack{ static_cast<StackId>(Blocks::CarpetFirst), 1 };
        grid[8] = ItemStack{ Items::DyeBlue, 1 };
        check(Crafting::match(grid, 3).id == colourIn(Blocks::CarpetFirst, 11) && Crafting::match(grid, 3).count == 8,
              "and eight carpets round a dye take its colour");

        bool wood = true;
        for (int i = 0; i < 20; ++i)
        {
            const BlockId block = static_cast<BlockId>(Blocks::WoodBlockFirst + i);
            const int kind = woodBlockWood(block);
            if (!isLog(block) || blockMaterial(block) != Material::Wood) wood = false;
            if (planksFromLog(block) != planksOfWood(kind)) wood = false;
            if (i < 10 && strippedOf(block) != woodBlockOf(kind, true)) wood = false;
            if (i >= 10 && (!isStrippedLog(block) || strippedOf(block) != Blocks::Air)) wood = false;
            clear();
            grid[0] = grid[1] = grid[3] = grid[4] = ItemStack{ i < 10 ? logOfWood(kind) : woodSet(kind).stripped, 1 };
            if (Crafting::match(grid, 3).id != block || Crafting::match(grid, 3).count != 3) wood = false;
            clear();
            grid[4] = ItemStack{ block, 1 };
            if (Crafting::match(grid, 3).id != planksOfWood(kind) || Crafting::match(grid, 3).count != 4) wood = false;
        }
        check(wood, "wood and hyphae of every tree: four logs make three, an axe strips them and they saw into planks");

        bool infested = true;
        for (int i = 0; i < 7; ++i)
        {
            const BlockId block = static_cast<BlockId>(Blocks::InfestedFirst + i);
            if (infestedOf(infestedBase(block)) != block || blockDrop(block) != Blocks::Air) infested = false;
            if (blockMaterial(block) != Material::Stone) infested = false;
            if (blockInfo(block).hardness >= blockInfo(infestedBase(block)).hardness) infested = false;
        }
        check(infested, "seven infested blocks that look like stone, break faster and drop nothing");

        clear();
        grid[0] = grid[1] = grid[3] = grid[4] = ItemStack{ Blocks::Basalt, 1 };
        check(Crafting::match(grid, 3).id == Blocks::PolishedBasalt && Crafting::match(grid, 3).count == 4,
              "four basalt polish into four");
    }


    void testSwitchesAndPlates()
    {
        section("redstone: buttons, pressure plates, wall torches and levers on walls");
        bool buttons = true;
        for (int m = 0; m < BUTTON_MATERIALS; ++m)
            for (int face = 0; face < 3; ++face)
                for (int f = 0; f < 4; ++f)
                    for (int on = 0; on < 2; ++on)
                    {
                        const BlockId id = buttonOf(m, face, f, on != 0);
                        if (!isFamilyButton(id) || buttonMaterial(id) != m || buttonFace(id) != face || buttonFacing(id) != f ||
                            buttonPowered(id) != (on != 0))
                            buttons = false;
                        if (Redstone::isSource(id) != (on != 0)) buttons = false;
                        if (blockDrop(id) != buttonItem(m)) buttons = false;
                        const int support = supportDirection(id);
                        if (face == 0 ? support != 0 : face == 2 ? support != 1 : support != supportFromFacing(f)) buttons = false;
                    }
        check(buttons, "thirteen kinds of button on floors, walls and ceilings, powering only while pressed");
        check(buttonSeconds(buttonOf(0, 1, 0, false)) == 1.0f && buttonSeconds(buttonOf(5, 1, 0, false)) == 1.5f,
              "a stone button holds for a second and a wooden one for a second and a half");
        check(blockDrop(buttonOf(0, 1, 2, true)) == Blocks::ButtonOff, "and a stone one comes back as the stone button");

        bool plates = true;
        for (int m = 0; m < PLATE_MATERIALS; ++m)
        {
            if (plateMaterial(plateOf(m, true)) != m || !platePowered(plateOf(m, true)) || platePowered(plateOf(m, false))) plates = false;
            if (!Redstone::isSource(plateOf(m, true)) || Redstone::isSource(plateOf(m, false))) plates = false;
            if (blockDrop(plateOf(m, true)) != plateOf(m, false) || supportDirection(plateOf(m, false)) != 0) plates = false;
        }
        check(plates, "fifteen pressure plates that power while something stands on them");
        check(!plateFeelsItems(plateOf(0, false)) && !plateFeelsItems(plateOf(1, false)) && plateFeelsItems(plateOf(2, false)) &&
              plateFeelsItems(plateOf(6, false)), "stone ones feel only creatures; weighted and wooden ones feel items too");
        check(blockMaterial(plateOf(3, false)) == Material::Stone && blockMaterial(plateOf(5, false)) == Material::Wood,
              "the heavy plate wants a pickaxe and a wooden one an axe");

        bool torches = true;
        for (int kind = 0; kind < 4; ++kind)
            for (int f = 0; f < 4; ++f)
            {
                const BlockId id = wallTorchOf(kind, f);
                if (!isWallTorch(id) || wallTorchKind(id) != kind || wallTorchFacing(id) != f) torches = false;
                if (supportDirection(id) != supportFromFacing(f)) torches = false;
            }
        check(torches, "torches, soul torches and redstone torches hang on all four walls");
        check(blockDrop(wallTorchOf(0, 2)) == Blocks::Torch && blockDrop(wallTorchOf(1, 0)) == Blocks::SoulTorch &&
              blockDrop(wallTorchOf(2, 1)) == Blocks::RedstoneTorch, "and break back into the torch you held");
        check(lightEmission(wallTorchOf(0, 0)) == 14 && Redstone::isSource(wallTorchOf(2, 3)),
              "a wall torch lights as a torch does and a redstone one powers");

        bool levers = true;
        for (int face = 0; face < 3; ++face)
            for (int f = 0; f < 4; ++f)
            {
                const BlockId off = leverOf(face, f, false), on = leverOf(face, f, true);
                if (leverOn(off) || !leverOn(on) || leverFace(on) != face || leverFacing(on) != f) levers = false;
                if (Redstone::isSource(off) || !Redstone::isSource(on) || blockDrop(on) != Blocks::LeverOff) levers = false;
            }
        check(levers, "levers on floors, walls and ceilings");
        check(supportDirection(Blocks::Torch) == 0 && supportDirection(Blocks::Stone) < 0, "a floor torch rests on what is under it");

        ItemStack grid[9];
        for (ItemStack& cell : grid) cell = ItemStack{};
        grid[4] = ItemStack{ Blocks::SprucePlanks, 1 };
        check(Crafting::match(grid, 3).id == buttonOf(3, 0, 0, false), "one spruce plank makes a spruce button");
        grid[4] = ItemStack{};
        grid[0] = grid[1] = ItemStack{ Blocks::BirchPlanks, 1 };
        check(Crafting::match(grid, 3).id == plateOf(6, false), "two birch planks make a birch plate");
        grid[0] = grid[1] = ItemStack{ Items::GoldIngot, 1 };
        check(Crafting::match(grid, 3).id == plateOf(2, false), "and two gold ingots the light weighted one");
    }


    void testFallingBlocks()
    {
        section("blocks: sand, gravel, concrete powder and anvils fall");
        check(fallsByGravity(Blocks::Sand) && fallsByGravity(Blocks::Gravel) && fallsByGravity(Blocks::RedSand) &&
              fallsByGravity(Blocks::ConcretePowderFirst + 4) && fallsByGravity(Blocks::Anvil) && fallsByGravity(Blocks::DragonEgg),
              "the blocks Minecraft lets fall do");
        check(!fallsByGravity(Blocks::Stone) && !fallsByGravity(Blocks::Dirt) && !fallsByGravity(Blocks::Sandstone),
              "and stone, dirt and sandstone stay where they are");
        check(canFallThrough(Blocks::Air) && canFallThrough(Blocks::Water) && canFallThrough(Blocks::TallGrass) &&
              !canFallThrough(Blocks::Stone) && !canFallThrough(Blocks::Torch), "they fall through air, water and grass, not stone");

        World world(42, "selftest-falling", 2, Dimension::Overworld);
        DroppedItems drops;
        drops.spawnFalling(glm::ivec3(0, 120, 0), Blocks::Sand);
        check(drops.fallingCount() == 1, "a block let go is in the air");
        std::vector<DroppedItems::Landing> landed;
        for (int frame = 0; frame < 400 && landed.empty(); ++frame) drops.updateFalling(0.05f, world, landed);
        check(landed.size() == 1 && landed[0].block == Blocks::Sand && landed[0].cell.y <= 0 && drops.fallingCount() == 0,
              "and with nothing under it, it comes down to the bottom of the world");
        std::error_code gone;
        std::filesystem::remove_all("selftest-falling", gone);

        section("projectiles: fireballs hit back, bullets home in, arrows picked up");
        {
            Projectiles shots;
            shots.spawn(glm::vec3(0.0f, 10.0f, 3.0f), glm::vec3(0.0f, 0.0f, -10.0f), 6, false, Arrows::Kind::Fire, 0, static_cast<int>(MobId::Ghast));
            check(shots.deflect(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 4.0f) && shots.all().size() == 1 &&
                  shots.all()[0].fromPlayer && shots.all()[0].velocity.z > 0.0f, "a punched fireball turns round and is the player's");
            Projectiles bullets;
            bullets.spawn(glm::vec3(0.0f, 10.0f, 3.0f), glm::vec3(0.0f, 0.0f, -7.0f), 4, false, Arrows::Kind::Bullet);
            check(bullets.deflect(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 4.0f) && bullets.all().empty(), "a punched shulker bullet breaks");
            ItemStack grid[9];
            const StackId pattern[9] = { Blocks::Glass, Blocks::Glass, Blocks::Glass, Blocks::Glass, Items::EyeOfEnder, Blocks::Glass,
                                         Blocks::Glass, Items::GhastTear, Blocks::Glass };
            for (int k = 0; k < 9; ++k) { grid[k].id = pattern[k]; grid[k].count = 1; }
            check(Crafting::match(grid, 3).id == static_cast<StackId>(Blocks::EndCrystal), "end crystals can be crafted, so the dragon can be brought back");
        }

        section("items: snowballs, eggs and ender pearls are thrown");
        bool thrown = true;
        for (Arrows::Kind kind : { Arrows::Kind::Snowball, Arrows::Kind::Egg, Arrows::Kind::Pearl })
        {
            Arrow arrow;
            arrow.kind = kind;
            if (arrow.shaft() || arrow.flies() || !arrow.bursts() || arrow.ghost()) thrown = false;
        }
        check(thrown, "all three are lobbed and break where they land");
    }


    void testSnowLayers()
    {
        section("snow layers: one to eight deep, stacked by hand, laid by snowfall");
        check(snowLayers(snowLayerOf(3)) == 3 && snowLayerOf(0) == Blocks::SnowLayerFirst && snowLayerOf(9) == Blocks::SnowLayerLast,
              "eight depths, one id each");
        check(!isSolid(snowLayerOf(1)) && isSolid(snowLayerOf(2)) && !blockInfo(snowLayerOf(7)).opaque && blockInfo(snowLayerOf(8)).opaque,
              "a single layer is walked through, eight are a full cube");
        check(blockOverlaps(snowLayerOf(4), 0, 0, 0, 0.2f, 0.3f, 0.2f, 0.8f, 1.2f, 0.8f, 0) &&
              !blockOverlaps(snowLayerOf(4), 0, 0, 0, 0.2f, 0.4f, 0.2f, 0.8f, 1.2f, 0.8f, 0),
              "and four stand three eighths of a block high, as Minecraft's collision has it");
        check(snowCanRestOn(Blocks::Stone) && snowCanRestOn(Blocks::Leaves) && snowCanRestOn(snowLayerOf(8)) &&
              !snowCanRestOn(Blocks::Ice) && !snowCanRestOn(Blocks::PackedIce) && !snowCanRestOn(snowLayerOf(3)) && !snowCanRestOn(Blocks::Air),
              "snow settles on full blocks and leaves but not on ice or shallow snow");
        check(supportDirection(snowLayerOf(5)) == 0, "and goes when the block under it goes");
        check(Tools::canHarvest(Items::WoodShovel, snowLayerOf(2)) && !Tools::canHarvest(Blocks::Air, snowLayerOf(2)) &&
              !Tools::canHarvest(Blocks::Air, Blocks::Snow), "only a shovel gets anything out of snow");
        check(blockDrop(snowLayerOf(6)) == Items::Snowball, "a layer breaks into snowballs");
        check(RecipeBook::idOf(static_cast<StackId>(Blocks::SnowLayerFirst)) == "snow" && RecipeBook::idOf(Blocks::Snow) == "snow_block" &&
              RecipeBook::stackNamed("snow") == static_cast<StackId>(Blocks::SnowLayerFirst),
              "the layer is 1.21.1's snow and the full block its snow_block");
        ItemStack row[9];
        for (int i : { 0, 1, 2 }) { row[i].id = Blocks::Snow; row[i].count = 1; }
        const CraftOutput layers = Crafting::match(row, 3);
        check(layers.valid() && layers.id == static_cast<StackId>(Blocks::SnowLayerFirst) && layers.count == 6,
              "three snow blocks in a row make six layers");

        const WorldGen generator(2024u);
        bool cold = false;
        int coldX = 0, coldZ = 0;
        for (int cz = -40; cz <= 40 && !cold; cz += 2)
            for (int cx = -40; cx <= 40 && !cold; cx += 2)
                if (generator.freezesAt(cx * 16 + 8, cz * 16 + 8) && generator.surfaceHeight(cx * 16 + 8, cz * 16 + 8) > WorldGen::SEA_LEVEL)
                {
                    cold = true;
                    coldX = cx;
                    coldZ = cz;
                }
        check(isAnyCauldron(Blocks::LavaCauldron) && isAnyCauldron(powderCauldronAt(2)) && !isCauldron(Blocks::LavaCauldron) &&
              powderCauldronLevel(powderCauldronAt(9)) == CAULDRON_FULL && powderCauldronAt(0) == Blocks::Cauldron0,
              "a cauldron holds lava, or powder snow at three depths");
        check(blockInfo(Blocks::LavaCauldron).lightEmission == 15 && blockDrop(Blocks::LavaCauldron) == Blocks::Cauldron0 &&
              blockDrop(powderCauldronAt(1)) == Blocks::Cauldron0 && !isObtainable(Blocks::LavaCauldron),
              "lava in a cauldron glows, and breaking either gives back the empty cauldron");
        check(cold, "there is cold land near the origin of a test world");
        if (cold)
        {
            Chunk chunk(ChunkPos{ coldX, coldZ });
            generator.generate(chunk);
            int covered = 0, columns = 0;
            for (int lz = 0; lz < Chunk::SZ; ++lz)
                for (int lx = 0; lx < Chunk::SX; ++lx)
                {
                    if (!generator.freezesAt(coldX * 16 + lx, coldZ * 16 + lz)) continue;
                    ++columns;
                    int top = Chunk::SY - 1;
                    while (top > 0 && chunk.getBlock(lx, top, lz) == Blocks::Air) --top;
                    const BlockId at = chunk.getBlock(lx, top, lz);
                    if (isSnowLayer(at) || at == Blocks::Ice || at == Blocks::PackedIce || at == Blocks::Snow) ++covered;
                }
            check(columns > 0 && covered * 10 >= columns * 7, "a cold chunk comes out with snow lying on top of it");
        }
    }

    void testHeads()
    {
        section("mob heads: seven kinds, sixteen turns on the floor and four on a wall");
        check(headKind(headOf(4, false, 9)) == 4 && headRotation(headOf(4, false, 9)) == 9 && !headOnWall(headOf(4, false, 9)),
              "a creeper head set down at the ninth turn");
        check(headOnWall(headOf(2, true, 3)) && headFacing(headOf(2, true, 3)) == 3 && headKind(headOf(2, true, 3)) == 2,
              "a zombie head on a wall faces one of four ways");
        check(blockDrop(headOf(1, true, 2)) == Blocks::WitherSkull && isWitherSkullBlock(headOf(1, false, 5)) &&
              blockDrop(headOf(0, false, 7)) == static_cast<StackId>(headOf(0, false, 0)),
              "every turn breaks back into the one item, the wither's into its old skull");
        check(blockOverlaps(headOf(0, false, 3), 0, 0, 0, 0.4f, 0.1f, 0.4f, 0.6f, 0.4f, 0.6f, 0) &&
              !blockOverlaps(headOf(0, false, 3), 0, 0, 0, 0.4f, 0.6f, 0.4f, 0.6f, 0.9f, 0.6f, 0) &&
              blockOverlaps(headOf(0, true, 0), 0, 0, 0, 0.4f, 0.4f, 0.9f, 0.6f, 0.6f, 0.95f, 0) &&
              !blockOverlaps(headOf(0, true, 0), 0, 0, 0, 0.4f, 0.4f, 0.05f, 0.6f, 0.6f, 0.2f, 0),
              "a skull is half a block high on the floor and hangs against the wall it is set on");
        check(Armour::pieceOf(static_cast<StackId>(headOf(3, false, 0))) == Armour::Piece::Helmet &&
              Armour::pieceOf(Blocks::WitherSkull) == Armour::Piece::Helmet, "a head is worn as a helmet");
        check(Tools::canHarvest(Blocks::Air, headOf(5, false, 0)), "and comes off by hand");
        check(RecipeBook::stackNamed("creeper_banner_pattern") == static_cast<StackId>(Items::CreeperBannerPattern) &&
              RecipeBook::stackNamed("mojang_banner_pattern") == static_cast<StackId>(Items::MojangBannerPattern),
              "the creeper charge and thing banner patterns exist under their 1.21.1 names");
        check(Banners::patternOfItem(Items::CreeperBannerPattern) == Banners::Creeper && Banners::patternOfItem(Items::MojangBannerPattern) == Banners::Mojang,
              "and each puts its own charge on a banner in the loom");
        {
            ItemStack grid[9];
            grid[0] = ItemStack{ Items::Paper, 1 };
            grid[1] = ItemStack{ static_cast<StackId>(headOf(4, false, 0)), 1 };
            const CraftOutput creeper = Crafting::match(grid, 2);
            grid[1] = ItemStack{ Items::EnchantedGoldenApple, 1 };
            const CraftOutput thing = Crafting::match(grid, 2);
            check(creeper.valid() && creeper.id == Items::CreeperBannerPattern && thing.valid() && thing.id == Items::MojangBannerPattern,
                  "paper and a creeper head make one, paper and an enchanted golden apple the other");
        }
        Mob creeper(MobId::Creeper, glm::vec3(0.0f), 7u);
        check(!creeper.charged(), "a creeper starts uncharged");
        creeper.setCharged(true);
        check(creeper.charged(), "and lightning charges it");
        check(debugStickTurn(snowLayerOf(8), 1) == snowLayerOf(1) && debugStickTurn(snowLayerOf(1), -1) == snowLayerOf(8) &&
              debugStickTurn(headOf(2, false, 15), 1) == headOf(2, true, 0) && debugStickTurn(Blocks::Stone, 1) == Blocks::Stone,
              "the debug stick steps a block through its own states and leaves stateless ones alone");
        check(maxStackOf(Items::DebugStick) == 1 && RecipeBook::stackNamed("debug_stick") == static_cast<StackId>(Items::DebugStick),
              "and is one unstackable item under its 1.21.1 name");
        check(blockDrop(Blocks::BambooShoot) == Blocks::Bamboo && !isSolid(Blocks::BambooShoot) && isSolid(Blocks::Bamboo) &&
              blockOverlaps(Blocks::Bamboo, 0, 0, 0, 0.45f, 0.2f, 0.45f, 0.55f, 0.8f, 0.55f, 0) &&
              !blockOverlaps(Blocks::Bamboo, 0, 0, 0, 0.0f, 0.2f, 0.0f, 0.3f, 0.8f, 0.3f, 0),
              "bamboo starts as a shoot you walk through and grows into a thin stalk you bump into");
        check(maxStackOf(Blocks::ShulkerBox) == 1 && maxStackOf(static_cast<StackId>(Blocks::ShulkerBoxFirst + 3)) == 1 && maxStackOf(Blocks::Bed) == 1,
              "shulker boxes and beds stack to one, as in 1.21.1");

        {
            const std::vector<Trading::Deal> stock = Trading::wandererDeals(12345u);
            bool emeralds = true;
            for (const Trading::Deal& deal : stock) emeralds = emeralds && deal.offer.want == Items::Emerald && deal.offer.valid();
            check(stock.size() == 6 && emeralds && Trading::wandererDeals(12345u)[0].offer.give == stock[0].offer.give,
                  "a wandering trader sells five common things and one rare one for emeralds, the same five for the same trader");
        }

        section("campfires");
        check(isAnyCampfire(Blocks::CampfireUnlit) && !isCampfire(Blocks::CampfireUnlit) && lightEmission(Blocks::CampfireUnlit) == 0 &&
              lightEmission(Blocks::Campfire) == 15 && isSoulCampfire(Blocks::SoulCampfireUnlit),
              "a campfire can be put out, and gives no light when it is");
        check(blockDrop(Blocks::Campfire) == Items::Charcoal && blockDrop(Blocks::SoulCampfireUnlit) == Blocks::SoulSoil,
              "it breaks into charcoal, a soul campfire into soul soil");
        check(blockOverlaps(Blocks::Campfire, 0, 0, 0, 0.2f, 0.3f, 0.2f, 0.8f, 0.4f, 0.8f, 0) &&
              !blockOverlaps(Blocks::Campfire, 0, 0, 0, 0.2f, 0.5f, 0.2f, 0.8f, 0.9f, 0.8f, 0),
              "and is a low pile of logs you can stand in the flames of");
        check(Smelting::resultIn(Smelting::Kind::Smoker, Items::RawBeef) == Items::Steak &&
              Smelting::resultIn(Smelting::Kind::Smoker, Blocks::Cobblestone) == Blocks::Air,
              "it cooks what a smoker cooks and nothing else");

        section("cactus");
        check(!blockInfo(Blocks::Cactus).opaque && blockOverlaps(Blocks::Cactus, 0, 0, 0, 0.5f, 0.2f, 0.05f, 0.6f, 0.5f, 0.1f, 0) &&
              !blockOverlaps(Blocks::Cactus, 0, 0, 0, 0.5f, 0.2f, 0.0f, 0.6f, 0.5f, 0.05f, 0),
              "a cactus stands a pixel in from its cell on every side, so you can brush against it");

        section("farmland dries out and is trampled");
        check(isFarmland(Blocks::FarmlandDry) && isFarmland(Blocks::Farmland) && blockDrop(Blocks::FarmlandDry) == Blocks::Dirt &&
              blockInfo(Blocks::FarmlandDry).tileTop != blockInfo(Blocks::Farmland).tileTop && !isObtainable(Blocks::FarmlandDry),
              "dry farmland is its own block with its own texture, and still farmland");
        check(blockMaterial(Blocks::FarmlandDry) == blockMaterial(Blocks::Farmland), "and is dug like wet farmland");

        section("logs, stems and pillars lie the way they are set");
        {
            const BlockId sideways = withAxis(Blocks::BirchLog, 1);
            check(sideways != Blocks::BirchLog && axisBase(sideways) == Blocks::BirchLog && blockAxis(sideways) == 1 &&
                  blockAxis(withAxis(Blocks::QuartzPillar, 2)) == 2 && withAxis(Blocks::Stone, 1) == Blocks::Stone,
                  "a log set against a wall lies along x or z, and a block with no grain stays as it is");
            check(isLog(sideways) && blockDrop(sideways) == Blocks::BirchLog && !isObtainable(sideways) &&
                  std::string(blockInfo(sideways).name) == blockInfo(Blocks::BirchLog).name,
                  "and is the same log in every other way");
            check(strippedOf(sideways) == withAxis(Blocks::StrippedBirchLog, 1) && planksFromLog(sideways) == Blocks::BirchPlanks,
                  "stripping it keeps it lying the same way");
            check(blockInfo(withAxis(Blocks::OchreFroglight, 2)).lightEmission == 15 && hasAxis(Blocks::Deepslate) && hasAxis(Blocks::HayBale),
                  "froglights, deepslate, hay and the rest of 1.21.1's axis blocks turn too");
        }

        section("the last six animals with spawn eggs, and the giant");
        {
            bool named = true, eggs = true, sheets = true;
            for (MobId kind : { MobId::Donkey, MobId::Mule, MobId::Mooshroom, MobId::Ocelot, MobId::SkeletonHorse, MobId::ZombieHorse })
            {
                unsigned int base = 0, spots = 0;
                if (!spawnEggColours(kind, base, spots)) eggs = false;
                if (!*mobType(kind).packSheet) sheets = false;
            }
            named = EntityManager::mobFromId("donkey") == MobId::Donkey && EntityManager::mobFromId("mule") == MobId::Mule &&
                    EntityManager::mobFromId("mooshroom") == MobId::Mooshroom && EntityManager::mobFromId("ocelot") == MobId::Ocelot &&
                    EntityManager::mobFromId("skeleton_horse") == MobId::SkeletonHorse && EntityManager::mobFromId("zombie_horse") == MobId::ZombieHorse &&
                    EntityManager::mobFromId("giant") == MobId::Giant;
            check(named, "donkey, mule, mooshroom, ocelot, skeleton horse, zombie horse and giant answer to their 1.21.1 names");
            check(eggs && sheets, "the six with eggs in 1.21.1 have them, and every one is drawn from its pack sheet");
            check(isHorseKind(MobId::Donkey) && isHorseKind(MobId::ZombieHorse) && mobType(MobId::Donkey).rideable &&
                  !isHorseKind(MobId::Mooshroom), "the four horse kinds are ridden like a horse");
            check(mobType(MobId::Giant).modelScale > 5.0f && mobType(MobId::Giant).maxHealth == 100, "a giant is a zombie six times over");
        }
        check(bambooCanRootIn(Blocks::Podzol) && bambooCanRootIn(Blocks::Gravel) && !bambooCanRootIn(Blocks::Stone),
              "and roots in the ground 1.21.1 lets it");
    }

    void testGameRules()
    {
        section("game rules: all 51, saved per world and set with /gamerule");
        check(GameRules::COUNT == 51, "all 51 rules of 1.21.1 exist");
        GameRules::Set rules;
        check(rules.isDefault() && !rules.on(GameRules::Rule::KeepInventory) && rules.on(GameRules::Rule::DoDaylightCycle) &&
              rules.value(GameRules::Rule::RandomTickSpeed) == 3 && rules.value(GameRules::Rule::SpawnRadius) == 10 &&
              rules.value(GameRules::Rule::PlayersSleepingPercentage) == 100, "with Minecraft's defaults");
        GameRules::Rule found;
        check(GameRules::find("keepInventory", found) && found == GameRules::Rule::KeepInventory && !GameRules::find("keepinventory", found),
              "found by their exact names");
        rules.set(GameRules::Rule::KeepInventory, 1);
        rules.set(GameRules::Rule::RandomTickSpeed, 40);
        GameRules::Set back;
        back.load(rules.save());
        check(back.on(GameRules::Rule::KeepInventory) && back.value(GameRules::Rule::RandomTickSpeed) == 40 && !back.isDefault(),
              "and come back from a save as they were");
        const Chat::Command set = Chat::parse("/gamerule keepInventory true", true);
        check(set.kind == Chat::Kind::GameRuleSet && set.mob == static_cast<int>(GameRules::Rule::KeepInventory) && set.count == 1,
              "/gamerule sets a rule");
        const Chat::Command ask = Chat::parse("/gamerule randomTickSpeed", true);
        check(ask.kind == Chat::Kind::GameRuleQuery, "and with no value says what it is");
        check(Chat::parse("/gamerule keepInventory maybe", true).kind == Chat::Kind::Unknown &&
              Chat::parse("/gamerule nonsense true", true).kind == Chat::Kind::Unknown, "and refuses a bad value or name");
        check(Chat::parse("/gamerule keepInventory true", false).kind == Chat::Kind::Denied, "it is a cheat");
    }

    void testEverySoundHasANoise()
    {
        section("sounds: every sound makes a noise");

        // The synthesised set is what browsercraft.net serves, so a
        // sound with nothing behind it is silent for everybody who has
        // not dropped their own audio in.
        AudioEngine audio;

        std::vector<int> silent;
        for (int i = 0; i < static_cast<int>(Sound::Count); ++i)
            if (audio.clipCount(static_cast<Sound>(i)) <= 0) silent.push_back(i);

        for (int i : silent)
            check(false, "sound " + std::to_string(i) + " has nothing to play");
        check(silent.empty(),
              "all " + std::to_string(static_cast<int>(Sound::Count)) +
                  " sounds have something to play");

        // Every mob's three voices must be real sounds, or a species
        // added without them falls back to whatever sound zero is.
        bool voiced = true;
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobType& t = mobType(static_cast<MobId>(i));
            if (audio.clipCount(t.voice) <= 0) voiced = false;
            if (audio.clipCount(t.hurtVoice) <= 0) voiced = false;
            if (audio.clipCount(t.deathVoice) <= 0) voiced = false;
        }
        check(voiced, "every mob has a voice, a yelp and a death cry");

        // No two species should share a voice: that is the mark of one
        // added by copying another and never given its own.
        std::set<int> voices;
        bool distinct = true;
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobType& t = mobType(static_cast<MobId>(i));
            if (t.inanimate || static_cast<MobId>(i) == MobId::TropicalFishLarge || static_cast<MobId>(i) == MobId::TraderLlama ||
                static_cast<MobId>(i) == MobId::Mooshroom || static_cast<MobId>(i) == MobId::Giant) continue;
            if (!voices.insert(static_cast<int>(t.voice)).second) distinct = false;
        }
        check(distinct, "and no two living species share one");
    }

    void testDrowning()
    {
        section("drowning");

        Player diver(glm::vec3(0.0f, 64.0f, 0.0f));
        check(diver.air == Player::MAX_AIR_SECONDS, "you start with a full lungful");

        // Fourteen seconds under and nothing has happened yet.
        bool hurtEarly = false;
        for (int i = 0; i < 140; ++i)
            if (diver.breathe(0.1f, true)) hurtEarly = true;
        check(!hurtEarly, "fourteen seconds under costs nothing");
        check(diver.health == Player::MAX_HEALTH, "and you are untouched");
        check(diver.air > 0.0f && diver.air < 2.0f, "but the air is nearly gone");

        // Past fifteen, it starts.
        int blows = 0;
        for (int i = 0; i < 100; ++i)
            if (diver.breathe(0.1f, true)) ++blows;
        check(diver.air == 0.0f, "the air runs out");
        check(blows > 0, "and then it starts to hurt");
        check(diver.health < Player::MAX_HEALTH, "which costs health");

        // Once a second, not once a frame: the whole point of the
        // timer. Ten seconds of drowning is about ten blows, not a
        // hundred, and certainly not one per step.
        Player counter(glm::vec3(0.0f, 64.0f, 0.0f));
        counter.air = 0.0f;
        int landed = 0;
        for (int i = 0; i < 100; ++i)   // ten seconds at a tenth each
            if (counter.breathe(0.1f, true)) ++landed;
        check(landed >= 9 && landed <= 11,
              "ten seconds drowning lands " + std::to_string(landed) + " blows, about one a second");

        // Drowning must not be swallowed by the mercy that follows an
        // ordinary blow, or you could sit on the bottom for ever.
        Player sitting(glm::vec3(0.0f, 64.0f, 0.0f));
        sitting.air = 0.0f;
        for (int i = 0; i < 300 && sitting.health > 0; ++i)
            sitting.breathe(0.1f, true);
        check(sitting.health <= 0, "stay under long enough and it kills you");

        // Surfacing refills, and faster than it emptied.
        Player surfacing(glm::vec3(0.0f, 64.0f, 0.0f));
        surfacing.air = 0.0f;
        for (int i = 0; i < 40; ++i) surfacing.breathe(0.1f, false);
        check(surfacing.air == Player::MAX_AIR_SECONDS,
              "four seconds up refills fifteen seconds of air");

        bool hurtAbove = false;
        for (int i = 0; i < 200; ++i)
            if (surfacing.breathe(0.1f, false)) hurtAbove = true;
        check(!hurtAbove, "and standing in the air never drowns you");

        // Creative does not drown.
        Player builder(glm::vec3(0.0f, 64.0f, 0.0f));
        builder.mode = GameMode::Creative;
        bool hurtInCreative = false;
        for (int i = 0; i < 400; ++i)
            if (builder.breathe(0.1f, true)) hurtInCreative = true;
        check(!hurtInCreative, "creative does not drown");
        check(builder.air == Player::MAX_AIR_SECONDS, "and never loses air");

        // Dying and coming back gives you your breath back.
        Player drowned(glm::vec3(0.0f, 64.0f, 0.0f));
        drowned.air = 0.0f;
        drowned.respawn(glm::vec3(0.0f, 64.0f, 0.0f));
        check(drowned.air == Player::MAX_AIR_SECONDS, "respawning restores your air");
    }

    void testQuickMoveKeepsMetadata()
    {
        section("inventory: shift-clicking keeps what an item carries");

        // A tool carries wear, and now enchantments too. Both have to
        // survive being shift-clicked out of a container: mergeInto
        // moves only the count, so a destination slot that starts empty
        // gets the id and the count and nothing else -- which repaired
        // the tool and stripped it on the way.
        Inventory pack;
        ItemStack& inChest = pack.slot(Inventory::CHEST_FIRST);
        inChest.id = Items::DiamondPickaxe;
        inChest.count = 1;
        inChest.damage = 431;
        inChest.enchantments =
            Enchanting::withLevel(0u, Enchanting::Kind::Efficiency, 4);

        pack.quickMove(Inventory::CHEST_FIRST);

        check(pack.slot(Inventory::CHEST_FIRST).empty(), "it leaves the chest");

        int found = -1;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i)
            if (pack.slot(i).id == Items::DiamondPickaxe) found = i;
        check(found >= 0, "and arrives in the pack");

        if (found >= 0)
        {
            check(pack.slot(found).damage == 431,
                  "with its wear, rather than repaired on the way");
            check(Enchanting::levelOf(pack.slot(found).enchantments,
                                      Enchanting::Kind::Efficiency) == 4,
                  "and with its enchantments still on it");
        }

        // The same out of an armour slot, which is the other place a
        // worn thing sits.
        Inventory worn;
        ItemStack& helmet = worn.slot(Inventory::ARMOR_FIRST);
        helmet.id = Items::DiamondHelmet;
        helmet.count = 1;
        helmet.damage = 77;
        helmet.enchantments =
            Enchanting::withLevel(0u, Enchanting::Kind::Protection, 3);

        worn.quickMove(Inventory::ARMOR_FIRST);

        int helmetAt = -1;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i)
            if (worn.slot(i).id == Items::DiamondHelmet) helmetAt = i;
        check(helmetAt >= 0, "a helmet comes off into the pack");
        if (helmetAt >= 0)
        {
            check(worn.slot(helmetAt).damage == 77, "keeping its wear");
            check(Enchanting::levelOf(worn.slot(helmetAt).enchantments,
                                      Enchanting::Kind::Protection) == 3,
                  "and its protection");
        }

        // A plain stack still moves the ordinary way.
        Inventory plain;
        plain.slot(Inventory::CHEST_FIRST).id = Blocks::Stone;
        plain.slot(Inventory::CHEST_FIRST).count = 64;
        plain.quickMove(Inventory::CHEST_FIRST);
        check(plain.countOf(Blocks::Stone) == 64, "and a plain stack still moves whole");

        section("inventory: the crafting grid gives back what it was given");

        // Closing the screen empties the grid back into the pack. It
        // used to put back only an id and a count, so a tool left in a
        // recipe came out repaired and stripped.
        Inventory grid;
        ItemStack& inGrid = grid.slot(Inventory::CRAFT_FIRST);
        inGrid.id = Items::IronSword;
        inGrid.count = 1;
        inGrid.damage = 61;
        inGrid.enchantments = Enchanting::withLevel(0u, Enchanting::Kind::Sharpness, 2);

        std::vector<ItemStack> spilled;
        grid.clearCraftGrid(spilled);

        int swordAt = -1;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i)
            if (grid.slot(i).id == Items::IronSword) swordAt = i;
        check(swordAt >= 0, "a sword left in the grid comes back");
        if (swordAt >= 0)
        {
            check(grid.slot(swordAt).damage == 61, "with its wear");
            check(Enchanting::levelOf(grid.slot(swordAt).enchantments,
                                      Enchanting::Kind::Sharpness) == 2,
                  "and its sharpness");
        }

        section("inventory: a stack put in whole comes out whole");

        // addStack is what every one of those paths now goes through.
        Inventory direct;
        ItemStack axe;
        axe.id = Items::DiamondAxe;
        axe.count = 1;
        axe.damage = 9;
        axe.enchantments = Enchanting::withLevel(0u, Enchanting::Kind::Unbreaking, 3);
        check(direct.addStack(axe) == 0, "an enchanted axe fits");
        check(direct.slot(0).damage == 9, "with its wear");
        check(Enchanting::levelOf(direct.slot(0).enchantments,
                                  Enchanting::Kind::Unbreaking) == 3,
              "and its unbreaking");

        // A full pack hands the whole thing back rather than swallowing
        // it, which is what the caller needs in order to drop it.
        Inventory full;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i)
        {
            full.slot(i).id = Blocks::Stone;
            full.slot(i).count = 64;
        }
        check(full.addStack(axe) == axe.count, "a full pack gives it straight back");
    }

    void testContainersPerDimension()
    {
        section("chests and furnaces belong to a dimension");

        // A chest in the Nether at the same x, y and z as one in the
        // overworld is a different chest. Keyed on position alone they
        // shared their contents, so a chest built at matching
        // coordinates in the Nether opened holding the overworld's
        // loot -- and emptying it emptied the one back home.
        const glm::ivec3 spot(100, 64, 100);

        Chests chests;
        chests.setDimension(0);
        chests.at(spot)[0] = ItemStack{ Items::Diamond, 7 };
        check(chests.has(spot), "a chest in the overworld is on record");

        chests.setDimension(1);
        check(!chests.has(spot), "and the same spot in the Nether is empty");

        chests.at(spot)[0] = ItemStack{ Blocks::Netherrack, 3 };
        check(chests.at(spot)[0].id == Blocks::Netherrack, "which can hold its own things");

        chests.setDimension(0);
        check(chests.at(spot)[0].id == Items::Diamond, "while the overworld keeps its diamonds");
        check(chests.at(spot)[0].count == 7, "all seven of them");

        // Breaking one must not reach through to the other.
        chests.remove(spot);
        check(!chests.has(spot), "breaking the overworld chest forgets it");
        chests.setDimension(1);
        check(chests.has(spot), "and leaves the Nether one standing");

        // Two chests at the same spot are two records, not one.
        check(chests.all().size() == 1, "one left of the two");

        section("furnaces likewise");

        Furnaces furnaces;
        furnaces.setDimension(0);
        furnaces.at(spot).burnLeft = 12.0f;

        furnaces.setDimension(1);
        check(!furnaces.has(spot), "a furnace is its own in each world");
        furnaces.at(spot).burnLeft = 3.0f;

        furnaces.setDimension(0);
        check(furnaces.at(spot).burnLeft == 12.0f, "and keeps burning its own fuel");
        check(furnaces.all().size() == 2, "both are on record");

        section("saving: containers come back to the world they were in");

        const std::string dir = "selftest-dimsave";
        WorldSave::ensureDirectories(dir);

        LevelState wrote;
        wrote.seed = 7u;

        LevelState::SavedChest home;
        home.x = 100; home.y = 64; home.z = 100;
        home.slots.resize(Chests::SLOTS, { 0, 0 });
        home.slots[0] = { static_cast<uint16_t>(Items::Diamond), 7 };
        wrote.chests.push_back(home);
        wrote.chestDimensions.push_back(0);

        LevelState::SavedChest below = home;
        below.slots[0] = { static_cast<uint16_t>(Blocks::Netherrack), 3 };
        wrote.chests.push_back(below);
        wrote.chestDimensions.push_back(1);

        WorldSave::saveLevel(dir, wrote);

        LevelState read;
        check(WorldSave::loadLevel(dir, read), "a level with two such chests reads back");
        check(read.chests.size() == 2, "both of them");
        check(read.chestDimensions.size() == 2, "each with the world it was in");
        if (read.chestDimensions.size() == 2)
        {
            check(read.chestDimensions[0] == 0, "the first in the overworld");
            check(read.chestDimensions[1] == 1, "and the second in the Nether");
        }

        section("signs");

        // Four facings off the same arithmetic the ladder uses, so a
        // sign hung on a north wall is the one that faces north.
        for (int facing = 0; facing < 4; ++facing)
        {
            const BlockId board = signFacingAs(facing);
            check(isSign(board), "every facing reads as a sign");
            check(signFacing(board) == facing, "and knows which way it faces");
            check(blockDrop(board) == Items::Sign,
                  "and gives back one sign however it was hung");
            check(!isObtainable(board),
                  "the block is where it ended up, not the thing in your hand");
            check(!blockInfo(board).collides, "you walk through a board");
        }
        check(signFacingAs(99) == Blocks::SignWest, "a facing cannot run off the end");
        check(signFacingAs(-4) == Blocks::SignNorth, "nor off the front");
        check(!isSign(Blocks::LadderNorth), "a ladder is not a sign");

        // What a sign says belongs to the sign, and to the world it
        // stands in. Keyed on position alone, a sign in the Nether at
        // matching coordinates read back the overworld's words --
        // which is exactly the bug the chests had.
        Signs signs;
        signs.setDimension(0);
        signs.at(spot) = Signs::Text{ "HOME", "", "", "" };
        check(signs.has(spot), "a sign in the overworld is on record");

        signs.setDimension(1);
        check(!signs.has(spot), "and the same spot in the Nether is blank");
        signs.at(spot) = Signs::Text{ "MIND", "THE", "LAVA", "" };

        signs.setDimension(0);
        check(signs.find(spot) != nullptr, "the overworld one is still there");
        check((*signs.find(spot))[0] == "HOME", "and still says what it said");
        check(signs.all().size() == 2, "two signs, not one");

        signs.remove(spot);
        check(!signs.has(spot), "breaking it forgets what it said");
        signs.setDimension(1);
        check(signs.has(spot), "and leaves the Nether one standing");

        // A board with nothing on it needs no row.
        check(Signs::blank(Signs::Text{}), "four blank lines is a blank sign");
        check(!Signs::blank(Signs::Text{ "", "", "X", "" }), "one word is not");

        // Only what is close enough to read.
        {
            Signs reach;
            reach.setDimension(0);
            reach.at(glm::ivec3(0, 64, 0)) = Signs::Text{ "NEAR", "", "", "" };
            reach.at(glm::ivec3(60, 64, 0)) = Signs::Text{ "FAR", "", "", "" };

            const auto close = reach.near(glm::vec3(0.0f, 64.0f, 0.0f), 14.0f);
            check(close.size() == 1, "only the near one is worth drawing");
            if (close.size() == 1)
                check((*close[0].text)[0] == "NEAR", "and it is the near one");

            reach.setDimension(1);
            check(reach.near(glm::vec3(0.0f, 64.0f, 0.0f), 14.0f).empty(),
                  "and a sign in another world is not drawn in this one");
        }

        // Through the file and back, words and all.
        {
            LevelState withSigns;
            withSigns.seed = 11u;

            LevelState::SavedSign written;
            written.x = 4; written.y = 70; written.z = -9;
            written.dimension = 1;
            written.lines = { "TO THE", "FORTRESS", "->", "" };
            withSigns.signs.push_back(written);

            WorldSave::saveLevel(dir, withSigns);

            LevelState back;
            check(WorldSave::loadLevel(dir, back), "a level with a sign reads back");
            check(back.signs.size() == 1, "the sign is there");
            if (back.signs.size() == 1)
            {
                check(back.signs[0].x == 4 && back.signs[0].y == 70 &&
                      back.signs[0].z == -9, "where it was");
                check(back.signs[0].dimension == 1, "in the world it was in");
                check(back.signs[0].lines[0] == "TO THE", "saying what it said");
                check(back.signs[0].lines[1] == "FORTRESS", "every line of it");
                check(back.signs[0].lines[3].empty(), "blank lines included");
            }
        }

        section("item frames");

        for (int facing = 0; facing < 4; ++facing)
        {
            const BlockId frame = frameFacingAs(facing);
            check(isFrame(frame), "every facing reads as a frame");
            check(frameFacing(frame) == facing, "and knows which way it faces");
            check(blockDrop(frame) == Items::ItemFrame, "and gives back one frame");
            check(!isObtainable(frame), "the block is where it ended up");
            check(!isSign(frame), "a frame is not a sign");
        }
        check(frameFacingAs(9) == Blocks::FrameWest, "a facing cannot run off the end");
        for (int facing = 0; facing < 4; ++facing)
        {
            const BlockId glowing = frameFacingAs(facing, true);
            check(isFrame(glowing) && isGlowFrame(glowing) && frameFacing(glowing) == facing && !isGlowFrame(frameFacingAs(facing)),
                  "a glow item frame hangs four ways like a plain one");
        }
        check(blockDrop(Blocks::GlowFrameEast) == Items::GlowItemFrame, "and comes back as a glow item frame");
        {
            ItemStack grid[9];
            for (ItemStack& cell : grid) cell.clear();
            grid[0] = ItemStack{ Items::ItemFrame, 1 };
            grid[4] = ItemStack{ Items::GlowInkSac, 1 };
            check(Crafting::match(grid, 3).id == Items::GlowItemFrame, "an item frame and a glow ink sac make one");
        }

        // The four facings have to agree with the way a wall is faced,
        // or the words and the item stand inside the wall instead of
        // in front of it.
        check(wallOut(0) == glm::vec3(0.0f, 0.0f, 1.0f), "north faces +z");
        check(wallOut(1) == glm::vec3(-1.0f, 0.0f, 0.0f), "east faces -x");
        check(wallOut(2) == glm::vec3(0.0f, 0.0f, -1.0f), "south faces -z");
        check(wallOut(3) == glm::vec3(1.0f, 0.0f, 0.0f), "west faces +x");
        for (int facing = 0; facing < 4; ++facing)
            check(glm::length(wallOut(facing)) > 0.99f &&
                  glm::length(wallOut(facing)) < 1.01f,
                  "and each of them is one block long");

        Frames frames;
        frames.setDimension(0);
        frames.at(spot) = ItemStack{ Items::Diamond, 1 };
        check(frames.has(spot), "a frame in the overworld is on record");

        frames.setDimension(1);
        check(!frames.has(spot), "and the same spot in the Nether is empty");
        frames.at(spot) = ItemStack{ Blocks::Netherrack, 1 };

        frames.setDimension(0);
        check(frames.find(spot) != nullptr && frames.find(spot)->id == Items::Diamond,
              "and the overworld one keeps its diamond");

        {
            Frames reach;
            reach.setDimension(0);
            reach.at(glm::ivec3(0, 64, 0)) = ItemStack{ Items::Diamond, 1 };
            reach.at(glm::ivec3(90, 64, 0)) = ItemStack{ Items::Coal, 1 };
            // A frame nobody has put anything in is nothing to draw.
            reach.at(glm::ivec3(1, 64, 0));

            const auto close = reach.near(glm::vec3(0.0f, 64.0f, 0.0f), 32.0f);
            check(close.size() == 1, "only the near, full one is worth drawing");
            if (close.size() == 1)
                check(close[0].held.id == Items::Diamond, "and it is the near one");
        }

        {
            LevelState withFrames;
            withFrames.seed = 13u;

            LevelState::SavedFrame hung;
            hung.x = -3; hung.y = 65; hung.z = 8;
            hung.dimension = 0;
            hung.item = static_cast<uint16_t>(Items::DiamondPickaxe);
            hung.count = 1;
            hung.damage = 44;
            hung.enchantments = 0x15u;
            withFrames.frames.push_back(hung);

            LevelState::SavedBeacon beacon;
            beacon.x = 10; beacon.y = 70; beacon.z = -4;
            beacon.primary = static_cast<int8_t>(Effects::Kind::Haste);
            beacon.secondary = static_cast<int8_t>(Effects::Kind::Regeneration);
            withFrames.beacons.push_back(beacon);

            WorldSave::saveLevel(dir, withFrames);

            LevelState back;
            check(WorldSave::loadLevel(dir, back), "a level with a frame reads back");
            check(back.frames.size() == 1, "the frame is there");
            if (back.frames.size() == 1)
            {
                check(back.frames[0].item == static_cast<uint16_t>(Items::DiamondPickaxe),
                      "holding what it held");
                check(back.frames[0].damage == 44, "worn as it was worn");
                check(back.frames[0].enchantments == 0x15u, "and enchanted as it was");
            }
            check(back.beacons.size() == 1, "the beacon's chosen powers come back");
            if (back.beacons.size() == 1)
            {
                check(back.beacons[0].primary == static_cast<int8_t>(Effects::Kind::Haste), "haste first");
                check(back.beacons[0].secondary == static_cast<int8_t>(Effects::Kind::Regeneration),
                      "and regeneration second");
                check(back.beacons[0].y == 70, "at the beacon's own place");
            }
        }

        // A level written before signs existed has none, and reading
        // one must not leave the loader hunting for them in whatever
        // bytes happen to follow.
        {
            LevelState old;
            old.seed = 12u;
            WorldSave::saveLevel(dir, old);

            LevelState back;
            check(WorldSave::loadLevel(dir, back), "a level with no signs reads back");
            check(back.signs.empty(), "and comes back with none");
            check(back.frames.empty(), "and no frames either");
        }

        {
            LevelState older;
            older.seed = 14u;
            older.inventory.emplace_back(static_cast<uint16_t>(Blocks::Stone), 5);
            older.inventory.emplace_back(static_cast<uint16_t>(1024 + 7), 1);
            LevelState::SavedChest chest;
            chest.slots.emplace_back(static_cast<uint16_t>(1024 + 2), 3);
            older.chests.push_back(chest);
            WorldSave::saveLevel(dir, older);
            const std::string path = dir + "/level.dat";
            std::error_code ec;
            const auto size = std::filesystem::file_size(path, ec);
            if (!ec) std::filesystem::resize_file(path, size - sizeof(uint16_t) - sizeof(uint8_t) - 3 * sizeof(int) - sizeof(uint32_t) - sizeof(int32_t) - 6 * sizeof(uint32_t) -
                                                        sizeof(uint8_t) - 3 * sizeof(float) - sizeof(int8_t) - sizeof(uint32_t) - 10 * sizeof(uint32_t) - sizeof(uint8_t), ec);

            LevelState back;
            check(WorldSave::loadLevel(dir, back), "a level from before items moved up still loads");
            check(back.inventory.size() == 2 && back.inventory[0].first == static_cast<uint16_t>(Blocks::Stone),
                  "its blocks stay where they were");
            check(back.inventory.size() == 2 && back.inventory[1].first == static_cast<uint16_t>(Items::FIRST + 7),
                  "and its items move up with the items");
            check(back.chests.size() == 1 && back.chests[0].slots.size() == 1 &&
                      back.chests[0].slots[0].first == static_cast<uint16_t>(Items::FIRST + 2),
                  "chests included");
        }
    }

    // --- weather ---------------------------------------------------------

    void testWeather()
    {
        section("weather: it turns, and keeps turning");

        // A week of weather, a tenth of a second at a time. Nothing in
        // here is random enough to need a long run; what it is checking
        // is that the two countdowns cannot deadlock -- a spell that
        // ends without starting another leaves the sky stuck for ever,
        // and nothing else in the game would notice.
        Weather::State state;
        uint32_t seed = 4242u;

        int wet = 0;
        int dry = 0;
        int storms = 0;
        int flashes = 0;

        // A second at a time for a week of game time. The fade is
        // checked separately at a tenth of a second; what this run is
        // for is the two countdowns, and they are measured in minutes.
        constexpr float STEP = 1.0f;
        constexpr int STEPS = 7 * 24 * 60 * 60;

        Weather::Kind was = state.kind;
        int changes = 0;

        for (int i = 0; i < STEPS; ++i)
        {
            if (Weather::step(state, STEP, seed)) ++flashes;
            if (state.kind != was) { ++changes; was = state.kind; }

            if (state.raining()) ++wet; else ++dry;
            if (state.storming()) ++storms;
        }

        // Spells average something under an hour and a half, so a week
        // should see dozens of turns. The point is that it never stops:
        // a spell that ends without starting another leaves the sky
        // stuck for good, and nothing else in the game would notice.
        check(changes > 40, "the weather turns over and over (" +
                               std::to_string(changes) + " times in a week)");
        check(wet > 0, "it rains");
        check(dry > 0, "and it stops raining");
        check(storms > 0, "and some of it is thunder");
        check(flashes > 0, "which has lightning in it (" +
                               std::to_string(flashes) + " flashes)");

        // Clear weather is the common case, as it is in Minecraft: a
        // world that is wet more often than dry would be wrong however
        // well each spell behaved.
        check(dry > wet, "and it is dry more often than wet");

        section("weather: the sky follows it");

        // Darkness has to arrive with the cloud rather than the instant
        // the clock turns over.
        Weather::State turning;
        uint32_t turnSeed = 7u;
        Weather::begin(turning, Weather::Kind::Rain, turnSeed);

        check(turning.depth == 0.0f, "the sky is still clear the moment it starts");
        check(Weather::lightScale(turning) == 1.0f, "so nothing is dimmed yet");

        for (int i = 0; i < 100; ++i) Weather::step(turning, 0.1f, turnSeed);
        check(turning.depth > 0.9f, "ten seconds later the cloud is in");
        check(Weather::lightScale(turning) < 0.9f, "and the day is dimmer for it");

        // And it has to lift again when the rain stops.
        Weather::State clearing = turning;
        uint32_t clearSeed = 9u;
        Weather::begin(clearing, Weather::Kind::Clear, clearSeed);
        for (int i = 0; i < 100; ++i) Weather::step(clearing, 0.1f, clearSeed);
        check(clearing.depth < 0.05f, "and lifts again when it stops");
        check(Weather::lightScale(clearing) > 0.98f, "giving the day back");

        // A storm is darker than plain rain, which is what lets things
        // spawn under one at noon.
        Weather::State rain;
        Weather::State storm;
        uint32_t a = 1u, b = 2u;
        Weather::begin(rain, Weather::Kind::Rain, a);
        Weather::begin(storm, Weather::Kind::Storm, b);
        for (int i = 0; i < 100; ++i)
        {
            Weather::step(rain, 0.1f, a);
            Weather::step(storm, 0.1f, b);
        }
        check(Weather::lightScale(storm) < Weather::lightScale(rain),
              "a storm is darker than rain");

        section("weather: lightning");

        // No lightning out of a clear sky, and none out of plain rain.
        Weather::State fine;
        uint32_t fineSeed = 11u;
        Weather::begin(fine, Weather::Kind::Clear, fineSeed);
        bool flashedWhenClear = false;
        for (int i = 0; i < 20000; ++i)
            if (Weather::step(fine, 0.01f, fineSeed) && !fine.storming())
                flashedWhenClear = true;
        check(!flashedWhenClear, "a clear sky has no lightning in it");

        Weather::State drizzle;
        uint32_t drizzleSeed = 13u;
        Weather::begin(drizzle, Weather::Kind::Rain, drizzleSeed);
        bool flashedInRain = false;
        for (int i = 0; i < 2000; ++i)
        {
            // Held as plain rain: the spell is long, so this never
            // rolls over into something else.
            drizzle.kind = Weather::Kind::Rain;
            if (Weather::step(drizzle, 0.05f, drizzleSeed)) flashedInRain = true;
        }
        check(!flashedInRain, "and neither has plain rain");

        // A flash is brief and bright, and fades rather than switching
        // off.
        Weather::State lit;
        uint32_t litSeed = 17u;
        Weather::begin(lit, Weather::Kind::Storm, litSeed);
        for (int i = 0; i < 100; ++i) Weather::step(lit, 0.1f, litSeed);   // let it arrive

        int guard = 0;
        while (!lit.flashing() && guard++ < 100000)
        {
            lit.kind = Weather::Kind::Storm;
            Weather::step(lit, 0.01f, litSeed);
        }
        check(lit.flashing(), "a storm flashes sooner or later");

        const float atStart = Weather::flashBoost(lit);
        check(atStart > 0.0f, "which lights the sky");
        Weather::step(lit, Weather::FLASH_SECONDS * 0.5f, litSeed);
        const float halfway = Weather::flashBoost(lit);
        check(halfway < atStart, "and fades through it");
        Weather::step(lit, Weather::FLASH_SECONDS, litSeed);
        check(Weather::flashBoost(lit) == 0.0f, "and is gone");

        // Nothing moves on a zero-length step, which is what a paused
        // game hands it.
        Weather::State still;
        uint32_t stillSeed = 19u;
        Weather::begin(still, Weather::Kind::Storm, stillSeed);
        const float before = still.secondsLeft;
        check(!Weather::step(still, 0.0f, stillSeed), "a step of no time does nothing");
        check(still.secondsLeft == before, "and moves no clock");
    }

    void testIllagers()
    {
        section("the illagers");

        const MobType& pillager = mobType(MobId::Pillager);
        const MobType& vindicator = mobType(MobId::Vindicator);
        const MobType& evoker = mobType(MobId::Evoker);
        const MobType& ravager = mobType(MobId::Ravager);

        // Three ways of fighting, one each. If two of them fought the
        // same way there would be no reason to have both.
        check(pillager.arrowDamage > 0 && pillager.attackDamage == 0,
              "a pillager shoots and does not swing");
        check(pillager.arrowInterval > mobType(MobId::Skeleton).arrowInterval,
              "and reloads slower than a bow, which is what a crossbow is");
        check(vindicator.attackDamage > 0 && vindicator.arrowDamage == 0,
              "a vindicator swings and does not shoot");
        check(vindicator.attackDamage > mobType(MobId::Zombie).attackDamage,
              "and hits harder than a zombie, because of the axe");
        check(evoker.conjuresFangs && !evoker.throwsPotions, "an evoker conjures fangs rather than closing");
        check(evoker.arrowDamage > 0 && evoker.attackDamage == 0,
              "and keeps its distance");

        // All three are hostile, all three are the villager's size,
        // and none of them burns in the sun -- an illager out in the
        // open at noon is the whole point of an outpost.
        for (const MobType* who : { &pillager, &vindicator, &evoker })
        {
            const std::string name = who->name;
            check(who->spawnClass == SpawnClass::Hostile, name + " is hostile");
            check(!who->burnsInSunlight, name + " does not burn in daylight");
            check(!who->neutral, name + " needs no provoking");
            check(std::fabs(standsAt(*who) - standsAt(mobType(MobId::Villager))) < 0.02f,
                  name + " stands as tall as the villager it used to be");
        }

        // The ravager is the big one: more health than anything short
        // of a boss, and the hardest hand in the Overworld.
        check(ravager.maxHealth > 4 * vindicator.maxHealth, "a ravager takes a lot of killing");
        check(ravager.attackDamage > vindicator.attackDamage, "and hits harder than an axe");
        check(ravager.width > 1.5f, "it is too wide for its own front door");
        check(!ravager.boss, "but it is not a boss");
        check(ravager.walkSpeed < vindicator.walkSpeed, "and it is slower than the men with it");

        section("the totem of undying");

        // Only an evoker has one, and nothing makes one. That is the
        // whole reason a mansion is worth the walk.
        check(evoker.drop == Items::TotemOfUndying, "an evoker leaves a totem");
        bool elsewhere = false;
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobId id = static_cast<MobId>(i);
            if (id == MobId::Evoker) continue;
            const MobType& type = mobType(id);
            if (type.drop == Items::TotemOfUndying ||
                type.secondDrop == Items::TotemOfUndying) elsewhere = true;
        }
        check(!elsewhere, "and nothing else in the game does");

        const std::vector<StackId> made = Crafting::everyOutput();
        check(std::find(made.begin(), made.end(), StackId{ Items::TotemOfUndying }) == made.end(),
              "nor can one be made");
        check(itemInfo(Items::TotemOfUndying).maxStack == 1, "and they do not stack");

        // The blow that would kill you is turned aside, once.
        {
            Player saved(glm::vec3(0.0f, 70.0f, 0.0f));
            saved.health = 4;
            saved.holdingTotem = true;
            check(saved.damage(20), "the blow still lands");
            check(saved.health == 1, "and leaves you on half a heart");
            check(saved.takeTotemSpent(), "the totem was spent");
            check(!saved.takeTotemSpent(), "and only reports itself once");
            check(!saved.holdingTotem, "it is not still in your hand");
            check(saved.effects.has(Effects::Kind::Regeneration),
                  "regeneration carries you out of the room");
        }

        // Without one, the same blow kills.
        {
            Player bare(glm::vec3(0.0f, 70.0f, 0.0f));
            bare.health = 4;
            check(bare.damage(20), "the blow lands");
            check(bare.health == 0, "and that is that");
            check(!bare.takeTotemSpent(), "nothing was spent");
        }

        // It saves you from a fatal blow, not from every blow: a hit
        // you would have walked away from costs nothing.
        {
            Player grazed(glm::vec3(0.0f, 70.0f, 0.0f));
            grazed.health = 20;
            grazed.holdingTotem = true;
            grazed.damage(4);
            check(grazed.health == 16, "a survivable blow is just a blow");
            check(!grazed.takeTotemSpent(), "and the totem stays in your hand");
            check(grazed.holdingTotem, "still holding it");
        }

        // And it works once. The second fatal blow of the same fight
        // is the one that kills.
        {
            Player twice(glm::vec3(0.0f, 70.0f, 0.0f));
            twice.health = 6;
            twice.holdingTotem = true;
            twice.damage(20);
            check(twice.health == 1, "the first one is survived");
            twice.clearHurtCooldown();
            twice.damage(20);
            check(twice.health == 0, "the second one is not");
        }
    }

    void testCaveCrowd()
    {
        section("underground: who lives down there");

        // The list the spawner actually reads, not a copy of it.
        bool hasSlime = false, hasBat = false, hasCaveSpider = false;
        bool hasSilverfish = false, hasZombie = false;
        for (int k = 0; k < EntityManager::caveSpeciesCount(); ++k)
        {
            const MobId id = EntityManager::caveSpecies(k);
            if (id == MobId::Slime) hasSlime = true;
            if (id == MobId::Bat) hasBat = true;
            if (id == MobId::CaveSpider) hasCaveSpider = true;
            if (id == MobId::Silverfish) hasSilverfish = true;
            if (id == MobId::Zombie) hasZombie = true;
        }
        check(hasSlime, "a slime lives underground");
        check(hasBat, "and a bat");
        check(hasCaveSpider, "and a cave spider");
        check(hasSilverfish, "and a silverfish");
        check(hasZombie, "and the things that come out at night, which also live down there");

        // Nothing that belongs to the surface or to another world is
        // on it: a horse in a cave is as wrong as a sheep in one.
        for (int k = 0; k < EntityManager::caveSpeciesCount(); ++k)
        {
            const MobType& type = mobType(EntityManager::caveSpecies(k));
            check(type.dimension == Dimension::Overworld,
                  std::string(type.name) + " belongs to this world");
            check(!type.boss, std::string(type.name) + " is not a boss");
        }

        section("the slime");

        const MobType& slime = mobType(MobId::Slime);
        const MobType& small = mobType(MobId::SlimeSmall);

        // The big one leaves nothing. That is what makes killing it
        // worth doing twice: the slimeballs are on the halves.
        check(slime.drop == Blocks::Air && slime.dropCount == 0,
              "a big slime leaves nothing behind");
        check(small.drop == Items::Slimeball, "a small one leaves a slimeball");
        check(standsAt(slime) > standsAt(small) * 1.8f, "and it really is half the size");
        check(slime.maxHealth > small.maxHealth, "and harder to kill");
        check(slime.attackDamage > small.attackDamage, "and hits harder");
        check(small.walkSpeed > slime.walkSpeed, "but the small one is quicker");

        // Nothing else drops a slimeball, so a slime is the only way
        // to a sticky piston.
        int sources = 0;
        for (int i = 0; i < mobTypeCount(); ++i)
        {
            const MobType& type = mobType(static_cast<MobId>(i));
            if (type.drop == Items::Slimeball || type.secondDrop == Items::Slimeball) ++sources;
        }
        check(sources == 1, "and it is the only thing in the game that leaves one");

        section("what a slimeball is for");

        {
            Inventory bench;
            bench.setCraftSize(3);
            for (int cell = 0; cell < 9; ++cell)
                bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ Items::Slimeball, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Blocks::SlimeBlock,
                  "nine slimeballs make a block");
        }
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST) = ItemStack{ Blocks::SlimeBlock, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Items::Slimeball,
                  "and the block gives the nine back");
            check(bench.slot(Inventory::CRAFT_RESULT).count == 9, "all nine");
        }
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Blocks::PistonNorth, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 1) = ItemStack{ Items::Slimeball, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Blocks::StickyPistonNorth,
                  "a slimeball makes a piston sticky, which is Minecraft's own recipe");
        }
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Items::Slimeball, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 1) = ItemStack{ Items::BlazePowder, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Items::MagmaCream,
                  "and with blaze powder it is magma cream");
        }

        section("the slime block");

        check(blockDrop(Blocks::SlimeBlock) == Blocks::SlimeBlock, "it picks up as itself");
        check(Tools::requiredLevel(Blocks::SlimeBlock) == 0, "it comes apart in your hands");
        check(!blockInfo(Blocks::SlimeBlock).opaque, "light gets through it");
        check(blockInfo(Blocks::SlimeBlock).collides, "but you cannot walk through it");

        // The one thing in the game that takes a fall for you.
        check(Player::breaksFall(Blocks::SlimeBlock), "a slime block breaks a fall");
        check(!Player::breaksFall(Blocks::Stone), "stone does not");
        check(!Player::breaksFall(Blocks::Wool), "nor does wool, however soft it looks");
        check(!Player::breaksFall(Blocks::Air), "and nor does nothing at all");

        section("the bat, the cave spider and the silverfish");

        const MobType& bat = mobType(MobId::Bat);
        check(bat.spawnClass == SpawnClass::Passive, "a bat attacks nobody");
        check(bat.attackDamage == 0, "it has nothing to attack with");
        check(bat.experience == 0, "and killing one is worth nothing");
        check(bat.hoverHeight > 0.0f, "it flies rather than walks");
        check(bat.drop == Blocks::Air, "and leaves nothing");

        const MobType& caveSpider = mobType(MobId::CaveSpider);
        const MobType& spider = mobType(MobId::Spider);
        check(spans(caveSpider) < spans(spider), "a cave spider is smaller than a spider");
        check(spans(caveSpider) < spans(spider),
              "narrow enough for a gap a spider could not take");
        check(caveSpider.walkSpeed > spider.walkSpeed, "and quicker");
        check(caveSpider.maxHealth < spider.maxHealth, "and easier to kill");
        check(caveSpider.drop == Items::StringItem, "it leaves string, like its cousin");

        const MobType& silverfish = mobType(MobId::Silverfish);
        check(standsAt(silverfish) < 0.5f, "a silverfish is barely there");
        check(silverfish.attackDamage > 0, "but it bites");
        check(silverfish.drop == Blocks::Air, "and leaves nothing worth having");
    }

    void testNetherCrowd()
    {
        section("the Nether: the piglins and what lives with them");

        const MobType& piglin = mobType(MobId::Piglin);
        const MobType& brute = mobType(MobId::PiglinBrute);
        const MobType& hoglin = mobType(MobId::Hoglin);
        const MobType& strider = mobType(MobId::Strider);

        for (const MobType* who : { &piglin, &brute, &hoglin, &strider })
            check(who->dimension == Dimension::Nether,
                  std::string(who->name) + " belongs to the Nether");

        // A piglin minds its own business until you give it a reason
        // not to; a brute never did and never will.
        check(piglin.neutral, "a piglin is neutral");
        check(!brute.neutral, "a brute is not");
        check(brute.maxHealth > 2 * piglin.maxHealth, "and it takes far more killing");
        check(brute.attackDamage > piglin.attackDamage, "and hits harder");
        check(brute.experience > piglin.experience, "and is worth more");

        // Gold. Nine nuggets to an ingot, which is the only thing in
        // the game that turns what a mob leaves into a metal.
        check(piglin.drop == Items::GoldNugget, "a piglin leaves gold");
        check(brute.drop == Items::GoldNugget, "and so does a brute");
        check(brute.dropCount > piglin.dropCount, "the brute carries more of it");
        {
            Inventory bench;
            bench.setCraftSize(3);
            for (int cell = 0; cell < 9; ++cell)
                bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ Items::GoldNugget, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Items::GoldIngot,
                  "nine nuggets make an ingot");
        }
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST) = ItemStack{ Items::GoldIngot, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Items::GoldNugget,
                  "and the ingot gives the nine back");
            check(bench.slot(Inventory::CRAFT_RESULT).count == 9, "all nine");
        }

        // The hoglin is the Nether's livestock, in the sense that it
        // is made of meat and would rather you were.
        check(hoglin.drop == Items::RawPorkchop, "a hoglin is made of pork");
        check(hoglin.dropCount >= 2, "enough of it to be worth hunting");
        check(hoglin.spawnClass == SpawnClass::Hostile, "and it comes for you");
        check(hoglin.width > 1.0f, "it is a big animal");

        section("the strider");

        // The one way across a lava sea, and the only passive thing
        // down there.
        check(strider.lavaWalker, "a strider walks on lava");
        check(strider.fireproof, "which would be a short walk otherwise");
        check(strider.spawnClass == SpawnClass::Passive, "it attacks nobody");
        check(strider.rideable, "and you can get on it");
        check(strider.saddleHeight > 1.0f, "sitting up where you can see");

        // Nothing else does. A world where everything walked on lava
        // would be a world where lava was a floor.
        int walkers = 0;
        for (int i = 0; i < mobTypeCount(); ++i)
            if (mobType(static_cast<MobId>(i)).lavaWalker) ++walkers;
        check(walkers == 1, "and it is the only thing in the game that does");

        // The rule the spawn asks, rather than the flag behind it.
        check(EntityManager::roomFor(strider, Blocks::Lava),
              "lava is somewhere a strider can be put");
        check(!EntityManager::roomFor(mobType(MobId::Piglin), Blocks::Lava),
              "and nowhere a piglin can");
        check(!EntityManager::roomFor(strider, Blocks::Netherrack),
              "rock is still rock to it");
    }

    void testTheSea()
    {
        section("the sea: what lives in it");

        // Everything on the list lives in water and can be put in it.
        // The squid was the first and was alone in it for a long time.
        bool hasSquid = false;
        for (int k = 0; k < EntityManager::seaSpeciesCount(); ++k)
        {
            const MobId id = EntityManager::seaSpecies(k);
            const MobType& type = mobType(id);
            const std::string who = type.name;

            if (id == MobId::Squid) hasSquid = true;
            check(type.aquatic, who + " lives in the water");
            check(EntityManager::roomFor(type, Blocks::Water),
                  who + " can be put in water");
            check(!EntityManager::roomFor(type, Blocks::Lava), who + " cannot be put in lava");
            check(type.dimension == Dimension::Overworld, who + " belongs to this world");
            check(type.attackDamage == 0, who + " attacks nobody");
        }
        check(hasSquid, "the squid is still down there");
        check(EntityManager::seaSpeciesCount() >= 7, "and it is not alone any more");

        section("the fish");

        const MobType& cod = mobType(MobId::Cod);
        const MobType& salmon = mobType(MobId::Salmon);
        const MobType& puffer = mobType(MobId::Pufferfish);
        const MobType& dolphin = mobType(MobId::Dolphin);
        const MobType& turtle = mobType(MobId::Turtle);
        const MobType& axolotl = mobType(MobId::Axolotl);

        check(cod.drop == Items::RawCod, "a cod is a cod");
        check(salmon.drop == Items::RawSalmon, "and a salmon is a salmon");
        check(puffer.drop == Items::Pufferfish, "and a pufferfish is a mistake");
        check(salmon.width > cod.width, "a salmon is the bigger fish");
        check(dolphin.walkSpeed > cod.walkSpeed, "a dolphin is quicker than anything it eats");
        check(turtle.maxHealth > 5 * cod.maxHealth, "a turtle takes some killing");
        check(turtle.walkSpeed < cod.walkSpeed, "and is in no hurry");
        check(axolotl.breedingFood == Items::RawCod, "an axolotl follows a fish");
        check(turtle.breedingFood != Blocks::Air, "and a turtle follows something too");

        // A turtle walks the bottom; the fish hang in the water.
        check(turtle.hoverHeight == 0.0f, "a turtle walks on the bottom");
        for (const MobType* who : { &cod, &salmon, &puffer, &dolphin, &axolotl })
            check(who->hoverHeight > 0.0f,
                  std::string(who->name) + " hangs in the water rather than sinking");

        section("fish as food");

        check(Food::isEdible(Items::RawCod), "a raw cod is food");
        check(Food::isEdible(Items::CookedCod), "and a cooked one is better");
        check(Food::valueOf(Items::CookedCod).hunger > Food::valueOf(Items::RawCod).hunger,
              "cooking a cod is worth the furnace");
        check(Food::valueOf(Items::CookedSalmon).hunger >
                  Food::valueOf(Items::CookedCod).hunger,
              "and a salmon is worth more than a cod");

        check(Smelting::resultOf(Items::RawCod) == Items::CookedCod, "a cod cooks");
        check(Smelting::resultOf(Items::RawSalmon) == Items::CookedSalmon, "and so does a salmon");
        check(Smelting::suits(Smelting::Kind::Smoker, Items::RawCod),
              "and a smoker will do it, because a smoker cooks what you eat");

        // The one food in the game that is a mistake.
        check(Food::isEdible(Items::Pufferfish), "a pufferfish can be eaten");
        check(Food::poisons(Items::Pufferfish), "and it will make you ill");
        check(!Food::poisons(Items::CookedCod), "a cooked cod will not");
        check(!Food::poisons(Items::Bread), "nor will bread");
        check(Smelting::resultOf(Items::Pufferfish) == Blocks::Air,
              "and cooking does not save it");

        int poisonous = 0;
        for (int id = Items::FIRST; id < Items::Count; ++id)
            if (Food::poisons(static_cast<StackId>(id))) ++poisonous;
        check(poisonous == 1, "it is the only food in the game that is worse than nothing");
    }

    void testCountryAnimals()
    {
        section("the country: what lives where");

        auto spawns = [](Biomes::Kind where, Biomes::Group group, MobId who) {
            for (int step = 0; step < 400; ++step)
                if (EntityManager::pickSpawn(where, group, static_cast<float>(step) / 400.0f) == who) return true;
            return false;
        };
        const Biomes::Group day = Biomes::Group::Creature;
        check(spawns(Biomes::Kind::Plains, day, MobId::Sheep) && spawns(Biomes::Kind::Plains, day, MobId::Cow) &&
              spawns(Biomes::Kind::Plains, day, MobId::Pig) && spawns(Biomes::Kind::Plains, day, MobId::Chicken),
              "plains have the four farm animals, as the 1.21.1 data says");
        struct Local { Biomes::Kind where; MobId who; const char* what; };
        const Local LOCALS[] = {
            { Biomes::Kind::Forest,       MobId::Wolf,      "wolves keep to the trees" },
            { Biomes::Kind::Taiga,        MobId::Fox,       "foxes to the taiga" },
            { Biomes::Kind::Plains,       MobId::Horse,     "horses keep to open country" },
            { Biomes::Kind::Desert,       MobId::Rabbit,    "a rabbit is what a desert has" },
            { Biomes::Kind::SnowyPlains,  MobId::PolarBear, "a polar bear belongs in the snow" },
            { Biomes::Kind::SnowySlopes,  MobId::Goat,      "a goat wants somewhere steep" },
            { Biomes::Kind::WindsweptHills, MobId::Llama,   "and a llama is happy up there too" },
            { Biomes::Kind::SavannaPlateau, MobId::Llama,   "a llama belongs on a savanna plateau" },
            { Biomes::Kind::BambooJungle, MobId::Panda,     "a panda belongs in a bamboo jungle" },
            { Biomes::Kind::Jungle,       MobId::Parrot,    "and a parrot in the jungle" },
            { Biomes::Kind::Swamp,        MobId::Frog,      "frogs in the swamp" },
        };
        for (const Local& local : LOCALS) check(spawns(local.where, day, local.who), local.what);

        check(!spawns(Biomes::Kind::Jungle, day, MobId::PolarBear), "a polar bear is not a jungle animal");
        check(!spawns(Biomes::Kind::Desert, day, MobId::Panda), "nor is a panda a desert one");
        check(EntityManager::pickSpawn(Biomes::Kind::Ocean, day, 0.5f) == MobId::Count &&
                  EntityManager::pickSpawn(Biomes::Kind::MushroomFields, Biomes::Group::Monster, 0.5f) == MobId::Count,
              "the sea has no land animals and mushroom fields no monsters");

        section("the country animals themselves");

        const MobType& rabbit = mobType(MobId::Rabbit);
        const MobType& fox = mobType(MobId::Fox);
        const MobType& goat = mobType(MobId::Goat);
        const MobType& panda = mobType(MobId::Panda);
        const MobType& bee = mobType(MobId::Bee);
        const MobType& cat = mobType(MobId::Cat);
        const MobType& llama = mobType(MobId::Llama);
        const MobType& bear = mobType(MobId::PolarBear);

        // Everything out here has something that tempts it, or it is
        // an animal you can never do anything with.
        for (const MobType* who : { &rabbit, &fox, &goat, &panda, &cat, &llama })
            check(who->breedingFood != Blocks::Air,
                  std::string(who->name) + " can be led somewhere");

        // The two that fight back if you start it, and nothing else.
        check(bee.neutral, "a bee stings only if you hit it");
        check(bear.neutral, "and a polar bear is the same");
        check(!rabbit.neutral && rabbit.attackDamage == 0, "a rabbit does nothing at all");
        check(!fox.neutral && fox.attackDamage == 0, "nor does a fox");

        check(bee.hoverHeight > 0.0f, "a bee flies");
        check(bee.drop == Items::Honeycomb, "and the wax it leaves is what a candle is");
        check(rabbit.drop == Items::RawRabbit, "a rabbit is food");
        check(goat.drop == Items::RawMutton, "and a goat is mutton, near enough");

        check(bear.maxHealth > 2 * fox.maxHealth, "a polar bear takes some killing");
        check(bear.attackDamage >= 5, "and hits like the thing it is");
        check(rabbit.walkSpeed > goat.walkSpeed, "a rabbit is quicker than a goat");
        check(llama.height > goat.height, "a llama stands taller than a goat");
        check(panda.maxHealth > rabbit.maxHealth, "a panda is a bigger animal than a rabbit");
        check(cat.breedingFood == Items::RawCod, "a cat follows a fish");

        section("rabbit and honeycomb");

        check(Food::isEdible(Items::RawRabbit), "raw rabbit is food");
        check(Smelting::resultOf(Items::RawRabbit) == Items::CookedRabbit, "and it cooks");
        check(Food::valueOf(Items::CookedRabbit).hunger > Food::valueOf(Items::RawRabbit).hunger,
              "which is worth doing");

        // Minecraft's candle, now that there are bees to take the wax
        // off: it stood on wool before, for want of one.
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Items::StringItem, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 3) = ItemStack{ Items::Honeycomb, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Blocks::Candle,
                  "a wick in wax makes a candle");
        }
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Items::StringItem, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 3) = ItemStack{ Blocks::Wool, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).empty(),
                  "and wool no longer will, now that wax exists");
        }
    }

    void testNightVariants()
    {
        section("the night: what a particular place has");

        auto night = [](Biomes::Kind where, MobId who) {
            for (int step = 0; step < 400; ++step)
                if (EntityManager::pickSpawn(where, Biomes::Group::Monster, static_cast<float>(step) / 400.0f) == who) return true;
            return false;
        };
        struct Local { MobId who; Biome home; Biome alsoHome; const char* what; };
        const Local LOCALS[] = {
            { MobId::Stray,  Biome::Snowy,  Biome::Snowy,         "a stray belongs to the snow" },
            { MobId::Husk,   Biome::Desert, Biome::Desert,        "a husk to the desert" },
            { MobId::Bogged, Biome::Swamp,  Biome::MangroveSwamp, "and a bogged to the swamps" },
        };
        for (const Local& local : LOCALS)
        {
            bool here = false, elsewhere = false;
            for (int biome = 0; biome < Biomes::COUNT; ++biome)
            {
                const Biomes::Kind kind = static_cast<Biomes::Kind>(biome);
                if (Biomes::dimension(kind) != Dimension::Overworld || !night(kind, local.who)) continue;
                const Biome family = Biomes::family(kind);
                if (family == local.home || family == local.alsoHome) here = true;
                else elsewhere = true;
            }
            check(here, local.what);
            check(!elsewhere, std::string(mobType(local.who).name) + " is nowhere else after dark");
        }
        check(night(Biomes::Kind::SnowyPlains, MobId::Zombie) && night(Biomes::Kind::Desert, MobId::Zombie) &&
              night(Biomes::Kind::Swamp, MobId::Skeleton), "and the ordinary night still happens beside them");

        section("the three variants themselves");

        const MobType& stray = mobType(MobId::Stray);
        const MobType& husk = mobType(MobId::Husk);
        const MobType& bogged = mobType(MobId::Bogged);
        const MobType& skeleton = mobType(MobId::Skeleton);
        const MobType& zombie = mobType(MobId::Zombie);

        // A stray is a skeleton with the cold in it.
        check(stray.arrowDamage > skeleton.arrowDamage, "a stray shoots harder");
        check(stray.drop == Items::Bone, "and still leaves bones");
        check(stray.burnsInSunlight, "and still burns at dawn");

        // A husk is a zombie that does not.
        check(!husk.burnsInSunlight, "a husk does not burn in daylight");
        check(zombie.burnsInSunlight, "where a zombie does");
        check(husk.attackDamage > zombie.attackDamage, "and it hits harder");

        // A bogged is softer and faster with the bow, which is 1.21's.
        check(bogged.maxHealth < skeleton.maxHealth, "a bogged is softer than a skeleton");
        check(bogged.arrowInterval < skeleton.arrowInterval, "and shoots more often");

        section("the phantom, the endermite and the vex");

        const MobType& phantom = mobType(MobId::Phantom);
        check(phantom.hoverHeight > 3.0f, "a phantom comes out of the sky");
        check(phantom.walkSpeed > zombie.walkSpeed, "and it is faster than anything on legs");
        check(phantom.attackDamage > 0, "and it means it");

        const MobType& endermite = mobType(MobId::Endermite);
        check(standsAt(endermite) < 0.5f, "an endermite is barely there");
        bool inEnd = false;
        for (int k = 0; k < EntityManager::endSpeciesCount(); ++k)
            if (EntityManager::endSpecies(k) == MobId::Endermite) inEnd = true;
        check(inEnd, "and the End has them in its walls");

        const MobType& vex = mobType(MobId::Vex);
        const MobType& evoker = mobType(MobId::Evoker);
        check(vex.hoverHeight > 0.0f, "a vex flies");
        check(vex.walkSpeed > evoker.walkSpeed * 2.0f, "and it is far faster than its master");
        check(evoker.arrowDamage > vex.attackDamage, "and his fangs bite harder than it does");
        check(vex.maxHealth < evoker.maxHealth, "but there is less of it");

        // Every one of the seven has three voices of its own: this is
        // the check that catches a block pasted from the one above it.
        const MobId NEW_TONIGHT[] = { MobId::Stray, MobId::Husk, MobId::Bogged,
                                      MobId::Phantom, MobId::ZombieVillager,
                                      MobId::Endermite, MobId::Vex };
        std::set<int> voices;
        for (MobId id : NEW_TONIGHT)
        {
            const MobType& type = mobType(id);
            check(voices.insert(static_cast<int>(type.voice)).second,
                  std::string(type.name) + " has a voice nobody else has");
            check(type.spawnClass == SpawnClass::Hostile,
                  std::string(type.name) + " is hostile");
        }
    }

    void testGolemsAndTheLast()
    {
        section("building a golem");

        // Two snow blocks under a pumpkin, which is Minecraft's.
        {
            Golems::Shape snow;
            snow.upper = Blocks::Snow;
            snow.lower = Blocks::Snow;
            check(Golems::madeBy(snow) == MobId::SnowGolem, "two snow blocks make a snow golem");
        }

        // Iron wants the arms as well. Without that rule a tower of
        // iron blocks with a pumpkin on it would be a golem by
        // accident, which is exactly what Minecraft's rule prevents.
        {
            Golems::Shape tower;
            tower.upper = Blocks::IronBlock;
            tower.lower = Blocks::IronBlock;
            check(Golems::madeBy(tower) == MobId::Count, "a tower of iron is not a golem");

            tower.armA = Blocks::IronBlock;
            check(Golems::madeBy(tower) == MobId::Count, "nor is one with a single arm");

            tower.armB = Blocks::IronBlock;
            check(Golems::madeBy(tower) == MobId::IronGolem, "the cross is what makes one");
        }

        // And nothing else does.
        {
            Golems::Shape wrong;
            wrong.upper = Blocks::Dirt;
            wrong.lower = Blocks::Dirt;
            check(Golems::madeBy(wrong) == MobId::Count, "dirt makes nothing");

            wrong.upper = Blocks::Snow;
            check(Golems::madeBy(wrong) == MobId::Count, "one snow block is not enough");

            wrong.upper = Blocks::IronBlock;
            wrong.lower = Blocks::Snow;
            wrong.armA = Blocks::IronBlock;
            wrong.armB = Blocks::IronBlock;
            check(Golems::madeBy(wrong) == MobId::Count, "and the two do not mix");
        }

        check(Golems::isTrigger(Blocks::Pumpkin), "a pumpkin is what finishes one");
        check(!Golems::isTrigger(Blocks::Snow), "a snow block on its own is not");
        check(!Golems::isTrigger(Blocks::IronBlock), "nor is an iron one");

        section("the snow golem");

        const MobType& snowman = mobType(MobId::SnowGolem);
        check(snowman.arrowDamage > 0, "a snow golem throws something");
        check(snowman.arrowDamage <= 2, "and what it throws barely hurts, which is the point");
        check(snowman.arrowInterval < mobType(MobId::Skeleton).arrowInterval,
              "it throws faster than a skeleton shoots");
        check(snowman.maxHealth <= 6, "it comes apart easily");
        check(snowman.drop == Items::Snowball, "and leaves the snow it was made of");

        // Snowballs: off a snow block and back into one.
        check(blockDrop(Blocks::Snow) == Items::Snowball, "a snow block comes apart into snowballs");
        {
            Inventory bench;
            for (int cell : { 0, 1, 3, 4 })
                bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ Items::Snowball, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Blocks::Snow,
                  "and four of them pack back into one");
        }

        section("the elder guardian, the trader, the camel, the parrot and the allay");

        const MobType& elder = mobType(MobId::ElderGuardian);
        const MobType& guardian = mobType(MobId::Guardian);
        check(elder.maxHealth > 2 * guardian.maxHealth, "an elder guardian is far harder to kill");
        check(elder.arrowDamage > guardian.arrowDamage, "and its beam hurts more");
        check(elder.aquatic, "it lives in the water like its smaller cousin");
        check(standsAt(elder) > standsAt(guardian), "and it is drawn bigger");
        check(elder.secondDrop == Blocks::WetSponge, "it leaves a wet sponge");

        const MobType& trader = mobType(MobId::WanderingTrader);
        check(trader.spawnClass == SpawnClass::Passive, "a wandering trader fights nobody");
        check(trader.experience == 0, "and killing one is worth nothing, like a villager");
        check(std::fabs(standsAt(trader) - standsAt(mobType(MobId::Villager))) < 0.02f,
              "he is a villager's size");

        const MobType& camel = mobType(MobId::Camel);
        check(camel.rideable, "a camel can be ridden");
        check(camel.saddleHeight > mobType(MobId::Horse).saddleHeight,
              "and you sit higher on one than on a horse");
        check(camel.height > mobType(MobId::Horse).height, "because it is taller");
        check(camel.breedingFood == Blocks::Cactus, "it follows a cactus");

        const MobType& parrot = mobType(MobId::Parrot);
        check(parrot.hoverHeight > 0.0f, "a parrot flies");
        check(parrot.drop == Items::Feather, "and leaves a feather");

        const MobType& allay = mobType(MobId::Allay);
        check(allay.hoverHeight > 0.0f, "an allay flies too");
        check(allay.attackDamage == 0, "it fights nobody");
        check(allay.experience == 0, "and is worth nothing to kill");
        check(allay.width < 0.5f, "it is the smallest thing in the game");
    }

    void testTheEnd()
    {
        section("the End: the way it is laid out");

        // Minecraft's own numbers. The island you arrive on is about a
        // hundred across the radius; then a thousand blocks of
        // nothing; then the outer islands.
        check(WorldGen::END_MAIN_RADIUS >= 90.0f && WorldGen::END_MAIN_RADIUS <= 110.0f,
              "the island you arrive on is about a hundred across the radius");
        check(WorldGen::END_OUTER_VOID >= 1000.0f,
              "and a thousand blocks of nothing lie past it");
        check(WorldGen::END_CITY_VOID >= static_cast<long long>(WorldGen::END_OUTER_VOID),
              "no city stands on the island you arrive on");

        section("the End: the ring of gateways");

        // Twelve, evenly round the island, all at the same distance.
        std::set<std::pair<int, int>> where;
        for (int i = 0; i < WorldGen::END_GATES; ++i)
        {
            int gx = 0, gz = 0;
            WorldGen::endGateway(i, gx, gz);

            const float away = std::sqrt(static_cast<float>(gx * gx + gz * gz));
            check(std::fabs(away - WorldGen::END_GATE_RING) < 2.0f,
                  "gateway " + std::to_string(i) + " stands on the ring");
            check(where.insert({ gx, gz }).second,
                  "and nowhere another one already stands");

            // Clear of the pillars, or a gateway would be inside one.
            check(away > WorldGen::END_PILLAR_RING + 20,
                  "and well outside the ring of pillars");
        }

        // Each one puts you down along its own bearing, out past the
        // void, which is what makes the ring legible rather than a
        // dozen identical holes.
        for (int i = 0; i < WorldGen::END_GATES; ++i)
        {
            int gx = 0, gz = 0, ox = 0, oz = 0;
            WorldGen::endGateway(i, gx, gz);
            WorldGen::endGatewayExit(i, ox, oz);

            const float out = std::sqrt(static_cast<float>(ox * ox + oz * oz));
            check(out > WorldGen::END_OUTER_VOID,
                  "gateway " + std::to_string(i) + " lands past the void");

            // The same bearing. Asked as the sine of the angle between
            // them rather than as the cross product itself: both ends
            // are whole blocks, and half a block of rounding at
            // ninety-six becomes five at a thousand.
            const float ring = std::sqrt(static_cast<float>(gx * gx + gz * gz));
            const float cross = static_cast<float>(gx) * oz - static_cast<float>(gz) * ox;
            const float dot = static_cast<float>(gx) * ox + static_cast<float>(gz) * oz;
            check(std::fabs(cross) / (ring * out) < 0.02f, "on its own bearing");
            check(dot > 0.0f, "and the same way out, not the opposite side");
        }

        check(!blockInfo(Blocks::EndGateway).collides, "you walk into a gateway");
        check(blockInfo(Blocks::EndGateway).lightEmission > 0,
              "and it is its own light, so the ring can be seen from the island");
    }

    void testBeaconPowers()
    {
        section("beacon powers, haste and resistance");

        const int speed = static_cast<int>(Effects::Kind::Speed);
        const int strength = static_cast<int>(Effects::Kind::Strength);
        const int regen = static_cast<int>(Effects::Kind::Regeneration);
        check(Beacon::isPrimary(speed, 1), "one layer offers speed");
        check(!Beacon::isPrimary(strength, 2), "but strength wants three");
        check(Beacon::isPrimary(strength, 3), "and has it at three");
        check(!Beacon::isPrimary(regen, 4), "regeneration is never a primary");
        check(!Beacon::isSecondary(regen, speed, 3), "nothing second below four layers");
        check(Beacon::isSecondary(regen, speed, 4), "regeneration second at four");
        check(Beacon::isSecondary(speed, speed, 4), "or the primary again, which makes it II");
        check(!Beacon::isSecondary(strength, speed, 4), "and nothing else");
        check(Beacon::seconds(4) == 17.0f, "a full pyramid lasts seventeen seconds a sweep");

        Effects::Set set;
        check(Effects::digMultiplier(set) == 1.0f, "no haste, no change");
        Effects::give(set, Effects::Kind::Haste, 1, 10.0f);
        check(Effects::digMultiplier(set) > 1.39f && Effects::digMultiplier(set) < 1.41f,
              "haste II digs forty per cent faster");
        Effects::give(set, Effects::Kind::Resistance, 0, 10.0f);
        check(Effects::resisted(set, 10) == 8, "resistance I takes a fifth off a blow");

        Player tough(glm::vec3(0.0f, 70.0f, 0.0f));
        tough.health = Player::MAX_HEALTH;
        Effects::give(tough.effects, Effects::Kind::Resistance, 4, 10.0f);
        check(!tough.damage(6), "resistance V shrugs off a blow");
        check(tough.health == Player::MAX_HEALTH, "entirely");

        check(Inventory::belongsIn(Inventory::BEACON_PAYMENT, Items::Diamond), "a diamond pays a beacon");
        check(!Inventory::belongsIn(Inventory::BEACON_PAYMENT, Items::Stick), "a stick does not");
    }

    void testGrindstone()
    {
        section("the grindstone repairs and disenchants");

        Anvil::Input sword;
        sword.id = Items::DiamondSword;
        sword.count = 1;
        sword.damage = 900;
        sword.enchantments = Enchanting::withLevel(0, Enchanting::Kind::Sharpness, 4);
        Anvil::Input worn = sword;
        worn.enchantments = 0;
        worn.damage = 1200;
        Anvil::Input none;

        const Grindstone::Result alone = Grindstone::combine(sword, none);
        check(alone.valid, "one enchanted sword grinds on its own");
        check(alone.enchantments == 0, "and comes off bare");
        check(alone.damage == 900, "no more worn than it went in");
        check(!Grindstone::combine(worn, none).valid, "a plain one alone gives nothing");

        const int most = Anvil::maxDurability(Items::DiamondSword);
        const Grindstone::Result both = Grindstone::combine(sword, worn);
        check(both.valid, "two of the same sword grind together");
        check(both.damage == std::max(0, most - ((most - 900) + (most - 1200) + most * 5 / 100)),
              "their wear added up with five per cent on top");
        check(both.enchantments == 0, "and nothing enchanted survives");

        Anvil::Input pick = worn;
        pick.id = Items::IronPickaxe;
        check(!Grindstone::combine(sword, pick).valid, "a sword and a pickaxe do not mix");

        check(Grindstone::powerOf(sword.enchantments) == 34, "sharpness IV is worth thirty-four");
        const int low = Grindstone::experienceFor(34, 0u);
        const int high = Grindstone::experienceFor(34, 16u);
        check(low == 17 && high == 33, "which pays seventeen to thirty-three points");
        check(Grindstone::experienceFor(0, 5u) == 0, "nothing for nothing");
    }

    void testWoodSets()
    {
        section("the woods: doors, trapdoors, signs, saplings, stripped logs");

        for (int wood = 0; wood < WOOD_COUNT; ++wood)
        {
            const BlockId base = doorBaseOf(wood);
            const BlockId open = doorPart(base, 2, true, true);
            check(isDoor(open) && doorWood(open) == wood, "a door knows its wood");
            check(doorIsTop(open) && doorIsOpen(open) && blockFacing(open) == 2, "and its half, state and facing");
            const BlockId trap = trapdoorPart(trapdoorBaseOf(wood), 1, true, false);
            check(isTrapdoor(trap) && trapdoorBase(trap) == trapdoorBaseOf(wood), "a trapdoor knows its wood");
            check(trapdoorIsTop(trap) && !trapdoorIsOpen(trap) && trapdoorFacing(trap) == 1, "and its state");
            const BlockId sign = signFacingAs(signBaseOf(wood), 3);
            check(isSign(sign) && signWood(sign) == wood && signFacing(sign) == 3, "a sign knows its wood");
            check(blockDrop(sign) == signItemOfWood(wood), "and drops its own sign");
            check(blockDrop(open) == base, "a door drops a whole door");
            check(planksFromLog(strippedOf(logOfWood(wood))) == planksOfWood(wood), "stripped logs make their own planks");
        }
        check(planksFromLog(Blocks::BirchLog) == Blocks::BirchPlanks, "birch has planks of its own now");

        ItemStack grid[9];
        for (int i = 0; i < 6; ++i) grid[i] = ItemStack{ Blocks::SprucePlanks, 1 };
        const CraftOutput trap = Crafting::match(grid, 3);
        check(trap.id == trapdoorBaseOf(1), "six spruce planks in two rows make spruce trapdoors");
        ItemStack doors[9];
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 2; ++col) doors[row * 3 + col] = ItemStack{ Blocks::JunglePlanks, 1 };
        check(Crafting::match(doors, 3).id == doorBaseOf(3), "and jungle planks make jungle doors");

        struct Plot : Saplings::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> cells;
            BlockId at(int x, int y, int z) const override
            {
                const auto found = cells.find({ x, y, z });
                if (found != cells.end()) return found->second;
                return y < 64 ? Blocks::Grass : Blocks::Air;
            }
            void set(int x, int y, int z, BlockId id) override { cells[{ x, y, z }] = id; }
        };

        Plot oak;
        oak.set(0, 64, 0, Blocks::OakSapling);
        check(Saplings::grow(oak, glm::ivec3(0, 64, 0), 7u), "an oak sapling grows");
        check(oak.at(0, 64, 0) == Blocks::Log && oak.at(0, 66, 0) == Blocks::Log, "into an oak trunk");

        Plot lone;
        lone.set(0, 64, 0, Blocks::DarkOakSapling);
        check(!Saplings::grow(lone, glm::ivec3(0, 64, 0), 7u), "one dark oak sapling never grows");
        check(lone.at(0, 64, 0) == Blocks::DarkOakSapling, "and is still there");

        Plot four;
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz) four.set(dx, 64, dz, Blocks::DarkOakSapling);
        check(Saplings::grow(four, glm::ivec3(1, 64, 1), 9u), "four in a square grow");
        check(four.at(0, 64, 0) == Blocks::DarkOakLog && four.at(1, 65, 1) == Blocks::DarkOakLog,
              "into a trunk two wide");

        Plot jungle;
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz) jungle.set(dx, 64, dz, Blocks::JungleSapling);
        check(Saplings::grow(jungle, glm::ivec3(0, 64, 0), 11u), "four jungle saplings grow");
        check(jungle.at(1, 74, 0) == Blocks::JungleLog, "into a giant at least ten high");

        {
            const WorldGen generator(4242u);
            int groves = 0;
            for (int x = -6000; x <= 6000 && groves == 0; x += 48)
                for (int z = -6000; z <= 6000 && groves == 0; z += 48)
                    if (generator.biomeAt(x, z) == Biome::CherryGrove) ++groves;
            check(groves > 0, "cherry groves grow somewhere in a world");
            check(std::string(biomeName(Biome::CherryGrove)) == "Cherry Grove", "and are called so");
        }

        check(!isSapling(Blocks::Air) && woodOfLeaves(Blocks::Air) < 0, "air is no wood's sapling or leaves");
        check(planksFromLog(Blocks::BambooBlock) == Blocks::BambooPlanks, "a block of bamboo makes bamboo planks");
        {
            ItemStack slabs[9];
            for (int i = 0; i < 3; ++i) slabs[i] = ItemStack{ Blocks::BambooPlanks, 1 };
            check(Crafting::match(slabs, 3).id == Blocks::BambooSlab, "bamboo planks make bamboo slabs");
            ItemStack mosaic[9];
            mosaic[0] = ItemStack{ Blocks::BambooSlab, 1 };
            mosaic[3] = ItemStack{ Blocks::BambooSlab, 1 };
            check(Crafting::match(mosaic, 3).id == Blocks::BambooMosaic, "and two of those a mosaic");
        }

        Plot mangrove;
        mangrove.set(0, 64, 0, Blocks::MangrovePropagule);
        check(Saplings::grow(mangrove, glm::ivec3(0, 64, 0), 3u), "a propagule grows");
        check(mangrove.at(0, 64, 0) == Blocks::MangroveRoots && mangrove.at(0, 66, 0) == Blocks::MangroveLog,
              "into a mangrove standing on its roots");
        check(blockDrop(Blocks::MangrovePropaguleHanging) == Blocks::MangrovePropagule,
              "and a hanging propagule comes off as one to plant");
        {
            const WorldGen generator(4242u);
            int mangroves = 0;
            for (int x = -8000; x <= 8000 && mangroves == 0; x += 32)
                for (int z = -8000; z <= 8000 && mangroves == 0; z += 32)
                    if (generator.biomeAt(x, z) == Biome::MangroveSwamp) ++mangroves;
            check(mangroves > 0, "mangrove swamps grow somewhere in a world");
        }

        Plot crimson;
        crimson.cells[{ 0, 63, 0 }] = Blocks::CrimsonNylium;
        crimson.set(0, 64, 0, Blocks::CrimsonFungus);
        check(Saplings::grow(crimson, glm::ivec3(0, 64, 0), 21u), "a crimson fungus on crimson nylium grows");
        check(crimson.at(0, 64, 0) == Blocks::CrimsonStem, "into a huge fungus on a crimson stem");
        int wartBlocks = 0;
        for (const auto& [where, id] : crimson.cells)
            if (id == Blocks::NetherWartBlock || id == Blocks::Shroomlight) ++wartBlocks;
        check(wartBlocks > 10, "under a hat of wart");
        Plot wrongGround;
        wrongGround.cells[{ 0, 63, 0 }] = Blocks::CrimsonNylium;
        wrongGround.set(0, 64, 0, Blocks::WarpedFungus);
        check(!Saplings::grow(wrongGround, glm::ivec3(0, 64, 0), 21u), "a warped fungus will not grow on crimson");
        check(blockDrop(Blocks::WarpedNylium) == Blocks::Netherrack, "nylium breaks back to netherrack");
        check(isClimbable(Blocks::WeepingVinesPlant), "weeping vines can be climbed");
        check(lightEmission(Blocks::Shroomlight) == 15, "and shroomlight lights a forest");
        {
            const WorldGen generator(4242u, Dimension::Nether);
            int red = 0, teal = 0;
            for (int x = -3000; x <= 3000; x += 40)
                for (int z = -3000; z <= 3000; z += 40)
                {
                    const Biome here = generator.netherBiomeAt(x, z);
                    if (here == Biome::CrimsonForest) ++red;
                    if (here == Biome::WarpedForest) ++teal;
                }
            check(red > 0 && teal > 0, "both nether forests grow somewhere");
        }

        Plot cherry;
        cherry.set(0, 64, 0, Blocks::CherrySapling);
        check(Saplings::grow(cherry, glm::ivec3(0, 64, 0), 5u), "a cherry sapling grows");
        check(cherry.at(0, 64, 0) == Blocks::CherryLog, "into a cherry tree");
        int pink = 0;
        for (const auto& [where, id] : cherry.cells)
            if (id == Blocks::CherryLeaves) ++pink;
        check(pink > 20, "with a pink crown");
        check(planksFromLog(Blocks::CherryLog) == Blocks::CherryPlanks, "cherry logs make cherry planks");
        check(isPinkPetals(Blocks::PinkPetals3) && blockDrop(Blocks::PinkPetals3) == Blocks::PinkPetals1,
              "petals come back as one handful");

        int saplings = 0;
        for (uint32_t roll = 0; roll < 2000u; ++roll)
            if (Saplings::leafDrop(Blocks::Leaves, roll) == Blocks::OakSapling) ++saplings;
        check(saplings == 100, "oak leaves give a sapling one time in twenty");
        check(Saplings::leafDrop(Blocks::Stone, 0u) == Blocks::Air, "stone gives none");
    }

    void testLushPlants()
    {
        section("lush cave plants");
        check(caveVineOf(true, true) == Blocks::CaveVinesLit && caveVineOf(false, true) == Blocks::GlowBerries &&
                  caveVineOf(false, false) == Blocks::CaveVinesPlant && caveVineOf(true, false) == Blocks::CaveVines,
              "a cave vine is a head or a stalk, with berries or without");
        check(blockDrop(Blocks::CaveVinesLit) == Items::GlowBerries && blockDrop(Blocks::CaveVinesPlant) == Blocks::Air,
              "only a vine with berries gives any");
        check(blockInfo(Blocks::CaveVinesLit).lightEmission == 14 && blockInfo(Blocks::CaveVines).lightEmission == 0,
              "and only the berries glow");
        bool leaves = true;
        for (int facing = 0; facing < 4; ++facing)
            for (int tilt = 0; tilt < 4; ++tilt)
            {
                const BlockId leaf = bigDripleafOf(facing, static_cast<DripleafTilt>(tilt));
                if (!isBigDripleaf(leaf) || dripleafFacing(leaf) != facing || static_cast<int>(dripleafTilt(leaf)) != tilt) leaves = false;
                if (blockDrop(leaf) != bigDripleafOf(0, DripleafTilt::None)) leaves = false;
            }
        check(leaves, "a big dripleaf faces four ways and tilts four ways");
        check(blockOverlaps(bigDripleafOf(1, DripleafTilt::Partial), 0, 0, 0, 0.2f, 0.7f, 0.2f, 0.8f, 0.9f, 0.8f) &&
                  !blockOverlaps(bigDripleafOf(1, DripleafTilt::Full), 0, 0, 0, 0.2f, 0.7f, 0.2f, 0.8f, 0.9f, 0.8f),
              "you can stand on a leaf until it tips right over");
        check(smallDripleafUpper(smallDripleafOf(2, true)) && !smallDripleafUpper(smallDripleafOf(2, false)) &&
                  dripleafFacing(smallDripleafOf(2, true)) == 2,
              "a small dripleaf is two blocks");
        for (int face = 0; face < 6; ++face)
            check(isGlowLichen(glowLichenOn(face)) && glowLichenFace(glowLichenOn(face)) == face &&
                      blockInfo(glowLichenOn(face)).lightEmission == 7,
                  "glow lichen on face " + std::to_string(face) + " glows");
        check(pottedOf(Blocks::Azalea) == Blocks::PottedAzalea && pottedPlant(Blocks::PottedFloweringAzalea) == Blocks::FloweringAzalea,
              "azaleas go in pots");
        struct Plot : Saplings::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> cells;
            BlockId at(int x, int y, int z) const override
            {
                const auto found = cells.find({ x, y, z });
                if (found != cells.end()) return found->second;
                return y < 64 ? Blocks::Grass : Blocks::Air;
            }
            void set(int x, int y, int z, BlockId id) override { cells[{ x, y, z }] = id; }
        };
        Plot plot;
        check(Saplings::azaleaTree(plot, glm::ivec3(0, 64, 0), 5u), "an azalea grows into a tree");
        check(plot.at(0, 63, 0) == Blocks::RootedDirt && plot.at(0, 64, 0) == Blocks::Log, "with rooted dirt under its trunk");
        int flowering = 0, plain = 0;
        for (const auto& cell : plot.cells)
        {
            if (cell.second == Blocks::FloweringAzaleaLeaves) ++flowering;
            if (cell.second == Blocks::AzaleaLeaves) ++plain;
        }
        check(plain > 0 && flowering > 0, "and leaves of both kinds");
        ItemStack carpet[9];
        carpet[0] = ItemStack{ Blocks::Moss, 1 };
        carpet[1] = ItemStack{ Blocks::Moss, 1 };
        check(Crafting::match(carpet, 3).id == Blocks::MossCarpet && Crafting::match(carpet, 3).count == 3,
              "two moss make three carpets");
    }

    void testMushrooms()
    {
        section("mushrooms, huge mushrooms and the dirt kinds");
        struct Plot : Saplings::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> cells;
            BlockId at(int x, int y, int z) const override
            {
                const auto found = cells.find({ x, y, z });
                if (found != cells.end()) return found->second;
                return y < 64 ? Blocks::Mycelium : Blocks::Air;
            }
            void set(int x, int y, int z, BlockId id) override { cells[{ x, y, z }] = id; }
        };
        {
            Plot red;
            check(Saplings::hugeMushroom(red, glm::ivec3(0, 64, 0), true, 3u), "a red mushroom grows huge on mycelium");
            check(red.at(0, 64, 0) == Blocks::MushroomStem && red.at(0, 66, 0) == Blocks::MushroomStem, "on a stem");
            int caps = 0;
            for (const auto& cell : red.cells) if (cell.second == Blocks::RedMushroomBlock) ++caps;
            check(caps > 20, "under a dome of red cap");
        }
        {
            Plot brown;
            check(Saplings::hugeMushroom(brown, glm::ivec3(0, 64, 0), false, 4u), "a brown one grows too");
            int caps = 0;
            for (const auto& cell : brown.cells) if (cell.second == Blocks::BrownMushroomBlock) ++caps;
            check(caps == 45, "with a flat cap seven wide and its corners cut");
        }
        {
            Plot cramped;
            cramped.set(0, 67, 0, Blocks::Stone);
            check(!Saplings::hugeMushroom(cramped, glm::ivec3(0, 64, 0), true, 3u), "but not under a roof");
        }
        {
            Plot stone;
            stone.set(0, 63, 0, Blocks::Stone);
            check(!Saplings::hugeMushroom(stone, glm::ivec3(0, 64, 0), false, 3u), "nor on bare stone");
        }
        check(blockDrop(Blocks::BrownMushroomBlock) == Blocks::BrownMushroom && blockDrop(Blocks::MushroomStem) == Blocks::Air,
              "a cap gives mushrooms and a stem gives nothing");
        check(blockDrop(Blocks::Mycelium) == Blocks::Dirt && blockDrop(Blocks::Podzol) == Blocks::Dirt &&
                  blockDrop(Blocks::CoarseDirt) == Blocks::CoarseDirt && blockDrop(Blocks::RootedDirt) == Blocks::RootedDirt,
              "mycelium and podzol break to dirt; coarse and rooted dirt keep themselves");
        check(blockInfo(Blocks::BrownMushroom).lightEmission == 1, "a brown mushroom glows a little");
        check(blockMaterial(Blocks::RedMushroomBlock) == Material::Wood && blockMaterial(Blocks::Podzol) == Material::Dirt,
              "caps want an axe and podzol a shovel");
        check(pottedOf(Blocks::RedMushroom) != Blocks::Air && pottedOf(Blocks::BrownMushroom) != Blocks::Air,
              "mushrooms go in pots");
        ItemStack coarse[9];
        coarse[0] = ItemStack{ Blocks::Dirt, 1 };
        coarse[1] = ItemStack{ Blocks::Gravel, 1 };
        coarse[3] = ItemStack{ Blocks::Gravel, 1 };
        coarse[4] = ItemStack{ Blocks::Dirt, 1 };
        check(Crafting::match(coarse, 3).id == Blocks::CoarseDirt && Crafting::match(coarse, 3).count == 4,
              "dirt and gravel make coarse dirt");
        {
            const WorldGen generator(4242u);
            bool found = false;
            for (int ring = 0; ring < 400 && !found; ring += 2)
                for (int step = 0; step < 32 && !found; ++step)
                {
                    const float angle = step * 0.19635f;
                    const int x = static_cast<int>(std::cos(angle) * ring) * 16 + 8;
                    const int z = static_cast<int>(std::sin(angle) * ring) * 16 + 8;
                    if (generator.biomeAt(x, z) == Biome::MushroomFields)
                    {
                        found = generator.surfaceHeight(x, z) > WorldGen::SEA_LEVEL;
                    }
                }
            check(found, "mushroom fields rise out of the sea somewhere");
        }
    }

    void testPlants()
    {
        section("flowers, tall plants, ferns and flower pots");
        bool pairs = true;
        for (int k = 0; k < static_cast<int>(DoublePlant::Count); ++k)
        {
            const DoublePlant kind = static_cast<DoublePlant>(k);
            const BlockId low = doublePlantOf(kind, false), high = doublePlantOf(kind, true);
            if (!isDoublePlant(low) || !isDoublePlant(high) || doublePlantUpper(low) || !doublePlantUpper(high) ||
                doublePlantKind(low) != kind || doublePlantKind(high) != kind)
                pairs = false;
            if (isObtainable(high)) pairs = false;
        }
        check(pairs, "every tall plant is a bottom and a top, and only the bottom is carried");
        check(blockDrop(doublePlantOf(DoublePlant::Lilac, true)) == doublePlantOf(DoublePlant::Lilac, false),
              "a lilac broken at the top gives one lilac");
        check(blockDrop(doublePlantOf(DoublePlant::TallGrass, false)) == Items::WheatSeeds, "tall grass gives seeds");
        for (int stage = 0; stage < 5; ++stage)
        {
            const BlockId crop = pitcherCropOf(stage, false);
            check(isPitcherCrop(crop) && pitcherCropStage(crop) == stage && !pitcherCropUpper(crop),
                  "the pitcher crop at stage " + std::to_string(stage));
            if (stage >= 3)
                check(pitcherCropUpper(pitcherCropOf(stage, true)) && pitcherCropStage(pitcherCropOf(stage, true)) == stage,
                      "and its top half");
        }
        check(blockDrop(pitcherCropOf(4, false)) == doublePlantOf(DoublePlant::PitcherPlant, false) &&
                  blockDrop(pitcherCropOf(2, false)) == Items::PitcherPod,
              "a ripe pitcher crop gives the plant; an unripe one its pod");
        check(blockDrop(Blocks::TorchflowerCrop1) == Items::TorchflowerSeeds, "a torchflower crop gives its seeds back");
        bool pots = true;
        for (BlockId pot = Blocks::PottedFirst; pot <= Blocks::PottedLast; pot = static_cast<BlockId>(pot + 1))
        {
            const BlockId plant = pottedPlant(pot);
            if (plant == Blocks::Air || pottedOf(plant) != pot) pots = false;
            if (blockDrop(pot) != Blocks::FlowerPot || isObtainable(pot)) pots = false;
        }
        check(pots, "every potted plant knows its plant and breaks into a pot");
        check(pottedOf(Blocks::Stone) == Blocks::Air, "a pot will not take stone");
        check(pottedOf(Blocks::Cactus) != Blocks::Air && pottedOf(Blocks::Fern) != Blocks::Air &&
                  pottedOf(Blocks::CherrySapling) != Blocks::Air,
              "a pot takes cactus, fern and saplings");
        check(!blockOverlaps(Blocks::FlowerPot, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f, 0.3f) &&
                  blockOverlaps(Blocks::FlowerPot, 0, 0, 0, 0.4f, 0.1f, 0.4f, 0.6f, 0.2f, 0.6f),
              "a flower pot is small");

        struct DyeCase { BlockId flower; StackId dye; int count; };
        const DyeCase CASES[] = {
            { Blocks::Allium, Items::DyeMagenta, 1 }, { Blocks::AzureBluet, Items::DyeLightGray, 1 },
            { Blocks::BlueOrchid, Items::DyeLightBlue, 1 }, { Blocks::Cornflower, Items::DyeBlue, 1 },
            { Blocks::LilyOfTheValley, Items::DyeWhite, 1 }, { Blocks::OxeyeDaisy, Items::DyeLightGray, 1 },
            { Blocks::RedTulip, Items::DyeRed, 1 }, { Blocks::OrangeTulip, Items::DyeOrange, 1 },
            { Blocks::WhiteTulip, Items::DyeLightGray, 1 }, { Blocks::PinkTulip, Items::DyePink, 1 },
            { Blocks::WitherRose, Items::DyeBlack, 1 }, { Blocks::Torchflower, Items::DyeOrange, 1 },
            { doublePlantOf(DoublePlant::Sunflower, false), Items::DyeYellow, 2 },
            { doublePlantOf(DoublePlant::Lilac, false), Items::DyeMagenta, 2 },
            { doublePlantOf(DoublePlant::RoseBush, false), Items::DyeRed, 2 },
            { doublePlantOf(DoublePlant::Peony, false), Items::DyePink, 2 },
            { doublePlantOf(DoublePlant::PitcherPlant, false), Items::DyeCyan, 2 },
        };
        for (const DyeCase& c : CASES)
        {
            ItemStack grid[9];
            grid[4] = ItemStack{ c.flower, 1 };
            const CraftOutput made = Crafting::match(grid, 3);
            check(made.id == c.dye && made.count == c.count, std::string(blockInfo(c.flower).name) + " gives its dye");
        }
        ItemStack pot[9];
        pot[0] = ItemStack{ Items::Brick, 1 };
        pot[2] = ItemStack{ Items::Brick, 1 };
        pot[4] = ItemStack{ Items::Brick, 1 };
        check(Crafting::match(pot, 3).id == Blocks::FlowerPot, "three bricks make a flower pot");
        check(blockDrop(Blocks::Clay) == Items::ClayBall, "a clay block breaks into clay balls");
        check(blockMaterial(Blocks::RedTulip) == Material::Plant && blockMaterial(doublePlantOf(DoublePlant::Peony, true)) == Material::Plant,
              "flowers come away in your hand");
        check(std::string(blockInfo(Blocks::TallGrass).name) == "Short Grass" &&
                  std::string(blockInfo(doublePlantOf(DoublePlant::TallGrass, false)).name) == "Tall Grass",
              "short grass and tall grass have 1.21's names");
    }

    void testCopperSet()
    {
        section("the copper set, waxing and the lightning rod");
        bool roundTrip = true;
        for (CopperFamily family : { CopperFamily::Block, CopperFamily::Cut, CopperFamily::Chiseled, CopperFamily::Grate,
                                     CopperFamily::Slab, CopperFamily::Bulb, CopperFamily::Door, CopperFamily::Trapdoor })
            for (int stage = 0; stage < 4; ++stage)
                for (int waxed = 0; waxed < 2; ++waxed)
                {
                    const int states = family == CopperFamily::Bulb ? 4 : (family == CopperFamily::Door || family == CopperFamily::Trapdoor) ? 16 : 1;
                    for (int state = 0; state < states; ++state)
                    {
                        const BlockId id = copperBlockOf(family, stage, waxed == 1, state);
                        const CopperForm form = copperForm(id);
                        if (form.family != family || form.stage != stage || form.waxed != (waxed == 1) || form.state != state)
                            roundTrip = false;
                    }
                }
        check(roundTrip, "every copper block knows its family, stage, wax and state");
        check(Copper::next(copperBlockOf(CopperFamily::Cut, 0, false)) == copperBlockOf(CopperFamily::Cut, 1, false),
              "cut copper weathers a stage at a time");
        check(Copper::next(copperBlockOf(CopperFamily::Grate, 3, false)) == copperBlockOf(CopperFamily::Grate, 3, false),
              "oxidised is the end of it");
        check(!Copper::ages(copperBlockOf(CopperFamily::Door, 1, true, 9)), "waxed copper never weathers");
        check(Copper::waxed(copperBlockOf(CopperFamily::Chiseled, 2, false)) == copperBlockOf(CopperFamily::Chiseled, 2, true),
              "honeycomb waxes copper where it stands");
        check(Copper::scraped(copperBlockOf(CopperFamily::Trapdoor, 2, true, 5)) == copperBlockOf(CopperFamily::Trapdoor, 2, false, 5),
              "an axe scrapes the wax off first");
        check(Copper::scraped(copperBlockOf(CopperFamily::Trapdoor, 2, false, 5)) == copperBlockOf(CopperFamily::Trapdoor, 1, false, 5),
              "and then a stage of green");
        check(Copper::cleaned(Blocks::OxidizedCopper) == Blocks::CopperBlock &&
                  Copper::cleaned(copperBlockOf(CopperFamily::Block, 3, true)) == copperBlockOf(CopperFamily::Block, 3, true),
              "lightning cleans bare copper back to new, and leaves waxed alone");
        check(isDoor(copperBlockOf(CopperFamily::Door, 3, false, 0)) &&
                  doorBase(copperBlockOf(CopperFamily::Door, 3, false, 13)) == copperBlockOf(CopperFamily::Door, 3, false, 0),
              "a copper door is a door, and knows which door it is");
        check(isTrapdoor(copperBlockOf(CopperFamily::Trapdoor, 1, true, 0)) &&
                  trapdoorBase(copperBlockOf(CopperFamily::Trapdoor, 1, true, 7)) == copperBlockOf(CopperFamily::Trapdoor, 1, true, 0),
              "and a copper trapdoor likewise");
        check(blockMaterial(copperBlockOf(CopperFamily::Door, 0, false)) == Material::Stone, "copper wants a pickaxe");
        check(lightEmission(copperBlockOf(CopperFamily::Bulb, 3, false, 1)) == 4 &&
                  lightEmission(copperBlockOf(CopperFamily::Bulb, 1, true, 1)) == 12,
              "a bulb dims as it weathers");
        check(copperBulbWith(copperBlockOf(CopperFamily::Bulb, 2, true, 0), true, true) == copperBlockOf(CopperFamily::Bulb, 2, true, 3),
              "a bulb keeps its stage when it is switched");

        ItemStack cut[9];
        for (int i : { 0, 1, 3, 4 }) cut[i] = ItemStack{ Blocks::WeatheredCopper, 1 };
        check(Crafting::match(cut, 3).id == copperBlockOf(CopperFamily::Cut, 2, false) && Crafting::match(cut, 3).count == 4,
              "four weathered copper make four weathered cut copper");
        ItemStack grate[9];
        for (int i : { 1, 3, 5, 7 }) grate[i] = ItemStack{ Blocks::CopperBlock, 1 };
        check(Crafting::match(grate, 3).id == copperBlockOf(CopperFamily::Grate, 0, false), "copper round a gap makes grates");
        ItemStack slabs[9];
        slabs[1] = ItemStack{ copperBlockOf(CopperFamily::Slab, 1, false), 1 };
        slabs[4] = ItemStack{ copperBlockOf(CopperFamily::Slab, 1, false), 1 };
        check(Crafting::match(slabs, 3).id == copperBlockOf(CopperFamily::Chiseled, 1, false), "two slabs make chiseled copper");
        ItemStack wax[9];
        wax[0] = ItemStack{ copperBlockOf(CopperFamily::Door, 2, false), 1 };
        wax[1] = ItemStack{ Items::Honeycomb, 1 };
        check(Crafting::match(wax, 3).id == copperBlockOf(CopperFamily::Door, 2, true), "honeycomb waxes a door at the bench too");
        ItemStack door[9];
        for (int i : { 0, 1, 3, 4, 6, 7 }) door[i] = ItemStack{ Items::CopperIngot, 1 };
        check(Crafting::match(door, 3).id == copperBlockOf(CopperFamily::Door, 0, false) && Crafting::match(door, 3).count == 3,
              "six ingots make three copper doors");
        ItemStack rod[9];
        for (int i : { 1, 4, 7 }) rod[i] = ItemStack{ Items::CopperIngot, 1 };
        check(Crafting::match(rod, 3).id == lightningRodOf(0, false), "three ingots stood up make a lightning rod");

        bool rods = true;
        for (int facing = 0; facing < 6; ++facing)
            for (int powered = 0; powered < 2; ++powered)
            {
                const BlockId id = lightningRodOf(facing, powered == 1);
                if (!isLightningRod(id) || lightningRodFacing(id) != facing || lightningRodPowered(id) != (powered == 1)) rods = false;
                if (blockDrop(id) != lightningRodOf(0, false)) rods = false;
            }
        check(rods, "a lightning rod points six ways and always breaks into one rod");
        check(Redstone::isSource(lightningRodOf(2, true)) && !Redstone::isSource(lightningRodOf(2, false)),
              "a struck rod gives a pulse of power");
        check(blockOverlaps(lightningRodOf(0, false), 0, 0, 0, 0.45f, 0.5f, 0.45f, 0.55f, 0.6f, 0.55f) &&
                  !blockOverlaps(lightningRodOf(0, false), 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f, 0.3f),
              "a rod is thin");
    }

    void testChestKinds()
    {
        section("double, trapped and ender chests");
        bool roundTrip = true;
        for (int kind = 0; kind < 3; ++kind)
            for (int facing = 0; facing < 4; ++facing)
                for (int half = 0; half < (kind == 2 ? 1 : 3); ++half)
                    for (int open = 0; open < (kind == 1 ? 2 : 1); ++open)
                    {
                        const BlockId id = chestOf(kind, facing, half, open == 1);
                        const ChestState s = chestState(id);
                        if (!isChestFamily(id) || s.kind != kind || s.facing != facing || s.half != half ||
                            s.open != (open == 1))
                            roundTrip = false;
                    }
        check(roundTrip, "every kind, facing, half and lid of chest is its own block");
        check(chestOf(0, 2, 0) == Blocks::Chest, "a plain chest facing south is the chest that was always there");
        int dx = 0, dz = 0;
        check(chestPartnerStep(chestOf(0, 0, 1), dx, dz) && dx == 1 && dz == 0,
              "the left half of a north-facing chest has its partner to the east");
        check(chestPartnerStep(chestOf(0, 0, 2), dx, dz) && dx == -1 && dz == 0, "and the right half to the west");
        check(!chestPartnerStep(chestOf(0, 1, 0), dx, dz), "a single chest has no partner");
        check(isContainerBlock(chestOf(1, 3, 2)) && !isContainerBlock(chestOf(2, 1, 0)),
              "a trapped chest spills like a chest; an ender chest keeps its things elsewhere");
        check(Redstone::isSource(chestOf(1, 0, 0, true)) && !Redstone::isSource(chestOf(1, 0, 0, false)),
              "a trapped chest gives power only while it is open");
        check(blockDrop(chestOf(0, 1, 2)) == Blocks::Chest && blockDrop(chestOf(1, 3, 1, true)) == chestOf(1, 2, 0),
              "any half of a chest breaks into one chest of its kind");
        check(blockDrop(chestOf(2, 0, 0)) == Blocks::Obsidian, "an ender chest breaks into obsidian");
        check(isObtainable(chestOf(1, 2, 0)) && isObtainable(chestOf(2, 2, 0)) && !isObtainable(chestOf(0, 1, 1)),
              "only the plain facing of each chest is carried");
        check(blockInfo(chestOf(2, 0, 0)).lightEmission == 7, "an ender chest glows a little");
        check(Hopper::slotsOf(chestOf(1, 0, 1)) == 27 && Hopper::slotsOf(chestOf(2, 0, 0)) == 0,
              "hoppers feed chests and trapped chests but not ender chests");

        ItemStack hook[9];
        hook[1] = ItemStack{ Items::IronIngot, 1 };
        hook[4] = ItemStack{ Items::Stick, 1 };
        hook[7] = ItemStack{ Blocks::BirchPlanks, 1 };
        check(Crafting::match(hook, 3).id == Items::TripwireHook && Crafting::match(hook, 3).count == 2,
              "iron, a stick and planks make two tripwire hooks");
        ItemStack trapped[9];
        trapped[0] = ItemStack{ Blocks::Chest, 1 };
        trapped[5] = ItemStack{ Items::TripwireHook, 1 };
        check(Crafting::match(trapped, 3).id == chestOf(1, 2, 0), "a chest and a hook make a trapped chest");
        ItemStack ender[9];
        for (int i = 0; i < 9; ++i) ender[i] = ItemStack{ Blocks::Obsidian, 1 };
        ender[4] = ItemStack{ Items::EyeOfEnder, 1 };
        check(Crafting::match(ender, 3).id == chestOf(2, 2, 0), "obsidian round an eye of ender makes an ender chest");

        Chests store;
        store.ender()[3] = ItemStack{ Items::Diamond, 2 };
        store.setDimension(1);
        check(store.ender()[3].id == Items::Diamond, "the ender chest holds the same things in every dimension");
        check(Inventory::CHEST2_FIRST + Inventory::CHEST_SLOTS == Inventory::SLOT_COUNT &&
                  Inventory::CHEST2_FIRST > Inventory::GRIND_RESULT,
              "a large chest's lower half has slots of its own after every older slot");
    }

    void testBeds()
    {
        section("beds in sixteen colours");
        bool roundTrip = true;
        for (int colour = 0; colour < 16; ++colour)
            for (int facing = 0; facing < 4; ++facing)
                for (int head = 0; head < 2; ++head)
                {
                    const BlockId bed = bedOf(colour, facing, head == 1);
                    if (!isBed(bed) || bedColour(bed) != colour || bedFacing(bed) != facing ||
                        bedIsHead(bed) != (head == 1))
                        roundTrip = false;
                    if (isObtainable(bed) != (facing == 0 && head == 0)) roundTrip = false;
                    if (blockDrop(bed) != bedOf(colour, 0, false)) roundTrip = false;
                }
        check(roundTrip, "every colour, facing and half is its own bed and breaks to one bed of its colour");
        check(blockDrop(Blocks::Bed) == bedOf(14, 0, false) && !isObtainable(Blocks::Bed),
              "an old bed breaks into a red one");
        int dx = 0, dz = 0;
        bedStep(1, dx, dz);
        check(dx == 1 && dz == 0, "a bed facing east has its head to the east");
        check(blockOverlaps(bedOf(3, 2, true), 0, 0, 0, 0.1f, 0.0f, 0.1f, 0.9f, 0.5f, 0.9f) &&
                  !blockOverlaps(bedOf(3, 2, true), 0, 0, 0, 0.1f, 0.6f, 0.1f, 0.9f, 1.0f, 0.9f),
              "a bed is nine sixteenths tall");
        for (int colour = 0; colour < 16; ++colour)
        {
            ItemStack grid[9];
            for (int i = 0; i < 3; ++i) grid[i] = ItemStack{ woolOfColour(colour), 1 };
            for (int i = 3; i < 6; ++i) grid[i] = ItemStack{ colour % 2 ? Blocks::SprucePlanks : Blocks::Planks, 1 };
            check(Crafting::match(grid, 3).id == bedOf(colour, 0, false),
                  "three wool of colour " + std::to_string(colour) + " over planks make that bed");
        }
        check(blockMaterial(bedOf(0, 0, false)) == Material::Wool, "a bed comes apart easily");
    }

    void testShulkerBoxes()
    {
        section("shulker boxes in sixteen colours");
        for (int colour = 0; colour < DYE_COLOURS; ++colour)
        {
            const BlockId box = colourIn(Blocks::ShulkerBoxFirst, colour);
            check(isShulkerBox(box) && isContainerBlock(box) && shulkerBoxColour(box) == colour,
                  std::string(blockInfo(box).name) + " is a shulker box of its colour");
            check(isObtainable(box) && blockDrop(box) == box, "and comes back as itself");
            ItemStack grid[9];
            grid[4] = ItemStack{ Blocks::ShulkerBox, 1 };
            grid[0] = ItemStack{ dyeOfColour(colour), 1 };
            check(Crafting::match(grid, 3).id == box, "a shulker box takes the dye");
            grid[4] = ItemStack{ colourIn(Blocks::ShulkerBoxFirst, (colour + 3) % 16), 1 };
            check(Crafting::match(grid, 3).id == box, "even one already dyed");
        }
        check(shulkerBoxColour(Blocks::ShulkerBox) == -1, "the plain box has no dye");
        check(std::string(colourFileName(3)) == "light_blue" && std::string(colourFileName(16)).empty(),
              "colours have their file names");
        Mob shulker(MobId::Shulker, glm::vec3(0.0f), 5u);
        check(shulker.shulkerColour() == 16, "a shulker is born undyed");
        shulker.setShulkerColour(14);
        check(shulker.shulkerColour() == 14, "and keeps the colour it is given");
        shulker.setShulkerColour(40);
        check(shulker.shulkerColour() == 16, "and nothing past black");
    }

    void testCandles()
    {
        section("candles in sixteen colours, and candle cakes");

        bool roundTrip = true;
        for (int shade = 0; shade <= 16; ++shade)
            for (int count = 1; count <= 4; ++count)
                for (int lit = 0; lit < 2; ++lit)
                {
                    const BlockId id = candleOf(shade, count, lit == 1);
                    if (!isCandle(id) || candleShade(id) != shade || candleCount(id) != count ||
                        candleLit(id) != (lit == 1))
                        roundTrip = false;
                    if (blockInfo(id).lightEmission != (lit ? 3 * count : 0)) roundTrip = false;
                    if (isObtainable(id) != (count == 1 && !lit)) roundTrip = false;
                    if (blockDrop(id) != candleOf(shade, 1, false)) roundTrip = false;
                }
        check(roundTrip, "every colour, count and flame is its own candle, and gives its own light");
        check(candleOf(0, 1, false) == Blocks::Candle && candleOf(0, 1, true) == Blocks::CandleLit,
              "the plain candle is the one that was always there");
        check(blockInfo(candleOf(14, 4, true)).lightEmission == 12, "four lit candles give twelve");

        for (int colour = 0; colour < DYE_COLOURS; ++colour)
        {
            ItemStack grid[9];
            grid[0] = ItemStack{ Blocks::Candle, 1 };
            grid[4] = ItemStack{ dyeOfColour(colour), 1 };
            const CraftOutput made = Crafting::match(grid, 3);
            check(made.id == candleOf(colour + 1, 1, false) && made.count == 1,
                  "a candle and dye " + std::to_string(colour) + " make that candle");
        }
        {
            ItemStack grid[9];
            grid[0] = ItemStack{ candleOf(3, 1, false), 1 };
            grid[1] = ItemStack{ Items::DyeRed, 1 };
            check(!Crafting::match(grid, 3).valid(), "a coloured candle is not dyed again");
        }

        for (int shade = 0; shade <= 16; ++shade)
        {
            const BlockId cake = candleCakeOf(shade, shade % 2 == 1);
            if (!isCandleCake(cake) || candleCakeShade(cake) != shade) check(false, "candle cake round trip");
        }
        check(blockDrop(candleCakeOf(5, true)) == candleOf(5, 1, false), "a broken candle cake gives back its candle");
        check(blockInfo(candleCakeOf(0, true)).lightEmission == 3 && blockInfo(candleCakeOf(0, false)).lightEmission == 0,
              "a lit candle cake glows like one candle");
        check(!isObtainable(candleCakeOf(2, false)), "nobody carries a candle cake");
        check(!blockOverlaps(Blocks::Cake0, 0, 0, 0, 0.2f, 0.6f, 0.2f, 0.8f, 1.0f, 0.8f),
              "a cake is half a block tall");
        check(blockOverlaps(Blocks::Cake0, 0, 0, 0, 0.2f, 0.3f, 0.2f, 0.8f, 1.0f, 0.8f), "and solid below that");
        check(!blockOverlaps(Blocks::Cake6, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.5f, 0.4f, 1.0f),
              "the eaten end of a cake is gone");
        check(blockMaterial(candleOf(9, 2, true)) == Material::Wool, "candles come away in your hand");
    }

    void testColourBlocks()
    {
        section("concrete, terracotta and stained glass");

        auto ring = [](StackId middle, StackId around) {
            Inventory bench;
            bench.setCraftSize(3);
            for (int cell = 0; cell < 9; ++cell)
                bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ cell == 4 ? middle : around, 1 };
            bench.refreshCraftResult();
            return bench.slot(Inventory::CRAFT_RESULT);
        };
        for (int colour = 0; colour < DYE_COLOURS; ++colour)
        {
            const StackId dye = dyeOfColour(colour);
            const ItemStack terracotta = ring(dye, Blocks::Terracotta);
            check(terracotta.id == colourIn(Blocks::TerracottaFirst, colour) && terracotta.count == 8,
                  "eight terracotta round dye " + std::to_string(colour) + " take its colour");
            const ItemStack glass = ring(dye, Blocks::Glass);
            check(glass.id == colourIn(Blocks::StainedGlassFirst, colour) && glass.count == 8,
                  "and eight glass do too");
            const ItemStack panes = ring(dye, Blocks::GlassPane);
            check(panes.id == colourIn(Blocks::StainedGlassPaneFirst, colour) && panes.count == 8,
                  "and eight panes");
            check(Smelting::resultOf(colourIn(Blocks::TerracottaFirst, colour)) ==
                      colourIn(Blocks::GlazedTerracottaFirst, colour),
                  "coloured terracotta fires glazed");
        }
        check(ring(Items::DyeRed, Blocks::Stone).empty(), "dye does nothing to stone");

        {
            Inventory bench;
            bench.setCraftSize(3);
            for (int cell = 0; cell < 4; ++cell) bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ Blocks::Sand, 1 };
            for (int cell = 4; cell < 8; ++cell) bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ Blocks::Gravel, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 8) = ItemStack{ Items::DyeLime, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == colourIn(Blocks::ConcretePowderFirst, 5) &&
                      bench.slot(Inventory::CRAFT_RESULT).count == 8,
                  "four sand, four gravel and lime dye make eight lime concrete powder");
        }

        ItemStack glass[9];
        for (int i = 0; i < 6; ++i) glass[i] = ItemStack{ Blocks::Glass, 1 };
        check(Crafting::match(glass, 3).id == Blocks::GlassPane && Crafting::match(glass, 3).count == 16,
              "six glass make sixteen panes");
        ItemStack blue[9];
        for (int i = 3; i < 9; ++i) blue[i] = ItemStack{ colourIn(Blocks::StainedGlassFirst, 11), 1 };
        check(Crafting::match(blue, 3).id == colourIn(Blocks::StainedGlassPaneFirst, 11),
              "six blue glass make blue panes");

        check(blockDrop(Blocks::AmethystCluster) == Items::AmethystShard, "an amethyst cluster gives shards");
        ItemStack shards[9];
        for (int i : { 0, 1, 3, 4 }) shards[i] = ItemStack{ Items::AmethystShard, 1 };
        check(Crafting::match(shards, 3).id == Blocks::AmethystBlock, "four shards make an amethyst block");
        ItemStack tinted[9];
        for (int i : { 1, 3, 5, 7 }) tinted[i] = ItemStack{ Items::AmethystShard, 1 };
        tinted[4] = ItemStack{ Blocks::Glass, 1 };
        check(Crafting::match(tinted, 3).id == Blocks::TintedGlass && Crafting::match(tinted, 3).count == 2,
              "glass inside four shards makes two tinted glass");
        check(blockMaterial(Blocks::TintedGlass) == Material::Glass &&
                  blockMaterial(colourIn(Blocks::StainedGlassPaneFirst, 3)) == Material::Glass,
              "tinted glass and panes are glass");
        check(blockMaterial(colourIn(Blocks::ConcretePowderFirst, 2)) == Material::Sand,
              "powder wants a shovel");
        check(!isOpaque(Blocks::TintedGlass) && !isOpaque(colourIn(Blocks::StainedGlassFirst, 0)),
              "glass of any kind can be seen through");

        struct Pool : Fluids::Cells
        {
            std::map<std::tuple<int, int, int>, BlockId> cells;
            BlockId at(int x, int y, int z) const override
            {
                const auto found = cells.find({ x, y, z });
                return found == cells.end() ? Blocks::Air : found->second;
            }
            void set(int x, int y, int z, BlockId id) override { cells[{ x, y, z }] = id; }
        };
        {
            Pool pool;
            pool.set(0, 64, 0, colourIn(Blocks::ConcretePowderFirst, 14));
            check(Fluids::hardenAround(pool, 0, 64, 0) == 0, "dry powder stays powder");
            pool.set(1, 64, 0, Blocks::Water);
            Fluids::hardenAround(pool, 1, 64, 0);
            check(pool.at(0, 64, 0) == colourIn(Blocks::ConcreteFirst, 14), "water beside it sets it to red concrete");
        }
        {
            Pool pool;
            pool.set(0, 64, 0, Blocks::ConcretePowderFirst);
            pool.set(0, 63, 0, Blocks::Water);
            Fluids::hardenAround(pool, 0, 63, 0);
            check(pool.at(0, 64, 0) == Blocks::ConcretePowderFirst, "water only underneath does not");
            pool.set(0, 65, 0, Blocks::WaterFalling);
            Fluids::hardenAround(pool, 0, 65, 0);
            check(pool.at(0, 64, 0) == Blocks::ConcreteFirst, "water falling on it does");
        }

        const int lone = paneLinks([](int, int) { return Blocks::Air; });
        check(lone == 0, "a lone pane joins nothing");
        const int run = paneLinks([](int dx, int) { return dx != 0 ? Blocks::GlassPane : Blocks::Air; });
        check(run == (4 | 8), "a pane in a row joins both neighbours");
        check(paneLinks([](int dx, int dz) { return dz == -1 && dx == 0 ? Blocks::Stone : Blocks::Air; }) == 1,
              "and joins a wall");
        check(!blockOverlaps(Blocks::GlassPane, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f, 0.3f, 0),
              "you can stand in the corner of a lone pane's cell");
        check(blockOverlaps(Blocks::GlassPane, 0, 0, 0, 0.0f, 0.0f, 0.4f, 0.3f, 1.0f, 0.6f, 4),
              "but not across its arm");
        check(isPane(Blocks::GlassPane) && isPane(colourIn(Blocks::StainedGlassPaneFirst, 15)), "panes are panes");

        {
            const WorldGen generator(4242u);
            int foundX = 0, foundZ = 0;
            bool found = false;
            for (int x = -8000; x <= 8000 && !found; x += 32)
                for (int z = -8000; z <= 8000 && !found; z += 32)
                    if (generator.biomeAt(x, z) == Biome::Badlands &&
                        generator.biomeAt(x + 16, z + 16) == Biome::Badlands)
                    { foundX = x; foundZ = z; found = true; }
            int terracotta = 0;
            if (found)
            {
                Chunk chunk(ChunkPos{ foundX / Chunk::SX, foundZ / Chunk::SZ });
                generator.generate(chunk);
                for (int y = 0; y < Chunk::SY; ++y)
                    for (int lx = 0; lx < Chunk::SX; ++lx)
                        for (int lz = 0; lz < Chunk::SZ; ++lz)
                        {
                            const BlockId b = chunk.getBlock(lx, y, lz);
                            if (b == Blocks::Terracotta || isColouredTerracotta(b)) ++terracotta;
                        }
            }
            check(terracotta > 0, "the badlands are banded with terracotta");
        }
    }

    void testStoneSets()
    {
        section("the stone sets");

        ItemStack grid[9];
        for (int i : { 0, 1, 3, 4 }) grid[i] = ItemStack{ Blocks::Stone, 1 };
        check(Crafting::match(grid, 3).id == Blocks::StoneBricks, "four stone make stone bricks");
        ItemStack slabs[9];
        slabs[0] = ItemStack{ Blocks::StoneBrickSlab, 1 };
        slabs[3] = ItemStack{ Blocks::StoneBrickSlab, 1 };
        check(Crafting::match(slabs, 3).id == Blocks::ChiseledStoneBricks, "two brick slabs make chiseled bricks");
        check(Smelting::resultOf(Blocks::StoneBricks) == Blocks::CrackedStoneBricks, "fired bricks crack");
        check(Smelting::resultOf(Blocks::Stone) == Blocks::SmoothStone, "and fired stone goes smooth");
        check(blockDrop(Blocks::Deepslate) == Blocks::CobbledDeepslate, "deepslate breaks to cobbled deepslate");
        check(Smelting::resultOf(Blocks::CobbledDeepslate) == Blocks::Deepslate, "which fires back to deepslate");
        ItemStack red[9];
        red[0] = ItemStack{ Items::NetherWart, 1 };
        red[1] = ItemStack{ Items::NetherBrick, 1 };
        red[3] = ItemStack{ Items::NetherBrick, 1 };
        red[4] = ItemStack{ Items::NetherWart, 1 };
        check(Crafting::match(red, 3).id == Blocks::RedNetherBricks, "wart and nether brick make red nether bricks");
        check(blockMaterial(Blocks::PolishedBlackstone) == Material::Stone, "polished blackstone wants a pickaxe");

        ItemStack redSand[9];
        for (int i : { 0, 1, 3, 4 }) redSand[i] = ItemStack{ Blocks::RedSand, 1 };
        check(Crafting::match(redSand, 3).id == Blocks::RedSandstone, "four red sand make red sandstone");
        check(Smelting::resultOf(Blocks::Sandstone) == Blocks::SmoothSandstone, "fired sandstone goes smooth");
        check(Smelting::resultOf(Blocks::RedSand) == Blocks::Glass, "and red sand makes glass too");
        {
            const WorldGen generator(4242u);
            int badlands = 0;
            for (int x = -8000; x <= 8000 && badlands == 0; x += 32)
                for (int z = -8000; z <= 8000 && badlands == 0; z += 32)
                    if (generator.biomeAt(x, z) == Biome::Badlands) ++badlands;
            check(badlands > 0, "badlands of red sand lie somewhere in a world");
        }

        check(blockDrop(Blocks::IronOre) == Items::RawIron && blockDrop(Blocks::GoldOre) == Items::RawGold,
              "iron and gold ore give raw metal");
        check(Smelting::resultOf(Items::RawIron) == Items::IronIngot, "which fires to an ingot");
        check(blockDrop(Blocks::DeepslateDiamondOre) == Items::Diamond, "deepslate diamond ore gives diamond");
        check(Tools::requiredLevel(Blocks::DeepslateDiamondOre) == Tools::requiredLevel(Blocks::DiamondOre),
              "and wants the same pickaxe");
        check(Experience::forMining(Blocks::DeepslateEmeraldOre).any(), "and pays the same experience");
        check(Smelting::resultOf(Blocks::DeepslateIronOre) == Items::IronIngot, "a deepslate ore fires like its twin");
        check(blockDrop(Blocks::NetherGoldOre) == Items::GoldNugget, "nether gold ore gives nuggets");
        {
            const WorldGen generator(4242u);
            int deep = 0;
            for (int cx = -2; cx <= 2; ++cx)
                for (int cz = -2; cz <= 2; ++cz)
                {
                    Chunk chunk(ChunkPos{ cx, cz });
                    generator.generate(chunk);
                    for (int y = 0; y < DEEPSLATE_TOP; ++y)
                        for (int x = 0; x < Chunk::SX; ++x)
                            for (int z = 0; z < Chunk::SZ; ++z)
                                if (oreBase(chunk.getBlock(x, y, z)) != chunk.getBlock(x, y, z)) ++deep;
                }
            check(deep > 0, "ore in the deepslate comes out as deepslate ore");
        }

        check(blockDrop(Blocks::NetherQuartzOre) == Items::NetherQuartz, "quartz ore gives quartz");
        check(Experience::forMining(Blocks::NetherQuartzOre).any(), "and experience with it");
        ItemStack quartz[9];
        for (int i : { 0, 1, 3, 4 }) quartz[i] = ItemStack{ Items::NetherQuartz, 1 };
        check(Crafting::match(quartz, 3).id == Blocks::QuartzBlock, "four quartz make a block of it");
        ItemStack pillar[9];
        pillar[0] = ItemStack{ Blocks::QuartzBlock, 1 };
        pillar[3] = ItemStack{ Blocks::QuartzBlock, 1 };
        check(Crafting::match(pillar, 3).id == Blocks::QuartzPillar, "two stacked make a pillar");
        {
            const WorldGen nether(4242u, Dimension::Nether);
            Chunk chunk(ChunkPos{ 0, 0 });
            nether.generate(chunk);
            int ore = 0;
            for (int y = 0; y < Chunk::SY; ++y)
                for (int x = 0; x < Chunk::SX; ++x)
                    for (int z = 0; z < Chunk::SZ; ++z)
                        if (chunk.getBlock(x, y, z) == Blocks::NetherQuartzOre) ++ore;
            check(ore > 10, "and the Nether is full of quartz ore");
        }
    }

    void testGameModes()
    {
        section("adventure and spectator");

        check(Chat::parse("/gamemode adventure", true).mode == static_cast<int>(GameMode::Adventure),
              "/gamemode adventure names adventure");
        check(Chat::parse("/gamemode spectator", true).mode == static_cast<int>(GameMode::Spectator),
              "/gamemode spectator names spectator");
        check(!modeCanBuild(GameMode::Adventure), "an adventurer cannot build or break");
        check(!modeCanBuild(GameMode::Spectator), "nor can a spectator");
        check(modeCanBuild(GameMode::Survival) && modeCanBuild(GameMode::Creative), "survival and creative can");
        check(modeTakesDamage(GameMode::Adventure), "an adventurer gets hurt");
        check(!modeTakesDamage(GameMode::Spectator), "a spectator does not");

        Player ghost(glm::vec3(0.0f, 70.0f, 0.0f));
        ghost.mode = GameMode::Spectator;
        ghost.health = Player::MAX_HEALTH;
        check(!ghost.damage(5), "a blow passes through a spectator");
        check(ghost.health == Player::MAX_HEALTH, "and takes nothing off");
    }

    void testAttackCooldown()
    {
        section("a swing has to recharge, and a weapon sets how fast");

        check(Tools::attackSpeed(Blocks::Air) == 4.0f, "a bare hand is ready four times a second");
        check(Tools::attackSpeed(Items::DiamondSword) == 1.6f, "a sword 1.6 times");
        check(Tools::attackSpeed(Items::WoodAxe) < Tools::attackSpeed(Items::DiamondSword),
              "an axe is slower than a sword");
        check(Tools::attackSpeed(Items::Mace) < Tools::attackSpeed(Items::WoodAxe), "and a mace slower still");

        Mob pig(MobId::Pig, glm::vec3(0.0f, 64.0f, 0.0f), 11u);
        const int full = pig.health();
        check(pig.strike(3, glm::vec3(1.0f, 0.0f, 0.0f)), "a first hit lands");
        check(pig.health() == full - 3, "for all of it");
        check(!pig.strike(2, glm::vec3(1.0f, 0.0f, 0.0f)), "a weaker one straight after does nothing");
        check(pig.health() == full - 3, "and takes nothing");
        check(pig.strike(5, glm::vec3(1.0f, 0.0f, 0.0f)), "a harder one straight after lands");
        check(pig.health() == full - 5, "but only for the difference");
    }

    void testArmourSlotsHoldArmour()
    {
        section("the armour slots hold armour and nothing else");

        const int helmet = Inventory::ARMOR_FIRST + Armour::slotFor(Items::DiamondHelmet);
        const int chest = Inventory::ARMOR_FIRST + Armour::slotFor(Items::IronChestplate);

        check(Inventory::belongsIn(helmet, Items::DiamondHelmet), "a helmet goes on the head");
        check(!Inventory::belongsIn(helmet, Items::IronChestplate), "a chestplate does not");
        check(!Inventory::belongsIn(helmet, Items::DiamondPickaxe), "nor does a pickaxe");
        check(Inventory::belongsIn(chest, Items::Elytra), "the wings go where a chestplate goes");
        check(Inventory::belongsIn(0, Items::DiamondPickaxe), "and the pack takes anything");
        check(Inventory::belongsIn(Inventory::MAIN_SLOTS - 1, Items::DiamondHelmet),
              "a helmet included");

        // A save that put a pickaxe on the player's head -- which is
        // what the save's stash did with a full pack -- is mended on
        // load: the pickaxe goes back in the pack, the real armour
        // stays on.
        Inventory mended;
        mended.slot(helmet) = ItemStack{ Items::DiamondPickaxe, 1 };
        mended.slot(helmet).damage = 12;
        mended.slot(chest) = ItemStack{ Items::IronChestplate, 1 };

        check(mended.evictMisfitArmour().empty(), "everything finds room");
        check(mended.slot(helmet).empty(), "the pickaxe is off the head");
        check(mended.slot(chest).id == Items::IronChestplate, "the chestplate is left alone");
        check(mended.has(Items::DiamondPickaxe), "and the pickaxe is in the pack");

        bool wearKept = false;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i)
            if (mended.slot(i).id == Items::DiamondPickaxe && mended.slot(i).damage == 12)
                wearKept = true;
        check(wearKept, "with its wear, so mending a save does not repair a tool");

        // With nowhere to put it, it comes off anyway and is handed back
        // to be dropped, the way Minecraft drops anything that has no
        // room: a pickaxe left on the head would look like the bug it is.
        Inventory full;
        for (int i = 0; i < Inventory::MAIN_SLOTS; ++i)
            full.slot(i) = ItemStack{ Blocks::Stone, 64 };
        full.slot(helmet) = ItemStack{ Items::DiamondPickaxe, 1 };
        full.slot(helmet).damage = 7;
        const std::vector<ItemStack> spilled = full.evictMisfitArmour();
        check(full.slot(helmet).empty(), "a full pack still takes it off the head");
        check(spilled.size() == 1 && spilled[0].id == Items::DiamondPickaxe &&
                  spilled[0].count == 1 && spilled[0].damage == 7,
              "and hands it back whole, to be dropped rather than lost");
    }

    void testTheTablesTake121sPrices()
    {
        section("an enchanting table and a brewing stand take what 1.21's take");

        for (int row = 0; row < 3; ++row)
        {
            const Enchanting::Price price = Enchanting::priceOf(row);
            check(price.levels == row + 1 && price.lapis == row + 1,
                  "row " + std::to_string(row + 1) + " costs as many levels and lapis as its number");
        }

        Enchanting::Offer offer;
        offer.cost = 30;
        offer.kind = Enchanting::Kind::Efficiency;
        offer.level = 3;
        check(Enchanting::affordable(offer, 2, 30, 3, false), "thirty levels and three lapis buy the bottom row");
        check(!Enchanting::affordable(offer, 2, 29, 3, false), "twenty-nine levels do not");
        check(!Enchanting::affordable(offer, 2, 30, 2, false), "nor do two lapis");
        check(Enchanting::affordable(offer, 2, 0, 0, true), "creative pays nothing");
        Enchanting::Offer none;
        check(!Enchanting::affordable(none, 0, 99, 64, true), "an empty row cannot be bought at any price");

        check(Inventory::belongsIn(Inventory::ENCHANT_LAPIS, Items::LapisLazuli), "lapis goes in the lapis slot");
        check(!Inventory::belongsIn(Inventory::ENCHANT_LAPIS, Items::Diamond), "a diamond does not");
        check(Inventory::belongsIn(Inventory::BREW_FUEL, Items::BlazePowder), "blaze powder fuels a stand");
        check(!Inventory::belongsIn(Inventory::BREW_FUEL, Items::Coal), "coal does not");

        Inventory pack;
        pack.slot(0) = ItemStack{ Items::Coal, 5 };
        pack.leftClick(0);
        pack.leftClick(Inventory::BREW_FUEL);
        check(pack.slot(Inventory::BREW_FUEL).empty(), "coal dropped on the fuel slot stays on the cursor");
        pack.leftClick(0);
        pack.slot(1) = ItemStack{ Items::BlazePowder, 3 };
        pack.leftClick(1);
        pack.leftClick(Inventory::BREW_FUEL);
        check(pack.slot(Inventory::BREW_FUEL).id == Items::BlazePowder &&
                  pack.slot(Inventory::BREW_FUEL).count == 3,
              "blaze powder dropped on it goes in");

        check(Brewing::FUEL_PER_POWDER == 20, "one powder is twenty brews");
    }

    void testTheHand()
    {
        section("the hand: where Minecraft puts it");

        // An identity camera, so what comes back is camera space:
        // +x right, +y up, +z forwards.
        const glm::mat4 eye(1.0f);
        HeldItem hand;

        const glm::vec3 item = glm::vec3(hand.pose(eye, true)[3]);
        check(std::fabs(item.x - 0.56f) < 0.001f,
              "something held sits 0.56 out to the right, which is Mojang's 0.7 by 0.8");
        check(std::fabs(item.y + 0.52f) < 0.001f, "and 0.52 down");
        check(std::fabs(item.z - 0.72f) < 0.001f, "and 0.72 forward");

        const glm::vec3 fist = glm::vec3(hand.pose(eye, false)[3]);
        check(fist.x > item.x, "an empty hand sits further out than a full one");
        check(fist.y > item.y, "and higher");
        check(fist.z > item.z, "and further forward, which is where its fist lands");

        // The axes stay a proper frame however it is turned, or the
        // thing in it would be sheared.
        for (int step = 0; step <= 10; ++step)
        {
            hand.setSwingForTest(HeldItem::SWING_SECONDS * (1.0f - step / 10.0f));
            const glm::mat4 m = hand.pose(eye, true);
            for (int axis = 0; axis < 3; ++axis)
                check(std::fabs(glm::length(glm::vec3(m[axis])) - 1.0f) < 0.001f,
                      "the hand's axes stay unit length through a swing");
            check(std::fabs(glm::dot(glm::vec3(m[0]), glm::vec3(m[1]))) < 0.001f,
                  "and square to each other");
        }

        section("the hand: the swing and the swap");

        // Halfway through a swing the hand has gone left, up and
        // forward, which is Minecraft's arc: it pulls back across you
        // and punches out, rather than simply dropping.
        hand.setSwingForTest(0.0f);
        const glm::vec3 rest = glm::vec3(hand.pose(eye, true)[3]);
        hand.setSwingForTest(HeldItem::SWING_SECONDS * 0.5f);
        const glm::vec3 mid = glm::vec3(hand.pose(eye, true)[3]);
        check(mid.x < rest.x, "mid-swing the hand comes in towards the middle");
        check(mid.z > rest.z, "and out away from you");

        hand.setSwingForTest(0.0f);

        // And swapping what you carry drops it out of shot and brings
        // it back, which is the only cue Minecraft gives you.
        hand.setEquipForTest(0.0f);
        const glm::vec3 raising = glm::vec3(hand.pose(eye, true)[3]);
        check(std::fabs(raising.y - (rest.y - 0.6f)) < 0.001f,
              "something being raised starts a whole 0.6 lower");
        hand.setEquipForTest(1.0f);
        check(glm::length(glm::vec3(hand.pose(eye, true)[3]) - rest) < 0.001f,
              "and ends exactly where it rests");

        // The swap takes about an eighth of a second, which is
        // Minecraft's four tenths of the way per tick.
        hand.setEquipForTest(0.0f);
        float raised = 0.0f;
        for (int tick = 0; tick < 3; ++tick)
            raised = std::min(1.0f, raised + HeldItem::EQUIP_RATE * 0.05f);
        check(raised >= 1.0f, "three ticks is enough to raise it");
    }

    void testInterfaceScale()
    {
        section("the interface: how big it is drawn");

        // Minecraft's rule: step the scale up while the canvas it
        // leaves still holds 320 by 240, which is the smallest an
        // interface is ever laid out on.
        check(Gui::scaleFor(1280, 720) == 3, "a 1280 by 720 window draws at three");
        check(Gui::scaleFor(1920, 1080) == 4, "a 1920 by 1080 one at four");
        check(Gui::scaleFor(3840, 2160) == 9, "and a 4K one at nine");
        check(Gui::scaleFor(854, 480) == 2, "the smallest window Minecraft allows draws at two");
        check(Gui::scaleFor(640, 480) == 2, "480 tall is exactly two rows of 240");
        check(Gui::scaleFor(320, 240) == 1, "and a canvas-sized window at one");

        // Never nought, however small the window gets: a scale of
        // nought would divide the whole interface by zero.
        check(Gui::scaleFor(1, 1) == 1, "a window of nothing still draws at one");
        check(Gui::scaleFor(0, 0) == 1, "and so does a window of no size at all");

        // The setting pins it. Minecraft's own menu offers 1 to 4 and
        // an automatic setting, which is nought here.
        check(Gui::scaleFor(3840, 2160, 2) == 2, "the setting pins the scale down");
        check(Gui::scaleFor(3840, 2160, 0) == 9, "and nought means take what fits");
        check(Gui::scaleFor(640, 480, 4) == 2,
              "but a window too small for the setting still gets what fits");

        // The canvas never falls below the minimum, which is the whole
        // point of the rule: a panel 176 across has to fit on it.
        for (int width = 320; width <= 4096; width += 37)
            for (int height = 240; height <= 2304; height += 53)
            {
                const int scale = Gui::scaleFor(width, height);
                check(width / scale >= Gui::MIN_WIDTH && height / scale >= Gui::MIN_HEIGHT,
                      "the canvas is never smaller than an interface needs");
            }

        // And a bigger window never draws the interface smaller.
        int last = 1;
        for (int width = 320; width <= 4096; width += 64)
        {
            const int scale = Gui::scaleFor(width, width * 3 / 4);
            check(scale >= last, "a wider window never shrinks the interface");
            last = scale;
        }
    }

    void testTheShulker()
    {
        section("the shulker: a block that is alive");

        const MobType& shell = mobType(MobId::Shulker);

        check(shell.stationary, "a shulker is bolted where it stands");
        check(shell.walkSpeed == 0.0f, "it has no walking pace");
        check(shell.limbSwing == 0.0f, "and nothing on it swings");
        check(shell.arrowDamage > 0, "it fights by shooting");
        check(shell.attackDamage == 0, "and never by swinging, having no reach");

        // One in the whole game. Anything else that turned up bolted
        // down would be a species somebody forgot to give legs to.
        int bolted = 0;
        for (int i = 0; i < mobTypeCount(); ++i)
            if (mobType(static_cast<MobId>(i)).stationary) ++bolted;
        check(bolted == 1, "and it is the only one of its kind in the game");

        // The lid is its own part, so the renderer can lift it. Filed
        // as a head it turned to watch you, which is the one thing a
        // shulker does not do.
        int lids = 0, elsewhere = 0;
        for (int i = 0; i < mobTypeCount(); ++i)
            for (const MobBox& box : mobType(static_cast<MobId>(i)).model)
                if (box.part == Part::Lid)
                {
                    if (static_cast<MobId>(i) == MobId::Shulker) ++lids;
                    else ++elsewhere;
                }
        check(lids == 1, "the shulker has one lid");
        check(elsewhere == 0, "and nothing else in the game has one at all");

        section("the shulker: on the grid");

        // Wherever it is put down it ends up in the middle of the
        // block, so a row of them along a wall lines up.
        const glm::vec3 centred = Shulker::onGrid(glm::vec3(3.7f, 64.2f, -2.1f));
        check(centred.x == 3.5f, "it sits in the middle of its block across");
        check(centred.z == -2.5f, "and in the middle the other way");
        check(centred.y == 64.0f, "with its feet on the floor of it");
        check(Shulker::onGrid(centred) == centred, "and putting it down again moves nothing");

        // The same, through the mob itself.
        Mob sat(MobId::Shulker, glm::vec3(10.8f, 70.0f, 4.3f), 7u);
        check(sat.position() == glm::vec3(10.5f, 70.0f, 4.5f), "a shulker spawns on the grid");
        check(sat.yaw() == 0.0f, "facing the way every block faces");
        check(sat.peek() == 0.0f, "and shut");

        // Hitting it does not shove it off. Everything else in the
        // game is knocked back, which for a shulker would walk it off
        // its block one blow at a time.
        sat.damage(1, glm::vec3(1.0f, 0.0f, 0.0f));
        check(sat.position() == glm::vec3(10.5f, 70.0f, 4.5f), "a blow does not shift it");
        check(sat.velocity().x == 0.0f && sat.velocity().z == 0.0f,
              "and does not knock it back");
        check(sat.hurtFlash() > 0.0f, "though it felt it");

        section("the shulker: the lid");

        // Opening and shutting, at a pace you can watch.
        float peek = 0.0f;
        for (int step = 0; step < 60; ++step) peek = Shulker::peekTowards(peek, true, 1.0f / 60.0f);
        check(peek == 1.0f, "a second of asking opens it the whole way");
        peek = Shulker::peekTowards(peek, true, 1.0f);
        check(peek == 1.0f, "and it opens no further than open");

        for (int step = 0; step < 60; ++step) peek = Shulker::peekTowards(peek, false, 1.0f / 60.0f);
        check(peek == 0.0f, "leaving it alone shuts it again");
        peek = Shulker::peekTowards(peek, false, 1.0f);
        check(peek == 0.0f, "and it shuts no further than shut");

        check(Shulker::peekTowards(0.0f, true, 1.0f / 60.0f) < 0.1f,
              "one frame does not fling it open");

        // Mojang's curve: shut it sits flush, open it has risen a
        // whole block, and it never goes backwards on the way.
        check(std::fabs(Shulker::lidLift(0.0f)) < 0.001f, "a shut lid sits flush on the base");
        check(std::fabs(Shulker::lidLift(1.0f) - 16.0f) < 0.001f,
              "an open one has risen a whole block");

        float lastLift = -1.0f;
        for (int step = 0; step <= 20; ++step)
        {
            const float lift = Shulker::lidLift(step / 20.0f);
            check(lift >= lastLift - 0.001f, "the lid never drops on its way open");
            lastLift = lift;
        }

        // The lid clears the base as it goes, which is what makes the
        // head inside it visible. The base is eight pixels tall and
        // the lid starts four up it.
        check(4.0f + Shulker::lidLift(1.0f) > 8.0f, "an open lid is clear of the base");
        check(4.0f + Shulker::lidLift(0.0f) < 8.0f, "a shut one is down over it");

        // And the twist, which is Mojang's own: nothing at all until
        // the lid is properly moving, and a whole number of turns by
        // the time it is open, so it lands square.
        check(Shulker::lidTwist(0.0f) == 0.0f, "a shut lid is not turned");
        check(Shulker::lidTwist(0.2f) == 0.0f, "nor one barely cracked");
        check(Shulker::lidTwist(0.5f) > 0.0f, "halfway open it is turning");

        const float turns = Shulker::lidTwist(1.0f) / (2.0f * 3.14159265f);
        check(std::fabs(turns - std::round(turns)) < 0.001f,
              "and an open one has come round to square again");
    }

    void testDyes()
    {
        section("dye: the sixteen colours");

        // White is the wool that was in the game before any of this,
        // so colour nought has to be that block and not a new one --
        // every world ever saved has white wool in it under the old
        // number.
        check(woolOfColour(0) == Blocks::Wool, "white is the wool that was always here");

        for (int colour = 0; colour < DYE_COLOURS; ++colour)
        {
            const BlockId wool = woolOfColour(colour);
            const StackId dye = dyeOfColour(colour);
            const std::string which = std::to_string(colour);

            check(isWool(wool), "colour " + which + " is a wool");
            check(woolColour(wool) == colour, "and reads back as its own colour");
            check(isDye(dye), "colour " + which + " has a dye");
            check(dyeColour(dye) == colour, "and that dye reads back the same");
            check(blockDrop(wool) == wool, "wool " + which + " picks up as itself");
            check(isObtainable(wool), "and is a block you can hold");
            check(*displayName(dye), "dye " + which + " is named");
            check(*blockInfo(wool).name, "and so is the wool");
        }

        // Sixteen distinct wools and sixteen distinct dyes. A slip in
        // the arithmetic would land two colours on one block and the
        // test above would not notice.
        {
            std::set<BlockId> wools;
            std::set<StackId> dyes;
            for (int colour = 0; colour < DYE_COLOURS; ++colour)
            {
                wools.insert(woolOfColour(colour));
                dyes.insert(dyeOfColour(colour));
            }
            check(static_cast<int>(wools.size()) == DYE_COLOURS, "sixteen different wools");
            check(static_cast<int>(dyes.size()) == DYE_COLOURS, "and sixteen different dyes");
        }

        check(!isWool(Blocks::Planks), "a plank is not a wool");
        check(!isWool(Blocks::Stone), "nor is stone");
        check(!isDye(Items::Stick), "a stick is not a dye");
        check(woolOfColour(99) == Blocks::WoolBlack, "a colour cannot run off the end");

        section("dye: dyeing wool");

        // One wool and one dye, in any two cells, makes that colour.
        for (int colour = 0; colour < DYE_COLOURS; ++colour)
        {
            Inventory bench;
            bench.setCraftSize(3);
            bench.slot(Inventory::CRAFT_FIRST + 4) = ItemStack{ Blocks::Wool, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 8) = ItemStack{ dyeOfColour(colour), 1 };
            bench.refreshCraftResult();

            const ItemStack& made = bench.slot(Inventory::CRAFT_RESULT);
            check(made.id == woolOfColour(colour),
                  "dye " + std::to_string(colour) + " makes its own wool");
            check(made.count == 1, "one wool in, one wool out");
        }

        // Eight at a time, which is what a 3x3 grid holds beside the dye.
        {
            Inventory bench;
            bench.setCraftSize(3);
            for (int cell = 0; cell < 8; ++cell)
                bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ Blocks::Wool, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 8) = ItemStack{ Items::DyeRed, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Blocks::WoolRed,
                  "eight wools dye red");
            check(bench.slot(Inventory::CRAFT_RESULT).count == 8, "all eight of them");
        }

        {
            Inventory bench;
            bench.setCraftSize(3);
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Blocks::Wool, 9 };
            bench.slot(Inventory::CRAFT_FIRST + 8) = ItemStack{ Items::DyeRed, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).count == 1, "a stack of nine in one cell dyes one");
        }

        // Already-coloured wool dyes over, which is how a mistake is
        // undone.
        {
            Inventory bench;
            bench.setCraftSize(3);
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Blocks::WoolGreen, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 1) = ItemStack{ Items::DyeWhite, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Blocks::Wool,
                  "green wool dyes back to white");
        }

        // Two dyes and a wool is not a recipe, and nor is a wool, a dye
        // and something else in the grid.
        {
            Inventory bench;
            bench.setCraftSize(3);
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ Blocks::Wool, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 1) = ItemStack{ Items::DyeRed, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 2) = ItemStack{ Items::DyeBlue, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).empty(), "two dyes dye nothing");

            bench.slot(Inventory::CRAFT_FIRST + 2) = ItemStack{ Items::Stick, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).empty(), "and a stick spoils it");
        }

        section("dye: where the colours come from");

        // The six that are found rather than mixed.
        struct Source { StackId from; StackId dye; const char* what; };
        const Source SOURCES[] = {
            { Items::BoneMeal,       Items::DyeWhite,  "bone meal makes white" },
            { Blocks::FlowerYellow,  Items::DyeYellow, "a dandelion makes yellow" },
            { Blocks::FlowerRed,     Items::DyeRed,    "a poppy makes red" },
            { Items::LapisLazuli,    Items::DyeBlue,   "lapis makes blue" },
            { Items::InkSac,         Items::DyeBlack,  "an ink sac makes black" },
            { Items::CocoaBeans,     Items::DyeBrown,  "cocoa makes brown" },
        };
        for (const Source& source : SOURCES)
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST) = ItemStack{ source.from, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == source.dye, source.what);
        }

        // Green is smelted out of a cactus rather than crafted, which
        // is Minecraft's own odd one out.
        check(Smelting::resultOf(Blocks::Cactus) == Items::DyeGreen,
              "a cactus smelts into green");

        // And the nine that are mixed. Two in, two out, so mixing is
        // never a loss.
        struct Mix { StackId a; StackId b; StackId made; const char* what; };
        const Mix MIXES[] = {
            { Items::DyeRed,    Items::DyeYellow, Items::DyeOrange,    "red and yellow make orange" },
            { Items::DyeRed,    Items::DyeBlue,   Items::DyePurple,    "red and blue make purple" },
            { Items::DyeRed,    Items::DyeWhite,  Items::DyePink,      "red and white make pink" },
            { Items::DyeBlue,   Items::DyeGreen,  Items::DyeCyan,      "blue and green make cyan" },
            { Items::DyeBlue,   Items::DyeWhite,  Items::DyeLightBlue, "blue and white make light blue" },
            { Items::DyeGreen,  Items::DyeWhite,  Items::DyeLime,      "green and white make lime" },
            { Items::DyeBlack,  Items::DyeWhite,  Items::DyeGray,      "black and white make grey" },
            { Items::DyeGray,   Items::DyeWhite,  Items::DyeLightGray, "grey and white make light grey" },
            { Items::DyePurple, Items::DyePink,   Items::DyeMagenta,   "purple and pink make magenta" },
        };
        for (const Mix& mix : MIXES)
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST + 0) = ItemStack{ mix.a, 1 };
            bench.slot(Inventory::CRAFT_FIRST + 1) = ItemStack{ mix.b, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == mix.made, mix.what);
            check(bench.slot(Inventory::CRAFT_RESULT).count == 2, "and makes two of it");
        }

        // Every colour has to be reachable, or a wool nobody can make
        // is a wool that is not in the game. Six are found and nine are
        // mixed; green comes out of a furnace.
        for (int colour = 0; colour < DYE_COLOURS; ++colour)
        {
            const StackId dye = dyeOfColour(colour);
            bool made = dye == Items::DyeGreen;
            for (const Source& source : SOURCES) if (source.dye == dye) made = true;
            for (const Mix& mix : MIXES) if (mix.made == dye) made = true;
            check(made, "colour " + std::to_string(colour) + " can be got hold of");
        }

        section("dye: lapis");

        check(blockDrop(Blocks::LapisOre) == Items::LapisLazuli, "the seam gives up lapis");
        check(Tools::requiredLevel(Blocks::LapisOre) == 2, "and wants a stone pickaxe");
        check(!Tools::canHarvest(Items::WoodPickaxe, Blocks::LapisOre),
              "wood gets nothing out of it");
        check(Tools::canHarvest(Items::StonePickaxe, Blocks::LapisOre), "stone does");

        {
            Inventory bench;
            bench.setCraftSize(3);
            for (int cell = 0; cell < 9; ++cell)
                bench.slot(Inventory::CRAFT_FIRST + cell) = ItemStack{ Items::LapisLazuli, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Blocks::LapisBlock,
                  "nine lapis make a block");
        }
        {
            Inventory bench;
            bench.slot(Inventory::CRAFT_FIRST) = ItemStack{ Blocks::LapisBlock, 1 };
            bench.refreshCraftResult();
            check(bench.slot(Inventory::CRAFT_RESULT).id == Items::LapisLazuli,
                  "and the block gives the nine back");
            check(bench.slot(Inventory::CRAFT_RESULT).count == 9, "all nine of them");
        }

        section("dye: the squid");

        const MobType& squid = mobType(MobId::Squid);
        check(squid.drop == Items::InkSac, "a squid is where an ink sac comes from");
        check(squid.spawnClass == SpawnClass::Passive, "it attacks nobody");
        check(squid.aquatic, "water is where it lives");
        check(squid.hoverHeight > 0.0f, "and it swims rather than lying on the surface");

        // The same flag the guardian and the drowned were missing. A
        // mob that lives in water and is not marked for it cannot be
        // put anywhere it belongs: the spawn reads every water square
        // as a wall and quietly refuses, which is why the monument had
        // no guardians in it and the sea had no drowned.
        check(mobType(MobId::Guardian).aquatic, "a guardian lives in water too");
        check(mobType(MobId::Drowned).aquatic, "and so does a drowned");
        check(!mobType(MobId::Zombie).aquatic, "a zombie does not");

        // And the rule the spawn actually asks, rather than the flag
        // behind it. This is the check that was missing: the flag was
        // right and the question was never put.
        const MobId INHABITANTS[] = { MobId::Squid, MobId::Guardian, MobId::Drowned };
        for (MobId id : INHABITANTS)
        {
            const MobType& type = mobType(id);
            const std::string who = type.name;
            check(EntityManager::roomFor(type, Blocks::Water),
                  std::string(who) + " can be put in water");
            check(EntityManager::roomFor(type, Blocks::Air),
                  who + " can be put in air");
            check(!EntityManager::roomFor(type, Blocks::Lava), who + " cannot be put in lava");
            check(!EntityManager::roomFor(type, Blocks::Stone), who + " cannot be put in rock");
        }

        check(!EntityManager::roomFor(mobType(MobId::Zombie), Blocks::Water),
              "a zombie cannot be put in water");
        check(EntityManager::roomFor(mobType(MobId::Boat), Blocks::Water),
              "a boat can: it sits on top of it");
    }
}

int runSelfTest()
{
    std::printf("\n=== self test ===\n");
    testAdd();
    testClicks();
    testQuickMove();
    testInventoryShortcuts();
    testChestLids();
    testSoundCategories();
    testDripKinds();
    testBigTrees();
    testCaveAndSpikeFeatures();
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
    testEndermanStare();
    testAnvil();
    testShield();
    testWolves();
    testWoods();
    testIronGolem();
    testRiding();
    testBoats();
    testRails();
    testHeightStretch();
    testNether();
    testVillages();
    testVillageLoot();
    testVillageLife();
    testBrewing();
    testEffects();
    testEffectsOnThePlayer();
    testFarming();
    testRootCrops();
    testWorkstations();
    testMelonAndBamboo();
    testPumpkinsAndBees();
    testOcean();
    testIceAndSnow();
    testWiring();
    testFireAndSoul();
    testArchaeology();
    testStonecutter();
    testMaps();
    testBanners();
    testSculkExtras();
    testCreativeBlocks();
    testFishing();
    testArchery();
    testFoodsThatBite();
    testMaterials();
    testMobBuckets();
    testBooks();
    testVehicles();
    testSpawnEggs();
    testFireworkCrafting();
    testMusicDiscs();
    testPotionForms();
    testArmadillo();
    testGlowSquid();
    testFangsZoglinIllusioner();
    testPackSheetsAndLlamas();
    testPaintings();
    testArmorStands();
    testShieldBanners();
    testSignKinds();
    testMapMarks();
    testTrimPalettes();
    testNewEffects();
    testRecipeBook();
    testAdvancements();
    testStatistics();
    testWorlds();
    testServerList();
    testResourcePacks();
    testFonts();
    testWorldBorder();
    testAllBiomes();
    testBiomeColours();
    testMadeThings();
    testTheEndAndTheSea();
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
    testLadders();
    testFirstPersonArm();
    testHandSwing();
    testStructurePlacement();
    testFortressAndMineshaft();
    testLandmarks();
    testOceanStructures();
    testStructureLoot();
    testSeabed();
    testDesertWell();
    testSwamp();
    testEverythingIsReachable();
    testWitchPotions();
    testDragonFight();
    testWither();
    testBeacon();
    testRedstoneParts();
    testHopper();
    testGeodes();
    testAppleAndBoneMeal();
    testAncientCity();
    testOtherDimensionStructures();
    testSearchAgreesWithStamp();
    testCaves();
    testCopper();
    testPistons();
    testTrialChambers();
    testTrickyTrials();
    testRedstone();
    testBiomes();
    testOreRates();
    testDoors();
    testStairs();
    testPortalFrames();
    testExplosions();
    testArrows();
    testSleep();
    testChestStore();
    testLevelRoundTrip();
    testExperience();
    testSugarCane();
    testFluids();
    testEnchanting();
    testTrading();
    testEndPortal();
    testWeather();
    testEveryBlockHasAPicture();
    testEverySoundHasANoise();
    testShapeFamilies();
    testCarpetsWoodAndInfested();
    testSwitchesAndPlates();
    testFallingBlocks();
    testGameRules();
    testSnowLayers();
    testHeads();
    testDrowning();
    testQuickMoveKeepsMetadata();
    testContainersPerDimension();
    testDyes();
    testIllagers();
    testCaveCrowd();
    testNetherCrowd();
    testTheSea();
    testCountryAnimals();
    testNightVariants();
    testGolemsAndTheLast();
    testTheEnd();
    testTheShulker();
    testInterfaceScale();
    testTheHand();
    testArmourSlotsHoldArmour();
    testAttackCooldown();
    testGameModes();
    testBeaconPowers();
    testGrindstone();
    testWoodSets();
    testStoneSets();
    testColourBlocks();
    testCandles();
    testShulkerBoxes();
    testBeds();
    testChestKinds();
    testCopperSet();
    testPlants();
    testMushrooms();
    testLushPlants();
    testTheTablesTake121sPrices();

    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
