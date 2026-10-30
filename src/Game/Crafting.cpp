#include "Crafting.h"
#include "Player/Inventory.h"
#include "Items.h"
#include <algorithm>

namespace
{
    constexpr StackId _ = Blocks::Air;   // an empty cell, readably

    struct Recipe
    {
        bool shapeless;
        int width, height;        // shaped only
        StackId pattern[9];       // shaped: row-major; shapeless: the ingredient list
        int inputCount;           // shapeless only
        StackId result;
        int resultCount;
    };

    // The recipe book is still short: there are items now, but no
    // sticks, ingots or tools, so most of Minecraft's recipes have no
    // ingredients to be made from yet.
    const Recipe RECIPES[] = {
        // One log becomes four planks of its own wood. Birch has no
        // plank of its own yet, so it gives oak -- planksFromLog() says
        // so in one place rather than this table saying it four times.
        { true,  0, 0, { Blocks::Log },        1, Blocks::Planks,        4 },
        { true,  0, 0, { Blocks::BirchLog },   1, Blocks::Planks,        4 },
        { true,  0, 0, { Blocks::SpruceLog },  1, Blocks::SprucePlanks,  4 },
        { true,  0, 0, { Blocks::DarkOakLog }, 1, Blocks::DarkOakPlanks, 4 },
        { true,  0, 0, { Blocks::JungleLog },  1, Blocks::JunglePlanks,  4 },
        { true,  0, 0, { Blocks::AcaciaLog },  1, Blocks::AcaciaPlanks,  4 },

        // Three wheat becomes a loaf. Minecraft wants them in a row,
        // which needs a bench this game has not got yet, so it takes
        // them in any arrangement for the same three wheat.
        { true,  0, 0, { Items::Wheat, Items::Wheat, Items::Wheat }, 3, Items::Bread, 1 },

        // An eye of ender: a pearl and the powder off a blaze's rod,
        // which is Minecraft's. It used to take gunpowder, because
        // there was no blaze to get the powder from.
        { true,  0, 0, { Items::EnderPearl, Items::BlazePowder }, 2, Items::EyeOfEnder, 1 },

        // The frames. In Minecraft these are only found in
        // strongholds, and there are none; without a recipe the End is
        // somewhere you can only reach from the command line.
        { false, 3, 3, { Blocks::EndStone,  Blocks::EndStone,  Blocks::EndStone,
                         Blocks::EndStone,  Items::EnderPearl, Blocks::EndStone,
                         Blocks::EndStone,  Blocks::EndStone,  Blocks::EndStone },
          0, Blocks::EndPortalFrame, 4 },

        // A shield: planks round an iron boss.
        { false, 3, 3, { Blocks::Planks,    Items::IronIngot,  Blocks::Planks,
                         Blocks::Planks,    Blocks::Planks,    Blocks::Planks,
                         _,                 Blocks::Planks,    _ },
          0, Items::Shield, 1 },

        // A boat: planks round three sides, open at the top.
        { false, 3, 2, { Blocks::Planks,    _,                 Blocks::Planks,
                         Blocks::Planks,    Blocks::Planks,    Blocks::Planks },
          0, Items::Boat, 1 },

        // A minecart: five iron round an open top, the same shape the
        // boat is made in.
        { false, 3, 2, { Items::IronIngot,  _,                 Items::IronIngot,
                         Items::IronIngot,  Items::IronIngot,  Items::IronIngot },
          0, Items::Minecart, 1 },

        // Rails: six iron down the sides with a stick between them.
        { false, 3, 3, { Items::IronIngot,  _,                 Items::IronIngot,
                         Items::IronIngot,  Items::Stick,      Items::IronIngot,
                         Items::IronIngot,  _,                 Items::IronIngot },
          0, Blocks::RailNorthSouth, 16 },

        // Brewing's glassware and the sweetener. One cane makes one
        // sugar; three glass makes three bottles.
        { true,  0, 0, { Blocks::SugarCane },
          1, Items::Sugar, 1 },
        { false, 3, 2, { Blocks::Glass,     _,                 Blocks::Glass,
                         _,                 Blocks::Glass,     _ },
          0, Items::GlassBottle, 3 },

        // A rod ground down. Two powders a rod, which is Minecraft's,
        // and it is the whole supply: every potion in the game is
        // downstream of something a blaze was carrying.
        { true,  0, 0, { Items::BlazeRod },
          1, Items::BlazePowder, 2 },

        // Four nether bricks, which is the only thing to build with
        // down there that is not the ground itself.
        { false, 2, 2, { Items::NetherBrick, Items::NetherBrick,
                         Items::NetherBrick, Items::NetherBrick },
          0, Blocks::NetherBricks, 1 },

        // 1.21's two. A rod makes four charges, which is Minecraft's;
        // the mace wants a heavy core with its rod and there are no
        // vaults here to find one in, so it takes an iron ingot
        // instead -- the one other thing in the game that is nothing
        // but weight.
        { true,  0, 0, { Items::BreezeRod },
          1, Items::WindCharge, 4 },
        { true,  0, 0, { Items::BreezeRod, Items::IronIngot },
          2, Items::Mace, 1 },

        // Copper: nine ingots into a block and back out again, and the
        // bulb, which is Minecraft's -- copper round a blaze rod with
        // redstone behind it.
        { false, 3, 3, { Items::CopperIngot, Items::CopperIngot, Items::CopperIngot,
                         Items::CopperIngot, Items::CopperIngot, Items::CopperIngot,
                         Items::CopperIngot, Items::CopperIngot, Items::CopperIngot },
          0, Blocks::CopperBlock, 1 },
        { true,  0, 0, { Blocks::CopperBlock },
          1, Items::CopperIngot, 9 },
        { false, 3, 3, { _,                  Items::CopperIngot, _,
                         Items::CopperIngot, Items::BlazeRod,    Items::CopperIngot,
                         _,                  Items::Redstone,    _ },
          0, Blocks::CopperBulb, 1 },

        // The crafter: a bench that works on its own. Minecraft wants a
        // dropper with the iron and the redstone, and there are none
        // here, so it takes a piston -- the one other thing in the game
        // whose whole job is moving something along when it is told to.
        { false, 3, 3, { Items::IronIngot, Items::IronIngot,     Items::IronIngot,
                         Items::IronIngot, Blocks::CraftingTable, Items::IronIngot,
                         Items::Redstone,  Blocks::PistonNorth,   Items::Redstone },
          0, Blocks::Crafter, 1 },

        // A piston: planks over cobble with an iron plate and the dust
        // that drives it, which is Minecraft's. Sticky wants a
        // slimeball and there are no slimes here, so it takes the
        // magma cream off a magma cube -- the one other thing in the
        // game that is sticky at all.
        { false, 3, 3, { Blocks::Planks,      Blocks::Planks,    Blocks::Planks,
                         Blocks::Cobblestone, Items::IronIngot,  Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Redstone,   Blocks::Cobblestone },
          0, Blocks::PistonNorth, 1 },
        { true,  0, 0, { Blocks::PistonNorth, Items::MagmaCream },
          2, Blocks::StickyPistonNorth, 1 },

        // Redstone. A torch is dust on a stick, a lever is a stick on
        // a cobble, a button is one stone, a lamp is glowstone wrapped
        // in dust, and nine dust packs into a block -- all Minecraft's,
        // except that the lever takes the cobble beside the stick
        // rather than under it, which this grid cannot tell apart.
        { false, 1, 2, { Items::Redstone,
                         Items::Stick },
          0, Blocks::RedstoneTorch, 1 },
        { true,  0, 0, { Items::Stick, Blocks::Cobblestone },
          2, Blocks::LeverOff, 1 },
        { true,  0, 0, { Blocks::Stone },
          1, Blocks::ButtonOff, 1 },
        { false, 3, 3, { _,                 Items::Redstone,   _,
                         Items::Redstone,   Blocks::Glowstone, Items::Redstone,
                         _,                 Items::Redstone,   _ },
          0, Blocks::RedstoneLamp, 1 },
        { false, 3, 3, { Items::Redstone,   Items::Redstone,   Items::Redstone,
                         Items::Redstone,   Items::Redstone,   Items::Redstone,
                         Items::Redstone,   Items::Redstone,   Items::Redstone },
          0, Blocks::RedstoneBlock, 1 },
        { true,  0, 0, { Blocks::RedstoneBlock },
          1, Items::Redstone, 9 },

        // The stand: a blaze powder sat on three cobblestone.
        { false, 3, 2, { _,                 Items::BlazePowder, _,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::BrewingStand, 1 },

        // Seven iron: a slab across the top, a stem, and a foot.
        // Minecraft spends three iron blocks on the top row, but there
        // is no iron block here to spend, so it is ingots all the way.
        { false, 3, 3, { Items::IronIngot,  Items::IronIngot,  Items::IronIngot,
                         _,                 Items::IronIngot,  _,
                         Items::IronIngot,  Items::IronIngot,  Items::IronIngot },
          0, Blocks::Anvil, 1 },

        // Four obsidian, two diamond and a book. The book is why the
        // whole cane chain above has to exist.
        { false, 3, 3, { _,                 Items::Book,       _,
                         Items::Diamond,    Blocks::Obsidian,  Items::Diamond,
                         Blocks::Obsidian,  Blocks::Obsidian,  Blocks::Obsidian },
          0, Blocks::EnchantingTable, 1 },

        // Nine of a thing into one block, and the one block back
        // into nine. Minecraft's oldest way of tidying a chest, and
        // what a beacon's pyramid is built out of.
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot },
          0, Blocks::IronBlock, 1 },
        { true, 0, 0, { Blocks::IronBlock }, 1, Items::IronIngot, 9 },
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot,
                         Items::GoldIngot, Items::GoldIngot, Items::GoldIngot,
                         Items::GoldIngot, Items::GoldIngot, Items::GoldIngot },
          0, Blocks::GoldBlock, 1 },
        { true, 0, 0, { Blocks::GoldBlock }, 1, Items::GoldIngot, 9 },
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond,
                         Items::Diamond, Items::Diamond, Items::Diamond,
                         Items::Diamond, Items::Diamond, Items::Diamond },
          0, Blocks::DiamondBlock, 1 },
        { true, 0, 0, { Blocks::DiamondBlock }, 1, Items::Diamond, 9 },
        { false, 3, 3, { Items::Emerald, Items::Emerald, Items::Emerald,
                         Items::Emerald, Items::Emerald, Items::Emerald,
                         Items::Emerald, Items::Emerald, Items::Emerald },
          0, Blocks::EmeraldBlock, 1 },
        { true, 0, 0, { Blocks::EmeraldBlock }, 1, Items::Emerald, 9 },
        { false, 3, 3, { Items::Coal, Items::Coal, Items::Coal,
                         Items::Coal, Items::Coal, Items::Coal,
                         Items::Coal, Items::Coal, Items::Coal },
          0, Blocks::CoalBlock, 1 },
        { true, 0, 0, { Blocks::CoalBlock }, 1, Items::Coal, 9 },

        // A lantern: iron round a torch. A barrel: planks and slabs,
        // which is Minecraft's and the cheapest chest in the game. A
        // campfire: logs under a stick frame, with coal to light it.
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Blocks::Torch,    Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot },
          0, Blocks::Lantern, 1 },

        { false, 3, 3, { Blocks::Planks,   Blocks::PlankSlab, Blocks::Planks,
                         Blocks::Planks,   Blocks::Air,       Blocks::Planks,
                         Blocks::Planks,   Blocks::PlankSlab, Blocks::Planks },
          0, Blocks::Barrel, 1 },

        { false, 3, 3, { Blocks::Air,    Items::Stick,  Blocks::Air,
                         Items::Stick,   Items::Coal,   Items::Stick,
                         Blocks::Log,    Blocks::Log,   Blocks::Log },
          0, Blocks::Campfire, 1 },

        // A bone ground up makes three meal, which is Minecraft's.
        { true, 0, 0, { Items::Bone }, 1, Items::BoneMeal, 3 },

        // Three planks in a V. The bowl is the only container in the
        // game that is not a block, and the only way to carry a soup.
        { false, 3, 3, { Blocks::Planks, Blocks::Air,    Blocks::Planks,
                         Blocks::Air,    Blocks::Planks, Blocks::Air,
                         Blocks::Air,    Blocks::Air,    Blocks::Air },
          0, Items::Bowl, 4 },

        // Six beets in a bowl, which is Minecraft's and is the one
        // thing a beetroot is for.
        { false, 3, 3, { Items::Beetroot, Items::Beetroot, Items::Beetroot,
                         Items::Beetroot, Items::Beetroot, Items::Beetroot,
                         Blocks::Air,     Items::Bowl,     Blocks::Air },
          0, Items::BeetrootSoup, 1 },

        // Nine slices make a melon back, and one slice makes the seed
        // that grows another. Both are Minecraft's.
        { false, 3, 3, { Items::MelonSlice, Items::MelonSlice, Items::MelonSlice,
                         Items::MelonSlice, Items::MelonSlice, Items::MelonSlice,
                         Items::MelonSlice, Items::MelonSlice, Items::MelonSlice },
          0, Blocks::Melon, 1 },

        { true, 0, 0, { Items::MelonSlice }, 1, Items::MelonSeeds, 1 },

        // A cake: milk along the top, an egg between two sugars, and
        // wheat under it. Minecraft's, buckets and all -- and the
        // buckets come back, which is handled where the grid is
        // emptied rather than here.
        { false, 3, 3, { Items::MilkBucket, Items::MilkBucket, Items::MilkBucket,
                         Items::Sugar,      Items::Egg,        Items::Sugar,
                         Items::Wheat,      Items::Wheat,      Items::Wheat },
          0, Items::Cake, 1 },

        // Two wheat round a bean makes eight cookies, which is
        // Minecraft's and the reason cocoa is worth climbing for.
        { false, 3, 3, { Items::Wheat, Items::CocoaBeans, Items::Wheat,
                         Blocks::Air,  Blocks::Air,       Blocks::Air,
                         Blocks::Air,  Blocks::Air,       Blocks::Air },
          0, Items::Cookie, 8 },

        // Six canes and a string make six scaffolding, which is
        // Minecraft's and the only thing bamboo is really for.
        { false, 3, 3, { Blocks::Bamboo, Items::StringItem, Blocks::Bamboo,
                         Blocks::Bamboo, Blocks::Air,       Blocks::Bamboo,
                         Blocks::Bamboo, Blocks::Air,       Blocks::Bamboo },
          0, Blocks::Scaffolding, 6 },

        // A wick on something soft. Minecraft wants honeycomb for the
        // wax and there are no bees here, so the wick stands on wool
        // instead -- which is at least white, and the same shape.
        { false, 1, 2, { Items::StringItem,
                         Blocks::Wool }, 0, Blocks::Candle, 1 },

        // The workstations. A smoker is a furnace with logs built
        // round it; a blast furnace is one with iron over it and
        // smooth stone under. Both are Minecraft's, and both are the
        // same trade: half the diet for twice the speed.
        { false, 3, 3, { Blocks::Air,  Blocks::Log,     Blocks::Air,
                         Blocks::Log,  Blocks::Furnace, Blocks::Log,
                         Blocks::Air,  Blocks::Log,     Blocks::Air },
          0, Blocks::Smoker, 1 },

        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Blocks::Furnace,  Items::IronIngot,
                         Blocks::Stone,    Blocks::Stone,    Blocks::Stone },
          0, Blocks::BlastFurnace, 1 },

        // A composter is a slatted box: seven slabs with the top open.
        { false, 3, 3, { Blocks::PlankSlab, Blocks::Air,       Blocks::PlankSlab,
                         Blocks::PlankSlab, Blocks::Air,       Blocks::PlankSlab,
                         Blocks::PlankSlab, Blocks::PlankSlab, Blocks::PlankSlab },
          0, Blocks::Composter0, 1 },

        // A grindstone is a wheel slung between two posts.
        { false, 3, 3, { Items::Stick,     Blocks::StoneSlab, Items::Stick,
                         Blocks::Planks,   Blocks::Air,       Blocks::Planks,
                         Blocks::Air,      Blocks::Air,       Blocks::Air },
          0, Blocks::Grindstone, 1 },

        // A lectern is a bookshelf on a stand, which is what it is.
        { false, 3, 3, { Blocks::PlankSlab, Blocks::PlankSlab, Blocks::PlankSlab,
                         Blocks::Air,       Blocks::Bookshelf, Blocks::Air,
                         Blocks::Air,       Blocks::PlankSlab, Blocks::Air },
          0, Blocks::Lectern, 1 },

        // A cauldron is seven iron round an open top.
        { false, 3, 3, { Items::IronIngot, Blocks::Air,      Items::IronIngot,
                         Items::IronIngot, Blocks::Air,      Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot },
          0, Blocks::Cauldron0, 1 },

        // A hopper: iron round a chest, which is Minecraft's. A
        // dropper is cobble round nothing with redstone behind it,
        // and a dispenser is a dropper with a bow in it.
        { false, 3, 3, { Items::IronIngot,    Blocks::Air,     Items::IronIngot,
                         Items::IronIngot,    Blocks::Chest,   Items::IronIngot,
                         Blocks::Air,         Items::IronIngot, Blocks::Air },
          0, Blocks::Hopper, 1 },

        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Blocks::Cobblestone, Blocks::Air,         Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Redstone,     Blocks::Cobblestone },
          0, Blocks::Dropper, 1 },

        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Bow,          Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Redstone,     Blocks::Cobblestone },
          0, Blocks::Dispenser, 1 },

        // A note block: planks round redstone. An observer: cobble
        // and redstone round a lump of quartz -- there is no quartz
        // here, so a diamond stands in as the one thing precious
        // enough to make it feel earned.
        { false, 3, 3, { Blocks::Planks, Blocks::Planks,  Blocks::Planks,
                         Blocks::Planks, Items::Redstone, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks,  Blocks::Planks },
          0, Blocks::NoteBlock, 1 },

        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Items::Redstone,     Items::Redstone,     Items::Diamond,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::Observer, 1 },

        // The beacon: glass over the star, obsidian under it. The one
        // recipe in the game you can only make once you have killed
        // something, which is the whole point of a nether star.
        { false, 3, 3, { Blocks::Glass,    Blocks::Glass,      Blocks::Glass,
                         Blocks::Glass,    Items::NetherStar,  Blocks::Glass,
                         Blocks::Obsidian, Blocks::Obsidian,   Blocks::Obsidian },
          0, Blocks::Beacon, 1 },

        // Three cane in a row makes three paper, three paper and a
        // hide makes a book, and six planks round three books makes a
        // shelf. All three want a bench: every one of them is three
        // wide, which the two-by-two grid cannot reach.
        { false, 3, 1, { Blocks::SugarCane, Blocks::SugarCane, Blocks::SugarCane },
          0, Items::Paper, 3 },
        { true,  0, 0, { Items::Paper, Items::Paper, Items::Paper, Items::Leather },
          4, Items::Book, 1 },
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Items::Book,    Items::Book,    Items::Book,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::Bookshelf, 1 },

        // Four sand in a square becomes one sandstone.
        { false, 2, 2, { Blocks::Sand, Blocks::Sand,
                         Blocks::Sand, Blocks::Sand }, 0, Blocks::Sandstone, 1 },

        // Four planks make the bench, which is the only way to reach a
        // 3x3 grid and so the only way to reach anything three wide.
        { false, 2, 2, { Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks }, 0, Blocks::CraftingTable, 1 },

        // Eight cobblestone round an empty middle make a furnace.
        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Blocks::Cobblestone, _,                   Blocks::Cobblestone,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::Furnace, 1 },

        // Eight planks round an empty middle make a chest.
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, _,              Blocks::Planks,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::Chest, 1 },

        // Five gunpowder and four sand, crossed, makes TNT.
        { false, 3, 3, { Items::Gunpowder, Blocks::Sand,     Items::Gunpowder,
                         Blocks::Sand,     Items::Gunpowder, Blocks::Sand,
                         Items::Gunpowder, Blocks::Sand,     Items::Gunpowder },
          0, Blocks::Tnt, 1 },

        // Six of a block in a staircase shape makes four stairs.
        { false, 3, 3, { Blocks::Stone, _, _,
                         Blocks::Stone, Blocks::Stone, _,
                         Blocks::Stone, Blocks::Stone, Blocks::Stone },
          0, Blocks::StoneStairsNorth, 4 },
        { false, 3, 3, { Blocks::Cobblestone, _, _,
                         Blocks::Cobblestone, Blocks::Cobblestone, _,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::CobbleStairsNorth, 4 },
        { false, 3, 3, { Blocks::Planks, _, _,
                         Blocks::Planks, Blocks::Planks, _,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::PlankStairsNorth, 4 },
        { false, 3, 3, { Blocks::Sandstone, _, _,
                         Blocks::Sandstone, Blocks::Sandstone, _,
                         Blocks::Sandstone, Blocks::Sandstone, Blocks::Sandstone },
          0, Blocks::SandstoneStairsNorth, 4 },

        // Six planks make three doors.
        { false, 2, 3, { Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks },
          0, Blocks::DoorLowShutNorth, 3 },

        // Four planks and two sticks make three lengths of fence.
        { false, 3, 2, { Blocks::Planks, Items::Stick, Blocks::Planks,
                         Blocks::Planks, Items::Stick, Blocks::Planks },
          0, Blocks::OakFence, 3 },

        // Seven sticks make three ladders.
        { false, 3, 3, { Items::Stick, _,            Items::Stick,
                         Items::Stick, Items::Stick, Items::Stick,
                         Items::Stick, _,            Items::Stick },
          0, Blocks::LadderNorth, 3 },

        // Three of a block in a row makes six slabs of it.
        { false, 3, 1, { Blocks::Stone, Blocks::Stone, Blocks::Stone },
          0, Blocks::StoneSlab, 6 },
        { false, 3, 1, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::CobblestoneSlab, 6 },
        { false, 3, 1, { Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::PlankSlab, 6 },
        { false, 3, 1, { Blocks::Sandstone, Blocks::Sandstone, Blocks::Sandstone },
          0, Blocks::SandstoneSlab, 6 },

        // Three iron in a V makes a bucket.
        { false, 3, 2, { Items::IronIngot, _,                Items::IronIngot,
                         _,                Items::IronIngot, _ },
          0, Items::Bucket, 1 },

        // Iron and flint make the thing that lights a portal.
        { true, 0, 0, { Items::IronIngot, Items::Flint }, 2, Items::FlintAndSteel, 1 },

        // Three sticks and three string make a bow.
        { false, 3, 3, { _,                Items::Stick,      Items::StringItem,
                         Items::Stick,     _,                 Items::StringItem,
                         _,                Items::Stick,      Items::StringItem },
          0, Items::Bow, 1 },

        // Flint, stick and feather make four arrows.
        { false, 1, 3, { Items::Flint,
                         Items::Stick,
                         Items::Feather }, 0, Items::Arrow, 4 },

        // Coal over a stick makes four torches, which is the recipe the
        // whole early game turns on -- there was no way to light a cave
        // before it.
        { false, 1, 2, { Items::Coal,
                         Items::Stick }, 0, Blocks::Torch, 4 },

        // Four string makes a block of wool, so a bed does not depend on
        // finding sheep.
        { false, 2, 2, { Items::StringItem, Items::StringItem,
                         Items::StringItem, Items::StringItem }, 0, Blocks::Wool, 1 },

        // Three wool over three planks makes a bed.
        { false, 3, 2, { Blocks::Wool,   Blocks::Wool,   Blocks::Wool,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::Bed, 1 },

        // Two planks, one above the other, make four sticks.
        { false, 1, 2, { Blocks::Planks,
                         Blocks::Planks }, 0, Items::Stick, 4 },


        // Armour. Every piece is its material beaten into shape;
        // all four are three wide, so all four want the bench.
        { false, 3, 2, { Items::Leather, Items::Leather, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherHelmet, 1 },
        { false, 3, 3, { Items::Leather, _, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather }, 0, Items::LeatherChestplate, 1 },
        { false, 3, 3, { Items::Leather, Items::Leather, Items::Leather, Items::Leather, _, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherLeggings, 1 },
        { false, 3, 2, { Items::Leather, _, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherBoots, 1 },

        { false, 3, 2, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldHelmet, 1 },
        { false, 3, 3, { Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot }, 0, Items::GoldChestplate, 1 },
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldLeggings, 1 },
        { false, 3, 2, { Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldBoots, 1 },

        { false, 3, 2, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronHelmet, 1 },
        { false, 3, 3, { Items::IronIngot, _, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot }, 0, Items::IronChestplate, 1 },
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronLeggings, 1 },
        { false, 3, 2, { Items::IronIngot, _, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronBoots, 1 },

        { false, 3, 2, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondHelmet, 1 },
        { false, 3, 3, { Items::Diamond, _, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond }, 0, Items::DiamondChestplate, 1 },
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondLeggings, 1 },
        { false, 3, 2, { Items::Diamond, _, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondBoots, 1 },

        // Tools. Every one is its head material over a stick or two,
        // which is why none of them could be made before the bench --
        // all but the shovel and the sword are wider than a 2x2 grid.
        // Wood
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::WoodPickaxe, 1 },
        { false, 2, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks, Items::Stick, _, Items::Stick }, 0, Items::WoodAxe, 1 },
        { false, 1, 3, { Blocks::Planks, Items::Stick, Items::Stick }, 0, Items::WoodShovel, 1 },
        { false, 1, 3, { Blocks::Planks, Blocks::Planks, Items::Stick }, 0, Items::WoodSword, 1 },
        { false, 2, 3, { Blocks::Planks, Blocks::Planks, _, Items::Stick, _, Items::Stick }, 0, Items::WoodHoe, 1 },

        // Stone
        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::StonePickaxe, 1 },
        { false, 2, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone, Items::Stick, _, Items::Stick }, 0, Items::StoneAxe, 1 },
        { false, 1, 3, { Blocks::Cobblestone, Items::Stick, Items::Stick }, 0, Items::StoneShovel, 1 },
        { false, 1, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Items::Stick }, 0, Items::StoneSword, 1 },
        { false, 2, 3, { Blocks::Cobblestone, Blocks::Cobblestone, _, Items::Stick, _, Items::Stick }, 0, Items::StoneHoe, 1 },

        // Iron
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::IronPickaxe, 1 },
        { false, 2, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::Stick, _, Items::Stick }, 0, Items::IronAxe, 1 },
        { false, 1, 3, { Items::IronIngot, Items::Stick, Items::Stick }, 0, Items::IronShovel, 1 },
        { false, 1, 3, { Items::IronIngot, Items::IronIngot, Items::Stick }, 0, Items::IronSword, 1 },
        { false, 2, 3, { Items::IronIngot, Items::IronIngot, _, Items::Stick, _, Items::Stick }, 0, Items::IronHoe, 1 },

        // Gold
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::GoldPickaxe, 1 },
        { false, 2, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::Stick, _, Items::Stick }, 0, Items::GoldAxe, 1 },
        { false, 1, 3, { Items::GoldIngot, Items::Stick, Items::Stick }, 0, Items::GoldShovel, 1 },
        { false, 1, 3, { Items::GoldIngot, Items::GoldIngot, Items::Stick }, 0, Items::GoldSword, 1 },
        { false, 2, 3, { Items::GoldIngot, Items::GoldIngot, _, Items::Stick, _, Items::Stick }, 0, Items::GoldHoe, 1 },

        // Diamond
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::DiamondPickaxe, 1 },
        { false, 2, 3, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Stick, _, Items::Stick }, 0, Items::DiamondAxe, 1 },
        { false, 1, 3, { Items::Diamond, Items::Stick, Items::Stick }, 0, Items::DiamondShovel, 1 },
        { false, 1, 3, { Items::Diamond, Items::Diamond, Items::Stick }, 0, Items::DiamondSword, 1 },
        { false, 2, 3, { Items::Diamond, Items::Diamond, _, Items::Stick, _, Items::Stick }, 0, Items::DiamondHoe, 1 },
    };

    // Shrinks the used area of the grid down to its bounding box, so a
    // recipe placed in the bottom-right corner still matches.
    bool trim(const ItemStack* grid, int size, int& outX, int& outY, int& outW, int& outH)
    {
        int minX = size, minY = size, maxX = -1, maxY = -1;

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
                if (!grid[y * 3 + x].empty())
                {
                    minX = std::min(minX, x); maxX = std::max(maxX, x);
                    minY = std::min(minY, y); maxY = std::max(maxY, y);
                }

        if (maxX < 0) return false;   // nothing in the grid

        outX = minX; outY = minY;
        outW = maxX - minX + 1;
        outH = maxY - minY + 1;
        return true;
    }

    // Whether what is in the cell will do for what the recipe asked
    // for. Exact, except that any plank stands in for any other: in
    // Minecraft a spruce bench is a bench. Written once here rather
    // than by listing every wood in all thirty-odd recipes that take
    // planks, which is how a new wood ends up unable to make anything.
    bool satisfies(StackId wanted, StackId have)
    {
        if (have == wanted) return true;
        if (!isBlockStack(wanted) || !isBlockStack(have)) return false;
        return isPlanks(asBlock(wanted)) && isPlanks(asBlock(have));
    }

    bool matchesShaped(const Recipe& recipe, const ItemStack* grid, int size)
    {
        int x0 = 0, y0 = 0, width = 0, height = 0;
        if (!trim(grid, size, x0, y0, width, height)) return false;
        if (width != recipe.width || height != recipe.height) return false;

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                const ItemStack& cell = grid[(y0 + y) * 3 + (x0 + x)];
                const StackId wanted = recipe.pattern[y * recipe.width + x];
                if (wanted == _ && !cell.empty()) return false;
                if (wanted != _ && (cell.empty() || !satisfies(wanted, cell.id))) return false;
            }

        return true;
    }

    bool matchesShapeless(const Recipe& recipe, const ItemStack* grid, int size)
    {
        // Every filled cell has to be accounted for by exactly one
        // ingredient, and every ingredient has to be found.
        bool used[9] = { false };
        for (int i = 0; i < recipe.inputCount; ++i)
        {
            bool found = false;
            for (int y = 0; y < size && !found; ++y)
                for (int x = 0; x < size && !found; ++x)
                {
                    const int cell = y * 3 + x;
                    if (used[cell]) continue;
                    if (grid[cell].empty() || !satisfies(recipe.pattern[i], grid[cell].id)) continue;
                    used[cell] = true;
                    found = true;
                }
            if (!found) return false;
        }

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const int cell = y * 3 + x;
                if (!grid[cell].empty() && !used[cell]) return false;
            }

        return true;
    }
}

namespace Crafting
{
    CraftOutput match(const ItemStack* grid, int size)
    {
        for (const Recipe& recipe : RECIPES)
        {
            const bool hit = recipe.shapeless ? matchesShapeless(recipe, grid, size)
                                              : matchesShaped(recipe, grid, size);
            if (hit) return CraftOutput{ recipe.result, recipe.resultCount };
        }
        return CraftOutput{};
    }

    void consume(ItemStack* grid, int size)
    {
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                ItemStack& cell = grid[y * 3 + x];
                if (cell.empty()) continue;

                // A recipe uses what is in the bucket, not the bucket.
                // Without this a cake cost three iron as well as three
                // milk, which is Minecraft's recipe at four times the
                // price. Only a single bucket can be in a slot, so
                // there is never a count to carry across.
                if (cell.id == Items::MilkBucket || cell.id == Items::WaterBucket ||
                    cell.id == Items::LavaBucket)
                {
                    cell.id = Items::Bucket;
                    cell.count = 1;
                    cell.damage = 0;
                    cell.enchantments = 0;
                    continue;
                }

                cell.count -= 1;
                if (cell.count <= 0) cell.clear();
            }
    }
}


std::vector<StackId> Crafting::everyOutput()
{
    std::vector<StackId> out;
    out.reserve(std::size(RECIPES));
    for (const Recipe& recipe : RECIPES) out.push_back(recipe.result);
    return out;
}

std::vector<StackId> Crafting::everyIngredient()
{
    std::vector<StackId> out;
    for (const Recipe& recipe : RECIPES)
    {
        const int many = recipe.shapeless ? recipe.inputCount : 9;
        for (int i = 0; i < many; ++i)
            if (recipe.pattern[i] != Blocks::Air) out.push_back(recipe.pattern[i]);
    }
    return out;
}
