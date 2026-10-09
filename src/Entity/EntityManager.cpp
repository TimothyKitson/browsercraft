#include "EntityManager.h"
#include "World/World.h"
#include "Pathfinder.h"
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

    // The world as the pathfinder sees it: can a mob of this height
    // stand here, with a floor under it and nothing in the way.
    struct WorldGround : Pathfinding::Ground
    {
        const World* world = nullptr;
        int clearance = 2;

        bool standable(const glm::ivec3& feet) const override
        {
            if (feet.y < 1 || feet.y + clearance >= Chunk::SY) return false;
            if (!isSolid(world->getBlock(feet.x, feet.y - 1, feet.z))) return false;

            for (int i = 0; i < clearance; ++i)
            {
                const BlockId at = world->getBlock(feet.x, feet.y + i, feet.z);
                // Liquids are not a floor and not worth drowning in.
                if (isSolid(at) || isLiquid(at)) return false;
            }
            return true;
        }
    };

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

bool EntityManager::spawnAt(MobId type, glm::vec3 feetPosition, const World& world, bool baby)
{
    if (static_cast<int>(m_mobs.size()) >= MAX_MOBS) return false;
    if (!fits(world, mobType(type), feetPosition)) return false;

    m_mobs.emplace_back(type, feetPosition,
                        m_rng ^ static_cast<uint32_t>(m_mobs.size() * 2654435761u), baby);
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
        mob.update(deltaTime, world, playerPosition, daylight);

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

    pairOffLovers(world);
    routeChasers(world, playerPosition);

    m_spawnTimer -= deltaTime;
    if (m_spawnTimer <= 0.0f)
    {
        m_spawnTimer = SPAWN_INTERVAL;
        trySpawnWave(world, playerPosition, daylight);
    }
}

void EntityManager::routeChasers(const World& world, const glm::vec3& playerPosition)
{
    const glm::ivec3 goal(static_cast<int>(std::floor(playerPosition.x)),
                          static_cast<int>(std::floor(playerPosition.y)),
                          static_cast<int>(std::floor(playerPosition.z)));

    WorldGround ground;
    ground.world = &world;

    int searches = 0;
    for (Mob& mob : m_mobs)
    {
        if (searches >= PATHS_PER_UPDATE) break;
        if (!mob.wantsPath()) continue;

        // Close enough to walk straight at, and a route would only get
        // in the way of actually reaching them.
        const glm::vec3 apart = playerPosition - mob.position();
        if (glm::length(glm::vec3(apart.x, 0.0f, apart.z)) < 2.0f) continue;

        ground.clearance = std::max(1, static_cast<int>(std::ceil(mob.height())));

        const glm::ivec3 from(static_cast<int>(std::floor(mob.position().x)),
                              static_cast<int>(std::floor(mob.position().y)),
                              static_cast<int>(std::floor(mob.position().z)));

        mob.setPath(Pathfinding::find(ground, from, goal), goal);
        ++searches;
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

std::vector<MobShot> EntityManager::drainShots()
{
    std::vector<MobShot> all;
    for (Mob& mob : m_mobs)
    {
        if (mob.shots().empty()) continue;
        all.insert(all.end(), mob.shots().begin(), mob.shots().end());
        mob.shots().clear();
    }
    return all;
}

std::vector<MobBlast> EntityManager::drainBlasts()
{
    std::vector<MobBlast> all;
    for (Mob& mob : m_mobs)
    {
        if (mob.blasts().empty()) continue;
        all.insert(all.end(), mob.blasts().begin(), mob.blasts().end());
        mob.blasts().clear();
    }
    return all;
}

std::vector<glm::vec3> EntityManager::drainBirths()
{
    std::vector<glm::vec3> all;
    all.swap(m_births);
    return all;
}

bool EntityManager::feed(const glm::vec3& origin, const glm::vec3& direction,
                         float maxDistance, StackId food)
{
    Mob* target = pick(origin, direction, maxDistance);
    return target && target->feed(food);
}

// Two of a kind, both in love, close enough to be standing together.
void EntityManager::pairOffLovers(const World& world)
{
    for (size_t a = 0; a < m_mobs.size(); ++a)
    {
        if (!m_mobs[a].inLove() || !m_mobs[a].canBreed()) continue;

        for (size_t b = a + 1; b < m_mobs.size(); ++b)
        {
            if (m_mobs[b].typeId() != m_mobs[a].typeId()) continue;
            if (!m_mobs[b].inLove() || !m_mobs[b].canBreed()) continue;

            const glm::vec3 apart = m_mobs[b].position() - m_mobs[a].position();
            if (glm::length(apart) > 3.0f) continue;

            const glm::vec3 between = m_mobs[a].position() + apart * 0.5f;
            m_mobs[a].onBred();
            m_mobs[b].onBred();

            if (static_cast<int>(m_mobs.size()) < MAX_MOBS &&
                fits(world, mobType(m_mobs[a].typeId()), between))
            {
                m_mobs.emplace_back(m_mobs[a].typeId(), between,
                                    m_rng ^ static_cast<uint32_t>(m_mobs.size() * 40503u), true);
                m_births.push_back(between);
            }

            // Both are spent; neither can pair again this pass.
            break;
        }
    }
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
        // A calf is half the size, and half as easy to hit.
        const float half = mob.width() * 0.5f;
        const glm::vec3 low = mob.position() - glm::vec3(half, 0.0f, half);
        const glm::vec3 high = mob.position() + glm::vec3(half, mob.height(), half);

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
