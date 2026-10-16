#pragma once

// Tile indices into the block atlas. The atlas is a 16x16 grid of 16x16
// pixel tiles (256x256 total). Tile 0 is the top-left, indices increase
// left-to-right then down.
//
// Atlas.cpp generates a texture for each of these procedurally -- the game
// ships no image files. Block.cpp maps block faces to these indices.
namespace Tiles
{
    enum : int
    {
        Blank = 0,
        Stone,
        Dirt,
        GrassSide,
        GrassTop,
        Cobblestone,
        Planks,
        Bedrock,
        Water,
        Sand,
        Gravel,
        GoldOre,
        IronOre,
        CoalOre,
        DiamondOre,
        LogSide,
        LogTop,
        Leaves,
        Glass,
        SandstoneSide,
        SandstoneTop,
        SnowTile,
        Ice,
        CactusSide,
        CactusTop,
        Bricks,
        Obsidian,
        TallGrass,
        FlowerRed,
        FlowerYellow,
        MossyCobble,
        Clay,
        PumpkinSide,
        PumpkinTop,
        Wool,
        Torch,
        BirchLogSide,
        BirchLogTop,
        BirchLeaves,
        SnowGrassSide,
        Lava,
        Glowstone,
        Netherrack,
        SoulSand,
        EndStone,
        NetherPortal,
        FarmlandTop,
        FarmlandSide,
        WheatStage0, WheatStage1, WheatStage2, WheatStage3,
        WheatStage4, WheatStage5, WheatStage6, WheatStage7,
        CraftingTop, CraftingSide,
        FurnaceFront, FurnaceLit, FurnaceTop,
        ChestFront, ChestSide, ChestTop,
        BedTop, BedSide,
        TntTop, TntSide, TntBottom, TntPrimedSide,
        LadderTile,
        DoorLower, DoorUpper,
        SugarCane,
        BookshelfSide,
        EnchantTableTop,
        EnchantTableSide,
        EmeraldOre,
        EndFrameTop,
        EndFrameEye,
        EndFrameSide,
        EndPortalTile,
        AnvilTop,
        AnvilSide,
        NetherWartTile,
        BrewingStandTile,
        DirtPathTop,
        DirtPathSide,
        RailTile,
        RailCorner,
        Granite,
        Diorite,
        Andesite,
        Deepslate,
        DeepslateTop,
        Tuff,
        Calcite,
        SpruceLogTop,
        SpruceLogSide,
        SpruceLeaves,
        SprucePlanks,
        DarkOakLogTop,
        DarkOakLogSide,
        DarkOakLeaves,
        DarkOakPlanks,
        NetherBricks,
        JungleLogTop,
        JungleLogSide,
        JungleLeaves,
        JunglePlanks,
        AcaciaLogTop,
        AcaciaLogSide,
        AcaciaLeaves,
        AcaciaPlanks,
        RainDrop,
        SnowFlake,

        TileCount
    };

    // Items share the atlas with the blocks rather than having one of
    // their own: a hotbar slot draws the same way whichever it holds,
    // and one atlas is one texture bind. They start well clear of the
    // blocks so adding a block never renumbers them.
    // Raised from 64 when the blocks grew past it: the bench, the
    // furnace, the chest and the bed between them put the block tiles
    // over that line. Raised again from 96 when the stone stopped being
    // one grey block. Nothing stores a tile index, so moving the items
    // up costs nothing but the static_assert below keeping watch.
    //
    // The atlas is sixteen tiles square, so there are 256 in all. As of
    // the stone family that is 99 blocks, 85 items and 10 cracks: 194
    // used and 62 spare, but the spare was all stranded between the
    // items and the cracks where no block could reach it. The line sits
    // at 144 now, which leaves both sides room -- about forty more
    // blocks and a dozen more items.
    //
    // Growing past 256 means a bigger grid, and a bigger grid costs
    // texture memory in proportion: at the top tile size three
    // 4096-square textures are already most of a gigabyte with their
    // mipmaps. The next step is to drop MAX_TILE_PIXELS, not to widen
    // the grid and hope.
    constexpr int ItemFirst = 144;

    enum : int
    {
        ItemWheatSeeds = ItemFirst,
        ItemWheat,
        ItemLeather,
        ItemRawBeef,
        ItemRawPork,
        ItemRawChicken,
        ItemRawMutton,
        ItemFeather,
        ItemBone,
        ItemString,
        ItemGunpowder,
        ItemBread,

        ItemSteak,
        ItemCookedPork,
        ItemCookedChicken,
        ItemCookedMutton,

        ItemStick,
        ItemCoal,
        ItemFlint,
        ItemBow,
        ItemArrow,
        ItemFlintAndSteel,
        ItemBucket,
        ItemWaterBucket,
        ItemLavaBucket,
        ItemDiamond,
        ItemIronIngot,
        ItemGoldIngot,

        // Five kinds in each of five tiers, in the same order as the
        // Items enum, so the painter can work out which is which from
        // the offset instead of being told twenty-five times.
        ItemToolFirst,
        ItemToolEnd = ItemToolFirst + 25,

        // Four pieces in each of four tiers, same ordering as the Items
        // enum so the painter works it out from the offset.
        ItemArmourFirst = ItemToolEnd,
        ItemArmourEnd = ItemArmourFirst + 16,

        // An arrow in flight, drawn along the tile rather than across
        // its diagonal: the quad it goes on is long and thin, and the
        // item icon squashed down to nothing on it.
        ArrowInFlight = ItemArmourEnd,

        // A blaze's fire charge in flight, on the same long thin quad.
        FireCharge = ArrowInFlight + 1,

        // An experience orb. Not an item -- it never sits in a slot --
        // but it is a sprite drawn in the world, so it lives with them.
        ExperienceOrb = FireCharge + 1,
        ItemPaper = ExperienceOrb + 1,
        ItemBook,
        ItemEmerald,
        ItemEnderPearl,
        ItemEyeOfEnder,
        ItemGlassBottle,
        ItemPotion,
        ItemNetherWart,
        ItemSugar,
        ItemBlazePowder,
        ItemGlowstoneDust,
        ItemShield,
        ItemBoat,
        ItemMinecart,
        ItemBlazeRod,
        ItemNetherBrick,
        ItemMagmaCream,

        ItemTileEnd = ItemMagmaCream + 1
    };

    // Ten progressively more broken overlay tiles for the mining animation.
    constexpr int CrackFirst = 240;
    constexpr int CrackStages = 10;

    static_assert(static_cast<int>(TileCount) <= ItemFirst, "block tiles ran into the items");
    static_assert(static_cast<int>(ItemTileEnd) <= CrackFirst, "item tiles ran into the cracks");
}
