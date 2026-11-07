#include "Biomes.h"
#include "WorldGen.h"
#include "Dimension.h"

namespace Biomes
{
    Kind fromId(const std::string& id, Kind fallback)
    {
        const std::string full = id.find(':') == std::string::npos ? "minecraft:" + id : id;
        for (int i = 0; i < COUNT; ++i)
            if (full == info(static_cast<Kind>(i)).id) return static_cast<Kind>(i);
        return fallback;
    }

    bool known(const std::string& id)
    {
        return fromId(id, Kind::Count) != Kind::Count;
    }

    Biome family(Kind kind)
    {
        switch (kind)
        {
            case Kind::Badlands: case Kind::ErodedBadlands: case Kind::WoodedBadlands: return Biome::Badlands;
            case Kind::BambooJungle: case Kind::Jungle: case Kind::SparseJungle: return Biome::Jungle;
            case Kind::BasaltDeltas: case Kind::SoulSandValley: case Kind::NetherWastes: return Biome::NetherWastes;
            case Kind::CrimsonForest: return Biome::CrimsonForest;
            case Kind::WarpedForest: return Biome::WarpedForest;
            case Kind::Beach: case Kind::SnowyBeach: case Kind::StonyShore: return Biome::Beach;
            case Kind::BirchForest: case Kind::DarkForest: case Kind::FlowerForest: case Kind::Forest: case Kind::OldGrowthBirchForest:
            case Kind::Taiga: case Kind::OldGrowthPineTaiga: case Kind::OldGrowthSpruceTaiga:
                return Biome::Forest;
            case Kind::CherryGrove: return Biome::CherryGrove;
            case Kind::ColdOcean: case Kind::DeepColdOcean: case Kind::DeepFrozenOcean: case Kind::DeepLukewarmOcean: case Kind::DeepOcean:
            case Kind::FrozenOcean: case Kind::LukewarmOcean: case Kind::Ocean: case Kind::WarmOcean: case Kind::River: case Kind::FrozenRiver:
                return Biome::Ocean;
            case Kind::Desert: return Biome::Desert;
            case Kind::FrozenPeaks: case Kind::JaggedPeaks: case Kind::StonyPeaks: case Kind::SnowySlopes: case Kind::Grove:
            case Kind::WindsweptForest: case Kind::WindsweptGravellyHills: case Kind::WindsweptHills: case Kind::WindsweptSavanna:
                return Biome::Mountains;
            case Kind::IceSpikes: case Kind::SnowyPlains: case Kind::SnowyTaiga: return Biome::Snowy;
            case Kind::MangroveSwamp: return Biome::MangroveSwamp;
            case Kind::Swamp: return Biome::Swamp;
            case Kind::MushroomFields: return Biome::MushroomFields;
            case Kind::Savanna: case Kind::SavannaPlateau: return Biome::Savanna;
            default: return Biome::Plains;
        }
    }

    Kind kindOf(Biome biome)
    {
        switch (biome)
        {
            case Biome::Ocean: return Kind::Ocean;
            case Biome::Beach: return Kind::Beach;
            case Biome::Forest: return Kind::Forest;
            case Biome::Desert: return Kind::Desert;
            case Biome::Mountains: return Kind::WindsweptHills;
            case Biome::Snowy: return Kind::SnowyPlains;
            case Biome::Jungle: return Kind::Jungle;
            case Biome::Savanna: return Kind::Savanna;
            case Biome::Swamp: return Kind::Swamp;
            case Biome::CherryGrove: return Kind::CherryGrove;
            case Biome::MangroveSwamp: return Kind::MangroveSwamp;
            case Biome::Badlands: return Kind::Badlands;
            case Biome::NetherWastes: return Kind::NetherWastes;
            case Biome::CrimsonForest: return Kind::CrimsonForest;
            case Biome::WarpedForest: return Kind::WarpedForest;
            case Biome::MushroomFields: return Kind::MushroomFields;
            default: return Kind::Plains;
        }
    }

    Dimension dimension(Kind kind)
    {
        switch (kind)
        {
            case Kind::BasaltDeltas: case Kind::CrimsonForest: case Kind::NetherWastes: case Kind::SoulSandValley: case Kind::WarpedForest:
                return Dimension::Nether;
            case Kind::EndBarrens: case Kind::EndHighlands: case Kind::EndMidlands: case Kind::SmallEndIslands: case Kind::TheEnd:
                return Dimension::End;
            default:
                return Dimension::Overworld;
        }
    }
}
