#pragma once
#include "Renderer/Mesh.h"
#include <vector>
#include <glm/glm.hpp>

class World;
class Shader;

// The rain and snow you see falling.
//
// Not particles: there is no list of drops that are born and die. A
// fixed set of them is kept in a column that travels with the player and
// wraps round as they fall, so walking never outruns the weather and the
// cost does not depend on how long it has been raining. Minecraft does
// the same thing, and for the same reason.
class Rain
{
public:
    // How far out the column reaches, and how many drops fill it.
    static constexpr float RADIUS = 14.0f;
    static constexpr float TOP = 12.0f;     // above the player's head
    static constexpr float BOTTOM = -8.0f;  // and below their feet
    static constexpr int DROPS = 2600;

    static constexpr float FALL_SPEED = 26.0f;
    static constexpr float SNOW_FALL_SPEED = 4.0f;

    // `strength` is 0 when the sky is clear and 1 in full rain; drops
    // are drawn in proportion, so the weather arrives and leaves rather
    // than switching on. `snowing` draws the slower, drifting sort.
    void update(float deltaTime, const glm::vec3& around, float strength, bool snowing);

    void render(Shader& chunkShader, const World& world,
                const glm::vec3& cameraRight, const glm::vec3& cameraUp);

    void clear() { m_drops.clear(); }

private:
    struct Drop
    {
        glm::vec3 offset{ 0.0f };   // relative to the column's centre
        float drift = 0.0f;         // sideways wander, snow only
        float phase = 0.0f;
    };

    std::vector<Drop> m_drops;
    glm::vec3 m_centre{ 0.0f };
    float m_strength = 0.0f;
    bool m_snowing = false;
    float m_elapsed = 0.0f;

    Mesh m_mesh;
    std::vector<float> m_vertices;
    uint32_t m_random = 12345u;

    float nextRandom();
    void fill();
};
