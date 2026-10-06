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
            return false;
        default:
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
            return "grass";

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
        case Blocks::Torch:
            return "wood";

        case Blocks::Wool:
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
