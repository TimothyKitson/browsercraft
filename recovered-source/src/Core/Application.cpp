#include "Application.h"
#include <vector>
#include <cctype>
#include <sstream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "GLFunctions.h"
#include "World/WorldSave.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{
    constexpr int DEFAULT_RENDER_DISTANCE = 8;
    constexpr float REACH_DISTANCE = 5.0f;
    constexpr float DAY_LENGTH_SECONDS = 1200.0f; // 20 minutes, like Minecraft
    constexpr float AUTOSAVE_INTERVAL = 60.0f;
    constexpr float MOUSE_SENSITIVITY = 0.11f;

    // Hotbar / inventory geometry, in pixels before UI scaling.
    constexpr float SLOT_SIZE = 46.0f;
    constexpr float SLOT_GAP = 4.0f;

    const glm::vec4 PANEL_COLOR(0.09f, 0.09f, 0.11f, 0.82f);
    const glm::vec4 SLOT_COLOR(0.28f, 0.28f, 0.32f, 0.85f);
    const glm::vec4 SELECTED_COLOR(1.0f, 1.0f, 1.0f, 0.95f);
    const glm::vec4 TEXT_COLOR(0.96f, 0.96f, 0.96f, 1.0f);

    // What a block turns into when you mine it.
    BlockId blockDrop(BlockId id)
    {
        switch (id)
        {
            case Blocks::Grass:
            case Blocks::SnowGrass: return Blocks::Dirt;
            case Blocks::Stone: return Blocks::Cobblestone;
            case Blocks::Leaves:
            case Blocks::BirchLeaves: return Blocks::Air; // leaves drop nothing
            case Blocks::Ice: return Blocks::Air;
            default: return id;
        }
    }
}

Application::Application()
    : m_window("Voxel Game", 1280, 720)
    , m_atlas()
    , m_chunkShader("assets/shaders/chunk.vert", "assets/shaders/chunk.frag")
    , m_sky()
    , m_selection()
    , m_ui()
    , m_camera(glm::vec3(0.0f), -90.0f, 0.0f)
    , m_player(glm::vec3(0.0f, 80.0f, 0.0f))
    , m_savePath("world")
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    m_keybinds.load(m_savePath + "/keybinds.txt");

    m_console.setHandler([this](const std::string& command) { return runCommand(command); });

    // The seed is shared so the three dimensions line up with one another;
    // each generator branches on its own dimension.
    for (int i = 0; i < 3; ++i)
        m_worlds[i] = std::make_unique<World>(0u, m_savePath, DEFAULT_RENDER_DISTANCE,
                                              static_cast<Dimension>(i));

    loadLevel();

    // The game opens on the main menu, so the cursor has to stay free. Relative
    // mouse mode stops mouseX/mouseY tracking the pointer, which would leave
    // every menu button unclickable.
    setMouseCaptured(m_screen == Screen::Playing);

    std::printf("\n=== Voxel Game ===\n");
    std::printf("%s move | %s jump | %s sneak | %s sprint\n",
                "WASD",
                m_keybinds.bindingLabel(Keybinds::Jump).c_str(),
                m_keybinds.bindingLabel(Keybinds::Sneak).c_str(),
                m_keybinds.bindingLabel(Keybinds::Sprint).c_str());
    std::printf("%s mine | %s place | %s pick\n",
                m_keybinds.bindingLabel(Keybinds::Attack).c_str(),
                m_keybinds.bindingLabel(Keybinds::Use).c_str(),
                m_keybinds.bindingLabel(Keybinds::Pick).c_str());
    std::printf("1-9 / scroll hotbar | %s inventory | double-tap jump to fly\n",
                m_keybinds.bindingLabel(Keybinds::Inventory).c_str());
    std::printf("F3 debug | %s %s render distance | ~ menu | T chat | / command\n\n",
                m_keybinds.bindingLabel(Keybinds::DistanceDown).c_str(),
                m_keybinds.bindingLabel(Keybinds::DistanceUp).c_str());
}

Application::~Application()
{
    saveLevel();
}

void Application::setMouseCaptured(bool captured)
{
    m_window.setRelativeMouseMode(captured);
}

void Application::loadLevel()
{
    LevelState state;
    const bool loaded = WorldSave::loadLevel(m_savePath, state);

    int spawnX = 0, spawnY = 80, spawnZ = 0;
    world().generator().findSpawn(spawnX, spawnY, spawnZ);
    m_spawnPoint = glm::vec3(spawnX + 0.5f, static_cast<float>(spawnY), spawnZ + 0.5f);

    if (loaded)
    {
        m_player.position = state.playerPosition;
        m_camera.yaw = state.yaw;
        m_camera.pitch = state.pitch;
        m_timeOfDay = state.timeOfDay;
        m_player.health = state.health;
        m_player.creative = state.creative;
        m_inventory.setSelectedSlot(state.selectedSlot);

        for (size_t i = 0; i < state.inventory.size() && i < Inventory::TOTAL_SLOTS; ++i)
        {
            m_inventory.slot(static_cast<int>(i)).id = state.inventory[i].first;
            m_inventory.slot(static_cast<int>(i)).count = state.inventory[i].second;
        }
        std::printf("Loaded saved world from ./%s\n", m_savePath.c_str());
    }
    else
    {
        m_player.position = m_spawnPoint;
        // Every mode starts empty-handed. Creative picks blocks from the
        // palette, survival mines them, and /give covers the rest.
        std::printf("Created a new world (seed %u)\n", world().seed());
    }

    m_camera.position = m_player.eyePosition();
}

void Application::saveLevel()
{
    LevelState state;
    state.seed = world().seed();
    state.playerPosition = m_player.position;
    state.yaw = m_camera.yaw;
    state.pitch = m_camera.pitch;
    state.timeOfDay = m_timeOfDay;
    state.health = m_player.health;
    state.creative = m_player.creative;
    state.selectedSlot = m_inventory.selectedSlot();

    for (int i = 0; i < Inventory::TOTAL_SLOTS; ++i)
    {
        const ItemStack& stack = m_inventory.slot(i);
        state.inventory.emplace_back(stack.id, static_cast<uint16_t>(std::max(0, stack.count)));
    }

    WorldSave::saveLevel(m_savePath, state);
    world().saveAll();
}

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

namespace
{
    Uint64 g_previousTicks = 0;

#ifdef __EMSCRIPTEN__
    void frameTrampoline(void* arg)
    {
        static_cast<Application*>(arg)->frame();
    }
#endif
}

void Application::run()
{
    g_previousTicks = SDL_GetPerformanceCounter();

#ifdef __EMSCRIPTEN__
    // A blocking while-loop never returns to the browser, which wedges the
    // tab: no input, no compositing, "page unresponsive". The browser has to
    // call us once per frame instead, so hand it the loop body and return.
    // fps 0 means "use requestAnimationFrame", and the 1 makes this call
    // unwind the stack rather than fall through to the end of main().
    emscripten_set_main_loop_arg(frameTrampoline, this, 0, 1);
#else
    while (m_running) frame();
#endif
}

void Application::frame()
{
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
        float deltaTime = static_cast<float>((now - g_previousTicks) / frequency);
        g_previousTicks = now;
        // Clamp so a hitch (or a breakpoint) can't teleport the player
        // through the world on the next physics step.
        deltaTime = std::min(deltaTime, 0.1f);

        m_elapsedSeconds += deltaTime;
        m_fpsAccumulator += deltaTime;
        ++m_framesThisSecond;
        if (m_fpsAccumulator >= 0.5f)
        {
            m_fps = m_framesThisSecond / m_fpsAccumulator;
            m_fpsAccumulator = 0.0f;
            m_framesThisSecond = 0;
        }

        handleEvents();

        // The console takes keys before anything else, so typing never also
        // walks the player around.
        if (m_console.open())
        {
            m_console.handleText(m_input.typedText());
            const SDL_Scancode key = m_input.anyKeyPressed();
            if (key != SDL_SCANCODE_UNKNOWN)
                m_console.handleKey(key, m_input.isKeyDown(SDL_SCANCODE_LCTRL) ||
                                         m_input.isKeyDown(SDL_SCANCODE_RCTRL));
        }
        else if (m_screen == Screen::Playing && !m_paused && !m_inventoryOpen && !m_controlsOpen)
        {
            if (m_input.wasKeyPressed(SDL_SCANCODE_T)) m_console.openInput(false);
            else if (m_input.wasKeyPressed(SDL_SCANCODE_SLASH)) m_console.openInput(true);
        }

        m_console.update(deltaTime);

        // /tick freeze stops the world without stopping the renderer or input.
        if (m_tickFrozen) deltaTime = 0.0f;
        else deltaTime *= m_tickScale;

        if (!m_running)
        {
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
            return;
        }

        if (m_console.open())
            { /* the console already consumed this frame's input */ }
        else if (m_controlsOpen)
            updateControlsScreen();
        else if (m_screen != Screen::Playing)
            updateMenu();
        else
            updateGameplay(deltaTime);

        m_timeOfDay += deltaTime / DAY_LENGTH_SECONDS;
        if (m_timeOfDay >= 1.0f) m_timeOfDay -= 1.0f;

        world().update(m_player.position);

        m_autosaveTimer += deltaTime;
        if (m_autosaveTimer >= AUTOSAVE_INTERVAL)
        {
            m_autosaveTimer = 0.0f;
            saveLevel();
        }

        render();
        m_window.swapBuffers();
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

    // Flying is a double-tap of jump and only in creative, so it is not a
    // binding and never appears on the Controls screen.
    if (pressed(Keybinds::Jump))
    {
        if (m_player.creative && m_elapsedSeconds - m_lastJumpTap < 0.3f)
        {
            m_player.flying = !m_player.flying;
            m_lastJumpTap = -1.0f;
        }
        else
        {
            m_lastJumpTap = m_elapsedSeconds;
        }
    }

    // The menu opens on ~, which is fixed.
    if (m_input.wasKeyPressed(SDL_SCANCODE_GRAVE))
    {
        if (m_inventoryOpen) m_inventoryOpen = false;
        else m_paused = !m_paused;
        setMouseCaptured(!uiHasFocus());
    }

    if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        if (m_inventoryOpen) m_inventoryOpen = false;
        else m_paused = !m_paused;
        setMouseCaptured(!uiHasFocus());
    }

    if (m_paused && m_input.wasKeyPressed(SDL_SCANCODE_C))
    {
        m_controlsOpen = true;
        m_rebindingAction = -1;
        setMouseCaptured(false);
    }

    if (m_paused && pressed(Keybinds::Drop)) m_running = false;

    if (pressed(Keybinds::Inventory))
    {
        m_inventoryOpen = !m_inventoryOpen;
        m_paused = false;
        setMouseCaptured(!uiHasFocus());
    }

    if (m_input.wasKeyPressed(SDL_SCANCODE_F3)) m_showDebug = !m_showDebug;

    if (m_player.health <= 0 && m_input.wasKeyPressed(SDL_SCANCODE_R))
    {
        m_player.respawn(m_spawnPoint);
        setMouseCaptured(true);
    }

    if (pressed(Keybinds::DistanceDown))
        world().setRenderDistance(world().renderDistance() - 1);
    if (pressed(Keybinds::DistanceUp))
        world().setRenderDistance(world().renderDistance() + 1);

    // Clicking inside the inventory screen assigns to the selected hotbar slot.
    if (m_inventoryOpen && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT))
    {
        const int index = paletteSlotAt(m_input.mouseX(), m_input.mouseY());
        if (index >= 0)
        {
            const std::vector<BlockId> palette = paletteBlocks();
            if (index < static_cast<int>(palette.size()))
            {
                ItemStack& target = m_inventory.selected();
                target.id = palette[index];
                target.count = m_player.creative ? 64 : std::max(target.count, 1);
            }
        }
    }
}


bool Application::held(Keybinds::Action action) const
{
    if (m_keybinds.device(action) == Keybinds::Mouse)
        return m_input.isMouseButtonDown(static_cast<Uint8>(m_keybinds.code(action)));
    return m_input.isKeyDown(static_cast<SDL_Scancode>(m_keybinds.code(action)));
}

bool Application::pressed(Keybinds::Action action) const
{
    if (m_keybinds.device(action) == Keybinds::Mouse)
        return m_input.wasMouseButtonPressed(static_cast<Uint8>(m_keybinds.code(action)));
    return m_input.wasKeyPressed(static_cast<SDL_Scancode>(m_keybinds.code(action)));
}



bool Application::tryLightPortal(const glm::ivec3& placed)
{
    if (m_dimension == Dimension::End) return false;

    constexpr int INNER_W = 2; // classic frame: 2 wide, 3 tall inside
    constexpr int INNER_H = 3;

    // Try both upright orientations: the frame runs along X or along Z.
    const glm::ivec3 axes[2] = { { 1, 0, 0 }, { 0, 0, 1 } };

    for (const glm::ivec3& across : axes)
    {
        // The placed block could be any part of the frame, so sweep every
        // interior origin that could involve it.
        for (int offA = -(INNER_W + 1); offA <= INNER_W + 1; ++offA)
        {
            for (int offY = -(INNER_H + 1); offY <= INNER_H + 1; ++offY)
            {
                const glm::ivec3 origin = placed + across * offA + glm::ivec3(0, offY, 0);

                bool frameOk = true;

                // Interior must be clear.
                for (int a = 0; a < INNER_W && frameOk; ++a)
                    for (int h = 0; h < INNER_H && frameOk; ++h)
                    {
                        const glm::ivec3 cell = origin + across * a + glm::ivec3(0, h, 0);
                        const BlockId inside = world().getBlock(cell.x, cell.y, cell.z);
                        if (inside != Blocks::Air) frameOk = false;
                    }

                // Sides, floor and lintel must all be obsidian.
                for (int h = 0; h < INNER_H && frameOk; ++h)
                {
                    const glm::ivec3 left = origin + across * -1 + glm::ivec3(0, h, 0);
                    const glm::ivec3 right = origin + across * INNER_W + glm::ivec3(0, h, 0);
                    if (world().getBlock(left.x, left.y, left.z) != Blocks::Obsidian) frameOk = false;
                    if (world().getBlock(right.x, right.y, right.z) != Blocks::Obsidian) frameOk = false;
                }
                for (int a = 0; a < INNER_W && frameOk; ++a)
                {
                    const glm::ivec3 below = origin + across * a + glm::ivec3(0, -1, 0);
                    const glm::ivec3 above = origin + across * a + glm::ivec3(0, INNER_H, 0);
                    if (world().getBlock(below.x, below.y, below.z) != Blocks::Obsidian) frameOk = false;
                    if (world().getBlock(above.x, above.y, above.z) != Blocks::Obsidian) frameOk = false;
                }

                if (!frameOk) continue;

                for (int a = 0; a < INNER_W; ++a)
                    for (int h = 0; h < INNER_H; ++h)
                    {
                        const glm::ivec3 cell = origin + across * a + glm::ivec3(0, h, 0);
                        world().setBlock(cell.x, cell.y, cell.z, Blocks::NetherPortal);
                    }

                m_sound.play("random/glass", 3, glm::vec3(origin) + 0.5f, 0.9f, 0.7f);
                std::printf("[Portal] lit a portal at %d %d %d\n", origin.x, origin.y, origin.z);
                return true;
            }
        }
    }

    return false;
}

void Application::switchDimension(Dimension target)
{
    if (target == m_dimension) return;

    // The Nether is eight times smaller horizontally, so a short walk there
    // covers a long one here. Vertical position carries over unchanged.
    const float scale = (target == Dimension::Nether && m_dimension == Dimension::Overworld) ? (1.0f / 8.0f)
                      : (m_dimension == Dimension::Nether && target == Dimension::Overworld) ? 8.0f
                      : 1.0f;

    glm::vec3 destination = m_player.position;
    destination.x *= scale;
    destination.z *= scale;

    m_dimension = target;
    m_entities.clear(); // mobs do not follow you through

    // Let the destination stream in before looking for ground, otherwise every
    // arrival lands in ungenerated air.
    world().update(destination);

    // Drop to the first solid footing under the arrival column, and if there
    // is none, carve a small pocket rather than dumping the player in a wall.
    int groundY = -1;
    const int cx = static_cast<int>(std::floor(destination.x));
    const int cz = static_cast<int>(std::floor(destination.z));
    for (int y = std::min(Chunk::SY - 3, 118); y > 2; --y)
    {
        if (isSolid(world().getBlock(cx, y, cz)) &&
            !isSolid(world().getBlock(cx, y + 1, cz)) &&
            !isSolid(world().getBlock(cx, y + 2, cz)))
        {
            groundY = y + 1;
            break;
        }
    }

    if (groundY < 0)
    {
        groundY = (target == Dimension::Nether) ? 64 : 62;
        for (int dx = -1; dx <= 1; ++dx)
            for (int dz = -1; dz <= 1; ++dz)
            {
                world().setBlock(cx + dx, groundY - 1, cz + dz,
                                 target == Dimension::Nether ? Blocks::Netherrack : Blocks::EndStone);
                for (int dy = 0; dy < 3; ++dy)
                    world().setBlock(cx + dx, groundY + dy, cz + dz, Blocks::Air);
            }
    }

    destination.y = static_cast<float>(groundY);
    m_player.position = destination;
    m_player.velocity = glm::vec3(0.0f);
    m_camera.position = m_player.eyePosition();
    m_portalCooldown = 3.0f;

    std::printf("[Portal] now in %s at %.0f %.0f %.0f\n",
                dimensionName(m_dimension), destination.x, destination.y, destination.z);
}

void Application::updatePortal(float deltaTime)
{
    if (m_portalCooldown > 0.0f)
    {
        m_portalCooldown -= deltaTime;
        return;
    }

    const glm::vec3 eye = m_player.eyePosition();
    const bool inPortal = world().getBlock(static_cast<int>(std::floor(eye.x)),
                                           static_cast<int>(std::floor(eye.y)),
                                           static_cast<int>(std::floor(eye.z))) == Blocks::NetherPortal;

    if (!inPortal)
    {
        m_portalTimer = 0.0f;
        return;
    }

    // A moment of standing in it before you go, so brushing past does nothing.
    m_portalTimer += deltaTime;
    if (m_portalTimer < 1.2f) return;

    m_portalTimer = 0.0f;
    switchDimension(m_dimension == Dimension::Overworld ? Dimension::Nether : Dimension::Overworld);
}

void Application::updateGameplay(float deltaTime)
{
    // Toggles above this line run whenever the game is the active screen, so
    // an open inventory or pause menu can always be closed again. Below it is
    // simulation, which pauses while one is up.
    if (uiHasFocus()) return;

    updatePortal(deltaTime);

    // --- mobs ---
    m_entities.update(deltaTime, world(), m_player.position, daylightFactor());

    // Mobs queue their noises; hand them to the mixer.
    m_sound.setListener(m_camera.position, m_camera.front, m_camera.right);
    for (const MobSound& sound : m_entities.drainSounds())
        m_sound.play(sound.name, sound.variants, sound.position, sound.volume, sound.pitch);

    // Footsteps, driven by distance walked so they keep pace with the legs.
    if (m_player.onGround && !m_player.flying)
    {
        const glm::vec3 motion = m_player.velocity * deltaTime;
        m_stepDistance += glm::length(glm::vec3(motion.x, 0.0f, motion.z));
        if (m_stepDistance >= 1.9f)
        {
            m_stepDistance = 0.0f;
            const glm::vec3 feet = m_player.position;
            const BlockId ground = world().getBlock(static_cast<int>(std::floor(feet.x)),
                                                    static_cast<int>(std::floor(feet.y - 0.2f)),
                                                    static_cast<int>(std::floor(feet.z)));
            if (const char* group = blockSoundGroup(ground))
                m_sound.play(std::string("step/") + group, blockStepVariants(group), feet, 0.22f, 0.9f + 0.2f * (m_elapsedSeconds - std::floor(m_elapsedSeconds)));
        }
    }

    // --- look ---
    m_camera.addLook(m_input.mouseDeltaX(), m_input.mouseDeltaY(), MOUSE_SENSITIVITY);

    // --- mode toggles ---
    if (pressed(Keybinds::Pbr)) m_pbrEnabled = !m_pbrEnabled;
    if (m_input.wasKeyPressed(SDL_SCANCODE_G))
    {
        m_player.creative = !m_player.creative;
        if (!m_player.creative) m_player.flying = false;
    }

    // --- hotbar selection ---
    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
        if (pressed(static_cast<Keybinds::Action>(Keybinds::Hotbar1 + i)))
            m_inventory.setSelectedSlot(i);
    m_inventory.scrollSelection(m_input.wheelDelta());

    // --- movement ---
    Player::Controls controls;
    const glm::vec3 flatFront = glm::normalize(glm::vec3(m_camera.front.x, 0.0f, m_camera.front.z));
    const glm::vec3 flatRight = glm::normalize(glm::vec3(m_camera.right.x, 0.0f, m_camera.right.z));

    glm::vec3 wish(0.0f);
    if (held(Keybinds::Forward)) wish += flatFront;
    if (held(Keybinds::Back)) wish -= flatFront;
    if (held(Keybinds::Left)) wish -= flatRight;
    if (held(Keybinds::Right)) wish += flatRight;
    if (glm::length(wish) > 0.0001f) wish = glm::normalize(wish);

    controls.wishDirection = wish;
    controls.jump = pressed(Keybinds::Jump);
    controls.jumpHeld = held(Keybinds::Jump);
    controls.sprint = held(Keybinds::Sprint);
    controls.sneak = held(Keybinds::Sneak);
    controls.descend = controls.sneak;

    // Hold the player still until the ground beneath them exists. Without this
    // a fresh start drops them through ungenerated air and kills them before
    // the first chunk arrives.
    const Chunk* standingOn = world().chunkAt(World::floorDiv(static_cast<int>(std::floor(m_player.position.x)), Chunk::SX),
                                              World::floorDiv(static_cast<int>(std::floor(m_player.position.z)), Chunk::SZ));
    const bool groundReady = standingOn && standingOn->state.load() >= ChunkState::Generated;

    if (!groundReady)
    {
        m_player.velocity = glm::vec3(0.0f);
        m_camera.position = m_player.eyePosition();
    }
    else
    {
        m_player.update(deltaTime, world(), controls);
    }
    m_camera.position = m_player.eyePosition();

    // A little extra field of view while sprinting sells the speed.
    const float targetFov = m_player.sprinting ? 78.0f : 70.0f;
    m_camera.fov += (targetFov - m_camera.fov) * std::clamp(8.0f * deltaTime, 0.0f, 1.0f);

    // --- block interaction ---
    updateMining(deltaTime);
    handlePlacement();

    if (m_player.health <= 0) setMouseCaptured(false);
}

void Application::updateMining(float deltaTime)
{
    const RaycastHit hit = world().raycast(m_camera.position, m_camera.front, REACH_DISTANCE);

    if (!hit.hit || !m_input.isMouseButtonDown(SDL_BUTTON_LEFT))
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

    const BlockId id = world().getBlock(hit.block.x, hit.block.y, hit.block.z);
    const float hardness = blockInfo(id).hardness;
    if (hardness < 0.0f) return; // bedrock and liquids never break

    if (m_player.creative || hardness <= 0.0f)
        m_breakProgress = 1.0f;
    else
        m_breakProgress += deltaTime / hardness;

    if (m_breakProgress >= 1.0f)
    {
        const BlockId drop = blockDrop(id);
        if (const char* group = blockSoundGroup(id))
            m_sound.play(std::string("dig/") + group, 4, glm::vec3(hit.block) + 0.5f, 0.75f, 0.85f);
        world().setBlock(hit.block.x, hit.block.y, hit.block.z, Blocks::Air);
        if (!m_player.creative && drop != Blocks::Air)
            m_inventory.add(drop, 1);
        m_breakProgress = 0.0f;
        m_hasTarget = false;
    }
}

void Application::handlePlacement()
{
    if (m_input.wasMouseButtonPressed(SDL_BUTTON_MIDDLE))
    {
        const RaycastHit hit = world().raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
        if (hit.hit)
        {
            const BlockId picked = world().getBlock(hit.block.x, hit.block.y, hit.block.z);
            if (isObtainable(picked))
            {
                ItemStack& stack = m_inventory.selected();
                stack.id = picked;
                stack.count = std::max(stack.count, m_player.creative ? 64 : 1);
            }
        }
        return;
    }

    if (!m_input.wasMouseButtonPressed(SDL_BUTTON_RIGHT)) return;

    ItemStack& stack = m_inventory.selected();
    if (stack.empty()) return;

    const RaycastHit hit = world().raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
    if (!hit.hit) return;

    const glm::ivec3 target = hit.previous;
    const BlockId existing = world().getBlock(target.x, target.y, target.z);
    if (existing != Blocks::Air && !isLiquid(existing)) return;

    // Don't entomb the player in their own block.
    const AABB box = m_player.aabb();
    const bool overlapsPlayer =
        box.max.x > target.x && box.min.x < target.x + 1 &&
        box.max.y > target.y && box.min.y < target.y + 1 &&
        box.max.z > target.z && box.min.z < target.z + 1;
    if (overlapsPlayer && isSolid(stack.id)) return;

    if (const char* group = blockSoundGroup(stack.id))
        m_sound.play(std::string("dig/") + group, 4, glm::vec3(target) + 0.5f, 0.7f, 0.9f);
    world().setBlock(target.x, target.y, target.z, stack.id);

    if (stack.id == Blocks::Obsidian) tryLightPortal(target);

    if (!m_player.creative)
    {
        stack.count -= 1;
        if (stack.count <= 0) stack.clear();
    }
}

void Application::render()
{
    glViewport(0, 0, m_window.width(), m_window.height());
    glClearColor(0.5f, 0.72f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderWorld();

    m_ui.begin(m_window.width(), m_window.height());
    if (m_player.health > 0 && m_screen == Screen::Playing) renderHud();
    if (m_showDebug) renderDebugOverlay();
    if (m_screen == Screen::Playing) renderConsole();
    if (m_inventoryOpen && m_screen == Screen::Playing) renderInventoryScreen();
    if (m_controlsOpen) renderControlsScreen();
    else if (m_screen == Screen::MainMenu) renderMainMenu();
    else if (m_screen == Screen::Singleplayer) renderSingleplayerMenu();
    else if (m_screen == Screen::Settings) renderSettingsMenu();
    else if (m_paused && m_screen == Screen::Playing) renderPauseMenu();
    if (m_player.health <= 0)
    {
        const float w = static_cast<float>(m_window.width());
        const float h = static_cast<float>(m_window.height());
        m_ui.quad(0, 0, w, h, glm::vec4(0.4f, 0.0f, 0.0f, 0.45f));
        const std::string message = "YOU DIED";
        m_ui.textWithShadow(message, w * 0.5f - UIRenderer::textWidth(message, 6.0f) * 0.5f, h * 0.38f, 6.0f,
                            glm::vec4(1.0f, 0.85f, 0.85f, 1.0f));
        const std::string hint = "PRESS R TO RESPAWN";
        m_ui.textWithShadow(hint, w * 0.5f - UIRenderer::textWidth(hint, 2.5f) * 0.5f, h * 0.5f, 2.5f, TEXT_COLOR);
    }
    m_ui.end();
}

void Application::renderWorld()
{
    const glm::mat4 view = m_camera.viewMatrix();
    const glm::mat4 projection = m_camera.projectionMatrix(m_window.aspect());
    const float daylight = daylightFactor();
    const glm::vec3 sun = sunDirection();

    m_sky.render(view, projection, m_camera.position, sun, daylight, m_elapsedSeconds);

    const bool underwater = m_player.isHeadUnderwater(world());
    const glm::vec3 fogColor = SkyRenderer::horizonColor(daylight, sun);
    const float viewDistance = static_cast<float>(world().renderDistance() * Chunk::SX);

    m_chunkShader.bind();
    m_chunkShader.setMat4("uView", view);
    m_chunkShader.setMat4("uProjection", projection);
    m_chunkShader.setInt("uAtlas", 0);
    m_chunkShader.setFloat("uDaylight", daylight);
    m_chunkShader.setVec3("uFogColor", fogColor);
    m_chunkShader.setFloat("uFogStart", viewDistance * 0.55f);
    m_chunkShader.setFloat("uFogEnd", viewDistance * 0.95f);
    m_chunkShader.setInt("uUnderwater", underwater ? 1 : 0);

    // Atlas::bind puts albedo on unit 0 and the LabPBR normal and specular
    // atlases on 1 and 2, which is what chunk.frag samples behind uPbrEnabled.
    m_chunkShader.setInt("uNormalAtlas", 1);
    m_chunkShader.setInt("uSpecularAtlas", 2);
    m_chunkShader.setInt("uPbrEnabled", m_pbrEnabled ? 1 : 0);
    m_chunkShader.setVec3("uSunDirection", sun);
    m_chunkShader.setVec3("uCameraPos", m_camera.position);
    m_atlas.bind(0);

    Frustum frustum;
    frustum.update(projection * view);

    // Opaque terrain first, nearest to farthest is fine thanks to the depth buffer.
    struct VisibleChunk { Chunk* chunk; float distance; };
    std::vector<VisibleChunk> visible;
    visible.reserve(world().chunks().size());

    for (const auto& [pos, chunk] : world().chunks())
    {
        if (chunk->opaqueMesh.empty() && chunk->transparentMesh.empty()) continue;
        if (!frustum.intersectsAABB(chunk->aabbMin(), chunk->aabbMax())) continue;

        const glm::vec3 center = chunk->worldOrigin() + glm::vec3(Chunk::SX * 0.5f, Chunk::SY * 0.5f, Chunk::SZ * 0.5f);
        visible.push_back({ chunk.get(), glm::length(center - m_camera.position) });
    }

    for (const VisibleChunk& entry : visible)
    {
        if (entry.chunk->opaqueMesh.empty()) continue;
        m_chunkShader.setVec3("uChunkOffset", entry.chunk->worldOrigin());
        entry.chunk->opaqueMesh.draw(GL_TRIANGLES);
    }

    // Mobs are opaque and share the chunk shader, so they go in right after
    // the terrain and before anything that blends.
    m_mobRenderer.render(m_entities, world(), m_chunkShader);

    // Selection outline + mining cracks sit between the two passes so water
    // still blends over them correctly.
    if (!uiHasFocus())
    {
        const RaycastHit hit = world().raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
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
}

void Application::renderHud()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    // Crosshair
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    const glm::vec4 crosshair(1.0f, 1.0f, 1.0f, 0.75f);
    m_ui.quad(cx - 9.0f, cy - 1.0f, 18.0f, 2.0f, crosshair);
    m_ui.quad(cx - 1.0f, cy - 9.0f, 2.0f, 18.0f, crosshair);

    // Hotbar
    const float barWidth = Inventory::HOTBAR_SLOTS * SLOT_SIZE + (Inventory::HOTBAR_SLOTS + 1) * SLOT_GAP;
    const float barX = cx - barWidth * 0.5f;
    const float barY = h - SLOT_SIZE - SLOT_GAP * 3.0f;

    m_ui.quad(barX, barY - SLOT_GAP, barWidth, SLOT_SIZE + SLOT_GAP * 2.0f, PANEL_COLOR);

    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
    {
        const float slotX = barX + SLOT_GAP + i * (SLOT_SIZE + SLOT_GAP);
        const bool selected = (i == m_inventory.selectedSlot());

        m_ui.quad(slotX, barY, SLOT_SIZE, SLOT_SIZE, SLOT_COLOR);
        if (selected)
        {
            // Highlight ring around the active slot.
            const float t = 3.0f;
            m_ui.quad(slotX - t, barY - t, SLOT_SIZE + t * 2, t, SELECTED_COLOR);
            m_ui.quad(slotX - t, barY + SLOT_SIZE, SLOT_SIZE + t * 2, t, SELECTED_COLOR);
            m_ui.quad(slotX - t, barY, t, SLOT_SIZE, SELECTED_COLOR);
            m_ui.quad(slotX + SLOT_SIZE, barY, t, SLOT_SIZE, SELECTED_COLOR);
        }

        const ItemStack& stack = m_inventory.slot(i);
        if (stack.empty()) continue;

        const BlockInfo& info = blockInfo(stack.id);
        m_ui.blockIcon(m_atlas.textureId(), tileUV(info.tileTop), tileUV(info.tileSide),
                       tileUV(info.tileSide), slotX + 5.0f, barY + 5.0f, SLOT_SIZE - 10.0f);

        if (!m_player.creative && stack.count > 1)
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
        const std::string name = blockInfo(held.id).name;
        m_ui.textWithShadow(name, cx - UIRenderer::textWidth(name, 2.2f) * 0.5f, barY - 26.0f, 2.2f, TEXT_COLOR);
    }

    // Health hearts (hidden in creative, like Minecraft).
    if (!m_player.creative)
    {
        const float heartSize = 16.0f;
        const float heartsY = barY - 52.0f;
        const float heartsX = barX + SLOT_GAP;
        for (int i = 0; i < 10; ++i)
        {
            const int hpForHeart = m_player.health - i * 2;
            glm::vec4 color(0.22f, 0.05f, 0.05f, 0.85f);
            if (hpForHeart >= 2) color = glm::vec4(0.86f, 0.16f, 0.16f, 1.0f);
            else if (hpForHeart == 1) color = glm::vec4(0.86f, 0.16f, 0.16f, 0.55f);
            m_ui.quad(heartsX + i * (heartSize + 3.0f), heartsY, heartSize, heartSize, color);
        }
    }

    // Red flash when hurt.
    if (m_player.damageFlash > 0.0f)
        m_ui.quad(0, 0, w, h, glm::vec4(0.7f, 0.0f, 0.0f, m_player.damageFlash * 0.5f));

    if (m_player.creative)
        m_ui.textWithShadow("CREATIVE", 12.0f, h - 24.0f, 2.0f, glm::vec4(0.7f, 0.9f, 1.0f, 0.9f));
}

std::vector<BlockId> Application::paletteBlocks() const
{
    std::vector<BlockId> blocks;
    for (BlockId id = 1; id < Blocks::Count; ++id)
        if (isObtainable(id)) blocks.push_back(id);
    return blocks;
}

int Application::paletteSlotAt(int mouseX, int mouseY) const
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());
    const int columns = 9;
    const float gridWidth = columns * (SLOT_SIZE + SLOT_GAP) + SLOT_GAP;
    const float startX = w * 0.5f - gridWidth * 0.5f + SLOT_GAP;
    const float startY = h * 0.5f - 150.0f;

    const float localX = mouseX - startX;
    const float localY = mouseY - startY;
    if (localX < 0.0f || localY < 0.0f) return -1;

    const int column = static_cast<int>(localX / (SLOT_SIZE + SLOT_GAP));
    const int row = static_cast<int>(localY / (SLOT_SIZE + SLOT_GAP));
    if (column < 0 || column >= columns || row < 0) return -1;

    // Reject clicks that land in the gap between slots.
    if (std::fmod(localX, SLOT_SIZE + SLOT_GAP) > SLOT_SIZE) return -1;
    if (std::fmod(localY, SLOT_SIZE + SLOT_GAP) > SLOT_SIZE) return -1;

    return row * columns + column;
}

void Application::renderInventoryScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());
    const std::vector<BlockId> palette = paletteBlocks();

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));

    const int columns = 9;
    const int rows = static_cast<int>((palette.size() + columns - 1) / columns);
    const float gridWidth = columns * (SLOT_SIZE + SLOT_GAP) + SLOT_GAP;
    const float gridHeight = rows * (SLOT_SIZE + SLOT_GAP) + SLOT_GAP;
    const float startX = w * 0.5f - gridWidth * 0.5f;
    const float startY = h * 0.5f - 150.0f;

    m_ui.quad(startX, startY - 36.0f, gridWidth, gridHeight + 36.0f, PANEL_COLOR);
    m_ui.textWithShadow("SELECT A BLOCK", startX + 10.0f, startY - 28.0f, 2.2f, TEXT_COLOR);

    for (size_t i = 0; i < palette.size(); ++i)
    {
        const int column = static_cast<int>(i) % columns;
        const int row = static_cast<int>(i) / columns;
        const float slotX = startX + SLOT_GAP + column * (SLOT_SIZE + SLOT_GAP);
        const float slotY = startY + SLOT_GAP + row * (SLOT_SIZE + SLOT_GAP);

        m_ui.quad(slotX, slotY, SLOT_SIZE, SLOT_SIZE, SLOT_COLOR);

        const BlockInfo& paletteInfo = blockInfo(palette[i]);
        m_ui.blockIcon(m_atlas.textureId(), tileUV(paletteInfo.tileTop), tileUV(paletteInfo.tileSide),
                       tileUV(paletteInfo.tileSide), slotX + 5.0f, slotY + 5.0f, SLOT_SIZE - 10.0f);

        if (!m_player.creative)
        {
            const int owned = m_inventory.countOf(palette[i]);
            if (owned > 0)
            {
                const std::string count = std::to_string(owned);
                m_ui.textWithShadow(count,
                                    slotX + SLOT_SIZE - UIRenderer::textWidth(count, 1.8f) - 4.0f,
                                    slotY + SLOT_SIZE - UIRenderer::textHeight(1.8f) - 3.0f,
                                    1.8f, TEXT_COLOR);
            }
        }
    }

    const std::string hint = "CLICK TO PUT IT IN THE SELECTED HOTBAR SLOT - E TO CLOSE";
    m_ui.textWithShadow(hint, w * 0.5f - UIRenderer::textWidth(hint, 1.8f) * 0.5f,
                        startY + gridHeight + 14.0f, 1.8f, glm::vec4(0.8f, 0.8f, 0.85f, 1.0f));
}



bool Application::menuButton(const std::string& label, float x, float y, float w, float h)
{
    const float mx = static_cast<float>(m_input.mouseX());
    const float my = static_cast<float>(m_input.mouseY());
    const bool hovered = (mx >= x && mx < x + w && my >= y && my < y + h);

    m_ui.quad(x, y, w, h, hovered ? glm::vec4(0.62f, 0.66f, 0.74f, 0.95f)
                                  : glm::vec4(0.42f, 0.44f, 0.49f, 0.92f));
    m_ui.quad(x, y, w, 2.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.35f));
    m_ui.quad(x, y + h - 2.0f, w, 2.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));

    const float scale = std::max(1.5f, h / 20.0f);
    m_ui.textWithShadow(label,
                        x + w * 0.5f - UIRenderer::textWidth(label, scale) * 0.5f,
                        y + h * 0.5f - UIRenderer::textHeight(scale) * 0.5f,
                        scale, TEXT_COLOR);

    return hovered && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT);
}


namespace
{
    // Splits on whitespace.
    std::vector<std::string> tokenize(const std::string& line)
    {
        std::vector<std::string> out;
        std::istringstream stream(line);
        std::string word;
        while (stream >> word) out.push_back(word);
        return out;
    }

    std::string lower(std::string value)
    {
        for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return value;
    }

    // Accepts "oak_planks", "Oak Planks" or "planks" for the same block.
    BlockId blockFromName(const std::string& wanted)
    {
        const std::string target = lower(wanted);
        BlockId partial = Blocks::Air;

        for (int id = 1; id < Blocks::Count; ++id)
        {
            std::string name = lower(blockInfo(static_cast<BlockId>(id)).name);
            for (char& c : name) if (c == ' ') c = '_';

            if (name == target) return static_cast<BlockId>(id);
            if (partial == Blocks::Air && name.find(target) != std::string::npos)
                partial = static_cast<BlockId>(id);
        }
        return partial;
    }

    // Parses a coordinate, where "~" means "where the player is" and "~5" means
    // five past it, the way Minecraft's relative coordinates work.
    bool parseCoord(const std::string& token, float origin, int& out)
    {
        try
        {
            if (token == "~") { out = static_cast<int>(std::floor(origin)); return true; }
            if (!token.empty() && token[0] == '~')
            {
                out = static_cast<int>(std::floor(origin)) + std::stoi(token.substr(1));
                return true;
            }
            out = std::stoi(token);
            return true;
        }
        catch (...) { return false; }
    }
}

std::string Application::runCommand(const std::string& command)
{
    const std::vector<std::string> args = tokenize(command);
    if (args.empty()) return "";

    const std::string name = lower(args[0]);

    if (name == "help")
    {
        return "Commands: gamemode, give, clear, fill, tp, time, tick, seed, help\n"
               "Not in this build: xp, enchant, difficulty, weather";
    }

    if (name == "gamemode")
    {
        if (args.size() < 2) return "!Usage: /gamemode (survival|creative|adventure|spectator)";
        const std::string mode = lower(args[1]);

        if (mode == "creative" || mode == "c" || mode == "1")
        {
            m_player.creative = true;
            return "Set game mode to Creative";
        }
        if (mode == "survival" || mode == "s" || mode == "0")
        {
            m_player.creative = false;
            m_player.flying = false;
            return "Set game mode to Survival";
        }
        if (mode == "adventure" || mode == "a" || mode == "2")
            return "!Adventure mode is not implemented";
        if (mode == "spectator" || mode == "sp" || mode == "3")
            return "!Spectator mode is not implemented";

        return "!Unknown game mode: " + args[1];
    }

    if (name == "give")
    {
        if (args.size() < 2) return "!Usage: /give <block> [count]";
        const BlockId id = blockFromName(args[1]);
        if (id == Blocks::Air) return "!No block called " + args[1];
        if (!isObtainable(id)) return "!" + std::string(blockInfo(id).name) + " cannot be carried";

        int count = 1;
        if (args.size() >= 3) { try { count = std::stoi(args[2]); } catch (...) { return "!Count must be a number"; } }
        count = std::max(1, std::min(640, count));

        const int added = m_inventory.add(id, count);
        if (added <= 0) return "!No room in the inventory";
        return "Gave " + std::to_string(added) + " " + blockInfo(id).name + " to you";
    }

    if (name == "clear")
    {
        int cleared = 0;
        for (int i = 0; i < Inventory::TOTAL_SLOTS; ++i)
        {
            if (!m_inventory.slot(i).empty()) cleared += m_inventory.slot(i).count;
            m_inventory.slot(i).clear();
        }
        return "Removed " + std::to_string(cleared) + " items from you";
    }

    if (name == "tp")
    {
        if (args.size() < 4) return "!Usage: /tp <x> <y> <z>";
        int x = 0, y = 0, z = 0;
        if (!parseCoord(args[1], m_player.position.x, x) ||
            !parseCoord(args[2], m_player.position.y, y) ||
            !parseCoord(args[3], m_player.position.z, z))
            return "!Coordinates must be numbers, or ~ for your own";

        m_player.position = glm::vec3(x + 0.5f, static_cast<float>(y), z + 0.5f);
        m_player.velocity = glm::vec3(0.0f);
        m_camera.position = m_player.eyePosition();
        return ""; // you can see where you landed
    }

    if (name == "fill")
    {
        if (args.size() < 8) return "!Usage: /fill <x1> <y1> <z1> <x2> <y2> <z2> <block>";
        int x1, y1, z1, x2, y2, z2;
        if (!parseCoord(args[1], m_player.position.x, x1) || !parseCoord(args[2], m_player.position.y, y1) ||
            !parseCoord(args[3], m_player.position.z, z1) || !parseCoord(args[4], m_player.position.x, x2) ||
            !parseCoord(args[5], m_player.position.y, y2) || !parseCoord(args[6], m_player.position.z, z2))
            return "!Coordinates must be numbers, or ~ for your own";

        const BlockId id = (lower(args[7]) == "air") ? Blocks::Air : blockFromName(args[7]);
        if (id == Blocks::Air && lower(args[7]) != "air") return "!No block called " + args[7];

        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);
        if (z1 > z2) std::swap(z1, z2);
        y1 = std::max(0, y1);
        y2 = std::min(Chunk::SY - 1, y2);

        const long long volume = static_cast<long long>(x2 - x1 + 1) * (y2 - y1 + 1) * (z2 - z1 + 1);
        if (volume > 32768) return "!That is " + std::to_string(volume) + " blocks; the limit is 32768";
        if (volume <= 0) return "!That region is empty";

        for (int x = x1; x <= x2; ++x)
            for (int y = y1; y <= y2; ++y)
                for (int z = z1; z <= z2; ++z)
                    world().setBlock(x, y, z, id);

        return "Filled " + std::to_string(volume) + " blocks";
    }

    if (name == "time")
    {
        if (args.size() < 3 || lower(args[1]) != "set") return "!Usage: /time set (day|noon|night|midnight)";
        const std::string when = lower(args[2]);
        if (when == "day") m_timeOfDay = 0.25f;
        else if (when == "noon") m_timeOfDay = 0.5f;
        else if (when == "night") m_timeOfDay = 0.75f;
        else if (when == "midnight") m_timeOfDay = 0.0f;
        else return "!Unknown time: " + args[2];
        return ""; // the sky says it
    }

    if (name == "tick")
    {
        if (args.size() < 2) return "!Usage: /tick (query|rate <number>|freeze|unfreeze)";
        const std::string sub = lower(args[1]);

        if (sub == "query")
            return m_tickFrozen ? "The game is frozen"
                                : "The game is running normally";
        if (sub == "freeze") { m_tickFrozen = true; return "The game is now frozen"; }
        if (sub == "unfreeze") { m_tickFrozen = false; return "The game is no longer frozen"; }
        if (sub == "rate")
        {
            if (args.size() < 3) return "!Usage: /tick rate <number>";
            try { m_tickScale = std::max(0.05f, std::min(10.0f, std::stof(args[2]))); }
            catch (...) { return "!Rate must be a number"; }
            m_tickFrozen = false;
            return "Set the rate to " + std::to_string(m_tickScale) + "x";
        }
        return "!Unknown option: " + args[1];
    }

    if (name == "seed") return "Seed: " + std::to_string(world().seed());

    // Parsed and answered honestly rather than accepted and ignored: these all
    // need a system this build does not have.
    if (name == "xp" || name == "experience")
        return "!/xp is not implemented";
    if (name == "enchant")
        return "!/enchant is not implemented";
    if (name == "difficulty")
        return "!/difficulty is not implemented";
    if (name == "weather")
        return "!/weather is not implemented";

    return "!Unknown or incomplete command";
}

void Application::updateMenu()
{

    // Escape backs out one level, and out of the game entirely from the top.
    if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        if (m_screen == Screen::Singleplayer || m_screen == Screen::Settings)
            m_screen = Screen::MainMenu;
    }
}

void Application::renderMainMenu()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));

    const std::string title = "BROWSERCRAFT";
    m_ui.textWithShadow(title, w * 0.5f - UIRenderer::textWidth(title, 6.0f) * 0.5f, h * 0.12f, 6.0f, TEXT_COLOR);

    const float bw = std::min(430.0f, w * 0.62f);
    const float bh = std::max(34.0f, h * 0.075f);
    const float bx = w * 0.5f - bw * 0.5f;
    float by = h * 0.40f;

    if (menuButton("SINGLEPLAYER", bx, by, bw, bh)) m_screen = Screen::Singleplayer;
    by += bh * 1.25f;
    if (menuButton("SETTINGS", bx, by, bw, bh)) m_screen = Screen::Settings;
    by += bh * 1.25f;
    if (menuButton("QUIT", bx, by, bw, bh)) m_running = false;

    const std::string version = "BROWSERCRAFT 0.1  -  BROWSERCRAFT.NET";
    m_ui.textWithShadow(version, 10.0f, h - UIRenderer::textHeight(1.4f) - 8.0f, 1.4f,
                        glm::vec4(0.85f, 0.88f, 0.92f, 1.0f));
}

void Application::renderSingleplayerMenu()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));

    const std::string title = "SINGLEPLAYER";
    m_ui.textWithShadow(title, w * 0.5f - UIRenderer::textWidth(title, 4.5f) * 0.5f, h * 0.12f, 4.5f, TEXT_COLOR);

    const float bw = std::min(430.0f, w * 0.62f);
    const float bh = std::max(34.0f, h * 0.075f);
    const float bx = w * 0.5f - bw * 0.5f;
    float by = h * 0.32f;

    if (menuButton("SURVIVAL", bx, by, bw, bh))
    {
        m_player.creative = false;
        m_player.flying = false;
        m_screen = Screen::Playing;
        setMouseCaptured(true);
    }
    by += bh * 1.25f;
    if (menuButton("CREATIVE", bx, by, bw, bh))
    {
        m_player.creative = true;
        m_screen = Screen::Playing;
        setMouseCaptured(true);
    }
    by += bh * 1.25f;
    if (menuButton("BACK", bx, by, bw, bh)) m_screen = Screen::MainMenu;
}

void Application::renderSettingsMenu()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));

    const std::string title = "SETTINGS";
    m_ui.textWithShadow(title, w * 0.5f - UIRenderer::textWidth(title, 4.5f) * 0.5f, h * 0.10f, 4.5f, TEXT_COLOR);

    const float bw = std::min(430.0f, w * 0.62f);
    const float bh = std::max(32.0f, h * 0.07f);
    const float bx = w * 0.5f - bw * 0.5f;
    float by = h * 0.26f;

    const float stepW = bh;
    if (menuButton("-", bx - stepW - 6.0f, by, stepW, bh))
        world().setRenderDistance(std::max(2, world().renderDistance() - 1));
    if (menuButton("RENDER DISTANCE: " + std::to_string(world().renderDistance()), bx, by, bw, bh)) {}
    if (menuButton("+", bx + bw + 6.0f, by, stepW, bh))
        world().setRenderDistance(std::min(24, world().renderDistance() + 1));

    by += bh * 1.25f;
    if (menuButton(std::string("SOUND: ") + (m_sound.muted() ? "OFF" : "ON"), bx, by, bw, bh))
        m_sound.setMuted(!m_sound.muted());

    by += bh * 1.25f;
    if (menuButton(std::string("FANCY LIGHTING: ") + (m_pbrEnabled ? "ON" : "OFF"), bx, by, bw, bh))
        m_pbrEnabled = !m_pbrEnabled;

    by += bh * 1.25f;
    if (menuButton("CONTROLS", bx, by, bw, bh))
    {
        m_controlsOpen = true;
        m_rebindingAction = -1;
    }

    by += bh * 1.25f;
    if (menuButton("BACK", bx, by, bw, bh)) m_screen = Screen::MainMenu;
}

void Application::updateControlsScreen()
{
    // Capturing a rebind: the next key or button pressed becomes the binding,
    // except Escape, which backs out.
    if (m_rebindingAction >= 0)
    {
        const SDL_Scancode key = m_input.anyKeyPressed();
        const Uint8 button = m_input.anyMouseButtonPressed();

        if (key == SDL_SCANCODE_ESCAPE)
        {
            m_rebindingAction = -1;
        }
        else if (key != SDL_SCANCODE_UNKNOWN)
        {
            m_keybinds.set(static_cast<Keybinds::Action>(m_rebindingAction), Keybinds::Keyboard, key);
            m_keybinds.save(m_savePath + "/keybinds.txt");
            m_rebindingAction = -1;
        }
        else if (button != 0)
        {
            m_keybinds.set(static_cast<Keybinds::Action>(m_rebindingAction), Keybinds::Mouse, button);
            m_keybinds.save(m_savePath + "/keybinds.txt");
            m_rebindingAction = -1;
        }
        return;
    }

    if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        m_controlsOpen = false;
        setMouseCaptured(!uiHasFocus());
        return;
    }

    if (m_input.wasKeyPressed(SDL_SCANCODE_R))
    {
        m_keybinds.resetToDefaults();
        m_keybinds.save(m_savePath + "/keybinds.txt");
        return;
    }

    if (!m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT)) return;

    // Hit-test the rows, laid out the same way renderControlsScreen draws them.
    const float h = static_cast<float>(m_window.height());
    const float rowH = std::max(18.0f, h * 0.035f);
    const float top = h * 0.18f;
    const float my = static_cast<float>(m_input.mouseY());

    const int row = static_cast<int>((my - top) / rowH);
    if (row >= 0 && row < Keybinds::Count)
        m_rebindingAction = row;
}


void Application::renderConsole()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());
    const float scale = std::max(1.2f, h / 300.0f);
    const float lineH = UIRenderer::textHeight(scale) + 4.0f;

    const std::vector<const Console::Line*> lines = m_console.recentLines();
    const float inputY = h - lineH - 46.0f;

    float y = inputY - lineH * static_cast<float>(lines.size()) - 6.0f;
    for (const Console::Line* line : lines)
    {
        // Messages fade once they are old, but only while the console is shut;
        // with it open the whole scrollback stays solid so it can be read.
        float alpha = 1.0f;
        if (!m_console.open())
        {
            const float remaining = Console::LINE_FADE_SECONDS - line->age;
            alpha = std::max(0.0f, std::min(1.0f, remaining));
        }
        if (alpha <= 0.0f) { y += lineH; continue; }

        m_ui.quad(6.0f, y - 2.0f,
                  UIRenderer::textWidth(line->text, scale) + 10.0f, lineH,
                  glm::vec4(0.0f, 0.0f, 0.0f, 0.45f * alpha));
        m_ui.textWithShadow(line->text, 10.0f, y, scale,
                            line->error ? glm::vec4(1.0f, 0.45f, 0.4f, alpha)
                                        : glm::vec4(1.0f, 1.0f, 1.0f, alpha));
        y += lineH;
    }

    if (!m_console.open()) return;

    m_ui.quad(0.0f, inputY - 4.0f, w, lineH + 6.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.65f));

    // A block cursor that blinks, so an empty input still looks like a prompt.
    const bool caret = std::fmod(m_elapsedSeconds, 1.0f) < 0.5f;
    const std::string shown = m_console.input() + (caret ? "_" : "");
    m_ui.textWithShadow(shown, 10.0f, inputY, scale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void Application::renderControlsScreen()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.82f));

    const std::string title = "CONTROLS";
    m_ui.textWithShadow(title, w * 0.5f - UIRenderer::textWidth(title, 4.0f) * 0.5f, h * 0.07f, 4.0f, TEXT_COLOR);

    const float rowH = std::max(18.0f, h * 0.035f);
    const float top = h * 0.18f;
    const float scale = std::max(1.0f, rowH / 14.0f);
    const float labelX = w * 0.18f;
    const float valueRight = w * 0.82f;

    for (int i = 0; i < Keybinds::Count; ++i)
    {
        const float y = top + i * rowH;
        const bool capturing = (m_rebindingAction == i);

        const float my = static_cast<float>(m_input.mouseY());
        const bool hovered = (my >= y && my < y + rowH);
        if (capturing || hovered)
            m_ui.quad(labelX - 8.0f, y, valueRight - labelX + 16.0f, rowH - 2.0f,
                      glm::vec4(1.0f, 1.0f, 1.0f, capturing ? 0.22f : 0.08f));

        const auto action = static_cast<Keybinds::Action>(i);
        m_ui.textWithShadow(Keybinds::actionLabel(action), labelX, y + 2.0f, scale, TEXT_COLOR);

        const std::string value = capturing ? "PRESS A KEY" : m_keybinds.bindingLabel(action);
        const glm::vec4 colour = capturing ? glm::vec4(1.0f, 0.85f, 0.3f, 1.0f)
                                           : glm::vec4(1.0f, 1.0f, 0.6f, 1.0f);
        m_ui.textWithShadow(value, valueRight - UIRenderer::textWidth(value, scale), y + 2.0f, scale, colour);
    }

    const std::string hint = "CLICK A ROW TO REBIND   R RESET DEFAULTS   ESC DONE";
    m_ui.textWithShadow(hint, w * 0.5f - UIRenderer::textWidth(hint, 1.4f) * 0.5f, h * 0.93f, 1.4f,
                        glm::vec4(0.7f, 0.75f, 0.8f, 1.0f));
}

void Application::renderPauseMenu()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));

    const std::string title = "PAUSED";
    m_ui.textWithShadow(title, w * 0.5f - UIRenderer::textWidth(title, 5.0f) * 0.5f, h * 0.35f, 5.0f, TEXT_COLOR);

    const char* lines[] = {
        "ESC  RESUME",
        "C    CONTROLS",
        "Q    SAVE AND QUIT",
        "",
        "WASD MOVE   SPACE JUMP   SHIFT SNEAK   CTRL SPRINT",
        "LMB MINE   RMB PLACE   MMB PICK   E INVENTORY",
        "F FLY   G CREATIVE   F3 DEBUG   [ ] RENDER DISTANCE",
    };

    float y = h * 0.48f;
    for (const char* line : lines)
    {
        const std::string text = line;
        if (!text.empty())
            m_ui.textWithShadow(text, w * 0.5f - UIRenderer::textWidth(text, 2.0f) * 0.5f, y, 2.0f, TEXT_COLOR);
        y += 28.0f;
    }
}

void Application::renderDebugOverlay()
{
    const glm::vec3 p = m_player.position;
    const int bx = static_cast<int>(std::floor(p.x));
    const int by = static_cast<int>(std::floor(p.y));
    const int bz = static_cast<int>(std::floor(p.z));

    char buffer[256];
    float y = 10.0f;
    const float scale = 2.0f;
    auto line = [&](const std::string& text) {
        m_ui.textWithShadow(text, 10.0f, y, scale, TEXT_COLOR);
        y += UIRenderer::textHeight(scale) + 6.0f;
    };

    std::snprintf(buffer, sizeof(buffer), "FPS %.0f", m_fps);
    line(buffer);

    std::snprintf(buffer, sizeof(buffer), "XYZ %.1f / %.1f / %.1f", p.x, p.y, p.z);
    line(buffer);

    std::snprintf(buffer, sizeof(buffer), "CHUNK %d %d", World::floorDiv(bx, Chunk::SX), World::floorDiv(bz, Chunk::SZ));
    line(buffer);

    // Biomes only mean anything in the overworld; elsewhere name the dimension.
    if (m_dimension == Dimension::Overworld)
        std::snprintf(buffer, sizeof(buffer), "BIOME %s", biomeName(world().generator().biomeAt(bx, bz)));
    else
        std::snprintf(buffer, sizeof(buffer), "DIMENSION: %s", dimensionName(m_dimension));
    line(buffer);

    std::snprintf(buffer, sizeof(buffer), "LIGHT SKY %d BLOCK %d",
                  static_cast<int>(world().skyLight(bx, by + 1, bz)),
                  static_cast<int>(world().blockLightAt(bx, by + 1, bz)));
    line(buffer);

    std::snprintf(buffer, sizeof(buffer), "CHUNKS %d  JOBS %d  DIST %d",
                  world().loadedChunks(), world().pendingJobs(), world().renderDistance());
    line(buffer);

    const int hours = static_cast<int>(m_timeOfDay * 24.0f);
    const int minutes = static_cast<int>((m_timeOfDay * 24.0f - hours) * 60.0f);
    std::snprintf(buffer, sizeof(buffer), "TIME %02d:%02d  LIGHT %.2f", hours, minutes, daylightFactor());
    line(buffer);

    std::snprintf(buffer, sizeof(buffer), "MODE %s%s",
                  m_player.creative ? "CREATIVE" : "SURVIVAL",
                  m_player.flying ? " FLYING" : (m_player.onGround ? " GROUNDED" : " AIRBORNE"));
    line(buffer);
}
