#include "Tools.h"
#include <algorithm>

namespace
{
    constexpr int KINDS_PER_TIER = 5;

    bool inToolRange(StackId id)
    {
        return id >= Items::WoodPickaxe && id <= Items::DiamondHoe;
    }

    // Mining speed, durability and harvest level, in tier order.
    struct TierInfo
    {
        float speed;
        int durability;
        int level;
        int swordDamage;
    };

    const TierInfo TIERS[] = {
        { 1.0f,     0,    0, 1 },   // None: bare hands
        { 2.0f,    59,    1, 4 },   // Wood
        { 4.0f,   131,    2, 5 },   // Stone
        { 6.0f,   250,    3, 6 },   // Iron
        { 12.0f,   32,    1, 4 },   // Gold: quick and fragile, mines no more than wood
        { 8.0f,  1561,    4, 7 },   // Diamond
    };

    // Which tool a block gives way to. A block not listed here comes
    // apart as readily however you go at it.
    Tools::Kind toolFor(BlockId block)
    {
        if (isWheat(block)) return Tools::Kind::None;

        switch (block)
        {
            case Blocks::Leaves:
            case Blocks::BirchLeaves:
            case Blocks::TallGrass:
            case Blocks::FlowerRed:
            case Blocks::FlowerYellow:
                return Tools::Kind::Sword;
            default: break;
        }

        switch (blockMaterial(block))
        {
            case Material::Stone: return Tools::Kind::Pickaxe;
            case Material::Wood:  return Tools::Kind::Axe;
            case Material::Dirt:
            case Material::Grass:
            case Material::Sand:
            case Material::Snow:  return Tools::Kind::Shovel;
            default:              return Tools::Kind::None;
        }
    }
}

Tools::Kind Tools::kindOf(StackId id)
{
    if (!inToolRange(id)) return Kind::None;
    const int offset = (id - Items::WoodPickaxe) % KINDS_PER_TIER;
    return static_cast<Kind>(offset + 1);
}

Tools::Tier Tools::tierOf(StackId id)
{
    if (!inToolRange(id)) return Tier::None;
    const int step = (id - Items::WoodPickaxe) / KINDS_PER_TIER;
    return static_cast<Tier>(step + 1);
}

int Tools::maxDurability(Tier tier)
{
    return TIERS[static_cast<int>(tier)].durability;
}

int Tools::harvestLevel(Tier tier)
{
    return TIERS[static_cast<int>(tier)].level;
}

int Tools::requiredLevel(BlockId block)
{
    switch (block)
    {
        case Blocks::Obsidian:   return 4;
        case Blocks::DiamondOre: return 3;
        case Blocks::GoldOre:
        case Blocks::IronOre:    return 2;
        default: break;
    }

    // Everything else made of stone wants a pickaxe of some sort; the
    // softer materials want nothing in particular.
    return blockMaterial(block) == Material::Stone ? 1 : 0;
}

bool Tools::canHarvest(StackId tool, BlockId block)
{
    const int needed = requiredLevel(block);
    if (needed == 0) return true;

    // The right kind as well as a good enough tier: an iron shovel is
    // no help at all against stone, however hard it is.
    if (kindOf(tool) != toolFor(block)) return false;
    return harvestLevel(tierOf(tool)) >= needed;
}

float Tools::breakSeconds(StackId tool, BlockId block)
{
    const float hardness = blockInfo(block).hardness;
    if (hardness < 0.0f) return -1.0f;       // never breaks
    if (hardness == 0.0f) return 0.0f;       // comes apart on contact

    float seconds = hardness;

    // The right tool speeds the work up by its tier; the wrong one is
    // no better than bare hands.
    if (kindOf(tool) != Kind::None && kindOf(tool) == toolFor(block))
        seconds /= TIERS[static_cast<int>(tierOf(tool))].speed;

    // Going at a block that wanted a tool without one is slow, and
    // leaves nothing behind either.
    if (!canHarvest(tool, block)) seconds *= 3.33f;

    return seconds;
}

bool Tools::wearsOnBlock(StackId tool, BlockId block)
{
    if (!isTool(tool)) return false;
    if (block == Blocks::Air) return false;
    return blockInfo(block).hardness > 0.0f;
}

int Tools::attackDamage(StackId tool)
{
    const Tier tier = tierOf(tool);
    const int sword = TIERS[static_cast<int>(tier)].swordDamage;

    switch (kindOf(tool))
    {
        case Kind::Sword:   return sword;
        case Kind::Axe:     return std::max(1, sword - 1);
        case Kind::Pickaxe: return std::max(1, sword - 2);
        case Kind::Shovel:  return std::max(1, sword - 3);
        case Kind::Hoe:     return 1;
        default:            return 1;
    }
}
