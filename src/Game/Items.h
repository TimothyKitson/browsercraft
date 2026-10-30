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
    // Where the items begin. Blocks have everything below this, so
    // this is also the ceiling on how many blocks there can be.
    //
    // It was 256, which was the whole of a uint8_t and so the whole of
    // the block space. Both moved together: a block id is sixteen bits
    // now, and the items start far enough up that running out again is
    // not something anybody has to think about.
    constexpr StackId FIRST = 1024;

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
        Flint,
        Bow,
        Arrow,
        FlintAndSteel,
        Bucket,
        WaterBucket,
        LavaBucket,
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

        // Sugar cane is a block, so it is not here; what it is made
        // into is.
        Paper,
        Book,
        Emerald,
        EnderPearl,
        EyeOfEnder,

        // Brewing. The potion is one id for every drink there is: which
        // one it holds rides in the stack's damage field, the way wear
        // does on a tool, so adding a potion costs no item id and a
        // saved one comes back without the format changing.
        GlassBottle,
        Potion,
        NetherWart,
        Sugar,
        BlazePowder,
        GlowstoneDust,

        // Held up against what is coming at you.
        Shield,

        // Set down on water and got into.
        Boat,
        // Set down on a rail and got into.
        Minecart,

        // What a blaze is carrying. Ground down, it is what every
        // brewing stand and every potion in the game starts from.
        BlazeRod,

        // Netherrack, fired. Four of them make the block a fortress is
        // built of, which is the only thing to build with down there
        // that is not the ground itself.
        NetherBrick,

        // What a magma cube leaves. Brewed, it is the only way to walk
        // through fire, which is what makes the Nether survivable.
        MagmaCream,

        // Redstone dust, in the hand. Put down it becomes the block
        // that carries power along the floor.
        Redstone,

        // What a breeze is made of, and what it leaves. The rod makes
        // the mace; the charges are what it was throwing at you.
        BreezeRod,
        WindCharge,

        // 1.21's weapon. It hits for what it is worth standing still
        // and for a great deal more at the end of a fall, which is the
        // whole of how it is meant to be used.
        Mace,

        // What a trial spawner pays out when its waves are done, and
        // the only thing that opens a vault.
        TrialKey,

        // Copper, out of the ground and out of the furnace.
        RawCopper,
        CopperIngot,

        // The one thing you can eat that grows underground.
        GlowBerries,

        // The hard way round a trial chamber. The bottle is what a
        // vault pays you for doing it the easy way; drinking it turns
        // the next chamber ominous, and an ominous spawner pays the
        // key that opens the vaults worth opening.
        OminousBottle,
        OminousTrialKey,

        // Off an oak, now and then. The one food in the game you get
        // by cutting down a tree rather than by farming or killing.
        Apple,

        // Ground bone. Thrown on a crop it brings it on, which is the
        // difference between a field and a wait.
        BoneMeal,

        // The two other crops. A carrot and a potato are both the
        // seed and the food, which is Minecraft's and saves two item
        // ids: you plant the thing you eat.
        Carrot,
        Potato,
        BakedPotato,

        // Three planks. The one thing in the game that is a container
        // rather than a tool or a food, and the only way to carry a
        // soup about.
        Bowl,

        // The fourth crop, and the one thing you can make of it. A
        // beetroot is barely food; six of them in a bowl is a meal.
        Beetroot,
        BeetrootSeeds,
        BeetrootSoup,

        // A melon is a block, so it is not here; what it is cut into
        // is, and the seed that goes back in the ground.
        MelonSlice,
        MelonSeeds,

        // What a chicken lays, and what a bucket gets out of a cow.
        // Both are here for the cake, which cannot be made without
        // them, and the milk is also the one thing that clears what
        // is on you.
        Egg,
        MilkBucket,

        // Set down and eaten where it stands, which nothing else in
        // the game is.
        Cake,

        // Two wheat and a bean. The bean grows on the side of a
        // jungle trunk and is the only thing in the game that does.
        Cookie,
        CocoaBeans,

        // A board on a wall with words on it. One item, four ways of
        // hanging it, the same as a ladder.
        Sign,

        // A frame hung on a wall to show one thing off, or to say what
        // is in the chest under it.
        ItemFrame,

        // Worn where a chestplate goes and stops nothing. What it does
        // is turn a fall into a flight, which is the only thing in the
        // game that changes how you get anywhere.
        Elytra,

        // Off a shulker. Two of them round a chest make a box that
        // can be carried.
        ShulkerShell,

        // What a ghast leaves. Brewed, it is the only thing in the
        // game that puts hearts back on their own.
        GhastTear,

        // Off a drowned, now and then. The hardest swing in the game
        // short of a mace at the end of a fall. Minecraft throws it
        // as well; nothing here can throw a thing and get it back.
        Trident,

        // The sixteen dyes, in Minecraft's order. White is nought and
        // black is fifteen, and the wool a dye makes shares its
        // number -- which is what lets one recipe cover all sixteen
        // rather than sixteen recipes covering one each.
        DyeWhite, DyeOrange, DyeMagenta, DyeLightBlue, DyeYellow, DyeLime,
        DyePink, DyeGray, DyeLightGray, DyeCyan, DyePurple, DyeBlue,
        DyeBrown, DyeGreen, DyeRed, DyeBlack,

        // Out of the ground, and the only blue there is.
        LapisLazuli,

        // Out of a squid, and the only black there is.
        InkSac,

        // Netherite, and the tier above diamond it makes.
        //
        // The nine tools and pieces are at the end of the list rather
        // than beside the diamond ones they follow, because an item's
        // number is what a saved inventory holds: slotting them in
        // where they belong would have renumbered half the game and
        // emptied every world anybody had. The arithmetic that finds
        // a tier reads them specially instead, which is one `if` a
        // side against breaking every save there is.
        NetheriteScrap,
        NetheriteIngot,

        // What a wither leaves. One of them makes a beacon, which is
        // the only thing in the game it is for.
        NetherStar,

        NetheritePickaxe, NetheriteAxe, NetheriteShovel, NetheriteSword, NetheriteHoe,
        NetheriteHelmet, NetheriteChestplate, NetheriteLeggings, NetheriteBoots,

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

// What a block turns into when you mine it. Air means it drops
// nothing; anything else is what lands on the floor.
//
// Declared here rather than beside the rest of what a block is,
// because a drop can be an item and Block.h cannot see StackId --
// Items.h is the one that includes Block.h, not the other way round.
StackId blockDrop(BlockId id);

// These three work on blocks and items alike, which is what lets the
// inventory, the hotbar and the dropped-item renderer stay unaware of
// which they are holding.
const char* displayName(StackId id);
int atlasTileFor(StackId id);
int maxStackOf(StackId id);

// A dye, and which of the sixteen colours it is. The dyes are
// contiguous and in the same order as the wools, so this is
// arithmetic rather than a table.
inline bool isDye(StackId id)
{
    return id >= Items::DyeWhite && id <= Items::DyeBlack;
}
inline int dyeColour(StackId id) { return isDye(id) ? id - Items::DyeWhite : 0; }
inline StackId dyeOfColour(int colour)
{
    if (colour < 0) colour = 0;
    if (colour > 15) colour = 15;
    return static_cast<StackId>(Items::DyeWhite + colour);
}

// Whether an animal will follow and breed for it.
bool isBreedingFood(StackId id);
