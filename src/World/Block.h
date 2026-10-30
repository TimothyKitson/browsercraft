#pragma once
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

        Count
    };
}

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
    Flat    // the same panel, for anything that is not a rail
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
};

const BlockInfo& blockInfo(BlockId id);

inline bool isAir(BlockId id) { return id == Blocks::Air; }
inline bool isOpaque(BlockId id) { return blockInfo(id).opaque; }
inline bool isSolid(BlockId id) { return blockInfo(id).collides; }
inline bool isLiquid(BlockId id) { return blockInfo(id).render == RenderType::Liquid; }
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
inline bool isSign(BlockId id)
{
    return id >= Blocks::SignNorth && id <= Blocks::SignWest;
}
inline int signFacing(BlockId id) { return isSign(id) ? id - Blocks::SignNorth : 0; }
inline BlockId signFacingAs(int facing)
{
    if (facing < 0) facing = 0;
    if (facing > 3) facing = 3;
    return static_cast<BlockId>(Blocks::SignNorth + facing);
}

// An item frame, and which way it faces. The same four as a sign, off
// the same arithmetic.
inline bool isFrame(BlockId id)
{
    return id >= Blocks::FrameNorth && id <= Blocks::FrameWest;
}
inline int frameFacing(BlockId id) { return isFrame(id) ? id - Blocks::FrameNorth : 0; }
inline BlockId frameFacingAs(int facing)
{
    if (facing < 0) facing = 0;
    if (facing > 3) facing = 3;
    return static_cast<BlockId>(Blocks::FrameNorth + facing);
}

// What you can go up the outside of. A ladder, and scaffolding, which
// is the only reason to build scaffolding.
inline bool isClimbable(BlockId id)
{
    return isLadder(id) || id == Blocks::Scaffolding;
}
inline bool isStairs(BlockId id) { return blockInfo(id).render == RenderType::Stairs; }
inline bool isFence(BlockId id) { return blockInfo(id).render == RenderType::Fence; }
inline bool isDoor(BlockId id) { return blockInfo(id).render == RenderType::Door; }
inline bool isRail(BlockId id) { return blockInfo(id).render == RenderType::Rail; }

// Pistons. Everything about one is read off its id: which way it
// points, whether it is sticky, and whether its head is out.
inline bool isPistonBody(BlockId id)
{
    return (id >= Blocks::PistonNorth && id <= Blocks::PistonOutWest) ||
           (id >= Blocks::StickyPistonNorth && id <= Blocks::StickyPistonOutWest);
}

inline bool isPistonHead(BlockId id)
{
    return (id >= Blocks::PistonHeadNorth && id <= Blocks::PistonHeadWest) ||
           (id >= Blocks::StickyPistonHeadNorth && id <= Blocks::StickyPistonHeadWest);
}

inline bool isPiston(BlockId id) { return isPistonBody(id) || isPistonHead(id); }

inline bool pistonSticky(BlockId id)
{
    return id >= Blocks::StickyPistonNorth;
}

inline bool pistonOut(BlockId id)
{
    return (id >= Blocks::PistonOutNorth && id <= Blocks::PistonOutWest) ||
           (id >= Blocks::StickyPistonOutNorth && id <= Blocks::StickyPistonOutWest);
}

inline int pistonFacing(BlockId id)
{
    return (id - Blocks::PistonNorth) & 3;
}

// The three parts of a piston, built back up from what it is.
inline BlockId pistonBody(int facing, bool sticky, bool out)
{
    const BlockId first = sticky ? (out ? Blocks::StickyPistonOutNorth
                                        : Blocks::StickyPistonNorth)
                                 : (out ? Blocks::PistonOutNorth : Blocks::PistonNorth);
    return static_cast<BlockId>(first + (facing & 3));
}

inline BlockId pistonHead(int facing, bool sticky)
{
    const BlockId first = sticky ? Blocks::StickyPistonHeadNorth : Blocks::PistonHeadNorth;
    return static_cast<BlockId>(first + (facing & 3));
}

inline bool isMonsterSpawner(BlockId id) { return id == Blocks::MonsterSpawner; }

// Whether this is something that grows in the water rather than in the
// air. The mesher needs it so the sea does not draw a face against
// every blade of seagrass in it, which would put a glassy box round
// each one; the player needs it so swimming through kelp does not read
// as coming up for air.
inline bool isWaterPlant(BlockId id)
{
    return id == Blocks::Kelp || id == Blocks::KelpTop ||
           id == Blocks::Seagrass || id == Blocks::SeaPickle ||
           (id >= Blocks::TubeCoral && id <= Blocks::HornCoral);
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
inline bool isContainerBlock(BlockId id)
{
    return id == Blocks::Chest || id == Blocks::Barrel || id == Blocks::ShulkerBox;
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
    return id == Blocks::Hopper || id == Blocks::Dropper || id == Blocks::Dispenser;
}

inline bool isNoteBlock(BlockId id)
{
    return id == Blocks::NoteBlock || id == Blocks::NoteBlockLit;
}

inline bool isObserver(BlockId id)
{
    return id == Blocks::Observer || id == Blocks::ObserverLit;
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

// A trial spawner in any of its three states: waiting, with its wave
// out, or finished with.
inline bool isTrialSpawner(BlockId id)
{
    return id == Blocks::TrialSpawner || id == Blocks::TrialSpawnerActive ||
           id == Blocks::TrialSpawnerSpent;
}

// Copper in any of its four stages, which is the set that weathers.
inline bool isCopper(BlockId id)
{
    return id >= Blocks::CopperBlock && id <= Blocks::OxidizedCopper;
}

inline bool isPointedDripstone(BlockId id)
{
    return id == Blocks::DripstoneDown || id == Blocks::DripstoneUp;
}

inline bool isCopperBulb(BlockId id)
{
    return id == Blocks::CopperBulb || id == Blocks::CopperBulbLit;
}

inline bool isCrafter(BlockId id)
{
    return id == Blocks::Crafter || id == Blocks::CrafterLit;
}

inline bool isVault(BlockId id)
{
    return id == Blocks::Vault || id == Blocks::VaultOpen ||
           id == Blocks::OminousVault || id == Blocks::OminousVaultOpen;
}

inline bool isOminousVault(BlockId id)
{
    return id == Blocks::OminousVault || id == Blocks::OminousVaultOpen;
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
           id == Blocks::AcaciaPlanks;
}

inline bool isLog(BlockId id)
{
    return id == Blocks::Log || id == Blocks::BirchLog ||
           id == Blocks::SpruceLog || id == Blocks::DarkOakLog ||
           id == Blocks::JungleLog || id == Blocks::AcaciaLog;
}

// The planks a log breaks down into.
inline BlockId planksFromLog(BlockId log)
{
    switch (log)
    {
        case Blocks::BirchLog:   return Blocks::Planks;   // birch has no plank of its own yet
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
        default: return false;
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

// The three things a door id carries, and how to build one back up.
inline bool doorIsTop(BlockId id) { return id >= Blocks::DoorHighShutNorth; }
inline bool doorIsOpen(BlockId id)
{
    return ((id - Blocks::DoorLowShutNorth) & 4) != 0;
}
inline BlockId doorPart(int facing, bool top, bool open)
{
    return static_cast<BlockId>(Blocks::DoorLowShutNorth + (top ? 8 : 0) +
                                (open ? 4 : 0) + (facing & 3));
}

// How far above its own cell a block stands. A fence is half a block
// taller than the cell it sits in, which is what stops you stepping
// over one; everything else stops at the top of its cell.
inline float blockOverhang(BlockId id) { return isFence(id) ? 0.5f : 0.0f; }

// Which way a facing block points: 0 north (-z), 1 east (+x), 2 south
// (+z), 3 west (-x). Meaningless on anything that does not face.
inline int blockFacing(BlockId id)
{
    if (isLadder(id)) return id - Blocks::LadderNorth;
    if (isSign(id)) return signFacing(id);
    if (isFrame(id)) return frameFacing(id);
    if (isStairs(id)) return (id - Blocks::StoneStairsNorth) & 3;
    if (isDoor(id)) return (id - Blocks::DoorLowShutNorth) & 3;
    if (isPiston(id)) return pistonFacing(id);
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
    const int base = ((stairs - Blocks::StoneStairsNorth) & ~3) + Blocks::StoneStairsNorth;
    return static_cast<BlockId>(base + (facing & 3));
}

// How tall the block actually stands in its own cell. Everything is a
// full block but the slabs, and the collision and the mesher both read
// this rather than each keeping their own list.
inline float blockHeight(BlockId id) { return isSlab(id) ? 0.5f : 1.0f; }

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
                   float maxX, float maxY, float maxZ);
inline bool isLeaves(BlockId id)
{
    return id == Blocks::Leaves || id == Blocks::BirchLeaves ||
           id == Blocks::SpruceLeaves || id == Blocks::DarkOakLeaves ||
           id == Blocks::JungleLeaves || id == Blocks::AcaciaLeaves ||
           id == Blocks::AzaleaLeaves;
}
inline bool isVisible(BlockId id) { return blockInfo(id).render != RenderType::None; }
inline uint8_t lightEmission(BlockId id) { return blockInfo(id).lightEmission; }

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

// Any of the three. Everything that treats a crop as a crop -- growing
// it, harvesting it, bone meal, what it drops -- asks this rather than
// naming them one by one, so a fourth costs one line here and nothing
// anywhere else.
inline bool isCrop(BlockId id)
{
    return isWheat(id) || isCarrot(id) || isPotato(id) || isBeetroot(id) ||
           isMelonStem(id);
}

// Which of the eight stages a crop is at, whichever crop it is.
inline int cropStage(BlockId id)
{
    if (isWheat(id)) return id - Blocks::Wheat0;
    if (isCarrot(id)) return id - Blocks::Carrot0;
    if (isPotato(id)) return id - Blocks::Potato0;
    if (isBeetroot(id)) return id - Blocks::Beetroot0;
    if (isMelonStem(id)) return id - Blocks::MelonStem0;
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
    return id;
}
// The three machines that smelt. They are one block as far as
// everything that opens, ticks or lights one is concerned; which of
// them it is decides only what it will take and how fast.
inline bool isFurnaceBlock(BlockId id)
{
    return id == Blocks::Furnace || id == Blocks::FurnaceLit ||
           id == Blocks::Smoker || id == Blocks::SmokerLit ||
           id == Blocks::BlastFurnace || id == Blocks::BlastFurnaceLit;
}
inline BlockId furnaceLit(BlockId id)
{
    if (id == Blocks::Furnace || id == Blocks::FurnaceLit) return Blocks::FurnaceLit;
    if (id == Blocks::Smoker || id == Blocks::SmokerLit) return Blocks::SmokerLit;
    if (id == Blocks::BlastFurnace || id == Blocks::BlastFurnaceLit)
        return Blocks::BlastFurnaceLit;
    return id;
}
inline BlockId furnaceCold(BlockId id)
{
    if (id == Blocks::Furnace || id == Blocks::FurnaceLit) return Blocks::Furnace;
    if (id == Blocks::Smoker || id == Blocks::SmokerLit) return Blocks::Smoker;
    if (id == Blocks::BlastFurnace || id == Blocks::BlastFurnaceLit)
        return Blocks::BlastFurnace;
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

// Grown as far as it goes, whichever crop it is.
inline bool isCropRipe(BlockId id)
{
    return isCrop(id) && cropStage(id) == WHEAT_STAGES - 1;
}
inline BlockId wheatAtStage(int stage) { return static_cast<BlockId>(Blocks::Wheat0 + (stage < 0 ? 0 : (stage > WHEAT_STAGES - 1 ? WHEAT_STAGES - 1 : stage))); }
