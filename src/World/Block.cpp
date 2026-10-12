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
    switch (id)
    {
        case Blocks::Grass:
        case Blocks::SnowGrass:
        case Blocks::Leaves:
        case Blocks::BirchLeaves:
        case Blocks::TallGrass:
        case Blocks::FlowerRed:
        case Blocks::FlowerYellow:
        case Blocks::Cactus:
        case Blocks::Pumpkin:
        case Blocks::SugarCane:
        case Blocks::NetherWart:
            return "grass";

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

    switch (id)
    {
        case Blocks::Dirt:
        case Blocks::Farmland:
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
        case Blocks::Leaves:
        case Blocks::BirchLeaves:
        case Blocks::TallGrass:
        case Blocks::FlowerRed:
        case Blocks::FlowerYellow:
        case Blocks::Cactus:
        case Blocks::Pumpkin:
        case Blocks::SugarCane:
        case Blocks::Torch:
        case Blocks::NetherWart:
        case Blocks::BrewingStand:
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
