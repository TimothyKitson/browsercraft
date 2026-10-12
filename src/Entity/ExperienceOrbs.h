#pragma once
#include "World/Block.h"
#include "Renderer/Mesh.h"
#include <vector>
#include <glm/glm.hpp>

class World;
class Shader;
class Player;
class AudioEngine;

// The little green motes a kill or a broken seam leaves behind.
//
// They behave unlike dropped blocks in the one way that matters: an orb
// comes to you. Get within a few blocks and it accelerates towards your
// chest and keeps accelerating, so a scattered kill is swept up in one
// pass rather than walked over one mote at a time. That homing is the
// whole reason this is its own thing and not a flag on DroppedItems.
class ExperienceOrbs
{
public:
    static constexpr float SIZE = 0.22f;
    static constexpr float ATTRACT_RADIUS = 5.0f;
    static constexpr float COLLECT_RADIUS = 0.7f;
    static constexpr float ATTRACT_SPEED = 9.0f;
    static constexpr float SPAWN_DELAY = 0.25f;
    static constexpr float LIFETIME_SECONDS = 300.0f;
    static constexpr float MERGE_RADIUS = 0.7f;

    // One orb of exactly this worth. Prefer spawnValue, which splits an
    // award into the sizes orbs actually come in.
    void spawnOne(const glm::vec3& position, int worth);

    // An award of `worth`, scattered as the orbs Minecraft would drop.
    void spawnValue(const glm::vec3& position, int worth);

    // Returns the experience collected this step, for the caller to add
    // to the player: this class deliberately knows nothing about levels.
    int update(float deltaTime, const World& world, const Player& player, AudioEngine& audio);

    void render(Shader& chunkShader, const World& world);

    void clear() { m_orbs.clear(); }
    int count() const { return static_cast<int>(m_orbs.size()); }
    int totalWorth() const;

private:
    struct Orb
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        int worth = 1;
        float age = 0.0f;
        float bob = 0.0f;
    };

    std::vector<Orb> m_orbs;
    Mesh m_mesh;
    std::vector<float> m_vertices;

    bool blocked(const World& world, const glm::vec3& centre) const;
    void mergeNearby();
};
