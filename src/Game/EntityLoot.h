#pragma once
#include "Player/Inventory.h"
#include <cstdint>
#include <string>
#include <vector>

namespace EntityLoot
{
    struct Rule
    {
        enum Kind : uint8_t
        {
            None,
            Player,
            Chance,
            LootingChance,
            Fire,
            KillerSkeleton,
            Size,
            Captain,
            Lightning,
            Frog,
            FrogWarm,
            FrogCold,
            FrogTemperate,
            NotFrog,
            OpenWater,
            Jungle,
            Never
        };
        Kind kind;
        float a;
        float b;
        float c;
    };

    struct Entry
    {
        const char* item;
        const char* table;
        int weight;
        int quality;
        float countMin;
        float countMax;
        float lootingMin;
        float lootingMax;
        int lootingLimit;
        bool smelt;
        const char* potion;
        float ampMin;
        float ampMax;
        float damageMin;
        float damageMax;
        float levelsMin;
        float levelsMax;
        const char* enchantOptions;
        const char* fixedEnchant;
        const char* stew;
        bool map;
        bool instrument;
        Rule rules[3];
        int ruleCount;
    };

    struct Enchant
    {
        const char* id;
        int weight;
        int minBase;
        int minPer;
        int maxBase;
        int maxPer;
        int maxLevel;
        bool nonTreasure;
        bool randomLoot;
    };

    struct Pool
    {
        float rollsMin;
        float rollsMax;
        Rule rules[3];
        int ruleCount;
        int first;
        int count;
    };

    struct Table
    {
        const char* name;
        int firstPool;
        int poolCount;
    };

    struct Context
    {
        bool byPlayer = false;
        int looting = 0;
        bool burning = false;
        bool fireAspect = false;
        bool killerSkeleton = false;
        int frog = -1;
        int size = 1;
        bool captain = false;
        bool lightning = false;
        float luck = 0.0f;
        bool openWater = true;
        bool jungle = false;
        uint32_t seed = 1u;
    };

    bool has(const std::string& table);
    int tableCount();
    std::vector<std::string> unknownItems();
    const char* tableName(int index);
    std::vector<ItemStack> roll(const std::string& table, Context context);
    std::vector<ItemStack> fillChest(const std::string& table, uint32_t seed, int slots);
    ItemStack enchantWithLevels(ItemStack stack, int levels, uint32_t seed, bool treasure);

    struct Pick
    {
        bool trident = false;
        int kind = -1;
        int level = 0;
    };
    std::vector<Pick> tablePicks(StackId item, int level, uint32_t seed);

    struct EnchantInfo
    {
        std::string id;
        int maxLevel = 1;
        bool nonTreasure = false;
        bool randomLoot = false;
    };
    std::vector<EnchantInfo> enchantList();
    ItemStack withEnchant(ItemStack stack, const std::string& id, int level);
    ItemStack applyPicks(ItemStack stack, const std::vector<Pick>& picks);
}
