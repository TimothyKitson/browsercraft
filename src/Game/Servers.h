#pragma once
#include <string>
#include <vector>

namespace Servers
{
    enum class Packs : int { Enabled, Disabled, Prompt };
    const char* packsName(Packs packs);

    struct Entry
    {
        std::string name = "Minecraft Server";
        std::string address;
        Packs packs = Packs::Prompt;
    };

    std::vector<std::string> lines(const std::vector<Entry>& servers);
    std::vector<Entry> parse(const std::vector<std::string>& lines);
    std::vector<Entry> load(const std::string& file);
    bool save(const std::string& file, const std::vector<Entry>& servers);
    void move(std::vector<Entry>& servers, int from, int to);
}
