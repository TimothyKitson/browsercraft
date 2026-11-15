#include "Tools.h"
#include "RecipeBook.h"
#include <string>
#include <unordered_map>
#include <algorithm>

namespace
{
    constexpr int KINDS_PER_TIER = 5;

    bool inToolRange(StackId id)
    {
        return (id >= Items::WoodPickaxe && id <= Items::DiamondHoe) ||
               Tools::isNetheriteTool(id);
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
        // Netherite: barely faster than diamond and far harder to
        // break, which is Minecraft's. The reason to want it is that
        // it outlasts everything, not that it digs quicker.
        { 9.0f,  2031,    5, 8 },   // Netherite
    };

    Tools::Kind toolFor(BlockId block)
    {
        static const std::unordered_map<BlockId, Tools::Kind> TABLE = [] {
#include "MineableData.inl"
            std::unordered_map<std::string, Tools::Kind> byName;
            for (const auto& entry : MINEABLE) byName.emplace(entry.first, entry.second);
            std::unordered_map<BlockId, Tools::Kind> out;
            for (int id = 1; isBlockId(id); id = nextBlockId(id))
            {
                const auto found = byName.find(RecipeBook::vanillaIdOf(static_cast<StackId>(id)));
                if (found != byName.end()) out.emplace(static_cast<BlockId>(id), found->second);
            }
            return out;
        }();
        const auto found = TABLE.find(block);
        return found == TABLE.end() ? Tools::Kind::None : found->second;
    }
}

Tools::Kind Tools::kindOf(StackId id)
{
    // The mace is not one of the five that come in tiers, so it is
    // named here rather than worked out from where it sits.
    if (id == Items::Mace) return Kind::Mace;
    if (id == Items::Trident) return Kind::Trident;

    // The nine netherite items are at the end of the list rather than
    // after the diamond ones, so the offset has to be taken from
    // their own start. Five tools, in the same order as the rest.
    if (isNetheriteTool(id))
        return static_cast<Kind>((id - Items::NetheritePickaxe) + 1);

    if (!inToolRange(id)) return Kind::None;
    const int offset = (id - Items::WoodPickaxe) % KINDS_PER_TIER;
    return static_cast<Kind>(offset + 1);
}

Tools::Tier Tools::tierOf(StackId id)
{
    // Iron's, for how long it lasts. It mines nothing whatever its
    // tier: canHarvest asks for the right kind first, and no block in
    // the game is got at with a mace.
    if (id == Items::Mace) return Tier::Iron;

    if (isNetheriteTool(id)) return Tier::Netherite;

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
    static const std::unordered_map<BlockId, int> TABLE = [] {
#include "HarvestData.inl"
        std::unordered_map<std::string, int> byName;
        for (const auto& entry : NEEDS_TOOL) byName.emplace(entry.first, entry.second);
        std::unordered_map<BlockId, int> out;
        for (int id = 1; isBlockId(id); id = nextBlockId(id))
        {
            const auto found = byName.find(RecipeBook::vanillaIdOf(static_cast<StackId>(id)));
            if (found != byName.end()) out.emplace(static_cast<BlockId>(id), found->second);
        }
        return out;
    }();
    const auto found = TABLE.find(block);
    return found == TABLE.end() ? 0 : found->second;
}

bool Tools::canHarvest(StackId tool, BlockId block)
{
    const int needed = requiredLevel(block);
    if (needed == 0) return true;
    if (block == Blocks::Cobweb) return kindOf(tool) == Kind::Sword || tool == Items::Shears;

    // The right kind as well as a good enough tier: an iron shovel is
    // no help at all against stone, however hard it is.
    if (kindOf(tool) != toolFor(block)) return false;
    return harvestLevel(tierOf(tool)) >= needed;
}

float Tools::digSpeed(StackId tool, BlockId block)
{
    if (block == Blocks::Cobweb && (kindOf(tool) == Kind::Sword || tool == Items::Shears)) return 15.0f;
    if (tool == Items::Shears)
    {
        if (isLeaves(block) || block == Blocks::FloweringAzaleaLeaves) return 15.0f;
        if (isWool(block)) return 5.0f;
        if (isVine(block) || isGlowLichen(block)) return 2.0f;
    }
    if (kindOf(tool) != Kind::None && kindOf(tool) == toolFor(block))
        return TIERS[static_cast<int>(tierOf(tool))].speed;
    if (kindOf(tool) == Kind::Sword &&
        (isLeaves(block) || block == Blocks::FloweringAzaleaLeaves || isSapling(block) || isSmallFlower(block) || isCrop(block) ||
         isDoublePlant(block) || isVine(block) || isGlowLichen(block) || isCocoa(block) || isCarvedPumpkin(block) || isJackOLantern(block) ||
         isBigDripleaf(block) || isBigDripleafStem(block) || isPinkPetals(block) || isSweetBerryBush(block) || isMushroom(block) ||
         block == Blocks::Pumpkin || block == Blocks::Melon || block == Blocks::ChorusPlant || block == Blocks::ChorusFlower ||
         block == Blocks::MossCarpet || block == Blocks::LilyPad || block == Blocks::HangingRoots || block == Blocks::SporeBlossom ||
         block == Blocks::TallGrass || block == Blocks::Fern || block == Blocks::DeadBush || block == Blocks::SugarCane))
        return 1.5f;
    return 1.0f;
}

Tools::Kind Tools::preferredTool(BlockId block)
{
    return toolFor(block);
}

float Tools::breakSeconds(StackId tool, BlockId block)
{
    const float hardness = blockInfo(block).hardness;
    if (hardness < 0.0f) return -1.0f;
    if (hardness == 0.0f) return 0.0f;
    if ((block == Blocks::Bamboo || block == Blocks::BambooShoot) && kindOf(tool) == Kind::Sword) return 0.0f;
    return hardness * (canHarvest(tool, block) ? 1.5f : 5.0f) / digSpeed(tool, block);
}

bool Tools::wearsOnBlock(StackId tool, BlockId block)
{
    if (!isTool(tool)) return false;
    if (block == Blocks::Air) return false;
    return blockInfo(block).hardness > 0.0f;
}

int Tools::maceSmashBonus(float fallenBlocks)
{
    if (fallenBlocks <= 0.0f) return 0;

    const float first = std::min(fallenBlocks, 3.0f);
    const float second = std::clamp(fallenBlocks - 3.0f, 0.0f, 5.0f);
    const float rest = std::max(0.0f, fallenBlocks - 8.0f);

    return static_cast<int>(first * 4.0f + second * 2.0f + rest);
}

StackId Tools::upgradeOf(StackId diamond)
{
    // The five tools, in the order they sit in both lists.
    if (diamond >= Items::DiamondPickaxe && diamond <= Items::DiamondHoe)
        return static_cast<StackId>(Items::NetheritePickaxe +
                                    (diamond - Items::DiamondPickaxe));

    // And the four pieces of armour, likewise.
    if (diamond >= Items::DiamondHelmet && diamond <= Items::DiamondBoots)
        return static_cast<StackId>(Items::NetheriteHelmet +
                                    (diamond - Items::DiamondHelmet));

    return Blocks::Air;
}

float Tools::attackSpeed(StackId tool)
{
    const Tier tier = tierOf(tool);
    switch (kindOf(tool))
    {
        case Kind::Sword:   return 1.6f;
        case Kind::Axe:     return tier == Tier::Wood || tier == Tier::Stone ? 0.8f : tier == Tier::Iron ? 0.9f : 1.0f;
        case Kind::Pickaxe: return 1.2f;
        case Kind::Shovel:  return 1.0f;
        case Kind::Hoe:
            switch (tier)
            {
                case Tier::Stone:   return 2.0f;
                case Tier::Iron:    return 3.0f;
                case Tier::Diamond:
                case Tier::Netherite: return 4.0f;
                default:            return 1.0f;
            }
        case Kind::Mace:    return 0.6f;
        case Kind::Trident: return 1.1f;
        default:            return 4.0f;
    }
}

float Tools::attackDamageExact(StackId tool)
{
    const Tier tier = tierOf(tool);
    const float sword = static_cast<float>(TIERS[static_cast<int>(tier)].swordDamage);
    switch (kindOf(tool))
    {
        case Kind::Sword:   return sword;
        case Kind::Axe:
            switch (tier)
            {
                case Tier::Wood: case Tier::Gold: return 7.0f;
                case Tier::Netherite: return 10.0f;
                case Tier::None: return 1.0f;
                default: return 9.0f;
            }
        case Kind::Pickaxe: return std::max(1.0f, sword - 2.0f);
        case Kind::Shovel:  return std::max(1.0f, sword - 1.5f);
        case Kind::Hoe:     return 1.0f;
        case Kind::Mace:    return 6.0f;
        case Kind::Trident: return 9.0f;
        default:            return 1.0f;
    }
}

int Tools::attackDamage(StackId tool)
{
    const Tier tier = tierOf(tool);
    const int sword = TIERS[static_cast<int>(tier)].swordDamage;

    switch (kindOf(tool))
    {
        case Kind::Sword:   return sword;
        case Kind::Axe:     return static_cast<int>(attackDamageExact(tool));
        case Kind::Pickaxe: return std::max(1, sword - 2);
        case Kind::Shovel:  return static_cast<int>(attackDamageExact(tool));
        case Kind::Hoe:     return 1;
        case Kind::Mace:    return 6;
        // Minecraft's, and the hardest swing in the game short of a
        // mace coming down off a fall. Thrown there, held here: the
        // projectiles in this engine cannot drop what threw them.
        case Kind::Trident: return 9;
        default:            return 1;
    }
}
