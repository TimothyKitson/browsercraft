#include "World.h"
#include "WorldSave.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <limits>

namespace
{
    constexpr int GEN_JOBS_PER_FRAME = 12;
    constexpr int LIGHT_CHUNKS_PER_FRAME = 3;
    constexpr int MESH_JOBS_PER_FRAME = 6;
    constexpr int MESH_UPLOADS_PER_FRAME = 6;
    constexpr int LIGHT_NODE_BUDGET = 40000;
    constexpr double CHUNK_JOB_BUDGET_MS = 6.0; // per frame, single-threaded builds

    const int NEIGHBOUR_DX[6] = { 1, -1, 0, 0, 0, 0 };
    const int NEIGHBOUR_DY[6] = { 0, 0, 1, -1, 0, 0 };
    const int NEIGHBOUR_DZ[6] = { 0, 0, 0, 0, 1, -1 };
    constexpr int DIR_DOWN = 3;
}

uint8_t lightAttenuation(BlockId id)
{
    if (isOpaque(id)) return 15;
    switch (id)
    {
        case Blocks::Water:
        case Blocks::Ice:
            return 1;
        case Blocks::Leaves:
        case Blocks::BirchLeaves:
            return 2;
        default:
            return 0;
    }
}

void fillSkyColumns(Chunk& chunk)
{
    for (int x = 0; x < Chunk::SX; ++x)
    {
        for (int z = 0; z < Chunk::SZ; ++z)
        {
            uint8_t level = 15;
            for (int y = Chunk::SY - 1; y >= 0; --y)
            {
                const uint8_t attenuation = lightAttenuation(chunk.getBlock(x, y, z));
                if (attenuation >= 15) level = 0;
                else level = (level > attenuation) ? static_cast<uint8_t>(level - attenuation) : 0;
                chunk.setSkyLight(x, y, z, level);
            }
        }
    }
}

World::World(uint32_t seed, std::string saveDirectory, int renderDistance, Dimension dimension)
    : m_seed(seed)
    // Each dimension keeps its chunks in its own folder, the way Minecraft
    // splits DIM-1 and DIM1 out from the overworld's region files.
    , m_saveDirectory(*dimensionFolder(dimension)
                          ? saveDirectory + "/" + dimensionFolder(dimension)
                          : std::move(saveDirectory))
    , m_renderDistance(renderDistance)
    , m_generator(seed, dimension)
    , m_pool(std::max(2u, std::thread::hardware_concurrency() > 2 ? std::thread::hardware_concurrency() - 1 : 2u))
{
    WorldSave::ensureDirectories(m_saveDirectory);
}

World::~World()
{
    // Let in-flight generation/mesh jobs finish before chunks disappear.
    while (m_activeJobs.load() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    saveAll();
}

void World::setRenderDistance(int distance)
{
    m_renderDistance = std::clamp(distance, 2, 24);
}

Chunk* World::chunkAt(int chunkX, int chunkZ) const
{
    auto it = m_chunks.find(ChunkPos{ chunkX, chunkZ });
    return it != m_chunks.end() ? it->second.get() : nullptr;
}

BlockId World::getBlock(int x, int y, int z) const
{
    if (y < 0 || y >= Chunk::SY) return Blocks::Air;
    Chunk* chunk = chunkAt(floorDiv(x, Chunk::SX), floorDiv(z, Chunk::SZ));
    if (!chunk) return Blocks::Air;
    if (chunk->state.load() < ChunkState::Generated) return Blocks::Air;
    return chunk->getBlock(floorMod(x, Chunk::SX), y, floorMod(z, Chunk::SZ));
}

uint8_t World::skyLight(int x, int y, int z) const
{
    if (y < 0) return 0;
    if (y >= Chunk::SY) return 15;
    Chunk* chunk = chunkAt(floorDiv(x, Chunk::SX), floorDiv(z, Chunk::SZ));
    if (!chunk) return 0;
    return chunk->skyLight(floorMod(x, Chunk::SX), y, floorMod(z, Chunk::SZ));
}

uint8_t World::blockLightAt(int x, int y, int z) const
{
    if (y < 0 || y >= Chunk::SY) return 0;
    Chunk* chunk = chunkAt(floorDiv(x, Chunk::SX), floorDiv(z, Chunk::SZ));
    if (!chunk) return 0;
    return chunk->blockLight(floorMod(x, Chunk::SX), y, floorMod(z, Chunk::SZ));
}

void World::setSkyLight(int x, int y, int z, uint8_t value)
{
    if (y < 0 || y >= Chunk::SY) return;
    Chunk* chunk = chunkAt(floorDiv(x, Chunk::SX), floorDiv(z, Chunk::SZ));
    if (!chunk) return;
    chunk->setSkyLight(floorMod(x, Chunk::SX), y, floorMod(z, Chunk::SZ), value);
    chunk->meshDirty = true;
}

void World::setBlockLight(int x, int y, int z, uint8_t value)
{
    if (y < 0 || y >= Chunk::SY) return;
    Chunk* chunk = chunkAt(floorDiv(x, Chunk::SX), floorDiv(z, Chunk::SZ));
    if (!chunk) return;
    chunk->setBlockLight(floorMod(x, Chunk::SX), y, floorMod(z, Chunk::SZ), value);
    chunk->meshDirty = true;
}

void World::markDirty(int chunkX, int chunkZ)
{
    if (Chunk* chunk = chunkAt(chunkX, chunkZ))
        chunk->meshDirty = true;
}

void World::markNeighboursDirty(ChunkPos pos)
{
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            markDirty(pos.x + dx, pos.z + dz);
}

void World::setBlock(int x, int y, int z, BlockId id)
{
    if (y < 0 || y >= Chunk::SY) return;

    const int chunkX = floorDiv(x, Chunk::SX);
    const int chunkZ = floorDiv(z, Chunk::SZ);
    Chunk* chunk = chunkAt(chunkX, chunkZ);
    if (!chunk || chunk->state.load() < ChunkState::Generated) return;

    const int lx = floorMod(x, Chunk::SX);
    const int lz = floorMod(z, Chunk::SZ);
    const BlockId previous = chunk->getBlock(lx, y, lz);
    if (previous == id) return;

    chunk->setBlockRaw(lx, y, lz, id);
    chunk->modified = true;
    chunk->meshDirty = true;

    // A block on a chunk border changes the neighbour's visible faces too.
    if (lx == 0) markDirty(chunkX - 1, chunkZ);
    if (lx == Chunk::SX - 1) markDirty(chunkX + 1, chunkZ);
    if (lz == 0) markDirty(chunkX, chunkZ - 1);
    if (lz == Chunk::SZ - 1) markDirty(chunkX, chunkZ + 1);

    // --- block light ---
    const uint8_t previousEmission = lightEmission(previous);
    if (previousEmission > 0)
    {
        m_blockRemoveQueue.push_back({ x, y, z, previousEmission });
        setBlockLight(x, y, z, 0);
    }
    else if (isOpaque(id))
    {
        const uint8_t existing = blockLightAt(x, y, z);
        if (existing > 0)
        {
            m_blockRemoveQueue.push_back({ x, y, z, existing });
            setBlockLight(x, y, z, 0);
        }
    }

    const uint8_t emission = lightEmission(id);
    if (emission > 0)
    {
        setBlockLight(x, y, z, emission);
        m_blockAddQueue.push_back({ x, y, z });
    }
    else if (!isOpaque(id))
    {
        // Mining a block lets neighbouring light flow into the gap.
        for (int d = 0; d < 6; ++d)
            m_blockAddQueue.push_back({ x + NEIGHBOUR_DX[d], y + NEIGHBOUR_DY[d], z + NEIGHBOUR_DZ[d] });
    }

    // --- skylight ---
    relightColumn(x, z);
}

void World::relightColumn(int worldX, int worldZ)
{
    Chunk* chunk = chunkAt(floorDiv(worldX, Chunk::SX), floorDiv(worldZ, Chunk::SZ));
    if (!chunk) return;

    const int lx = floorMod(worldX, Chunk::SX);
    const int lz = floorMod(worldZ, Chunk::SZ);

    uint8_t level = 15;
    for (int y = Chunk::SY - 1; y >= 0; --y)
    {
        const uint8_t attenuation = lightAttenuation(chunk->getBlock(lx, y, lz));
        if (attenuation >= 15) level = 0;
        else level = (level > attenuation) ? static_cast<uint8_t>(level - attenuation) : 0;

        const uint8_t current = chunk->skyLight(lx, y, lz);
        if (level < current)
        {
            // Light here may still be legitimate from the side; zero it and
            // let the removal pass re-add whatever genuinely reaches it.
            chunk->setSkyLight(lx, y, lz, 0);
            m_skyRemoveQueue.push_back({ worldX, y, worldZ, current });
        }
        else if (level > current)
        {
            chunk->setSkyLight(lx, y, lz, level);
            m_skyAddQueue.push_back({ worldX, y, worldZ });
        }
        else if (level > 0)
        {
            m_skyAddQueue.push_back({ worldX, y, worldZ });
        }
    }
    chunk->meshDirty = true;
}

void World::seedChunkLight(Chunk& chunk)
{
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    auto localSky = [&](int lx, int ly, int lz) -> int {
        if (ly < 0) return 0;
        if (ly >= Chunk::SY) return 15;
        if (lx >= 0 && lx < Chunk::SX && lz >= 0 && lz < Chunk::SZ)
            return chunk.skyLight(lx, ly, lz);
        return skyLight(baseX + lx, ly, baseZ + lz);
    };

    auto localOpaque = [&](int lx, int ly, int lz) -> bool {
        if (ly < 0) return true;
        if (ly >= Chunk::SY) return false;
        if (lx >= 0 && lx < Chunk::SX && lz >= 0 && lz < Chunk::SZ)
            return isOpaque(chunk.getBlock(lx, ly, lz));
        return isOpaque(getBlock(baseX + lx, ly, baseZ + lz));
    };

    for (int y = 0; y < Chunk::SY; ++y)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            for (int lx = 0; lx < Chunk::SX; ++lx)
            {
                const BlockId id = chunk.getBlock(lx, y, lz);

                if (lightEmission(id) > 0)
                {
                    chunk.setBlockLight(lx, y, lz, lightEmission(id));
                    m_blockAddQueue.push_back({ baseX + lx, y, baseZ + lz });
                }

                const int sky = chunk.skyLight(lx, y, lz);
                if (sky <= 1) continue;

                // Only seed cells that actually border something darker --
                // seeding every lit cell would flood the queue with millions
                // of nodes that have nothing to do.
                bool bordersDarker = false;
                for (int d = 0; d < 6 && !bordersDarker; ++d)
                {
                    const int nx = lx + NEIGHBOUR_DX[d];
                    const int ny = y + NEIGHBOUR_DY[d];
                    const int nz = lz + NEIGHBOUR_DZ[d];
                    if (localOpaque(nx, ny, nz)) continue;
                    if (localSky(nx, ny, nz) < sky - 1) bordersDarker = true;
                }

                if (bordersDarker)
                    m_skyAddQueue.push_back({ baseX + lx, y, baseZ + lz });
            }
        }
    }
}

void World::processLightQueues(int nodeBudget)
{
    // Removals must run before additions, otherwise freshly added light
    // gets wiped by a stale removal wave.
    while (!m_skyRemoveQueue.empty() && nodeBudget > 0)
    {
        const LightRemovalNode node = m_skyRemoveQueue.front();
        m_skyRemoveQueue.pop_front();
        --nodeBudget;

        for (int d = 0; d < 6; ++d)
        {
            const int nx = node.x + NEIGHBOUR_DX[d];
            const int ny = node.y + NEIGHBOUR_DY[d];
            const int nz = node.z + NEIGHBOUR_DZ[d];
            if (ny < 0 || ny >= Chunk::SY) continue;
            if (!chunkAt(floorDiv(nx, Chunk::SX), floorDiv(nz, Chunk::SZ))) continue;

            const uint8_t neighbourLevel = skyLight(nx, ny, nz);
            if (neighbourLevel == 0) continue;

            const bool straightDownSunlight = (d == DIR_DOWN && node.level == 15);
            if (neighbourLevel < node.level || straightDownSunlight)
            {
                setSkyLight(nx, ny, nz, 0);
                m_skyRemoveQueue.push_back({ nx, ny, nz, neighbourLevel });
            }
            else
            {
                m_skyAddQueue.push_back({ nx, ny, nz });
            }
        }
    }

    while (!m_blockRemoveQueue.empty() && nodeBudget > 0)
    {
        const LightRemovalNode node = m_blockRemoveQueue.front();
        m_blockRemoveQueue.pop_front();
        --nodeBudget;

        for (int d = 0; d < 6; ++d)
        {
            const int nx = node.x + NEIGHBOUR_DX[d];
            const int ny = node.y + NEIGHBOUR_DY[d];
            const int nz = node.z + NEIGHBOUR_DZ[d];
            if (ny < 0 || ny >= Chunk::SY) continue;
            if (!chunkAt(floorDiv(nx, Chunk::SX), floorDiv(nz, Chunk::SZ))) continue;

            const uint8_t neighbourLevel = blockLightAt(nx, ny, nz);
            if (neighbourLevel == 0) continue;

            if (neighbourLevel < node.level)
            {
                setBlockLight(nx, ny, nz, 0);
                m_blockRemoveQueue.push_back({ nx, ny, nz, neighbourLevel });
            }
            else
            {
                m_blockAddQueue.push_back({ nx, ny, nz });
            }
        }
    }

    while (!m_skyAddQueue.empty() && nodeBudget > 0)
    {
        const LightNode node = m_skyAddQueue.front();
        m_skyAddQueue.pop_front();
        --nodeBudget;

        const uint8_t level = skyLight(node.x, node.y, node.z);
        if (level == 0) continue;

        for (int d = 0; d < 6; ++d)
        {
            const int nx = node.x + NEIGHBOUR_DX[d];
            const int ny = node.y + NEIGHBOUR_DY[d];
            const int nz = node.z + NEIGHBOUR_DZ[d];
            if (ny < 0 || ny >= Chunk::SY) continue;
            if (!chunkAt(floorDiv(nx, Chunk::SX), floorDiv(nz, Chunk::SZ))) continue;

            const uint8_t attenuation = lightAttenuation(getBlock(nx, ny, nz));
            if (attenuation >= 15) continue;

            // Sunlight falls straight down without weakening.
            int target = (d == DIR_DOWN && level == 15)
                ? 15 - attenuation
                : static_cast<int>(level) - 1 - attenuation;
            if (target <= 0) continue;

            if (skyLight(nx, ny, nz) < target)
            {
                setSkyLight(nx, ny, nz, static_cast<uint8_t>(target));
                m_skyAddQueue.push_back({ nx, ny, nz });
            }
        }
    }

    while (!m_blockAddQueue.empty() && nodeBudget > 0)
    {
        const LightNode node = m_blockAddQueue.front();
        m_blockAddQueue.pop_front();
        --nodeBudget;

        const uint8_t level = blockLightAt(node.x, node.y, node.z);
        if (level <= 1) continue;

        for (int d = 0; d < 6; ++d)
        {
            const int nx = node.x + NEIGHBOUR_DX[d];
            const int ny = node.y + NEIGHBOUR_DY[d];
            const int nz = node.z + NEIGHBOUR_DZ[d];
            if (ny < 0 || ny >= Chunk::SY) continue;
            if (!chunkAt(floorDiv(nx, Chunk::SX), floorDiv(nz, Chunk::SZ))) continue;

            const uint8_t attenuation = lightAttenuation(getBlock(nx, ny, nz));
            if (attenuation >= 15) continue;

            const int target = static_cast<int>(level) - 1 - attenuation;
            if (target <= 0) continue;

            if (blockLightAt(nx, ny, nz) < target)
            {
                setBlockLight(nx, ny, nz, static_cast<uint8_t>(target));
                m_blockAddQueue.push_back({ nx, ny, nz });
            }
        }
    }
}

bool World::neighboursReady(ChunkPos pos) const
{
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dz = -1; dz <= 1; ++dz)
        {
            const Chunk* neighbour = chunkAt(pos.x + dx, pos.z + dz);
            if (!neighbour) return false;
            if (neighbour->state.load() != ChunkState::Lit) return false;
        }
    }
    return true;
}

void World::buildSnapshot(const Chunk& chunk, MeshSnapshot& snapshot) const
{
    snapshot.pos = chunk.position();
    const int baseX = chunk.position().x * Chunk::SX;
    const int baseZ = chunk.position().z * Chunk::SZ;

    for (int z = -MeshSnapshot::PAD; z < Chunk::SZ + MeshSnapshot::PAD; ++z)
    {
        for (int x = -MeshSnapshot::PAD; x < Chunk::SX + MeshSnapshot::PAD; ++x)
        {
            const bool inside = (x >= 0 && x < Chunk::SX && z >= 0 && z < Chunk::SZ);
            const Chunk* source = inside
                ? &chunk
                : chunkAt(floorDiv(baseX + x, Chunk::SX), floorDiv(baseZ + z, Chunk::SZ));

            const int sx = inside ? x : floorMod(baseX + x, Chunk::SX);
            const int sz = inside ? z : floorMod(baseZ + z, Chunk::SZ);

            for (int y = 0; y < Chunk::SY; ++y)
            {
                const int index = MeshSnapshot::index(x, y, z);
                if (source)
                {
                    snapshot.blocks[index] = source->getBlock(sx, y, sz);
                    snapshot.light[index] = static_cast<uint8_t>((source->skyLight(sx, y, sz) << 4)
                                                                 | source->blockLight(sx, y, sz));
                }
                else
                {
                    snapshot.blocks[index] = Blocks::Air;
                    snapshot.light[index] = 15 << 4;
                }
            }
        }
    }
}

void World::queueGeneration(const glm::vec3& playerPosition)
{
    const int playerChunkX = floorDiv(static_cast<int>(std::floor(playerPosition.x)), Chunk::SX);
    const int playerChunkZ = floorDiv(static_cast<int>(std::floor(playerPosition.z)), Chunk::SZ);
    const int loadRadius = m_renderDistance + 1;

    struct Candidate { ChunkPos pos; int distanceSquared; };
    std::vector<Candidate> missing;

    for (int dx = -loadRadius; dx <= loadRadius; ++dx)
    {
        for (int dz = -loadRadius; dz <= loadRadius; ++dz)
        {
            if (dx * dx + dz * dz > loadRadius * loadRadius) continue;
            const ChunkPos pos{ playerChunkX + dx, playerChunkZ + dz };
            if (m_chunks.find(pos) != m_chunks.end()) continue;
            missing.push_back({ pos, dx * dx + dz * dz });
        }
    }

    if (missing.empty()) return;

    const size_t take = std::min<size_t>(missing.size(), GEN_JOBS_PER_FRAME);
    std::partial_sort(missing.begin(), missing.begin() + take, missing.end(),
                      [](const Candidate& a, const Candidate& b) { return a.distanceSquared < b.distanceSquared; });

    for (size_t i = 0; i < take; ++i)
    {
        const ChunkPos pos = missing[i].pos;
        auto chunk = std::make_unique<Chunk>(pos);
        Chunk* raw = chunk.get();
        m_chunks.emplace(pos, std::move(chunk));

        raw->state.store(ChunkState::Generating);
        m_activeJobs.fetch_add(1);

        m_pool.enqueue([this, raw]() {
            if (WorldSave::loadChunk(m_saveDirectory, *raw))
                raw->loadedFromDisk = true;
            else
                m_generator.generate(*raw);

            fillSkyColumns(*raw);
            raw->state.store(ChunkState::Generated);
            m_activeJobs.fetch_sub(1);
        });
    }
}

void World::finishGeneratedChunks()
{
    int processed = 0;
    for (auto& [pos, chunk] : m_chunks)
    {
        if (processed >= LIGHT_CHUNKS_PER_FRAME) break;
        if (chunk->state.load() != ChunkState::Generated) continue;

        seedChunkLight(*chunk);
        chunk->state.store(ChunkState::Lit);
        chunk->meshDirty = true;
        markNeighboursDirty(pos);
        ++processed;
    }
}

void World::queueMeshJobs(const glm::vec3& playerPosition)
{
    const int playerChunkX = floorDiv(static_cast<int>(std::floor(playerPosition.x)), Chunk::SX);
    const int playerChunkZ = floorDiv(static_cast<int>(std::floor(playerPosition.z)), Chunk::SZ);

    struct Candidate { Chunk* chunk; int distanceSquared; };
    std::vector<Candidate> candidates;

    for (auto& [pos, chunk] : m_chunks)
    {
        if (!chunk->meshDirty || chunk->meshJobActive) continue;
        if (chunk->state.load() != ChunkState::Lit) continue;

        const int dx = pos.x - playerChunkX;
        const int dz = pos.z - playerChunkZ;
        if (dx * dx + dz * dz > m_renderDistance * m_renderDistance) continue;
        if (!neighboursReady(pos)) continue;

        candidates.push_back({ chunk.get(), dx * dx + dz * dz });
    }

    if (candidates.empty()) return;

    const size_t take = std::min<size_t>(candidates.size(), MESH_JOBS_PER_FRAME);
    std::partial_sort(candidates.begin(), candidates.begin() + take, candidates.end(),
                      [](const Candidate& a, const Candidate& b) { return a.distanceSquared < b.distanceSquared; });

    for (size_t i = 0; i < take; ++i)
    {
        Chunk* chunk = candidates[i].chunk;
        chunk->meshDirty = false;
        chunk->meshJobActive = true;

        auto snapshot = std::make_shared<MeshSnapshot>();
        buildSnapshot(*chunk, *snapshot);

        m_activeJobs.fetch_add(1);
        m_pool.enqueue([this, snapshot, chunk]() {
            MeshResult result;
            buildChunkMesh(*snapshot, result);
            {
                std::lock_guard<std::mutex> lock(m_completedMutex);
                m_completedMeshes.push_back(std::move(result));
            }
            chunk->meshJobActive = false;
            m_activeJobs.fetch_sub(1);
        });
    }
}

void World::uploadCompletedMeshes()
{
    std::vector<MeshResult> ready;
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        if (m_completedMeshes.empty()) return;
        const size_t take = std::min<size_t>(m_completedMeshes.size(), MESH_UPLOADS_PER_FRAME);
        ready.insert(ready.end(),
                     std::make_move_iterator(m_completedMeshes.begin()),
                     std::make_move_iterator(m_completedMeshes.begin() + take));
        m_completedMeshes.erase(m_completedMeshes.begin(), m_completedMeshes.begin() + take);
    }

    static const std::vector<int> LAYOUT = { 3, 2, 1, 1, 1 }; // pos, uv, shade, sky, block

    for (MeshResult& result : ready)
    {
        Chunk* chunk = chunkAt(result.pos.x, result.pos.z);
        if (!chunk) continue; // unloaded while the job was running
        chunk->opaqueMesh.upload(result.opaqueVertices, LAYOUT);
        chunk->transparentMesh.upload(result.transparentVertices, LAYOUT);
    }
}

void World::unloadDistantChunks(const glm::vec3& playerPosition)
{
    const int playerChunkX = floorDiv(static_cast<int>(std::floor(playerPosition.x)), Chunk::SX);
    const int playerChunkZ = floorDiv(static_cast<int>(std::floor(playerPosition.z)), Chunk::SZ);
    const int keepRadius = m_renderDistance + 3;

    for (auto it = m_chunks.begin(); it != m_chunks.end();)
    {
        Chunk* chunk = it->second.get();
        const int dx = it->first.x - playerChunkX;
        const int dz = it->first.z - playerChunkZ;
        const bool tooFar = (dx * dx + dz * dz) > keepRadius * keepRadius;

        // Never drop a chunk a worker is still writing into.
        const bool busy = chunk->state.load() == ChunkState::Generating || chunk->meshJobActive;

        if (tooFar && !busy)
        {
            if (chunk->modified) WorldSave::saveChunk(m_saveDirectory, *chunk);
            it = m_chunks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void World::update(const glm::vec3& playerPosition)
{
    // On the web there are no worker threads, so the queued generation and
    // meshing jobs only make progress if something pumps them. Without this
    // the queue grows forever and no chunk is ever built. The budget keeps a
    // frame from stalling; it is a no-op on native builds, where real workers
    // are already draining the same queue.
    m_pool.runPending(CHUNK_JOB_BUDGET_MS);

    queueGeneration(playerPosition);
    finishGeneratedChunks();
    processLightQueues(LIGHT_NODE_BUDGET);
    queueMeshJobs(playerPosition);
    uploadCompletedMeshes();
    unloadDistantChunks(playerPosition);
}

void World::saveAll()
{
    for (auto& [pos, chunk] : m_chunks)
    {
        if (chunk->modified)
            WorldSave::saveChunk(m_saveDirectory, *chunk);
    }
}

RaycastHit World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const
{
    RaycastHit hit;

    glm::vec3 dir = direction;
    if (glm::length(dir) < 1e-6f) return hit;
    dir = glm::normalize(dir);

    glm::ivec3 voxel(static_cast<int>(std::floor(origin.x)),
                     static_cast<int>(std::floor(origin.y)),
                     static_cast<int>(std::floor(origin.z)));

    const glm::ivec3 step(dir.x > 0 ? 1 : (dir.x < 0 ? -1 : 0),
                          dir.y > 0 ? 1 : (dir.y < 0 ? -1 : 0),
                          dir.z > 0 ? 1 : (dir.z < 0 ? -1 : 0));

    const float infinity = std::numeric_limits<float>::infinity();
    glm::vec3 tDelta(dir.x != 0.0f ? std::fabs(1.0f / dir.x) : infinity,
                     dir.y != 0.0f ? std::fabs(1.0f / dir.y) : infinity,
                     dir.z != 0.0f ? std::fabs(1.0f / dir.z) : infinity);

    auto firstBoundary = [](float originComponent, int voxelComponent, float directionComponent) -> float {
        if (directionComponent > 0.0f)
            return ((voxelComponent + 1) - originComponent) / directionComponent;
        if (directionComponent < 0.0f)
            return (originComponent - voxelComponent) / -directionComponent;
        return std::numeric_limits<float>::infinity();
    };

    glm::vec3 tMax(firstBoundary(origin.x, voxel.x, dir.x),
                   firstBoundary(origin.y, voxel.y, dir.y),
                   firstBoundary(origin.z, voxel.z, dir.z));

    glm::ivec3 normal(0);
    float travelled = 0.0f;

    // Amanatides & Woo: step to whichever axis boundary is nearest, so every
    // voxel along the ray is visited exactly once.
    while (travelled <= maxDistance)
    {
        const BlockId id = getBlock(voxel.x, voxel.y, voxel.z);
        if (id != Blocks::Air && !isLiquid(id))
        {
            hit.hit = true;
            hit.block = voxel;
            hit.normal = normal;
            hit.previous = voxel + normal;
            hit.distance = travelled;
            return hit;
        }

        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            voxel.x += step.x;
            travelled = tMax.x;
            tMax.x += tDelta.x;
            normal = glm::ivec3(-step.x, 0, 0);
        }
        else if (tMax.y < tMax.z)
        {
            voxel.y += step.y;
            travelled = tMax.y;
            tMax.y += tDelta.y;
            normal = glm::ivec3(0, -step.y, 0);
        }
        else
        {
            voxel.z += step.z;
            travelled = tMax.z;
            tMax.z += tDelta.z;
            normal = glm::ivec3(0, 0, -step.z);
        }

        if (voxel.y < -1 || voxel.y > Chunk::SY + 1) break;
    }

    return hit;
}
