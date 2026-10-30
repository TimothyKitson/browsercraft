#include "Projectiles.h"
#include "World/World.h"
#include "Renderer/Atlas.h"
#include "Renderer/AtlasTiles.h"
#include "Renderer/Camera.h"
#include "Renderer/Shader.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };
}

float Arrows::drawFraction(float heldSeconds)
{
    return std::clamp(heldSeconds / DRAW_SECONDS, 0.0f, 1.0f);
}

float Arrows::speedFor(float drawnFraction)
{
    const float t = std::clamp(drawnFraction, 0.0f, 1.0f);
    return MIN_SPEED + (MAX_SPEED - MIN_SPEED) * t * t;
}

int Arrows::damageFor(float drawnFraction)
{
    const float t = std::clamp(drawnFraction, 0.0f, 1.0f);
    // One to six, the way Minecraft's bow runs.
    return std::max(1, static_cast<int>(std::lround(1.0f + 5.0f * t * t)));
}

void Projectiles::spawn(const glm::vec3& from, const glm::vec3& velocity,
                        int damage, bool fromPlayer, Arrows::Kind kind)
{
    Arrow arrow;
    arrow.position = from;
    arrow.velocity = velocity;
    arrow.damage = damage;
    arrow.fromPlayer = fromPlayer;
    arrow.kind = kind;
    m_arrows.push_back(arrow);
}

// The shove, falling off to nothing at the edge of the burst and never
// pushing straight down: a charge under your own feet is meant to throw
// you into the air, which is most of what anybody uses one for.
glm::vec3 Arrows::windImpulse(const glm::vec3& burst, const glm::vec3& target)
{
    glm::vec3 away = target - burst;
    const float distance = glm::length(away);
    if (distance > WIND_RADIUS) return glm::vec3(0.0f);

    if (distance < 0.001f) away = glm::vec3(0.0f, 1.0f, 0.0f);
    else away /= distance;

    away.y = std::max(away.y, 0.35f);
    away = glm::normalize(away);

    const float strength = WIND_PUSH * (1.0f - distance / WIND_RADIUS);
    return away * strength;
}

namespace
{
    // The real world, behind the one question the arrows ask of it.
    struct WorldBlocks : Projectiles::Blocks
    {
        const World* world = nullptr;
        bool solidAt(int x, int y, int z) const override
        {
            return isSolid(world->getBlock(x, y, z));
        }
    };
}

void Projectiles::update(float deltaTime, const World& world, Targets& targets)
{
    WorldBlocks blocks;
    blocks.world = &world;
    update(deltaTime, blocks, targets);
}

void Projectiles::update(float deltaTime, const Blocks& blocks, Targets& targets)
{
    for (size_t i = 0; i < m_arrows.size();)
    {
        Arrow& arrow = m_arrows[i];
        arrow.age += deltaTime;

        if (arrow.kind == Arrows::Kind::Eye && arrow.age > Arrows::EYE_LIFETIME)
        {
            m_arrows[i] = m_arrows.back();
            m_arrows.pop_back();
            continue;
        }

        if (arrow.age > Arrows::LIFETIME)
        {
            m_arrows[i] = m_arrows.back();
            m_arrows.pop_back();
            continue;
        }

        if (arrow.stuck) { ++i; continue; }

        // A charge flies flat: it is thrown, not loosed, and it is
        // spent long before it would have dropped.
        if (!arrow.flies()) arrow.velocity.y += Arrows::GRAVITY * deltaTime;

        // Stepped along its path rather than moved in one go: a fast
        // arrow covers most of a block per frame, and a single test
        // would shoot it through a wall.
        const glm::vec3 step = arrow.velocity * deltaTime;
        const float distance = glm::length(step);
        const int steps = std::max(1, static_cast<int>(distance / 0.25f) + 1);
        const glm::vec3 slice = step / static_cast<float>(steps);

        bool done = false;

        for (int s = 0; s < steps && !done; ++s)
        {
            const glm::vec3 next = arrow.position + slice;

            // Anything living in the way. Whoever owns the arrows
            // answers this, and an arrow never hits the one who loosed
            // it -- that is what the flag is for.
            if (targets.strike(next, arrow.damage, arrow.fromPlayer, arrow.kind))
            {
                if (arrow.bursts()) targets.burst(next, arrow.kind, arrow.fromPlayer);
                done = true;
                break;
            }

            if (!arrow.ghost() &&
                blocks.solidAt(static_cast<int>(std::floor(next.x)),
                               static_cast<int>(std::floor(next.y)),
                               static_cast<int>(std::floor(next.z))))
            {
                // A charge bursts rather than sticking: nothing is left
                // in the wall to look at. So does a potion, which is
                // the whole of what a potion is for.
                if (arrow.bursts())
                {
                    targets.burst(next - slice * 0.5f, arrow.kind, arrow.fromPlayer);
                    done = true;
                    break;
                }

                arrow.stuck = true;
                arrow.velocity = glm::vec3(0.0f);
                // Backed off the face it struck so it is not buried.
                arrow.position = next - slice * 0.5f;
                done = true;
                break;
            }

            arrow.position = next;
        }

        if (done && !arrow.stuck)
        {
            m_arrows[i] = m_arrows.back();
            m_arrows.pop_back();
            continue;
        }

        // An arrow stuck in a wall is worth looking at for a moment, not
        // for ever.
        if (arrow.stuck && arrow.age > 12.0f)
        {
            m_arrows[i] = m_arrows.back();
            m_arrows.pop_back();
            continue;
        }

        ++i;
    }
}

// An arrow is drawn as a cross of two quads turned to lie along its own
// flight, so it reads as a thing with a point and a direction rather
// than a floating card. A stuck one keeps the heading it struck with.
void Projectiles::render(Shader& shader, const Camera& camera, const Atlas& atlas,
                         const World& world)
{
    (void)atlas;
    if (m_arrows.empty()) return;

    m_vertices.clear();

    const TileUV arrowUV = tileUV(Tiles::ArrowInFlight);
    const TileUV fireUV = tileUV(Tiles::FireCharge);
    const TileUV windUV = tileUV(Tiles::WindCharge);
    const TileUV eyeUV = tileUV(Tiles::ItemEyeOfEnder);
    const TileUV potionUV = tileUV(Tiles::ItemPotion);
    constexpr float LENGTH = 0.62f;
    constexpr float WIDTH = 0.16f;

    for (const Arrow& arrow : m_arrows)
    {
        // Along its flight, or along the way it was last going.
        glm::vec3 along = arrow.velocity;
        if (glm::length(along) < 0.001f) along = glm::vec3(0.0f, -1.0f, 0.0f);
        along = glm::normalize(along);

        // Two perpendiculars, so the cross stands up whatever the angle.
        const glm::vec3 reference = std::abs(along.y) > 0.95f ? glm::vec3(1.0f, 0.0f, 0.0f)
                                                             : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 side = glm::normalize(glm::cross(along, reference));
        const glm::vec3 up = glm::normalize(glm::cross(side, along));

        const int bx = static_cast<int>(std::floor(arrow.position.x));
        const int by = static_cast<int>(std::floor(arrow.position.y));
        const int bz = static_cast<int>(std::floor(arrow.position.z));
        const float sky = world.skyLight(bx, by, bz) / 15.0f;
        // A charge is its own light: drawn at the Nether's gloom it
        // would be a dull smudge, and the one thing it has to do is be
        // seen coming.
        const bool charge = arrow.flies();
        const float blockLight = charge ? 1.0f
                                        : world.blockLightAt(bx, by, bz) / 15.0f;
        const TileUV uv = (arrow.kind == Arrows::Kind::Fire) ? fireUV
                        : (arrow.kind == Arrows::Kind::Wind) ? windUV
                        : (arrow.kind == Arrows::Kind::Eye)  ? eyeUV
                        : (arrow.kind == Arrows::Kind::Potion) ? potionUV
                                                             : arrowUV;
        // A potion is drawn as the bottle it is rather than as a
        // shaft, so it is square like a charge without being one.
        const bool bottle = arrow.kind == Arrows::Kind::Potion;
        const float length = (charge || bottle) ? 0.34f : LENGTH;
        const float width = (charge || bottle) ? 0.34f : WIDTH;

        auto push = [&](const glm::vec3& p, float tu, float tv, float shade) {
            m_vertices.push_back(p.x);
            m_vertices.push_back(p.y);
            m_vertices.push_back(p.z);
            m_vertices.push_back(tu);
            m_vertices.push_back(tv);
            m_vertices.push_back(shade);
            m_vertices.push_back(sky);
            m_vertices.push_back(blockLight);
            m_vertices.push_back(6.0f);   // face 6: no tangent-space normal mapping
        };

        const glm::vec3 head = arrow.position + along * (length * 0.5f);
        const glm::vec3 tail = arrow.position - along * (length * 0.5f);

        for (int plane = 0; plane < 2; ++plane)
        {
            const glm::vec3 across = (plane == 0 ? side : up) * (width * 0.5f);

            const glm::vec3 corner[4] = {
                tail - across, head - across, head + across, tail + across
            };
            const float tu[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
            const float tv[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };

            const int front[6] = { 0, 1, 2, 0, 2, 3 };
            const int back[6]  = { 0, 2, 1, 0, 3, 2 };
            for (int k : front) push(corner[k], tu[k], tv[k], 1.0f);
            for (int k : back)  push(corner[k], tu[k], tv[k], 0.82f);
        }
    }

    if (m_vertices.empty()) return;

    (void)camera;
    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    m_mesh.draw(GL_TRIANGLES);
}
