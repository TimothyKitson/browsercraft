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
        float countMin;
        float countMax;
        float lootingMin;
        float lootingMax;
        int lootingLimit;
        bool smelt;
        const char* potion;
        bool ominousAmplifier;
        Rule rules[3];
        int ruleCount;
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
        uint32_t seed = 1u;
    };

    bool has(const std::string& table);
    std::vector<ItemStack> roll(const std::string& table, Context context);
}
