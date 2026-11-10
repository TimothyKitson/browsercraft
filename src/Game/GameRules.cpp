#include "GameRules.h"
#include <cstdlib>
#include <sstream>

namespace
{
    using GameRules::Category;
    const GameRules::Info INFO[GameRules::COUNT] = {
        { "doFireTick", "Update fire", Category::Updates, true, 1 },
        { "mobGriefing", "Allow destructive mob actions", Category::Mobs, true, 1 },
        { "keepInventory", "Keep inventory after death", Category::Player, true, 0 },
        { "doMobSpawning", "Spawn mobs", Category::Spawning, true, 1 },
        { "doMobLoot", "Drop mob loot", Category::Drops, true, 1 },
        { "projectilesCanBreakBlocks", "Projectiles can break blocks", Category::Drops, true, 1 },
        { "doTileDrops", "Drop blocks", Category::Drops, true, 1 },
        { "doEntityDrops", "Drop entity equipment", Category::Drops, true, 1 },
        { "commandBlockOutput", "Broadcast command block output", Category::Chat, true, 1 },
        { "naturalRegeneration", "Regenerate health", Category::Player, true, 1 },
        { "doDaylightCycle", "Advance time of day", Category::Updates, true, 1 },
        { "logAdminCommands", "Broadcast admin commands", Category::Chat, true, 1 },
        { "showDeathMessages", "Show death messages", Category::Chat, true, 1 },
        { "randomTickSpeed", "Random tick speed rate", Category::Updates, false, 3 },
        { "sendCommandFeedback", "Send command feedback", Category::Chat, true, 1 },
        { "reducedDebugInfo", "Reduce debug info", Category::Misc, true, 0 },
        { "spectatorsGenerateChunks", "Allow spectators to generate terrain", Category::Player, true, 1 },
        { "spawnRadius", "Respawn location radius", Category::Player, false, 10 },
        { "disableElytraMovementCheck", "Disable elytra movement check", Category::Player, true, 0 },
        { "maxEntityCramming", "Entity cramming threshold", Category::Mobs, false, 24 },
        { "doWeatherCycle", "Update weather", Category::Updates, true, 1 },
        { "doLimitedCrafting", "Require recipe for crafting", Category::Player, true, 0 },
        { "maxCommandChainLength", "Command chain size limit", Category::Misc, false, 65536 },
        { "maxCommandForkCount", "Command context limit", Category::Misc, false, 65536 },
        { "commandModificationBlockLimit", "Command modification block limit", Category::Misc, false, 32768 },
        { "announceAdvancements", "Announce advancements", Category::Chat, true, 1 },
        { "disableRaids", "Disable raids", Category::Mobs, true, 0 },
        { "doInsomnia", "Spawn phantoms", Category::Spawning, true, 1 },
        { "doImmediateRespawn", "Respawn immediately", Category::Player, true, 0 },
        { "playersNetherPortalDefaultDelay", "Player's Nether portal delay in non-creative mode", Category::Player, false, 80 },
        { "playersNetherPortalCreativeDelay", "Player's Nether portal delay in creative mode", Category::Player, false, 1 },
        { "drowningDamage", "Deal drowning damage", Category::Player, true, 1 },
        { "fallDamage", "Deal fall damage", Category::Player, true, 1 },
        { "fireDamage", "Deal fire damage", Category::Player, true, 1 },
        { "freezeDamage", "Deal freeze damage", Category::Player, true, 1 },
        { "doPatrolSpawning", "Spawn pillager patrols", Category::Spawning, true, 1 },
        { "doTraderSpawning", "Spawn Wandering Traders", Category::Spawning, true, 1 },
        { "doWardenSpawning", "Spawn Wardens", Category::Spawning, true, 1 },
        { "forgiveDeadPlayers", "Forgive dead players", Category::Mobs, true, 1 },
        { "universalAnger", "Universal anger", Category::Mobs, true, 0 },
        { "playersSleepingPercentage", "Sleep percentage", Category::Player, false, 100 },
        { "blockExplosionDropDecay", "In block interaction explosions, some blocks won't drop their loot", Category::Drops, true, 1 },
        { "mobExplosionDropDecay", "In mob explosions, some blocks won't drop their loot", Category::Drops, true, 1 },
        { "tntExplosionDropDecay", "In TNT explosions, some blocks won't drop their loot", Category::Drops, true, 0 },
        { "snowAccumulationHeight", "Snow accumulation height", Category::Updates, false, 1 },
        { "waterSourceConversion", "Water converts to source", Category::Updates, true, 1 },
        { "lavaSourceConversion", "Lava converts to source", Category::Updates, true, 0 },
        { "globalSoundEvents", "Global sound events", Category::Misc, true, 1 },
        { "doVinesSpread", "Vines spread", Category::Updates, true, 1 },
        { "enderPearlsVanishOnDeath", "Thrown Ender Pearls vanish on death", Category::Player, true, 1 },
        { "spawnChunkRadius", "Spawn chunk radius", Category::Misc, false, 2 },
    };
}

namespace GameRules
{
    const Info& info(Rule rule) { return INFO[static_cast<int>(rule)]; }

    bool find(const std::string& name, Rule& out)
    {
        for (int i = 0; i < COUNT; ++i)
            if (name == INFO[i].name)
            {
                out = static_cast<Rule>(i);
                return true;
            }
        return false;
    }

    const char* categoryTitle(Category category)
    {
        switch (category)
        {
            case Category::Player: return "Player";
            case Category::Mobs: return "Mobs";
            case Category::Spawning: return "Spawning";
            case Category::Drops: return "Drops";
            case Category::Updates: return "World Updates";
            case Category::Chat: return "Chat";
            default: return "Miscellaneous";
        }
    }

    Set::Set() { reset(); }

    void Set::reset()
    {
        for (int i = 0; i < COUNT; ++i) m_values[i] = INFO[i].fallback;
    }

    void Set::set(Rule rule, int value)
    {
        const int i = static_cast<int>(rule);
        if (i < 0 || i >= COUNT) return;
        m_values[i] = INFO[i].flag ? (value != 0 ? 1 : 0) : value;
    }

    bool Set::isDefault() const
    {
        for (int i = 0; i < COUNT; ++i)
            if (m_values[i] != INFO[i].fallback) return false;
        return true;
    }

    std::string Set::save() const
    {
        std::ostringstream out;
        for (int i = 0; i < COUNT; ++i) out << INFO[i].name << "=" << m_values[i] << "\n";
        return out.str();
    }

    void Set::load(const std::string& text)
    {
        reset();
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line))
        {
            const size_t equals = line.find('=');
            if (equals == std::string::npos) continue;
            Rule rule;
            if (!find(line.substr(0, equals), rule)) continue;
            set(rule, std::atoi(line.c_str() + equals + 1));
        }
    }
}
