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
        { "Water",        Water,         Water,          Water,          false, false, RenderType::Liquid, 0,  -1.0f },
        { "Sand",         Sand,          Sand,           Sand,           true,  true,  RenderType::Cube,   0,   0.5f },
        { "Gravel",       Gravel,        Gravel,         Gravel,         true,  true,  RenderType::Cube,   0,   0.6f },
        { "Gold Ore",     GoldOre,       GoldOre,        GoldOre,        true,  true,  RenderType::Cube,   0,   3.0f },
        { "Iron Ore",     IronOre,       IronOre,        IronOre,        true,  true,  RenderType::Cube,   0,   3.0f },
        { "Coal Ore",     CoalOre,       CoalOre,        CoalOre,        true,  true,  RenderType::Cube,   0,   2.5f },
        { "Diamond Ore",  DiamondOre,    DiamondOre,     DiamondOre,     true,  true,  RenderType::Cube,   0,   3.5f },
        { "Oak Log",      LogTop,        LogTop,         LogSide,        true,  true,  RenderType::Cube,   0,   2.0f },
        { "Oak Leaves",   Leaves,        Leaves,         Leaves,         true,  false, RenderType::Cube,   0,   0.2f },
        { "Glass",        Glass,         Glass,          Glass,          true,  false, RenderType::Cube,   0,   0.3f },
        { "Sandstone",    SandstoneTop,  SandstoneTop,   SandstoneSide,  true,  true,  RenderType::Cube,   0,   0.8f },
        { "Snow",         SnowTile,      SnowTile,       SnowTile,       true,  true,  RenderType::Cube,   0,   0.3f },
        { "Ice",          Ice,           Ice,            Ice,            true,  false, RenderType::Cube,   0,   0.5f },
        { "Cactus",       CactusTop,     CactusTop,      CactusSide,     true,  true,  RenderType::Cube,   0,   0.4f },
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
        { "Lava",         Lava,          Lava,           Lava,           false, false, RenderType::Liquid,15,  -1.0f },
        { "Snowy Grass",  SnowTile,      Dirt,           SnowGrassSide,  true,  true,  RenderType::Cube,   0,   0.6f },
        { "Netherrack",   Netherrack,    Netherrack,     Netherrack,     true,  true,  RenderType::Cube,   0,   0.4f },
        { "Soul Sand",    SoulSand,      SoulSand,       SoulSand,       true,  true,  RenderType::Cube,   0,   0.5f },
        { "End Stone",    EndStone,      EndStone,       EndStone,       true,  true,  RenderType::Cube,   0,   3.0f },
        // The portal surface is walked into rather than stood on, so it does
        // not collide, and it glows enough to light its own frame.
        { "Nether Portal",NetherPortal,  NetherPortal,   NetherPortal,   false, false, RenderType::Liquid, 11, -1.0f },

        // Tilled earth is solid and a shade darker than dirt; the crop
        // standing in it is a cross-shaped plant you can walk through.
        { "Crafting Table", CraftingTop, Planks,     CraftingSide,   true,  true,  RenderType::Cube,   0,   2.5f },
        { "Furnace",      FurnaceTop,    FurnaceTop,     FurnaceFront,   true,  true,  RenderType::Cube,   0,   3.5f },
        { "Furnace",      FurnaceTop,    FurnaceTop,     FurnaceLit,     true,  true,  RenderType::Cube,  13,   3.5f },
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
        { "Enchanting Table", EnchantTableTop, Obsidian,   EnchantTableSide, true, false, RenderType::Slab, 7,   5.0f },
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
        { "Trial Spawner",TrialSpawnerTop,TrialSpawnerTop,TrialSpawnerSide,true, true,  RenderType::Cube,   3,  -1.0f },
        { "Trial Spawner",TrialSpawnerTopLit,TrialSpawnerTop,TrialSpawnerSideLit,true,true,RenderType::Cube,8,-1.0f },
        { "Trial Spawner",TrialSpawnerTop,TrialSpawnerTop,TrialSpawnerSide,true, true,  RenderType::Cube,   0,  -1.0f },
        { "Vault",        VaultTop,      VaultTop,       VaultSide,      true,  true,  RenderType::Cube,   4,  -1.0f },
        { "Vault",        VaultTop,      VaultTop,       VaultSideOpen,  true,  true,  RenderType::Cube,   8,  -1.0f },
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
        { "Crafter",      CrafterTop,    CrafterSide,    CrafterSide,    true,  true,  RenderType::Cube,   0,   2.5f },
        { "Crafter",      CrafterTopLit, CrafterSide,    CrafterSide,    true,  true,  RenderType::Cube,   6,   2.5f },
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
        { "Ominous Vault",VaultTop,      VaultTop,       OminousVaultSide,true, true,  RenderType::Cube,   6,  -1.0f },
        { "Ominous Vault",VaultTop,      VaultTop,       OminousVaultSideOpen,true,true,RenderType::Cube,  10,  -1.0f },
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
        { "Sculk Sensor", SculkSensorTop,SculkTile,      SculkSensorSide,true,  true,  RenderType::Cube,   1,   1.5f },
        { "Sculk Sensor", SculkSensorTopLit,SculkTile,   SculkSensorSide,true,  true,  RenderType::Cube,   6,   1.5f },
        { "Sculk Shrieker",SculkShriekerTop,SculkTile,   SculkShriekerSide,true,true,  RenderType::Cube,   2,   3.0f },
        { "Sculk Catalyst",SculkCatalystTop,SculkTile,   SculkCatalystSide,true,true,  RenderType::Cube,   6,   3.0f },
        { "Soul Lantern", SoulLanternTile,SoulLanternTile,SoulLanternTile,true, true,  RenderType::Cube,  10,   0.3f },
        { "Reinforced Deepslate",ReinforcedDeepslateTop,ReinforcedDeepslateTop,ReinforcedDeepslateSide,true,true,RenderType::Cube,0,-1.0f },
        { "Purpur Block", PurpurTile,    PurpurTile,     PurpurTile,     true,  true,  RenderType::Cube,   0,   1.5f },
        { "Purpur Pillar",PurpurPillarTop,PurpurPillarTop,PurpurPillarSide,true,true,  RenderType::Cube,   0,   1.5f },
        { "End Stone Bricks",EndBricksTile,EndBricksTile,EndBricksTile,  true,  true,  RenderType::Cube,   0,   3.0f },
        { "Chorus Plant", ChorusPlantTile,ChorusPlantTile,ChorusPlantTile,true, true,  RenderType::Cube,   0,   0.4f },
        { "Chorus Flower",ChorusFlowerTile,ChorusFlowerTile,ChorusFlowerTile,true,true, RenderType::Cube,  0,   0.4f },
        { "Blackstone",   BlackstoneTile,BlackstoneTile, BlackstoneTile, true,  true,  RenderType::Cube,   0,   1.5f },
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
        { "Observer",     ObserverTop,   ObserverTop,    ObserverSide,   true,  true,  RenderType::Cube,   0,   3.0f },
        { "Observer",     ObserverTopLit,ObserverTopLit, ObserverSide,   true,  true,  RenderType::Cube,   0,   3.0f },
        { "Hopper",       HopperTop,     HopperSide,     HopperSide,     true,  false, RenderType::Cube,   0,   3.0f },
        { "Dropper",      DropperFront,  Cobblestone,   Cobblestone,    true,  true,  RenderType::Cube,   0,   3.5f },
        { "Dispenser",    DispenserFront,Cobblestone,   Cobblestone,    true,  true,  RenderType::Cube,   0,   3.5f },
    };
}

const BlockInfo& blockInfo(BlockId id)
{
    if (id >= Blocks::Count) return TABLE[Blocks::Air];
    return TABLE[id];
}

bool isObtainable(BlockId id)
{
    switch (id)
    {
        case Blocks::Air:
        case Blocks::Water:
        case Blocks::Lava:
        case Blocks::NetherPortal: // made by lighting a frame, never placed
        case Blocks::EndPortal:    // the same: it appears when a ring is finished
        case Blocks::FurnaceLit:   // the burning state of one, not a thing you hold
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
            if (isWheat(id)) return false;

            // Likewise for stairs: one of each four is the thing you
            // hold, the rest are it turned round.
            if (isStairs(id) && blockFacing(id) != 0) return false;

            // A door is held as one item and placed as two blocks, so
            // only the shut lower north half is a thing in your hand.
            if (isDoor(id) && id != Blocks::DoorLowShutNorth) return false;

            return id < Blocks::Count;
    }
}

const char* blockSoundGroup(BlockId id)
{
    // By family, so a wood added later sounds like wood rather than
    // like whatever the default happens to be.
    if (isLeaves(id)) return "grass";
    if (isLog(id) || isPlanks(id)) return "wood";

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
    if (isWheat(id)) return Material::Plant;

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
