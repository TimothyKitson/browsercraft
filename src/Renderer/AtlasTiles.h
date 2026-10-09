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
        TileCount
    };

    // Items share the atlas with the blocks rather than having one of
    // their own: a hotbar slot draws the same way whichever it holds,
    // and one atlas is one texture bind. They start well clear of the
    // blocks so adding a block never renumbers them.
    constexpr int ItemFirst = 64;

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

        ItemStick,
        ItemCoal,
        ItemDiamond,
        ItemIronIngot,
        ItemGoldIngot,

        // Five kinds in each of five tiers, in the same order as the
        // Items enum, so the painter can work out which is which from
        // the offset instead of being told twenty-five times.
        ItemToolFirst,
        ItemToolEnd = ItemToolFirst + 25,

        ItemTileEnd
    };

    // Ten progressively more broken overlay tiles for the mining animation.
    constexpr int CrackFirst = 240;
    constexpr int CrackStages = 10;

    static_assert(static_cast<int>(TileCount) <= ItemFirst, "block tiles ran into the items");
    static_assert(static_cast<int>(ItemTileEnd) <= CrackFirst, "item tiles ran into the cracks");
}
