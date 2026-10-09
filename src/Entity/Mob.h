#pragma once
#include "MobType.h"
#include "Pathfinder.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

class World;

// One noise a mob wants made this frame. Mobs queue these rather than
// reaching for the audio engine themselves, so the physics stays testable
// without a sound card.
struct MobSound
{
    Sound id = Sound::MobGrunt;
    glm::vec3 position{ 0.0f };
    float volume = 1.0f;
    float pitch = 1.0f;
};

// A blow a mob has landed on the player this frame. Queued rather than
// applied, for the same reason the sounds are: a mob knows nothing about
// the player beyond where they are standing.
struct MobStrike
{
    int damage = 0;
    glm::vec3 from{ 0.0f };     // the mob's position, for knockback
};

// A living thing: gravity and box collision against the voxel world, plus
// enough wits to wander, notice the player, fight them, and make noise.
class Mob
{
public:
    Mob(MobId type, glm::vec3 feetPosition, uint32_t seed, bool baby = false);

    void update(float deltaTime, const World& world, const glm::vec3& playerPosition,
                float daylight = 0.0f);

    // Set while the sun is burning it: the renderer tints it, and it is
    // losing health once a second until it finds shade or dies.
    bool burning() const { return m_burnTimer > 0.0f; }

    // Hurt an animal and it bolts rather than ambles: twice its walking
    // pace, for long enough to actually get away from you.
    static constexpr float PANIC_SECONDS = 5.0f;
    static constexpr float PANIC_SPEED = 2.0f;

    bool panicking() const { return m_panicTimer > 0.0f; }
    float currentSpeed() const;

    static constexpr float SUNLIGHT_BURN_INTERVAL = 1.0f;
    static constexpr float SUNLIGHT_DAYLIGHT = 0.5f;

    // Whether the sun is on it right now. Pulled out of the update so
    // --selftest can ask the question without a world to stand in: a
    // block of wool overhead or a metre of water is all it takes.
    static bool burnsNow(const MobType& type, float daylight, int skyLight, bool inLiquid);

    void damage(int amount, const glm::vec3& fromDirection);
    bool alive() const { return m_health > 0; }
    bool finished() const { return m_removeTimer <= 0.0f && !alive(); }

    MobId typeId() const { return m_type; }
    const MobType& type() const { return mobType(m_type); }

    glm::vec3 position() const { return m_position; }
    // Degrees, the same convention as the camera and the player model.
    float yaw() const { return m_yaw; }
    int health() const { return m_health; }

    // Non-zero while flashing from a hit.
    float hurtFlash() const { return m_hurtFlash; }
    // 0 standing, 1 at walking pace; drives how far the legs swing.
    float gaitAmount() const { return m_gaitAmount; }
    // Radians around the walk cycle.
    float gait() const { return m_gait; }
    // 1 while alive, falling to 0 as a dead one keels over.
    float deathFade() const;

    std::vector<MobSound>& sounds() { return m_sounds; }
    std::vector<MobStrike>& strikes() { return m_strikes; }

    // True exactly once, on the first call after it dies. A corpse
    // lingers for a moment before it is dropped, so whatever it leaves
    // behind cannot wait for the removal.
    bool takeDeathReport();

    // --- breeding ---
    //
    // A fed animal is "in love" for half a minute. Two of the same kind
    // in love and standing close enough pair off, and one of them
    // reports the birth.
    static constexpr float LOVE_SECONDS = 30.0f;
    static constexpr float BREEDING_COOLDOWN = 60.0f;
    static constexpr float BABY_SECONDS = 300.0f;   // five minutes to grow up
    static constexpr float BABY_SCALE = 0.5f;

    bool baby() const { return m_babyTimer > 0.0f; }
    bool inLove() const { return m_loveTimer > 0.0f; }
    bool canBreed() const { return !baby() && m_breedTimer <= 0.0f && alive(); }
    // Half size while young, which is also what shrinks its hitbox.
    float scale() const { return baby() ? BABY_SCALE : 1.0f; }
    float width() const { return type().width * scale(); }
    float height() const { return type().height * scale(); }

    // --- pathfinding ---
    //
    // A mob does not search for itself. It says it would like a route,
    // the manager works out how many searches the frame can afford, and
    // the answer comes back here. That is what keeps forty-eight of them
    // from each searching the world every frame.
    static constexpr float REPATH_INTERVAL = 1.0f;

    bool wantsPath() const;
    glm::ivec3 pathGoal() const { return m_pathGoal; }
    void setPath(std::vector<glm::ivec3> path, const glm::ivec3& goal);
    bool hasPath() const { return m_pathIndex < m_path.size(); }
    int pathLength() const { return static_cast<int>(m_path.size()); }

    // Accepts the food and falls in love. False if it is not interested.
    bool feed(StackId food);
    // Called on both parents once they have paired off.
    void onBred();

private:
    float random01();
    void emit(Sound id, float volume, float pitch);
    bool collidesAt(const World& world, const glm::vec3& feet) const;
    void moveAxis(const World& world, float delta, int axis);
    void chooseNewGoal(const glm::vec3& playerPosition);

    MobId m_type;
    glm::vec3 m_position{ 0.0f };
    glm::vec3 m_velocity{ 0.0f };
    float m_yaw = 0.0f;
    int m_health = 1;
    bool m_onGround = false;

    float m_goalYaw = 0.0f;
    float m_goalTimer = 0.0f;
    bool m_moving = false;

    std::vector<glm::ivec3> m_path;
    size_t m_pathIndex = 0;
    glm::ivec3 m_pathGoal{ 0 };
    float m_repathTimer = 0.0f;
    bool m_chasing = false;

    // Steers towards the next waypoint. True while there is one.
    void burnInSunlight(float deltaTime, const World& world, float daylight);
    bool followPath();
    // A heading that is actually clear, for when the chosen one is not.
    void steerAroundObstacle(const World& world);

    float m_attackTimer = 0.0f;
    float m_ambientTimer = 0.0f;
    float m_stepDistance = 0.0f;
    float m_gait = 0.0f;
    float m_gaitAmount = 0.0f;
    float m_loveTimer = 0.0f;
    float m_breedTimer = 0.0f;
    float m_babyTimer = 0.0f;
    float m_hurtFlash = 0.0f;
    float m_panicTimer = 0.0f;
    float m_burnTimer = 0.0f;
    float m_burnTick = 0.0f;
    float m_removeTimer = 0.6f;
    bool m_deathReported = false;

    uint32_t m_rng = 1;
    std::vector<MobSound> m_sounds;
    std::vector<MobStrike> m_strikes;
};
