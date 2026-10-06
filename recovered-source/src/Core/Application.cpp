#include "Application.h"
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
    , m_world(0, "world", DEFAULT_RENDER_DISTANCE)
    , m_camera(glm::vec3(0.0f), -90.0f, 0.0f)
    , m_player(glm::vec3(0.0f, 80.0f, 0.0f))
    , m_savePath("world")
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    m_keybinds.load(m_savePath + "/keybinds.txt");

    loadLevel();
    setMouseCaptured(true);

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
    std::printf("1-9 / scroll hotbar | %s inventory | %s fly | G creative\n",
                m_keybinds.bindingLabel(Keybinds::Inventory).c_str(),
                m_keybinds.bindingLabel(Keybinds::Fly).c_str());
    std::printf("%s debug | %s %s render distance | Esc menu\n\n",
                m_keybinds.bindingLabel(Keybinds::Debug).c_str(),
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
    m_world.generator().findSpawn(spawnX, spawnY, spawnZ);
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
        // A small starter kit so there's something to build with right away.
        m_inventory.add(Blocks::Planks, 64);
        m_inventory.add(Blocks::Cobblestone, 64);
        m_inventory.add(Blocks::Torch, 32);
        m_inventory.add(Blocks::Glass, 32);
        std::printf("Created a new world (seed %u)\n", m_world.seed());
    }

    m_camera.position = m_player.eyePosition();
}

void Application::saveLevel()
{
    LevelState state;
    state.seed = m_world.seed();
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
    m_world.saveAll();
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

void Application::run()
{
    Uint64 previousTicks = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    while (m_running)
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        float deltaTime = static_cast<float>((now - previousTicks) / frequency);
        previousTicks = now;
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
        if (!m_running) break;

        if (!uiHasFocus())
            updateGameplay(deltaTime);

        m_timeOfDay += deltaTime / DAY_LENGTH_SECONDS;
        if (m_timeOfDay >= 1.0f) m_timeOfDay -= 1.0f;

        m_world.update(m_player.position);

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

    if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        if (m_inventoryOpen) m_inventoryOpen = false;
        else m_paused = !m_paused;
        setMouseCaptured(!uiHasFocus());
    }

    if (m_paused && pressed(Keybinds::Drop)) m_running = false;

    if (pressed(Keybinds::Inventory))
    {
        m_inventoryOpen = !m_inventoryOpen;
        m_paused = false;
        setMouseCaptured(!uiHasFocus());
    }

    if (pressed(Keybinds::Debug)) m_showDebug = !m_showDebug;

    if (m_player.health <= 0 && m_input.wasKeyPressed(SDL_SCANCODE_R))
    {
        m_player.respawn(m_spawnPoint);
        setMouseCaptured(true);
    }

    if (pressed(Keybinds::DistanceDown))
        m_world.setRenderDistance(m_world.renderDistance() - 1);
    if (pressed(Keybinds::DistanceUp))
        m_world.setRenderDistance(m_world.renderDistance() + 1);

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

void Application::updateGameplay(float deltaTime)
{
    // --- mobs ---
    m_entities.update(deltaTime, m_world, m_player.position, daylightFactor());

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
            const BlockId ground = m_world.getBlock(static_cast<int>(std::floor(feet.x)),
                                                    static_cast<int>(std::floor(feet.y - 0.2f)),
                                                    static_cast<int>(std::floor(feet.z)));
            if (const char* group = blockSoundGroup(ground))
                m_sound.play(std::string("step/") + group, blockStepVariants(group), feet, 0.22f, 0.9f + 0.2f * (m_elapsedSeconds - std::floor(m_elapsedSeconds)));
        }
    }

    // --- look ---
    m_camera.addLook(m_input.mouseDeltaX(), m_input.mouseDeltaY(), MOUSE_SENSITIVITY);

    // --- mode toggles ---
    if (pressed(Keybinds::Fly)) m_player.flying = !m_player.flying;
    if (pressed(Keybinds::Pbr)) m_pbrEnabled = !m_pbrEnabled;
    if (m_input.wasKeyPressed(SDL_SCANCODE_G))
    {
        m_player.creative = !m_player.creative;
        if (!m_player.creative) m_player.flying = false;
    }

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

    m_player.update(deltaTime, m_world, controls);
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
    const RaycastHit hit = m_world.raycast(m_camera.position, m_camera.front, REACH_DISTANCE);

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

    const BlockId id = m_world.getBlock(hit.block.x, hit.block.y, hit.block.z);
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
        m_world.setBlock(hit.block.x, hit.block.y, hit.block.z, Blocks::Air);
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
        const RaycastHit hit = m_world.raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
        if (hit.hit)
        {
            const BlockId picked = m_world.getBlock(hit.block.x, hit.block.y, hit.block.z);
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

    const RaycastHit hit = m_world.raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
    if (!hit.hit) return;

    const glm::ivec3 target = hit.previous;
    const BlockId existing = m_world.getBlock(target.x, target.y, target.z);
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
    m_world.setBlock(target.x, target.y, target.z, stack.id);

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
    if (m_player.health > 0) renderHud();
    if (m_showDebug) renderDebugOverlay();
    if (m_inventoryOpen) renderInventoryScreen();
    if (m_paused) renderPauseMenu();
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

    const bool underwater = m_player.isHeadUnderwater(m_world);
    const glm::vec3 fogColor = SkyRenderer::horizonColor(daylight, sun);
    const float viewDistance = static_cast<float>(m_world.renderDistance() * Chunk::SX);

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
    visible.reserve(m_world.chunks().size());

    for (const auto& [pos, chunk] : m_world.chunks())
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
    m_mobRenderer.render(m_entities, m_world, m_chunkShader);

    // Selection outline + mining cracks sit between the two passes so water
    // still blends over them correctly.
    if (!uiHasFocus())
    {
        const RaycastHit hit = m_world.raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
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

        const TileUV uv = tileUV(blockInfo(stack.id).tileTop);
        m_ui.texturedQuad(m_atlas.textureId(), slotX + 5.0f, barY + 5.0f, SLOT_SIZE - 10.0f, SLOT_SIZE - 10.0f,
                          uv.u0, uv.vTop, uv.u1, uv.vBottom, glm::vec4(1.0f));

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

        const TileUV uv = tileUV(blockInfo(palette[i]).tileTop);
        m_ui.texturedQuad(m_atlas.textureId(), slotX + 5.0f, slotY + 5.0f, SLOT_SIZE - 10.0f, SLOT_SIZE - 10.0f,
                          uv.u0, uv.vTop, uv.u1, uv.vBottom, glm::vec4(1.0f));

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

void Application::renderPauseMenu()
{
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));

    const std::string title = "PAUSED";
    m_ui.textWithShadow(title, w * 0.5f - UIRenderer::textWidth(title, 5.0f) * 0.5f, h * 0.35f, 5.0f, TEXT_COLOR);

    const char* lines[] = {
        "ESC  RESUME",
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

    std::snprintf(buffer, sizeof(buffer), "BIOME %s", biomeName(m_world.generator().biomeAt(bx, bz)));
    line(buffer);

    std::snprintf(buffer, sizeof(buffer), "LIGHT SKY %d BLOCK %d",
                  static_cast<int>(m_world.skyLight(bx, by + 1, bz)),
                  static_cast<int>(m_world.blockLightAt(bx, by + 1, bz)));
    line(buffer);

    std::snprintf(buffer, sizeof(buffer), "CHUNKS %d  JOBS %d  DIST %d",
                  m_world.loadedChunks(), m_world.pendingJobs(), m_world.renderDistance());
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
