#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Stats
{
    enum class Custom : uint8_t
    {
        LeaveGame, PlayTime, TotalWorldTime, TimeSinceDeath, TimeSinceRest, SneakTime, WalkOneCm, CrouchOneCm, SprintOneCm,
        WalkOnWaterOneCm, FallOneCm, ClimbOneCm, FlyOneCm, WalkUnderWaterOneCm, MinecartOneCm, BoatOneCm, PigOneCm, HorseOneCm,
        AviateOneCm, SwimOneCm, StriderOneCm, Jump, Drop, DamageDealt, DamageDealtAbsorbed, DamageDealtResisted, DamageTaken,
        DamageBlockedByShield, DamageAbsorbed, DamageResisted, Deaths, MobKills, AnimalsBred, PlayerKills, FishCaught,
        TalkedToVillager, TradedWithVillager, EatCakeSlice, FillCauldron, UseCauldron, CleanArmor, CleanBanner, CleanShulkerBox,
        InteractWithBrewingstand, InteractWithBeacon, InspectDropper, InspectHopper, InspectDispenser, PlayNoteblock, TuneNoteblock,
        PotFlower, TriggerTrappedChest, OpenEnderchest, EnchantItem, PlayRecord, InteractWithFurnace, InteractWithCraftingTable,
        OpenChest, SleepInBed, OpenShulkerBox, OpenBarrel, InteractWithBlastFurnace, InteractWithSmoker, InteractWithLectern,
        InteractWithCampfire, InteractWithCartographyTable, InteractWithLoom, InteractWithStonecutter, BellRing, RaidTrigger,
        RaidWin, InteractWithAnvil, InteractWithGrindstone, TargetHit, InteractWithSmithingTable,
        Count
    };
    constexpr int CUSTOM_COUNT = static_cast<int>(Custom::Count);

    enum class Format : uint8_t { Plain, Time, Distance, Tenths };
    enum class ItemStat : uint8_t { Mined, Broken, Crafted, Used, PickedUp, Dropped, Count };
    constexpr int ITEM_STATS = static_cast<int>(ItemStat::Count);

    const char* id(Custom stat);
    const char* name(Custom stat);
    Format format(Custom stat);
    std::string formatted(Format format, long long value);
    std::string grouped(long long value);

    class Book
    {
    public:
        Book();
        void add(Custom stat, long long amount = 1);
        void set(Custom stat, long long value);
        long long get(Custom stat) const;
        void item(ItemStat stat, uint16_t id, long long amount = 1);
        long long count(ItemStat stat, uint16_t id) const;
        void killed(int mob, long long amount = 1);
        void killedBy(int mob, long long amount = 1);
        long long kills(int mob) const;
        long long deathsBy(int mob) const;
        const std::map<uint16_t, std::array<long long, ITEM_STATS>>& items() const { return m_items; }
        const std::map<int, std::array<long long, 2>>& mobs() const { return m_mobs; }
        std::vector<std::string> lines() const;
        void load(const std::vector<std::string>& lines);
        void clear();

    private:
        std::array<long long, CUSTOM_COUNT> m_custom{};
        std::map<uint16_t, std::array<long long, ITEM_STATS>> m_items;
        std::map<int, std::array<long long, 2>> m_mobs;
    };
}
