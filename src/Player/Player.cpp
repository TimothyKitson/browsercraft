#include "Player.h"
#include "Game/Glide.h"
#include "Game/Experience.h"
#include "Game/Enchanting.h"
#include "Game/Armour.h"
#include "Game/Shield.h"
#include "World/World.h"
#include <algorithm>
#include <cmath>

namespace
{
    // Whether there is water next to this cell, which is how a block
    // that grows in the sea tells itself apart from the same block
    // stood on dry land by somebody in creative.
    bool besideWater(const World& world, int x, int y, int z)
    {
        return isWater(world.fluidAt(x + 1, y, z)) ||
               isWater(world.fluidAt(x - 1, y, z)) ||
               isWater(world.fluidAt(x, y, z + 1)) ||
               isWater(world.fluidAt(x, y, z - 1)) ||
               isWater(world.fluidAt(x, y + 1, z));
    }

    // Water, lava, or something standing in water: all of them hold
    // the player up and all of them slow them down.
    bool swimmableAt(const World& world, int x, int y, int z)
    {
        const BlockId block = world.fluidAt(x, y, z);
        if (isLiquid(block)) return true;
        return isWaterPlant(block) && besideWater(world, x, y, z);
    }
}


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
    constexpr float SLOW_FALL_TERMINAL = -9.8f;
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
    if (exempt() || amount <= 0.0f) return;
    m_exhaustion += amount;
}

bool Player::eat(int hungerPoints, float saturationPoints, bool always)
{
    if (!canEat() && !always) return false;

    hunger = std::min(MAX_HUNGER, hunger + hungerPoints);
    saturation = std::min(static_cast<float>(hunger), saturation + saturationPoints);
    return true;
}

void Player::updateVitals(float deltaTime)
{
    if (exempt())
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
        else if (difficulty > 0) hunger = std::max(0, hunger - 1);
    }

    if (difficulty <= 0)
    {
        m_peacefulHeal += deltaTime;
        m_peacefulFeed += deltaTime;
        if (m_peacefulHeal >= 1.0f)
        {
            m_peacefulHeal -= 1.0f;
            if (naturalRegeneration && health > 0 && health < maxHealth()) heal(1);
        }
        if (m_peacefulFeed >= 0.5f)
        {
            m_peacefulFeed -= 0.5f;
            if (hunger < MAX_HUNGER) ++hunger;
        }
    }

    if (hunger > 0) m_starveTimer = 0.0f;
    else
    {
        m_starveTimer += deltaTime;
        if (m_starveTimer >= 4.0f)
        {
            m_starveTimer = 0.0f;
            if (health > 10 || difficulty >= 3 || (health > 1 && difficulty == 2))
            {
                cause = "starve";
                damage(1);
            }
        }
    }

    regenerate(deltaTime);
}

void Player::regenerate(float deltaTime)
{
    if (!naturalRegeneration) return;
    const bool wellFed = exempt() || hunger >= REGEN_HUNGER;
    const bool feasting = !exempt() && hunger >= MAX_HUNGER && saturation > 0.0f;
    if (health > 0 && health < maxHealth() && feasting)
    {
        m_regenTimer += deltaTime;
        if (m_regenTimer >= 0.5f)
        {
            m_regenTimer = 0.0f;
            const float spent = std::min(saturation, 6.0f);
            heal(1);
            addExhaustion(spent);
        }
    }
    else if (health > 0 && health < maxHealth() && damageFlash <= 0.0f && wellFed)
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
        feetPosition + glm::vec3(half.x, height(), half.z)
    };
}

glm::vec3 Player::eyePosition() const
{
    const float eye = pose == Pose::Swimming ? SWIM_EYE_HEIGHT : (pose == Pose::Sneaking || sneaking) ? SNEAK_EYE_HEIGHT : EYE_HEIGHT;
    return position + glm::vec3(0.0f, eye, 0.0f);
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
                const BlockId here = world.getBlock(x, y, z);
                if (here == Blocks::PowderSnow)
                {
                    if (powderSnowFirm && !sneaking && position.y >= static_cast<float>(y + 1) - 0.001f &&
                        box.min.y < static_cast<float>(y + 1) && box.max.y > static_cast<float>(y) &&
                        box.max.x > x && box.min.x < x + 1 && box.max.z > z && box.min.z < z + 1)
                        return true;
                    continue;
                }
                const int links = shapeLinks(here, [&](int dx, int dz) { return world.getBlock(x + dx, y, z + dz); });
                if (blockOverlaps(here, x, y, z,
                                  box.min.x, box.min.y, box.min.z,
                                  box.max.x, box.max.y, box.max.z, links))
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
                const BlockId landing = world.getBlock(static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(next.y)),
                                                       static_cast<int>(std::floor(position.z)));
                if (landing == Blocks::SlimeBlock && !sneaking && velocity.y < -2.0f)
                {
                    m_falling = false;
                    velocity.y = -velocity.y * 0.95f;
                    onGround = false;
                    return;
                }
                const BlockId inside = world.getBlock(static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(position.y - 0.05f)),
                                                      static_cast<int>(std::floor(position.z)));
                if ((isBed(landing) || isBed(inside)) && !sneaking && velocity.y < -2.0f)
                {
                    if (m_falling) applyFallDamage(world, position.y);
                    m_falling = false;
                    velocity.y = -velocity.y * 0.66f;
                    onGround = false;
                    return;
                }
                if (m_falling) applyFallDamage(world, position.y);
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

void Player::applyFallDamage(const World& world, float landingY)
{
    if (exempt() || flying || !fallHurts) return;

    const float fallDistance = m_fallStartY - landingY;
    const float safe = static_cast<float>(attributes.value(Attributes::Kind::SafeFallDistance));
    const float hurtScale = static_cast<float>(attributes.value(Attributes::Kind::FallDamageMultiplier));
    if (fallDistance <= safe || hurtScale <= 0.0f) return;

    // What was landed on can cancel the whole of it.
    const int x = static_cast<int>(std::floor(position.x));
    const int y = static_cast<int>(std::floor(landingY - 0.1f));
    const int z = static_cast<int>(std::floor(position.z));
    const BlockId landed = world.getBlock(x, y, z);
    if (breaksFall(landed)) return;
    if (landed == Blocks::HoneyBlock || isBed(landed))
    {
        cause = "fall";
        damage(static_cast<int>(std::floor((fallDistance - safe) * (landed == Blocks::HoneyBlock ? 0.2f : 0.5f) * hurtScale)));
        return;
    }
    if (landed == Blocks::HayBale || axisBase(landed) == Blocks::HayBale)
    {
        cause = "fall";
        damage(static_cast<int>(std::floor((fallDistance - safe) * 0.2f * hurtScale)));
        return;
    }
    if (landed == Blocks::DripstoneUp)
    {
        cause = "stalagmite";
        damage(static_cast<int>(std::floor((fallDistance + 2.0f - safe) * 2.0f * hurtScale)));
        return;
    }

    cause = fallDistance > 5.0f ? "fallHigh" : "fall";
    damage(static_cast<int>(std::floor((fallDistance - safe) * hurtScale)));
}

bool Player::onLadder(const World& world) const
{
    // Either of the two blocks the body occupies will do, so you do not
    // drop off a ladder the moment your head clears the top of it.
    const int x = static_cast<int>(std::floor(position.x));
    const int z = static_cast<int>(std::floor(position.z));
    const int footY = static_cast<int>(std::floor(position.y));
    const int headY = static_cast<int>(std::floor(position.y + HEIGHT * 0.6f));

    return isClimbable(world.getBlock(x, footY, z)) ||
           isClimbable(world.getBlock(x, headY, z));
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
                if (swimmableAt(world, x, y, z))
                    return true;

    return false;
}

// Water only. Lava used to count here, which meant a fall into it
// drowned the player rather than burning them -- and left fire
// resistance, whose whole point is swimming the stuff, with nothing to
// protect against.
bool Player::isHeadUnderwater(const World& world) const
{
    const glm::vec3 eye = eyePosition();
    const int x = static_cast<int>(std::floor(eye.x));
    const int y = static_cast<int>(std::floor(eye.y));
    const int z = static_cast<int>(std::floor(eye.z));

    const BlockId here = world.fluidAt(x, y, z);
    if (isWater(here))
        return isWater(world.fluidAt(x, y + 1, z)) || eye.y < static_cast<float>(y) + fluidAmount(here) / 9.0f;

    // A blade of seagrass stands in the water rather than beside it,
    // so swimming through one is still swimming. Without this, a kelp
    // forest reads as a row of air pockets: the overlay drops out for
    // a frame every time the eye crosses a frond.
    return isWaterPlant(here) && besideWater(world, x, y, z);
}

bool Player::isInLava(const World& world) const
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
                if (isLava(world.getBlock(x, y, z)))
                    return true;
    {
        const BlockId pot = world.getBlock(static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(position.y)),
                                           static_cast<int>(std::floor(position.z)));
        if (pot == Blocks::LavaCauldron && position.y - std::floor(position.y) < 0.9375f) return true;
    }

    return false;
}

// Standing in a campfire. Kept apart from the lava test because what
// it does is different: lava is a hazard you fall into and a campfire
// is one you walk over, so this only counts the block under your feet
// and the one you are standing in.
bool Player::isInFire(const World& world) const
{
    const AABB box = aabb();
    const int minX = static_cast<int>(std::floor(box.min.x));
    const int maxX = static_cast<int>(std::floor(box.max.x - 0.0001f));
    const int minZ = static_cast<int>(std::floor(box.min.z));
    const int maxZ = static_cast<int>(std::floor(box.max.z - 0.0001f));
    const int footY = static_cast<int>(std::floor(box.min.y));

    for (int x = minX; x <= maxX; ++x)
        for (int z = minZ; z <= maxZ; ++z)
        {
            const BlockId here = world.getBlock(x, footY, z);
            if (isCampfire(here) || isFire(here)) return true;
            if (isFire(world.getBlock(x, footY + 1, z))) return true;
        }

    return false;
}

void Player::ignite(float seconds)
{
    if (exempt() || health <= 0 || Effects::fireImmune(effects)) return;
    float scale = static_cast<float>(attributes.value(Attributes::Kind::BurningTime));
    for (uint32_t set : armourEnchants) scale = std::min(scale, Enchanting::burnScale(set));
    m_fireSeconds = std::max(m_fireSeconds, seconds * scale);
}

bool Player::burn(float deltaTime, bool inLava)
{
    if (exempt() || health <= 0 || Effects::fireImmune(effects) || !fireHurts)
    {
        m_burnTimer = 0.0f;
        m_fireSeconds = 0.0f;
        m_fireTimer = 0.0f;
        return false;
    }

    // Lava sets you alight as well as cooking you, so climbing out of
    // it is not the end of it.
    if (inLava)
    {
        float scale = 1.0f;
        for (uint32_t set : armourEnchants) scale = std::min(scale, Enchanting::burnScale(set));
        m_fireSeconds = LAVA_FIRE_SECONDS * scale;
    }

    if (!inLava)
    {
        if (m_fireSeconds <= 0.0f)
        {
            m_burnTimer = 0.0f;
            m_fireTimer = 0.0f;
            return false;
        }

        // Standing in water puts it out, which is the whole reason to
        // run for the nearest lake.
        m_fireSeconds -= deltaTime;
        m_fireTimer += deltaTime;
        if (m_fireTimer >= FIRE_INTERVAL)
        {
            m_fireTimer -= FIRE_INTERVAL;
            cause = "onFire";
            return damage(FIRE_DAMAGE);
        }
        return false;
    }

    // The first blow lands at once rather than half a second in, so
    // stepping into lava hurts immediately the way it should.
    const bool first = (m_burnTimer == 0.0f);
    m_burnTimer += deltaTime;

    if (first || m_burnTimer >= LAVA_INTERVAL)
    {
        if (!first) m_burnTimer -= LAVA_INTERVAL;
        cause = "lava";
        return damage(LAVA_DAMAGE);
    }

    return false;
}

bool Player::damage(int amount, bool throughArmour)
{
    if (exempt() || amount <= 0 || m_hurtCooldown > 0.0f) return false;

    if (!throughArmour)
        amount = Armour::reduce(amount, armourPoints + static_cast<int>(attributes.value(Attributes::Kind::Armor)),
                                armourToughness + static_cast<int>(attributes.value(Attributes::Kind::ArmorToughness)));

    {
        const std::string why = cause ? cause : "";
        Enchanting::Hurt hurt = Enchanting::Hurt::Plain;
        if (projectileHit) hurt = Enchanting::Hurt::Projectile;
        else if (why == "onFire" || why == "lava" || why == "hotFloor" || why == "inFire" || why == "campfire") hurt = Enchanting::Hurt::Fire;
        else if (why == "explosion" || why == "fireworks" || why == "badRespawnPoint") hurt = Enchanting::Hurt::Blast;
        else if (why == "fall" || why == "fallHigh" || why == "stalagmite" || why == "enderPearl" || why == "flyIntoWall") hurt = Enchanting::Hurt::Fall;
        else if (why == "starve" || why == "kill" || why == "outOfWorld" || why == "sonicBoom") hurt = Enchanting::Hurt::Unprotected;
        projectileHit = false;
        bool suit = false;
        int factor = 0;
        for (uint32_t set : armourEnchants)
        {
            if (!Enchanting::any(set)) continue;
            suit = true;
            factor += Enchanting::protectionFactor(set, hurt);
        }
        if (suit) amount = Enchanting::reducedByFactor(factor, amount);
        else if (hurt != Enchanting::Hurt::Unprotected) amount = Enchanting::protectionReduced(protection, amount);
    }
    const int unresisted = amount;
    amount = Effects::resisted(effects, amount);
    if (unresisted > amount) m_tally.resisted += unresisted - amount;
    if (amount <= 0 && unresisted > 0) return false;

    if (absorption > 0)
    {
        const int soaked = std::min(absorption, amount);
        absorption -= soaked;
        amount -= soaked;
        m_tally.absorbed += soaked;
        if (amount <= 0)
        {
            m_hurtCooldown = HURT_IMMUNITY;
            damageFlash = 0.4f;
            return true;
        }
    }

    m_tally.taken += amount;
    health = std::max(0, health - amount);

    if (effects.has(Effects::Kind::Infested))
    {
        m_infestRoll = m_infestRoll * 1664525u + 1013904223u;
        if (static_cast<float>((m_infestRoll >> 8) % 1000u) < Effects::INFESTED_CHANCE * 1000.0f)
            m_silverfishOwed += 1 + static_cast<int>((m_infestRoll >> 20) & 1u);
    }

    // The totem: the one second chance in the game. Minecraft leaves
    // you on a single heart with regeneration running and the totem
    // gone, and that is the whole of it -- it cannot be saved for
    // later, because it only works in the hand.
    if (health <= 0 && holdingTotem)
    {
        health = 1;
        holdingTotem = false;
        m_totemSpent = true;
        Effects::clear(effects);
        Effects::give(effects, Effects::Kind::Regeneration, 1, 45.0f);
        Effects::give(effects, Effects::Kind::Absorption, 1, 5.0f);
        Effects::give(effects, Effects::Kind::FireResistance, 0, 40.0f);
        absorption = std::max(absorption, 8);
    }

    damageFlash = 0.4f;
    m_hurtCooldown = HURT_IMMUNITY;
    m_exhaustion += 0.1f;
    return true;
}

void Player::heal(int amount)
{
    health = std::min(maxHealth(), health + amount);
}

void Player::takeEffect(Effects::Kind kind, int amplifier, float seconds, float strength)
{
    if (Effects::isInstant(kind))
    {
        const int amount = Effects::instantAmount(kind, amplifier, strength);
        if (amount <= 0 || health <= 0) return;
        if (kind == Effects::Kind::InstantHealth) heal(amount);
        else damage(amount, true);
        return;
    }
    if (exempt() && Effects::isHarmful(kind) && !Effects::isNeutral(kind)) return;
    Effects::give(effects, kind, amplifier, seconds);
}

// Drowning ignores the half-second of mercy that follows an ordinary
// blow: Minecraft drowns you once a second regardless, and routing it
// through damage() would have the immunity swallow most of it and leave
// you able to sit on the bottom indefinitely.
// Poison is the one blow that never kills: Minecraft leaves you on half
// a heart and lets you walk it off, so this floors at one rather than
// going through damage().
bool Player::updateEffects(float deltaTime)
{
    if (health <= 0) { Effects::clear(effects); return false; }

    const Effects::Tick tick = Effects::step(effects, deltaTime);
    if (!effects.has(Effects::Kind::Absorption)) absorption = 0;
    const int healthBefore = health;
    health = std::min(health, maxHealth());
    if (tick.instantHeal > 0) heal(tick.instantHeal);
    if (tick.instantHarm > 0)
    {
        cause = "magic";
        damage(tick.instantHarm, true);
    }
    for (int pulse = 0; pulse < tick.witherPulses && health > 0; ++pulse)
    {
        cause = "wither";
        damage(1, true);
    }
    if (tick.exhaustion > 0.0f) addExhaustion(tick.exhaustion);
    if (tick.feedPulses > 0)
    {
        hunger = std::min(MAX_HUNGER, hunger + tick.feedPulses);
        saturation = std::min(static_cast<float>(hunger), saturation + 2.0f * static_cast<float>(tick.feedPulses));
    }
    if (tick.healPulses == 0 && tick.poisonPulses == 0) return health != healthBefore;

    const int before = healthBefore;

    if (tick.healPulses > 0) heal(tick.healPulses);
    if (tick.poisonPulses > 0 && !exempt())
        health = std::max(1, health - tick.poisonPulses);

    return health != before;
}

bool Player::breathe(float deltaTime, bool headUnderwater)
{
    if (exempt() || health <= 0)
    {
        air = MAX_AIR_SECONDS;
        m_drownTimer = 0.0f;
        return false;
    }

    // Water breathing fills the lungs as fast under as over, so a dive
    // never starts the drowning clock at all.
    if (!headUnderwater || Effects::breathesWater(effects))
    {
        air = std::min(MAX_AIR_SECONDS, air + deltaTime * AIR_REFILL_RATE);
        m_drownTimer = 0.0f;
        return false;
    }

    if (air > 0.0f)
    {
        const float held = static_cast<float>(Enchanting::levelOf(armourEnchants[0], Enchanting::Kind::Respiration));
        air = std::max(0.0f, air - deltaTime / (1.0f + held + static_cast<float>(attributes.value(Attributes::Kind::OxygenBonus))));
        m_drownTimer = 0.0f;
        return false;
    }

    if (!drowningHurts)
    {
        m_drownTimer = 0.0f;
        return false;
    }
    m_drownTimer += deltaTime;
    if (m_drownTimer < DROWN_INTERVAL) return false;
    cause = "drown";

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
    if (spectator()) flying = true;
    if (damageFlash > 0.0f) damageFlash = std::max(0.0f, damageFlash - deltaTime);
    if (m_hurtCooldown > 0.0f) m_hurtCooldown = std::max(0.0f, m_hurtCooldown - deltaTime);

    updateVitals(deltaTime);

    sneaking = controls.sneak && !flying;
    const bool inWater = isInWater(world);
    const bool climbing = !flying && !inWater && onLadder(world);
    sprinting = controls.sprint && !sneaking && glm::length(controls.wishDirection) > 0.1f &&
                (exempt() || hunger > SPRINT_HUNGER);

    if (spectator() || flying || gliding) pose = Pose::Standing;
    else
    {
        const bool submerged = isHeadUnderwater(world);
        Pose wanted = Pose::Standing;
        if (sprinting && inWater && (pose == Pose::Swimming || submerged)) wanted = Pose::Swimming;
        else if (sneaking) wanted = Pose::Sneaking;
        auto fits = [&](float tall) {
            const glm::vec3 half(WIDTH * 0.5f, 0.0f, WIDTH * 0.5f);
            return !collides(world, AABB{ position - half + glm::vec3(0.0f, 0.01f, 0.0f), position + glm::vec3(half.x, tall, half.z) });
        };
        if (wanted == Pose::Standing && !fits(HEIGHT)) wanted = fits(SNEAK_HEIGHT) ? Pose::Sneaking : Pose::Swimming;
        if (wanted == Pose::Sneaking && !fits(SNEAK_HEIGHT)) wanted = Pose::Swimming;
        pose = wanted;
        if (pose == Pose::Swimming && !inWater) sprinting = false;
    }

    // Wings. The rules and the flying are both Glide's, so a descent
    // can be flown in a test without a world to fly it over.
    {
        Glide::Conditions wings;
        wings.wearingElytra = controls.wearingElytra;
        wings.onGround = onGround;
        wings.inWater = inWater;
        wings.climbing = climbing;
        wings.flying = flying;
        wings.jumpPressed = controls.jump;
        wings.alreadyGliding = gliding;
        wings.velocityY = velocity.y;
        gliding = Glide::holds(wings);
    }

    // --- choose a target horizontal velocity ---
    float targetSpeed;
    if (flying) targetSpeed = (controls.sprint ? FLY_SPRINT_SPEED : FLY_SPEED) * (spectator() ? flySpeedScale : 1.0f);
    else if (inWater && pose == Pose::Swimming)
        targetSpeed = 5.6f * Effects::swimMultiplier(effects);
    else if (pose == Pose::Swimming)
        targetSpeed = SNEAK_SPEED;
    else if (inWater)
    {
        const float strider = static_cast<float>(std::min(3, Enchanting::levelOf(armourEnchants[3], Enchanting::Kind::DepthStrider))) / 3.0f;
        const float landPace = sprinting ? SPRINT_SPEED : WALK_SPEED;
        targetSpeed = (SWIM_SPEED + (landPace - SWIM_SPEED) * strider) * Effects::swimMultiplier(effects);
    }
    else if (sneaking)
        targetSpeed = std::min(WALK_SPEED, SNEAK_SPEED * (1.0f + 0.5f * static_cast<float>(Enchanting::levelOf(armourEnchants[2], Enchanting::Kind::SwiftSneak))));
    else if (sprinting) targetSpeed = SPRINT_SPEED;
    else targetSpeed = WALK_SPEED;

    // Speed and slowness scale whatever the mode settled on, so they
    // read the same walking, sprinting or swimming.
    targetSpeed *= Effects::speedMultiplier(effects);
    if (flying) targetSpeed *= static_cast<float>(attributes.ratio(Attributes::Kind::FlyingSpeed));
    else
    {
        targetSpeed *= static_cast<float>(attributes.ratio(Attributes::Kind::MovementSpeed));
        if (sneaking && pose == Pose::Sneaking) targetSpeed *= static_cast<float>(attributes.ratio(Attributes::Kind::SneakingSpeed));
    }

    // A raised shield slows you to a walk. Without that cost it would
    // simply be worn at all times.
    if (blocking && !flying) targetSpeed *= Shield::RAISED_SPEED;

    inBerryBush = false;
    inPowderSnow = false;
    inCobweb = false;
    if (!flying)
    {
        const AABB box = aabb();
        for (int bx = static_cast<int>(std::floor(box.min.x)); bx <= static_cast<int>(std::floor(box.max.x)); ++bx)
            for (int by = static_cast<int>(std::floor(box.min.y)); by <= static_cast<int>(std::floor(box.max.y - 0.001f)); ++by)
                for (int bz = static_cast<int>(std::floor(box.min.z)); bz <= static_cast<int>(std::floor(box.max.z)); ++bz)
                    if (world.getBlock(bx, by, bz) == Blocks::PowderSnow) inPowderSnow = true;
        if (inPowderSnow) targetSpeed *= 0.45f;
        for (int bx = static_cast<int>(std::floor(box.min.x)); bx <= static_cast<int>(std::floor(box.max.x)); ++bx)
            for (int by = static_cast<int>(std::floor(box.min.y)); by <= static_cast<int>(std::floor(box.max.y - 0.001f)); ++by)
                for (int bz = static_cast<int>(std::floor(box.min.z)); bz <= static_cast<int>(std::floor(box.max.z)); ++bz)
                    if (world.getBlock(bx, by, bz) == Blocks::Cobweb) inCobweb = true;
        if (inCobweb) targetSpeed *= Effects::cobwebDrag(effects);
        for (int bx = static_cast<int>(std::floor(box.min.x)); bx <= static_cast<int>(std::floor(box.max.x)); ++bx)
            for (int by = static_cast<int>(std::floor(box.min.y)); by <= static_cast<int>(std::floor(box.max.y)); ++by)
                for (int bz = static_cast<int>(std::floor(box.min.z)); bz <= static_cast<int>(std::floor(box.max.z)); ++bz)
                    if (isSweetBerryBush(world.getBlock(bx, by, bz))) inBerryBush = true;
        if (inBerryBush) targetSpeed *= 0.35f;
        const BlockId under = world.getBlock(static_cast<int>(std::floor(position.x)),
                                             static_cast<int>(std::floor(position.y - 0.1f)),
                                             static_cast<int>(std::floor(position.z)));
        const int soulSpeed = Enchanting::levelOf(armourEnchants[3], Enchanting::Kind::SoulSpeed);
        if (soulSpeed > 0 && (under == Blocks::SoulSand || under == Blocks::SoulSoil))
            targetSpeed *= 1.3f + 0.105f * static_cast<float>(soulSpeed);
        else if (under == Blocks::HoneyBlock || under == Blocks::SoulSand) targetSpeed *= 0.4f;
    }

    const float lean = (pose == Pose::Swimming && inWater) ? std::sqrt(std::max(0.0f, 1.0f - controls.look.y * controls.look.y)) : 1.0f;
    const glm::vec3 targetVelocity = controls.wishDirection * targetSpeed * lean;

    float acceleration;
    if (flying) acceleration = 16.0f;
    else if (inWater) acceleration = WATER_ACCELERATION;
    else if (onGround) acceleration = GROUND_ACCELERATION;
    else acceleration = AIR_ACCELERATION;
    if (onGround && !flying && !inWater)
    {
        const BlockId floorBlock = world.getBlock(static_cast<int>(std::floor(position.x)),
                                                  static_cast<int>(std::floor(position.y - 0.1f)),
                                                  static_cast<int>(std::floor(position.z)));
        if (isSlippery(floorBlock)) acceleration = floorBlock == Blocks::BlueIce ? 0.8f : 1.6f;
    }

    if (inPowderSnow && !freezeImmune && !spectator() && !creative())
        frozenSeconds = std::min(FREEZE_SECONDS, frozenSeconds + deltaTime);
    else
        frozenSeconds = std::max(0.0f, frozenSeconds - 2.0f * deltaTime);

    // A glide steers itself off where you are looking; the walking
    // keys do nothing to it. Done before the blend below so the
    // ordinary air control does not fight it.
    if (gliding)
    {
        velocity = Glide::step(velocity, controls.look, deltaTime);

        // A glide is not a fall. Landing out of one hurts only as far
        // as you drop after it ends.
        m_falling = false;
        m_fallStartY = position.y;
    }
    else
    {
        const float blend = std::clamp(acceleration * deltaTime, 0.0f, 1.0f);
        velocity.x += (targetVelocity.x - velocity.x) * blend;
        velocity.z += (targetVelocity.z - velocity.z) * blend;
    }

    // --- vertical motion ---
    if (gliding)
    {
        // Already done above: a glide owns its own vertical motion.
        onGround = false;
    }
    else if (flying)
    {
        float verticalTarget = 0.0f;
        if (controls.jumpHeld) verticalTarget += targetSpeed;
        if (controls.descend) verticalTarget -= targetSpeed;
        velocity.y += (verticalTarget - velocity.y) * std::clamp(16.0f * deltaTime, 0.0f, 1.0f);
        onGround = false;
        m_falling = false;
    }
    else if (!onGround && velocity.y < -1.0f && [&] {
                 const AABB box = aabb();
                 for (int by = static_cast<int>(std::floor(box.min.y)); by <= static_cast<int>(std::floor(box.max.y - 0.001f)); ++by)
                 {
                     const int x0 = static_cast<int>(std::floor(box.min.x - 0.07f)), x1 = static_cast<int>(std::floor(box.max.x + 0.07f));
                     const int z0 = static_cast<int>(std::floor(box.min.z - 0.07f)), z1 = static_cast<int>(std::floor(box.max.z + 0.07f));
                     for (int bx = x0; bx <= x1; ++bx)
                         for (int bz = z0; bz <= z1; ++bz)
                             if (world.getBlock(bx, by, bz) == Blocks::HoneyBlock) return true;
                 }
                 return false;
             }())
    {
        velocity.y = -1.0f;
        m_falling = false;
    }
    else if (inWater && pose == Pose::Swimming)
    {
        const float pace = 5.6f * Effects::swimMultiplier(effects);
        velocity.y += (controls.look.y * pace - velocity.y) * std::clamp(6.0f * deltaTime, 0.0f, 1.0f);
        m_falling = false;
    }
    else if (inWater)
    {
        velocity.y += WATER_GRAVITY * deltaTime;
        if (controls.jumpHeld) velocity.y = SWIM_UP_VELOCITY;
        velocity.y = std::max(velocity.y, WATER_TERMINAL);
        const int bubbles = world.bubbleColumnAt(static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(position.y + 0.1f)),
                                                 static_cast<int>(std::floor(position.z)));
        if (bubbles > 0) velocity.y = std::min(std::max(velocity.y, 0.0f) + 40.0f * deltaTime, 10.0f);
        else if (bubbles < 0) velocity.y = std::max(std::min(velocity.y, 0.0f) - 14.0f * deltaTime, -6.0f);
        m_falling = false; // water cancels fall damage
    }
    else if (inCobweb)
    {
        const float web = effects.has(Effects::Kind::Weaving) ? 5.0f : 1.0f;
        velocity.y = controls.jumpHeld ? 0.6f * web : std::max(velocity.y + GRAVITY * deltaTime * 0.05f * web, -0.8f * web);
        m_falling = false;
    }
    else if (inPowderSnow)
    {
        if (controls.jumpHeld && powderSnowFirm) velocity.y = 1.8f;
        else velocity.y = std::max(velocity.y + GRAVITY * deltaTime, -1.0f);
        m_falling = false;
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
        const float rise = Effects::levitationSpeed(effects);
        if (rise > 0.0f)
        {
            velocity.y += (rise - velocity.y) * (1.0f - std::pow(0.8f, deltaTime * 20.0f));
            m_falling = false;
            m_fallStartY = position.y;
        }
        else if (Effects::fallsSlowly(effects) && velocity.y <= 0.0f)
        {
            velocity.y = std::max(velocity.y + GRAVITY * 0.125f * deltaTime, SLOW_FALL_TERMINAL);
            m_falling = false;
            m_fallStartY = position.y;
        }
        else
        {
            velocity.y += GRAVITY * static_cast<float>(attributes.ratio(Attributes::Kind::Gravity)) * deltaTime;
            velocity.y = std::max(velocity.y, TERMINAL_VELOCITY);
        }

        // Held rather than pressed, so holding the key bunny-hops the way
        // Minecraft does instead of jumping once and stopping.
        if (controls.jumpHeld && wasOnGround)
        {
            velocity.y = JUMP_VELOCITY * Effects::jumpMultiplier(effects) * static_cast<float>(attributes.ratio(Attributes::Kind::JumpStrength));
            if (world.getBlock(static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(position.y - 0.1f)),
                               static_cast<int>(std::floor(position.z))) == Blocks::HoneyBlock)
                velocity.y *= 0.5f;
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
    if (spectator())
        position += velocity * deltaTime;
    else
    {
        const float flatBefore = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
        moveAxis(world, velocity.x * deltaTime, 0, ledgeGuard);
        moveAxis(world, velocity.y * deltaTime, 1, false);
        moveAxis(world, velocity.z * deltaTime, 2, ledgeGuard);
        const float flatAfter = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
        const int kinetic = static_cast<int>((flatBefore - flatAfter) * 0.5f - 3.0f);
        if (gliding && kinetic > 0 && !exempt())
        {
            cause = "flyIntoWall";
            damage(kinetic);
        }
    }

    if (!onGround && !flying && !inWater && !m_falling && velocity.y < 0.0f)
    {
        m_falling = true;
        m_fallStartY = position.y;
    }

    // Falling out of the world shouldn't trap you forever.
    if (position.y < -8.0f && !exempt())
    {
        cause = "outOfWorld";
        damage(2);
    }

    if (onGround && !flying && !inWater)
    {
        const glm::vec3 moved = position - m_lastPosition;
        const float distance = glm::length(glm::vec3(moved.x, 0.0f, moved.z));
        if (distance > 0.0f)
            addExhaustion(distance * (sprinting ? 0.1f : sneaking ? 0.0f : 0.01f));
    }
    else if (inWater && !flying)
    {
        const float distance = glm::length(position - m_lastPosition);
        if (distance > 0.0f && distance < 2.0f) addExhaustion(distance * 0.01f);
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
