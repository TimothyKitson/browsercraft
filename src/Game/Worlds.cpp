#include "Worlds.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <sstream>

namespace
{
    const char* const TYPE_NAMES[Worlds::TYPE_COUNT] = { "Default", "Superflat", "Large Biomes", "AMPLIFIED", "Single Biome" };
    const char* const TYPE_IDS[Worlds::TYPE_COUNT] = { "default", "flat", "large_biomes", "amplified", "single_biome_surface" };

    std::string trimmed(const std::string& text)
    {
        size_t start = 0, end = text.size();
        while (start < end && std::isspace(static_cast<unsigned char>(text[start]))) ++start;
        while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
        return text.substr(start, end - start);
    }

    std::string lowered(std::string text)
    {
        for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    }

    bool reserved(const std::string& name)
    {
        if (!name.empty() && name.back() == '.') return true;
        std::string upper = name;
        for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        const size_t dot = upper.find('.');
        const std::string stem = dot == std::string::npos ? upper : upper.substr(0, dot);
        static const char* const RESERVED[] = { "CON", "PRN", "AUX", "NUL", "CLOCK$", "COM", "COM1", "COM2", "COM3", "COM4", "COM5",
                                                "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6",
                                                "LPT7", "LPT8", "LPT9" };
        for (const char* word : RESERVED)
            if (stem == word) return true;
        return false;
    }
}

const char* Worlds::typeName(Type type) { return TYPE_NAMES[static_cast<int>(type) % TYPE_COUNT]; }
const char* Worlds::typeId(Type type) { return TYPE_IDS[static_cast<int>(type) % TYPE_COUNT]; }

Worlds::Type Worlds::typeFromId(const std::string& id)
{
    for (int i = 0; i < TYPE_COUNT; ++i)
        if (id == TYPE_IDS[i]) return static_cast<Type>(i);
    return Type::Default;
}

const char* Worlds::difficultyName(Difficulty difficulty)
{
    switch (difficulty)
    {
        case Difficulty::Peaceful: return "Peaceful";
        case Difficulty::Easy: return "Easy";
        case Difficulty::Hard: return "Hard";
        default: return "Normal";
    }
}

const char* Worlds::difficultyInfo(Difficulty difficulty)
{
    switch (difficulty)
    {
        case Difficulty::Peaceful: return "No hostile mobs and only some neutral mobs spawn. Hunger bar doesn't deplete and health replenishes over time.";
        case Difficulty::Easy: return "Hostile mobs spawn but deal less damage. Hunger bar depletes and drains health down to 5 hearts.";
        case Difficulty::Hard: return "Hostile mobs spawn and deal more damage. Hunger bar depletes and drains all health.";
        default: return "Hostile mobs spawn and deal standard damage. Hunger bar depletes and drains health down to half a heart.";
    }
}

std::vector<std::string> Worlds::metaLines(const Meta& meta)
{
    return { "name=" + meta.name,
             "lastPlayed=" + std::to_string(meta.lastPlayed),
             "mode=" + std::to_string(meta.mode),
             "cheats=" + std::string(meta.cheats ? "1" : "0"),
             "type=" + std::string(typeId(meta.type)),
             "structures=" + std::string(meta.structures ? "1" : "0"),
             "bonusChest=" + std::string(meta.bonusChest ? "1" : "0"),
             "difficulty=" + std::to_string(static_cast<int>(meta.difficulty)),
             "layers=" + meta.layers,
             "biome=" + meta.biome };
}

Worlds::Meta Worlds::parseMeta(const std::vector<std::string>& lines)
{
    Meta meta;
    for (const std::string& line : lines)
    {
        const size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        const std::string key = line.substr(0, equals);
        const std::string value = line.substr(equals + 1);
        if (key == "name") meta.name = value;
        else if (key == "lastPlayed") meta.lastPlayed = std::atoll(value.c_str());
        else if (key == "mode") meta.mode = std::atoi(value.c_str());
        else if (key == "cheats") meta.cheats = value == "1";
        else if (key == "type") meta.type = typeFromId(value);
        else if (key == "structures") meta.structures = value != "0";
        else if (key == "bonusChest") meta.bonusChest = value == "1";
        else if (key == "difficulty") meta.difficulty = static_cast<Difficulty>(std::clamp(std::atoi(value.c_str()), 0, 3));
        else if (key == "layers") meta.layers = value;
        else if (key == "biome") meta.biome = value;
    }
    return meta;
}

bool Worlds::readMeta(const std::string& folder, Meta& out)
{
    std::FILE* file = std::fopen((folder + "/world.txt").c_str(), "rb");
    if (!file) return false;
    std::vector<std::string> lines;
    char line[512];
    while (std::fgets(line, sizeof(line), file))
    {
        std::string text(line);
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
        lines.push_back(text);
    }
    std::fclose(file);
    out = parseMeta(lines);
    return true;
}

bool Worlds::writeMeta(const std::string& folder, const Meta& meta)
{
    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    std::FILE* file = std::fopen((folder + "/world.txt").c_str(), "wb");
    if (!file) return false;
    for (const std::string& line : metaLines(meta)) std::fprintf(file, "%s\n", line.c_str());
    std::fclose(file);
    return true;
}

std::string Worlds::sanitize(const std::string& name)
{
    std::string out = name;
    for (char& c : out)
        if (c == '/' || c == '\n' || c == '\r' || c == '\t' || c == '\0' || c == '\f' || c == '`' || c == '?' || c == '*' ||
            c == '\\' || c == '<' || c == '>' || c == '|' || c == '"' || c == ':' || c == '.')
            c = '_';
    if (reserved(out)) out = "_" + out + "_";
    return out;
}

std::string Worlds::uniqueFolder(const std::string& savesDirectory, const std::string& name)
{
    std::string base = sanitize(trimmed(name).empty() ? std::string("New World") : trimmed(name));
    int count = 0;
    const size_t open = base.rfind(" (");
    if (open != std::string::npos && base.size() > open + 3 && base.back() == ')')
    {
        const std::string digits = base.substr(open + 2, base.size() - open - 3);
        if (!digits.empty() && std::all_of(digits.begin(), digits.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }))
        {
            count = std::atoi(digits.c_str());
            base = base.substr(0, open);
        }
    }
    if (base.size() > 255) base = base.substr(0, 255);
    std::error_code ec;
    while (true)
    {
        std::string attempt = base;
        if (count != 0)
        {
            const std::string suffix = " (" + std::to_string(count) + ")";
            if (attempt.size() > 255 - suffix.size()) attempt = attempt.substr(0, 255 - suffix.size());
            attempt += suffix;
        }
        if (!std::filesystem::exists(savesDirectory + "/" + attempt, ec)) return attempt;
        ++count;
    }
}

std::vector<Worlds::Entry> Worlds::list(const std::string& savesDirectory)
{
    std::vector<Entry> out;
    std::error_code ec;
    if (!std::filesystem::is_directory(savesDirectory, ec)) return out;
    for (const auto& item : std::filesystem::directory_iterator(savesDirectory, ec))
    {
        if (!item.is_directory(ec)) continue;
        const std::string path = item.path().string();
        if (!std::filesystem::exists(path + "/level.dat", ec) && !std::filesystem::exists(path + "/world.txt", ec)) continue;
        Entry entry;
        entry.folder = item.path().filename().string();
        entry.path = path;
        if (!readMeta(path, entry.meta)) entry.meta.name = entry.folder;
        entry.icon = std::filesystem::exists(path + "/icon.png", ec);
        out.push_back(entry);
    }
    std::sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) {
        if (a.meta.lastPlayed != b.meta.lastPlayed) return a.meta.lastPlayed > b.meta.lastPlayed;
        return a.folder < b.folder;
    });
    return out;
}

bool Worlds::matches(const Entry& entry, const std::string& search)
{
    const std::string wanted = lowered(search);
    return lowered(entry.meta.name).find(wanted) != std::string::npos || lowered(entry.folder).find(wanted) != std::string::npos;
}

long long Worlds::nowMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string Worlds::formatDate(long long millis)
{
    const std::time_t seconds = static_cast<std::time_t>(millis / 1000);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    const int hour = local.tm_hour % 12 == 0 ? 12 : local.tm_hour % 12;
    char text[48];
    std::snprintf(text, sizeof(text), "%d/%d/%02d, %d:%02d %s", local.tm_mon + 1, local.tm_mday, local.tm_year % 100, hour,
                  local.tm_min, local.tm_hour < 12 ? "AM" : "PM");
    return text;
}

std::vector<Worlds::Layer> Worlds::parseLayers(const std::string& text)
{
    std::vector<Layer> out;
    std::stringstream in(text);
    std::string part;
    while (std::getline(in, part, ','))
    {
        part = trimmed(part);
        if (part.empty()) continue;
        Layer layer;
        const size_t star = part.find('*');
        if (star != std::string::npos)
        {
            layer.count = std::max(0, std::atoi(part.substr(0, star).c_str()));
            layer.block = part.substr(star + 1);
        }
        else
            layer.block = part;
        if (layer.block.find(':') == std::string::npos) layer.block = "minecraft:" + layer.block;
        if (layer.count > 0) out.push_back(layer);
    }
    return out;
}

std::string Worlds::formatLayers(const std::vector<Layer>& layers)
{
    std::string out;
    for (const Layer& layer : layers)
    {
        if (!out.empty()) out += ",";
        if (layer.count > 1) out += std::to_string(layer.count) + "*";
        out += layer.block;
    }
    return out;
}

const std::vector<Worlds::FlatPreset>& Worlds::flatPresets()
{
    static const std::vector<FlatPreset> PRESETS = {
        { "Classic Flat", "grass_block", "minecraft:bedrock,2*minecraft:dirt,minecraft:grass_block", "minecraft:plains" },
        { "Tunnelers' Dream", "stone", "minecraft:bedrock,230*minecraft:stone,5*minecraft:dirt,minecraft:grass_block", "minecraft:windswept_hills" },
        { "Water World", "water_bucket", "minecraft:bedrock,64*minecraft:deepslate,5*minecraft:stone,5*minecraft:dirt,5*minecraft:gravel,90*minecraft:water", "minecraft:deep_ocean" },
        { "Overworld", "short_grass", "minecraft:bedrock,59*minecraft:stone,3*minecraft:dirt,minecraft:grass_block", "minecraft:plains" },
        { "Snowy Kingdom", "snow", "minecraft:bedrock,59*minecraft:stone,3*minecraft:dirt,minecraft:grass_block,minecraft:snow", "minecraft:snowy_plains" },
        { "Bottomless Pit", "feather", "2*minecraft:cobblestone,3*minecraft:dirt,minecraft:grass_block", "minecraft:plains" },
        { "Desert", "sand", "minecraft:bedrock,3*minecraft:stone,52*minecraft:sandstone,8*minecraft:sand", "minecraft:desert" },
        { "Redstone Ready", "redstone", "minecraft:bedrock,3*minecraft:stone,116*minecraft:sandstone", "minecraft:desert" },
        { "The Void", "barrier", "minecraft:air", "minecraft:the_void" },
    };
    return PRESETS;
}

std::string Worlds::presetCode(const std::string& layers, const std::string& biome)
{
    return layers + ";" + biome;
}

bool Worlds::parsePresetCode(const std::string& code, std::string& layers, std::string& biome)
{
    const size_t semicolon = code.find(';');
    const std::string layerPart = trimmed(semicolon == std::string::npos ? code : code.substr(0, semicolon));
    std::string biomePart = semicolon == std::string::npos ? std::string("minecraft:plains") : trimmed(code.substr(semicolon + 1));
    if (biomePart.empty()) biomePart = "minecraft:plains";
    if (biomePart.find(':') == std::string::npos) biomePart = "minecraft:" + biomePart;
    const std::vector<Layer> parsed = parseLayers(layerPart);
    if (parsed.empty()) return false;
    layers = formatLayers(parsed);
    biome = biomePart;
    return true;
}

int Worlds::javaHash(const std::string& text)
{
    uint32_t hash = 0;
    for (unsigned char c : text) hash = hash * 31u + c;
    return static_cast<int>(hash);
}

uint32_t Worlds::seedFrom(const std::string& text, uint32_t fallback)
{
    const std::string seed = trimmed(text);
    if (seed.empty()) return fallback;
    bool numeric = true;
    for (size_t i = 0; i < seed.size(); ++i)
        numeric = numeric && (std::isdigit(static_cast<unsigned char>(seed[i])) || (i == 0 && seed[i] == '-' && seed.size() > 1));
    if (numeric && seed.size() <= 19)
    {
        const long long value = std::strtoll(seed.c_str(), nullptr, 10);
        const uint64_t bits = static_cast<uint64_t>(value);
        return static_cast<uint32_t>(bits ^ (bits >> 32));
    }
    return static_cast<uint32_t>(javaHash(seed));
}
