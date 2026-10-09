#include "WorldSave.h"
#include <filesystem>
#include <fstream>
#include <cstdio>

namespace fs = std::filesystem;

namespace
{
    constexpr uint32_t CHUNK_MAGIC = 0x56584348; // "VXCH"
    constexpr uint32_t LEVEL_MAGIC = 0x56584C56; // "VXLV"
    // 2: slot ids widened to sixteen bits so inventories can hold items,
    //    and the game mode and hardcore death now persist.
    constexpr uint16_t FORMAT_VERSION = 2;

    std::string chunkPath(const std::string& dir, ChunkPos pos)
    {
        return dir + "/chunks/c." + std::to_string(pos.x) + "." + std::to_string(pos.z) + ".dat";
    }

    std::string levelPath(const std::string& dir) { return dir + "/level.dat"; }

    template <typename T>
    void write(std::ofstream& out, const T& value)
    {
        out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template <typename T>
    bool read(std::ifstream& in, T& value)
    {
        in.read(reinterpret_cast<char*>(&value), sizeof(T));
        return static_cast<bool>(in);
    }
}

void WorldSave::ensureDirectories(const std::string& saveDirectory)
{
    std::error_code ec;
    fs::create_directories(saveDirectory + "/chunks", ec);
}

void WorldSave::saveChunk(const std::string& saveDirectory, const Chunk& chunk)
{
    std::ofstream out(chunkPath(saveDirectory, chunk.position()), std::ios::binary);
    if (!out) return;

    write(out, CHUNK_MAGIC);
    write(out, FORMAT_VERSION);

    // Run-length encode: voxel terrain is extremely repetitive, so this
    // typically shrinks a 32 KB chunk to a few hundred bytes.
    const std::vector<BlockId>& blocks = chunk.blocks();
    size_t i = 0;
    while (i < blocks.size())
    {
        const BlockId id = blocks[i];
        uint16_t run = 1;
        while (i + run < blocks.size() && blocks[i + run] == id && run < 0xFFFF) ++run;
        write(out, id);
        write(out, run);
        i += run;
    }
}

bool WorldSave::loadChunk(const std::string& saveDirectory, Chunk& chunk)
{
    std::ifstream in(chunkPath(saveDirectory, chunk.position()), std::ios::binary);
    if (!in) return false;

    uint32_t magic = 0;
    uint16_t version = 0;
    if (!read(in, magic) || !read(in, version)) return false;
    if (magic != CHUNK_MAGIC || version != FORMAT_VERSION) return false;

    std::vector<BlockId>& blocks = chunk.blocks();
    size_t i = 0;
    while (i < blocks.size())
    {
        BlockId id = 0;
        uint16_t run = 0;
        if (!read(in, id) || !read(in, run)) return false;
        if (run == 0) return false;
        for (uint16_t k = 0; k < run && i < blocks.size(); ++k)
            blocks[i++] = id;
    }
    return true;
}

void WorldSave::saveLevel(const std::string& saveDirectory, const LevelState& state)
{
    std::ofstream out(levelPath(saveDirectory), std::ios::binary);
    if (!out) return;

    write(out, LEVEL_MAGIC);
    write(out, FORMAT_VERSION);
    write(out, state.seed);
    write(out, state.playerPosition.x);
    write(out, state.playerPosition.y);
    write(out, state.playerPosition.z);
    write(out, state.yaw);
    write(out, state.pitch);
    write(out, state.timeOfDay);
    write(out, state.health);
    write(out, state.creative);
    write(out, state.selectedSlot);
    write(out, state.gameMode);
    write(out, state.dead);

    uint16_t slotCount = static_cast<uint16_t>(state.inventory.size());
    write(out, slotCount);
    for (const auto& slot : state.inventory)
    {
        write(out, slot.first);
        write(out, slot.second);
    }

    // Appended after everything else rather than slotted in beside the
    // health, so that a level written before hunger existed still loads:
    // the read below simply runs out of file and keeps the defaults.
    // FORMAT_VERSION is shared with the chunk files, and bumping it to
    // add two numbers would throw away every saved chunk in the world.
    write(out, state.hunger);
    write(out, state.saturation);
}

bool WorldSave::loadLevel(const std::string& saveDirectory, LevelState& out)
{
    std::ifstream in(levelPath(saveDirectory), std::ios::binary);
    if (!in) return false;

    uint32_t magic = 0;
    uint16_t version = 0;
    if (!read(in, magic) || !read(in, version)) return false;
    if (magic != LEVEL_MAGIC || version != FORMAT_VERSION) return false;

    if (!read(in, out.seed)) return false;
    if (!read(in, out.playerPosition.x)) return false;
    if (!read(in, out.playerPosition.y)) return false;
    if (!read(in, out.playerPosition.z)) return false;
    if (!read(in, out.yaw)) return false;
    if (!read(in, out.pitch)) return false;
    if (!read(in, out.timeOfDay)) return false;
    if (!read(in, out.health)) return false;
    if (!read(in, out.creative)) return false;
    if (!read(in, out.selectedSlot)) return false;
    if (!read(in, out.gameMode)) return false;
    if (!read(in, out.dead)) return false;

    uint16_t slotCount = 0;
    if (!read(in, slotCount)) return false;
    out.inventory.clear();
    for (uint16_t i = 0; i < slotCount; ++i)
    {
        uint16_t id = 0;
        uint16_t count = 0;
        if (!read(in, id) || !read(in, count)) return false;
        out.inventory.emplace_back(id, count);
    }

    int hunger = out.hunger;
    float saturation = out.saturation;
    if (read(in, hunger) && read(in, saturation))
    {
        out.hunger = hunger;
        out.saturation = saturation;
    }
    return true;
}
