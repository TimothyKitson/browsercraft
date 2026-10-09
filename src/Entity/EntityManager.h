#pragma once
#include "Mob.h"
#include <cstdint>
#include <vector>

class World;

// Owns every mob in the world: spawns them around the player, steps them,
// and drops the ones that have died or wandered too far.
//
// Mobs live in a flat vector addressed by index. Nothing holds a pointer
// across frames, so compacting the vector when one dies is safe and there
// is no handle bookkeeping to get wrong.
class EntityManager
{
public:
    static constexpr int MAX_MOBS = 48;
    static constexpr float SPAWN_MIN_DISTANCE = 22.0f;
    static constexpr float SPAWN_MAX_DISTANCE = 44.0f;
    static constexpr float DESPAWN_DISTANCE = 72.0f;
    // How many mobs may search for a route in one update. A search is
    // bounded but not free, and nothing looks different if a zombie
    // takes another tenth of a second to work out where to go.
    static constexpr int PATHS_PER_UPDATE = 4;

    void update(float deltaTime, const World& world, const glm::vec3& playerPosition,
                float daylight);

    // Hands over every noise the mobs queued this frame and clears them.
    std::vector<MobSound> drainSounds();

    // Likewise for blows landed on the player.
    std::vector<MobStrike> drainStrikes();
    std::vector<MobShot> drainShots();
    std::vector<MobBlast> drainBlasts();

    // What a mob leaves on the ground, once per death. A corpse lingers
    // before it is dropped, so this has to fire on the frame it dies
    // rather than the frame it is removed.
    struct Death
    {
        MobId type = MobId::Sheep;
        glm::vec3 position{ 0.0f };
    };
    std::vector<Death> drainDeaths();

    // Where a calf was just born, for the hearts and the sound.
    std::vector<glm::vec3> drainBirths();

    // Feeds whatever the ray hits. Returns true when the food was taken,
    // which is the caller's cue to spend one from the stack.
    bool feed(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
              StackId food);

    bool spawnAt(MobId type, glm::vec3 feetPosition, const World& world, bool baby = false);
    void clear() { m_mobs.clear(); m_deaths.clear(); m_births.clear(); }

    const std::vector<Mob>& mobs() const { return m_mobs; }
    std::vector<Mob>& mobs() { return m_mobs; }
    int count() const { return static_cast<int>(m_mobs.size()); }
    int youngCount() const
    {
        int young = 0;
        for (const Mob& mob : m_mobs) if (mob.baby()) ++young;
        return young;
    }

    // Nearest living mob the ray passes through, for melee hits.
    Mob* pick(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);

    // Which species may appear given the light and the block underfoot.
    static bool canSpawnOn(MobId type, BlockId ground, int light);

    // Which world this manager is spawning into. Set when the player
    // travels, so the Nether gets its own and nothing else.
    void setDimension(Dimension dimension) { m_dimension = dimension; }

private:
    float random01();
    void trySpawnWave(const World& world, const glm::vec3& playerPosition, float daylight);
    int surfaceBelow(const World& world, int x, int startY, int z) const;
    bool fits(const World& world, const MobType& type, const glm::vec3& feet) const;

    Dimension m_dimension = Dimension::Overworld;
    std::vector<Mob> m_mobs;
    std::vector<Death> m_deaths;
    std::vector<glm::vec3> m_births;
    void pairOffLovers(const World& world);
    // Hands routes to the mobs that asked, newest-hungry first and no
    // more than a handful a pass.
    void routeChasers(const World& world, const glm::vec3& playerPosition);
    float m_spawnTimer = 2.0f;
    uint32_t m_rng = 0x9E3779B9u;
};
