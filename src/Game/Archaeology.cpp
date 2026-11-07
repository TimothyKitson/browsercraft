#include "Archaeology.h"
#include "World/Noise.h"
#include <vector>

namespace
{
    const char* PATTERNS[Archaeology::SHERD_COUNT] = {
        "angler",
        "archer",
        "arms_up",
        "blade",
        "brewer",
        "burn",
        "danger",
        "explorer",
        "flow",
        "friend",
        "guster",
        "heart",
        "heartbreak",
        "howl",
        "miner",
        "mourner",
        "plenty",
        "prize",
        "scrape",
        "sheaf",
        "shelter",
        "skull",
        "snort"
    };

    struct Entry
    {
        StackId id;
        int weight;
    };

    StackId sherdOf(const char* pattern)
    {
        for (int k = 0; k < Archaeology::SHERD_COUNT; ++k)
        {
            const char* a = PATTERNS[k];
            const char* b = pattern;
            while (*a && *a == *b) { ++a; ++b; }
            if (*a == 0 && *b == 0) return Archaeology::sherd(k);
        }
        return Blocks::Air;
    }

    StackId dye(int colour) { return static_cast<StackId>(Items::DyeWhite + colour); }
}

StackId Archaeology::sherd(int index)
{
    if (index < 0 || index >= SHERD_COUNT) return Blocks::Air;
    return static_cast<StackId>(Items::AnglerSherd + index);
}

int Archaeology::sherdIndex(StackId id)
{
    if (id < Items::AnglerSherd || id > Items::SnortSherd) return -1;
    return id - Items::AnglerSherd;
}

const char* Archaeology::sherdPattern(int index)
{
    if (index < 0 || index >= SHERD_COUNT) return "";
    return PATTERNS[index];
}

Archaeology::Find Archaeology::roll(int table, int x, int y, int z, uint32_t seed)
{
    std::vector<Entry> entries;
    switch (table)
    {
        case DesertPyramid:
            entries = { { sherdOf("archer"), 1 }, { sherdOf("miner"), 1 }, { sherdOf("prize"), 1 }, { sherdOf("skull"), 1 },
                        { Items::Diamond, 1 }, { Blocks::Tnt, 1 }, { Items::Gunpowder, 1 }, { Items::Emerald, 1 } };
            break;
        case DesertWell:
            entries = { { sherdOf("arms_up"), 2 }, { sherdOf("brewer"), 2 }, { Items::Brick, 1 }, { Items::Emerald, 1 },
                        { Items::Stick, 1 } };
            break;
        case WarmOceanRuin:
            entries = { { sherdOf("angler"), 1 }, { sherdOf("shelter"), 1 }, { sherdOf("snort"), 1 },
                        { Blocks::SnifferEgg0, 1 }, { Items::IronAxe, 1 }, { Items::Emerald, 2 }, { Items::Wheat, 2 }, { Items::WoodHoe, 2 },
                        { Items::Coal, 2 }, { Items::GoldNugget, 2 } };
            break;
        case ColdOceanRuin:
            entries = { { sherdOf("blade"), 1 }, { sherdOf("explorer"), 1 }, { sherdOf("mourner"), 1 }, { sherdOf("plenty"), 1 },
                        { Items::IronAxe, 1 }, { Items::Emerald, 2 }, { Items::Wheat, 2 }, { Items::WoodHoe, 2 },
                        { Items::Coal, 2 }, { Items::GoldNugget, 2 } };
            break;
        case TrailRare:
            entries = { { sherdOf("burn"), 1 }, { sherdOf("danger"), 1 }, { sherdOf("friend"), 1 }, { sherdOf("heart"), 1 },
                        { sherdOf("heartbreak"), 1 }, { sherdOf("howl"), 1 }, { sherdOf("sheaf"), 1 },
                        { Items::WayfinderTemplate, 1 }, { Items::RaiserTemplate, 1 }, { Items::ShaperTemplate, 1 },
                        { Items::HostTemplate, 1 } };
            break;
        default:
            entries = { { Items::Emerald, 2 }, { Items::Wheat, 2 }, { Items::WoodHoe, 2 }, { Blocks::Clay, 2 }, { Items::Brick, 2 },
                        { dye(4), 2 }, { dye(11), 2 }, { dye(3), 2 }, { dye(0), 2 }, { dye(1), 2 },
                        { candleOf(15, 1, false), 2 }, { candleOf(14, 1, false), 2 }, { candleOf(11, 1, false), 2 },
                        { candleOf(13, 1, false), 2 },
                        { colourIn(Blocks::StainedGlassPaneFirst, 2), 1 }, { colourIn(Blocks::StainedGlassPaneFirst, 6), 1 },
                        { colourIn(Blocks::StainedGlassPaneFirst, 11), 1 }, { colourIn(Blocks::StainedGlassPaneFirst, 3), 1 },
                        { colourIn(Blocks::StainedGlassPaneFirst, 14), 1 }, { colourIn(Blocks::StainedGlassPaneFirst, 4), 1 },
                        { colourIn(Blocks::StainedGlassPaneFirst, 10), 1 }, { Items::GoldNugget, 1 }, { Items::Coal, 1 },
                        { Items::WheatSeeds, 1 }, { Items::BeetrootSeeds, 1 }, { Blocks::DeadBush, 1 },
                        { Blocks::FlowerPot, 1 }, { Items::StringItem, 1 } };
            break;
    }

    int total = 0;
    for (const Entry& e : entries) total += e.weight;
    if (total <= 0) return Find{};
    int pick = static_cast<int>(hashToFloat(hashCoords(x, y, z, seed + 33301u + static_cast<uint32_t>(table))) * total);
    for (const Entry& e : entries)
    {
        if (pick < e.weight) return Find{ e.id, 1 };
        pick -= e.weight;
    }
    return Find{ entries.back().id, 1 };
}

uint32_t Archaeology::packPot(const int sides[4])
{
    uint32_t packed = 0;
    for (int k = 0; k < 4; ++k)
    {
        const int value = sides[k] < 0 || sides[k] >= SHERD_COUNT ? 0 : sides[k] + 1;
        packed |= static_cast<uint32_t>(value) << (k * 5);
    }
    return packed;
}

int Archaeology::potSide(uint32_t packed, int side)
{
    return static_cast<int>((packed >> (side * 5)) & 31u) - 1;
}

BlockId Archaeology::settled(BlockId suspicious)
{
    if (isSuspiciousSand(suspicious)) return Blocks::Sand;
    if (isSuspiciousGravel(suspicious)) return Blocks::Gravel;
    return suspicious;
}
