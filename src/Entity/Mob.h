#pragma once
#include "MobType.h"
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

// A living thing: gravity and box collision against the voxel world, plus
// enough wits to wander, notice the player, and make noise about it.
class Mob
{
public:
    Mob(MobId type, glm::vec3 feetPosition, uint32_t seed);

    void update(float deltaTime, const World& world, const glm::vec3& playerPosition);

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

    float m_ambientTimer = 0.0f;
    float m_stepDistance = 0.0f;
    float m_gait = 0.0f;
    float m_gaitAmount = 0.0f;
    float m_hurtFlash = 0.0f;
    float m_removeTimer = 0.6f;

    uint32_t m_rng = 1;
    std::vector<MobSound> m_sounds;
};
