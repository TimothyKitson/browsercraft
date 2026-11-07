#include "Smithing.h"
#include "Armour.h"
#include "Tools.h"

namespace
{
    const char* PATTERNS[Smithing::TRIM_PATTERNS] = { "sentry", "vex", "wild", "coast", "dune", "wayfinder", "raiser", "shaper", "host", "ward", "silence", "tide", "snout", "rib", "eye", "spire", "flow", "bolt" };

    const StackId MATERIALS[Smithing::TRIM_MATERIALS] = {
        Items::IronIngot, Items::CopperIngot, Items::GoldIngot, Items::LapisLazuli, Items::Emerald,
        Items::Diamond, Items::NetheriteIngot, Items::Redstone, Items::AmethystShard, Items::NetherQuartz,
    };

    const unsigned int COLOURS[Smithing::TRIM_MATERIALS] = {
        0xECECECu, 0xB4684Du, 0xDEB12Du, 0x416EAAu, 0x11A036u,
        0x6EECD2u, 0x443A3Bu, 0x971607u, 0x9A5CC6u, 0xE3D4C4u,
    };

    int maxWear(StackId id)
    {
        return Tools::kindOf(id) != Tools::Kind::None ? Tools::maxDurability(Tools::tierOf(id)) : Armour::maxDurability(id);
    }
}

int Smithing::patternOfTemplate(StackId id)
{
    if (id < Items::SentryTemplate || id > Items::BoltTemplate) return -1;
    return id - Items::SentryTemplate;
}

int Smithing::materialOf(StackId id)
{
    for (int k = 0; k < TRIM_MATERIALS; ++k)
        if (MATERIALS[k] == id) return k;
    return -1;
}

bool Smithing::trimmable(StackId id)
{
    return isItem(id) && Armour::isArmour(id) && id != Items::Elytra;
}

int Smithing::trimPattern(uint32_t enchantments)
{
    const int value = static_cast<int>(enchantments >> TRIM_SHIFT);
    return value == 0 ? -1 : (value - 1) / TRIM_MATERIALS;
}

int Smithing::trimMaterial(uint32_t enchantments)
{
    const int value = static_cast<int>(enchantments >> TRIM_SHIFT);
    return value == 0 ? -1 : (value - 1) % TRIM_MATERIALS;
}

uint32_t Smithing::withTrim(uint32_t enchantments, int pattern, int material)
{
    const uint32_t kept = enchantments & ((1u << TRIM_SHIFT) - 1u);
    if (pattern < 0 || material < 0) return kept;
    return kept | (static_cast<uint32_t>(1 + pattern * TRIM_MATERIALS + material) << TRIM_SHIFT);
}

const char* Smithing::patternName(int pattern)
{
    return pattern >= 0 && pattern < TRIM_PATTERNS ? PATTERNS[pattern] : "";
}

unsigned int Smithing::materialColour(int material)
{
    return material >= 0 && material < TRIM_MATERIALS ? COLOURS[material] : 0xFFFFFFu;
}

Smithing::Result Smithing::combine(const ItemStack& pattern, const ItemStack& base, const ItemStack& addition)
{
    Result out;
    if (pattern.empty() || base.empty() || addition.empty() || base.count != 1) return out;

    if (pattern.id == Items::NetheriteUpgradeTemplate)
    {
        if (addition.id != Items::NetheriteIngot) return out;
        const StackId upgraded = Tools::upgradeOf(base.id);
        if (upgraded == Blocks::Air) return out;
        out.id = upgraded;
        const int wasMax = maxWear(base.id), nowMax = maxWear(upgraded);
        out.damage = wasMax > 0 ? static_cast<int>(static_cast<long long>(base.damage) * nowMax / wasMax) : 0;
        out.enchantments = base.enchantments;
        return out;
    }

    const int trim = patternOfTemplate(pattern.id);
    const int material = materialOf(addition.id);
    if (trim < 0 || material < 0 || !trimmable(base.id)) return out;
    if (trimPattern(base.enchantments) == trim && trimMaterial(base.enchantments) == material) return out;
    out.id = base.id;
    out.damage = base.damage;
    out.enchantments = withTrim(base.enchantments, trim, material);
    return out;
}

std::vector<StackId> Smithing::everyOutput()
{
    std::vector<StackId> out;
    for (int id = Items::FIRST; id < Items::Count; ++id)
        if (Tools::upgradeOf(static_cast<StackId>(id)) != Blocks::Air) out.push_back(Tools::upgradeOf(static_cast<StackId>(id)));
    return out;
}
