#pragma once
#include "MobType.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

class World;

// One sound the mob wants played this frame. The engine has no audio system
// yet, so mobs queue these and the host drains them; wiring in a player later
// means reading this queue instead of touching mob code.
//
// `name` is a sound path without the variant number or extension, e.g.
// "mob/sheep/say" -> assets/sounds/mob/sheep/say2.ogg. `variants` says how
// many numbered files that event has, so the host can pick one.
struct MobSound
{
    const char* name = "";
    int variants = 0;
    glm::vec3 position{ 0.0f };
    float volume = 1.0f;
    float pitch = 1.0f;
};

// A living entity: gravity and AABB collision against the voxel world, plus
// enough AI to wander, notice the player, and make noise.
class Mob
{
public:
    Mob(MobId type, glm::vec3 feetPosition, uint32_t seed);

    void update(float deltaTime, const World& world, const glm::vec3& playerPosition);

    void damage(int amount);
    bool alive() const { return m_health > 0; }
    bool finished() const { return m_removeTimer <= 0.0f && !alive(); }

    MobId typeId() const { return m_type; }
    const MobType& type() const { return mobType(m_type); }

    glm::vec3 position() const { return m_position; }
    float yaw() const { return m_yaw; }
    int health() const { return m_health; }

    // Non-zero while the mob is flashing red from a hit.
    float hurtFlash() const { return m_hurtFlash; }

    // How far through its walk cycle the mob is, for leg swing.
    float gait() const { return m_gait; }

    std::vector<MobSound>& sounds() { return m_sounds; }

private:
    float random01();
    void emit(const char* name, int variants, float volume, float pitch);
    bool collidesAt(const World& world, const glm::vec3& feet) const;
    void moveAxis(const World& world, float delta, int axis);
    void chooseNewGoal(const glm::vec3& playerPosition);

    MobId m_type;
    glm::vec3 m_position{ 0.0f };
    glm::vec3 m_velocity{ 0.0f };
    float m_yaw = 0.0f;         // radians, 0 = +Z
    int m_health = 1;
    bool m_onGround = false;

    float m_goalYaw = 0.0f;
    float m_goalTimer = 0.0f;   // seconds until the mob picks a new heading
    bool m_moving = false;

    float m_ambientTimer = 0.0f;
    float m_stepDistance = 0.0f; // metres walked since the last footstep sound
    float m_gait = 0.0f;
    float m_hurtFlash = 0.0f;
    float m_removeTimer = 0.6f;  // death animation before the mob is dropped

    uint32_t m_rng = 1;
    std::vector<MobSound> m_sounds;
};
