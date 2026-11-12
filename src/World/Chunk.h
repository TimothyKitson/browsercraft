#pragma once
#include "Block.h"
#include "Renderer/Mesh.h"
#include <array>
#include <vector>
#include <atomic>
#include <cstdint>
#include <glm/glm.hpp>

struct ChunkPos
{
    int x = 0;
    int z = 0;
    bool operator==(const ChunkPos& o) const { return x == o.x && z == o.z; }
};

struct ChunkPosHash
{
    size_t operator()(const ChunkPos& p) const
    {
        // 2x 32-bit ints folded into one 64-bit hash.
        uint64_t a = static_cast<uint32_t>(p.x);
        uint64_t b = static_cast<uint32_t>(p.z);
        uint64_t h = a * 0x9E3779B97F4A7C15ull ^ (b + 0x165667B19E3779F9ull + (a << 6) + (a >> 2));
        return static_cast<size_t>(h);
    }
};

enum class ChunkState : uint8_t
{
    Empty,       // allocated, nothing in it yet
    Generating,  // a worker is filling in terrain
    Generated,   // terrain present, light not computed yet
    Lit          // terrain + light ready; can be meshed
};

// A 16 x 128 x 16 column of blocks plus its light data and GPU meshes.
class Chunk
{
public:
    static constexpr int SX = 16;
    static constexpr int SY = 128;
    static constexpr int SZ = 16;
    static constexpr int BLOCK_COUNT = SX * SY * SZ;

    explicit Chunk(ChunkPos position);

    static bool inBounds(int x, int y, int z)
    {
        return x >= 0 && x < SX && y >= 0 && y < SY && z >= 0 && z < SZ;
    }

    static int index(int x, int y, int z) { return (y * SZ + z) * SX + x; }

    BlockId getBlock(int x, int y, int z) const
    {
        if (!inBounds(x, y, z)) return Blocks::Air;
        return static_cast<BlockId>(m_blocks[index(x, y, z)] & ~WATERLOGGED);
    }

    BlockId rawBlock(int x, int y, int z) const
    {
        if (!inBounds(x, y, z)) return Blocks::Air;
        return m_blocks[index(x, y, z)];
    }

    bool waterlogged(int x, int y, int z) const
    {
        return inBounds(x, y, z) && (m_blocks[index(x, y, z)] & WATERLOGGED) != 0;
    }

    void setWaterlogged(int x, int y, int z, bool on)
    {
        if (!inBounds(x, y, z)) return;
        BlockId& cell = m_blocks[index(x, y, z)];
        cell = on ? static_cast<BlockId>(cell | WATERLOGGED) : static_cast<BlockId>(cell & ~WATERLOGGED);
    }

    void setBlockRaw(int x, int y, int z, BlockId id)
    {
        if (!inBounds(x, y, z)) return;
        BlockId& cell = m_blocks[index(x, y, z)];
        if ((id & WATERLOGGED) == 0 && (cell & WATERLOGGED) != 0 && canWaterlog(id)) id = static_cast<BlockId>(id | WATERLOGGED);
        cell = id;
    }

    uint8_t skyLight(int x, int y, int z) const
    {
        if (!inBounds(x, y, z)) return 0;
        return static_cast<uint8_t>(m_light[index(x, y, z)] >> 4);
    }

    uint8_t blockLight(int x, int y, int z) const
    {
        if (!inBounds(x, y, z)) return 0;
        return static_cast<uint8_t>(m_light[index(x, y, z)] & 0x0F);
    }

    void setSkyLight(int x, int y, int z, uint8_t value)
    {
        if (!inBounds(x, y, z)) return;
        uint8_t& cell = m_light[index(x, y, z)];
        cell = static_cast<uint8_t>((cell & 0x0F) | ((value & 0x0F) << 4));
    }

    void setBlockLight(int x, int y, int z, uint8_t value)
    {
        if (!inBounds(x, y, z)) return;
        uint8_t& cell = m_light[index(x, y, z)];
        cell = static_cast<uint8_t>((cell & 0xF0) | (value & 0x0F));
    }

    void clearLight() { std::fill(m_light.begin(), m_light.end(), uint8_t{ 0 }); }

    std::vector<BlockId>& blocks() { return m_blocks; }
    const std::vector<BlockId>& blocks() const { return m_blocks; }
    std::vector<uint8_t>& lightData() { return m_light; }

    ChunkPos position() const { return m_position; }
    glm::vec3 worldOrigin() const { return glm::vec3(m_position.x * SX, 0.0f, m_position.z * SZ); }
    glm::vec3 aabbMin() const { return worldOrigin(); }
    glm::vec3 aabbMax() const { return worldOrigin() + glm::vec3(SX, SY, SZ); }

    std::atomic<ChunkState> state{ ChunkState::Empty };
    bool meshDirty = true;      // geometry needs rebuilding
    bool meshJobActive = false; // a mesher job is in flight
    bool modified = false;      // player edited it, so it must be saved
    bool loadedFromDisk = false;

    std::array<uint32_t, SX * SZ> grassTint{};
    std::array<uint32_t, SX * SZ> foliageTint{};
    std::array<uint32_t, SX * SZ> waterTint{};

    Mesh opaqueMesh;
    Mesh transparentMesh;
    std::vector<glm::ivec3> signs;

private:
    ChunkPos m_position;
    std::vector<BlockId> m_blocks;
    std::vector<uint8_t> m_light; // high nibble = skylight, low nibble = block light
};
