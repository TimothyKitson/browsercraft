#pragma once
#include <cstdint>
#include <string>

enum class Biome : uint8_t;
enum class Dimension : uint8_t;

namespace Biomes
{
    enum class Kind : uint8_t
    {
        Badlands,
        BambooJungle,
        BasaltDeltas,
        Beach,
        BirchForest,
        CherryGrove,
        ColdOcean,
        CrimsonForest,
        DarkForest,
        DeepColdOcean,
        DeepDark,
        DeepFrozenOcean,
        DeepLukewarmOcean,
        DeepOcean,
        Desert,
        DripstoneCaves,
        EndBarrens,
        EndHighlands,
        EndMidlands,
        ErodedBadlands,
        FlowerForest,
        Forest,
        FrozenOcean,
        FrozenPeaks,
        FrozenRiver,
        Grove,
        IceSpikes,
        JaggedPeaks,
        Jungle,
        LukewarmOcean,
        LushCaves,
        MangroveSwamp,
        Meadow,
        MushroomFields,
        NetherWastes,
        Ocean,
        OldGrowthBirchForest,
        OldGrowthPineTaiga,
        OldGrowthSpruceTaiga,
        Plains,
        River,
        Savanna,
        SavannaPlateau,
        SmallEndIslands,
        SnowyBeach,
        SnowyPlains,
        SnowySlopes,
        SnowyTaiga,
        SoulSandValley,
        SparseJungle,
        StonyPeaks,
        StonyShore,
        SunflowerPlains,
        Swamp,
        Taiga,
        TheEnd,
        TheVoid,
        WarmOcean,
        WarpedForest,
        WindsweptForest,
        WindsweptGravellyHills,
        WindsweptHills,
        WindsweptSavanna,
        WoodedBadlands,
        Count
    };

    constexpr int COUNT = static_cast<int>(Kind::Count);

    struct Info
    {
        const char* id;
        const char* title;
        float temperature;
        float downfall;
        bool precipitation;
        bool frozen;
        uint32_t sky;
        uint32_t fog;
        uint32_t water;
        uint32_t waterFog;
        int32_t grass;
        int32_t foliage;
        uint8_t grassModifier;
        const char* particle;
        float particleChance;
    };

    enum class Group : uint8_t { Monster, Creature, Ambient, WaterCreature, UndergroundWater, WaterAmbient, Axolotls, Count };

    struct Spawn
    {
        Kind kind;
        uint8_t group;
        const char* mob;
        int weight;
        int minCount;
        int maxCount;
    };

    const Spawn* spawns(int& count);

    const Info& info(Kind kind);
    Kind fromId(const std::string& id, Kind fallback = Kind::Plains);
    bool known(const std::string& id);
    Biome family(Kind kind);
    Kind kindOf(Biome biome);
    Dimension dimension(Kind kind);
}
