#include "Mob.h"
#include "PlayerAnimation.h"
#include "World/World.h"
#include "Game/Items.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float GRAVITY = -26.0f;
    constexpr float TERMINAL_VELOCITY = -60.0f;
    constexpr float COLLISION_STEP = 0.05f;
    constexpr float STEP_SOUND_DISTANCE = 1.9f;
    constexpr float NOTICE_RANGE = 16.0f;   // hostiles start chasing here
    constexpr float FLEE_RANGE = 5.0f;      // animals back off here
    constexpr float DEATH_SECONDS = 0.6f;
}

Mob::Mob(MobId type, glm::vec3 feetPosition, uint32_t seed, bool baby)
    : m_type(type)
    , m_position(feetPosition)
    , m_rng(seed ? seed : 1u)
{
    if (baby) m_babyTimer = BABY_SECONDS;
    const MobType& t = mobType(type);
    m_health = t.maxHealth;
    m_yaw = random01() * 360.0f;
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

bool Mob::wantsPath() const
{
    // Only something that is hunting asks. A wandering animal steers
    // round what is in front of it instead, which costs nothing.
    return alive() && m_chasing && m_repathTimer <= 0.0f;
}

void Mob::setPath(std::vector<glm::ivec3> path, const glm::ivec3& goal)
{
    m_path = std::move(path);
    m_pathIndex = 0;
    m_pathGoal = goal;
    m_repathTimer = REPATH_INTERVAL;
}

bool Mob::followPath()
{
    while (m_pathIndex < m_path.size())
    {
        const glm::vec3 waypoint(m_path[m_pathIndex].x + 0.5f,
                                 static_cast<float>(m_path[m_pathIndex].y),
                                 m_path[m_pathIndex].z + 0.5f);

        const glm::vec3 toward = waypoint - m_position;
        const float flat = glm::length(glm::vec3(toward.x, 0.0f, toward.z));

        // Close enough to call it reached, and on to the next.
        if (flat < 0.55f) { ++m_pathIndex; continue; }

        m_goalYaw = glm::degrees(std::atan2(toward.z, toward.x));
        m_moving = true;
        return true;
    }

    m_path.clear();
    m_pathIndex = 0;
    return false;
}

// Blocked. Rather than re-rolling a heading at random and walking into
// the same wall again, try turning by steps until something is clear.
void Mob::steerAroundObstacle(const World& world)
{
    const float reach = std::max(0.6f, width());

    for (int turn = 1; turn <= 4; ++turn)
    {
        for (int side = 0; side < 2; ++side)
        {
            const float candidate = m_yaw + (side == 0 ? 1.0f : -1.0f) * turn * 35.0f;
            const float radians = glm::radians(candidate);
            const glm::vec3 ahead = m_position + glm::vec3(std::cos(radians), 0.0f,
                                                           std::sin(radians)) * reach;

            if (collidesAt(world, ahead)) continue;

            m_goalYaw = PlayerAnimation::wrapDegrees(candidate);
            m_goalTimer = 1.5f;
            m_moving = true;
            return;
        }
    }

    // Hemmed in on every side: turn round and hope.
    m_goalYaw = PlayerAnimation::wrapDegrees(m_yaw + 180.0f);
    m_goalTimer = 1.0f;
}

bool Mob::feed(StackId food)
{
    const MobType& t = type();
    if (!alive() || t.breedingFood == Blocks::Air || food != t.breedingFood) return false;

    // Feeding a calf hurries it along instead; that is what Minecraft
    // does, and it stops you breeding something that is still a baby.
    if (baby())
    {
        m_babyTimer = std::max(0.0f, m_babyTimer - BABY_SECONDS * 0.1f);
        return true;
    }

    if (m_breedTimer > 0.0f || inLove()) return false;

    m_loveTimer = LOVE_SECONDS;
    emit(t.voice, 0.7f, 1.15f);
    return true;
}

void Mob::onBred()
{
    m_loveTimer = 0.0f;
    m_breedTimer = BREEDING_COOLDOWN;
}

bool Mob::takeDeathReport()
{
    if (alive() || m_deathReported) return false;
    m_deathReported = true;
    return true;
}

float Mob::deathFade() const
{
    if (alive()) return 1.0f;
    return std::clamp(m_removeTimer / DEATH_SECONDS, 0.0f, 1.0f);
}

void Mob::emit(Sound id, float volume, float pitch)
{
    MobSound sound;
    sound.id = id;
    sound.position = m_position;
    sound.volume = volume;
    sound.pitch = pitch * type().voicePitch;
    m_sounds.push_back(sound);
}

bool Mob::collidesAt(const World& world, const glm::vec3& feet) const
{
    const float half = width() * 0.5f;
    constexpr float EPS = 0.0001f;

    const int minX = static_cast<int>(std::floor(feet.x - half));
    const int maxX = static_cast<int>(std::floor(feet.x + half - EPS));
    const int minY = static_cast<int>(std::floor(feet.y));
    const int maxY = static_cast<int>(std::floor(feet.y + height() - EPS));
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
            // Walking into a one-block step: hop up rather than stall,
            // which is what keeps mobs from piling against every lip of
            // terrain.
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

float Mob::currentSpeed() const
{
    return type().walkSpeed * (panicking() ? PANIC_SPEED : 1.0f);
}

void Mob::chooseNewGoal(const glm::vec3& playerPosition)
{
    const MobType& t = type();
    const glm::vec3 toPlayer = playerPosition - m_position;
    const float distance = glm::length(glm::vec3(toPlayer.x, 0.0f, toPlayer.z));

    // Running from whatever just hit it beats anything else it might do.
    if (panicking() && distance > 0.001f)
    {
        m_goalYaw = glm::degrees(std::atan2(-toPlayer.z, -toPlayer.x));
        m_moving = true;
        m_goalTimer = 0.4f;
        m_chasing = false;
        return;
    }

    // Yaw is degrees about +y with 0 looking down +x, matching the
    // camera, so a heading is atan2(z, x).
    if (t.spawnClass == SpawnClass::Hostile && distance < NOTICE_RANGE && distance > 0.001f)
    {
        m_goalYaw = glm::degrees(std::atan2(toPlayer.z, toPlayer.x));
        m_moving = true;
        m_goalTimer = 0.5f;
        m_chasing = true;
        return;
    }

    m_chasing = false;

    if (t.spawnClass == SpawnClass::Passive && distance < FLEE_RANGE && distance > 0.001f)
    {
        m_goalYaw = glm::degrees(std::atan2(-toPlayer.z, -toPlayer.x));
        m_moving = true;
        m_goalTimer = 1.0f + random01();
        return;
    }

    // Idle: stand still about a third of the time, otherwise amble off.
    m_moving = random01() > 0.34f;
    m_goalYaw = random01() * 360.0f;
    m_goalTimer = 2.0f + random01() * 4.0f;
}

bool Mob::burnsNow(const MobType& type, float daylight, int skyLight, bool inLiquid)
{
    if (!type.burnsInSunlight) return false;
    if (daylight < SUNLIGHT_DAYLIGHT) return false;
    if (inLiquid) return false;
    return skyLight >= 15;
}

void Mob::burnInSunlight(float deltaTime, const World& world, float daylight)
{
    const int x = static_cast<int>(std::floor(m_position.x));
    const int z = static_cast<int>(std::floor(m_position.z));
    const int head = static_cast<int>(std::floor(m_position.y + height() * 0.9f));

    const bool inLiquid = isLiquid(world.getBlock(x, head, z));

    if (!burnsNow(type(), daylight, world.skyLight(x, head, z), inLiquid))
    {
        m_burnTimer = 0.0f;
        m_burnTick = 0.0f;
        return;
    }

    m_burnTimer += deltaTime;
    m_burnTick += deltaTime;
    if (m_burnTick >= SUNLIGHT_BURN_INTERVAL)
    {
        m_burnTick = 0.0f;
        damage(1, glm::vec3(0.0f));
    }
}

void Mob::update(float deltaTime, const World& world, const glm::vec3& playerPosition,
                 float daylight)
{
    const MobType& t = type();

    if (m_hurtFlash > 0.0f) m_hurtFlash = std::max(0.0f, m_hurtFlash - deltaTime);
    if (m_panicTimer > 0.0f) m_panicTimer = std::max(0.0f, m_panicTimer - deltaTime);
    if (m_loveTimer > 0.0f) m_loveTimer = std::max(0.0f, m_loveTimer - deltaTime);
    if (m_breedTimer > 0.0f) m_breedTimer = std::max(0.0f, m_breedTimer - deltaTime);
    if (m_babyTimer > 0.0f) m_babyTimer = std::max(0.0f, m_babyTimer - deltaTime);

    if (!alive())
    {
        m_removeTimer -= deltaTime;
        return;
    }

    burnInSunlight(deltaTime, world, daylight);
    if (!alive()) return;

    if (m_repathTimer > 0.0f) m_repathTimer = std::max(0.0f, m_repathTimer - deltaTime);

    m_goalTimer -= deltaTime;
    if (m_goalTimer <= 0.0f) chooseNewGoal(playerPosition);

    // A route, where there is one, overrules the heading just chosen.
    followPath();

    // Turn towards the goal heading the short way round.
    m_yaw = PlayerAnimation::wrapDegrees(
        m_yaw + PlayerAnimation::wrapDegrees(m_goalYaw - m_yaw) *
                    std::min(1.0f, 6.0f * deltaTime));

    const float radians = glm::radians(m_yaw);
    const glm::vec3 forward(std::cos(radians), 0.0f, std::sin(radians));
    const glm::vec3 target = m_moving ? forward * currentSpeed() : glm::vec3(0.0f);

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

    // Blocked flat against something. Turning until the way is clear is
    // what stops a mob grinding into the same wall for as long as it can
    // see you; a random new heading only worked by accident.
    if (m_moving && wasOnGround)
    {
        const glm::vec3 moved = m_position - before;
        if (glm::length(glm::vec3(moved.x, 0.0f, moved.z)) < 0.001f)
        {
            // Give up on a route that is walking us into something.
            m_path.clear();
            m_pathIndex = 0;
            steerAroundObstacle(world);
        }
    }

    // The walk cycle is driven by ground covered, so the legs stay in
    // step with the floor at any frame rate -- the same rule the player
    // model follows.
    const glm::vec3 delta = m_position - before;
    const float travelled = glm::length(glm::vec3(delta.x, 0.0f, delta.z));
    const float speed = deltaTime > 0.0001f ? travelled / deltaTime : 0.0f;

    m_gait += travelled * PlayerAnimation::PHASE_PER_BLOCK;
    m_gaitAmount = PlayerAnimation::approach(
        m_gaitAmount, std::clamp(speed / std::max(0.5f, currentSpeed()), 0.0f, 1.0f),
        10.0f, deltaTime);

    if (m_onGround)
    {
        m_stepDistance += travelled;
        if (m_stepDistance >= STEP_SOUND_DISTANCE)
        {
            m_stepDistance = 0.0f;
            emit(Sound::MobStep, 0.22f, 0.9f + random01() * 0.2f);
        }
    }

    // Close enough to swing at. Measured between the two boxes rather
    // than between their centres, so a spider's bulk counts for as much
    // as its reach does.
    if (m_attackTimer > 0.0f) m_attackTimer = std::max(0.0f, m_attackTimer - deltaTime);

    if (t.attackDamage > 0 && m_attackTimer <= 0.0f)
    {
        const glm::vec3 toPlayer = playerPosition - m_position;
        const float flat = glm::length(glm::vec3(toPlayer.x, 0.0f, toPlayer.z));
        const float reach = t.attackReach + width() * 0.5f;
        const bool levelWith = std::fabs(toPlayer.y) < std::max(t.height, 1.8f);

        if (flat < reach && levelWith)
        {
            MobStrike strike;
            strike.damage = t.attackDamage;
            strike.from = m_position;
            m_strikes.push_back(strike);
            m_attackTimer = t.attackInterval;
        }
    }

    m_ambientTimer -= deltaTime;
    if (m_ambientTimer <= 0.0f)
    {
        m_ambientTimer = t.ambientMinSeconds + random01() * (t.ambientMaxSeconds - t.ambientMinSeconds);
        emit(t.voice, 0.8f, 0.9f + random01() * 0.2f);
    }
}

void Mob::damage(int amount, const glm::vec3& fromDirection)
{
    if (amount <= 0 || !alive()) return;

    m_health = std::max(0, m_health - amount);
    m_hurtFlash = 0.35f;

    // Knocked back and off its feet a little, so a hit reads as a hit.
    const glm::vec3 flat(fromDirection.x, 0.0f, fromDirection.z);
    const float length = glm::length(flat);
    if (length > 0.001f) m_velocity += (flat / length) * 5.0f;
    m_velocity.y = std::max(m_velocity.y, 4.2f);

    // Being hit is a good reason to stop ambling and react.
    m_goalTimer = 0.0f;
    if (type().spawnClass == SpawnClass::Passive) m_panicTimer = PANIC_SECONDS;

    if (alive()) emit(type().hurtVoice, 0.9f, 0.9f + random01() * 0.2f);
    else emit(type().deathVoice, 1.0f, 0.9f + random01() * 0.2f);
}
