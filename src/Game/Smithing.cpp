#include "Smithing.h"
#include <algorithm>
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

    const char* PALETTES[Smithing::TRIM_MATERIALS] = {
        "iron", "copper", "gold", "lapis", "emerald", "diamond", "netherite", "redstone", "amethyst", "quartz",
    };

    const char* ICONS[4] = { "helmet_trim", "chestplate_trim", "leggings_trim", "boots_trim" };

    std::string modelTrimPath(int p, bool leggings)
    {
        if (leggings) return "assets/textures/trims/models/armor/" + std::string(PATTERNS[p]) + "_leggings.png";
        return "assets/textures/trims/models/armor/" + std::string(PATTERNS[p]) + ".png";
    }

    std::string colourPath(int material, bool darker)
    {
        if (darker) return "assets/textures/trims/color_palettes/" + std::string(PALETTES[material]) + "_darker.png";
        return "assets/textures/trims/color_palettes/" + std::string(PALETTES[material]) + ".png";
    }

    std::string iconPath(int slot)
    {
        return "assets/textures/trims/items/" + std::string(ICONS[slot]) + ".png";
    }

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

std::string Smithing::trimPath(int pattern, bool leggings)
{
    return modelTrimPath(std::clamp(pattern, 0, TRIM_PATTERNS - 1), leggings);
}

std::string Smithing::palettePath(int material, bool darker)
{
    if (material < 0 || material >= TRIM_MATERIALS) return "assets/textures/trims/color_palettes/trim_palette.png";
    return colourPath(material, darker);
}

std::string Smithing::trimIconPath(int slot)
{
    return iconPath(std::clamp(slot, 0, 3));
}

bool Smithing::darkerOn(int material, StackId armour)
{
    if (material < 0 || material >= TRIM_MATERIALS) return false;
    switch (Armour::tierOf(armour))
    {
        case Armour::Tier::Iron: return MATERIALS[material] == Items::IronIngot;
        case Armour::Tier::Gold: return MATERIALS[material] == Items::GoldIngot;
        case Armour::Tier::Diamond: return MATERIALS[material] == Items::Diamond;
        case Armour::Tier::Netherite: return MATERIALS[material] == Items::NetheriteIngot;
        default: return false;
    }
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
