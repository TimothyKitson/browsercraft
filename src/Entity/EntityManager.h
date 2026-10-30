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
    // What the night sends. Exposed so --selftest can ask what can
    // turn up without waiting for dark -- which is how the breeze
    // being in this list went unnoticed until there were chambers.
    static int nightSpeciesCount();
    static MobId nightSpecies(int index);

    // The two other worlds' own lists. Exposed for the same reason
    // the night one is: --selftest asks what can actually spawn
    // rather than keeping its own copy of the answer, which is how a
    // ghast came to exist with nowhere to appear.
    static int netherSpeciesCount();
    static MobId netherSpecies(int index);
    static int endSpeciesCount();
    static MobId endSpecies(int index);

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

    // Turns neutral mobs hostile when the player is looking at them.
    // Separate from update because it is the only thing out here that
    // needs to know where the camera is pointing, and threading that
    // through the whole mob update to serve one mob is not worth it.
    void noticeStares(const World& world, const glm::vec3& eye, const glm::vec3& look);

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
        // Recorded here rather than looked up later: by the time the
        // death is drained the mob it belonged to may already be gone.
        bool baby = false;
    };
    std::vector<Death> drainDeaths();

    // Where a chicken has just laid an egg. Drained the same way a
    // death is, and for the same reason: the egg belongs where the
    // bird was standing, not where it has wandered to by the time
    // anybody asks.
    std::vector<glm::vec3> drainLaidEggs();

    // Where a frog has just eaten a magma cube. The cube is gone by
    // the time this is drained; what is left is the light, and the
    // caller is the one that knows which colour the ground makes it.
    std::vector<glm::vec3> drainFrogMeals();

    // Where a calf was just born, for the hearts and the sound.
    std::vector<glm::vec3> drainBirths();

    // Feeds whatever the ray hits. Returns true when the food was taken,
    // which is the caller's cue to spend one from the stack.
    bool feed(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
              StackId food);

    // Offers the food to whatever is in front, and tames it if that is
    // what the food does to it. True when it took.
    bool tame(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
              StackId food);

    // Whatever tamed, rideable thing is in front of you, if any. Null
    // when what is there cannot be ridden or has not been tamed.
    // Tells a tamed animal in front of you to sit, or to get up again.
    // Null when there is nothing there that takes orders.
    Mob* toggleSit(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);

    Mob* rideable(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);

    bool spawnAt(MobId type, glm::vec3 feetPosition, const World& world, bool baby = false);
    void clear()
    {
        m_mobs.clear(); m_deaths.clear(); m_births.clear(); m_laidEggs.clear();
        m_frogMeals.clear();
    }

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
    std::vector<glm::vec3> m_laidEggs;
    std::vector<glm::vec3> m_frogMeals;
    std::vector<glm::vec3> m_births;
    void pairOffLovers(const World& world);

    // Villagers pair off on their own, counted against the village's
    // beds. Run on a timer rather than every frame: it walks every mob
    // twice and nothing about it needs to be that fresh.
    static constexpr float COURT_INTERVAL = 12.0f;
    float m_courtTimer = COURT_INTERVAL;
    void courtVillagers(const World& world, float deltaTime);

    // Points every tamed wolf at the nearest hostile and lets it
    // bite when it gets there.
    void setWolvesOnHostiles(float deltaTime);
    // Hands routes to the mobs that asked, newest-hungry first and no
    // more than a handful a pass.
    void routeChasers(const World& world, const glm::vec3& playerPosition);
    float m_spawnTimer = 2.0f;
    uint32_t m_rng = 0x9E3779B9u;
};
