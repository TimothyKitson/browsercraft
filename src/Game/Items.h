#pragma once
#include "World/Block.h"
#include "Renderer/AtlasTiles.h"
#include <cstdint>

// What can sit in an inventory slot: a block, or an item.
//
// Deliberately wider than BlockId. A block id is a single byte because
// the world stores millions of them and doubling that would double the
// memory a loaded world costs; an inventory slot stores a few dozen, so
// it can afford the extra byte it takes to tell a block from an item.
//
// Below Items::FIRST the number means a block and casts straight back to
// a BlockId. From Items::FIRST up it means an item. Because FIRST is
// exactly one past the largest value a BlockId can hold, no block can
// ever grow into the item range.
using StackId = uint16_t;

namespace Items
{
    constexpr StackId FIRST = 256;

    enum : StackId
    {
        WheatSeeds = FIRST,
        Wheat,
        Leather,
        RawBeef,
        RawPorkchop,
        RawChicken,
        RawMutton,
        Feather,
        Bone,
        StringItem,     // "String" alone collides with too much
        Gunpowder,
        Bread,

        // Materials. Coal and diamond come straight out of the ground;
        // the two ingots want a furnace, which is why nothing smelted
        // can be crafted yet even though the recipes are here.
        Steak,
        CookedPorkchop,
        CookedChicken,
        CookedMutton,

        Stick,
        Coal,
        Diamond,
        IronIngot,
        GoldIngot,

        // Armour, grouped by tier so Armour::pieceOf is arithmetic
        // rather than a switch with sixteen cases. Keep the four pieces
        // in this order within every tier.
        LeatherHelmet, LeatherChestplate, LeatherLeggings, LeatherBoots,
        GoldHelmet, GoldChestplate, GoldLeggings, GoldBoots,
        IronHelmet, IronChestplate, IronLeggings, IronBoots,
        DiamondHelmet, DiamondChestplate, DiamondLeggings, DiamondBoots,

        // Tools, grouped by tier so Tools::tierOf is arithmetic rather
        // than a switch with twenty-five cases in it. Keep the five
        // kinds in this order within every tier.
        WoodPickaxe, WoodAxe, WoodShovel, WoodSword, WoodHoe,
        StonePickaxe, StoneAxe, StoneShovel, StoneSword, StoneHoe,
        IronPickaxe, IronAxe, IronShovel, IronSword, IronHoe,
        GoldPickaxe, GoldAxe, GoldShovel, GoldSword, GoldHoe,
        DiamondPickaxe, DiamondAxe, DiamondShovel, DiamondSword, DiamondHoe,

        Count
    };
}

struct ItemInfo
{
    const char* name = "";
    int tile = Tiles::Blank;
    int maxStack = 64;
};

inline bool isItem(StackId id) { return id >= Items::FIRST && id < Items::Count; }
// Everything below the items is a block, Air included.
inline bool isBlockStack(StackId id) { return id < Items::FIRST; }
// Only call this once isBlockStack has said yes.
inline BlockId asBlock(StackId id) { return static_cast<BlockId>(id); }

const ItemInfo& itemInfo(StackId id);

// These three work on blocks and items alike, which is what lets the
// inventory, the hotbar and the dropped-item renderer stay unaware of
// which they are holding.
const char* displayName(StackId id);
int atlasTileFor(StackId id);
int maxStackOf(StackId id);

// Whether an animal will follow and breed for it.
bool isBreedingFood(StackId id);
