#pragma once
#include "Window.h"
#include "Input.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Renderer/Camera.h"
#include "Renderer/UIRenderer.h"
#include "Renderer/SkyRenderer.h"
#include "Renderer/SelectionRenderer.h"
#include "World/World.h"
#include "Player/Player.h"
#include "Player/Inventory.h"
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
    Atlas m_atlas;
    Shader m_chunkShader;
    SkyRenderer m_sky;
    SelectionRenderer m_selection;
    UIRenderer m_ui;

    World m_world;
    Camera m_camera;
    Player m_player;
    Inventory m_inventory;

    std::string m_savePath;
    glm::vec3 m_spawnPoint{ 0.0f };

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
    bool uiHasFocus() const { return m_paused || m_inventoryOpen || m_player.health <= 0; }

    glm::vec3 sunDirection() const;
    float daylightFactor() const;

    void loadLevel();
    void saveLevel();

    // Blocks in the creative palette, in display order.
    std::vector<BlockId> paletteBlocks() const;
    int paletteSlotAt(int mouseX, int mouseY) const;
};
