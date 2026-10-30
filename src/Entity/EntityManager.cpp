#include "EntityManager.h"
#include "World/World.h"
#include "World/Village.h"
#include "Game/Trading.h"
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

    // Horses keep to open country, which is where Minecraft puts them
    // and where there is room to ride one.
    const MobId PLAINS_SPECIES[] = { MobId::Horse };

    // Wolves keep to the trees, which is where Minecraft puts them and
    // what stops a pack turning up in the middle of a field.
    const MobId FOREST_SPECIES[] = { MobId::Wolf };
    // No breeze. It comes out of a trial spawner and nowhere else,
    // which is Minecraft's -- it was in this list only while there
    // were no chambers for it to come out of.
    const MobId HOSTILE_SPECIES[] = { MobId::Zombie, MobId::Skeleton, MobId::Creeper,
                                      MobId::Spider, MobId::Enderman };

    // The Nether has its own, and nothing else belongs there: a sheep on
    // netherrack and a piglin in a meadow are both wrong.
    const MobId NETHER_SPECIES[] = { MobId::ZombiePigman, MobId::WitherSkeleton,
                                     MobId::MagmaCube, MobId::Blaze, MobId::Ghast,
                                     MobId::Piglin, MobId::Hoglin, MobId::Strider };

    // Underground, which until now had nothing at all: the spawner
    // only ever looked down from the sky, so the first standable
    // block it found was always the surface and no cave floor was
    // ever offered a mob. Every monster in the game was one you met
    // outdoors, and the whole underworld was a safe place to walk.
    const MobId CAVE_SPECIES[] = { MobId::Zombie, MobId::Skeleton, MobId::Creeper,
                                   MobId::Spider, MobId::CaveSpider, MobId::Silverfish,
                                   MobId::Slime, MobId::Bat };

    // How far down a slime will go to, as Minecraft has it: they keep
    // to the bottom of the world rather than the first cave you find.
    constexpr int SLIME_CEILING = 40;

    // The End has endermen and nothing else, which is also true of
    // Minecraft's. The enderman is the one species in two lists: it
    // belongs to the End and wanders the overworld at night.
    const MobId END_SPECIES[] = { MobId::Enderman };

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
        return isSolid(id) && !isLeaves(id);
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

int EntityManager::nightSpeciesCount()
{
    return static_cast<int>(std::size(HOSTILE_SPECIES));
}

MobId EntityManager::nightSpecies(int index)
{
    const int count = nightSpeciesCount();
    return HOSTILE_SPECIES[((index % count) + count) % count];
}

int EntityManager::lavaSurfaceBelow(const World& world, int x, int startY, int z) const
{
    for (int y = std::min(startY, WORLD_TOP); y > 1; --y)
    {
        if (world.getBlock(x, y, z) != Blocks::Lava) continue;
        if (world.getBlock(x, y + 1, z) != Blocks::Air) continue;
        if (world.getBlock(x, y + 2, z) != Blocks::Air) continue;

        // Feet in the top block of the lava, the way a boat's are in
        // the top block of the water it sits on.
        return y;
    }

    return -1;
}

int EntityManager::caveSpeciesCount()
{
    return static_cast<int>(std::size(CAVE_SPECIES));
}

MobId EntityManager::caveSpecies(int index)
{
    const int count = caveSpeciesCount();
    return CAVE_SPECIES[((index % count) + count) % count];
}

int EntityManager::netherSpeciesCount()
{
    return static_cast<int>(std::size(NETHER_SPECIES));
}

MobId EntityManager::netherSpecies(int index)
{
    const int count = netherSpeciesCount();
    return NETHER_SPECIES[((index % count) + count) % count];
}

int EntityManager::endSpeciesCount()
{
    return static_cast<int>(std::size(END_SPECIES));
}

MobId EntityManager::endSpecies(int index)
{
    const int count = endSpeciesCount();
    return END_SPECIES[((index % count) + count) % count];
}

bool EntityManager::roomFor(const MobType& type, BlockId block)
{
    if (isSolid(block)) return false;

    // Water is somewhere to stand for a thing that floats on it or
    // lives in it: refusing it put boats everywhere but the pond, and
    // kept the guardian out of its own monument and the drowned out of
    // the sea -- both were spawned into water that the spawn then read
    // as rock, so neither was ever once seen. Lava turns everything
    // away.
    if (isLiquid(block) && !((type.floats || type.aquatic) && block == Blocks::Water))
    {
        // Except the one thing that walks on lava, to which lava is
        // the floor and water is nothing at all.
        if (!(type.lavaWalker && block == Blocks::Lava)) return false;
    }

    return true;
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
                if (!roomFor(type, world.getBlock(x, y, z))) return false;

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

namespace
{
    // What trade a building's villager keeps. Butchers and ordinary
    // households have no trade of their own in the four Minecraft gives
    // us here, so they take the two that deal in what they would have:
    // food, and what a household makes.
    Trading::Profession tradeOf(Village::Kind kind)
    {
        switch (kind)
        {
            case Village::Kind::Blacksmith: return Trading::Profession::Toolsmith;
            case Village::Kind::Library:    return Trading::Profession::Librarian;
            case Village::Kind::Butcher:    return Trading::Profession::Farmer;
            default:                        return Trading::Profession::Fletcher;
        }
    }
}

void EntityManager::trySpawnWave(const World& world, const glm::vec3& playerPosition, float daylight)
{
    if (static_cast<int>(m_mobs.size()) >= MAX_MOBS) return;

    // Monsters come out when the sun is down; animals keep to the daylight.
    const bool night = daylight < 0.35f;

    // Whether this whole wave goes looking underground. Decided once
    // for the wave rather than per attempt: a wave stops at its first
    // success, and the first attempt on the surface nearly always
    // succeeds, so alternating attempts meant the cave branch was
    // reached perhaps once an hour and the caves stayed empty.
    const bool digging = m_dimension == Dimension::Overworld && random01() < 0.55f;

    for (int attempt = 0; attempt < ATTEMPTS_PER_WAVE; ++attempt)
    {
        const float angle = random01() * 6.2831853f;
        const float distance = SPAWN_MIN_DISTANCE + random01() * (SPAWN_MAX_DISTANCE - SPAWN_MIN_DISTANCE);

        const int x = static_cast<int>(std::floor(playerPosition.x + std::sin(angle) * distance));
        const int z = static_cast<int>(std::floor(playerPosition.z + std::cos(angle) * distance));

        // Where to start looking down from. In the overworld that is
        // the sky; in the Nether it cannot be, because the Nether is a
        // closed cavern with a roof on it and the first standable
        // block below the sky is the top of that roof. Every mob the
        // Nether ever spawned was put on the roof, above the ceiling,
        // where nobody has ever been -- they all read y=120 when the
        // spawner was finally asked to say where it was putting them.
        const int ceiling = (m_dimension == Dimension::Nether)
                                ? static_cast<int>(playerPosition.y) + 16
                                : WORLD_TOP;

        // Half the overworld's waves look for a cave floor rather than
        // the ground: start the search from a random depth below the
        // surface, and the first standable block under that is a cave
        // floor by construction.
        const int surface = surfaceBelow(world, x, ceiling, z);

        if (surface < 0) continue;

        const bool underground = digging && surface > 20;

        int y = surface;
        if (underground)
        {
            const int from = 6 + static_cast<int>(random01() *
                                                  static_cast<float>(surface - 14));
            y = surfaceBelow(world, x, from, z);
            if (y < 0 || y >= surface - 3) continue;   // that was the surface again
        }

        const BlockId ground = world.getBlock(x, y - 1, z);
        if (ground == Blocks::Air) continue;   // chunk not generated yet

        const int sky = static_cast<int>(world.skyLight(x, y, z));
        const int block = static_cast<int>(world.blockLightAt(x, y, z));
        const int light = std::max(block, static_cast<int>(sky * daylight));

        // The village this spot belongs to, if any. Declared out here
        // because the spawn below needs it to hand a villager its trade.
        Village::Site home;

        MobId species;
        if (m_dimension == Dimension::End)
        {
            constexpr int END_COUNT = static_cast<int>(std::size(END_SPECIES));
            species = END_SPECIES[static_cast<int>(random01() * END_COUNT) % END_COUNT];
        }
        else if (m_dimension != Dimension::Overworld)
        {
            constexpr int NETHER_COUNT = static_cast<int>(std::size(NETHER_SPECIES));
            species = NETHER_SPECIES[static_cast<int>(random01() * NETHER_COUNT) % NETHER_COUNT];
        }
        else if (underground)
        {
            constexpr int CAVE_COUNT = static_cast<int>(std::size(CAVE_SPECIES));
            species = CAVE_SPECIES[static_cast<int>(random01() * CAVE_COUNT) % CAVE_COUNT];

            // A slime keeps to the bottom of the world. Anywhere above
            // that, the roll goes to whatever else was on the list.
            if (species == MobId::Slime && y > SLIME_CEILING) species = MobId::Zombie;
        }
        else if (night)
        {
            constexpr int HOSTILE_COUNT = static_cast<int>(std::size(HOSTILE_SPECIES));
            species = HOSTILE_SPECIES[static_cast<int>(random01() * HOSTILE_COUNT) % HOSTILE_COUNT];
        }
        else
        {
            // Inside a village, villagers are most of what spawns:
            // that is the whole point of the place. The cheap test
            // comes first -- which square this is, and whether its
            // village reaches here -- and only a spot that passes it
            // pays for the survey that says whether the village was
            // actually built.
            bool inVillage = false;
            if (m_dimension == Dimension::Overworld)
            {
                home = Village::siteFor(Village::regionOf(x), Village::regionOf(z),
                                        world.seed());
                if (Village::reaches(home, x, z, x, z))
                    inVillage = world.generator().nearestVillage(x, z, 0).exists;
            }

            if (inVillage)
            {
                // Mostly villagers, with the occasional golem on patrol
                // and some livestock between the houses.
                const float roll = random01();
                species = roll < 0.70f  ? MobId::Villager
                        : roll < 0.78f  ? MobId::IronGolem
                                        : PASSIVE_SPECIES[static_cast<int>(random01() * 4.0f) & 3];
            }
            else
            {
                // Out in the country they are rare enough that meeting
                // one is an event rather than livestock.
                const Biome here = world.generator().biomeAt(x, z);
                const bool woods = (here == Biome::Forest);

                if (woods && random01() < 0.22f)
                {
                    constexpr int FOREST_COUNT =
                        static_cast<int>(std::size(FOREST_SPECIES));
                    species = FOREST_SPECIES[static_cast<int>(random01() * FOREST_COUNT)
                                             % FOREST_COUNT];
                }
                else if (here == Biome::Plains && random01() < 0.14f)
                {
                    constexpr int PLAINS_COUNT =
                        static_cast<int>(std::size(PLAINS_SPECIES));
                    species = PLAINS_SPECIES[static_cast<int>(random01() * PLAINS_COUNT)
                                             % PLAINS_COUNT];
                }
                else
                {
                    species = (random01() < 0.02f)
                                  ? MobId::Villager
                                  : PASSIVE_SPECIES[static_cast<int>(random01() * 4.0f) & 3];
                }
            }
        }

        // The list for this dimension is what says who belongs here.
        // The type's own dimension is only a backstop for the
        // overworld, and asking it everywhere is what left the End
        // empty: the End drew from the Nether's list, and every one of
        // those was then thrown out for being a Nether mob.
        if (m_dimension == Dimension::Overworld &&
            mobType(species).dimension != Dimension::Overworld)
            continue;
        glm::vec3 mobFeet(x + 0.5f, static_cast<float>(y), z + 0.5f);

        // Two species are not asked the ordinary question about where
        // they may stand, because the ordinary question is the wrong
        // one for both of them.
        //
        // A strider stands on the lava, not on the bank beside it, so
        // the lava's own top replaces whatever the ordinary search
        // found -- the ordinary search cannot see a lava surface at
        // all, because lava is nowhere to stand to anything else in
        // the game. Two earlier rules here were both wrong and both
        // gave nothing: the surface a column offers is usually a
        // ledge far above its lava, so neither "only when there is no
        // footing" nor "only when the lava is the higher of the two"
        // ever let one through. And it is passive, so the
        // daylight-and-grass rule would have refused it on netherrack
        // even once it was standing in the right place.
        const bool onLava = mobType(species).lavaWalker;
        if (onLava)
        {
            const int top = lavaSurfaceBelow(world, x, ceiling, z);
            if (top < 0) continue;
            mobFeet.y = static_cast<float>(top);
        }
        // A bat is the one passive thing that belongs in the dark, so
        // the daylight-and-grass rule that keeps animals out of caves
        // is the wrong question to ask about one either.
        else if (species == MobId::Bat)
        {
            if (!isSolid(ground) || light > HOSTILE_MAX_LIGHT) continue;
        }
        else if (!canSpawnOn(species, ground, light)) continue;

        if (!spawnAt(species, mobFeet, world)) continue;

        // A villager standing in a building takes that building's
        // trade, so the smith is the one who lives in the smithy. One
        // met out in the country has no building to take it from and
        // keeps the one its birth seed draws.
        if (species == MobId::Villager && home.exists)
        {
            Village::Building where;
            if (Village::buildingAt(home, world.seed(), x, z, where))
                m_mobs.back().setProfessionSlot(static_cast<int>(tradeOf(where.kind)));

            // And it belongs to the village, not to the countryside.
            // Without this they wandered off a step at a time and the
            // houses stood empty by morning.
            m_mobs.back().setHome(glm::vec3(static_cast<float>(home.x), mobFeet.y,
                                            static_cast<float>(home.z)),
                                  static_cast<float>(Village::MAX_RADIUS));
        }

        return;   // one per wave keeps it gradual
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
            m_deaths.push_back(Death{ mob.typeId(), mob.position(), mob.baby() });

        if (mob.takeEggReport()) m_laidEggs.push_back(mob.position());
    }

    // A frog eats a magma cube it can reach. This is the whole reason
    // the frog is in the game, and the only way to a froglight.
    //
    // Walked here rather than inside the loop above: eating one kills
    // it, and killing a mob while the list is being stepped is how a
    // reference goes stale. There are never many frogs, and the inner
    // walk stops at the first cube close enough.
    constexpr float FROG_REACH = 1.6f;
    for (Mob& frog : m_mobs)
    {
        if (frog.typeId() != MobId::Frog || !frog.alive() || frog.baby()) continue;

        for (Mob& cube : m_mobs)
        {
            if (cube.typeId() != MobId::MagmaCube || !cube.alive()) continue;

            const glm::vec3 away = cube.position() - frog.position();
            if (glm::dot(away, away) > FROG_REACH * FROG_REACH) continue;

            cube.damage(cube.health(), glm::vec3(0.0f));
            m_frogMeals.push_back(cube.position());
            break;
        }
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

    setWolvesOnHostiles(deltaTime);
    courtVillagers(world, deltaTime);
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

void EntityManager::noticeStares(const World& world, const glm::vec3& eye, const glm::vec3& look)
{
    for (Mob& mob : m_mobs)
    {
        if (!mob.alive() || mob.provoked() || !mob.type().neutral) continue;
        if (!Stare::atHead(eye, look, mob.position(), mob.type().height)) continue;

        // A wall in the way means you are not looking at it, whatever
        // the ray says.
        const glm::vec3 head = mob.position() +
                               glm::vec3(0.0f, mob.type().height - Stare::HEAD_RADIUS, 0.0f);
        if (!world.hasLineOfSight(eye, head)) continue;

        mob.provoke();
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

bool EntityManager::tame(const glm::vec3& origin, const glm::vec3& direction,
                         float maxDistance, StackId food)
{
    Mob* target = pick(origin, direction, maxDistance);
    return target && target->tameWith(food);
}

Mob* EntityManager::toggleSit(const glm::vec3& origin, const glm::vec3& direction,
                              float maxDistance)
{
    Mob* target = pick(origin, direction, maxDistance);
    if (!target || !target->alive()) return nullptr;
    if (!Mob::canSit(target->typeId(), target->tamed())) return nullptr;

    target->toggleSit();
    return target;
}

Mob* EntityManager::rideable(const glm::vec3& origin, const glm::vec3& direction,
                             float maxDistance)
{
    Mob* target = pick(origin, direction, maxDistance);
    if (!target || !target->alive()) return nullptr;
    if (!Mob::canBeRidden(target->type(), target->tamed(), target->baby()))
        return nullptr;
    return target;
}

// A tamed wolf goes for whatever hostile is nearest. The wolf cannot
// see the other mobs -- only this can -- so the quarry is picked here
// and handed to it as a heading.
void EntityManager::setWolvesOnHostiles(float deltaTime)
{
    for (size_t i = 0; i < m_mobs.size(); ++i)
    {
        // A tamed wolf hunts for whoever tamed it. A golem hunts
        // because that is the whole of what a golem is for, and needs
        // nobody's permission.
        const MobId kind = m_mobs[i].typeId();
        const bool golem = kind == MobId::IronGolem;
        if (kind != MobId::Wolf && !golem) continue;
        if (!m_mobs[i].alive() || m_mobs[i].baby()) continue;
        if (!golem && !m_mobs[i].tamed()) continue;

        // A wolf that has been told to stay does exactly that, which is
        // the whole reason for telling it.
        if (m_mobs[i].sitting()) continue;

        const glm::vec3 from = m_mobs[i].position();

        size_t quarry = m_mobs.size();
        float nearest = Mob::HUNT_RANGE;

        for (size_t j = 0; j < m_mobs.size(); ++j)
        {
            if (j == i || !m_mobs[j].alive()) continue;
            if (mobType(m_mobs[j].typeId()).spawnClass != SpawnClass::Hostile) continue;

            const glm::vec3 apart = m_mobs[j].position() - from;
            const float away = glm::length(glm::vec3(apart.x, 0.0f, apart.z));
            if (away >= nearest) continue;

            nearest = away;
            quarry = j;
        }

        if (quarry >= m_mobs.size()) continue;

        m_mobs[i].huntTowards(m_mobs[quarry].position());

        // A golem hits with what its own type says it hits with, and
        // reaches further: it is twice the height of a wolf.
        const MobType& t = mobType(kind);
        const float reach = golem ? t.attackReach + t.width * 0.5f : Mob::BITE_REACH;
        const int damage = golem ? t.attackDamage : Mob::BITE_DAMAGE;

        if (nearest <= reach && m_mobs[i].tryBite(deltaTime))
        {
            const glm::vec3 away = m_mobs[quarry].position() - from;
            m_mobs[quarry].damage(damage, away);
        }
    }
}

// Villagers pair off on their own: nobody hands one a carrot. What
// decides whether they do is whether the village has a spare bed, the
// way Minecraft counts it -- so a village grows to fill its houses and
// then stops.
void EntityManager::courtVillagers(const World& world, float deltaTime)
{
    if (m_dimension != Dimension::Overworld) return;

    m_courtTimer -= deltaTime;
    if (m_courtTimer > 0.0f) return;
    m_courtTimer = COURT_INTERVAL;

    // Group what is loaded by the village it stands in. A villager out
    // in the country belongs to no village and does not pair off.
    struct Home { Village::Site site; int villagers = 0; };
    std::vector<Home> homes;

    for (const Mob& mob : m_mobs)
    {
        if (mob.typeId() != MobId::Villager || !mob.alive()) continue;

        const int x = static_cast<int>(std::floor(mob.position().x));
        const int z = static_cast<int>(std::floor(mob.position().z));

        const Village::Site site = Village::siteFor(Village::regionOf(x), Village::regionOf(z),
                                                    world.seed());
        if (!Village::reaches(site, x, z, x, z)) continue;

        // The box reaching here is not the same as a village having
        // been built here: plenty of sites are turned down for the
        // biome or the slope, and villagers standing in one of those
        // are standing in open country with no beds to count.
        if (!world.generator().nearestVillage(x, z, 0).exists) continue;

        Home* found = nullptr;
        for (Home& home : homes)
            if (home.site.x == site.x && home.site.z == site.z) found = &home;

        if (!found)
        {
            homes.push_back(Home{ site, 0 });
            found = &homes.back();
        }

        // Children count, and count once: they will want the bed they
        // were born for. Adding them to both tallies made the village
        // stop a child short of full.
        ++found->villagers;
    }

    for (const Home& home : homes)
    {
        const int beds = Village::bedCount(home.site, world.seed());
        if (home.villagers >= beds) continue;   // nowhere to put another

        // Two grown villagers in the same village, both free, become
        // willing. pairOffLovers does the rest, and only if they are
        // actually standing together.
        int made = 0;
        for (Mob& mob : m_mobs)
        {
            if (made >= 2) break;
            if (mob.typeId() != MobId::Villager || !mob.alive()) continue;
            if (mob.baby() || mob.inLove() || !mob.canBreed()) continue;
            if (mob.asleep()) continue;   // not in the middle of the night

            const int x = static_cast<int>(std::floor(mob.position().x));
            const int z = static_cast<int>(std::floor(mob.position().z));
            if (!Village::reaches(home.site, x, z, x, z)) continue;

            mob.makeWilling();
            ++made;
        }
    }
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
                // Read before the push: emplace_back can move the
                // whole vector, and the parent reference would be
                // pointing at freed memory by the time it is asked.
                const bool inherits = m_mobs[a].hasHome();
                const glm::vec3 parentHome = m_mobs[a].home();
                const int parentTrade = m_mobs[a].professionSlot();

                m_mobs.emplace_back(m_mobs[a].typeId(), between,
                                    m_rng ^ static_cast<uint32_t>(m_mobs.size() * 40503u), true);

                // A child belongs where its parents do, and takes up
                // the family trade. Born without either it would walk
                // out of the village as soon as it could.
                if (inherits)
                {
                    m_mobs.back().setHome(parentHome, static_cast<float>(Village::MAX_RADIUS));
                    if (parentTrade >= 0) m_mobs.back().setProfessionSlot(parentTrade);
                }

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

std::vector<glm::vec3> EntityManager::drainLaidEggs()
{
    std::vector<glm::vec3> all;
    all.swap(m_laidEggs);
    return all;
}

std::vector<glm::vec3> EntityManager::drainFrogMeals()
{
    std::vector<glm::vec3> all;
    all.swap(m_frogMeals);
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
