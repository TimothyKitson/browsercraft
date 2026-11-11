#pragma once
#include <algorithm>
#include <cstdint>

// Sixteen bits rather than eight.
//
// The game ran out: there were 256 ids and the two hundred and
// fifty-seventh block was a lantern. Everything past that point was
// simply impossible to add, which is not a limit worth keeping for a
// byte.
//
// It costs a byte per block in a chunk -- 32 KB becomes 64 KB, and
// what a render distance of eight holds goes from about ten megabytes
// to twenty. That is affordable where "no more blocks, ever" is not.
using BlockId = uint16_t;

// What a block is made of, as far as the ear is concerned: it picks the
// digging and footstep sounds.
enum class Material { Stone, Dirt, Grass, Wood, Sand, Glass, Wool, Plant, Snow };

// Block ids. Keep Air == 0: empty chunk memory is then all air.
namespace Blocks
{
    enum : BlockId
    {
        Air = 0,
        Stone,
        Grass,
        Dirt,
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
        Log,
        Leaves,
        Glass,
        Sandstone,
        Snow,
        Ice,
        Cactus,
        Bricks,
        Obsidian,
        TallGrass,
        FlowerRed,
        FlowerYellow,
        MossyCobblestone,
        Clay,
        Pumpkin,
        Wool,
        Torch,
        BirchLog,
        BirchLeaves,
        Glowstone,
        Lava,
        SnowGrass,
        Netherrack,
        SoulSand,
        EndStone,
        NetherPortal,

        // Farming. The crop is eight blocks rather than one with a
        // growth value, because a block here is a bare id with nowhere
        // to put metadata; the stage is the id.
        CraftingTable,
        Furnace,
        FurnaceLit,
        Chest,
        Bed,

        // Half blocks. Each is its own id rather than a flag on the
        // block it is cut from, for the same reason wheat's growth
        // stages are ids: there is nowhere to put a flag.
        Tnt,
        TntPrimed,

        // A ladder faces one of four ways, and each facing is its own
        // id. A block has nowhere to keep which way it points, so the
        // id carries it -- the same answer wheat's growth stages use.
        // Keep the four in this order: north, east, south, west.
        LadderNorth,
        LadderEast,
        LadderSouth,
        LadderWest,

        // Stairs, four facings each, in the same north-east-south-west
        // order the ladders use. The facing is the way the step faces
        // -- the low side -- which is the way you walk up them.
        StoneStairsNorth, StoneStairsEast, StoneStairsSouth, StoneStairsWest,
        CobbleStairsNorth, CobbleStairsEast, CobbleStairsSouth, CobbleStairsWest,
        PlankStairsNorth, PlankStairsEast, PlankStairsSouth, PlankStairsWest,
        SandstoneStairsNorth, SandstoneStairsEast, SandstoneStairsSouth, SandstoneStairsWest,

        // A door is two cells tall, faces one of four ways and is either
        // shut or standing open: sixteen ids for one door. The order is
        // fixed and the helpers below are the only things that should
        // know it.
        //   bottom shut N E S W, bottom open N E S W,
        //   top shut N E S W, top open N E S W
        DoorLowShutNorth, DoorLowShutEast, DoorLowShutSouth, DoorLowShutWest,
        DoorLowOpenNorth, DoorLowOpenEast, DoorLowOpenSouth, DoorLowOpenWest,
        DoorHighShutNorth, DoorHighShutEast, DoorHighShutSouth, DoorHighShutWest,
        DoorHighOpenNorth, DoorHighOpenEast, DoorHighOpenSouth, DoorHighOpenWest,

        OakFence,
        StoneSlab,
        CobblestoneSlab,
        PlankSlab,
        SandstoneSlab,

        Farmland,
        Wheat0, Wheat1, Wheat2, Wheat3, Wheat4, Wheat5, Wheat6, Wheat7,

        // Sugar cane, and what it is eventually for. Appended at the
        // end: a block's id is what a saved chunk stores, so inserting
        // one anywhere else would turn every stone in every existing
        // world into something else.
        SugarCane,
        Bookshelf,
        EnchantingTable,
        EmeraldOre,

        // The way to the End. The frame is two ids rather than one with
        // a flag, for the same reason the doors are sixteen: a block
        // here has nowhere to keep whether an eye has been set in it.
        EndPortalFrame,
        EndPortalFrameFilled,
        EndPortal,

        // Mending and moving enchantments. Appended, like the rest.
        Anvil,

        // Brewing. The wart grows on soul sand in the Nether, which is
        // the only reason to go looking in the flats down there.
        NetherWart,
        BrewingStand,

        // Trodden earth. What a village lays between its houses, and
        // the thing that makes one read as a place rather than a set
        // of buildings dropped on a field.
        DirtPath,

        // Rails. Flat on the floor, and shaped by what they touch:
        // two straights and four corners, each its own id for the same
        // reason the doors are sixteen -- a block here has nowhere to
        // keep a shape of its own. Appended, like the rest.
        RailNorthSouth,
        RailEastWest,
        RailNorthEast,
        RailSouthEast,
        RailSouthWest,
        RailNorthWest,

        // The stone the ground is actually made of. Vanilla stopped
        // being one grey block a long time ago: granite, diorite and
        // andesite come in blobs through the upper rock, deepslate
        // takes over below zero with tuff seaming through it, and
        // calcite turns up in pockets. Appended, like the rest.
        Granite,
        Diorite,
        Andesite,
        Deepslate,
        Tuff,
        Calcite,

        // Two more woods. Spruce for the cold and the heights, dark oak
        // for the thick of the forest -- which is where vanilla puts
        // them, and what stops every tree in the world being the same
        // tree. Appended, like the rest.
        SpruceLog,
        SpruceLeaves,
        SprucePlanks,
        DarkOakLog,
        DarkOakLeaves,
        DarkOakPlanks,

        // What a fortress is built of, and the one thing down there you
        // can build with that is not red.
        NetherBricks,

        // The jungle's wood, and the savanna's. Jungle is the darkest
        // red of the six; acacia is the orange one nothing else looks
        // anything like.
        JungleLog,
        JungleLeaves,
        JunglePlanks,
        AcaciaLog,
        AcaciaLeaves,
        AcaciaPlanks,

        // Redstone. Dust carries power along the floor, a torch and a
        // block give it out for nothing, a lever and a button ask for
        // it, and a lamp and a door answer. Every one of them is two
        // ids rather than a block with a bit set, which is how the
        // doors already work here.
        RedstoneOre,
        RedstoneDust,
        RedstoneDustLit,
        RedstoneTorch,
        RedstoneTorchOff,
        LeverOff,
        LeverOn,
        ButtonOff,
        ButtonOn,
        RedstoneLamp,
        RedstoneLampLit,
        RedstoneBlock,

        // A trial chamber: the brick it is built of, the spawner that
        // fills it and the vault the spawner's key opens. The spawner
        // and the vault each carry their state in their id, the way
        // everything else here does.
        TuffBricks,
        TrialSpawner,
        TrialSpawnerActive,
        TrialSpawnerSpent,
        Vault,
        VaultOpen,

        // Pistons. Four facings each, because everything that faces in
        // this game faces along the compass -- a ladder, a door, a set
        // of stairs -- and a piston that could also point up and down
        // would be six more ids of the same thing. Three states to a
        // facing: the body waiting, the body with its head out, and
        // the head itself, which is its own block in the cell in
        // front. Sticky is the same set again.
        PistonNorth, PistonEast, PistonSouth, PistonWest,
        PistonOutNorth, PistonOutEast, PistonOutSouth, PistonOutWest,
        PistonHeadNorth, PistonHeadEast, PistonHeadSouth, PistonHeadWest,
        StickyPistonNorth, StickyPistonEast, StickyPistonSouth, StickyPistonWest,
        StickyPistonOutNorth, StickyPistonOutEast, StickyPistonOutSouth, StickyPistonOutWest,
        StickyPistonHeadNorth, StickyPistonHeadEast, StickyPistonHeadSouth, StickyPistonHeadWest,

        // The crafter: a bench that works on its own when something
        // tells it to. Two ids, the way everything powered here is:
        // waiting, and just fired.
        Crafter,
        CrafterLit,

        // Copper. The seam, the block it makes, and the three stages
        // that block goes through standing in the open air -- which is
        // the only thing in this world that changes on its own once it
        // has been put down.
        CopperOre,
        CopperBlock,
        ExposedCopper,
        WeatheredCopper,
        OxidizedCopper,

        // 1.21's lamp: it flips every time something gives it a pulse
        // rather than following the power, so one button is a switch.
        CopperBulb,
        CopperBulbLit,

        // What a cave is made of where it is not just grey. Dripstone
        // hangs and stands; moss, azalea and the berries that light
        // themselves are the other sort of cave.
        DripstoneBlock,
        DripstoneDown,
        DripstoneUp,
        Moss,
        AzaleaLeaves,
        GlowBerries,

        // The vaults an ominous key opens, which hold the best of what
        // there is to find.
        OminousVault,
        OminousVaultOpen,

        // A monster spawner: the cage in a fortress or a mineshaft
        // that keeps sending the same thing out while you are near it.
        // What it sends is decided by where it is rather than by
        // anything stored in it -- a cage in the Nether makes blazes,
        // one in the rock makes spiders -- because a block here is an
        // id and nothing else.
        MonsterSpawner,

        // The sea: the three prismarines and the lantern a monument is
        // built out of, and the sponge in the room behind the elder.
        Prismarine,
        PrismarineBricks,
        DarkPrismarine,
        SeaLantern,
        Sponge,
        WetSponge,

        // What grows in the water. Kelp stands in a column with a
        // frond on top of it; seagrass is one blade on the floor; the
        // corals are five colours of reef, each a solid block and a
        // fan that grows off it.
        Kelp,
        KelpTop,
        Seagrass,
        SeaPickle,
        TubeCoralBlock,
        BrainCoralBlock,
        BubbleCoralBlock,
        FireCoralBlock,
        HornCoralBlock,
        TubeCoral,
        BrainCoral,
        BubbleCoral,
        FireCoral,
        HornCoral,

        // The deep dark, and the city in it. Sculk is what has grown
        // over everything down there; the sensor and the shrieker are
        // what it listens with.
        DeepslateBricks,
        DeepslateTiles,
        Sculk,
        SculkSensor,
        SculkSensorLit,
        SculkShrieker,
        SculkCatalyst,
        SoulLantern,
        ReinforcedDeepslate,

        // The End's city: purpur, and the end stone cut into bricks.
        PurpurBlock,
        PurpurPillar,
        EndStoneBricks,
        ChorusPlant,
        ChorusFlower,

        // The Nether's bastion: blackstone, and the gold in it.
        Blackstone,
        PolishedBlackstoneBricks,
        GildedBlackstone,
        Basalt,

        // The swamp: what floats on it and what hangs over it.
        LilyPad,
        Vine,
        Mud,

        // The End's own two: the cage a crystal sits in, and the
        // bedrock slab under it.
        EndCrystal,
        EndCrystalBase,

        // What a wither is built out of, and what it pays for.
        WitherSkull,
        Beacon,

        // The storage blocks. Nine of a thing packed into one, which
        // is Minecraft's oldest way of tidying a chest -- and what a
        // beacon's pyramid is built out of.
        IronBlock,
        GoldBlock,
        DiamondBlock,
        EmeraldBlock,
        CoalBlock,

        // The two redstone parts that need nothing but a signal: one
        // that makes a noise when it gets one, and one that sends one
        // when what it is watching changes.
        NoteBlock,
        NoteBlockLit,
        Observer,
        ObserverLit,

        // The three that hold things: one that moves them along on its
        // own, and two that throw them out when told.
        Hopper,
        Dropper,
        Dispenser,

        // A geode: a hollow ball of amethyst buried in the rock, with
        // a shell of calcite round it. The one thing underground that
        // is worth finding for how it looks.
        AmethystBlock,
        BuddingAmethyst,
        AmethystCluster,
        SmoothBasalt,

        // The three every base ends up with: a light you can hang, a
        // chest that wants no room above it, and a fire you can cook
        // on and should not stand in.
        Lantern,
        Barrel,
        Campfire,

        // Two more crops, so a field is a field rather than a wheat
        // demonstration. Eight stages each, the same as the wheat:
        // a crop keeps its whole state in its block id, so a field
        // saves and loads with the chunk it stands in.
        Carrot0, Carrot1, Carrot2, Carrot3, Carrot4, Carrot5, Carrot6, Carrot7,
        Potato0, Potato1, Potato2, Potato3, Potato4, Potato5, Potato6, Potato7,

        // The workstations. A village had a bench and a furnace in it
        // and nothing else anybody would stand at, which is most of
        // why its houses read as sheds.
        //
        // The first two are the furnace with a narrower diet and half
        // the clock: food in one, metal in the other. Everything that
        // runs a furnace runs these, which is why they are a kind
        // rather than a second machine.
        Smoker, SmokerLit,
        BlastFurnace, BlastFurnaceLit,

        // Nine ids for nine levels, because a composter's whole state
        // is how full it is -- the same trick the crops use, and for
        // the same reason: it saves with the chunk it stands in.
        Composter0, Composter1, Composter2, Composter3, Composter4,
        Composter5, Composter6, Composter7, Composter8,

        // One takes an enchantment back off, and one holds a book
        // open. Neither is worth much alone; together they are what a
        // village library is for.
        Grindstone,
        Lectern,
        LecternBook,

        // Four levels of water, filled from a bucket or by the rain.
        Cauldron0, Cauldron1, Cauldron2, Cauldron3,

        // The fourth crop. Costs eight ids and a tile and nothing
        // else, which is the whole reason the crops were made general
        // when the carrot went in.
        Beetroot0, Beetroot1, Beetroot2, Beetroot3,
        Beetroot4, Beetroot5, Beetroot6, Beetroot7,

        // A melon, and the stem that grows one. The stem is a crop
        // like any other; what makes it a melon is that the thing it
        // ripens into stands beside it rather than on it.
        Melon,
        MelonStem0, MelonStem1, MelonStem2, MelonStem3,
        MelonStem4, MelonStem5, MelonStem6, MelonStem7,

        // What grows fastest of anything in the game, and the only
        // thing in a jungle worth cutting down twice.
        Bamboo,

        // Cocoa, which grows on the side of a jungle trunk and is the
        // only thing in the game that grows on wood.
        Cocoa0, Cocoa1, Cocoa2,

        // A cake is eaten where it stands rather than carried, so its
        // seven states are seven blocks: a cake with a slice out of it
        // is a different block from a whole one.
        Cake0, Cake1, Cake2, Cake3, Cake4, Cake5, Cake6,

        // A light you can put out again, which no other light in the
        // game can do.
        Candle, CandleLit,

        // Climbed rather than stood on, and the only quick way up the
        // outside of something you are building.
        Scaffolding,

        // A board on a wall with words on it. Four facings, the same
        // four a ladder has and hung the same way: what it says lives
        // in a table beside the level, because a block id has nowhere
        // to put a sentence.
        SignNorth, SignEast, SignSouth, SignWest,

        // A frame on a wall holding one thing, hung the same four
        // ways. What is in it lives in a table beside the level, the
        // same as a sign's words.
        FrameNorth, FrameEast, FrameSouth, FrameWest,

        // Two shells round a chest. Minecraft's keeps what is in it
        // when you pick it up; this one spills it like any other box,
        // because a stack here has nowhere to carry a chest inside it.
        ShulkerBox,

        // The other fifteen wools. White is Blocks::Wool, which is
        // where it has always been: an id is what a saved chunk holds,
        // so the sixteen cannot be one tidy run without emptying every
        // world anybody has built. isWool knows both halves.
        WoolOrange, WoolMagenta, WoolLightBlue, WoolYellow, WoolLime,
        WoolPink, WoolGray, WoolLightGray, WoolCyan, WoolPurple,
        WoolBlue, WoolBrown, WoolGreen, WoolRed, WoolBlack,

        // The one ore that is not a metal or a gem: lapis, which is
        // where blue comes from and the only blue in the game.
        LapisOre,
        LapisBlock,

        // Buried in the deep Nether, and the block nine ingots make.
        // The one ore in the game that does not burn and the one a
        // blast cannot destroy.
        AncientDebris,
        NetheriteBlock,

        // What a frog leaves when it eats a magma cube. Three of them,
        // by the climate the frog grew up in, and the brightest light
        // in the game that is not on fire.
        OchreFroglight,
        VerdantFroglight,
        PearlescentFroglight,

        // Nine slimeballs. It breaks a fall outright, which is the
        // only thing in the game that does.
        SlimeBlock,

        // The way across the thousand blocks of nothing between the
        // island you arrive on and the ones the cities are built on.
        // Step into one and it puts you down on the far side.
        EndGateway,

        FurnaceTurned,
        FurnaceTurnedEnd = FurnaceTurned + 18,
        BarrelOpen = FurnaceTurnedEnd,
        NetherWart0,
        NetherWart1,
        AnvilChipped,
        AnvilDamaged,
        CopperBulbPowered,
        CopperBulbLitPowered,
        SculkCatalystBloom,
        VaultEjecting,
        OminousVaultEjecting,
        TrialSpawnerActiveOminous,
        TrialSpawnerEjecting,
        TrialSpawnerEjectingOminous,
        TrialSpawnerSpentOminous,
        WaterFlow1, WaterFlow2, WaterFlow3, WaterFlow4, WaterFlow5, WaterFlow6, WaterFlow7, WaterFalling,
        LavaFlow1, LavaFlow2, LavaFlow3, LavaFlow4, LavaFlow5, LavaFlow6, LavaFlow7, LavaFalling,
        SmallAmethystBud, MediumAmethystBud, LargeAmethystBud,
        BirchPlanks, OakSapling, SpruceSapling, BirchSapling, JungleSapling, AcaciaSapling, DarkOakSapling, StrippedOakLog, StrippedSpruceLog, StrippedBirchLog, StrippedJungleLog, StrippedAcaciaLog, StrippedDarkOakLog,
        SpruceDoor, SpruceDoorLast = SpruceDoor + 15,
        BirchDoor, BirchDoorLast = BirchDoor + 15,
        JungleDoor, JungleDoorLast = JungleDoor + 15,
        AcaciaDoor, AcaciaDoorLast = AcaciaDoor + 15,
        DarkOakDoor, DarkOakDoorLast = DarkOakDoor + 15,
        OakTrapdoor, OakTrapdoorLast = OakTrapdoor + 15,
        SpruceTrapdoor, SpruceTrapdoorLast = SpruceTrapdoor + 15,
        BirchTrapdoor, BirchTrapdoorLast = BirchTrapdoor + 15,
        JungleTrapdoor, JungleTrapdoorLast = JungleTrapdoor + 15,
        AcaciaTrapdoor, AcaciaTrapdoorLast = AcaciaTrapdoor + 15,
        DarkOakTrapdoor, DarkOakTrapdoorLast = DarkOakTrapdoor + 15,
        SpruceSign, SpruceSignLast = SpruceSign + 3,
        BirchSign, BirchSignLast = BirchSign + 3,
        JungleSign, JungleSignLast = JungleSign + 3,
        AcaciaSign, AcaciaSignLast = AcaciaSign + 3,
        DarkOakSign, DarkOakSignLast = DarkOakSign + 3,
        CherryLog, CherryLeaves, CherryPlanks, StrippedCherryLog, CherrySapling,
        CherryDoor, CherryDoorLast = CherryDoor + 15,
        CherryTrapdoor, CherryTrapdoorLast = CherryTrapdoor + 15,
        CherrySign, CherrySignLast = CherrySign + 3,
        PinkPetals1, PinkPetals2, PinkPetals3, PinkPetals4,
        MangroveLog, MangroveLeaves, MangrovePlanks, StrippedMangroveLog, MangrovePropagule,
        MangrovePropaguleHanging, MangroveRoots, MuddyMangroveRoots,
        MangroveDoor, MangroveDoorLast = MangroveDoor + 15,
        MangroveTrapdoor, MangroveTrapdoorLast = MangroveTrapdoor + 15,
        MangroveSign, MangroveSignLast = MangroveSign + 3,
        MudBricks, PackedMud,
        BambooBlock, StrippedBambooBlock, BambooPlanks, BambooMosaic, BambooSlab,
        BambooDoor, BambooDoorLast = BambooDoor + 15,
        BambooTrapdoor, BambooTrapdoorLast = BambooTrapdoor + 15,
        BambooSign, BambooSignLast = BambooSign + 3,
        CrimsonStem, StrippedCrimsonStem, CrimsonPlanks, CrimsonNylium, CrimsonFungus, CrimsonRoots,
        WarpedStem, StrippedWarpedStem, WarpedPlanks, WarpedNylium, WarpedFungus, WarpedRoots,
        NetherSprouts, NetherWartBlock, WarpedWartBlock, Shroomlight,
        WeepingVines, WeepingVinesPlant, TwistingVines, TwistingVinesPlant,
        CrimsonDoor, CrimsonDoorLast = CrimsonDoor + 15,
        WarpedDoor, WarpedDoorLast = WarpedDoor + 15,
        CrimsonTrapdoor, CrimsonTrapdoorLast = CrimsonTrapdoor + 15,
        WarpedTrapdoor, WarpedTrapdoorLast = WarpedTrapdoor + 15,
        CrimsonSign, CrimsonSignLast = CrimsonSign + 3,
        WarpedSign, WarpedSignLast = WarpedSign + 3,
        StoneBricks, MossyStoneBricks, CrackedStoneBricks, ChiseledStoneBricks, SmoothStone, SmoothStoneSlab, StoneBrickSlab, PolishedGranite, PolishedDiorite, PolishedAndesite, CobbledDeepslate, PolishedDeepslate, ChiseledDeepslate, CrackedDeepslateBricks, CrackedDeepslateTiles, CobbledDeepslateSlab, PolishedTuff, ChiseledTuff, ChiseledTuffBricks, TuffSlab, TuffBrickSlab, PolishedBlackstone, ChiseledPolishedBlackstone, CrackedPolishedBlackstoneBricks, PolishedBlackstoneSlab, RedNetherBricks, CrackedNetherBricks, ChiseledNetherBricks, NetherBrickSlab,
        NetherQuartzOre, QuartzBlock, QuartzBricks, QuartzPillar, ChiseledQuartzBlock, QuartzSlab,
        RedSand, RedSandstone, CutRedSandstone, ChiseledRedSandstone, SmoothRedSandstone, RedSandstoneSlab, CutSandstone, ChiseledSandstone, SmoothSandstone,
        DeepslateCoalOre, DeepslateIronOre, DeepslateCopperOre, DeepslateGoldOre, DeepslateRedstoneOre, DeepslateEmeraldOre, DeepslateLapisOre, DeepslateDiamondOre, NetherGoldOre, RawIronBlock, RawGoldBlock, RawCopperBlock,
        ConcreteFirst, ConcreteLast = ConcreteFirst + 15,
        ConcretePowderFirst, ConcretePowderLast = ConcretePowderFirst + 15,
        Terracotta,
        TerracottaFirst, TerracottaLast = TerracottaFirst + 15,
        GlazedTerracottaFirst, GlazedTerracottaLast = GlazedTerracottaFirst + 15,
        StainedGlassFirst, StainedGlassLast = StainedGlassFirst + 15,
        GlassPane,
        StainedGlassPaneFirst, StainedGlassPaneLast = StainedGlassPaneFirst + 15,
        TintedGlass,
        PlainCandleFirst, PlainCandleLast = PlainCandleFirst + 5,
        DyedCandleFirst, DyedCandleLast = DyedCandleFirst + 127,
        CandleCakeFirst, CandleCakeLast = CandleCakeFirst + 33,
        ShulkerBoxFirst, ShulkerBoxLast = ShulkerBoxFirst + 15,
        BedFirst, BedLast = BedFirst + 127,
        ChestRunFirst, ChestRunLast = ChestRunFirst + 38,
        WaxedCopperFirst, WaxedCopperLast = WaxedCopperFirst + 3,
        CutCopperFirst, CutCopperLast = CutCopperFirst + 7,
        ChiseledCopperFirst, ChiseledCopperLast = ChiseledCopperFirst + 7,
        CopperGrateFirst, CopperGrateLast = CopperGrateFirst + 7,
        CutCopperSlabFirst, CutCopperSlabLast = CutCopperSlabFirst + 7,
        CopperBulbRunFirst, CopperBulbRunLast = CopperBulbRunFirst + 27,
        CopperDoorFirst, CopperDoorLast = CopperDoorFirst + 127,
        CopperTrapdoorFirst, CopperTrapdoorLast = CopperTrapdoorFirst + 127,
        LightningRodFirst, LightningRodLast = LightningRodFirst + 11,
        Allium, AzureBluet, BlueOrchid, Cornflower, LilyOfTheValley, OxeyeDaisy,
        RedTulip, OrangeTulip, WhiteTulip, PinkTulip, WitherRose, Torchflower,
        Fern, DeadBush,
        TorchflowerCrop0, TorchflowerCrop1,
        DoublePlantFirst, DoublePlantLast = DoublePlantFirst + 13,
        PitcherCropFirst, PitcherCropLast = PitcherCropFirst + 6,
        FlowerPot,
        PottedFirst, PottedLast = PottedFirst + 31,
        BrownMushroom, RedMushroom, BrownMushroomBlock, RedMushroomBlock, MushroomStem,
        Mycelium, Podzol, CoarseDirt, RootedDirt, HangingRoots,
        CaveVines, CaveVinesLit, CaveVinesPlant,
        SporeBlossom,
        BigDripleafStemFirst, BigDripleafStemLast = BigDripleafStemFirst + 3,
        BigDripleafFirst, BigDripleafLast = BigDripleafFirst + 15,
        SmallDripleafFirst, SmallDripleafLast = SmallDripleafFirst + 7,
        Azalea, FloweringAzalea, FloweringAzaleaLeaves,
        GlowLichenFirst, GlowLichenLast = GlowLichenFirst + 5,
        MossCarpet,
        PottedAzalea, PottedFloweringAzalea,
        PumpkinStem0, PumpkinStem1, PumpkinStem2, PumpkinStem3, PumpkinStem4, PumpkinStem5, PumpkinStem6, PumpkinStem7,
        AttachedMelonStemFirst, AttachedMelonStemLast = AttachedMelonStemFirst + 3,
        AttachedPumpkinStemFirst, AttachedPumpkinStemLast = AttachedPumpkinStemFirst + 3,
        CarvedPumpkinFirst, CarvedPumpkinLast = CarvedPumpkinFirst + 3,
        JackOLanternFirst, JackOLanternLast = JackOLanternFirst + 3,
        HayBale, BoneBlock, DriedKelpBlock,
        SweetBerryBush0, SweetBerryBush1, SweetBerryBush2, SweetBerryBush3,
        BeeNestFirst, BeeNestLast = BeeNestFirst + 23,
        BeehiveFirst, BeehiveLast = BeehiveFirst + 23,
        HoneyBlock, HoneycombBlock,
        TallSeagrassLower, TallSeagrassUpper,
        CoralFanFirst, CoralFanLast = CoralFanFirst + 4,
        CoralWallFanFirst, CoralWallFanLast = CoralWallFanFirst + 19,
        DeadCoralBlockFirst, DeadCoralBlockLast = DeadCoralBlockFirst + 4,
        DeadCoralFirst, DeadCoralLast = DeadCoralFirst + 4,
        DeadCoralFanFirst, DeadCoralFanLast = DeadCoralFanFirst + 4,
        DeadCoralWallFanFirst, DeadCoralWallFanLast = DeadCoralWallFanFirst + 19,
        Conduit,
        TurtleEggFirst, TurtleEggLast = TurtleEggFirst + 11,
        PackedIce, BlueIce,
        FrostedIce0, FrostedIce1, FrostedIce2, FrostedIce3,
        PowderSnow,
        RepeaterFirst, RepeaterLast = RepeaterFirst + 31,
        ComparatorFirst, ComparatorLast = ComparatorFirst + 15,
        DaylightDetector, DaylightDetectorLit, DaylightDetectorInverted, DaylightDetectorInvertedLit,
        Target, TargetLit,
        TripwireHookFirst, TripwireHookLast = TripwireHookFirst + 15,
        Tripwire, TripwireActive,
        PoweredRailFirst, PoweredRailLast = PoweredRailFirst + 3,
        DetectorRailFirst, DetectorRailLast = DetectorRailFirst + 3,
        ActivatorRailFirst, ActivatorRailLast = ActivatorRailFirst + 3,
        IronDoorFirst, IronDoorLast = IronDoorFirst + 15,
        IronTrapdoorFirst, IronTrapdoorLast = IronTrapdoorFirst + 15,
        IronBars,
        ChainFirst, ChainLast = ChainFirst + 2,
        EndRodFirst, EndRodLast = EndRodFirst + 5,
        Cobweb,
        SoulTorch, SoulCampfire, SoulSoil,
        Fire0, Fire1, Fire2, Fire3, SoulFire,
        MagmaBlock, CryingObsidian, Lodestone,
        RespawnAnchor0, RespawnAnchor1, RespawnAnchor2, RespawnAnchor3, RespawnAnchor4,
        BellFirst, BellLast = BellFirst + 3,
        Jukebox,
        ChiseledBookshelfFirst, ChiseledBookshelfLast = ChiseledBookshelfFirst + 27,
        DragonEgg, HeavyCore,
        SuspiciousSandFirst, SuspiciousSandLast = SuspiciousSandFirst + 11,
        SuspiciousGravelFirst, SuspiciousGravelLast = SuspiciousGravelFirst + 11,
        DecoratedPotFirst, DecoratedPotLast = DecoratedPotFirst + 3,
        SnifferEgg0, SnifferEgg1, SnifferEgg2,
        Stonecutter, FletchingTable, SmithingTable, CartographyTable,
        LoomFirst, LoomLast = LoomFirst + 3,
        BannerFirst, BannerLast = BannerFirst + 127,
        SculkVeinFirst, SculkVeinLast = SculkVeinFirst + 5,
        CalibratedSensorFirst, CalibratedSensorLast = CalibratedSensorFirst + 7,
        CommandBlockFirst, CommandBlockLast = CommandBlockFirst + 35,
        StructureBlockFirst, StructureBlockLast = StructureBlockFirst + 3,
        JigsawFirst, JigsawLast = JigsawFirst + 5,
        Barrier,
        LightFirst, LightLast = LightFirst + 15,
        StructureVoid,
        Frogspawn,
        GlowFrameNorth, GlowFrameEast, GlowFrameSouth, GlowFrameWest,
        StandingSignFirst, StandingSignLast = StandingSignFirst + 175,
        HangingSignFirst, HangingSignLast = HangingSignFirst + 175,
        WallHangingSignFirst, WallHangingSignLast = WallHangingSignFirst + 43,

        Count,

        ExtFirst = 8192,
        StairsRunFirst = ExtFirst, StairsRunLast = StairsRunFirst + 415,
        StairsTopFirst, StairsTopLast = StairsTopFirst + 15,
        SlabRunFirst, SlabRunLast = SlabRunFirst + 113,
        SlabExtraFirst, SlabExtraLast = SlabExtraFirst + 43,
        FenceRunFirst, FenceRunLast = FenceRunFirst + 10,
        FenceGateFirst, FenceGateLast = FenceGateFirst + 87,
        WallRunFirst, WallRunLast = WallRunFirst + 24,
        SmoothQuartz,
        CarpetFirst, CarpetLast = CarpetFirst + 15,
        WoodBlockFirst, WoodBlockLast = WoodBlockFirst + 19,
        InfestedFirst, InfestedLast = InfestedFirst + 6,
        PolishedBasalt,
        ButtonRunFirst, ButtonRunLast = ButtonRunFirst + 311,
        PlateRunFirst, PlateRunLast = PlateRunFirst + 29,
        WallTorchFirst, WallTorchLast = WallTorchFirst + 15,
        LeverRunFirst, LeverRunLast = LeverRunFirst + 23,
        SnowLayerFirst, SnowLayerLast = SnowLayerFirst + 7,
        LavaCauldron,
        PowderCauldronFirst, PowderCauldronLast = PowderCauldronFirst + 2,
        HeadRunFirst, HeadRunLast = HeadRunFirst + 139,
        BambooShoot,
        AxisRunFirst, AxisRunLast = AxisRunFirst + 65,
        FarmlandDry,
        CampfireUnlit, SoulCampfireUnlit,
        GlazedTurnFirst, GlazedTurnLast = GlazedTurnFirst + 47,
        LanternHanging, SoulLanternHanging,
        HopperSideFirst, HopperSideLast = HopperSideFirst + 3,
        ThrowerTurnFirst, ThrowerTurnLast = ThrowerTurnFirst + 11,
        ObserverTurnFirst, ObserverTurnLast = ObserverTurnFirst + 11,
        PistonVertFirst, PistonVertLast = PistonVertFirst + 11,
        ExtCount
    };
}

inline bool isBlockId(int id) { return (id >= 0 && id < Blocks::Count) || (id >= Blocks::ExtFirst && id < Blocks::ExtCount); }
inline int nextBlockId(int id) { return id + 1 == Blocks::Count ? Blocks::ExtFirst : id + 1; }

enum class RenderType : uint8_t
{
    None,   // invisible (air)
    Cube,   // standard full block
    Cross,  // two diagonal quads (plants, torch)
    Liquid, // full block, slightly lowered top surface, drawn in the transparent pass
    Slab,   // half a block, sitting on the floor of its own cell
    Ladder, // a flat panel against one wall of its cell, climbable
    Stairs, // a half block with a step on the back half of its top
    Fence,  // a post in the middle of its cell, with arms to its neighbours
    Door,   // a thin panel on one edge of its cell, which swings aside
    Rail,   // a flat panel on the floor of its cell, which carts run on
    Flat,   // the same panel, for anything that is not a rail
    Trapdoor,
    Pane,
    Candle,
    Bed,
    Chest,
    Rod,
    Pot,
    Lush,
    SlabTop,
    Gate,
    Wall,
    Button,
    Plate,
    WallTorch,
    Lever,
    Head
};

struct BlockInfo
{
    const char* name;
    int tileTop;
    int tileBottom;
    int tileSide;
    bool collides;      // stops the player
    bool opaque;        // blocks light and hides the neighbouring face
    RenderType render;
    uint8_t lightEmission; // 0-15
    float hardness;        // seconds to mine; negative = unbreakable

    // The face it points at, for anything that points. Last in the
    // struct and defaulted, so the hundred and fifty rows that face
    // nothing are untouched: -1 means "the side texture", which is
    // what all of them want.
    int tileFront = -1;
    int tileBack = -1;
};

const BlockInfo& blockInfo(BlockId id);

constexpr int STAIRS_MATERIALS = 56;
constexpr int SLAB_OLD = 22;
constexpr int SLAB_MATERIALS = 60;
constexpr int WALL_MATERIALS = 25;
constexpr int STAIRS_OLD = 4;
constexpr int PETRIFIED_SLAB = SLAB_OLD + 12;
int stairsMaterial(BlockId id);
bool stairsUpsideDown(BlockId id);
int stairsFacingOf(BlockId id);
BlockId stairsOf(int material, bool upsideDown, int facing);
BlockId stairsMaterialBase(int material);
int stairsMaterialWood(int material);
int slabMaterial(BlockId id);
int slabHalf(BlockId id);
BlockId slabOf(int material, int half);
BlockId slabMaterialBase(int material);
int slabMaterialWood(int material);
inline bool isSlabBlock(BlockId id) { return slabMaterial(id) >= 0; }
inline bool isTopSlab(BlockId id) { return blockInfo(id).render == RenderType::SlabTop; }
inline bool isFenceGate(BlockId id) { return id >= Blocks::FenceGateFirst && id <= Blocks::FenceGateLast; }
inline int gateWood(BlockId id) { return (id - Blocks::FenceGateFirst) / 8; }
inline bool gateOpen(BlockId id) { return isFenceGate(id) && ((id - Blocks::FenceGateFirst) & 4) != 0; }
inline int gateFacing(BlockId id) { return (id - Blocks::FenceGateFirst) & 3; }
inline BlockId gateOf(int wood, bool open, int facing)
{
    if (wood < 0 || wood > 10) wood = 0;
    return static_cast<BlockId>(Blocks::FenceGateFirst + wood * 8 + (open ? 4 : 0) + (facing & 3));
}
inline bool isWall(BlockId id) { return id >= Blocks::WallRunFirst && id <= Blocks::WallRunLast; }
BlockId wallMaterialBase(int material);
inline int wallMaterial(BlockId id) { return isWall(id) ? id - Blocks::WallRunFirst : -1; }
BlockId fenceOfWood(int wood);
int fenceWood(BlockId id);
BlockId shapeBase(BlockId id);
int shapeWood(BlockId id);
inline bool isShapeFamily(BlockId id) { return shapeBase(id) != 0; }
inline bool isCarpet(BlockId id) { return id >= Blocks::CarpetFirst && id <= Blocks::CarpetLast; }
constexpr int HEAD_KINDS = 7;
inline bool isHead(BlockId id) { return id >= Blocks::HeadRunFirst && id <= Blocks::HeadRunLast; }
inline int headKind(BlockId id) { return isHead(id) ? (id - Blocks::HeadRunFirst) / 20 : id == Blocks::WitherSkull ? 1 : -1; }
inline bool headOnWall(BlockId id) { return isHead(id) && (id - Blocks::HeadRunFirst) % 20 >= 16; }
inline int headRotation(BlockId id) { return isHead(id) && !headOnWall(id) ? (id - Blocks::HeadRunFirst) % 20 : 0; }
inline int headFacing(BlockId id) { return headOnWall(id) ? (id - Blocks::HeadRunFirst) % 20 - 16 : 0; }
inline BlockId headOf(int kind, bool wall, int turn)
{
    return static_cast<BlockId>(Blocks::HeadRunFirst + kind * 20 + (wall ? 16 + (turn & 3) : (turn & 15)));
}
inline bool isWitherSkullBlock(BlockId id) { return id == Blocks::WitherSkull || (isHead(id) && headKind(id) == 1); }
constexpr int AXIS_BLOCKS = 33;
inline bool isFarmland(BlockId id) { return id == Blocks::Farmland || id == Blocks::FarmlandDry; }
BlockId axisBase(BlockId id);
int blockAxis(BlockId id);
bool hasAxis(BlockId id);
BlockId withAxis(BlockId base, int axis);
int glazedTurn(BlockId id);
BlockId glazedOf(BlockId base, int turn);
inline bool isSnowLayer(BlockId id) { return id >= Blocks::SnowLayerFirst && id <= Blocks::SnowLayerLast; }
inline int snowLayers(BlockId id) { return isSnowLayer(id) ? id - Blocks::SnowLayerFirst + 1 : 0; }
inline BlockId snowLayerOf(int layers)
{
    return static_cast<BlockId>(Blocks::SnowLayerFirst + (layers < 1 ? 0 : layers > 8 ? 7 : layers - 1));
}
bool snowCanRestOn(BlockId below);
bool bambooCanRootIn(BlockId ground);
BlockId debugStickTurn(BlockId id, int step);
constexpr int BUTTON_MATERIALS = 13;
constexpr int PLATE_MATERIALS = 15;
inline bool isFamilyButton(BlockId id) { return id >= Blocks::ButtonRunFirst && id <= Blocks::ButtonRunLast; }
inline int buttonMaterial(BlockId id) { return isFamilyButton(id) ? (id - Blocks::ButtonRunFirst) / 24 : 0; }
inline int buttonFace(BlockId id) { return isFamilyButton(id) ? ((id - Blocks::ButtonRunFirst) % 24) / 8 : 0; }
inline int buttonFacing(BlockId id) { return isFamilyButton(id) ? ((id - Blocks::ButtonRunFirst) % 8) / 2 : 0; }
inline bool buttonPowered(BlockId id) { return isFamilyButton(id) ? ((id - Blocks::ButtonRunFirst) & 1) != 0 : id == Blocks::ButtonOn; }
inline BlockId buttonOf(int material, int face, int facing, bool powered)
{
    if (material < 0 || material >= BUTTON_MATERIALS) material = 0;
    if (face < 0 || face > 2) face = 0;
    return static_cast<BlockId>(Blocks::ButtonRunFirst + material * 24 + face * 8 + (facing & 3) * 2 + (powered ? 1 : 0));
}
inline bool isAnyButton(BlockId id) { return isFamilyButton(id) || id == Blocks::ButtonOff || id == Blocks::ButtonOn; }
inline float buttonSeconds(BlockId id) { return buttonMaterial(id) >= 2 ? 1.5f : 1.0f; }
inline BlockId buttonItem(int material)
{
    return material <= 0 ? static_cast<BlockId>(Blocks::ButtonOff) : buttonOf(material, 0, 0, false);
}
inline bool isPlate(BlockId id) { return id >= Blocks::PlateRunFirst && id <= Blocks::PlateRunLast; }
inline int plateMaterial(BlockId id) { return isPlate(id) ? (id - Blocks::PlateRunFirst) / 2 : -1; }
inline bool platePowered(BlockId id) { return isPlate(id) && ((id - Blocks::PlateRunFirst) & 1) != 0; }
inline BlockId plateOf(int material, bool powered)
{
    if (material < 0 || material >= PLATE_MATERIALS) material = 0;
    return static_cast<BlockId>(Blocks::PlateRunFirst + material * 2 + (powered ? 1 : 0));
}
inline bool plateFeelsItems(BlockId id) { return plateMaterial(id) >= 2; }
inline bool isWallTorch(BlockId id) { return id >= Blocks::WallTorchFirst && id <= Blocks::WallTorchLast; }
inline int wallTorchKind(BlockId id) { return (id - Blocks::WallTorchFirst) / 4; }
inline int wallTorchFacing(BlockId id) { return (id - Blocks::WallTorchFirst) & 3; }
inline BlockId wallTorchOf(int kind, int facing)
{
    return static_cast<BlockId>(Blocks::WallTorchFirst + (kind & 3) * 4 + (facing & 3));
}
inline bool isFamilyLever(BlockId id) { return id >= Blocks::LeverRunFirst && id <= Blocks::LeverRunLast; }
inline bool isLeverBlock(BlockId id) { return isFamilyLever(id) || id == Blocks::LeverOff || id == Blocks::LeverOn; }
inline int leverFace(BlockId id) { return isFamilyLever(id) ? (id - Blocks::LeverRunFirst) / 8 : 0; }
inline int leverFacing(BlockId id) { return isFamilyLever(id) ? ((id - Blocks::LeverRunFirst) % 8) / 2 : 0; }
inline bool leverOn(BlockId id) { return isFamilyLever(id) ? ((id - Blocks::LeverRunFirst) & 1) != 0 : id == Blocks::LeverOn; }
inline BlockId leverOf(int face, int facing, bool on)
{
    if (face < 0 || face > 2) face = 0;
    return static_cast<BlockId>(Blocks::LeverRunFirst + face * 8 + (facing & 3) * 2 + (on ? 1 : 0));
}
inline int supportFromFacing(int facing)
{
    static const int SUPPORT[4] = { 4, 5, 2, 3 };
    return SUPPORT[facing & 3];
}
int supportDirection(BlockId id);
BlockId buttonMaterialBase(int material);
BlockId plateMaterialBase(int material);
inline bool isWoodBlock(BlockId id) { return id >= Blocks::WoodBlockFirst && id <= Blocks::WoodBlockLast; }
inline bool isStrippedWoodBlock(BlockId id) { return isWoodBlock(id) && id - Blocks::WoodBlockFirst >= 10; }
int woodBlockWood(BlockId id);
BlockId woodBlockOf(int wood, bool stripped);
inline bool isInfested(BlockId id) { return id >= Blocks::InfestedFirst && id <= Blocks::InfestedLast; }
BlockId infestedBase(BlockId id);
BlockId infestedOf(BlockId base);
inline BlockId shaderStandIn(BlockId id)
{
    for (int hop = 0; hop < 3 && id >= Blocks::Count; ++hop)
    {
        if (id == Blocks::SmoothQuartz) return Blocks::QuartzBlock;
        const BlockId base = shapeBase(id);
        if (base == 0) return Blocks::Stone;
        id = base;
    }
    return id >= Blocks::Count ? static_cast<BlockId>(Blocks::Stone) : id;
}

inline bool isAir(BlockId id) { return id == Blocks::Air; }
inline bool isOpaque(BlockId id) { return blockInfo(id).opaque; }
inline bool isSolid(BlockId id) { return blockInfo(id).collides; }
inline bool isLiquid(BlockId id) { return blockInfo(id).render == RenderType::Liquid; }
inline bool isWater(BlockId id) { return id == Blocks::Water || (id >= Blocks::WaterFlow1 && id <= Blocks::WaterFalling); }
inline bool isFrostedIce(BlockId id) { return id >= Blocks::FrostedIce0 && id <= Blocks::FrostedIce3; }
inline bool isRepeater(BlockId id) { return id >= Blocks::RepeaterFirst && id <= Blocks::RepeaterLast; }
inline int repeaterOut(BlockId id) { return (id - Blocks::RepeaterFirst) / 8; }
inline int repeaterDelay(BlockId id) { return ((id - Blocks::RepeaterFirst) % 8) / 2 + 1; }
inline bool repeaterLit(BlockId id) { return isRepeater(id) && ((id - Blocks::RepeaterFirst) & 1) != 0; }
inline BlockId repeaterOf(int out, int delay, bool lit)
{
    return static_cast<BlockId>(Blocks::RepeaterFirst + (out & 3) * 8 + ((delay < 1 ? 1 : delay > 4 ? 4 : delay) - 1) * 2 + (lit ? 1 : 0));
}
inline bool isComparator(BlockId id) { return id >= Blocks::ComparatorFirst && id <= Blocks::ComparatorLast; }
inline int comparatorOut(BlockId id) { return (id - Blocks::ComparatorFirst) / 4; }
inline bool comparatorSubtracts(BlockId id) { return isComparator(id) && ((id - Blocks::ComparatorFirst) & 2) != 0; }
inline bool comparatorLit(BlockId id) { return isComparator(id) && ((id - Blocks::ComparatorFirst) & 1) != 0; }
inline BlockId comparatorOf(int out, bool subtract, bool lit)
{
    return static_cast<BlockId>(Blocks::ComparatorFirst + (out & 3) * 4 + (subtract ? 2 : 0) + (lit ? 1 : 0));
}
inline bool isDiode(BlockId id) { return isRepeater(id) || isComparator(id); }
inline int diodeOut(BlockId id) { return isRepeater(id) ? repeaterOut(id) : comparatorOut(id); }
inline bool diodeLit(BlockId id) { return repeaterLit(id) || comparatorLit(id); }
inline bool isDaylightDetector(BlockId id) { return id >= Blocks::DaylightDetector && id <= Blocks::DaylightDetectorInvertedLit; }
inline bool daylightInverted(BlockId id) { return id == Blocks::DaylightDetectorInverted || id == Blocks::DaylightDetectorInvertedLit; }
inline bool daylightLit(BlockId id) { return id == Blocks::DaylightDetectorLit || id == Blocks::DaylightDetectorInvertedLit; }
inline BlockId daylightOf(bool inverted, bool lit)
{
    return static_cast<BlockId>(Blocks::DaylightDetector + (inverted ? 2 : 0) + (lit ? 1 : 0));
}
inline bool isTripwireHook(BlockId id) { return id >= Blocks::TripwireHookFirst && id <= Blocks::TripwireHookLast; }
inline int hookFacing(BlockId id) { return (id - Blocks::TripwireHookFirst) / 4; }
inline bool hookAttached(BlockId id) { return isTripwireHook(id) && ((id - Blocks::TripwireHookFirst) & 2) != 0; }
inline bool hookPowered(BlockId id) { return isTripwireHook(id) && ((id - Blocks::TripwireHookFirst) & 1) != 0; }
inline BlockId hookOf(int facing, bool attached, bool powered)
{
    return static_cast<BlockId>(Blocks::TripwireHookFirst + (facing & 3) * 4 + (attached ? 2 : 0) + (powered ? 1 : 0));
}
inline bool isTripwire(BlockId id) { return id == Blocks::Tripwire || id == Blocks::TripwireActive; }
inline int specialRailFamily(BlockId id)
{
    if (id >= Blocks::PoweredRailFirst && id <= Blocks::PoweredRailLast) return 1;
    if (id >= Blocks::DetectorRailFirst && id <= Blocks::DetectorRailLast) return 2;
    if (id >= Blocks::ActivatorRailFirst && id <= Blocks::ActivatorRailLast) return 3;
    return 0;
}
inline bool isSpecialRail(BlockId id) { return specialRailFamily(id) != 0; }
inline bool specialRailEastWest(BlockId id)
{
    static const BlockId FIRST[4] = { 0, Blocks::PoweredRailFirst, Blocks::DetectorRailFirst, Blocks::ActivatorRailFirst };
    return isSpecialRail(id) && ((id - FIRST[specialRailFamily(id)]) & 2) != 0;
}
inline bool specialRailPowered(BlockId id)
{
    static const BlockId FIRST[4] = { 0, Blocks::PoweredRailFirst, Blocks::DetectorRailFirst, Blocks::ActivatorRailFirst };
    return isSpecialRail(id) && ((id - FIRST[specialRailFamily(id)]) & 1) != 0;
}
inline BlockId specialRailOf(int family, bool eastWest, bool powered)
{
    static const BlockId FIRST[4] = { Blocks::PoweredRailFirst, Blocks::PoweredRailFirst, Blocks::DetectorRailFirst, Blocks::ActivatorRailFirst };
    return static_cast<BlockId>(FIRST[family & 3] + (eastWest ? 2 : 0) + (powered ? 1 : 0));
}
inline bool isChain(BlockId id) { return id >= Blocks::ChainFirst && id <= Blocks::ChainLast; }
inline int chainAxis(BlockId id) { return isChain(id) ? id - Blocks::ChainFirst : 0; }
inline bool isEndRod(BlockId id) { return id >= Blocks::EndRodFirst && id <= Blocks::EndRodLast; }
inline int endRodFacing(BlockId id) { return isEndRod(id) ? id - Blocks::EndRodFirst : 0; }
inline bool isFire(BlockId id) { return (id >= Blocks::Fire0 && id <= Blocks::Fire3) || id == Blocks::SoulFire; }
inline bool isCampfire(BlockId id) { return id == Blocks::Campfire || id == Blocks::SoulCampfire; }
bool isAnyCampfire(BlockId id);
inline bool isLantern(BlockId id)
{
    return id == Blocks::Lantern || id == Blocks::SoulLantern || id == Blocks::LanternHanging || id == Blocks::SoulLanternHanging;
}
inline bool lanternHangs(BlockId id) { return id == Blocks::LanternHanging || id == Blocks::SoulLanternHanging; }
inline bool isDispenser(BlockId id) { return id == Blocks::Dispenser || (id >= Blocks::ThrowerTurnFirst && id < Blocks::ThrowerTurnFirst + 6); }
inline bool isDropper(BlockId id) { return id == Blocks::Dropper || (id >= Blocks::ThrowerTurnFirst + 6 && id <= Blocks::ThrowerTurnLast); }
inline bool isObserver(BlockId id)
{
    return id == Blocks::Observer || id == Blocks::ObserverLit || (id >= Blocks::ObserverTurnFirst && id <= Blocks::ObserverTurnLast);
}
inline bool observerLit(BlockId id) { return id == Blocks::ObserverLit || (id >= Blocks::ObserverTurnFirst + 6 && id <= Blocks::ObserverTurnLast); }
inline int sixFacing(BlockId id)
{
    if (id >= Blocks::ThrowerTurnFirst && id <= Blocks::ThrowerTurnLast) return (id - Blocks::ThrowerTurnFirst) % 6;
    if (id >= Blocks::ObserverTurnFirst && id <= Blocks::ObserverTurnLast) return (id - Blocks::ObserverTurnFirst) % 6;
    return 4;
}
inline BlockId dispenserFacingTo(int facing) { return static_cast<BlockId>(Blocks::ThrowerTurnFirst + ((facing % 6) + 6) % 6); }
inline BlockId dropperFacingTo(int facing) { return static_cast<BlockId>(Blocks::ThrowerTurnFirst + 6 + ((facing % 6) + 6) % 6); }
inline BlockId observerFacingTo(int facing, bool lit) { return static_cast<BlockId>(Blocks::ObserverTurnFirst + (lit ? 6 : 0) + ((facing % 6) + 6) % 6); }
inline bool isHopper(BlockId id) { return id == Blocks::Hopper || (id >= Blocks::HopperSideFirst && id <= Blocks::HopperSideLast); }
inline int hopperFacing(BlockId id) { return id >= Blocks::HopperSideFirst && id <= Blocks::HopperSideLast ? id - Blocks::HopperSideFirst : -1; }
inline BlockId hopperFacingTo(int facing) { return facing < 0 ? Blocks::Hopper : static_cast<BlockId>(Blocks::HopperSideFirst + (facing & 3)); }
bool isSoulCampfire(BlockId id);
inline bool isRespawnAnchor(BlockId id) { return id >= Blocks::RespawnAnchor0 && id <= Blocks::RespawnAnchor4; }
inline int anchorCharge(BlockId id) { return isRespawnAnchor(id) ? id - Blocks::RespawnAnchor0 : 0; }
inline BlockId anchorOf(int charge) { return static_cast<BlockId>(Blocks::RespawnAnchor0 + (charge < 0 ? 0 : charge > 4 ? 4 : charge)); }
inline bool isBell(BlockId id) { return id >= Blocks::BellFirst && id <= Blocks::BellLast; }
inline bool isChiseledBookshelf(BlockId id) { return id >= Blocks::ChiseledBookshelfFirst && id <= Blocks::ChiseledBookshelfLast; }
inline int shelfBooks(BlockId id) { return isChiseledBookshelf(id) ? (id - Blocks::ChiseledBookshelfFirst) % 7 : 0; }
inline int shelfFacing(BlockId id) { return isChiseledBookshelf(id) ? (id - Blocks::ChiseledBookshelfFirst) / 7 : 0; }
inline BlockId shelfOf(int facing, int books)
{
    return static_cast<BlockId>(Blocks::ChiseledBookshelfFirst + (facing & 3) * 7 + (books < 0 ? 0 : books > 6 ? 6 : books));
}
inline bool isSuspiciousSand(BlockId id) { return id >= Blocks::SuspiciousSandFirst && id <= Blocks::SuspiciousSandLast; }
inline bool isSuspiciousGravel(BlockId id) { return id >= Blocks::SuspiciousGravelFirst && id <= Blocks::SuspiciousGravelLast; }
inline bool isSuspicious(BlockId id) { return isSuspiciousSand(id) || isSuspiciousGravel(id); }
inline int dustStage(BlockId id)
{
    if (isSuspiciousSand(id)) return (id - Blocks::SuspiciousSandFirst) % 4;
    if (isSuspiciousGravel(id)) return (id - Blocks::SuspiciousGravelFirst) % 4;
    return 0;
}
inline int dustTable(BlockId id)
{
    if (isSuspiciousSand(id)) return (id - Blocks::SuspiciousSandFirst) / 4;
    if (isSuspiciousGravel(id)) return 3 + (id - Blocks::SuspiciousGravelFirst) / 4;
    return 0;
}
inline BlockId suspiciousOf(int table, int stage)
{
    if (stage < 0) stage = 0;
    if (stage > 3) stage = 3;
    if (table < 3) return static_cast<BlockId>(Blocks::SuspiciousSandFirst + table * 4 + stage);
    return static_cast<BlockId>(Blocks::SuspiciousGravelFirst + (table - 3) * 4 + stage);
}
inline bool isSnifferEgg(BlockId id) { return id >= Blocks::SnifferEgg0 && id <= Blocks::SnifferEgg2; }
inline bool isDecoratedPot(BlockId id) { return id >= Blocks::DecoratedPotFirst && id <= Blocks::DecoratedPotLast; }
inline bool isLoom(BlockId id) { return id >= Blocks::LoomFirst && id <= Blocks::LoomLast; }
inline bool isSculkVein(BlockId id) { return id >= Blocks::SculkVeinFirst && id <= Blocks::SculkVeinLast; }
inline int sculkVeinFace(BlockId id) { return isSculkVein(id) ? id - Blocks::SculkVeinFirst : 0; }
inline BlockId sculkVeinOn(int face) { return static_cast<BlockId>(Blocks::SculkVeinFirst + (face < 0 || face > 5 ? 0 : face)); }
inline bool isCalibratedSensor(BlockId id) { return id >= Blocks::CalibratedSensorFirst && id <= Blocks::CalibratedSensorLast; }
inline bool calibratedLit(BlockId id) { return isCalibratedSensor(id) && ((id - Blocks::CalibratedSensorFirst) & 1) != 0; }
inline int calibratedFacing(BlockId id) { return isCalibratedSensor(id) ? (id - Blocks::CalibratedSensorFirst) >> 1 : 0; }
inline BlockId calibratedOf(int facing, bool lit)
{
    return static_cast<BlockId>(Blocks::CalibratedSensorFirst + (facing & 3) * 2 + (lit ? 1 : 0));
}
inline bool isAnySculkSensor(BlockId id)
{
    return id == Blocks::SculkSensor || id == Blocks::SculkSensorLit || isCalibratedSensor(id);
}
inline bool sculkSensorLit(BlockId id) { return id == Blocks::SculkSensorLit || calibratedLit(id); }
inline bool isCommandBlock(BlockId id) { return id >= Blocks::CommandBlockFirst && id <= Blocks::CommandBlockLast; }
inline int commandKind(BlockId id) { return isCommandBlock(id) ? (id - Blocks::CommandBlockFirst) / 12 : 0; }
inline int commandFacing(BlockId id) { return isCommandBlock(id) ? ((id - Blocks::CommandBlockFirst) % 12) / 2 : 0; }
inline bool commandConditional(BlockId id) { return isCommandBlock(id) && ((id - Blocks::CommandBlockFirst) & 1) != 0; }
inline BlockId commandBlockOf(int kind, int facing, bool conditional)
{
    kind = kind < 0 ? 0 : kind > 2 ? 2 : kind;
    facing = facing < 0 ? 0 : facing > 5 ? 5 : facing;
    return static_cast<BlockId>(Blocks::CommandBlockFirst + kind * 12 + facing * 2 + (conditional ? 1 : 0));
}
inline bool isStructureBlock(BlockId id) { return id >= Blocks::StructureBlockFirst && id <= Blocks::StructureBlockLast; }
inline int structureMode(BlockId id) { return isStructureBlock(id) ? id - Blocks::StructureBlockFirst : 0; }
inline bool isJigsaw(BlockId id) { return id >= Blocks::JigsawFirst && id <= Blocks::JigsawLast; }
inline int jigsawFacing(BlockId id) { return isJigsaw(id) ? id - Blocks::JigsawFirst : 0; }
inline bool isLightBlock(BlockId id) { return id >= Blocks::LightFirst && id <= Blocks::LightLast; }
inline int lightBlockLevel(BlockId id) { return isLightBlock(id) ? id - Blocks::LightFirst : 0; }
inline BlockId lightBlockOf(int level) { return static_cast<BlockId>(Blocks::LightFirst + (level < 0 ? 0 : level > 15 ? 15 : level)); }
inline bool isCreativeOnly(BlockId id)
{
    return isCommandBlock(id) || isStructureBlock(id) || isJigsaw(id) || isLightBlock(id) ||
           id == Blocks::Barrier || id == Blocks::StructureVoid;
}
inline bool isBanner(BlockId id) { return id >= Blocks::BannerFirst && id <= Blocks::BannerLast; }
inline int bannerColour(BlockId id) { return isBanner(id) ? (id - Blocks::BannerFirst) / 8 : 0; }
inline bool bannerOnWall(BlockId id) { return isBanner(id) && ((id - Blocks::BannerFirst) & 4) != 0; }
inline int bannerFacing(BlockId id) { return isBanner(id) ? (id - Blocks::BannerFirst) & 3 : 0; }
inline BlockId bannerOf(int colour, bool wall, int facing)
{
    colour = colour < 0 ? 0 : colour > 15 ? 15 : colour;
    return static_cast<BlockId>(Blocks::BannerFirst + colour * 8 + (wall ? 4 : 0) + (facing & 3));
}
inline bool isTarget(BlockId id) { return id == Blocks::Target || id == Blocks::TargetLit; }
inline bool isIronDoor(BlockId id) { return id >= Blocks::IronDoorFirst && id <= Blocks::IronDoorLast; }
inline bool isIronTrapdoor(BlockId id) { return id >= Blocks::IronTrapdoorFirst && id <= Blocks::IronTrapdoorLast; }
inline bool isSlippery(BlockId id) { return id == Blocks::Ice || id == Blocks::PackedIce || id == Blocks::BlueIce || isFrostedIce(id); }
inline bool isLava(BlockId id) { return id == Blocks::Lava || (id >= Blocks::LavaFlow1 && id <= Blocks::LavaFalling); }
inline bool isFluid(BlockId id) { return isWater(id) || isLava(id); }
inline bool isFluidSource(BlockId id) { return id == Blocks::Water || id == Blocks::Lava; }
inline bool isFluidFalling(BlockId id) { return id == Blocks::WaterFalling || id == Blocks::LavaFalling; }
inline BlockId fluidBase(BlockId id)
{
    if (isWater(id)) return Blocks::Water;
    if (isLava(id)) return Blocks::Lava;
    return Blocks::Air;
}
inline int fluidAmount(BlockId id)
{
    if (isFluidSource(id) || isFluidFalling(id)) return 8;
    if (id >= Blocks::WaterFlow1 && id <= Blocks::WaterFlow7) return id - Blocks::WaterFlow1 + 1;
    if (id >= Blocks::LavaFlow1 && id <= Blocks::LavaFlow7) return id - Blocks::LavaFlow1 + 1;
    return 0;
}
inline BlockId flowingFluid(BlockId base, int amount, bool falling)
{
    const bool lava = base == Blocks::Lava;
    if (falling) return lava ? Blocks::LavaFalling : Blocks::WaterFalling;
    if (amount <= 0) return Blocks::Air;
    if (amount > 7) amount = 7;
    return static_cast<BlockId>((lava ? Blocks::LavaFlow1 : Blocks::WaterFlow1) + amount - 1);
}
inline bool isCross(BlockId id) { return blockInfo(id).render == RenderType::Cross; }
inline bool isSlab(BlockId id) { return blockInfo(id).render == RenderType::Slab; }
// Anything drawn as a flat panel against one wall of its cell: the
// ladders, the signs and the item frames. The mesher asks this; the
// rest of the game wants to know which of the three it is.
inline bool isWallPanel(BlockId id)
{
    return blockInfo(id).render == RenderType::Ladder;
}

// A ladder, and only a ladder. This used to ask the render type, and
// the moment a sign was drawn as a panel too every sign in the world
// became something to climb -- and asked the ladder arithmetic which
// way it faced, which put every one of them on the same wall.
inline bool isLadder(BlockId id)
{
    return id >= Blocks::LadderNorth && id <= Blocks::LadderWest;
}

// A sign, and which way it faces. The facings are in the ladder's
// order, so the same arithmetic places both.
constexpr int WOOD_COUNT = 11;

struct WoodSet
{
    BlockId log, stripped, planks, leaves, sapling, door, trapdoor, sign;
};

inline const WoodSet& woodSet(int wood)
{
    static const WoodSet SETS[WOOD_COUNT] = {
        { Blocks::Log, Blocks::StrippedOakLog, Blocks::Planks, Blocks::Leaves, Blocks::OakSapling,
          Blocks::DoorLowShutNorth, Blocks::OakTrapdoor, Blocks::SignNorth },
        { Blocks::SpruceLog, Blocks::StrippedSpruceLog, Blocks::SprucePlanks, Blocks::SpruceLeaves,
          Blocks::SpruceSapling, Blocks::SpruceDoor, Blocks::SpruceTrapdoor, Blocks::SpruceSign },
        { Blocks::BirchLog, Blocks::StrippedBirchLog, Blocks::BirchPlanks, Blocks::BirchLeaves,
          Blocks::BirchSapling, Blocks::BirchDoor, Blocks::BirchTrapdoor, Blocks::BirchSign },
        { Blocks::JungleLog, Blocks::StrippedJungleLog, Blocks::JunglePlanks, Blocks::JungleLeaves,
          Blocks::JungleSapling, Blocks::JungleDoor, Blocks::JungleTrapdoor, Blocks::JungleSign },
        { Blocks::AcaciaLog, Blocks::StrippedAcaciaLog, Blocks::AcaciaPlanks, Blocks::AcaciaLeaves,
          Blocks::AcaciaSapling, Blocks::AcaciaDoor, Blocks::AcaciaTrapdoor, Blocks::AcaciaSign },
        { Blocks::DarkOakLog, Blocks::StrippedDarkOakLog, Blocks::DarkOakPlanks, Blocks::DarkOakLeaves,
          Blocks::DarkOakSapling, Blocks::DarkOakDoor, Blocks::DarkOakTrapdoor, Blocks::DarkOakSign },
        { Blocks::CherryLog, Blocks::StrippedCherryLog, Blocks::CherryPlanks, Blocks::CherryLeaves,
          Blocks::CherrySapling, Blocks::CherryDoor, Blocks::CherryTrapdoor, Blocks::CherrySign },
        { Blocks::MangroveLog, Blocks::StrippedMangroveLog, Blocks::MangrovePlanks, Blocks::MangroveLeaves,
          Blocks::MangrovePropagule, Blocks::MangroveDoor, Blocks::MangroveTrapdoor, Blocks::MangroveSign },
        { Blocks::BambooBlock, Blocks::StrippedBambooBlock, Blocks::BambooPlanks, Blocks::Air,
          Blocks::Air, Blocks::BambooDoor, Blocks::BambooTrapdoor, Blocks::BambooSign },
        { Blocks::CrimsonStem, Blocks::StrippedCrimsonStem, Blocks::CrimsonPlanks, Blocks::Air,
          Blocks::CrimsonFungus, Blocks::CrimsonDoor, Blocks::CrimsonTrapdoor, Blocks::CrimsonSign },
        { Blocks::WarpedStem, Blocks::StrippedWarpedStem, Blocks::WarpedPlanks, Blocks::Air,
          Blocks::WarpedFungus, Blocks::WarpedDoor, Blocks::WarpedTrapdoor, Blocks::WarpedSign },
    };
    return SETS[wood < 0 || wood >= WOOD_COUNT ? 0 : wood];
}

inline bool isStandingSign(BlockId id) { return id >= Blocks::StandingSignFirst && id <= Blocks::StandingSignLast; }
inline bool isCeilingHangingSign(BlockId id) { return id >= Blocks::HangingSignFirst && id <= Blocks::HangingSignLast; }
inline bool isWallHangingSign(BlockId id) { return id >= Blocks::WallHangingSignFirst && id <= Blocks::WallHangingSignLast; }
inline bool isHangingSign(BlockId id) { return isCeilingHangingSign(id) || isWallHangingSign(id); }
inline int wallSignWood(BlockId id)
{
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (id >= woodSet(wood).sign && id < woodSet(wood).sign + 4) return wood;
    return -1;
}
inline bool isWallSign(BlockId id) { return wallSignWood(id) >= 0; }
inline int signWood(BlockId id)
{
    if (isStandingSign(id)) return (id - Blocks::StandingSignFirst) / 16;
    if (isCeilingHangingSign(id)) return (id - Blocks::HangingSignFirst) / 16;
    if (isWallHangingSign(id)) return (id - Blocks::WallHangingSignFirst) / 4;
    return wallSignWood(id);
}
inline bool isSign(BlockId id) { return signWood(id) >= 0; }
inline BlockId signBaseOf(int wood) { return woodSet(wood).sign; }
inline BlockId signBase(BlockId id) { return signBaseOf(signWood(id)); }
inline int signRotation(BlockId id)
{
    if (isStandingSign(id)) return (id - Blocks::StandingSignFirst) % 16;
    if (isCeilingHangingSign(id)) return (id - Blocks::HangingSignFirst) % 16;
    if (isWallHangingSign(id)) return ((id - Blocks::WallHangingSignFirst) % 4) * 4;
    if (isWallSign(id)) return (id - signBase(id)) * 4;
    return 0;
}
inline int signFacing(BlockId id)
{
    if (isWallSign(id)) return id - signBase(id);
    return isSign(id) ? ((signRotation(id) + 2) / 4) & 3 : 0;
}
inline BlockId standingSignOf(int wood, int rotation)
{
    return static_cast<BlockId>(Blocks::StandingSignFirst + std::clamp(wood, 0, WOOD_COUNT - 1) * 16 + (rotation & 15));
}
inline BlockId hangingSignOf(int wood, int rotation)
{
    return static_cast<BlockId>(Blocks::HangingSignFirst + std::clamp(wood, 0, WOOD_COUNT - 1) * 16 + (rotation & 15));
}
inline BlockId wallHangingSignOf(int wood, int facing)
{
    return static_cast<BlockId>(Blocks::WallHangingSignFirst + std::clamp(wood, 0, WOOD_COUNT - 1) * 4 + (facing & 3));
}
inline BlockId signFacingAs(BlockId base, int facing)
{
    if (facing < 0) facing = 0;
    if (facing > 3) facing = 3;
    return static_cast<BlockId>(base + facing);
}
inline BlockId signFacingAs(int facing) { return signFacingAs(Blocks::SignNorth, facing); }

// An item frame, and which way it faces. The same four as a sign, off
// the same arithmetic.
inline bool isGlowFrame(BlockId id)
{
    return id >= Blocks::GlowFrameNorth && id <= Blocks::GlowFrameWest;
}
inline bool isFrame(BlockId id)
{
    return (id >= Blocks::FrameNorth && id <= Blocks::FrameWest) || isGlowFrame(id);
}
inline int frameFacing(BlockId id)
{
    if (isGlowFrame(id)) return id - Blocks::GlowFrameNorth;
    return isFrame(id) ? id - Blocks::FrameNorth : 0;
}
inline BlockId frameFacingAs(int facing, bool glow = false)
{
    if (facing < 0) facing = 0;
    if (facing > 3) facing = 3;
    return static_cast<BlockId>((glow ? Blocks::GlowFrameNorth : Blocks::FrameNorth) + facing);
}

// What you can go up the outside of. A ladder, and scaffolding, which
// is the only reason to build scaffolding.
inline bool isClimbable(BlockId id)
{
    return isLadder(id) || id == Blocks::Scaffolding || id == Blocks::WeepingVines ||
           id == Blocks::WeepingVinesPlant || id == Blocks::TwistingVines || id == Blocks::TwistingVinesPlant ||
           id == Blocks::Vine || id == Blocks::CaveVines || id == Blocks::CaveVinesLit || id == Blocks::CaveVinesPlant || id == Blocks::GlowBerries;
}
inline bool isStairs(BlockId id) { return blockInfo(id).render == RenderType::Stairs; }
inline bool isFence(BlockId id) { return blockInfo(id).render == RenderType::Fence; }
inline bool isDoor(BlockId id) { return blockInfo(id).render == RenderType::Door; }
inline bool isPane(BlockId id) { return blockInfo(id).render == RenderType::Pane; }
inline bool isRail(BlockId id) { return blockInfo(id).render == RenderType::Rail; }

// Pistons. Everything about one is read off its id: which way it
// points, whether it is sticky, and whether its head is out.
inline bool pistonVertical(BlockId id) { return id >= Blocks::PistonVertFirst && id <= Blocks::PistonVertLast; }

inline bool isPistonBody(BlockId id)
{
    return (id >= Blocks::PistonNorth && id <= Blocks::PistonOutWest) ||
           (id >= Blocks::StickyPistonNorth && id <= Blocks::StickyPistonOutWest) ||
           (pistonVertical(id) && (id - Blocks::PistonVertFirst) % 6 < 4);
}

inline bool isPistonHead(BlockId id)
{
    return (id >= Blocks::PistonHeadNorth && id <= Blocks::PistonHeadWest) ||
           (id >= Blocks::StickyPistonHeadNorth && id <= Blocks::StickyPistonHeadWest) ||
           (pistonVertical(id) && (id - Blocks::PistonVertFirst) % 6 >= 4);
}

inline bool isPiston(BlockId id) { return isPistonBody(id) || isPistonHead(id); }

inline bool pistonSticky(BlockId id)
{
    if (pistonVertical(id)) return id - Blocks::PistonVertFirst >= 6;
    return id >= Blocks::StickyPistonNorth && id <= Blocks::StickyPistonHeadWest;
}

inline bool pistonOut(BlockId id)
{
    if (pistonVertical(id)) return (id - Blocks::PistonVertFirst) % 6 / 2 == 1;
    return (id >= Blocks::PistonOutNorth && id <= Blocks::PistonOutWest) ||
           (id >= Blocks::StickyPistonOutNorth && id <= Blocks::StickyPistonOutWest);
}

inline int pistonFacing(BlockId id)
{
    if (pistonVertical(id)) return 4 + ((id - Blocks::PistonVertFirst) & 1);
    return (id - Blocks::PistonNorth) & 3;
}

inline BlockId pistonBody(int facing, bool sticky, bool out)
{
    if (facing >= 4) return static_cast<BlockId>(Blocks::PistonVertFirst + (sticky ? 6 : 0) + (out ? 2 : 0) + (facing == 5 ? 1 : 0));
    const BlockId first = sticky ? (out ? Blocks::StickyPistonOutNorth
                                        : Blocks::StickyPistonNorth)
                                 : (out ? Blocks::PistonOutNorth : Blocks::PistonNorth);
    return static_cast<BlockId>(first + (facing & 3));
}

inline BlockId pistonHead(int facing, bool sticky)
{
    if (facing >= 4) return static_cast<BlockId>(Blocks::PistonVertFirst + (sticky ? 6 : 0) + 4 + (facing == 5 ? 1 : 0));
    const BlockId first = sticky ? Blocks::StickyPistonHeadNorth : Blocks::PistonHeadNorth;
    return static_cast<BlockId>(first + (facing & 3));
}

inline bool isMonsterSpawner(BlockId id) { return id == Blocks::MonsterSpawner; }

// Whether this is something that grows in the water rather than in the
// air. The mesher needs it so the sea does not draw a face against
// every blade of seagrass in it, which would put a glassy box round
// each one; the player needs it so swimming through kelp does not read
// as coming up for air.
inline bool isTallSeagrass(BlockId id) { return id == Blocks::TallSeagrassLower || id == Blocks::TallSeagrassUpper; }
inline bool isLiveCoralPlant(BlockId id) { return id >= Blocks::TubeCoral && id <= Blocks::HornCoral; }
inline bool isCoralFan(BlockId id) { return id >= Blocks::CoralFanFirst && id <= Blocks::CoralFanLast; }
inline bool isCoralWallFan(BlockId id) { return id >= Blocks::CoralWallFanFirst && id <= Blocks::CoralWallFanLast; }
inline bool isDeadCoralBlock(BlockId id) { return id >= Blocks::DeadCoralBlockFirst && id <= Blocks::DeadCoralBlockLast; }
inline bool isDeadCoralPlant(BlockId id) { return id >= Blocks::DeadCoralFirst && id <= Blocks::DeadCoralLast; }
inline bool isDeadCoralFan(BlockId id) { return id >= Blocks::DeadCoralFanFirst && id <= Blocks::DeadCoralFanLast; }
inline bool isDeadCoralWallFan(BlockId id) { return id >= Blocks::DeadCoralWallFanFirst && id <= Blocks::DeadCoralWallFanLast; }
inline bool isAnyCoralFan(BlockId id) { return isCoralFan(id) || isDeadCoralFan(id); }
inline bool isAnyCoralWallFan(BlockId id) { return isCoralWallFan(id) || isDeadCoralWallFan(id); }
inline int coralWallFanFacing(BlockId id)
{
    if (isCoralWallFan(id)) return (id - Blocks::CoralWallFanFirst) & 3;
    if (isDeadCoralWallFan(id)) return (id - Blocks::DeadCoralWallFanFirst) & 3;
    return 0;
}
inline BlockId coralWallFanOf(bool dead, int kind, int facing)
{
    return static_cast<BlockId>((dead ? Blocks::DeadCoralWallFanFirst : Blocks::CoralWallFanFirst) + kind * 4 + (facing & 3));
}
inline bool isTurtleEgg(BlockId id) { return id >= Blocks::TurtleEggFirst && id <= Blocks::TurtleEggLast; }
inline int turtleEggCount(BlockId id) { return isTurtleEgg(id) ? (id - Blocks::TurtleEggFirst) / 3 + 1 : 0; }
inline int turtleEggHatch(BlockId id) { return isTurtleEgg(id) ? (id - Blocks::TurtleEggFirst) % 3 : 0; }
inline BlockId turtleEggOf(int count, int hatch)
{
    if (count < 1) count = 1;
    if (count > 4) count = 4;
    return static_cast<BlockId>(Blocks::TurtleEggFirst + (count - 1) * 3 + (hatch < 0 ? 0 : hatch > 2 ? 2 : hatch));
}

inline bool isWaterPlant(BlockId id)
{
    return id == Blocks::Kelp || id == Blocks::KelpTop ||
           id == Blocks::Seagrass || id == Blocks::SeaPickle ||
           isTallSeagrass(id) || isLiveCoralPlant(id) || isCoralFan(id) || isCoralWallFan(id) ||
           isDeadCoralPlant(id) || isDeadCoralFan(id) || isDeadCoralWallFan(id);
}

// Whether this is one of the deep dark's blocks. The city is built of
// them and nothing else is, so this is also the test for "is this part
// of an ancient city".
inline bool isDeepDark(BlockId id)
{
    return id >= Blocks::DeepslateBricks && id <= Blocks::ReinforcedDeepslate;
}

// Whether this block keeps a few item slots of its own. All three use
// the same table the chests do: a block is a bare id with nowhere to
// put contents, so they live beside the world.
// What a geode is made of. Useful as one question because the whole
// shell is tested together.
// A barrel holds what a chest holds, and is opened the same way. The
// difference is that it needs no room above it, which is the whole
// reason to build one.
inline bool isColourBed(BlockId id) { return id >= Blocks::BedFirst && id <= Blocks::BedLast; }
inline bool isBed(BlockId id) { return id == Blocks::Bed || isColourBed(id); }
inline int bedColour(BlockId id) { return isColourBed(id) ? (id - Blocks::BedFirst) / 8 : 14; }
inline int bedFacing(BlockId id) { return isColourBed(id) ? ((id - Blocks::BedFirst) / 2) % 4 : 0; }
inline bool bedIsHead(BlockId id) { return isColourBed(id) && (id - Blocks::BedFirst) % 2 == 1; }
inline BlockId bedOf(int colour, int facing, bool head)
{
    if (colour < 0) colour = 0;
    if (colour > 15) colour = 15;
    return static_cast<BlockId>(Blocks::BedFirst + ((colour * 4 + (facing & 3)) * 2) + (head ? 1 : 0));
}
inline void bedStep(int facing, int& dx, int& dz)
{
    static const int STEP[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    dx = STEP[facing & 3][0];
    dz = STEP[facing & 3][1];
}
struct ChestState
{
    int kind;
    int facing;
    int half;
    bool open;
};
inline bool isChestFamily(BlockId id) { return id == Blocks::Chest || (id >= Blocks::ChestRunFirst && id <= Blocks::ChestRunLast); }
inline ChestState chestState(BlockId id)
{
    if (!isChestFamily(id) || id == Blocks::Chest) return { 0, 2, 0, false };
    int k = id - Blocks::ChestRunFirst;
    if (k < 11)
    {
        if (k >= 6) ++k;
        return { 0, k / 3, k % 3, false };
    }
    k -= 11;
    if (k < 24) return { 1, (k % 12) / 3, (k % 12) % 3, k >= 12 };
    return { 2, k - 24, 0, false };
}
inline BlockId chestOf(int kind, int facing, int half, bool open = false)
{
    facing &= 3;
    if (half < 0 || half > 2) half = 0;
    if (kind == 2) return static_cast<BlockId>(Blocks::ChestRunFirst + 35 + facing);
    if (kind == 1) return static_cast<BlockId>(Blocks::ChestRunFirst + 11 + (open ? 12 : 0) + facing * 3 + half);
    int k = facing * 3 + half;
    if (k == 6) return Blocks::Chest;
    if (k > 6) --k;
    return static_cast<BlockId>(Blocks::ChestRunFirst + k);
}
inline bool isStorageChest(BlockId id) { return isChestFamily(id) && chestState(id).kind != 2; }
inline bool isTrappedChest(BlockId id) { return isChestFamily(id) && chestState(id).kind == 1; }
inline bool isEnderChest(BlockId id) { return isChestFamily(id) && chestState(id).kind == 2; }
inline bool chestPartnerStep(BlockId id, int& dx, int& dz)
{
    static const int STEP[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    const ChestState s = chestState(id);
    if (!isChestFamily(id) || s.half == 0) { dx = dz = 0; return false; }
    const int way = s.half == 1 ? (s.facing + 1) & 3 : (s.facing + 3) & 3;
    dx = STEP[way][0];
    dz = STEP[way][1];
    return true;
}
void setChristmasChests(bool on);
bool christmasChests();
inline bool isShulkerBox(BlockId id)
{
    return id == Blocks::ShulkerBox || (id >= Blocks::ShulkerBoxFirst && id <= Blocks::ShulkerBoxLast);
}
inline int shulkerBoxColour(BlockId id)
{
    return (id >= Blocks::ShulkerBoxFirst && id <= Blocks::ShulkerBoxLast) ? id - Blocks::ShulkerBoxFirst : -1;
}
inline bool isContainerBlock(BlockId id)
{
    return isStorageChest(id) || id == Blocks::Barrel || id == Blocks::BarrelOpen ||
           isShulkerBox(id);
}
inline const char* colourFileName(int colour)
{
    static const char* NAMES[16] = { "white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray",
                                     "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black" };
    return (colour >= 0 && colour < 16) ? NAMES[colour] : "";
}

// Wool, in any of its sixteen colours. White is where it always was
// and the other fifteen are at the end of the list, so this asks two
// questions rather than one.
inline bool isWool(BlockId id)
{
    return id == Blocks::Wool ||
           (id >= Blocks::WoolOrange && id <= Blocks::WoolBlack);
}

// Minecraft's sixteen, in Minecraft's order: white is nought and
// black is fifteen. The dyes are in the same order, so a dye and the
// wool it makes share a number.
constexpr int DYE_COLOURS = 16;
inline int woolColour(BlockId id)
{
    if (id == Blocks::Wool) return 0;
    if (id < Blocks::WoolOrange || id > Blocks::WoolBlack) return 0;
    return (id - Blocks::WoolOrange) + 1;
}
inline BlockId woolOfColour(int colour)
{
    if (colour <= 0) return Blocks::Wool;
    if (colour >= DYE_COLOURS) colour = DYE_COLOURS - 1;
    return static_cast<BlockId>(Blocks::WoolOrange + colour - 1);
}

inline bool isFroglight(BlockId id)
{
    return id >= Blocks::OchreFroglight && id <= Blocks::PearlescentFroglight;
}

inline bool isGeode(BlockId id)
{
    return id >= Blocks::AmethystBlock && id <= Blocks::SmoothBasalt;
}

inline bool isItemHolder(BlockId id)
{
    return id == Blocks::Hopper || (id >= Blocks::HopperSideFirst && id <= Blocks::HopperSideLast) || id == Blocks::Dropper || id == Blocks::Dispenser ||
           (id >= Blocks::ThrowerTurnFirst && id <= Blocks::ThrowerTurnLast);
}

inline bool isNoteBlock(BlockId id)
{
    return id == Blocks::NoteBlock || id == Blocks::NoteBlockLit;
}

// What a beacon will stand on. Minecraft's pyramid takes iron, gold,
// diamond and emerald and nothing else -- coal is the one storage
// block that is not good enough, which is the joke of it.
inline bool isBeaconBase(BlockId id)
{
    return id == Blocks::IronBlock || id == Blocks::GoldBlock ||
           id == Blocks::DiamondBlock || id == Blocks::EmeraldBlock;
}

// Whether this is one of the End city's blocks, or one of a bastion's.
// Each structure is built of its own family and nothing else is, which
// is what makes these usable as "am I standing in one".
inline bool isEndCityBlock(BlockId id)
{
    return id >= Blocks::PurpurBlock && id <= Blocks::ChorusFlower;
}

inline bool isBastionBlock(BlockId id)
{
    return id >= Blocks::Blackstone && id <= Blocks::Basalt;
}

// The two that listen. A sensor lights when something moves near it;
// a shrieker is what the sensor tells.
inline bool isSculkSensor(BlockId id)
{
    return id == Blocks::SculkSensor || id == Blocks::SculkSensorLit;
}

// A reef's solid blocks, which is what a coral clump is mostly made of.
inline bool isCoralBlock(BlockId id)
{
    return id >= Blocks::TubeCoralBlock && id <= Blocks::HornCoralBlock;
}

inline int coralKind(BlockId id)
{
    if (isCoralBlock(id)) return id - Blocks::TubeCoralBlock;
    if (isLiveCoralPlant(id)) return id - Blocks::TubeCoral;
    if (isCoralFan(id)) return id - Blocks::CoralFanFirst;
    if (isCoralWallFan(id)) return (id - Blocks::CoralWallFanFirst) / 4;
    if (isDeadCoralBlock(id)) return id - Blocks::DeadCoralBlockFirst;
    if (isDeadCoralPlant(id)) return id - Blocks::DeadCoralFirst;
    if (isDeadCoralFan(id)) return id - Blocks::DeadCoralFanFirst;
    if (isDeadCoralWallFan(id)) return (id - Blocks::DeadCoralWallFanFirst) / 4;
    return -1;
}

inline bool isLivingCoral(BlockId id)
{
    return isCoralBlock(id) || isLiveCoralPlant(id) || isCoralFan(id) || isCoralWallFan(id);
}

inline BlockId deadCoralOf(BlockId id)
{
    const int kind = coralKind(id);
    if (isCoralBlock(id)) return static_cast<BlockId>(Blocks::DeadCoralBlockFirst + kind);
    if (isLiveCoralPlant(id)) return static_cast<BlockId>(Blocks::DeadCoralFirst + kind);
    if (isCoralFan(id)) return static_cast<BlockId>(Blocks::DeadCoralFanFirst + kind);
    if (isCoralWallFan(id)) return coralWallFanOf(true, kind, coralWallFanFacing(id));
    return id;
}

// A trial spawner in any of its three states: waiting, with its wave
// out, or finished with.
inline bool isTrialSpawner(BlockId id)
{
    return id == Blocks::TrialSpawner || id == Blocks::TrialSpawnerActive ||
           id == Blocks::TrialSpawnerSpent || id == Blocks::TrialSpawnerActiveOminous ||
           id == Blocks::TrialSpawnerEjecting || id == Blocks::TrialSpawnerEjectingOminous ||
           id == Blocks::TrialSpawnerSpentOminous;
}
inline bool isTrialSpawnerActive(BlockId id)
{
    return id == Blocks::TrialSpawnerActive || id == Blocks::TrialSpawnerActiveOminous;
}

enum class DoublePlant : uint8_t { Sunflower, Lilac, RoseBush, Peony, TallGrass, LargeFern, PitcherPlant, Count };
inline bool isDoublePlant(BlockId id) { return id >= Blocks::DoublePlantFirst && id <= Blocks::DoublePlantLast; }
inline DoublePlant doublePlantKind(BlockId id) { return static_cast<DoublePlant>((id - Blocks::DoublePlantFirst) / 2); }
inline bool doublePlantUpper(BlockId id) { return isDoublePlant(id) && (id - Blocks::DoublePlantFirst) % 2 == 1; }
inline BlockId doublePlantOf(DoublePlant kind, bool upper)
{
    return static_cast<BlockId>(Blocks::DoublePlantFirst + static_cast<int>(kind) * 2 + (upper ? 1 : 0));
}
inline bool isPitcherCrop(BlockId id) { return id >= Blocks::PitcherCropFirst && id <= Blocks::PitcherCropLast; }
inline bool pitcherCropUpper(BlockId id) { return isPitcherCrop(id) && id - Blocks::PitcherCropFirst >= 5; }
inline int pitcherCropStage(BlockId id)
{
    if (!isPitcherCrop(id)) return 0;
    const int k = id - Blocks::PitcherCropFirst;
    return k < 5 ? k : k - 2;
}
inline BlockId pitcherCropOf(int stage, bool upper)
{
    if (stage < 0) stage = 0;
    if (stage > 4) stage = 4;
    if (upper && stage >= 3) return static_cast<BlockId>(Blocks::PitcherCropFirst + 5 + (stage - 3));
    return static_cast<BlockId>(Blocks::PitcherCropFirst + stage);
}
inline bool isTorchflowerCrop(BlockId id) { return id == Blocks::TorchflowerCrop0 || id == Blocks::TorchflowerCrop1; }
inline bool isSmallFlower(BlockId id)
{
    return id == Blocks::FlowerRed || id == Blocks::FlowerYellow || (id >= Blocks::Allium && id <= Blocks::Torchflower);
}
inline bool isPotted(BlockId id)
{
    return (id >= Blocks::PottedFirst && id <= Blocks::PottedLast) || id == Blocks::PottedAzalea ||
           id == Blocks::PottedFloweringAzalea;
}
inline bool isFlowerPot(BlockId id) { return id == Blocks::FlowerPot || isPotted(id); }
BlockId pottedPlant(BlockId pot);
BlockId pottedOf(BlockId plant);
inline bool isCaveVine(BlockId id)
{
    return id == Blocks::CaveVines || id == Blocks::CaveVinesLit || id == Blocks::CaveVinesPlant || id == Blocks::GlowBerries;
}
inline bool caveVineLit(BlockId id) { return id == Blocks::CaveVinesLit || id == Blocks::GlowBerries; }
inline bool caveVineHead(BlockId id) { return id == Blocks::CaveVines || id == Blocks::CaveVinesLit; }
inline BlockId caveVineOf(bool head, bool lit)
{
    if (head) return lit ? Blocks::CaveVinesLit : Blocks::CaveVines;
    return lit ? Blocks::GlowBerries : Blocks::CaveVinesPlant;
}
enum class DripleafTilt : uint8_t { None, Unstable, Partial, Full };
inline bool isBigDripleaf(BlockId id) { return id >= Blocks::BigDripleafFirst && id <= Blocks::BigDripleafLast; }
inline bool isBigDripleafStem(BlockId id) { return id >= Blocks::BigDripleafStemFirst && id <= Blocks::BigDripleafStemLast; }
inline bool isSmallDripleaf(BlockId id) { return id >= Blocks::SmallDripleafFirst && id <= Blocks::SmallDripleafLast; }
inline int dripleafFacing(BlockId id)
{
    if (isBigDripleaf(id)) return (id - Blocks::BigDripleafFirst) / 4;
    if (isBigDripleafStem(id)) return id - Blocks::BigDripleafStemFirst;
    if (isSmallDripleaf(id)) return (id - Blocks::SmallDripleafFirst) / 2;
    return 0;
}
inline DripleafTilt dripleafTilt(BlockId id)
{
    return isBigDripleaf(id) ? static_cast<DripleafTilt>((id - Blocks::BigDripleafFirst) % 4) : DripleafTilt::None;
}
inline BlockId bigDripleafOf(int facing, DripleafTilt tilt)
{
    return static_cast<BlockId>(Blocks::BigDripleafFirst + (facing & 3) * 4 + static_cast<int>(tilt));
}
inline BlockId bigDripleafStemOf(int facing) { return static_cast<BlockId>(Blocks::BigDripleafStemFirst + (facing & 3)); }
inline bool smallDripleafUpper(BlockId id) { return isSmallDripleaf(id) && (id - Blocks::SmallDripleafFirst) % 2 == 1; }
inline BlockId smallDripleafOf(int facing, bool upper)
{
    return static_cast<BlockId>(Blocks::SmallDripleafFirst + (facing & 3) * 2 + (upper ? 1 : 0));
}
inline bool isGlowLichen(BlockId id) { return id >= Blocks::GlowLichenFirst && id <= Blocks::GlowLichenLast; }
inline int glowLichenFace(BlockId id) { return isGlowLichen(id) ? id - Blocks::GlowLichenFirst : 0; }
inline BlockId glowLichenOn(int face) { return static_cast<BlockId>(Blocks::GlowLichenFirst + (face < 0 || face > 5 ? 0 : face)); }
inline bool isAzaleaBush(BlockId id) { return id == Blocks::Azalea || id == Blocks::FloweringAzalea; }
inline bool isMushroom(BlockId id) { return id == Blocks::BrownMushroom || id == Blocks::RedMushroom; }
inline bool isMushroomBlock(BlockId id)
{
    return id == Blocks::BrownMushroomBlock || id == Blocks::RedMushroomBlock || id == Blocks::MushroomStem;
}
inline bool isWildPlant(BlockId id)
{
    return isSmallFlower(id) || id == Blocks::Fern || id == Blocks::DeadBush || id == Blocks::TallGrass ||
           isDoublePlant(id) || isPitcherCrop(id) || isTorchflowerCrop(id) || isMushroom(id) ||
           id == Blocks::HangingRoots || isCaveVine(id) || id == Blocks::SporeBlossom || isBigDripleaf(id) ||
           isBigDripleafStem(id) || isSmallDripleaf(id) || isAzaleaBush(id) || isGlowLichen(id) ||
           id == Blocks::MossCarpet || isCarpet(id);
}
inline bool isEarthy(BlockId id)
{
    return id == Blocks::Mycelium || id == Blocks::Podzol || id == Blocks::CoarseDirt || id == Blocks::RootedDirt;
}
enum class CopperFamily : uint8_t { None, Block, Cut, Chiseled, Grate, Slab, Bulb, Door, Trapdoor };
struct CopperForm
{
    CopperFamily family;
    int stage;
    bool waxed;
    int state;
};
CopperForm copperForm(BlockId id);
BlockId copperBlockOf(CopperFamily family, int stage, bool waxed, int state = 0);
inline bool isCopper(BlockId id) { return copperForm(id).family != CopperFamily::None; }
inline BlockId copperWith(BlockId id, int stage, bool waxed)
{
    const CopperForm form = copperForm(id);
    if (form.family == CopperFamily::None) return id;
    return copperBlockOf(form.family, stage, waxed, form.state);
}
inline bool isLightningRod(BlockId id) { return id >= Blocks::LightningRodFirst && id <= Blocks::LightningRodLast; }
inline int lightningRodFacing(BlockId id) { return isLightningRod(id) ? (id - Blocks::LightningRodFirst) / 2 : 0; }
inline bool lightningRodPowered(BlockId id) { return isLightningRod(id) && (id - Blocks::LightningRodFirst) % 2 == 1; }
inline BlockId lightningRodOf(int facing, bool powered)
{
    if (facing < 0 || facing > 5) facing = 0;
    return static_cast<BlockId>(Blocks::LightningRodFirst + facing * 2 + (powered ? 1 : 0));
}

inline bool isAmethystGrowth(BlockId id)
{
    return id == Blocks::SmallAmethystBud || id == Blocks::MediumAmethystBud ||
           id == Blocks::LargeAmethystBud || id == Blocks::AmethystCluster;
}
inline int amethystStage(BlockId id)
{
    switch (id)
    {
        case Blocks::SmallAmethystBud: return 0;
        case Blocks::MediumAmethystBud: return 1;
        case Blocks::LargeAmethystBud: return 2;
        case Blocks::AmethystCluster: return 3;
        default: return -1;
    }
}
inline BlockId amethystAtStage(int stage)
{
    static const BlockId STAGES[4] = { Blocks::SmallAmethystBud, Blocks::MediumAmethystBud,
                                       Blocks::LargeAmethystBud, Blocks::AmethystCluster };
    return STAGES[stage < 0 ? 0 : stage > 3 ? 3 : stage];
}
inline bool isPointedDripstone(BlockId id)
{
    return id == Blocks::DripstoneDown || id == Blocks::DripstoneUp;
}

inline bool isCopperBulb(BlockId id)
{
    return id == Blocks::CopperBulb || id == Blocks::CopperBulbLit ||
           id == Blocks::CopperBulbPowered || id == Blocks::CopperBulbLitPowered ||
           (id >= Blocks::CopperBulbRunFirst && id <= Blocks::CopperBulbRunLast);
}
inline bool copperBulbLit(BlockId id)
{
    return (copperForm(id).state & 1) != 0;
}
inline BlockId copperBulbAs(bool lit, bool powered)
{
    if (lit) return powered ? Blocks::CopperBulbLitPowered : Blocks::CopperBulbLit;
    return powered ? Blocks::CopperBulbPowered : Blocks::CopperBulb;
}
inline BlockId copperBulbWith(BlockId id, bool lit, bool powered)
{
    const CopperForm form = copperForm(id);
    return copperBlockOf(CopperFamily::Bulb, form.stage, form.waxed, (lit ? 1 : 0) + (powered ? 2 : 0));
}
inline bool isAnvil(BlockId id)
{
    return id == Blocks::Anvil || id == Blocks::AnvilChipped || id == Blocks::AnvilDamaged;
}
inline bool isNetherWart(BlockId id)
{
    return id == Blocks::NetherWart || id == Blocks::NetherWart0 || id == Blocks::NetherWart1;
}
inline bool isSculkCatalyst(BlockId id)
{
    return id == Blocks::SculkCatalyst || id == Blocks::SculkCatalystBloom;
}

inline bool isCrafter(BlockId id)
{
    return id == Blocks::Crafter || id == Blocks::CrafterLit;
}

inline bool isVault(BlockId id)
{
    return id == Blocks::Vault || id == Blocks::VaultOpen ||
           id == Blocks::OminousVault || id == Blocks::OminousVaultOpen ||
           id == Blocks::VaultEjecting || id == Blocks::OminousVaultEjecting;
}

inline bool isOminousVault(BlockId id)
{
    return id == Blocks::OminousVault || id == Blocks::OminousVaultOpen ||
           id == Blocks::OminousVaultEjecting;
}

// Painted on the floor but not a track. Dust is drawn exactly as a rail
// is and must not be treated as one, or laying it beside a line bends
// it into the track and a cart runs onto it.
inline bool isFlat(BlockId id) { return blockInfo(id).render == RenderType::Flat; }

// The stones that count as "the ground" -- what ores replace and what
// caves are cut through. Stone alone was the test everywhere before
// there was more than one kind, which would have left every seam in the
// deep rock hanging in a pocket of its own.
inline bool isStoneFamily(BlockId id)
{
    return id == Blocks::Stone || id == Blocks::Granite || id == Blocks::Diorite ||
           id == Blocks::Andesite || id == Blocks::Deepslate || id == Blocks::Tuff ||
           id == Blocks::Calcite;
}

// Every plank, whichever tree it came off. Anything that asks "is this
// a plank" has to ask this rather than naming Blocks::Planks, or a
// spruce house is not a house.
inline bool isPlanks(BlockId id)
{
    return id == Blocks::Planks || id == Blocks::SprucePlanks ||
           id == Blocks::DarkOakPlanks || id == Blocks::JunglePlanks ||
           id == Blocks::AcaciaPlanks || id == Blocks::BirchPlanks || id == Blocks::CherryPlanks ||
           id == Blocks::MangrovePlanks || id == Blocks::BambooPlanks ||
           id == Blocks::CrimsonPlanks || id == Blocks::WarpedPlanks;
}

inline bool isLog(BlockId id)
{
    id = axisBase(id);
    return isWoodBlock(id) || id == Blocks::Log || id == Blocks::BirchLog ||
           id == Blocks::SpruceLog || id == Blocks::DarkOakLog ||
           id == Blocks::JungleLog || id == Blocks::AcaciaLog || id == Blocks::CherryLog ||
           id == Blocks::MangroveLog || id == Blocks::StrippedMangroveLog ||
           id == Blocks::BambooBlock || id == Blocks::StrippedBambooBlock ||
           id == Blocks::CrimsonStem || id == Blocks::StrippedCrimsonStem ||
           id == Blocks::WarpedStem || id == Blocks::StrippedWarpedStem ||
           (id >= Blocks::StrippedOakLog && id <= Blocks::StrippedDarkOakLog) || id == Blocks::StrippedCherryLog;
}

// The planks a log breaks down into.
inline BlockId planksFromLog(BlockId log)
{
    log = axisBase(log);
    if (isWoodBlock(log)) return woodSet(woodBlockWood(log)).planks;
    if (log == Blocks::CherryLog || log == Blocks::StrippedCherryLog) return Blocks::CherryPlanks;
    if (log == Blocks::MangroveLog || log == Blocks::StrippedMangroveLog) return Blocks::MangrovePlanks;
    if (log == Blocks::BambooBlock || log == Blocks::StrippedBambooBlock) return Blocks::BambooPlanks;
    if (log == Blocks::CrimsonStem || log == Blocks::StrippedCrimsonStem) return Blocks::CrimsonPlanks;
    if (log == Blocks::WarpedStem || log == Blocks::StrippedWarpedStem) return Blocks::WarpedPlanks;
    if (log >= Blocks::StrippedOakLog && log <= Blocks::StrippedDarkOakLog)
    {
        static const BlockId FROM[6] = { Blocks::Planks, Blocks::SprucePlanks, Blocks::BirchPlanks,
                                         Blocks::JunglePlanks, Blocks::AcaciaPlanks, Blocks::DarkOakPlanks };
        return FROM[log - Blocks::StrippedOakLog];
    }
    switch (log)
    {
        case Blocks::BirchLog:   return Blocks::BirchPlanks;
        case Blocks::SpruceLog:  return Blocks::SprucePlanks;
        case Blocks::DarkOakLog: return Blocks::DarkOakPlanks;
        case Blocks::JungleLog:  return Blocks::JunglePlanks;
        case Blocks::AcaciaLog:  return Blocks::AcaciaPlanks;
        default:                 return Blocks::Planks;
    }
}

// Where deepslate takes over from stone.
constexpr int DEEPSLATE_TOP = 18;
inline bool isRailCorner(BlockId id)
{
    return id >= Blocks::RailNorthEast && id <= Blocks::RailNorthWest;
}

// Which two ways a rail runs. 0 north (-z), 1 east (+x), 2 south (+z),
// 3 west (-x) -- the same compass the ladders and stairs use.
inline bool railLinks(BlockId rail, int facing)
{
    switch (rail)
    {
        case Blocks::RailNorthSouth: return facing == 0 || facing == 2;
        case Blocks::RailEastWest:   return facing == 1 || facing == 3;
        case Blocks::RailNorthEast:  return facing == 0 || facing == 1;
        case Blocks::RailSouthEast:  return facing == 2 || facing == 1;
        case Blocks::RailSouthWest:  return facing == 2 || facing == 3;
        case Blocks::RailNorthWest:  return facing == 0 || facing == 3;
        default:
            if (isSpecialRail(rail)) return specialRailEastWest(rail) ? (facing == 1 || facing == 3) : (facing == 0 || facing == 2);
            return false;
    }
}

// The shape a rail takes given what it has to join up with. A straight
// run wins over a corner where both are possible, which is what stops a
// long line kinking every time a spur is laid beside it.
inline BlockId railShapeFor(bool north, bool east, bool south, bool west)
{
    if (north && south) return Blocks::RailNorthSouth;
    if (east && west)   return Blocks::RailEastWest;

    if (north && east) return Blocks::RailNorthEast;
    if (south && east) return Blocks::RailSouthEast;
    if (south && west) return Blocks::RailSouthWest;
    if (north && west) return Blocks::RailNorthWest;

    if (east || west) return Blocks::RailEastWest;

    // Nothing to join, or one neighbour north or south: lie along z,
    // which is what a single rail laid on its own looks like.
    return Blocks::RailNorthSouth;
}

inline int doorWood(BlockId id)
{
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (id >= woodSet(wood).door && id < woodSet(wood).door + 16) return wood;
    return -1;
}
inline BlockId doorBaseOf(int wood) { return woodSet(wood).door; }
inline BlockId doorBase(BlockId id)
{
    if (id >= Blocks::IronDoorFirst && id <= Blocks::IronDoorLast) return Blocks::IronDoorFirst;
    if (id >= Blocks::CopperDoorFirst && id <= Blocks::CopperDoorLast)
        return static_cast<BlockId>(Blocks::CopperDoorFirst + ((id - Blocks::CopperDoorFirst) / 16) * 16);
    return doorBaseOf(doorWood(id));
}
inline bool doorIsTop(BlockId id) { return ((id - doorBase(id)) & 8) != 0; }
inline bool doorIsOpen(BlockId id) { return ((id - doorBase(id)) & 4) != 0; }
inline BlockId doorPart(BlockId base, int facing, bool top, bool open)
{
    return static_cast<BlockId>(base + (top ? 8 : 0) + (open ? 4 : 0) + (facing & 3));
}
inline BlockId doorPart(int facing, bool top, bool open)
{
    return doorPart(Blocks::DoorLowShutNorth, facing, top, open);
}

inline bool isTrapdoor(BlockId id) { return blockInfo(id).render == RenderType::Trapdoor; }
inline BlockId trapdoorBaseOf(int wood) { return woodSet(wood).trapdoor; }
inline int trapdoorWood(BlockId id)
{
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (id >= woodSet(wood).trapdoor && id < woodSet(wood).trapdoor + 16) return wood;
    return 0;
}
inline BlockId trapdoorBase(BlockId id)
{
    if (id >= Blocks::IronTrapdoorFirst && id <= Blocks::IronTrapdoorLast) return Blocks::IronTrapdoorFirst;
    if (id >= Blocks::CopperTrapdoorFirst && id <= Blocks::CopperTrapdoorLast)
        return static_cast<BlockId>(Blocks::CopperTrapdoorFirst + ((id - Blocks::CopperTrapdoorFirst) / 16) * 16);
    return trapdoorBaseOf(trapdoorWood(id));
}
inline bool trapdoorIsTop(BlockId id) { return ((id - trapdoorBase(id)) & 8) != 0; }
inline bool trapdoorIsOpen(BlockId id) { return ((id - trapdoorBase(id)) & 4) != 0; }
inline int trapdoorFacing(BlockId id) { return (id - trapdoorBase(id)) & 3; }
inline BlockId trapdoorPart(BlockId base, int facing, bool top, bool open)
{
    return static_cast<BlockId>(base + (top ? 8 : 0) + (open ? 4 : 0) + (facing & 3));
}

inline int saplingWood(BlockId id)
{
    if (id == Blocks::Air) return -1;
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (woodSet(wood).sapling == id) return wood;
    return -1;
}
inline bool isSapling(BlockId id) { return saplingWood(id) >= 0; }
inline bool isStrippedLog(BlockId id)
{
    id = axisBase(id);
    if (isStrippedWoodBlock(id)) return true;
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (woodSet(wood).stripped == id) return true;
    return false;
}
inline BlockId logOfWood(int wood) { return woodSet(wood).log; }
inline BlockId planksOfWood(int wood) { return woodSet(wood).planks; }
inline BlockId leavesOfWood(int wood) { return woodSet(wood).leaves; }
inline int woodOfLog(BlockId id)
{
    id = axisBase(id);
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (woodSet(wood).log == id || woodSet(wood).stripped == id) return wood;
    return -1;
}
inline int woodOfLeaves(BlockId id)
{
    if (id == Blocks::Air) return -1;
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (woodSet(wood).leaves == id) return wood;
    return -1;
}
inline BlockId strippedOf(BlockId log)
{
    if (hasAxis(log) && blockAxis(log) != 0)
    {
        const BlockId bare = strippedOf(axisBase(log));
        return bare == Blocks::Air ? bare : withAxis(bare, blockAxis(log));
    }
    if (isWoodBlock(log)) return isStrippedWoodBlock(log) ? static_cast<BlockId>(Blocks::Air) : woodBlockOf(woodBlockWood(log), true);
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (woodSet(wood).log == log) return woodSet(wood).stripped;
    return Blocks::Air;
}
inline bool isPinkPetals(BlockId id) { return id >= Blocks::PinkPetals1 && id <= Blocks::PinkPetals4; }
inline BlockId oreBase(BlockId id)
{
    switch (id)
    {
        case Blocks::DeepslateCoalOre: return Blocks::CoalOre;
        case Blocks::DeepslateIronOre: return Blocks::IronOre;
        case Blocks::DeepslateCopperOre: return Blocks::CopperOre;
        case Blocks::DeepslateGoldOre: return Blocks::GoldOre;
        case Blocks::DeepslateRedstoneOre: return Blocks::RedstoneOre;
        case Blocks::DeepslateEmeraldOre: return Blocks::EmeraldOre;
        case Blocks::DeepslateLapisOre: return Blocks::LapisOre;
        case Blocks::DeepslateDiamondOre: return Blocks::DiamondOre;
        default: return id;
    }
}
inline BlockId deepslateOreOf(BlockId ore)
{
    switch (ore)
    {
        case Blocks::CoalOre: return Blocks::DeepslateCoalOre;
        case Blocks::IronOre: return Blocks::DeepslateIronOre;
        case Blocks::CopperOre: return Blocks::DeepslateCopperOre;
        case Blocks::GoldOre: return Blocks::DeepslateGoldOre;
        case Blocks::RedstoneOre: return Blocks::DeepslateRedstoneOre;
        case Blocks::EmeraldOre: return Blocks::DeepslateEmeraldOre;
        case Blocks::LapisOre: return Blocks::DeepslateLapisOre;
        case Blocks::DiamondOre: return Blocks::DeepslateDiamondOre;
        default: return ore;
    }
}
inline bool inRun(BlockId id, BlockId first) { return id >= first && id <= first + 15; }
inline bool isConcrete(BlockId id) { return inRun(id, Blocks::ConcreteFirst); }
inline bool isConcretePowder(BlockId id) { return inRun(id, Blocks::ConcretePowderFirst); }
inline bool isColouredTerracotta(BlockId id) { return inRun(id, Blocks::TerracottaFirst); }
inline bool isGlazedTerracotta(BlockId id) { return inRun(id, Blocks::GlazedTerracottaFirst); }
inline bool isStainedGlass(BlockId id) { return inRun(id, Blocks::StainedGlassFirst); }
inline bool isStainedPane(BlockId id) { return inRun(id, Blocks::StainedGlassPaneFirst); }
inline bool isGlassLike(BlockId id)
{
    return id == Blocks::Glass || id == Blocks::TintedGlass || isStainedGlass(id);
}
inline bool isCandle(BlockId id)
{
    return id == Blocks::Candle || id == Blocks::CandleLit ||
           (id >= Blocks::PlainCandleFirst && id <= Blocks::PlainCandleLast) ||
           (id >= Blocks::DyedCandleFirst && id <= Blocks::DyedCandleLast);
}
inline bool isDyedCandle(BlockId id) { return id >= Blocks::DyedCandleFirst && id <= Blocks::DyedCandleLast; }
inline int candleShade(BlockId id) { return isDyedCandle(id) ? 1 + (id - Blocks::DyedCandleFirst) / 8 : 0; }
inline int candleCount(BlockId id)
{
    if (id >= Blocks::PlainCandleFirst && id <= Blocks::PlainCandleLast) return 2 + (id - Blocks::PlainCandleFirst) / 2;
    if (isDyedCandle(id)) return 1 + ((id - Blocks::DyedCandleFirst) / 2) % 4;
    return isCandle(id) ? 1 : 0;
}
inline bool candleLit(BlockId id)
{
    if (id == Blocks::CandleLit) return true;
    if (id >= Blocks::PlainCandleFirst && id <= Blocks::PlainCandleLast) return (id - Blocks::PlainCandleFirst) % 2 == 1;
    if (isDyedCandle(id)) return (id - Blocks::DyedCandleFirst) % 2 == 1;
    return false;
}
inline BlockId candleOf(int shade, int count, bool lit)
{
    if (shade < 0) shade = 0;
    if (shade > 16) shade = 16;
    if (count < 1) count = 1;
    if (count > 4) count = 4;
    if (shade == 0)
    {
        if (count == 1) return lit ? Blocks::CandleLit : Blocks::Candle;
        return static_cast<BlockId>(Blocks::PlainCandleFirst + (count - 2) * 2 + (lit ? 1 : 0));
    }
    return static_cast<BlockId>(Blocks::DyedCandleFirst + ((shade - 1) * 4 + count - 1) * 2 + (lit ? 1 : 0));
}
inline bool isCandleCake(BlockId id) { return id >= Blocks::CandleCakeFirst && id <= Blocks::CandleCakeLast; }
inline int candleCakeShade(BlockId id) { return isCandleCake(id) ? (id - Blocks::CandleCakeFirst) / 2 : 0; }
inline bool candleCakeLit(BlockId id) { return isCandleCake(id) && (id - Blocks::CandleCakeFirst) % 2 == 1; }
inline BlockId candleCakeOf(int shade, bool lit)
{
    if (shade < 0) shade = 0;
    if (shade > 16) shade = 16;
    return static_cast<BlockId>(Blocks::CandleCakeFirst + shade * 2 + (lit ? 1 : 0));
}
inline bool paneJoins(BlockId other)
{
    return blockInfo(other).render == RenderType::Pane || isGlassLike(other) ||
           (blockInfo(other).opaque && blockInfo(other).render == RenderType::Cube);
}
template <typename At>
int paneLinks(At at)
{
    int links = 0;
    if (paneJoins(at(0, -1))) links |= 1;
    if (paneJoins(at(0, 1))) links |= 2;
    if (paneJoins(at(-1, 0))) links |= 4;
    if (paneJoins(at(1, 0))) links |= 8;
    return links;
}

inline int stairsBack(BlockId id) { return (stairsFacingOf(id) + 2) & 3; }
inline int stairsSideQuadrants(int direction)
{
    static const int SIDES[4] = { 3, 10, 12, 5 };
    return SIDES[direction & 3];
}
template <typename At>
int stairsQuadrantsAt(BlockId id, At at)
{
    static const int DX[4] = { 0, 1, 0, -1 };
    static const int DZ[4] = { -1, 0, 1, 0 };
    const int back = stairsBack(id);
    const bool top = stairsUpsideDown(id);
    auto matching = [&](BlockId other) { return isStairs(other) && stairsUpsideDown(other) == top; };
    auto different = [&](int direction) {
        const BlockId other = at(DX[direction], DZ[direction]);
        return !isStairs(other) || stairsBack(other) != back || stairsUpsideDown(other) != top;
    };
    const BlockId behind = at(DX[back], DZ[back]);
    if (matching(behind))
    {
        const int turn = stairsBack(behind);
        if ((turn & 1) != (back & 1) && different((turn + 2) & 3))
            return stairsSideQuadrants(back) & stairsSideQuadrants(turn);
    }
    const int front = (back + 2) & 3;
    const BlockId ahead = at(DX[front], DZ[front]);
    if (matching(ahead))
    {
        const int turn = stairsBack(ahead);
        if ((turn & 1) != (back & 1) && different(turn))
            return stairsSideQuadrants(back) | (stairsSideQuadrants(front) & stairsSideQuadrants(turn));
    }
    return stairsSideQuadrants(back);
}
inline bool fenceJoins(BlockId self, BlockId other)
{
    if (isFence(other)) return (fenceWood(self) >= 0) == (fenceWood(other) >= 0);
    if (isFenceGate(other)) return fenceWood(self) >= 0;
    return blockInfo(other).opaque && blockInfo(other).render == RenderType::Cube;
}
inline bool wallJoins(BlockId other)
{
    return isWall(other) || isFenceGate(other) || blockInfo(other).render == RenderType::Pane ||
           (blockInfo(other).opaque && blockInfo(other).render == RenderType::Cube);
}
template <typename At>
int shapeLinks(BlockId id, At at)
{
    if (isPane(id)) return paneLinks(at);
    if (isStairs(id)) return 0x100 | (stairsQuadrantsAt(id, at) << 4);
    int links = 0;
    if (isFence(id))
    {
        if (fenceJoins(id, at(0, -1))) links |= 1;
        if (fenceJoins(id, at(0, 1))) links |= 2;
        if (fenceJoins(id, at(-1, 0))) links |= 4;
        if (fenceJoins(id, at(1, 0))) links |= 8;
        return links;
    }
    if (isWall(id))
    {
        if (wallJoins(at(0, -1))) links |= 1;
        if (wallJoins(at(0, 1))) links |= 2;
        if (wallJoins(at(-1, 0))) links |= 4;
        if (wallJoins(at(1, 0))) links |= 8;
        return links;
    }
    return 15;
}
inline int runColour(BlockId id, BlockId first) { return inRun(id, first) ? id - first : 0; }
inline BlockId colourIn(BlockId first, int colour)
{
    if (colour < 0) colour = 0;
    if (colour > 15) colour = 15;
    return static_cast<BlockId>(first + colour);
}
inline bool isNylium(BlockId id) { return id == Blocks::CrimsonNylium || id == Blocks::WarpedNylium; }
inline bool isFungus(BlockId id) { return id == Blocks::CrimsonFungus || id == Blocks::WarpedFungus; }
inline bool isNetherPlant(BlockId id)
{
    return isFungus(id) || id == Blocks::CrimsonRoots || id == Blocks::WarpedRoots || id == Blocks::NetherSprouts ||
           id == Blocks::WeepingVines || id == Blocks::WeepingVinesPlant || id == Blocks::TwistingVines ||
           id == Blocks::TwistingVinesPlant;
}

// How far above its own cell a block stands. A fence is half a block
// taller than the cell it sits in, which is what stops you stepping
// over one; everything else stops at the top of its cell.
inline float blockOverhang(BlockId id) { return isFence(id) ? 0.5f : 0.0f; }

// Which way a facing block points: 0 north (-z), 1 east (+x), 2 south
// (+z), 3 west (-x). Meaningless on anything that does not face.
inline int furnaceSlot(BlockId id)
{
    switch (id)
    {
        case Blocks::Furnace: return 0;
        case Blocks::FurnaceLit: return 1;
        case Blocks::Smoker: return 2;
        case Blocks::SmokerLit: return 3;
        case Blocks::BlastFurnace: return 4;
        case Blocks::BlastFurnaceLit: return 5;
        default: break;
    }
    if (id >= Blocks::FurnaceTurned && id < Blocks::FurnaceTurnedEnd)
        return (id - Blocks::FurnaceTurned) / 3;
    return -1;
}
inline BlockId furnaceOfSlot(int slot)
{
    static const BlockId BASES[6] = { Blocks::Furnace, Blocks::FurnaceLit, Blocks::Smoker,
                                      Blocks::SmokerLit, Blocks::BlastFurnace, Blocks::BlastFurnaceLit };
    return BASES[slot < 0 ? 0 : slot % 6];
}
inline bool isFurnaceBlock(BlockId id) { return furnaceSlot(id) >= 0; }
inline int furnaceFacing(BlockId id)
{
    if (id >= Blocks::FurnaceTurned && id < Blocks::FurnaceTurnedEnd)
        return (id - Blocks::FurnaceTurned) % 3 + 1;
    return 0;
}
inline BlockId furnaceBase(BlockId id)
{
    const int slot = furnaceSlot(id);
    return slot < 0 ? id : furnaceOfSlot(slot);
}
inline BlockId furnaceTurned(BlockId id, int facing)
{
    const int slot = furnaceSlot(id);
    if (slot < 0) return id;
    facing &= 3;
    if (facing == 0) return furnaceOfSlot(slot);
    return static_cast<BlockId>(Blocks::FurnaceTurned + slot * 3 + facing - 1);
}
inline BlockId furnaceLit(BlockId id)
{
    const int slot = furnaceSlot(id);
    if (slot < 0) return id;
    return furnaceTurned(furnaceOfSlot(slot | 1), furnaceFacing(id));
}
inline BlockId furnaceCold(BlockId id)
{
    const int slot = furnaceSlot(id);
    if (slot < 0) return id;
    return furnaceTurned(furnaceOfSlot(slot & ~1), furnaceFacing(id));
}

inline bool isCarvedPumpkin(BlockId id) { return id >= Blocks::CarvedPumpkinFirst && id <= Blocks::CarvedPumpkinLast; }
inline bool isJackOLantern(BlockId id) { return id >= Blocks::JackOLanternFirst && id <= Blocks::JackOLanternLast; }
inline bool isSweetBerryBush(BlockId id) { return id >= Blocks::SweetBerryBush0 && id <= Blocks::SweetBerryBush3; }
inline int sweetBerryStage(BlockId id) { return isSweetBerryBush(id) ? id - Blocks::SweetBerryBush0 : 0; }
inline bool isBeeNest(BlockId id) { return id >= Blocks::BeeNestFirst && id <= Blocks::BeeNestLast; }
inline bool isBeehive(BlockId id) { return id >= Blocks::BeehiveFirst && id <= Blocks::BeehiveLast; }
inline bool isHive(BlockId id) { return isBeeNest(id) || isBeehive(id); }
inline int hiveHoney(BlockId id)
{
    if (isBeeNest(id)) return (id - Blocks::BeeNestFirst) % 6;
    if (isBeehive(id)) return (id - Blocks::BeehiveFirst) % 6;
    return 0;
}
inline int hiveFacing(BlockId id)
{
    if (isBeeNest(id)) return (id - Blocks::BeeNestFirst) / 6;
    if (isBeehive(id)) return (id - Blocks::BeehiveFirst) / 6;
    return 0;
}
inline BlockId hiveOf(bool nest, int facing, int honey)
{
    if (honey < 0) honey = 0;
    if (honey > 5) honey = 5;
    return static_cast<BlockId>((nest ? Blocks::BeeNestFirst : Blocks::BeehiveFirst) + (facing & 3) * 6 + honey);
}

inline int blockFacing(BlockId id)
{
    if (isLadder(id)) return id - Blocks::LadderNorth;
    if (isSign(id)) return signFacing(id);
    if (isFrame(id)) return frameFacing(id);
    if (isStairs(id)) return stairsFacingOf(id);
    if (isFenceGate(id)) return gateFacing(id);
    if (isDoor(id)) return (id - doorBase(id)) & 3;
    if (isTrapdoor(id)) return trapdoorFacing(id);
    if (isPiston(id)) return pistonFacing(id);
    if (isFurnaceBlock(id)) return furnaceFacing(id);
    if (isVault(id)) return 2;
    if (isCarvedPumpkin(id)) return id - Blocks::CarvedPumpkinFirst;
    if (isJackOLantern(id)) return id - Blocks::JackOLanternFirst;
    if (isHive(id)) return hiveFacing(id);
    if (isAnyCoralWallFan(id)) return coralWallFanFacing(id);
    if (isDiode(id)) return diodeOut(id);
    if (isTripwireHook(id)) return hookFacing(id);
    if (isBell(id)) return id - Blocks::BellFirst;
    if (isChiseledBookshelf(id)) return shelfFacing(id);
    if (isDecoratedPot(id)) return id - Blocks::DecoratedPotFirst;
    if (isLoom(id)) return id - Blocks::LoomFirst;
    if (isBanner(id)) return bannerFacing(id);
    if (isCalibratedSensor(id)) return calibratedFacing(id);
    return 0;
}

// The ladder that hangs on the wall in the given direction.
inline BlockId ladderFacing(int facing)
{
    return static_cast<BlockId>(Blocks::LadderNorth + (facing & 3));
}

// The same flight of stairs turned to face another way.
inline BlockId stairsFacing(BlockId stairs, int facing)
{
    if (!isStairs(stairs)) return stairs;
    return stairsOf(stairsMaterial(stairs), stairsUpsideDown(stairs), facing);
}

// How tall the block actually stands in its own cell. Everything is a
// full block but the slabs, and the collision and the mesher both read
// this rather than each keeping their own list.
inline float blockHeight(BlockId id) { return isSlab(id) || isAnySculkSensor(id) ? 0.5f : 1.0f; }

// Whether the solid part of the block at (bx, by, bz) overlaps the box
// between the two corners given. Everything fills its whole cell but
// the slabs and the stairs, and those two are the whole reason this
// exists rather than a plain "is it solid".
//
// Plain floats rather than a vector type on purpose: this header is
// included almost everywhere, and pulling glm in alongside the vendored
// C sources is not worth the one convenience.
bool blockOverlaps(BlockId id, int bx, int by, int bz,
                   float minX, float minY, float minZ,
                   float maxX, float maxY, float maxZ, int links = 15);
inline bool isLeaves(BlockId id)
{
    return id == Blocks::Leaves || id == Blocks::BirchLeaves ||
           id == Blocks::SpruceLeaves || id == Blocks::DarkOakLeaves ||
           id == Blocks::JungleLeaves || id == Blocks::AcaciaLeaves ||
           id == Blocks::AzaleaLeaves || id == Blocks::CherryLeaves ||
           id == Blocks::MangroveLeaves;
}
inline bool isVisible(BlockId id) { return blockInfo(id).render != RenderType::None; }
inline uint8_t lightEmission(BlockId id) { return blockInfo(id).lightEmission; }
inline bool fallsByGravity(BlockId id)
{
    return id == Blocks::Sand || id == Blocks::RedSand || id == Blocks::Gravel || isConcretePowder(id) ||
           isAnvil(id) || id == Blocks::DragonEgg || isSuspicious(id);
}
inline bool canFallThrough(BlockId id)
{
    return id == Blocks::Air || isLiquid(id) || isFire(id) || id == Blocks::TallGrass || id == Blocks::Fern ||
           id == Blocks::DeadBush || id == Blocks::Seagrass || isTallSeagrass(id) || isLightBlock(id);
}

// Blocks the player can pick up / place. Used by the creative palette.
bool isObtainable(BlockId id);

// Which footstep/dig sound family a block belongs to. The names match the
// folders under assets/sounds, so a caller builds "dig/" or "step/" plus this.
// Returns nullptr for blocks that make no sound, such as air.
const char* blockSoundGroup(BlockId id);

// How many numbered footstep takes a sound family has; they are not uniform.
int blockStepVariants(const char* group);
Material blockMaterial(BlockId id);

// Farming helpers. WHEAT_STAGES is how many ids the crop spans, and
// WHEAT_RIPE is the last of them.
constexpr int WHEAT_STAGES = 8;
constexpr BlockId WHEAT_RIPE = Blocks::Wheat7;
inline bool isWheat(BlockId id) { return id >= Blocks::Wheat0 && id <= Blocks::Wheat7; }
inline int wheatStage(BlockId id) { return isWheat(id) ? id - Blocks::Wheat0 : 0; }

inline bool isCarrot(BlockId id) { return id >= Blocks::Carrot0 && id <= Blocks::Carrot7; }
inline bool isPotato(BlockId id) { return id >= Blocks::Potato0 && id <= Blocks::Potato7; }
inline bool isBeetroot(BlockId id) { return id >= Blocks::Beetroot0 && id <= Blocks::Beetroot7; }

// The melon's stem. A crop in every way that matters -- it is sown,
// it ripens through eight stages and it is broken for its seed -- but
// what it ripens into is a block beside it rather than itself.
inline bool isMelonStem(BlockId id)
{
    return id >= Blocks::MelonStem0 && id <= Blocks::MelonStem7;
}
inline bool isPumpkinStem(BlockId id) { return id >= Blocks::PumpkinStem0 && id <= Blocks::PumpkinStem7; }
inline bool isAttachedStem(BlockId id)
{
    return (id >= Blocks::AttachedMelonStemFirst && id <= Blocks::AttachedMelonStemLast) ||
           (id >= Blocks::AttachedPumpkinStemFirst && id <= Blocks::AttachedPumpkinStemLast);
}
inline bool attachedStemIsPumpkin(BlockId id) { return id >= Blocks::AttachedPumpkinStemFirst && id <= Blocks::AttachedPumpkinStemLast; }
inline int attachedStemFacing(BlockId id)
{
    return attachedStemIsPumpkin(id) ? id - Blocks::AttachedPumpkinStemFirst : id - Blocks::AttachedMelonStemFirst;
}
inline BlockId attachedStemOf(bool pumpkin, int facing)
{
    return static_cast<BlockId>((pumpkin ? Blocks::AttachedPumpkinStemFirst : Blocks::AttachedMelonStemFirst) + (facing & 3));
}

// Any of the three. Everything that treats a crop as a crop -- growing
// it, harvesting it, bone meal, what it drops -- asks this rather than
// naming them one by one, so a fourth costs one line here and nothing
// anywhere else.
inline bool isCrop(BlockId id)
{
    return isWheat(id) || isCarrot(id) || isPotato(id) || isBeetroot(id) ||
           isMelonStem(id) || isPumpkinStem(id);
}

// Which of the eight stages a crop is at, whichever crop it is.
inline int cropStage(BlockId id)
{
    if (isWheat(id)) return id - Blocks::Wheat0;
    if (isCarrot(id)) return id - Blocks::Carrot0;
    if (isPotato(id)) return id - Blocks::Potato0;
    if (isBeetroot(id)) return id - Blocks::Beetroot0;
    if (isMelonStem(id)) return id - Blocks::MelonStem0;
    if (isPumpkinStem(id)) return id - Blocks::PumpkinStem0;
    return 0;
}

// The same crop at a given stage.
inline BlockId cropAtStage(BlockId id, int stage)
{
    if (stage < 0) stage = 0;
    if (stage > 7) stage = 7;
    if (isWheat(id)) return static_cast<BlockId>(Blocks::Wheat0 + stage);
    if (isCarrot(id)) return static_cast<BlockId>(Blocks::Carrot0 + stage);
    if (isPotato(id)) return static_cast<BlockId>(Blocks::Potato0 + stage);
    if (isBeetroot(id)) return static_cast<BlockId>(Blocks::Beetroot0 + stage);
    if (isMelonStem(id)) return static_cast<BlockId>(Blocks::MelonStem0 + stage);
    if (isPumpkinStem(id)) return static_cast<BlockId>(Blocks::PumpkinStem0 + stage);
    return id;
}
// A composter, and how full it is. Eight is full and ready to empty.
constexpr int COMPOST_FULL = 8;
inline bool isComposter(BlockId id)
{
    return id >= Blocks::Composter0 && id <= Blocks::Composter8;
}
inline int compostLevel(BlockId id)
{
    return isComposter(id) ? id - Blocks::Composter0 : 0;
}
inline BlockId composterAt(int level)
{
    if (level < 0) level = 0;
    if (level > COMPOST_FULL) level = COMPOST_FULL;
    return static_cast<BlockId>(Blocks::Composter0 + level);
}

// Cocoa, and how ripe it is. Three stages rather than eight, which is
// Minecraft's: it is a pod rather than a crop and grows on wood.
constexpr int COCOA_RIPE = 2;
inline bool isCocoa(BlockId id)
{
    return id >= Blocks::Cocoa0 && id <= Blocks::Cocoa2;
}
inline int cocoaStage(BlockId id) { return isCocoa(id) ? id - Blocks::Cocoa0 : 0; }
inline BlockId cocoaAt(int stage)
{
    if (stage < 0) stage = 0;
    if (stage > COCOA_RIPE) stage = COCOA_RIPE;
    return static_cast<BlockId>(Blocks::Cocoa0 + stage);
}

// A cake, and how many slices are gone. Seven is the last one: eat
// that and there is no cake.
constexpr int CAKE_SLICES = 7;
inline bool isCake(BlockId id)
{
    return id >= Blocks::Cake0 && id <= Blocks::Cake6;
}
inline int cakeEaten(BlockId id) { return isCake(id) ? id - Blocks::Cake0 : 0; }

// A lectern, with or without a book open on it. Like a chest, it does
// not say from outside what is in it -- the engine draws cubes, and
// Minecraft's book is a model rather than a texture.
inline bool isLectern(BlockId id)
{
    return id == Blocks::Lectern || id == Blocks::LecternBook;
}

// A cauldron, and how much water is in it. Three is full.
constexpr int CAULDRON_FULL = 3;
inline bool isCauldron(BlockId id)
{
    return id >= Blocks::Cauldron0 && id <= Blocks::Cauldron3;
}
inline int cauldronLevel(BlockId id)
{
    return isCauldron(id) ? id - Blocks::Cauldron0 : 0;
}
inline BlockId cauldronAt(int level)
{
    if (level < 0) level = 0;
    if (level > CAULDRON_FULL) level = CAULDRON_FULL;
    return static_cast<BlockId>(Blocks::Cauldron0 + level);
}
inline bool isPowderCauldron(BlockId id) { return id >= Blocks::PowderCauldronFirst && id <= Blocks::PowderCauldronLast; }
inline int powderCauldronLevel(BlockId id) { return isPowderCauldron(id) ? id - Blocks::PowderCauldronFirst + 1 : 0; }
inline BlockId powderCauldronAt(int level)
{
    if (level <= 0) return Blocks::Cauldron0;
    return static_cast<BlockId>(Blocks::PowderCauldronFirst + (level > CAULDRON_FULL ? CAULDRON_FULL : level) - 1);
}
inline bool isAnyCauldron(BlockId id) { return isCauldron(id) || id == Blocks::LavaCauldron || isPowderCauldron(id); }

// Grown as far as it goes, whichever crop it is.
inline bool isCropRipe(BlockId id)
{
    return isCrop(id) && cropStage(id) == WHEAT_STAGES - 1;
}
inline BlockId wheatAtStage(int stage) { return static_cast<BlockId>(Blocks::Wheat0 + (stage < 0 ? 0 : (stage > WHEAT_STAGES - 1 ? WHEAT_STAGES - 1 : stage))); }

inline bool isFlammable(BlockId id)
{
    if (isLog(id) || isPlanks(id) || isLeaves(id) || isWool(id) || isCarpet(id)) return true;
    if (isDoublePlant(id) || isHive(id) || isComposter(id)) return true;
    if (shapeWood(id) >= 0) return true;
    switch (id)
    {
        case Blocks::Bookshelf: case Blocks::HayBale: case Blocks::TallGrass: case Blocks::Fern:
        case Blocks::DeadBush: case Blocks::Vine: case Blocks::PlankSlab: case Blocks::OakFence:
        case Blocks::Tnt: case Blocks::Target: case Blocks::DriedKelpBlock: case Blocks::Scaffolding:
            return true;
        default:
            return false;
    }
}
