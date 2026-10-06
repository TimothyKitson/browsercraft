#pragma once
#include "Window.h"
#include "Input.h"
#include "Keybinds.h"
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
    bool uiHasFocus() const { return m_controlsOpen || m_paused || m_inventoryOpen || m_player.health <= 0; }

    glm::vec3 sunDirection() const;
    float daylightFactor() const;

    void loadLevel();
    void saveLevel();

    // Blocks in the creative palette, in display order.
    std::vector<BlockId> paletteBlocks() const;
    int paletteSlotAt(int mouseX, int mouseY) const;
};
