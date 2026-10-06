#pragma once
#include "Chunk.h"
#include "ChunkMesher.h"
#include "WorldGen.h"
#include "Core/ThreadPool.h"
#include <unordered_map>
#include <memory>
#include <deque>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <glm/glm.hpp>

struct RaycastHit
{
    bool hit = false;
    glm::ivec3 block{ 0 };   // the solid block that was hit
    glm::ivec3 previous{ 0 };// the empty cell just before it (where a new block goes)
    glm::ivec3 normal{ 0 };  // face normal that was hit
    float distance = 0.0f;
};

// One cell waiting to spread its light to its neighbours.
struct LightNode
{
    int x, y, z;
};

struct LightRemovalNode
{
    int x, y, z;
    uint8_t level;
};

// Owns every loaded chunk, streams them in and out around the player,
// propagates light, and answers block queries in world coordinates.
class World
{
public:
    World(uint32_t seed, std::string saveDirectory, int renderDistance,
          Dimension dimension = Dimension::Overworld);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void update(const glm::vec3& playerPosition);

    BlockId getBlock(int x, int y, int z) const;
    // Player-driven edit: updates light, flags neighbours for remeshing and
    // marks the chunk as needing a save.
    void setBlock(int x, int y, int z, BlockId id);

    uint8_t skyLight(int x, int y, int z) const;
    uint8_t blockLightAt(int x, int y, int z) const;

    Chunk* chunkAt(int chunkX, int chunkZ) const;
    const std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash>& chunks() const { return m_chunks; }

    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const;

    const WorldGen& generator() const { return m_generator; }
    Dimension dimension() const { return m_generator.dimension(); }
    uint32_t seed() const { return m_seed; }

    void saveAll();

    int loadedChunks() const { return static_cast<int>(m_chunks.size()); }
    int pendingJobs() const { return m_activeJobs.load(); }
    int renderDistance() const { return m_renderDistance; }
    void setRenderDistance(int distance);

    static int floorDiv(int value, int size) { return (value >= 0) ? (value / size) : ((value - size + 1) / size); }
    static int floorMod(int value, int size) { int m = value % size; return m < 0 ? m + size : m; }

private:
    uint32_t m_seed;
    std::string m_saveDirectory;
    int m_renderDistance;

    WorldGen m_generator;
    ThreadPool m_pool;

    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> m_chunks;

    std::mutex m_completedMutex;
    std::vector<MeshResult> m_completedMeshes;
    std::atomic<int> m_activeJobs{ 0 };

    std::deque<LightNode> m_skyAddQueue;
    std::deque<LightNode> m_blockAddQueue;
    std::deque<LightRemovalNode> m_skyRemoveQueue;
    std::deque<LightRemovalNode> m_blockRemoveQueue;

    // --- streaming ---
    void queueGeneration(const glm::vec3& playerPosition);
    void finishGeneratedChunks();
    void queueMeshJobs(const glm::vec3& playerPosition);
    void uploadCompletedMeshes();
    void unloadDistantChunks(const glm::vec3& playerPosition);

    bool neighboursReady(ChunkPos pos) const;
    void buildSnapshot(const Chunk& chunk, MeshSnapshot& snapshot) const;
    void markDirty(int chunkX, int chunkZ);
    void markNeighboursDirty(ChunkPos pos);

    // --- lighting ---
    void setSkyLight(int x, int y, int z, uint8_t value);
    void setBlockLight(int x, int y, int z, uint8_t value);
    void seedChunkLight(Chunk& chunk);
    void processLightQueues(int nodeBudget);
    void relightColumn(int worldX, int worldZ);

    friend void fillSkyColumns(Chunk& chunk);
};

// Fills a chunk's skylight purely from its own columns -- no neighbours
// needed, so this runs on the generation worker thread.
void fillSkyColumns(Chunk& chunk);

// How much light a block absorbs as it passes through (15 = fully blocks).
uint8_t lightAttenuation(BlockId id);
