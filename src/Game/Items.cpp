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

int atlasTileFor(StackId id)
{
    // An item has one sprite and that is all it has.
    if (isItem(id)) return itemInfo(id).tile;

    const BlockId block = asBlock(id);

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
    if (isLeaves(id)) return Blocks::Air;   // leaves drop nothing

    // A cocoa pod gives its beans; the pod itself is not a thing.
    if (isCocoa(id)) return Items::CocoaBeans;

    // A cake part eaten is not a cake. Minecraft gives nothing back
    // for breaking one either -- you eat it or you lose it.
    if (isCake(id)) return Blocks::Air;

    if (id == Blocks::CandleLit) return Blocks::Candle;

    // However it was hung, what comes back is one sign.
    if (isSign(id)) return Items::Sign;
    if (isFrame(id)) return Items::ItemFrame;

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
        case Blocks::Ice: return Blocks::Air;
        // Where wheat comes from, until there is farmland to grow it
        // on: grass gives up its seeds the way it does in Minecraft,
        // and three of them make a wheat in the crafting grid. That
        // second step is the stopgap and goes when farming lands.
        case Blocks::TallGrass: return Items::WheatSeeds;

        // Ore gives up what is in it. Iron and gold stay as ore --
        // they want a furnace, and there is not one yet.
        case Blocks::FurnaceLit: return Blocks::Furnace;
        case Blocks::SmokerLit: return Blocks::Smoker;
        case Blocks::BlastFurnaceLit: return Blocks::BlastFurnace;

        // The book on it is given back by the hand that breaks it,
        // the way a furnace gives back what was inside.
        case Blocks::LecternBook: return Blocks::Lectern;
        case Blocks::CoalOre: return Items::Coal;
        case Blocks::DiamondOre: return Items::Diamond;
        case Blocks::EmeraldOre: return Items::Emerald;
        case Blocks::Glowstone: return Items::GlowstoneDust;
        case Blocks::NetherWart: return Items::NetherWart;
        case Blocks::RedstoneOre: return Items::Redstone;
        case Blocks::LapisOre: return Items::LapisLazuli;

        // The berries come off the vine as something you can eat. They
        // used to come off as the block, which meant the one food that
        // grows underground could be picked up and never eaten. The
        // item audit found it; Food.cpp had listed it as edible all
        // along and the self-test had checked that it was.
        case Blocks::GlowBerries: return Items::GlowBerries;
        case Blocks::CopperOre: return Items::RawCopper;
        case Blocks::CopperBulbLit: return Blocks::CopperBulb;

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
