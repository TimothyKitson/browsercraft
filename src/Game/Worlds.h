#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Worlds
{
    enum class Type : uint8_t { Default, Flat, LargeBiomes, Amplified, SingleBiome, Count };
    constexpr int TYPE_COUNT = static_cast<int>(Type::Count);
    const char* typeName(Type type);
    const char* typeId(Type type);
    Type typeFromId(const std::string& id);

    enum class Difficulty : uint8_t { Peaceful, Easy, Normal, Hard, Count };
    const char* difficultyName(Difficulty difficulty);
    const char* difficultyInfo(Difficulty difficulty);

    struct Meta
    {
        std::string name = "New World";
        long long lastPlayed = 0;
        int mode = 0;
        bool cheats = false;
        Type type = Type::Default;
        bool structures = true;
        bool bonusChest = false;
        Difficulty difficulty = Difficulty::Normal;
        std::string layers = "minecraft:bedrock,2*minecraft:dirt,minecraft:grass_block";
        std::string biome = "minecraft:plains";
    };

    std::vector<std::string> metaLines(const Meta& meta);
    Meta parseMeta(const std::vector<std::string>& lines);
    bool readMeta(const std::string& folder, Meta& out);
    bool writeMeta(const std::string& folder, const Meta& meta);

    std::string sanitize(const std::string& name);
    std::string uniqueFolder(const std::string& savesDirectory, const std::string& name);

    struct Entry
    {
        std::string folder;
        std::string path;
        Meta meta;
        bool icon = false;
    };
    std::vector<Entry> list(const std::string& savesDirectory);
    bool matches(const Entry& entry, const std::string& search);

    long long nowMillis();
    std::string formatDate(long long millis);

    struct Layer
    {
        std::string block;
        int count = 1;
    };
    std::vector<Layer> parseLayers(const std::string& text);
    std::string formatLayers(const std::vector<Layer>& layers);

    struct FlatPreset
    {
        const char* name;
        const char* icon;
        const char* layers;
        const char* biome;
    };
    const std::vector<FlatPreset>& flatPresets();
    std::string presetCode(const std::string& layers, const std::string& biome);
    bool parsePresetCode(const std::string& code, std::string& layers, std::string& biome);

    int javaHash(const std::string& text);
    uint32_t seedFrom(const std::string& text, uint32_t fallback);
}
