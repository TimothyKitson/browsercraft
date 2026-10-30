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
    // 3: block ids are sixteen bits. A version 2 file stores them as
    // single bytes, so reading one as this format would turn every
    // chunk into noise -- the version check rejects it instead and
    // the world is generated again, which is what that check is for.
    constexpr uint16_t FORMAT_VERSION = 3;

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
    write(out, state.cheats);
    write(out, state.spawn.x);
    write(out, state.spawn.y);
    write(out, state.spawn.z);

    uint16_t damageCount = static_cast<uint16_t>(state.damage.size());
    write(out, damageCount);
    for (uint16_t worn : state.damage) write(out, worn);

    uint16_t furnaceCount = static_cast<uint16_t>(state.furnaces.size());
    write(out, furnaceCount);
    for (const LevelState::SavedFurnace& f : state.furnaces)
    {
        write(out, f.x); write(out, f.y); write(out, f.z);
        write(out, f.input); write(out, f.inputCount);
        write(out, f.fuel); write(out, f.fuelCount);
        write(out, f.output); write(out, f.outputCount);
        write(out, f.burnLeft); write(out, f.burnTotal); write(out, f.cooked);
    }

    uint16_t chestCount = static_cast<uint16_t>(state.chests.size());
    write(out, chestCount);
    for (const LevelState::SavedChest& c : state.chests)
    {
        write(out, c.x); write(out, c.y); write(out, c.z);
        uint16_t slots = static_cast<uint16_t>(c.slots.size());
        write(out, slots);
        for (const auto& slot : c.slots) { write(out, slot.first); write(out, slot.second); }
    }

    write(out, state.experience);
    write(out, state.experienceOwedHundredths);

    uint16_t enchantCount = static_cast<uint16_t>(state.enchantments.size());
    write(out, enchantCount);
    for (uint32_t set : state.enchantments) write(out, set);

    // Appended last, after the enchantments, because that is the whole
    // trick this format relies on: a reader that runs out of file keeps
    // its defaults. Slotted in before them instead, a level written by
    // the build before this one would have had its enchantment count
    // read as the air supply and every enchantment on every item in it
    // shifted by two bytes.
    write(out, state.air);

    // One byte per container, counted off the container lists rather
    // than off these -- the reader counts them that way, so a caller
    // that left them short would leave every later field reading the
    // wrong bytes. Nothing after an implicitly-sized field is safe
    // unless the writer and the reader agree on the size exactly.
    for (size_t i = 0; i < state.chests.size(); ++i)
        write(out, i < state.chestDimensions.size()
                       ? state.chestDimensions[i] : static_cast<uint8_t>(0));
    for (size_t i = 0; i < state.furnaces.size(); ++i)
        write(out, i < state.furnaceDimensions.size()
                       ? state.furnaceDimensions[i] : static_cast<uint8_t>(0));

    // Last of all, for the same reason as the air above.
    write(out, static_cast<uint16_t>(state.effectAmplifiers.size()));
    for (size_t i = 0; i < state.effectAmplifiers.size(); ++i)
    {
        write(out, state.effectAmplifiers[i]);
        write(out, i < state.effectSeconds.size() ? state.effectSeconds[i] : 0.0f);
    }
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

        bool cheats = out.cheats;
        if (read(in, cheats))
        {
            out.cheats = cheats;

            glm::vec3 spawn{ 0.0f };
            if (read(in, spawn.x) && read(in, spawn.y) && read(in, spawn.z))
            {
                out.spawn = spawn;
                out.hasSpawn = true;

                uint16_t damageCount = 0;
                if (read(in, damageCount))
                {
                    out.damage.clear();
                    for (uint16_t i = 0; i < damageCount; ++i)
                    {
                        uint16_t worn = 0;
                        if (!read(in, worn)) break;
                        out.damage.push_back(worn);
                    }

                    uint16_t furnaceCount = 0;
                    if (read(in, furnaceCount))
                    {
                        out.furnaces.clear();
                        for (uint16_t i = 0; i < furnaceCount; ++i)
                        {
                            LevelState::SavedFurnace f;
                            if (!read(in, f.x) || !read(in, f.y) || !read(in, f.z)) break;
                            if (!read(in, f.input) || !read(in, f.inputCount)) break;
                            if (!read(in, f.fuel) || !read(in, f.fuelCount)) break;
                            if (!read(in, f.output) || !read(in, f.outputCount)) break;
                            if (!read(in, f.burnLeft) || !read(in, f.burnTotal) ||
                                !read(in, f.cooked)) break;
                            out.furnaces.push_back(f);
                        }

                        uint16_t chestCount = 0;
                        if (read(in, chestCount))
                        {
                            out.chests.clear();
                            for (uint16_t i = 0; i < chestCount; ++i)
                            {
                                LevelState::SavedChest c;
                                if (!read(in, c.x) || !read(in, c.y) || !read(in, c.z)) break;

                                uint16_t slots = 0;
                                if (!read(in, slots)) break;

                                bool whole = true;
                                for (uint16_t k = 0; k < slots; ++k)
                                {
                                    uint16_t id = 0, count = 0;
                                    if (!read(in, id) || !read(in, count)) { whole = false; break; }
                                    c.slots.emplace_back(id, count);
                                }
                                if (!whole) break;
                                out.chests.push_back(c);
                            }

                            int experience = out.experience;
                            int owed = out.experienceOwedHundredths;
                            if (read(in, experience) && read(in, owed))
                            {
                                out.experience = experience;
                                out.experienceOwedHundredths = owed;

                                uint16_t enchantCount = 0;
                                if (read(in, enchantCount))
                                {
                                    out.enchantments.clear();
                                    for (uint16_t i = 0; i < enchantCount; ++i)
                                    {
                                        uint32_t set = 0;
                                        if (!read(in, set)) break;
                                        out.enchantments.push_back(set);
                                    }

                                    float air = out.air;
                                    if (read(in, air)) out.air = air;

                                    // One byte per chest and per
                                    // furnace, in the order they were
                                    // written. An older file simply
                                    // runs out here and every container
                                    // stays in the overworld.
                                    out.chestDimensions.assign(out.chests.size(), 0);
                                    out.furnaceDimensions.assign(out.furnaces.size(), 0);
                                    for (size_t i = 0; i < out.chests.size(); ++i)
                                    {
                                        uint8_t dimension = 0;
                                        if (!read(in, dimension)) break;
                                        out.chestDimensions[i] = dimension;
                                    }
                                    for (size_t i = 0; i < out.furnaces.size(); ++i)
                                    {
                                        uint8_t dimension = 0;
                                        if (!read(in, dimension)) break;
                                        out.furnaceDimensions[i] = dimension;
                                    }

                                    uint16_t effectCount = 0;
                                    if (read(in, effectCount))
                                    {
                                        for (uint16_t i = 0; i < effectCount; ++i)
                                        {
                                            int32_t amplifier = 0;
                                            float seconds = 0.0f;
                                            if (!read(in, amplifier)) break;
                                            if (!read(in, seconds)) break;
                                            out.effectAmplifiers.push_back(amplifier);
                                            out.effectSeconds.push_back(seconds);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}
