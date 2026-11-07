#include "Items.h"
#include <algorithm>

// Blocks and items share one number line: everything below
// Items::FIRST is a block and everything above it is an item, which is
// what lets one stack hold either.
//
// This used to assert that FIRST was past the largest value a BlockId
// could hold. That worked while a BlockId was a byte and FIRST was
// 256, and it is what caught the widening -- but with sixteen bits the
// type's maximum is 65535 and a bound there would mean no items at
// all. What actually has to hold is that FIRST is past the last block
// in use, so that is what is checked. Saved worlds from before the
// widening are rejected by the format version rather than migrated.
static_assert(static_cast<unsigned long long>(Items::FIRST) >
                  static_cast<unsigned long long>(Blocks::Count),
              "the blocks have reached Items::FIRST; raise it");

namespace
{
    const ItemInfo ITEMS[] = {
        { "Wheat Seeds",  Tiles::ItemWheatSeeds, 64 },
        { "Wheat",        Tiles::ItemWheat,      64 },
        { "Leather",      Tiles::ItemLeather,    64 },
        { "Raw Beef",     Tiles::ItemRawBeef,    64 },
        { "Raw Porkchop", Tiles::ItemRawPork,    64 },
        { "Raw Chicken",  Tiles::ItemRawChicken, 64 },
        { "Raw Mutton",   Tiles::ItemRawMutton,  64 },
        { "Feather",      Tiles::ItemFeather,    64 },
        { "Bone",         Tiles::ItemBone,       64 },
        { "String",       Tiles::ItemString,     64 },
        { "Gunpowder",    Tiles::ItemGunpowder,  64 },
        { "Bread",        Tiles::ItemBread,      64 },

        { "Steak",          Tiles::ItemSteak,         64 },
        { "Cooked Porkchop", Tiles::ItemCookedPork,   64 },
        { "Cooked Chicken", Tiles::ItemCookedChicken, 64 },
        { "Cooked Mutton",  Tiles::ItemCookedMutton,  64 },

        { "Stick",        Tiles::ItemStick,      64 },
        { "Coal",         Tiles::ItemCoal,       64 },
        { "Flint",        Tiles::ItemFlint,      64 },
        { "Bow",          Tiles::ItemBow,         1 },
        { "Arrow",        Tiles::ItemArrow,      64 },
        { "Flint and Steel", Tiles::ItemFlintAndSteel, 1 },
        { "Bucket",       Tiles::ItemBucket,     16 },
        { "Water Bucket", Tiles::ItemWaterBucket, 1 },
        { "Lava Bucket",  Tiles::ItemLavaBucket,  1 },
        { "Diamond",      Tiles::ItemDiamond,    64 },
        { "Iron Ingot",   Tiles::ItemIronIngot,  64 },
        { "Gold Ingot",   Tiles::ItemGoldIngot,  64 },


        // Armour is one of a kind too, so it never stacks.
        { "Leather Helmet",      Tiles::ItemArmourFirst + 0,  1 },
        { "Leather Chestplate",  Tiles::ItemArmourFirst + 1,  1 },
        { "Leather Leggings",    Tiles::ItemArmourFirst + 2,  1 },
        { "Leather Boots",       Tiles::ItemArmourFirst + 3,  1 },
        { "Golden Helmet",       Tiles::ItemArmourFirst + 4,  1 },
        { "Golden Chestplate",   Tiles::ItemArmourFirst + 5,  1 },
        { "Golden Leggings",     Tiles::ItemArmourFirst + 6,  1 },
        { "Golden Boots",        Tiles::ItemArmourFirst + 7,  1 },
        { "Iron Helmet",         Tiles::ItemArmourFirst + 8,  1 },
        { "Iron Chestplate",     Tiles::ItemArmourFirst + 9,  1 },
        { "Iron Leggings",       Tiles::ItemArmourFirst + 10,  1 },
        { "Iron Boots",          Tiles::ItemArmourFirst + 11,  1 },
        { "Diamond Helmet",      Tiles::ItemArmourFirst + 12,  1 },
        { "Diamond Chestplate",  Tiles::ItemArmourFirst + 13,  1 },
        { "Diamond Leggings",    Tiles::ItemArmourFirst + 14,  1 },
        { "Diamond Boots",       Tiles::ItemArmourFirst + 15,  1 },

        // A tool is one of a kind, so it never stacks.
        { "Wooden Pickaxe",  Tiles::ItemToolFirst + 0,  1 },
        { "Wooden Axe",      Tiles::ItemToolFirst + 1,  1 },
        { "Wooden Shovel",   Tiles::ItemToolFirst + 2,  1 },
        { "Wooden Sword",    Tiles::ItemToolFirst + 3,  1 },
        { "Wooden Hoe",      Tiles::ItemToolFirst + 4,  1 },
        { "Stone Pickaxe",   Tiles::ItemToolFirst + 5,  1 },
        { "Stone Axe",       Tiles::ItemToolFirst + 6,  1 },
        { "Stone Shovel",    Tiles::ItemToolFirst + 7,  1 },
        { "Stone Sword",     Tiles::ItemToolFirst + 8,  1 },
        { "Stone Hoe",       Tiles::ItemToolFirst + 9,  1 },
        { "Iron Pickaxe",    Tiles::ItemToolFirst + 10,  1 },
        { "Iron Axe",        Tiles::ItemToolFirst + 11,  1 },
        { "Iron Shovel",     Tiles::ItemToolFirst + 12,  1 },
        { "Iron Sword",      Tiles::ItemToolFirst + 13,  1 },
        { "Iron Hoe",        Tiles::ItemToolFirst + 14,  1 },
        { "Golden Pickaxe",  Tiles::ItemToolFirst + 15,  1 },
        { "Golden Axe",      Tiles::ItemToolFirst + 16,  1 },
        { "Golden Shovel",   Tiles::ItemToolFirst + 17,  1 },
        { "Golden Sword",    Tiles::ItemToolFirst + 18,  1 },
        { "Golden Hoe",      Tiles::ItemToolFirst + 19,  1 },
        { "Diamond Pickaxe",  Tiles::ItemToolFirst + 20,  1 },
        { "Diamond Axe",     Tiles::ItemToolFirst + 21,  1 },
        { "Diamond Shovel",  Tiles::ItemToolFirst + 22,  1 },
        { "Diamond Sword",   Tiles::ItemToolFirst + 23,  1 },
        { "Diamond Hoe",     Tiles::ItemToolFirst + 24,  1 },
        { "Paper",           Tiles::ItemPaper,          64 },
        { "Book",            Tiles::ItemBook,           64 },
        { "Emerald",         Tiles::ItemEmerald,        64 },
        { "Ender Pearl",     Tiles::ItemEnderPearl,     16 },
        { "Eye of Ender",    Tiles::ItemEyeOfEnder,     64 },
        { "Glass Bottle",    Tiles::ItemGlassBottle,    64 },
        // A potion stacks to one: two of them are rarely the same
        // drink, and the stack has only one damage field to say which.
        { "Potion",          Tiles::ItemPotion,          1 },
        { "Nether Wart",     Tiles::ItemNetherWart,     64 },
        { "Sugar",           Tiles::ItemSugar,          64 },
        { "Blaze Powder",    Tiles::ItemBlazePowder,    64 },
        { "Glowstone Dust",  Tiles::ItemGlowstoneDust,  64 },
        { "Shield",          Tiles::ItemShield,          1 },
        { "Boat",            Tiles::ItemBoat,            1 },
        { "Minecart",        Tiles::ItemMinecart,        1 },
        { "Blaze Rod",       Tiles::ItemBlazeRod,       64 },
        { "Nether Brick",    Tiles::ItemNetherBrick,    64 },
        { "Magma Cream",     Tiles::ItemMagmaCream,     64 },
        { "Redstone",        Tiles::ItemRedstone,       64 },
        { "Breeze Rod",      Tiles::ItemBreezeRod,      64 },
        { "Wind Charge",     Tiles::ItemWindCharge,     64 },
        { "Mace",            Tiles::ItemMace,            1 },
        { "Trial Key",       Tiles::ItemTrialKey,       64 },
        { "Raw Copper",      Tiles::ItemRawCopper,      64 },
        { "Copper Ingot",    Tiles::ItemCopperIngot,    64 },
        { "Glow Berries",    Tiles::ItemGlowBerries,    64 },
        { "Ominous Bottle",  Tiles::ItemOminousBottle,   1 },
        { "Ominous Trial Key", Tiles::ItemOminousKey,    64 },
        { "Apple",           Tiles::ItemApple,         64 },
        { "Bone Meal",       Tiles::ItemBoneMeal,      64 },
        { "Carrot",          Tiles::ItemCarrot,        64 },
        { "Potato",          Tiles::ItemPotato,        64 },
        { "Baked Potato",    Tiles::ItemBakedPotato,   64 },
        { "Bowl",            Tiles::ItemBowl,          64 },
        { "Beetroot",        Tiles::ItemBeetroot,      64 },
        { "Beetroot Seeds",  Tiles::ItemBeetrootSeeds, 64 },
        { "Beetroot Soup",   Tiles::ItemBeetrootSoup,   1 },
        { "Melon Slice",     Tiles::ItemMelonSlice,    64 },
        { "Melon Seeds",     Tiles::ItemMelonSeeds,    64 },
        { "Egg",             Tiles::ItemEgg,           16 },
        { "Milk Bucket",     Tiles::ItemMilkBucket,     1 },
        { "Cake",            Tiles::ItemCake,           1 },
        { "Cookie",          Tiles::ItemCookie,        64 },
        { "Cocoa Beans",     Tiles::ItemCocoaBeans,    64 },
        { "Sign",            Tiles::ItemSign,          16 },
        { "Item Frame",      Tiles::ItemItemFrame,     64 },
        { "Elytra",          Tiles::ItemElytra,         1 },
        { "Ghast Tear",      Tiles::ItemGhastTear,     64 },
        { "Shulker Shell",   Tiles::ItemShulkerShell,  64 },
        { "Trident",         Tiles::ItemTrident,        1 },
        { "White Dye",       Tiles::ItemDyeFirst +  0, 64 },
        { "Orange Dye",      Tiles::ItemDyeFirst +  1, 64 },
        { "Magenta Dye",     Tiles::ItemDyeFirst +  2, 64 },
        { "Light Blue Dye",  Tiles::ItemDyeFirst +  3, 64 },
        { "Yellow Dye",      Tiles::ItemDyeFirst +  4, 64 },
        { "Lime Dye",        Tiles::ItemDyeFirst +  5, 64 },
        { "Pink Dye",        Tiles::ItemDyeFirst +  6, 64 },
        { "Gray Dye",        Tiles::ItemDyeFirst +  7, 64 },
        { "Light Gray Dye",  Tiles::ItemDyeFirst +  8, 64 },
        { "Cyan Dye",        Tiles::ItemDyeFirst +  9, 64 },
        { "Purple Dye",      Tiles::ItemDyeFirst + 10, 64 },
        { "Blue Dye",        Tiles::ItemDyeFirst + 11, 64 },
        { "Brown Dye",       Tiles::ItemDyeFirst + 12, 64 },
        { "Green Dye",       Tiles::ItemDyeFirst + 13, 64 },
        { "Red Dye",         Tiles::ItemDyeFirst + 14, 64 },
        { "Black Dye",       Tiles::ItemDyeFirst + 15, 64 },
        { "Lapis Lazuli",    Tiles::ItemLapis,         64 },
        { "Ink Sac",         Tiles::ItemInkSac,        64 },
        { "Netherite Scrap", Tiles::ItemNetheriteScrap, 64 },
        { "Netherite Ingot", Tiles::ItemNetheriteIngot, 64 },
        { "Nether Star",     Tiles::ItemNetherStar,     1 },
        { "Netherite Pickaxe",  Tiles::ItemNetheriteFirst + 0, 1 },
        { "Netherite Axe",      Tiles::ItemNetheriteFirst + 1, 1 },
        { "Netherite Shovel",   Tiles::ItemNetheriteFirst + 2, 1 },
        { "Netherite Sword",    Tiles::ItemNetheriteFirst + 3, 1 },
        { "Netherite Hoe",      Tiles::ItemNetheriteFirst + 4, 1 },
        { "Netherite Helmet",     Tiles::ItemNetheriteFirst + 5, 1 },
        { "Netherite Chestplate", Tiles::ItemNetheriteFirst + 6, 1 },
        { "Netherite Leggings",   Tiles::ItemNetheriteFirst + 7, 1 },
        { "Netherite Boots",      Tiles::ItemNetheriteFirst + 8, 1 },
        { "Totem of Undying",     Tiles::ItemTotem,              1 },
        { "Slimeball",            Tiles::ItemSlimeball,         64 },
        { "Gold Nugget",          Tiles::ItemGoldNugget,        64 },
        { "Raw Cod",              Tiles::ItemRawCod,            64 },
        { "Cooked Cod",           Tiles::ItemCookedCod,         64 },
        { "Raw Salmon",           Tiles::ItemRawSalmon,         64 },
        { "Cooked Salmon",        Tiles::ItemCookedSalmon,      64 },
        { "Pufferfish",           Tiles::ItemPufferfish,        64 },
        { "Raw Rabbit",           Tiles::ItemRawRabbit,         64 },
        { "Cooked Rabbit",        Tiles::ItemCookedRabbit,      64 },
        { "Honeycomb",            Tiles::ItemHoneycomb,         64 },
        { "Snowball",             Tiles::ItemSnowball,          16 },
        { "Saddle",               Tiles::ItemSaddle,            1 },
        { "Leather Horse Armor",  Tiles::ItemHorseArmorLeather, 1 },
        { "Iron Horse Armor",     Tiles::ItemHorseArmorIron,    1 },
        { "Golden Horse Armor",   Tiles::ItemHorseArmorGold,    1 },
        { "Diamond Horse Armor",  Tiles::ItemHorseArmorDiamond, 1 },
        { "Spruce Sign",          Tiles::ItemSpruceSign,        16 },
        { "Birch Sign",           Tiles::ItemBirchSign,         16 },
        { "Jungle Sign",          Tiles::ItemJungleSign,        16 },
        { "Acacia Sign",          Tiles::ItemAcaciaSign,        16 },
        { "Dark Oak Sign",        Tiles::ItemDarkOakSign,       16 },
        { "Cherry Sign",          Tiles::ItemCherrySign,        16 },
        { "Mangrove Sign",        Tiles::ItemMangroveSign,      16 },
        { "Bamboo Sign",          Tiles::ItemBambooSign,        16 },
        { "Crimson Sign",         Tiles::ItemCrimsonSign,       16 },
        { "Warped Sign",          Tiles::ItemWarpedSign,        16 },
        { "Nether Quartz",        Tiles::ItemQuartz,            64 },
        { "Raw Iron",             Tiles::ItemRawIron,           64 },
        { "Raw Gold",             Tiles::ItemRawGold,           64 },
        { "Amethyst Shard",       Tiles::ItemAmethystShard,     64 },
        { "Tripwire Hook",        Tiles::ItemTripwireHook,      64 },
        { "Torchflower Seeds",    Tiles::ItemTorchflowerSeeds,  64 },
        { "Pitcher Pod",          Tiles::ItemPitcherPod,        64 },
        { "Clay Ball",            Tiles::ItemClayBall,          64 },
        { "Brick",                Tiles::ItemBrick,             64 },
        { "Pumpkin Seeds",        Tiles::ItemPumpkinSeeds,      64 },
        { "Shears",               Tiles::ItemShears,            1 },
        { "Honey Bottle",         Tiles::ItemHoneyBottle,       16 },
        { "Sweet Berries",        Tiles::ItemSweetBerries,      64 },
        { "Dried Kelp",           Tiles::ItemDriedKelp,         64 },
        { "Nautilus Shell",       Tiles::ItemNautilusShell,     64 },
        { "Heart of the Sea",     Tiles::ItemHeartOfTheSea,     64 },
        { "Turtle Scute",         Tiles::ItemTurtleScute,       64 },
        { "Turtle Shell",         Tiles::ItemTurtleHelmet,      1 },
        { "Powder Snow Bucket",   Tiles::ItemPowderSnowBucket,  1 },
        { "Iron Nugget",          Tiles::ItemIronNugget,        64 },
        { "Chorus Fruit",         Tiles::ItemChorusFruit,       64 },
        { "Popped Chorus Fruit",  Tiles::ItemPoppedChorusFruit, 64 },
        { "Brush", Tiles::ItemBrush, 1 },
        { "Netherite Upgrade", Tiles::ItemTemplateFirst, 64 },
        { "Sentry Armor Trim", Tiles::ItemTemplateFirst + 1, 64 },
        { "Vex Armor Trim", Tiles::ItemTemplateFirst + 2, 64 },
        { "Wild Armor Trim", Tiles::ItemTemplateFirst + 3, 64 },
        { "Coast Armor Trim", Tiles::ItemTemplateFirst + 4, 64 },
        { "Dune Armor Trim", Tiles::ItemTemplateFirst + 5, 64 },
        { "Wayfinder Armor Trim", Tiles::ItemTemplateFirst + 6, 64 },
        { "Raiser Armor Trim", Tiles::ItemTemplateFirst + 7, 64 },
        { "Shaper Armor Trim", Tiles::ItemTemplateFirst + 8, 64 },
        { "Host Armor Trim", Tiles::ItemTemplateFirst + 9, 64 },
        { "Ward Armor Trim", Tiles::ItemTemplateFirst + 10, 64 },
        { "Silence Armor Trim", Tiles::ItemTemplateFirst + 11, 64 },
        { "Tide Armor Trim", Tiles::ItemTemplateFirst + 12, 64 },
        { "Snout Armor Trim", Tiles::ItemTemplateFirst + 13, 64 },
        { "Rib Armor Trim", Tiles::ItemTemplateFirst + 14, 64 },
        { "Eye Armor Trim", Tiles::ItemTemplateFirst + 15, 64 },
        { "Spire Armor Trim", Tiles::ItemTemplateFirst + 16, 64 },
        { "Flow Armor Trim", Tiles::ItemTemplateFirst + 17, 64 },
        { "Bolt Armor Trim", Tiles::ItemTemplateFirst + 18, 64 },
        { "Angler Pottery Sherd", Tiles::ItemSherdFirst + 0, 64 },
        { "Archer Pottery Sherd", Tiles::ItemSherdFirst + 1, 64 },
        { "Arms Up Pottery Sherd", Tiles::ItemSherdFirst + 2, 64 },
        { "Blade Pottery Sherd", Tiles::ItemSherdFirst + 3, 64 },
        { "Brewer Pottery Sherd", Tiles::ItemSherdFirst + 4, 64 },
        { "Burn Pottery Sherd", Tiles::ItemSherdFirst + 5, 64 },
        { "Danger Pottery Sherd", Tiles::ItemSherdFirst + 6, 64 },
        { "Explorer Pottery Sherd", Tiles::ItemSherdFirst + 7, 64 },
        { "Flow Pottery Sherd", Tiles::ItemSherdFirst + 8, 64 },
        { "Friend Pottery Sherd", Tiles::ItemSherdFirst + 9, 64 },
        { "Guster Pottery Sherd", Tiles::ItemSherdFirst + 10, 64 },
        { "Heart Pottery Sherd", Tiles::ItemSherdFirst + 11, 64 },
        { "Heartbreak Pottery Sherd", Tiles::ItemSherdFirst + 12, 64 },
        { "Howl Pottery Sherd", Tiles::ItemSherdFirst + 13, 64 },
        { "Miner Pottery Sherd", Tiles::ItemSherdFirst + 14, 64 },
        { "Mourner Pottery Sherd", Tiles::ItemSherdFirst + 15, 64 },
        { "Plenty Pottery Sherd", Tiles::ItemSherdFirst + 16, 64 },
        { "Prize Pottery Sherd", Tiles::ItemSherdFirst + 17, 64 },
        { "Scrape Pottery Sherd", Tiles::ItemSherdFirst + 18, 64 },
        { "Sheaf Pottery Sherd", Tiles::ItemSherdFirst + 19, 64 },
        { "Shelter Pottery Sherd", Tiles::ItemSherdFirst + 20, 64 },
        { "Skull Pottery Sherd", Tiles::ItemSherdFirst + 21, 64 },
        { "Snort Pottery Sherd", Tiles::ItemSherdFirst + 22, 64 },
        { "Compass", Tiles::ItemCompass, 64 },
        { "Empty Map", Tiles::ItemEmptyMap, 64 },
        { "Map", Tiles::ItemFilledMap, 1 },
        { "Flower Charge Banner Pattern", Tiles::ItemBannerPatternFirst + 0, 1 },
        { "Skull Charge Banner Pattern", Tiles::ItemBannerPatternFirst + 1, 1 },
        { "Globe Banner Pattern", Tiles::ItemBannerPatternFirst + 2, 1 },
        { "Snout Banner Pattern", Tiles::ItemBannerPatternFirst + 3, 1 },
        { "Flow Banner Pattern", Tiles::ItemBannerPatternFirst + 4, 1 },
        { "Guster Banner Pattern", Tiles::ItemBannerPatternFirst + 5, 1 },
        { "Fishing Rod", Tiles::ItemFishingRod, 1 },
        { "Carrot on a Stick", Tiles::ItemCarrotOnAStick, 1 },
        { "Warped Fungus on a Stick", Tiles::ItemWarpedFungusOnAStick, 1 },
        { "Tropical Fish", Tiles::ItemTropicalFish, 64 },
        { "Name Tag", Tiles::ItemNameTag, 64 },
        { "Lead", Tiles::ItemLead, 64 },
        { "Spyglass", Tiles::ItemSpyglass, 1 },
        { "Clock", Tiles::ItemClockFirst, 64 },
        { "Recovery Compass", Tiles::ItemRecoveryFirst + 16, 64 },
        { "Echo Shard", Tiles::ItemEchoShard, 64 },
        { "Goat Horn", Tiles::ItemGoatHorn, 1 },
        { "Bundle", Tiles::ItemBundle, 1 },
        { "Crossbow", Tiles::ItemCrossbow, 1 },
        { "Spectral Arrow", Tiles::ItemSpectralArrow, 64 },
        { "Tipped Arrow", Tiles::ItemTippedArrow, 64 },
        { "Chainmail Helmet", Tiles::ItemChainmailFirst + 0, 1 },
        { "Chainmail Chestplate", Tiles::ItemChainmailFirst + 1, 1 },
        { "Chainmail Leggings", Tiles::ItemChainmailFirst + 2, 1 },
        { "Chainmail Boots", Tiles::ItemChainmailFirst + 3, 1 },
        { "Golden Apple", Tiles::ItemFoodFirst + 0, 64 },
        { "Enchanted Golden Apple", Tiles::ItemFoodFirst + 1, 64 },
        { "Golden Carrot", Tiles::ItemFoodFirst + 2, 64 },
        { "Glistering Melon Slice", Tiles::ItemFoodFirst + 3, 64 },
        { "Mushroom Stew", Tiles::ItemFoodFirst + 4, 1 },
        { "Rabbit Stew", Tiles::ItemFoodFirst + 5, 1 },
        { "Suspicious Stew", Tiles::ItemFoodFirst + 6, 1 },
        { "Pumpkin Pie", Tiles::ItemFoodFirst + 7, 64 },
        { "Rotten Flesh", Tiles::ItemFoodFirst + 8, 64 },
        { "Spider Eye", Tiles::ItemFoodFirst + 9, 64 },
        { "Fermented Spider Eye", Tiles::ItemFoodFirst + 10, 64 },
        { "Poisonous Potato", Tiles::ItemFoodFirst + 11, 64 },
    };

    static_assert(sizeof(ITEMS) / sizeof(ITEMS[0]) ==
                      static_cast<size_t>(Items::Count - Items::FIRST),
                  "every item needs a row in ITEMS");

    const ItemInfo UNKNOWN{};
}

const ItemInfo& itemInfo(StackId id)
{
    if (!isItem(id)) return UNKNOWN;
    return ITEMS[id - Items::FIRST];
}

const char* displayName(StackId id)
{
    return isItem(id) ? itemInfo(id).name : blockInfo(asBlock(id)).name;
}

int flatIconFor(BlockId id)
{
    if (isDoor(id) && copperForm(id).family == CopperFamily::Door)
        return Tiles::ItemCopperDoorFirst + copperForm(id).stage;
    if (isLightningRod(id)) return Tiles::LightningRodTile;
    if (isRepeater(id)) return Tiles::ItemRepeater;
    if (isComparator(id)) return Tiles::ItemComparator;
    if (isIronDoor(id)) return Tiles::ItemIronDoor;
    if (isTurtleEgg(id)) return Tiles::ItemTurtleEggIcon;
    if (isTripwireHook(id)) return Tiles::ItemTripwireHook;
    if (isChain(id)) return Tiles::ItemChain;
    if (isBell(id)) return Tiles::ItemBell;
    if (isChiseledBookshelf(id)) return Tiles::ShelfOccupiedTile;
    if (id == Blocks::DragonEgg) return Tiles::DragonEggTile;
    if (id == Blocks::HeavyCore) return Tiles::HeavyCoreTile;
    if (isDecoratedPot(id)) return Tiles::PotSideTile;
    if (isBanner(id)) return Tiles::ItemBannerFirst + bannerColour(id);
    if (isSculkVein(id)) return Tiles::SculkVeinTile;
    if (id == Blocks::Barrier) return Tiles::ItemBarrier;
    if (id == Blocks::StructureVoid) return Tiles::ItemStructureVoid;
    if (isLightBlock(id)) return Tiles::ItemLightFirst + lightBlockLevel(id);
    if (isSnifferEgg(id)) return Tiles::ItemSnifferEgg;
    if (isEndRod(id)) return Tiles::EndRodTile;
    if (isDoublePlant(id))
    {
        switch (doublePlantKind(id))
        {
            case DoublePlant::Sunflower: return Tiles::SunflowerFrontTile;
            case DoublePlant::PitcherPlant: return Tiles::ItemPitcherPlant;
            default: return blockInfo(doublePlantOf(doublePlantKind(id), true)).tileSide;
        }
    }
    if (isFlowerPot(id)) return Tiles::ItemFlowerPot;
    if (isBigDripleaf(id)) return Tiles::BigDripleafTopTile;
    if (isSmallDripleaf(id)) return Tiles::SmallDripleafTopTile;
    if (isGlowLichen(id)) return Tiles::GlowLichenTile;
    if (id == Blocks::SporeBlossom) return Tiles::SporeBlossomTile;
    if (isAzaleaBush(id)) return blockInfo(id).tileSide;
    if (isDoor(id))
    {
        static const int ICONS[WOOD_COUNT] = { Tiles::IconOakDoor, Tiles::IconSpruceDoor, Tiles::IconBirchDoor,
                                               Tiles::IconJungleDoor, Tiles::IconAcaciaDoor, Tiles::IconDarkOakDoor,
                                               Tiles::IconCherryDoor, Tiles::IconMangroveDoor,
                                               Tiles::IconBambooDoor, Tiles::IconCrimsonDoor,
                                               Tiles::IconWarpedDoor };
        const int wood = doorWood(id);
        return ICONS[wood < 0 ? 0 : wood];
    }
    if (isTrapdoor(id)) return blockInfo(id).tileTop;
    if (isPane(id)) return blockInfo(id).tileSide;
    if (isCandle(id)) return Tiles::ItemCandleFirst + candleShade(id);
    if (isBed(id)) return Tiles::ItemBedFirst + bedColour(id);
    if (id == Blocks::MangrovePropagule || id == Blocks::MangrovePropaguleHanging) return Tiles::IconMangrovePropagule;
    if (id == Blocks::Campfire) return Tiles::IconCampfire;
    if (id == Blocks::SoulCampfire) return Tiles::SoulCampfireTop;
    if (isCauldron(id)) return Tiles::IconCauldron;
    if (id == Blocks::Hopper) return Tiles::IconHopper;
    if (isPointedDripstone(id)) return Tiles::IconDripstone;
    if (id == Blocks::Bamboo) return Tiles::IconBamboo;
    return -1;
}

int atlasTileFor(StackId id)
{
    // An item has one sprite and that is all it has.
    if (isItem(id)) return itemInfo(id).tile;

    const BlockId block = asBlock(id);
    if (flatIconFor(block) >= 0) return flatIconFor(block);

    // A block is shown by its top face, which is what the inventory has
    // always drawn -- except where the top is not what the block looks
    // like. A bookshelf is planks on top, exactly as in Minecraft, so
    // by its top face it is indistinguishable from the planks block
    // sitting next to it in the menu. Its side is the whole of what
    // makes it a bookshelf.
    if (block == Blocks::Bookshelf) return blockInfo(block).tileSide;

    return blockInfo(block).tileTop;
}

int maxStackOf(StackId id)
{
    if (isDecoratedPot(asBlock(id)) && !isItem(id)) return 1;
    if (isBanner(asBlock(id)) && !isItem(id)) return 16;
    return isItem(id) ? itemInfo(id).maxStack : 64;
}

bool isBreedingFood(StackId id)
{
    return id == Items::Wheat;
}

// What a block turns into when you mine it.
//
// Lived in Application's anonymous namespace, where the only thing
// that could ask was the mining code. It is part of what a block is,
// and the audit has to be able to ask it too: an item whose only
// route into the game is "mine this block" is reachable, and nothing
// outside Application could tell.
StackId blockDrop(BlockId id)
{
    id = oreBase(id);
    if (isLeaves(id)) return Blocks::Air;   // leaves drop nothing

    // A cocoa pod gives its beans; the pod itself is not a thing.
    if (isCocoa(id)) return Items::CocoaBeans;

    // A cake part eaten is not a cake. Minecraft gives nothing back
    // for breaking one either -- you eat it or you lose it.
    if (isCake(id)) return Blocks::Air;

    if (isFurnaceBlock(id)) return furnaceCold(furnaceBase(id));
    if (id == Blocks::BarrelOpen) return Blocks::Barrel;
    if (isAmethystGrowth(id) && id != Blocks::AmethystCluster) return Blocks::Air;
    if (id == Blocks::AmethystCluster) return Items::AmethystShard;
    if (id == Blocks::NetherWart0 || id == Blocks::NetherWart1) return Items::NetherWart;
    if (isCopperBulb(id)) return copperBulbWith(id, false, false);
    if (isLightningRod(id)) return lightningRodOf(0, false);
    if (id == Blocks::SculkCatalystBloom) return Blocks::SculkCatalyst;

    if (isCandle(id)) return candleOf(candleShade(id), 1, false);
    if (isBed(id)) return bedOf(bedColour(id), 0, false);
    if (isDoublePlant(id))
    {
        const DoublePlant kind = doublePlantKind(id);
        if (kind == DoublePlant::TallGrass || kind == DoublePlant::LargeFern) return Items::WheatSeeds;
        return doublePlantOf(kind, false);
    }
    if (isPitcherCrop(id))
        return pitcherCropStage(id) >= 4 ? doublePlantOf(DoublePlant::PitcherPlant, false) : Items::PitcherPod;
    if (isTorchflowerCrop(id)) return Items::TorchflowerSeeds;
    if (isPotted(id)) return Blocks::FlowerPot;
    if (id == Blocks::Fern) return Items::WheatSeeds;
    if (id == Blocks::DeadBush) return Items::Stick;
    if (id == Blocks::Clay) return Items::ClayBall;
    if (id == Blocks::BrownMushroomBlock) return Blocks::BrownMushroom;
    if (id == Blocks::RedMushroomBlock) return Blocks::RedMushroom;
    if (id == Blocks::MushroomStem) return Blocks::Air;
    if (id == Blocks::Mycelium || id == Blocks::Podzol) return Blocks::Dirt;
    if (isCaveVine(id)) return caveVineLit(id) ? Items::GlowBerries : Blocks::Air;
    if (isBigDripleaf(id) || isBigDripleafStem(id)) return bigDripleafOf(0, DripleafTilt::None);
    if (isSmallDripleaf(id)) return smallDripleafOf(0, false);
    if (isGlowLichen(id)) return glowLichenOn(0);
    if (isEnderChest(id)) return Blocks::Obsidian;
    if (isTrappedChest(id)) return chestOf(1, 2, 0);
    if (isChestFamily(id)) return Blocks::Chest;
    if (isCandleCake(id)) return candleOf(candleCakeShade(id), 1, false);

    // However it was hung, what comes back is one sign.
    if (isSign(id)) return signItemOfWood(signWood(id));
    if (isDoor(id)) return doorBase(id);
    if (isTrapdoor(id)) return trapdoorBase(id);
    if (isPinkPetals(id)) return Blocks::PinkPetals1;
    if (id == Blocks::MangrovePropaguleHanging) return Blocks::MangrovePropagule;
    if (isNylium(id)) return Blocks::Netherrack;
    if (id == Blocks::WeepingVinesPlant) return Blocks::WeepingVines;
    if (id == Blocks::TwistingVinesPlant) return Blocks::TwistingVines;
    if (isFrame(id)) return Items::ItemFrame;

    if (isAttachedStem(id)) return attachedStemIsPumpkin(id) ? Items::PumpkinSeeds : Items::MelonSeeds;
    if (isCarvedPumpkin(id)) return static_cast<StackId>(Blocks::CarvedPumpkinFirst + 2);
    if (isJackOLantern(id)) return static_cast<StackId>(Blocks::JackOLanternFirst + 2);
    if (isBeehive(id)) return hiveOf(false, 2, 0);
    if (isBeeNest(id)) return Blocks::Air;
    if (isSweetBerryBush(id)) return Items::SweetBerries;
    if (isCoralBlock(id)) return static_cast<StackId>(Blocks::DeadCoralBlockFirst + coralKind(id));
    if (isLiveCoralPlant(id) || isDeadCoralPlant(id) || isAnyCoralFan(id) || isAnyCoralWallFan(id)) return Blocks::Air;
    if (isTallSeagrass(id) || isTurtleEgg(id)) return Blocks::Air;
    if (id == Blocks::PackedIce || id == Blocks::BlueIce || isFrostedIce(id) || id == Blocks::PowderSnow) return Blocks::Air;
    if (isRepeater(id)) return repeaterOf(0, 1, false);
    if (isComparator(id)) return comparatorOf(0, false, false);
    if (isDaylightDetector(id)) return Blocks::DaylightDetector;
    if (isTarget(id)) return Blocks::Target;
    if (isTripwireHook(id)) return Items::TripwireHook;
    if (isTripwire(id)) return Items::StringItem;
    if (isSpecialRail(id)) return specialRailOf(specialRailFamily(id), false, false);
    if (isIronDoor(id)) return Blocks::IronDoorFirst;
    if (isIronTrapdoor(id)) return Blocks::IronTrapdoorFirst;
    if (isChain(id)) return Blocks::ChainFirst;
    if (isEndRod(id)) return Blocks::EndRodFirst;
    if (id == Blocks::Cobweb) return Items::StringItem;
    if (isFire(id)) return Blocks::Air;
    if (id == Blocks::SoulCampfire) return Blocks::SoulSoil;
    if (isRespawnAnchor(id)) return Blocks::RespawnAnchor0;
    if (isBell(id)) return static_cast<StackId>(Blocks::BellFirst + 2);
    if (isChiseledBookshelf(id)) return shelfOf(2, 0);
    if (isSuspicious(id)) return Blocks::Air;
    if (isDecoratedPot(id)) return static_cast<StackId>(Blocks::DecoratedPotFirst + 2);
    if (isLoom(id)) return static_cast<StackId>(Blocks::LoomFirst + 2);
    if (isSculkVein(id)) return Blocks::Air;
    if (isCreativeOnly(id)) return Blocks::Air;
    if (isCalibratedSensor(id)) return static_cast<StackId>(calibratedOf(2, false));
    if (id == Blocks::SculkSensorLit) return Blocks::SculkSensor;
    if (isBanner(id)) return static_cast<StackId>(bannerOf(bannerColour(id), false, 2));
    if (isSnifferEgg(id)) return Blocks::SnifferEgg0;
    if (id == Blocks::ChorusPlant) return Items::ChorusFruit;

    // A melon comes apart into slices rather than into a melon, which
    // is Minecraft's and is what makes one worth planting.
    if (id == Blocks::Melon) return Items::MelonSlice;

    // A composter and a cauldron come up empty however full they were,
    // which is Minecraft's: what was in them is not yours.
    if (isComposter(id)) return Blocks::Composter0;
    if (isCauldron(id)) return Blocks::Cauldron0;

    switch (id)
    {
        case Blocks::Grass:
        case Blocks::SnowGrass: return Blocks::Dirt;
        case Blocks::Stone: return Blocks::Cobblestone;
        case Blocks::Deepslate: return Blocks::CobbledDeepslate;
        case Blocks::Ice: return Blocks::Air;
        // Where wheat comes from, until there is farmland to grow it
        // on: grass gives up its seeds the way it does in Minecraft,
        // and three of them make a wheat in the crafting grid. That
        // second step is the stopgap and goes when farming lands.
        case Blocks::TallGrass: return Items::WheatSeeds;

        // Ore gives up what is in it. Iron and gold stay as ore --
        // they want a furnace, and there is not one yet.

        // The book on it is given back by the hand that breaks it,
        // the way a furnace gives back what was inside.
        case Blocks::LecternBook: return Blocks::Lectern;
        case Blocks::CoalOre: return Items::Coal;
        case Blocks::DiamondOre: return Items::Diamond;
        case Blocks::NetherQuartzOre: return Items::NetherQuartz;
        case Blocks::EmeraldOre: return Items::Emerald;
        case Blocks::Glowstone: return Items::GlowstoneDust;
        case Blocks::NetherWart: return Items::NetherWart;
        case Blocks::RedstoneOre: return Items::Redstone;
        case Blocks::LapisOre: return Items::LapisLazuli;
        // Minecraft's: a snow block comes apart into snowballs, and
        // four of them pack back into one.
        case Blocks::Snow: return Items::Snowball;

        // The berries come off the vine as something you can eat. They
        // used to come off as the block, which meant the one food that
        // grows underground could be picked up and never eaten. The
        // item audit found it; Food.cpp had listed it as edible all
        // along and the self-test had checked that it was.
        case Blocks::GlowBerries: return Items::GlowBerries;
        case Blocks::CopperOre: return Items::RawCopper;
        case Blocks::IronOre: return Items::RawIron;
        case Blocks::GoldOre: return Items::RawGold;
        case Blocks::NetherGoldOre: return Items::GoldNugget;

        // Whatever state a thing was in when you broke it, what
        // comes up is the thing itself.
        case Blocks::RedstoneDust:
        case Blocks::RedstoneDustLit: return Items::Redstone;
        case Blocks::RedstoneTorchOff: return Blocks::RedstoneTorch;
        case Blocks::LeverOn: return Blocks::LeverOff;
        case Blocks::ButtonOn: return Blocks::ButtonOff;
        case Blocks::RedstoneLampLit: return Blocks::RedstoneLamp;

        default: break;
    }

    // Whichever way it was pointing and whether or not its head was
    // out, what comes up is a piston.
    if (isPiston(id))
        return pistonSticky(id) ? Blocks::StickyPistonNorth : Blocks::PistonNorth;

    return id;
}

int itemDurability(StackId id)
{
    switch (id)
    {
        case Items::FishingRod: return 64;
        case Items::CarrotOnAStick: return 25;
        case Items::WarpedFungusOnAStick: return 100;
        case Items::Crossbow: return 465;
        case Items::Trident: return 250;
        default: return 0;
    }
}
