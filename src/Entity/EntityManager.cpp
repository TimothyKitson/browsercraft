#include "EntityManager.h"
#include "World/World.h"
#include "World/Village.h"
#include "Game/Trading.h"
#include "Pathfinder.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace
{
    constexpr float SPAWN_INTERVAL = 4.0f;  // seconds between attempts
    constexpr int ATTEMPTS_PER_WAVE = 8;
    constexpr int WORLD_TOP = 120;
    constexpr int HOSTILE_MAX_LIGHT = 7;    // dark enough for monsters
    constexpr int PASSIVE_MIN_LIGHT = 9;

    const MobId PASSIVE_SPECIES[] = { MobId::Sheep, MobId::Pig, MobId::Cow, MobId::Chicken };

    // What each biome has that the next one does not. One table
    // rather than a chain of ifs: adding an animal to a biome is
    // adding a row, and it was two ifs -- wolves in the trees and
    // horses in the fields -- for the whole of the overworld.
    //
    // The chances are read in order and add up, so a biome's rows
    // must total less than one: whatever is left over is the ordinary
    // livestock, which is most of what you meet anywhere.
    const MobId FORTRESS_SPECIES[] = { MobId::Blaze, MobId::Blaze, MobId::WitherSkeleton, MobId::WitherSkeleton, MobId::ZombiePigman,
                                       MobId::MagmaCube, MobId::Skeleton };

    // Underground, which until now had nothing at all: the spawner
    // only ever looked down from the sky, so the first standable
    // block it found was always the surface and no cave floor was
    // ever offered a mob. Every monster in the game was one you met
    // outdoors, and the whole underworld was a safe place to walk.
    const MobId CAVE_SPECIES[] = { MobId::Zombie, MobId::Skeleton, MobId::Creeper,
                                   MobId::Spider, MobId::CaveSpider, MobId::Silverfish,
                                   MobId::Slime, MobId::Bat, MobId::Endermite };

    // What swims. The squid was alone down there; these are the rest
    // of it. The axolotl belongs to a lush cave in Minecraft rather
    // than to the open sea, and goes in here because this engine has
    // no underground water worth a rule of its own yet.
    const MobId SEA_SPECIES[] = { MobId::Squid, MobId::Cod, MobId::Salmon,
                                  MobId::Pufferfish, MobId::Dolphin,
                                  MobId::Turtle, MobId::Axolotl,
                                  MobId::TropicalFish, MobId::TropicalFishLarge };

    // How far down a slime will go to, as Minecraft has it: they keep
    // to the bottom of the world rather than the first cave you find.
    constexpr int SLIME_CEILING = 40;


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
           (ground == Blocks::Grass || ground == Blocks::SnowGrass || ground == Blocks::Sand || ground == Blocks::RedSand);
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

namespace
{
    std::vector<MobId> spawnUnion(Dimension dimension, bool creatures, std::initializer_list<MobId> extra)
    {
        std::vector<MobId> out;
        int count = 0;
        const Biomes::Spawn* rows = Biomes::spawns(count);
        auto add = [&out](MobId mob) {
            if (mob != MobId::Count && std::find(out.begin(), out.end(), mob) == out.end()) out.push_back(mob);
        };
        for (int i = 0; i < count; ++i)
        {
            if (Biomes::dimension(rows[i].kind) != dimension) continue;
            const bool monster = rows[i].group == static_cast<uint8_t>(Biomes::Group::Monster);
            const bool creature = rows[i].group == static_cast<uint8_t>(Biomes::Group::Creature);
            if (monster || (creatures && creature)) add(EntityManager::mobFromId(rows[i].mob));
        }
        for (MobId mob : extra) add(mob);
        return out;
    }

    const std::vector<MobId>& nightList()
    {
        static const std::vector<MobId> LIST = spawnUnion(Dimension::Overworld, false, { MobId::Phantom });
        return LIST;
    }

    const std::vector<MobId>& netherList()
    {
        static const std::vector<MobId> LIST = spawnUnion(Dimension::Nether, true,
                                                          { MobId::Blaze, MobId::WitherSkeleton, MobId::Skeleton, MobId::MagmaCube });
        return LIST;
    }

    const std::vector<MobId>& endList()
    {
        static const std::vector<MobId> LIST = spawnUnion(Dimension::End, false, { MobId::Endermite });
        return LIST;
    }
}

int EntityManager::nightSpeciesCount()
{
    return static_cast<int>(nightList().size());
}

MobId EntityManager::nightSpecies(int index)
{
    const int count = nightSpeciesCount();
    return nightList()[static_cast<size_t>(((index % count) + count) % count)];
}

MobId EntityManager::mobFromId(const std::string& id)
{
    const std::string wanted = id.rfind("minecraft:", 0) == 0 ? id.substr(10) : id;
    for (int m = 0; m < static_cast<int>(MobId::Count); ++m)
    {
        std::string name = mobType(static_cast<MobId>(m)).name ? mobType(static_cast<MobId>(m)).name : "";
        for (char& c : name) c = c == ' ' ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (name == wanted) return static_cast<MobId>(m);
    }
    return MobId::Count;
}

MobId EntityManager::pickSpawn(Biomes::Kind kind, Biomes::Group group, float roll)
{
    struct Entry
    {
        MobId mob;
        int weight;
    };
    static const std::vector<std::vector<Entry>> TABLE = [] {
        std::vector<std::vector<Entry>> table(static_cast<size_t>(Biomes::COUNT) * static_cast<size_t>(Biomes::Group::Count));
        int count = 0;
        const Biomes::Spawn* rows = Biomes::spawns(count);
        for (int i = 0; i < count; ++i)
        {
            const MobId mob = mobFromId(rows[i].mob);
            if (mob == MobId::Count || rows[i].weight <= 0) continue;
            table[static_cast<size_t>(rows[i].kind) * static_cast<size_t>(Biomes::Group::Count) + rows[i].group].push_back({ mob, rows[i].weight });
        }
        return table;
    }();
    const std::vector<Entry>& entries = TABLE[static_cast<size_t>(kind) * static_cast<size_t>(Biomes::Group::Count) + static_cast<size_t>(group)];
    int total = 0;
    for (const Entry& entry : entries) total += entry.weight;
    if (total <= 0) return MobId::Count;
    int pick = std::min(total - 1, static_cast<int>(roll * static_cast<float>(total)));
    for (const Entry& entry : entries)
    {
        if (pick < entry.weight) return entry.mob;
        pick -= entry.weight;
    }
    return MobId::Count;
}

bool EntityManager::glowSquidWater(const World& world, int x, int y, int z)
{
    return isWater(world.getBlock(x, y, z)) && world.skyLight(x, y, z) == 0 &&
           world.blockLightAt(x, y, z) == 0;
}

int EntityManager::seaSpeciesCount()
{
    return static_cast<int>(std::size(SEA_SPECIES));
}

MobId EntityManager::seaSpecies(int index)
{
    const int count = seaSpeciesCount();
    return SEA_SPECIES[((index % count) + count) % count];
}

int EntityManager::lavaSurfaceBelow(const World& world, int x, int startY, int z) const
{
    for (int y = std::min(startY, WORLD_TOP); y > 1; --y)
    {
        if (!isLava(world.getBlock(x, y, z))) continue;
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
    return static_cast<int>(netherList().size());
}

MobId EntityManager::netherSpecies(int index)
{
    const int count = netherSpeciesCount();
    return netherList()[static_cast<size_t>(((index % count) + count) % count)];
}

int EntityManager::endSpeciesCount()
{
    return static_cast<int>(endList().size());
}

MobId EntityManager::endSpecies(int index)
{
    const int count = endSpeciesCount();
    return endList()[static_cast<size_t>(((index % count) + count) % count)];
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
    if (isLiquid(block) && !((type.floats || type.aquatic) && isWater(block)))
    {
        // Except the one thing that walks on lava, to which lava is
        // the floor and water is nothing at all.
        if (!(type.lavaWalker && isLava(block))) return false;
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

bool EntityManager::summonAt(MobId type, glm::vec3 feetPosition, bool baby)
{
    if (static_cast<int>(m_mobs.size()) >= MAX_MOBS) return false;
    m_mobs.emplace_back(type, feetPosition, m_rng ^ static_cast<uint32_t>(m_mobs.size() * 2654435761u), baby);
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
            case Village::Kind::Butcher:    return Trading::Profession::Butcher;
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
            species = random01() < 0.05f ? MobId::Endermite : pickSpawn(world.generator().endKindAt(x, z), Biomes::Group::Monster, random01());
            if (species == MobId::Count) continue;
        }
        else if (m_dimension != Dimension::Overworld && ground == Blocks::NetherBricks)
        {
            constexpr int FORTRESS_COUNT = static_cast<int>(std::size(FORTRESS_SPECIES));
            species = FORTRESS_SPECIES[static_cast<int>(random01() * FORTRESS_COUNT) % FORTRESS_COUNT];
        }
        else if (m_dimension != Dimension::Overworld)
        {
            const Biomes::Kind kind = world.generator().netherKindAt(x, z);
            species = pickSpawn(kind, random01() < 0.2f ? Biomes::Group::Creature : Biomes::Group::Monster, random01());
            if (species == MobId::Count) species = pickSpawn(kind, Biomes::Group::Monster, random01());
            if (species == MobId::Count) continue;
        }
        else if (underground)
        {
            if (world.generator().kindAt(x, y, z) == Biomes::Kind::DeepDark) continue;
            constexpr int CAVE_COUNT = static_cast<int>(std::size(CAVE_SPECIES));
            species = CAVE_SPECIES[static_cast<int>(random01() * CAVE_COUNT) % CAVE_COUNT];

            // A slime keeps to the bottom of the world. Anywhere above
            // that, the roll goes to whatever else was on the list.
            if (species == MobId::Slime && y > SLIME_CEILING) species = MobId::Zombie;
        }
        else if (night)
        {
            // What this place has after dark, if it has anything, and
            // the ordinary five otherwise.
            const Biomes::Kind kind = world.generator().kindAt(x, z);
            species = random01() < 0.03f && m_insomnia ? MobId::Phantom : pickSpawn(kind, Biomes::Group::Monster, random01());
            if (species == MobId::Count) continue;
            if (species == MobId::Slime && kind != Biomes::Kind::Swamp && kind != Biomes::Kind::MangroveSwamp) continue;
            if (species == MobId::Slime && !underground)
            {
                static const float FULLNESS[8] = { 1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f };
                if (random01() >= FULLNESS[m_moonPhase & 7]) continue;
            }
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
                const bool desert = world.generator().biomeAt(x, z) == Biome::Desert;
                species = Village::abandoned(home, world.seed()) ? (roll < 0.8f ? MobId::ZombieVillager : MobId::Zombie)
                        : roll < 0.70f  ? MobId::Villager
                        : roll < 0.78f  ? MobId::IronGolem
                        : roll < 0.84f  ? (desert && roll < 0.81f ? MobId::Camel : MobId::Cat)
                                        : PASSIVE_SPECIES[static_cast<int>(random01() * 4.0f) & 3];
            }
            else
            {
                // What this particular biome has, if it has anything.
                // Out in the country they are rare enough that meeting
                // one is an event rather than livestock.
                const float roll = random01();
                species = roll < 0.015f && m_traders ? MobId::WanderingTrader
                        : roll < 0.035f ? MobId::Villager
                                        : pickSpawn(world.generator().kindAt(x, z), Biomes::Group::Creature, random01());
                if (species == MobId::Count) continue;
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
        if (m_peaceful && mobType(species).spawnClass == SpawnClass::Hostile) continue;

        const bool zombieKind = species == MobId::Zombie || species == MobId::Husk || species == MobId::Drowned ||
                                species == MobId::ZombieVillager || species == MobId::ZombiePigman;
        const bool youngster = (zombieKind && random01() < 0.05f) ||
                               (mobType(species).spawnClass == SpawnClass::Passive && mobType(species).breedingFood != Blocks::Air &&
                                species != MobId::Villager && random01() < 0.05f);
        if (!spawnAt(species, mobFeet, world, youngster)) continue;
        {
            const MobType& kindType = mobType(species);
            const bool herd = (kindType.spawnClass == SpawnClass::Hostile || kindType.spawnClass == SpawnClass::Passive) &&
                              species != MobId::Villager && species != MobId::WanderingTrader && species != MobId::Witch &&
                              species != MobId::Creeper && !kindType.lavaWalker && species != MobId::Bat;
            if (herd)
            {
                const int extra = static_cast<int>(random01() * 4.0f);
                for (int k = 0; k < extra; ++k)
                {
                    const glm::vec3 near = mobFeet + glm::vec3(std::floor(random01() * 5.0f) - 2.0f, 0.0f, std::floor(random01() * 5.0f) - 2.0f);
                    const glm::ivec3 cell = glm::ivec3(glm::floor(near));
                    if (!isSolid(world.getBlock(cell.x, cell.y - 1, cell.z))) continue;
                    spawnAt(species, near, world, (zombieKind || kindType.spawnClass == SpawnClass::Passive) && random01() < 0.05f);
                }
            }
        }
        if (youngster && zombieKind && species != MobId::Drowned && random01() < 0.05f)
        {
            const size_t rider = m_mobs.size() - 1;
            if (spawnAt(MobId::Chicken, mobFeet, world)) m_mobs[rider].ride(m_mobs.back().serial());
        }
        if (species == MobId::Spider && random01() < 0.01f)
        {
            const uint32_t spider = m_mobs.back().serial();
            if (spawnAt(MobId::Skeleton, mobFeet + glm::vec3(0.0f, 0.7f, 0.0f), world) || spawnAt(MobId::Skeleton, mobFeet, world))
            {
                m_mobs.back().ride(spider);
                m_mobs.back().dress(m_difficulty);
            }
        }

        if (species == MobId::WanderingTrader)
            for (const float side : { -1.6f, 1.6f })
                spawnAt(MobId::TraderLlama, mobFeet + glm::vec3(side, 0.0f, 1.2f), world);

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

void EntityManager::tickHelpers(float deltaTime, const World& world, const glm::vec3& playerPosition)
{
    for (auto& entry : m_castTimers) entry.second -= deltaTime;
    const size_t count = m_mobs.size();
    {
        std::vector<size_t> led;
        std::vector<bool> taken(count, false);
        for (size_t i = 0; i < count; ++i)
            if (m_mobs[i].alive() && m_mobs[i].leashed() && (m_mobs[i].typeId() == MobId::Llama || m_mobs[i].typeId() == MobId::TraderLlama))
                led.push_back(i);
        for (size_t head : led)
        {
            taken[head] = true;
            size_t front = head;
            for (int link = 0; link < 9; ++link)
            {
                size_t next = count;
                float best = 8.0f;
                for (size_t j = 0; j < count; ++j)
                {
                    if (taken[j] || !m_mobs[j].alive() || m_mobs[j].leashed() || m_mobs[j].baby()) continue;
                    if (m_mobs[j].typeId() != MobId::Llama && m_mobs[j].typeId() != MobId::TraderLlama) continue;
                    const float apart = glm::length(m_mobs[j].position() - m_mobs[front].position());
                    if (apart < best) { best = apart; next = j; }
                }
                if (next >= count) break;
                taken[next] = true;
                if (best > 2.2f) m_mobs[next].huntTowards(m_mobs[front].position());
                front = next;
            }
        }
    }
    for (size_t i = 0; i < count; ++i)
    {
        Mob& mob = m_mobs[i];
        if (!mob.alive()) continue;
        const MobId kind = mob.typeId();
        float& wait = m_castTimers[mob.serial()];

        if (kind == MobId::SnowGolem)
        {
            const glm::ivec3 feet = glm::ivec3(glm::floor(mob.position() + glm::vec3(0.0f, 0.1f, 0.0f)));
            if (world.getBlock(feet.x, feet.y, feet.z) == Blocks::Air && isOpaque(world.getBlock(feet.x, feet.y - 1, feet.z)))
                m_snowTrail.push_back(glm::vec3(feet));
            if (wait > 0.0f) continue;
            size_t quarry = m_mobs.size();
            float nearest = 10.0f;
            for (size_t j = 0; j < m_mobs.size(); ++j)
            {
                if (j == i || !m_mobs[j].alive() || mobType(m_mobs[j].typeId()).spawnClass != SpawnClass::Hostile) continue;
                const float apart = glm::length(m_mobs[j].position() - mob.position());
                if (apart < nearest) { nearest = apart; quarry = j; }
            }
            if (quarry >= m_mobs.size()) continue;
            wait = 1.0f;
            const glm::vec3 eye = mob.position() + glm::vec3(0.0f, mob.height() * 0.8f, 0.0f);
            const glm::vec3 aim = m_mobs[quarry].position() + glm::vec3(0.0f, m_mobs[quarry].height() * 0.5f, 0.0f) - eye;
            MobShot shot;
            shot.from = eye + glm::normalize(aim) * 0.6f;
            shot.velocity = glm::normalize(aim + glm::vec3(0.0f, glm::length(aim) * 0.08f, 0.0f)) * 18.0f;
            shot.damage = m_mobs[quarry].typeId() == MobId::Blaze ? 3 : 0;
            shot.kind = Arrows::Kind::Snowball;
            mob.shotQueue().push_back(shot);
        }
        else if (kind == MobId::WanderingTrader)
        {
            if (wait > 0.0f) continue;
            wait = 5.0f;
            if (m_daylight < 0.3f) mob.giveEffect(Effects::Kind::Invisibility, 0, 12.0f);
        }
        else if (kind == MobId::Witch)
        {
            if (wait > 0.0f) continue;
            if (mob.health() * 2 < mob.type().maxHealth)
            {
                wait = 4.0f;
                mob.setGear(Mob::MainHand, static_cast<uint16_t>(Items::Potion));
                mob.heal(4);
                mob.emitNamed("entity/witch/drink", Sound::MobWitchSay, 0.8f, 0.9f + random01() * 0.2f);
            }
            else if (mob.gear(Mob::MainHand) == static_cast<uint16_t>(Items::Potion))
                mob.setGear(Mob::MainHand, 0);
        }
        else if (kind == MobId::Evoker)
        {
            if (wait > 0.0f) continue;
            if (glm::length(mob.position() - playerPosition) > 16.0f)
            {
                if (!m_mobGriefing) continue;
                for (Mob& sheep : m_mobs)
                {
                    if (sheep.typeId() != MobId::Sheep || !sheep.alive() || sheep.woolColour() != 11) continue;
                    if (glm::length(sheep.position() - mob.position()) > 16.0f) continue;
                    mob.emitNamed("mob/evocation_illager/prepare_wololo", Sound::EvokerCast, 1.0f, 1.0f);
                    mob.emitNamed("mob/evocation_illager/cast", Sound::EvokerCast, 1.0f, 1.0f);
                    sheep.setWool(14, sheep.sheared());
                    wait = 7.0f;
                    break;
                }
                continue;
            }
            int vexes = 0;
            for (const Mob& other : m_mobs)
                if (other.alive() && other.typeId() == MobId::Vex && glm::length(other.position() - mob.position()) < 24.0f) ++vexes;
            wait = 17.0f;
            if (vexes < 8)
            {
                mob.emitNamed("mob/evocation_illager/prepare_summon", Sound::EvokerCast, 1.0f, 1.0f);
                mob.emitNamed("mob/evocation_illager/cast", Sound::EvokerCast, 1.0f, 1.0f);
            }
            for (int k = 0; k < 3 && vexes < 8; ++k)
            {
                const float angle = random01() * 6.2831853f;
                const glm::vec3 spot = mob.position() + glm::vec3(std::cos(angle) * 1.5f, 1.0f, std::sin(angle) * 1.5f);
                if (spawnAt(MobId::Vex, spot, world)) ++vexes;
            }
        }
    }
}

void EntityManager::spawnJockey(MobId rider, MobId mount, const glm::vec3& feet, const World& world, bool babyRider)
{
    if (!spawnAt(mount, feet, world)) return;
    const uint32_t steed = m_mobs.back().serial();
    if (spawnAt(rider, feet + glm::vec3(0.0f, 0.7f, 0.0f), world, babyRider) || spawnAt(rider, feet, world, babyRider))
    {
        m_mobs.back().ride(steed);
        m_mobs.back().dress(m_difficulty);
    }
}

bool EntityManager::spawnPatrol(const World& world, const glm::vec3& playerPosition)
{
    const float angle = random01() * 6.2831853f;
    const float reach = 24.0f + random01() * 24.0f;
    const int x = static_cast<int>(std::floor(playerPosition.x + std::cos(angle) * reach));
    const int z = static_cast<int>(std::floor(playerPosition.z + std::sin(angle) * reach));
    int ground = -1;
    for (int y = std::min(Chunk::SY - 2, static_cast<int>(playerPosition.y) + 24); y > static_cast<int>(playerPosition.y) - 24 && y > 1; --y)
        if (isSolid(world.getBlock(x, y - 1, z)) && !isSolid(world.getBlock(x, y, z)) && !isSolid(world.getBlock(x, y + 1, z)))
        {
            ground = y;
            break;
        }
    if (ground < 0 || world.skyLight(x, ground, z) < 15) return false;
    const int members = 2 + static_cast<int>(random01() * 4.0f);
    bool led = false;
    for (int k = 0; k < members; ++k)
    {
        const glm::vec3 feet(x + 0.5f + (k % 3) - 1.0f, static_cast<float>(ground), z + 0.5f + (k / 3));
        const MobId kind = (m_difficulty >= 3 && k > 0 && random01() < 0.3f) ? MobId::Vindicator : MobId::Pillager;
        if (!spawnAt(kind, feet, world) && !spawnAt(kind, glm::vec3(x + 0.5f, static_cast<float>(ground), z + 0.5f), world)) continue;
        Mob& member = m_mobs.back();
        member.dress(m_difficulty);
        member.keepFromDespawning();
        if (!led)
        {
            member.setCaptain(true);
            led = true;
        }
    }
    return led;
}

void EntityManager::update(float deltaTime, const World& world,
                           const glm::vec3& playerPosition, float daylight)
{
    m_daylight = daylight;
    const glm::vec3 target = m_playerHidden ? playerPosition + glm::vec3(0.0f, 100000.0f, 0.0f) : playerPosition;
    for (Mob& mob : m_mobs)
    {
        glm::vec3 aim = target;
        int victim = -1;
        float nearest = glm::length(mob.position() - target);
        for (size_t k = 0; k < m_others.size(); ++k)
        {
            const float apart = glm::length(mob.position() - m_others[k]);
            if (apart < nearest)
            {
                nearest = apart;
                aim = m_others[k];
                victim = static_cast<int>(k);
            }
        }
        if (mob.typeId() == MobId::Creeper || mob.typeId() == MobId::Phantom)
        {
            const float wary = mob.typeId() == MobId::Creeper ? 6.0f : 16.0f;
            for (const Mob& other : m_mobs)
            {
                if ((other.typeId() != MobId::Cat && other.typeId() != MobId::Ocelot) || !other.alive()) continue;
                glm::vec3 away = mob.position() - other.position();
                if (glm::length(away) >= wary) continue;
                away.y = mob.typeId() == MobId::Phantom ? 0.0f : away.y;
                if (glm::length(away) < 0.01f) away = glm::vec3(1.0f, 0.0f, 0.0f);
                aim = mob.position() + glm::normalize(away) * 12.0f + (mob.typeId() == MobId::Phantom ? glm::vec3(0.0f, 6.0f, 0.0f) : glm::vec3(0.0f));
                victim = -2;
                break;
            }
        }
        mob.setVictim(victim);
        mob.setAim(aim);
        const bool unheard = mob.typeId() == MobId::Warden && m_playerQuiet && victim < 0 && glm::length(mob.position() - aim) > 5.0f;
        mob.update(deltaTime, world, unheard ? aim + glm::vec3(0.0f, 100000.0f, 0.0f) : aim, daylight);

        // Caught here rather than at removal: the corpse lingers, and
        // whatever it drops belongs where it fell, not where it stopped.
        if (mob.takeDeathReport())
        {
            Death death{ mob.typeId(), mob.position(), mob.baby(), mob.variant(), mob.serial(), mob.shotByMob() };
            death.oozing = mob.effects().has(Effects::Kind::Oozing);
            death.weaving = mob.effects().has(Effects::Kind::Weaving);
            death.windCharged = mob.effects().has(Effects::Kind::WindCharged);
            death.byPlayer = mob.hitByPlayer();
            death.size = mob.size();
            for (int slot = 0; slot < Mob::GearSlots; ++slot)
            {
                death.gear[static_cast<size_t>(slot)] = mob.gear(slot);
                death.gearSure[static_cast<size_t>(slot)] = mob.gearSure(slot);
            }
            death.chargedBlast = mob.chargedBlastHit();
            death.saddled = mob.saddled();
            death.worn = mob.horseArmour();
            death.how = mob.playerHitHow();
            death.shot = mob.playerShot();
            death.distance = mob.playerHitDistance();
            death.captain = mob.captain();
            m_deaths.push_back(death);
        }
        if (const int silverfish = mob.takeSilverfish())
            m_infestations.emplace_back(mob.position() + glm::vec3(0.0f, mob.height() * 0.5f, 0.0f), silverfish);

        if (mob.takeGrazeReport()) m_grazes.push_back(mob.position());
        if (mob.takeEggReport()) (mob.typeId() == MobId::Armadillo ? m_shedScutes : m_laidEggs).push_back(mob.position());
        if (mob.takeGrowthReport() && mob.typeId() == MobId::Turtle) m_grownTurtles.push_back(mob.position());
        if (mob.typeId() == MobId::Tadpole && mob.alive() && mob.age() + mob.growthBoost() >= Mob::TADPOLE_SECONDS)
        {
            m_grownTadpoles.push_back(mob.position());
            mob.vanish();
        }
        if (mob.typeId() == MobId::ZombieVillager && mob.alive() && mob.cureDone(deltaTime))
        {
            Zombified cured{ mob.position(), mob.yaw(), mob.baby() };
            cured.into = MobId::Villager;
            m_zombified.push_back(cured);
            mob.emitNamed("mob/zombie/unfect", Sound::MobVillagerSay, 1.0f, 1.0f);
            mob.vanish();
        }
        if (mob.typeId() == MobId::WanderingTrader && mob.alive() && mob.age() > 2400.0f && mob.customName().empty() && !mob.leashed())
            mob.vanish();
        if (mob.typeId() == MobId::Hoglin && mob.alive())
        {
            mob.addZombifyTime(m_dimension == Dimension::Nether ? -1.0f : deltaTime);
            if (mob.zombifyTime() > ZOMBIFY_SECONDS)
            {
                m_zombified.push_back(Zombified{ mob.position(), mob.yaw(), mob.baby() });
                mob.vanish();
            }
        }
        if (mob.alive())
        {
            const MobId kind = mob.typeId();
            const glm::vec3 at = mob.position();
            const int fx = static_cast<int>(std::floor(at.x)), fz = static_cast<int>(std::floor(at.z));
            MobId into = MobId::Count;
            bool condition = false;
            float needed = 0.0f;
            if (kind == MobId::Piglin || kind == MobId::PiglinBrute)
            {
                into = MobId::ZombiePigman;
                condition = m_dimension != Dimension::Nether;
                needed = ZOMBIFY_SECONDS;
            }
            else if (kind == MobId::Zombie || kind == MobId::Husk)
            {
                into = kind == MobId::Zombie ? MobId::Drowned : MobId::Zombie;
                condition = isWater(world.getBlock(fx, static_cast<int>(std::floor(at.y + mob.height() * 0.85f)), fz));
                needed = 45.0f;
            }
            else if (kind == MobId::Skeleton)
            {
                into = MobId::Stray;
                condition = world.getBlock(fx, static_cast<int>(std::floor(at.y + 0.1f)), fz) == Blocks::PowderSnow;
                needed = 7.0f;
            }
            if (into != MobId::Count)
            {
                mob.addZombifyTime(condition ? deltaTime : -1.0f);
                if (mob.zombifyTime() > needed)
                {
                    Zombified turned{ at, mob.yaw(), mob.baby() };
                    turned.into = into;
                    m_zombified.push_back(turned);
                    mob.vanish();
                }
            }
        }
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

    for (Mob& goat : m_mobs)
    {
        if (goat.typeId() != MobId::Goat || !goat.alive() || goat.baby() || goat.ridden()) continue;
        if (!goat.ramReady(deltaTime)) continue;
        const float radians = glm::radians(goat.yaw());
        const glm::vec3 forward(std::cos(radians), 0.0f, std::sin(radians));
        goat.shove(forward * 7.0f);
        if (goat.hornsLeft() <= 0) continue;
        const glm::vec3 ahead = goat.position() + forward * (goat.type().width * 0.5f + 0.9f) + glm::vec3(0.0f, 0.5f, 0.0f);
        const BlockId struck = world.getBlock(static_cast<int>(std::floor(ahead.x)), static_cast<int>(std::floor(ahead.y)),
                                              static_cast<int>(std::floor(ahead.z)));
        if (!breaksHorn(struck)) continue;
        goat.loseHorn();
        const int variant = (goat.screamingGoat() ? 4 : 0) + static_cast<int>((goat.birthSeed() >> 3) + static_cast<uint32_t>(goat.hornsLeft())) % 4;
        m_horns.push_back(ShedHorn{ ahead - forward * 0.7f, variant });
    }

    for (Mob& mob : m_mobs)
        if (mob.finished() && !mob.sounds().empty())
        {
            m_orphanSounds.insert(m_orphanSounds.end(), mob.sounds().begin(), mob.sounds().end());
            mob.sounds().clear();
        }

    // Drop anything dead-and-done, or far enough off that nobody will
    // miss it.
    m_mobs.erase(
        std::remove_if(m_mobs.begin(), m_mobs.end(), [&](const Mob& mob) {
            if (mob.finished()) return true;
            if (isTechnicalMob(mob.typeId())) return false;
            if (m_peaceful && mob.typeId() != MobId::EnderDragon && mobType(mob.typeId()).spawnClass == SpawnClass::Hostile) return true;
            glm::vec3 delta = mob.position() - playerPosition;
            for (const glm::vec3& other : m_others)
                if (glm::length(mob.position() - other) < glm::length(delta)) delta = mob.position() - other;
            const float keep = (mob.customName().empty() && !mob.leashed() && !mob.persistent()) ? DESPAWN_DISTANCE : DESPAWN_DISTANCE * 3.0f;
            return glm::length(glm::vec3(delta.x, 0.0f, delta.z)) > keep;
        }),
        m_mobs.end());

    {
        auto isBoat = [](MobId id) {
            return id == MobId::Boat || id == MobId::ChestBoat || id == MobId::Raft || id == MobId::ChestRaft;
        };
        for (size_t b = 0; b < m_mobs.size(); ++b)
        {
            const Mob& boat = m_mobs[b];
            if (!boat.alive() || !isBoat(boat.typeId())) continue;
            const int seats = (boat.typeId() == MobId::ChestBoat || boat.typeId() == MobId::ChestRaft) ? 1 : 2;
            int taken = boat.ridden() ? 1 : 0;
            for (const Mob& other : m_mobs)
                if (other.mount() == boat.serial()) ++taken;
            if (taken >= seats) continue;
            for (Mob& passenger : m_mobs)
            {
                if (taken >= seats) break;
                if (&passenger == &boat || !passenger.alive() || passenger.mount() != 0 || passenger.ridden() || passenger.leashed()) continue;
                const MobId kind = passenger.typeId();
                if (isBoat(kind) || kind == MobId::Minecart || mobType(kind).aquatic || passenger.width() >= 1.375f) continue;
                if (kind == MobId::EnderDragon || kind == MobId::Wither || kind == MobId::Warden || kind == MobId::Ghast) continue;
                const glm::vec3 apart = passenger.position() - boat.position();
                if (std::fabs(apart.y) > 0.8f || glm::length(glm::vec2(apart.x, apart.z)) > 0.9f) continue;
                passenger.ride(boat.serial());
                ++taken;
            }
        }

        std::unordered_map<uint32_t, size_t> bySerial;
        for (size_t k = 0; k < m_mobs.size(); ++k) bySerial[m_mobs[k].serial()] = k;
        for (Mob& rider : m_mobs)
        {
            if (rider.mount() == 0) continue;
            const auto found = bySerial.find(rider.mount());
            if (!rider.alive() || found == bySerial.end() || !m_mobs[found->second].alive())
            {
                rider.ride(0);
                continue;
            }
            Mob& steed = m_mobs[found->second];
            if (isBoat(steed.typeId()))
            {
                glm::vec3 seat = steed.position() + glm::vec3(0.0f, 0.1f, 0.0f);
                const glm::vec3 away(steed.position().x - target.x, 0.0f, steed.position().z - target.z);
                if (steed.ridden() && glm::length(away) > 0.01f) seat += glm::normalize(away) * 0.6f;
                rider.seatAt(seat);
                continue;
            }
            rider.seatAt(steed.position() + glm::vec3(0.0f, steed.height() * 0.75f, 0.0f));
            const glm::vec3 toPlayer = target - steed.position();
            if (mobType(rider.typeId()).spawnClass == SpawnClass::Hostile && glm::length(toPlayer) < 16.0f)
                steed.huntTowards(target);
        }
    }

    tickHelpers(deltaTime, world, target);

    m_golemTimer -= deltaTime;
    if (m_golemTimer <= 0.0f)
    {
        m_golemTimer = 5.0f;
        for (const Mob& villager : m_mobs)
        {
            if (!villager.alive() || villager.typeId() != MobId::Villager || villager.baby()) continue;
            const glm::vec3 at = villager.position();
            int neighbours = 0;
            bool threatened = false;
            bool guarded = false;
            for (const Mob& other : m_mobs)
            {
                if (!other.alive()) continue;
                const float apart = glm::length(other.position() - at);
                if (other.typeId() == MobId::Villager && apart < 16.0f) ++neighbours;
                if (other.typeId() == MobId::IronGolem && apart < 32.0f) guarded = true;
                if (mobType(other.typeId()).spawnClass == SpawnClass::Hostile && apart < 10.0f) threatened = true;
            }
            if (neighbours >= 3 && threatened && !guarded)
            {
                const float angle = random01() * 6.2831853f;
                const glm::vec3 spot = at + glm::vec3(std::cos(angle) * 3.0f, 0.0f, std::sin(angle) * 3.0f);
                if (spawnAt(MobId::IronGolem, spot, world) || spawnAt(MobId::IronGolem, at + glm::vec3(0.0f, 0.5f, 0.0f), world))
                {
                    m_golemTimer = 30.0f;
                    break;
                }
            }
        }
    }

    setWolvesOnHostiles(deltaTime, target);
    courtVillagers(world, deltaTime);
    pairOffLovers(world);
    if (!m_playerHidden) routeChasers(world, playerPosition);

    m_spawnTimer -= deltaTime;
    if (m_spawnTimer <= 0.0f)
    {
        m_spawnTimer = SPAWN_INTERVAL;
        if (m_naturalSpawning) trySpawnWave(world, playerPosition, daylight);
    }
}

void EntityManager::routeChasers(const World& world, const glm::vec3& hostPosition)
{
    WorldGround ground;
    ground.world = &world;

    int searches = 0;
    for (Mob& mob : m_mobs)
    {
        if (searches >= PATHS_PER_UPDATE) break;
        if (!mob.wantsPath()) continue;
        const glm::vec3 playerPosition = mob.victim() != -1 ? mob.aim() : hostPosition;
        const glm::ivec3 goal(static_cast<int>(std::floor(playerPosition.x)), static_cast<int>(std::floor(playerPosition.y)),
                              static_cast<int>(std::floor(playerPosition.z)));

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
        if (mob.typeId() == MobId::Enderman) mob.emitNamed("mob/endermen/stare", Sound::MobEndermanSay, 1.0f, 1.0f);
    }
}

std::vector<MobSound> EntityManager::drainSounds()
{
    std::vector<MobSound> all;
    all.swap(m_orphanSounds);
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
        for (MobStrike& strike : mob.strikes())
        {
            strike.attacker = static_cast<int>(mob.typeId());
            strike.victim = mob.victim();
        }
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
        for (MobShot& shot : mob.shots()) shot.shooter = static_cast<int>(mob.typeId());
        all.insert(all.end(), mob.shots().begin(), mob.shots().end());
        mob.shots().clear();
    }
    return all;
}

bool EntityManager::breaksHorn(BlockId block)
{
    if (isLog(block)) return true;
    switch (block)
    {
        case Blocks::Stone: case Blocks::Cobblestone: case Blocks::Deepslate: case Blocks::Andesite: case Blocks::Granite:
        case Blocks::Diorite: case Blocks::Tuff: case Blocks::Calcite: case Blocks::PackedIce:
        case Blocks::CoalOre: case Blocks::IronOre: case Blocks::GoldOre: case Blocks::CopperOre: case Blocks::EmeraldOre:
            return true;
        default:
            return false;
    }
}

std::vector<MobBlast> EntityManager::drainBlasts()
{
    std::vector<MobBlast> all;
    for (Mob& mob : m_mobs)
    {
        if (mob.blasts().empty()) continue;
        for (MobBlast& blast : mob.blasts()) blast.source = static_cast<int>(mob.typeId());
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
    if (!Mob::canBeRidden(target->type(), target->tamed(), target->baby(), target->saddled()))
        return nullptr;
    return target;
}

// A tamed wolf goes for whatever hostile is nearest. The wolf cannot
// see the other mobs -- only this can -- so the quarry is picked here
// and handed to it as a heading.
void EntityManager::setWolvesOnHostiles(float deltaTime, const glm::vec3& playerPosition)
{
    for (size_t i = 0; i < m_mobs.size(); ++i)
    {
        if (m_mobs[i].typeId() == MobId::TraderLlama && m_mobs[i].alive() && !m_mobs[i].hostileNow())
        {
            const glm::vec3 from = m_mobs[i].position();
            float nearest = 24.0f;
            const Mob* trader = nullptr;
            for (const Mob& other : m_mobs)
            {
                if (other.typeId() != MobId::WanderingTrader || !other.alive()) continue;
                const float away = glm::length(other.position() - from);
                if (away < nearest)
                {
                    nearest = away;
                    trader = &other;
                }
            }
            if (trader && nearest > 4.0f) m_mobs[i].huntTowards(trader->position());
            continue;
        }
        if (m_mobs[i].typeId() == MobId::Zoglin && m_mobs[i].alive())
        {
            const glm::vec3 from = m_mobs[i].position();
            const glm::vec3 toPlayer = playerPosition - from;
            float nearest = std::min(Mob::HUNT_RANGE, glm::length(glm::vec3(toPlayer.x, 0.0f, toPlayer.z)));
            size_t quarry = m_mobs.size();
            for (size_t j = 0; j < m_mobs.size(); ++j)
            {
                if (j == i || !m_mobs[j].alive()) continue;
                const MobId kind = m_mobs[j].typeId();
                if (kind == MobId::Zoglin || kind == MobId::Creeper || m_mobs[j].type().inanimate) continue;
                const glm::vec3 apart = m_mobs[j].position() - from;
                const float away = glm::length(glm::vec3(apart.x, 0.0f, apart.z));
                if (away >= nearest) continue;
                nearest = away;
                quarry = j;
            }
            if (quarry >= m_mobs.size()) continue;
            m_mobs[i].huntTowards(m_mobs[quarry].position());
            const MobType& t = m_mobs[i].type();
            if (nearest <= t.attackReach + t.width * 0.5f && m_mobs[i].tryBite(deltaTime))
            {
                const glm::vec3 away = m_mobs[quarry].position() - from;
                m_mobs[quarry].damage(m_mobs[i].baby() ? std::max(1, t.attackDamage / 2) : t.attackDamage, away);
                m_mobs[quarry].shove(glm::vec3(0.0f, 4.0f, 0.0f));
            }
            continue;
        }

        {
            const MobId hunter = m_mobs[i].typeId();
            const bool johnny = hunter == MobId::Vindicator && m_mobs[i].customName() == "Johnny";
            auto preys = [&](MobId prey) {
                if (johnny)
                    return prey != MobId::Vindicator && prey != MobId::Pillager && prey != MobId::Evoker && prey != MobId::Illusioner &&
                           prey != MobId::Vex && prey != MobId::Ravager && prey != MobId::Witch && prey != MobId::Ghast;
                switch (hunter)
                {
                    case MobId::Zombie: case MobId::Husk: case MobId::Drowned: case MobId::ZombieVillager:
                        return prey == MobId::Villager || prey == MobId::WanderingTrader || prey == MobId::IronGolem ||
                               (prey == MobId::Turtle);
                    case MobId::Pillager: case MobId::Vindicator: case MobId::Evoker: case MobId::Ravager: case MobId::Illusioner:
                        return prey == MobId::Villager || prey == MobId::WanderingTrader || prey == MobId::IronGolem;
                    case MobId::Wolf:
                        return !m_mobs[i].tamed() && (prey == MobId::Sheep || prey == MobId::Rabbit || prey == MobId::Fox ||
                                                      prey == MobId::Skeleton || prey == MobId::Stray || prey == MobId::Bogged);
                    case MobId::Fox:
                        return prey == MobId::Chicken || prey == MobId::Rabbit || prey == MobId::Cod || prey == MobId::Salmon;
                    case MobId::Cat:
                        return !m_mobs[i].tamed() && (prey == MobId::Rabbit || prey == MobId::Chicken);
                    case MobId::Ocelot:
                        return prey == MobId::Chicken;
                    case MobId::Axolotl:
                        return prey == MobId::Cod || prey == MobId::Salmon || prey == MobId::TropicalFish || prey == MobId::Squid ||
                               prey == MobId::GlowSquid || prey == MobId::Drowned || prey == MobId::Guardian || prey == MobId::Pufferfish;
                    case MobId::Wither:
                        return prey != MobId::Wither && prey != MobId::WitherSkeleton && prey != MobId::Skeleton && prey != MobId::Zombie &&
                               prey != MobId::ZombiePigman && prey != MobId::Drowned && prey != MobId::Husk && prey != MobId::Stray &&
                               prey != MobId::Phantom && prey != MobId::Zoglin && prey != MobId::ZombieVillager;
                    default:
                        return false;
                }
            };
            const bool hunts = hunter == MobId::Zombie || hunter == MobId::Husk || hunter == MobId::Drowned ||
                               hunter == MobId::ZombieVillager || hunter == MobId::Pillager || hunter == MobId::Vindicator ||
                               hunter == MobId::Evoker || hunter == MobId::Ravager || hunter == MobId::Illusioner ||
                               (hunter == MobId::Wolf && !m_mobs[i].tamed()) || hunter == MobId::Fox ||
                               (hunter == MobId::Cat && !m_mobs[i].tamed()) || hunter == MobId::Ocelot || hunter == MobId::Axolotl ||
                               hunter == MobId::Wither;
            if (hunts && m_mobs[i].alive() && !m_mobs[i].baby())
            {
                const glm::vec3 from = m_mobs[i].position();
                const glm::vec3 toPlayer = playerPosition - from;
                const bool hostile = mobType(hunter).spawnClass == SpawnClass::Hostile;
                float nearest = std::min(Mob::HUNT_RANGE, hostile ? glm::length(glm::vec3(toPlayer.x, 0.0f, toPlayer.z)) : Mob::HUNT_RANGE);
                size_t quarry = m_mobs.size();
                for (size_t j = 0; j < m_mobs.size(); ++j)
                {
                    if (j == i || !m_mobs[j].alive() || m_mobs[j].type().inanimate) continue;
                    if (!preys(m_mobs[j].typeId())) continue;
                    if (m_mobs[j].typeId() == MobId::Turtle && !m_mobs[j].baby()) continue;
                    const glm::vec3 apart = m_mobs[j].position() - from;
                    const float away = glm::length(glm::vec3(apart.x, 0.0f, apart.z));
                    if (away >= nearest || std::fabs(apart.y) > 4.0f) continue;
                    nearest = away;
                    quarry = j;
                }
                if (quarry < m_mobs.size())
                {
                    m_mobs[i].huntTowards(m_mobs[quarry].position());
                    const MobType& t = m_mobs[i].type();
                    const float reach = std::max(Mob::BITE_REACH, t.attackReach + t.width * 0.5f);
                    if (nearest <= reach && m_mobs[i].tryBite(deltaTime))
                    {
                        const glm::vec3 away = m_mobs[quarry].position() - from;
                        const int bite = t.attackDamage > 0 ? t.attackDamage : Mob::BITE_DAMAGE;
                        m_mobs[quarry].damage(bite, away);
                        if (hunter == MobId::Axolotl && !m_mobs[quarry].alive() && m_mobs[quarry].hitByPlayer()) ++m_axolotlAssists;
                        const bool zombie = hunter == MobId::Zombie || hunter == MobId::Husk || hunter == MobId::Drowned ||
                                            hunter == MobId::ZombieVillager;
                        if (zombie && m_mobs[quarry].typeId() == MobId::Villager && !m_mobs[quarry].alive() &&
                            (m_difficulty >= 3 || (m_difficulty == 2 && random01() < 0.5f)))
                        {
                            Zombified bitten{ m_mobs[quarry].position(), m_mobs[quarry].yaw(), m_mobs[quarry].baby() };
                            bitten.into = MobId::ZombieVillager;
                            m_zombified.push_back(bitten);
                            m_mobs[i].emitNamed("mob/zombie/infect", Sound::MobZombieSay, 1.0f, 1.0f);
                            m_mobs[quarry].vanish();
                        }
                    }
                    continue;
                }
            }
        }

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
            const MobId ta = m_mobs[a].typeId(), tb = m_mobs[b].typeId();
            const bool mule = (ta == MobId::Horse && tb == MobId::Donkey) || (ta == MobId::Donkey && tb == MobId::Horse);
            if (tb != ta && !mule) continue;
            if (!m_mobs[b].inLove() || !m_mobs[b].canBreed()) continue;

            const glm::vec3 apart = m_mobs[b].position() - m_mobs[a].position();
            if (glm::length(apart) > 3.0f) continue;

            const glm::vec3 between = m_mobs[a].position() + apart * 0.5f;
            m_mobs[a].onBred();
            m_mobs[b].onBred();

            if (m_mobs[a].typeId() == MobId::Turtle)
            {
                m_turtleNests.push_back(between);
                break;
            }
            if (m_mobs[a].typeId() == MobId::Sniffer)
            {
                m_snifferNests.push_back(m_mobs[a].position());
                break;
            }
            if (m_mobs[a].typeId() == MobId::Frog)
            {
                m_frogSpawns.push_back(between);
                break;
            }

            if (static_cast<int>(m_mobs.size()) < MAX_MOBS &&
                fits(world, mobType(m_mobs[a].typeId()), between))
            {
                // Read before the push: emplace_back can move the
                // whole vector, and the parent reference would be
                // pointing at freed memory by the time it is asked.
                const bool inherits = m_mobs[a].hasHome();
                const glm::vec3 parentHome = m_mobs[a].home();

                const int woolA = m_mobs[a].woolColour(), woolB = m_mobs[b].woolColour();
                m_mobs.emplace_back(mule ? MobId::Mule : m_mobs[a].typeId(), between,
                                    m_rng ^ static_cast<uint32_t>(m_mobs.size() * 40503u), true);
                if (m_mobs.back().typeId() == MobId::Sheep) m_mobs.back().setWool(random01() < 0.5f ? woolA : woolB, false);

                // A child belongs where its parents do, and takes up
                // the family trade. Born without either it would walk
                // out of the village as soon as it could.
                if (inherits)
                {
                    m_mobs.back().setHome(parentHome, static_cast<float>(Village::MAX_RADIUS));
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

std::vector<glm::vec3> EntityManager::drainTurtleNests()
{
    std::vector<glm::vec3> all;
    all.swap(m_turtleNests);
    return all;
}

std::vector<glm::vec3> EntityManager::drainSnifferNests()
{
    std::vector<glm::vec3> all;
    all.swap(m_snifferNests);
    return all;
}

std::vector<glm::vec3> EntityManager::drainGrownTurtles()
{
    std::vector<glm::vec3> all;
    all.swap(m_grownTurtles);
    return all;
}

std::vector<glm::vec3> EntityManager::drainLaidEggs()
{
    std::vector<glm::vec3> all;
    all.swap(m_laidEggs);
    return all;
}

std::vector<std::pair<glm::vec3, int>> EntityManager::drainInfestations()
{
    std::vector<std::pair<glm::vec3, int>> all;
    all.swap(m_infestations);
    return all;
}

std::vector<glm::vec3> EntityManager::drainFrogMeals()
{
    std::vector<glm::vec3> all;
    all.swap(m_frogMeals);
    return all;
}

std::vector<glm::vec3> EntityManager::drainShedScutes()
{
    std::vector<glm::vec3> all;
    all.swap(m_shedScutes);
    return all;
}

std::vector<glm::vec3> EntityManager::drainFrogSpawns()
{
    std::vector<glm::vec3> all;
    all.swap(m_frogSpawns);
    return all;
}

std::vector<EntityManager::Zombified> EntityManager::drainZombified()
{
    std::vector<Zombified> all;
    all.swap(m_zombified);
    return all;
}

std::vector<glm::vec3> EntityManager::drainGrownTadpoles()
{
    std::vector<glm::vec3> all;
    all.swap(m_grownTadpoles);
    return all;
}

Mob* EntityManager::pick(const glm::vec3& origin, const glm::vec3& direction, float maxDistance)
{
    Mob* best = nullptr;
    float bestDistance = maxDistance;

    for (Mob& mob : m_mobs)
    {
        if (!mob.alive() || isTechnicalMob(mob.typeId())) continue;

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
