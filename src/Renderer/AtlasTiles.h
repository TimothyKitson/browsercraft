#pragma once

// Tile indices into the block atlas. The atlas is a 20x20 grid of tiles,
// four hundred in all. Tile 0 is the top-left, indices increase
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
        RedstoneOre,
        RedstoneDust,
        RedstoneDustLit,
        RedstoneTorch,
        RedstoneTorchOff,
        LeverTile,
        LeverOnTile,
        ButtonTile,
        ButtonOnTile,
        RedstoneLamp,
        RedstoneLampLit,
        RedstoneBlockTile,
        TuffBricks,
        TrialSpawnerTop,
        TrialSpawnerTopLit,
        TrialSpawnerSide,
        TrialSpawnerSideLit,
        VaultTop,
        VaultSide,
        VaultSideOpen,
        PistonTop,
        PistonSide,
        PistonFace,
        StickyPistonFace,
        PistonBack,
        PistonArm,
        CrafterTop,
        CrafterTopLit,
        CrafterSide,
        CopperOre,
        CopperBlock,
        ExposedCopper,
        WeatheredCopper,
        OxidizedCopper,
        CopperBulbTile,
        CopperBulbLitTile,
        DripstoneBlock,
        DripstoneSpike,
        MossTile,
        AzaleaLeavesTile,
        GlowBerriesTile,
        OminousVaultSide,
        OminousVaultSideOpen,
        SpawnerTile,

        PrismarineTile,
        PrismarineBricksTile,
        DarkPrismarineTile,
        SeaLanternTile,
        SpongeTile,
        WetSpongeTile,

        KelpTile,
        KelpTopTile,
        SeagrassTile,
        SeaPickleTile,
        TubeCoralBlockTile,
        BrainCoralBlockTile,
        BubbleCoralBlockTile,
        FireCoralBlockTile,
        HornCoralBlockTile,
        TubeCoralTile,
        BrainCoralTile,
        BubbleCoralTile,
        FireCoralTile,
        HornCoralTile,

        DeepslateBricksTile,
        DeepslateTilesTile,
        SculkTile,
        SculkSensorTop,
        SculkSensorTopLit,
        SculkSensorSide,
        SculkShriekerTop,
        SculkShriekerSide,
        SculkCatalystTop,
        SculkCatalystSide,
        SoulLanternTile,
        ReinforcedDeepslateTop,
        ReinforcedDeepslateSide,

        PurpurTile,
        PurpurPillarTop,
        PurpurPillarSide,
        EndBricksTile,
        ChorusPlantTile,
        ChorusFlowerTile,
        BlackstoneTile,
        PolishedBlackstoneTile,
        GildedBlackstoneTile,
        BasaltTop,
        BasaltSide,

        LilyPadTile,
        VineTile,
        MudTile,
        EndCrystalTile,
        WitherSkullTile,
        BeaconTile,
        IronBlockTile,
        GoldBlockTile,
        DiamondBlockTile,
        EmeraldBlockTile,
        CoalBlockTile,
        NoteBlockTile,
        ObserverTop,
        ObserverTopLit,
        ObserverSide,
        HopperTop,
        HopperSide,
        DropperFront,
        DispenserFront,
        AmethystTile,
        BuddingTile,
        ClusterTile,
        SmoothBasaltTile,
        LanternTile,
        BarrelTop,
        BarrelSide,
        CampfireTop,
        CampfireLog,
        CarrotStage0, CarrotStage1, CarrotStage2, CarrotStage3,
        CarrotStage4, CarrotStage5, CarrotStage6, CarrotStage7,
        PotatoStage0, PotatoStage1, PotatoStage2, PotatoStage3,
        PotatoStage4, PotatoStage5, PotatoStage6, PotatoStage7,

        SmokerFront, SmokerFrontLit, SmokerTop, SmokerSide,
        BlastFurnaceFront, BlastFurnaceFrontLit, BlastFurnaceTop, BlastFurnaceSide,
        ComposterSide, ComposterBottom, ComposterTop,
        ComposterFill1, ComposterFill2, ComposterFill3, ComposterReady,
        GrindstoneSide, GrindstoneFace, GrindstonePivot,
        LecternTop, LecternTopBook, LecternSide, LecternFront, LecternBase,
        CauldronTop, CauldronSide, CauldronBottom,
        CauldronWater1, CauldronWater2, CauldronWater3,

        BeetrootStage0, BeetrootStage1, BeetrootStage2, BeetrootStage3,
        BeetrootStage4, BeetrootStage5, BeetrootStage6, BeetrootStage7,
        MelonSide, MelonTop,
        MelonStemYoung, MelonStemRipe,
        BambooTile,
        CocoaStage0, CocoaStage1, CocoaStage2,
        CakeTop, CakeSide, CakeBottom, CakeInner,
        CandleTile, CandleLitTile,
        ScaffoldTop, ScaffoldSide, ScaffoldBottom,
        SignTile,
        ItemFrameTile,
        ShulkerBoxTile,
        // Fifteen more wools; white is the Wool tile up above, where
        // it has always been.
        WoolFirst,
        WoolEnd = WoolFirst + 15,

        LapisOreTile = WoolEnd, LapisBlockTile,
        AncientDebrisTop, AncientDebrisSide, NetheriteBlockTile,
        OchreFroglightTop, OchreFroglightSide,
        VerdantFroglightTop, VerdantFroglightSide,
        PearlescentFroglightTop, PearlescentFroglightSide,
        SlimeBlockTile,

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
    // The atlas is twenty tiles square, so there are 400 in all. It was
    // sixteen and 256 until the blocks filled it: 145 block tiles
    // against a line at 144, with copper still to come.
    //
    // A bigger grid costs texture memory in proportion, so the ceiling
    // on tile size came down with it -- twenty across at 128 pixels is
    // less VRAM than sixteen across at 256 was. Widening the grid
    // without lowering that ceiling is what the old note here warned
    // against, and it was right.
    // Raised from 200 when the End's city and the Nether's bastion
    // took the block tiles past it. This costs nothing: the atlas is
    // still twenty tiles square, and the items were using 63 of the
    // 190 slots between them and the cracks. Moving the line is free;
    // widening the grid would have cost texture memory on a machine
    // that has none to spare.
    // Raised from 240 when the crops took the block tiles past it --
    // sixteen stages between carrots and potatoes. The grid went from
    // 400 tiles to 576 at the same time, so this is not the items
    // being squeezed: blocks get 400 and items get 160, where before
    // they had 240 and 150.
    constexpr int ItemFirst = 400;

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

        // A blaze's fire charge in flight, on the same long thin quad,
        // and a wind charge beside it.
        FireCharge = ArrowInFlight + 1,
        WindCharge = FireCharge + 1,

        // An experience orb. Not an item -- it never sits in a slot --
        // but it is a sprite drawn in the world, so it lives with them.
        ExperienceOrb = WindCharge + 1,
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
        ItemRedstone,
        ItemBreezeRod,
        ItemWindCharge,
        ItemMace,
        ItemTrialKey,
        ItemRawCopper,
        ItemCopperIngot,
        ItemGlowBerries,
        ItemOminousBottle,
        ItemOminousKey,
        ItemNetherStar,
        ItemApple,
        ItemBoneMeal,
        ItemCarrot,
        ItemPotato,
        ItemBakedPotato,
        ItemBowl,
        ItemBeetroot,
        ItemBeetrootSeeds,
        ItemBeetrootSoup,
        ItemMelonSlice,
        ItemMelonSeeds,
        ItemEgg,
        ItemMilkBucket,
        ItemCake,
        ItemCookie,
        ItemCocoaBeans,
        ItemSign,
        ItemItemFrame,
        ItemElytra,
        // Sixteen dyes, in Minecraft's order, so a dye's offset is the
        // colour it makes.
        ItemDyeFirst,
        ItemDyeEnd = ItemDyeFirst + 16,

        ItemLapis = ItemDyeEnd,
        ItemInkSac,
        ItemNetheriteScrap,
        ItemNetheriteIngot,

        // Five tools then four pieces of armour, in the same order as
        // the Items enum, so the painter works out which is which
        // from the offset instead of being told nine times.
        ItemNetheriteFirst,
        ItemNetheriteEnd = ItemNetheriteFirst + 9,

        ItemGhastTear = ItemNetheriteEnd,
        ItemShulkerShell,
        ItemTrident,
        ItemTotem,
        ItemSlimeball,
        ItemGoldNugget,

        ItemTileEnd = ItemGoldNugget + 1
    };

    // Ten progressively more broken overlay tiles for the mining animation.
    constexpr int CrackFirst = 560;
    constexpr int CrackStages = 10;

    static_assert(static_cast<int>(TileCount) <= ItemFirst, "block tiles ran into the items");
    static_assert(static_cast<int>(ItemTileEnd) <= CrackFirst, "item tiles ran into the cracks");
}
