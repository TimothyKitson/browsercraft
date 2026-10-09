#pragma once
#include "Window.h"
#include "Input.h"
#include "KeyBindings.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Renderer/Camera.h"
#include "Renderer/UIRenderer.h"
#include "Renderer/SkyRenderer.h"
#include "Renderer/SelectionRenderer.h"
#include "Renderer/GuiTextures.h"
#include "World/World.h"
#include "Player/Player.h"
#include "Player/Inventory.h"
#include "Audio/AudioEngine.h"
#include "Entity/DroppedItems.h"
#include "Entity/Particles.h"
#include "Entity/PlayerSkin.h"
#include "Entity/PlayerModel.h"
#include "Entity/HeldItem.h"
#include "Entity/EntityManager.h"
#include "Entity/MobModel.h"
#include "Game/GameMode.h"
#include "Net/Session.h"
#ifndef __EMSCRIPTEN__
#include "Net/SocketTransport.h"   // LAN announce/discover; desktop only
#endif
#include <string>
#include <vector>
#include <memory>
#include <map>

// Which part of the game is on screen. The world only exists while playing,
// so the title screen can run before anything has been generated.
enum class Screen
{
    NameEntry,   // "profile": your name, your character, your model
    Title,
    Singleplayer,
    Multiplayer,
    Settings,
    Controls,
    Playing,
    Paused,
    Dead
};

// Menu buttons are drawn and hit-tested in the same pass, so a click
// records what to do and the loop carries it out once drawing is finished.
// Starting a world mid-render would pull the ground out from under the
// very UI being drawn.
enum class MenuAction
{
    None,
    GoSingleplayer,
    GoMultiplayer,
    GoSettings,
    GoControls,
    ResetBindings,
    GoNameEntry,
    ConfirmName,
    PickSkin,
    ToggleSkinStyle,
    OpenToLan,
    CloseToLan,
    JoinTyped,
    JoinDiscovered,
    ResumeSaved,
    NewSurvival,
    NewCreative,
    NewHardcore,
    BackToTitle,
    Resume,
    QuitToTitle,
    QuitGame,
    Respawn,
    DeleteWorld,
    TogglePbr,
    ToggleSound,
    RenderDistanceDown,
    RenderDistanceUp
};

// Owns every subsystem and runs the game loop. Read frame() first: it shows
// the shape of a frame (input -> simulate -> stream -> render).
class Application
{
public:
    Application(int windowWidth, int windowHeight);
    ~Application();

    void run();

    // Takes a screenshot this many seconds after the world is ready, then
    // quits. Used for automated visual checks; 0 disables it.
    void setAutoScreenshot(float delaySeconds) { m_autoScreenshotDelay = delaySeconds; }
    void setPbrEnabled(bool enabled) { m_pbrEnabled = enabled; }
    // Skips the title screen and drops straight into a world.
    void setStartupMode(GameMode mode) { m_startupMode = mode; m_startImmediately = true; }
    // Dev aid: open on a particular screen so menus can be screenshotted.
    void setStartupScreen(const std::string& screen) { m_startupScreen = screen; }
    // Generate the next new world from a specific seed instead of a random one.
    void setSeedOverride(uint32_t seed) { m_seedOverride = seed; m_hasSeedOverride = true; }
    // Dev aids: start a world and immediately share it, or join one.
    void setHostOnStart() { m_hostOnStart = true; m_startImmediately = true; }
    void setJoinOnStart(const std::string& address) { m_joinOnStart = address; }
    // Dev aid: stamp a landmark at spawn, so an automated run has
    // something to check block syncing against.
    void setBuildTest() { m_buildTest = true; }
    // Dev aid: line one of every species up in front of the spawn point,
    // so a screenshot shows all eight without waiting for the spawner.
    void setMobTest() { m_mobTest = true; m_startImmediately = true; }
    // Dev aid: sow a field of wheat at every stage of growth in front
    // of the player, so a screenshot shows the whole crop at once.
    void setFarmTest() { m_farmTest = true; m_startImmediately = true; }
    // Dev aid: open straight into another dimension.
    void setStartupDimension(Dimension dimension) { m_startupDimension = dimension; }
    // Dev aid: preview a candidate panorama viewpoint.
    void setPanoramaOverride(uint32_t seed, float x, float y, float z)
    {
        m_panoramaSeed = seed;
        m_panoramaSpot = glm::vec3(x, y, z);
        m_hasPanoramaOverride = true;
    }

private:
    Window m_window;
    Input m_input;
    KeyBindings m_keys;

    // Which control the player is currently rebinding, and the one the
    // mouse is over. Set while the controls screen is open.
    Action m_rebinding = Action::Count;
    int m_controlsHover = -1;
    Atlas m_atlas;
    GuiTextures m_gui;
    Shader m_chunkShader;
    SkyRenderer m_sky;
    SelectionRenderer m_selection;
    UIRenderer m_ui;

    AudioEngine m_audio;
    // Only alive while a world is loaded, so the title screen costs nothing.
    std::unique_ptr<World> m_world;
    Camera m_camera;
    Player m_player;
    Inventory m_inventory;
    DroppedItems m_drops;
    Particles m_particles;
    // One texture per character in the picker. Painting eight 64x32
    // skins costs well under a millisecond, so the whole library is kept
    // resident rather than rebuilt as you scroll through it.
    std::vector<std::unique_ptr<PlayerSkin>> m_skins;
    PlayerSkin m_customSkin;
    int m_skinVariant = 0;
    int m_pendingSkinVariant = 0;   // which swatch was clicked this frame
    PlayerSkin::Style m_skinStyle = PlayerSkin::Style::Classic;

    // Multiplayer. Idle until the world is opened or one is joined, so
    // singleplayer costs nothing but a few bytes of state.
    Net::Session m_net;
#ifndef __EMSCRIPTEN__
    Net::LanDiscovery m_discovery;
#endif
    // Drawing everybody else. Only the choice of character crosses the
    // wire, so each one someone is wearing gets painted here on demand
    // and kept; the key is the variant and the model type together.
    PlayerModel m_playerModel;
    EntityManager m_entities;
    MobModel m_mobModel;
    std::map<int, std::unique_ptr<PlayerSkin>> m_remoteSkins;

    // Limb state cannot come off the wire -- positions arrive twenty
    // times a second and the walk cycle runs every frame -- so it is
    // worked out here from how far each player has moved. Keyed by
    // player id, with the local player under LOCAL_ANIM.
    struct Animation
    {
        float phase = 0.0f;     // radians around the walk cycle
        float amount = 0.0f;    // 0 standing, 1 at walking pace
        float bodyYaw = 0.0f;   // trails the head
        glm::vec3 lastPosition{ 0.0f };
        bool started = false;
    };
    static constexpr uint32_t LOCAL_ANIM = 0xFFFFFFFFu;
    std::map<uint32_t, Animation> m_animation;

    // Name tags, gathered while the world is drawn and written out in
    // the HUD pass: the text has to go on top of everything, but only
    // the world pass knows where on screen each player ended up.
    struct Nameplate
    {
        std::string name;
        float x = 0.0f, y = 0.0f;
        float scale = 1.0f;
    };
    std::vector<Nameplate> m_nameplates;

    // Minecraft's F5: your own eyes, behind you, or facing you.
    enum class Perspective { FirstPerson, ThirdBack, ThirdFront };
    Perspective m_perspective = Perspective::FirstPerson;

    std::string m_addressDraft = "127.0.0.1";
    // A guest may already be standing in a singleplayer world when it
    // joins, so "has the world been rebuilt from the host's seed yet?"
    // cannot be answered by looking at m_world.
    bool m_joinedWorldBuilt = false;
    int m_joinChoice = -1;      // index into the discovered list, set by a click

    Screen m_screen = Screen::Title;
    MenuAction m_pendingAction = MenuAction::None;
    GameMode m_gameMode = GameMode::Survival;
    bool m_hardcoreDeath = false; // the run is over; no respawn offered

    // Footstep pacing and landing detection.
    float m_distanceWalked = 0.0f;
    bool m_wasOnGround = true;
    int m_previousHealth = Player::MAX_HEALTH;

    std::string m_savePath = "world";
    glm::vec3 m_spawnPoint{ 0.0f };

    // 0 = midnight, 0.5 = noon. Mid-morning: bright enough to show the
    // scenery off, with the sun still low enough to be interesting.
    static constexpr float TITLE_TIME_OF_DAY = 0.38f;
    float m_timeOfDay = TITLE_TIME_OF_DAY;
    float m_elapsedSeconds = 0.0f;
    float m_autosaveTimer = 0.0f;
    float m_titleSpin = 0.0f;            // slow camera drift behind the menus
    bool m_menuWorld = false;            // the loaded world is the title panorama
    glm::vec3 m_menuFocus{ 0.0f };       // point the title camera orbits

    std::string m_playerName;   // empty until the first visit picks one
    std::string m_nameDraft;    // what is being typed on the name screen

    bool m_inventoryOpen = false;
    bool m_showDebug = false;
    // How much the debug screen shows. F3+F6 cycles it, the way F3+F4
    // and friends change the screen in Minecraft.
    enum class DebugDetail { Minimal, Default, Full };
    DebugDetail m_debugDetail = DebugDetail::Default;
    bool m_hudHidden = false;   // F1
    bool m_running = true;

    // Physics stays frozen until the chunk under the player exists --
    // otherwise they fall through an empty world and get buried when
    // terrain finally streams in.
    bool m_worldReady = false;
    bool m_placeOnSurface = false;

    // LabPBR normal/specular shading from the resource pack (toggle with P).
    bool m_pbrEnabled = true;

    float m_autoScreenshotDelay = 0.0f;
    float m_readySeconds = 0.0f;
    bool m_pendingScreenshot = false;
    bool m_automated = false;
    bool m_startImmediately = false;
    GameMode m_startupMode = GameMode::Survival;
    std::string m_startupScreen;
    uint32_t m_seedOverride = 0;
    bool m_hasSeedOverride = false;
    bool m_hostOnStart = false;
    std::string m_joinOnStart;
    bool m_buildTest = false;
    bool m_buildTestDone = false;
    bool m_mobTest = false;
    bool m_mobTestDone = false;
    bool m_farmTest = false;
    bool m_farmTestDone = false;
    Dimension m_startupDimension = Dimension::Overworld;
    uint32_t m_panoramaSeed = 0;
    glm::vec3 m_panoramaSpot{ 0.0f };
    bool m_hasPanoramaOverride = false;

    unsigned long long m_previousTicks = 0;

    // Mining state
    bool m_hasTarget = false;
    glm::ivec3 m_targetBlock{ 0 };
    float m_breakProgress = 0.0f;
    float m_digSoundTimer = 0.0f;
    float m_attackCooldown = 0.0f;
    float m_eatTimer = 0.0f;
    HeldItem m_heldItem;
    float m_growthTimer = 0.0f;
    uint32_t m_growthSeed = 0x1F123BB5u;

    // Frame statistics
    float m_fps = 0.0f;
    float m_fpsAccumulator = 0.0f;
    int m_framesThisSecond = 0;

    bool frame();

    void handleEvents();
    void waitForSpawnChunk();
    void updateGameplay(float deltaTime);
    void updateMovementAudio(const glm::vec3& positionBefore, bool jumped);
    void updateMining(float deltaTime);
    void handlePlacement();

    void render();
    void renderWorld();
    void renderMenuBackdrop(float deltaTime);
    void renderHud();
    void renderTitleScreen();
    void renderSingleplayerScreen();
    void renderMultiplayerScreen();
    void renderSettingsScreen();
    void renderControlsScreen();
    void renderNameEntryScreen();
    // Everyone else, and yourself when the camera is not behind your own
    // eyes. Called from renderWorld, inside the opaque pass.
    void renderPlayers(const glm::mat4& view, const glm::mat4& projection);
    void renderNameplates();
    // Steps every mob and plays whatever noise they made.
    void updateMobs(float deltaTime);
    // Mobs and crops together, for both the played and automated paths.
    void updateWorldAround(float deltaTime);
    // Advances every player's walk cycle from the ground they covered.
    void updatePlayerAnimation(float deltaTime);
    void advanceAnimation(Animation& animation, const glm::vec3& position,
                          float headYaw, float deltaTime);
    // The painted character someone picked, built the first time it is
    // asked for.
    const PlayerSkin& skinFor(const Net::Appearance& look);
    Net::Appearance localAppearance() const;
    // Where the camera actually sits this frame. The eye camera stays
    // put so that mining, placing and the selection outline keep working
    // from the player's eyes whatever the view is set to.
    Camera viewCamera() const;
    void renderPauseMenu();
    void renderDeathScreen();
    void renderInventoryScreen();
    void drawItem(float x, float y, float size, StackId id, int count, const glm::vec4& tint);
    void drawPlayerDoll(const PlayerSkin& skin, float x, float y, float unit);
    // Rebuilds every painted character at the current model type.
    void rebuildSkinLibrary();
    const PlayerSkin& activeSkin() const;
    void handleInventoryClick(int mouseX, int mouseY, bool rightButton, bool shiftHeld);
    void returnCursorToWorld();
    void renderDebugOverlay();
    void renderHand();
    void renderHearts(float x, float y);
    void renderFood(float rightX, float y);
    void renderEatProgress(float cx, float cy);
    void handleEating(float deltaTime);

    // --- simple immediate-mode menu widgets ---
    struct Rect { float x, y, w, h; };
    bool menuButton(const Rect& rect, const std::string& label, bool enabled = true);

    // Minecraft's whole interface is one shape: a flat slab with a
    // two-pixel bevel, light along the top and left and dark along the
    // bottom and right. Invert it and the same slab reads as a hole,
    // which is all an inventory slot is.
    // A filled rectangle with its corners stepped in. Minecraft cuts the
    // corner pixels off its panels and buttons rather than drawing a
    // true curve, which at interface scale reads as a soft corner and
    // stays crisp at any size -- a real radius would blur.
    void roundedQuad(float x, float y, float w, float h, const glm::vec4& colour, float cut = 4.0f);
    void bevel(float x, float y, float w, float h, const glm::vec4& face,
               const glm::vec4& light, const glm::vec4& dark, float edge = 2.0f);
    void guiPanel(float x, float y, float w, float h);
    void guiWell(float x, float y, float w, float h);
    void centredText(const std::string& text, float y, float scale, const glm::vec4& colour);
    void applyMenuAction();

    void setMouseCaptured(bool captured);
    bool playing() const { return m_screen == Screen::Playing; }
    bool uiHasFocus() const { return m_screen != Screen::Playing || m_inventoryOpen; }

    glm::vec3 sunDirection() const;
    float daylightFactor() const;

    // The Nether and the End have no sun, so they get a flat backdrop
    // and a raised light floor instead of a sky and a day cycle.
    bool hasSky() const { return m_dimension == Dimension::Overworld; }
    glm::vec3 dimensionBackdrop() const;
    float dimensionAmbient() const;

    // --- world lifecycle ---
    bool saveExists() const;
    void startWorld(GameMode mode, bool freshWorld);
    void quitToTitle();
    void deleteSavedWorld();
    void loadLevel();
    void saveLevel();

    // A real world generated behind the menus, with the camera slowly
    // orbiting it -- the same trick as Minecraft's title panorama, except
    // it's live terrain rather than a pre-rendered cubemap.
    // The player's name lives beside the save, so the browser keeps it in
    // IndexedDB along with the world.
    void loadProfile();
    void saveProfile();
    // Pushes the in-memory filesystem out to IndexedDB. Does nothing on
    // the desktop, where the files are already on disk.
    void flushSaveStorage();
    // Saves the bindings and makes sure they survive a page reload.
    void saveBindings();

    // Builds the world a host described, instead of the local save.
    void startJoinedWorld();

    // Rebuilds the world in another dimension, keeping the same save and
    // dropping the player somewhere they can stand.
    void travelTo(Dimension dimension);
    Dimension m_dimension = Dimension::Overworld;

    void startMenuWorld();
    void updateMenuCamera(float deltaTime);
    bool inGame() const
    {
        return m_screen == Screen::Playing || m_screen == Screen::Paused || m_screen == Screen::Dead;
    }

    // One clickable box in the open inventory. The list is rebuilt while
    // drawing and then reused for hit-testing, so what you see and what
    // you can click can never drift apart -- they used to be two separate
    // copies of the same geometry, and only one of them ever got fixed.
    struct SlotBox
    {
        enum class Kind { Slot, Palette, Tab };
        Kind kind = Kind::Slot;
        int index = 0;        // inventory slot, block id, or tab number
        float x = 0.0f, y = 0.0f, size = 0.0f;
    };
    std::vector<SlotBox> m_slotBoxes;
    int m_creativeTab = 0;

    const SlotBox* slotBoxAt(int mouseX, int mouseY) const;
};
