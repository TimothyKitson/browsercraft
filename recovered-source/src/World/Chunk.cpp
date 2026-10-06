#include "Chunk.h"

// Everything else a chunk does is inline in Chunk.h, and the geometry is
// built by buildChunkMesh() in ChunkMesher.cpp so it can run on a worker
// thread. All that is left here is allocating the block and light arrays.

Chunk::Chunk(ChunkPos position)
    : m_position(position)
    , m_blocks(BLOCK_COUNT, Blocks::Air)
    , m_light(BLOCK_COUNT, uint8_t{ 0 })
{
}
