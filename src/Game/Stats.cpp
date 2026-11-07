#include "Stats.h"
#include <cstdio>
#include <sstream>

namespace
{
    std::string twoPlaces(double value)
    {
        char text[48];
        std::snprintf(text, sizeof(text), "%.2f", value);
        return text;
    }
}

std::string Stats::grouped(long long value)
{
    const bool negative = value < 0;
    std::string digits = std::to_string(negative ? -value : value);
    std::string out;
    for (size_t i = 0; i < digits.size(); ++i)
    {
        if (i > 0 && (digits.size() - i) % 3 == 0) out.push_back(',');
        out.push_back(digits[i]);
    }
    return negative ? "-" + out : out;
}

std::string Stats::formatted(Format format, long long value)
{
    switch (format)
    {
        case Format::Tenths: return twoPlaces(static_cast<double>(value) * 0.1);
        case Format::Distance:
        {
            const double metres = static_cast<double>(value) / 100.0;
            const double kilometres = metres / 1000.0;
            if (kilometres > 0.5) return twoPlaces(kilometres) + " km";
            if (metres > 0.5) return twoPlaces(metres) + " m";
            return std::to_string(value) + " cm";
        }
        case Format::Time:
        {
            const double seconds = static_cast<double>(value) / 20.0;
            const double minutes = seconds / 60.0;
            const double hours = minutes / 60.0;
            const double days = hours / 24.0;
            const double years = days / 365.0;
            if (years > 0.5) return twoPlaces(years) + " y";
            if (days > 0.5) return twoPlaces(days) + " d";
            if (hours > 0.5) return twoPlaces(hours) + " h";
            if (minutes > 0.5) return twoPlaces(minutes) + " min";
            return twoPlaces(seconds) + " s";
        }
        default: return grouped(value);
    }
}

Stats::Book::Book() { clear(); }

void Stats::Book::clear()
{
    m_custom.fill(0);
    m_items.clear();
    m_mobs.clear();
}

void Stats::Book::add(Custom stat, long long amount) { m_custom[static_cast<size_t>(stat)] += amount; }
void Stats::Book::set(Custom stat, long long value) { m_custom[static_cast<size_t>(stat)] = value; }
long long Stats::Book::get(Custom stat) const { return m_custom[static_cast<size_t>(stat)]; }

void Stats::Book::item(ItemStat stat, uint16_t id, long long amount)
{
    if (amount <= 0) return;
    auto found = m_items.find(id);
    if (found == m_items.end()) found = m_items.emplace(id, std::array<long long, ITEM_STATS>{}).first;
    found->second[static_cast<size_t>(stat)] += amount;
}

long long Stats::Book::count(ItemStat stat, uint16_t id) const
{
    const auto found = m_items.find(id);
    return found == m_items.end() ? 0 : found->second[static_cast<size_t>(stat)];
}

void Stats::Book::killed(int mob, long long amount) { m_mobs[mob][0] += amount; }
void Stats::Book::killedBy(int mob, long long amount) { m_mobs[mob][1] += amount; }

long long Stats::Book::kills(int mob) const
{
    const auto found = m_mobs.find(mob);
    return found == m_mobs.end() ? 0 : found->second[0];
}

long long Stats::Book::deathsBy(int mob) const
{
    const auto found = m_mobs.find(mob);
    return found == m_mobs.end() ? 0 : found->second[1];
}

std::vector<std::string> Stats::Book::lines() const
{
    std::vector<std::string> out;
    for (int i = 0; i < CUSTOM_COUNT; ++i)
        if (m_custom[static_cast<size_t>(i)] != 0)
            out.push_back(std::string("custom ") + id(static_cast<Custom>(i)) + " " + std::to_string(m_custom[static_cast<size_t>(i)]));
    for (const auto& [item, counts] : m_items)
    {
        std::string line = "item " + std::to_string(item);
        for (long long count : counts) line += " " + std::to_string(count);
        out.push_back(line);
    }
    for (const auto& [mob, counts] : m_mobs)
        out.push_back("mob " + std::to_string(mob) + " " + std::to_string(counts[0]) + " " + std::to_string(counts[1]));
    return out;
}

void Stats::Book::load(const std::vector<std::string>& lines)
{
    clear();
    for (const std::string& line : lines)
    {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "custom")
        {
            std::string name;
            long long value = 0;
            if (!(in >> name >> value)) continue;
            for (int i = 0; i < CUSTOM_COUNT; ++i)
                if (name == id(static_cast<Custom>(i))) m_custom[static_cast<size_t>(i)] = value;
        }
        else if (kind == "item")
        {
            int item = 0;
            if (!(in >> item)) continue;
            std::array<long long, ITEM_STATS> counts{};
            for (long long& count : counts) in >> count;
            m_items[static_cast<uint16_t>(item)] = counts;
        }
        else if (kind == "mob")
        {
            int mob = 0;
            long long a = 0, b = 0;
            if (in >> mob >> a >> b) m_mobs[mob] = { a, b };
        }
    }
}
