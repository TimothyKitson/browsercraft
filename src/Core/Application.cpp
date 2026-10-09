#include "Application.h"
#include "GLFunctions.h"
#include "Renderer/Screenshot.h"
#include "Entity/PlayerAnimation.h"
#include "Game/Farming.h"
#include "Game/Food.h"
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
    const glm::vec4 DIM_TEXT(0.72f, 0.74f, 0.78f, 1.0f);

    // What a block turns into when you mine it.
    StackId blockDrop(BlockId id)
    {
        switch (id)
        {
            case Blocks::Grass:
            case Blocks::SnowGrass: return Blocks::Dirt;
            case Blocks::Stone: return Blocks::Cobblestone;
            case Blocks::Leaves:
            case Blocks::BirchLeaves: return Blocks::Air; // leaves drop nothing
            case Blocks::Ice: return Blocks::Air;
            // Where wheat comes from, until there is farmland to grow it
            // on: grass gives up its seeds the way it does in Minecraft,
            // and three of them make a wheat in the crafting grid. That
            // second step is the stopgap and goes when farming lands.
            case Blocks::TallGrass: return Items::WheatSeeds;
            default: return id;
        }
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

    std::printf("\n=== Browsercraft ===\n");
    std::printf("Controls are rebindable in Settings - Controls.\n\n");
}

Application::~Application()
{
    if (m_world) saveLevel();
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
    m_worldReady = false;
    m_placeOnSurface = true;
    m_drops.clear();
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
        m_player.position = state.playerPosition;
        m_camera.yaw = state.yaw;
        m_camera.pitch = state.pitch;
        m_timeOfDay = state.timeOfDay;
        m_player.health = state.health;
        m_player.hunger = state.hunger;
        m_player.saturation = state.saturation;
        m_gameMode = static_cast<GameMode>(state.gameMode);
        m_player.mode = m_gameMode;
        m_inventory.setSelectedSlot(state.selectedSlot);

        for (size_t i = 0; i < state.inventory.size() && i < Inventory::TOTAL_SLOTS; ++i)
        {
            m_inventory.slot(static_cast<int>(i)).id = state.inventory[i].first;
            m_inventory.slot(static_cast<int>(i)).count = state.inventory[i].second;
        }
    }
    else
    {
        m_player.position = m_spawnPoint;
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
    state.saturation = m_player.saturation;
    state.gameMode = static_cast<uint8_t>(m_gameMode);
    state.dead = m_hardcoreDeath;
    state.selectedSlot = m_inventory.selectedSlot();

    for (int i = 0; i < Inventory::TOTAL_SLOTS; ++i)
    {
        const ItemStack& stack = m_inventory.slot(i);
        state.inventory.emplace_back(stack.id, static_cast<uint16_t>(std::max(0, stack.count)));
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

float Application::daylightFactor() const
{
    const float angle = m_timeOfDay * 6.2831853f - 1.5707963f;
    return std::clamp(std::sin(angle) * 2.2f + 0.35f, 0.12f, 1.0f);
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
            m_particles.update(deltaTime, *m_world);
        }

        // Death handling.
        if (playing() && m_player.health <= 0)
        {
            m_screen = Screen::Dead;
            m_hardcoreDeath = modeIsHardcore(m_gameMode);
            m_inventoryOpen = false;
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
        if (m_startupScreen == "inventory" || m_startupScreen == "creative")
        {
            // Dev aid: open the inventory with something in it, so the
            // screenshot shows a real grid rather than 36 empty boxes.
            m_inventoryOpen = true;
            setMouseCaptured(false);
            m_inventory.add(Blocks::Cobblestone, 64);
            m_inventory.add(Blocks::Planks, 37);
            m_inventory.add(Blocks::Torch, 12);
            m_inventory.add(Blocks::Glowstone, 3);
            m_inventory.add(Blocks::DiamondOre, 1);
            m_inventory.add(Blocks::Sand, 22);
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
            returnCursorToWorld();
            m_inventoryOpen = false;
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

    if (m_keys.pressed(m_input, Action::Inventory) && (playing() || m_inventoryOpen))
    {
        if (m_inventoryOpen) returnCursorToWorld();
        m_inventoryOpen = !m_inventoryOpen;
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
}

void Application::updateGameplay(float deltaTime)
{
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
    m_inventory.scrollSelection(m_input.wheelDelta());

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
    m_player.update(deltaTime, *m_world, controls);
    m_camera.position = m_player.eyePosition();
    updateMovementAudio(positionBefore, controls.jump);

    // A little extra field of view while sprinting sells the speed.
    const float targetFov = m_player.sprinting ? 78.0f : 70.0f;
    m_camera.fov += (targetFov - m_camera.fov) * std::clamp(8.0f * deltaTime, 0.0f, 1.0f);

    // --- the rest of the world ---
    updateWorldAround(deltaTime);
    updateMining(deltaTime);
    handleEating(deltaTime);
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

    if (jumped && m_wasOnGround && !m_player.flying)
        m_audio.play(Sound::Jump, 0.5f);

    if (m_player.onGround && !m_wasOnGround && !m_player.flying)
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
                target->damage(PUNCH_DAMAGE, target->position() - m_player.position);
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
    const float hardness = blockInfo(id).hardness;
    if (hardness < 0.0f) return; // bedrock and liquids never break

    if (m_player.creative() || hardness <= 0.0f)
        m_breakProgress = 1.0f;
    else
        m_breakProgress += deltaTime / hardness;
        m_heldItem.swing();

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
            else
            {
                const StackId drop = blockDrop(id);
                if (drop != Blocks::Air) m_drops.spawnFromBrokenBlock(hit.block, drop);
            }
        }
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

    ItemStack& stack = m_inventory.selected();
    if (stack.empty()) return;

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

    // Items are not blocks and never become one: a handful of wheat
    // cannot be set down in the world the way a plank can.
    if (isItem(stack.id)) return;

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
    const BlockId placing = asBlock(stack.id);
    if (overlapsPlayer && isSolid(placing)) return;

    m_world->setBlock(target.x, target.y, target.z, placing);
    m_net.onLocalBlockChange(target.x, target.y, target.z, placing);
    m_audio.playPlace(placing);

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
            else if (!m_hudHidden) renderHud();
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
    m_sky.render(view, projection, backdrop.position, sunDirection(), daylightFactor(), m_elapsedSeconds);
}

glm::vec3 Application::dimensionBackdrop() const
{
    switch (m_dimension)
    {
        case Dimension::Nether: return glm::vec3(0.30f, 0.09f, 0.07f);
        case Dimension::End:    return glm::vec3(0.06f, 0.04f, 0.09f);
        default:                return glm::vec3(0.5f, 0.72f, 0.95f);
    }
}

float Application::dimensionAmbient() const
{
    switch (m_dimension)
    {
        case Dimension::Nether: return 0.38f;   // gloomy, not blind
        case Dimension::End:    return 0.18f;
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
    const float daylight = daylightFactor();
    const glm::vec3 sun = sunDirection();

    // Only the overworld has a sun to draw; the others are a flat
    // backdrop already cleared to the right colour.
    if (hasSky())
        m_sky.render(view, projection, camera.position, sun, daylight, m_elapsedSeconds);

    const bool underwater = m_player.isHeadUnderwater(*m_world);
    const glm::vec3 fogColor = hasSky() ? SkyRenderer::horizonColor(daylight, sun)
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
                      std::max(0.35f, daylightFactor());
    const float blockLight = static_cast<float>(m_world->blockLightAt(lx, ly, lz)) / 15.0f;

    const ItemStack& held = m_inventory.selected();

    m_heldItem.render(m_chunkShader, m_camera, skinFor(localAppearance()), m_atlas,
                      held.empty() ? Blocks::Air : held.id,
                      sky, blockLight, m_player.sneaking);
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

    m_entities.update(deltaTime, *m_world, m_player.position, daylightFactor());

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
        if (!m_player.damage(strike.damage)) continue;

        m_audio.play(Sound::Hurt, 0.8f);

        const glm::vec3 away = m_player.position - strike.from;
        const glm::vec3 flat(away.x, 0.0f, away.z);
        const float length = glm::length(flat);
        if (length > 0.001f) m_player.velocity += (flat / length) * 6.0f;
        if (m_player.onGround) m_player.velocity.y = std::max(m_player.velocity.y, 4.0f);
    }

    // What they leave behind. Most species drop nothing yet, because
    // what they ought to drop -- leather, bone, string -- are items, and
    // this engine only has blocks.
    for (const EntityManager::Death& death : m_entities.drainDeaths())
    {
        const MobType& type = mobType(death.type);
        const glm::vec3 where = death.position + glm::vec3(0.0f, type.height * 0.5f, 0.0f);

        for (int i = 0; i < type.dropCount; ++i) m_drops.spawn(where, type.drop, 1);
        for (int i = 0; i < type.secondDropCount; ++i) m_drops.spawn(where, type.secondDrop, 1);
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

void Application::renderHud()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

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

        const TileUV uv = tileUV(atlasTileFor(stack.id));
        m_ui.texturedQuad(m_atlas.textureId(), slotX + 5.0f, barY + 5.0f, SLOT_SIZE - 10.0f, SLOT_SIZE - 10.0f,
                          uv.u0, uv.vTop, uv.u1, uv.vBottom, glm::vec4(1.0f));

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
        m_ui.textWithShadow(name, cx - UIRenderer::textWidth(name, 2.2f) * 0.5f, barY - 30.0f, 2.2f, TEXT_COLOR);
    }

    // Hearts are hidden in creative, like Minecraft.
    if (!m_player.creative())
    {
        renderHearts(barX + SLOT_GAP, barY - 60.0f);
        renderFood(barX + barWidth - SLOT_GAP, barY - 60.0f);
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
