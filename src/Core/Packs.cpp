#include "Packs.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <stb_image.h>

namespace
{
    std::vector<std::string>& roots()
    {
        static std::vector<std::string> active;
        return active;
    }

    std::string readAll(const std::string& path)
    {
        std::string out;
        if (std::FILE* in = std::fopen(path.c_str(), "rb"))
        {
            char buffer[4096];
            size_t got = 0;
            while ((got = std::fread(buffer, 1, sizeof(buffer), in)) > 0) out.append(buffer, got);
            std::fclose(in);
        }
        return out;
    }

    size_t valueAfter(const std::string& json, const std::string& key)
    {
        const size_t at = json.find("\"" + key + "\"");
        if (at == std::string::npos) return std::string::npos;
        const size_t colon = json.find(':', at + key.size() + 2);
        if (colon == std::string::npos) return std::string::npos;
        size_t i = colon + 1;
        while (i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
        return i;
    }

    std::string stringAt(const std::string& json, size_t i)
    {
        std::string out;
        if (i >= json.size() || json[i] != '"') return out;
        for (++i; i < json.size() && json[i] != '"'; ++i)
        {
            if (json[i] == '\\' && i + 1 < json.size())
            {
                ++i;
                if (json[i] == 'n') out.push_back('\n');
                else out.push_back(json[i]);
            }
            else
                out.push_back(json[i]);
        }
        return out;
    }

    std::string stripFormatting(const std::string& text)
    {
        std::string out;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (static_cast<unsigned char>(text[i]) == 0xC2 && i + 1 < text.size() && static_cast<unsigned char>(text[i + 1]) == 0xA7)
            {
                i += 2;
                continue;
            }
            out.push_back(text[i]);
        }
        return out;
    }
}

std::string Packs::directory(const std::string& configPath)
{
    return configPath + "/resourcepacks";
}

std::string Packs::descriptionFrom(const std::string& json)
{
    const size_t i = valueAfter(json, "description");
    if (i == std::string::npos) return std::string();
    if (i < json.size() && json[i] == '"') return stripFormatting(stringAt(json, i));
    std::string out;
    size_t at = i;
    while (true)
    {
        const size_t text = json.find("\"text\"", at);
        if (text == std::string::npos) break;
        const size_t value = valueAfter(json.substr(text), "text");
        if (value == std::string::npos) break;
        out += stringAt(json, text + value);
        at = text + value + 1;
    }
    if (out.empty())
    {
        size_t quote = json.find('"', i);
        if (quote != std::string::npos) out = stringAt(json, quote);
    }
    return stripFormatting(out);
}

int Packs::formatFrom(const std::string& json)
{
    const size_t i = valueAfter(json, "pack_format");
    if (i == std::string::npos) return 0;
    return std::atoi(json.c_str() + i);
}

std::vector<Packs::Info> Packs::discover(const std::string& configPath)
{
    std::vector<Info> out;
    std::error_code ec;
    const std::string folder = directory(configPath);
    std::filesystem::create_directories(folder, ec);
    for (const auto& item : std::filesystem::directory_iterator(folder, ec))
    {
        if (!item.is_directory(ec)) continue;
        const std::string root = item.path().string();
        if (!std::filesystem::exists(root + "/pack.mcmeta", ec)) continue;
        const std::string json = readAll(root + "/pack.mcmeta");
        Info info;
        info.id = "file/" + item.path().filename().string();
        info.name = item.path().filename().string();
        info.description = descriptionFrom(json);
        info.format = formatFrom(json);
        info.root = root;
        info.icon = std::filesystem::exists(root + "/pack.png", ec);
        out.push_back(info);
    }
    std::sort(out.begin(), out.end(), [](const Info& a, const Info& b) { return a.name < b.name; });
    return out;
}

std::vector<std::string> Packs::selectionFrom(const std::string& line)
{
    std::vector<std::string> out;
    std::stringstream in(line);
    std::string part;
    while (std::getline(in, part, ','))
        if (!part.empty()) out.push_back(part);
    return out;
}

std::string Packs::selectionLine(const std::vector<std::string>& ids)
{
    std::string out;
    for (const std::string& id : ids)
    {
        if (!out.empty()) out += ",";
        out += id;
    }
    return out;
}

std::vector<std::string> Packs::savedSelection(const std::string& configPath)
{
    const std::string options = readAll(configPath + "/options.txt");
    const size_t at = options.find("resourcePacks=");
    if (at == std::string::npos) return {};
    const size_t end = options.find('\n', at);
    return selectionFrom(options.substr(at + 14, end == std::string::npos ? std::string::npos : end - at - 14));
}

void Packs::setActive(const std::vector<std::string>& list)
{
    roots() = list;
}

const std::vector<std::string>& Packs::active()
{
    return roots();
}

void Packs::activateFromOptions(const std::string& configPath)
{
    const std::vector<Info> found = discover(configPath);
    std::vector<std::string> chosen;
    for (const std::string& id : savedSelection(configPath))
        for (const Info& info : found)
            if (info.id == id) chosen.push_back(info.root);
    setActive(chosen);
}

std::string Packs::file(const std::string& path)
{
    static const std::string PREFIX = "assets/textures/";
    if (roots().empty() || path.compare(0, PREFIX.size(), PREFIX) != 0) return path;
    const std::string rest = path.substr(PREFIX.size());
    std::error_code ec;
    for (const std::string& root : roots())
    {
        const std::string candidate = root + "/assets/minecraft/textures/" + rest;
        if (std::filesystem::exists(candidate, ec)) return candidate;
    }
    return path;
}

unsigned char* Packs::load(const char* path, int* width, int* height, int* channels, int wanted)
{
    return stbi_load(file(path).c_str(), width, height, channels, wanted);
}
