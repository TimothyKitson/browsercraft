#include "ExperienceOrbs.h"
#include "Game/Experience.h"
#include "World/World.h"
#include "World/Noise.h"
#include "Player/Player.h"
#include "Audio/AudioEngine.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Renderer/AtlasTiles.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float GRAVITY = -14.0f;
    constexpr float BOUNCE_DAMPING = 0.3f;
    constexpr float GROUND_FRICTION = 4.0f;
    constexpr float BOB_HEIGHT = 0.07f;
    constexpr float SPIN_SPEED = 2.2f;

    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    // Orbs are bigger the more they carry, so a stray point and a lump
    // worth a whole level do not look alike. Minecraft does the same;
    // without it a floor strewn with ones is indistinguishable from the
    // one orb actually worth picking up.
    float sizeFor(int worth)
    {
        if (worth >= 307) return ExperienceOrbs::SIZE * 1.7f;
        if (worth >= 73) return ExperienceOrbs::SIZE * 1.45f;
        if (worth >= 17) return ExperienceOrbs::SIZE * 1.25f;
        if (worth >= 7) return ExperienceOrbs::SIZE * 1.1f;
        return ExperienceOrbs::SIZE;
    }
}

void ExperienceOrbs::spawnOne(const glm::vec3& position, int worth)
{
    if (worth <= 0) return;
    if (m_orbs.size() > 600) return; // safety valve

    Orb orb;
    orb.position = position;
    orb.worth = worth;

    const uint32_t h = hashCoords(static_cast<int>(position.x * 16.0f),
                                  static_cast<int>(position.y * 16.0f),
                                  static_cast<int>(position.z * 16.0f) +
                                      static_cast<int>(m_orbs.size()),
                                  777u);
    orb.velocity = glm::vec3((hashToFloat(h) - 0.5f) * 2.4f,
                             1.6f + hashToFloat(h >> 8) * 1.4f,
                             (hashToFloat(h >> 16) - 0.5f) * 2.4f);
    orb.bob = hashToFloat(h >> 4) * 6.2831853f;

    m_orbs.push_back(orb);
}

void ExperienceOrbs::spawnValue(const glm::vec3& position, int worth)
{
    for (int one : Experience::splitIntoOrbs(worth))
        spawnOne(position, one);
}

int ExperienceOrbs::totalWorth() const
{
    int total = 0;
    for (const Orb& orb : m_orbs) total += orb.worth;
    return total;
}

bool ExperienceOrbs::blocked(const World& world, const glm::vec3& centre) const
{
    const float half = SIZE * 0.5f;
    const int minX = static_cast<int>(std::floor(centre.x - half));
    const int maxX = static_cast<int>(std::floor(centre.x + half));
    const int minY = static_cast<int>(std::floor(centre.y - half));
    const int maxY = static_cast<int>(std::floor(centre.y + half));
    const int minZ = static_cast<int>(std::floor(centre.z - half));
    const int maxZ = static_cast<int>(std::floor(centre.z + half));

    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                if (isSolid(world.getBlock(x, y, z))) return true;

    return false;
}

int ExperienceOrbs::update(float deltaTime, const World& world, const Player& player,
                           AudioEngine& audio)
{
    // Aimed at the chest rather than the feet: an orb that homes on
    // where you stand dives into the floor on the way to you.
    const glm::vec3 chest = player.position + glm::vec3(0.0f, Player::HEIGHT * 0.5f, 0.0f);

    int collected = 0;
    bool chimed = false;

    for (size_t i = 0; i < m_orbs.size();)
    {
        Orb& orb = m_orbs[i];
        orb.age += deltaTime;

        const glm::vec3 toPlayer = chest - orb.position;
        const float distance = glm::length(toPlayer);

        // The short delay stops a kill's own orbs being swallowed before
        // they have visibly scattered, which otherwise reads as no drop
        // at all.
        const bool homing = orb.age > SPAWN_DELAY && distance < ATTRACT_RADIUS;

        if (homing && distance < COLLECT_RADIUS)
        {
            collected += orb.worth;
            if (!chimed)
            {
                chimed = true;
                // Pitch rises with the size of the lump, so a big orb
                // sounds like one.
                audio.play(Sound::Pickup, 0.5f, 1.2f + 0.05f * static_cast<float>(i % 5));
            }
            m_orbs[i] = m_orbs.back();
            m_orbs.pop_back();
            continue;
        }

        if (orb.age > LIFETIME_SECONDS)
        {
            m_orbs[i] = m_orbs.back();
            m_orbs.pop_back();
            continue;
        }

        if (homing)
        {
            // Pulled harder the closer it gets, and flown straight
            // through blocks on the way in. An orb that collided while
            // homing would snag on the lip of the hole you mined it
            // out of and never arrive.
            const glm::vec3 pull = toPlayer / std::max(distance, 0.001f);
            const float eagerness = 1.0f - distance / ATTRACT_RADIUS;
            orb.velocity += pull * ATTRACT_SPEED * (0.4f + eagerness) * deltaTime;
            orb.velocity *= std::max(0.0f, 1.0f - 2.0f * deltaTime);
            orb.position += orb.velocity * deltaTime;
            ++i;
            continue;
        }

        orb.velocity.y += GRAVITY * deltaTime;

        glm::vec3 next = orb.position;

        next.x += orb.velocity.x * deltaTime;
        if (blocked(world, next)) { next.x = orb.position.x; orb.velocity.x = 0.0f; }

        next.z += orb.velocity.z * deltaTime;
        if (blocked(world, glm::vec3(next.x, orb.position.y, next.z)))
        {
            next.z = orb.position.z;
            orb.velocity.z = 0.0f;
        }

        next.y += orb.velocity.y * deltaTime;
        if (blocked(world, next))
        {
            next.y = orb.position.y;
            orb.velocity.y *= (orb.velocity.y < 0.0f) ? -BOUNCE_DAMPING : 0.0f;
        }

        orb.position = next;

        const float friction = std::clamp(GROUND_FRICTION * deltaTime, 0.0f, 1.0f);
        orb.velocity.x -= orb.velocity.x * friction;
        orb.velocity.z -= orb.velocity.z * friction;

        ++i;
    }

    mergeNearby();
    return collected;
}

// Orbs that come to rest together become one worth both, which keeps a
// big kill from leaving a drift of motes that each cost a draw call and
// a pickup sound. Two already flying at you merge as readily, which
// costs nothing: the worth is carried over either way, and the pair
// were a moment from being collected together anyway.
void ExperienceOrbs::mergeNearby()
{
    for (size_t a = 0; a < m_orbs.size(); ++a)
    {
        for (size_t b = a + 1; b < m_orbs.size();)
        {
            if (glm::length(m_orbs[b].position - m_orbs[a].position) > MERGE_RADIUS)
            {
                ++b;
                continue;
            }

            m_orbs[a].worth += m_orbs[b].worth;
            m_orbs[a].age = std::max(m_orbs[a].age, m_orbs[b].age);
            m_orbs[b] = m_orbs.back();
            m_orbs.pop_back();
        }
    }
}

void ExperienceOrbs::render(Shader& chunkShader, const World& world)
{
    if (m_orbs.empty()) return;

    m_vertices.clear();
    m_vertices.reserve(m_orbs.size() * 12 * 9);

    const TileUV uv = tileUV(Tiles::ExperienceOrb);

    for (const Orb& orb : m_orbs)
    {
        const float half = sizeFor(orb.worth) * 0.5f;
        const float bob = std::sin(orb.age * 3.0f + orb.bob) * BOB_HEIGHT;
        const glm::vec3 centre = orb.position + glm::vec3(0.0f, half + bob, 0.0f);

        const float spin = orb.age * SPIN_SPEED + orb.bob;
        const float s = std::sin(spin);
        const float c = std::cos(spin);

        int lx = static_cast<int>(std::floor(centre.x));
        int ly = static_cast<int>(std::floor(centre.y));
        int lz = static_cast<int>(std::floor(centre.z));
        for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step)
            ++ly;

        const float sky = world.skyLight(lx, ly, lz) / 15.0f;

        // Lit as if it glowed: an orb in a dark cave is the one thing
        // you are meant to be able to see down there.
        const float blockLight = std::max(world.blockLightAt(lx, ly, lz) / 15.0f, 0.75f);

        auto push = [&](const glm::vec3& p, float tu, float tv, float shade) {
            m_vertices.push_back(p.x);
            m_vertices.push_back(p.y);
            m_vertices.push_back(p.z);
            m_vertices.push_back(tu);
            m_vertices.push_back(tv);
            m_vertices.push_back(shade);
            m_vertices.push_back(sky);
            m_vertices.push_back(blockLight);
            m_vertices.push_back(6.0f);
        };

        // Two quads crossed at right angles, so the orb reads as round
        // from any side without needing the camera to billboard it.
        for (int pass = 0; pass < 2; ++pass)
        {
            const float ps = (pass == 0) ? s : c;
            const float pc = (pass == 0) ? c : -s;
            const glm::vec3 across(half * pc, 0.0f, half * ps);
            const glm::vec3 up(0.0f, half, 0.0f);

            const glm::vec3 corner[4] = {
                centre - across - up, centre + across - up,
                centre + across + up, centre - across + up
            };
            const float tu[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
            const float tv[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };

            const int front[6] = { 0, 1, 2, 0, 2, 3 };
            const int back[6]  = { 0, 2, 1, 0, 3, 2 };
            for (int k : front) push(corner[k], tu[k], tv[k], 1.0f);
            for (int k : back)  push(corner[k], tu[k], tv[k], 1.0f);
        }
    }

    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));
    m_mesh.draw(GL_TRIANGLES);
}
