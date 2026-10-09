#pragma once
#include <cstdint>

using BlockId = uint8_t;

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

        Farmland,
        Wheat0, Wheat1, Wheat2, Wheat3, Wheat4, Wheat5, Wheat6, Wheat7,

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
inline bool isLeaves(BlockId id) { return id == Blocks::Leaves || id == Blocks::BirchLeaves; }
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
inline BlockId wheatAtStage(int stage) { return static_cast<BlockId>(Blocks::Wheat0 + (stage < 0 ? 0 : (stage > WHEAT_STAGES - 1 ? WHEAT_STAGES - 1 : stage))); }
