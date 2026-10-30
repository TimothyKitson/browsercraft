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
    , m_birthSeed(seed ? seed : 1u)
{
    if (baby) m_babyTimer = BABY_SECONDS;
    const MobType& t = mobType(type);
    m_health = t.maxHealth;
    m_yaw = random01() * 360.0f;

    // A shulker is a block that happens to be alive. It goes in the
    // middle of the one it was put in and it faces the way every other
    // block does, which is to say it does not face at all.
    if (t.stationary)
    {
        m_position = Shulker::onGrid(m_position);
        m_yaw = 0.0f;
    }
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
// Held at the waterline. Pulled towards the surface in proportion to how
// far off it is and damped on the way, which settles rather than bobbing;
// returns whether there was any water under it to float on at all.
bool Mob::floatTowardsSurface(const World& world, float deltaTime)
{
    const int x = static_cast<int>(std::floor(m_position.x));
    const int z = static_cast<int>(std::floor(m_position.z));
    const int feet = static_cast<int>(std::floor(m_position.y + 0.05f));

    // What this one rides on. A strider stands on lava exactly the
    // way a boat sits on water, so it is the same arithmetic over a
    // different block rather than a second copy of it.
    const BlockId liquid = type().lavaWalker ? Blocks::Lava : Blocks::Water;

    if (world.getBlock(x, feet, z) != liquid) return false;

    // The top of the liquid it is sitting in, so a hull dropped into a
    // deep lake rises to the surface instead of resting on the bed.
    int surface = feet;
    while (surface < feet + FLOAT_RISE && world.getBlock(x, surface + 1, z) == liquid)
        ++surface;

    const float waterline = static_cast<float>(surface + 1) - FLOAT_DRAFT;

    m_velocity.y = verticalStep(m_velocity.y, m_position.y, waterline, true, deltaTime);
    return true;
}

float Mob::verticalStep(float velocityY, float positionY, float target,
                        bool held, float pull, float damping, float deltaTime)
{
    // Held at a height -- the waterline for a hull, the air for a blaze
    // -- gravity has no part in it. The first try added the spring on
    // top of the fall, which settles wherever the two cancel: a whole
    // block under, as it turned out, with the hull showing through the
    // surface as a blue smudge.
    if (held)
    {
        velocityY += (target - positionY) * pull * deltaTime;
        return velocityY * std::max(0.0f, 1.0f - damping * deltaTime);
    }

    return std::max(velocityY + GRAVITY * deltaTime, TERMINAL_VELOCITY);
}

// Held in the air above whatever it is over. The same spring as the
// waterline and for the same reason: there is a height it wants and
// nothing else decides where it sits. Returns false over a bottomless
// drop, which is the one case where it should simply fall.
bool Mob::hoverAboveGround(const World& world, float deltaTime)
{
    const int x = static_cast<int>(std::floor(m_position.x));
    const int z = static_cast<int>(std::floor(m_position.z));
    const int feet = static_cast<int>(std::floor(m_position.y));

    int ground = feet;
    while (ground > feet - HOVER_DROP && !isSolid(world.getBlock(x, ground - 1, z)))
        --ground;

    if (!isSolid(world.getBlock(x, ground - 1, z))) return false;

    const float wanted = static_cast<float>(ground) + type().hoverHeight;
    m_velocity.y = verticalStep(m_velocity.y, m_position.y, wanted, true,
                                HOVER_PULL, HOVER_DAMPING, deltaTime);
    return true;
}

float Mob::railSpeedStep(float speed, bool pushing, float maxSpeed, float deltaTime)
{
    speed += (pushing ? RAIL_PUSH : -RAIL_DRAG) * deltaTime;
    return std::clamp(speed, 0.0f, maxSpeed);
}

glm::vec3 Mob::railDirection(int facing)
{
    switch (facing & 3)
    {
        case 0:  return glm::vec3(0.0f, 0.0f, -1.0f);   // north
        case 1:  return glm::vec3(1.0f, 0.0f, 0.0f);    // east
        case 2:  return glm::vec3(0.0f, 0.0f, 1.0f);    // south
        default: return glm::vec3(-1.0f, 0.0f, 0.0f);   // west
    }
}

float Mob::railYaw(int facing)
{
    switch (facing & 3)
    {
        case 0:  return -90.0f;
        case 1:  return 0.0f;
        case 2:  return 90.0f;
        default: return 180.0f;
    }
}

bool Mob::railGuide(BlockId rail, const glm::ivec3& cell, const glm::vec3& position,
                    float yawDegrees, glm::vec3& snapped, float& exitYaw)
{
    if (!isRail(rail)) return false;

    int ends[2] = { 0, 0 };
    int found = 0;
    for (int d = 0; d < 4 && found < 2; ++d)
        if (railLinks(rail, d)) ends[found++] = d;
    if (found < 2) return false;

    const float radians = glm::radians(yawDegrees);
    const glm::vec3 heading(std::cos(radians), 0.0f, std::sin(radians));

    // Out by whichever end it is already pointed at. A cart reversed
    // into a bend leaves the way it came, which is what a cart does.
    int best = ends[0];
    float bestDot = -2.0f;
    for (int i = 0; i < 2; ++i)
    {
        const float dot = glm::dot(railDirection(ends[i]), heading);
        if (dot > bestDot) { bestDot = dot; best = ends[i]; }
    }

    exitYaw = railYaw(best);

    // Held on the track: whatever of its position lies across the rail
    // is pulled back to the middle of the cell, and what lies along it
    // is left alone so the cart can actually travel.
    const glm::vec3 along = railDirection(best);
    snapped = position;
    if (std::fabs(along.x) > 0.5f) snapped.z = static_cast<float>(cell.z) + 0.5f;
    else                           snapped.x = static_cast<float>(cell.x) + 0.5f;
    snapped.y = static_cast<float>(cell.y) + RAIL_LIFT;

    return true;
}

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

StackId Mob::tamingFood(MobId type)
{
    switch (type)
    {
        case MobId::Wolf:  return Items::Bone;
        // Minecraft tames a horse by being thrown off it until it gives
        // in. Wheat is the same bargain with fewer broken bones, and it
        // is what the horse already wants.
        case MobId::Horse: return Items::Wheat;
        default:           return Blocks::Air;
    }
}

void Mob::setRidden(bool on)
{
    m_ridden = on;
    if (!on)
    {
        m_steerForward = false;
        m_steerJump = false;
    }
    // Whatever it was doing before, it is not doing it now.
    m_path.clear();
    m_pathIndex = 0;
    m_huntTimer = 0.0f;
}

void Mob::steer(float yawDegrees, bool forward, bool jump)
{
    m_goalYaw = yawDegrees;
    m_steerForward = forward;
    m_steerJump = jump;
}

void Mob::huntTowards(const glm::vec3& at)
{
    const glm::vec3 toward = at - m_position;
    const float away = glm::length(glm::vec2(toward.x, toward.z));
    if (away < 0.001f) return;

    m_goalYaw = glm::degrees(std::atan2(toward.z, toward.x));
    m_moving = away > BITE_REACH * 0.6f;
    m_goalTimer = 0.4f;

    // Held briefly, so the wolf keeps going at it between the passes
    // that pick its quarry rather than stopping to wander in between.
    m_huntTimer = 1.0f;
}

bool Mob::tryBite(float deltaTime)
{
    if (m_biteTimer > 0.0f)
    {
        m_biteTimer = std::max(0.0f, m_biteTimer - deltaTime);
        return false;
    }

    m_biteTimer = BITE_INTERVAL;
    emit(type().voice, 0.8f, 1.1f);
    return true;
}

bool Mob::tameWith(StackId food)
{
    const StackId wants = tamingFood(m_type);
    if (wants == Blocks::Air || food != wants) return false;
    if (!alive() || m_tamed) return false;

    m_tamed = true;
    m_panicTimer = 0.0f;   // it has no reason to run from you now
    emit(type().voice, 0.8f, 1.3f);
    return true;
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

void Mob::setHome(const glm::vec3& where, float radius)
{
    m_home = where;
    m_homeRadius = std::max(0.0f, radius);
}

bool Mob::wouldTurnBack(const glm::vec3& at, const glm::vec3& home, float radius)
{
    if (radius <= 0.0f) return false;
    const glm::vec3 away = home - at;
    return glm::length(glm::vec2(away.x, away.z)) > radius;
}

void Mob::makeWilling()
{
    if (m_breedTimer > 0.0f || inLove() || baby() || !alive()) return;
    m_loveTimer = LOVE_SECONDS;
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

bool Mob::takeEggReport()
{
    if (!m_eggReady) return false;
    m_eggReady = false;
    return true;
}

float Mob::fuse() const
{
    const MobType& t = type();
    if (!m_lit || t.fuseSeconds <= 0.0f) return 0.0f;
    return std::clamp(1.0f - m_fuseTimer / t.fuseSeconds, 0.0f, 1.0f);
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
    if (hostileNow() && distance < NOTICE_RANGE && distance > 0.001f)
    {
        m_goalYaw = glm::degrees(std::atan2(toPlayer.z, toPlayer.x));
        m_moving = true;
        m_goalTimer = 0.5f;
        m_chasing = true;
        return;
    }

    m_chasing = false;

    if (!m_tamed && t.spawnClass == SpawnClass::Passive &&
        distance < FLEE_RANGE && distance > 0.001f)
    {
        m_goalYaw = glm::degrees(std::atan2(-toPlayer.z, -toPlayer.x));
        m_moving = true;
        m_goalTimer = 1.0f + random01();
        return;
    }

    // Set on something: whoever keeps the mobs has pointed it at a
    // quarry, and that beats heeling. Without this the wolf would turn
    // back to its owner between every bite.
    if (hunting()) return;

    // A tamed one keeps up -- but only one you walk beside. A horse
    // follows nobody: you leave it where you got off it, and finding
    // it there again is the whole arrangement. One that trailed after
    // you on foot would never be anywhere you left it.
    if (m_tamed && !t.rideable && distance > 3.0f && distance < 64.0f)
    {
        m_goalYaw = glm::degrees(std::atan2(toPlayer.z, toPlayer.x));
        m_moving = distance > 2.5f;
        m_goalTimer = 0.5f;
        return;
    }

    // Strayed too far from where it belongs: head back. This comes
    // before the idle wander, which is what used to walk villagers out
    // of their village a step at a time and never bring them home.
    if (wouldTurnBack(m_position, m_home, m_homeRadius))
    {
        const glm::vec3 back = m_home - m_position;
        m_goalYaw = glm::degrees(std::atan2(back.z, back.x));
        m_moving = true;
        m_goalTimer = 1.5f + random01() * 2.0f;
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
    if (m_hurtVoiceTimer > 0.0f) m_hurtVoiceTimer = std::max(0.0f, m_hurtVoiceTimer - deltaTime);
    if (m_panicTimer > 0.0f) m_panicTimer = std::max(0.0f, m_panicTimer - deltaTime);
    if (m_loveTimer > 0.0f) m_loveTimer = std::max(0.0f, m_loveTimer - deltaTime);
    if (m_breedTimer > 0.0f) m_breedTimer = std::max(0.0f, m_breedTimer - deltaTime);
    if (m_huntTimer > 0.0f) m_huntTimer = std::max(0.0f, m_huntTimer - deltaTime);
    if (m_biteTimer > 0.0f) m_biteTimer = std::max(0.0f, m_biteTimer - deltaTime);
    if (m_babyTimer > 0.0f) m_babyTimer = std::max(0.0f, m_babyTimer - deltaTime);
    m_age += deltaTime;

    // The lid. It opens when there is something to shoot at and shuts
    // again when there is not, and a shut shulker is a block: it does
    // not fire, and there is nothing of it showing to hit.
    if (t.stationary && alive())
    {
        const glm::vec3 toPlayer = playerPosition - m_position;
        m_peek = Shulker::peekTowards(m_peek, glm::length(toPlayer) < t.arrowRange,
                                      deltaTime);
    }

    // A grown chicken lays an egg every few minutes. The clock runs
    // whether anybody is watching or not, which is why a flock left
    // alone is worth coming back to.
    if (m_type == MobId::Chicken && alive() && !baby())
    {
        m_eggTimer -= deltaTime;
        if (m_eggTimer <= 0.0f)
        {
            m_eggTimer = EGG_SECONDS;
            m_eggReady = true;
        }
    }

    if (!alive())
    {
        m_removeTimer -= deltaTime;
        return;
    }

    burnInSunlight(deltaTime, world, daylight);
    if (!alive()) return;

    if (m_repathTimer > 0.0f) m_repathTimer = std::max(0.0f, m_repathTimer - deltaTime);

    // Villagers turn in when the light goes. Asleep they stand where
    // they are rather than walking to a bed -- the bed is what the
    // village counts, not what they lie on -- but a village standing
    // still at night is what makes it read as asleep.
    // Anything that has just been hurt is awake, whatever the hour: a
    // villager that stood still while a zombie ate it would be worse
    // than one that never slept.
    m_asleep = sleepsAt(m_type, daylight, m_panicTimer > 0.0f);
    if (m_asleep)
    {
        m_moving = false;
        m_path.clear();
    }

    if (m_ridden)
    {
        // Ridden: it goes where it is pointed and nowhere else. No
        // goal of its own, and no route -- a mob that kept pathing
        // while being ridden fought the reins.
        m_moving = m_steerForward;

        // A boat has no legs to push off with, so it does not jump.
        if (m_steerJump && m_onGround && !t.inanimate) m_velocity.y = RIDDEN_JUMP;
        m_steerJump = false;
    }
    else if (t.inanimate || t.stationary || m_sitting)
    {
        // Left alone it stays where it was put. A boat has nothing to
        // want; a sitting wolf has been told; a shulker is bolted on.
        m_moving = false;
        m_path.clear();
    }
    else
    {
        m_goalTimer -= deltaTime;
        if (!m_asleep && m_goalTimer <= 0.0f) chooseNewGoal(playerPosition);

        // A route, where there is one, overrules the heading just chosen.
        followPath();
    }

    // On rails the track does the steering: the rider only chooses
    // whether the cart rolls, never where it goes.
    bool onRail = false;
    if (t.ridesRails)
    {
        const int cx = static_cast<int>(std::floor(m_position.x));
        const int cz = static_cast<int>(std::floor(m_position.z));

        // The cell the wheels are in, and failing that the one under
        // it: a cart sits a sixteenth proud of its rail, so which of
        // the two it rounds into depends on where in the step it is.
        int cy = static_cast<int>(std::floor(m_position.y + 0.1f));
        BlockId rail = world.getBlock(cx, cy, cz);
        if (!isRail(rail)) { --cy; rail = world.getBlock(cx, cy, cz); }

        glm::vec3 snapped(0.0f);
        float exitYaw = m_yaw;
        if (railGuide(rail, glm::ivec3(cx, cy, cz), m_position, m_yaw, snapped, exitYaw))
        {
            m_position = snapped;
            m_velocity.y = 0.0f;

            // Set rather than turned towards: a cart takes a bend at
            // the bend, not over the second after it.
            m_goalYaw = exitYaw;
            m_yaw = exitYaw;
            onRail = true;

            // Rolling is the cart's own business: the rider decides
            // whether to push, and the track decides where that goes.
            m_railSpeed = railSpeedStep(m_railSpeed, m_ridden && m_steerForward,
                                        currentSpeed(), deltaTime);
            m_moving = m_railSpeed > 0.01f;
        }
    }

    // Turn towards the goal heading the short way round. A shulker
    // never does: it is fixed to the block the way a chest is, and one
    // that swivelled to watch you would read as a creature.
    if (t.stationary)
    {
        m_goalYaw = m_yaw;
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
        const glm::vec3 grid = Shulker::onGrid(m_position);
        m_position.x = grid.x;
        m_position.z = grid.z;
    }
    else
    {
        m_yaw = PlayerAnimation::wrapDegrees(
            m_yaw + PlayerAnimation::wrapDegrees(m_goalYaw - m_yaw) *
                        std::min(1.0f, 6.0f * deltaTime));
    }

    const float radians = glm::radians(m_yaw);
    const glm::vec3 forward(std::cos(radians), 0.0f, std::sin(radians));
    const glm::vec3 target = m_moving ? forward * currentSpeed() : glm::vec3(0.0f);

    const float blend = std::min(1.0f, 10.0f * deltaTime);
    m_velocity.x += (target.x - m_velocity.x) * blend;
    m_velocity.z += (target.z - m_velocity.z) * blend;

    // A cart is not eased towards a speed, it simply has one: the rail
    // carries it and nothing else is pushing it sideways.
    if (onRail)
    {
        m_velocity.x = forward.x * m_railSpeed;
        m_velocity.z = forward.z * m_railSpeed;
    }
    else if (t.ridesRails)
    {
        m_railSpeed = 0.0f;   // off the track it keeps nothing
    }

    // What floats is held at the waterline rather than swum: pulled
    // towards the surface and damped, so it settles instead of bobbing
    // for ever. Out of the water gravity has it like anything else, and
    // that is the only branch that applies the fall.
    const bool afloat = (t.floats || t.lavaWalker) && floatTowardsSurface(world, deltaTime);
    const bool hovering = !afloat && t.hoverHeight > 0.0f &&
                          hoverAboveGround(world, deltaTime);
    if (onRail) m_velocity.y = 0.0f;
    else if (!afloat && !hovering)
        m_velocity.y = verticalStep(m_velocity.y, m_position.y, 0.0f, false, deltaTime);

    const bool wasOnGround = m_onGround;
    m_onGround = false;

    const glm::vec3 before = m_position;
    moveAxis(world, m_velocity.x * deltaTime, 0);
    moveAxis(world, m_velocity.y * deltaTime, 1);
    moveAxis(world, m_velocity.z * deltaTime, 2);

    // Neither water nor a rail is a floor, so moveAxis never reports
    // standing on either. Something riding on one is as settled as it
    // is going to get.
    if (afloat || onRail || hovering) m_onGround = true;

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

    if (m_onGround && !t.inanimate)
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

    // The fuse. A creeper lights it when it gets close enough and goes
    // off whether or not you are still there when it finishes -- that is
    // the whole trick of it, and backing away is the answer rather than
    // standing and trading blows.
    if (t.fuseSeconds > 0.0f)
    {
        const glm::vec3 toPlayer = playerPosition - m_position;
        const float flat = glm::length(glm::vec3(toPlayer.x, 0.0f, toPlayer.z));
        const float lightAt = t.blastRadius * 0.55f;

        if (!m_lit && flat < lightAt && std::fabs(toPlayer.y) < 3.0f)
        {
            m_lit = true;
            m_fuseTimer = t.fuseSeconds;
            emit(Sound::Click, 0.9f, 0.6f);
        }
        else if (m_lit && flat > lightAt * 2.0f)
        {
            // Got away. The fuse goes out and can be lit again.
            m_lit = false;
            m_fuseTimer = 0.0f;
        }

        if (m_lit)
        {
            m_fuseTimer -= deltaTime;
            if (m_fuseTimer <= 0.0f)
            {
                MobBlast blast;
                blast.at = m_position + glm::vec3(0.0f, height() * 0.5f, 0.0f);
                blast.radius = t.blastRadius;
                blast.damage = t.blastDamage;
                m_blasts.push_back(blast);

                // It goes with the blast. Nothing left to drop: the
                // gunpowder went up with it.
                m_health = 0;
                m_removeTimer = DEATH_SECONDS;
                return;
            }
        }
    }

    // Shooting. A bow wants a clear line and some distance; up close
    // the arrow is no use and the skeleton backs away instead.
    // A shulker shoots out of an open lid and no other way, so the
    // bullet waits on the animation rather than the other way round.
    if (t.arrowDamage > 0 && (!t.stationary || m_peek >= Shulker::PEEK_OPEN))
    {
        if (m_shootTimer > 0.0f) m_shootTimer = std::max(0.0f, m_shootTimer - deltaTime);

        const glm::vec3 eye = m_position + glm::vec3(0.0f, height() * 0.85f, 0.0f);
        const glm::vec3 aim = playerPosition + glm::vec3(0.0f, 0.9f, 0.0f);
        const glm::vec3 toPlayer = aim - eye;
        const float range = glm::length(toPlayer);

        if (m_shootTimer <= 0.0f && range > 2.5f && range < t.arrowRange &&
            world.hasLineOfSight(eye, aim))
        {
            m_shootTimer = t.arrowInterval;

            // Aimed a little high, so the arrow drops onto the target
            // rather than always falling short. A fire charge does not
            // drop, so it is aimed straight at what it is thrown at.
            const glm::vec3 flat = glm::normalize(toPlayer);
            const bool charge = t.shootsFire || t.shootsWind;
            const float lift = charge ? 0.0f
                                      : std::clamp(range * 0.018f, 0.0f, 0.35f);
            const glm::vec3 heading = glm::normalize(flat + glm::vec3(0.0f, lift, 0.0f));

            MobShot shot;
            shot.from = eye + heading * 0.5f;
            shot.velocity = heading * (charge ? 16.0f : 28.0f);
            shot.damage = t.arrowDamage;
            shot.kind = t.shootsFire ? Arrows::Kind::Fire
                      : t.shootsWind ? Arrows::Kind::Wind
                      : t.throwsPotions ? Arrows::Kind::Potion
                                     : Arrows::Kind::Shaft;
            m_shots.push_back(shot);

            emit(t.shootsFire ? Sound::Fire : Sound::Click, 0.5f,
                 t.shootsWind ? 1.6f : 1.2f);
        }
    }

    m_ambientTimer -= deltaTime;
    if (m_ambientTimer <= 0.0f && !t.inanimate)
    {
        m_ambientTimer = t.ambientMinSeconds + random01() * (t.ambientMaxSeconds - t.ambientMinSeconds);
        emit(t.voice, 0.8f, 0.9f + random01() * 0.2f);
    }
}

bool Stare::atHead(const glm::vec3& eye, const glm::vec3& look,
                  const glm::vec3& feet, float height,
                  float range, float headRadius)
{
    const float lookLength = glm::length(look);
    if (lookLength < 0.001f) return false;
    const glm::vec3 dir = look / lookLength;

    const glm::vec3 head = feet + glm::vec3(0.0f, height - headRadius, 0.0f);
    const glm::vec3 toHead = head - eye;

    const float along = glm::dot(toHead, dir);
    if (along <= 0.0f || along > range) return false;

    return glm::length(toHead - dir * along) <= headRadius;
}

bool Mob::canSit(MobId type, bool tamed)
{
    // Only something that can be tamed at all, and only once it has
    // been: a horse is tamed and will not sit, it is ridden instead.
    if (!tamed || tamingFood(type) == Blocks::Air) return false;
    return !mobType(type).rideable;
}

bool Mob::hostileNow(const MobType& t, bool provoked)
{
    // Anything with no way of hurting you is never hostile, whatever
    // is done to it: a cow that has been hit is a frightened cow.
    // Everything else is, unless it is neutral and has been left alone.
    //
    // This used to ask whether the species spawned as a monster, which
    // was the same question right up until the golem: it spawns in
    // daylight as part of its village and would never have fought back.
    if (t.attackDamage <= 0 && t.arrowDamage <= 0 && t.blastDamage <= 0) return false;
    return !t.neutral || provoked;
}

bool Mob::hostileNow() const
{
    return hostileNow(mobType(m_type), m_provoked);
}

void Mob::damage(int amount, const glm::vec3& fromDirection)
{
    if (amount <= 0 || !alive()) return;

    // Hitting one is a reason, whatever it was doing before. Burning in
    // the sun is not, but that passes no direction and no attacker, and
    // a neutral mob that stands in daylight is not being attacked by
    // anyone it could go after.
    if (glm::length(glm::vec3(fromDirection.x, 0.0f, fromDirection.z)) > 0.001f)
        m_provoked = true;

    m_health = std::max(0, m_health - amount);
    m_hurtFlash = 0.35f;

    // Knocked back and off its feet a little, so a hit reads as a hit.
    // Only when something actually struck it from somewhere, though:
    // burning in the sun passes no direction, and lifting the mob for
    // that had the undead hopping on the spot once a second all dawn.
    const glm::vec3 flat(fromDirection.x, 0.0f, fromDirection.z);
    const float length = glm::length(flat);
    if (length > 0.001f && !type().stationary)
    {
        m_velocity += (flat / length) * 5.0f;
        m_velocity.y = std::max(m_velocity.y, 4.2f);
    }

    // Being hit is a good reason to stop ambling and react. What
    // cannot fight back runs; what can turns round, which is the
    // whole difference between a cow and a polar bear. m_provoked is
    // already set above, so hostileNow() knows which this is.
    m_goalTimer = 0.0f;
    if (type().spawnClass == SpawnClass::Passive && !hostileNow())
        m_panicTimer = PANIC_SECONDS;

    // A mob on fire takes a hit every second for as long as it lasts, and
    // a yelp for each one is a lot of yelping from a herd at dawn. Dying
    // is always worth hearing; being hurt is worth hearing now and then.
    if (!alive())
        emit(type().deathVoice, 1.0f, 0.9f + random01() * 0.2f);
    else if (m_hurtVoiceTimer <= 0.0f)
    {
        m_hurtVoiceTimer = HURT_VOICE_SECONDS;
        emit(type().hurtVoice, 0.9f, 0.9f + random01() * 0.2f);
    }
}

void Mob::heal(int amount)
{
    if (amount <= 0 || m_health <= 0) return;
    m_health = std::min(m_health + amount, mobType(m_type).maxHealth);
}
