#include "Block.h"
#include <string>
#include "Renderer/AtlasTiles.h"

using namespace Tiles;

namespace
{
    // Order must match the Blocks enum exactly.
    const BlockInfo TABLE[Blocks::Count] = {
        //  name            top            bottom          side           collide opaque render               light hardness
        { "Air",          Blank,         Blank,          Blank,          false, false, RenderType::None,   0,  -1.0f },
        { "Stone",        Stone,         Stone,          Stone,          true,  true,  RenderType::Cube,   0,   1.5f },
        { "Grass Block",  GrassTop,      Dirt,           GrassSide,      true,  true,  RenderType::Cube,   0,   0.6f },
        { "Dirt",         Dirt,          Dirt,           Dirt,           true,  true,  RenderType::Cube,   0,   0.5f },
        { "Cobblestone",  Cobblestone,   Cobblestone,    Cobblestone,    true,  true,  RenderType::Cube,   0,   1.7f },
        { "Planks",       Planks,        Planks,         Planks,         true,  true,  RenderType::Cube,   0,   1.5f },
        { "Bedrock",      Bedrock,       Bedrock,        Bedrock,        true,  true,  RenderType::Cube,   0,  -1.0f },
        { "Water",        Water,         Water,          WaterFlow,      false, false, RenderType::Liquid, 0,  -1.0f },
        { "Sand",         Sand,          Sand,           Sand,           true,  true,  RenderType::Cube,   0,   0.5f },
        { "Gravel",       Gravel,        Gravel,         Gravel,         true,  true,  RenderType::Cube,   0,   0.6f },
        { "Gold Ore",     GoldOre,       GoldOre,        GoldOre,        true,  true,  RenderType::Cube,   0,   3.0f },
        { "Iron Ore",     IronOre,       IronOre,        IronOre,        true,  true,  RenderType::Cube,   0,   3.0f },
        { "Coal Ore",     CoalOre,       CoalOre,        CoalOre,        true,  true,  RenderType::Cube,   0,   2.5f },
        { "Diamond Ore",  DiamondOre,    DiamondOre,     DiamondOre,     true,  true,  RenderType::Cube,   0,   3.5f },
        { "Oak Log",      LogTop,        LogTop,         LogSide,        true,  true,  RenderType::Cube,   0,   2.0f },
        { "Oak Leaves",   Leaves,        Leaves,         Leaves,         true,  false, RenderType::Cube,   0,   0.2f },
        { "Glass",        Glass,         Glass,          Glass,          true,  false, RenderType::Cube,   0,   0.3f },
        { "Sandstone",    SandstoneTop,  SandstoneBottom, SandstoneSide,  true,  true,  RenderType::Cube,   0,   0.8f },
        { "Snow",         SnowTile,      SnowTile,       SnowTile,       true,  true,  RenderType::Cube,   0,   0.3f },
        { "Ice",          Ice,           Ice,            Ice,            true,  false, RenderType::Cube,   0,   0.5f },
        { "Cactus",       CactusTop,     CactusBottom,   CactusSide,     true,  true,  RenderType::Cube,   0,   0.4f },
        { "Bricks",       Bricks,        Bricks,         Bricks,         true,  true,  RenderType::Cube,   0,   2.0f },
        { "Obsidian",     Obsidian,      Obsidian,       Obsidian,       true,  true,  RenderType::Cube,   0,   8.0f },
        { "Short Grass",  TallGrass,     TallGrass,      TallGrass,      false, false, RenderType::Cross,  0,   0.0f },
        { "Poppy",        FlowerRed,     FlowerRed,      FlowerRed,      false, false, RenderType::Cross,  0,   0.0f },
        { "Dandelion",    FlowerYellow,  FlowerYellow,   FlowerYellow,   false, false, RenderType::Cross,  0,   0.0f },
        { "Mossy Cobble", MossyCobble,   MossyCobble,    MossyCobble,    true,  true,  RenderType::Cube,   0,   1.7f },
        { "Clay",         Clay,          Clay,           Clay,           true,  true,  RenderType::Cube,   0,   0.6f },
        { "Pumpkin",      PumpkinTop,    PumpkinTop,     PumpkinSide,    true,  true,  RenderType::Cube,   0,   1.0f },
        { "Wool",         Wool,          Wool,           Wool,           true,  true,  RenderType::Cube,   0,   0.8f },
        { "Torch",        Torch,         Torch,          Torch,          false, false, RenderType::Cross, 14,   0.0f },
        { "Birch Log",    BirchLogTop,   BirchLogTop,    BirchLogSide,   true,  true,  RenderType::Cube,   0,   2.0f },
        { "Birch Leaves", BirchLeaves,   BirchLeaves,    BirchLeaves,    true,  false, RenderType::Cube,   0,   0.2f },
        { "Glowstone",    Glowstone,     Glowstone,      Glowstone,      true,  true,  RenderType::Cube,  15,   0.3f },
        { "Lava",         Lava,          Lava,           LavaFlow,       false, false, RenderType::Liquid,15,  -1.0f },
        { "Snowy Grass",  SnowTile,      Dirt,           SnowGrassSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Netherrack",   Netherrack,    Netherrack,     Netherrack,     true,  true,  RenderType::Cube,   0,   0.4f },
        { "Soul Sand",    SoulSand,      SoulSand,       SoulSand,       true,  true,  RenderType::Cube,   0,   0.5f },
        { "End Stone",    EndStone,      EndStone,       EndStone,       true,  true,  RenderType::Cube,   0,   3.0f },
        // The portal surface is walked into rather than stood on, so it does
        // not collide, and it glows enough to light its own frame.
        { "Nether Portal",NetherPortal,  NetherPortal,   NetherPortal,   false, false, RenderType::Liquid, 11, -1.0f },

        // Tilled earth is solid and a shade darker than dirt; the crop
        // standing in it is a cross-shaped plant you can walk through.
        { "Crafting Table", CraftingTop, Planks,     CraftingSide,   true,  true,  RenderType::Cube,   0,   2.5f, CraftingFront },
        { "Furnace",      FurnaceTop,    FurnaceTop,     FurnaceSide,    true,  true,  RenderType::Cube,   0,   3.5f, FurnaceFront },
        { "Furnace",      FurnaceTop,    FurnaceTop,     FurnaceSide,    true,  true,  RenderType::Cube,  13,   3.5f, FurnaceLit },
        { "Chest",        ChestTileFirst + 0, ChestTileFirst + 4, ChestTileFirst + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Bed",          BedTop,        Planks,         BedSide,        true,  true,  RenderType::Cube,   0,   0.2f },
        { "TNT",          TntTop,        TntBottom,      TntSide,        true,  true,  RenderType::Cube,   0,   0.0f },
        { "TNT",          TntTop,        TntBottom,      TntPrimedSide,  true,  true,  RenderType::Cube,   7,   0.0f },
        { "Ladder",       LadderTile,    LadderTile,     LadderTile,     false, false, RenderType::Ladder, 0,  0.4f },
        { "Ladder",       LadderTile,    LadderTile,     LadderTile,     false, false, RenderType::Ladder, 0,  0.4f },
        { "Ladder",       LadderTile,    LadderTile,     LadderTile,     false, false, RenderType::Ladder, 0,  0.4f },
        { "Ladder",       LadderTile,    LadderTile,     LadderTile,     false, false, RenderType::Ladder, 0,  0.4f },
        { "Stone Stairs",         Stone,          Stone,           Stone,           true,  false, RenderType::Stairs, 0,   1.5f },
        { "Stone Stairs",         Stone,          Stone,           Stone,           true,  false, RenderType::Stairs, 0,   1.5f },
        { "Stone Stairs",         Stone,          Stone,           Stone,           true,  false, RenderType::Stairs, 0,   1.5f },
        { "Stone Stairs",         Stone,          Stone,           Stone,           true,  false, RenderType::Stairs, 0,   1.5f },
        { "Cobblestone Stairs",   Cobblestone,    Cobblestone,     Cobblestone,     true,  false, RenderType::Stairs, 0,   1.7f },
        { "Cobblestone Stairs",   Cobblestone,    Cobblestone,     Cobblestone,     true,  false, RenderType::Stairs, 0,   1.7f },
        { "Cobblestone Stairs",   Cobblestone,    Cobblestone,     Cobblestone,     true,  false, RenderType::Stairs, 0,   1.7f },
        { "Cobblestone Stairs",   Cobblestone,    Cobblestone,     Cobblestone,     true,  false, RenderType::Stairs, 0,   1.7f },
        { "Oak Stairs",           Planks,         Planks,          Planks,          true,  false, RenderType::Stairs, 0,   1.5f },
        { "Oak Stairs",           Planks,         Planks,          Planks,          true,  false, RenderType::Stairs, 0,   1.5f },
        { "Oak Stairs",           Planks,         Planks,          Planks,          true,  false, RenderType::Stairs, 0,   1.5f },
        { "Oak Stairs",           Planks,         Planks,          Planks,          true,  false, RenderType::Stairs, 0,   1.5f },
        { "Sandstone Stairs",     SandstoneTop,   SandstoneTop,    SandstoneSide,   true,  false, RenderType::Stairs, 0,   0.8f },
        { "Sandstone Stairs",     SandstoneTop,   SandstoneTop,    SandstoneSide,   true,  false, RenderType::Stairs, 0,   0.8f },
        { "Sandstone Stairs",     SandstoneTop,   SandstoneTop,    SandstoneSide,   true,  false, RenderType::Stairs, 0,   0.8f },
        { "Sandstone Stairs",     SandstoneTop,   SandstoneTop,    SandstoneSide,   true,  false, RenderType::Stairs, 0,   0.8f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorLower,    DoorLower,     DoorLower,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Door",     DoorUpper,    DoorUpper,     DoorUpper,     true,  false, RenderType::Door,   0,   3.0f },
        { "Oak Fence",    Planks,        Planks,         Planks,         true,  false, RenderType::Fence,  0,   1.5f },
        { "Stone Slab",   Stone,         Stone,          Stone,          true,  false, RenderType::Slab,   0,   1.5f },
        { "Cobblestone Slab", Cobblestone, Cobblestone,  Cobblestone,    true,  false, RenderType::Slab,   0,   1.7f },
        { "Oak Slab",     Planks,        Planks,         Planks,         true,  false, RenderType::Slab,   0,   1.5f },
        { "Sandstone Slab", SandstoneTop, SandstoneTop,  SandstoneSide,  true,  false, RenderType::Slab,   0,   0.8f },
        { "Farmland",     FarmlandTop,   Dirt,           FarmlandSide,   true,  true,  RenderType::Cube,   0,   0.6f },
        { "Wheat Crop",   WheatStage0,   WheatStage0,    WheatStage0,    false, false, RenderType::Cross,  0,   0.0f },
        { "Wheat Crop",   WheatStage1,   WheatStage1,    WheatStage1,    false, false, RenderType::Cross,  0,   0.0f },
        { "Wheat Crop",   WheatStage2,   WheatStage2,    WheatStage2,    false, false, RenderType::Cross,  0,   0.0f },
        { "Wheat Crop",   WheatStage3,   WheatStage3,    WheatStage3,    false, false, RenderType::Cross,  0,   0.0f },
        { "Wheat Crop",   WheatStage4,   WheatStage4,    WheatStage4,    false, false, RenderType::Cross,  0,   0.0f },
        { "Wheat Crop",   WheatStage5,   WheatStage5,    WheatStage5,    false, false, RenderType::Cross,  0,   0.0f },
        { "Wheat Crop",   WheatStage6,   WheatStage6,    WheatStage6,    false, false, RenderType::Cross,  0,   0.0f },
        { "Wheat Crop",   WheatStage7,   WheatStage7,    WheatStage7,    false, false, RenderType::Cross,  0,   0.0f },
        { "Sugar Cane",   SugarCane,     SugarCane,      SugarCane,      false, false, RenderType::Cross,  0,   0.0f },
        { "Bookshelf",    Planks,        Planks,         BookshelfSide,  true,  true,  RenderType::Cube,   0,   1.5f },
        { "Enchanting Table", EnchantTableTop, EnchantTableBottom,   EnchantTableSide, true, false, RenderType::Slab, 7,   5.0f },
        { "Emerald Ore",  EmeraldOre,    EmeraldOre,     EmeraldOre,     true,  true,  RenderType::Cube,   0,   3.0f },
        { "End Portal Frame", EndFrameTop, EndStone,      EndFrameSide,   true,  false, RenderType::Slab,   1,  -1.0f },
        { "End Portal Frame", EndFrameEye, EndStone,      EndFrameSide,   true,  false, RenderType::Slab,   6,  -1.0f },
        { "End Portal",   EndPortalTile, EndPortalTile,  EndPortalTile,  false, false, RenderType::Cube,  11,  -1.0f },
        { "Anvil",        AnvilTop,      AnvilTop,       AnvilSide,      true,  false, RenderType::Slab,   0,   5.0f },
        { "Nether Wart",  NetherWartTile, NetherWartTile, NetherWartTile, false, false, RenderType::Cross,  0,   0.0f },
        { "Brewing Stand", BrewingStandTile, BrewingStandTile, BrewingStandTile, false, false, RenderType::Cross, 1, 0.5f },
        { "Dirt Path",    DirtPathTop,   Dirt,           DirtPathSide,   true,  true,  RenderType::Cube,   0,   0.6f },
        { "Rail",         RailTile,      RailTile,       RailTile,       false, false, RenderType::Rail,   0,   0.7f },
        { "Rail",         RailTile,      RailTile,       RailTile,       false, false, RenderType::Rail,   0,   0.7f },
        { "Rail",         RailCorner,    RailCorner,     RailCorner,     false, false, RenderType::Rail,   0,   0.7f },
        { "Rail",         RailCorner,    RailCorner,     RailCorner,     false, false, RenderType::Rail,   0,   0.7f },
        { "Rail",         RailCorner,    RailCorner,     RailCorner,     false, false, RenderType::Rail,   0,   0.7f },
        { "Rail",         RailCorner,    RailCorner,     RailCorner,     false, false, RenderType::Rail,   0,   0.7f },
        { "Granite",      Granite,       Granite,        Granite,        true,  true,  RenderType::Cube,   0,   1.5f },
        { "Diorite",      Diorite,       Diorite,        Diorite,        true,  true,  RenderType::Cube,   0,   1.5f },
        { "Andesite",     Andesite,      Andesite,       Andesite,       true,  true,  RenderType::Cube,   0,   1.5f },
        { "Deepslate",    DeepslateTop,  DeepslateTop,   Deepslate,      true,  true,  RenderType::Cube,   0,   3.0f },
        { "Tuff",         Tuff,          Tuff,           Tuff,           true,  true,  RenderType::Cube,   0,   1.5f },
        { "Calcite",      Calcite,       Calcite,        Calcite,        true,  true,  RenderType::Cube,   0,   0.75f },
        { "Spruce Log",   SpruceLogTop,  SpruceLogTop,   SpruceLogSide,  true,  true,  RenderType::Cube,   0,   2.0f },
        { "Spruce Leaves",SpruceLeaves,  SpruceLeaves,   SpruceLeaves,   true,  false, RenderType::Cube,   0,   0.2f },
        { "Spruce Planks",SprucePlanks,  SprucePlanks,   SprucePlanks,   true,  true,  RenderType::Cube,   0,   1.5f },
        { "Dark Oak Log", DarkOakLogTop, DarkOakLogTop,  DarkOakLogSide, true,  true,  RenderType::Cube,   0,   2.0f },
        { "Dark Oak Leaves",DarkOakLeaves,DarkOakLeaves, DarkOakLeaves,  true,  false, RenderType::Cube,   0,   0.2f },
        { "Dark Oak Planks",DarkOakPlanks,DarkOakPlanks, DarkOakPlanks,  true,  true,  RenderType::Cube,   0,   1.5f },
        { "Nether Bricks",NetherBricks,  NetherBricks,   NetherBricks,   true,  true,  RenderType::Cube,   0,   2.0f },
        { "Jungle Log",   JungleLogTop,  JungleLogTop,   JungleLogSide,  true,  true,  RenderType::Cube,   0,   2.0f },
        { "Jungle Leaves",JungleLeaves,  JungleLeaves,   JungleLeaves,   true,  false, RenderType::Cube,   0,   0.2f },
        { "Jungle Planks",JunglePlanks,  JunglePlanks,   JunglePlanks,   true,  true,  RenderType::Cube,   0,   1.5f },
        { "Acacia Log",   AcaciaLogTop,  AcaciaLogTop,   AcaciaLogSide,  true,  true,  RenderType::Cube,   0,   2.0f },
        { "Acacia Leaves",AcaciaLeaves,  AcaciaLeaves,   AcaciaLeaves,   true,  false, RenderType::Cube,   0,   0.2f },
        { "Acacia Planks",AcaciaPlanks,  AcaciaPlanks,   AcaciaPlanks,   true,  true,  RenderType::Cube,   0,   1.5f },
        { "Redstone Ore", RedstoneOre,   RedstoneOre,    RedstoneOre,    true,  true,  RenderType::Cube,   0,   3.0f },
        { "Redstone",     RedstoneDust,  RedstoneDust,   RedstoneDust,   false, false, RenderType::Flat,   0,   0.0f },
        { "Redstone",     RedstoneDustLit,RedstoneDustLit,RedstoneDustLit,false,false, RenderType::Flat,   0,   0.0f },
        { "Redstone Torch",RedstoneTorch,RedstoneTorch,  RedstoneTorch,  false, false, RenderType::Cross,  7,   0.0f },
        { "Redstone Torch",RedstoneTorchOff,RedstoneTorchOff,RedstoneTorchOff,false,false,RenderType::Cross,0, 0.0f },
        { "Lever",        LeverTile,     LeverTile,      LeverTile,      false, false, RenderType::Cross,  0,   0.5f },
        { "Lever",        LeverOnTile,   LeverOnTile,    LeverOnTile,    false, false, RenderType::Cross,  0,   0.5f },
        { "Button",       ButtonTile,    ButtonTile,     ButtonTile,     false, false, RenderType::Cross,  0,   0.5f },
        { "Button",       ButtonOnTile,  ButtonOnTile,   ButtonOnTile,   false, false, RenderType::Cross,  0,   0.5f },
        { "Redstone Lamp",RedstoneLamp,  RedstoneLamp,   RedstoneLamp,   true,  true,  RenderType::Cube,   0,   0.3f },
        { "Redstone Lamp",RedstoneLampLit,RedstoneLampLit,RedstoneLampLit,true, true,  RenderType::Cube,  15,   0.3f },
        { "Block of Redstone",RedstoneBlockTile,RedstoneBlockTile,RedstoneBlockTile,true,true,RenderType::Cube,0,5.0f },
        { "Tuff Bricks",  TuffBricks,    TuffBricks,     TuffBricks,     true,  true,  RenderType::Cube,   0,   1.5f },
        { "Trial Spawner",TrialSpawnerTop,TrialSpawnerBottom,TrialSpawnerSide,true, true,  RenderType::Cube,   3,  -1.0f },
        { "Trial Spawner",TrialSpawnerTopLit,TrialSpawnerBottom,TrialSpawnerSideLit,true,true,RenderType::Cube,8,-1.0f },
        { "Trial Spawner",TrialSpawnerTop,TrialSpawnerBottom,TrialSpawnerSide,true, true,  RenderType::Cube,   0,  -1.0f },
        { "Vault",        VaultTop,      VaultBottom,    VaultSideOff,   true,  true,  RenderType::Cube,   4,  -1.0f, VaultSide },
        { "Vault",        VaultTop,      VaultBottom,    VaultSideOn,    true,  true,  RenderType::Cube,   8,  -1.0f, VaultSideOpen },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, PistonFace },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Sticky Piston",  PistonTop,     PistonTop,      PistonSide,     true,  true,  RenderType::Cube,   0,   0.5f, PistonBack },
        { "Sticky Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Sticky Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Sticky Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Sticky Piston Head",PistonTop, PistonTop,  PistonArm,      true,  true,  RenderType::Cube,   0,   0.5f, StickyPistonFace },
        { "Crafter",      CrafterNorth,  CrafterSouth,   CrafterSide,    true,  true,  RenderType::Cube,   0,   2.5f, CrafterTop, CrafterBottom },
        { "Crafter",      CrafterNorthCrafting, CrafterSouthTriggered, CrafterEastTriggered, true, true, RenderType::Cube, 6, 2.5f, CrafterTopLit, CrafterBottom },
        { "Copper Ore",   CopperOre,     CopperOre,      CopperOre,      true,  true,  RenderType::Cube,   0,   3.0f },
        { "Block of Copper",CopperBlock, CopperBlock,    CopperBlock,    true,  true,  RenderType::Cube,   0,   3.0f },
        { "Exposed Copper",ExposedCopper,ExposedCopper,  ExposedCopper,  true,  true,  RenderType::Cube,   0,   3.0f },
        { "Weathered Copper",WeatheredCopper,WeatheredCopper,WeatheredCopper,true,true,RenderType::Cube,   0,   3.0f },
        { "Oxidized Copper",OxidizedCopper,OxidizedCopper,OxidizedCopper,true,  true,  RenderType::Cube,   0,   3.0f },
        { "Copper Bulb",  CopperBulbTile,CopperBulbTile, CopperBulbTile, true,  true,  RenderType::Cube,   0,   3.0f },
        { "Copper Bulb",  CopperBulbLitTile,CopperBulbLitTile,CopperBulbLitTile,true,true,RenderType::Cube,15,3.0f },
        { "Dripstone Block",DripstoneBlock,DripstoneBlock, DripstoneBlock, true,  true,  RenderType::Cube,   0,   1.5f },
        { "Pointed Dripstone",DripstoneSpike,DripstoneSpike,DripstoneSpike,false,false, RenderType::Cross,  0,   0.5f },
        { "Pointed Dripstone",DripstoneSpike,DripstoneSpike,DripstoneSpike,false,false, RenderType::Cross,  0,   0.5f },
        { "Moss Block",   MossTile,      MossTile,       MossTile,       true,  true,  RenderType::Cube,   0,   0.1f },
        { "Azalea Leaves",AzaleaLeavesTile,AzaleaLeavesTile,AzaleaLeavesTile,true,false,RenderType::Cube,   0,   0.2f },
        { "Cave Vines",   GlowBerriesTile,GlowBerriesTile,GlowBerriesTile,false, false, RenderType::Cross, 14,   0.0f },
        { "Ominous Vault",VaultTopOminous,VaultBottomOminous,VaultSideOffOminous,true, true,  RenderType::Cube,   6,  -1.0f, OminousVaultSide },
        { "Ominous Vault",VaultTopOminous,VaultBottomOminous,VaultSideOnOminous,true,true,RenderType::Cube,  10,  -1.0f, OminousVaultSideOpen },
        { "Monster Spawner",SpawnerTile, SpawnerTile,    SpawnerTile,    true,  false, RenderType::Cube,   1,   5.0f },
        { "Prismarine",   PrismarineTile,PrismarineTile, PrismarineTile, true,  true,  RenderType::Cube,   0,   1.5f },
        { "Prismarine Bricks",PrismarineBricksTile,PrismarineBricksTile,PrismarineBricksTile,true,true,RenderType::Cube,0,1.5f },
        { "Dark Prismarine",DarkPrismarineTile,DarkPrismarineTile,DarkPrismarineTile,true,true,RenderType::Cube,0,1.5f },
        { "Sea Lantern",  SeaLanternTile,SeaLanternTile, SeaLanternTile, true,  true,  RenderType::Cube,  15,   0.3f },
        { "Sponge",       SpongeTile,    SpongeTile,     SpongeTile,     true,  true,  RenderType::Cube,   0,   0.6f },
        { "Wet Sponge",   WetSpongeTile, WetSpongeTile,  WetSpongeTile,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Kelp",         KelpTile,      KelpTile,       KelpTile,       false, false, RenderType::Cross,  0,   0.0f },
        { "Kelp",         KelpTopTile,   KelpTopTile,    KelpTopTile,    false, false, RenderType::Cross,  0,   0.0f },
        { "Seagrass",     SeagrassTile,  SeagrassTile,   SeagrassTile,   false, false, RenderType::Cross,  0,   0.0f },
        { "Sea Pickle",   SeaPickleTile, SeaPickleTile,  SeaPickleTile,  false, false, RenderType::Cross,  6,   0.0f },
        { "Tube Coral Block",TubeCoralBlockTile,TubeCoralBlockTile,TubeCoralBlockTile,true,true,RenderType::Cube,0,1.5f },
        { "Brain Coral Block",BrainCoralBlockTile,BrainCoralBlockTile,BrainCoralBlockTile,true,true,RenderType::Cube,0,1.5f },
        { "Bubble Coral Block",BubbleCoralBlockTile,BubbleCoralBlockTile,BubbleCoralBlockTile,true,true,RenderType::Cube,0,1.5f },
        { "Fire Coral Block",FireCoralBlockTile,FireCoralBlockTile,FireCoralBlockTile,true,true,RenderType::Cube,0,1.5f },
        { "Horn Coral Block",HornCoralBlockTile,HornCoralBlockTile,HornCoralBlockTile,true,true,RenderType::Cube,0,1.5f },
        { "Tube Coral",   TubeCoralTile, TubeCoralTile,  TubeCoralTile,  false, false, RenderType::Cross,  0,   0.0f },
        { "Brain Coral",  BrainCoralTile,BrainCoralTile, BrainCoralTile, false, false, RenderType::Cross,  0,   0.0f },
        { "Bubble Coral", BubbleCoralTile,BubbleCoralTile,BubbleCoralTile,false,false, RenderType::Cross,  0,   0.0f },
        { "Fire Coral",   FireCoralTile, FireCoralTile,  FireCoralTile,  false, false, RenderType::Cross,  0,   0.0f },
        { "Horn Coral",   HornCoralTile, HornCoralTile,  HornCoralTile,  false, false, RenderType::Cross,  0,   0.0f },
        { "Deepslate Bricks",DeepslateBricksTile,DeepslateBricksTile,DeepslateBricksTile,true,true,RenderType::Cube,0,3.5f },
        { "Deepslate Tiles",DeepslateTilesTile,DeepslateTilesTile,DeepslateTilesTile,true,true,RenderType::Cube,0,3.5f },
        { "Sculk",        SculkTile,     SculkTile,      SculkTile,      true,  true,  RenderType::Cube,   0,   0.2f },
        { "Sculk Sensor", SculkSensorTop,SculkSensorBottom,SculkSensorSide,true,  true,  RenderType::Cube,   1,   1.5f },
        { "Sculk Sensor", SculkSensorTopLit,SculkSensorBottom,SculkSensorSide,true,  true,  RenderType::Cube,   6,   1.5f },
        { "Sculk Shrieker",SculkShriekerTop,SculkShriekerBottom,   SculkShriekerSide,true,true,  RenderType::Cube,   2,   3.0f },
        { "Sculk Catalyst",SculkCatalystTop,SculkCatalystBottom,   SculkCatalystSide,true,true,  RenderType::Cube,   6,   3.0f },
        { "Soul Lantern", SoulLanternTile,SoulLanternTile,SoulLanternTile,true, true,  RenderType::Cube,  10,   0.3f },
        { "Reinforced Deepslate",ReinforcedDeepslateTop,ReinforcedDeepslateBottom,ReinforcedDeepslateSide,true,true,RenderType::Cube,0,-1.0f },
        { "Purpur Block", PurpurTile,    PurpurTile,     PurpurTile,     true,  true,  RenderType::Cube,   0,   1.5f },
        { "Purpur Pillar",PurpurPillarTop,PurpurPillarTop,PurpurPillarSide,true,true,  RenderType::Cube,   0,   1.5f },
        { "End Stone Bricks",EndBricksTile,EndBricksTile,EndBricksTile,  true,  true,  RenderType::Cube,   0,   3.0f },
        { "Chorus Plant", ChorusPlantTile,ChorusPlantTile,ChorusPlantTile,true, true,  RenderType::Cube,   0,   0.4f },
        { "Chorus Flower",ChorusFlowerTile,ChorusFlowerTile,ChorusFlowerTile,true,true, RenderType::Cube,  0,   0.4f },
        { "Blackstone",   BlackstoneTop, BlackstoneTop,  BlackstoneTile, true,  true,  RenderType::Cube,   0,   1.5f },
        { "Polished Blackstone Bricks",PolishedBlackstoneTile,PolishedBlackstoneTile,PolishedBlackstoneTile,true,true,RenderType::Cube,0,2.0f },
        { "Gilded Blackstone",GildedBlackstoneTile,GildedBlackstoneTile,GildedBlackstoneTile,true,true,RenderType::Cube,0,1.5f },
        { "Basalt",       BasaltTop,     BasaltTop,      BasaltSide,     true,  true,  RenderType::Cube,   0,   1.25f },
        { "Lily Pad",     LilyPadTile,   LilyPadTile,    LilyPadTile,    false, false, RenderType::Flat,   0,   0.0f },
        { "Vines",        VineTile,      VineTile,       VineTile,       false, false, RenderType::Cross,  0,   0.2f },
        { "Mud",          MudTile,       MudTile,        MudTile,        true,  true,  RenderType::Cube,   0,   0.5f },
        { "End Crystal",  EndCrystalTile,EndCrystalTile, EndCrystalTile, false, false, RenderType::Cube,  15,   0.0f },
        { "Crystal Base", Bedrock,       Bedrock,        Bedrock,        true,  true,  RenderType::Cube,   0,  -1.0f },
        { "Wither Skeleton Skull",WitherSkullTile,WitherSkullTile,WitherSkullTile,true,true,RenderType::Cube,0,1.0f },
        { "Beacon",       BeaconTile,    BeaconTile,     BeaconTile,     true,  true,  RenderType::Cube,  15,   3.0f },
        { "Block of Iron",IronBlockTile, IronBlockTile,  IronBlockTile,  true,  true,  RenderType::Cube,   0,   5.0f },
        { "Block of Gold",GoldBlockTile, GoldBlockTile,  GoldBlockTile,  true,  true,  RenderType::Cube,   0,   3.0f },
        { "Block of Diamond",DiamondBlockTile,DiamondBlockTile,DiamondBlockTile,true,true,RenderType::Cube,0,5.0f },
        { "Block of Emerald",EmeraldBlockTile,EmeraldBlockTile,EmeraldBlockTile,true,true,RenderType::Cube,0,5.0f },
        { "Block of Coal", CoalBlockTile, CoalBlockTile,  CoalBlockTile,  true,  true,  RenderType::Cube,   0,   5.0f },
        { "Note Block",   NoteBlockTile, NoteBlockTile,  NoteBlockTile,  true,  true,  RenderType::Cube,   0,   0.8f },
        { "Note Block",   NoteBlockTile, NoteBlockTile,  NoteBlockTile,  true,  true,  RenderType::Cube,   0,   0.8f },
        { "Observer",     ObserverTop,   ObserverBack,   ObserverSide,   true,  true,  RenderType::Cube,   0,   3.0f, ObserverArrow, ObserverArrow },
        { "Observer",     ObserverTopLit,ObserverBackLit,ObserverSide,   true,  true,  RenderType::Cube,   0,   3.0f, ObserverArrow, ObserverArrow },
        { "Hopper",       HopperTop,     HopperSide,     HopperSide,     true,  false, RenderType::Cube,   0,   3.0f },
        { "Dropper",      DropperFront,  DispenserSide, DispenserSide,    true,  true,  RenderType::Cube,   0,   3.5f },
        { "Dispenser",    DispenserFront,DispenserSide, DispenserSide,    true,  true,  RenderType::Cube,   0,   3.5f },
        { "Block of Amethyst",AmethystTile,AmethystTile,  AmethystTile,   true,  true,  RenderType::Cube,   0,   1.5f },
        { "Budding Amethyst",BuddingTile, BuddingTile,    BuddingTile,    true,  true,  RenderType::Cube,   0,   1.5f },
        { "Amethyst Cluster",ClusterTile, ClusterTile,    ClusterTile,    false, false, RenderType::Cross,  5,   1.5f },
        { "Smooth Basalt",SmoothBasaltTile,SmoothBasaltTile,SmoothBasaltTile,true,true,RenderType::Cube,   0,   1.25f },
        { "Lantern",      LanternTile,   LanternTile,    LanternTile,    true,  true,  RenderType::Cube,  15,   0.3f },
        { "Barrel",       BarrelTop,     BarrelBottom,   BarrelSide,     true,  true,  RenderType::Cube,   0,   2.5f },
        { "Campfire",     CampfireTop,   CampfireLog,    CampfireLogLit,    true,  false, RenderType::Cube,  15,   2.0f },
        { "Carrot Crop",  CarrotStage0,  CarrotStage0,   CarrotStage0,   false, false, RenderType::Cross,  0,   0.0f },
        { "Carrot Crop",  CarrotStage1,  CarrotStage1,   CarrotStage1,   false, false, RenderType::Cross,  0,   0.0f },
        { "Carrot Crop",  CarrotStage2,  CarrotStage2,   CarrotStage2,   false, false, RenderType::Cross,  0,   0.0f },
        { "Carrot Crop",  CarrotStage3,  CarrotStage3,   CarrotStage3,   false, false, RenderType::Cross,  0,   0.0f },
        { "Carrot Crop",  CarrotStage4,  CarrotStage4,   CarrotStage4,   false, false, RenderType::Cross,  0,   0.0f },
        { "Carrot Crop",  CarrotStage5,  CarrotStage5,   CarrotStage5,   false, false, RenderType::Cross,  0,   0.0f },
        { "Carrot Crop",  CarrotStage6,  CarrotStage6,   CarrotStage6,   false, false, RenderType::Cross,  0,   0.0f },
        { "Carrot Crop",  CarrotStage7,  CarrotStage7,   CarrotStage7,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage0,  PotatoStage0,   PotatoStage0,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage1,  PotatoStage1,   PotatoStage1,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage2,  PotatoStage2,   PotatoStage2,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage3,  PotatoStage3,   PotatoStage3,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage4,  PotatoStage4,   PotatoStage4,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage5,  PotatoStage5,   PotatoStage5,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage6,  PotatoStage6,   PotatoStage6,   false, false, RenderType::Cross,  0,   0.0f },
        { "Potato Crop",  PotatoStage7,  PotatoStage7,   PotatoStage7,   false, false, RenderType::Cross,  0,   0.0f },

        { "Smoker",       SmokerTop,     SmokerBottom,   SmokerSide,     true,  true,  RenderType::Cube,   0,   3.5f, SmokerFront },
        { "Smoker",       SmokerTop,     SmokerBottom,   SmokerSide,     true,  true,  RenderType::Cube,  13,   3.5f, SmokerFrontLit },
        { "Blast Furnace",BlastFurnaceTop,BlastFurnaceTop,BlastFurnaceSide,true,true,RenderType::Cube,   0,   3.5f, BlastFurnaceFront },
        { "Blast Furnace",BlastFurnaceTop,BlastFurnaceTop,BlastFurnaceSide,true,true,RenderType::Cube,13, 3.5f, BlastFurnaceFrontLit },

        { "Composter",    ComposterTop,  ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterFill1,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterFill1,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterFill2,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterFill2,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterFill3,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterFill3,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterFill3,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Composter",    ComposterReady,ComposterBottom,ComposterSide,  true,  true,  RenderType::Cube,   0,   0.6f },

        { "Grindstone",   GrindstonePivot,GrindstonePivot,GrindstoneSide,true,  true,  RenderType::Cube,   0,   2.0f },
        { "Lectern",      LecternTop,    LecternBase,    LecternSide,    true,  true,  RenderType::Cube,   0,   2.5f },
        { "Lectern",      LecternTopBook,LecternBase,    LecternSide,    true,  true,  RenderType::Cube,   0,   2.5f },

        { "Cauldron",     CauldronTop,   CauldronBottom, CauldronSide,   true,  true,  RenderType::Cube,   0,   2.0f },
        { "Cauldron",     CauldronWater1,CauldronBottom, CauldronSide,   true,  true,  RenderType::Cube,   0,   2.0f },
        { "Cauldron",     CauldronWater2,CauldronBottom, CauldronSide,   true,  true,  RenderType::Cube,   0,   2.0f },
        { "Cauldron",     CauldronWater3,CauldronBottom, CauldronSide,   true,  true,  RenderType::Cube,   0,   2.0f },

        { "Beetroots",    BeetrootStage0,BeetrootStage0, BeetrootStage0, false, false, RenderType::Cross,  0,   0.0f },
        { "Beetroots",    BeetrootStage1,BeetrootStage1, BeetrootStage1, false, false, RenderType::Cross,  0,   0.0f },
        { "Beetroots",    BeetrootStage2,BeetrootStage2, BeetrootStage2, false, false, RenderType::Cross,  0,   0.0f },
        { "Beetroots",    BeetrootStage3,BeetrootStage3, BeetrootStage3, false, false, RenderType::Cross,  0,   0.0f },
        { "Beetroots",    BeetrootStage4,BeetrootStage4, BeetrootStage4, false, false, RenderType::Cross,  0,   0.0f },
        { "Beetroots",    BeetrootStage5,BeetrootStage5, BeetrootStage5, false, false, RenderType::Cross,  0,   0.0f },
        { "Beetroots",    BeetrootStage6,BeetrootStage6, BeetrootStage6, false, false, RenderType::Cross,  0,   0.0f },
        { "Beetroots",    BeetrootStage7,BeetrootStage7, BeetrootStage7, false, false, RenderType::Cross,  0,   0.0f },

        { "Melon",        MelonTop,      MelonTop,       MelonSide,      true,  true,  RenderType::Cube,   0,   1.0f },

        { "Melon Stem",   MelonStemYoung,MelonStemYoung, MelonStemYoung, false, false, RenderType::Cross,  0,   0.0f },
        { "Melon Stem",   MelonStemYoung,MelonStemYoung, MelonStemYoung, false, false, RenderType::Cross,  0,   0.0f },
        { "Melon Stem",   MelonStemYoung,MelonStemYoung, MelonStemYoung, false, false, RenderType::Cross,  0,   0.0f },
        { "Melon Stem",   MelonStemYoung,MelonStemYoung, MelonStemYoung, false, false, RenderType::Cross,  0,   0.0f },
        { "Melon Stem",   MelonStemYoung,MelonStemYoung, MelonStemYoung, false, false, RenderType::Cross,  0,   0.0f },
        { "Melon Stem",   MelonStemYoung,MelonStemYoung, MelonStemYoung, false, false, RenderType::Cross,  0,   0.0f },
        { "Melon Stem",   MelonStemRipe, MelonStemRipe,  MelonStemRipe,  false, false, RenderType::Cross,  0,   0.0f },
        { "Melon Stem",   MelonStemRipe, MelonStemRipe,  MelonStemRipe,  false, false, RenderType::Cross,  0,   0.0f },

        { "Bamboo",       BambooTile,    BambooTile,     BambooTile,     false, false, RenderType::Cross,  0,   0.3f },

        { "Cocoa",        CocoaStage0,   CocoaStage0,    CocoaStage0,    false, false, RenderType::Cross,  0,   0.2f },
        { "Cocoa",        CocoaStage1,   CocoaStage1,    CocoaStage1,    false, false, RenderType::Cross,  0,   0.2f },
        { "Cocoa",        CocoaStage2,   CocoaStage2,    CocoaStage2,    false, false, RenderType::Cross,  0,   0.2f },

        // A whole cake, and the six stages of one somebody has started.
        // The cut face is the sponge, which is what tells the two
        // apart across a room.
        { "Cake",         CakeTop,       CakeBottom,     CakeSide,       true,  false, RenderType::Cube,   0,   0.5f },
        { "Cake",         CakeTop,       CakeBottom,     CakeInner,      true,  false, RenderType::Cube,   0,   0.5f },
        { "Cake",         CakeTop,       CakeBottom,     CakeInner,      true,  false, RenderType::Cube,   0,   0.5f },
        { "Cake",         CakeTop,       CakeBottom,     CakeInner,      true,  false, RenderType::Cube,   0,   0.5f },
        { "Cake",         CakeTop,       CakeBottom,     CakeInner,      true,  false, RenderType::Cube,   0,   0.5f },
        { "Cake",         CakeTop,       CakeBottom,     CakeInner,      true,  false, RenderType::Cube,   0,   0.5f },
        { "Cake",         CakeTop,       CakeBottom,     CakeInner,      true,  false, RenderType::Cube,   0,   0.5f },

        { "Candle",       CandleTile,    CandleTile,     CandleTile,     false, false, RenderType::Candle, 0,   0.1f },
        { "Candle",       CandleLitTile, CandleLitTile,  CandleLitTile,  false, false, RenderType::Candle, 3,   0.1f },

        { "Scaffolding",  ScaffoldTop,   ScaffoldBottom, ScaffoldSide,   false, false, RenderType::Cube,   0,   0.2f },

        { "Sign",         SignTile,      SignTile,       SignTile,       false, false, RenderType::Ladder, 0,   1.0f },
        { "Sign",         SignTile,      SignTile,       SignTile,       false, false, RenderType::Ladder, 0,   1.0f },
        { "Sign",         SignTile,      SignTile,       SignTile,       false, false, RenderType::Ladder, 0,   1.0f },
        { "Sign",         SignTile,      SignTile,       SignTile,       false, false, RenderType::Ladder, 0,   1.0f },

        { "Item Frame",   ItemFrameTile, ItemFrameTile,  ItemFrameTile,  false, false, RenderType::Ladder, 0,   0.3f },
        { "Item Frame",   ItemFrameTile, ItemFrameTile,  ItemFrameTile,  false, false, RenderType::Ladder, 0,   0.3f },
        { "Item Frame",   ItemFrameTile, ItemFrameTile,  ItemFrameTile,  false, false, RenderType::Ladder, 0,   0.3f },
        { "Item Frame",   ItemFrameTile, ItemFrameTile,  ItemFrameTile,  false, false, RenderType::Ladder, 0,   0.3f },

        { "Shulker Box",  ShulkerBoxTile,ShulkerBoxTile, ShulkerBoxTile, true,  true,  RenderType::Cube,   0,   2.0f },

        { "Orange Wool",      WoolFirst +  0, WoolFirst +  0, WoolFirst +  0, true, true, RenderType::Cube, 0, 0.8f },
        { "Magenta Wool",     WoolFirst +  1, WoolFirst +  1, WoolFirst +  1, true, true, RenderType::Cube, 0, 0.8f },
        { "Light Blue Wool",  WoolFirst +  2, WoolFirst +  2, WoolFirst +  2, true, true, RenderType::Cube, 0, 0.8f },
        { "Yellow Wool",      WoolFirst +  3, WoolFirst +  3, WoolFirst +  3, true, true, RenderType::Cube, 0, 0.8f },
        { "Lime Wool",        WoolFirst +  4, WoolFirst +  4, WoolFirst +  4, true, true, RenderType::Cube, 0, 0.8f },
        { "Pink Wool",        WoolFirst +  5, WoolFirst +  5, WoolFirst +  5, true, true, RenderType::Cube, 0, 0.8f },
        { "Gray Wool",        WoolFirst +  6, WoolFirst +  6, WoolFirst +  6, true, true, RenderType::Cube, 0, 0.8f },
        { "Light Gray Wool",  WoolFirst +  7, WoolFirst +  7, WoolFirst +  7, true, true, RenderType::Cube, 0, 0.8f },
        { "Cyan Wool",        WoolFirst +  8, WoolFirst +  8, WoolFirst +  8, true, true, RenderType::Cube, 0, 0.8f },
        { "Purple Wool",      WoolFirst +  9, WoolFirst +  9, WoolFirst +  9, true, true, RenderType::Cube, 0, 0.8f },
        { "Blue Wool",        WoolFirst + 10, WoolFirst + 10, WoolFirst + 10, true, true, RenderType::Cube, 0, 0.8f },
        { "Brown Wool",       WoolFirst + 11, WoolFirst + 11, WoolFirst + 11, true, true, RenderType::Cube, 0, 0.8f },
        { "Green Wool",       WoolFirst + 12, WoolFirst + 12, WoolFirst + 12, true, true, RenderType::Cube, 0, 0.8f },
        { "Red Wool",         WoolFirst + 13, WoolFirst + 13, WoolFirst + 13, true, true, RenderType::Cube, 0, 0.8f },
        { "Black Wool",       WoolFirst + 14, WoolFirst + 14, WoolFirst + 14, true, true, RenderType::Cube, 0, 0.8f },

        { "Lapis Ore",    LapisOreTile,  LapisOreTile,   LapisOreTile,   true,  true,  RenderType::Cube,   0,   3.0f },
        { "Block of Lapis", LapisBlockTile, LapisBlockTile, LapisBlockTile, true, true, RenderType::Cube,  0,   3.0f },
        { "Ancient Debris", AncientDebrisTop, AncientDebrisTop, AncientDebrisSide, true, true, RenderType::Cube, 0, 30.0f },
        { "Block of Netherite", NetheriteBlockTile, NetheriteBlockTile, NetheriteBlockTile, true, true, RenderType::Cube, 0, 50.0f },
        { "Ochre Froglight", OchreFroglightTop, OchreFroglightTop, OchreFroglightSide, true, true, RenderType::Cube, 15, 0.3f },
        { "Verdant Froglight", VerdantFroglightTop, VerdantFroglightTop, VerdantFroglightSide, true, true, RenderType::Cube, 15, 0.3f },
        { "Pearlescent Froglight", PearlescentFroglightTop, PearlescentFroglightTop, PearlescentFroglightSide, true, true, RenderType::Cube, 15, 0.3f },
        // Not opaque: Minecraft's is see-through, and the light that
        // gets through it is half the reason anybody builds with one.
        { "Slime Block", SlimeBlockTile, SlimeBlockTile, SlimeBlockTile, true, false, RenderType::Cube, 0, 0.0f },
        // You walk into one, so it cannot collide; it is its own
        // light, so the ring of them is visible from the island.
        { "End Gateway", EndGatewayTile, EndGatewayTile, EndGatewayTile, false, false, RenderType::Cube, 15, -1.0f },
        { "Furnace", FurnaceTop, FurnaceTop, FurnaceSide, true, true, RenderType::Cube, 0, 3.5f, FurnaceFront },
        { "Furnace", FurnaceTop, FurnaceTop, FurnaceSide, true, true, RenderType::Cube, 0, 3.5f, FurnaceFront },
        { "Furnace", FurnaceTop, FurnaceTop, FurnaceSide, true, true, RenderType::Cube, 0, 3.5f, FurnaceFront },
        { "Furnace", FurnaceTop, FurnaceTop, FurnaceSide, true, true, RenderType::Cube, 13, 3.5f, FurnaceLit },
        { "Furnace", FurnaceTop, FurnaceTop, FurnaceSide, true, true, RenderType::Cube, 13, 3.5f, FurnaceLit },
        { "Furnace", FurnaceTop, FurnaceTop, FurnaceSide, true, true, RenderType::Cube, 13, 3.5f, FurnaceLit },
        { "Smoker", SmokerTop, SmokerBottom, SmokerSide, true, true, RenderType::Cube, 0, 3.5f, SmokerFront },
        { "Smoker", SmokerTop, SmokerBottom, SmokerSide, true, true, RenderType::Cube, 0, 3.5f, SmokerFront },
        { "Smoker", SmokerTop, SmokerBottom, SmokerSide, true, true, RenderType::Cube, 0, 3.5f, SmokerFront },
        { "Smoker", SmokerTop, SmokerBottom, SmokerSide, true, true, RenderType::Cube, 13, 3.5f, SmokerFrontLit },
        { "Smoker", SmokerTop, SmokerBottom, SmokerSide, true, true, RenderType::Cube, 13, 3.5f, SmokerFrontLit },
        { "Smoker", SmokerTop, SmokerBottom, SmokerSide, true, true, RenderType::Cube, 13, 3.5f, SmokerFrontLit },
        { "Blast Furnace", BlastFurnaceTop, BlastFurnaceTop, BlastFurnaceSide, true, true, RenderType::Cube, 0, 3.5f, BlastFurnaceFront },
        { "Blast Furnace", BlastFurnaceTop, BlastFurnaceTop, BlastFurnaceSide, true, true, RenderType::Cube, 0, 3.5f, BlastFurnaceFront },
        { "Blast Furnace", BlastFurnaceTop, BlastFurnaceTop, BlastFurnaceSide, true, true, RenderType::Cube, 0, 3.5f, BlastFurnaceFront },
        { "Blast Furnace", BlastFurnaceTop, BlastFurnaceTop, BlastFurnaceSide, true, true, RenderType::Cube, 13, 3.5f, BlastFurnaceFrontLit },
        { "Blast Furnace", BlastFurnaceTop, BlastFurnaceTop, BlastFurnaceSide, true, true, RenderType::Cube, 13, 3.5f, BlastFurnaceFrontLit },
        { "Blast Furnace", BlastFurnaceTop, BlastFurnaceTop, BlastFurnaceSide, true, true, RenderType::Cube, 13, 3.5f, BlastFurnaceFrontLit },
        { "Barrel", BarrelTopOpen, BarrelBottom, BarrelSide, true, true, RenderType::Cube, 0, 2.5f },
        { "Nether Wart", NetherWart0Tile, NetherWart0Tile, NetherWart0Tile, false, false, RenderType::Cross, 0, 0.0f },
        { "Nether Wart", NetherWart1Tile, NetherWart1Tile, NetherWart1Tile, false, false, RenderType::Cross, 0, 0.0f },
        { "Chipped Anvil", AnvilTopChipped, AnvilTopChipped, AnvilSide, true, false, RenderType::Slab, 0, 5.0f },
        { "Damaged Anvil", AnvilTopDamaged, AnvilTopDamaged, AnvilSide, true, false, RenderType::Slab, 0, 5.0f },
        { "Copper Bulb", CopperBulbPoweredTile, CopperBulbPoweredTile, CopperBulbPoweredTile, true, true, RenderType::Cube, 0, 3.0f },
        { "Copper Bulb", CopperBulbLitPoweredTile, CopperBulbLitPoweredTile, CopperBulbLitPoweredTile, true, true, RenderType::Cube, 15, 3.0f },
        { "Sculk Catalyst", SculkCatalystTopBloom, SculkCatalystBottom, SculkCatalystSideBloom, true, true, RenderType::Cube, 6, 3.0f },
        { "Vault", VaultTopEjecting, VaultBottom, VaultSideOn, true, true, RenderType::Cube, 8, -1.0f, VaultFrontEjecting },
        { "Ominous Vault", VaultTopEjectingOminous, VaultBottomOminous, VaultSideOnOminous, true, true, RenderType::Cube, 10, -1.0f, VaultFrontEjectingOminous },
        { "Trial Spawner", TrialSpawnerTopActiveOminous, TrialSpawnerBottom, TrialSpawnerSideActiveOminous, true, true, RenderType::Cube, 8, -1.0f },
        { "Trial Spawner", TrialSpawnerTopEjecting, TrialSpawnerBottom, TrialSpawnerSideLit, true, true, RenderType::Cube, 8, -1.0f },
        { "Trial Spawner", TrialSpawnerTopEjectingOminous, TrialSpawnerBottom, TrialSpawnerSideActiveOminous, true, true, RenderType::Cube, 8, -1.0f },
        { "Trial Spawner", TrialSpawnerTopInactiveOminous, TrialSpawnerBottom, TrialSpawnerSideInactiveOminous, true, true, RenderType::Cube, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Water", WaterFlow, Water, WaterFlow, false, false, RenderType::Liquid, 0, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Lava", LavaFlow, Lava, LavaFlow, false, false, RenderType::Liquid, 15, -1.0f },
        { "Small Amethyst Bud", SmallBudTile, SmallBudTile, SmallBudTile, false, false, RenderType::Cross, 1, 1.5f },
        { "Medium Amethyst Bud", MediumBudTile, MediumBudTile, MediumBudTile, false, false, RenderType::Cross, 2, 1.5f },
        { "Large Amethyst Bud", LargeBudTile, LargeBudTile, LargeBudTile, false, false, RenderType::Cross, 4, 1.5f },
        { "Birch Planks", BirchPlanksTile, BirchPlanksTile, BirchPlanksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Oak Sapling", OakSaplingTile, OakSaplingTile, OakSaplingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Spruce Sapling", SpruceSaplingTile, SpruceSaplingTile, SpruceSaplingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Birch Sapling", BirchSaplingTile, BirchSaplingTile, BirchSaplingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Jungle Sapling", JungleSaplingTile, JungleSaplingTile, JungleSaplingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Acacia Sapling", AcaciaSaplingTile, AcaciaSaplingTile, AcaciaSaplingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Dark Oak Sapling", DarkOakSaplingTile, DarkOakSaplingTile, DarkOakSaplingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Stripped Oak Log", StrippedOakLogTop, StrippedOakLogTop, StrippedOakLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Stripped Spruce Log", StrippedSpruceLogTop, StrippedSpruceLogTop, StrippedSpruceLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Stripped Birch Log", StrippedBirchLogTop, StrippedBirchLogTop, StrippedBirchLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Stripped Jungle Log", StrippedJungleLogTop, StrippedJungleLogTop, StrippedJungleLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Stripped Acacia Log", StrippedAcaciaLogTop, StrippedAcaciaLogTop, StrippedAcaciaLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Stripped Dark Oak Log", StrippedDarkOakLogTop, StrippedDarkOakLogTop, StrippedDarkOakLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorLower, SpruceDoorLower, SpruceDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Spruce Door", SpruceDoorUpper, SpruceDoorUpper, SpruceDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorLower, BirchDoorLower, BirchDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Birch Door", BirchDoorUpper, BirchDoorUpper, BirchDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorLower, JungleDoorLower, JungleDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Jungle Door", JungleDoorUpper, JungleDoorUpper, JungleDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorLower, AcaciaDoorLower, AcaciaDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Acacia Door", AcaciaDoorUpper, AcaciaDoorUpper, AcaciaDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorLower, DarkOakDoorLower, DarkOakDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Dark Oak Door", DarkOakDoorUpper, DarkOakDoorUpper, DarkOakDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oak Trapdoor", OakTrapdoorTile, OakTrapdoorTile, OakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Trapdoor", SpruceTrapdoorTile, SpruceTrapdoorTile, SpruceTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Birch Trapdoor", BirchTrapdoorTile, BirchTrapdoorTile, BirchTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Jungle Trapdoor", JungleTrapdoorTile, JungleTrapdoorTile, JungleTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Acacia Trapdoor", AcaciaTrapdoorTile, AcaciaTrapdoorTile, AcaciaTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Dark Oak Trapdoor", DarkOakTrapdoorTile, DarkOakTrapdoorTile, DarkOakTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Spruce Sign", SprucePlanks, SprucePlanks, SprucePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Spruce Sign", SprucePlanks, SprucePlanks, SprucePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Spruce Sign", SprucePlanks, SprucePlanks, SprucePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Spruce Sign", SprucePlanks, SprucePlanks, SprucePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Birch Sign", BirchPlanksTile, BirchPlanksTile, BirchPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Birch Sign", BirchPlanksTile, BirchPlanksTile, BirchPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Birch Sign", BirchPlanksTile, BirchPlanksTile, BirchPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Birch Sign", BirchPlanksTile, BirchPlanksTile, BirchPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Jungle Sign", JunglePlanks, JunglePlanks, JunglePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Jungle Sign", JunglePlanks, JunglePlanks, JunglePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Jungle Sign", JunglePlanks, JunglePlanks, JunglePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Jungle Sign", JunglePlanks, JunglePlanks, JunglePlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Acacia Sign", AcaciaPlanks, AcaciaPlanks, AcaciaPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Acacia Sign", AcaciaPlanks, AcaciaPlanks, AcaciaPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Acacia Sign", AcaciaPlanks, AcaciaPlanks, AcaciaPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Acacia Sign", AcaciaPlanks, AcaciaPlanks, AcaciaPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Dark Oak Sign", DarkOakPlanks, DarkOakPlanks, DarkOakPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Dark Oak Sign", DarkOakPlanks, DarkOakPlanks, DarkOakPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Dark Oak Sign", DarkOakPlanks, DarkOakPlanks, DarkOakPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Dark Oak Sign", DarkOakPlanks, DarkOakPlanks, DarkOakPlanks, false, false, RenderType::Ladder, 0, 1.0f },
        { "Cherry Log", CherryLogTop, CherryLogTop, CherryLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Cherry Leaves", CherryLeavesTile, CherryLeavesTile, CherryLeavesTile, true, false, RenderType::Cube, 0, 0.2f },
        { "Cherry Planks", CherryPlanksTile, CherryPlanksTile, CherryPlanksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Stripped Cherry Log", StrippedCherryLogTop, StrippedCherryLogTop, StrippedCherryLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Cherry Sapling", CherrySaplingTile, CherrySaplingTile, CherrySaplingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorLower, CherryDoorLower, CherryDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Door", CherryDoorUpper, CherryDoorUpper, CherryDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Trapdoor", CherryTrapdoorTile, CherryTrapdoorTile, CherryTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Cherry Sign", CherryPlanksTile, CherryPlanksTile, CherryPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Cherry Sign", CherryPlanksTile, CherryPlanksTile, CherryPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Cherry Sign", CherryPlanksTile, CherryPlanksTile, CherryPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Cherry Sign", CherryPlanksTile, CherryPlanksTile, CherryPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Pink Petals", PinkPetalsTile, PinkPetalsTile, PinkPetalsTile, false, false, RenderType::Flat, 0, 0.0f },
        { "Pink Petals", PinkPetalsTile, PinkPetalsTile, PinkPetalsTile, false, false, RenderType::Flat, 0, 0.0f },
        { "Pink Petals", PinkPetalsTile, PinkPetalsTile, PinkPetalsTile, false, false, RenderType::Flat, 0, 0.0f },
        { "Pink Petals", PinkPetalsTile, PinkPetalsTile, PinkPetalsTile, false, false, RenderType::Flat, 0, 0.0f },
        { "Mangrove Log", MangroveLogTop, MangroveLogTop, MangroveLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Mangrove Leaves", MangroveLeavesTile, MangroveLeavesTile, MangroveLeavesTile, true, false, RenderType::Cube, 0, 0.2f },
        { "Mangrove Planks", MangrovePlanksTile, MangrovePlanksTile, MangrovePlanksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Stripped Mangrove Log", StrippedMangroveLogTop, StrippedMangroveLogTop, StrippedMangroveLogSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Mangrove Propagule", MangrovePropaguleTile, MangrovePropaguleTile, MangrovePropaguleTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Mangrove Propagule", MangrovePropaguleHangingTile, MangrovePropaguleHangingTile, MangrovePropaguleHangingTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Mangrove Roots", MangroveRootsTop, MangroveRootsTop, MangroveRootsSide, true, false, RenderType::Cube, 0, 0.7f },
        { "Muddy Mangrove Roots", MuddyMangroveRootsTop, MuddyMangroveRootsTop, MuddyMangroveRootsSide, true, true, RenderType::Cube, 0, 0.7f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorLower, MangroveDoorLower, MangroveDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Door", MangroveDoorUpper, MangroveDoorUpper, MangroveDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Trapdoor", MangroveTrapdoorTile, MangroveTrapdoorTile, MangroveTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Mangrove Sign", MangrovePlanksTile, MangrovePlanksTile, MangrovePlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Mangrove Sign", MangrovePlanksTile, MangrovePlanksTile, MangrovePlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Mangrove Sign", MangrovePlanksTile, MangrovePlanksTile, MangrovePlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Mangrove Sign", MangrovePlanksTile, MangrovePlanksTile, MangrovePlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Mud Bricks", MudBricksTile, MudBricksTile, MudBricksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Packed Mud", PackedMudTile, PackedMudTile, PackedMudTile, true, true, RenderType::Cube, 0, 1.0f },
        { "Block of Bamboo", BambooBlockTop, BambooBlockTop, BambooBlockSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Block of Stripped Bamboo", StrippedBambooBlockTop, StrippedBambooBlockTop, StrippedBambooBlockSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Bamboo Planks", BambooPlanksTile, BambooPlanksTile, BambooPlanksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Bamboo Mosaic", BambooMosaicTile, BambooMosaicTile, BambooMosaicTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Bamboo Slab", BambooPlanksTile, BambooPlanksTile, BambooPlanksTile, true, false, RenderType::Slab, 0, 1.5f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorLower, BambooDoorLower, BambooDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Door", BambooDoorUpper, BambooDoorUpper, BambooDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Trapdoor", BambooTrapdoorTile, BambooTrapdoorTile, BambooTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Bamboo Sign", BambooPlanksTile, BambooPlanksTile, BambooPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Bamboo Sign", BambooPlanksTile, BambooPlanksTile, BambooPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Bamboo Sign", BambooPlanksTile, BambooPlanksTile, BambooPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Bamboo Sign", BambooPlanksTile, BambooPlanksTile, BambooPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Crimson Stem", CrimsonStemTop, CrimsonStemTop, CrimsonStemSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Stripped Crimson Stem", StrippedCrimsonStemTop, StrippedCrimsonStemTop, StrippedCrimsonStemSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Crimson Planks", CrimsonPlanksTile, CrimsonPlanksTile, CrimsonPlanksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Crimson Nylium", CrimsonNyliumTop, Netherrack, CrimsonNyliumSide, true, true, RenderType::Cube, 0, 0.6f },
        { "Crimson Fungus", CrimsonFungusTile, CrimsonFungusTile, CrimsonFungusTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Crimson Roots", CrimsonRootsTile, CrimsonRootsTile, CrimsonRootsTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Warped Stem", WarpedStemTop, WarpedStemTop, WarpedStemSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Stripped Warped Stem", StrippedWarpedStemTop, StrippedWarpedStemTop, StrippedWarpedStemSide, true, true, RenderType::Cube, 0, 2.0f },
        { "Warped Planks", WarpedPlanksTile, WarpedPlanksTile, WarpedPlanksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Warped Nylium", WarpedNyliumTop, Netherrack, WarpedNyliumSide, true, true, RenderType::Cube, 0, 0.6f },
        { "Warped Fungus", WarpedFungusTile, WarpedFungusTile, WarpedFungusTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Warped Roots", WarpedRootsTile, WarpedRootsTile, WarpedRootsTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Nether Sprouts", NetherSproutsTile, NetherSproutsTile, NetherSproutsTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Nether Wart Block", NetherWartBlockTile, NetherWartBlockTile, NetherWartBlockTile, true, true, RenderType::Cube, 0, 1.0f },
        { "Warped Wart Block", WarpedWartBlockTile, WarpedWartBlockTile, WarpedWartBlockTile, true, true, RenderType::Cube, 0, 1.0f },
        { "Shroomlight", ShroomlightTile, ShroomlightTile, ShroomlightTile, true, true, RenderType::Cube, 15, 1.0f },
        { "Weeping Vines", WeepingVinesTile, WeepingVinesTile, WeepingVinesTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Weeping Vines", WeepingVinesPlantTile, WeepingVinesPlantTile, WeepingVinesPlantTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Twisting Vines", TwistingVinesTile, TwistingVinesTile, TwistingVinesTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Twisting Vines", TwistingVinesPlantTile, TwistingVinesPlantTile, TwistingVinesPlantTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorLower, CrimsonDoorLower, CrimsonDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Door", CrimsonDoorUpper, CrimsonDoorUpper, CrimsonDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorLower, WarpedDoorLower, WarpedDoorLower, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Warped Door", WarpedDoorUpper, WarpedDoorUpper, WarpedDoorUpper, true, false, RenderType::Door, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Trapdoor", CrimsonTrapdoorTile, CrimsonTrapdoorTile, CrimsonTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Warped Trapdoor", WarpedTrapdoorTile, WarpedTrapdoorTile, WarpedTrapdoorTile, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Crimson Sign", CrimsonPlanksTile, CrimsonPlanksTile, CrimsonPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Crimson Sign", CrimsonPlanksTile, CrimsonPlanksTile, CrimsonPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Crimson Sign", CrimsonPlanksTile, CrimsonPlanksTile, CrimsonPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Crimson Sign", CrimsonPlanksTile, CrimsonPlanksTile, CrimsonPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Warped Sign", WarpedPlanksTile, WarpedPlanksTile, WarpedPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Warped Sign", WarpedPlanksTile, WarpedPlanksTile, WarpedPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Warped Sign", WarpedPlanksTile, WarpedPlanksTile, WarpedPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Warped Sign", WarpedPlanksTile, WarpedPlanksTile, WarpedPlanksTile, false, false, RenderType::Ladder, 0, 1.0f },
        { "Stone Bricks", StoneBricksTile, StoneBricksTile, StoneBricksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Mossy Stone Bricks", MossyStoneBricksTile, MossyStoneBricksTile, MossyStoneBricksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Cracked Stone Bricks", CrackedStoneBricksTile, CrackedStoneBricksTile, CrackedStoneBricksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Chiseled Stone Bricks", ChiseledStoneBricksTile, ChiseledStoneBricksTile, ChiseledStoneBricksTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Smooth Stone", SmoothStoneTile, SmoothStoneTile, SmoothStoneTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Smooth Stone Slab", SmoothStoneTile, SmoothStoneTile, SmoothStoneSlabSide, true, false, RenderType::Slab, 0, 2.0f },
        { "Stone Brick Slab", StoneBricksTile, StoneBricksTile, StoneBricksTile, true, false, RenderType::Slab, 0, 2.0f },
        { "Polished Granite", PolishedGraniteTile, PolishedGraniteTile, PolishedGraniteTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Polished Diorite", PolishedDioriteTile, PolishedDioriteTile, PolishedDioriteTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Polished Andesite", PolishedAndesiteTile, PolishedAndesiteTile, PolishedAndesiteTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Cobbled Deepslate", CobbledDeepslateTile, CobbledDeepslateTile, CobbledDeepslateTile, true, true, RenderType::Cube, 0, 3.5f },
        { "Polished Deepslate", PolishedDeepslateTile, PolishedDeepslateTile, PolishedDeepslateTile, true, true, RenderType::Cube, 0, 3.5f },
        { "Chiseled Deepslate", ChiseledDeepslateTile, ChiseledDeepslateTile, ChiseledDeepslateTile, true, true, RenderType::Cube, 0, 3.5f },
        { "Cracked Deepslate Bricks", CrackedDeepslateBricksTile, CrackedDeepslateBricksTile, CrackedDeepslateBricksTile, true, true, RenderType::Cube, 0, 3.5f },
        { "Cracked Deepslate Tiles", CrackedDeepslateTilesTile, CrackedDeepslateTilesTile, CrackedDeepslateTilesTile, true, true, RenderType::Cube, 0, 3.5f },
        { "Cobbled Deepslate Slab", CobbledDeepslateTile, CobbledDeepslateTile, CobbledDeepslateTile, true, false, RenderType::Slab, 0, 3.5f },
        { "Polished Tuff", PolishedTuffTile, PolishedTuffTile, PolishedTuffTile, true, true, RenderType::Cube, 0, 1.5f },
        { "Chiseled Tuff", ChiseledTuffTop, ChiseledTuffTop, ChiseledTuffSide, true, true, RenderType::Cube, 0, 1.5f },
        { "Chiseled Tuff Bricks", ChiseledTuffBricksTop, ChiseledTuffBricksTop, ChiseledTuffBricksSide, true, true, RenderType::Cube, 0, 1.5f },
        { "Tuff Slab", Tuff, Tuff, Tuff, true, false, RenderType::Slab, 0, 1.5f },
        { "Tuff Brick Slab", TuffBricks, TuffBricks, TuffBricks, true, false, RenderType::Slab, 0, 1.5f },
        { "Polished Blackstone", PolishedBlackstonePlainTile, PolishedBlackstonePlainTile, PolishedBlackstonePlainTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Chiseled Polished Blackstone", ChiseledPolishedBlackstoneTile, ChiseledPolishedBlackstoneTile, ChiseledPolishedBlackstoneTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Cracked Polished Blackstone Bricks", CrackedPolishedBlackstoneBricksTile, CrackedPolishedBlackstoneBricksTile, CrackedPolishedBlackstoneBricksTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Polished Blackstone Slab", PolishedBlackstonePlainTile, PolishedBlackstonePlainTile, PolishedBlackstonePlainTile, true, false, RenderType::Slab, 0, 2.0f },
        { "Red Nether Bricks", RedNetherBricksTile, RedNetherBricksTile, RedNetherBricksTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Cracked Nether Bricks", CrackedNetherBricksTile, CrackedNetherBricksTile, CrackedNetherBricksTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Chiseled Nether Bricks", ChiseledNetherBricksTile, ChiseledNetherBricksTile, ChiseledNetherBricksTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Nether Brick Slab", NetherBricks, NetherBricks, NetherBricks, true, false, RenderType::Slab, 0, 2.0f },
        { "Nether Quartz Ore", NetherQuartzOreTile, NetherQuartzOreTile, NetherQuartzOreTile, true, true, RenderType::Cube, 0, 3.0f },
        { "Block of Quartz", QuartzBlockTop, QuartzBlockBottom, QuartzBlockSide, true, true, RenderType::Cube, 0, 0.8f },
        { "Quartz Bricks", QuartzBricksTile, QuartzBricksTile, QuartzBricksTile, true, true, RenderType::Cube, 0, 0.8f },
        { "Quartz Pillar", QuartzPillarTop, QuartzPillarTop, QuartzPillarSide, true, true, RenderType::Cube, 0, 0.8f },
        { "Chiseled Quartz Block", ChiseledQuartzTop, ChiseledQuartzTop, ChiseledQuartzSide, true, true, RenderType::Cube, 0, 0.8f },
        { "Quartz Slab", QuartzBlockTop, QuartzBlockTop, QuartzBlockSide, true, false, RenderType::Slab, 0, 2.0f },
        { "Red Sand", RedSandTile, RedSandTile, RedSandTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Red Sandstone", RedSandstoneTop, RedSandstoneBottom, RedSandstoneSide, true, true, RenderType::Cube, 0, 0.8f },
        { "Cut Red Sandstone", RedSandstoneTop, RedSandstoneTop, CutRedSandstoneTile, true, true, RenderType::Cube, 0, 0.8f },
        { "Chiseled Red Sandstone", RedSandstoneTop, RedSandstoneTop, ChiseledRedSandstoneTile, true, true, RenderType::Cube, 0, 0.8f },
        { "Smooth Red Sandstone", RedSandstoneTop, RedSandstoneTop, RedSandstoneTop, true, true, RenderType::Cube, 0, 2.0f },
        { "Red Sandstone Slab", RedSandstoneTop, RedSandstoneBottom, RedSandstoneSide, true, false, RenderType::Slab, 0, 2.0f },
        { "Cut Sandstone", SandstoneTop, SandstoneTop, CutSandstoneTile, true, true, RenderType::Cube, 0, 0.8f },
        { "Chiseled Sandstone", SandstoneTop, SandstoneTop, ChiseledSandstoneTile, true, true, RenderType::Cube, 0, 0.8f },
        { "Smooth Sandstone", SandstoneTop, SandstoneTop, SandstoneTop, true, true, RenderType::Cube, 0, 2.0f },
        { "Deepslate Coal Ore", DeepslateCoalOreTile, DeepslateCoalOreTile, DeepslateCoalOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Deepslate Iron Ore", DeepslateIronOreTile, DeepslateIronOreTile, DeepslateIronOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Deepslate Copper Ore", DeepslateCopperOreTile, DeepslateCopperOreTile, DeepslateCopperOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Deepslate Gold Ore", DeepslateGoldOreTile, DeepslateGoldOreTile, DeepslateGoldOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Deepslate Redstone Ore", DeepslateRedstoneOreTile, DeepslateRedstoneOreTile, DeepslateRedstoneOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Deepslate Emerald Ore", DeepslateEmeraldOreTile, DeepslateEmeraldOreTile, DeepslateEmeraldOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Deepslate Lapis Lazuli Ore", DeepslateLapisOreTile, DeepslateLapisOreTile, DeepslateLapisOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Deepslate Diamond Ore", DeepslateDiamondOreTile, DeepslateDiamondOreTile, DeepslateDiamondOreTile, true, true, RenderType::Cube, 0, 4.5f },
        { "Nether Gold Ore", NetherGoldOreTile, NetherGoldOreTile, NetherGoldOreTile, true, true, RenderType::Cube, 0, 3.0f },
        { "Block of Raw Iron", RawIronBlockTile, RawIronBlockTile, RawIronBlockTile, true, true, RenderType::Cube, 0, 5.0f },
        { "Block of Raw Gold", RawGoldBlockTile, RawGoldBlockTile, RawGoldBlockTile, true, true, RenderType::Cube, 0, 5.0f },
        { "Block of Raw Copper", RawCopperBlockTile, RawCopperBlockTile, RawCopperBlockTile, true, true, RenderType::Cube, 0, 5.0f },
        { "White Concrete", ConcreteTile + 0, ConcreteTile + 0, ConcreteTile + 0, true, true, RenderType::Cube, 0, 1.8f },
        { "Orange Concrete", ConcreteTile + 1, ConcreteTile + 1, ConcreteTile + 1, true, true, RenderType::Cube, 0, 1.8f },
        { "Magenta Concrete", ConcreteTile + 2, ConcreteTile + 2, ConcreteTile + 2, true, true, RenderType::Cube, 0, 1.8f },
        { "Light Blue Concrete", ConcreteTile + 3, ConcreteTile + 3, ConcreteTile + 3, true, true, RenderType::Cube, 0, 1.8f },
        { "Yellow Concrete", ConcreteTile + 4, ConcreteTile + 4, ConcreteTile + 4, true, true, RenderType::Cube, 0, 1.8f },
        { "Lime Concrete", ConcreteTile + 5, ConcreteTile + 5, ConcreteTile + 5, true, true, RenderType::Cube, 0, 1.8f },
        { "Pink Concrete", ConcreteTile + 6, ConcreteTile + 6, ConcreteTile + 6, true, true, RenderType::Cube, 0, 1.8f },
        { "Gray Concrete", ConcreteTile + 7, ConcreteTile + 7, ConcreteTile + 7, true, true, RenderType::Cube, 0, 1.8f },
        { "Light Gray Concrete", ConcreteTile + 8, ConcreteTile + 8, ConcreteTile + 8, true, true, RenderType::Cube, 0, 1.8f },
        { "Cyan Concrete", ConcreteTile + 9, ConcreteTile + 9, ConcreteTile + 9, true, true, RenderType::Cube, 0, 1.8f },
        { "Purple Concrete", ConcreteTile + 10, ConcreteTile + 10, ConcreteTile + 10, true, true, RenderType::Cube, 0, 1.8f },
        { "Blue Concrete", ConcreteTile + 11, ConcreteTile + 11, ConcreteTile + 11, true, true, RenderType::Cube, 0, 1.8f },
        { "Brown Concrete", ConcreteTile + 12, ConcreteTile + 12, ConcreteTile + 12, true, true, RenderType::Cube, 0, 1.8f },
        { "Green Concrete", ConcreteTile + 13, ConcreteTile + 13, ConcreteTile + 13, true, true, RenderType::Cube, 0, 1.8f },
        { "Red Concrete", ConcreteTile + 14, ConcreteTile + 14, ConcreteTile + 14, true, true, RenderType::Cube, 0, 1.8f },
        { "Black Concrete", ConcreteTile + 15, ConcreteTile + 15, ConcreteTile + 15, true, true, RenderType::Cube, 0, 1.8f },
        { "White Concrete Powder", ConcretePowderTile + 0, ConcretePowderTile + 0, ConcretePowderTile + 0, true, true, RenderType::Cube, 0, 0.5f },
        { "Orange Concrete Powder", ConcretePowderTile + 1, ConcretePowderTile + 1, ConcretePowderTile + 1, true, true, RenderType::Cube, 0, 0.5f },
        { "Magenta Concrete Powder", ConcretePowderTile + 2, ConcretePowderTile + 2, ConcretePowderTile + 2, true, true, RenderType::Cube, 0, 0.5f },
        { "Light Blue Concrete Powder", ConcretePowderTile + 3, ConcretePowderTile + 3, ConcretePowderTile + 3, true, true, RenderType::Cube, 0, 0.5f },
        { "Yellow Concrete Powder", ConcretePowderTile + 4, ConcretePowderTile + 4, ConcretePowderTile + 4, true, true, RenderType::Cube, 0, 0.5f },
        { "Lime Concrete Powder", ConcretePowderTile + 5, ConcretePowderTile + 5, ConcretePowderTile + 5, true, true, RenderType::Cube, 0, 0.5f },
        { "Pink Concrete Powder", ConcretePowderTile + 6, ConcretePowderTile + 6, ConcretePowderTile + 6, true, true, RenderType::Cube, 0, 0.5f },
        { "Gray Concrete Powder", ConcretePowderTile + 7, ConcretePowderTile + 7, ConcretePowderTile + 7, true, true, RenderType::Cube, 0, 0.5f },
        { "Light Gray Concrete Powder", ConcretePowderTile + 8, ConcretePowderTile + 8, ConcretePowderTile + 8, true, true, RenderType::Cube, 0, 0.5f },
        { "Cyan Concrete Powder", ConcretePowderTile + 9, ConcretePowderTile + 9, ConcretePowderTile + 9, true, true, RenderType::Cube, 0, 0.5f },
        { "Purple Concrete Powder", ConcretePowderTile + 10, ConcretePowderTile + 10, ConcretePowderTile + 10, true, true, RenderType::Cube, 0, 0.5f },
        { "Blue Concrete Powder", ConcretePowderTile + 11, ConcretePowderTile + 11, ConcretePowderTile + 11, true, true, RenderType::Cube, 0, 0.5f },
        { "Brown Concrete Powder", ConcretePowderTile + 12, ConcretePowderTile + 12, ConcretePowderTile + 12, true, true, RenderType::Cube, 0, 0.5f },
        { "Green Concrete Powder", ConcretePowderTile + 13, ConcretePowderTile + 13, ConcretePowderTile + 13, true, true, RenderType::Cube, 0, 0.5f },
        { "Red Concrete Powder", ConcretePowderTile + 14, ConcretePowderTile + 14, ConcretePowderTile + 14, true, true, RenderType::Cube, 0, 0.5f },
        { "Black Concrete Powder", ConcretePowderTile + 15, ConcretePowderTile + 15, ConcretePowderTile + 15, true, true, RenderType::Cube, 0, 0.5f },
        { "Terracotta", TerracottaPlainTile, TerracottaPlainTile, TerracottaPlainTile, true, true, RenderType::Cube, 0, 1.25f },
        { "White Terracotta", TerracottaTile + 0, TerracottaTile + 0, TerracottaTile + 0, true, true, RenderType::Cube, 0, 1.25f },
        { "Orange Terracotta", TerracottaTile + 1, TerracottaTile + 1, TerracottaTile + 1, true, true, RenderType::Cube, 0, 1.25f },
        { "Magenta Terracotta", TerracottaTile + 2, TerracottaTile + 2, TerracottaTile + 2, true, true, RenderType::Cube, 0, 1.25f },
        { "Light Blue Terracotta", TerracottaTile + 3, TerracottaTile + 3, TerracottaTile + 3, true, true, RenderType::Cube, 0, 1.25f },
        { "Yellow Terracotta", TerracottaTile + 4, TerracottaTile + 4, TerracottaTile + 4, true, true, RenderType::Cube, 0, 1.25f },
        { "Lime Terracotta", TerracottaTile + 5, TerracottaTile + 5, TerracottaTile + 5, true, true, RenderType::Cube, 0, 1.25f },
        { "Pink Terracotta", TerracottaTile + 6, TerracottaTile + 6, TerracottaTile + 6, true, true, RenderType::Cube, 0, 1.25f },
        { "Gray Terracotta", TerracottaTile + 7, TerracottaTile + 7, TerracottaTile + 7, true, true, RenderType::Cube, 0, 1.25f },
        { "Light Gray Terracotta", TerracottaTile + 8, TerracottaTile + 8, TerracottaTile + 8, true, true, RenderType::Cube, 0, 1.25f },
        { "Cyan Terracotta", TerracottaTile + 9, TerracottaTile + 9, TerracottaTile + 9, true, true, RenderType::Cube, 0, 1.25f },
        { "Purple Terracotta", TerracottaTile + 10, TerracottaTile + 10, TerracottaTile + 10, true, true, RenderType::Cube, 0, 1.25f },
        { "Blue Terracotta", TerracottaTile + 11, TerracottaTile + 11, TerracottaTile + 11, true, true, RenderType::Cube, 0, 1.25f },
        { "Brown Terracotta", TerracottaTile + 12, TerracottaTile + 12, TerracottaTile + 12, true, true, RenderType::Cube, 0, 1.25f },
        { "Green Terracotta", TerracottaTile + 13, TerracottaTile + 13, TerracottaTile + 13, true, true, RenderType::Cube, 0, 1.25f },
        { "Red Terracotta", TerracottaTile + 14, TerracottaTile + 14, TerracottaTile + 14, true, true, RenderType::Cube, 0, 1.25f },
        { "Black Terracotta", TerracottaTile + 15, TerracottaTile + 15, TerracottaTile + 15, true, true, RenderType::Cube, 0, 1.25f },
        { "White Glazed Terracotta", GlazedTile + 0, GlazedTile + 0, GlazedTile + 0, true, true, RenderType::Cube, 0, 1.4f },
        { "Orange Glazed Terracotta", GlazedTile + 1, GlazedTile + 1, GlazedTile + 1, true, true, RenderType::Cube, 0, 1.4f },
        { "Magenta Glazed Terracotta", GlazedTile + 2, GlazedTile + 2, GlazedTile + 2, true, true, RenderType::Cube, 0, 1.4f },
        { "Light Blue Glazed Terracotta", GlazedTile + 3, GlazedTile + 3, GlazedTile + 3, true, true, RenderType::Cube, 0, 1.4f },
        { "Yellow Glazed Terracotta", GlazedTile + 4, GlazedTile + 4, GlazedTile + 4, true, true, RenderType::Cube, 0, 1.4f },
        { "Lime Glazed Terracotta", GlazedTile + 5, GlazedTile + 5, GlazedTile + 5, true, true, RenderType::Cube, 0, 1.4f },
        { "Pink Glazed Terracotta", GlazedTile + 6, GlazedTile + 6, GlazedTile + 6, true, true, RenderType::Cube, 0, 1.4f },
        { "Gray Glazed Terracotta", GlazedTile + 7, GlazedTile + 7, GlazedTile + 7, true, true, RenderType::Cube, 0, 1.4f },
        { "Light Gray Glazed Terracotta", GlazedTile + 8, GlazedTile + 8, GlazedTile + 8, true, true, RenderType::Cube, 0, 1.4f },
        { "Cyan Glazed Terracotta", GlazedTile + 9, GlazedTile + 9, GlazedTile + 9, true, true, RenderType::Cube, 0, 1.4f },
        { "Purple Glazed Terracotta", GlazedTile + 10, GlazedTile + 10, GlazedTile + 10, true, true, RenderType::Cube, 0, 1.4f },
        { "Blue Glazed Terracotta", GlazedTile + 11, GlazedTile + 11, GlazedTile + 11, true, true, RenderType::Cube, 0, 1.4f },
        { "Brown Glazed Terracotta", GlazedTile + 12, GlazedTile + 12, GlazedTile + 12, true, true, RenderType::Cube, 0, 1.4f },
        { "Green Glazed Terracotta", GlazedTile + 13, GlazedTile + 13, GlazedTile + 13, true, true, RenderType::Cube, 0, 1.4f },
        { "Red Glazed Terracotta", GlazedTile + 14, GlazedTile + 14, GlazedTile + 14, true, true, RenderType::Cube, 0, 1.4f },
        { "Black Glazed Terracotta", GlazedTile + 15, GlazedTile + 15, GlazedTile + 15, true, true, RenderType::Cube, 0, 1.4f },
        { "White Stained Glass", StainedGlassTile + 0, StainedGlassTile + 0, StainedGlassTile + 0, true, false, RenderType::Cube, 0, 0.3f },
        { "Orange Stained Glass", StainedGlassTile + 1, StainedGlassTile + 1, StainedGlassTile + 1, true, false, RenderType::Cube, 0, 0.3f },
        { "Magenta Stained Glass", StainedGlassTile + 2, StainedGlassTile + 2, StainedGlassTile + 2, true, false, RenderType::Cube, 0, 0.3f },
        { "Light Blue Stained Glass", StainedGlassTile + 3, StainedGlassTile + 3, StainedGlassTile + 3, true, false, RenderType::Cube, 0, 0.3f },
        { "Yellow Stained Glass", StainedGlassTile + 4, StainedGlassTile + 4, StainedGlassTile + 4, true, false, RenderType::Cube, 0, 0.3f },
        { "Lime Stained Glass", StainedGlassTile + 5, StainedGlassTile + 5, StainedGlassTile + 5, true, false, RenderType::Cube, 0, 0.3f },
        { "Pink Stained Glass", StainedGlassTile + 6, StainedGlassTile + 6, StainedGlassTile + 6, true, false, RenderType::Cube, 0, 0.3f },
        { "Gray Stained Glass", StainedGlassTile + 7, StainedGlassTile + 7, StainedGlassTile + 7, true, false, RenderType::Cube, 0, 0.3f },
        { "Light Gray Stained Glass", StainedGlassTile + 8, StainedGlassTile + 8, StainedGlassTile + 8, true, false, RenderType::Cube, 0, 0.3f },
        { "Cyan Stained Glass", StainedGlassTile + 9, StainedGlassTile + 9, StainedGlassTile + 9, true, false, RenderType::Cube, 0, 0.3f },
        { "Purple Stained Glass", StainedGlassTile + 10, StainedGlassTile + 10, StainedGlassTile + 10, true, false, RenderType::Cube, 0, 0.3f },
        { "Blue Stained Glass", StainedGlassTile + 11, StainedGlassTile + 11, StainedGlassTile + 11, true, false, RenderType::Cube, 0, 0.3f },
        { "Brown Stained Glass", StainedGlassTile + 12, StainedGlassTile + 12, StainedGlassTile + 12, true, false, RenderType::Cube, 0, 0.3f },
        { "Green Stained Glass", StainedGlassTile + 13, StainedGlassTile + 13, StainedGlassTile + 13, true, false, RenderType::Cube, 0, 0.3f },
        { "Red Stained Glass", StainedGlassTile + 14, StainedGlassTile + 14, StainedGlassTile + 14, true, false, RenderType::Cube, 0, 0.3f },
        { "Black Stained Glass", StainedGlassTile + 15, StainedGlassTile + 15, StainedGlassTile + 15, true, false, RenderType::Cube, 0, 0.3f },
        { "Glass Pane", GlassPaneTopTile, GlassPaneTopTile, Glass, true, false, RenderType::Pane, 0, 0.3f },
        { "White Stained Glass Pane", StainedPaneTopTile + 0, StainedPaneTopTile + 0, StainedGlassTile + 0, true, false, RenderType::Pane, 0, 0.3f },
        { "Orange Stained Glass Pane", StainedPaneTopTile + 1, StainedPaneTopTile + 1, StainedGlassTile + 1, true, false, RenderType::Pane, 0, 0.3f },
        { "Magenta Stained Glass Pane", StainedPaneTopTile + 2, StainedPaneTopTile + 2, StainedGlassTile + 2, true, false, RenderType::Pane, 0, 0.3f },
        { "Light Blue Stained Glass Pane", StainedPaneTopTile + 3, StainedPaneTopTile + 3, StainedGlassTile + 3, true, false, RenderType::Pane, 0, 0.3f },
        { "Yellow Stained Glass Pane", StainedPaneTopTile + 4, StainedPaneTopTile + 4, StainedGlassTile + 4, true, false, RenderType::Pane, 0, 0.3f },
        { "Lime Stained Glass Pane", StainedPaneTopTile + 5, StainedPaneTopTile + 5, StainedGlassTile + 5, true, false, RenderType::Pane, 0, 0.3f },
        { "Pink Stained Glass Pane", StainedPaneTopTile + 6, StainedPaneTopTile + 6, StainedGlassTile + 6, true, false, RenderType::Pane, 0, 0.3f },
        { "Gray Stained Glass Pane", StainedPaneTopTile + 7, StainedPaneTopTile + 7, StainedGlassTile + 7, true, false, RenderType::Pane, 0, 0.3f },
        { "Light Gray Stained Glass Pane", StainedPaneTopTile + 8, StainedPaneTopTile + 8, StainedGlassTile + 8, true, false, RenderType::Pane, 0, 0.3f },
        { "Cyan Stained Glass Pane", StainedPaneTopTile + 9, StainedPaneTopTile + 9, StainedGlassTile + 9, true, false, RenderType::Pane, 0, 0.3f },
        { "Purple Stained Glass Pane", StainedPaneTopTile + 10, StainedPaneTopTile + 10, StainedGlassTile + 10, true, false, RenderType::Pane, 0, 0.3f },
        { "Blue Stained Glass Pane", StainedPaneTopTile + 11, StainedPaneTopTile + 11, StainedGlassTile + 11, true, false, RenderType::Pane, 0, 0.3f },
        { "Brown Stained Glass Pane", StainedPaneTopTile + 12, StainedPaneTopTile + 12, StainedGlassTile + 12, true, false, RenderType::Pane, 0, 0.3f },
        { "Green Stained Glass Pane", StainedPaneTopTile + 13, StainedPaneTopTile + 13, StainedGlassTile + 13, true, false, RenderType::Pane, 0, 0.3f },
        { "Red Stained Glass Pane", StainedPaneTopTile + 14, StainedPaneTopTile + 14, StainedGlassTile + 14, true, false, RenderType::Pane, 0, 0.3f },
        { "Black Stained Glass Pane", StainedPaneTopTile + 15, StainedPaneTopTile + 15, StainedGlassTile + 15, true, false, RenderType::Pane, 0, 0.3f },
        { "Tinted Glass", TintedGlassTile, TintedGlassTile, TintedGlassTile, true, false, RenderType::Cube, 0, 0.3f },
        { "Candle", CandleTile, CandleTile, CandleTile, false, false, RenderType::Candle, 0, 0.1f },
        { "Candle", CandleLitTile, CandleLitTile, CandleLitTile, false, false, RenderType::Candle, 6, 0.1f },
        { "Candle", CandleTile, CandleTile, CandleTile, false, false, RenderType::Candle, 0, 0.1f },
        { "Candle", CandleLitTile, CandleLitTile, CandleLitTile, false, false, RenderType::Candle, 9, 0.1f },
        { "Candle", CandleTile, CandleTile, CandleTile, false, false, RenderType::Candle, 0, 0.1f },
        { "Candle", CandleLitTile, CandleLitTile, CandleLitTile, false, false, RenderType::Candle, 12, 0.1f },
        { "White Candle", DyedCandleTile + 0, DyedCandleTile + 0, DyedCandleTile + 0, false, false, RenderType::Candle, 0, 0.1f },
        { "White Candle", DyedCandleTile + 1, DyedCandleTile + 1, DyedCandleTile + 1, false, false, RenderType::Candle, 3, 0.1f },
        { "White Candle", DyedCandleTile + 0, DyedCandleTile + 0, DyedCandleTile + 0, false, false, RenderType::Candle, 0, 0.1f },
        { "White Candle", DyedCandleTile + 1, DyedCandleTile + 1, DyedCandleTile + 1, false, false, RenderType::Candle, 6, 0.1f },
        { "White Candle", DyedCandleTile + 0, DyedCandleTile + 0, DyedCandleTile + 0, false, false, RenderType::Candle, 0, 0.1f },
        { "White Candle", DyedCandleTile + 1, DyedCandleTile + 1, DyedCandleTile + 1, false, false, RenderType::Candle, 9, 0.1f },
        { "White Candle", DyedCandleTile + 0, DyedCandleTile + 0, DyedCandleTile + 0, false, false, RenderType::Candle, 0, 0.1f },
        { "White Candle", DyedCandleTile + 1, DyedCandleTile + 1, DyedCandleTile + 1, false, false, RenderType::Candle, 12, 0.1f },
        { "Orange Candle", DyedCandleTile + 2, DyedCandleTile + 2, DyedCandleTile + 2, false, false, RenderType::Candle, 0, 0.1f },
        { "Orange Candle", DyedCandleTile + 3, DyedCandleTile + 3, DyedCandleTile + 3, false, false, RenderType::Candle, 3, 0.1f },
        { "Orange Candle", DyedCandleTile + 2, DyedCandleTile + 2, DyedCandleTile + 2, false, false, RenderType::Candle, 0, 0.1f },
        { "Orange Candle", DyedCandleTile + 3, DyedCandleTile + 3, DyedCandleTile + 3, false, false, RenderType::Candle, 6, 0.1f },
        { "Orange Candle", DyedCandleTile + 2, DyedCandleTile + 2, DyedCandleTile + 2, false, false, RenderType::Candle, 0, 0.1f },
        { "Orange Candle", DyedCandleTile + 3, DyedCandleTile + 3, DyedCandleTile + 3, false, false, RenderType::Candle, 9, 0.1f },
        { "Orange Candle", DyedCandleTile + 2, DyedCandleTile + 2, DyedCandleTile + 2, false, false, RenderType::Candle, 0, 0.1f },
        { "Orange Candle", DyedCandleTile + 3, DyedCandleTile + 3, DyedCandleTile + 3, false, false, RenderType::Candle, 12, 0.1f },
        { "Magenta Candle", DyedCandleTile + 4, DyedCandleTile + 4, DyedCandleTile + 4, false, false, RenderType::Candle, 0, 0.1f },
        { "Magenta Candle", DyedCandleTile + 5, DyedCandleTile + 5, DyedCandleTile + 5, false, false, RenderType::Candle, 3, 0.1f },
        { "Magenta Candle", DyedCandleTile + 4, DyedCandleTile + 4, DyedCandleTile + 4, false, false, RenderType::Candle, 0, 0.1f },
        { "Magenta Candle", DyedCandleTile + 5, DyedCandleTile + 5, DyedCandleTile + 5, false, false, RenderType::Candle, 6, 0.1f },
        { "Magenta Candle", DyedCandleTile + 4, DyedCandleTile + 4, DyedCandleTile + 4, false, false, RenderType::Candle, 0, 0.1f },
        { "Magenta Candle", DyedCandleTile + 5, DyedCandleTile + 5, DyedCandleTile + 5, false, false, RenderType::Candle, 9, 0.1f },
        { "Magenta Candle", DyedCandleTile + 4, DyedCandleTile + 4, DyedCandleTile + 4, false, false, RenderType::Candle, 0, 0.1f },
        { "Magenta Candle", DyedCandleTile + 5, DyedCandleTile + 5, DyedCandleTile + 5, false, false, RenderType::Candle, 12, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 6, DyedCandleTile + 6, DyedCandleTile + 6, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 7, DyedCandleTile + 7, DyedCandleTile + 7, false, false, RenderType::Candle, 3, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 6, DyedCandleTile + 6, DyedCandleTile + 6, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 7, DyedCandleTile + 7, DyedCandleTile + 7, false, false, RenderType::Candle, 6, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 6, DyedCandleTile + 6, DyedCandleTile + 6, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 7, DyedCandleTile + 7, DyedCandleTile + 7, false, false, RenderType::Candle, 9, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 6, DyedCandleTile + 6, DyedCandleTile + 6, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Blue Candle", DyedCandleTile + 7, DyedCandleTile + 7, DyedCandleTile + 7, false, false, RenderType::Candle, 12, 0.1f },
        { "Yellow Candle", DyedCandleTile + 8, DyedCandleTile + 8, DyedCandleTile + 8, false, false, RenderType::Candle, 0, 0.1f },
        { "Yellow Candle", DyedCandleTile + 9, DyedCandleTile + 9, DyedCandleTile + 9, false, false, RenderType::Candle, 3, 0.1f },
        { "Yellow Candle", DyedCandleTile + 8, DyedCandleTile + 8, DyedCandleTile + 8, false, false, RenderType::Candle, 0, 0.1f },
        { "Yellow Candle", DyedCandleTile + 9, DyedCandleTile + 9, DyedCandleTile + 9, false, false, RenderType::Candle, 6, 0.1f },
        { "Yellow Candle", DyedCandleTile + 8, DyedCandleTile + 8, DyedCandleTile + 8, false, false, RenderType::Candle, 0, 0.1f },
        { "Yellow Candle", DyedCandleTile + 9, DyedCandleTile + 9, DyedCandleTile + 9, false, false, RenderType::Candle, 9, 0.1f },
        { "Yellow Candle", DyedCandleTile + 8, DyedCandleTile + 8, DyedCandleTile + 8, false, false, RenderType::Candle, 0, 0.1f },
        { "Yellow Candle", DyedCandleTile + 9, DyedCandleTile + 9, DyedCandleTile + 9, false, false, RenderType::Candle, 12, 0.1f },
        { "Lime Candle", DyedCandleTile + 10, DyedCandleTile + 10, DyedCandleTile + 10, false, false, RenderType::Candle, 0, 0.1f },
        { "Lime Candle", DyedCandleTile + 11, DyedCandleTile + 11, DyedCandleTile + 11, false, false, RenderType::Candle, 3, 0.1f },
        { "Lime Candle", DyedCandleTile + 10, DyedCandleTile + 10, DyedCandleTile + 10, false, false, RenderType::Candle, 0, 0.1f },
        { "Lime Candle", DyedCandleTile + 11, DyedCandleTile + 11, DyedCandleTile + 11, false, false, RenderType::Candle, 6, 0.1f },
        { "Lime Candle", DyedCandleTile + 10, DyedCandleTile + 10, DyedCandleTile + 10, false, false, RenderType::Candle, 0, 0.1f },
        { "Lime Candle", DyedCandleTile + 11, DyedCandleTile + 11, DyedCandleTile + 11, false, false, RenderType::Candle, 9, 0.1f },
        { "Lime Candle", DyedCandleTile + 10, DyedCandleTile + 10, DyedCandleTile + 10, false, false, RenderType::Candle, 0, 0.1f },
        { "Lime Candle", DyedCandleTile + 11, DyedCandleTile + 11, DyedCandleTile + 11, false, false, RenderType::Candle, 12, 0.1f },
        { "Pink Candle", DyedCandleTile + 12, DyedCandleTile + 12, DyedCandleTile + 12, false, false, RenderType::Candle, 0, 0.1f },
        { "Pink Candle", DyedCandleTile + 13, DyedCandleTile + 13, DyedCandleTile + 13, false, false, RenderType::Candle, 3, 0.1f },
        { "Pink Candle", DyedCandleTile + 12, DyedCandleTile + 12, DyedCandleTile + 12, false, false, RenderType::Candle, 0, 0.1f },
        { "Pink Candle", DyedCandleTile + 13, DyedCandleTile + 13, DyedCandleTile + 13, false, false, RenderType::Candle, 6, 0.1f },
        { "Pink Candle", DyedCandleTile + 12, DyedCandleTile + 12, DyedCandleTile + 12, false, false, RenderType::Candle, 0, 0.1f },
        { "Pink Candle", DyedCandleTile + 13, DyedCandleTile + 13, DyedCandleTile + 13, false, false, RenderType::Candle, 9, 0.1f },
        { "Pink Candle", DyedCandleTile + 12, DyedCandleTile + 12, DyedCandleTile + 12, false, false, RenderType::Candle, 0, 0.1f },
        { "Pink Candle", DyedCandleTile + 13, DyedCandleTile + 13, DyedCandleTile + 13, false, false, RenderType::Candle, 12, 0.1f },
        { "Gray Candle", DyedCandleTile + 14, DyedCandleTile + 14, DyedCandleTile + 14, false, false, RenderType::Candle, 0, 0.1f },
        { "Gray Candle", DyedCandleTile + 15, DyedCandleTile + 15, DyedCandleTile + 15, false, false, RenderType::Candle, 3, 0.1f },
        { "Gray Candle", DyedCandleTile + 14, DyedCandleTile + 14, DyedCandleTile + 14, false, false, RenderType::Candle, 0, 0.1f },
        { "Gray Candle", DyedCandleTile + 15, DyedCandleTile + 15, DyedCandleTile + 15, false, false, RenderType::Candle, 6, 0.1f },
        { "Gray Candle", DyedCandleTile + 14, DyedCandleTile + 14, DyedCandleTile + 14, false, false, RenderType::Candle, 0, 0.1f },
        { "Gray Candle", DyedCandleTile + 15, DyedCandleTile + 15, DyedCandleTile + 15, false, false, RenderType::Candle, 9, 0.1f },
        { "Gray Candle", DyedCandleTile + 14, DyedCandleTile + 14, DyedCandleTile + 14, false, false, RenderType::Candle, 0, 0.1f },
        { "Gray Candle", DyedCandleTile + 15, DyedCandleTile + 15, DyedCandleTile + 15, false, false, RenderType::Candle, 12, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 16, DyedCandleTile + 16, DyedCandleTile + 16, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 17, DyedCandleTile + 17, DyedCandleTile + 17, false, false, RenderType::Candle, 3, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 16, DyedCandleTile + 16, DyedCandleTile + 16, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 17, DyedCandleTile + 17, DyedCandleTile + 17, false, false, RenderType::Candle, 6, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 16, DyedCandleTile + 16, DyedCandleTile + 16, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 17, DyedCandleTile + 17, DyedCandleTile + 17, false, false, RenderType::Candle, 9, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 16, DyedCandleTile + 16, DyedCandleTile + 16, false, false, RenderType::Candle, 0, 0.1f },
        { "Light Gray Candle", DyedCandleTile + 17, DyedCandleTile + 17, DyedCandleTile + 17, false, false, RenderType::Candle, 12, 0.1f },
        { "Cyan Candle", DyedCandleTile + 18, DyedCandleTile + 18, DyedCandleTile + 18, false, false, RenderType::Candle, 0, 0.1f },
        { "Cyan Candle", DyedCandleTile + 19, DyedCandleTile + 19, DyedCandleTile + 19, false, false, RenderType::Candle, 3, 0.1f },
        { "Cyan Candle", DyedCandleTile + 18, DyedCandleTile + 18, DyedCandleTile + 18, false, false, RenderType::Candle, 0, 0.1f },
        { "Cyan Candle", DyedCandleTile + 19, DyedCandleTile + 19, DyedCandleTile + 19, false, false, RenderType::Candle, 6, 0.1f },
        { "Cyan Candle", DyedCandleTile + 18, DyedCandleTile + 18, DyedCandleTile + 18, false, false, RenderType::Candle, 0, 0.1f },
        { "Cyan Candle", DyedCandleTile + 19, DyedCandleTile + 19, DyedCandleTile + 19, false, false, RenderType::Candle, 9, 0.1f },
        { "Cyan Candle", DyedCandleTile + 18, DyedCandleTile + 18, DyedCandleTile + 18, false, false, RenderType::Candle, 0, 0.1f },
        { "Cyan Candle", DyedCandleTile + 19, DyedCandleTile + 19, DyedCandleTile + 19, false, false, RenderType::Candle, 12, 0.1f },
        { "Purple Candle", DyedCandleTile + 20, DyedCandleTile + 20, DyedCandleTile + 20, false, false, RenderType::Candle, 0, 0.1f },
        { "Purple Candle", DyedCandleTile + 21, DyedCandleTile + 21, DyedCandleTile + 21, false, false, RenderType::Candle, 3, 0.1f },
        { "Purple Candle", DyedCandleTile + 20, DyedCandleTile + 20, DyedCandleTile + 20, false, false, RenderType::Candle, 0, 0.1f },
        { "Purple Candle", DyedCandleTile + 21, DyedCandleTile + 21, DyedCandleTile + 21, false, false, RenderType::Candle, 6, 0.1f },
        { "Purple Candle", DyedCandleTile + 20, DyedCandleTile + 20, DyedCandleTile + 20, false, false, RenderType::Candle, 0, 0.1f },
        { "Purple Candle", DyedCandleTile + 21, DyedCandleTile + 21, DyedCandleTile + 21, false, false, RenderType::Candle, 9, 0.1f },
        { "Purple Candle", DyedCandleTile + 20, DyedCandleTile + 20, DyedCandleTile + 20, false, false, RenderType::Candle, 0, 0.1f },
        { "Purple Candle", DyedCandleTile + 21, DyedCandleTile + 21, DyedCandleTile + 21, false, false, RenderType::Candle, 12, 0.1f },
        { "Blue Candle", DyedCandleTile + 22, DyedCandleTile + 22, DyedCandleTile + 22, false, false, RenderType::Candle, 0, 0.1f },
        { "Blue Candle", DyedCandleTile + 23, DyedCandleTile + 23, DyedCandleTile + 23, false, false, RenderType::Candle, 3, 0.1f },
        { "Blue Candle", DyedCandleTile + 22, DyedCandleTile + 22, DyedCandleTile + 22, false, false, RenderType::Candle, 0, 0.1f },
        { "Blue Candle", DyedCandleTile + 23, DyedCandleTile + 23, DyedCandleTile + 23, false, false, RenderType::Candle, 6, 0.1f },
        { "Blue Candle", DyedCandleTile + 22, DyedCandleTile + 22, DyedCandleTile + 22, false, false, RenderType::Candle, 0, 0.1f },
        { "Blue Candle", DyedCandleTile + 23, DyedCandleTile + 23, DyedCandleTile + 23, false, false, RenderType::Candle, 9, 0.1f },
        { "Blue Candle", DyedCandleTile + 22, DyedCandleTile + 22, DyedCandleTile + 22, false, false, RenderType::Candle, 0, 0.1f },
        { "Blue Candle", DyedCandleTile + 23, DyedCandleTile + 23, DyedCandleTile + 23, false, false, RenderType::Candle, 12, 0.1f },
        { "Brown Candle", DyedCandleTile + 24, DyedCandleTile + 24, DyedCandleTile + 24, false, false, RenderType::Candle, 0, 0.1f },
        { "Brown Candle", DyedCandleTile + 25, DyedCandleTile + 25, DyedCandleTile + 25, false, false, RenderType::Candle, 3, 0.1f },
        { "Brown Candle", DyedCandleTile + 24, DyedCandleTile + 24, DyedCandleTile + 24, false, false, RenderType::Candle, 0, 0.1f },
        { "Brown Candle", DyedCandleTile + 25, DyedCandleTile + 25, DyedCandleTile + 25, false, false, RenderType::Candle, 6, 0.1f },
        { "Brown Candle", DyedCandleTile + 24, DyedCandleTile + 24, DyedCandleTile + 24, false, false, RenderType::Candle, 0, 0.1f },
        { "Brown Candle", DyedCandleTile + 25, DyedCandleTile + 25, DyedCandleTile + 25, false, false, RenderType::Candle, 9, 0.1f },
        { "Brown Candle", DyedCandleTile + 24, DyedCandleTile + 24, DyedCandleTile + 24, false, false, RenderType::Candle, 0, 0.1f },
        { "Brown Candle", DyedCandleTile + 25, DyedCandleTile + 25, DyedCandleTile + 25, false, false, RenderType::Candle, 12, 0.1f },
        { "Green Candle", DyedCandleTile + 26, DyedCandleTile + 26, DyedCandleTile + 26, false, false, RenderType::Candle, 0, 0.1f },
        { "Green Candle", DyedCandleTile + 27, DyedCandleTile + 27, DyedCandleTile + 27, false, false, RenderType::Candle, 3, 0.1f },
        { "Green Candle", DyedCandleTile + 26, DyedCandleTile + 26, DyedCandleTile + 26, false, false, RenderType::Candle, 0, 0.1f },
        { "Green Candle", DyedCandleTile + 27, DyedCandleTile + 27, DyedCandleTile + 27, false, false, RenderType::Candle, 6, 0.1f },
        { "Green Candle", DyedCandleTile + 26, DyedCandleTile + 26, DyedCandleTile + 26, false, false, RenderType::Candle, 0, 0.1f },
        { "Green Candle", DyedCandleTile + 27, DyedCandleTile + 27, DyedCandleTile + 27, false, false, RenderType::Candle, 9, 0.1f },
        { "Green Candle", DyedCandleTile + 26, DyedCandleTile + 26, DyedCandleTile + 26, false, false, RenderType::Candle, 0, 0.1f },
        { "Green Candle", DyedCandleTile + 27, DyedCandleTile + 27, DyedCandleTile + 27, false, false, RenderType::Candle, 12, 0.1f },
        { "Red Candle", DyedCandleTile + 28, DyedCandleTile + 28, DyedCandleTile + 28, false, false, RenderType::Candle, 0, 0.1f },
        { "Red Candle", DyedCandleTile + 29, DyedCandleTile + 29, DyedCandleTile + 29, false, false, RenderType::Candle, 3, 0.1f },
        { "Red Candle", DyedCandleTile + 28, DyedCandleTile + 28, DyedCandleTile + 28, false, false, RenderType::Candle, 0, 0.1f },
        { "Red Candle", DyedCandleTile + 29, DyedCandleTile + 29, DyedCandleTile + 29, false, false, RenderType::Candle, 6, 0.1f },
        { "Red Candle", DyedCandleTile + 28, DyedCandleTile + 28, DyedCandleTile + 28, false, false, RenderType::Candle, 0, 0.1f },
        { "Red Candle", DyedCandleTile + 29, DyedCandleTile + 29, DyedCandleTile + 29, false, false, RenderType::Candle, 9, 0.1f },
        { "Red Candle", DyedCandleTile + 28, DyedCandleTile + 28, DyedCandleTile + 28, false, false, RenderType::Candle, 0, 0.1f },
        { "Red Candle", DyedCandleTile + 29, DyedCandleTile + 29, DyedCandleTile + 29, false, false, RenderType::Candle, 12, 0.1f },
        { "Black Candle", DyedCandleTile + 30, DyedCandleTile + 30, DyedCandleTile + 30, false, false, RenderType::Candle, 0, 0.1f },
        { "Black Candle", DyedCandleTile + 31, DyedCandleTile + 31, DyedCandleTile + 31, false, false, RenderType::Candle, 3, 0.1f },
        { "Black Candle", DyedCandleTile + 30, DyedCandleTile + 30, DyedCandleTile + 30, false, false, RenderType::Candle, 0, 0.1f },
        { "Black Candle", DyedCandleTile + 31, DyedCandleTile + 31, DyedCandleTile + 31, false, false, RenderType::Candle, 6, 0.1f },
        { "Black Candle", DyedCandleTile + 30, DyedCandleTile + 30, DyedCandleTile + 30, false, false, RenderType::Candle, 0, 0.1f },
        { "Black Candle", DyedCandleTile + 31, DyedCandleTile + 31, DyedCandleTile + 31, false, false, RenderType::Candle, 9, 0.1f },
        { "Black Candle", DyedCandleTile + 30, DyedCandleTile + 30, DyedCandleTile + 30, false, false, RenderType::Candle, 0, 0.1f },
        { "Black Candle", DyedCandleTile + 31, DyedCandleTile + 31, DyedCandleTile + 31, false, false, RenderType::Candle, 12, 0.1f },
        { "Cake with Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with White Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with White Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Orange Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Orange Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Magenta Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Magenta Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Light Blue Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Light Blue Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Yellow Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Yellow Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Lime Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Lime Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Pink Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Pink Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Gray Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Gray Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Light Gray Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Light Gray Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Cyan Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Cyan Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Purple Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Purple Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Blue Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Blue Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Brown Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Brown Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Green Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Green Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Red Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Red Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "Cake with Black Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 0, 0.5f },
        { "Cake with Black Candle", CakeTop, CakeBottom, CakeSide, true, false, RenderType::Candle, 3, 0.5f },
        { "White Shulker Box", ShulkerBoxColourTile + 0, ShulkerBoxColourTile + 0, ShulkerBoxColourTile + 0, true, true, RenderType::Cube, 0, 2.0f },
        { "Orange Shulker Box", ShulkerBoxColourTile + 1, ShulkerBoxColourTile + 1, ShulkerBoxColourTile + 1, true, true, RenderType::Cube, 0, 2.0f },
        { "Magenta Shulker Box", ShulkerBoxColourTile + 2, ShulkerBoxColourTile + 2, ShulkerBoxColourTile + 2, true, true, RenderType::Cube, 0, 2.0f },
        { "Light Blue Shulker Box", ShulkerBoxColourTile + 3, ShulkerBoxColourTile + 3, ShulkerBoxColourTile + 3, true, true, RenderType::Cube, 0, 2.0f },
        { "Yellow Shulker Box", ShulkerBoxColourTile + 4, ShulkerBoxColourTile + 4, ShulkerBoxColourTile + 4, true, true, RenderType::Cube, 0, 2.0f },
        { "Lime Shulker Box", ShulkerBoxColourTile + 5, ShulkerBoxColourTile + 5, ShulkerBoxColourTile + 5, true, true, RenderType::Cube, 0, 2.0f },
        { "Pink Shulker Box", ShulkerBoxColourTile + 6, ShulkerBoxColourTile + 6, ShulkerBoxColourTile + 6, true, true, RenderType::Cube, 0, 2.0f },
        { "Gray Shulker Box", ShulkerBoxColourTile + 7, ShulkerBoxColourTile + 7, ShulkerBoxColourTile + 7, true, true, RenderType::Cube, 0, 2.0f },
        { "Light Gray Shulker Box", ShulkerBoxColourTile + 8, ShulkerBoxColourTile + 8, ShulkerBoxColourTile + 8, true, true, RenderType::Cube, 0, 2.0f },
        { "Cyan Shulker Box", ShulkerBoxColourTile + 9, ShulkerBoxColourTile + 9, ShulkerBoxColourTile + 9, true, true, RenderType::Cube, 0, 2.0f },
        { "Purple Shulker Box", ShulkerBoxColourTile + 10, ShulkerBoxColourTile + 10, ShulkerBoxColourTile + 10, true, true, RenderType::Cube, 0, 2.0f },
        { "Blue Shulker Box", ShulkerBoxColourTile + 11, ShulkerBoxColourTile + 11, ShulkerBoxColourTile + 11, true, true, RenderType::Cube, 0, 2.0f },
        { "Brown Shulker Box", ShulkerBoxColourTile + 12, ShulkerBoxColourTile + 12, ShulkerBoxColourTile + 12, true, true, RenderType::Cube, 0, 2.0f },
        { "Green Shulker Box", ShulkerBoxColourTile + 13, ShulkerBoxColourTile + 13, ShulkerBoxColourTile + 13, true, true, RenderType::Cube, 0, 2.0f },
        { "Red Shulker Box", ShulkerBoxColourTile + 14, ShulkerBoxColourTile + 14, ShulkerBoxColourTile + 14, true, true, RenderType::Cube, 0, 2.0f },
        { "Black Shulker Box", ShulkerBoxColourTile + 15, ShulkerBoxColourTile + 15, ShulkerBoxColourTile + 15, true, true, RenderType::Cube, 0, 2.0f },
        { "White Bed", BedFootTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "White Bed", BedHeadTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "White Bed", BedFootTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "White Bed", BedHeadTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "White Bed", BedFootTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "White Bed", BedHeadTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "White Bed", BedFootTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "White Bed", BedHeadTopTile + 0, Planks, BedSidesTile + 0, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedFootTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedHeadTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedFootTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedHeadTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedFootTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedHeadTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedFootTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Orange Bed", BedHeadTopTile + 1, Planks, BedSidesTile + 1, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedFootTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedHeadTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedFootTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedHeadTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedFootTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedHeadTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedFootTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Magenta Bed", BedHeadTopTile + 2, Planks, BedSidesTile + 2, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedFootTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedHeadTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedFootTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedHeadTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedFootTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedHeadTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedFootTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Blue Bed", BedHeadTopTile + 3, Planks, BedSidesTile + 3, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedFootTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedHeadTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedFootTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedHeadTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedFootTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedHeadTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedFootTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Yellow Bed", BedHeadTopTile + 4, Planks, BedSidesTile + 4, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedFootTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedHeadTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedFootTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedHeadTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedFootTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedHeadTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedFootTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Lime Bed", BedHeadTopTile + 5, Planks, BedSidesTile + 5, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedFootTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedHeadTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedFootTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedHeadTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedFootTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedHeadTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedFootTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Pink Bed", BedHeadTopTile + 6, Planks, BedSidesTile + 6, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedFootTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedHeadTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedFootTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedHeadTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedFootTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedHeadTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedFootTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Gray Bed", BedHeadTopTile + 7, Planks, BedSidesTile + 7, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedFootTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedHeadTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedFootTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedHeadTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedFootTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedHeadTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedFootTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Light Gray Bed", BedHeadTopTile + 8, Planks, BedSidesTile + 8, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedFootTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedHeadTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedFootTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedHeadTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedFootTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedHeadTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedFootTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Cyan Bed", BedHeadTopTile + 9, Planks, BedSidesTile + 9, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedFootTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedHeadTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedFootTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedHeadTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedFootTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedHeadTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedFootTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Purple Bed", BedHeadTopTile + 10, Planks, BedSidesTile + 10, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedFootTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedHeadTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedFootTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedHeadTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedFootTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedHeadTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedFootTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Blue Bed", BedHeadTopTile + 11, Planks, BedSidesTile + 11, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedFootTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedHeadTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedFootTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedHeadTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedFootTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedHeadTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedFootTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Brown Bed", BedHeadTopTile + 12, Planks, BedSidesTile + 12, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedFootTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedHeadTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedFootTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedHeadTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedFootTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedHeadTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedFootTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Green Bed", BedHeadTopTile + 13, Planks, BedSidesTile + 13, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedFootTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedHeadTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedFootTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedHeadTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedFootTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedHeadTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedFootTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Red Bed", BedHeadTopTile + 14, Planks, BedSidesTile + 14, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedFootTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedHeadTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedFootTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedHeadTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedFootTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedHeadTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedFootTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Black Bed", BedHeadTopTile + 15, Planks, BedSidesTile + 15, true, false, RenderType::Bed, 0, 0.2f },
        { "Chest", ChestTileFirst + 0, ChestTileFirst + 0 + 4, ChestTileFirst + 0 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 5, ChestTileFirst + 5 + 4, ChestTileFirst + 5 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 10, ChestTileFirst + 10 + 4, ChestTileFirst + 10 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 0, ChestTileFirst + 0 + 4, ChestTileFirst + 0 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 5, ChestTileFirst + 5 + 4, ChestTileFirst + 5 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 10, ChestTileFirst + 10 + 4, ChestTileFirst + 10 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 5, ChestTileFirst + 5 + 4, ChestTileFirst + 5 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 10, ChestTileFirst + 10 + 4, ChestTileFirst + 10 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 0, ChestTileFirst + 0 + 4, ChestTileFirst + 0 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 5, ChestTileFirst + 5 + 4, ChestTileFirst + 5 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Chest", ChestTileFirst + 10, ChestTileFirst + 10 + 4, ChestTileFirst + 10 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 15, ChestTileFirst + 15 + 4, ChestTileFirst + 15 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 20, ChestTileFirst + 20 + 4, ChestTileFirst + 20 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Trapped Chest", ChestTileFirst + 25, ChestTileFirst + 25 + 4, ChestTileFirst + 25 + 1, true, false, RenderType::Chest, 0, 2.5f },
        { "Ender Chest", ChestTileFirst + 45, ChestTileFirst + 45 + 4, ChestTileFirst + 45 + 1, true, false, RenderType::Chest, 7, 22.5f },
        { "Ender Chest", ChestTileFirst + 45, ChestTileFirst + 45 + 4, ChestTileFirst + 45 + 1, true, false, RenderType::Chest, 7, 22.5f },
        { "Ender Chest", ChestTileFirst + 45, ChestTileFirst + 45 + 4, ChestTileFirst + 45 + 1, true, false, RenderType::Chest, 7, 22.5f },
        { "Ender Chest", ChestTileFirst + 45, ChestTileFirst + 45 + 4, ChestTileFirst + 45 + 1, true, false, RenderType::Chest, 7, 22.5f },
        { "Waxed Block of Copper", Tiles::CopperBlock, Tiles::CopperBlock, Tiles::CopperBlock, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Exposed Copper", ExposedCopper, ExposedCopper, ExposedCopper, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Weathered Copper", WeatheredCopper, WeatheredCopper, WeatheredCopper, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Oxidized Copper", OxidizedCopper, OxidizedCopper, OxidizedCopper, true, true, RenderType::Cube, 0, 3.0f },
        { "Cut Copper", CutCopperTile + 0, CutCopperTile + 0, CutCopperTile + 0, true, true, RenderType::Cube, 0, 3.0f },
        { "Exposed Cut Copper", CutCopperTile + 1, CutCopperTile + 1, CutCopperTile + 1, true, true, RenderType::Cube, 0, 3.0f },
        { "Weathered Cut Copper", CutCopperTile + 2, CutCopperTile + 2, CutCopperTile + 2, true, true, RenderType::Cube, 0, 3.0f },
        { "Oxidized Cut Copper", CutCopperTile + 3, CutCopperTile + 3, CutCopperTile + 3, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Cut Copper", CutCopperTile + 0, CutCopperTile + 0, CutCopperTile + 0, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Exposed Cut Copper", CutCopperTile + 1, CutCopperTile + 1, CutCopperTile + 1, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Weathered Cut Copper", CutCopperTile + 2, CutCopperTile + 2, CutCopperTile + 2, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Oxidized Cut Copper", CutCopperTile + 3, CutCopperTile + 3, CutCopperTile + 3, true, true, RenderType::Cube, 0, 3.0f },
        { "Chiseled Copper", ChiseledCopperTile + 0, ChiseledCopperTile + 0, ChiseledCopperTile + 0, true, true, RenderType::Cube, 0, 3.0f },
        { "Exposed Chiseled Copper", ChiseledCopperTile + 1, ChiseledCopperTile + 1, ChiseledCopperTile + 1, true, true, RenderType::Cube, 0, 3.0f },
        { "Weathered Chiseled Copper", ChiseledCopperTile + 2, ChiseledCopperTile + 2, ChiseledCopperTile + 2, true, true, RenderType::Cube, 0, 3.0f },
        { "Oxidized Chiseled Copper", ChiseledCopperTile + 3, ChiseledCopperTile + 3, ChiseledCopperTile + 3, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Chiseled Copper", ChiseledCopperTile + 0, ChiseledCopperTile + 0, ChiseledCopperTile + 0, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Exposed Chiseled Copper", ChiseledCopperTile + 1, ChiseledCopperTile + 1, ChiseledCopperTile + 1, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Weathered Chiseled Copper", ChiseledCopperTile + 2, ChiseledCopperTile + 2, ChiseledCopperTile + 2, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Oxidized Chiseled Copper", ChiseledCopperTile + 3, ChiseledCopperTile + 3, ChiseledCopperTile + 3, true, true, RenderType::Cube, 0, 3.0f },
        { "Copper Grate", CopperGrateTile + 0, CopperGrateTile + 0, CopperGrateTile + 0, true, false, RenderType::Cube, 0, 3.0f },
        { "Exposed Copper Grate", CopperGrateTile + 1, CopperGrateTile + 1, CopperGrateTile + 1, true, false, RenderType::Cube, 0, 3.0f },
        { "Weathered Copper Grate", CopperGrateTile + 2, CopperGrateTile + 2, CopperGrateTile + 2, true, false, RenderType::Cube, 0, 3.0f },
        { "Oxidized Copper Grate", CopperGrateTile + 3, CopperGrateTile + 3, CopperGrateTile + 3, true, false, RenderType::Cube, 0, 3.0f },
        { "Waxed Copper Grate", CopperGrateTile + 0, CopperGrateTile + 0, CopperGrateTile + 0, true, false, RenderType::Cube, 0, 3.0f },
        { "Waxed Exposed Copper Grate", CopperGrateTile + 1, CopperGrateTile + 1, CopperGrateTile + 1, true, false, RenderType::Cube, 0, 3.0f },
        { "Waxed Weathered Copper Grate", CopperGrateTile + 2, CopperGrateTile + 2, CopperGrateTile + 2, true, false, RenderType::Cube, 0, 3.0f },
        { "Waxed Oxidized Copper Grate", CopperGrateTile + 3, CopperGrateTile + 3, CopperGrateTile + 3, true, false, RenderType::Cube, 0, 3.0f },
        { "Cut Copper Slab", CutCopperTile + 0, CutCopperTile + 0, CutCopperTile + 0, true, false, RenderType::Slab, 0, 3.0f },
        { "Exposed Cut Copper Slab", CutCopperTile + 1, CutCopperTile + 1, CutCopperTile + 1, true, false, RenderType::Slab, 0, 3.0f },
        { "Weathered Cut Copper Slab", CutCopperTile + 2, CutCopperTile + 2, CutCopperTile + 2, true, false, RenderType::Slab, 0, 3.0f },
        { "Oxidized Cut Copper Slab", CutCopperTile + 3, CutCopperTile + 3, CutCopperTile + 3, true, false, RenderType::Slab, 0, 3.0f },
        { "Waxed Cut Copper Slab", CutCopperTile + 0, CutCopperTile + 0, CutCopperTile + 0, true, false, RenderType::Slab, 0, 3.0f },
        { "Waxed Exposed Cut Copper Slab", CutCopperTile + 1, CutCopperTile + 1, CutCopperTile + 1, true, false, RenderType::Slab, 0, 3.0f },
        { "Waxed Weathered Cut Copper Slab", CutCopperTile + 2, CutCopperTile + 2, CutCopperTile + 2, true, false, RenderType::Slab, 0, 3.0f },
        { "Waxed Oxidized Cut Copper Slab", CutCopperTile + 3, CutCopperTile + 3, CutCopperTile + 3, true, false, RenderType::Slab, 0, 3.0f },
        { "Exposed Copper Bulb", CopperBulbStageTile + 0, CopperBulbStageTile + 0, CopperBulbStageTile + 0, true, true, RenderType::Cube, 0, 3.0f },
        { "Exposed Copper Bulb", CopperBulbStageTile + 1, CopperBulbStageTile + 1, CopperBulbStageTile + 1, true, true, RenderType::Cube, 12, 3.0f },
        { "Exposed Copper Bulb", CopperBulbStageTile + 2, CopperBulbStageTile + 2, CopperBulbStageTile + 2, true, true, RenderType::Cube, 0, 3.0f },
        { "Exposed Copper Bulb", CopperBulbStageTile + 3, CopperBulbStageTile + 3, CopperBulbStageTile + 3, true, true, RenderType::Cube, 12, 3.0f },
        { "Weathered Copper Bulb", CopperBulbStageTile + 4, CopperBulbStageTile + 4, CopperBulbStageTile + 4, true, true, RenderType::Cube, 0, 3.0f },
        { "Weathered Copper Bulb", CopperBulbStageTile + 5, CopperBulbStageTile + 5, CopperBulbStageTile + 5, true, true, RenderType::Cube, 8, 3.0f },
        { "Weathered Copper Bulb", CopperBulbStageTile + 6, CopperBulbStageTile + 6, CopperBulbStageTile + 6, true, true, RenderType::Cube, 0, 3.0f },
        { "Weathered Copper Bulb", CopperBulbStageTile + 7, CopperBulbStageTile + 7, CopperBulbStageTile + 7, true, true, RenderType::Cube, 8, 3.0f },
        { "Oxidized Copper Bulb", CopperBulbStageTile + 8, CopperBulbStageTile + 8, CopperBulbStageTile + 8, true, true, RenderType::Cube, 0, 3.0f },
        { "Oxidized Copper Bulb", CopperBulbStageTile + 9, CopperBulbStageTile + 9, CopperBulbStageTile + 9, true, true, RenderType::Cube, 4, 3.0f },
        { "Oxidized Copper Bulb", CopperBulbStageTile + 10, CopperBulbStageTile + 10, CopperBulbStageTile + 10, true, true, RenderType::Cube, 0, 3.0f },
        { "Oxidized Copper Bulb", CopperBulbStageTile + 11, CopperBulbStageTile + 11, CopperBulbStageTile + 11, true, true, RenderType::Cube, 4, 3.0f },
        { "Waxed Copper Bulb", CopperBulbTile, CopperBulbTile, CopperBulbTile, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Copper Bulb", CopperBulbLitTile, CopperBulbLitTile, CopperBulbLitTile, true, true, RenderType::Cube, 15, 3.0f },
        { "Waxed Copper Bulb", CopperBulbPoweredTile, CopperBulbPoweredTile, CopperBulbPoweredTile, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Copper Bulb", CopperBulbLitPoweredTile, CopperBulbLitPoweredTile, CopperBulbLitPoweredTile, true, true, RenderType::Cube, 15, 3.0f },
        { "Waxed Exposed Copper Bulb", CopperBulbStageTile + 0, CopperBulbStageTile + 0, CopperBulbStageTile + 0, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Exposed Copper Bulb", CopperBulbStageTile + 1, CopperBulbStageTile + 1, CopperBulbStageTile + 1, true, true, RenderType::Cube, 12, 3.0f },
        { "Waxed Exposed Copper Bulb", CopperBulbStageTile + 2, CopperBulbStageTile + 2, CopperBulbStageTile + 2, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Exposed Copper Bulb", CopperBulbStageTile + 3, CopperBulbStageTile + 3, CopperBulbStageTile + 3, true, true, RenderType::Cube, 12, 3.0f },
        { "Waxed Weathered Copper Bulb", CopperBulbStageTile + 4, CopperBulbStageTile + 4, CopperBulbStageTile + 4, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Weathered Copper Bulb", CopperBulbStageTile + 5, CopperBulbStageTile + 5, CopperBulbStageTile + 5, true, true, RenderType::Cube, 8, 3.0f },
        { "Waxed Weathered Copper Bulb", CopperBulbStageTile + 6, CopperBulbStageTile + 6, CopperBulbStageTile + 6, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Weathered Copper Bulb", CopperBulbStageTile + 7, CopperBulbStageTile + 7, CopperBulbStageTile + 7, true, true, RenderType::Cube, 8, 3.0f },
        { "Waxed Oxidized Copper Bulb", CopperBulbStageTile + 8, CopperBulbStageTile + 8, CopperBulbStageTile + 8, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Oxidized Copper Bulb", CopperBulbStageTile + 9, CopperBulbStageTile + 9, CopperBulbStageTile + 9, true, true, RenderType::Cube, 4, 3.0f },
        { "Waxed Oxidized Copper Bulb", CopperBulbStageTile + 10, CopperBulbStageTile + 10, CopperBulbStageTile + 10, true, true, RenderType::Cube, 0, 3.0f },
        { "Waxed Oxidized Copper Bulb", CopperBulbStageTile + 11, CopperBulbStageTile + 11, CopperBulbStageTile + 11, true, true, RenderType::Cube, 4, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, CopperDoorBottomTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Copper Door", CopperDoorTopTile + 0, CopperDoorTopTile + 0, CopperDoorTopTile + 0, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, CopperDoorBottomTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Exposed Copper Door", CopperDoorTopTile + 1, CopperDoorTopTile + 1, CopperDoorTopTile + 1, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, CopperDoorBottomTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Weathered Copper Door", CopperDoorTopTile + 2, CopperDoorTopTile + 2, CopperDoorTopTile + 2, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, CopperDoorBottomTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Waxed Oxidized Copper Door", CopperDoorTopTile + 3, CopperDoorTopTile + 3, CopperDoorTopTile + 3, true, false, RenderType::Door, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Copper Trapdoor", CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, CopperTrapdoorTile + 0, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Exposed Copper Trapdoor", CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, CopperTrapdoorTile + 1, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Weathered Copper Trapdoor", CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, CopperTrapdoorTile + 2, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Waxed Oxidized Copper Trapdoor", CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, CopperTrapdoorTile + 3, true, false, RenderType::Trapdoor, 0, 3.0f },
        { "Lightning Rod", LightningRodTile, LightningRodTile, LightningRodTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodOnTile, LightningRodOnTile, LightningRodOnTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodTile, LightningRodTile, LightningRodTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodOnTile, LightningRodOnTile, LightningRodOnTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodTile, LightningRodTile, LightningRodTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodOnTile, LightningRodOnTile, LightningRodOnTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodTile, LightningRodTile, LightningRodTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodOnTile, LightningRodOnTile, LightningRodOnTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodTile, LightningRodTile, LightningRodTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodOnTile, LightningRodOnTile, LightningRodOnTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodTile, LightningRodTile, LightningRodTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Lightning Rod", LightningRodOnTile, LightningRodOnTile, LightningRodOnTile, true, false, RenderType::Rod, 0, 3.0f },
        { "Allium", AlliumTile, AlliumTile, AlliumTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Azure Bluet", AzureBluetTile, AzureBluetTile, AzureBluetTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Blue Orchid", BlueOrchidTile, BlueOrchidTile, BlueOrchidTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Cornflower", CornflowerTile, CornflowerTile, CornflowerTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Lily of the Valley", LilyOfTheValleyTile, LilyOfTheValleyTile, LilyOfTheValleyTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Oxeye Daisy", OxeyeDaisyTile, OxeyeDaisyTile, OxeyeDaisyTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Red Tulip", RedTulipTile, RedTulipTile, RedTulipTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Orange Tulip", OrangeTulipTile, OrangeTulipTile, OrangeTulipTile, false, false, RenderType::Cross, 0, 0.0f },
        { "White Tulip", WhiteTulipTile, WhiteTulipTile, WhiteTulipTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pink Tulip", PinkTulipTile, PinkTulipTile, PinkTulipTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Wither Rose", WitherRoseTile, WitherRoseTile, WitherRoseTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Torchflower", TorchflowerTile, TorchflowerTile, TorchflowerTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Fern", FernTile, FernTile, FernTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Dead Bush", DeadBushTile, DeadBushTile, DeadBushTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Torchflower Crop", TorchflowerCrop0Tile, TorchflowerCrop0Tile, TorchflowerCrop0Tile, false, false, RenderType::Cross, 0, 0.0f },
        { "Torchflower Crop", TorchflowerCrop1Tile, TorchflowerCrop1Tile, TorchflowerCrop1Tile, false, false, RenderType::Cross, 0, 0.0f },
        { "Sunflower", SunflowerBottomTile, SunflowerBottomTile, SunflowerBottomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Sunflower", SunflowerTopTile, SunflowerTopTile, SunflowerTopTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Lilac", LilacBottomTile, LilacBottomTile, LilacBottomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Lilac", LilacTopTile, LilacTopTile, LilacTopTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Rose Bush", RoseBushBottomTile, RoseBushBottomTile, RoseBushBottomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Rose Bush", RoseBushTopTile, RoseBushTopTile, RoseBushTopTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Peony", PeonyBottomTile, PeonyBottomTile, PeonyBottomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Peony", PeonyTopTile, PeonyTopTile, PeonyTopTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Tall Grass", TallGrassBottomTile, TallGrassBottomTile, TallGrassBottomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Tall Grass", TallGrassTopTile, TallGrassTopTile, TallGrassTopTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Large Fern", LargeFernBottomTile, LargeFernBottomTile, LargeFernBottomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Large Fern", LargeFernTopTile, LargeFernTopTile, LargeFernTopTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Plant", PitcherBottomTile + 4, PitcherBottomTile + 4, PitcherBottomTile + 4, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Plant", PitcherTopTile + 1, PitcherTopTile + 1, PitcherTopTile + 1, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Crop", PitcherBottomTile + 0, PitcherBottomTile + 0, PitcherBottomTile + 0, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Crop", PitcherBottomTile + 1, PitcherBottomTile + 1, PitcherBottomTile + 1, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Crop", PitcherBottomTile + 2, PitcherBottomTile + 2, PitcherBottomTile + 2, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Crop", PitcherBottomTile + 3, PitcherBottomTile + 3, PitcherBottomTile + 3, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Crop", PitcherBottomTile + 4, PitcherBottomTile + 4, PitcherBottomTile + 4, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Crop", PitcherTopTile + 0, PitcherTopTile + 0, PitcherTopTile + 0, false, false, RenderType::Cross, 0, 0.0f },
        { "Pitcher Crop", PitcherTopTile + 1, PitcherTopTile + 1, PitcherTopTile + 1, false, false, RenderType::Cross, 0, 0.0f },
        { "Flower Pot", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Poppy", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Dandelion", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Allium", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Azure Bluet", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Blue Orchid", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Cornflower", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Lily of the Valley", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Oxeye Daisy", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Red Tulip", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Orange Tulip", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted White Tulip", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Pink Tulip", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Wither Rose", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Torchflower", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Fern", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Dead Bush", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Cactus", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Bamboo", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Oak Sapling", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Spruce Sapling", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Birch Sapling", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Jungle Sapling", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Acacia Sapling", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Dark Oak Sapling", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Cherry Sapling", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Mangrove Propagule", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Crimson Fungus", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Warped Fungus", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Crimson Roots", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Warped Roots", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Brown Mushroom", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Red Mushroom", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Brown Mushroom", BrownMushroomTile, BrownMushroomTile, BrownMushroomTile, false, false, RenderType::Cross, 1, 0.0f },
        { "Red Mushroom", RedMushroomTile, RedMushroomTile, RedMushroomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Brown Mushroom Block", BrownMushroomBlockTile, MushroomInsideTile, BrownMushroomBlockTile, true, true, RenderType::Cube, 0, 0.2f },
        { "Red Mushroom Block", RedMushroomBlockTile, MushroomInsideTile, RedMushroomBlockTile, true, true, RenderType::Cube, 0, 0.2f },
        { "Mushroom Stem", MushroomInsideTile, MushroomInsideTile, MushroomStemTile, true, true, RenderType::Cube, 0, 0.2f },
        { "Mycelium", MyceliumTopTile, Dirt, MyceliumSideTile, true, true, RenderType::Cube, 0, 0.6f },
        { "Podzol", PodzolTopTile, Dirt, PodzolSideTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Coarse Dirt", CoarseDirtTile, CoarseDirtTile, CoarseDirtTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Rooted Dirt", RootedDirtTile, RootedDirtTile, RootedDirtTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Hanging Roots", HangingRootsTile, HangingRootsTile, HangingRootsTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Cave Vines", CaveVinesTile, CaveVinesTile, CaveVinesTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Cave Vines", CaveVinesLitTile, CaveVinesLitTile, CaveVinesLitTile, false, false, RenderType::Cross, 14, 0.0f },
        { "Cave Vines", CaveVinesPlantTile, CaveVinesPlantTile, CaveVinesPlantTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Spore Blossom", SporeBlossomTile, SporeBlossomBaseTile, SporeBlossomTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Big Dripleaf", BigDripleafStemTile, BigDripleafStemTile, BigDripleafStemTile, false, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafStemTile, BigDripleafStemTile, BigDripleafStemTile, false, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafStemTile, BigDripleafStemTile, BigDripleafStemTile, false, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafStemTile, BigDripleafStemTile, BigDripleafStemTile, false, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Big Dripleaf", BigDripleafTopTile, BigDripleafStemTile, BigDripleafSideTile, true, false, RenderType::Lush, 0, 0.1f },
        { "Small Dripleaf", SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Small Dripleaf", SmallDripleafTopTile, SmallDripleafStemTopTile, SmallDripleafSideTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Small Dripleaf", SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Small Dripleaf", SmallDripleafTopTile, SmallDripleafStemTopTile, SmallDripleafSideTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Small Dripleaf", SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Small Dripleaf", SmallDripleafTopTile, SmallDripleafStemTopTile, SmallDripleafSideTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Small Dripleaf", SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, SmallDripleafStemBottomTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Small Dripleaf", SmallDripleafTopTile, SmallDripleafStemTopTile, SmallDripleafSideTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Azalea", AzaleaTopTile, AzaleaPlantTile, AzaleaSideTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Flowering Azalea", FloweringAzaleaTopTile, AzaleaPlantTile, FloweringAzaleaSideTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Flowering Azalea Leaves", FloweringAzaleaLeavesTile, FloweringAzaleaLeavesTile, FloweringAzaleaLeavesTile, true, false, RenderType::Cube, 0, 0.2f },
        { "Glow Lichen", GlowLichenTile, GlowLichenTile, GlowLichenTile, false, false, RenderType::Lush, 7, 0.2f },
        { "Glow Lichen", GlowLichenTile, GlowLichenTile, GlowLichenTile, false, false, RenderType::Lush, 7, 0.2f },
        { "Glow Lichen", GlowLichenTile, GlowLichenTile, GlowLichenTile, false, false, RenderType::Lush, 7, 0.2f },
        { "Glow Lichen", GlowLichenTile, GlowLichenTile, GlowLichenTile, false, false, RenderType::Lush, 7, 0.2f },
        { "Glow Lichen", GlowLichenTile, GlowLichenTile, GlowLichenTile, false, false, RenderType::Lush, 7, 0.2f },
        { "Glow Lichen", GlowLichenTile, GlowLichenTile, GlowLichenTile, false, false, RenderType::Lush, 7, 0.2f },
        { "Moss Carpet", MossTile, MossTile, MossTile, false, false, RenderType::Lush, 0, 0.1f },
        { "Potted Azalea", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Potted Flowering Azalea", FlowerPotTile, FlowerPotTile, FlowerPotTile, true, false, RenderType::Pot, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemYoungTile, PumpkinStemYoungTile, PumpkinStemYoungTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemYoungTile, PumpkinStemYoungTile, PumpkinStemYoungTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemYoungTile, PumpkinStemYoungTile, PumpkinStemYoungTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemYoungTile, PumpkinStemYoungTile, PumpkinStemYoungTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemYoungTile, PumpkinStemYoungTile, PumpkinStemYoungTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemYoungTile, PumpkinStemYoungTile, PumpkinStemYoungTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemRipeTile, PumpkinStemRipeTile, PumpkinStemRipeTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Pumpkin Stem", PumpkinStemRipeTile, PumpkinStemRipeTile, PumpkinStemRipeTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Attached Melon Stem", AttachedMelonStemTile, AttachedMelonStemTile, AttachedMelonStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Attached Melon Stem", AttachedMelonStemTile, AttachedMelonStemTile, AttachedMelonStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Attached Melon Stem", AttachedMelonStemTile, AttachedMelonStemTile, AttachedMelonStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Attached Melon Stem", AttachedMelonStemTile, AttachedMelonStemTile, AttachedMelonStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Attached Pumpkin Stem", AttachedPumpkinStemTile, AttachedPumpkinStemTile, AttachedPumpkinStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Attached Pumpkin Stem", AttachedPumpkinStemTile, AttachedPumpkinStemTile, AttachedPumpkinStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Attached Pumpkin Stem", AttachedPumpkinStemTile, AttachedPumpkinStemTile, AttachedPumpkinStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Attached Pumpkin Stem", AttachedPumpkinStemTile, AttachedPumpkinStemTile, AttachedPumpkinStemTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Carved Pumpkin", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 0, 1.0f, CarvedPumpkinTile },
        { "Carved Pumpkin", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 0, 1.0f, CarvedPumpkinTile },
        { "Carved Pumpkin", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 0, 1.0f, CarvedPumpkinTile },
        { "Carved Pumpkin", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 0, 1.0f, CarvedPumpkinTile },
        { "Jack o'Lantern", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 15, 1.0f, JackOLanternTile },
        { "Jack o'Lantern", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 15, 1.0f, JackOLanternTile },
        { "Jack o'Lantern", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 15, 1.0f, JackOLanternTile },
        { "Jack o'Lantern", PumpkinTop, PumpkinTop, PumpkinSide, true, true, RenderType::Cube, 15, 1.0f, JackOLanternTile },
        { "Hay Bale", HayTopTile, HayTopTile, HaySideTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Bone Block", BoneTopTile, BoneTopTile, BoneSideTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Dried Kelp Block", DriedKelpTopTile, DriedKelpBottomTile, DriedKelpSideTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Sweet Berry Bush", SweetBerryBushTile + 0, SweetBerryBushTile + 0, SweetBerryBushTile + 0, false, false, RenderType::Cross, 0, 0.0f },
        { "Sweet Berry Bush", SweetBerryBushTile + 1, SweetBerryBushTile + 1, SweetBerryBushTile + 1, false, false, RenderType::Cross, 0, 0.0f },
        { "Sweet Berry Bush", SweetBerryBushTile + 2, SweetBerryBushTile + 2, SweetBerryBushTile + 2, false, false, RenderType::Cross, 0, 0.0f },
        { "Sweet Berry Bush", SweetBerryBushTile + 3, SweetBerryBushTile + 3, SweetBerryBushTile + 3, false, false, RenderType::Cross, 0, 0.0f },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontHoneyTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontHoneyTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontHoneyTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontTile },
        { "Bee Nest", BeeNestTopTile, BeeNestBottomTile, BeeNestSideTile, true, true, RenderType::Cube, 0, 0.3f, BeeNestFrontHoneyTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontHoneyTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontHoneyTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontHoneyTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontTile },
        { "Beehive", BeehiveEndTile, BeehiveEndTile, BeehiveSideTile, true, true, RenderType::Cube, 0, 0.6f, BeehiveFrontHoneyTile },
        { "Honey Block", HoneyTopTile, HoneyBottomTile, HoneySideTile, true, false, RenderType::Cube, 0, 0.0f },
        { "Honeycomb Block", HoneycombBlockTile, HoneycombBlockTile, HoneycombBlockTile, true, true, RenderType::Cube, 0, 0.6f },
        { "Tall Seagrass", TallSeagrassBottomTile, TallSeagrassBottomTile, TallSeagrassBottomTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Tall Seagrass", TallSeagrassTopTile, TallSeagrassTopTile, TallSeagrassTopTile, false, false, RenderType::Cross, 0, 0.0f },
        { "Tube Coral Fan", CoralFanTileFirst + 0, CoralFanTileFirst + 0, CoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Brain Coral Fan", CoralFanTileFirst + 1, CoralFanTileFirst + 1, CoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Bubble Coral Fan", CoralFanTileFirst + 2, CoralFanTileFirst + 2, CoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Fire Coral Fan", CoralFanTileFirst + 3, CoralFanTileFirst + 3, CoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Horn Coral Fan", CoralFanTileFirst + 4, CoralFanTileFirst + 4, CoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Tube Coral Wall Fan", CoralFanTileFirst + 0, CoralFanTileFirst + 0, CoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Tube Coral Wall Fan", CoralFanTileFirst + 0, CoralFanTileFirst + 0, CoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Tube Coral Wall Fan", CoralFanTileFirst + 0, CoralFanTileFirst + 0, CoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Tube Coral Wall Fan", CoralFanTileFirst + 0, CoralFanTileFirst + 0, CoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Brain Coral Wall Fan", CoralFanTileFirst + 1, CoralFanTileFirst + 1, CoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Brain Coral Wall Fan", CoralFanTileFirst + 1, CoralFanTileFirst + 1, CoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Brain Coral Wall Fan", CoralFanTileFirst + 1, CoralFanTileFirst + 1, CoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Brain Coral Wall Fan", CoralFanTileFirst + 1, CoralFanTileFirst + 1, CoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Bubble Coral Wall Fan", CoralFanTileFirst + 2, CoralFanTileFirst + 2, CoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Bubble Coral Wall Fan", CoralFanTileFirst + 2, CoralFanTileFirst + 2, CoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Bubble Coral Wall Fan", CoralFanTileFirst + 2, CoralFanTileFirst + 2, CoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Bubble Coral Wall Fan", CoralFanTileFirst + 2, CoralFanTileFirst + 2, CoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Fire Coral Wall Fan", CoralFanTileFirst + 3, CoralFanTileFirst + 3, CoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Fire Coral Wall Fan", CoralFanTileFirst + 3, CoralFanTileFirst + 3, CoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Fire Coral Wall Fan", CoralFanTileFirst + 3, CoralFanTileFirst + 3, CoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Fire Coral Wall Fan", CoralFanTileFirst + 3, CoralFanTileFirst + 3, CoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Horn Coral Wall Fan", CoralFanTileFirst + 4, CoralFanTileFirst + 4, CoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Horn Coral Wall Fan", CoralFanTileFirst + 4, CoralFanTileFirst + 4, CoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Horn Coral Wall Fan", CoralFanTileFirst + 4, CoralFanTileFirst + 4, CoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Horn Coral Wall Fan", CoralFanTileFirst + 4, CoralFanTileFirst + 4, CoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Tube Coral Block", DeadCoralBlockTileFirst + 0, DeadCoralBlockTileFirst + 0, DeadCoralBlockTileFirst + 0, true, true, RenderType::Cube, 0, 1.5f },
        { "Dead Brain Coral Block", DeadCoralBlockTileFirst + 1, DeadCoralBlockTileFirst + 1, DeadCoralBlockTileFirst + 1, true, true, RenderType::Cube, 0, 1.5f },
        { "Dead Bubble Coral Block", DeadCoralBlockTileFirst + 2, DeadCoralBlockTileFirst + 2, DeadCoralBlockTileFirst + 2, true, true, RenderType::Cube, 0, 1.5f },
        { "Dead Fire Coral Block", DeadCoralBlockTileFirst + 3, DeadCoralBlockTileFirst + 3, DeadCoralBlockTileFirst + 3, true, true, RenderType::Cube, 0, 1.5f },
        { "Dead Horn Coral Block", DeadCoralBlockTileFirst + 4, DeadCoralBlockTileFirst + 4, DeadCoralBlockTileFirst + 4, true, true, RenderType::Cube, 0, 1.5f },
        { "Dead Tube Coral", DeadCoralTileFirst + 0, DeadCoralTileFirst + 0, DeadCoralTileFirst + 0, false, false, RenderType::Cross, 0, 0.0f },
        { "Dead Brain Coral", DeadCoralTileFirst + 1, DeadCoralTileFirst + 1, DeadCoralTileFirst + 1, false, false, RenderType::Cross, 0, 0.0f },
        { "Dead Bubble Coral", DeadCoralTileFirst + 2, DeadCoralTileFirst + 2, DeadCoralTileFirst + 2, false, false, RenderType::Cross, 0, 0.0f },
        { "Dead Fire Coral", DeadCoralTileFirst + 3, DeadCoralTileFirst + 3, DeadCoralTileFirst + 3, false, false, RenderType::Cross, 0, 0.0f },
        { "Dead Horn Coral", DeadCoralTileFirst + 4, DeadCoralTileFirst + 4, DeadCoralTileFirst + 4, false, false, RenderType::Cross, 0, 0.0f },
        { "Dead Tube Coral Fan", DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Brain Coral Fan", DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Bubble Coral Fan", DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Fire Coral Fan", DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Horn Coral Fan", DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Tube Coral Wall Fan", DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Tube Coral Wall Fan", DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Tube Coral Wall Fan", DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Tube Coral Wall Fan", DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, DeadCoralFanTileFirst + 0, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Brain Coral Wall Fan", DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Brain Coral Wall Fan", DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Brain Coral Wall Fan", DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Brain Coral Wall Fan", DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, DeadCoralFanTileFirst + 1, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Bubble Coral Wall Fan", DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Bubble Coral Wall Fan", DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Bubble Coral Wall Fan", DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Bubble Coral Wall Fan", DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, DeadCoralFanTileFirst + 2, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Fire Coral Wall Fan", DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Fire Coral Wall Fan", DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Fire Coral Wall Fan", DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Fire Coral Wall Fan", DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, DeadCoralFanTileFirst + 3, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Horn Coral Wall Fan", DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Horn Coral Wall Fan", DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Horn Coral Wall Fan", DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Dead Horn Coral Wall Fan", DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, DeadCoralFanTileFirst + 4, false, false, RenderType::Lush, 0, 0.0f },
        { "Conduit", ConduitTile, ConduitTile, ConduitTile, true, false, RenderType::Lush, 15, 3.0f },
        { "Turtle Egg", TurtleEggTile, TurtleEggTile, TurtleEggTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggTile, TurtleEggTile, TurtleEggTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggTile, TurtleEggTile, TurtleEggTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggTile, TurtleEggTile, TurtleEggTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, TurtleEggSlightlyCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Turtle Egg", TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, TurtleEggVeryCrackedTile, true, false, RenderType::Lush, 0, 0.5f },
        { "Packed Ice", PackedIceTile, PackedIceTile, PackedIceTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Blue Ice", BlueIceTile, BlueIceTile, BlueIceTile, true, true, RenderType::Cube, 0, 2.8f },
        { "Frosted Ice", FrostedIceTileFirst, FrostedIceTileFirst, FrostedIceTileFirst, true, false, RenderType::Cube, 0, 0.5f },
        { "Frosted Ice", FrostedIceTileFirst + 1, FrostedIceTileFirst + 1, FrostedIceTileFirst + 1, true, false, RenderType::Cube, 0, 0.5f },
        { "Frosted Ice", FrostedIceTileFirst + 2, FrostedIceTileFirst + 2, FrostedIceTileFirst + 2, true, false, RenderType::Cube, 0, 0.5f },
        { "Frosted Ice", FrostedIceTileFirst + 3, FrostedIceTileFirst + 3, FrostedIceTileFirst + 3, true, false, RenderType::Cube, 0, 0.5f },
        { "Powder Snow", PowderSnowTile, PowderSnowTile, PowderSnowTile, false, true, RenderType::Cube, 0, 0.25f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Repeater", RepeaterOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Redstone Comparator", ComparatorOnTile, SmoothStoneTile, SmoothStoneTile, true, false, RenderType::Lush, 0, 0.0f },
        { "Daylight Detector", DaylightTopTile, DaylightSideTile, DaylightSideTile, true, false, RenderType::Lush, 0, 0.2f },
        { "Daylight Detector", DaylightTopTile, DaylightSideTile, DaylightSideTile, true, false, RenderType::Lush, 0, 0.2f },
        { "Daylight Detector", DaylightInvertedTopTile, DaylightSideTile, DaylightSideTile, true, false, RenderType::Lush, 0, 0.2f },
        { "Daylight Detector", DaylightInvertedTopTile, DaylightSideTile, DaylightSideTile, true, false, RenderType::Lush, 0, 0.2f },
        { "Target", TargetTopTile, TargetTopTile, TargetSideTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Target", TargetTopTile, TargetTopTile, TargetSideTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire Hook", TripwireHookTile, Planks, TripwireHookTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire", TripwireTile, TripwireTile, TripwireTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Tripwire", TripwireTile, TripwireTile, TripwireTile, false, false, RenderType::Lush, 0, 0.0f },
        { "Powered Rail", PoweredRailTile, PoweredRailTile, PoweredRailTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Powered Rail", PoweredRailOnTile, PoweredRailOnTile, PoweredRailOnTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Powered Rail", PoweredRailTile, PoweredRailTile, PoweredRailTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Powered Rail", PoweredRailOnTile, PoweredRailOnTile, PoweredRailOnTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Detector Rail", DetectorRailTile, DetectorRailTile, DetectorRailTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Detector Rail", DetectorRailOnTile, DetectorRailOnTile, DetectorRailOnTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Detector Rail", DetectorRailTile, DetectorRailTile, DetectorRailTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Detector Rail", DetectorRailOnTile, DetectorRailOnTile, DetectorRailOnTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Activator Rail", ActivatorRailTile, ActivatorRailTile, ActivatorRailTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Activator Rail", ActivatorRailOnTile, ActivatorRailOnTile, ActivatorRailOnTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Activator Rail", ActivatorRailTile, ActivatorRailTile, ActivatorRailTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Activator Rail", ActivatorRailOnTile, ActivatorRailOnTile, ActivatorRailOnTile, false, false, RenderType::Rail, 0, 0.7f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorBottomTile, IronDoorBottomTile, IronDoorBottomTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Door", IronDoorTopTile, IronDoorTopTile, IronDoorTopTile, true, false, RenderType::Door, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Trapdoor", IronTrapdoorTile, IronTrapdoorTile, IronTrapdoorTile, true, false, RenderType::Trapdoor, 0, 5.0f },
        { "Iron Bars", IronBarsTile, IronBarsTile, IronBarsTile, true, false, RenderType::Pane, 0, 5.0f },
        { "Chain", ChainTile, ChainTile, ChainTile, true, false, RenderType::Lush, 0, 5.0f },
        { "Chain", ChainTile, ChainTile, ChainTile, true, false, RenderType::Lush, 0, 5.0f },
        { "Chain", ChainTile, ChainTile, ChainTile, true, false, RenderType::Lush, 0, 5.0f },
        { "End Rod", EndRodTile, EndRodTile, EndRodTile, true, false, RenderType::Lush, 14, 0.0f },
        { "End Rod", EndRodTile, EndRodTile, EndRodTile, true, false, RenderType::Lush, 14, 0.0f },
        { "End Rod", EndRodTile, EndRodTile, EndRodTile, true, false, RenderType::Lush, 14, 0.0f },
        { "End Rod", EndRodTile, EndRodTile, EndRodTile, true, false, RenderType::Lush, 14, 0.0f },
        { "End Rod", EndRodTile, EndRodTile, EndRodTile, true, false, RenderType::Lush, 14, 0.0f },
        { "End Rod", EndRodTile, EndRodTile, EndRodTile, true, false, RenderType::Lush, 14, 0.0f },
        { "Cobweb", CobwebTile, CobwebTile, CobwebTile, false, false, RenderType::Cross, 0, 4.0f },
        { "Soul Torch", SoulTorchTile, SoulTorchTile, SoulTorchTile, false, false, RenderType::Cross, 10, 0.0f },
        { "Soul Campfire", SoulCampfireTop, CampfireLog, SoulCampfireLogLit, true, false, RenderType::Cube, 10, 2.0f },
        { "Soul Soil", SoulSoilTile, SoulSoilTile, SoulSoilTile, true, true, RenderType::Cube, 0, 0.5f },
        { "Fire", FireTile, FireAltTile, FireTile, false, false, RenderType::Lush, 15, 0.0f },
        { "Fire", FireTile, FireAltTile, FireTile, false, false, RenderType::Lush, 15, 0.0f },
        { "Fire", FireTile, FireAltTile, FireTile, false, false, RenderType::Lush, 15, 0.0f },
        { "Fire", FireTile, FireAltTile, FireTile, false, false, RenderType::Lush, 15, 0.0f },
        { "Soul Fire", SoulFireTile, SoulFireTile, SoulFireTile, false, false, RenderType::Lush, 10, 0.0f },
        { "Magma Block", MagmaTile, MagmaTile, MagmaTile, true, true, RenderType::Cube, 3, 0.5f },
        { "Crying Obsidian", CryingObsidianTile, CryingObsidianTile, CryingObsidianTile, true, true, RenderType::Cube, 10, 50.0f },
        { "Lodestone", LodestoneTopTile, LodestoneTopTile, LodestoneSideTile, true, true, RenderType::Cube, 0, 3.5f },
        { "Respawn Anchor", AnchorTopTileFirst + 0, AnchorBottomTile, AnchorSideTileFirst + 0, true, true, RenderType::Cube, 0, 50.0f },
        { "Respawn Anchor", AnchorTopTileFirst + 1, AnchorBottomTile, AnchorSideTileFirst + 1, true, true, RenderType::Cube, 3, 50.0f },
        { "Respawn Anchor", AnchorTopTileFirst + 2, AnchorBottomTile, AnchorSideTileFirst + 2, true, true, RenderType::Cube, 7, 50.0f },
        { "Respawn Anchor", AnchorTopTileFirst + 3, AnchorBottomTile, AnchorSideTileFirst + 3, true, true, RenderType::Cube, 11, 50.0f },
        { "Respawn Anchor", AnchorTopTileFirst + 4, AnchorBottomTile, AnchorSideTileFirst + 4, true, true, RenderType::Cube, 15, 50.0f },
        { "Bell", BellTopTile, BellBottomTile, BellSideTile, true, false, RenderType::Lush, 0, 5.0f },
        { "Bell", BellTopTile, BellBottomTile, BellSideTile, true, false, RenderType::Lush, 0, 5.0f },
        { "Bell", BellTopTile, BellBottomTile, BellSideTile, true, false, RenderType::Lush, 0, 5.0f },
        { "Bell", BellTopTile, BellBottomTile, BellSideTile, true, false, RenderType::Lush, 0, 5.0f },
        { "Jukebox", JukeboxTopTile, JukeboxSideTile, JukeboxSideTile, true, true, RenderType::Cube, 0, 2.0f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Chiseled Bookshelf", ShelfTopTile, ShelfTopTile, ShelfSideTile, true, false, RenderType::Lush, 0, 1.5f },
        { "Dragon Egg", DragonEggTile, DragonEggTile, DragonEggTile, true, false, RenderType::Lush, 1, 3.0f },
        { "Heavy Core", HeavyCoreTile, HeavyCoreTile, HeavyCoreTile, true, false, RenderType::Lush, 0, 10.0f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 0, SuspiciousSandTileFirst + 0, SuspiciousSandTileFirst + 0, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 1, SuspiciousSandTileFirst + 1, SuspiciousSandTileFirst + 1, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 2, SuspiciousSandTileFirst + 2, SuspiciousSandTileFirst + 2, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 3, SuspiciousSandTileFirst + 3, SuspiciousSandTileFirst + 3, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 0, SuspiciousSandTileFirst + 0, SuspiciousSandTileFirst + 0, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 1, SuspiciousSandTileFirst + 1, SuspiciousSandTileFirst + 1, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 2, SuspiciousSandTileFirst + 2, SuspiciousSandTileFirst + 2, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 3, SuspiciousSandTileFirst + 3, SuspiciousSandTileFirst + 3, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 0, SuspiciousSandTileFirst + 0, SuspiciousSandTileFirst + 0, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 1, SuspiciousSandTileFirst + 1, SuspiciousSandTileFirst + 1, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 2, SuspiciousSandTileFirst + 2, SuspiciousSandTileFirst + 2, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Sand", SuspiciousSandTileFirst + 3, SuspiciousSandTileFirst + 3, SuspiciousSandTileFirst + 3, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 0, SuspiciousGravelTileFirst + 0, SuspiciousGravelTileFirst + 0, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 1, SuspiciousGravelTileFirst + 1, SuspiciousGravelTileFirst + 1, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 2, SuspiciousGravelTileFirst + 2, SuspiciousGravelTileFirst + 2, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 3, SuspiciousGravelTileFirst + 3, SuspiciousGravelTileFirst + 3, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 0, SuspiciousGravelTileFirst + 0, SuspiciousGravelTileFirst + 0, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 1, SuspiciousGravelTileFirst + 1, SuspiciousGravelTileFirst + 1, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 2, SuspiciousGravelTileFirst + 2, SuspiciousGravelTileFirst + 2, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 3, SuspiciousGravelTileFirst + 3, SuspiciousGravelTileFirst + 3, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 0, SuspiciousGravelTileFirst + 0, SuspiciousGravelTileFirst + 0, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 1, SuspiciousGravelTileFirst + 1, SuspiciousGravelTileFirst + 1, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 2, SuspiciousGravelTileFirst + 2, SuspiciousGravelTileFirst + 2, true, true, RenderType::Cube, 0, 0.25f },
        { "Suspicious Gravel", SuspiciousGravelTileFirst + 3, SuspiciousGravelTileFirst + 3, SuspiciousGravelTileFirst + 3, true, true, RenderType::Cube, 0, 0.25f },
    };
}

const BlockInfo& blockInfo(BlockId id)
{
    if (id >= Blocks::Count) return TABLE[Blocks::Air];
    return TABLE[id];
}

bool isObtainable(BlockId id)
{
    if (isFluid(id) && !isFluidSource(id)) return false;
    if (isAmethystGrowth(id) && id != Blocks::AmethystCluster) return false;
    if (furnaceFacing(id) != 0) return false;
    switch (id)
    {
        case Blocks::Air:
        case Blocks::Water:
        case Blocks::Lava:
        case Blocks::NetherPortal: // made by lighting a frame, never placed
        case Blocks::EndPortal:    // the same: it appears when a ring is finished
        case Blocks::FurnaceLit:   // the burning state of one, not a thing you hold
        case Blocks::SmokerLit:
        case Blocks::BlastFurnaceLit:
        case Blocks::BarrelOpen:
        case Blocks::NetherWart0:
        case Blocks::NetherWart1:
        case Blocks::CopperBulbPowered:
        case Blocks::CopperBulbLitPowered:
        case Blocks::SculkCatalystBloom:
        case Blocks::VaultEjecting:
        case Blocks::OminousVaultEjecting:
        case Blocks::TrialSpawnerActiveOminous:
        case Blocks::TrialSpawnerEjecting:
        case Blocks::TrialSpawnerEjectingOminous:
        case Blocks::TrialSpawnerSpentOminous:
        case Blocks::EndPortalFrameFilled: // a frame with an eye already in it
        case Blocks::TntPrimed:    // likewise: a lit stick is not an item
        // Only one ladder is a thing you hold; the other three are the
        // same ladder hung on a different wall.
        case Blocks::LadderEast:
        case Blocks::LadderSouth:
        case Blocks::LadderWest:
            return false;

        // The same for the rails: you hold one rail, and which of the
        // six shapes it lands as is decided by what it touches.
        case Blocks::RailEastWest:
        case Blocks::RailNorthEast:
        case Blocks::RailSouthEast:
        case Blocks::RailSouthWest:
        case Blocks::RailNorthWest:
            return false;

        default:
            // A crop is planted rather than placed, and only the ripe
            // one would mean anything in a hand anyway.
            if (isCrop(id)) return false;

            // A composter and a cauldron keep their contents in their
            // id, the way a crop keeps its growth. Only the empty one
            // is a thing you hold.
            if (isComposter(id) && id != Blocks::Composter0) return false;

            // A lectern with a book on it is the same block holding
            // something, which is not a second thing to hold.
            if (id == Blocks::LecternBook) return false;

            // A cake is placed from an item and eaten where it
            // stands: it never goes back in a hand, whole or part
            // eaten. A lit candle is the same candle alight.
            if (isCake(id)) return false;
            if (isCandle(id) && (candleCount(id) != 1 || candleLit(id))) return false;
            if (isCandleCake(id)) return false;
            if (id == Blocks::Bed) return false;
            if (isDoublePlant(id) && doublePlantUpper(id)) return false;
            if (isPitcherCrop(id) || isTorchflowerCrop(id) || isPotted(id)) return false;
            if (isCaveVine(id) || isBigDripleafStem(id) || smallDripleafUpper(id)) return false;
            if (isPumpkinStem(id) || isAttachedStem(id) || isSweetBerryBush(id)) return false;
            if (isCarvedPumpkin(id) && id != Blocks::CarvedPumpkinFirst + 2) return false;
            if (isJackOLantern(id) && id != Blocks::JackOLanternFirst + 2) return false;
            if (isHive(id) && id != hiveOf(isBeeNest(id), 2, 0)) return false;
            if (id == Blocks::TallSeagrassUpper) return false;
            if (isAnyCoralWallFan(id)) return false;
            if (isTurtleEgg(id) && id != turtleEggOf(1, 0)) return false;
            if (isFrostedIce(id) || id == Blocks::PowderSnow) return false;
            if (isRepeater(id) && id != repeaterOf(0, 1, false)) return false;
            if (isComparator(id) && id != comparatorOf(0, false, false)) return false;
            if (isDaylightDetector(id) && id != Blocks::DaylightDetector) return false;
            if (id == Blocks::TargetLit || isTripwireHook(id) || isTripwire(id)) return false;
            if (isSpecialRail(id) && id != specialRailOf(specialRailFamily(id), false, false)) return false;
            if (isIronDoor(id) && id != Blocks::IronDoorFirst) return false;
            if (isIronTrapdoor(id) && id != Blocks::IronTrapdoorFirst) return false;
            if (isChain(id) && id != Blocks::ChainFirst) return false;
            if (isEndRod(id) && id != Blocks::EndRodFirst) return false;
            if (id == Blocks::ChorusPlant) return false;
            if (isFire(id)) return false;
            if (isRespawnAnchor(id) && id != Blocks::RespawnAnchor0) return false;
            if (isBell(id) && id != Blocks::BellFirst + 2) return false;
            if (isChiseledBookshelf(id) && id != shelfOf(2, 0)) return false;
            if (isSuspicious(id)) return false;
            if (isBigDripleaf(id) && id != bigDripleafOf(0, DripleafTilt::None)) return false;
            if (isSmallDripleaf(id) && id != smallDripleafOf(0, false)) return false;
            if (isGlowLichen(id) && id != glowLichenOn(0)) return false;
            if (isCopperBulb(id) && copperForm(id).state != 0) return false;
            if (isLightningRod(id) && id != lightningRodOf(0, false)) return false;
            if (isChestFamily(id) && id != Blocks::Chest && id != chestOf(chestState(id).kind, 2, 0)) return false;
            if (isColourBed(id) && id != bedOf(bedColour(id), 0, false)) return false;

            // Cocoa is a pod on a trunk rather than a block anybody
            // places, and only the beans come off it.
            if (isCocoa(id)) return false;

            // A sign is held as one item and hung four ways, the same
            // as a ladder: the block is where it ended up, not the
            // thing in your hand.
            if (isSign(id)) return false;
            if (isFrame(id)) return false;
            if (isCauldron(id) && id != Blocks::Cauldron0) return false;

            // Likewise for stairs: one of each four is the thing you
            // hold, the rest are it turned round.
            if (isStairs(id) && blockFacing(id) != 0) return false;

            // A door is held as one item and placed as two blocks, so
            // only the shut lower north half is a thing in your hand.
            if (isDoor(id) && id != doorBase(id)) return false;
            if (isTrapdoor(id) && id != trapdoorBase(id)) return false;
            if (isPinkPetals(id) && id != Blocks::PinkPetals1) return false;
            if (id == Blocks::MangrovePropaguleHanging) return false;
            if (id == Blocks::WeepingVinesPlant || id == Blocks::TwistingVinesPlant) return false;

            return id < Blocks::Count;
    }
}

const char* blockSoundGroup(BlockId id)
{
    if (isFluid(id)) return nullptr;
    // By family, so a wood added later sounds like wood rather than
    // like whatever the default happens to be.
    if (isLeaves(id)) return "grass";
    if (isLog(id) || isPlanks(id)) return "wood";
    if (isWool(id)) return "cloth";
    if (isComposter(id) || isLectern(id)) return "wood";
    if (id == Blocks::Bamboo || id == Blocks::Scaffolding) return "wood";
    if (isSign(id) || isFrame(id)) return "wood";
    if (isCopper(id) || isLightningRod(id)) return "stone";
    if (isWildPlant(id)) return "grass";
    if (isMushroomBlock(id)) return "wood";
    if (isPumpkinStem(id) || isAttachedStem(id) || isSweetBerryBush(id) || id == Blocks::HayBale || id == Blocks::DriedKelpBlock) return "grass";
    if (isCarvedPumpkin(id) || isJackOLantern(id) || isHive(id)) return "wood";
    if (id == Blocks::HoneyBlock || id == Blocks::HoneycombBlock) return "cloth";
    if (isTallSeagrass(id)) return "grass";
    if (isLivingCoral(id) || isDeadCoralBlock(id) || isDeadCoralPlant(id) || isAnyCoralFan(id) || isAnyCoralWallFan(id)) return "stone";
    if (id == Blocks::Conduit || isTurtleEgg(id)) return "stone";
    if (id == Blocks::PackedIce || id == Blocks::BlueIce || isFrostedIce(id)) return "stone";
    if (id == Blocks::PowderSnow) return "snow";
    if (isDiode(id) || isIronDoor(id) || isIronTrapdoor(id) || isSpecialRail(id)) return "stone";
    if (isDaylightDetector(id) || isTripwireHook(id)) return "wood";
    if (isTripwire(id)) return "cloth";
    if (id == Blocks::IronBars || isChain(id) || isEndRod(id)) return "stone";
    if (id == Blocks::Cobweb) return "cloth";
    if (id == Blocks::SoulSoil) return "sand";
    if (id == Blocks::SoulCampfire) return "wood";
    if (isChiseledBookshelf(id) || id == Blocks::Jukebox) return "wood";
    if (isBell(id) || id == Blocks::HeavyCore || id == Blocks::DragonEgg) return "stone";
    if (isSuspiciousSand(id)) return "sand";
    if (isSuspiciousGravel(id)) return "gravel";
    if (id == Blocks::Target || id == Blocks::TargetLit) return "grass";
    if (isEarthy(id)) return "gravel";
    if (isFlowerPot(id)) return "stone";
    if (isDoor(id) || isTrapdoor(id)) return "wood";
    if (isConcretePowder(id)) return "sand";
    if (isColourBed(id)) return "cloth";
    if (isEnderChest(id)) return "stone";
    if (isCopper(id) || isLightningRod(id)) return "stone";
    if (isStorageChest(id)) return "wood";
    if (isSapling(id) || isPinkPetals(id) || id == Blocks::MangrovePropaguleHanging) return "grass";
    if (isNetherPlant(id) || id == Blocks::NetherWartBlock || id == Blocks::WarpedWartBlock ||
        id == Blocks::Shroomlight) return "grass";
    if (id == Blocks::MangroveRoots || id == Blocks::BambooMosaic || id == Blocks::BambooSlab) return "wood";
    if (id == Blocks::MuddyMangroveRoots || id == Blocks::PackedMud) return "gravel";
    if (id == Blocks::Melon || isCocoa(id)) return "grass";
    if (isCake(id) || isCandle(id) || isCandleCake(id)) return "cloth";

    switch (id)
    {
        case Blocks::Grass:
        case Blocks::SnowGrass:
        case Blocks::TallGrass:
        case Blocks::FlowerRed:
        case Blocks::FlowerYellow:
        case Blocks::Cactus:
        case Blocks::Pumpkin:
        case Blocks::SugarCane:
        case Blocks::NetherWart:
        case Blocks::DirtPath:
            return "grass";

        // Leaves rustle whichever tree they came off.

        case Blocks::Farmland:
            return "gravel";

        case Blocks::Dirt:
        case Blocks::Clay:
            return "gravel";

        case Blocks::Gravel:
            return "gravel";

        case Blocks::Sand:
        case Blocks::Sandstone:
        case Blocks::SoulSand:
            return "sand";

        case Blocks::Snow:
            return "snow";

        case Blocks::Planks:
        case Blocks::Log:
        case Blocks::BirchLog:
        case Blocks::CraftingTable:
        case Blocks::Chest:
        case Blocks::PlankSlab:
        case Blocks::OakFence:
        case Blocks::DoorLowShutNorth:
        case Blocks::PlankStairsNorth:
        case Blocks::PlankStairsEast:
        case Blocks::PlankStairsSouth:
        case Blocks::PlankStairsWest:
        case Blocks::LadderNorth:
        case Blocks::LadderEast:
        case Blocks::LadderSouth:
        case Blocks::LadderWest:
        case Blocks::Bookshelf:
        case Blocks::Torch:
        case Blocks::RedstoneTorch:
        case Blocks::RedstoneTorchOff:
        case Blocks::LeverOff:
        case Blocks::LeverOn:
            return "wood";

        case Blocks::Wool:
        case Blocks::Bed:
        case Blocks::Tnt:
        case Blocks::TntPrimed:
            return "cloth";

        case Blocks::Air:
        case Blocks::Water:
        case Blocks::Lava:
        case Blocks::NetherPortal:
            return nullptr;

        // Stone, ores, brick, obsidian, glass, ice and anything new default to
        // the stone family, which is the right guess for a mineral block.
        default:
            return "stone";
    }
}

int blockStepVariants(const char* group)
{
    // How many numbered footstep files each family actually ships.
    if (!group) return 0;
    const std::string name(group);
    if (name == "grass" || name == "stone" || name == "wood") return 6;
    if (name == "sand") return 5;
    return 4; // gravel, snow, cloth
}

// This began as a table of what a block sounds like, where falling
// through to stone was harmless -- an unlisted block merely sounded
// wrong. It now also decides which tool a block gives way to and
// whether a bare hand gets anything out of it, so the half of the block
// list that was never written down (birch, gravel, clay, snow, the
// flowers) has to be here: defaulting those to stone meant punching a
// birch tree dropped nothing at all.
Material blockMaterial(BlockId id)
{
    if (isCrop(id)) return Material::Plant;

    // Asked of the family, so a wood added later is wood and its leaves
    // are leaves without anyone having to remember this function. Named
    // one by one, the first two new trees both fell through to stone and
    // wanted a pickaxe -- which is what the block table's own test
    // caught, and the third time it has caught exactly that.
    if (isLog(id) || isPlanks(id)) return Material::Wood;
    if (isLeaves(id)) return Material::Plant;

    // What grows in the sea is a plant, like what grows on land. Asked
    // of the family for the same reason the woods are: left to fall
    // through, every blade of seagrass came out as stone and wanted a
    // pickaxe. The block table's own test caught it, again.
    if (isWaterPlant(id)) return Material::Plant;
    if (id == Blocks::LilyPad || id == Blocks::Vine) return Material::Plant;

    // A crystal is struck rather than mined -- in Minecraft it is an
    // entity you shoot. Left to fall through it came out as stone and
    // wanted a pickaxe to break something with no hardness at all,
    // which the block table's own test caught. Again.
    if (id == Blocks::EndCrystal) return Material::Plant;

    // A cluster is knocked off rather than mined, like a plant.
    if (isAmethystGrowth(id)) return Material::Plant;

    // The workstations, whichever state they are in. A composter and a
    // lectern are boxes of planks; the rest are stone and iron.
    if (isWool(id)) return Material::Wool;
    if (isComposter(id) || isLectern(id)) return Material::Wood;
    if (id == Blocks::Melon) return Material::Plant;
    if (id == Blocks::Bamboo) return Material::Wood;
    if (isCocoa(id)) return Material::Plant;
    if (isCake(id)) return Material::Wool;      // soft, and no tool helps
    if (isCandle(id) || isCandleCake(id)) return Material::Wool;
    if (id == Blocks::Scaffolding) return Material::Wood;
    // A slime block comes apart in your hands; a pickaxe is no help
    // and asking for one would have made the softest block in the game
    // want the hardest tool. The block table's test caught that too.
    if (id == Blocks::SlimeBlock) return Material::Wool;
    if (isSign(id) || isFrame(id)) return Material::Wood;
    if (isCopper(id) || isLightningRod(id)) return Material::Stone;
    if (isWildPlant(id)) return Material::Plant;
    if (isMushroomBlock(id)) return Material::Wood;
    if (isPumpkinStem(id) || isAttachedStem(id) || isSweetBerryBush(id)) return Material::Plant;
    if (id == Blocks::HayBale || id == Blocks::DriedKelpBlock) return Material::Plant;
    if (isCarvedPumpkin(id) || isJackOLantern(id) || isHive(id)) return Material::Wood;
    if (id == Blocks::Conduit) return Material::Glass;
    if (id == Blocks::PackedIce || id == Blocks::BlueIce || isFrostedIce(id)) return Material::Glass;
    if (id == Blocks::PowderSnow) return Material::Snow;
    if (isDiode(id) || isTripwireHook(id) || isTripwire(id) || isSpecialRail(id) || isTarget(id)) return Material::Plant;
    if (isDaylightDetector(id)) return Material::Wood;
    if (id == Blocks::IronBars || isChain(id)) return Material::Stone;
    if (isEndRod(id)) return Material::Plant;
    if (isChiseledBookshelf(id) || id == Blocks::Jukebox) return Material::Wood;
    if (id == Blocks::DragonEgg) return Material::Plant;
    if (isSuspiciousSand(id)) return Material::Sand;
    if (isSuspiciousGravel(id)) return Material::Dirt;
    if (id == Blocks::Cobweb) return Material::Wool;
    if (id == Blocks::SoulTorch || isFire(id)) return Material::Plant;
    if (id == Blocks::SoulCampfire) return Material::Wood;
    if (id == Blocks::SoulSoil) return Material::Sand;
    if (isIronDoor(id) || isIronTrapdoor(id)) return Material::Stone;
    if (isTurtleEgg(id)) return Material::Plant;
    if (id == Blocks::HoneyBlock || id == Blocks::HoneycombBlock) return Material::Wool;
    if (isEarthy(id)) return Material::Dirt;
    if (isFlowerPot(id)) return Material::Plant;
    if (isDoor(id) || isTrapdoor(id)) return Material::Wood;
    if (isConcretePowder(id)) return Material::Sand;
    if (isColourBed(id)) return Material::Wool;
    if (isEnderChest(id)) return Material::Stone;
    if (isCopper(id) || isLightningRod(id)) return Material::Stone;
    if (isStorageChest(id)) return Material::Wood;
    if (isGlassLike(id) || isPane(id)) return Material::Glass;
    if (isSapling(id) || isPinkPetals(id) || id == Blocks::MangrovePropaguleHanging) return Material::Plant;
    if (isNetherPlant(id) || id == Blocks::NetherWartBlock || id == Blocks::WarpedWartBlock ||
        id == Blocks::Shroomlight) return Material::Plant;
    if (id == Blocks::MangroveRoots || id == Blocks::BambooMosaic || id == Blocks::BambooSlab) return Material::Wood;
    if (id == Blocks::MuddyMangroveRoots || id == Blocks::PackedMud) return Material::Dirt;
    if (isCauldron(id)) return Material::Stone;
    if (isFurnaceBlock(id)) return Material::Stone;

    switch (id)
    {
        case Blocks::Dirt:
        case Blocks::Farmland:
        case Blocks::DirtPath:
        case Blocks::Clay:
        case Blocks::SoulSand:
            return Material::Dirt;

        case Blocks::Grass:
        case Blocks::SnowGrass:
            return Material::Grass;

        case Blocks::Sand:
        case Blocks::Gravel:
            return Material::Sand;

        case Blocks::Glass:
        case Blocks::Ice:
            return Material::Glass;

        // Iron on wooden sleepers. Wool here means no tool is needed,
        // which is the rule that matters: a rail you could not pick up
        // by hand would be a rail you laid once and never moved.
        case Blocks::RailNorthSouth:
        case Blocks::RailEastWest:
        case Blocks::RailNorthEast:
        case Blocks::RailSouthEast:
        case Blocks::RailSouthWest:
        case Blocks::RailNorthWest:
        case Blocks::Wool:
        case Blocks::Bed:
        // TNT is paper and powder. Letting it fall through to stone made
        // it want a pickaxe, which the all-blocks sweep caught the
        // moment it was added.
        case Blocks::Tnt:
        case Blocks::TntPrimed:
            return Material::Wool;

        case Blocks::Snow:
            return Material::Snow;

        case Blocks::Log:
        case Blocks::BirchLog:
        case Blocks::Planks:
        case Blocks::CraftingTable:
        case Blocks::Chest:
        case Blocks::PlankSlab:
        case Blocks::OakFence:
        case Blocks::DoorLowShutNorth:
        case Blocks::PlankStairsNorth:
        case Blocks::PlankStairsEast:
        case Blocks::PlankStairsSouth:
        case Blocks::PlankStairsWest:
        case Blocks::Bookshelf:
            return Material::Wood;

        case Blocks::LadderNorth:
        case Blocks::LadderEast:
        case Blocks::LadderSouth:
        case Blocks::LadderWest:
        case Blocks::TallGrass:
        case Blocks::FlowerRed:
        case Blocks::FlowerYellow:
        case Blocks::Cactus:
        case Blocks::Pumpkin:
        case Blocks::SugarCane:
        case Blocks::Torch:
        case Blocks::NetherWart:
        case Blocks::BrewingStand:
        case Blocks::Moss:
        case Blocks::GlowBerries:
        case Blocks::DripstoneDown:
        case Blocks::DripstoneUp:
        // Dust and the switches come up with a hand: a wire painted on
        // the floor is not something you take a pickaxe to.
        case Blocks::RedstoneDust:
        case Blocks::RedstoneDustLit:
        case Blocks::RedstoneTorch:
        case Blocks::RedstoneTorchOff:
        case Blocks::LeverOff:
        case Blocks::LeverOn:
        case Blocks::ButtonOff:
        case Blocks::ButtonOn:
            return Material::Plant;

        // Stone, the ores, the bricks, obsidian, the nether and end
        // stones, the furnace: everything a pickaxe is for.
        default:
            return Material::Stone;
    }
}

bool blockOverlaps(BlockId id, int bx, int by, int bz,
                   float minX, float minY, float minZ,
                   float maxX, float maxY, float maxZ, int links)
{
    if (!isSolid(id)) return false;

    const float cx = static_cast<float>(bx);
    const float cy = static_cast<float>(by);
    const float cz = static_cast<float>(bz);

    auto overlaps = [&](float loX, float loY, float loZ, float hiX, float hiY, float hiZ) {
        return minX < cx + hiX && maxX > cx + loX &&
               minY < cy + hiY && maxY > cy + loY &&
               minZ < cz + hiZ && maxZ > cz + loZ;
    };

    if (isDoor(id))
    {
        // The same thin slab the mesher draws, so what you see is what
        // you walk into.
        constexpr float THICK = 0.1875f;
        const int resting = doorIsOpen(id) ? ((blockFacing(id) + 1) & 3) : blockFacing(id);

        switch (resting)
        {
            case 0: return overlaps(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, THICK);
            case 1: return overlaps(1.0f - THICK, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            case 2: return overlaps(0.0f, 0.0f, 1.0f - THICK, 1.0f, 1.0f, 1.0f);
            default: return overlaps(0.0f, 0.0f, 0.0f, THICK, 1.0f, 1.0f);
        }
    }

    if (isTrapdoor(id))
    {
        constexpr float THICK = 0.1875f;
        if (!trapdoorIsOpen(id))
            return trapdoorIsTop(id) ? overlaps(0.0f, 1.0f - THICK, 0.0f, 1.0f, 1.0f, 1.0f)
                                     : overlaps(0.0f, 0.0f, 0.0f, 1.0f, THICK, 1.0f);
        switch ((trapdoorFacing(id) + 2) & 3)
        {
            case 0: return overlaps(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, THICK);
            case 1: return overlaps(1.0f - THICK, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            case 2: return overlaps(0.0f, 0.0f, 1.0f - THICK, 1.0f, 1.0f, 1.0f);
            default: return overlaps(0.0f, 0.0f, 0.0f, THICK, 1.0f, 1.0f);
        }
    }

    if (isColourBed(id)) return overlaps(0.0f, 0.0f, 0.0f, 1.0f, 0.5625f, 1.0f);
    if (isFlowerPot(id)) return overlaps(0.3125f, 0.0f, 0.3125f, 0.6875f, 0.375f, 0.6875f);
    if (id == Blocks::HoneyBlock) return overlaps(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.9375f, 0.9375f);
    if (id == Blocks::Conduit) return overlaps(0.3125f, 0.3125f, 0.3125f, 0.6875f, 0.6875f, 0.6875f);
    if (id == Blocks::HeavyCore) return overlaps(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
    if (id == Blocks::DragonEgg) return overlaps(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
    if (isDiode(id)) return overlaps(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
    if (isDaylightDetector(id)) return overlaps(0.0f, 0.0f, 0.0f, 1.0f, 0.375f, 1.0f);
    if (isChain(id))
    {
        constexpr float LO = 0.40625f, HI = 0.59375f;
        if (chainAxis(id) == 0) return overlaps(LO, 0.0f, LO, HI, 1.0f, HI);
        if (chainAxis(id) == 1) return overlaps(0.0f, LO, LO, 1.0f, HI, HI);
        return overlaps(LO, LO, 0.0f, HI, HI, 1.0f);
    }
    if (isEndRod(id))
    {
        constexpr float LO = 0.375f, HI = 0.625f;
        switch (endRodFacing(id))
        {
            case 0: case 1: return overlaps(LO, 0.0f, LO, HI, 1.0f, HI);
            case 2: case 4: return overlaps(LO, LO, 0.0f, HI, HI, 1.0f);
            default: return overlaps(0.0f, LO, LO, 1.0f, HI, HI);
        }
    }
    if (isTurtleEgg(id))
        return turtleEggCount(id) == 1 ? overlaps(0.1875f, 0.0f, 0.1875f, 0.75f, 0.4375f, 0.75f)
                                       : overlaps(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.4375f, 0.9375f);
    if (isBigDripleaf(id))
        return dripleafTilt(id) != DripleafTilt::Full && overlaps(0.0f, 0.6875f, 0.0f, 1.0f, 0.9375f, 1.0f);
    if (isAzaleaBush(id)) return overlaps(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f) || overlaps(0.375f, 0.0f, 0.375f, 0.625f, 0.5f, 0.625f);
    if (isChestFamily(id)) return overlaps(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.875f, 0.9375f);
    if (isLightningRod(id))
    {
        constexpr float LO = 0.375f, HI = 0.625f;
        switch (lightningRodFacing(id))
        {
            case 0: case 1: return overlaps(LO, 0.0f, LO, HI, 1.0f, HI);
            case 2: case 4: return overlaps(LO, LO, 0.0f, HI, HI, 1.0f);
            default: return overlaps(0.0f, LO, LO, 1.0f, HI, HI);
        }
    }

    if (isCake(id) || isCandleCake(id))
    {
        const float bite = 0.125f * static_cast<float>(cakeEaten(id));
        return overlaps(0.0625f + bite, 0.0f, 0.0625f, 0.9375f, 0.5f, 0.9375f);
    }

    if (isPane(id))
    {
        constexpr float LO = 0.4375f, HI = 0.5625f;
        if (overlaps(LO, 0.0f, LO, HI, 1.0f, HI)) return true;
        if ((links & 1) && overlaps(LO, 0.0f, 0.0f, HI, 1.0f, LO)) return true;
        if ((links & 2) && overlaps(LO, 0.0f, HI, HI, 1.0f, 1.0f)) return true;
        if ((links & 4) && overlaps(0.0f, 0.0f, LO, LO, 1.0f, HI)) return true;
        if ((links & 8) && overlaps(HI, 0.0f, LO, 1.0f, 1.0f, HI)) return true;
        return false;
    }

    if (isFence(id))
    {
        // The post, standing half a block proud of its own cell so it
        // cannot be stepped over. The arms are thin enough not to be
        // worth colliding against on their own.
        return overlaps(0.375f, 0.0f, 0.375f, 0.625f, 1.5f, 0.625f);
    }

    if (isStairs(id))
    {
        // The bottom half of the cell, and a step on the back half of
        // the top of it.
        if (overlaps(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f)) return true;

        float loX = 0.0f, loZ = 0.0f, hiX = 1.0f, hiZ = 1.0f;
        switch (blockFacing(id))
        {
            case 0: loZ = 0.5f; break;
            case 1: hiX = 0.5f; break;
            case 2: hiZ = 0.5f; break;
            default: loX = 0.5f; break;
        }
        return overlaps(loX, 0.5f, loZ, hiX, 1.0f, hiZ);
    }

    return overlaps(0.0f, 0.0f, 0.0f, 1.0f, blockHeight(id), 1.0f);
}

namespace
{
    bool g_christmasChests = false;
}

void setChristmasChests(bool on) { g_christmasChests = on; }
bool christmasChests() { return g_christmasChests; }

CopperForm copperForm(BlockId id)
{
    if (id >= Blocks::CopperBlock && id <= Blocks::OxidizedCopper)
        return { CopperFamily::Block, id - Blocks::CopperBlock, false, 0 };
    if (id >= Blocks::WaxedCopperFirst && id <= Blocks::WaxedCopperLast)
        return { CopperFamily::Block, id - Blocks::WaxedCopperFirst, true, 0 };
    struct Run { BlockId first; BlockId last; CopperFamily family; };
    static const Run RUNS[] = {
        { Blocks::CutCopperFirst, Blocks::CutCopperLast, CopperFamily::Cut },
        { Blocks::ChiseledCopperFirst, Blocks::ChiseledCopperLast, CopperFamily::Chiseled },
        { Blocks::CopperGrateFirst, Blocks::CopperGrateLast, CopperFamily::Grate },
        { Blocks::CutCopperSlabFirst, Blocks::CutCopperSlabLast, CopperFamily::Slab },
    };
    for (const Run& run : RUNS)
        if (id >= run.first && id <= run.last)
        {
            const int t = id - run.first;
            return { run.family, t % 4, t >= 4, 0 };
        }
    if (id == Blocks::CopperBulb) return { CopperFamily::Bulb, 0, false, 0 };
    if (id == Blocks::CopperBulbLit) return { CopperFamily::Bulb, 0, false, 1 };
    if (id == Blocks::CopperBulbPowered) return { CopperFamily::Bulb, 0, false, 2 };
    if (id == Blocks::CopperBulbLitPowered) return { CopperFamily::Bulb, 0, false, 3 };
    if (id >= Blocks::CopperBulbRunFirst && id <= Blocks::CopperBulbRunLast)
    {
        const int k = id - Blocks::CopperBulbRunFirst;
        const int t = 1 + k / 4;
        return { CopperFamily::Bulb, t % 4, t >= 4, k % 4 };
    }
    if (id >= Blocks::CopperDoorFirst && id <= Blocks::CopperDoorLast)
    {
        const int k = id - Blocks::CopperDoorFirst;
        const int t = k / 16;
        return { CopperFamily::Door, t % 4, t >= 4, k % 16 };
    }
    if (id >= Blocks::CopperTrapdoorFirst && id <= Blocks::CopperTrapdoorLast)
    {
        const int k = id - Blocks::CopperTrapdoorFirst;
        const int t = k / 16;
        return { CopperFamily::Trapdoor, t % 4, t >= 4, k % 16 };
    }
    return { CopperFamily::None, 0, false, 0 };
}

BlockId copperBlockOf(CopperFamily family, int stage, bool waxed, int state)
{
    if (stage < 0) stage = 0;
    if (stage > 3) stage = 3;
    const int t = stage + (waxed ? 4 : 0);
    switch (family)
    {
        case CopperFamily::Block:
            return waxed ? static_cast<BlockId>(Blocks::WaxedCopperFirst + stage)
                         : static_cast<BlockId>(Blocks::CopperBlock + stage);
        case CopperFamily::Cut: return static_cast<BlockId>(Blocks::CutCopperFirst + t);
        case CopperFamily::Chiseled: return static_cast<BlockId>(Blocks::ChiseledCopperFirst + t);
        case CopperFamily::Grate: return static_cast<BlockId>(Blocks::CopperGrateFirst + t);
        case CopperFamily::Slab: return static_cast<BlockId>(Blocks::CutCopperSlabFirst + t);
        case CopperFamily::Bulb:
        {
            const int st = state & 3;
            if (t == 0)
            {
                static const BlockId FIRST[4] = { Blocks::CopperBulb, Blocks::CopperBulbLit,
                                                  Blocks::CopperBulbPowered, Blocks::CopperBulbLitPowered };
                return FIRST[st];
            }
            return static_cast<BlockId>(Blocks::CopperBulbRunFirst + (t - 1) * 4 + st);
        }
        case CopperFamily::Door: return static_cast<BlockId>(Blocks::CopperDoorFirst + t * 16 + (state & 15));
        case CopperFamily::Trapdoor: return static_cast<BlockId>(Blocks::CopperTrapdoorFirst + t * 16 + (state & 15));
        default: return Blocks::Air;
    }
}

namespace
{
    const BlockId POTTABLE[32] = {
        Blocks::FlowerRed, Blocks::FlowerYellow, Blocks::Allium, Blocks::AzureBluet, Blocks::BlueOrchid,
        Blocks::Cornflower, Blocks::LilyOfTheValley, Blocks::OxeyeDaisy, Blocks::RedTulip, Blocks::OrangeTulip,
        Blocks::WhiteTulip, Blocks::PinkTulip, Blocks::WitherRose, Blocks::Torchflower, Blocks::Fern,
        Blocks::DeadBush, Blocks::Cactus, Blocks::Bamboo, Blocks::OakSapling, Blocks::SpruceSapling,
        Blocks::BirchSapling, Blocks::JungleSapling, Blocks::AcaciaSapling, Blocks::DarkOakSapling,
        Blocks::CherrySapling, Blocks::MangrovePropagule, Blocks::CrimsonFungus, Blocks::WarpedFungus,
        Blocks::CrimsonRoots, Blocks::WarpedRoots, Blocks::BrownMushroom, Blocks::RedMushroom,
    };
}

BlockId pottedPlant(BlockId pot)
{
    if (pot == Blocks::PottedAzalea) return Blocks::Azalea;
    if (pot == Blocks::PottedFloweringAzalea) return Blocks::FloweringAzalea;
    if (!isPotted(pot)) return Blocks::Air;
    return POTTABLE[pot - Blocks::PottedFirst];
}

BlockId pottedOf(BlockId plant)
{
    if (plant == Blocks::Azalea) return Blocks::PottedAzalea;
    if (plant == Blocks::FloweringAzalea) return Blocks::PottedFloweringAzalea;
    for (int i = 0; i < 32; ++i)
        if (POTTABLE[i] == plant) return static_cast<BlockId>(Blocks::PottedFirst + i);
    return Blocks::Air;
}
