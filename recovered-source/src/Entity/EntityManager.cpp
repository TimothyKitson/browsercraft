#include "EntityManager.h"
#include "World/World.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float SPAWN_INTERVAL = 4.0f;  // seconds between spawn attempts
    constexpr int ATTEMPTS_PER_WAVE = 8;
    constexpr int WORLD_TOP = 120;
    constexpr uint8_t HOSTILE_MAX_LIGHT = 7; // dark enough for monsters

    const MobId PASSIVE_SPECIES[] = { MobId::Sheep, MobId::Pig, MobId::Cow, MobId::Chicken };
    const MobId HOSTILE_SPECIES[] = { MobId::Zombie, MobId::Skeleton, MobId::Creeper, MobId::Spider };
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
        if (isSolid(world.getBlock(x, y, z)) && !isSolid(world.getBlock(x, y + 1, z)))
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
                if (isSolid(world.getBlock(x, y, z)) || isLiquid(world.getBlock(x, y, z)))
                    return false;

    return true;
}

bool EntityManager::spawnAt(MobId type, glm::vec3 feetPosition, const World& world)
{
    if (static_cast<int>(m_mobs.size()) >= MAX_MOBS) return false;
    if (!fits(world, mobType(type), feetPosition)) return false;

    m_mobs.emplace_back(type, feetPosition, m_rng ^ static_cast<uint32_t>(m_mobs.size() * 2654435761u));
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

        // Only spawn on ground the world has actually generated.
        if (world.getBlock(x, y - 1, z) == Blocks::Air) continue;

        const uint8_t sky = world.skyLight(x, y, z);
        const uint8_t block = world.blockLightAt(x, y, z);
        const uint8_t light = static_cast<uint8_t>(std::max<int>(block, static_cast<int>(sky * daylight)));

        MobId species;
        if (night && light <= HOSTILE_MAX_LIGHT)
            species = HOSTILE_SPECIES[static_cast<int>(random01() * 4.0f) & 3];
        else if (!night && sky > 8)
            species = PASSIVE_SPECIES[static_cast<int>(random01() * 4.0f) & 3];
        else
            continue;

        const glm::vec3 feet(x + 0.5f, static_cast<float>(y), z + 0.5f);
        if (spawnAt(species, feet, world)) return; // one per wave keeps it gradual
    }
}

void EntityManager::update(float deltaTime, const World& world, const glm::vec3& playerPosition, float daylight)
{
    for (Mob& mob : m_mobs)
        mob.update(deltaTime, world, playerPosition);

    // Drop anything dead-and-done or far enough away that nobody will miss it.
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

Mob* EntityManager::pick(const glm::vec3& origin, const glm::vec3& direction, float maxDistance)
{
    Mob* best = nullptr;
    float bestDistance = maxDistance;

    for (Mob& mob : m_mobs)
    {
        if (!mob.alive()) continue;

        const MobType& type = mob.type();
        const glm::vec3 centre = mob.position() + glm::vec3(0.0f, type.height * 0.5f, 0.0f);
        const glm::vec3 toCentre = centre - origin;

        const float along = glm::dot(toCentre, direction);
        if (along < 0.0f || along > bestDistance) continue;

        // Distance from the mob's centre to the ray, against its own girth.
        const float offAxis = glm::length(toCentre - direction * along);
        if (offAxis > std::max(type.width, type.height) * 0.5f) continue;

        best = &mob;
        bestDistance = along;
    }

    return best;
}
