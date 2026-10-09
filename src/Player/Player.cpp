#include "Player.h"
#include "World/World.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float WALK_SPEED = 4.3f;
    constexpr float SPRINT_SPEED = 5.9f;
    constexpr float SNEAK_SPEED = 1.4f;
    constexpr float SWIM_SPEED = 2.6f;
    constexpr float FLY_SPEED = 11.0f;
    constexpr float FLY_SPRINT_SPEED = 24.0f;

    constexpr float GRAVITY = -30.0f;
    constexpr float WATER_GRAVITY = -5.0f;
    constexpr float JUMP_VELOCITY = 8.6f;   // ~1.25 blocks, like Minecraft
    constexpr float SWIM_UP_VELOCITY = 3.4f;
    constexpr float TERMINAL_VELOCITY = -78.0f;
    constexpr float WATER_TERMINAL = -6.0f;

    constexpr float GROUND_ACCELERATION = 14.0f;
    constexpr float AIR_ACCELERATION = 4.0f;
    constexpr float WATER_ACCELERATION = 6.0f;

    constexpr float COLLISION_STEP = 0.05f;
    constexpr float SAFE_FALL_DISTANCE = 3.0f;
}

Player::Player(glm::vec3 feetPosition)
    : position(feetPosition), m_lastPosition(feetPosition)
{
}

void Player::addExhaustion(float amount)
{
    if (creative() || amount <= 0.0f) return;
    m_exhaustion += amount;
}

bool Player::eat(int hungerPoints, float saturationPoints)
{
    if (!canEat()) return false;

    hunger = std::min(MAX_HUNGER, hunger + hungerPoints);
    saturation = std::min(static_cast<float>(hunger), saturation + saturationPoints);
    return true;
}

void Player::updateVitals(float deltaTime)
{
    if (creative())
    {
        hunger = MAX_HUNGER;
        saturation = 5.0f;
        m_exhaustion = 0.0f;
        m_starveTimer = 0.0f;
        regenerate(deltaTime);
        return;
    }

    while (m_exhaustion >= EXHAUSTION_PER_DRAIN)
    {
        m_exhaustion -= EXHAUSTION_PER_DRAIN;
        if (saturation > 0.0f) saturation = std::max(0.0f, saturation - 1.0f);
        else hunger = std::max(0, hunger - 1);
    }

    if (hunger > 0) m_starveTimer = 0.0f;
    else
    {
        m_starveTimer += deltaTime;
        if (m_starveTimer >= 4.0f)
        {
            m_starveTimer = 0.0f;
            damage(1);
        }
    }

    regenerate(deltaTime);
}

void Player::regenerate(float deltaTime)
{
    const bool wellFed = creative() || hunger >= REGEN_HUNGER;
    if (health > 0 && health < MAX_HEALTH && damageFlash <= 0.0f && wellFed)
    {
        m_regenTimer += deltaTime;
        if (m_regenTimer >= 4.0f)
        {
            m_regenTimer = 0.0f;
            heal(1);
            addExhaustion(6.0f);
        }
    }
    else
    {
        m_regenTimer = 0.0f;
    }
}

AABB Player::aabbAt(const glm::vec3& feetPosition) const
{
    const glm::vec3 half(WIDTH * 0.5f, 0.0f, WIDTH * 0.5f);
    return AABB{
        feetPosition - half,
        feetPosition + glm::vec3(half.x, HEIGHT, half.z)
    };
}

glm::vec3 Player::eyePosition() const
{
    return position + glm::vec3(0.0f, sneaking ? SNEAK_EYE_HEIGHT : EYE_HEIGHT, 0.0f);
}

bool Player::collides(const World& world, const AABB& box) const
{
    constexpr float EPS = 0.0001f;
    const int minX = static_cast<int>(std::floor(box.min.x));
    const int maxX = static_cast<int>(std::floor(box.max.x - EPS));
    const int minY = static_cast<int>(std::floor(box.min.y));
    const int maxY = static_cast<int>(std::floor(box.max.y - EPS));
    const int minZ = static_cast<int>(std::floor(box.min.z));
    const int maxZ = static_cast<int>(std::floor(box.max.z - EPS));

    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                if (isSolid(world.getBlock(x, y, z)))
                    return true;

    return false;
}

bool Player::supported(const World& world, const glm::vec3& feetPosition) const
{
    AABB probe = aabbAt(feetPosition);
    probe.min.y -= 0.08f;
    probe.max.y = probe.min.y + 0.05f;
    return collides(world, probe);
}

void Player::moveAxis(const World& world, float delta, int axis, bool preventLedgeFall)
{
    if (delta == 0.0f) return;

    // Step in small increments rather than solving the exact contact point:
    // simpler to get right, and at 5 cm the error is imperceptible.
    float remaining = delta;
    const float step = (delta > 0.0f) ? COLLISION_STEP : -COLLISION_STEP;

    while (std::fabs(remaining) > 0.00001f)
    {
        const float move = (std::fabs(remaining) < std::fabs(step)) ? remaining : step;

        glm::vec3 next = position;
        next[axis] += move;

        if (collides(world, aabbAt(next)))
        {
            if (axis == 1 && move < 0.0f)
            {
                if (m_falling) applyFallDamage(position.y);
                onGround = true;
                m_falling = false;
            }
            velocity[axis] = 0.0f;
            return;
        }

        // Sneaking stops you shuffling off the edge of what you're standing on.
        if (preventLedgeFall && axis != 1 && !supported(world, next))
            return;

        position = next;
        remaining -= move;
    }
}

void Player::applyFallDamage(float landingY)
{
    if (creative() || flying) return;

    const float fallDistance = m_fallStartY - landingY;
    if (fallDistance > SAFE_FALL_DISTANCE)
        damage(static_cast<int>(std::floor(fallDistance - SAFE_FALL_DISTANCE)));
}

bool Player::isInWater(const World& world) const
{
    const AABB box = aabb();
    const int minX = static_cast<int>(std::floor(box.min.x));
    const int maxX = static_cast<int>(std::floor(box.max.x - 0.0001f));
    const int minY = static_cast<int>(std::floor(box.min.y));
    const int maxY = static_cast<int>(std::floor(box.max.y - 0.0001f));
    const int minZ = static_cast<int>(std::floor(box.min.z));
    const int maxZ = static_cast<int>(std::floor(box.max.z - 0.0001f));

    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                if (isLiquid(world.getBlock(x, y, z)))
                    return true;

    return false;
}

bool Player::isHeadUnderwater(const World& world) const
{
    const glm::vec3 eye = eyePosition();
    return isLiquid(world.getBlock(static_cast<int>(std::floor(eye.x)),
                                   static_cast<int>(std::floor(eye.y)),
                                   static_cast<int>(std::floor(eye.z))));
}

bool Player::damage(int amount)
{
    if (creative() || amount <= 0 || m_hurtCooldown > 0.0f) return false;
    health = std::max(0, health - amount);
    damageFlash = 0.4f;
    m_hurtCooldown = HURT_IMMUNITY;
    m_exhaustion += 0.1f;
    return true;
}

void Player::heal(int amount)
{
    health = std::min(MAX_HEALTH, health + amount);
}

void Player::respawn(glm::vec3 feetPosition)
{
    position = feetPosition;
    velocity = glm::vec3(0.0f);
    health = MAX_HEALTH;
    hunger = MAX_HUNGER;
    saturation = 5.0f;
    m_exhaustion = 0.0f;
    m_starveTimer = 0.0f;
    m_falling = false;
    m_jumped = false;
    damageFlash = 0.0f;
    m_hurtCooldown = 0.0f;
    m_lastPosition = feetPosition;
}

void Player::update(float deltaTime, const World& world, const Controls& controls)
{
    m_jumped = false;
    if (damageFlash > 0.0f) damageFlash = std::max(0.0f, damageFlash - deltaTime);
    if (m_hurtCooldown > 0.0f) m_hurtCooldown = std::max(0.0f, m_hurtCooldown - deltaTime);

    updateVitals(deltaTime);

    sneaking = controls.sneak && !flying;
    const bool inWater = isInWater(world);
    sprinting = controls.sprint && !sneaking && glm::length(controls.wishDirection) > 0.1f &&
                (creative() || hunger > SPRINT_HUNGER);

    // --- choose a target horizontal velocity ---
    float targetSpeed;
    if (flying) targetSpeed = controls.sprint ? FLY_SPRINT_SPEED : FLY_SPEED;
    else if (inWater) targetSpeed = SWIM_SPEED;
    else if (sneaking) targetSpeed = SNEAK_SPEED;
    else if (sprinting) targetSpeed = SPRINT_SPEED;
    else targetSpeed = WALK_SPEED;

    const glm::vec3 targetVelocity = controls.wishDirection * targetSpeed;

    float acceleration;
    if (flying) acceleration = 16.0f;
    else if (inWater) acceleration = WATER_ACCELERATION;
    else if (onGround) acceleration = GROUND_ACCELERATION;
    else acceleration = AIR_ACCELERATION;

    const float blend = std::clamp(acceleration * deltaTime, 0.0f, 1.0f);
    velocity.x += (targetVelocity.x - velocity.x) * blend;
    velocity.z += (targetVelocity.z - velocity.z) * blend;

    // --- vertical motion ---
    if (flying)
    {
        float verticalTarget = 0.0f;
        if (controls.jumpHeld) verticalTarget += targetSpeed;
        if (controls.descend) verticalTarget -= targetSpeed;
        velocity.y += (verticalTarget - velocity.y) * std::clamp(16.0f * deltaTime, 0.0f, 1.0f);
        onGround = false;
        m_falling = false;
    }
    else if (inWater)
    {
        velocity.y += WATER_GRAVITY * deltaTime;
        if (controls.jumpHeld) velocity.y = SWIM_UP_VELOCITY;
        velocity.y = std::max(velocity.y, WATER_TERMINAL);
        m_falling = false; // water cancels fall damage
    }
    else
    {
        const bool wasOnGround = onGround;
        velocity.y += GRAVITY * deltaTime;
        velocity.y = std::max(velocity.y, TERMINAL_VELOCITY);

        // Held rather than pressed, so holding the key bunny-hops the way
        // Minecraft does instead of jumping once and stopping.
        if (controls.jumpHeld && wasOnGround)
        {
            velocity.y = JUMP_VELOCITY;
            m_jumped = true;
            m_falling = true;
            m_fallStartY = position.y;
            addExhaustion(sprinting ? 0.2f : 0.05f);
        }

        // Started descending without jumping (walked off a ledge).
        if (!m_falling && !wasOnGround && velocity.y < 0.0f)
        {
            m_falling = true;
            m_fallStartY = position.y;
        }
        if (m_falling && velocity.y > 0.0f)
            m_fallStartY = std::max(m_fallStartY, position.y);
    }

    onGround = false;

    // Resolve one axis at a time so sliding along a wall only cancels the
    // component that is actually blocked.
    const bool ledgeGuard = sneaking && !flying;
    moveAxis(world, velocity.x * deltaTime, 0, ledgeGuard);
    moveAxis(world, velocity.y * deltaTime, 1, false);
    moveAxis(world, velocity.z * deltaTime, 2, ledgeGuard);

    if (!onGround && !flying && !inWater && !m_falling && velocity.y < 0.0f)
    {
        m_falling = true;
        m_fallStartY = position.y;
    }

    // Falling out of the world shouldn't trap you forever.
    if (position.y < -8.0f && !creative())
        damage(2);

    if (onGround && !flying)
    {
        const glm::vec3 moved = position - m_lastPosition;
        const float distance = glm::length(glm::vec3(moved.x, 0.0f, moved.z));
        if (distance > 0.0f)
            addExhaustion(distance * (sprinting ? 0.1f : sneaking ? 0.0f : 0.01f));
    }
    m_lastPosition = position;
}
