#include "Items.h"
#include <algorithm>

// A BlockId is a byte, and Items::FIRST is one past the largest value a
// byte holds, so the two ranges cannot meet. Checked rather than assumed,
// because the day BlockId widens this is what will catch it.
static_assert(static_cast<unsigned long long>(Items::FIRST) >
                  static_cast<unsigned long long>(static_cast<BlockId>(~BlockId{ 0 })),
              "BlockId has grown; raise Items::FIRST and migrate saved inventories");

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
    // A block is shown by its top face, which is what the inventory has
    // always drawn; an item has one sprite and that is all it has.
    return isItem(id) ? itemInfo(id).tile : blockInfo(asBlock(id)).tileTop;
}

int maxStackOf(StackId id)
{
    return isItem(id) ? itemInfo(id).maxStack : 64;
}

bool isBreedingFood(StackId id)
{
    return id == Items::Wheat;
}
