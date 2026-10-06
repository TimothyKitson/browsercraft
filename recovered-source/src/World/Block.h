#pragma once
#include <cstdint>

using BlockId = uint8_t;

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
        Count
    };
}

enum class RenderType : uint8_t
{
    None,   // invisible (air)
    Cube,   // standard full block
    Cross,  // two diagonal quads (plants, torch)
    Liquid  // full block, slightly lowered top surface, drawn in the transparent pass
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
};

const BlockInfo& blockInfo(BlockId id);

inline bool isAir(BlockId id) { return id == Blocks::Air; }
inline bool isOpaque(BlockId id) { return blockInfo(id).opaque; }
inline bool isSolid(BlockId id) { return blockInfo(id).collides; }
inline bool isLiquid(BlockId id) { return blockInfo(id).render == RenderType::Liquid; }
inline bool isCross(BlockId id) { return blockInfo(id).render == RenderType::Cross; }
inline bool isVisible(BlockId id) { return blockInfo(id).render != RenderType::None; }
inline uint8_t lightEmission(BlockId id) { return blockInfo(id).lightEmission; }

// Blocks the player can pick up / place. Used by the creative palette.
bool isObtainable(BlockId id);

// Which footstep/dig sound family a block belongs to. The names match the
// folders under assets/sounds, so a caller builds "dig/" or "step/" plus this.
// Returns nullptr for blocks that make no sound, such as air.
const char* blockSoundGroup(BlockId id);
