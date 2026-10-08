#pragma once
#include "Chunk.h"
#include <vector>

// A thread-safe copy of a chunk plus a one-block border of its neighbours.
// The main thread fills this in while nothing else touches those chunks;
// the worker then meshes from the copy, so no locking is needed and faces
// at chunk borders can still be culled correctly.
struct MeshSnapshot
{
    static constexpr int PAD = 1;
    static constexpr int PX = Chunk::SX + 2 * PAD; // 18
    static constexpr int PZ = Chunk::SZ + 2 * PAD; // 18
    static constexpr int PY = Chunk::SY;

    ChunkPos pos{};
    std::vector<BlockId> blocks = std::vector<BlockId>(PX * PY * PZ, Blocks::Air);
    std::vector<uint8_t> light = std::vector<uint8_t>(PX * PY * PZ, 0);

    // x and z are chunk-local and may be -1 .. SX (inclusive).
    static int index(int x, int y, int z)
    {
        return ((y * PZ) + (z + PAD)) * PX + (x + PAD);
    }

    static bool inRange(int x, int y, int z)
    {
        return x >= -PAD && x < Chunk::SX + PAD &&
               z >= -PAD && z < Chunk::SZ + PAD &&
               y >= 0 && y < PY;
    }

    BlockId block(int x, int y, int z) const
    {
        if (y < 0) return Blocks::Stone;         // below the world: never draw downward faces
        if (y >= PY) return Blocks::Air;         // above the world: open sky
        if (!inRange(x, y, z)) return Blocks::Air;
        return blocks[index(x, y, z)];
    }

    uint8_t skyLight(int x, int y, int z) const
    {
        if (y >= PY) return 15;
        if (y < 0 || !inRange(x, y, z)) return 0;
        return static_cast<uint8_t>(light[index(x, y, z)] >> 4);
    }

    uint8_t blockLight(int x, int y, int z) const
    {
        if (y < 0 || y >= PY || !inRange(x, y, z)) return 0;
        return static_cast<uint8_t>(light[index(x, y, z)] & 0x0F);
    }
};

// Vertex layout produced by the mesher, matching assets/shaders/chunk.vert:
//   position (3), uv (2), shade (1), skylight (1), blocklight (1)
struct MeshResult
{
    ChunkPos pos{};
    std::vector<float> opaqueVertices;
    std::vector<float> transparentVertices;
};

// Pure CPU work -- safe to run on a worker thread.
void buildChunkMesh(const MeshSnapshot& snapshot, MeshResult& out);
