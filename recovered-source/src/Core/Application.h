#pragma once
#include "Window.h"
#include "Input.h"
#include "Keybinds.h"
#include "Console.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Renderer/Camera.h"
#include "Renderer/UIRenderer.h"
#include "Renderer/SkyRenderer.h"
#include "Renderer/SelectionRenderer.h"
#include "Renderer/MobRenderer.h"
#include "Entity/EntityManager.h"
#include "Audio/SoundSystem.h"
#include "World/World.h"
#include "Player/Player.h"
#include "Player/Inventory.h"
#include <memory>
#include <string>
#include <vector>

// Owns every subsystem and runs the game loop. Read run() first: it shows
// the shape of a frame (input -> simulate -> stream -> render).
class Application
{
public:
    Application();
    ~Application();

    void run();

    // One iteration of the loop. On the web the browser drives this through
    // emscripten_set_main_loop_arg; natively run() just calls it in a loop.
    void frame();

private:
    Window m_window;
    Input m_input;
    Keybinds m_keybinds;
    Atlas m_atlas;
    Shader m_chunkShader;
    SkyRenderer m_sky;
    SelectionRenderer m_selection;
    UIRenderer m_ui;
    MobRenderer m_mobRenderer;

    // One world per dimension, all kept loaded so stepping through a portal
    // does not discard the chunks on the far side. m_dimension says which one
    // the player is standing in; world() is the shorthand for it.
    std::unique_ptr<World> m_worlds[3];
    Dimension m_dimension = Dimension::Overworld;

    World& world() { return *m_worlds[static_cast<int>(m_dimension)]; }
    const World& world() const { return *m_worlds[static_cast<int>(m_dimension)]; }

    // Carries the player through a portal, mapping coordinates on the way.
    void switchDimension(Dimension target);
    void updatePortal(float deltaTime);

    // Looks for a completed obsidian frame around a just-placed block and
    // fills it in. There is no flint and steel yet, so finishing the frame
    // is what lights it.
    bool tryLightPortal(const glm::ivec3& placed);

    float m_portalTimer = 0.0f;  // seconds stood inside a portal
    float m_portalCooldown = 0.0f;
    Camera m_camera;
    Player m_player;
    Inventory m_inventory;
    EntityManager m_entities;
    SoundSystem m_sound;
    float m_stepDistance = 0.0f; // metres walked since the last footstep

    std::string m_savePath;
    glm::vec3 m_spawnPoint{ 0.0f };

    // Which screen is in front of the world. The world keeps rendering behind
    // all of them, so the menus sit over a live scene rather than a backdrop.
    enum class Screen { MainMenu, Singleplayer, Settings, Generating, Playing };
    Screen m_screen = Screen::MainMenu;

    void updateMenu();
    void renderMainMenu();
    void renderSingleplayerMenu();
    void renderSettingsMenu();
    void renderGeneratingScreen();
    void updateGenerating(float deltaTime);

    // Time left on the deliberate pause after generation finishes, so the
    // world is not just built but settled before anyone sees it.
    float m_generationGrace = 0.0f;
    bool m_generationDone = false;

    // Draws a button and reports whether it was clicked this frame.
    bool menuButton(const std::string& label, float x, float y, float w, float h);

    Console m_console;

    // Runs one console command and returns what to print. A reply starting
    // with '!' is shown as an error.
    std::string runCommand(const std::string& command);
    void renderConsole();

    // Menu backdrop: a camera that drifts around above the spawn point, so the
    // menus sit over scenery rather than whatever the player happened to face.
    glm::mat4 panoramaView() const;
    glm::vec3 panoramaEye() const;

    // A saved viewpoint for the menu backdrop. Panoramas are camera positions
    // in the live world rather than captured images, so they stay lit by the
    // real sky and time of day.
    struct Panorama
    {
        std::string name;
        glm::vec3 position{ 0.0f };
        float yaw = 0.0f;
        float pitch = 0.0f;
    };

    std::vector<Panorama> m_panoramas;
    int m_panoramaIndex = -1; // -1 is the default orbit over spawn

    void loadPanoramas();
    void savePanoramas() const;
    std::string currentPanoramaName() const;

    std::string m_playerName;
    bool m_editingName = false;
    std::string m_nameDraft;

    void loadProfile();
    void saveProfile() const;

    float m_lastJumpTap = -1.0f; // for the double-tap that toggles flight

    float m_tickScale = 1.0f;   // /tick rate, 1.0 being normal speed
    bool m_tickFrozen = false;  // /tick freeze

    bool m_controlsOpen = false;
    int m_rebindingAction = -1;   // index into Keybinds::Action, -1 when idle

    void renderControlsScreen();
    void updateControlsScreen();

    bool m_pbrEnabled = true; // LabPBR normal/specular lighting, toggled with P

    float m_timeOfDay = 0.3f; // 0 = midnight, 0.5 = noon
    float m_elapsedSeconds = 0.0f;
    float m_autosaveTimer = 0.0f;

    bool m_paused = false;
    bool m_inventoryOpen = false;
    bool m_showDebug = false;
    bool m_running = true;

    // Mining state
    bool m_hasTarget = false;
    glm::ivec3 m_targetBlock{ 0 };
    float m_breakProgress = 0.0f;

    // Frame statistics
    float m_fps = 0.0f;
    float m_fpsAccumulator = 0.0f;
    int m_framesThisSecond = 0;

    // Input routed through the keymap rather than physical keys.
    bool held(Keybinds::Action action) const;
    bool pressed(Keybinds::Action action) const;

    void handleEvents();
    void updateGameplay(float deltaTime);
    void updateMining(float deltaTime);
    void handlePlacement();
    void render();
    void renderWorld();
    void renderHud();
    void renderPauseMenu();
    void renderInventoryScreen();
    void renderDebugOverlay();

    void setMouseCaptured(bool captured);
    bool uiHasFocus() const { return m_console.open() || m_screen != Screen::Playing || m_controlsOpen || m_paused || m_inventoryOpen || m_player.health <= 0; }

    glm::vec3 sunDirection() const;
    float daylightFactor() const;

    void loadLevel();
    void saveLevel();

    // Blocks in the creative palette, in display order.
    std::vector<BlockId> paletteBlocks() const;
    int paletteSlotAt(int mouseX, int mouseY) const;
};
