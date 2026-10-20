#include "Application.h"
#include "BuildTag.h"
#include "GLFunctions.h"
#include "Renderer/Screenshot.h"
#include "Entity/PlayerAnimation.h"
#include "Game/Farming.h"
#include "Game/Food.h"
#include "Game/Tools.h"
#include "Game/Armour.h"
#include "Game/Experience.h"
#include "Game/Cane.h"
#include "Game/Enchanting.h"
#include "Game/EndPortal.h"
#include "Game/Redstone.h"
#include "Game/Trial.h"
#include "Game/Piston.h"
#include "Game/Crafting.h"
#include "Game/Copper.h"
#include "Game/Sleep.h"
#include "Game/Explosion.h"
#include "Game/Portal.h"
#include "World/WorldSave.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h> // canvas sizing and device pixel ratio
#endif
#include <algorithm>
#include <cmath>
#include <iterator>
#include <cstdio>
#include <cctype>
#include <string>
#include <filesystem>

namespace
{
#ifdef __EMSCRIPTEN__
    constexpr int DEFAULT_RENDER_DISTANCE = 5; // single-threaded in the browser
#else
    constexpr int DEFAULT_RENDER_DISTANCE = 8;
#endif
    constexpr float REACH_DISTANCE = 5.0f;
    constexpr float BENCH_REACH = 6.0f;
    // A stick of TNT burns for four seconds, as in Minecraft, and goes
    // off harder and wider than a creeper.
    constexpr float TNT_FUSE_SECONDS = 4.0f;
    constexpr float TNT_RADIUS = 4.5f;
    constexpr int TNT_DAMAGE = 34;
    // Long enough that walking across a portal does not take you
    // through it, short enough not to feel like waiting.
    constexpr float PORTAL_SECONDS = 1.6f;
    // Minecraft lets you reach stone further than you can reach a mob.
    constexpr float ATTACK_REACH = 3.0f;
    constexpr float ATTACK_INTERVAL = 0.25f;
    constexpr int PUNCH_DAMAGE = 1;         // a bare fist, until there are tools
    constexpr float DAY_LENGTH_SECONDS = 1200.0f; // 20 minutes, like Minecraft
    constexpr float AUTOSAVE_INTERVAL = 60.0f;
    constexpr float MOUSE_SENSITIVITY = 0.11f;

    // Minecraft's own limits, and all the font can draw anyway.
    constexpr int MIN_NAME_LENGTH = 3;
    constexpr int MAX_NAME_LENGTH = 16;

    // The yellow line next to the logo. All written for this game --
    // Minecraft's own splashes are Mojang's.
    const char* SPLASHES[] = {
        "NO INSTALL REQUIRED!",
        "RUNS ON SCHOOL LAPTOPS!",
        "BUILT FROM SCRATCH!",
        "NOW WITH TREES!",
        "ZERO MEGABYTES OF JAVA!",
        "PRESS F3!",
        "BLOCKS ALL THE WAY DOWN!",
        "OPEN IT TO LAN!",
        "ALSO TRY MINING!",
        "SIXTY FOUR BY THIRTY TWO!",
        "MADE OF TRIANGLES, SECRETLY",
        "IT IS A WEBSITE!",
    };

    // Eaglercraft fills the username in for you rather than opening on an
    // empty field you cannot submit. Same idea.
    std::string randomPlayerName(uint32_t seed)
    {
        static const char* FIRST[] = { "BLOCK", "CUBE", "PIXEL", "STONE",
                                       "EMBER", "FROST", "MOSS", "CLAY" };
        static const char* SECOND[] = { "FAN", "HEAD", "WORKS", "PUNK",
                                        "SMITH", "WALKER", "DIGGER", "MAKER" };
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%s%s%02u",
                      FIRST[seed % std::size(FIRST)],
                      SECOND[(seed >> 5) % std::size(SECOND)],
                      (seed >> 11) % 100u);
        return buffer;
    }

    // Hotbar / inventory geometry, in pixels before UI scaling.
    constexpr float SLOT_SIZE = 46.0f;
    constexpr float SLOT_GAP = 4.0f;

    const glm::vec4 PANEL_COLOR(0.09f, 0.09f, 0.11f, 0.82f);

    // Minecraft's GUI palette, straight off the vanilla widgets. The
    // light panel is what every container screen is made of; the darker
    // well is a slot; buttons are their own grey.
    const glm::vec4 GUI_PANEL(0.776f, 0.776f, 0.776f, 1.0f);        // #C6C6C6
    const glm::vec4 GUI_PANEL_LIGHT(1.0f, 1.0f, 1.0f, 1.0f);        // #FFFFFF
    const glm::vec4 GUI_PANEL_DARK(0.333f, 0.333f, 0.333f, 1.0f);   // #555555
    const glm::vec4 GUI_WELL(0.545f, 0.545f, 0.545f, 1.0f);         // #8B8B8B
    const glm::vec4 GUI_WELL_DARK(0.216f, 0.216f, 0.216f, 1.0f);    // #373737
    const glm::vec4 GUI_TEXT(0.251f, 0.251f, 0.251f, 1.0f);         // #404040
    const glm::vec4 GUI_TEXT_GHOST(0.42f, 0.42f, 0.42f, 1.0f);

    const glm::vec4 BUTTON_FACE(0.424f, 0.424f, 0.424f, 1.0f);      // #6C6C6C
    const glm::vec4 BUTTON_FACE_HOVER(0.498f, 0.533f, 0.624f, 1.0f);
    const glm::vec4 BUTTON_FACE_OFF(0.290f, 0.290f, 0.290f, 1.0f);
    const glm::vec4 BUTTON_LIGHT(0.600f, 0.600f, 0.600f, 1.0f);
    const glm::vec4 BUTTON_DARK(0.196f, 0.196f, 0.196f, 1.0f);
    // Minecraft turns the label yellow under the cursor rather than
    // moving the button, which is the whole hover feedback.
    const glm::vec4 BUTTON_TEXT_HOVER(1.0f, 1.0f, 0.627f, 1.0f);    // #FFFFA0
    const glm::vec4 BUTTON_TEXT_OFF(0.627f, 0.627f, 0.627f, 1.0f);
    const glm::vec4 SLOT_COLOR(0.28f, 0.28f, 0.32f, 0.85f);
    const glm::vec4 SELECTED_COLOR(1.0f, 1.0f, 1.0f, 0.95f);
    const glm::vec4 TEXT_COLOR(0.96f, 0.96f, 0.96f, 1.0f);

    // For the things on a panel that are information rather than
    // choices: an offer you cannot afford, a count of shelves.
    const glm::vec4 TEXT_DIM(0.62f, 0.62f, 0.62f, 1.0f);
    const glm::vec4 DIM_TEXT(0.72f, 0.74f, 0.78f, 1.0f);

    // What a block turns into when you mine it.
    StackId blockDrop(BlockId id)
    {
        if (isLeaves(id)) return Blocks::Air;   // leaves drop nothing

        switch (id)
        {
            case Blocks::Grass:
            case Blocks::SnowGrass: return Blocks::Dirt;
            case Blocks::Stone: return Blocks::Cobblestone;
            case Blocks::Ice: return Blocks::Air;
            // Where wheat comes from, until there is farmland to grow it
            // on: grass gives up its seeds the way it does in Minecraft,
            // and three of them make a wheat in the crafting grid. That
            // second step is the stopgap and goes when farming lands.
            case Blocks::TallGrass: return Items::WheatSeeds;

            // Ore gives up what is in it. Iron and gold stay as ore --
            // they want a furnace, and there is not one yet.
            case Blocks::FurnaceLit: return Blocks::Furnace;
            case Blocks::CoalOre: return Items::Coal;
            case Blocks::DiamondOre: return Items::Diamond;
            case Blocks::EmeraldOre: return Items::Emerald;
            case Blocks::Glowstone: return Items::GlowstoneDust;
            case Blocks::NetherWart: return Items::NetherWart;
            case Blocks::RedstoneOre: return Items::Redstone;
            case Blocks::CopperOre: return Items::RawCopper;
            case Blocks::CopperBulbLit: return Blocks::CopperBulb;

            // Whatever state a thing was in when you broke it, what
            // comes up is the thing itself.
            case Blocks::RedstoneDust:
            case Blocks::RedstoneDustLit: return Items::Redstone;
            case Blocks::RedstoneTorchOff: return Blocks::RedstoneTorch;
            case Blocks::LeverOn: return Blocks::LeverOff;
            case Blocks::ButtonOn: return Blocks::ButtonOff;
            case Blocks::RedstoneLampLit: return Blocks::RedstoneLamp;

            default: break;
        }

        // Whichever way it was pointing and whether or not its head was
        // out, what comes up is a piston.
        if (isPiston(id))
            return pistonSticky(id) ? Blocks::StickyPistonNorth : Blocks::PistonNorth;

        return id;
    }
}

Application::Application(int windowWidth, int windowHeight)
    : m_window("Browsercraft", windowWidth, windowHeight)
    , m_atlas()
    , m_gui()
    , m_chunkShader("assets/shaders/chunk.vert", "assets/shaders/chunk.frag")
    , m_sky()
    , m_selection()
    , m_ui()
    , m_camera(glm::vec3(0.0f, 80.0f, 0.0f), -90.0f, 0.0f)
    , m_player(glm::vec3(0.0f, 80.0f, 0.0f))
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    rebuildSkinLibrary();

    // Stamped at compile time, so "did my build actually update?" is
    // answerable by looking rather than by guessing.
    std::printf("\n=== Browsercraft ===\n");
    std::printf("Build: %s, compiled %s %s\n", BUILD_TAG, __DATE__, __TIME__);
    std::printf("Controls are rebindable in Settings - Controls.\n\n");
}

Application::~Application()
{
    if (m_world) saveLevel();
}

namespace
{
    // Reads the real world for the cane rules, which take only the
    // handful of lookups they need rather than a whole World.
    struct WorldCaneBlocks : Cane::Blocks
    {
        const World* world = nullptr;
        BlockId at(int x, int y, int z) const override
        {
            return world->getBlock(x, y, z);
        }
    };

    // And for the ring of frames round an end portal.
    struct WorldEndFrames : EndPortal::Blocks
    {
        const World* world = nullptr;
        BlockId at(int x, int y, int z) const override
        {
            return world->getBlock(x, y, z);
        }
    };

    // The same, for counting the shelves round an enchanting table.
    struct WorldEnchantRoom : Enchanting::Surroundings
    {
        const World* world = nullptr;
        BlockId at(int x, int y, int z) const override
        {
            return world->getBlock(x, y, z);
        }
    };
}

void Application::setMouseCaptured(bool captured)
{
    m_window.setRelativeMouseMode(captured);
}

// ---------------------------------------------------------------- world ---

bool Application::saveExists() const
{
    std::error_code ec;
    return std::filesystem::exists(m_savePath + "/level.dat", ec);
}

void Application::deleteSavedWorld()
{
    std::error_code ec;
    std::filesystem::remove_all(m_savePath, ec);
    saveProfile(); // the name outlives any one world
}

void Application::rebuildSkinLibrary()
{
    m_skins.clear();
    for (int i = 0; i < PlayerSkin::variantCount(); ++i)
    {
        auto skin = std::make_unique<PlayerSkin>();
        skin->build(m_skinStyle, i);
        m_skins.push_back(std::move(skin));
    }

    // Your own PNG, offered alongside the painted ones when it is there.
    if (PlayerSkin::customAvailable())
        m_customSkin.build(m_skinStyle, PlayerSkin::CUSTOM_VARIANT);
}

const PlayerSkin& Application::activeSkin() const
{
    if (m_skinVariant == PlayerSkin::CUSTOM_VARIANT && m_customSkin.textureId())
        return m_customSkin;
    const int index = std::clamp(m_skinVariant, 0, static_cast<int>(m_skins.size()) - 1);
    return *m_skins[index];
}

void Application::flushSaveStorage()
{
#ifdef __EMSCRIPTEN__
    // Everything written so far lives in an in-memory filesystem that
    // the page throws away on reload; this is what makes it stick.
    EM_ASM({
        if (typeof FS !== 'undefined') FS.syncfs(false, function (err) {
            if (err) console.error('Save failed:', err);
        });
    });
#endif
}

void Application::saveBindings()
{
    m_keys.save(m_savePath);
    flushSaveStorage();
}

void Application::loadProfile()
{
    std::FILE* file = std::fopen((m_savePath + "/profile.txt").c_str(), "rb");
    if (!file) return;

    char buffer[64] = { 0 };
    const size_t read = std::fread(buffer, 1, sizeof(buffer) - 1, file);
    std::fclose(file);

    // Line 1 is the name, then the model type and the chosen character.
    // A file written before those existed is just line 1, and still loads.
    std::string text(buffer, read);
    const size_t firstBreak = text.find('\n');
    m_playerName = text.substr(0, firstBreak);
    while (!m_playerName.empty() && std::isspace(static_cast<unsigned char>(m_playerName.back())))
        m_playerName.pop_back();

    if (firstBreak == std::string::npos) return;

    int style = 0, variant = 0;
    if (std::sscanf(text.c_str() + firstBreak + 1, "%d %d", &style, &variant) == 2)
    {
        m_skinStyle = (style == 1) ? PlayerSkin::Style::Slim : PlayerSkin::Style::Classic;
        m_skinVariant = (variant == PlayerSkin::CUSTOM_VARIANT)
                            ? PlayerSkin::CUSTOM_VARIANT
                            : std::clamp(variant, 0, PlayerSkin::variantCount() - 1);
        rebuildSkinLibrary();
    }
}

void Application::saveProfile()
{
    std::error_code ec;
    std::filesystem::create_directories(m_savePath, ec);

    std::FILE* file = std::fopen((m_savePath + "/profile.txt").c_str(), "wb");
    if (!file) return;
    std::fprintf(file, "%s\n%d %d\n", m_playerName.c_str(),
                 m_skinStyle == PlayerSkin::Style::Slim ? 1 : 0, m_skinVariant);
    std::fclose(file);

#ifdef __EMSCRIPTEN__
    // Same story as the world: the name only survives a reload once it has
    // been flushed out of the in-memory filesystem into IndexedDB.
    EM_ASM({
        if (typeof FS !== 'undefined') FS.syncfs(false, function (err) {
            if (err) console.error('Profile save failed:', err);
        });
    });
#endif
}

void Application::startWorld(GameMode mode, bool freshWorld)
{
    if (freshWorld) deleteSavedWorld();

    uint32_t seed = m_hasSeedOverride ? m_seedOverride
                                      : static_cast<uint32_t>(SDL_GetPerformanceCounter() ^ 0x9E3779B9u);
    LevelState existing;
    const bool hasSave = !freshWorld && WorldSave::loadLevel(m_savePath, existing);
    if (hasSave && existing.seed != 0) seed = existing.seed;

    // Reset everything that belongs to a run.
    m_drops.clear();
    m_orbs.clear();
    m_rain.clear();
    m_particles.clear();
    m_entities.clear();
    m_animation.clear();
    m_nameplates.clear();
    m_inventory.clear();
    m_inventory.setSelectedSlot(0);
    m_worldReady = false;
    m_menuWorld = false;
    m_placeOnSurface = false;
    m_hardcoreDeath = false;
    m_readySeconds = 0.0f;
    m_autosaveTimer = 0.0f;
    m_breakProgress = 0.0f;
    m_hasTarget = false;

    m_gameMode = hasSave ? static_cast<GameMode>(existing.gameMode) : mode;
    m_player = Player(glm::vec3(0.0f, 80.0f, 0.0f));
    m_player.mode = m_gameMode;
    m_player.flying = modeIsCreative(m_gameMode);

    m_dimension = m_startupDimension;

    // The spawner has to be told as well. travelTo does it, but
    // starting straight in a dimension does not go through travelTo,
    // and the spawner left on the overworld there picked overworld
    // species: animals wanting grass underfoot, which neither
    // netherrack nor end stone is, so nothing spawned at all.
    m_entities.setDimension(m_dimension);
    m_chests.setDimension(static_cast<int>(m_dimension));
    m_furnaces.setDimension(static_cast<int>(m_dimension));

    m_world = std::make_unique<World>(seed, m_savePath, DEFAULT_RENDER_DISTANCE, m_dimension);
    if (m_dimension != Dimension::Overworld)
    {
        // Dev entry straight into another dimension: the overworld
        // spawn point means nothing there.
        m_player.position = glm::vec3(
            0.5f,
            m_dimension == Dimension::Nether
                ? static_cast<float>(WorldGen::NETHER_LAVA_LEVEL + 24)
                : static_cast<float>(WorldGen::END_ISLAND_LEVEL + 20),
            0.5f);
        m_placeOnSurface = true;
    }

    loadLevel();

    m_previousHealth = m_player.health;
    m_screen = Screen::Playing;
    setMouseCaptured(!m_automated);

    std::printf("%s world (%s, seed %u)\n", hasSave ? "Loaded" : "Created",
                gameModeName(m_gameMode), seed);
}

// A guest does not load the local save: the host's seed builds the same
// terrain, and the host then sends every block it has changed since.
void Application::startJoinedWorld()
{
    const Net::Handshake& handshake = m_net.handshake();
    m_joinedWorldBuilt = true;

    m_drops.clear();
    m_orbs.clear();
    m_rain.clear();
    m_particles.clear();
    m_entities.clear();
    m_animation.clear();
    m_nameplates.clear();
    m_inventory.clear();
    m_inventory.setSelectedSlot(0);
    m_worldReady = false;
    m_menuWorld = false;
    m_placeOnSurface = true;
    m_hardcoreDeath = false;
    m_readySeconds = 0.0f;
    m_autosaveTimer = 0.0f;
    m_breakProgress = 0.0f;
    m_hasTarget = false;

    m_gameMode = static_cast<GameMode>(handshake.gameMode);
    m_timeOfDay = handshake.timeOfDay;
    m_player = Player(handshake.spawn);
    m_player.mode = m_gameMode;
    m_player.flying = modeIsCreative(m_gameMode);

    // Scratch directory, wiped on every join: a guest must never touch
    // the local singleplayer save, and nothing here is worth keeping.
    const std::string guestPath = m_savePath + "_guest";
    std::error_code ec;
    std::filesystem::remove_all(guestPath, ec);
    m_world = std::make_unique<World>(handshake.seed, guestPath, DEFAULT_RENDER_DISTANCE);

    m_spawnPoint = handshake.spawn;
    m_camera.position = m_player.eyePosition();
    m_previousHealth = m_player.health;
    m_screen = Screen::Playing;
    setMouseCaptured(!m_automated);

    std::printf("Joined world (seed %u, %s)\n", handshake.seed, gameModeName(m_gameMode));
}

void Application::travelTo(Dimension dimension)
{
    if (!m_world || m_dimension == dimension) return;

    // Hand the chunks you changed back to disk before they are thrown
    // away -- otherwise stepping through a portal unbuilds your house.
    m_world->saveAll();

    m_dimension = dimension;
    m_entities.setDimension(dimension);

    // The containers are keyed by dimension as well as position, so
    // they are told too rather than cleared: a chest left behind in the
    // overworld still holds what was in it when you come back.
    m_chests.setDimension(static_cast<int>(dimension));
    m_furnaces.setDimension(static_cast<int>(dimension));

    m_entities.clear();
    m_worldReady = false;
    m_placeOnSurface = true;
    m_drops.clear();
    m_orbs.clear();
    m_rain.clear();
    m_particles.clear();

    m_world = std::make_unique<World>(m_world->seed(), m_savePath,
                                      DEFAULT_RENDER_DISTANCE, dimension);

    // The Nether is a slab with a ceiling, so the overworld's spawn
    // height would drop you inside bedrock; start mid-slab and let the
    // standing-spot search sort it out once chunks arrive.
    const float startY = (dimension == Dimension::Nether)
                             ? static_cast<float>(WorldGen::NETHER_LAVA_LEVEL + 24)
                             : static_cast<float>(WorldGen::END_ISLAND_LEVEL + 20);
    m_player.position = (dimension == Dimension::Overworld)
                            ? m_spawnPoint
                            : glm::vec3(0.5f, startY, 0.5f);
    m_player.velocity = glm::vec3(0.0f);
    m_camera.position = m_player.eyePosition();

    std::printf("Travelled to %s\n", dimensionName(dimension));
    std::fflush(stdout);
}

void Application::quitToTitle()
{
    m_net.leave();
    m_joinedWorldBuilt = false;
#ifndef __EMSCRIPTEN__
    m_discovery.stopAnnouncing();
#endif
    if (m_world && !m_menuWorld)
    {
        saveLevel();
        m_world.reset();
    }
    m_drops.clear();
    m_orbs.clear();
    m_rain.clear();
    m_particles.clear();
    m_entities.clear();
    m_animation.clear();
    m_nameplates.clear();
    m_perspective = Perspective::FirstPerson;
    m_screen = Screen::Title;
    m_inventoryOpen = false;
    m_timeOfDay = TITLE_TIME_OF_DAY;
    setMouseCaptured(false);
    startMenuWorld();
}

void Application::startMenuWorld()
{
    // A hand-picked seed and viewpoint, chosen with tools/--scout for
    // dramatic terrain and a bit of coastline, and checked to have ten
    // clear blocks around the camera so nothing sits in your face.
    constexpr uint32_t MENU_SEED = 1006713u;
    // Height matters more than it looks: the browser build only renders
    // three chunks out, and fog is complete by 46 blocks. Perched high
    // above the ground, everything in frame fell inside the fog band and
    // the panorama washed out to flat sky. Twelve blocks up keeps the
    // nearby terrain well inside the clear zone and still leaves the ten
    // blocks of clearance the viewpoint was chosen for.
    const glm::vec3 MENU_SPOT(300.5f, 99.0f, 120.5f);
    // The panorama only needs the scenery immediately around the camera,
    // and the browser build generates it on the main thread, so keep it small.
    // The same on both: at three chunks the fog reaches its full
    // strength before the scenery does, and the panorama renders as
    // blank sky. The menu generates this once and has nothing else to
    // spend time on.
    constexpr int MENU_RENDER_DISTANCE = 5;

    m_menuWorld = true;
    m_worldReady = true; // no player to drop in, so nothing to wait for
    m_timeOfDay = TITLE_TIME_OF_DAY;
    const uint32_t seed = m_hasPanoramaOverride ? m_panoramaSeed : MENU_SEED;
    m_world = std::make_unique<World>(seed, "menu", MENU_RENDER_DISTANCE);
    m_menuFocus = m_hasPanoramaOverride ? m_panoramaSpot : MENU_SPOT;

    updateMenuCamera(0.0f);
}

void Application::updateMenuCamera(float deltaTime)
{
    // Orbit the focus point, which reads as the world turning slowly.
    m_titleSpin += deltaTime * 2.2f; // degrees per second
    if (m_titleSpin > 360.0f) m_titleSpin -= 360.0f;

    // The camera stays put and turns on the spot, which is what
    // Minecraft's panorama does. Orbiting a focus point swung the
    // nearest terrain past the lens and made the menu feel unsteady.
    m_camera.position = m_menuFocus;
    m_camera.yaw = m_titleSpin;
    m_camera.pitch = -6.0f;
    m_camera.addLook(0.0f, 0.0f, 0.0f); // recompute the direction vectors
}

void Application::loadLevel()
{
    LevelState state;
    const bool loaded = WorldSave::loadLevel(m_savePath, state);

    int spawnX = 0, spawnY = 80, spawnZ = 0;
    m_world->generator().findSpawn(spawnX, spawnY, spawnZ);
    m_spawnPoint = glm::vec3(spawnX + 0.5f, static_cast<float>(spawnY), spawnZ + 0.5f);

    if (loaded)
    {
        // The spawn the world was actually started at, not the one the
        // generator would pick today. Without this, loading a world and
        // then dying dropped you somewhere you had never been.
        if (state.hasSpawn) m_spawnPoint = state.spawn;

        m_furnaces.clear();
        for (const LevelState::SavedFurnace& saved : state.furnaces)
        {
            Furnaces::State furnace;
            furnace.input.id = saved.input;
            furnace.input.count = saved.inputCount;
            furnace.fuel.id = saved.fuel;
            furnace.fuel.count = saved.fuelCount;
            furnace.output.id = saved.output;
            furnace.output.count = saved.outputCount;
            furnace.burnLeft = saved.burnLeft;
            furnace.burnTotal = saved.burnTotal;
            furnace.cooked = saved.cooked;
            const size_t which = static_cast<size_t>(&saved - state.furnaces.data());
            const int where = which < state.furnaceDimensions.size()
                                  ? state.furnaceDimensions[which] : 0;
            m_furnaces.putIn(where, glm::ivec3(saved.x, saved.y, saved.z), furnace);
        }

        // Only a change of state swaps the block, so one loaded already
        // alight would have sat there looking cold until its fire went
        // out. Put every one right once the chunks are up.
        m_pendingFurnaceLight = true;

        m_chests.clear();
        for (const LevelState::SavedChest& saved : state.chests)
        {
            Chests::Contents held{};
            for (size_t i = 0; i < saved.slots.size() && i < Chests::SLOTS; ++i)
            {
                held[i].id = saved.slots[i].first;
                held[i].count = saved.slots[i].second;
            }
            const size_t which = static_cast<size_t>(&saved - state.chests.data());
            const int where = which < state.chestDimensions.size()
                                  ? state.chestDimensions[which] : 0;
            m_chests.putIn(where, glm::ivec3(saved.x, saved.y, saved.z), held);
        }

        m_player.position = state.playerPosition;
        m_camera.yaw = state.yaw;
        m_camera.pitch = state.pitch;
        m_timeOfDay = state.timeOfDay;
        m_player.health = state.health;
        m_player.hunger = state.hunger;
        m_player.experience = state.experience;
        m_player.experienceOwedHundredths = state.experienceOwedHundredths;
        m_player.air = state.air;

        Effects::clear(m_player.effects);
        for (size_t i = 0; i < state.effectAmplifiers.size() &&
                           i < static_cast<size_t>(Effects::KIND_COUNT); ++i)
        {
            if (state.effectSeconds[i] <= 0.0f) continue;
            Effects::give(m_player.effects, static_cast<Effects::Kind>(i),
                          state.effectAmplifiers[i], state.effectSeconds[i]);
        }
        m_player.saturation = state.saturation;
        m_allowCheats = state.cheats;
        m_gameMode = static_cast<GameMode>(state.gameMode);
        m_player.mode = m_gameMode;
        m_inventory.setSelectedSlot(state.selectedSlot);

        for (size_t i = 0; i < state.inventory.size() && i < Inventory::TOTAL_SLOTS; ++i)
        {
            m_inventory.slot(static_cast<int>(i)).id = state.inventory[i].first;
            m_inventory.slot(static_cast<int>(i)).count = state.inventory[i].second;
            m_inventory.slot(static_cast<int>(i)).damage =
                i < state.damage.size() ? state.damage[i] : 0;
            m_inventory.slot(static_cast<int>(i)).enchantments =
                i < state.enchantments.size() ? state.enchantments[i] : 0u;
        }
    }
    else
    {
        // The spawn point is a place in the overworld. Starting straight
        // in another dimension has already put the player where that one
        // begins, and taking the spawn point here instead dropped them
        // into the End's void at the overworld's coordinates.
        if (m_dimension == Dimension::Overworld) m_player.position = m_spawnPoint;
        m_placeOnSurface = true;
        m_timeOfDay = 0.3f;
        // You start with nothing, in both modes: survival is meant to
        // begin by punching a tree, and creative has the whole palette
        // one keypress away.
    }

    m_camera.position = m_player.eyePosition();
}

void Application::saveLevel()
{
    if (!m_world) return;
    // A guest is a visitor in someone else's world; writing it to this
    // machine's save would overwrite their own singleplayer game.
    if (m_net.role() == Net::Role::Guest) return;

    LevelState state;
    state.seed = m_world->seed();
    state.playerPosition = m_player.position;
    state.yaw = m_camera.yaw;
    state.pitch = m_camera.pitch;
    state.timeOfDay = m_timeOfDay;
    state.health = m_player.health;
    state.hunger = m_player.hunger;
    state.experience = m_player.experience;
    state.experienceOwedHundredths = m_player.experienceOwedHundredths;
    state.air = m_player.air;

    state.effectAmplifiers.clear();
    state.effectSeconds.clear();
    for (int i = 0; i < Effects::KIND_COUNT; ++i)
    {
        const Effects::Kind kind = static_cast<Effects::Kind>(i);
        state.effectAmplifiers.push_back(m_player.effects.slots[i].amplifier);
        state.effectSeconds.push_back(m_player.effects.secondsLeft(kind));
    }
    state.saturation = m_player.saturation;
    state.cheats = m_allowCheats;
    state.gameMode = static_cast<uint8_t>(m_gameMode);
    state.dead = m_hardcoreDeath;
    state.selectedSlot = m_inventory.selectedSlot();
    state.spawn = m_spawnPoint;

    for (int i = 0; i < Inventory::TOTAL_SLOTS; ++i)
    {
        const ItemStack& stack = m_inventory.slot(i);
        state.inventory.emplace_back(stack.id, static_cast<uint16_t>(std::max(0, stack.count)));
        state.damage.push_back(static_cast<uint16_t>(std::clamp(stack.damage, 0, 65535)));
        state.enchantments.push_back(stack.enchantments);
    }

    // An open container's slots are the inventory's while its screen is
    // up, so what they hold is copied back before any of it is written
    // out -- copied, not stowed: stowing also shuts the screen, and an
    // autosave is no reason to close the chest somebody is using.
    syncOpenContainers();

    // The crafting grid and the enchanting table's slot belong to the
    // screen rather than to the player, so they are past the end of
    // what the loop above writes. Closing the screen hands their
    // contents back -- but quitting saves without closing, and that
    // used to lose whatever was sitting in them. They are tucked into
    // empty slots of the snapshot instead, which leaves the screen the
    // player is looking at exactly as it was.
    auto stash = [&](int slot) {
        const ItemStack& held = m_inventory.slot(slot);
        if (held.empty()) return;

        for (auto& entry : state.inventory)
        {
            const size_t index = static_cast<size_t>(&entry - state.inventory.data());
            if (entry.second != 0) continue;

            entry.first = held.id;
            entry.second = static_cast<uint16_t>(std::max(0, held.count));
            state.damage[index] = static_cast<uint16_t>(std::clamp(held.damage, 0, 65535));
            state.enchantments[index] = held.enchantments;
            return;
        }
    };

    stash(Inventory::ENCHANT_SLOT);
    for (int i = 0; i < Inventory::CRAFT_SLOTS; ++i) stash(Inventory::CRAFT_FIRST + i);

    for (const auto& entry : m_furnaces.all())
    {
        const Furnaces::State& furnace = entry.second;

        LevelState::SavedFurnace saved;
        saved.x = entry.first[1];
        saved.y = entry.first[2];
        saved.z = entry.first[3];
        state.furnaceDimensions.push_back(static_cast<uint8_t>(entry.first[0]));
        saved.input = furnace.input.id;
        saved.inputCount = static_cast<uint16_t>(std::max(0, furnace.input.count));
        saved.fuel = furnace.fuel.id;
        saved.fuelCount = static_cast<uint16_t>(std::max(0, furnace.fuel.count));
        saved.output = furnace.output.id;
        saved.outputCount = static_cast<uint16_t>(std::max(0, furnace.output.count));
        saved.burnLeft = furnace.burnLeft;
        saved.burnTotal = furnace.burnTotal;
        saved.cooked = furnace.cooked;
        state.furnaces.push_back(saved);
    }

    for (const auto& entry : m_chests.all())
    {
        LevelState::SavedChest saved;
        saved.x = entry.first[1];
        saved.y = entry.first[2];
        saved.z = entry.first[3];
        state.chestDimensions.push_back(static_cast<uint8_t>(entry.first[0]));
        for (const ItemStack& held : entry.second)
            saved.slots.emplace_back(held.id, static_cast<uint16_t>(std::max(0, held.count)));
        state.chests.push_back(saved);
    }

    WorldSave::saveLevel(m_savePath, state);
    m_world->saveAll();

#ifdef __EMSCRIPTEN__
    // In the browser the save lives in an in-memory filesystem that is
    // thrown away on reload. Flush it to IndexedDB so the world is still
    // there next visit. (Cookies are far too small for chunk data.)
    EM_ASM({
        if (typeof FS !== 'undefined') {
            FS.syncfs(false, function (err) {
                if (err) console.error('World save failed:', err);
            });
        }
    });
#endif
}

// ------------------------------------------------------------- lighting ---

glm::vec3 Application::sunDirection() const
{
    // timeOfDay 0.25 = sunrise, 0.5 = noon, 0.75 = sunset.
    const float angle = m_timeOfDay * 6.2831853f - 1.5707963f;
    return glm::normalize(glm::vec3(std::cos(angle), std::sin(angle), 0.22f));
}

// Runs the weather, and whatever the player can see and hear of it.
void Application::updateWeather(float deltaTime)
{
    // Only the overworld. The Nether has a roof rather than a sky, and
    // the End has no weather in Minecraft either -- and rain there
    // would fall out of a void onto islands with nothing above them.
    if (m_dimension != Dimension::Overworld)
    {
        m_weather = Weather::State{};
        m_rain.clear();
        return;
    }

    if (Weather::step(m_weather, deltaTime, m_weatherSeed))
    {
        // The flash is already in the sky by the time this returns; the
        // thunder is what the player hears a moment later. Playing them
        // together is wrong, and far enough apart is the one detail
        // that makes a storm feel like it has distance in it.
        m_audio.play(Sound::Thunder, 0.85f, 0.85f + (m_weatherSeed % 32u) * 0.01f);
    }

    // Snow rather than rain where it is cold enough, which is the one
    // thing about the weather that depends on where you are standing.
    const Biome biome = m_world->generator().biomeAt(
        static_cast<int>(std::floor(m_player.position.x)),
        static_cast<int>(std::floor(m_player.position.z)));
    m_snowing = (biome == Biome::Snowy);

    m_rain.update(deltaTime, m_player.position, m_weather.depth, m_snowing);

    if (m_weather.depth <= 0.05f)
    {
        m_rainSoundTimer = 0.0f;
        return;
    }

    // There is no looping voice in this engine, so the hiss is started
    // again a little before the clip would run out.
    m_rainSoundTimer -= deltaTime;
    if (m_rainSoundTimer > 0.0f) return;

    m_rainSoundTimer = 2.2f;
    m_audio.play(Sound::Rain, 0.30f * m_weather.depth, 1.0f);
}

float Application::daylightFactor() const
{
    const float angle = m_timeOfDay * 6.2831853f - 1.5707963f;
    const float sun = std::clamp(std::sin(angle) * 2.2f + 0.35f, 0.12f, 1.0f);

    // Weather dims the sun here rather than in each of the half-dozen
    // places that ask how bright it is. That is the whole reason it is
    // done here: the sky, the fog, the world's own lighting and the
    // rule about where monsters may appear all read this one number, so
    // a storm at noon lets the things that only come out at night come
    // out -- which is Minecraft's rule, and falls out rather than
    // having to be written down anywhere.
    const float weathered = sun * Weather::lightScale(m_weather);
    return std::clamp(weathered + Weather::flashBoost(m_weather), 0.0f, 1.0f);
}

// What the world is lit by as far as the eye is concerned, which is not
// the same number. Night vision belongs here and nowhere else: folded
// into daylightFactor it also told the mobs it was daytime, so a drink
// of it set every zombie on fire and kept the villagers out of bed.
float Application::viewDaylight() const
{
    return std::max(daylightFactor(), Effects::nightVisionFloor(m_player.effects));
}

// ----------------------------------------------------------------- loop ---

void Application::run()
{
    // An automated screenshot run must not grab the mouse or react to input:
    // it would steal clicks and keystrokes from whatever else is running.
    m_automated = m_autoScreenshotDelay > 0.0f;

    // Dev aid: --screen opens straight onto a given menu for screenshots.
    const bool menuOnly = (m_startupScreen == "title" || m_startupScreen == "modes" ||
                           m_startupScreen == "settings" || m_startupScreen == "multiplayer" ||
                           m_startupScreen == "controls" || m_startupScreen == "name");
    if (m_startupScreen == "title") m_screen = Screen::Title;
    if (m_startupScreen == "modes") m_screen = Screen::Singleplayer;
    if (m_startupScreen == "settings") m_screen = Screen::Settings;
    if (m_startupScreen == "multiplayer")
    {
        m_screen = Screen::Multiplayer;
#ifndef __EMSCRIPTEN__
        m_discovery.startListening();   // normally kicked off by the menu button
#endif
    }
    if (m_startupScreen == "controls") m_screen = Screen::Controls;
    if (m_startupScreen == "name") m_screen = Screen::NameEntry;
    if (m_startupScreen == "hardcore") m_startupMode = GameMode::Hardcore;
    if (m_startupScreen == "creative") m_startupMode = GameMode::Creative;

    // FPS in the captured frame, but not on top of a menu we are trying to read.
    if (m_automated && !menuOnly) m_showDebug = true;

    // A returning visitor keeps their name; a new one is asked for it
    // before anything else, the way the game asks on first launch.
    loadProfile();
    m_keys.load(m_savePath);
    if (m_playerName.empty() && !m_automated && m_startupScreen.empty())
        m_screen = Screen::NameEntry;
    m_nameDraft = m_playerName.empty()
                      ? randomPlayerName(static_cast<uint32_t>(SDL_GetPerformanceCounter()))
                      : m_playerName;

    if (!menuOnly && (m_automated || m_startImmediately))
    {
        startWorld(m_startupMode, m_startupScreen == "dead" || m_startupScreen == "hardcore");
    }
    else
    {
        setMouseCaptured(false);
        startMenuWorld();
    }

#ifndef __EMSCRIPTEN__
    if (m_hostOnStart && m_world)
    {
        const std::string host = m_playerName.empty() ? "HOST" : m_playerName;
        if (m_net.openToLan(host, m_world->seed(), static_cast<uint8_t>(m_gameMode),
                            m_timeOfDay, m_spawnPoint))
            m_discovery.startAnnouncing(host + "'S WORLD", m_net.port());
        std::printf("[net] %s\n", m_net.message().c_str());
        std::fflush(stdout);
    }
    else if (!m_joinOnStart.empty())
    {
        m_net.join(m_joinOnStart, Net::DEFAULT_PORT,
                   m_playerName.empty() ? "GUEST" : m_playerName);
        std::printf("[net] %s\n", m_net.message().c_str());
        std::fflush(stdout);
    }
#endif

    m_previousTicks = SDL_GetPerformanceCounter();

#ifdef __EMSCRIPTEN__
    // The browser owns the frame clock: hand it a callback instead of
    // blocking here, otherwise the page would simply freeze.
    emscripten_set_main_loop_arg(
        [](void* context) { static_cast<Application*>(context)->frame(); },
        this, 0, 1);
#else
    while (frame()) {}
#endif
}

bool Application::frame()
{
    if (!m_running) return false;

    const Uint64 now = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    float deltaTime = static_cast<float>((now - m_previousTicks) / frequency);
    m_previousTicks = now;
    // Clamp so a hitch (or a breakpoint) can't teleport the player
    // through the world on the next physics step.
    deltaTime = std::min(deltaTime, 0.1f);

    m_elapsedSeconds += deltaTime;
    m_fpsAccumulator += deltaTime;

    // Back the streamer off only when frames are genuinely running long.
    //
    // This used to subtract the frame time from a 16 ms target, which
    // looked reasonable and was badly wrong: deltaTime includes the wait
    // for vsync, so a perfectly healthy 60 Hz frame reports 16.6 ms and
    // the budget pinned itself to the 1 ms floor. The desktop never
    // noticed because worker threads do the generating, but the browser
    // build does it inline and the world stopped loading.
    if (m_world)
    {
        const double frameMs = static_cast<double>(deltaTime) * 1000.0;

        // The title panorama has no gameplay to protect and looks broken
        // until it has filled in, so it gets a much larger slice. During
        // play the budget backs off when frames run long.
        const double budget = !inGame() ? 14.0 : (frameMs > 24.0 ? 2.0 : 6.0);
        m_world->setStreamingBudget(budget);
    }
    ++m_framesThisSecond;
    if (m_fpsAccumulator >= 0.5f)
    {
        m_fps = m_framesThisSecond / m_fpsAccumulator;
        m_fpsAccumulator = 0.0f;
        m_framesThisSecond = 0;
    }

#ifdef __EMSCRIPTEN__
    // The page can resize the canvas at any time and SDL is not told about
    // it, so the viewport and the UI's idea of the screen drift apart --
    // menus end up drawn where the mouse isn't. Re-sync every frame.
    {
        double cssWidth = 0.0, cssHeight = 0.0;
        emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
        // Render at CSS resolution rather than device pixels: the HUD is
        // laid out in raw pixels, so a 2x backing store would halve the
        // apparent size of every button and heart. It also keeps the
        // single-threaded web build's fill rate down.
        const int targetWidth = std::max(1, static_cast<int>(cssWidth));
        const int targetHeight = std::max(1, static_cast<int>(cssHeight));

        if (targetWidth != m_window.width() || targetHeight != m_window.height())
        {
            emscripten_set_canvas_element_size("#canvas", targetWidth, targetHeight);
            m_window.setSize(targetWidth, targetHeight);
            glViewport(0, 0, targetWidth, targetHeight);
        }
    }
#endif

    handleEvents();
    if (!m_running) return false;

    // --- networking ---
    // Pumped before the world so an edit that arrives this frame is in
    // place by the time anything is drawn, and kept running while paused
    // so a host does not freeze everyone else by opening a menu.
    // Both are refreshed every frame rather than at each of the places
    // they can change: the profile screen, the title screen and two dev
    // flags can all start a session, and one missed call would put
    // somebody on screen wearing the wrong character.
    m_net.setLocalAppearance(localAppearance());
    m_net.setLocalSneaking(m_player.sneaking);
    m_net.update(deltaTime, m_world && !m_menuWorld ? m_world.get() : nullptr,
                 m_player.position, m_camera.yaw, m_camera.pitch, m_timeOfDay);

    // A guest gets the seed before it has the world; build it now,
    // replacing whatever was on screen.
    if (m_net.role() == Net::Role::Guest && m_net.handshake().received && !m_joinedWorldBuilt)
        startJoinedWorld();

    // Limbs keep moving while you are in a menu: everyone else is still
    // playing, and freezing them mid-stride is worse than not drawing
    // them at all.
    if (m_world && !m_menuWorld) updatePlayerAnimation(deltaTime);

#ifndef __EMSCRIPTEN__
    if (m_net.role() == Net::Role::Host) m_discovery.announce(deltaTime);
    if (m_screen == Screen::Multiplayer) m_discovery.update(deltaTime);
#endif

    if (m_world && !inGame())
    {
        // Title panorama: no player, just the camera circling the scenery.
        updateMenuCamera(deltaTime);
        m_world->update(m_camera.position);
        m_atlas.update(deltaTime);
    }
    else if (m_world)
    {
        if (!m_worldReady)
            waitForSpawnChunk();
        else if (m_automated)
        {
            // Let physics settle, but take no input. The world around
            // the player still has to run, or an automated screenshot
            // shows an empty one -- which is how the mob spawner came to
            // look broken when it was only ever being skipped.
            m_player.update(deltaTime, *m_world, Player::Controls{});
            m_camera.position = m_player.eyePosition();
            updateWorldAround(deltaTime);
        }
        else if (playing() && !m_inventoryOpen)
        {
            updateGameplay(deltaTime);
        }

        // The clock only runs while you're actually playing -- and on a
        // guest it is the host's clock, delivered by TimeSync.
        if (playing() && m_net.role() != Net::Role::Guest)
        {
            m_timeOfDay += deltaTime / DAY_LENGTH_SECONDS;
            if (m_timeOfDay >= 1.0f) m_timeOfDay -= 1.0f;
        }

        // Dev aid: a landmark the automated multiplayer test can look for.
        if (m_buildTest && m_worldReady && !m_buildTestDone)
        {
            m_buildTestDone = true;
            // Anchored to spawn, not to the host player: a guest arrives at
            // the spawn point, so this is where both of them can see it.
            const glm::ivec3 base(static_cast<int>(std::floor(m_spawnPoint.x)) + 4,
                                  static_cast<int>(std::floor(m_spawnPoint.y)),
                                  static_cast<int>(std::floor(m_spawnPoint.z)) + 4);
            int placed = 0;
            for (int x = 0; x < 4; ++x)
                for (int z = 0; z < 4; ++z)
                    for (int y = 0; y < 4; ++y)
                    {
                        const BlockId id = (y == 3) ? Blocks::Glowstone : Blocks::Planks;
                        m_world->setBlock(base.x + x, base.y + y, base.z + z, id);
                        m_net.onLocalBlockChange(base.x + x, base.y + y, base.z + z, id);
                        ++placed;
                    }
            std::printf("[test] stamped %d blocks at %d %d %d\n",
                        placed, base.x, base.y, base.z);
            std::fflush(stdout);
        }

        // Dev aid: a field sown at every stage at once, so one shot
        // shows what the crop looks like all the way through.
        if (m_farmTest && m_worldReady && !m_farmTestDone)
        {
            m_farmTestDone = true;

            const int baseY = static_cast<int>(std::floor(m_player.position.y));
            const int centreX = static_cast<int>(std::floor(m_player.position.x));
            const int centreZ = static_cast<int>(std::floor(m_player.position.z));

            // Flat ground to sow, and headroom to see it in.
            for (int x = centreX - 6; x <= centreX + 6; ++x)
                for (int z = centreZ - 14; z <= centreZ + 4; ++z)
                {
                    m_world->setBlock(x, baseY - 1, z, Blocks::Grass);
                    for (int y = baseY; y < baseY + 6; ++y)
                        m_world->setBlock(x, y, z, Blocks::Air);
                }

            // Eight rows, one per stage, plus a row left as bare tilled
            // earth so the farmland texture is visible too.
            int sown = 0;
            for (int stage = 0; stage < WHEAT_STAGES; ++stage)
                for (int z = centreZ - 2; z <= centreZ + 2; ++z)
                {
                    const glm::ivec3 ground(centreX - 4 + stage, baseY - 1, z);
                    if (!Farming::plant(*m_world, ground)) continue;
                    m_world->setBlock(ground.x, ground.y + 1, ground.z, wheatAtStage(stage));
                    ++sown;
                }

            m_world->setBlock(centreX + 4, baseY - 1, centreZ, Blocks::Farmland);

            m_inventory.add(Items::WheatSeeds, 32);

            m_player.position = glm::vec3(centreX + 0.5f, static_cast<float>(baseY),
                                          centreZ - 9.5f);
            m_player.velocity = glm::vec3(0.0f);
            m_camera.position = m_player.eyePosition();
            m_camera.setOrientation(90.0f, -22.0f);

            std::printf("[test] sowed %d crops across %d stages at y=%d\n",
                        sown, WHEAT_STAGES, baseY);
            std::fflush(stdout);
        }

        // Dev aid: one of every species, lined up in front of spawn and
        // facing the player, so a screenshot shows all eight at once
        // rather than waiting on the spawner to oblige.
        if (m_mobTest && m_worldReady && !m_mobTestDone)
        {
            m_mobTestDone = true;

            // A flat stone stage rather than whatever terrain happens to
            // be underfoot: spawn can land in a forest, and a model sheet
            // is no use with a tree in front of it. Deterministic for any
            // seed, which is the point of a check you repeat.
            const int baseY = static_cast<int>(std::floor(m_player.position.y));
            const int centreX = static_cast<int>(std::floor(m_player.position.x));
            const int centreZ = static_cast<int>(std::floor(m_player.position.z));
            const int halfWidth = mobTypeCount() + 2;

            for (int x = centreX - halfWidth; x <= centreX + halfWidth; ++x)
                for (int z = centreZ - 12; z <= centreZ + 4; ++z)
                {
                    m_world->setBlock(x, baseY - 1, z, Blocks::Stone);
                    for (int y = baseY; y < baseY + 8; ++y)
                        m_world->setBlock(x, y, z, Blocks::Air);
                }

            const int firstX = centreX - mobTypeCount();
            int placed = 0;
            for (int i = 0; i < mobTypeCount(); ++i)
            {
                const glm::vec3 feet(firstX + i * 2 + 0.5f, static_cast<float>(baseY),
                                     centreZ + 0.5f);
                if (m_entities.spawnAt(static_cast<MobId>(i), feet, *m_world)) ++placed;
            }

            // Stand back and look north along +z at the row.
            m_player.position = glm::vec3(centreX + 0.5f, static_cast<float>(baseY),
                                          centreZ - 10.5f);
            m_player.velocity = glm::vec3(0.0f);
            m_camera.position = m_player.eyePosition();
            m_camera.setOrientation(90.0f, -4.0f);

            // A breeding pair and a calf beside the line-up, plus wheat
            // in hand, so one screenshot covers the lot.
            const glm::vec3 pairAt(centreX + 3.5f, static_cast<float>(baseY), centreZ + 2.5f);
            m_entities.spawnAt(MobId::Cow, pairAt, *m_world);
            m_entities.spawnAt(MobId::Cow, pairAt + glm::vec3(1.2f, 0.0f, 0.0f), *m_world);
            m_entities.spawnAt(MobId::Cow, pairAt + glm::vec3(2.6f, 0.0f, 0.0f), *m_world, true);

            // Feed the pair here rather than waiting for a right-click,
            // so an automated run can show a calf being born.
            for (Mob& mob : m_entities.mobs())
                if (mob.typeId() == MobId::Cow && !mob.baby()) mob.feed(Items::Wheat);

            // A few on the ground too: a dropped item is drawn from its
            // own sprite rather than from block faces, and that is worth
            // seeing rather than assuming.
            const glm::vec3 litter(centreX - 2.5f, baseY + 1.0f, centreZ - 4.0f);
            m_drops.spawn(litter, Items::Wheat, 1);
            m_drops.spawn(litter + glm::vec3(0.8f, 0.0f, 0.0f), Items::Bone, 1);
            m_drops.spawn(litter + glm::vec3(1.6f, 0.0f, 0.0f), Items::Leather, 1);
            m_drops.spawn(litter + glm::vec3(2.4f, 0.0f, 0.0f), Blocks::Cobblestone, 1);

            m_inventory.add(Items::Wheat, 16);
            m_inventory.add(Items::Bread, 3);
            m_inventory.add(Items::Leather, 3);
            m_inventory.add(Items::Bone, 5);

            std::printf("[test] spawned %d of %d species on a stage at y=%d\n",
                        placed, mobTypeCount(), baseY);
            std::fflush(stdout);
        }

        // Keep streaming terrain even while paused so the world finishes loading.
        m_world->update(m_player.position);
        m_atlas.update(deltaTime);

        if (playing())
        {
            m_drops.update(deltaTime, *m_world, m_player, m_inventory, m_audio);

            // Orbs report what they were worth rather than touching the
            // player, so the one place experience is ever added stays
            // the one place the level-up chime is decided.
            if (const int gained = m_orbs.update(deltaTime, *m_world, m_player, m_audio))
            {
                if (m_player.addExperience(gained))
                    m_audio.play(Sound::Pickup, 0.7f, 0.55f);
            }
            m_particles.update(deltaTime, *m_world);
            updateArrows(deltaTime);
            updateFuses(deltaTime);
            updatePortal(deltaTime);

            // Dev aid: a steady stream, so a screenshot taken at any
            // moment has arrows in the air to look at.
            if (m_arrowDemo && m_arrows.all().size() < 10)
            {
                for (int i = -2; i <= 2; ++i)
                {
                    const glm::vec3 heading =
                        glm::normalize(m_camera.front + glm::vec3(0.0f, 0.12f, 0.0f) +
                                       m_camera.right * (static_cast<float>(i) * 0.1f));
                    m_arrows.spawn(m_camera.position + m_camera.front * 1.5f -
                                       m_camera.up * 0.3f,
                                   heading * 11.0f, 4, true);
                }
            }
        }

        if (playing() || m_inventoryOpen) tickFurnaces(deltaTime);

        if (playing()) tickTrialSpawners(deltaTime);
        if (playing()) tickCopper(deltaTime);

        // A pressed button lets go on its own, and everything it was
        // powering goes out with it.
        if (m_buttonTimer > 0.0f)
        {
            m_buttonTimer -= deltaTime;
            if (m_buttonTimer <= 0.0f)
            {
                m_buttonTimer = 0.0f;
                if (m_world &&
                    m_world->getBlock(m_buttonAt.x, m_buttonAt.y, m_buttonAt.z) ==
                        Blocks::ButtonOn)
                {
                    m_world->setBlock(m_buttonAt.x, m_buttonAt.y, m_buttonAt.z,
                                      Blocks::ButtonOff);
                    m_net.onLocalBlockChange(m_buttonAt.x, m_buttonAt.y, m_buttonAt.z,
                                             Blocks::ButtonOff);
                    settleRedstone(m_buttonAt);
                    m_audio.play(Sound::Click, 0.5f, 0.8f);
                }
            }
        }

        updateWeather(deltaTime);

        // Air. The head being under is the whole of what it needs to
        // know, so the rule itself has no idea there is a world.
        if (playing())
        {
            const bool under = m_player.isHeadUnderwater(*m_world);
            if (m_player.breathe(deltaTime, under))
                m_audio.play(Sound::Hurt, 0.7f, 0.9f);

            // Water puts it out, which is the whole reason to run for
            // the nearest lake with a blaze behind you.
            if (m_player.isInWater(*m_world)) m_player.douse();

            const bool wasAlight = m_player.onFire();
            if (m_player.burn(deltaTime, m_player.isInLava(*m_world)))
                m_audio.play(Sound::Hurt, 0.9f, 0.8f);
            if (m_player.onFire() && !wasAlight) m_audio.play(Sound::Fire, 0.7f);

            // Regeneration and poison pay out here, where the hurt
            // noise is to hand.
            const int before = m_player.health;
            if (m_player.updateEffects(deltaTime) && m_player.health < before)
                m_audio.play(Sound::Hurt, 0.5f, 1.2f);
        }

        // What is worn decides what a blow costs, and it can change
        // between one blow and the next.
        m_player.armourPoints = armourPoints();
        m_player.protection = armourProtection();

        // The big grid belongs to the bench, not to the player, so it
        // shuts as soon as they walk out of reach of it -- or mine it.
        // Trading ends if the villager wanders off, is killed, or you
        // walk away from it -- the same rule the bench follows, with a
        // mob in place of a block.
        if (m_tradeOpen && playing())
        {
            const Mob* villager = tradingVillager();
            if (!villager ||
                glm::length(m_player.position - villager->position()) > BENCH_REACH)
            {
                closeInventory();
                setMouseCaptured(true);
            }
        }

        if ((m_benchOpen || m_furnaceOpen || m_chestOpen || m_enchantOpen ||
             m_anvilOpen || m_brewOpen) && playing())
        {
            const glm::ivec3 block = m_benchOpen    ? m_benchBlock
                                   : m_chestOpen    ? m_chestBlock
                                   : m_enchantOpen  ? m_enchantBlock
                                   : m_anvilOpen    ? m_anvilBlock
                                   : m_brewOpen     ? m_brewBlock
                                                    : m_furnaceBlock;
            const glm::vec3 middle = glm::vec3(block) + glm::vec3(0.5f);
            const BlockId there = m_world->getBlock(block.x, block.y, block.z);

            const bool stillThere =
                m_benchOpen   ? there == Blocks::CraftingTable
              : m_chestOpen   ? there == Blocks::Chest
              : m_enchantOpen ? there == Blocks::EnchantingTable
              : m_anvilOpen   ? there == Blocks::Anvil
              : m_brewOpen    ? there == Blocks::BrewingStand
                              : (there == Blocks::Furnace || there == Blocks::FurnaceLit);

            if (!stillThere || glm::length(m_player.position - middle) > BENCH_REACH)
            {
                closeInventory();
                setMouseCaptured(true);
            }
        }

        // Death handling.
        if (playing() && m_player.health <= 0)
        {
            // Minecraft lets you drop what amounts to seven levels'
            // worth and keeps the rest, so dying deep into the curve
            // costs far more than dying early. respawn() clears the
            // total, so this has to happen first.
            const int dropped = std::min(7 * Experience::levelFor(m_player.experience), 100);
            if (dropped > 0)
                m_orbs.spawnValue(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), dropped);

            m_screen = Screen::Dead;
            m_hardcoreDeath = modeIsHardcore(m_gameMode);
            if (m_inventoryOpen) closeInventory();
            setMouseCaptured(false);
        }

        m_autosaveTimer += deltaTime;
        if (m_autosaveTimer >= AUTOSAVE_INTERVAL && playing() &&
            m_net.role() != Net::Role::Guest)
        {
            m_autosaveTimer = 0.0f;
            saveLevel();
        }
    }

    render();

    // Menu clicks are carried out here, once drawing is finished.
    applyMenuAction();

    // Dev aid: once the world is up, jump to the requested menu.
    if (!m_startupScreen.empty() && (m_worldReady || !m_world))
    {
        if (m_startupScreen == "inventory" || m_startupScreen == "creative" ||
            m_startupScreen == "bench" || m_startupScreen == "tools" ||
            m_startupScreen == "furnace" || m_startupScreen == "furnaceblock" ||
            m_startupScreen == "chest" || m_startupScreen == "armour" ||
            m_startupScreen == "armourhud" || m_startupScreen == "bed" ||
            m_startupScreen == "arrows" || m_startupScreen == "slabs" ||
            m_startupScreen == "boom" || m_startupScreen == "tnt" ||
            m_startupScreen == "portal" || m_startupScreen == "nether" ||
            m_startupScreen == "bucket" || m_startupScreen == "ladder" ||
            m_startupScreen == "stairs" || m_startupScreen == "fence" ||
            m_startupScreen == "door" || m_startupScreen == "hand" ||
            m_startupScreen == "handempty" || m_startupScreen == "xp" ||
            m_startupScreen == "shelf" || m_startupScreen == "enchant" ||
            m_startupScreen == "enchantblock" || m_startupScreen == "villager" ||
            m_startupScreen == "trade" || m_startupScreen == "endportal" ||
            m_startupScreen == "rain" || m_startupScreen == "storm" ||
            m_startupScreen == "pbr" || m_startupScreen == "anvil" ||
            m_startupScreen == "anvilblock" || m_startupScreen == "brew" ||
            m_startupScreen == "village" || m_startupScreen == "shield" ||
            m_startupScreen == "shieldup" ||
            m_startupScreen == "wolf" || m_startupScreen == "horse" ||
            m_startupScreen == "boat" || m_startupScreen == "rails" ||
            m_startupScreen == "mobs" || m_startupScreen == "golem" ||
            m_startupScreen == "nethermobs" ||
            m_startupScreen == "jungle" || m_startupScreen == "savanna" ||
            m_startupScreen == "redstone" || m_startupScreen == "chamber" ||
            m_startupScreen == "rock")
        {
            // Dev aid: open the inventory with something in it, so the
            // screenshot shows a real grid rather than 36 empty boxes.
            // "furnaceblock" is the exception: it wants the block in
            // view, not a panel in front of it.
            const bool showPanel = m_startupScreen != "furnaceblock" &&
                                  m_startupScreen != "armourhud" &&
                                  m_startupScreen != "bed" &&
                                  m_startupScreen != "arrows" &&
                                  m_startupScreen != "slabs" &&
                                  m_startupScreen != "boom" &&
                                  m_startupScreen != "ladder" &&
                                  m_startupScreen != "stairs" &&
                                  m_startupScreen != "fence" &&
                                  m_startupScreen != "door" &&
                                  m_startupScreen != "hand" &&
                                  m_startupScreen != "handempty" &&
                                  m_startupScreen != "xp" &&
                                  m_startupScreen != "shelf" &&
                                  m_startupScreen != "enchantblock" &&
                                  m_startupScreen != "villager" &&
                                  m_startupScreen != "endportal" &&
                                  m_startupScreen != "rain" &&
                                  m_startupScreen != "storm" &&
                                  m_startupScreen != "tnt" &&
                                  m_startupScreen != "pbr" &&
                                  m_startupScreen != "anvilblock" &&
                                  m_startupScreen != "village" &&
                                  m_startupScreen != "shield" &&
                                  m_startupScreen != "shieldup" &&
                                  m_startupScreen != "wolf" &&
                                  m_startupScreen != "horse" &&
                                  m_startupScreen != "boat" &&
                                  m_startupScreen != "rails" &&
                                  m_startupScreen != "mobs" &&
                                  m_startupScreen != "golem" &&
                                  m_startupScreen != "nethermobs" &&
                                  m_startupScreen != "jungle" &&
                                  m_startupScreen != "savanna" &&
                                  m_startupScreen != "redstone" &&
                                  m_startupScreen != "chamber" &&
                                  m_startupScreen != "rock" &&
                                  m_startupScreen != "portal" &&
                                  m_startupScreen != "nether";
            m_inventoryOpen = showPanel;
            setMouseCaptured(!showPanel);
            m_inventory.add(Blocks::Cobblestone, 64);
            m_inventory.add(Blocks::Planks, 37);
            m_inventory.add(Blocks::Torch, 12);
            m_inventory.add(Blocks::Glowstone, 3);
            m_inventory.add(Blocks::DiamondOre, 1);
            m_inventory.add(Blocks::Sand, 22);
            m_inventory.add(Blocks::CraftingTable, 1);

            if (m_startupScreen == "creative") m_creativeTab = 5;   // TOOLS
            if (m_startupScreen == "armour" || m_startupScreen == "armourhud")
            {
                m_creativeTab = 6;   // ARMOUR
                m_inventory.slot(Inventory::ARMOR_FIRST + 0).id = Items::DiamondHelmet;
                m_inventory.slot(Inventory::ARMOR_FIRST + 0).count = 1;
                m_inventory.slot(Inventory::ARMOR_FIRST + 1).id = Items::IronChestplate;
                m_inventory.slot(Inventory::ARMOR_FIRST + 1).count = 1;
                m_inventory.slot(Inventory::ARMOR_FIRST + 2).id = Items::GoldLeggings;
                m_inventory.slot(Inventory::ARMOR_FIRST + 2).count = 1;
                m_inventory.slot(Inventory::ARMOR_FIRST + 3).id = Items::LeatherBoots;
                m_inventory.slot(Inventory::ARMOR_FIRST + 3).count = 1;
                m_player.armourPoints = armourPoints();
            }

            if (m_startupScreen == "arrows") m_arrowDemo = true;

            // The armour HUD shot doubles as the one that shows every
            // row at once: hearts, armour, hunger, bubbles and the
            // experience bar, which is the only way to see whether the
            // left-hand and right-hand columns have grown into each
            // other.
            if (m_startupScreen == "armourhud")
            {
                m_player.air = Player::MAX_AIR_SECONDS * 0.45f;
                m_player.experience = Experience::totalForLevel(14) + 3;
            }

            if (m_startupScreen == "stairs")
            {
                m_camera.pitch = -14.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 6.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));

                // A flight climbing away from the camera, in each of the
                // four materials, plus one of each facing at the front.
                const BlockId KINDS[4] = { Blocks::StoneStairsNorth, Blocks::CobbleStairsNorth,
                                           Blocks::PlankStairsNorth, Blocks::SandstoneStairsNorth };

                for (int k = 0; k < 4; ++k)
                    for (int i = 0; i < 4; ++i)
                    {
                        for (int fill = 0; fill < i; ++fill)
                            m_world->setBlock(bx + k - 1, by + fill, bz - i, Blocks::Cobblestone);
                        m_world->setBlock(bx + k - 1, by + i, bz - i, stairsFacing(KINDS[k], 2));
                    }

                for (int f = 0; f < 4; ++f)
                    m_world->setBlock(bx + f - 1, by, bz + 2,
                                      stairsFacing(Blocks::CobbleStairsNorth, f));
            }

            if (m_startupScreen == "rain" || m_startupScreen == "storm")
            {
                Weather::begin(m_weather,
                               m_startupScreen == "storm" ? Weather::Kind::Storm
                                                          : Weather::Kind::Rain,
                               m_weatherSeed);
                // Already fallen rather than just beginning, so the
                // shot shows weather rather than a sky turning.
                m_weather.depth = 1.0f;
            }

            if (m_startupScreen == "endportal")
            {
                m_camera.pitch = -28.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 7.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y)) - 1;
                const int bz = static_cast<int>(std::floor(ahead.z));

                // A floor to stand the ring on, then eleven of the
                // twelve frames: the last is left empty so the shot
                // shows a ring that has not lit, and the eye in hand is
                // what would light it.
                for (int dx = -4; dx <= 4; ++dx)
                    for (int dz = -4; dz <= 4; ++dz)
                        m_world->setBlock(bx + dx, by, bz + dz, Blocks::EndStone);

                int offsets[EndPortal::FRAME_COUNT][2];
                EndPortal::frameOffsets(offsets);
                for (int i = 0; i < EndPortal::FRAME_COUNT; ++i)
                    m_world->setBlock(bx + offsets[i][0], by + 1, bz + offsets[i][1],
                                      (i == 0) ? Blocks::EndPortalFrame
                                               : Blocks::EndPortalFrameFilled);

                m_inventory.add(Items::EyeOfEnder, 4);
                m_inventory.add(Items::EnderPearl, 8);
            }

            if (m_startupScreen == "villager" || m_startupScreen == "trade")
            {
                m_camera.pitch = -4.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                // One of each profession, in a row, so the model and
                // all four trade lists can be seen in one shot.
                for (int i = 0; i < 4; ++i)
                {
                    const glm::vec3 where = m_player.position + m_camera.front * 2.5f +
                                            m_camera.right * (static_cast<float>(i) - 1.5f) * 1.6f;
                    m_entities.spawnAt(MobId::Villager, where, *m_world);
                }

                m_inventory.add(Items::Emerald, 32);
                m_inventory.add(Items::Wheat, 40);
                m_inventory.add(Items::Diamond, 4);

                // The first mob in the list need not be one of these:
                // the world has already spawned its own animals by now.
                if (m_startupScreen == "trade")
                {
                    for (const Mob& mob : m_entities.mobs())
                    {
                        if (mob.typeId() != MobId::Villager) continue;
                        openTrading(mob);
                        break;
                    }
                }
            }

            if (m_startupScreen == "enchant" || m_startupScreen == "enchantblock")
            {
                m_camera.pitch = -14.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                // Close enough to stay in reach: walk out of range and
                // the screen shuts itself, which is what happened the
                // first time this was put six blocks away.
                const glm::vec3 ahead = m_player.position + m_camera.front * 3.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));
                const int toward = (m_camera.front.z < 0.0f) ? 2 : -2;

                m_world->setBlock(bx, by, bz, Blocks::EnchantingTable);

                // A ring of shelves two cells out, with the gap left
                // clear, which is the arrangement that actually counts.
                // The row nearest the camera is left out so the table
                // itself can be seen rather than a wall of books.
                for (int dy = 0; dy <= 1; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                        for (int dz = -2; dz <= 2; ++dz)
                        {
                            if (std::max(std::abs(dx), std::abs(dz)) != 2) continue;
                            if (dz == toward) continue;
                            m_world->setBlock(bx + dx, by + dy, bz + dz, Blocks::Bookshelf);
                        }

                m_player.experience = Experience::totalForLevel(30);

                if (m_startupScreen == "enchant")
                {
                    ItemStack& slot = m_inventory.slot(Inventory::ENCHANT_SLOT);
                    slot.id = Items::DiamondPickaxe;
                    slot.count = 1;
                    openEnchantingTable(glm::ivec3(bx, by, bz));
                }
            }

            if (m_startupScreen == "horse")
            {
                m_camera.setOrientation(0.0f, -6.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                for (int ax = 0; ax <= 12; ++ax)
                    for (int az = -7; az <= 7; ++az)
                    {
                        for (int ay = 0; ay <= 5; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Grass);
                    }

                // Three of them, spread out. One horse wanders out of
                // frame before a screenshot can be taken: at six and a
                // half a second it crosses the whole clearing in two.
                for (int i = 0; i < 3; ++i)
                {
                    const glm::vec3 where(static_cast<float>(px + 5) + 0.5f,
                                          static_cast<float>(py),
                                          static_cast<float>(pz + (i - 1) * 3) + 0.5f);
                    if (m_entities.spawnAt(MobId::Horse, where, *m_world))
                        m_entities.mobs().back().tameWith(Items::Wheat);
                }

                m_inventory.add(Items::Wheat, 6);

                m_world->setBlock(px + 10, py, pz - 5, Blocks::Bookshelf);
                m_world->setBlock(px + 10, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 10, py, pz + 5, Blocks::EndPortalFrame);
            }

            if (m_startupScreen == "boat")
            {
                m_camera.setOrientation(0.0f, -22.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                // A pond two deep, cut into flat ground with a sand rim,
                // so the hulls have somewhere to sit and the waterline
                // is visible against something.
                for (int ax = -2; ax <= 16; ++ax)
                    for (int az = -9; az <= 9; ++az)
                    {
                        for (int ay = 0; ay <= 6; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);

                        const bool pond = ax >= 3 && ax <= 13 && az >= -6 && az <= 6;
                        m_world->setBlock(px + ax, py - 1, pz + az,
                                          pond ? Blocks::Water : Blocks::Grass);
                        m_world->setBlock(px + ax, py - 2, pz + az,
                                          pond ? Blocks::Water : Blocks::Dirt);
                        m_world->setBlock(px + ax, py - 3, pz + az, Blocks::Sand);
                    }

                for (int i = 0; i < 3; ++i)
                {
                    const glm::vec3 where(static_cast<float>(px + 4 + i * 3) + 0.5f,
                                          static_cast<float>(py) - 0.2f,
                                          static_cast<float>(pz + (i - 1) * 2) + 0.5f);
                    m_entities.spawnAt(MobId::Boat, where, *m_world);
                }

                // One on the bank as well, so the shot shows a hull
                // that no water is tinting.
                m_entities.spawnAt(MobId::Boat,
                                   glm::vec3(static_cast<float>(px + 5) + 0.5f,
                                             static_cast<float>(py),
                                             static_cast<float>(pz - 8) + 0.5f),
                                   *m_world);

                m_inventory.add(Items::Boat, 1);
                m_inventory.add(Blocks::Planks, 5);

                m_world->setBlock(px + 15, py, pz - 5, Blocks::Bookshelf);
                m_world->setBlock(px + 15, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 15, py, pz + 5, Blocks::EndPortalFrame);
            }

            if (m_startupScreen == "rails")
            {
                m_camera.setOrientation(0.0f, -34.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                for (int ax = -2; ax <= 18; ++ax)
                    for (int az = -10; az <= 10; ++az)
                    {
                        for (int ay = 0; ay <= 6; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Grass);
                    }

                // A closed loop, laid one rail at a time so the shaping
                // has to do its own work: every corner here is one the
                // game worked out, not one placed as a corner.
                const int x0 = px + 4, x1 = px + 10;
                const int z0 = pz - 3, z1 = pz + 3;
                for (int x = x0; x <= x1; ++x)
                {
                    m_world->setBlock(x, py, z0, Blocks::RailNorthSouth);
                    reshapeRailsAround(glm::ivec3(x, py, z0));
                    m_world->setBlock(x, py, z1, Blocks::RailNorthSouth);
                    reshapeRailsAround(glm::ivec3(x, py, z1));
                }
                for (int z = z0; z <= z1; ++z)
                {
                    m_world->setBlock(x0, py, z, Blocks::RailNorthSouth);
                    reshapeRailsAround(glm::ivec3(x0, py, z));
                    m_world->setBlock(x1, py, z, Blocks::RailNorthSouth);
                    reshapeRailsAround(glm::ivec3(x1, py, z));
                }

                m_entities.spawnAt(MobId::Minecart,
                                   glm::vec3(x0 + 2.5f, py + Mob::RAIL_LIFT, z1 + 0.5f),
                                   *m_world);
                m_entities.spawnAt(MobId::Minecart,
                                   glm::vec3(x0 + 0.5f, py + Mob::RAIL_LIFT, pz + 0.5f),
                                   *m_world);

                m_inventory.add(Items::Minecart, 1);
                m_inventory.add(Blocks::RailNorthSouth, 16);

                m_world->setBlock(px + 14, py, pz - 5, Blocks::Bookshelf);
                m_world->setBlock(px + 14, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 14, py, pz + 5, Blocks::EndPortalFrame);

                // Looking straight down at the loop, which is the only
                // view that says which way every rail is turned. On a
                // pad of its own: raised without one, the first version
                // of this shot fell twelve blocks and died.
                m_player.position = glm::vec3(px + 0.5f, static_cast<float>(py),
                                              pz + 0.5f);
                m_player.velocity = glm::vec3(0.0f);

                // The saved world these screens open carries whatever
                // health it was last left on, and a shot of the death
                // overlay is not a shot of the track.
                m_player.health = Player::MAX_HEALTH;
            }

            m_freezeMobs = m_startupScreen == "mobs" || m_startupScreen == "golem" ||
                           m_startupScreen == "nethermobs";

            if (m_startupScreen == "mobs")
            {
                m_camera.setOrientation(0.0f, -6.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                // The floor goes at the real surface of the player's own
                // column, not at the rounded-down spawn height: a spawn
                // that sits a block into the ground carved the arena
                // below the grass and left the camera in a trench.
                int py = static_cast<int>(std::floor(m_player.position.y));
                for (int probe = py + 8; probe >= py - 8; --probe)
                    if (isSolid(m_world->getBlock(px, probe, pz))) { py = probe + 1; break; }

                // Kept inside the chunks that are already generated when
                // a dev screen runs. A wider arena had its far half
                // written into chunks that did not exist yet, and
                // worldgen put the hillside back over the top of it.
                for (int ax = -2; ax <= 11; ++ax)
                    for (int az = -7; az <= 7; ++az)
                    {
                        for (int ay = 0; ay <= 10; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Grass);
                    }

                // One of everything, in a row across the view.
                // Midday, whatever the saved world was left on: half
                // these shots came back at dusk with the lineup in
                // silhouette.
                m_timeOfDay = 0.5f;

                const MobId LINEUP[] = {
                    MobId::Wolf, MobId::Horse, MobId::Villager, MobId::Breeze,
                    MobId::Pig, MobId::Sheep, MobId::Chicken,
                    MobId::Creeper, MobId::Cow, MobId::Skeleton,
                };
                // Two ranks of five, so ten of them fit the arena.
                int slot = 0;
                for (MobId id : LINEUP)
                {
                    const float x = static_cast<float>(px + (slot < 5 ? 5 : 8)) + 0.5f;
                    const float z = static_cast<float>(pz - 4 + (slot % 5) * 2) + 0.5f;
                    m_entities.spawnAt(id, glm::vec3(x, static_cast<float>(py), z), *m_world);
                    ++slot;
                }

                // The cart on its rail and the boat on its water, since
                // neither sits anywhere else.
                for (int az = 5; az <= 7; ++az)
                {
                    m_world->setBlock(px + 5, py, pz + az, Blocks::RailNorthSouth);
                    reshapeRailsAround(glm::ivec3(px + 5, py, pz + az));
                }
                m_entities.spawnAt(MobId::Minecart,
                                   glm::vec3(px + 5.5f, py + Mob::RAIL_LIFT, pz + 6.5f),
                                   *m_world);

                for (int ax = 7; ax <= 9; ++ax)
                    for (int az = 5; az <= 7; ++az)
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Water);
                m_entities.spawnAt(MobId::Boat,
                                   glm::vec3(px + 8.5f, py - 0.02f, pz + 6.5f),
                                   *m_world);

                m_world->setBlock(px + 10, py, pz - 5, Blocks::Bookshelf);
                m_world->setBlock(px + 10, py, pz - 3, Blocks::DiamondOre);
                m_world->setBlock(px + 10, py, pz - 1, Blocks::EndPortalFrame);

                // The player is left exactly where the world put them.
                // Moving them re-based the arena on the next run, and
                // three runs dug the camera eleven blocks into a hill.
                m_player.position.y = static_cast<float>(py);
                m_player.velocity = glm::vec3(0.0f);
                m_camera.position = m_player.eyePosition();

                // Creative, so a shot of ten mobs is not spoiled by one
                // of them being a creeper. Three screens in a row came
                // back as the death overlay before this went in.
                m_player.mode = GameMode::Creative;
                m_player.health = Player::MAX_HEALTH;
            }

            if (m_startupScreen == "golem")
            {
                m_camera.setOrientation(0.0f, -4.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                for (int ax = 0; ax <= 11; ++ax)
                    for (int az = -6; az <= 6; ++az)
                    {
                        for (int ay = 0; ay <= 6; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Grass);
                    }

                // A golem, a villager to stand it next to for scale,
                // and a zombie for it to object to.
                m_entities.spawnAt(MobId::IronGolem,
                                   glm::vec3(px + 8.5f, static_cast<float>(py), pz + 0.5f),
                                   *m_world);
                m_entities.spawnAt(MobId::Villager,
                                   glm::vec3(px + 8.5f, static_cast<float>(py), pz + 3.5f),
                                   *m_world);
                m_entities.spawnAt(MobId::Zombie,
                                   glm::vec3(px + 8.5f, static_cast<float>(py), pz - 3.5f),
                                   *m_world);

                m_world->setBlock(px + 10, py, pz - 5, Blocks::Bookshelf);
                m_world->setBlock(px + 10, py, pz - 1, Blocks::DiamondOre);
                m_world->setBlock(px + 10, py, pz + 5, Blocks::EndPortalFrame);

                m_player.mode = GameMode::Creative;
                m_player.health = Player::MAX_HEALTH;
            }

            if (m_startupScreen == "nethermobs")
            {
                m_camera.setOrientation(0.0f, -4.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                // A fortress floor, which is what you would be standing
                // on if you were meeting these three at once.
                for (int ax = 0; ax <= 13; ++ax)
                    for (int az = -7; az <= 7; ++az)
                    {
                        for (int ay = 0; ay <= 7; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::NetherBricks);
                    }

                m_entities.spawnAt(MobId::Blaze,
                                   glm::vec3(px + 8.5f, static_cast<float>(py), pz - 3.5f),
                                   *m_world);
                m_entities.spawnAt(MobId::WitherSkeleton,
                                   glm::vec3(px + 8.5f, static_cast<float>(py), pz + 0.5f),
                                   *m_world);
                m_entities.spawnAt(MobId::MagmaCube,
                                   glm::vec3(px + 8.5f, static_cast<float>(py), pz + 4.0f),
                                   *m_world);

                // Off to one side rather than behind them: the first
                // shot had a bookshelf showing between a blaze's rods.
                m_world->setBlock(px + 4, py, pz - 6, Blocks::Bookshelf);
                m_world->setBlock(px + 6, py, pz - 6, Blocks::DiamondOre);
                m_world->setBlock(px + 8, py, pz - 6, Blocks::EndPortalFrame);

                m_player.mode = GameMode::Creative;
                m_player.health = Player::MAX_HEALTH;
            }

            // Down into the nearest trial chamber, the same way the
            // village screen walks to a real village: what is worth
            // looking at is what the generator actually cut.
            if (m_startupScreen == "chamber")
            {
                const TrialChamber::Site site = m_world->generator().nearestChamber(
                    static_cast<int>(m_player.position.x),
                    static_cast<int>(m_player.position.z), 4);

                if (site.exists)
                {
                    // In the entrance room, under the shaft, looking
                    // along the row towards a spawner.
                    m_player.position = glm::vec3(static_cast<float>(site.x) + 0.5f,
                                                  static_cast<float>(site.y),
                                                  static_cast<float>(site.z) + 3.5f);
                    m_player.velocity = glm::vec3(0.0f);
                    m_player.flying = true;
                    m_placeOnSurface = false;
                    m_camera.setOrientation(-90.0f, -4.0f);
                    m_camera.addLook(0.0f, 0.0f, 0.0f);

                    std::printf("Trial chamber at %d, %d, floor %d\n", site.x, site.z, site.y);
                    std::fflush(stdout);

                    m_inventory.add(Items::TrialKey, 3);
                    m_inventory.add(Items::Mace, 1);
                    m_inventory.add(Items::WindCharge, 16);

                    // Nothing round the teleport has streamed in yet,
                    // so the props wait for the same world-ready gate
                    // the screenshot does.
                    m_pendingArena = true;
                    m_arenaKind = ArenaKind::ChamberProps;
                    m_arenaX = site.x;
                    m_arenaY = site.y;
                    m_arenaZ = site.z;
                }
                else
                {
                    std::printf("No trial chamber within four squares\n");
                    std::fflush(stdout);
                }

                m_player.mode = GameMode::Creative;
                m_player.health = Player::MAX_HEALTH;
            }

            if (m_startupScreen == "redstone")
            {
                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                // On the surface, whatever height the saved world left
                // the player at. Another dev screen had teleported them
                // into a trial chamber, and this one laid its circuit
                // out down there in the dark.
                const int py = m_world->generator().surfaceHeight(px, pz) + 1;

                // Wide enough to take the camera with it: the first
                // one cleared only the far side and put the lens inside
                // an acacia.
                for (int ax = -6; ax <= 15; ++ax)
                    for (int az = -8; az <= 10; ++az)
                    {
                        for (int ay = 0; ay <= 12; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Stone);
                    }

                // A lever on a block, a run of dust, and a lamp at the
                // end of it: the whole of what redstone does here.
                m_world->setBlock(px + 2, py, pz - 2, Blocks::Cobblestone);
                m_world->setBlock(px + 2, py + 1, pz - 2, Blocks::LeverOn);
                for (int ax = 3; ax <= 8; ++ax)
                    m_world->setBlock(px + ax, py, pz - 2, Blocks::RedstoneDust);
                m_world->setBlock(px + 9, py, pz - 2, Blocks::RedstoneLampLit);

                // And the same again with the switch left alone, so the
                // shot carries both states of every block in it.
                m_world->setBlock(px + 2, py, pz + 2, Blocks::Cobblestone);
                m_world->setBlock(px + 2, py + 1, pz + 2, Blocks::ButtonOff);
                for (int ax = 3; ax <= 8; ++ax)
                    m_world->setBlock(px + ax, py, pz + 2, Blocks::RedstoneDust);
                m_world->setBlock(px + 9, py, pz + 2, Blocks::RedstoneLamp);

                m_world->setBlock(px + 11, py, pz - 2, Blocks::RedstoneBlock);
                m_world->setBlock(px + 11, py, pz, Blocks::RedstoneTorch);
                m_world->setBlock(px + 11, py, pz + 2, Blocks::RedstoneOre);

                // A piston with a block in front of it and a block of
                // redstone behind: it is extended by the time the shot
                // is taken, because the power rule runs below.
                m_world->setBlock(px + 2, py, pz + 5, Blocks::RedstoneBlock);
                m_world->setBlock(px + 3, py, pz + 5, Blocks::PistonEast);
                m_world->setBlock(px + 5, py, pz + 5, Blocks::Cobblestone);

                // And one left alone, so both states are in the shot.
                m_world->setBlock(px + 8, py, pz + 5, Blocks::StickyPistonEast);
                m_world->setBlock(px + 9, py, pz + 5, Blocks::Planks);

                // A crafter with a lever of its own, loaded with four
                // planks: throw it and a bench comes out of the top.
                m_world->setBlock(px + 2, py, pz + 8, Blocks::Cobblestone);
                m_world->setBlock(px + 2, py + 1, pz + 8, Blocks::LeverOn);
                m_world->setBlock(px + 3, py, pz + 8, Blocks::Crafter);
                {
                    Chests::Contents& loaded = m_chests.at(glm::ivec3(px + 3, py, pz + 8));
                    for (int i : { 0, 1, 3, 4 })
                    {
                        loaded[i].id = Blocks::Planks;
                        loaded[i].count = 8;
                    }
                }

                // Copper, all four of its ages in a row with the ore
                // it came out of and a bulb at either end.
                {
                    const BlockId COPPER[] = { Blocks::CopperOre, Blocks::CopperBlock,
                                               Blocks::ExposedCopper, Blocks::WeatheredCopper,
                                               Blocks::OxidizedCopper, Blocks::CopperBulb,
                                               Blocks::CopperBulbLit };
                    int step = 0;
                    for (BlockId block : COPPER)
                        m_world->setBlock(px + 2 + step++ * 2, py, pz - 6, block);
                }

                m_world->setBlock(px + 4, py, pz + 6, Blocks::Bookshelf);
                m_world->setBlock(px + 7, py, pz + 6, Blocks::DiamondOre);
                m_world->setBlock(px + 10, py, pz + 6, Blocks::EndPortalFrame);

                // Run the power rule over both lines, which is what the
                // game does whenever anything is placed.
                settleRedstone(glm::ivec3(px + 5, py, pz - 2));
                settleRedstone(glm::ivec3(px + 5, py, pz + 2));
                settleRedstone(glm::ivec3(px + 4, py, pz + 5));
                settleRedstone(glm::ivec3(px + 3, py, pz + 8));

                // Up and back, looking down the two runs.
                m_player.position = glm::vec3(static_cast<float>(px) - 2.5f,
                                              static_cast<float>(py) + 4,
                                              static_cast<float>(pz) + 0.5f);
                m_placeOnSurface = false;
                m_player.velocity = glm::vec3(0.0f);
                m_player.flying = true;
                m_camera.setOrientation(0.0f, -28.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                m_player.mode = GameMode::Creative;
                m_player.health = Player::MAX_HEALTH;
            }

            if (m_startupScreen == "rock")
            {
                m_camera.setOrientation(0.0f, 0.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                // A gallery cut down the rock, so the whole column from
                // the upper stone to the deepslate is in one shot.
                const int floorY = 4;
                for (int ax = 0; ax <= 3; ++ax)
                    for (int az = -3; az <= 3; ++az)
                        for (int ay = floorY; ay <= floorY + 40; ++ay)
                            m_world->setBlock(px + ax, ay, pz + az, Blocks::Air);

                for (int ax = 0; ax <= 3; ++ax)
                    for (int az = -3; az <= 3; ++az)
                        m_world->setBlock(px + ax, floorY - 1, pz + az, Blocks::Glowstone);

                for (int ay = floorY; ay <= floorY + 36; ay += 6)
                    for (int az = -3; az <= 3; ++az)
                        m_world->setBlock(px + 4, ay, pz + az, Blocks::Glowstone);

                // Stood on a ledge part way up, so the shot takes in the
                // upper rock rather than the bottom of the shaft: left
                // in mid-air the player simply falls to the floor.
                const int ledge = floorY + 26;
                for (int az = -1; az <= 1; ++az)
                    m_world->setBlock(px + 1, ledge - 1, pz + az, Blocks::Glass);

                m_player.position = glm::vec3(px + 1.5f, static_cast<float>(ledge),
                                              pz + 0.5f);
                m_player.velocity = glm::vec3(0.0f);
                m_camera.position = m_player.eyePosition();
                m_player.mode = GameMode::Creative;
                m_player.health = Player::MAX_HEALTH;
            }

            if (m_startupScreen == "wolf")
            {
                m_camera.setOrientation(0.0f, -12.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                for (int ax = 0; ax <= 10; ++ax)
                    for (int az = -6; az <= 6; ++az)
                    {
                        for (int ay = 0; ay <= 4; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Grass);
                    }

                for (int i = 0; i < 3; ++i)
                {
                    const glm::vec3 where(static_cast<float>(px + 5) + 0.5f,
                                          static_cast<float>(py),
                                          static_cast<float>(pz + (i - 1) * 3) + 0.5f);
                    m_entities.spawnAt(MobId::Wolf, where, *m_world);
                }

                m_inventory.add(Items::Bone, 6);

                m_world->setBlock(px + 9, py, pz - 4, Blocks::Bookshelf);
                m_world->setBlock(px + 9, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 9, py, pz + 4, Blocks::EndPortalFrame);
            }

            if (m_startupScreen == "shield" || m_startupScreen == "shieldup")
            {
                m_inventory.slot(0).id = Items::Shield;
                m_inventory.slot(0).count = 1;
                m_inventory.setSelectedSlot(0);
                m_player.blocking = true;

                m_inventory.add(Items::IronIngot, 4);
                m_inventory.add(Blocks::Planks, 12);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));
                m_world->setBlock(px + 5, py, pz - 2, Blocks::Bookshelf);
                m_world->setBlock(px + 5, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 5, py, pz + 2, Blocks::EndPortalFrame);
            }

            if (m_startupScreen == "village")
            {
                // Walk to the nearest one rather than building a fake
                // village here: what is worth looking at is whether the
                // generator puts a real one together properly.
                const Village::Site site =
                    m_world->generator().nearestVillage(
                        static_cast<int>(m_player.position.x),
                        static_cast<int>(m_player.position.z), 4);

                if (site.exists)
                {
                    // Above the ground there, not at whatever height
                    // spawn happened to be: the surface search looks
                    // down from where the player already is, and from
                    // the old height it landed them in a cave.
                    // Back and well up, or the first tree between here
                    // and the village fills the whole screen.
                    const int standX = site.x;
                    const int standZ = site.z + 46;
                    const int top = m_world->generator().surfaceHeight(standX, standZ);
                    m_player.position = glm::vec3(static_cast<float>(standX) + 0.5f,
                                                  static_cast<float>(top + 16),
                                                  static_cast<float>(standZ) + 0.5f);
                    m_player.velocity = glm::vec3(0.0f);
                    m_player.flying = true;
                    m_placeOnSurface = false;
                    m_camera.setOrientation(-90.0f, -22.0f);
                    m_camera.addLook(0.0f, 0.0f, 0.0f);
                    // What is actually in it is --villageinfo's job:
                    // it generates the chunks outright instead of
                    // waiting on a running game to stream them in.
                    std::printf("Village at %d, %d\n", site.x, site.z);
                    std::fflush(stdout);
                }
                else
                {
                    std::printf("No village within four squares\n");
                    std::fflush(stdout);
                }
            }

            // Walk to the nearest jungle or savanna, the same way the
            // village screen walks to a real village: what is worth
            // looking at is what the generator actually makes, not an
            // arena with six trees planted in it.
            if (m_startupScreen == "jungle" || m_startupScreen == "savanna")
            {
                const Biome wanted = m_startupScreen == "jungle" ? Biome::Jungle
                                                                 : Biome::Savanna;
                const int fromX = static_cast<int>(m_player.position.x);
                const int fromZ = static_cast<int>(m_player.position.z);

                bool found = false;
                int standX = fromX, standZ = fromZ;

                // Outward in rings, so the first hit is the nearest one
                // and the shot is of somewhere you could have walked to.
                for (int ring = 0; ring <= 160 && !found; ++ring)
                    for (int step = -ring; step <= ring && !found; ++step)
                        for (int side = 0; side < 4 && !found; ++side)
                        {
                            const int dx = (side == 0) ? step : (side == 1) ? step
                                         : (side == 2) ? -ring : ring;
                            const int dz = (side == 0) ? -ring : (side == 1) ? ring : step;
                            const int x = fromX + dx * 16;
                            const int z = fromZ + dz * 16;
                            // The middle of one, not its edge. The first
                            // try took the first matching column and put
                            // the camera in a forest with one jungle
                            // block under its feet.
                            const WorldGen& gen = m_world->generator();
                            if (gen.biomeAt(x, z) != wanted) continue;
                            if (gen.biomeAt(x - 24, z) != wanted) continue;
                            if (gen.biomeAt(x + 24, z) != wanted) continue;
                            if (gen.biomeAt(x, z - 24) != wanted) continue;
                            if (gen.biomeAt(x, z + 24) != wanted) continue;
                            standX = x;
                            standZ = z;
                            found = true;
                        }

                if (found)
                {
                    // On the ground, not above it: from over the top a
                    // jungle is a green carpet, and the trees are the
                    // whole point of the shot.
                    const int top = m_world->generator().surfaceHeight(standX, standZ);
                    m_player.position = glm::vec3(static_cast<float>(standX) + 0.5f,
                                                  static_cast<float>(top + 1),
                                                  static_cast<float>(standZ) + 0.5f);
                    m_player.velocity = glm::vec3(0.0f);
                    m_player.flying = true;
                    m_placeOnSurface = false;
                    m_camera.setOrientation(0.0f, 6.0f);
                    m_camera.addLook(0.0f, 0.0f, 0.0f);

                    // The clearing itself waits: nothing round here
                    // has streamed in yet, and a setBlock into a chunk
                    // that does not exist is dropped on the floor.
                    m_pendingArena = true;
                    m_arenaKind = ArenaKind::BiomePocket;
                    m_arenaX = standX;
                    m_arenaZ = standZ;

                    std::printf("%s at %d, %d\n", biomeName(wanted), standX, standZ);
                    std::fflush(stdout);
                }
                else
                {
                    std::printf("No %s within a hundred and sixty chunks\n", biomeName(wanted));
                    std::fflush(stdout);
                }

                m_player.mode = GameMode::Creative;
                m_player.health = Player::MAX_HEALTH;
            }

            if (m_startupScreen == "brew")
            {
                m_camera.setOrientation(0.0f, -14.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                for (int ax = 0; ax <= 12; ++ax)
                    for (int az = -7; az <= 7; ++az)
                    {
                        for (int ay = 0; ay <= 5; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Planks);
                    }

                m_world->setBlock(px + 3, py, pz, Blocks::BrewingStand);
                // A second one out where the panel does not cover it,
                // since the one being used is behind the screen.
                m_world->setBlock(px + 5, py, pz - 3, Blocks::BrewingStand);

                // The three that every scene here carries.
                m_world->setBlock(px + 8, py, pz - 4, Blocks::Bookshelf);
                m_world->setBlock(px + 8, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 8, py, pz + 4, Blocks::EndPortalFrame);

                // Sugar on top of three water bottles: the moment just
                // before three potions of swiftness.
                ItemStack& dust = m_inventory.slot(Inventory::BREW_INGREDIENT);
                dust.id = Items::Sugar;
                dust.count = 5;

                for (int i = 0; i < Inventory::BREW_BOTTLES; ++i)
                {
                    ItemStack& bottle = m_inventory.slot(Inventory::BREW_BOTTLE_FIRST + i);
                    bottle.id = Items::Potion;
                    bottle.count = 1;
                    bottle.damage = Brewing::toDamage(Brewing::Potion::Awkward);
                }

                m_inventory.add(Items::NetherWart, 7);
                m_inventory.add(Items::GlassBottle, 3);
                m_inventory.add(Items::BlazePowder, 4);
                m_inventory.add(Items::GlowstoneDust, 6);
                m_inventory.add(Blocks::BrewingStand, 1);

                openBrewingStand(glm::ivec3(px + 3, py, pz));
                m_brewProgress = Brewing::BREW_SECONDS * 0.45f;
            }

            if (m_startupScreen == "anvil" || m_startupScreen == "anvilblock")
            {
                m_camera.setOrientation(0.0f, -14.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                // A clearing, or the grass swallows a block as low as an
                // anvil and the bank behind buries everything else.
                for (int ax = 0; ax <= 12; ++ax)
                    for (int az = -7; az <= 7; ++az)
                    {
                        for (int ay = 0; ay <= 5; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, Blocks::Planks);
                    }

                m_world->setBlock(px + 3, py, pz, Blocks::Anvil);

                // The three that every scene here carries, set wide
                // enough apart to clear the open panel between them.
                m_world->setBlock(px + 8, py, pz - 4, Blocks::Bookshelf);
                m_world->setBlock(px + 8, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 8, py, pz + 4, Blocks::EndPortalFrame);

                m_player.experience = Experience::totalForLevel(30);

                // A worn, enchanted pickaxe on the left and a book of
                // unbreaking on the right: the pairing the anvil exists
                // for, and the one with a price worth showing.
                ItemStack& left = m_inventory.slot(Inventory::ANVIL_LEFT);
                left.id = Items::DiamondPickaxe;
                left.count = 1;
                left.damage = Anvil::maxDurability(Items::DiamondPickaxe) * 3 / 4;
                left.enchantments = Enchanting::withLevel(0u, Enchanting::Kind::Efficiency, 3);

                ItemStack& right = m_inventory.slot(Inventory::ANVIL_RIGHT);
                right.id = Items::Book;
                right.count = 1;
                right.enchantments =
                    Enchanting::withLevel(0u, Enchanting::Kind::Unbreaking, 3);

                m_inventory.add(Items::IronIngot, 12);
                m_inventory.add(Blocks::Anvil, 1);

                // "anvilblock" shows the block in the world instead, the
                // way "furnaceblock" does: a panel in front of it is no
                // use for seeing what it looks like.
                if (m_startupScreen == "anvil") openAnvil(glm::ivec3(px + 3, py, pz));
                else { m_inventoryOpen = false; setMouseCaptured(false); }
            }

            if (m_startupScreen == "pbr")
            {
                // A flat run of materials with the sun low enough to rake
                // across them. Normal mapping shows up at a grazing angle
                // and almost nowhere else, so judging it off a wall seen
                // face-on says nothing either way.
                m_timeOfDay = 0.36f;
                m_camera.setOrientation(0.0f, -20.0f);
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const int px = static_cast<int>(std::floor(m_player.position.x));
                const int py = static_cast<int>(std::floor(m_player.position.y));
                const int pz = static_cast<int>(std::floor(m_player.position.z));

                const int strip[6] = { Blocks::Cobblestone, Blocks::Stone, Blocks::Bricks,
                                       Blocks::Planks, Blocks::Gravel, Blocks::Sandstone };

                for (int ax = 0; ax <= 20; ++ax)
                    for (int az = -10; az <= 10; ++az)
                    {
                        for (int ay = 0; ay <= 8; ++ay)
                            m_world->setBlock(px + ax, py + ay, pz + az, Blocks::Air);
                        m_world->setBlock(px + ax, py - 1, pz + az, strip[((az + 12) / 4) % 6]);
                    }

                for (int ay = 0; ay <= 5; ++ay)
                    for (int az = -10; az <= 10; ++az)
                        m_world->setBlock(px + 21, py + ay, pz + az, strip[((az + 12) / 4) % 6]);

                // The three the eye judges this by: shelf planks for
                // relief, diamond for the specular, frame for both.
                m_world->setBlock(px + 5, py, pz - 3, Blocks::Bookshelf);
                m_world->setBlock(px + 5, py, pz,     Blocks::DiamondOre);
                m_world->setBlock(px + 5, py, pz + 3, Blocks::EndPortalFrame);
            }

            if (m_startupScreen == "shelf")
            {
                m_camera.pitch = -6.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);
                const glm::vec3 ahead = m_player.position + m_camera.front * 5.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));
                for (int a = -2; a <= 2; ++a)
                    for (int b = 0; b <= 2; ++b)
                        m_world->setBlock(bx + a, by + b, bz, Blocks::Bookshelf);
                m_inventory.add(Blocks::SugarCane, 9);
                m_inventory.add(Items::Paper, 6);
                m_inventory.add(Items::Book, 3);
                m_inventory.add(Blocks::Bookshelf, 4);
            }

            if (m_startupScreen == "xp")
            {
                // Enough for a two-figure level, so the bar's numerals
                // and its fill are both on show, plus a scatter of orbs
                // out of reach of the player so they are not swept up
                // before the shot is taken.
                m_player.experience = Experience::totalForLevel(27) +
                                      Experience::toNextLevel(27) / 3;
                for (int i = 0; i < 9; ++i)
                {
                    const glm::vec3 where = m_player.position +
                                            m_camera.front * (7.0f + i * 0.7f) +
                                            m_camera.right * (static_cast<float>(i % 3) - 1.0f) * 1.1f +
                                            glm::vec3(0.0f, 1.0f, 0.0f);
                    m_orbs.spawnOne(where, Experience::ORB_VALUES[10 - i]);
                }
            }

            if (m_startupScreen == "hand" || m_startupScreen == "handempty")
            {
                m_inventory.clear();
                if (m_startupScreen == "hand")
                {
                    m_inventory.add(Blocks::Cobblestone, 1);
                    m_inventory.setSelectedSlot(0);
                }
            }

            if (m_startupScreen == "door")
            {
                m_camera.pitch = -4.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 6.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));

                // A wall with two doorways in it: one shut, one open.
                for (int a = -4; a <= 4; ++a)
                    for (int b = 0; b <= 2; ++b)
                        m_world->setBlock(bx + a, by + b, bz, Blocks::Planks);

                for (int b = 0; b <= 1; ++b)
                {
                    m_world->setBlock(bx - 2, by + b, bz, Blocks::Air);
                    m_world->setBlock(bx + 2, by + b, bz, Blocks::Air);
                }

                for (int half = 0; half < 2; ++half)
                {
                    m_world->setBlock(bx - 2, by + half, bz, doorPart(2, half == 1, false));
                    m_world->setBlock(bx + 2, by + half, bz, doorPart(2, half == 1, true));
                }
            }

            if (m_startupScreen == "fence")
            {
                m_camera.pitch = -10.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 5.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));

                // A run with a corner in it, and one post on its own, so
                // the arms and the lack of them are both in shot.
                for (int a = -3; a <= 2; ++a) m_world->setBlock(bx + a, by, bz, Blocks::OakFence);
                for (int b = 1; b <= 3; ++b) m_world->setBlock(bx + 2, by, bz - b, Blocks::OakFence);
                m_world->setBlock(bx + 4, by, bz + 1, Blocks::OakFence);
            }

            if (m_startupScreen == "ladder")
            {
                m_camera.pitch = -8.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 5.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));

                // A wall with a ladder up the near face of it, and a
                // flight of slab steps beside it for comparison.
                for (int a = -2; a <= 2; ++a)
                    for (int b = 0; b <= 5; ++b)
                        m_world->setBlock(bx + a, by + b, bz, Blocks::Cobblestone);

                for (int b = 0; b <= 5; ++b)
                    m_world->setBlock(bx, by + b, bz + 1, ladderFacing(0));

                for (int i = 0; i < 4; ++i)
                {
                    m_world->setBlock(bx + 2, by + i, bz + 1 + i, Blocks::Cobblestone);
                    m_world->setBlock(bx + 2, by + i + 1, bz + 1 + i, Blocks::CobblestoneSlab);
                }
            }

            if (m_startupScreen == "bucket")
            {
                m_creativeTab = 4;   // ITEMS
                m_inventory.clear();
                m_inventory.add(Items::Bucket, 3);
                m_inventory.add(Items::WaterBucket, 1);
                m_inventory.add(Items::LavaBucket, 1);
                m_inventory.add(Items::FlintAndSteel, 1);
                m_inventory.add(Items::Bow, 1);
                m_inventory.add(Items::Arrow, 32);
                m_inventory.add(Blocks::Tnt, 8);
                m_inventory.add(Blocks::Chest, 2);
                m_inventory.add(Blocks::Bed, 1);
                m_inventory.add(Blocks::StoneSlab, 16);
            }

            if (m_startupScreen == "portal" || m_startupScreen == "nether")
            {
                m_camera.pitch = -6.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 6.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));

                // A frame two across and three high, standing on a
                // plinth so it is clear of the grass.
                for (int a = -1; a <= 2; ++a)
                    for (int b = -1; b <= 3; ++b)
                    {
                        const bool edge = (a == -1 || a == 2 || b == -1 || b == 3);
                        m_world->setBlock(bx + a, by + b, bz,
                                          edge ? Blocks::Obsidian : Blocks::Air);
                    }

                const bool lit = lightPortal(glm::ivec3(bx, by, bz));
                std::printf("[portal] frame lit: %s\n", lit ? "yes" : "no");
                std::fflush(stdout);

                // "nether" walks through it as well.
                if (m_startupScreen == "nether" && lit)
                {
                    m_player.position = glm::vec3(bx + 0.5f, static_cast<float>(by), bz + 0.5f);
                    m_camera.position = m_player.eyePosition();
                }
            }

            if (m_startupScreen == "tnt")
            {
                m_camera.pitch = -22.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 5.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));

                for (int i = 0; i < 3; ++i)
                    for (int k = 0; k < 2; ++k)
                        m_world->setBlock(bx + i - 1, by + k, bz, Blocks::Tnt);

                // One of them lit, so both states are in the shot.
                primeTnt(glm::ivec3(bx, by + 1, bz));
                m_fuses.back().second = 600.0f;   // held, so it stays lit
            }

            if (m_startupScreen == "boom")
            {
                // Set one off in front of the camera, so the shot shows
                // the crater rather than the creeper.
                // Looking down at it, or the crater sits under the HUD.
                m_camera.pitch = -32.0f;
                m_camera.addLook(0.0f, 0.0f, 0.0f);

                const glm::vec3 ahead = m_player.position + m_camera.front * 7.0f;
                applyBlast(glm::vec3(ahead.x, m_player.position.y - 0.5f, ahead.z),
                           Explosion::CREEPER_RADIUS, Explosion::CREEPER_DAMAGE);
            }

            if (m_startupScreen == "slabs")
            {
                // A short flight of slab steps and a slab floor, so the
                // shot shows both the shape and the texture.
                const glm::vec3 ahead = m_player.position + m_camera.front * 4.0f;
                const int bx = static_cast<int>(std::floor(ahead.x));
                const int by = static_cast<int>(std::floor(m_player.position.y));
                const int bz = static_cast<int>(std::floor(ahead.z));

                const BlockId KINDS[4] = { Blocks::StoneSlab, Blocks::CobblestoneSlab,
                                           Blocks::PlankSlab, Blocks::SandstoneSlab };

                for (int i = 0; i < 4; ++i)
                    for (int j = 0; j < 3; ++j)
                        m_world->setBlock(bx - 1 + i, by, bz - 1 + j, KINDS[i]);

                // A step up, half a block at a time.
                m_world->setBlock(bx, by, bz - 3, Blocks::Cobblestone);
                m_world->setBlock(bx, by + 1, bz - 3, Blocks::CobblestoneSlab);
                m_world->setBlock(bx + 1, by, bz - 3, Blocks::Cobblestone);
                m_world->setBlock(bx + 1, by + 1, bz - 3, Blocks::Cobblestone);
                m_world->setBlock(bx + 1, by + 2, bz - 3, Blocks::CobblestoneSlab);
            }

            if (m_startupScreen == "bed")
            {
                const glm::vec3 ahead = m_player.position + m_camera.front * 3.0f;
                const glm::ivec3 where(static_cast<int>(std::floor(ahead.x)),
                                       static_cast<int>(std::floor(m_player.position.y)),
                                       static_cast<int>(std::floor(ahead.z)));
                m_world->setBlock(where.x, where.y + 1, where.z, Blocks::Bed);
                m_world->setBlock(where.x, where.y + 1, where.z - 1, Blocks::Chest);
            }

            if (m_startupScreen == "chest")
            {
                const glm::vec3 ahead = m_player.position + m_camera.front * 3.0f;
                const glm::ivec3 where(static_cast<int>(std::floor(ahead.x)),
                                       static_cast<int>(std::floor(m_player.position.y)),
                                       static_cast<int>(std::floor(ahead.z)));
                m_world->setBlock(where.x, where.y, where.z, Blocks::Chest);

                Chests::Contents& inside = m_chests.at(where);
                const StackId SAMPLE[] = {
                    Blocks::Cobblestone, Blocks::Planks, Blocks::Log, Items::Coal,
                    Items::IronIngot, Items::Diamond, Items::Bread, Items::Steak,
                    Items::DiamondPickaxe, Items::IronSword, Blocks::Torch, Items::Stick,
                };
                for (int i = 0; i < 12; ++i)
                {
                    inside[i].id = SAMPLE[i];
                    inside[i].count = isItem(SAMPLE[i]) && maxStackOf(SAMPLE[i]) == 1 ? 1 : (i + 1) * 3;
                }
                openChest(where);
            }
            if (m_startupScreen == "furnace" || m_startupScreen == "furnaceblock")
            {
                // A furnace a block away, already working, so the
                // screenshot shows the fire and the arrow part way.
                // Two blocks along the way the camera is looking, so it
                // is in shot rather than behind the player.
                const glm::vec3 ahead = m_player.position + m_camera.front * 3.0f;
                const glm::ivec3 where(static_cast<int>(std::floor(ahead.x)),
                                       static_cast<int>(std::floor(m_player.position.y)),
                                       static_cast<int>(std::floor(ahead.z)));
                m_world->setBlock(where.x, where.y, where.z, Blocks::Furnace);

                Furnaces::State& lit = m_furnaces.at(where);
                lit.input.id = Blocks::IronOre; lit.input.count = 12;
                lit.fuel.id = Items::Coal; lit.fuel.count = 6;
                lit.output.id = Items::IronIngot; lit.output.count = 4;
                lit.burnLeft = 48.0f; lit.burnTotal = 80.0f; lit.cooked = 6.2f;
                m_pendingFurnaceLight = true;

                // "furnaceblock" places it and leaves the screen shut,
                // so the block itself can be looked at.
                if (m_startupScreen == "furnace") openFurnace(where);
            }

            if (m_startupScreen == "tools")
            {
                m_inventory.clear();
                for (StackId id = Items::WoodPickaxe; id <= Items::DiamondHoe; ++id)
                    m_inventory.add(id, 1);
                m_inventory.add(Items::Stick, 12);
                m_inventory.add(Items::Coal, 7);
                m_inventory.add(Items::Diamond, 3);
                m_inventory.add(Items::IronIngot, 5);
                m_inventory.add(Items::GoldIngot, 2);

                // A few part-worn, so the durability bar shows.
                m_inventory.slot(0).damage = 40;
                m_inventory.slot(6).damage = 90;
                m_inventory.slot(12).damage = 180;
            }

            // A screenshot of the bench wants the big grid showing.
            if (m_startupScreen == "bench") m_inventory.setCraftSize(3);
        }
        else if (m_startupScreen == "pause") m_screen = Screen::Paused;
        else if (m_startupScreen == "dead" || m_startupScreen == "hardcore")
        {
            m_player.health = 0;
            m_hardcoreDeath = modeIsHardcore(m_gameMode);
            m_screen = Screen::Dead;
        }
        m_startupScreen.clear();
    }

    if (m_pendingArena && m_worldReady && m_world)
    {
        m_arenaDelay += deltaTime;
        if (m_arenaDelay > 1.5f)
        {
            if (m_arenaKind == ArenaKind::BiomePocket) buildBiomeArena(m_arenaX, m_arenaZ);
            else dressChamberRoom(m_arenaX, m_arenaY, m_arenaZ);
            m_pendingArena = false;
        }
    }

    // Screenshots must be read back before the buffer swap, while the
    // frame we just drew is still the back buffer.
    if (m_autoScreenshotDelay > 0.0f && (m_worldReady || !m_world))
    {
        m_readySeconds += deltaTime;
        if (m_readySeconds >= m_autoScreenshotDelay) m_pendingScreenshot = true;
    }

    if (m_pendingScreenshot)
    {
        m_pendingScreenshot = false;
        const std::string path = saveScreenshot(m_window.width(), m_window.height(), "screenshots");
        std::printf("%s\n", path.empty() ? "Screenshot failed" : ("Saved screenshot " + path).c_str());
        std::fflush(stdout);
        if (m_autoScreenshotDelay > 0.0f) m_running = false;
    }

    m_window.swapBuffers();
    return m_running;
}

void Application::applyMenuAction()
{
    const MenuAction action = m_pendingAction;
    m_pendingAction = MenuAction::None;

    switch (action)
    {
        case MenuAction::None: break;

        case MenuAction::ResumeSaved:    startWorld(GameMode::Survival, false); break;
        case MenuAction::GoSingleplayer: m_screen = Screen::Singleplayer; break;
        case MenuAction::GoMultiplayer:
            m_screen = Screen::Multiplayer;
#ifndef __EMSCRIPTEN__
            m_discovery.startListening();
#endif
            break;

#ifndef __EMSCRIPTEN__
        case MenuAction::OpenToLan:
        {
            const std::string host = m_playerName.empty() ? "SOMEONE" : m_playerName;
            if (m_net.openToLan(host, m_world ? m_world->seed() : 0,
                                static_cast<uint8_t>(m_gameMode), m_timeOfDay, m_spawnPoint))
            {
                m_discovery.startAnnouncing(host + "'S WORLD", m_net.port());
                m_audio.play(Sound::Click, 0.6f);
            }
            break;
        }

        case MenuAction::CloseToLan:
            m_net.leave();
            m_discovery.stopAnnouncing();
            break;

        case MenuAction::JoinTyped:
            m_joinedWorldBuilt = false;
            m_net.join(m_addressDraft, Net::DEFAULT_PORT,
                       m_playerName.empty() ? "PLAYER" : m_playerName);
            break;

        case MenuAction::JoinDiscovered:
        {
            const std::vector<Net::LanDiscovery::Game>& games = m_discovery.games();
            m_joinedWorldBuilt = false;
            if (m_joinChoice >= 0 && m_joinChoice < static_cast<int>(games.size()))
                m_net.join(games[m_joinChoice].address, games[m_joinChoice].port,
                           m_playerName.empty() ? "PLAYER" : m_playerName);
            m_joinChoice = -1;
            break;
        }
#else
        case MenuAction::OpenToLan:
        case MenuAction::CloseToLan:
        case MenuAction::JoinTyped:
        case MenuAction::JoinDiscovered:
            break;
#endif
        case MenuAction::GoSettings:     m_screen = Screen::Settings; break;
        case MenuAction::GoControls:
            m_rebinding = Action::Count;
            m_screen = Screen::Controls;
            break;

        case MenuAction::GoNameEntry:
            m_nameDraft = m_playerName;
            m_screen = Screen::NameEntry;
            break;

        case MenuAction::ToggleCheats:
            m_allowCheats = !m_allowCheats;
            m_audio.play(Sound::Click, 0.45f);
            break;

        case MenuAction::PickSkin:
            m_skinVariant = m_pendingSkinVariant;
            m_audio.play(Sound::Click, 0.45f);
            break;

        case MenuAction::ToggleSkinStyle:
            m_skinStyle = (m_skinStyle == PlayerSkin::Style::Classic)
                              ? PlayerSkin::Style::Slim
                              : PlayerSkin::Style::Classic;
            rebuildSkinLibrary();
            m_audio.play(Sound::Click, 0.45f);
            break;

        case MenuAction::ResetBindings:
            m_keys.resetToDefaults();
            saveBindings();
            m_audio.play(Sound::Click, 0.5f);
            break;

        case MenuAction::ConfirmName:
            m_playerName = m_nameDraft;
            saveProfile();
            m_screen = m_startupScreen.empty() ? Screen::Title : m_screen;
            break;

        case MenuAction::RenderDistanceDown:
            if (m_world) m_world->setRenderDistance(m_world->renderDistance() - 1);
            break;
        case MenuAction::RenderDistanceUp:
            if (m_world) m_world->setRenderDistance(m_world->renderDistance() + 1);
            break;
        case MenuAction::NewSurvival:   startWorld(GameMode::Survival, true); break;
        case MenuAction::NewCreative:   startWorld(GameMode::Creative, true); break;
        case MenuAction::NewHardcore:   startWorld(GameMode::Hardcore, true); break;
        case MenuAction::BackToTitle:
#ifndef __EMSCRIPTEN__
            m_discovery.stopListening();
#endif
            m_screen = Screen::Title;
            break;

        case MenuAction::Resume:
            m_screen = Screen::Playing;
            setMouseCaptured(true);
            break;

        case MenuAction::QuitToTitle:   quitToTitle(); break;
        case MenuAction::QuitGame:      m_running = false; break;

        case MenuAction::Respawn:
            m_player.respawn(m_spawnPoint);
            m_previousHealth = m_player.health;
            m_screen = Screen::Playing;
            setMouseCaptured(true);
            break;

        case MenuAction::DeleteWorld:
            // Hardcore: the run is over, the world goes with it.
            m_world.reset();
            deleteSavedWorld();
            m_drops.clear();
            m_orbs.clear();
    m_rain.clear();
    m_orbs.clear();
    m_rain.clear();
            m_particles.clear();
            m_hardcoreDeath = false;
            m_screen = Screen::Title;
            setMouseCaptured(false);
            break;

        case MenuAction::TogglePbr:     m_pbrEnabled = !m_pbrEnabled; break;
        case MenuAction::ToggleSound:   m_audio.setMuted(!m_audio.muted()); break;
    }
}

void Application::handleEvents()
{
    m_input.beginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) m_input.processEvent(event);

    if (m_input.quitRequested()) m_running = false;

    if (m_input.resized())
    {
        m_window.setSize(m_input.newWidth(), m_input.newHeight());
        glViewport(0, 0, m_window.width(), m_window.height());
    }

    if (m_automated) return; // automated runs ignore keyboard and mouse entirely

    if (m_screen == Screen::NameEntry)
    {
        // The font only has ASCII 32..95, and Minecraft-style names are
        // letters, digits and underscores anyway, so everything else is
        // dropped rather than drawn as a blank.
        for (char c : m_input.typedText())
        {
            if (static_cast<int>(m_nameDraft.size()) >= MAX_NAME_LENGTH) break;
            const bool allowed = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                                 (c >= '0' && c <= '9') || c == '_';
            if (allowed) m_nameDraft.push_back(c);
        }
        for (int i = 0; i < m_input.backspaces() && !m_nameDraft.empty(); ++i)
            m_nameDraft.pop_back();

        if ((m_input.wasKeyPressed(SDL_SCANCODE_RETURN) ||
             m_input.wasKeyPressed(SDL_SCANCODE_KP_ENTER)) &&
            static_cast<int>(m_nameDraft.size()) >= MIN_NAME_LENGTH)
            m_pendingAction = MenuAction::ConfirmName;

        // Escape only backs out if there is already a name to go back to.
        if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE) && !m_playerName.empty())
        {
            m_nameDraft = m_playerName;
            m_screen = Screen::Settings;
        }
        return;
    }

    if (m_screen == Screen::Controls)
    {
        if (m_rebinding != Action::Count)
        {
            // Escape clears the binding; anything else becomes it. The
            // whole frame is swallowed either way, so the key being
            // bound cannot also trigger whatever it is bound to.
            if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
            {
                m_keys.setBinding(m_rebinding, Binding{ false, 0 });
                m_rebinding = Action::Count;
                saveBindings();
            }
            else if (const int code = m_input.firstKeyPressed())
            {
                m_keys.setBinding(m_rebinding, Binding{ false, code });
                m_rebinding = Action::Count;
                saveBindings();
                m_audio.play(Sound::Click, 0.5f);
            }
            else if (const int button = m_input.firstMouseButtonPressed())
            {
                m_keys.setBinding(m_rebinding, Binding{ true, button });
                m_rebinding = Action::Count;
                saveBindings();
                m_audio.play(Sound::Click, 0.5f);
            }
            return;
        }

        // Starting a rebind consumes the click, or that same click would
        // immediately be recorded as the new binding.
        if (m_controlsHover >= 0 && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT))
        {
            m_rebinding = static_cast<Action>(m_controlsHover);
            return;
        }
    }

#ifndef __EMSCRIPTEN__
    if (m_screen == Screen::Multiplayer)
    {
        // Only what can appear in an IPv4 address, so a stray keypress
        // cannot produce something the parser will reject.
        for (char c : m_input.typedText())
        {
            if (m_addressDraft.size() >= 21) break;
            if ((c >= '0' && c <= '9') || c == '.' || c == ':') m_addressDraft.push_back(c);
        }
        for (int i = 0; i < m_input.backspaces() && !m_addressDraft.empty(); ++i)
            m_addressDraft.pop_back();

        if (m_input.wasKeyPressed(SDL_SCANCODE_RETURN) && m_addressDraft.size() >= 7)
            m_pendingAction = MenuAction::JoinTyped;
    }
#endif

    if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        if (m_inventoryOpen)
        {
            closeInventory();
            setMouseCaptured(true);
        }
        else if (m_screen == Screen::Playing)
        {
            m_screen = Screen::Paused;
            setMouseCaptured(false);
        }
        else if (m_screen == Screen::Paused)
        {
            m_screen = Screen::Playing;
            setMouseCaptured(true);
        }
        else if (m_screen == Screen::Controls)
        {
            m_rebinding = Action::Count;
            m_screen = Screen::Settings; // back up one level, not all the way out
        }
        else if (m_screen == Screen::Singleplayer || m_screen == Screen::Multiplayer ||
                 m_screen == Screen::Settings)
        {
            m_screen = Screen::Title;
        }
    }

    // F3+F6 cycles how much the debug screen shows. Checked before the
    // plain F3 toggle so holding F3 to use a combination does not also
    // turn the screen off underneath you.
    const bool debugHeld = m_keys.down(m_input, Action::Debug);
    if (debugHeld && m_input.wasKeyPressed(SDL_SCANCODE_F6))
    {
        m_debugDetail = (m_debugDetail == DebugDetail::Minimal) ? DebugDetail::Default
                      : (m_debugDetail == DebugDetail::Default) ? DebugDetail::Full
                                                                : DebugDetail::Minimal;
        m_showDebug = true;
        m_audio.play(Sound::Click, 0.4f);
    }
    else if (m_keys.pressed(m_input, Action::Debug))
    {
        m_showDebug = !m_showDebug;
    }
    if (m_keys.pressed(m_input, Action::Screenshot)) m_pendingScreenshot = true;
    if (m_keys.pressed(m_input, Action::HideHud)) m_hudHidden = !m_hudHidden;
    if (m_keys.pressed(m_input, Action::Perspective) && inGame())
    {
        // Your eyes, behind you, then facing you, the way F5 goes round.
        m_perspective = (m_perspective == Perspective::FirstPerson) ? Perspective::ThirdBack
                      : (m_perspective == Perspective::ThirdBack)   ? Perspective::ThirdFront
                                                                    : Perspective::FirstPerson;
    }
    if (m_keys.pressed(m_input, Action::Fullscreen)) m_window.toggleFullscreen();
    if (m_keys.pressed(m_input, Action::FancyLighting)) m_pbrEnabled = !m_pbrEnabled;
    if (m_keys.pressed(m_input, Action::Mute))
    {
        m_audio.setMuted(!m_audio.muted());
        if (!m_audio.muted()) m_audio.play(Sound::Click, 0.6f);
    }

    if (!m_world) return;

    // While the creative palette is up the wheel scrolls it rather than
    // the hotbar, which is the only way to reach a tab longer than the
    // five rows the grid shows.
    if (m_inventoryOpen && m_player.creative() && m_input.wheelDelta() != 0)
        m_paletteRow -= m_input.wheelDelta();

    if (m_keys.pressed(m_input, Action::Inventory) && (playing() || m_inventoryOpen))
    {
        if (m_inventoryOpen)
        {
            closeInventory();
        }
        else
        {
            // The player's own grid is the small one; only a bench opens
            // the big one.
            m_inventory.setCraftSize(2);
            m_inventoryOpen = true;
        }
        setMouseCaptured(!m_inventoryOpen);
    }

    if (m_keys.pressed(m_input, Action::RenderDistanceDown))
        m_world->setRenderDistance(m_world->renderDistance() - 1);
    if (m_keys.pressed(m_input, Action::RenderDistanceUp))
        m_world->setRenderDistance(m_world->renderDistance() + 1);

    // Throwing the held stack on the ground, the way Q does in Minecraft.
    if (m_keys.pressed(m_input, Action::Drop) && playing() && !m_inventoryOpen)
    {
        ItemStack& held = m_inventory.selected();
        if (!held.empty())
        {
            m_drops.spawn(m_player.eyePosition() + m_camera.front * 0.9f, held.id, 1);
            held.count -= 1;
            if (held.count <= 0) held.clear();
        }
    }

    if (m_inventoryOpen)
    {
        const bool shift = m_input.isKeyDown(SDL_SCANCODE_LSHIFT) ||
                           m_input.isKeyDown(SDL_SCANCODE_RSHIFT);
        if (m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT))
            handleInventoryClick(m_input.mouseX(), m_input.mouseY(), false, shift);
        else if (m_input.wasMouseButtonPressed(SDL_BUTTON_RIGHT))
            handleInventoryClick(m_input.mouseX(), m_input.mouseY(), true, shift);
    }

}

void Application::waitForSpawnChunk()
{
    const int chunkX = World::floorDiv(static_cast<int>(std::floor(m_player.position.x)), Chunk::SX);
    const int chunkZ = World::floorDiv(static_cast<int>(std::floor(m_player.position.z)), Chunk::SZ);

    const Chunk* chunk = m_world->chunkAt(chunkX, chunkZ);
    if (!chunk || chunk->state.load() != ChunkState::Lit) return;

    if (m_placeOnSurface)
    {
        // Put the player on ground they can actually stand on. Dropping
        // straight down the spawn column was not enough: water and lava
        // are not solid, so the old search fell through a lake and stood
        // the player on the seabed, underwater. Liquid at the feet or
        // head now rejects the column outright and the search spirals
        // out to a neighbouring one.
        const int originX = static_cast<int>(std::floor(m_player.position.x));
        const int originZ = static_cast<int>(std::floor(m_player.position.z));
        bool placed = false;

        for (int radius = 0; radius <= 24 && !placed; radius += 2)
        {
            const int steps = (radius == 0) ? 1 : 16;
            for (int step = 0; step < steps && !placed; ++step)
            {
                const float angle = step * 0.3927f;   // 2*pi/16
                const int x = originX + static_cast<int>(std::cos(angle) * radius);
                const int z = originZ + static_cast<int>(std::sin(angle) * radius);

                // Only look at terrain that has actually streamed in;
                // an unloaded chunk reads as air and would look inviting.
                const Chunk* here = m_world->chunkAt(World::floorDiv(x, Chunk::SX),
                                                     World::floorDiv(z, Chunk::SZ));
                if (!here || here->state.load() != ChunkState::Lit) continue;

                // Search down from just above where the player already is,
                // not from the top of the world. In the Nether the top of
                // the world is the underside of the bedrock roof, and
                // starting there parked the player in the ceiling cavern.
                const int from = std::min(Chunk::SY - 4,
                                          static_cast<int>(m_player.position.y) + 8);
                for (int y = from; y > 1; --y)
                {
                    const BlockId ground = m_world->getBlock(x, y, z);
                    if (isLiquid(ground)) break;      // a pool: try elsewhere
                    if (!isSolid(ground)) continue;

                    const BlockId feet = m_world->getBlock(x, y + 1, z);
                    const BlockId head = m_world->getBlock(x, y + 2, z);
                    if (isSolid(feet) || isSolid(head)) break;        // no headroom
                    if (isLiquid(feet) || isLiquid(head)) break;      // submerged

                    m_player.position = glm::vec3(x + 0.5f, static_cast<float>(y + 1), z + 0.5f);
                    placed = true;
                    break;
                }
            }
        }

        // Remember where that was, so dying returns you somewhere valid
        // rather than to the generator's original guess.
        if (placed) m_spawnPoint = m_player.position;
        m_placeOnSurface = false;
    }

    m_player.velocity = glm::vec3(0.0f);
    m_camera.position = m_player.eyePosition();
    m_worldReady = true;

    // Only once there are chunks to build it in.
    buildReturnPortal();
}

void Application::updateGameplay(float deltaTime)
{
    // Typing comes first and takes everything: no walking, no looking and
    // no mining while the chat line is open.
    updateChatInput();
    m_chatLog.update(deltaTime);
    if (m_chatOpen) return;

    // --- look ---
    // Moving/resizing the window or regaining focus can deliver one huge
    // bogus mouse delta, which would snap the view straight up. Real mouse
    // movement never covers this much ground in a single frame.
    constexpr float MAX_LOOK_DELTA = 180.0f;
    float lookX = m_input.mouseDeltaX();
    float lookY = m_input.mouseDeltaY();
    if (std::fabs(lookX) > MAX_LOOK_DELTA || std::fabs(lookY) > MAX_LOOK_DELTA)
    {
        lookX = 0.0f;
        lookY = 0.0f;
    }
    m_camera.addLook(lookX, lookY, MOUSE_SENSITIVITY);

    // Flight is a creative-mode privilege.
    if (m_keys.pressed(m_input, Action::Fly) && m_player.creative())
        m_player.flying = !m_player.flying;

    // --- hotbar selection ---
    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
        if (m_input.wasKeyPressed(static_cast<SDL_Scancode>(SDL_SCANCODE_1 + i)))
            m_inventory.setSelectedSlot(i);
    if (!m_inventoryOpen) m_inventory.scrollSelection(m_input.wheelDelta());

    // --- movement ---
    Player::Controls controls;
    const glm::vec3 flatFront = glm::normalize(glm::vec3(m_camera.front.x, 0.0f, m_camera.front.z));
    const glm::vec3 flatRight = glm::normalize(glm::vec3(m_camera.right.x, 0.0f, m_camera.right.z));

    glm::vec3 wish(0.0f);
    if (m_keys.down(m_input, Action::Forward)) wish += flatFront;
    if (m_keys.down(m_input, Action::Back))    wish -= flatFront;
    if (m_keys.down(m_input, Action::Left))    wish -= flatRight;
    if (m_keys.down(m_input, Action::Right))   wish += flatRight;
    if (glm::length(wish) > 0.0001f) wish = glm::normalize(wish);

    controls.wishDirection = wish;
    controls.jump = m_keys.pressed(m_input, Action::Jump);
    controls.jumpHeld = m_keys.down(m_input, Action::Jump);
    controls.sprint = m_keys.down(m_input, Action::Sprint);
    controls.sneak = m_keys.down(m_input, Action::Sneak);
    controls.descend = controls.sneak;

    const glm::vec3 positionBefore = m_player.position;

    // On horseback the mount does the walking and the rider goes where
    // it goes. The player's own physics are skipped outright rather
    // than fought: running both meant the two disagreed about where
    // the ground was and the rider juddered.
    if (Mob* mount = riddenMob())
    {
        mount->steer(m_camera.yaw,
                     m_keys.down(m_input, Action::Forward),
                     m_keys.down(m_input, Action::Jump));

        m_player.position = mount->saddle();
        m_player.velocity = glm::vec3(0.0f);
        m_player.onGround = true;

        // Sneak gets you off, which is what Minecraft uses and the one
        // key that is not already steering something.
        if (m_keys.pressed(m_input, Action::Sneak)) dismount();
    }
    else
    {
        m_player.update(deltaTime, *m_world, controls);
    }

    m_camera.position = m_player.eyePosition();
    updateMovementAudio(positionBefore, m_player.jumpedThisUpdate());

    // A little extra field of view while sprinting sells the speed.
    const float targetFov = m_player.sprinting ? 78.0f : 70.0f;
    m_camera.fov += (targetFov - m_camera.fov) * std::clamp(8.0f * deltaTime, 0.0f, 1.0f);

    // --- the rest of the world ---
    updateWorldAround(deltaTime);
    updateMining(deltaTime);
    // A shield goes up while Use is held, which is the same button
    // that places and eats -- so holding one takes that button over.
    m_player.blocking = inGame() && m_inventory.selected().id == Items::Shield &&
                        (m_keys.down(m_input, Action::Use) ||
                         m_startupScreen == "shieldup");

    handleEating(deltaTime);
    handleDrinking(deltaTime);
    tickBrewing(deltaTime);
    handlePlacement();
    m_heldItem.update(deltaTime);
}

void Application::updateMovementAudio(const glm::vec3& positionBefore, bool jumped)
{
    // The block directly under the player's feet decides how a step sounds.
    const glm::vec3 feet = m_player.position;
    const BlockId ground = m_world->getBlock(static_cast<int>(std::floor(feet.x)),
                                             static_cast<int>(std::floor(feet.y - 0.2f)),
                                             static_cast<int>(std::floor(feet.z)));

    // `jumped` is the player's own report of having left the ground, not
    // the state of the key: the key is down for as long as it is held,
    // and a held key over a surface that refuses the jump -- shallow
    // water, a ceiling a block above your head -- used to re-trigger this
    // on every single frame.
    if (jumped && !m_player.flying)
        m_audio.play(Sound::Jump, 0.5f);

    // Water cancels the fall, so it cancels the thump and the dust with
    // it. Bobbing about on a shallow bottom crosses this edge over and
    // over, and each crossing used to land as though from a height.
    const bool wading = m_player.isInWater(*m_world);

    if (m_player.onGround && !m_wasOnGround && !m_player.flying && !wading)
    {
        m_audio.play(Sound::Land, 0.55f);
        if (ground != Blocks::Air) m_particles.spawnLandingPuff(feet, ground);
        m_distanceWalked = 0.0f;
    }
    m_wasOnGround = m_player.onGround;

    if (m_player.onGround && !m_player.flying && ground != Blocks::Air)
    {
        const glm::vec3 delta = m_player.position - positionBefore;
        m_distanceWalked += glm::length(glm::vec2(delta.x, delta.z));

        // One footstep roughly every two blocks travelled, like Minecraft.
        const float stride = m_player.sneaking ? 3.0f : 2.1f;
        if (m_distanceWalked >= stride)
        {
            m_distanceWalked = 0.0f;
            m_audio.playStep(ground);
        }
    }

    if (m_player.health < m_previousHealth) m_audio.play(Sound::Hurt, 0.7f);
    m_previousHealth = m_player.health;
}

void Application::updateMining(float deltaTime)
{
    const RaycastHit hit = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);

    // A mob standing between you and the block takes the hit instead.
    // Minecraft's reach is shorter for something living than for stone,
    // so a mob you cannot quite touch does not stop you mining past it.
    if (m_attackCooldown > 0.0f) m_attackCooldown = std::max(0.0f, m_attackCooldown - deltaTime);

    if (Mob* target = m_entities.pick(m_camera.position, m_camera.front, ATTACK_REACH))
    {
        const float toMob = glm::length(target->position() - m_camera.position);
        const float toBlock = hit.hit ? glm::length(glm::vec3(hit.block) + glm::vec3(0.5f) - m_camera.position)
                                      : 1e9f;

        if (toMob <= toBlock)
        {
            // One swing per click, not one per frame: mining is a hold,
            // hitting is not.
            if (m_keys.pressed(m_input, Action::Attack)) m_heldItem.swing();
            if (m_keys.pressed(m_input, Action::Attack) && m_attackCooldown <= 0.0f)
            {
                // A sword hits hardest, an axe nearly as hard, a bare
                // hand barely at all.
                const StackId weapon = m_inventory.selected().id;
                // Strength adds and weakness takes away, and the two
                // together can go below nothing, so the total is
                // floored here rather than inside the effect.
                // A mace is paid for the fall that lands it. Landing
                // the blow also takes the fall over: in Minecraft a
                // smash costs you nothing on the way down, which is
                // the only reason anyone jumps off anything with one.
                const float fallen = m_player.fallenSoFar();
                const int smash = (Tools::kindOf(weapon) == Tools::Kind::Mace)
                                      ? Tools::maceSmashBonus(fallen)
                                      : 0;

                const int blow = std::max(0,
                    Tools::attackDamage(weapon) +
                        Enchanting::sharpnessBonus(m_inventory.selected().enchantments) +
                        Effects::attackBonus(m_player.effects) + smash);
                target->damage(blow, target->position() - m_player.position);

                if (smash > 0)
                {
                    m_player.clearFall();
                    m_player.velocity.y = std::max(m_player.velocity.y, 0.0f);
                    m_audio.play(Sound::Land, 0.9f, 0.7f);
                    m_particles.spawnBlockBreak(
                        glm::ivec3(glm::floor(target->position())), Blocks::Stone);
                }
                wearOnSwing(weapon);
                m_attackCooldown = ATTACK_INTERVAL;
            }

            m_hasTarget = false;
            m_breakProgress = 0.0f;
            return;
        }
    }

    if (!hit.hit || !m_keys.down(m_input, Action::Attack))
    {
        m_hasTarget = false;
        m_breakProgress = 0.0f;
        return;
    }

    // Switching target restarts progress.
    if (!m_hasTarget || m_targetBlock != hit.block)
    {
        m_targetBlock = hit.block;
        m_breakProgress = 0.0f;
        m_hasTarget = true;
    }

    const BlockId id = m_world->getBlock(hit.block.x, hit.block.y, hit.block.z);

    // A stick of TNT is lit rather than mined: hitting it starts the
    // fuse, which is how Minecraft does it with flint and steel.
    if (id == Blocks::Tnt && !m_player.creative())
    {
        primeTnt(hit.block);
        m_heldItem.swing();
        m_hasTarget = false;
        m_breakProgress = 0.0f;
        return;
    }

    // What is in your hand decides how long this takes and whether it
    // leaves anything behind. An empty hand is Air, which Tools reads as
    // bare hands.
    const StackId tool = m_inventory.selected().id;
    float seconds = Tools::breakSeconds(tool, id);

    // Efficiency, if the tool has any. Applied to the time rather than
    // folded into breakSeconds, which stays a pure function of a tool
    // and a block and knows nothing about what has been put on it.
    if (seconds > 0.0f)
        seconds /= Enchanting::digSpeedMultiplier(m_inventory.selected().enchantments);
    if (seconds < 0.0f) return; // bedrock and liquids never break

    if (m_player.creative() || seconds <= 0.0f)
        m_breakProgress = 1.0f;
    else
        m_breakProgress += deltaTime / seconds;

    // One swing after another for as long as you keep mining, rather
    // than one restarted every frame.
    if (!m_heldItem.swinging()) m_heldItem.swing();

    if (m_breakProgress >= 1.0f)
    {
        m_particles.spawnBlockBreak(hit.block, id);
        m_world->setBlock(hit.block.x, hit.block.y, hit.block.z, Blocks::Air);
        m_net.onLocalBlockChange(hit.block.x, hit.block.y, hit.block.z, Blocks::Air);
        m_audio.playDig(id, 0.9f);

        // Creative mode mines without producing pickups, like Minecraft.
        if (!m_player.creative())
        {
            if (isWheat(id))
            {
                const Farming::Harvest crop = Farming::harvestOf(
                    id, static_cast<uint32_t>(hit.block.x * 31 + hit.block.z * 17 +
                                              static_cast<int>(m_elapsedSeconds * 60.0f)));
                for (int i = 0; i < crop.firstCount; ++i)
                    m_drops.spawnFromBrokenBlock(hit.block, crop.first);
                for (int i = 0; i < crop.secondCount; ++i)
                    m_drops.spawnFromBrokenBlock(hit.block, crop.second);
            }
            else if (Tools::canHarvest(tool, id))
            {
                StackId drop = blockDrop(id);

                // Gravel gives up a flint about one time in ten, which
                // is where arrows come from.
                if (id == Blocks::Gravel)
                {
                    const uint32_t roll = static_cast<uint32_t>(
                        hit.block.x * 73856093 ^ hit.block.y * 19349663 ^
                        hit.block.z * 83492791 ^ static_cast<int>(m_elapsedSeconds * 977.0f));
                    if (roll % 10 == 0) drop = Items::Flint;
                }

                // Silk touch hands back the block itself rather than
                // what it breaks into, which is the whole point of it:
                // a seam comes home as ore, and glass as glass.
                const Enchanting::Set onTool = m_inventory.selected().enchantments;
                const bool silk = Enchanting::has(onTool, Enchanting::Kind::SilkTouch);
                if (silk && drop != Blocks::Air) drop = id;

                int dropped = 1;

                // Fortune multiplies a seam, but only where the block
                // breaks into something other than itself: multiplying
                // stone would be a way of making stone out of nothing.
                if (!silk && drop != id && drop != Blocks::Air)
                {
                    const uint32_t roll = static_cast<uint32_t>(
                        hit.block.x * 83492791 ^ hit.block.y * 73856093 ^
                        hit.block.z * 19349663 ^ static_cast<int>(m_elapsedSeconds * 389.0f));
                    dropped = Enchanting::fortuneDrops(onTool, roll);
                }

                if (drop != Blocks::Air)
                    m_drops.spawnFromBrokenBlock(hit.block, drop, dropped);

                // Ore you can use straight away pays out at the seam.
                // The ores you have to smelt pay at the furnace instead,
                // so iron and gold give nothing here -- otherwise they
                // would be paid for twice.
                const Experience::Range worth = Experience::forMining(id);
                if (worth.any() && !silk)
                {
                    const uint32_t roll = static_cast<uint32_t>(
                        hit.block.x * 19349663 ^ hit.block.y * 83492791 ^
                        hit.block.z * 73856093 ^ static_cast<int>(m_elapsedSeconds * 613.0f));
                    const int span = worth.high - worth.low + 1;
                    m_orbs.spawnValue(glm::vec3(hit.block) + glm::vec3(0.5f),
                                      worth.low + static_cast<int>(roll % static_cast<uint32_t>(span)));
                }
            }

            // Cane is a stack, and breaking one out of the middle
            // drops everything resting on it: without this the upper
            // canes hang in mid-air with nothing beneath them.
            if (id == Blocks::SugarCane)
            {
                for (int up = 1; up < Cane::MAX_HEIGHT; ++up)
                {
                    const glm::ivec3 over(hit.block.x, hit.block.y + up, hit.block.z);
                    if (m_world->getBlock(over.x, over.y, over.z) != Blocks::SugarCane) break;
                    m_world->setBlock(over.x, over.y, over.z, Blocks::Air);
                    m_net.onLocalBlockChange(over.x, over.y, over.z, Blocks::Air);
                    if (!m_player.creative())
                        m_drops.spawnFromBrokenBlock(over, Blocks::SugarCane);
                }
            }

            // Rails. Breaking the block under one takes the rail with
            // it, and breaking a rail leaves its neighbours to find a
            // shape that no longer reaches across the gap.
            {
                const glm::ivec3 over(hit.block.x, hit.block.y + 1, hit.block.z);
                if (isRail(m_world->getBlock(over.x, over.y, over.z)))
                {
                    const BlockId loose = m_world->getBlock(over.x, over.y, over.z);
                    m_world->setBlock(over.x, over.y, over.z, Blocks::Air);
                    m_net.onLocalBlockChange(over.x, over.y, over.z, Blocks::Air);
                    if (!m_player.creative())
                        m_drops.spawnFromBrokenBlock(over, loose);
                    reshapeRailsAround(over);
                }
            }
            if (isRail(id)) reshapeRailsAround(hit.block);

            // A piston with its head out is two blocks pretending to
            // be one, so breaking either takes the other with it.
            if (isPiston(id))
            {
                int dx = 0, dz = 0;
                Piston::step(blockFacing(id), dx, dz);
                const glm::ivec3 other = isPistonHead(id)
                                             ? glm::ivec3(hit.block.x - dx, hit.block.y,
                                                          hit.block.z - dz)
                                             : glm::ivec3(hit.block.x + dx, hit.block.y,
                                                          hit.block.z + dz);
                const BlockId paired = m_world->getBlock(other.x, other.y, other.z);
                if (isPiston(paired))
                {
                    m_world->setBlock(other.x, other.y, other.z, Blocks::Air);
                    m_net.onLocalBlockChange(other.x, other.y, other.z, Blocks::Air);
                    if (!m_player.creative() && isPistonBody(paired))
                        m_drops.spawnFromBrokenBlock(other, blockDrop(paired));
                }
            }

            // A door is one thing in two cells, so breaking either half
            // takes the other with it -- otherwise half a door is left
            // hanging in the air.
            if (isDoor(id))
            {
                const int other = doorIsTop(id) ? -1 : 1;
                const glm::ivec3 pair(hit.block.x, hit.block.y + other, hit.block.z);
                if (isDoor(m_world->getBlock(pair.x, pair.y, pair.z)))
                {
                    m_world->setBlock(pair.x, pair.y, pair.z, Blocks::Air);
                    m_net.onLocalBlockChange(pair.x, pair.y, pair.z, Blocks::Air);
                }
            }

            // A crafter gives back what was in its grid, the same way
            // a chest does.
            if (isCrafter(id))
            {
                if (m_crafterOpen && m_crafterBlock == hit.block) stowCrafter();

                if (m_chests.has(hit.block))
                {
                    Chests::Contents& held = m_chests.at(hit.block);
                    for (int slot = 0; slot < CRAFTER_SLOTS; ++slot)
                    {
                        for (int n = 0; n < held[slot].count; ++n)
                            m_drops.spawnFromBrokenBlock(hit.block, held[slot].id);
                        held[slot].clear();
                    }
                    m_chests.remove(hit.block);
                }
            }

            // A chest gives back everything in it. Twenty-seven stacks
            // is a lot to drop at once, which is why they come out of
            // the block rather than the player.
            if (id == Blocks::Chest)
            {
                if (m_chestOpen && m_chestBlock == hit.block) stowChest();

                if (m_chests.has(hit.block))
                {
                    for (const ItemStack& held : m_chests.at(hit.block))
                        if (!held.empty())
                            m_drops.spawnFromBrokenBlock(hit.block, held.id, held.count);
                    m_chests.remove(hit.block);
                }
            }

            // A furnace gives back whatever was inside it, lit or not,
            // and whether or not the furnace itself survives the pick.
            if (id == Blocks::Furnace || id == Blocks::FurnaceLit)
            {
                if (m_furnaceOpen && m_furnaceBlock == hit.block) stowFurnace();

                if (m_furnaces.has(hit.block))
                {
                    const Furnaces::State& inside = m_furnaces.at(hit.block);
                    for (const ItemStack* held : { &inside.input, &inside.fuel, &inside.output })
                        if (!held->empty())
                            m_drops.spawnFromBrokenBlock(hit.block, held->id, held->count);
                }
            }

            // Stone gone at with bare hands still breaks; it just leaves
            // nothing, which is what makes the first pickaxe matter.
            wearTool(id);
        }

        // Dust, a torch and the switches all sit on whatever is under
        // them. Pull that out and they come up too, the same way a rail
        // does -- in creative as well, where nothing drops but the
        // thing still cannot be left hanging in the air.
        {
            const glm::ivec3 over(hit.block.x, hit.block.y + 1, hit.block.z);
            const BlockId above = m_world->getBlock(over.x, over.y, over.z);
            if (Redstone::isWire(above) || above == Blocks::RedstoneTorch ||
                above == Blocks::RedstoneTorchOff || above == Blocks::LeverOff ||
                above == Blocks::LeverOn || above == Blocks::ButtonOff ||
                above == Blocks::ButtonOn)
            {
                m_world->setBlock(over.x, over.y, over.z, Blocks::Air);
                m_net.onLocalBlockChange(over.x, over.y, over.z, Blocks::Air);
                if (!m_player.creative())
                    m_drops.spawnFromBrokenBlock(over, blockDrop(above));
            }
        }

        // Whatever that was, the power around it has to be worked out
        // again: a broken wire puts out everything downstream of it.
        settleRedstone(hit.block);

        m_breakProgress = 0.0f;
        m_hasTarget = false;
    }
    else
    {
        // Tick a quiet dig sound while the block is being chipped away.
        m_digSoundTimer += deltaTime;
        if (m_digSoundTimer >= 0.22f)
        {
            m_digSoundTimer = 0.0f;
            m_audio.playDig(id, 0.28f);
            const glm::vec3 contact = glm::vec3(hit.block) + glm::vec3(0.5f) + glm::vec3(hit.normal) * 0.5f;
            m_particles.spawnBlockHit(contact, hit.normal, id);
        }
    }
}

void Application::lightEndPortalAround(const glm::ivec3& frame)
{
    WorldEndFrames around;
    around.world = m_world.get();

    int centreX = 0;
    int centreZ = 0;
    if (!EndPortal::findRingAround(around, frame.x, frame.y, frame.z, centreX, centreZ))
        return;

    // The three-by-three inside the ring, at the frames' own level.
    const int half = EndPortal::INTERIOR / 2;
    for (int dx = -half; dx <= half; ++dx)
    {
        for (int dz = -half; dz <= half; ++dz)
        {
            const int x = centreX + dx;
            const int z = centreZ + dz;
            if (m_world->getBlock(x, frame.y, z) == Blocks::EndPortal) continue;

            m_world->setBlock(x, frame.y, z, Blocks::EndPortal);
            m_net.onLocalBlockChange(x, frame.y, z, Blocks::EndPortal);
        }
    }

    m_audio.play(Sound::Click, 0.9f, 0.5f);
}

uint32_t Application::armourProtection() const
{
    int best = 0;
    for (int i = 0; i < Inventory::ARMOR_SLOTS; ++i)
    {
        const ItemStack& worn = m_inventory.slot(Inventory::ARMOR_FIRST + i);
        if (worn.empty()) continue;
        best = std::max(best,
                        Enchanting::levelOf(worn.enchantments, Enchanting::Kind::Protection));
    }
    return Enchanting::withLevel(0u, Enchanting::Kind::Protection, best);
}

int Application::armourPoints() const
{
    int points = 0;
    for (int i = 0; i < Inventory::ARMOR_SLOTS; ++i)
    {
        const ItemStack& worn = m_inventory.slot(Inventory::ARMOR_FIRST + i);
        if (!worn.empty()) points += Armour::defencePoints(worn.id);
    }
    return points;
}

// Armour wears where it is struck: every piece takes a share of the blow,
// and a piece that gives out leaves its slot empty mid-fight, which is
// exactly how Minecraft lets you know you are in trouble.
// Whatever is being ridden, if anything. Found by asking rather than
// remembered: the mobs live in a vector that moves as it grows, so a
// pointer or an index kept between frames would go stale. Only one can
// be ridden, since there is only one rider.
Mob* Application::riddenMob()
{
    for (Mob& mob : m_entities.mobs())
        if (mob.ridden() && mob.alive()) return &mob;
    return nullptr;
}

// Rails join up with whatever is beside them. Called on the cell that
// changed and its four neighbours, since laying one rail bends the one
// it was laid against as well as itself.
namespace
{
    // The world, behind the two questions the power rule asks of it.
    struct WorldCells : Redstone::Cells
    {
        World* world = nullptr;
        Net::Session* net = nullptr;

        BlockId at(int x, int y, int z) const override { return world->getBlock(x, y, z); }

        void set(int x, int y, int z, BlockId block) override
        {
            world->setBlock(x, y, z, block);
            if (net) net->onLocalBlockChange(x, y, z, block);
        }
    };
}

// One pulse, one craft. The grid is read straight out of the table the
// chests use, and what it makes goes out on top of the block rather
// than into whatever is next to it: nothing here has a facing to put it
// through.
bool Application::fireCrafter(const glm::ivec3& block)
{
    if (!m_chests.has(block)) return false;

    Chests::Contents& held = m_chests.at(block);

    ItemStack grid[9];
    for (int i = 0; i < CRAFTER_SLOTS; ++i) grid[i] = held[i];

    const CraftOutput made = Crafting::match(grid, 3);
    if (!made.valid()) return false;

    Crafting::consume(grid, 3);
    for (int i = 0; i < CRAFTER_SLOTS; ++i) held[i] = grid[i];

    // Open in front of the player, the contents have to go back into
    // the slots they are looking at as well.
    if (m_crafterOpen && m_crafterBlock == block)
        for (int i = 0; i < CRAFTER_SLOTS; ++i)
            m_inventory.slot(Inventory::CHEST_FIRST + i) = held[i];

    const glm::vec3 out = glm::vec3(block) + glm::vec3(0.5f, 1.2f, 0.5f);
    for (int i = 0; i < made.count; ++i) m_drops.spawn(out, made.id, 1);

    m_chests.forgetIfEmpty(block);
    m_audio.play(Sound::Click, 0.7f, 1.5f);
    return true;
}

// Everything copper within sight of the player gets a roll every few
// seconds. Only what is loaded ages, which is Minecraft's rule too: a
// wall you walked away from is the colour you left it.
void Application::tickCopper(float deltaTime)
{
    if (!m_world) return;

    m_copperTimer -= deltaTime;
    if (m_copperTimer > 0.0f) return;
    m_copperTimer = Copper::AGE_INTERVAL;
    ++m_copperTick;

    const glm::ivec3 middle(glm::floor(m_player.position));
    constexpr int REACH = 24;

    for (int dx = -REACH; dx <= REACH; ++dx)
        for (int dy = -REACH; dy <= REACH; ++dy)
            for (int dz = -REACH; dz <= REACH; ++dz)
            {
                const int x = middle.x + dx, y = middle.y + dy, z = middle.z + dz;
                const BlockId here = m_world->getBlock(x, y, z);
                if (!Copper::ages(here)) continue;
                if (!Copper::ticks(x, y, z, m_world->seed(), m_copperTick)) continue;

                const BlockId older = Copper::next(here);
                m_world->setBlock(x, y, z, older);
                m_net.onLocalBlockChange(x, y, z, older);
            }
}

// A trial spawner wakes when you come near, sends its wave, and pays a
// key once the wave is down. Which spawners are awake is kept here
// rather than in the world: the block id says waiting, fighting or
// finished, and a fight only needs a timer while it is actually on.
void Application::tickTrialSpawners(float deltaTime)
{
    if (!m_world) return;

    // Looking for spawners every frame would mean sweeping the blocks
    // round the player sixty times a second for something that changes
    // about twice a minute.
    m_trialScan -= deltaTime;
    if (m_trialScan <= 0.0f)
    {
        m_trialScan = 0.5f;

        const glm::ivec3 middle(glm::floor(m_player.position));
        const int reach = static_cast<int>(Trial::WAKE_RANGE);

        for (int dx = -reach; dx <= reach; ++dx)
            for (int dy = -4; dy <= 4; ++dy)
                for (int dz = -reach; dz <= reach; ++dz)
                {
                    const glm::ivec3 at(middle.x + dx, middle.y + dy, middle.z + dz);
                    if (m_world->getBlock(at.x, at.y, at.z) != Blocks::TrialSpawner) continue;

                    const float away = glm::length(glm::vec3(at) + glm::vec3(0.5f) -
                                                   m_player.position);
                    if (away > Trial::WAKE_RANGE) continue;

                    // Out they come, in a ring round the spawner so
                    // none of them starts inside the wall.
                    int sent = 0;
                    for (int i = 0; i < Trial::WAVE_SIZE; ++i)
                    {
                        const float angle = (i / static_cast<float>(Trial::WAVE_SIZE)) *
                                            6.2831853f;
                        const glm::vec3 feet(at.x + 0.5f + std::cos(angle) * 2.0f,
                                             static_cast<float>(at.y),
                                             at.z + 0.5f + std::sin(angle) * 2.0f);
                        const MobId species =
                            Trial::speciesFor(at.x, at.y, at.z, m_world->seed(), i);
                        if (m_entities.spawnAt(species, feet, *m_world)) ++sent;
                    }

                    if (sent == 0) continue;

                    m_world->setBlock(at.x, at.y, at.z, Blocks::TrialSpawnerActive);
                    m_net.onLocalBlockChange(at.x, at.y, at.z, Blocks::TrialSpawnerActive);
                    m_trials.push_back(ActiveTrial{ at, 0.0f });

                    m_audio.play(Sound::MobZombieSay, 0.7f, 0.6f);
                    m_chatLog.add("A trial spawner wakes.");
                }
    }

    // Anything still fighting. A spawner pays up when nothing hostile
    // is left near it, and not before it has had a moment to put its
    // wave into the world.
    for (size_t i = 0; i < m_trials.size();)
    {
        ActiveTrial& trial = m_trials[i];
        trial.since += deltaTime;

        const glm::vec3 middle = glm::vec3(trial.at) + glm::vec3(0.5f);

        // Somebody broke it, or the world was reloaded under it.
        if (m_world->getBlock(trial.at.x, trial.at.y, trial.at.z) !=
            Blocks::TrialSpawnerActive)
        {
            m_trials[i] = m_trials.back();
            m_trials.pop_back();
            continue;
        }

        if (trial.since < Trial::SETTLE_SECONDS) { ++i; continue; }

        // You have to be there when it finishes. Without this, walking
        // away until the wave despawned and strolling back was a key
        // for nothing.
        if (glm::length(m_player.position - middle) > Trial::WAVE_RANGE) { ++i; continue; }

        bool anyLeft = false;
        for (const Mob& mob : m_entities.mobs())
        {
            if (!mob.alive()) continue;
            if (mob.type().spawnClass != SpawnClass::Hostile) continue;
            if (glm::length(mob.position() - middle) > Trial::WAVE_RANGE) continue;
            anyLeft = true;
            break;
        }

        if (anyLeft) { ++i; continue; }

        m_world->setBlock(trial.at.x, trial.at.y, trial.at.z, Blocks::TrialSpawnerSpent);
        m_net.onLocalBlockChange(trial.at.x, trial.at.y, trial.at.z,
                                 Blocks::TrialSpawnerSpent);

        for (int k = 0; k < Trial::KEYS_PER_SPAWNER; ++k)
            m_drops.spawn(middle + glm::vec3(0.0f, 1.0f, 0.0f), Items::TrialKey, 1);

        m_audio.play(Sound::Pickup, 0.8f, 1.4f);
        m_chatLog.add("The spawner gives up its key.");

        m_trials[i] = m_trials.back();
        m_trials.pop_back();
    }
}

// A vault takes one key and gives up what is behind it. Once.
void Application::openVault(const glm::ivec3& block)
{
    if (m_world->getBlock(block.x, block.y, block.z) != Blocks::Vault)
    {
        m_chatLog.add("This vault has already been opened.");
        m_audio.play(Sound::Click, 0.4f, 0.7f);
        return;
    }

    if (!m_inventory.take(Items::TrialKey))
    {
        m_chatLog.add("You need a trial key.");
        m_audio.play(Sound::Click, 0.4f, 0.7f);
        return;
    }

    m_world->setBlock(block.x, block.y, block.z, Blocks::VaultOpen);
    m_net.onLocalBlockChange(block.x, block.y, block.z, Blocks::VaultOpen);

    const glm::vec3 out = glm::vec3(block) + glm::vec3(0.5f, 1.2f, 0.5f);
    for (const Trial::Stack& stack : Trial::vaultLoot(block.x, block.y, block.z,
                                                      m_world->seed()))
        for (int i = 0; i < stack.count; ++i)
            m_drops.spawn(out, stack.id, 1);

    m_audio.play(Sound::Pickup, 0.9f, 0.8f);
    m_chatLog.add("The vault opens.");
}

void Application::settleRedstone(const glm::ivec3& at)
{
    WorldCells cells;
    cells.world = m_world.get();
    cells.net = &m_net;
    Redstone::update(cells, at.x, at.y, at.z);

    // Pistons answer to the same power the lamps do, and they answer
    // after it: the rule has to have finished lighting the wires
    // before anything asks what a wire is doing.
    bool moved = false;
    for (int dx = -Redstone::REACH; dx <= Redstone::REACH; ++dx)
        for (int dy = -Redstone::REACH; dy <= Redstone::REACH; ++dy)
            for (int dz = -Redstone::REACH; dz <= Redstone::REACH; ++dz)
            {
                const int x = at.x + dx, y = at.y + dy, z = at.z + dz;
                const BlockId here = cells.at(x, y, z);

                // A copper bulb flips rather than follows: one pulse
                // turns it on and the next turns it off, so a button
                // works as a switch. Which is why it needs to know
                // whether it has already answered this pulse, and why
                // the answer lives in the cell above -- there is
                // nowhere else to put a bit.
                if (isCopperBulb(here))
                {
                    const std::array<int, 3> cell = { x, y, z };
                    const bool live = Redstone::poweredAt(cells, x, y, z);
                    const bool wasLive = m_bulbLive.count(cell) > 0;
                    if (live == wasLive) continue;

                    if (live)
                    {
                        m_bulbLive.insert(cell);
                        cells.set(x, y, z, here == Blocks::CopperBulb ? Blocks::CopperBulbLit
                                                                     : Blocks::CopperBulb);
                    }
                    else
                    {
                        m_bulbLive.erase(cell);
                    }
                    continue;
                }

                // A crafter fires on the edge rather than while the
                // power is on: the block id is the only state it has,
                // and without the two ids a lit line would have it
                // crafting sixty times a second.
                if (isCrafter(here))
                {
                    const bool live = Redstone::poweredAt(cells, x, y, z);
                    if (live == (here == Blocks::CrafterLit)) continue;

                    if (live) fireCrafter(glm::ivec3(x, y, z));
                    cells.set(x, y, z, live ? Blocks::CrafterLit : Blocks::Crafter);
                    continue;
                }

                if (!isPistonBody(here)) continue;

                const bool powered = Redstone::poweredAt(cells, x, y, z);
                if (powered == pistonOut(here)) continue;

                moved = powered ? Piston::extend(cells, x, y, z)
                                : Piston::retract(cells, x, y, z);
            }

    if (moved) m_audio.play(Sound::DigStone, 0.5f, 1.6f);
}

void Application::reshapeRailsAround(const glm::ivec3& at)
{
    const glm::ivec3 around[5] = {
        at,
        { at.x, at.y, at.z - 1 },
        { at.x + 1, at.y, at.z },
        { at.x, at.y, at.z + 1 },
        { at.x - 1, at.y, at.z },
    };

    for (const glm::ivec3& cell : around)
    {
        const BlockId here = m_world->getBlock(cell.x, cell.y, cell.z);
        if (!isRail(here)) continue;

        const BlockId want = railShapeFor(
            isRail(m_world->getBlock(cell.x, cell.y, cell.z - 1)),
            isRail(m_world->getBlock(cell.x + 1, cell.y, cell.z)),
            isRail(m_world->getBlock(cell.x, cell.y, cell.z + 1)),
            isRail(m_world->getBlock(cell.x - 1, cell.y, cell.z)));

        if (want == here) continue;

        m_world->setBlock(cell.x, cell.y, cell.z, want);
        m_net.onLocalBlockChange(cell.x, cell.y, cell.z, want);
    }
}

void Application::dismount()
{
    Mob* mount = riddenMob();
    if (!mount) return;

    mount->setRidden(false);

    // Put down beside it rather than inside it, or the rider would be
    // standing in the horse and get pushed out at speed.
    const float radians = glm::radians(m_camera.yaw + 90.0f);
    m_player.position = mount->position() +
                        glm::vec3(std::cos(radians) * 1.2f, 0.0f, std::sin(radians) * 1.2f);
    m_player.velocity = glm::vec3(0.0f);

    m_audio.play(Sound::Click, 0.4f, 0.8f);
}

void Application::wearShield(int points)
{
    if (m_player.creative() || points <= 0) return;

    ItemStack& held = m_inventory.selected();
    if (held.id != Items::Shield) return;

    const uint32_t roll = static_cast<uint32_t>(held.damage * 2654435761u) ^
                          static_cast<uint32_t>(m_elapsedSeconds * 977.0f);
    if (!Enchanting::wearsThisTime(held.enchantments, roll)) return;

    held.damage += points;
    if (held.damage >= Shield::MAX_DURABILITY)
    {
        held.clear();
        m_audio.play(Sound::Click, 0.8f, 0.5f);
    }
}

void Application::wearArmour(int incoming)
{
    if (m_player.creative() || incoming <= 0) return;

    const int cost = std::max(1, incoming / 4);

    for (int i = 0; i < Inventory::ARMOR_SLOTS; ++i)
    {
        ItemStack& worn = m_inventory.slot(Inventory::ARMOR_FIRST + i);
        if (worn.empty() || !Armour::isArmour(worn.id)) continue;

        const int limit = Armour::maxDurability(worn.id);
        if (limit <= 0) continue;

        const uint32_t roll = static_cast<uint32_t>(worn.damage * 2654435761u) ^
                              static_cast<uint32_t>(m_elapsedSeconds * 977.0f) ^
                              static_cast<uint32_t>(i * 7919);
        if (!Enchanting::wearsThisTime(worn.enchantments, roll)) continue;

        worn.damage += cost;
        if (worn.damage >= limit)
        {
            worn.clear();
            m_audio.play(Sound::DigWood, 0.7f, 0.5f);
        }
    }
}

// A tool is used up a point at a time, and goes with a snap rather than
// quietly: Minecraft makes a point of telling you, because losing a
// pickaxe underground is news.
void Application::wearTool(BlockId brokenBlock)
{
    if (m_player.creative()) return;

    ItemStack& stack = m_inventory.selected();
    if (!Tools::wearsOnBlock(stack.id, brokenBlock)) return;

    const int limit = Tools::maxDurability(Tools::tierOf(stack.id));
    if (limit <= 0) return;

    // Unbreaking does not make the tool tougher, it skips the wear:
    // at level n a point lands about one time in n+1.
    const uint32_t roll = static_cast<uint32_t>(stack.damage * 2654435761u) ^
                          static_cast<uint32_t>(m_elapsedSeconds * 1013.0f);
    if (!Enchanting::wearsThisTime(stack.enchantments, roll)) return;

    if (++stack.damage >= limit)
    {
        stack.clear();
        m_audio.play(Sound::DigWood, 0.8f, 0.55f);
    }
}

// Hitting something costs a tool twice what breaking a block does,
// which is Minecraft's rule and what makes a sword a sword rather than
// just the pointiest thing you happen to have.
void Application::wearOnSwing(StackId weapon)
{
    if (m_player.creative() || !Tools::isTool(weapon)) return;

    ItemStack& stack = m_inventory.selected();
    if (stack.id != weapon) return;

    const int limit = Tools::maxDurability(Tools::tierOf(weapon));
    if (limit <= 0) return;

    stack.damage += (Tools::kindOf(weapon) == Tools::Kind::Sword) ? 1 : 2;
    if (stack.damage >= limit)
    {
        stack.clear();
        m_audio.play(Sound::DigWood, 0.8f, 0.55f);
    }
}

// A bed does two things: it is where you wake up when you die, and it
// takes you through to morning. The first works whatever the hour; the
// second only at night, the same as Minecraft.
namespace
{
    // The world, behind the two questions the portal search asks of it.
    struct PortalBlocks : Portal::Blocks
    {
        const World* world = nullptr;

        // Fully qualified: inside something deriving from
        // Portal::Blocks, a bare "Blocks" is the base class, not the
        // namespace the block ids live in.
        bool isFrame(int x, int y, int z) const override
        {
            return world->getBlock(x, y, z) == ::Blocks::Obsidian;
        }
        bool isFillable(int x, int y, int z) const override
        {
            const BlockId block = world->getBlock(x, y, z);
            return block == ::Blocks::Air || block == ::Blocks::NetherPortal;
        }
    };
}

bool Application::lightPortal(const glm::ivec3& at)
{
    PortalBlocks blocks;
    blocks.world = m_world.get();

    const Portal::Found found = Portal::findFrame(blocks, at);
    if (!found.valid) return false;

    for (const glm::ivec3& cell : found.inside)
    {
        m_world->setBlock(cell.x, cell.y, cell.z, Blocks::NetherPortal);
        m_net.onLocalBlockChange(cell.x, cell.y, cell.z, Blocks::NetherPortal);
    }

    m_audio.play(Sound::DigStone, 0.8f, 1.6f);
    m_chatLog.add("The portal opens.");
    return true;
}

// Standing in one takes you through, after a moment: long enough that
// walking across a portal does not throw you into the Nether.
void Application::updatePortal(float deltaTime)
{
    const glm::vec3 chest = m_player.position + glm::vec3(0.0f, 1.0f, 0.0f);
    const BlockId inside = m_world->getBlock(static_cast<int>(std::floor(chest.x)),
                                             static_cast<int>(std::floor(chest.y)),
                                             static_cast<int>(std::floor(chest.z)));

    // The end portal is the other kind, and it drops you through at
    // once rather than after a wait: Minecraft's does the same, and
    // there is no stepping back out of one by accident the way there is
    // with a nether portal you built across a doorway.
    if (inside == Blocks::EndPortal)
    {
        m_portalTimer = 0.0f;

        if (m_dimension != Dimension::End)
        {
            m_endHome = m_player.position;
            m_hasEndHome = true;
            travelTo(Dimension::End);
            return;
        }

        travelTo(Dimension::Overworld);
        if (m_hasEndHome)
        {
            // Back where you left, and a step clear of the portal, or
            // you land in it and are sent straight back.
            m_player.position = m_endHome + glm::vec3(0.0f, 0.0f, 2.5f);
            m_player.velocity = glm::vec3(0.0f);
            m_camera.position = m_player.eyePosition();
        }
        return;
    }

    if (inside != Blocks::NetherPortal)
    {
        m_portalTimer = 0.0f;
        return;
    }

    m_portalTimer += deltaTime;
    if (m_portalTimer < PORTAL_SECONDS) return;

    m_portalTimer = 0.0f;

    if (m_dimension == Dimension::Overworld)
    {
        // Remember the way back before leaving by it.
        m_portalHome = m_player.position;
        m_hasPortalHome = true;
        m_owedReturnPortal = true;
        travelTo(Dimension::Nether);
        return;
    }

    // Going home. A portal is already standing where you left, so
    // nothing needs building and nothing needs digging out.
    m_owedReturnPortal = !m_hasPortalHome;
    travelTo(Dimension::Overworld);

    if (m_hasPortalHome)
    {
        m_player.position = m_portalHome + glm::vec3(0.0f, 0.0f, 1.2f);
        m_player.velocity = glm::vec3(0.0f);
        m_camera.position = m_player.eyePosition();
    }
}

// Somewhere to come back through. Minecraft builds one where you arrive
// if there is not one already, and without it the Nether is one way.
void Application::buildReturnPortal()
{
    if (!m_owedReturnPortal || !m_worldReady) return;
    m_owedReturnPortal = false;

    glm::ivec3 feet(static_cast<int>(std::floor(m_player.position.x)),
                    static_cast<int>(std::floor(m_player.position.y)),
                    static_cast<int>(std::floor(m_player.position.z)));

    // The Nether is mostly solid rock, so wherever you land is likely
    // inside it. Look for open air near by first -- arriving in a cave
    // is better than arriving in a hole of your own making -- and settle
    // for digging one out only if there is none.
    auto clearAt = [&](const glm::ivec3& at) {
        for (int b = 0; b < 3; ++b)
            if (isSolid(m_world->getBlock(at.x, at.y + b, at.z))) return false;
        return isSolid(m_world->getBlock(at.x, at.y - 1, at.z));
    };

    for (int drop = 0; drop < 48; ++drop)
    {
        const glm::ivec3 below(feet.x, feet.y - drop, feet.z);
        if (below.y < 8) break;
        if (clearAt(below)) { feet = below; break; }
    }

    // A chamber to stand up in, whether it was already there or not.
    for (int a = -3; a <= 3; ++a)
        for (int b = 0; b <= 4; ++b)
            for (int c = -3; c <= 3; ++c)
                m_world->setBlock(feet.x + a, feet.y + b, feet.z + c, Blocks::Air);

    // A floor under all of it, so nothing drops into the lava below.
    for (int a = -3; a <= 3; ++a)
        for (int c = -3; c <= 3; ++c)
            m_world->setBlock(feet.x + a, feet.y - 1, feet.z + c, Blocks::Obsidian);

    // And the frame itself, standing in the middle of the chamber.
    for (int a = -1; a <= 2; ++a)
        for (int b = -1; b <= 4; ++b)
        {
            const bool edge = (a == -1 || a == 2 || b == -1 || b == 4);
            const BlockId put = edge ? Blocks::Obsidian : Blocks::Air;
            m_world->setBlock(feet.x + a, feet.y + b, feet.z, put);
        }

    lightPortal(glm::ivec3(feet.x, feet.y, feet.z));

    m_player.position = glm::vec3(feet.x + 0.5f, static_cast<float>(feet.y), feet.z + 1.5f);
    m_player.velocity = glm::vec3(0.0f);

    // Looking away from it. You step out of a portal with your back to
    // it; arriving nose-first against one fills the screen with purple
    // and leaves you unsure which way you came.
    m_camera.yaw = 90.0f;
    m_camera.pitch = 0.0f;
    m_camera.addLook(0.0f, 0.0f, 0.0f);
    m_camera.position = m_player.eyePosition();
}

// Swinging a door. Both halves move together: a door with its top shut
// and its bottom open is not a door anyone has ever seen.
void Application::swingDoor(const glm::ivec3& block)
{
    const BlockId here = m_world->getBlock(block.x, block.y, block.z);
    if (!isDoor(here)) return;

    const bool open = !doorIsOpen(here);
    const int facing = blockFacing(here);

    const int lowY = doorIsTop(here) ? block.y - 1 : block.y;

    for (int half = 0; half < 2; ++half)
    {
        const int y = lowY + half;
        if (!isDoor(m_world->getBlock(block.x, y, block.z))) continue;

        const BlockId put = doorPart(facing, half == 1, open);
        m_world->setBlock(block.x, y, block.z, put);
        m_net.onLocalBlockChange(block.x, y, block.z, put);
    }

    m_audio.play(Sound::DigWood, 0.6f, open ? 1.2f : 0.9f);
}

void Application::sleepInBed(const glm::ivec3& block)
{
    const glm::vec3 beside = glm::vec3(block) + glm::vec3(0.5f, 1.0f, 0.5f);
    m_spawnPoint = beside;

    if (!Sleep::isNight(m_timeOfDay))
    {
        m_chatLog.add("You can only sleep at night. Spawn point set.");
        m_audio.play(Sound::Click, 0.4f);
        return;
    }

    m_timeOfDay = Sleep::wakeTime();

    // A night's rest. Minecraft heals you slowly through the night; with
    // no clock running while you sleep, the rest is given at once.
    if (!m_player.creative() && m_player.health > 0)
        m_player.heal(Player::MAX_HEALTH / 4);

    m_chatLog.add("Good morning. Spawn point set.");
    m_audio.play(Sound::DigWool, 0.6f, 0.8f);
}

// Lighting a stick. The block is swapped for its lit twin, which glows
// and flashes white, and a fuse is written down against it.
void Application::primeTnt(const glm::ivec3& block)
{
    if (m_world->getBlock(block.x, block.y, block.z) != Blocks::Tnt) return;

    m_world->setBlock(block.x, block.y, block.z, Blocks::TntPrimed);
    m_net.onLocalBlockChange(block.x, block.y, block.z, Blocks::TntPrimed);

    m_fuses.emplace_back(block, TNT_FUSE_SECONDS);
    m_audio.play(Sound::Click, 0.8f, 0.7f);
}

void Application::updateFuses(float deltaTime)
{
    for (size_t i = 0; i < m_fuses.size();)
    {
        const glm::ivec3 where = m_fuses[i].first;

        // Mined, or blown up by something else, before it went off.
        if (m_world->getBlock(where.x, where.y, where.z) != Blocks::TntPrimed)
        {
            m_fuses[i] = m_fuses.back();
            m_fuses.pop_back();
            continue;
        }

        m_fuses[i].second -= deltaTime;
        if (m_fuses[i].second > 0.0f) { ++i; continue; }

        m_world->setBlock(where.x, where.y, where.z, Blocks::Air);
        m_net.onLocalBlockChange(where.x, where.y, where.z, Blocks::Air);

        m_fuses[i] = m_fuses.back();
        m_fuses.pop_back();

        // After it is taken out of the world, so it does not have to
        // decide whether to destroy itself.
        applyBlast(glm::vec3(where) + glm::vec3(0.5f),
                   TNT_RADIUS, TNT_DAMAGE);
    }
}

void Application::applyBlast(const glm::vec3& at, float radius, int damage)
{
    m_audio.play(Sound::DigStone, 1.0f, 0.5f);
    m_particles.spawnBlockBreak(glm::ivec3(glm::floor(at)), Blocks::Gravel);

    // Blocks first, so anything the blast opens up is already gone when
    // the drops are thrown out into it.
    const int reach = static_cast<int>(std::ceil(radius));
    const glm::ivec3 middle = glm::ivec3(glm::floor(at));

    for (int dx = -reach; dx <= reach; ++dx)
        for (int dy = -reach; dy <= reach; ++dy)
            for (int dz = -reach; dz <= reach; ++dz)
            {
                const glm::ivec3 where = middle + glm::ivec3(dx, dy, dz);
                const BlockId block = m_world->getBlock(where.x, where.y, where.z);

                const glm::vec3 centre = glm::vec3(where) + glm::vec3(0.5f);
                const float distance = glm::length(centre - at);

                // A stick caught in a blast is lit rather than broken,
                // which is what makes a stack of it go up together.
                if (block == Blocks::Tnt && distance < radius)
                {
                    primeTnt(where);
                    // A short fuse, so a chain goes off in a run rather
                    // than all at once.
                    if (!m_fuses.empty()) m_fuses.back().second = 0.3f + distance * 0.1f;
                    continue;
                }

                if (!Explosion::destroys(block, distance, radius)) continue;

                // A chest or a furnace gives up what was in it rather
                // than taking it with it.
                if (block == Blocks::Chest && m_chests.has(where))
                {
                    for (const ItemStack& held : m_chests.at(where))
                        if (!held.empty())
                            m_drops.spawnFromBrokenBlock(where, held.id, held.count);
                    m_chests.remove(where);
                }
                if ((block == Blocks::Furnace || block == Blocks::FurnaceLit) &&
                    m_furnaces.has(where))
                {
                    const Furnaces::State& inside = m_furnaces.at(where);
                    for (const ItemStack* held : { &inside.input, &inside.fuel, &inside.output })
                        if (!held->empty())
                            m_drops.spawnFromBrokenBlock(where, held->id, held->count);
                    m_furnaces.remove(where);
                }

                m_world->setBlock(where.x, where.y, where.z, Blocks::Air);
                m_net.onLocalBlockChange(where.x, where.y, where.z, Blocks::Air);

                // Most of what a blast breaks is destroyed outright;
                // about a third of it survives to be picked up.
                const uint32_t roll = static_cast<uint32_t>(
                    where.x * 73856093 ^ where.y * 19349663 ^ where.z * 83492791);
                if (roll % 3 == 0)
                {
                    const StackId drop = blockDrop(block);
                    if (drop != Blocks::Air) m_drops.spawnFromBrokenBlock(where, drop);
                }
            }

    // Then everything living, the player included.
    const glm::vec3 chest = m_player.position + glm::vec3(0.0f, Player::HEIGHT * 0.5f, 0.0f);
    const float toPlayer = glm::length(chest - at);
    const int onPlayer = Explosion::damageAt(toPlayer, radius, damage);

    if (onPlayer > 0 && m_player.damage(onPlayer))
    {
        wearArmour(onPlayer);
        m_audio.play(Sound::Hurt, 0.9f);

        const glm::vec3 away = chest - at;
        if (glm::length(away) > 0.001f)
            m_player.velocity += glm::normalize(away) * 8.0f;
    }

    for (Mob& mob : m_entities.mobs())
    {
        if (!mob.alive()) continue;

        const glm::vec3 body = mob.position() + glm::vec3(0.0f, mob.height() * 0.5f, 0.0f);
        const int hurt = Explosion::damageAt(glm::length(body - at), radius, damage);
        if (hurt > 0) mob.damage(hurt, body - at);
    }
}

// Drawing the bow. Held down it winds up; let go it looses whatever it
// has wound up, which is what makes a snap shot weaker than a aimed one.
void Application::updateBow(float deltaTime)
{
    const ItemStack& held = m_inventory.selected();
    const bool holdingBow = inGame() && held.id == Items::Bow;
    const bool pulling = holdingBow && m_keys.down(m_input, Action::Use);

    if (pulling)
    {
        // Nothing to loose means nothing to draw, unless you are
        // building, where arrows come from nowhere.
        if (!m_player.creative() && !m_inventory.has(Items::Arrow))
        {
            m_bowDrawing = false;
            m_bowDraw = 0.0f;
            return;
        }

        m_bowDrawing = true;
        m_bowDraw += deltaTime;
        return;
    }

    if (!m_bowDrawing) return;

    const float drawn = Arrows::drawFraction(m_bowDraw);
    m_bowDrawing = false;
    m_bowDraw = 0.0f;

    if (Arrows::worthLoosing(drawn)) looseArrow(drawn);
}

void Application::looseArrow(float drawn)
{
    if (!m_player.creative() && !m_inventory.take(Items::Arrow)) return;

    const glm::vec3 from = m_camera.position + m_camera.front * 0.4f;
    m_arrows.spawn(from, m_camera.front * Arrows::speedFor(drawn),
                   Arrows::damageFor(drawn) +
                       Enchanting::powerBonus(m_inventory.selected().enchantments,
                                              Arrows::damageFor(drawn)),
                   true);

    m_heldItem.swing();
    m_audio.play(Sound::Click, 0.5f, 1.4f);
}

// Everything an arrow might hit that is not a block. A player's arrow
// looks for mobs; a skeleton's looks for the player.
namespace
{
    struct ArrowTargets : Projectiles::Targets
    {
        // Where the player is looking, for the shield. The player
        // itself does not carry this: the camera does.
        glm::vec3 facing{ 0.0f };
        int shotsBlocked = 0;

        EntityManager* entities = nullptr;
        Player* player = nullptr;
        int playerHits = 0;

        // How long a blaze's charge leaves whatever it hits burning.
        static constexpr float FIRE_SECONDS = 5.0f;

        // Set for the one strike that a charge is making, so the hit
        // itself stays the plain three-argument rule everything else
        // implements.
        bool burning = false;

        bool strike(const glm::vec3& point, int damage, bool fromPlayer,
                    Arrows::Kind kind) override
        {
            burning = kind == Arrows::Kind::Fire;
            return strike(point, damage, fromPlayer);
        }

        // A wind charge does its work here. The shove is round where it
        // went off, not on whatever stopped it: one thrown at your own
        // feet throws you into the air, and that is most of what
        // anybody uses one for.
        void burst(const glm::vec3& at, Arrows::Kind kind, bool fromPlayer) override
        {
            (void)fromPlayer;
            if (kind != Arrows::Kind::Wind) return;

            bursts.push_back(at);

            const glm::vec3 push = Arrows::windImpulse(at, player->position);
            if (glm::length(push) > 0.0f)
            {
                player->velocity += push;

                // Thrown clear by the wind, you land as if you had
                // stepped off from here: the fall that follows is the
                // charge's doing, not yours.
                player->clearFall();
            }

            for (Mob& mob : entities->mobs())
            {
                if (!mob.alive()) continue;
                const glm::vec3 shove = Arrows::windImpulse(at, mob.position());
                if (glm::length(shove) > 0.0f) mob.shove(shove);
            }
        }

        std::vector<glm::vec3> bursts;

        bool strike(const glm::vec3& point, int damage, bool fromPlayer) override
        {
            if (fromPlayer)
            {
                for (Mob& mob : entities->mobs())
                {
                    if (!mob.alive()) continue;

                    const glm::vec3 feet = mob.position();
                    const float half = mob.width() * 0.5f;
                    const glm::vec3 apart = point - feet;

                    if (std::abs(apart.x) > half || std::abs(apart.z) > half) continue;
                    if (apart.y < 0.0f || apart.y > mob.height()) continue;

                    mob.damage(damage, feet - point);
                    return true;
                }
                return false;
            }

            // A skeleton's arrow, looking for the player.
            const glm::vec3 feet = player->position;
            const glm::vec3 apart = point - feet;

            if (std::abs(apart.x) > 0.4f || std::abs(apart.z) > 0.4f) return false;
            if (apart.y < 0.0f || apart.y > Player::HEIGHT) return false;

            // A raised shield stops an arrow the same way it stops a
            // blow, and from the same arc. Letting arrows through was
            // the shield's one honest gap.
            if (player->blocking && Shield::covers(facing, feet, point))
            {
                ++shotsBlocked;
                return true;
            }

            if (player->damage(damage)) ++playerHits;

            // A charge sets them alight whether or not the blow itself
            // landed: the burn is not the hit, and the hurt cooldown
            // has nothing to say about it.
            if (burning) player->ignite(FIRE_SECONDS);

            // It stops whether or not the blow landed: an arrow does not
            // pass through someone just because they are still shaking
            // off the last one.
            return true;
        }
    };
}

void Application::updateArrows(float deltaTime)
{
    ArrowTargets targets;
    targets.entities = &m_entities;
    targets.player = &m_player;
    targets.facing = m_camera.front;

    m_arrows.update(deltaTime, *m_world, targets);

    if (targets.playerHits > 0)
    {
        m_audio.play(Sound::Hurt, 0.8f);
        wearArmour(2);
    }

    if (targets.shotsBlocked > 0)
    {
        m_audio.play(Sound::Click, 0.7f, 0.7f);
        wearShield(Shield::WEAR_PER_BLOCK * targets.shotsBlocked);
    }

    updateBow(deltaTime);
}

void Application::handleEating(float deltaTime)
{
    ItemStack& stack = m_inventory.selected();
    const bool holding = inGame() && !m_player.creative() && m_keys.down(m_input, Action::Use);

    const Food::Bite bite = Food::chew(holding && !stack.empty(), stack.id,
                                       m_player.canEat(), m_eatTimer, deltaTime);
    m_eatTimer = bite.timer;
    if (!bite.swallowed) return;

    const FoodValue value = Food::valueOf(stack.id);
    if (!m_player.eat(value.hunger, value.saturation)) return;

    stack.count -= 1;
    if (stack.count <= 0) stack.clear();
    m_audio.play(Sound::Pickup, 0.6f, 0.85f);
}

// Drinking is eating with a different payload, but it does not go
// through Food: a potion fills nothing, and a full player must still be
// able to drink one.
void Application::handleDrinking(float deltaTime)
{
    ItemStack& stack = m_inventory.selected();
    const bool holding = inGame() && m_keys.down(m_input, Action::Use) &&
                         stack.id == Items::Potion && !stack.empty();

    if (!holding)
    {
        m_drinkTimer = 0.0f;
        return;
    }

    m_drinkTimer += deltaTime;
    if (m_drinkTimer < Food::EAT_SECONDS) return;
    m_drinkTimer = 0.0f;

    const Brewing::Potion potion = Brewing::fromDamage(stack.damage);

    Effects::Kind kind;
    int amplifier = 0;
    float seconds = 0.0f;
    if (Brewing::effectOf(potion, kind, amplifier, seconds))
        Effects::give(m_player.effects, kind, amplifier, seconds);

    // The glass comes back whatever was in it, water included.
    stack.count -= 1;
    if (stack.count <= 0) stack.clear();
    if (m_inventory.add(Items::GlassBottle, 1) > 0)
        m_drops.spawn(m_player.position, Items::GlassBottle, 1);

    m_audio.play(Sound::Pickup, 0.6f, 1.25f);
}

void Application::handlePlacement()
{
    if (m_keys.pressed(m_input, Action::PickBlock))
    {
        const RaycastHit hit = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
        if (hit.hit)
        {
            const BlockId picked = m_world->getBlock(hit.block.x, hit.block.y, hit.block.z);
            if (isObtainable(picked))
            {
                ItemStack& stack = m_inventory.selected();
                stack.id = picked;
                stack.count = std::max(stack.count, m_player.creative() ? 64 : 1);
            }
        }
        return;
    }

    if (!m_keys.pressed(m_input, Action::Use)) return;

    // A bench is opened by using it, whatever is in your hand -- unless
    // you are sneaking, which is how Minecraft lets you place a block
    // against one rather than open it.
    // A villager is traded with by using it, the same as a bench is
    // opened by using it. Checked before the block ray, so standing one
    // in front of a chest does not open the chest through it.
    if (!m_player.sneaking)
    {
        if (const Mob* villager = m_entities.pick(m_camera.position, m_camera.front,
                                                  REACH_DISTANCE))
        {
            if (villager->typeId() == MobId::Villager && villager->alive())
            {
                openTrading(*villager);
                m_audio.play(Sound::MobVillagerSay, 0.6f);
                return;
            }
        }
    }

    if (!m_player.sneaking)
    {
        const RaycastHit bench = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
        if (bench.hit)
        {
            const BlockId used = m_world->getBlock(bench.block.x, bench.block.y, bench.block.z);
            if (used == Blocks::CraftingTable)
            {
                openBench(bench.block);
                m_audio.play(Sound::Click, 0.4f);
                return;
            }
            if (used == Blocks::Furnace || used == Blocks::FurnaceLit)
            {
                openFurnace(bench.block);
                m_audio.play(Sound::Click, 0.4f);
                return;
            }
            if (used == Blocks::Chest)
            {
                openChest(bench.block);
                m_audio.play(Sound::DigWood, 0.5f, 0.8f);
                return;
            }
            // An eye set into a frame. Done by using it, like a door,
            // rather than by placing a block: the eye goes into the
            // frame that is already there.
            if (used == Blocks::EndPortalFrame &&
                m_inventory.selected().id == Items::EyeOfEnder)
            {
                m_world->setBlock(bench.block.x, bench.block.y, bench.block.z,
                                  Blocks::EndPortalFrameFilled);
                m_net.onLocalBlockChange(bench.block.x, bench.block.y, bench.block.z,
                                         Blocks::EndPortalFrameFilled);
                m_audio.play(Sound::Click, 0.6f, 1.5f);

                if (!m_player.creative())
                {
                    ItemStack& held = m_inventory.selected();
                    held.count -= 1;
                    if (held.count <= 0) held.clear();
                }

                lightEndPortalAround(bench.block);
                return;
            }

            if (used == Blocks::EnchantingTable)
            {
                openEnchantingTable(bench.block);
                m_audio.play(Sound::Click, 0.4f);
                return;
            }
            if (used == Blocks::Anvil)
            {
                openAnvil(bench.block);
                m_audio.play(Sound::Click, 0.4f);
                return;
            }
            if (used == Blocks::BrewingStand)
            {
                openBrewingStand(bench.block);
                m_audio.play(Sound::Click, 0.4f);
                return;
            }
            if (used == Blocks::Bed)
            {
                sleepInBed(bench.block);
                return;
            }
            if (isDoor(used))
            {
                swingDoor(bench.block);
                return;
            }

            if (isVault(used))
            {
                openVault(bench.block);
                return;
            }

            if (isCrafter(used))
            {
                openCrafter(bench.block);
                m_audio.play(Sound::Click, 0.4f);
                return;
            }

            // A lever stays where you put it; a button lets go on its
            // own a second later, which is the whole difference
            // between the two.
            if (used == Blocks::LeverOff || used == Blocks::LeverOn ||
                used == Blocks::ButtonOff)
            {
                const BlockId now = Redstone::toggled(used);
                m_world->setBlock(bench.block.x, bench.block.y, bench.block.z, now);
                m_net.onLocalBlockChange(bench.block.x, bench.block.y, bench.block.z, now);
                settleRedstone(bench.block);
                m_audio.play(Sound::Click, 0.6f, now == Blocks::LeverOff ? 0.8f : 1.3f);

                if (now == Blocks::ButtonOn)
                {
                    m_buttonAt = bench.block;
                    m_buttonTimer = BUTTON_SECONDS;
                }
                return;
            }
        }
    }

    ItemStack& stack = m_inventory.selected();
    if (stack.empty()) return;

    // A glass bottle fills from water without taking the block, which
    // is the one way a bottle differs from a bucket -- otherwise every
    // potion would cost a pond.
    if (stack.id == Items::GlassBottle)
    {
        const RaycastHit scoop = m_world->raycast(m_camera.position, m_camera.front,
                                                  REACH_DISTANCE, true);
        if (!scoop.hit) return;
        if (m_world->getBlock(scoop.block.x, scoop.block.y, scoop.block.z) != Blocks::Water)
            return;

        ItemStack filled;
        filled.id = Items::Potion;
        filled.count = 1;
        filled.damage = Brewing::toDamage(Brewing::Potion::Water);

        stack.count -= 1;
        if (stack.count <= 0) stack.clear();
        if (m_inventory.addStack(filled) > 0)
            m_drops.spawnStack(m_player.position, filled);

        m_audio.play(Sound::Pickup, 0.5f, 1.1f);
        return;
    }

    // Buckets: fill from a liquid you are looking at, or empty into the
    // space in front of one. The liquids here do not flow, so a bucket
    // places a single block rather than a spreading source.
    if (stack.id == Items::Bucket || stack.id == Items::WaterBucket ||
        stack.id == Items::LavaBucket)
    {
        const RaycastHit scoop = m_world->raycast(m_camera.position, m_camera.front,
                                                  REACH_DISTANCE, true);
        if (!scoop.hit) return;

        const BlockId looking = m_world->getBlock(scoop.block.x, scoop.block.y, scoop.block.z);

        if (stack.id == Items::Bucket)
        {
            if (!isLiquid(looking)) return;

            m_world->setBlock(scoop.block.x, scoop.block.y, scoop.block.z, Blocks::Air);
            m_net.onLocalBlockChange(scoop.block.x, scoop.block.y, scoop.block.z, Blocks::Air);

            const StackId filled = (looking == Blocks::Lava) ? Items::LavaBucket
                                                             : Items::WaterBucket;
            // A stack of empty buckets fills one of them, and the full
            // one has to go somewhere.
            stack.count -= 1;
            if (stack.count <= 0) stack.clear();
            if (m_inventory.add(filled, 1) > 0) m_drops.spawn(m_player.position, filled, 1);

            m_audio.play(Sound::Pickup, 0.5f, 0.8f);
            return;
        }

        // Emptying it. Into the liquid's own cell if that is what is
        // being looked at, otherwise into the face in front of it.
        const glm::ivec3 into = isLiquid(looking) ? scoop.block : scoop.previous;
        const BlockId there = m_world->getBlock(into.x, into.y, into.z);
        if (there != Blocks::Air && !isLiquid(there)) return;

        const BlockId poured = (stack.id == Items::LavaBucket) ? Blocks::Lava : Blocks::Water;
        m_world->setBlock(into.x, into.y, into.z, poured);
        m_net.onLocalBlockChange(into.x, into.y, into.z, poured);

        stack.id = Items::Bucket;
        stack.count = 1;
        m_audio.play(Sound::Place, 0.6f, 0.9f);
        return;
    }

    // Flint and steel lights a portal, and nothing else yet.
    if (stack.id == Items::FlintAndSteel)
    {
        const RaycastHit spark = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
        if (spark.hit && lightPortal(spark.previous))
        {
            if (!m_player.creative())
            {
                stack.damage += 1;
                if (stack.damage >= 64) stack.clear();
            }
        }
        return;
    }

    // A piece of armour held in the hand is put on rather than placed.
    if (Armour::isArmour(stack.id))
    {
        if (m_inventory.wear(stack))
        {
            m_player.armourPoints = armourPoints();
            m_audio.play(Sound::DigWool, 0.7f, 0.9f);
        }
        return;
    }

    // A tamed wolf in front of you is being told to sit or to get up,
    // whatever is in your hand. This comes before feeding, or offering
    // one a bone would only ever feed it.
    if (Mob* told = m_entities.toggleSit(m_camera.position, m_camera.front, ATTACK_REACH))
    {
        m_audio.play(Sound::Click, 0.4f, told->sitting() ? 0.8f : 1.2f);
        return;
    }

    // Getting on something comes before all of it: a tamed horse in
    // front of you is to be ridden, not fed.
    if (Mob* ride = m_entities.rideable(m_camera.position, m_camera.front, ATTACK_REACH))
    {
        ride->setRidden(true);
        m_player.velocity = glm::vec3(0.0f);
        m_audio.play(Sound::Click, 0.5f, 0.9f);
        return;
    }

    // A boat is set down on the water in front of you, or on the
    // ground if that is what you are looking at -- the same as vanilla,
    // where a boat on land is simply a slow boat.
    if (stack.id == Items::Boat)
    {
        const RaycastHit shore = m_world->raycast(m_camera.position, m_camera.front,
                                                  REACH_DISTANCE, true);
        if (!shore.hit) return;

        const BlockId looking = m_world->getBlock(shore.block.x, shore.block.y, shore.block.z);
        const glm::ivec3 cell = isLiquid(looking) ? shore.block : shore.previous;

        // Sat on top of the water rather than in it, so it starts the
        // right way up and settles down to the waterline.
        const float lift = isLiquid(looking) ? 0.85f : 0.05f;
        const glm::vec3 where(cell.x + 0.5f, cell.y + lift, cell.z + 0.5f);

        if (!m_entities.spawnAt(MobId::Boat, where, *m_world)) return;

        if (!m_player.creative())
        {
            stack.count -= 1;
            if (stack.count <= 0) stack.clear();
        }
        m_audio.play(Sound::Place, 0.7f, 0.9f);
        return;
    }

    // A minecart only goes down on a rail, which is the one place it
    // is any use.
    if (stack.id == Items::Minecart)
    {
        const RaycastHit track = m_world->raycast(m_camera.position, m_camera.front,
                                                  REACH_DISTANCE);
        if (!track.hit) return;

        glm::ivec3 cell = track.block;
        if (!isRail(m_world->getBlock(cell.x, cell.y, cell.z))) cell = track.previous;
        if (!isRail(m_world->getBlock(cell.x, cell.y, cell.z))) return;

        const glm::vec3 where(cell.x + 0.5f,
                              static_cast<float>(cell.y) + Mob::RAIL_LIFT,
                              cell.z + 0.5f);
        if (!m_entities.spawnAt(MobId::Minecart, where, *m_world)) return;

        if (!m_player.creative())
        {
            stack.count -= 1;
            if (stack.count <= 0) stack.clear();
        }
        m_audio.play(Sound::Place, 0.7f, 0.8f);
        return;
    }

    // A bone offered to a wolf tames it, which comes before feeding
    // for the same reason feeding comes before placing: what is in
    // front of you takes it.
    if (m_entities.tame(m_camera.position, m_camera.front, ATTACK_REACH, stack.id))
    {
        if (!m_player.creative())
        {
            stack.count -= 1;
            if (stack.count <= 0) stack.clear();
        }
        m_audio.play(Sound::Pickup, 0.6f, 1.5f);
        return;
    }

    // Feeding comes first: an animal in front of you takes the wheat
    // rather than the block landing behind it.
    if (isBreedingFood(stack.id) &&
        m_entities.feed(m_camera.position, m_camera.front, ATTACK_REACH, stack.id))
    {
        if (!m_player.creative())
        {
            stack.count -= 1;
            if (stack.count <= 0) stack.clear();
        }
        m_audio.play(Sound::Pickup, 0.5f, 1.4f);
        return;
    }

    // Sowing. Seeds till the ground they are used on and plant
    // themselves in one action -- Minecraft wants a hoe first, and there
    // are no tools yet.
    if (stack.id == Items::WheatSeeds)
    {
        const RaycastHit soil = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
        if (soil.hit && Farming::plant(*m_world, soil.block))
        {
            m_net.onLocalBlockChange(soil.block.x, soil.block.y, soil.block.z, Blocks::Farmland);
            m_net.onLocalBlockChange(soil.block.x, soil.block.y + 1, soil.block.z, Blocks::Wheat0);
            m_audio.playPlace(Blocks::Farmland);
            if (!m_player.creative())
            {
                stack.count -= 1;
                if (stack.count <= 0) stack.clear();
            }
        }
        return;
    }

    // A wind charge is thrown rather than placed, which is the only
    // thing in the game you use by aiming it at nothing.
    if (stack.id == Items::WindCharge)
    {
        const glm::vec3 heading = glm::normalize(m_camera.front);
        m_arrows.spawn(m_player.eyePosition() + heading * 0.6f, heading * 18.0f,
                       1, true, Arrows::Kind::Wind);
        m_audio.play(Sound::Click, 0.6f, 1.7f);

        if (!m_player.creative())
        {
            stack.count -= 1;
            if (stack.count <= 0) stack.clear();
        }
        return;
    }

    // Items are not blocks and never become one: a handful of wheat
    // cannot be set down in the world the way a plank can. Dust is the
    // exception -- it is an item in the hand and a wire on the floor,
    // the same way a door is.
    BlockId fromItem = Blocks::Air;
    if (stack.id == Items::Redstone) fromItem = Blocks::RedstoneDust;
    if (isItem(stack.id) && fromItem == Blocks::Air) return;

    const RaycastHit hit = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
    if (!hit.hit) return;

    const glm::ivec3 target = hit.previous;
    const BlockId existing = m_world->getBlock(target.x, target.y, target.z);
    if (existing != Blocks::Air && !isLiquid(existing)) return;

    // Don't entomb the player in their own block.
    const AABB box = m_player.aabb();
    const bool overlapsPlayer =
        box.max.x > target.x && box.min.x < target.x + 1 &&
        box.max.y > target.y && box.min.y < target.y + 1 &&
        box.max.z > target.z && box.min.z < target.z + 1;
    BlockId placing = (fromItem != Blocks::Air) ? fromItem : asBlock(stack.id);
    if (overlapsPlayer && isSolid(placing)) return;

    // Cane only stands at the water's edge, or on cane already there.
    // Without this it could be planted down a mineshaft, which is the
    // one thing everybody tries first.
    if (placing == Blocks::SugarCane)
    {
        WorldCaneBlocks around;
        around.world = m_world.get();
        if (!Cane::canStandAt(around, target.x, target.y, target.z)) return;
    }

    // A door is two blocks tall and wants headroom, and it faces the
    // way you do so it opens away from you.
    if (isDoor(placing))
    {
        const BlockId above = m_world->getBlock(target.x, target.y + 1, target.z);
        if (above != Blocks::Air && !isLiquid(above)) return;

        const glm::vec3 look = m_camera.front;
        const int facing = (std::fabs(look.x) > std::fabs(look.z))
                               ? (look.x > 0.0f ? 3 : 1)
                               : (look.z > 0.0f ? 0 : 2);

        const BlockId low = doorPart(facing, false, false);
        const BlockId high = doorPart(facing, true, false);

        m_world->setBlock(target.x, target.y, target.z, low);
        m_world->setBlock(target.x, target.y + 1, target.z, high);
        m_net.onLocalBlockChange(target.x, target.y, target.z, low);
        m_net.onLocalBlockChange(target.x, target.y + 1, target.z, high);

        m_audio.playPlace(low);
        if (!m_player.creative())
        {
            stack.count -= 1;
            if (stack.count <= 0) stack.clear();
        }
        return;
    }

    // A piston points away from you, so it pushes what you are looking
    // at rather than back at your own feet.
    if (isPistonBody(placing))
    {
        const glm::vec3 look = m_camera.front;
        const int facing = (std::fabs(look.x) > std::fabs(look.z))
                               ? (look.x > 0.0f ? 1 : 3)
                               : (look.z > 0.0f ? 2 : 0);
        placing = pistonBody(facing, pistonSticky(placing), false);
    }

    // Stairs face the way you are looking, so you walk up them rather
    // than into them. Minecraft takes the direction from the player,
    // not the face clicked, and so does this.
    if (isStairs(placing))
    {
        const glm::vec3 look = m_camera.front;
        const int facing = (std::fabs(look.x) > std::fabs(look.z))
                               ? (look.x > 0.0f ? 3 : 1)    // walking east, step faces west
                               : (look.z > 0.0f ? 0 : 2);
        placing = stairsFacing(placing, facing);
    }

    // A ladder hangs on whichever wall you aimed at, so the id it is
    // placed as is not quite the id in your hand.
    if (isLadder(placing))
    {
        // The normal points out of the face that was clicked, so the
        // wall is the other way.
        const glm::ivec3 n = hit.normal;
        if (n.y != 0) return;   // nothing to hang it on

        const int facing = (n.z > 0) ? 0 : (n.x < 0) ? 1 : (n.z < 0) ? 2 : 3;
        placing = ladderFacing(facing);
    }

    // A rail is nailed to the floor, and bends to meet whatever is
    // already beside it. Which of the six shapes it lands as is worked
    // out after it is down, along with its neighbours.
    if (isRail(placing) &&
        !isSolid(m_world->getBlock(target.x, target.y - 1, target.z)))
        return;

    // So is dust, and so are the switches and the torches: all of them
    // want something under them to sit on.
    if ((isFlat(placing) || Redstone::isSource(placing) ||
         placing == Blocks::RedstoneTorch || placing == Blocks::LeverOff ||
         placing == Blocks::ButtonOff) &&
        !isSolid(m_world->getBlock(target.x, target.y - 1, target.z)))
        return;

    m_world->setBlock(target.x, target.y, target.z, placing);
    m_net.onLocalBlockChange(target.x, target.y, target.z, placing);
    m_audio.playPlace(placing);

    if (isRail(placing)) reshapeRailsAround(target);
    settleRedstone(target);

    if (!m_player.creative())
    {
        stack.count -= 1;
        if (stack.count <= 0) stack.clear();
    }
}

// --------------------------------------------------------------- render ---

void Application::render()
{
    glViewport(0, 0, m_window.width(), m_window.height());
    const glm::vec3 clear = (m_world && inGame()) ? dimensionBackdrop()
                                                 : glm::vec3(0.5f, 0.72f, 0.95f);
    glClearColor(clear.r, clear.g, clear.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_world) renderWorld();
    else renderMenuBackdrop(0.0f);

    // The HUD lights its block icons off the pack's own normal maps,
    // so a cobblestone in the hotbar is the cobblestone underfoot and
    // not a flat sticker of it. Zero when no pack is installed, which
    // draws them the way they always were.
    m_ui.setNormalAtlas((m_pbrEnabled && m_atlas.hasPbrMaps()) ? m_atlas.normalTextureId() : 0);
    m_ui.begin(m_window.width(), m_window.height());

    if (m_world && !m_worldReady)
    {
        const float w = static_cast<float>(m_window.width());
        const float h = static_cast<float>(m_window.height());
        m_ui.quad(0, 0, w, h, glm::vec4(0.04f, 0.05f, 0.07f, 0.92f));
        centredText("GENERATING WORLD", h * 0.45f, 4.0f, TEXT_COLOR);
        m_ui.end();
        return;
    }

    // Name tags belong on top of the world but under the menus, and they
    // stay visible while the game is paused -- the world is still there.
    if (inGame() && !m_hudHidden) renderNameplates();

    switch (m_screen)
    {
        case Screen::Title:        renderTitleScreen(); break;
        case Screen::Singleplayer: renderSingleplayerScreen(); break;
        case Screen::Multiplayer:  renderMultiplayerScreen(); break;
        case Screen::Settings:     renderSettingsScreen(); break;
        case Screen::Controls:     renderControlsScreen(); break;
        case Screen::NameEntry:    renderNameEntryScreen(); break;
        case Screen::Playing:
            // The inventory replaces the HUD rather than covering it --
            // a crosshair and a second hotbar drawn over the panel just
            // looked like a bug.
            if (m_inventoryOpen) renderInventoryScreen();
            else if (!m_hudHidden) { renderHud(); renderChat(); }
            break;
        case Screen::Paused:
            renderHud();
            renderPauseMenu();
            break;
        case Screen::Dead:
            renderHud();
            renderDeathScreen();
            break;
    }

    if (m_showDebug && m_world) renderDebugOverlay();

    m_ui.end();
}

void Application::renderMenuBackdrop(float)
{
    // No world yet: show the sky by itself, drifting slowly.
    Camera backdrop(glm::vec3(0.0f, 90.0f, 0.0f), m_titleSpin, -6.0f);
    const glm::mat4 view = backdrop.viewMatrix();
    const glm::mat4 projection = backdrop.projectionMatrix(m_window.aspect());
    m_sky.render(view, projection, backdrop.position, sunDirection(), viewDaylight(), m_elapsedSeconds);
}

glm::vec3 Application::dimensionBackdrop() const
{
    switch (m_dimension)
    {
        case Dimension::Nether: return glm::vec3(0.30f, 0.09f, 0.07f);
        case Dimension::End:    return glm::vec3(0.10f, 0.07f, 0.15f);
        default:                return glm::vec3(0.5f, 0.72f, 0.95f);
    }
}

float Application::dimensionAmbient() const
{
    switch (m_dimension)
    {
        case Dimension::Nether: return 0.38f;   // gloomy, not blind
        // Dim, but not so dim that end stone reads as black. There is
        // no sun there and nothing but the island glows, so this is the
        // whole of what lights the place.
        case Dimension::End:    return 0.34f;
        default:                return 0.07f;
    }
}

void Application::renderWorld()
{
    // Everything on screen is drawn from here, which in third person is
    // not where the player's eyes are. m_camera stays on the eyes so
    // that mining, placing and the selection outline are unaffected by
    // which view you happen to be in.
    const Camera camera = viewCamera();

    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix(m_window.aspect());
    const float daylight = viewDaylight();
    const glm::vec3 sun = sunDirection();

    // Only the overworld has a sun to draw; the others are a flat
    // backdrop already cleared to the right colour.
    if (hasSky())
        m_sky.render(view, projection, camera.position, sun, daylight, m_elapsedSeconds,
                     m_weather.depth);

    const bool underwater = m_player.isHeadUnderwater(*m_world);
    const glm::vec3 fogColor = hasSky() ? SkyRenderer::horizonColor(daylight, sun,
                                                                    m_weather.depth)
                                        : dimensionBackdrop();
    const float viewDistance = static_cast<float>(m_world->renderDistance() * Chunk::SX);

    m_chunkShader.bind();
    m_chunkShader.setMat4("uView", view);
    m_chunkShader.setMat4("uProjection", projection);
    m_chunkShader.setInt("uAtlas", 0);
    m_chunkShader.setInt("uNormalAtlas", 1);
    m_chunkShader.setInt("uSpecularAtlas", 2);
    m_chunkShader.setFloat("uDaylight", hasSky() ? daylight : 0.0f);
    m_chunkShader.setFloat("uAmbient", dimensionAmbient());
    m_chunkShader.setVec3("uFogColor", fogColor);
    m_chunkShader.setFloat("uFogStart", viewDistance * 0.55f);
    m_chunkShader.setFloat("uFogEnd", viewDistance * 0.95f);
    m_chunkShader.setInt("uUnderwater", underwater ? 1 : 0);
    m_chunkShader.setVec3("uSunDirection", sun);
    m_chunkShader.setVec3("uCameraPos", camera.position);
    m_chunkShader.setInt("uPbrEnabled", (m_pbrEnabled && m_atlas.hasPbrMaps()) ? 1 : 0);
    m_atlas.bind(0, 1, 2);

    Frustum frustum;
    frustum.update(projection * view);

    struct VisibleChunk { Chunk* chunk; float distance; };
    std::vector<VisibleChunk> visible;
    visible.reserve(m_world->chunks().size());

    for (const auto& [pos, chunk] : m_world->chunks())
    {
        if (chunk->opaqueMesh.empty() && chunk->transparentMesh.empty()) continue;
        if (!frustum.intersectsAABB(chunk->aabbMin(), chunk->aabbMax())) continue;

        const glm::vec3 center = chunk->worldOrigin() + glm::vec3(Chunk::SX * 0.5f, Chunk::SY * 0.5f, Chunk::SZ * 0.5f);
        visible.push_back({ chunk.get(), glm::length(center - camera.position) });
    }

    for (const VisibleChunk& entry : visible)
    {
        if (entry.chunk->opaqueMesh.empty()) continue;
        m_chunkShader.setVec3("uChunkOffset", entry.chunk->worldOrigin());
        entry.chunk->opaqueMesh.draw(GL_TRIANGLES);
    }

    // Dropped items and particles are opaque, so they join the terrain pass.
    m_drops.render(m_chunkShader, *m_world);
    m_orbs.render(m_chunkShader, *m_world);
    m_rain.render(m_chunkShader, *m_world, m_camera.right, m_camera.up);
    m_arrows.render(m_chunkShader, m_camera, m_atlas, *m_world);
    m_particles.render(m_chunkShader, *m_world, camera.right, camera.up);

    // Players are opaque too, but each one brings its own texture, so
    // the atlas has to go back on the albedo unit afterwards for the
    // mining cracks and the water that follow.
    renderPlayers(view, projection);
    if (inGame()) m_mobModel.render(m_chunkShader, *m_world, m_entities);
    m_atlas.bind(0, 1, 2);

    // Selection outline + mining cracks sit between the two passes so water
    // still blends over them correctly.
    if (playing() && !m_inventoryOpen)
    {
        const RaycastHit hit = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
        if (hit.hit)
        {
            if (m_hasTarget && m_breakProgress > 0.0f && m_targetBlock == hit.block)
            {
                m_chunkShader.bind();
                m_selection.drawCrack(m_chunkShader, hit.block,
                                      static_cast<int>(m_breakProgress * Tiles::CrackStages));
            }
            m_selection.drawOutline(view, projection, hit.block);
        }
    }

    // The debug axis cross, planted on the block the player stands on.
    if (m_showDebug && inGame())
    {
        const glm::vec3 feet(std::floor(m_player.position.x),
                             std::floor(m_player.position.y),
                             std::floor(m_player.position.z));
        m_selection.drawAxes(view, projection, feet, 1.0f);
    }

    // Transparent pass: farthest first, no depth writes, so water layers blend.
    std::sort(visible.begin(), visible.end(),
              [](const VisibleChunk& a, const VisibleChunk& b) { return a.distance > b.distance; });

    m_chunkShader.bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    for (const VisibleChunk& entry : visible)
    {
        if (entry.chunk->transparentMesh.empty()) continue;
        m_chunkShader.setVec3("uChunkOffset", entry.chunk->worldOrigin());
        entry.chunk->transparentMesh.draw(GL_TRIANGLES);
    }

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    renderHand();
}

void Application::updateChatInput()
{
    if (!playing() || m_inventoryOpen) { m_chatOpen = false; return; }

    if (!m_chatOpen)
    {
        if (m_keys.pressed(m_input, Action::Chat)) { m_chatOpen = true; m_chatDraft.clear(); }
        else if (m_keys.pressed(m_input, Action::SlashCommand)) { m_chatOpen = true; m_chatDraft = "/"; }
        return;
    }

    if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        m_chatOpen = false;
        m_chatDraft.clear();
        return;
    }

    if (m_input.wasKeyPressed(SDL_SCANCODE_BACKSPACE) && !m_chatDraft.empty())
        m_chatDraft.pop_back();

    for (char c : m_input.typedText())
        if (c >= 32 && m_chatDraft.size() < Chat::MAX_LENGTH) m_chatDraft.push_back(c);

    if (m_input.wasKeyPressed(SDL_SCANCODE_RETURN) || m_input.wasKeyPressed(SDL_SCANCODE_KP_ENTER))
    {
        const std::string line = m_chatDraft;
        m_chatOpen = false;
        m_chatDraft.clear();
        runChatLine(line);
    }
}

void Application::runChatLine(const std::string& line)
{
    const Chat::Command command = Chat::parse(line, m_allowCheats || m_player.creative());

    switch (command.kind)
    {
        case Chat::Kind::Nothing:
            return;

        case Chat::Kind::Say:
        {
            const std::string who = m_playerName.empty() ? "YOU" : m_playerName;
            m_chatLog.add("<" + who + "> " + command.text);
            return;
        }

        case Chat::Kind::GameMode:
            m_gameMode = static_cast<GameMode>(command.mode);
            m_player.mode = m_gameMode;
            break;

        case Chat::Kind::TimeSet:
            m_timeOfDay = command.time;
            break;

        case Chat::Kind::Teleport:
            m_player.position = command.position;
            m_player.velocity = glm::vec3(0.0f);
            break;

        case Chat::Kind::Give:
            m_inventory.add(command.item, command.count);
            break;

        case Chat::Kind::Kill:
            m_player.health = 0;
            break;

        case Chat::Kind::Seed:
            m_chatLog.add(m_world ? "Seed: " + std::to_string(m_world->seed())
                                  : std::string("No world loaded."));
            return;

        default:
            break;
    }

    m_chatLog.add(command.text);
}

void Application::renderChat()
{
    const float h = static_cast<float>(m_window.height());
    const float lineHeight = 20.0f;
    const float left = 12.0f;
    const float bottom = h - 120.0f;

    const std::vector<Chat::Log::Line>& lines = m_chatLog.lines();
    const int shown = std::min<int>(Chat::VISIBLE_LINES, static_cast<int>(lines.size()));

    for (int i = 0; i < shown; ++i)
    {
        const Chat::Log::Line& line = lines[lines.size() - shown + i];

        // Closed chat shows only what is still recent; opened, it shows
        // the lot, so you can read back what you missed.
        if (!m_chatOpen && line.age > Chat::LINE_SECONDS) continue;

        float fade = 1.0f;
        if (!m_chatOpen && line.age > Chat::LINE_SECONDS - 1.0f)
            fade = Chat::LINE_SECONDS - line.age;

        const float y = bottom - (shown - i) * lineHeight;
        m_ui.quad(left - 4.0f, y - 2.0f,
                  UIRenderer::textWidth(line.text, 1.9f) + 8.0f, lineHeight,
                  glm::vec4(0.0f, 0.0f, 0.0f, 0.35f * fade));
        m_ui.textWithShadow(line.text, left, y, 1.9f,
                            glm::vec4(1.0f, 1.0f, 1.0f, fade));
    }

    if (!m_chatOpen) return;

    const float y = bottom + 6.0f;
    m_ui.quad(left - 4.0f, y - 2.0f,
              static_cast<float>(m_window.width()) - left * 2.0f, lineHeight + 4.0f,
              glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
    m_ui.textWithShadow(m_chatDraft + "_", left, y, 1.9f, TEXT_COLOR);
}

// Your own arm, last of everything and on a cleared depth buffer, so
// standing against a wall does not swallow your hand.
void Application::renderHand()
{
    if (!playing() || m_inventoryOpen || m_hudHidden) return;
    if (m_perspective != Perspective::FirstPerson) return;

    const glm::vec3 eye = m_player.eyePosition();
    int lx = static_cast<int>(std::floor(eye.x));
    int ly = static_cast<int>(std::floor(eye.y));
    int lz = static_cast<int>(std::floor(eye.z));
    for (int step = 0; step < 3 && isOpaque(m_world->getBlock(lx, ly, lz)); ++step) ++ly;

    const float sky = static_cast<float>(m_world->skyLight(lx, ly, lz)) / 15.0f *
                      std::max(0.35f, viewDaylight());
    const float blockLight = static_cast<float>(m_world->blockLightAt(lx, ly, lz)) / 15.0f;

    const ItemStack& held = m_inventory.selected();

    m_heldItem.render(m_chunkShader, m_camera, skinFor(localAppearance()), m_atlas,
                      // Both arms of the conditional spelled as the same
                      // type: Blocks::Air is an unscoped enumerator and
                      // held.id is a StackId, and mixing them is what
                      // -Wextra objects to.
                      held.empty() ? static_cast<StackId>(Blocks::Air) : held.id,
                      sky, blockLight, m_player.sneaking, m_player.blocking);
}

// -------------------------------------------------------------- players ---

Camera Application::viewCamera() const
{
    if (m_perspective == Perspective::FirstPerson || !m_world || !inGame())
        return m_camera;

    Camera camera = m_camera;

    // Back out along the line of sight until something solid is in the
    // way, then stop just short of it. Without this the camera drops
    // through the wall behind you and you end up looking at the inside
    // of the world.
    const glm::vec3 away = (m_perspective == Perspective::ThirdBack) ? -m_camera.front
                                                                     : m_camera.front;
    constexpr float WANTED = 4.0f;
    constexpr float CLEARANCE = 0.25f;
    float distance = WANTED;

    for (float step = 0.25f; step <= WANTED; step += 0.25f)
    {
        const glm::vec3 probe = m_camera.position + away * step;
        if (!isSolid(m_world->getBlock(static_cast<int>(std::floor(probe.x)),
                                       static_cast<int>(std::floor(probe.y)),
                                       static_cast<int>(std::floor(probe.z)))))
            continue;

        distance = std::max(0.0f, step - CLEARANCE);
        break;
    }

    camera.position = m_camera.position + away * distance;
    if (m_perspective == Perspective::ThirdFront)
        camera.setOrientation(m_camera.yaw + 180.0f, -m_camera.pitch);

    return camera;
}

Net::Appearance Application::localAppearance() const
{
    Net::Appearance look;
    look.slim = m_skinStyle == PlayerSkin::Style::Slim;

    // Only the choice travels, never the pixels, so a player wearing
    // their own PNG has to be shown to everyone else as one of the
    // painted characters. Picking it from their name at least makes it
    // the same character every time, and on every machine.
    if (m_skinVariant == PlayerSkin::CUSTOM_VARIANT)
    {
        uint32_t hash = 2166136261u;
        for (char c : m_playerName)
        {
            hash ^= static_cast<uint8_t>(c);
            hash *= 16777619u;
        }
        look.variant = static_cast<uint8_t>(hash % static_cast<uint32_t>(PlayerSkin::variantCount()));
    }
    else
    {
        look.variant = static_cast<uint8_t>(std::clamp(m_skinVariant, 0, PlayerSkin::variantCount() - 1));
    }

    return look;
}

const PlayerSkin& Application::skinFor(const Net::Appearance& look)
{
    const int variant = std::clamp(static_cast<int>(look.variant), 0, PlayerSkin::variantCount() - 1);
    const int key = variant * 2 + (look.slim ? 1 : 0);

    auto it = m_remoteSkins.find(key);
    if (it == m_remoteSkins.end())
    {
        auto skin = std::make_unique<PlayerSkin>();
        skin->build(look.slim ? PlayerSkin::Style::Slim : PlayerSkin::Style::Classic, variant);
        it = m_remoteSkins.emplace(key, std::move(skin)).first;
    }

    return *it->second;
}

void Application::advanceAnimation(Animation& animation, const glm::vec3& position,
                                   float headYaw, float deltaTime)
{
    if (!animation.started)
    {
        animation.lastPosition = position;
        animation.bodyYaw = headYaw;
        animation.started = true;
    }

    // Distance covered on the ground this frame is the only movement
    // signal there is for someone on the other end of a wire, and it is
    // the one Minecraft drives the walk cycle from anyway.
    const glm::vec3 moved = position - animation.lastPosition;
    animation.lastPosition = position;

    const float distance = glm::length(glm::vec3(moved.x, 0.0f, moved.z));
    const float speed = deltaTime > 0.0001f ? distance / deltaTime : 0.0f;

    animation.phase += distance * PlayerAnimation::PHASE_PER_BLOCK;
    animation.amount = PlayerAnimation::approach(animation.amount,
                                                 PlayerAnimation::swingAmountFor(speed),
                                                 10.0f, deltaTime);
    animation.bodyYaw = PlayerAnimation::followHead(animation.bodyYaw, headYaw, deltaTime,
                                                    speed > 0.1f);
}

void Application::updatePlayerAnimation(float deltaTime)
{
    if (!m_world) return;

    advanceAnimation(m_animation[LOCAL_ANIM], m_player.position, m_camera.yaw, deltaTime);

    for (const auto& entry : m_net.players())
    {
        const Net::RemotePlayer& player = entry.second;
        if (!player.positioned) continue;
        advanceAnimation(m_animation[entry.first], player.smoothed(), player.yaw, deltaTime);
    }

    // Anyone who has left stops costing anything.
    for (auto it = m_animation.begin(); it != m_animation.end();)
    {
        if (it->first == LOCAL_ANIM || m_net.players().count(it->first)) ++it;
        else it = m_animation.erase(it);
    }
}

void Application::renderPlayers(const glm::mat4& view, const glm::mat4& projection)
{
    m_nameplates.clear();
    if (!m_world || !inGame()) return;

    const glm::mat4 viewProjection = projection * view;
    const float screenWidth = static_cast<float>(m_window.width());
    const float screenHeight = static_cast<float>(m_window.height());
    const glm::vec3 cameraPosition = viewCamera().position;

    auto draw = [&](const PlayerSkin& skin, const PlayerPose& pose, const std::string& name) {
        m_playerModel.render(m_chunkShader, *m_world, skin, pose);
        if (name.empty()) return;

        // The tag floats a little over the top of the head. Project it
        // here, where the matrices are; it gets written in the HUD pass,
        // which is the only one drawn on top of the world.
        const glm::vec3 above = pose.feet + glm::vec3(0.0f, PlayerModel::HEIGHT + 0.4f, 0.0f);
        const glm::vec4 clip = viewProjection * glm::vec4(above, 1.0f);
        if (clip.w <= 0.05f) return;

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.x < -1.3f || ndc.x > 1.3f || ndc.y < -1.3f || ndc.y > 1.3f) return;

        const float distance = glm::length(above - cameraPosition);
        if (distance > 48.0f) return;

        Nameplate plate;
        plate.name = name;
        plate.x = (ndc.x * 0.5f + 0.5f) * screenWidth;
        plate.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * screenHeight;
        plate.scale = std::clamp(2.2f - distance * 0.03f, 1.2f, 2.2f);
        m_nameplates.push_back(plate);
    };

    for (const auto& entry : m_net.players())
    {
        const Net::RemotePlayer& player = entry.second;
        if (!player.positioned) continue;

        const Animation& animation = m_animation[entry.first];

        PlayerPose pose;
        pose.feet = player.smoothed();
        pose.headYaw = player.yaw;
        pose.headPitch = player.pitch;
        pose.bodyYaw = animation.started ? animation.bodyYaw : player.yaw;
        pose.limbPhase = animation.phase;
        pose.limbAmount = animation.amount;
        pose.sneaking = player.sneaking;

        draw(skinFor(player.look), pose, player.name);
    }

    // Yourself, once the camera is no longer behind your own eyes. No
    // name tag: you know who you are, and Minecraft does not draw one.
    if (m_perspective != Perspective::FirstPerson)
    {
        const Animation& animation = m_animation[LOCAL_ANIM];

        PlayerPose pose;
        pose.feet = m_player.position;
        pose.headYaw = m_camera.yaw;
        pose.headPitch = m_camera.pitch;
        pose.bodyYaw = animation.started ? animation.bodyYaw : m_camera.yaw;
        pose.limbPhase = animation.phase;
        pose.limbAmount = animation.amount;
        pose.sneaking = m_player.sneaking;

        draw(activeSkin(), pose, std::string());
    }
}

// Everything that happens around the player whether or not they are
// pressing anything: mobs living their lives and crops growing. Both
// gameplay and the automated screenshot path come through here, because
// putting either one only in updateGameplay is how the mob spawner and
// then the crops each came to look broken when they were merely never
// being stepped.
void Application::updateWorldAround(float deltaTime)
{
    if (!m_world || !m_worldReady) return;

    m_growthTimer += deltaTime;
    if (m_growthTimer >= Farming::TICK_INTERVAL)
    {
        m_growthTimer = 0.0f;
        if (m_net.role() != Net::Role::Guest)
            Farming::grow(*m_world, m_player.position, m_growthSeed);
    }

    updateMobs(deltaTime);
}

void Application::updateMobs(float deltaTime)
{
    if (!m_world || !m_worldReady) return;

    // A guest watches the host's world but does not get to populate it:
    // mobs are not synced, so two machines spawning their own would see
    // different animals in the same field.
    if (m_net.role() == Net::Role::Guest) return;

    // The lineup screen is for looking at hides, and a wolf crosses a
    // twelve-block clearing in two seconds. Frozen, every one of them
    // is still there when the shutter goes.
    const float mobTime = m_freezeMobs ? 0.0f : deltaTime;
    m_entities.update(mobTime, *m_world, m_player.position, daylightFactor());
    m_entities.noticeStares(*m_world, m_camera.position, m_camera.front);

    // Distance alone decides how loud a mob is. There is no panning here
    // yet, and a wrong pan is more distracting than none.
    for (const MobSound& sound : m_entities.drainSounds())
    {
        const float distance = glm::length(sound.position - m_camera.position);
        if (distance > 32.0f) continue;

        const float falloff = 1.0f - std::clamp(distance / 32.0f, 0.0f, 1.0f);
        m_audio.play(sound.id, sound.volume * falloff * falloff, sound.pitch);
    }

    // Blows landed on the player. Several mobs can swing in the same
    // frame; the half second of immunity after the first one is what
    // decides how many of them actually land.
    for (const MobStrike& strike : m_entities.drainStrikes())
    {
        // A raised shield stops a blow outright, but only one that
        // comes at the front of it. Turning to meet what is hitting you
        // is the whole of using one.
        if (m_player.blocking &&
            Shield::covers(m_camera.front, m_player.position, strike.from))
        {
            wearShield(Shield::WEAR_PER_BLOCK);
            m_audio.play(Sound::Click, 0.7f, 0.7f);

            // Shoved back, but unhurt.
            const glm::vec3 off = m_player.position - strike.from;
            const glm::vec3 level(off.x, 0.0f, off.z);
            const float span = glm::length(level);
            if (span > 0.001f) m_player.velocity += (level / span) * 2.5f;
            continue;
        }

        if (!m_player.damage(strike.damage)) continue;

        wearArmour(strike.damage);
        m_audio.play(Sound::Hurt, 0.8f);

        const glm::vec3 away = m_player.position - strike.from;
        const glm::vec3 flat(away.x, 0.0f, away.z);
        const float length = glm::length(flat);
        if (length > 0.001f) m_player.velocity += (flat / length) * 6.0f;
        if (m_player.onGround) m_player.velocity.y = std::max(m_player.velocity.y, 4.0f);
    }

    // Anything that went off.
    for (const MobBlast& blast : m_entities.drainBlasts())
        applyBlast(blast.at, blast.radius, blast.damage);

    // Arrows loosed by anything that carries a bow, and the charges
    // thrown by anything that does not.
    for (const MobShot& shot : m_entities.drainShots())
        m_arrows.spawn(shot.from, shot.velocity, shot.damage, false, shot.kind);

    // What they leave behind. Most species drop nothing yet, because
    // what they ought to drop -- leather, bone, string -- are items, and
    // this engine only has blocks.
    for (const EntityManager::Death& death : m_entities.drainDeaths())
    {
        const MobType& type = mobType(death.type);
        const glm::vec3 where = death.position + glm::vec3(0.0f, type.height * 0.5f, 0.0f);

        for (int i = 0; i < type.dropCount; ++i) m_drops.spawn(where, type.drop, 1);
        for (int i = 0; i < type.secondDropCount; ++i) m_drops.spawn(where, type.secondDrop, 1);

        // A baby is worth nothing, the way it is in Minecraft -- else
        // penning animals and killing the lambs is the best experience
        // in the game.
        if (type.experience > 0 && !death.baby)
            m_orbs.spawnValue(where, type.experience);
    }

    for (const glm::vec3& born : m_entities.drainBirths())
    {
        (void)born;
        m_audio.play(Sound::Pickup, 0.6f, 1.6f);
    }
}

void Application::renderNameplates()
{
    for (const Nameplate& plate : m_nameplates)
    {
        const float width = UIRenderer::textWidth(plate.name, plate.scale);
        const float height = UIRenderer::textHeight(plate.scale);
        const float x = plate.x - width * 0.5f;
        const float y = plate.y - height;

        m_ui.quad(x - 4.0f, y - 3.0f, width + 8.0f, height + 6.0f,
                  glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));
        m_ui.textWithShadow(plate.name, x, y, plate.scale, TEXT_COLOR);
    }
}

// ------------------------------------------------------------------ hud ---

void Application::renderHearts(float x, float y)
{
    constexpr int HEART_COUNT = 10;
    constexpr float HEART_SIZE = 20.0f;
    constexpr float SPACING = 2.0f;

    const bool hardcore = modeIsHardcore(m_gameMode);
    const GuiSprite fullSprite = hardcore ? GuiSprite::HeartHardcoreFull : GuiSprite::HeartFull;
    const GuiSprite halfSprite = hardcore ? GuiSprite::HeartHardcoreHalf : GuiSprite::HeartHalf;
    const GuiSprite emptySprite = hardcore ? GuiSprite::HeartHardcoreContainer : GuiSprite::HeartContainer;

    for (int i = 0; i < HEART_COUNT; ++i)
    {
        const float heartX = x + i * (HEART_SIZE + SPACING);
        // Two health points per heart: 20 health == 10 full hearts.
        const int remaining = m_player.health - i * 2;

        // The empty container always sits underneath, like Minecraft.
        m_ui.texturedQuad(m_gui.texture(emptySprite), heartX, y, HEART_SIZE, HEART_SIZE,
                          0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));

        if (remaining >= 2)
        {
            m_ui.texturedQuad(m_gui.texture(fullSprite), heartX, y, HEART_SIZE, HEART_SIZE,
                              0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
        }
        else if (remaining == 1)
        {
            m_ui.texturedQuad(m_gui.texture(halfSprite), heartX, y, HEART_SIZE, HEART_SIZE,
                              0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
        }
    }
}

// The armour row, above the hearts, in the same half-and-whole steps the
// hearts use: twenty points is ten plates, as in Minecraft. Nothing is
// drawn at all when nothing is worn, so an unarmoured player's HUD looks
// exactly as it did before there was any armour in the game.
void Application::renderArmour(float x, float y)
{
    const int points = m_player.armourPoints;
    if (points <= 0) return;

    constexpr int PLATE_COUNT = 10;
    constexpr float PLATE_SIZE = 20.0f;
    constexpr float SPACING = 2.0f;

    for (int i = 0; i < PLATE_COUNT; ++i)
    {
        const float plateX = x + i * (PLATE_SIZE + SPACING);
        const int remaining = points - i * 2;

        m_ui.texturedQuad(m_gui.texture(GuiSprite::ArmorEmpty), plateX, y,
                          PLATE_SIZE, PLATE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));

        if (remaining >= 2)
            m_ui.texturedQuad(m_gui.texture(GuiSprite::ArmorFull), plateX, y,
                              PLATE_SIZE, PLATE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
        else if (remaining == 1)
            m_ui.texturedQuad(m_gui.texture(GuiSprite::ArmorHalf), plateX, y,
                              PLATE_SIZE, PLATE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
    }
}

void Application::renderFood(float rightX, float y)
{
    constexpr int FOOD_COUNT = 10;
    constexpr float FOOD_SIZE = 20.0f;
    constexpr float SPACING = 2.0f;

    for (int i = 0; i < FOOD_COUNT; ++i)
    {
        const float foodX = rightX - (i + 1) * FOOD_SIZE - i * SPACING;
        const int remaining = m_player.hunger - i * 2;

        m_ui.texturedQuad(m_gui.texture(GuiSprite::FoodContainer), foodX, y, FOOD_SIZE, FOOD_SIZE,
                          0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));

        if (remaining >= 2)
        {
            m_ui.texturedQuad(m_gui.texture(GuiSprite::FoodFull), foodX, y, FOOD_SIZE, FOOD_SIZE,
                              0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
        }
        else if (remaining == 1)
        {
            m_ui.texturedQuad(m_gui.texture(GuiSprite::FoodHalf), foodX, y, FOOD_SIZE, FOOD_SIZE,
                              0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
        }
    }
}

// The experience bar, across the top of the hotbar, with the level
// standing on it. Drawn as plain quads rather than a sprite: the bar is
// a flat green fill in Minecraft too, and a texture for it would be one
// more thing a resource pack has to supply before the HUD looks right.
void Application::renderExperience(float x, float y, float width)
{
    const int level = Experience::levelFor(m_player.experience);
    const float fraction = std::clamp(Experience::progress(m_player.experience), 0.0f, 1.0f);

    constexpr float HEIGHT = 10.0f;

    m_ui.quad(x - 1.0f, y - 1.0f, width + 2.0f, HEIGHT + 2.0f,
              glm::vec4(0.0f, 0.0f, 0.0f, 0.65f));
    m_ui.quad(x, y, width, HEIGHT, glm::vec4(0.16f, 0.16f, 0.16f, 0.9f));
    if (fraction > 0.0f)
        m_ui.quad(x, y, width * fraction, HEIGHT, glm::vec4(0.47f, 0.84f, 0.15f, 1.0f));

    // Level nought is not drawn at all, so an untouched HUD looks the
    // way it did before there was any experience in the game.
    if (level <= 0) return;

    const std::string text = std::to_string(level);
    const float scale = 2.2f;
    const float textX = x + width * 0.5f - UIRenderer::textWidth(text, scale) * 0.5f;
    const float textY = y - UIRenderer::textHeight(scale) * 0.5f + HEIGHT * 0.5f;

    // Outlined in black on all four sides, the way Minecraft does it:
    // bright green on a bright green bar is unreadable at a glance.
    const glm::vec4 outline(0.0f, 0.0f, 0.0f, 1.0f);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx || dy)
                m_ui.text(text, textX + dx * 2.0f, textY + dy * 2.0f, scale, outline);
    m_ui.text(text, textX, textY, scale, glm::vec4(0.49f, 0.98f, 0.24f, 1.0f));
}

// The bubbles, over on the right above the hunger. Drawn only while
// you are short of air, exactly as Minecraft does it, so a HUD on dry
// land looks the way it did before there was any drowning in the game.
void Application::renderAir(float rightX, float y)
{
    if (m_player.air >= Player::MAX_AIR_SECONDS) return;

    constexpr int BUBBLE_COUNT = 10;
    constexpr float BUBBLE_SIZE = 18.0f;
    constexpr float SPACING = 2.0f;

    const float share = m_player.air / Player::MAX_AIR_SECONDS;
    const float showing = share * BUBBLE_COUNT;

    for (int i = 0; i < BUBBLE_COUNT; ++i)
    {
        // Filled from the left, so the row empties towards the middle
        // of the screen the way the hunger beside it does.
        if (static_cast<float>(i) >= showing) continue;

        const float bubbleX = rightX - (i + 1) * BUBBLE_SIZE - i * SPACING;
        m_ui.quad(bubbleX + 3.0f, y + 3.0f, BUBBLE_SIZE - 6.0f, BUBBLE_SIZE - 6.0f,
                  glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
        m_ui.quad(bubbleX + 4.0f, y + 4.0f, BUBBLE_SIZE - 8.0f, BUBBLE_SIZE - 8.0f,
                  glm::vec4(0.78f, 0.90f, 1.0f, 0.95f));
    }
}

void Application::renderEatProgress(float cx, float cy)
{
    if (m_eatTimer <= 0.0f) return;

    const float fraction = std::min(1.0f, m_eatTimer / Food::EAT_SECONDS);
    const float width = 120.0f;
    const float height = 6.0f;
    const float x = cx - width * 0.5f;
    const float y = cy + 30.0f;

    m_ui.quad(x - 1.0f, y - 1.0f, width + 2.0f, height + 2.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    m_ui.quad(x, y, width * fraction, height, glm::vec4(0.85f, 0.7f, 0.35f, 0.95f));
}

// A pocket cut out of the trees with the three blocks every screenshot
// carries standing in it, and torches, because under a closed jungle
// canopy the light is nothing at all -- true of a jungle, and useless
// in a photograph.
void Application::buildBiomeArena(int standX, int standZ)
{
    const WorldGen& gen = m_world->generator();

    for (int ax = -1; ax <= 11; ++ax)
        for (int az = -4; az <= 4; ++az)
        {
            const int groundY = gen.surfaceHeight(standX + ax, standZ + az);
            for (int ay = 1; ay <= 7; ++ay)
                m_world->setBlock(standX + ax, groundY + ay, standZ + az, Blocks::Air);
        }

    // On the ground each one stands on, not at the height the camera
    // happens to be: the floor rolls, and the first set hung in the air.
    auto stand = [&](int ax, int az, BlockId block) {
        const int groundY = gen.surfaceHeight(standX + ax, standZ + az);
        m_world->setBlock(standX + ax, groundY + 1, standZ + az, block);
    };

    for (int ax : { 1, 10 })
        for (int az : { -4, 4 })
            stand(ax, az, Blocks::Torch);

    stand(7, -2, Blocks::Bookshelf);
    stand(7, 0, Blocks::DiamondOre);
    stand(7, 2, Blocks::EndPortalFrame);
}

// The three blocks every screenshot carries, stood against the far wall
// of the entrance room.
void Application::dressChamberRoom(int atX, int atY, int atZ)
{
    m_world->setBlock(atX - 3, atY, atZ - 3, Blocks::Bookshelf);
    m_world->setBlock(atX - 1, atY, atZ - 3, Blocks::DiamondOre);
    m_world->setBlock(atX + 2, atY, atZ - 3, Blocks::EndPortalFrame);
}

void Application::renderHud()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    // Alight. Minecraft draws flames licking up the bottom of the
    // screen and it is the only cue you get, so it is drawn before
    // everything else on the HUD and out of the way of the hotbar.
    if (m_player.onFire())
    {
        const TileUV uv = tileUV(Tiles::FireCharge);
        const float flame = h * 0.16f;
        const int across = std::max(3, static_cast<int>(w / flame) + 1);
        for (int i = 0; i < across; ++i)
        {
            // Each one on its own clock, so the row flickers rather
            // than pulsing as one sheet.
            const float beat = 0.72f + 0.28f * std::sin(m_elapsedSeconds * 7.0f + i * 1.7f);
            m_ui.texturedQuad(m_atlas.textureId(), i * flame, h - flame * beat,
                              flame, flame * beat, uv.u0, uv.vTop, uv.u1, uv.vBottom,
                              glm::vec4(1.0f, 1.0f, 1.0f, 0.86f));
        }
    }

    // Crosshair, with a dark backing so it stays readable on bright terrain.
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    const glm::vec4 crosshairEdge(0.0f, 0.0f, 0.0f, 0.45f);
    const glm::vec4 crosshair(1.0f, 1.0f, 1.0f, 0.85f);
    m_ui.quad(cx - 10.0f, cy - 2.0f, 20.0f, 4.0f, crosshairEdge);
    m_ui.quad(cx - 2.0f, cy - 10.0f, 4.0f, 20.0f, crosshairEdge);
    m_ui.quad(cx - 9.0f, cy - 1.0f, 18.0f, 2.0f, crosshair);
    m_ui.quad(cx - 1.0f, cy - 9.0f, 2.0f, 18.0f, crosshair);

    // Hotbar
    const float barWidth = Inventory::HOTBAR_SLOTS * SLOT_SIZE + (Inventory::HOTBAR_SLOTS + 1) * SLOT_GAP;
    const float barX = cx - barWidth * 0.5f;
    const float barY = h - SLOT_SIZE - SLOT_GAP * 3.0f;

    if (m_gui.fromPack(GuiSprite::Hotbar))
    {
        m_ui.texturedQuad(m_gui.texture(GuiSprite::Hotbar), barX, barY - SLOT_GAP,
                          barWidth, SLOT_SIZE + SLOT_GAP * 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
    }
    else
    {
        guiPanel(barX, barY - SLOT_GAP, barWidth, SLOT_SIZE + SLOT_GAP * 2.0f);
    }

    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
    {
        const float slotX = barX + SLOT_GAP + i * (SLOT_SIZE + SLOT_GAP);
        const bool selected = (i == m_inventory.selectedSlot());

        if (!m_gui.fromPack(GuiSprite::Hotbar))
            guiWell(slotX, barY, SLOT_SIZE, SLOT_SIZE);

        if (selected)
        {
            if (m_gui.fromPack(GuiSprite::HotbarSelection))
            {
                const float pad = 5.0f;
                m_ui.texturedQuad(m_gui.texture(GuiSprite::HotbarSelection),
                                  slotX - pad, barY - pad, SLOT_SIZE + pad * 2, SLOT_SIZE + pad * 2,
                                  0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
            }
            else
            {
                const float t = 3.0f;
                m_ui.quad(slotX - t, barY - t, SLOT_SIZE + t * 2, t, SELECTED_COLOR);
                m_ui.quad(slotX - t, barY + SLOT_SIZE, SLOT_SIZE + t * 2, t, SELECTED_COLOR);
                m_ui.quad(slotX - t, barY, t, SLOT_SIZE, SELECTED_COLOR);
                m_ui.quad(slotX + SLOT_SIZE, barY, t, SLOT_SIZE, SELECTED_COLOR);
            }
        }

        const ItemStack& stack = m_inventory.slot(i);
        if (stack.empty()) continue;

        // Through drawItem, the same as every slot on the inventory
        // panel, so a block in the hotbar is the cube it is there and
        // not a flat square of its own top texture. Drawing its own
        // quad here is what left the hotbar behind when the panel
        // learned to draw blocks as blocks -- and it also means the
        // wear on a tool now shows in the hand, where it matters most.
        drawItem(slotX, barY, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f), stack.damage);

        if (!m_player.creative() && stack.count > 1)
        {
            const std::string count = std::to_string(stack.count);
            m_ui.textWithShadow(count,
                                slotX + SLOT_SIZE - UIRenderer::textWidth(count, 2.0f) - 4.0f,
                                barY + SLOT_SIZE - UIRenderer::textHeight(2.0f) - 3.0f,
                                2.0f, TEXT_COLOR);
        }
    }

    // Name of the held block, above the hotbar.
    const ItemStack& held = m_inventory.selected();
    if (!held.empty())
    {
        const std::string name = displayName(held.id);
        // Clear of the armour row, which is the highest thing on the
        // left: the name is centred, and at 2.2 it is wide enough to
        // reach the hearts if it sits any lower.
        m_ui.textWithShadow(name, cx - UIRenderer::textWidth(name, 2.2f) * 0.5f, barY - 98.0f, 2.2f, TEXT_COLOR);
    }

    // Hearts are hidden in creative, like Minecraft.
    if (!m_player.creative())
    {
        renderExperience(barX + SLOT_GAP, barY - 24.0f,
                         barWidth - SLOT_GAP * 2.0f);
        renderHearts(barX + SLOT_GAP, barY - 52.0f);
        renderArmour(barX + SLOT_GAP, barY - 74.0f);
        renderFood(barX + barWidth - SLOT_GAP, barY - 52.0f);
        renderAir(barX + barWidth - SLOT_GAP, barY - 74.0f);
        renderEatProgress(cx, cy);
    }

    // Red flash when hurt.
    if (m_player.damageFlash > 0.0f)
        m_ui.quad(0, 0, w, h, glm::vec4(0.7f, 0.0f, 0.0f, m_player.damageFlash * 0.5f));

    const glm::vec4 modeColour = modeIsHardcore(m_gameMode) ? glm::vec4(1.0f, 0.55f, 0.5f, 0.95f)
                                                            : glm::vec4(0.7f, 0.9f, 1.0f, 0.9f);
    m_ui.textWithShadow(gameModeName(m_gameMode), 12.0f, h - 24.0f, 2.0f, modeColour);
    if (!m_playerName.empty())
        m_ui.textWithShadow(m_playerName, 12.0f, h - 46.0f, 2.2f, TEXT_COLOR);
}

// ---------------------------------------------------------------- menus ---

void Application::roundedQuad(float x, float y, float w, float h,
                              const glm::vec4& colour, float radius)
{
    radius = std::min(radius, std::min(w, h) * 0.5f);
    if (radius <= 0.5f) { m_ui.quad(x, y, w, h, colour); return; }

    // The corner is a real quarter circle, drawn as one horizontal band
    // per pixel of radius. Rounding this way keeps the edges hard, which
    // an alpha-blended curve would not -- and a soft edge next to this
    // font looks like a rendering mistake.
    const int steps = std::max(2, static_cast<int>(radius));
    const float band = radius / steps;

    m_ui.quad(x, y + radius, w, h - radius * 2.0f, colour);

    for (int i = 0; i < steps; ++i)
    {
        // Height of this band's centre above the centre of the arc.
        const float dy = radius - (i + 0.5f) * band;
        const float inset = radius - std::sqrt(std::max(0.0f, radius * radius - dy * dy));

        m_ui.quad(x + inset, y + i * band, w - inset * 2.0f, band, colour);
        m_ui.quad(x + inset, y + h - (i + 1) * band, w - inset * 2.0f, band, colour);
    }
}

void Application::bevel(float x, float y, float w, float h, const glm::vec4& face,
                        const glm::vec4& light, const glm::vec4& dark, float edge)
{
    const float radius = std::min(6.0f, std::min(w, h) * 0.22f);

    // Three stacked rounded rectangles: a lit one, a shaded one pushed
    // down and right so only that side of it shows, and the face on top.
    // Doing it this way means the bevel follows the curve for free.
    roundedQuad(x, y, w, h, light, radius);
    roundedQuad(x + edge, y + edge, w - edge, h - edge, dark, radius);
    roundedQuad(x + edge, y + edge, w - edge * 2.0f, h - edge * 2.0f, face,
                std::max(1.0f, radius - edge * 0.5f));
}


void Application::guiPanel(float x, float y, float w, float h)
{
    // A dark outline first: vanilla panels sit on a one-pixel black edge,
    // which is what keeps them from dissolving into a bright sky.
    roundedQuad(x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.70f), 5.0f);
    bevel(x, y, w, h, GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 3.0f);
}

void Application::guiWell(float x, float y, float w, float h)
{
    // The same bevel with the light and dark swapped, so it reads sunken.
    bevel(x, y, w, h, GUI_WELL, GUI_WELL_DARK, GUI_PANEL_LIGHT, 2.0f);
}

bool Application::menuButton(const Rect& rect, const std::string& label, bool enabled)
{
    // Menus are hit-tested during drawing, which bypasses the input gate in
    // handleEvents -- without this an automated capture would still react
    // to whatever the real cursor is doing.
    const bool acceptClicks = !m_automated;

    const float mouseX = static_cast<float>(m_input.mouseX());
    const float mouseY = static_cast<float>(m_input.mouseY());
    const bool hovered = enabled &&
                         mouseX >= rect.x && mouseX <= rect.x + rect.w &&
                         mouseY >= rect.y && mouseY <= rect.y + rect.h;

    const glm::vec4 face = !enabled ? BUTTON_FACE_OFF
                                    : (hovered ? BUTTON_FACE_HOVER : BUTTON_FACE);

    roundedQuad(rect.x - 2.0f, rect.y - 2.0f, rect.w + 4.0f, rect.h + 4.0f,
                glm::vec4(0.0f, 0.0f, 0.0f, 0.75f), 5.0f);
    bevel(rect.x, rect.y, rect.w, rect.h, face, BUTTON_LIGHT, BUTTON_DARK, 2.0f);

    const float scale = 2.4f;
    const glm::vec4 textColour = !enabled ? BUTTON_TEXT_OFF
                                          : (hovered ? BUTTON_TEXT_HOVER : TEXT_COLOR);
    m_ui.textWithShadow(label,
                        rect.x + rect.w * 0.5f - UIRenderer::textWidth(label, scale) * 0.5f,
                        rect.y + rect.h * 0.5f - UIRenderer::textHeight(scale) * 0.5f,
                        scale, textColour);

    return acceptClicks && hovered && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT);
}

void Application::centredText(const std::string& text, float y, float scale, const glm::vec4& colour)
{
    const float w = static_cast<float>(m_window.width());
    m_ui.textWithShadow(text, w * 0.5f - UIRenderer::textWidth(text, scale) * 0.5f, y, scale, colour);
}

void Application::renderTitleScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.30f));

    // Scale the wordmark to the window so it never runs off the edge on a
    // narrow display.
    const std::string title = "BROWSERCRAFT";
    const float titleScale = std::min(7.0f, (w * 0.78f) / UIRenderer::textWidth(title, 1.0f));
    const float titleY = h * 0.13f;
    centredText(title, titleY, titleScale, TEXT_COLOR);

    // The yellow line beside the logo, pulsing the way Minecraft's does.
    // Picked once per launch rather than per frame, or it would flicker
    // through the whole list sixty times a second.
    static const char* splash = SPLASHES[SDL_GetTicks() % std::size(SPLASHES)];
    const float pulse = 1.0f + 0.08f * std::sin(m_elapsedSeconds * 4.0f);
    float splashScale = 1.9f * pulse;

    // The longer lines ran off the right edge on a narrow window, so the
    // splash is shrunk to fit and then pulled back on screen.
    float splashWidth = UIRenderer::textWidth(splash, splashScale);
    const float splashRoom = w * 0.46f;
    if (splashWidth > splashRoom)
    {
        splashScale *= splashRoom / splashWidth;
        splashWidth = splashRoom;
    }

    float splashX = w * 0.5f + UIRenderer::textWidth(title, titleScale) * 0.40f;
    splashX = std::clamp(splashX, 8.0f, w - splashWidth - 10.0f);

    // Tilted up to the right, as in Minecraft. Negative because screen Y
    // runs downwards, so a negative angle lifts the end of the line.
    constexpr float SPLASH_TILT = -20.0f * 3.14159265f / 180.0f;
    m_ui.textWithShadowRotated(splash, splashX,
                               titleY + UIRenderer::textHeight(titleScale) * 0.86f,
                               splashScale, SPLASH_TILT, glm::vec4(1.0f, 1.0f, 0.31f, 1.0f));

    const float buttonWidth = std::min(420.0f, w * 0.6f);
    const float buttonX = w * 0.5f - buttonWidth * 0.5f;
    const float buttonHeight = 52.0f;
    float y = h * 0.42f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "SINGLEPLAYER"))
        m_pendingAction = MenuAction::GoSingleplayer;
    y += buttonHeight + 14.0f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "MULTIPLAYER"))
        m_pendingAction = MenuAction::GoMultiplayer;
    y += buttonHeight + 14.0f;

    // Settings and the profile share a row, as they do in Eaglercraft.
    const float halfWidth = buttonWidth * 0.5f - 6.0f;
    if (menuButton({ buttonX, y, halfWidth, buttonHeight }, "SETTINGS"))
        m_pendingAction = MenuAction::GoSettings;

    const Rect profileButton{ buttonX + halfWidth + 12.0f, y, halfWidth, buttonHeight };
    if (menuButton(profileButton, ""))
        m_pendingAction = MenuAction::GoNameEntry;

    // The button carries your character, so the title screen shows who
    // you are without needing a separate panel for it.
    {
        const PlayerSkin& skin = activeSkin();
        const float unit = (buttonHeight - 12.0f) / 32.0f;
        const float dollWidth = (skin.armWidth() * 2.0f + 8.0f) * unit;
        drawPlayerDoll(skin, profileButton.x + 10.0f, profileButton.y + 6.0f, unit);

        const std::string label = m_playerName.empty() ? "PROFILE" : m_playerName;
        const float scale = 1.9f;
        m_ui.textWithShadow(label, profileButton.x + dollWidth + 18.0f,
                            profileButton.y + buttonHeight * 0.5f - UIRenderer::textHeight(scale) * 0.5f,
                            scale, TEXT_COLOR);
    }
    y += buttonHeight + 14.0f;

#ifndef __EMSCRIPTEN__
    if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "QUIT"))
        m_pendingAction = MenuAction::QuitGame;
#endif

    // Version in the corner, where every Minecraft build puts it.
    const float footerY = h - UIRenderer::textHeight(1.7f) - 10.0f;
    m_ui.textWithShadow("BROWSERCRAFT 0.1  -  BROWSERCRAFT.NET", 10.0f, footerY, 1.7f, DIM_TEXT);

    // Whoever made the installed texture pack, opposite the version the
    // way Minecraft puts its copyright line. Most packs are free to pass
    // on only if they are credited, and this is the only screen everyone
    // sees. Nothing is claimed when no pack is installed.
    const std::string& credit = m_atlas.packCredit();
    if (!credit.empty())
        m_ui.textWithShadow(credit, w - UIRenderer::textWidth(credit, 1.7f) - 10.0f,
                            footerY, 1.7f, DIM_TEXT);
}

void Application::renderSingleplayerScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
    centredText("SINGLEPLAYER", h * 0.13f, 4.4f, TEXT_COLOR);

    const float buttonWidth = std::min(520.0f, w * 0.72f);
    const float buttonX = w * 0.5f - buttonWidth * 0.5f;
    const float buttonHeight = 54.0f;
    float y = h * 0.26f;

    // Your world is saved automatically, so resuming is just the top entry
    // rather than a separate button on the home screen.
    if (saveExists())
    {
        if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "YOUR SAVED WORLD"))
            m_pendingAction = MenuAction::ResumeSaved;
        y += buttonHeight + 22.0f;
    }

    struct ModeEntry { const char* label; MenuAction action; };
    const ModeEntry MODES[] = {
        { "SURVIVAL", MenuAction::NewSurvival },
        { "CREATIVE", MenuAction::NewCreative },
        { "HARDCORE", MenuAction::NewHardcore },
    };

    for (const ModeEntry& mode : MODES)
    {
        if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, mode.label))
            m_pendingAction = mode.action;
        y += buttonHeight + 14.0f;
    }

    y += 10.0f;

    // Decided before the world is made, and kept with it. Creative has
    // the lot anyway, so the toggle only really bites in survival.
    const std::string cheats = std::string("ALLOW CHEATS: ") + (m_allowCheats ? "ON" : "OFF");
    if (menuButton({ buttonX, y, buttonWidth, 46.0f }, cheats))
        m_pendingAction = MenuAction::ToggleCheats;
    y += 46.0f + 8.0f;

    centredText(m_allowCheats ? "/GAMEMODE /TIME /TP /GIVE /KILL /SEED"
                              : "CHAT WORKS EITHER WAY; THE COMMANDS DO NOT",
                y, 1.7f, DIM_TEXT);
    y += 30.0f;

    if (menuButton({ buttonX, y, buttonWidth, 46.0f }, "BACK"))
        m_pendingAction = MenuAction::BackToTitle;
}

void Application::renderMultiplayerScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
    centredText("MULTIPLAYER", h * 0.10f, 4.4f, TEXT_COLOR);

    const float panelWidth = std::min(560.0f, w * 0.78f);
    const float panelX = w * 0.5f - panelWidth * 0.5f;
    float y = h * 0.22f;

#ifdef __EMSCRIPTEN__
    centredText("THE BROWSER BUILD CANNOT OPEN SOCKETS YET.", y, 1.9f, DIM_TEXT);
    centredText("PLAY ON A LAN FROM THE DESKTOP BUILD FOR NOW.", y + 26.0f, 1.9f, DIM_TEXT);
    y += 80.0f;
#else
    // Worlds shouting on the local network. Nothing to type in the
    // common case: open a world on one machine, see it on the other.
    const std::vector<Net::LanDiscovery::Game>& games = m_discovery.games();

    m_ui.textWithShadow("WORLDS ON THIS NETWORK", panelX, y, 1.9f, DIM_TEXT);
    y += 28.0f;

    if (games.empty())
    {
        bevel(panelX, y, panelWidth, 46.0f, BUTTON_FACE_OFF, BUTTON_LIGHT, BUTTON_DARK, 2.0f);
        m_ui.textWithShadow("SEARCHING...", panelX + 14.0f, y + 16.0f, 1.9f, DIM_TEXT);
        y += 58.0f;
    }
    else
    {
        for (size_t i = 0; i < games.size() && i < 5; ++i)
        {
            const Net::LanDiscovery::Game& game = games[i];
            const std::string label = game.name + "  -  " + game.address;
            if (menuButton({ panelX, y, panelWidth, 46.0f }, label))
            {
                m_joinChoice = static_cast<int>(i);
                m_pendingAction = MenuAction::JoinDiscovered;
            }
            y += 54.0f;
        }
    }

    y += 10.0f;
    m_ui.textWithShadow("OR TYPE AN ADDRESS", panelX, y, 1.9f, DIM_TEXT);
    y += 28.0f;

    const float joinWidth = 130.0f;
    const float fieldWidth = panelWidth - joinWidth - 10.0f;
    roundedQuad(panelX - 3.0f, y - 3.0f, fieldWidth + 6.0f, 52.0f,
                glm::vec4(0.627f, 0.627f, 0.627f, 1.0f), 5.0f);
    roundedQuad(panelX, y, fieldWidth, 46.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 4.0f);
    const bool caretOn = ((SDL_GetTicks() / 500u) % 2u) == 0u;
    m_ui.textWithShadow(m_addressDraft + (caretOn ? "_" : " "),
                        panelX + 12.0f, y + 15.0f, 2.0f, TEXT_COLOR);
    if (menuButton({ panelX + fieldWidth + 10.0f, y, joinWidth, 46.0f }, "JOIN",
                   m_addressDraft.size() >= 7))
        m_pendingAction = MenuAction::JoinTyped;
    y += 62.0f;
#endif

    // Whatever the session is currently doing or complaining about.
    if (!m_net.message().empty())
    {
        const bool failed = m_net.status() == Net::Status::Failed;
        centredText(m_net.message(), y, 1.9f,
                    failed ? glm::vec4(1.0f, 0.6f, 0.52f, 1.0f) : DIM_TEXT);
        y += 34.0f;
    }

    if (menuButton({ panelX, y + 10.0f, panelWidth, 50.0f }, "BACK"))
        m_pendingAction = MenuAction::BackToTitle;
}

void Application::renderSettingsScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
    centredText("SETTINGS", h * 0.13f, 4.4f, TEXT_COLOR);

    const float buttonWidth = std::min(520.0f, w * 0.72f);
    const float buttonX = w * 0.5f - buttonWidth * 0.5f;
    const float buttonHeight = 52.0f;
    float y = h * 0.28f;

    // Render distance, as a row of [-] value [+]
    const float sideWidth = 60.0f;
    const int distance = m_world ? m_world->renderDistance() : 8;
    char label[64];
    std::snprintf(label, sizeof(label), "RENDER DISTANCE: %d", distance);

    if (menuButton({ buttonX, y, sideWidth, buttonHeight }, "-"))
        m_pendingAction = MenuAction::RenderDistanceDown;
    bevel(buttonX + sideWidth + 8.0f, y, buttonWidth - sideWidth * 2.0f - 16.0f, buttonHeight,
          BUTTON_FACE_OFF, BUTTON_LIGHT, BUTTON_DARK, 2.0f);
    m_ui.textWithShadow(label,
                        buttonX + buttonWidth * 0.5f - UIRenderer::textWidth(label, 2.2f) * 0.5f,
                        y + buttonHeight * 0.5f - UIRenderer::textHeight(2.2f) * 0.5f,
                        2.2f, TEXT_COLOR);
    if (menuButton({ buttonX + buttonWidth - sideWidth, y, sideWidth, buttonHeight }, "+"))
        m_pendingAction = MenuAction::RenderDistanceUp;
    y += buttonHeight + 14.0f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight },
                   m_audio.muted() ? "SOUND: OFF" : "SOUND: ON"))
        m_pendingAction = MenuAction::ToggleSound;
    y += buttonHeight + 14.0f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight },
                   m_pbrEnabled ? "FANCY LIGHTING: ON" : "FANCY LIGHTING: OFF",
                   m_atlas.hasPbrMaps()))
        m_pendingAction = MenuAction::TogglePbr;
    y += buttonHeight + 14.0f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "CONTROLS"))
        m_pendingAction = MenuAction::GoControls;
    y += buttonHeight + 14.0f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight },
                   m_playerName.empty() ? "SET NAME" : ("NAME: " + m_playerName)))
        m_pendingAction = MenuAction::GoNameEntry;
    y += buttonHeight + 24.0f;

    if (menuButton({ buttonX, y, buttonWidth, 46.0f }, "BACK"))
        m_pendingAction = MenuAction::BackToTitle;
}

void Application::renderControlsScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.62f));
    centredText("CONTROLS", h * 0.05f, 3.6f, TEXT_COLOR);

    const int rowCount = static_cast<int>(Action::Count);
    const int rows = (rowCount + 1) / 2;

    // Two columns, shrunk to fit a short window the same way the profile
    // screen does -- twenty-one rows do not fit a laptop otherwise.
    constexpr float ROW_H = 30.0f;
    const float headingSpace = h * 0.05f + UIRenderer::textHeight(3.6f) + 20.0f;
    const float footerSpace = 110.0f;
    const float available = h - headingSpace - footerSpace;
    const float ui = std::clamp(available / (rows * ROW_H), 0.6f, 1.0f);

    const float rowHeight = ROW_H * ui;
    const float scale = 1.8f * ui;
    const float columnWidth = std::min(430.0f, w * 0.46f);
    const float leftX = w * 0.5f - columnWidth - 10.0f;
    const float rightX = w * 0.5f + 10.0f;
    const float top = headingSpace;

    const float mouseX = static_cast<float>(m_input.mouseX());
    const float mouseY = static_cast<float>(m_input.mouseY());
    m_controlsHover = -1;

    for (int i = 0; i < rowCount; ++i)
    {
        const Action action = static_cast<Action>(i);
        const bool leftColumn = i < rows;
        const float x = leftColumn ? leftX : rightX;
        const float y = top + (leftColumn ? i : i - rows) * rowHeight;

        const bool hovered = mouseX >= x && mouseX < x + columnWidth &&
                             mouseY >= y && mouseY < y + rowHeight - 3.0f;
        if (hovered) m_controlsHover = i;

        const bool listening = (m_rebinding == action);
        const bool clash = m_keys.conflicts(action);

        roundedQuad(x, y, columnWidth, rowHeight - 3.0f,
                    listening ? glm::vec4(0.36f, 0.40f, 0.52f, 0.95f)
                              : (hovered ? glm::vec4(0.22f, 0.23f, 0.27f, 0.85f)
                                         : glm::vec4(0.12f, 0.13f, 0.16f, 0.70f)),
                    4.0f);

        m_ui.textWithShadow(KeyBindings::name(action), x + 10.0f,
                            y + rowHeight * 0.5f - UIRenderer::textHeight(scale) * 0.5f - 1.5f,
                            scale, TEXT_COLOR);

        // Listening shows "> ... <" the way Minecraft does, and a key
        // used twice turns red so the clash is obvious.
        const std::string bound = listening ? "> PRESS A KEY <"
                                            : KeyBindings::describe(m_keys.binding(action));
        const glm::vec4 colour = listening ? glm::vec4(1.0f, 1.0f, 0.62f, 1.0f)
                                           : (clash ? glm::vec4(1.0f, 0.46f, 0.42f, 1.0f)
                                                    : glm::vec4(1.0f, 0.92f, 0.62f, 1.0f));
        m_ui.textWithShadow(bound,
                            x + columnWidth - UIRenderer::textWidth(bound, scale) - 10.0f,
                            y + rowHeight * 0.5f - UIRenderer::textHeight(scale) * 0.5f - 1.5f,
                            scale, colour);
    }

    const float footerY = top + rows * rowHeight + 14.0f;

    const float buttonWidth = std::min(260.0f, w * 0.3f);
    if (menuButton({ w * 0.5f - buttonWidth - 6.0f, footerY + 24.0f, buttonWidth, 44.0f },
                   "RESET DEFAULTS"))
        m_pendingAction = MenuAction::ResetBindings;

    if (menuButton({ w * 0.5f + 6.0f, footerY + 24.0f, buttonWidth, 44.0f }, "DONE"))
        m_pendingAction = MenuAction::GoSettings;
}

void Application::renderNameEntryScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.62f));

    const bool returning = !m_playerName.empty();
    centredText(returning ? "EDIT PROFILE" : "WELCOME TO BROWSERCRAFT",
                h * 0.05f, 3.4f, TEXT_COLOR);

    // The panel is laid out at a comfortable size and then shrunk to fit
    // a short window. A 1366x768 laptop has barely 600 pixels of page
    // height once the browser's own chrome is gone, and the character
    // grid was falling off the bottom.
    constexpr float PAD = 16.0f;
    constexpr float LABEL = 18.0f;
    constexpr float FIELD_H = 48.0f;
    constexpr float MODEL_H = 42.0f;
    constexpr float SWATCH = 62.0f;
    constexpr int SWATCH_COLUMNS = 5;
    constexpr float CONTENT_H = PAD + LABEL + 6.0f + FIELD_H + 8.0f + LABEL + 8.0f +
                                MODEL_H + 12.0f + LABEL + 4.0f + 2.0f * (SWATCH + 6.0f) + PAD;

    const float headingSpace = h * 0.05f + UIRenderer::textHeight(3.4f) + 22.0f;
    const float buttonSpace = 48.0f + 34.0f;
    const float available = h - headingSpace - buttonSpace - 16.0f;
    const float ui = std::clamp(available / CONTENT_H, 0.55f, 1.0f);

    const float pad = PAD * ui;
    const float label = LABEL * ui;
    const float fieldHeight = FIELD_H * ui;
    const float modelHeight = MODEL_H * ui;
    const float swatch = SWATCH * ui;

    const float panelHeight = CONTENT_H * ui;
    const float panelWidth = std::min(640.0f, w * 0.86f);
    const float panelX = w * 0.5f - panelWidth * 0.5f;
    const float panelY = headingSpace;
    guiPanel(panelX, panelY, panelWidth, panelHeight);

    // --- your character, big, down the left ---
    const float dollBoxWidth = std::min(150.0f, panelWidth * 0.26f);
    const float dollBoxHeight = panelHeight - pad * 2.0f - label;
    guiWell(panelX + pad * 0.6f, panelY + pad + label, dollBoxWidth, dollBoxHeight);

    const PlayerSkin& active = activeSkin();
    const float unit = (dollBoxHeight - 16.0f * ui) / 32.0f;
    const float dollWidth = (active.armWidth() * 2.0f + 8.0f) * unit;
    drawPlayerDoll(active, panelX + pad * 0.6f + (dollBoxWidth - dollWidth) * 0.5f,
                   panelY + pad + label + 8.0f * ui, unit);

    const char* variantLabel = PlayerSkin::variantName(m_skinVariant);
    const float labelScale = 1.8f * ui;
    m_ui.text(variantLabel,
              panelX + pad * 0.6f + dollBoxWidth * 0.5f -
                  UIRenderer::textWidth(variantLabel, labelScale) * 0.5f,
              panelY + pad, labelScale, GUI_TEXT);

    // --- name, model type and the character library, down the right ---
    const float rightX = panelX + dollBoxWidth + pad * 1.6f;
    const float rightWidth = panelWidth - dollBoxWidth - pad * 2.6f;
    const float textScale = 1.8f * ui;
    float y = panelY + pad;

    m_ui.text("USERNAME", rightX, y, textScale, GUI_TEXT);
    y += label + 6.0f * ui;

    roundedQuad(rightX - 3.0f, y - 3.0f, rightWidth + 6.0f, fieldHeight + 6.0f,
                glm::vec4(0.627f, 0.627f, 0.627f, 1.0f), 5.0f);
    roundedQuad(rightX, y, rightWidth, fieldHeight, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 4.0f);

    const bool caretOn = ((SDL_GetTicks() / 500u) % 2u) == 0u;
    const float nameScale = 2.6f * ui;
    m_ui.textWithShadow(m_nameDraft + (caretOn ? "_" : " "), rightX + 12.0f * ui,
                        y + fieldHeight * 0.5f - UIRenderer::textHeight(nameScale) * 0.5f,
                        nameScale, TEXT_COLOR);
    y += fieldHeight + 8.0f * ui;

    const int length = static_cast<int>(m_nameDraft.size());
    const bool valid = length >= MIN_NAME_LENGTH;
    y += label + 8.0f * ui;

    // Classic or slim, exactly the choice Eaglercraft offers.
    if (menuButton({ rightX, y, rightWidth, modelHeight },
                   m_skinStyle == PlayerSkin::Style::Slim ? "MODEL: SLIM (3PX ARMS)"
                                                          : "MODEL: CLASSIC (4PX ARMS)"))
        m_pendingAction = MenuAction::ToggleSkinStyle;
    y += modelHeight + 12.0f * ui;

    m_ui.text("CHARACTER", rightX, y, textScale, GUI_TEXT);
    y += label + 4.0f * ui;

    // The library: every painted character, plus your own PNG if there
    // is one in assets/skins.
    const float step = std::min(swatch + 6.0f, rightWidth / SWATCH_COLUMNS);
    const int total = PlayerSkin::variantCount() + (PlayerSkin::customAvailable() ? 1 : 0);

    for (int i = 0; i < total; ++i)
    {
        const int variant = (i < PlayerSkin::variantCount()) ? i : PlayerSkin::CUSTOM_VARIANT;
        const float sx = rightX + (i % SWATCH_COLUMNS) * step;
        const float sy = y + (i / SWATCH_COLUMNS) * step;
        const bool chosen = (variant == m_skinVariant);

        if (chosen)
            m_ui.quad(sx - 3.0f, sy - 3.0f, swatch + 6.0f, swatch + 6.0f, SELECTED_COLOR);
        guiWell(sx, sy, swatch, swatch);

        const PlayerSkin& preview = (variant == PlayerSkin::CUSTOM_VARIANT)
                                        ? m_customSkin : *m_skins[variant];
        const float previewUnit = (swatch - 10.0f * ui) / 32.0f;
        const float previewWidth = (preview.armWidth() * 2.0f + 8.0f) * previewUnit;
        drawPlayerDoll(preview, sx + (swatch - previewWidth) * 0.5f, sy + 5.0f * ui, previewUnit);

        const float mouseX = static_cast<float>(m_input.mouseX());
        const float mouseY = static_cast<float>(m_input.mouseY());
        const bool hovered = mouseX >= sx && mouseX < sx + swatch &&
                             mouseY >= sy && mouseY < sy + swatch;
        if (!m_automated && hovered && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT))
        {
            m_pendingSkinVariant = variant;
            m_pendingAction = MenuAction::PickSkin;
        }
    }

    // --- confirm ---
    const float buttonWidth = std::min(420.0f, panelWidth * 0.5f);
    const float buttonY = panelY + panelHeight + 18.0f;
    const float confirmX = returning ? (w * 0.5f - buttonWidth - 6.0f)
                                     : (w * 0.5f - buttonWidth * 0.5f);

    if (menuButton({ confirmX, buttonY, buttonWidth, 48.0f },
                   returning ? "SAVE" : "PLAY", valid))
        m_pendingAction = MenuAction::ConfirmName;

    if (returning && menuButton({ w * 0.5f + 6.0f, buttonY, buttonWidth, 48.0f }, "CANCEL"))
    {
        m_nameDraft = m_playerName;
        m_pendingAction = MenuAction::GoSettings;
    }

}

void Application::renderPauseMenu()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.62f));

    centredText("GAME PAUSED", h * 0.12f, 5.0f, TEXT_COLOR);
    centredText(gameModeName(m_gameMode), h * 0.12f + 54.0f, 2.0f, DIM_TEXT);

    const float buttonWidth = std::min(440.0f, w * 0.64f);
    const float buttonX = w * 0.5f - buttonWidth * 0.5f;
    const float buttonHeight = 50.0f;
    float y = h * 0.30f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "BACK TO GAME"))
        m_pendingAction = MenuAction::Resume;
    y += buttonHeight + 12.0f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight },
                   m_pbrEnabled ? "PBR SHADING: ON" : "PBR SHADING: OFF"))
        m_pendingAction = MenuAction::TogglePbr;
    y += buttonHeight + 12.0f;

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight },
                   m_audio.muted() ? "SOUND: OFF" : "SOUND: ON"))
        m_pendingAction = MenuAction::ToggleSound;
    y += buttonHeight + 12.0f;

#ifndef __EMSCRIPTEN__
    // The same idea as Minecraft's Open to LAN: the world you are already
    // playing becomes the server, and nobody has to restart anything.
    if (m_net.role() == Net::Role::Host)
    {
        char label[80];
        std::snprintf(label, sizeof(label), "CLOSE TO LAN  -  %d PLAYING", m_net.playerCount());
        if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, label))
            m_pendingAction = MenuAction::CloseToLan;
    }
    else if (m_net.role() == Net::Role::Guest)
    {
        if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "LEAVE THIS WORLD"))
            m_pendingAction = MenuAction::QuitToTitle;
    }
    else if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "OPEN TO LAN"))
    {
        m_pendingAction = MenuAction::OpenToLan;
    }
    y += buttonHeight + 12.0f;
#endif

    if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "SAVE AND QUIT TO TITLE"))
        m_pendingAction = MenuAction::QuitToTitle;
    y += buttonHeight + 20.0f;

}

void Application::renderDeathScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    const bool hardcore = modeIsHardcore(m_gameMode);

    m_ui.quad(0, 0, w, h, hardcore ? glm::vec4(0.28f, 0.0f, 0.0f, 0.70f)
                                   : glm::vec4(0.40f, 0.0f, 0.0f, 0.50f));

    centredText(hardcore ? "GAME OVER" : "YOU DIED", h * 0.26f, 6.5f,
                glm::vec4(1.0f, 0.82f, 0.82f, 1.0f));

    const float buttonWidth = std::min(440.0f, w * 0.64f);
    const float buttonX = w * 0.5f - buttonWidth * 0.5f;
    const float buttonHeight = 52.0f;
    float y = h * 0.48f;

    if (!hardcore)
    {
        if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "RESPAWN"))
            m_pendingAction = MenuAction::Respawn;
        y += buttonHeight + 14.0f;

        if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "QUIT TO TITLE"))
            m_pendingAction = MenuAction::QuitToTitle;
    }
    else
    {
        if (menuButton({ buttonX, y, buttonWidth, buttonHeight }, "DELETE WORLD AND CONTINUE"))
            m_pendingAction = MenuAction::DeleteWorld;
    }
}

// ------------------------------------------------------------ inventory ---



#include "InventoryScreen.inl"

void Application::renderDebugOverlay()
{
    const glm::vec3 p = m_player.position;
    const int bx = static_cast<int>(std::floor(p.x));
    const int by = static_cast<int>(std::floor(p.y));
    const int bz = static_cast<int>(std::floor(p.z));
    const float w = static_cast<float>(m_window.width());

    char buffer[256];
    const float scale = 2.0f;
    const float lineStep = UIRenderer::textHeight(scale) + 6.0f;

    float leftY = 10.0f;
    auto left = [&](const char* text) {
        m_ui.textWithShadow(text, 10.0f, leftY, scale, TEXT_COLOR);
        leftY += lineStep;
    };

    float rightY = 10.0f;
    auto right = [&](const char* text) {
        m_ui.textWithShadow(text, w - UIRenderer::textWidth(text, scale) - 10.0f,
                            rightY, scale, TEXT_COLOR);
        rightY += lineStep;
    };

    const bool detailed = (m_debugDetail >= DebugDetail::Full);
    const bool minimal = (m_debugDetail == DebugDetail::Minimal);

    // ----------------------------------------------------------- left ---
    std::snprintf(buffer, sizeof(buffer), "BROWSERCRAFT 0.1 (%.0f FPS)", m_fps);
    left(buffer);

    if (!minimal)
    {
        std::snprintf(buffer, sizeof(buffer), "C: %d  E: %d  D: %d  M: %d (%d YOUNG)  JOBS: %d",
                      m_world->loadedChunks(), m_particles.count(),
                      m_drops.count(), m_entities.count(), m_entities.youngCount(),
                      m_world->pendingJobs());
        left(buffer);
        left("");
    }

    std::snprintf(buffer, sizeof(buffer), "XYZ: %.3f / %.5f / %.3f", p.x, p.y, p.z);
    left(buffer);

    std::snprintf(buffer, sizeof(buffer), "BLOCK: %d %d %d", bx, by, bz);
    left(buffer);

    if (!minimal)
    {
        std::snprintf(buffer, sizeof(buffer), "CHUNK: %d %d %d IN %d %d",
                      World::floorMod(bx, Chunk::SX), by, World::floorMod(bz, Chunk::SZ),
                      World::floorDiv(bx, Chunk::SX), World::floorDiv(bz, Chunk::SZ));
        left(buffer);

        // Which way you are looking, named the way Minecraft names it.
        const glm::vec3 f = m_camera.front;
        const char* facing = "NORTH";
        const char* towards = "TOWARDS NEGATIVE Z";
        if (std::abs(f.x) > std::abs(f.z))
        {
            facing = (f.x > 0.0f) ? "EAST" : "WEST";
            towards = (f.x > 0.0f) ? "TOWARDS POSITIVE X" : "TOWARDS NEGATIVE X";
        }
        else if (f.z > 0.0f)
        {
            facing = "SOUTH";
            towards = "TOWARDS POSITIVE Z";
        }
        std::snprintf(buffer, sizeof(buffer), "FACING: %s (%s) (%.1f / %.1f)",
                      facing, towards, m_camera.yaw, m_camera.pitch);
        left(buffer);

        if (hasSky())
            std::snprintf(buffer, sizeof(buffer), "BIOME: %s",
                          biomeName(m_world->generator().biomeAt(bx, bz)));
        else
            std::snprintf(buffer, sizeof(buffer), "DIMENSION: %s", dimensionName(m_dimension));
        left(buffer);

        std::snprintf(buffer, sizeof(buffer), "LIGHT: %d (%d SKY, %d BLOCK)",
                      std::max<int>(m_world->skyLight(bx, by + 1, bz),
                                    m_world->blockLightAt(bx, by + 1, bz)),
                      static_cast<int>(m_world->skyLight(bx, by + 1, bz)),
                      static_cast<int>(m_world->blockLightAt(bx, by + 1, bz)));
        left(buffer);

        const int hours = static_cast<int>(m_timeOfDay * 24.0f);
        const int minutes = static_cast<int>((m_timeOfDay * 24.0f - hours) * 60.0f);
        std::snprintf(buffer, sizeof(buffer), "TIME: %02d:%02d  DAYLIGHT: %.2f",
                      hours, minutes, daylightFactor());
        left(buffer);

        std::snprintf(buffer, sizeof(buffer), "MODE: %s%s", gameModeName(m_gameMode),
                      m_player.flying ? " FLYING"
                                      : (m_player.onGround ? " GROUNDED" : " AIRBORNE"));
        left(buffer);
    }

    // ---------------------------------------------------------- right ---
    if (detailed)
    {
        // Queried once: the driver returns a pointer to a static string
        // and asking every frame is pure waste.
        static const std::string gpu = []() {
            const char* text = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
            return std::string(text ? text : "unknown");
        }();
        static const std::string api = []() {
            const char* text = reinterpret_cast<const char*>(glGetString(GL_VERSION));
            return std::string(text ? text : "unknown");
        }();

        std::snprintf(buffer, sizeof(buffer), "DISPLAY: %dx%d", m_window.width(), m_window.height());
        right(buffer);
        right(api.c_str());
        right(gpu.c_str());
        right("");

        std::snprintf(buffer, sizeof(buffer), "SEED: %u", m_world->seed());
        right(buffer);
        std::snprintf(buffer, sizeof(buffer), "RENDER DISTANCE: %d", m_world->renderDistance());
        right(buffer);
        std::snprintf(buffer, sizeof(buffer), "ATLAS: %dpx TILES%s", m_atlas.tilePixels(),
                      m_atlas.hasPbrMaps() ? " +PBR" : "");
        right(buffer);

        // Chunk storage, which is where nearly all the memory goes.
        const double megabytes = m_world->loadedChunks() *
                                 (Chunk::BLOCK_COUNT * (sizeof(BlockId) + 1)) / (1024.0 * 1024.0);
        std::snprintf(buffer, sizeof(buffer), "CHUNK MEMORY: %.1f MB", megabytes);
        right(buffer);
    }

    // The footer names the key, because a view you cannot get back from
    // is worse than no view at all.
    const char* detailName = minimal ? "MINIMAL" : (detailed ? "FULL" : "DEFAULT");
    std::snprintf(buffer, sizeof(buffer), "F3 + F6: %s", detailName);
    m_ui.textWithShadow(buffer, w - UIRenderer::textWidth(buffer, 1.7f) - 10.0f,
                        static_cast<float>(m_window.height()) - lineStep - 8.0f,
                        1.7f, DIM_TEXT);
}
