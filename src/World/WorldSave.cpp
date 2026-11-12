#include "WorldSave.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
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
    constexpr uint16_t ITEM_BASE = 4096;
    constexpr uint16_t FIRST_ITEM_BASE = 1024;

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

    // A line of text, length first. Short on purpose: a sign's line
    // is fifteen characters, and a length that cannot be trusted is
    // the easiest way to turn a truncated file into a hang.
    void writeLine(std::ofstream& out, const std::string& text)
    {
        const uint8_t length = static_cast<uint8_t>(
            text.size() > 255 ? 255 : text.size());
        write(out, length);
        if (length > 0) out.write(text.data(), length);
    }

    void writeText(std::ofstream& out, const std::string& text)
    {
        const uint16_t length = static_cast<uint16_t>(text.size() > 4096 ? 4096 : text.size());
        write(out, length);
        if (length > 0) out.write(text.data(), length);
    }

    bool readText(std::ifstream& in, std::string& text)
    {
        uint16_t length = 0;
        if (!read(in, length)) return false;
        text.assign(length, '\0');
        if (length > 0) in.read(text.data(), length);
        return static_cast<bool>(in);
    }

    bool readLine(std::ifstream& in, std::string& text)
    {
        uint8_t length = 0;
        if (!read(in, length)) return false;
        text.assign(length, '\0');
        if (length > 0) in.read(text.data(), length);
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

    // And after those, which machine each furnace is. Counted off the
    // furnace list, like the dimensions above.
    for (size_t i = 0; i < state.furnaces.size(); ++i)
        write(out, i < state.furnaceKinds.size()
                       ? state.furnaceKinds[i] : static_cast<uint8_t>(0));

    // Last of all: what every sign says. Counted explicitly rather
    // than off another list, because nothing else in the file knows
    // how many signs there are.
    write(out, static_cast<uint16_t>(state.signs.size()));
    for (const LevelState::SavedSign& sign : state.signs)
    {
        write(out, sign.x);
        write(out, sign.y);
        write(out, sign.z);
        write(out, sign.dimension);
        for (const std::string& line : sign.lines) writeLine(out, line);
    }

    write(out, static_cast<uint16_t>(state.frames.size()));
    for (const LevelState::SavedFrame& frame : state.frames)
    {
        write(out, frame.x);
        write(out, frame.y);
        write(out, frame.z);
        write(out, frame.dimension);
        write(out, frame.item);
        write(out, frame.count);
        write(out, frame.damage);
        write(out, frame.enchantments);
    }

    write(out, static_cast<uint16_t>(state.beacons.size()));
    for (const LevelState::SavedBeacon& beacon : state.beacons)
    {
        write(out, beacon.x);
        write(out, beacon.y);
        write(out, beacon.z);
        write(out, beacon.dimension);
        write(out, beacon.primary);
        write(out, beacon.secondary);
    }

    write(out, ITEM_BASE);
    write(out, static_cast<uint8_t>(state.hasAnchor ? 1 : 0));
    write(out, state.anchorX);
    write(out, state.anchorY);
    write(out, state.anchorZ);
    write(out, static_cast<uint32_t>(state.pots.size()));
    for (const LevelState::SavedPot& pot : state.pots)
    {
        write(out, pot.x);
        write(out, pot.y);
        write(out, pot.z);
        write(out, pot.dimension);
        write(out, pot.sides);
    }
    write(out, static_cast<int32_t>(state.nextMapId));
    write(out, static_cast<uint32_t>(state.bannerDesigns.size()));
    for (const auto& design : state.bannerDesigns)
    {
        write(out, static_cast<uint8_t>(design.size()));
        for (const auto& layer : design)
        {
            write(out, layer.first);
            write(out, layer.second);
        }
    }
    write(out, static_cast<uint32_t>(state.banners.size()));
    for (const LevelState::SavedBanner& banner : state.banners)
    {
        write(out, banner.x);
        write(out, banner.y);
        write(out, banner.z);
        write(out, banner.dimension);
        write(out, banner.design);
    }
    write(out, static_cast<uint32_t>(state.commands.size()));
    for (const LevelState::SavedCommand& command : state.commands)
    {
        write(out, command.x);
        write(out, command.y);
        write(out, command.z);
        write(out, command.dimension);
        write(out, command.flags);
        writeLine(out, command.command);
        writeLine(out, command.output);
    }
    write(out, static_cast<uint32_t>(state.structures.size()));
    for (const LevelState::SavedStructure& structure : state.structures)
    {
        write(out, structure.x);
        write(out, structure.y);
        write(out, structure.z);
        write(out, structure.dimension);
        writeLine(out, structure.name);
        for (int k = 0; k < 3; ++k) write(out, static_cast<int32_t>(structure.offset[k]));
        for (int k = 0; k < 3; ++k) write(out, static_cast<int32_t>(structure.size[k]));
    }
    write(out, static_cast<uint32_t>(state.itemNames.size()));
    for (const std::string& name : state.itemNames) writeLine(out, name);
    write(out, static_cast<uint32_t>(state.lodestones.size()));
    for (const auto& stone : state.lodestones)
        for (int k = 0; k < 4; ++k) write(out, static_cast<int32_t>(stone[static_cast<size_t>(k)]));
    write(out, static_cast<uint8_t>(state.hasDeathPoint ? 1 : 0));
    for (int k = 0; k < 3; ++k) write(out, state.deathPoint[k]);
    write(out, state.deathDimension);
    write(out, static_cast<uint32_t>(state.bundles.size()));
    for (const auto& bundle : state.bundles)
    {
        write(out, static_cast<uint16_t>(bundle.size()));
        for (const LevelState::SavedBundleStack& stack : bundle)
        {
            write(out, stack.id);
            write(out, stack.count);
            write(out, stack.damage);
            write(out, stack.enchantments);
        }
    }
    write(out, static_cast<uint32_t>(state.books.size()));
    for (const LevelState::SavedBook& book : state.books)
    {
        writeLine(out, book.title);
        writeLine(out, book.author);
        write(out, static_cast<uint16_t>(book.pages.size()));
        for (const std::string& page : book.pages) writeText(out, page);
    }
    write(out, static_cast<uint32_t>(state.lecterns.size()));
    for (const LevelState::SavedLectern& lectern : state.lecterns)
    {
        write(out, lectern.x);
        write(out, lectern.y);
        write(out, lectern.z);
        write(out, lectern.dimension);
        write(out, lectern.stack.id);
        write(out, lectern.stack.count);
        write(out, lectern.stack.damage);
        write(out, lectern.stack.enchantments);
    }
    write(out, static_cast<uint32_t>(state.vehicles.size()));
    for (const LevelState::SavedVehicle& vehicle : state.vehicles)
    {
        write(out, vehicle.dimension);
        write(out, vehicle.type);
        write(out, vehicle.variant);
        write(out, vehicle.x);
        write(out, vehicle.y);
        write(out, vehicle.z);
        write(out, vehicle.yaw);
        write(out, vehicle.fuel);
        writeLine(out, vehicle.command);
        write(out, static_cast<uint8_t>(vehicle.contents.size()));
        for (const auto& slot : vehicle.contents)
        {
            write(out, slot.first);
            write(out, slot.second.id);
            write(out, slot.second.count);
            write(out, slot.second.damage);
            write(out, slot.second.enchantments);
        }
    }
    write(out, static_cast<uint32_t>(state.cages.size()));
    for (const LevelState::SavedCage& cage : state.cages)
    {
        write(out, cage.dimension);
        write(out, cage.x);
        write(out, cage.y);
        write(out, cage.z);
        write(out, cage.mob);
    }
    write(out, static_cast<uint32_t>(state.fireworkStars.size()));
    for (const LevelState::SavedExplosion& star : state.fireworkStars)
    {
        write(out, star.shape);
        write(out, star.flags);
        write(out, static_cast<uint8_t>(star.colours.size()));
        for (uint8_t c : star.colours) write(out, c);
        write(out, static_cast<uint8_t>(star.fades.size()));
        for (uint8_t c : star.fades) write(out, c);
    }
    write(out, static_cast<uint32_t>(state.fireworkRockets.size()));
    for (const LevelState::SavedRocket& rocket : state.fireworkRockets)
    {
        write(out, rocket.flight);
        write(out, static_cast<uint8_t>(rocket.explosions.size()));
        for (uint16_t id : rocket.explosions) write(out, id);
    }
    write(out, static_cast<uint8_t>(state.crossbowRockets.size()));
    for (uint16_t id : state.crossbowRockets) write(out, id);
    write(out, static_cast<uint32_t>(state.jukeboxes.size()));
    for (const LevelState::SavedLectern& box : state.jukeboxes)
    {
        write(out, box.x);
        write(out, box.y);
        write(out, box.z);
        write(out, box.dimension);
        write(out, box.stack.id);
    }
    write(out, static_cast<uint32_t>(state.signLooks.size()));
    for (const LevelState::SavedSignLook& look : state.signLooks)
    {
        write(out, look.x);
        write(out, look.y);
        write(out, look.z);
        write(out, look.dimension);
        write(out, look.colour);
        write(out, look.glow);
    }
    write(out, static_cast<uint32_t>(state.paintings.size()));
    for (const LevelState::SavedPainting& painting : state.paintings)
    {
        write(out, painting.x);
        write(out, painting.y);
        write(out, painting.z);
        write(out, painting.dimension);
        write(out, painting.facing);
        writeLine(out, painting.variant);
    }
    write(out, static_cast<uint32_t>(state.stands.size()));
    for (const LevelState::SavedStand& stand : state.stands)
    {
        write(out, stand.x);
        write(out, stand.y);
        write(out, stand.z);
        write(out, stand.yaw);
        write(out, stand.dimension);
        write(out, stand.pose);
        for (const LevelState::SavedGear& gear : stand.gear)
        {
            write(out, gear.id);
            write(out, gear.count);
            write(out, gear.damage);
            write(out, gear.enchantments);
        }
    }
    write(out, static_cast<uint32_t>(state.structures.size()));
    for (const LevelState::SavedStructure& structure : state.structures)
    {
        write(out, structure.rotation);
        write(out, structure.mirror);
        write(out, structure.integrity);
        write(out, structure.seed);
    }
}

bool WorldSave::loadLevel(const std::string& saveDirectory, LevelState& out)
{
    out.itemBase = FIRST_ITEM_BASE;
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

                                        // One byte per furnace saying
                                        // which machine it is. A level
                                        // from before the smoker runs
                                        // out here and keeps the zero,
                                        // which means a furnace.
                                        out.furnaceKinds.assign(out.furnaces.size(), 0);
                                        bool kindsWhole = true;
                                        for (size_t i = 0; i < out.furnaces.size(); ++i)
                                        {
                                            uint8_t kind = 0;
                                            if (!read(in, kind)) { kindsWhole = false; break; }
                                            out.furnaceKinds[i] = kind;
                                        }

                                        // The signs, if this file is
                                        // new enough to have any. A
                                        // short read anywhere above
                                        // means there is nothing here
                                        // to find.
                                        uint16_t signCount = 0;
                                        bool signsWhole = kindsWhole && read(in, signCount);
                                        if (signsWhole)
                                        {
                                            for (uint16_t i = 0; i < signCount; ++i)
                                            {
                                                LevelState::SavedSign sign;
                                                if (!read(in, sign.x)) { signsWhole = false; break; }
                                                if (!read(in, sign.y)) { signsWhole = false; break; }
                                                if (!read(in, sign.z)) { signsWhole = false; break; }
                                                if (!read(in, sign.dimension)) { signsWhole = false; break; }

                                                bool whole = true;
                                                for (std::string& line : sign.lines)
                                                    if (!readLine(in, line)) { whole = false; break; }
                                                if (!whole) { signsWhole = false; break; }

                                                out.signs.push_back(sign);
                                            }
                                        }

                                        uint16_t frameCount = 0;
                                        bool framesWhole = signsWhole && read(in, frameCount);
                                        if (framesWhole)
                                        {
                                            for (uint16_t i = 0; i < frameCount; ++i)
                                            {
                                                LevelState::SavedFrame frame;
                                                if (!read(in, frame.x) || !read(in, frame.y) || !read(in, frame.z) ||
                                                    !read(in, frame.dimension) || !read(in, frame.item) ||
                                                    !read(in, frame.count) || !read(in, frame.damage) ||
                                                    !read(in, frame.enchantments))
                                                {
                                                    framesWhole = false;
                                                    break;
                                                }
                                                out.frames.push_back(frame);
                                            }
                                        }

                                        uint16_t beaconCount = 0;
                                        bool beaconsWhole = framesWhole && read(in, beaconCount);
                                        if (beaconsWhole)
                                        {
                                            for (uint16_t i = 0; i < beaconCount; ++i)
                                            {
                                                LevelState::SavedBeacon beacon;
                                                if (!read(in, beacon.x) || !read(in, beacon.y) || !read(in, beacon.z) ||
                                                    !read(in, beacon.dimension) || !read(in, beacon.primary) ||
                                                    !read(in, beacon.secondary))
                                                {
                                                    beaconsWhole = false;
                                                    break;
                                                }
                                                out.beacons.push_back(beacon);
                                            }
                                        }
                                        uint16_t base = 0;
                                        if (beaconsWhole && read(in, base))
                                        {
                                            out.itemBase = base;
                                            uint8_t anchored = 0;
                                            if (read(in, anchored) && read(in, out.anchorX) && read(in, out.anchorY) && read(in, out.anchorZ))
                                            {
                                                out.hasAnchor = anchored != 0;
                                                uint32_t potCount = 0;
                                                bool potsWhole = read(in, potCount);
                                                if (potsWhole)
                                                    for (uint32_t i = 0; i < potCount && i < 1000000u; ++i)
                                                    {
                                                        LevelState::SavedPot pot;
                                                        if (!read(in, pot.x) || !read(in, pot.y) || !read(in, pot.z) ||
                                                            !read(in, pot.dimension) || !read(in, pot.sides))
                                                        {
                                                            potsWhole = false;
                                                            break;
                                                        }
                                                        out.pots.push_back(pot);
                                                    }
                                                int32_t nextMap = 0;
                                                const bool mapWhole = potsWhole && read(in, nextMap);
                                                if (mapWhole && nextMap > 0) out.nextMapId = nextMap;
                                                uint32_t designCount = 0;
                                                bool designsWhole = mapWhole && read(in, designCount);
                                                if (designsWhole)
                                                    for (uint32_t i = 0; i < designCount && i < 65536u; ++i)
                                                    {
                                                        uint8_t layers = 0;
                                                        if (!read(in, layers)) { designsWhole = false; break; }
                                                        std::vector<std::pair<uint8_t, uint8_t>> design;
                                                        for (uint8_t k = 0; k < layers; ++k)
                                                        {
                                                            uint8_t pattern = 0, colour = 0;
                                                            if (!read(in, pattern) || !read(in, colour)) { designsWhole = false; break; }
                                                            design.emplace_back(pattern, colour);
                                                        }
                                                        if (!designsWhole) break;
                                                        out.bannerDesigns.push_back(design);
                                                    }
                                                uint32_t bannerCount = 0;
                                                bool bannersWhole = designsWhole && read(in, bannerCount);
                                                if (bannersWhole)
                                                    for (uint32_t i = 0; i < bannerCount && i < 1000000u; ++i)
                                                    {
                                                        LevelState::SavedBanner banner;
                                                        if (!read(in, banner.x) || !read(in, banner.y) || !read(in, banner.z) ||
                                                            !read(in, banner.dimension) || !read(in, banner.design))
                                                        {
                                                            bannersWhole = false;
                                                            break;
                                                        }
                                                        out.banners.push_back(banner);
                                                    }
                                                uint32_t commandCount = 0;
                                                bool commandsWhole = bannersWhole && read(in, commandCount);
                                                if (commandsWhole)
                                                    for (uint32_t i = 0; i < commandCount && i < 1000000u; ++i)
                                                    {
                                                        LevelState::SavedCommand command;
                                                        if (!read(in, command.x) || !read(in, command.y) || !read(in, command.z) ||
                                                            !read(in, command.dimension) || !read(in, command.flags) ||
                                                            !readLine(in, command.command) || !readLine(in, command.output))
                                                        {
                                                            commandsWhole = false;
                                                            break;
                                                        }
                                                        out.commands.push_back(command);
                                                    }
                                                uint32_t structureCount = 0;
                                                bool structuresWhole = commandsWhole && read(in, structureCount);
                                                if (structuresWhole)
                                                    for (uint32_t i = 0; i < structureCount && i < 1000000u; ++i)
                                                    {
                                                        LevelState::SavedStructure structure;
                                                        bool whole = read(in, structure.x) && read(in, structure.y) && read(in, structure.z) &&
                                                                     read(in, structure.dimension) && readLine(in, structure.name);
                                                        for (int k = 0; k < 3 && whole; ++k)
                                                        {
                                                            int32_t value = 0;
                                                            whole = read(in, value);
                                                            structure.offset[k] = value;
                                                        }
                                                        for (int k = 0; k < 3 && whole; ++k)
                                                        {
                                                            int32_t value = 0;
                                                            whole = read(in, value);
                                                            structure.size[k] = value;
                                                        }
                                                        if (!whole)
                                                        {
                                                            structuresWhole = false;
                                                            break;
                                                        }
                                                        out.structures.push_back(structure);
                                                    }
                                                uint32_t nameCount = 0;
                                                bool namesWhole = structuresWhole && read(in, nameCount);
                                                if (namesWhole)
                                                    for (uint32_t i = 0; i < nameCount && i < 65536u; ++i)
                                                    {
                                                        std::string name;
                                                        if (!readLine(in, name)) { namesWhole = false; break; }
                                                        out.itemNames.push_back(name);
                                                    }
                                                uint32_t stoneCount = 0;
                                                bool stonesWhole = namesWhole && read(in, stoneCount);
                                                if (stonesWhole)
                                                    for (uint32_t i = 0; i < stoneCount && i < 65536u; ++i)
                                                    {
                                                        std::array<int, 4> stone{};
                                                        for (int k = 0; k < 4 && stonesWhole; ++k)
                                                        {
                                                            int32_t value = 0;
                                                            stonesWhole = read(in, value);
                                                            stone[static_cast<size_t>(k)] = value;
                                                        }
                                                        if (!stonesWhole) break;
                                                        out.lodestones.push_back(stone);
                                                    }
                                                uint8_t died = 0;
                                                const bool deathWhole = stonesWhole && read(in, died) && read(in, out.deathPoint[0]) &&
                                                    read(in, out.deathPoint[1]) && read(in, out.deathPoint[2]) && read(in, out.deathDimension);
                                                if (deathWhole) out.hasDeathPoint = died != 0;
                                                uint32_t bundleCount = 0;
                                                bool bundlesWhole = deathWhole && read(in, bundleCount);
                                                if (bundlesWhole)
                                                    for (uint32_t i = 0; i < bundleCount && i < 100000u; ++i)
                                                    {
                                                        uint16_t stacks = 0;
                                                        if (!read(in, stacks)) { bundlesWhole = false; break; }
                                                        std::vector<LevelState::SavedBundleStack> bundle;
                                                        bool whole = true;
                                                        for (uint16_t k = 0; k < stacks && whole; ++k)
                                                        {
                                                            LevelState::SavedBundleStack stack;
                                                            whole = read(in, stack.id) && read(in, stack.count) && read(in, stack.damage) &&
                                                                    read(in, stack.enchantments);
                                                            if (whole) bundle.push_back(stack);
                                                        }
                                                        if (!whole) { bundlesWhole = false; break; }
                                                        out.bundles.push_back(bundle);
                                                    }
                                                uint32_t bookCount = 0;
                                                bool booksWhole = bundlesWhole && read(in, bookCount);
                                                if (booksWhole)
                                                    for (uint32_t i = 0; i < bookCount && i < 60000u; ++i)
                                                    {
                                                        LevelState::SavedBook book;
                                                        uint16_t pages = 0;
                                                        if (!readLine(in, book.title) || !readLine(in, book.author) || !read(in, pages))
                                                        {
                                                            booksWhole = false;
                                                            break;
                                                        }
                                                        for (uint16_t k = 0; k < pages && booksWhole; ++k)
                                                        {
                                                            std::string page;
                                                            booksWhole = readText(in, page);
                                                            if (booksWhole) book.pages.push_back(page);
                                                        }
                                                        if (!booksWhole) break;
                                                        out.books.push_back(book);
                                                    }
                                                uint32_t lecternCount = 0;
                                                bool lecternsWhole = booksWhole && read(in, lecternCount);
                                                if (lecternsWhole)
                                                    for (uint32_t i = 0; i < lecternCount && i < 100000u; ++i)
                                                    {
                                                        LevelState::SavedLectern lectern;
                                                        if (!read(in, lectern.x) || !read(in, lectern.y) || !read(in, lectern.z) ||
                                                            !read(in, lectern.dimension) || !read(in, lectern.stack.id) ||
                                                            !read(in, lectern.stack.count) || !read(in, lectern.stack.damage) ||
                                                            !read(in, lectern.stack.enchantments))
                                                        {
                                                            lecternsWhole = false;
                                                            break;
                                                        }
                                                        out.lecterns.push_back(lectern);
                                                    }
                                                uint32_t vehicleCount = 0;
                                                bool vehiclesWhole = lecternsWhole && read(in, vehicleCount);
                                                if (vehiclesWhole)
                                                    for (uint32_t i = 0; i < vehicleCount && i < 100000u; ++i)
                                                    {
                                                        LevelState::SavedVehicle vehicle;
                                                        uint8_t slots = 0;
                                                        if (!read(in, vehicle.dimension) || !read(in, vehicle.type) || !read(in, vehicle.variant) ||
                                                            !read(in, vehicle.x) || !read(in, vehicle.y) || !read(in, vehicle.z) ||
                                                            !read(in, vehicle.yaw) || !read(in, vehicle.fuel) || !readLine(in, vehicle.command) ||
                                                            !read(in, slots))
                                                        {
                                                            vehiclesWhole = false;
                                                            break;
                                                        }
                                                        bool whole = true;
                                                        for (uint8_t k = 0; k < slots && whole; ++k)
                                                        {
                                                            std::pair<uint8_t, LevelState::SavedBundleStack> slot;
                                                            whole = read(in, slot.first) && read(in, slot.second.id) && read(in, slot.second.count) &&
                                                                    read(in, slot.second.damage) && read(in, slot.second.enchantments);
                                                            if (whole) vehicle.contents.push_back(slot);
                                                        }
                                                        if (!whole) { vehiclesWhole = false; break; }
                                                        out.vehicles.push_back(vehicle);
                                                    }
                                                uint32_t cageCount = 0;
                                                bool cagesWhole = vehiclesWhole && read(in, cageCount);
                                                if (cagesWhole)
                                                    for (uint32_t i = 0; i < cageCount && i < 100000u; ++i)
                                                    {
                                                        LevelState::SavedCage cage;
                                                        if (!read(in, cage.dimension) || !read(in, cage.x) || !read(in, cage.y) ||
                                                            !read(in, cage.z) || !read(in, cage.mob))
                                                        {
                                                            cagesWhole = false;
                                                            break;
                                                        }
                                                        out.cages.push_back(cage);
                                                    }
                                                uint32_t starCount = 0;
                                                bool starsWhole = cagesWhole && read(in, starCount);
                                                if (starsWhole)
                                                    for (uint32_t i = 0; i < starCount && i < 60000u && starsWhole; ++i)
                                                    {
                                                        LevelState::SavedExplosion star;
                                                        uint8_t colours = 0, fades = 0;
                                                        starsWhole = read(in, star.shape) && read(in, star.flags) && read(in, colours);
                                                        for (uint8_t k = 0; k < colours && starsWhole; ++k)
                                                        {
                                                            uint8_t c = 0;
                                                            starsWhole = read(in, c);
                                                            star.colours.push_back(c);
                                                        }
                                                        starsWhole = starsWhole && read(in, fades);
                                                        for (uint8_t k = 0; k < fades && starsWhole; ++k)
                                                        {
                                                            uint8_t c = 0;
                                                            starsWhole = read(in, c);
                                                            star.fades.push_back(c);
                                                        }
                                                        if (starsWhole) out.fireworkStars.push_back(star);
                                                    }
                                                uint32_t rocketCount = 0;
                                                bool rocketsWhole = starsWhole && read(in, rocketCount);
                                                if (rocketsWhole)
                                                    for (uint32_t i = 0; i < rocketCount && i < 60000u; ++i)
                                                    {
                                                        LevelState::SavedRocket rocket;
                                                        uint8_t stars = 0;
                                                        if (!read(in, rocket.flight) || !read(in, stars)) { rocketsWhole = false; break; }
                                                        bool whole = true;
                                                        for (uint8_t k = 0; k < stars && whole; ++k)
                                                        {
                                                            uint16_t id = 0;
                                                            whole = read(in, id);
                                                            rocket.explosions.push_back(id);
                                                        }
                                                        if (!whole) { rocketsWhole = false; break; }
                                                        out.fireworkRockets.push_back(rocket);
                                                    }
                                                uint8_t slotCount = 0;
                                                bool slotsWhole = rocketsWhole && read(in, slotCount);
                                                if (slotsWhole)
                                                    for (uint8_t k = 0; k < slotCount; ++k)
                                                    {
                                                        uint16_t id = 0;
                                                        if (!read(in, id)) { slotsWhole = false; break; }
                                                        if (k < 31) out.crossbowRockets.push_back(id);
                                                    }
                                                uint32_t jukeboxCount = 0;
                                                bool jukeboxesWhole = slotsWhole && read(in, jukeboxCount);
                                                if (jukeboxesWhole)
                                                    for (uint32_t i = 0; i < jukeboxCount && i < 100000u; ++i)
                                                    {
                                                        LevelState::SavedLectern box;
                                                        if (!read(in, box.x) || !read(in, box.y) || !read(in, box.z) ||
                                                            !read(in, box.dimension) || !read(in, box.stack.id))
                                                        {
                                                            jukeboxesWhole = false;
                                                            break;
                                                        }
                                                        box.stack.count = 1;
                                                        out.jukeboxes.push_back(box);
                                                    }
                                                uint32_t lookCount = 0;
                                                bool looksWhole = jukeboxesWhole && read(in, lookCount);
                                                if (looksWhole)
                                                    for (uint32_t i = 0; i < lookCount && i < 100000u; ++i)
                                                    {
                                                        LevelState::SavedSignLook look;
                                                        if (!read(in, look.x) || !read(in, look.y) || !read(in, look.z) ||
                                                            !read(in, look.dimension) || !read(in, look.colour) || !read(in, look.glow))
                                                        {
                                                            looksWhole = false;
                                                            break;
                                                        }
                                                        out.signLooks.push_back(look);
                                                    }
                                                uint32_t paintingCount = 0;
                                                bool paintingsWhole = looksWhole && read(in, paintingCount);
                                                if (paintingsWhole)
                                                    for (uint32_t i = 0; i < paintingCount && i < 100000u; ++i)
                                                    {
                                                        LevelState::SavedPainting painting;
                                                        if (!read(in, painting.x) || !read(in, painting.y) || !read(in, painting.z) ||
                                                            !read(in, painting.dimension) || !read(in, painting.facing) ||
                                                            !readLine(in, painting.variant))
                                                        {
                                                            paintingsWhole = false;
                                                            break;
                                                        }
                                                        out.paintings.push_back(painting);
                                                    }
                                                uint32_t standCount = 0;
                                                if (paintingsWhole && read(in, standCount))
                                                    for (uint32_t i = 0; i < standCount && i < 100000u; ++i)
                                                    {
                                                        LevelState::SavedStand stand;
                                                        bool whole = read(in, stand.x) && read(in, stand.y) && read(in, stand.z) &&
                                                                     read(in, stand.yaw) && read(in, stand.dimension) && read(in, stand.pose);
                                                        for (LevelState::SavedGear& gear : stand.gear)
                                                            whole = whole && read(in, gear.id) && read(in, gear.count) &&
                                                                    read(in, gear.damage) && read(in, gear.enchantments);
                                                        if (!whole) break;
                                                        out.stands.push_back(stand);
                                                    }
                                                uint32_t extraCount = 0;
                                                if (paintingsWhole && read(in, extraCount) && extraCount == out.structures.size())
                                                    for (LevelState::SavedStructure& structure : out.structures)
                                                    {
                                                        if (!(read(in, structure.rotation) && read(in, structure.mirror) &&
                                                              read(in, structure.integrity) && read(in, structure.seed)))
                                                            break;
                                                        structure.rotation = static_cast<int8_t>(structure.rotation & 3);
                                                        structure.mirror = static_cast<int8_t>(std::clamp<int>(structure.mirror, 0, 2));
                                                        structure.integrity = std::clamp(structure.integrity, 0.0f, 1.0f);
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
        }
    }
    if (out.itemBase != ITEM_BASE)
    {
        const uint16_t from = out.itemBase;
        auto move = [from](uint16_t& id) {
            if (id >= from) id = static_cast<uint16_t>(id - from + ITEM_BASE);
        };
        for (auto& slot : out.inventory) move(slot.first);
        for (LevelState::SavedFrame& frame : out.frames) move(frame.item);
        for (LevelState::SavedFurnace& furnace : out.furnaces)
        {
            move(furnace.input);
            move(furnace.fuel);
            move(furnace.output);
        }
        for (LevelState::SavedChest& chest : out.chests)
            for (auto& slot : chest.slots) move(slot.first);
        for (LevelState::SavedStand& stand : out.stands)
            for (LevelState::SavedGear& gear : stand.gear) move(gear.id);
        out.itemBase = ITEM_BASE;
    }
    return true;
}
