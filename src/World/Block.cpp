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
        { "Tall Grass",   TallGrass,     TallGrass,      TallGrass,      false, false, RenderType::Cross,  0,   0.0f },
        { "Red Flower",   FlowerRed,     FlowerRed,      FlowerRed,      false, false, RenderType::Cross,  0,   0.0f },
        { "Yellow Flower",FlowerYellow,  FlowerYellow,   FlowerYellow,   false, false, RenderType::Cross,  0,   0.0f },
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
        { "Chest",        ChestTop,      ChestTop,       ChestFront,     true,  true,  RenderType::Cube,   0,   2.5f },
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
        { "Glow Berries", GlowBerriesTile,GlowBerriesTile,GlowBerriesTile,false, false, RenderType::Cross, 14,   0.0f },
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

        { "Candle",       CandleTile,    CandleTile,     CandleTile,     false, false, RenderType::Cross,  0,   0.1f },
        { "Candle",       CandleLitTile, CandleLitTile,  CandleLitTile,  false, false, RenderType::Cross, 10,   0.1f },

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
            if (id == Blocks::CandleLit) return false;

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
    if (isDoor(id) || isTrapdoor(id)) return "wood";
    if (isSapling(id) || isPinkPetals(id) || id == Blocks::MangrovePropaguleHanging) return "grass";
    if (isNetherPlant(id) || id == Blocks::NetherWartBlock || id == Blocks::WarpedWartBlock ||
        id == Blocks::Shroomlight) return "grass";
    if (id == Blocks::MangroveRoots || id == Blocks::BambooMosaic || id == Blocks::BambooSlab) return "wood";
    if (id == Blocks::MuddyMangroveRoots || id == Blocks::PackedMud) return "gravel";
    if (id == Blocks::Melon || isCocoa(id)) return "grass";
    if (isCake(id) || id == Blocks::Candle || id == Blocks::CandleLit) return "cloth";

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
    if (id == Blocks::Candle || id == Blocks::CandleLit) return Material::Wool;
    if (id == Blocks::Scaffolding) return Material::Wood;
    // A slime block comes apart in your hands; a pickaxe is no help
    // and asking for one would have made the softest block in the game
    // want the hardest tool. The block table's test caught that too.
    if (id == Blocks::SlimeBlock) return Material::Wool;
    if (isSign(id) || isFrame(id)) return Material::Wood;
    if (isDoor(id) || isTrapdoor(id)) return Material::Wood;
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
                   float maxX, float maxY, float maxZ)
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
