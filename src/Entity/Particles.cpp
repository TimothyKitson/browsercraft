#include "Particles.h"
#include "World/World.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float GRAVITY = -16.0f;
    constexpr float DRAG = 1.4f;
    constexpr size_t MAX_PARTICLES = 1500;

    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };
}

float Particles::nextRandom()
{
    m_random ^= m_random << 13;
    m_random ^= m_random >> 17;
    m_random ^= m_random << 5;
    return static_cast<float>(m_random & 0xFFFFFF) / static_cast<float>(0x1000000);
}

void Particles::add(const glm::vec3& position, const glm::vec3& velocity, BlockId block,
                    float size, float lifetime)
{
    if (m_particles.size() >= MAX_PARTICLES) return;

    const BlockInfo& info = blockInfo(block);
    const TileUV tile = tileUV(info.tileSide);

    // Take a quarter-tile patch so each particle shows a different part of
    // the texture, the way Minecraft's break particles do.
    const float patch = 0.25f;
    const float offsetU = std::floor(nextRandom() * 4.0f) * patch;
    const float offsetV = std::floor(nextRandom() * 4.0f) * patch;
    const float tileWidth = tile.u1 - tile.u0;
    const float tileHeight = tile.vBottom - tile.vTop;

    Particle particle;
    particle.position = position;
    particle.velocity = velocity;
    particle.size = size;
    particle.lifetime = lifetime;
    particle.u0 = tile.u0 + offsetU * tileWidth;
    particle.u1 = particle.u0 + patch * tileWidth;
    particle.v0 = tile.vTop + offsetV * tileHeight;
    particle.v1 = particle.v0 + patch * tileHeight;

    m_particles.push_back(particle);
}

void Particles::spawnBlockBreak(const glm::ivec3& blockPosition, BlockId block)
{
    if (!isVisible(block)) return;

    const glm::vec3 base(blockPosition);
    for (int i = 0; i < 28; ++i)
    {
        const glm::vec3 position = base + glm::vec3(nextRandom(), nextRandom(), nextRandom());
        const glm::vec3 velocity((nextRandom() - 0.5f) * 3.4f,
                                 nextRandom() * 3.6f + 0.6f,
                                 (nextRandom() - 0.5f) * 3.4f);
        add(position, velocity, block, 0.10f + nextRandom() * 0.07f, 0.7f + nextRandom() * 0.5f);
    }
}

void Particles::spawnBlockHit(const glm::vec3& position, const glm::ivec3& faceNormal, BlockId block)
{
    if (!isVisible(block)) return;

    const glm::vec3 normal(faceNormal);
    for (int i = 0; i < 4; ++i)
    {
        // Spray outwards from the face being hit.
        const glm::vec3 jitter((nextRandom() - 0.5f) * 0.9f,
                               (nextRandom() - 0.5f) * 0.9f,
                               (nextRandom() - 0.5f) * 0.9f);
        const glm::vec3 velocity = normal * (1.4f + nextRandom()) + jitter;
        add(position + normal * 0.06f, velocity, block, 0.05f + nextRandom() * 0.04f,
            0.35f + nextRandom() * 0.3f);
    }
}

void Particles::spawnLandingPuff(const glm::vec3& feetPosition, BlockId ground)
{
    if (!isVisible(ground)) return;

    for (int i = 0; i < 10; ++i)
    {
        const float angle = nextRandom() * 6.2831853f;
        const float speed = 0.8f + nextRandom() * 1.4f;
        const glm::vec3 velocity(std::cos(angle) * speed, nextRandom() * 0.7f, std::sin(angle) * speed);
        add(feetPosition + glm::vec3((nextRandom() - 0.5f) * 0.5f, 0.06f, (nextRandom() - 0.5f) * 0.5f),
            velocity, ground, 0.05f + nextRandom() * 0.04f, 0.3f + nextRandom() * 0.3f);
    }
}

void Particles::update(float deltaTime, const World& world)
{
    for (size_t i = 0; i < m_particles.size();)
    {
        Particle& particle = m_particles[i];
        particle.age += deltaTime;

        if (particle.age >= particle.lifetime)
        {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        if (!particle.resting)
        {
            particle.velocity.y += GRAVITY * deltaTime;

            const float drag = std::clamp(DRAG * deltaTime, 0.0f, 1.0f);
            particle.velocity.x -= particle.velocity.x * drag;
            particle.velocity.z -= particle.velocity.z * drag;

            glm::vec3 next = particle.position + particle.velocity * deltaTime;

            // Particles are points: one block test is enough, and it keeps
            // a few hundred of them cheap.
            const bool hitSomething = isSolid(world.getBlock(
                static_cast<int>(std::floor(next.x)),
                static_cast<int>(std::floor(next.y)),
                static_cast<int>(std::floor(next.z))));

            if (hitSomething)
            {
                // Slide along the surface instead of sinking into it.
                const bool verticalBlocked = isSolid(world.getBlock(
                    static_cast<int>(std::floor(particle.position.x)),
                    static_cast<int>(std::floor(next.y)),
                    static_cast<int>(std::floor(particle.position.z))));

                if (verticalBlocked)
                {
                    particle.velocity.y = 0.0f;
                    next.y = particle.position.y;
                    particle.velocity.x *= 0.6f;
                    particle.velocity.z *= 0.6f;
                }
                else
                {
                    particle.velocity.x = 0.0f;
                    particle.velocity.z = 0.0f;
                    next.x = particle.position.x;
                    next.z = particle.position.z;
                }
            }

            particle.position = next;
        }

        ++i;
    }
}

void Particles::render(Shader& chunkShader, const World& world,
                       const glm::vec3& cameraRight, const glm::vec3& cameraUp)
{
    if (m_particles.empty()) return;

    m_vertices.clear();
    m_vertices.reserve(m_particles.size() * 6 * 9);

    for (const Particle& particle : m_particles)
    {
        // Shrink slightly as they age so they don't just blink out.
        const float fade = 1.0f - (particle.age / particle.lifetime);
        const float half = particle.size * 0.5f * (0.45f + 0.55f * fade);

        const glm::vec3 right = cameraRight * half;
        const glm::vec3 up = cameraUp * half;

        const int lx = static_cast<int>(std::floor(particle.position.x));
        const int ly = static_cast<int>(std::floor(particle.position.y));
        const int lz = static_cast<int>(std::floor(particle.position.z));
        const float sky = world.skyLight(lx, ly, lz) / 15.0f;
        const float blockLight = world.blockLightAt(lx, ly, lz) / 15.0f;

        const glm::vec3 corners[4] = {
            particle.position - right - up,
            particle.position + right - up,
            particle.position + right + up,
            particle.position - right + up
        };
        const float u[4] = { particle.u0, particle.u1, particle.u1, particle.u0 };
        const float v[4] = { particle.v1, particle.v1, particle.v0, particle.v0 };

        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        for (int k : order)
        {
            m_vertices.push_back(corners[k].x);
            m_vertices.push_back(corners[k].y);
            m_vertices.push_back(corners[k].z);
            m_vertices.push_back(u[k]);
            m_vertices.push_back(v[k]);
            m_vertices.push_back(0.85f); // flat shade; particles have no face direction
            m_vertices.push_back(sky);
            m_vertices.push_back(blockLight);
            m_vertices.push_back(6.0f);  // skip tangent-space normal mapping
        }
    }

    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE); // billboards can face either way
    m_mesh.draw(GL_TRIANGLES);
    glEnable(GL_CULL_FACE);
}
