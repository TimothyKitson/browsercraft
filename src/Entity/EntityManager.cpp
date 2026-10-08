#include "EntityManager.h"
#include "World/World.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float SPAWN_INTERVAL = 4.0f;  // seconds between attempts
    constexpr int ATTEMPTS_PER_WAVE = 8;
    constexpr int WORLD_TOP = 120;
    constexpr int HOSTILE_MAX_LIGHT = 7;    // dark enough for monsters
    constexpr int PASSIVE_MIN_LIGHT = 9;

    const MobId PASSIVE_SPECIES[] = { MobId::Sheep, MobId::Pig, MobId::Cow, MobId::Chicken };
    const MobId HOSTILE_SPECIES[] = { MobId::Zombie, MobId::Skeleton, MobId::Creeper, MobId::Spider };

    // Leaves collide, so a plain "topmost solid block" search stops on
    // the canopy and nothing ever spawns in a forest -- which is most of
    // the overworld. Nothing stands on a treetop.
    bool standable(BlockId id)
    {
        return isSolid(id) && id != Blocks::Leaves && id != Blocks::BirchLeaves;
    }
}

bool EntityManager::canSpawnOn(MobId type, BlockId ground, int light)
{
    if (!isSolid(ground)) return false;

    if (mobType(type).spawnClass == SpawnClass::Hostile)
        return light <= HOSTILE_MAX_LIGHT;

    // Animals want daylight and something growing underfoot, which is
    // what keeps them out of caves and off the bare stone of a mountain.
    return light >= PASSIVE_MIN_LIGHT &&
           (ground == Blocks::Grass || ground == Blocks::SnowGrass || ground == Blocks::Sand);
}

float EntityManager::random01()
{
    m_rng ^= m_rng << 13;
    m_rng ^= m_rng >> 17;
    m_rng ^= m_rng << 5;
    return static_cast<float>(m_rng & 0xFFFFFF) / static_cast<float>(0x1000000);
}

int EntityManager::surfaceBelow(const World& world, int x, int startY, int z) const
{
    for (int y = std::min(startY, WORLD_TOP); y > 1; --y)
    {
        if (!standable(world.getBlock(x, y, z))) continue;

        // Room to stand up in, or it is a ledge under an overhang.
        if (isSolid(world.getBlock(x, y + 1, z))) continue;
        if (isSolid(world.getBlock(x, y + 2, z))) continue;

        return y + 1; // standing position, one above the ground
    }

    return -1;
}

bool EntityManager::fits(const World& world, const MobType& type, const glm::vec3& feet) const
{
    const float half = type.width * 0.5f;
    const int minX = static_cast<int>(std::floor(feet.x - half));
    const int maxX = static_cast<int>(std::floor(feet.x + half));
    const int minY = static_cast<int>(std::floor(feet.y));
    const int maxY = static_cast<int>(std::floor(feet.y + type.height));
    const int minZ = static_cast<int>(std::floor(feet.z - half));
    const int maxZ = static_cast<int>(std::floor(feet.z + half));

    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
            {
                const BlockId here = world.getBlock(x, y, z);
                if (isSolid(here) || isLiquid(here)) return false;
            }

    return true;
}

bool EntityManager::spawnAt(MobId type, glm::vec3 feetPosition, const World& world)
{
    if (static_cast<int>(m_mobs.size()) >= MAX_MOBS) return false;
    if (!fits(world, mobType(type), feetPosition)) return false;

    m_mobs.emplace_back(type, feetPosition,
                        m_rng ^ static_cast<uint32_t>(m_mobs.size() * 2654435761u));
    return true;
}

void EntityManager::trySpawnWave(const World& world, const glm::vec3& playerPosition, float daylight)
{
    if (static_cast<int>(m_mobs.size()) >= MAX_MOBS) return;

    // Monsters come out when the sun is down; animals keep to the daylight.
    const bool night = daylight < 0.35f;

    for (int attempt = 0; attempt < ATTEMPTS_PER_WAVE; ++attempt)
    {
        const float angle = random01() * 6.2831853f;
        const float distance = SPAWN_MIN_DISTANCE + random01() * (SPAWN_MAX_DISTANCE - SPAWN_MIN_DISTANCE);

        const int x = static_cast<int>(std::floor(playerPosition.x + std::sin(angle) * distance));
        const int z = static_cast<int>(std::floor(playerPosition.z + std::cos(angle) * distance));

        const int y = surfaceBelow(world, x, WORLD_TOP, z);
        if (y < 0) continue;

        const BlockId ground = world.getBlock(x, y - 1, z);
        if (ground == Blocks::Air) continue;   // chunk not generated yet

        const int sky = static_cast<int>(world.skyLight(x, y, z));
        const int block = static_cast<int>(world.blockLightAt(x, y, z));
        const int light = std::max(block, static_cast<int>(sky * daylight));

        const MobId* pool = night ? HOSTILE_SPECIES : PASSIVE_SPECIES;
        const MobId species = pool[static_cast<int>(random01() * 4.0f) & 3];
        if (!canSpawnOn(species, ground, light)) continue;

        const glm::vec3 feet(x + 0.5f, static_cast<float>(y), z + 0.5f);
        if (spawnAt(species, feet, world)) return; // one per wave keeps it gradual
    }
}

void EntityManager::update(float deltaTime, const World& world,
                           const glm::vec3& playerPosition, float daylight)
{
    for (Mob& mob : m_mobs)
    {
        mob.update(deltaTime, world, playerPosition);

        // Caught here rather than at removal: the corpse lingers, and
        // whatever it drops belongs where it fell, not where it stopped.
        if (mob.takeDeathReport())
            m_deaths.push_back(Death{ mob.typeId(), mob.position() });
    }

    // Drop anything dead-and-done, or far enough off that nobody will
    // miss it.
    m_mobs.erase(
        std::remove_if(m_mobs.begin(), m_mobs.end(), [&](const Mob& mob) {
            if (mob.finished()) return true;
            const glm::vec3 delta = mob.position() - playerPosition;
            return glm::length(glm::vec3(delta.x, 0.0f, delta.z)) > DESPAWN_DISTANCE;
        }),
        m_mobs.end());

    m_spawnTimer -= deltaTime;
    if (m_spawnTimer <= 0.0f)
    {
        m_spawnTimer = SPAWN_INTERVAL;
        trySpawnWave(world, playerPosition, daylight);
    }
}

std::vector<MobSound> EntityManager::drainSounds()
{
    std::vector<MobSound> all;
    for (Mob& mob : m_mobs)
    {
        if (mob.sounds().empty()) continue;
        all.insert(all.end(), mob.sounds().begin(), mob.sounds().end());
        mob.sounds().clear();
    }
    return all;
}

std::vector<MobStrike> EntityManager::drainStrikes()
{
    std::vector<MobStrike> all;
    for (Mob& mob : m_mobs)
    {
        if (mob.strikes().empty()) continue;
        all.insert(all.end(), mob.strikes().begin(), mob.strikes().end());
        mob.strikes().clear();
    }
    return all;
}

std::vector<EntityManager::Death> EntityManager::drainDeaths()
{
    std::vector<Death> all;
    all.swap(m_deaths);
    return all;
}

Mob* EntityManager::pick(const glm::vec3& origin, const glm::vec3& direction, float maxDistance)
{
    Mob* best = nullptr;
    float bestDistance = maxDistance;

    for (Mob& mob : m_mobs)
    {
        if (!mob.alive()) continue;

        // Against the mob's own box rather than a sphere around it. A
        // sphere wide enough to cover a zombie's height is a metre wide
        // at the waist, so you could punch one by looking past it.
        const MobType& type = mob.type();
        const float half = type.width * 0.5f;
        const glm::vec3 low = mob.position() - glm::vec3(half, 0.0f, half);
        const glm::vec3 high = mob.position() + glm::vec3(half, type.height, half);

        // Slab test: clip the ray against each pair of parallel faces
        // and see whether anything is left of it.
        float enter = 0.0f;
        float exit = bestDistance;
        bool missed = false;

        for (int axis = 0; axis < 3 && !missed; ++axis)
        {
            if (std::fabs(direction[axis]) < 1e-6f)
            {
                // Parallel to this pair of faces: either inside them for
                // the whole ray, or never.
                if (origin[axis] < low[axis] || origin[axis] > high[axis]) missed = true;
                continue;
            }

            float near = (low[axis] - origin[axis]) / direction[axis];
            float far = (high[axis] - origin[axis]) / direction[axis];
            if (near > far) std::swap(near, far);

            enter = std::max(enter, near);
            exit = std::min(exit, far);
            if (enter > exit) missed = true;
        }

        if (missed || enter > bestDistance) continue;

        best = &mob;
        bestDistance = enter;
    }

    return best;
}
