#include "Mob.h"
#include "World/World.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float GRAVITY = -26.0f;
    constexpr float TERMINAL_VELOCITY = -60.0f;
    constexpr float COLLISION_STEP = 0.05f;
    constexpr float STEP_SOUND_DISTANCE = 2.1f; // metres between footfalls
    constexpr float NOTICE_RANGE = 16.0f;       // hostiles start chasing here
    constexpr float FLEE_RANGE = 5.0f;          // passives back off here
    constexpr float TWO_PI = 6.2831853f;
}

Mob::Mob(MobId type, glm::vec3 feetPosition, uint32_t seed)
    : m_type(type)
    , m_position(feetPosition)
    , m_rng(seed ? seed : 1u)
{
    const MobType& t = mobType(type);
    m_health = t.maxHealth;
    m_yaw = random01() * TWO_PI;
    m_goalYaw = m_yaw;
    m_ambientTimer = t.ambientMinSeconds + random01() * (t.ambientMaxSeconds - t.ambientMinSeconds);
}

float Mob::random01()
{
    // xorshift32: small, deterministic, and good enough for picking headings.
    m_rng ^= m_rng << 13;
    m_rng ^= m_rng >> 17;
    m_rng ^= m_rng << 5;
    return static_cast<float>(m_rng & 0xFFFFFF) / static_cast<float>(0x1000000);
}

void Mob::emit(const char* name, int variants, float volume, float pitch)
{
    if (!name || !*name || variants <= 0) return;
    MobSound sound;
    sound.name = name;
    sound.variants = variants;
    sound.position = m_position;
    sound.volume = volume;
    sound.pitch = pitch;
    m_sounds.push_back(sound);
}

bool Mob::collidesAt(const World& world, const glm::vec3& feet) const
{
    const MobType& t = type();
    const float half = t.width * 0.5f;
    constexpr float EPS = 0.0001f;

    const int minX = static_cast<int>(std::floor(feet.x - half));
    const int maxX = static_cast<int>(std::floor(feet.x + half - EPS));
    const int minY = static_cast<int>(std::floor(feet.y));
    const int maxY = static_cast<int>(std::floor(feet.y + t.height - EPS));
    const int minZ = static_cast<int>(std::floor(feet.z - half));
    const int maxZ = static_cast<int>(std::floor(feet.z + half - EPS));

    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                if (isSolid(world.getBlock(x, y, z)))
                    return true;

    return false;
}

void Mob::moveAxis(const World& world, float delta, int axis)
{
    if (delta == 0.0f) return;

    float remaining = delta;
    const float step = (delta > 0.0f) ? COLLISION_STEP : -COLLISION_STEP;

    while (std::fabs(remaining) > 0.00001f)
    {
        const float move = (std::fabs(remaining) < std::fabs(step)) ? remaining : step;

        glm::vec3 next = m_position;
        next[axis] += move;

        if (collidesAt(world, next))
        {
            // Walking into a one-block step: hop up rather than stall, which
            // is what keeps mobs from piling against every terrain lip.
            if (axis != 1 && m_onGround)
            {
                glm::vec3 stepUp = next;
                stepUp.y += 1.0f;
                if (!collidesAt(world, stepUp))
                {
                    m_position = stepUp;
                    remaining -= move;
                    continue;
                }
            }

            if (axis == 1 && move < 0.0f) m_onGround = true;
            m_velocity[axis] = 0.0f;
            return;
        }

        m_position = next;
        remaining -= move;
    }
}

void Mob::chooseNewGoal(const glm::vec3& playerPosition)
{
    const MobType& t = type();
    const glm::vec3 toPlayer = playerPosition - m_position;
    const float distance = glm::length(glm::vec3(toPlayer.x, 0.0f, toPlayer.z));

    if (t.spawnClass == SpawnClass::Hostile && distance < NOTICE_RANGE && distance > 0.001f)
    {
        m_goalYaw = std::atan2(toPlayer.x, toPlayer.z);
        m_moving = true;
        m_goalTimer = 0.5f;
        return;
    }

    if (t.spawnClass == SpawnClass::Passive && distance < FLEE_RANGE && distance > 0.001f)
    {
        m_goalYaw = std::atan2(-toPlayer.x, -toPlayer.z);
        m_moving = true;
        m_goalTimer = 1.0f + random01();
        return;
    }

    // Idle: stand still about a third of the time, otherwise amble somewhere.
    m_moving = random01() > 0.34f;
    m_goalYaw = random01() * TWO_PI;
    m_goalTimer = 2.0f + random01() * 4.0f;
}

void Mob::update(float deltaTime, const World& world, const glm::vec3& playerPosition)
{
    const MobType& t = type();

    if (m_hurtFlash > 0.0f) m_hurtFlash = std::max(0.0f, m_hurtFlash - deltaTime);

    if (!alive())
    {
        m_removeTimer -= deltaTime;
        return;
    }

    m_goalTimer -= deltaTime;
    if (m_goalTimer <= 0.0f) chooseNewGoal(playerPosition);

    // Turn towards the goal heading by the shortest way round.
    float difference = m_goalYaw - m_yaw;
    while (difference > 3.14159265f) difference -= TWO_PI;
    while (difference < -3.14159265f) difference += TWO_PI;
    m_yaw += difference * std::min(1.0f, 6.0f * deltaTime);

    const glm::vec3 forward(std::sin(m_yaw), 0.0f, std::cos(m_yaw));
    const glm::vec3 target = m_moving ? forward * t.walkSpeed : glm::vec3(0.0f);

    const float blend = std::min(1.0f, 10.0f * deltaTime);
    m_velocity.x += (target.x - m_velocity.x) * blend;
    m_velocity.z += (target.z - m_velocity.z) * blend;

    m_velocity.y = std::max(m_velocity.y + GRAVITY * deltaTime, TERMINAL_VELOCITY);

    const bool wasOnGround = m_onGround;
    m_onGround = false;

    const glm::vec3 before = m_position;
    moveAxis(world, m_velocity.x * deltaTime, 0);
    moveAxis(world, m_velocity.y * deltaTime, 1);
    moveAxis(world, m_velocity.z * deltaTime, 2);

    // Blocked flat against something: pick a new direction next tick.
    if (m_moving && wasOnGround)
    {
        const glm::vec3 moved = m_position - before;
        if (glm::length(glm::vec3(moved.x, 0.0f, moved.z)) < 0.001f)
            m_goalTimer = 0.0f;
    }

    // Footsteps are driven by distance covered, so they stay in step with the
    // legs regardless of frame rate.
    const glm::vec3 delta = m_position - before;
    const float travelled = glm::length(glm::vec3(delta.x, 0.0f, delta.z));
    m_gait += travelled * 2.6f;
    if (m_onGround)
    {
        m_stepDistance += travelled;
        if (m_stepDistance >= STEP_SOUND_DISTANCE)
        {
            m_stepDistance = 0.0f;
            emit(t.soundStep, t.stepVariants, 0.22f, 0.9f + random01() * 0.2f);
        }
    }

    m_ambientTimer -= deltaTime;
    if (m_ambientTimer <= 0.0f)
    {
        m_ambientTimer = t.ambientMinSeconds + random01() * (t.ambientMaxSeconds - t.ambientMinSeconds);
        emit(t.soundAmbient, t.ambientVariants, 0.8f, 0.9f + random01() * 0.2f);
    }
}

void Mob::damage(int amount)
{
    if (amount <= 0 || !alive()) return;

    const MobType& t = type();
    m_health = std::max(0, m_health - amount);
    m_hurtFlash = 0.35f;

    // Jump back a little so a hit reads as a hit.
    m_velocity.y = std::max(m_velocity.y, 4.2f);

    if (alive()) emit(t.soundHurt, t.hurtVariants, 0.9f, 0.9f + random01() * 0.2f);
    else emit(t.soundDeath, t.deathVariants, 1.0f, 0.9f + random01() * 0.2f);
}
