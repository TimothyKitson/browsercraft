#include "Servers.h"
#include <cstdio>
#include <cstdlib>
#include <utility>

const char* Servers::packsName(Packs packs)
{
    switch (packs)
    {
        case Packs::Enabled: return "Enabled";
        case Packs::Disabled: return "Disabled";
        default: return "Prompt";
    }
}

std::vector<std::string> Servers::lines(const std::vector<Entry>& servers)
{
    std::vector<std::string> out;
    for (const Entry& entry : servers)
        out.push_back(entry.name + "\t" + entry.address + "\t" + std::to_string(static_cast<int>(entry.packs)));
    return out;
}

std::vector<Servers::Entry> Servers::parse(const std::vector<std::string>& lines)
{
    std::vector<Entry> out;
    for (const std::string& line : lines)
    {
        const size_t first = line.find('\t');
        if (first == std::string::npos) continue;
        const size_t second = line.find('\t', first + 1);
        Entry entry;
        entry.name = line.substr(0, first);
        entry.address = line.substr(first + 1, second == std::string::npos ? std::string::npos : second - first - 1);
        if (second != std::string::npos)
        {
            const int packs = std::atoi(line.c_str() + second + 1);
            if (packs >= 0 && packs <= 2) entry.packs = static_cast<Packs>(packs);
        }
        out.push_back(entry);
    }
    return out;
}

std::vector<Servers::Entry> Servers::load(const std::string& file)
{
    std::vector<std::string> read;
    if (std::FILE* in = std::fopen(file.c_str(), "rb"))
    {
        char line[1024];
        while (std::fgets(line, sizeof(line), in))
        {
            std::string text(line);
            while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
            if (!text.empty()) read.push_back(text);
        }
        std::fclose(in);
    }
    return parse(read);
}

bool Servers::save(const std::string& file, const std::vector<Entry>& servers)
{
    std::FILE* out = std::fopen(file.c_str(), "wb");
    if (!out) return false;
    for (const std::string& line : lines(servers)) std::fprintf(out, "%s\n", line.c_str());
    std::fclose(out);
    return true;
}

void Servers::move(std::vector<Entry>& servers, int from, int to)
{
    if (from < 0 || to < 0 || from >= static_cast<int>(servers.size()) || to >= static_cast<int>(servers.size())) return;
    std::swap(servers[static_cast<size_t>(from)], servers[static_cast<size_t>(to)]);
}
