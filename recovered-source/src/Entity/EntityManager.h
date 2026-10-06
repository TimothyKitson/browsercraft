#pragma once
#include "Mob.h"
#include <cstdint>
#include <vector>

class World;

// Owns every mob in the world: spawns them around the player, steps them, and
// drops the ones that have died or wandered too far.
//
// Mobs are kept in a flat vector and addressed by index. Nothing holds a
// pointer to a mob across frames, so compacting the vector when one dies is
// safe and there is no handle bookkeeping to get wrong.
class EntityManager
{
public:
    static constexpr int MAX_MOBS = 48;
    static constexpr float SPAWN_MIN_DISTANCE = 22.0f;
    static constexpr float SPAWN_MAX_DISTANCE = 44.0f;
    static constexpr float DESPAWN_DISTANCE = 72.0f;

    void update(float deltaTime, const World& world, const glm::vec3& playerPosition, float daylight);

    // Hands over every sound the mobs queued this frame and clears them. The
    // engine has no audio yet, so for now the caller is free to ignore these;
    // wiring a player in later means draining this and nothing else.
    std::vector<MobSound> drainSounds();

    bool spawnAt(MobId type, glm::vec3 feetPosition, const World& world);
    void clear() { m_mobs.clear(); }

    const std::vector<Mob>& mobs() const { return m_mobs; }
    int count() const { return static_cast<int>(m_mobs.size()); }

    // Nearest living mob whose box the ray passes near, for melee hits.
    Mob* pick(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);

private:
    float random01();
    void trySpawnWave(const World& world, const glm::vec3& playerPosition, float daylight);
    int surfaceBelow(const World& world, int x, int startY, int z) const;
    bool fits(const World& world, const MobType& type, const glm::vec3& feet) const;

    std::vector<Mob> m_mobs;
    float m_spawnTimer = 2.0f;
    uint32_t m_rng = 0x9E3779B9u;
};
