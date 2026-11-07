#include "Fishing.h"
#include "Brewing.h"
#include "Enchanting.h"
#include "World/Noise.h"
#include <algorithm>

namespace
{
    struct Entry
    {
        StackId id;
        int weight;
        int count;
    };

    const Entry FISH[] = {
        { Items::RawCod, 60, 1 },
        { Items::RawSalmon, 25, 1 },
        { Items::TropicalFish, 2, 1 },
        { Items::Pufferfish, 13, 1 },
    };

    const Entry JUNK[] = {
        { Blocks::LilyPad, 17, 1 },
        { Items::LeatherBoots, 10, 1 },
        { Items::Leather, 10, 1 },
        { Items::Bone, 10, 1 },
        { Items::Potion, 10, 1 },
        { Items::StringItem, 5, 1 },
        { Items::FishingRod, 2, 1 },
        { Items::Bowl, 10, 1 },
        { Items::Stick, 5, 1 },
        { Items::InkSac, 1, 10 },
        { Items::TripwireHook, 10, 1 },
        { Blocks::Bamboo, 10, 1 },
    };

    const Entry TREASURE[] = {
        { Items::NameTag, 1, 1 },
        { Items::Saddle, 1, 1 },
        { Items::Bow, 1, 1 },
        { Items::FishingRod, 1, 1 },
        { Items::Book, 1, 1 },
        { Items::NautilusShell, 1, 1 },
    };

    float unit(uint32_t seed, uint32_t salt) { return hashToFloat(hashCoords(static_cast<int>(seed), static_cast<int>(salt), 77, 0x5157u)); }

    template <size_t N>
    const Entry& pick(const Entry (&table)[N], float roll, bool jungle)
    {
        int total = 0;
        for (const Entry& entry : table)
            if (jungle || entry.id != Blocks::Bamboo) total += entry.weight;
        int target = std::min(total - 1, static_cast<int>(roll * static_cast<float>(total)));
        for (const Entry& entry : table)
        {
            if (!jungle && entry.id == Blocks::Bamboo) continue;
            if (target < entry.weight) return entry;
            target -= entry.weight;
        }
        return table[0];
    }

    Enchanting::Set treasureEnchantment(StackId item, uint32_t seed)
    {
        std::vector<Enchanting::Kind> fits;
        for (int k = 0; k < static_cast<int>(Enchanting::Kind::Count); ++k)
        {
            const Enchanting::Kind kind = static_cast<Enchanting::Kind>(k);
            if (item == Items::Book ? !Enchanting::isTreasure(kind) : Enchanting::appliesTo(kind, item)) fits.push_back(kind);
        }
        if (fits.empty()) return 0;
        const Enchanting::Kind kind = fits[static_cast<size_t>(unit(seed, 31) * static_cast<float>(fits.size())) % fits.size()];
        const int level = 1 + static_cast<int>(unit(seed, 32) * static_cast<float>(Enchanting::maxLevel(kind)));
        return Enchanting::withLevel(0, kind, level);
    }
}

int Fishing::durabilityOf(StackId id)
{
    switch (id)
    {
        case Items::FishingRod: return ROD_DURABILITY;
        case Items::CarrotOnAStick: return CARROT_STICK_DURABILITY;
        case Items::WarpedFungusOnAStick: return FUNGUS_STICK_DURABILITY;
        default: return 0;
    }
}

Fishing::Catch Fishing::roll(uint32_t seed, bool openWater, bool jungle)
{
    Catch out;
    const float which = unit(seed, 1);
    const int junk = 10, treasure = openWater ? 5 : 0, fish = 85;
    const float total = static_cast<float>(junk + treasure + fish);
    const Entry* entry = nullptr;
    if (which * total < static_cast<float>(fish))
    {
        out.category = Category::Fish;
        entry = &pick(FISH, unit(seed, 2), jungle);
    }
    else if (which * total < static_cast<float>(fish + junk))
    {
        out.category = Category::Junk;
        entry = &pick(JUNK, unit(seed, 3), jungle);
    }
    else
    {
        out.category = Category::Treasure;
        entry = &pick(TREASURE, unit(seed, 4), jungle);
    }
    out.stack.id = entry->id;
    out.stack.count = entry->count;
    if (out.category == Category::Junk && (entry->id == Items::LeatherBoots || entry->id == Items::FishingRod))
    {
        const int limit = entry->id == Items::FishingRod ? ROD_DURABILITY : 65;
        out.stack.damage = static_cast<int>(unit(seed, 5) * 0.9f * static_cast<float>(limit));
    }
    if (out.category == Category::Junk && entry->id == Items::Potion) out.stack.damage = Brewing::toDamage(Brewing::Potion::Water);
    if (out.category == Category::Treasure && (entry->id == Items::Bow || entry->id == Items::FishingRod || entry->id == Items::Book))
    {
        out.stack.enchantments = treasureEnchantment(entry->id, seed);
        if (entry->id == Items::Bow) out.stack.damage = static_cast<int>(unit(seed, 6) * 0.25f * 384.0f);
        if (entry->id == Items::FishingRod) out.stack.damage = static_cast<int>(unit(seed, 6) * 0.25f * ROD_DURABILITY);
    }
    return out;
}

float Fishing::waitSeconds(uint32_t seed, bool openSky, bool raining)
{
    float seconds = 5.0f + unit(seed, 11) * 25.0f;
    if (!openSky) seconds *= 2.0f;
    if (raining && openSky) seconds *= 0.8f;
    return seconds;
}

float Fishing::approachSeconds(uint32_t seed) { return 1.0f + unit(seed, 12) * 3.0f; }
float Fishing::biteSeconds(uint32_t seed) { return 1.0f + unit(seed, 13) * 1.0f; }
int Fishing::experienceFor(uint32_t seed) { return 1 + static_cast<int>(unit(seed, 14) * 6.0f); }
