#include "Rain.h"
#include "World/World.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Renderer/AtlasTiles.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    constexpr float DROP_WIDTH = 0.055f;
    constexpr float DROP_LENGTH = 0.85f;
    constexpr float FLAKE_SIZE = 0.13f;
}

float Rain::nextRandom()
{
    m_random ^= m_random << 13;
    m_random ^= m_random >> 17;
    m_random ^= m_random << 5;
    return static_cast<float>(m_random & 0xFFFFFF) / static_cast<float>(0x1000000);
}

void Rain::fill()
{
    m_drops.clear();
    m_drops.reserve(DROPS);

    for (int i = 0; i < DROPS; ++i)
    {
        Drop drop;

        // Scattered over a disc rather than a square, so the column
        // around you has no corners to notice as you turn.
        const float angle = nextRandom() * 6.2831853f;
        const float radius = std::sqrt(nextRandom()) * RADIUS;
        drop.offset = glm::vec3(std::cos(angle) * radius,
                                BOTTOM + nextRandom() * (TOP - BOTTOM),
                                std::sin(angle) * radius);
        drop.drift = nextRandom() * 6.2831853f;
        drop.phase = nextRandom();
        m_drops.push_back(drop);
    }
}

void Rain::update(float deltaTime, const glm::vec3& around, float strength, bool snowing)
{
    m_strength = std::clamp(strength, 0.0f, 1.0f);
    m_snowing = snowing;
    m_centre = around;
    m_elapsed += deltaTime;

    if (m_strength <= 0.0f)
    {
        // Nothing falling, nothing to keep. The column is rebuilt from
        // scratch next time it rains, which costs one pass over nine
        // hundred drops and happens once a storm.
        if (!m_drops.empty()) m_drops.clear();
        return;
    }

    if (m_drops.empty()) fill();

    const float speed = m_snowing ? SNOW_FALL_SPEED : FALL_SPEED;
    const float span = TOP - BOTTOM;

    for (Drop& drop : m_drops)
    {
        drop.offset.y -= speed * deltaTime;

        // Back to the top rather than respawned: the set of drops never
        // changes, only where each one is in the column.
        while (drop.offset.y < BOTTOM) drop.offset.y += span;
    }
}

void Rain::render(Shader& chunkShader, const World& world,
                  const glm::vec3& cameraRight, const glm::vec3& cameraUp)
{
    if (m_drops.empty() || m_strength <= 0.0f) return;

    m_vertices.clear();
    m_vertices.reserve(m_drops.size() * 6 * 9);

    const TileUV uv = tileUV(m_snowing ? Tiles::SnowFlake : Tiles::RainDrop);

    // Thin and long for rain, square for snow. Rain hangs vertically
    // whatever way you look: a streak billboarded about both axes spins
    // as you turn and stops reading as falling water.
    const glm::vec3 across = m_snowing ? cameraRight * FLAKE_SIZE
                                       : cameraRight * DROP_WIDTH;
    const glm::vec3 along = m_snowing ? cameraUp * FLAKE_SIZE
                                      : glm::vec3(0.0f, DROP_LENGTH, 0.0f);

    // Fewer drops while the weather is arriving or leaving, rather than
    // fainter ones: a half-transparent sheet of rain reads as fog.
    const size_t showing = static_cast<size_t>(static_cast<float>(m_drops.size()) * m_strength);

    for (size_t i = 0; i < showing; ++i)
    {
        const Drop& drop = m_drops[i];

        glm::vec3 position = m_centre + drop.offset;
        if (m_snowing)
        {
            // Snow wanders on its way down; rain does not.
            const float wander = m_elapsed * 0.7f + drop.drift;
            position.x += std::sin(wander) * 0.45f;
            position.z += std::cos(wander * 0.8f) * 0.45f;
        }

        const int lx = static_cast<int>(std::floor(position.x));
        const int ly = static_cast<int>(std::floor(position.y));
        const int lz = static_cast<int>(std::floor(position.z));

        // Nothing falls through a roof. Checking the sky light rather
        // than tracing upwards costs one lookup and is what keeps rain
        // out of a house without knowing anything about the house.
        if (world.skyLight(lx, ly, lz) <= 0) continue;

        const float sky = world.skyLight(lx, ly, lz) / 15.0f;

        const glm::vec3 corners[4] = {
            position - across,
            position + across,
            position + across + along,
            position - across + along,
        };
        const float u[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
        const float v[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };

        // Both ways round, so a streak is there whichever side of it you
        // happen to be standing on.
        const int order[12] = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
        for (int k : order)
        {
            m_vertices.push_back(corners[k].x);
            m_vertices.push_back(corners[k].y);
            m_vertices.push_back(corners[k].z);
            m_vertices.push_back(u[k]);
            m_vertices.push_back(v[k]);
            m_vertices.push_back(0.95f);
            m_vertices.push_back(sky);
            m_vertices.push_back(0.0f);
            m_vertices.push_back(6.0f);   // no normal mapping on a raindrop
        }
    }

    if (m_vertices.empty()) return;

    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));
    m_mesh.draw(GL_TRIANGLES);
}
