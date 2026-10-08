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
