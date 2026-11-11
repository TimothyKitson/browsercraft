#include "Advancements.h"
#include <algorithm>
#include <map>
#include <sstream>

int Advancements::indexOf(const std::string& id)
{
    static const std::map<std::string, int> INDEX = [] {
        std::map<std::string, int> out;
        const std::vector<Def>& defs = all();
        for (size_t i = 0; i < defs.size(); ++i) out[defs[i].id] = static_cast<int>(i);
        return out;
    }();
    const auto found = INDEX.find(id);
    return found == INDEX.end() ? -1 : found->second;
}

int Advancements::parentOf(int index)
{
    const std::vector<Def>& defs = all();
    if (index < 0 || index >= static_cast<int>(defs.size())) return -1;
    return indexOf(defs[static_cast<size_t>(index)].parent);
}

int Advancements::rootOf(Tab tab)
{
    const std::vector<Def>& defs = all();
    for (size_t i = 0; i < defs.size(); ++i)
        if (defs[i].tab == tab && defs[i].parent[0] == '\0') return static_cast<int>(i);
    return -1;
}

const char* Advancements::tabBackground(Tab tab)
{
    switch (tab)
    {
        case Tab::Nether: return "advancements/backgrounds/nether";
        case Tab::End: return "advancements/backgrounds/end";
        case Tab::Adventure: return "advancements/backgrounds/adventure";
        case Tab::Husbandry: return "advancements/backgrounds/husbandry";
        default: return "advancements/backgrounds/stone";
    }
}

const char* Advancements::frameText(Frame frame)
{
    switch (frame)
    {
        case Frame::Goal: return "Goal Reached!";
        case Frame::Challenge: return "Challenge Complete!";
        default: return "Advancement Made!";
    }
}

unsigned int Advancements::frameColour(Frame frame)
{
    return frame == Frame::Challenge ? 0xAA00AAu : 0x55FF55u;
}

Advancements::Tracker::Tracker() : m_done(all().size()) {}

bool Advancements::Tracker::complete(int index) const
{
    const Def& def = all()[static_cast<size_t>(index)];
    const std::set<std::string>& got = m_done[static_cast<size_t>(index)];
    if (def.criteria.empty()) return false;
    if (def.any) return !got.empty();
    for (const Criterion& criterion : def.criteria)
        if (!got.count(criterion.name)) return false;
    return true;
}

bool Advancements::Tracker::grant(const std::string& id, const std::string& criterion)
{
    const int index = indexOf(id);
    if (index < 0) return false;
    const Def& def = all()[static_cast<size_t>(index)];
    bool known = false;
    for (const Criterion& c : def.criteria) known = known || criterion == c.name;
    if (!known) return false;
    const bool before = complete(index);
    m_done[static_cast<size_t>(index)].insert(criterion);
    const bool after = complete(index);
    if (!before && after)
    {
        m_new.push_back(index);
        return true;
    }
    return false;
}

bool Advancements::Tracker::revoke(const std::string& id, const std::string& criterion)
{
    const int index = indexOf(id);
    if (index < 0) return false;
    std::set<std::string>& done = m_done[static_cast<size_t>(index)];
    if (criterion.empty())
    {
        const bool had = !done.empty();
        done.clear();
        return had;
    }
    return done.erase(criterion) > 0;
}

bool Advancements::Tracker::grantAll(const std::string& id)
{
    const int index = indexOf(id);
    if (index < 0) return false;
    const Def& def = all()[static_cast<size_t>(index)];
    bool finished = false;
    for (const Criterion& c : def.criteria) finished = grant(id, c.name) || finished;
    return finished;
}

bool Advancements::Tracker::done(int index) const
{
    return index >= 0 && index < static_cast<int>(m_done.size()) && complete(index);
}

bool Advancements::Tracker::has(int index, const std::string& criterion) const
{
    return index >= 0 && index < static_cast<int>(m_done.size()) && m_done[static_cast<size_t>(index)].count(criterion) > 0;
}

int Advancements::Tracker::criteriaDone(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_done.size())) return 0;
    const Def& def = all()[static_cast<size_t>(index)];
    if (def.any) return complete(index) ? 1 : 0;
    return static_cast<int>(m_done[static_cast<size_t>(index)].size());
}

int Advancements::Tracker::criteriaNeeded(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_done.size())) return 0;
    const Def& def = all()[static_cast<size_t>(index)];
    return def.any ? 1 : static_cast<int>(def.criteria.size());
}

bool Advancements::Tracker::reachable(int index) const
{
    if (done(index)) return true;
    const std::vector<Def>& defs = all();
    for (size_t i = 0; i < defs.size(); ++i)
        if (indexOf(defs[i].parent) == index && reachable(static_cast<int>(i))) return true;
    return false;
}

bool Advancements::Tracker::visible(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_done.size())) return false;
    if (reachable(index)) return true;
    int at = index;
    for (int step = 0; step <= 2 && at >= 0; ++step)
    {
        if (done(at)) return true;
        if (all()[static_cast<size_t>(at)].hidden) return false;
        at = parentOf(at);
    }
    return false;
}

bool Advancements::Tracker::tabVisible(Tab tab) const
{
    return visible(rootOf(tab));
}

void Advancements::Tracker::reset()
{
    for (std::set<std::string>& got : m_done) got.clear();
    m_new.clear();
}

std::vector<std::string> Advancements::Tracker::lines() const
{
    std::vector<std::string> out;
    const std::vector<Def>& defs = all();
    for (size_t i = 0; i < defs.size(); ++i)
        for (const std::string& criterion : m_done[i]) out.push_back(std::string(defs[i].id) + " " + criterion);
    return out;
}

void Advancements::Tracker::load(const std::vector<std::string>& lines)
{
    reset();
    for (const std::string& line : lines)
    {
        std::istringstream in(line);
        std::string id, criterion;
        if (!(in >> id >> criterion)) continue;
        const int index = indexOf(id);
        if (index >= 0) m_done[static_cast<size_t>(index)].insert(criterion);
    }
    m_new.clear();
}

std::vector<int> Advancements::Tracker::takeNew()
{
    std::vector<int> out;
    out.swap(m_new);
    return out;
}
