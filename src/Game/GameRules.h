#pragma once
#include <string>

namespace GameRules
{
    enum class Rule
    {
        DoFireTick,
        MobGriefing,
        KeepInventory,
        DoMobSpawning,
        DoMobLoot,
        ProjectilesCanBreakBlocks,
        DoTileDrops,
        DoEntityDrops,
        CommandBlockOutput,
        NaturalRegeneration,
        DoDaylightCycle,
        LogAdminCommands,
        ShowDeathMessages,
        RandomTickSpeed,
        SendCommandFeedback,
        ReducedDebugInfo,
        SpectatorsGenerateChunks,
        SpawnRadius,
        DisableElytraMovementCheck,
        MaxEntityCramming,
        DoWeatherCycle,
        DoLimitedCrafting,
        MaxCommandChainLength,
        MaxCommandForkCount,
        CommandModificationBlockLimit,
        AnnounceAdvancements,
        DisableRaids,
        DoInsomnia,
        DoImmediateRespawn,
        PlayersNetherPortalDefaultDelay,
        PlayersNetherPortalCreativeDelay,
        DrowningDamage,
        FallDamage,
        FireDamage,
        FreezeDamage,
        DoPatrolSpawning,
        DoTraderSpawning,
        DoWardenSpawning,
        ForgiveDeadPlayers,
        UniversalAnger,
        PlayersSleepingPercentage,
        BlockExplosionDropDecay,
        MobExplosionDropDecay,
        TntExplosionDropDecay,
        SnowAccumulationHeight,
        WaterSourceConversion,
        LavaSourceConversion,
        GlobalSoundEvents,
        DoVinesSpread,
        EnderPearlsVanishOnDeath,
        SpawnChunkRadius,
        Count
    };

    constexpr int COUNT = static_cast<int>(Rule::Count);

    enum class Category { Player, Mobs, Spawning, Drops, Updates, Chat, Misc };

    struct Info
    {
        const char* name;
        const char* title;
        Category category;
        bool flag;
        int fallback;
    };

    const Info& info(Rule rule);
    bool find(const std::string& name, Rule& out);
    const char* categoryTitle(Category category);

    class Set
    {
    public:
        Set();
        int value(Rule rule) const { return m_values[static_cast<int>(rule)]; }
        bool on(Rule rule) const { return value(rule) != 0; }
        void set(Rule rule, int value);
        void reset();
        std::string save() const;
        void load(const std::string& text);
        bool isDefault() const;

    private:
        int m_values[COUNT];
    };
}
