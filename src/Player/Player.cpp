#include "Player.h"
#include "Game/Experience.h"
#include "Game/Enchanting.h"
#include "Game/Armour.h"
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

    // How high a rise you walk up rather than stop against. Just over
    // half a block, so a slab or a stair is walked onto and a whole
    // block still wants a jump, which is Minecraft's rule.
    constexpr float STEP_HEIGHT = 0.55f;

    // Climbing. Slower than walking, and the slide is what you do when
    // you are on a ladder and not pressing anything.
    constexpr float CLIMB_SPEED = 3.0f;
    constexpr float CLIMB_SLIDE = 1.2f;
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
    // One row lower than the box, because a fence stands half a block
    // proud of its own cell and would otherwise be stepped over.
    const int minY = static_cast<int>(std::floor(box.min.y)) - 1;
    const int maxY = static_cast<int>(std::floor(box.max.y - EPS));
    const int minZ = static_cast<int>(std::floor(box.min.z));
    const int maxZ = static_cast<int>(std::floor(box.max.z - EPS));

    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
            {
                // A slab fills half its cell and a stair fills two
                // corners of one, so the question is whether the solid
                // part of the block overlaps the box -- not whether the
                // cell is occupied at all.
                if (blockOverlaps(world.getBlock(x, y, z), x, y, z,
                                  box.min.x, box.min.y, box.min.z,
                                  box.max.x, box.max.y, box.max.z))
                    return true;
            }

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

            // Walking up a low rise rather than stopping dead against
            // it. Without this a slab is a wall: you have to jump onto
            // half a block, which is not how any of it is meant to feel.
            if (axis != 1 && onGround && !flying && !m_falling &&
                stepUp(world, next))
            {
                remaining -= move;
                continue;
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

// Tries to lift the player onto whatever is in the way. Takes the step
// only when there is room at the new height both where they are going
// and where they stand -- otherwise walking into a wall under a low
// ceiling would push them up into it.
bool Player::stepUp(const World& world, const glm::vec3& blocked)
{
    for (float lift = COLLISION_STEP; lift <= STEP_HEIGHT; lift += COLLISION_STEP)
    {
        const glm::vec3 raised(blocked.x, position.y + lift, blocked.z);
        if (collides(world, aabbAt(raised))) continue;

        // Room to rise where we are, as well as where we are going.
        const glm::vec3 here(position.x, position.y + lift, position.z);
        if (collides(world, aabbAt(here))) return false;

        position = raised;
        return true;
    }
    return false;
}

void Player::applyFallDamage(float landingY)
{
    if (creative() || flying) return;

    const float fallDistance = m_fallStartY - landingY;
    if (fallDistance > SAFE_FALL_DISTANCE)
        damage(static_cast<int>(std::floor(fallDistance - SAFE_FALL_DISTANCE)));
}

bool Player::onLadder(const World& world) const
{
    // Either of the two blocks the body occupies will do, so you do not
    // drop off a ladder the moment your head clears the top of it.
    const int x = static_cast<int>(std::floor(position.x));
    const int z = static_cast<int>(std::floor(position.z));
    const int footY = static_cast<int>(std::floor(position.y));
    const int headY = static_cast<int>(std::floor(position.y + HEIGHT * 0.6f));

    return isLadder(world.getBlock(x, footY, z)) || isLadder(world.getBlock(x, headY, z));
}

bool Player::isInWater(const World& world) const
{
    const AABB box = aabb();
    const int minX = static_cast<int>(std::floor(box.min.x));
    const int maxX = static_cast<int>(std::floor(box.max.x - 0.0001f));
    // One row lower than the box, because a fence stands half a block
    // proud of its own cell and would otherwise be stepped over.
    const int minY = static_cast<int>(std::floor(box.min.y)) - 1;
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

    amount = Armour::reduce(amount, armourPoints);

    // Protection takes its slice after the armour has taken its own,
    // which is the order Minecraft applies them in and the reason a
    // protected leather suit is worth more than its points suggest.
    amount = Enchanting::protectionReduced(protection, amount);

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

// Drowning ignores the half-second of mercy that follows an ordinary
// blow: Minecraft drowns you once a second regardless, and routing it
// through damage() would have the immunity swallow most of it and leave
// you able to sit on the bottom indefinitely.
bool Player::breathe(float deltaTime, bool headUnderwater)
{
    if (creative() || health <= 0)
    {
        air = MAX_AIR_SECONDS;
        m_drownTimer = 0.0f;
        return false;
    }

    if (!headUnderwater)
    {
        air = std::min(MAX_AIR_SECONDS, air + deltaTime * AIR_REFILL_RATE);
        m_drownTimer = 0.0f;
        return false;
    }

    if (air > 0.0f)
    {
        air = std::max(0.0f, air - deltaTime);
        m_drownTimer = 0.0f;
        return false;
    }

    m_drownTimer += deltaTime;
    if (m_drownTimer < DROWN_INTERVAL) return false;

    m_drownTimer -= DROWN_INTERVAL;
    health = std::max(0, health - DROWN_DAMAGE);
    damageFlash = 0.4f;
    return true;
}

void Player::respawn(glm::vec3 feetPosition)
{
    position = feetPosition;
    velocity = glm::vec3(0.0f);
    health = MAX_HEALTH;
    hunger = MAX_HUNGER;
    saturation = 5.0f;
    // Everything you had earned, gone. What the corpse leaves behind is
    // spawned by whoever handles the death, before this is called.
    experience = 0;
    experienceOwedHundredths = 0;
    air = MAX_AIR_SECONDS;
    m_drownTimer = 0.0f;
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
    const bool climbing = !flying && !inWater && onLadder(world);
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
    else if (climbing)
    {
        // A ladder holds you where you are unless you ask to move. Up
        // on jump, down on sneak, and a slow slide otherwise -- which
        // is what stops a ladder being a hole you fall down.
        if (controls.jumpHeld) velocity.y = CLIMB_SPEED;
        else if (controls.descend || sneaking) velocity.y = -CLIMB_SPEED;
        else velocity.y = -CLIMB_SLIDE;

        m_falling = false;
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

bool Player::addExperience(int amount)
{
    if (amount <= 0) return false;
    const int before = Experience::levelFor(experience);
    experience += amount;
    return Experience::levelFor(experience) > before;
}

bool Player::addExperienceHundredths(int hundredths)
{
    if (hundredths <= 0) return false;
    experienceOwedHundredths += hundredths;
    const int whole = experienceOwedHundredths / 100;
    if (whole <= 0) return false;
    experienceOwedHundredths -= whole * 100;
    return addExperience(whole);
}
