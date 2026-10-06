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
        TileCount
    };

    // Ten progressively more broken overlay tiles for the mining animation.
    constexpr int CrackFirst = 240;
    constexpr int CrackStages = 10;
}
