#include "Scoreboard.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <cstdlib>
#include <sstream>

namespace
{
    bool integer(const std::string& text, int& out)
    {
        if (text.empty()) return false;
        char* end = nullptr;
        const long value = std::strtol(text.c_str(), &end, 10);
        if (!end || *end != '\0') return false;
        out = static_cast<int>(value);
        return true;
    }

    std::string joined(const std::vector<std::string>& args, size_t from)
    {
        std::string text;
        for (size_t i = from; i < args.size(); ++i) text += (i > from ? " " : "") + args[i];
        if (text.size() >= 2 && text.front() == '"' && text.back() == '"') text = text.substr(1, text.size() - 2);
        if (text.size() >= 2 && text.front() == '{')
        {
            const size_t key = text.find("\"text\"");
            if (key != std::string::npos)
            {
                const size_t open = text.find('"', text.find(':', key) + 1);
                const size_t close = open == std::string::npos ? open : text.find('"', open + 1);
                if (open != std::string::npos && close != std::string::npos) text = text.substr(open + 1, close - open - 1);
            }
        }
        return text;
    }

    bool known(const std::string& criterion)
    {
        static const char* CRITERIA[] = { "dummy", "trigger", "deathCount", "playerKillCount", "totalKillCount", "health",
                                          "xp", "level", "food", "air", "armor" };
        for (const char* one : CRITERIA)
            if (criterion == one) return true;
        return criterion.rfind("minecraft.", 0) == 0;
    }

    bool readOnly(const std::string& criterion)
    {
        return criterion == "health" || criterion == "xp" || criterion == "level" || criterion == "food" || criterion == "air" ||
               criterion == "armor";
    }

    const char* COLOURS[] = { "black", "dark_blue", "dark_green", "dark_aqua", "dark_red", "dark_purple", "gold", "gray",
                              "dark_gray", "blue", "green", "aqua", "red", "light_purple", "yellow", "white", "reset" };
}

std::vector<std::string> Scoreboard::holders(const std::string& target, const std::string& self) const
{
    if (target == "@s" || target == "@p" || target == "@a" || target == "@r" || target.empty()) return { self };
    if (target == "*")
    {
        std::set<std::string> all;
        for (const auto& [objective, scores] : m_scores)
            for (const auto& [holder, value] : scores) all.insert(holder);
        return std::vector<std::string>(all.begin(), all.end());
    }
    return { target };
}

const Scoreboard::Objective* Scoreboard::objective(const std::string& name) const
{
    const auto found = m_objectives.find(name);
    return found == m_objectives.end() ? nullptr : &found->second;
}

const Scoreboard::Objective* Scoreboard::displayed(const std::string& slot) const
{
    const auto found = m_display.find(slot);
    return found == m_display.end() ? nullptr : objective(found->second);
}

std::vector<std::pair<std::string, int>> Scoreboard::sidebar() const
{
    std::vector<std::pair<std::string, int>> rows;
    const Objective* shown = displayed("sidebar");
    if (!shown) return rows;
    const auto scores = m_scores.find(shown->name);
    if (scores == m_scores.end()) return rows;
    for (const auto& [holder, value] : scores->second)
        if (holder.empty() || holder[0] != '#') rows.emplace_back(holder, value);
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });
    if (rows.size() > 15) rows.resize(15);
    return rows;
}

bool Scoreboard::has(const std::string& objective, const std::string& holder) const
{
    const auto scores = m_scores.find(objective);
    return scores != m_scores.end() && scores->second.count(holder) > 0;
}

int Scoreboard::score(const std::string& objective, const std::string& holder) const
{
    const auto scores = m_scores.find(objective);
    if (scores == m_scores.end()) return 0;
    const auto found = scores->second.find(holder);
    return found == scores->second.end() ? 0 : found->second;
}

void Scoreboard::setScore(const std::string& objective, const std::string& holder, int value)
{
    if (!m_objectives.count(objective)) return;
    m_scores[objective][holder] = value;
}

void Scoreboard::addToCriterion(const std::string& criterion, const std::string& holder, int amount)
{
    for (const auto& [name, objective] : m_objectives)
        if (objective.criterion == criterion) m_scores[name][holder] += amount;
}

void Scoreboard::refreshLive(const std::string& holder, const Live& live)
{
    for (const auto& [name, objective] : m_objectives)
    {
        const std::string& c = objective.criterion;
        if (!readOnly(c)) continue;
        const int value = c == "health" ? live.health : c == "food" ? live.food : c == "air" ? live.air
                        : c == "armor" ? live.armor : c == "level" ? live.level : live.xp;
        m_scores[name][holder] = value;
    }
}

const Scoreboard::Team* Scoreboard::teamOf(const std::string& holder) const
{
    for (const auto& [name, team] : m_teams)
        if (team.members.count(holder)) return &team;
    return nullptr;
}

bool Scoreboard::command(const std::string& name, const std::vector<std::string>& args, const std::string& self, std::string& output)
{
    if (name == "scoreboard")
    {
        if (args.empty())
        {
            output = "/scoreboard objectives|players ...";
            return false;
        }
        const std::vector<std::string> rest(args.begin() + 1, args.end());
        if (args[0] == "objectives") return objectives(rest, output);
        if (args[0] == "players") return players(rest, self, output);
        output = "/scoreboard objectives|players ...";
        return false;
    }
    if (name == "team") return team(args, self, output);
    if (name == "tag") return tag(args, output);
    if (name == "trigger") return trigger(args, self, output);
    output = "Unknown scoreboard command";
    return false;
}

bool Scoreboard::objectives(const std::vector<std::string>& args, std::string& output)
{
    const std::string action = args.empty() ? std::string() : args[0];
    if (action == "list")
    {
        if (m_objectives.empty())
        {
            output = "There are no objectives";
            return true;
        }
        output = "There are " + std::to_string(m_objectives.size()) + " objective(s):";
        for (const auto& [name, objective] : m_objectives) output += " [" + objective.displayName + "]";
        return true;
    }
    if (action == "add" && args.size() >= 3)
    {
        if (m_objectives.count(args[1]))
        {
            output = "An objective already exists by that name";
            return false;
        }
        if (!known(args[2]))
        {
            output = "Unknown criterion '" + args[2] + "'";
            return false;
        }
        Objective made{ args[1], args[2], args.size() > 3 ? joined(args, 3) : args[1] };
        m_objectives[args[1]] = made;
        output = "Created new objective [" + made.displayName + "]";
        return true;
    }
    if (action == "remove" && args.size() >= 2)
    {
        if (!m_objectives.erase(args[1]))
        {
            output = "Unknown scoreboard objective '" + args[1] + "'";
            return false;
        }
        m_scores.erase(args[1]);
        for (auto it = m_display.begin(); it != m_display.end();)
            it = it->second == args[1] ? m_display.erase(it) : std::next(it);
        output = "Removed objective [" + args[1] + "]";
        return true;
    }
    if (action == "setdisplay" && args.size() >= 2)
    {
        const std::string slot = args[1].rfind("sidebar.team.", 0) == 0 ? std::string("sidebar") : args[1];
        if (slot != "sidebar" && slot != "list" && slot != "below_name" && slot != "belowName")
        {
            output = "Unknown display slot '" + args[1] + "'";
            return false;
        }
        if (args.size() < 3)
        {
            m_display.erase(slot);
            output = "Cleared any objectives in display slot " + slot;
            return true;
        }
        if (!m_objectives.count(args[2]))
        {
            output = "Unknown scoreboard objective '" + args[2] + "'";
            return false;
        }
        m_display[slot == "belowName" ? std::string("below_name") : slot] = args[2];
        output = "Set display slot " + slot + " to show objective " + args[2];
        return true;
    }
    if (action == "modify" && args.size() >= 4 && args[2] == "displayname")
    {
        auto found = m_objectives.find(args[1]);
        if (found == m_objectives.end())
        {
            output = "Unknown scoreboard objective '" + args[1] + "'";
            return false;
        }
        found->second.displayName = joined(args, 3);
        output = "Changed the display name of " + args[1] + " to [" + found->second.displayName + "]";
        return true;
    }
    output = "/scoreboard objectives add|remove|list|setdisplay|modify ...";
    return false;
}

bool Scoreboard::players(const std::vector<std::string>& args, const std::string& self, std::string& output)
{
    const std::string action = args.empty() ? std::string() : args[0];
    if (action == "list")
    {
        if (args.size() < 2)
        {
            std::set<std::string> all;
            for (const auto& [objective, scores] : m_scores)
                for (const auto& [holder, value] : scores) all.insert(holder);
            output = all.empty() ? std::string("There are no tracked entities") : "There are " + std::to_string(all.size()) + " tracked entity/entities:";
            for (const std::string& holder : all) output += " " + holder;
            return true;
        }
        const std::string who = holders(args[1], self).front();
        std::string list;
        int count = 0;
        for (const auto& [objective, scores] : m_scores)
        {
            const auto found = scores.find(who);
            if (found == scores.end()) continue;
            list += " [" + m_objectives[objective].displayName + "]: " + std::to_string(found->second);
            ++count;
        }
        output = count == 0 ? who + " has no scores to show" : who + " has " + std::to_string(count) + " score(s):" + list;
        return true;
    }
    if (args.size() < 3 && action != "reset")
    {
        output = "/scoreboard players set|add|remove|reset|get|enable|operation|list ...";
        return false;
    }
    if (action == "reset")
    {
        if (args.size() < 2) return false;
        for (const std::string& who : holders(args[1], self))
            for (auto& [objective, scores] : m_scores)
                if (args.size() < 3 || objective == args[2]) scores.erase(who);
        output = "Reset scores for " + args[1];
        return true;
    }
    const std::string& objectiveName = args[2];
    const auto found = m_objectives.find(objectiveName);
    if (found == m_objectives.end())
    {
        output = "Unknown scoreboard objective '" + objectiveName + "'";
        return false;
    }
    const bool frozen = readOnly(found->second.criterion);
    if (action == "get")
    {
        const std::string who = holders(args[1], self).front();
        if (!has(objectiveName, who))
        {
            output = "Can't get value of " + objectiveName + " for " + who + "; none is set";
            return false;
        }
        output = who + " has " + std::to_string(score(objectiveName, who)) + " [" + found->second.displayName + "]";
        return true;
    }
    if (action == "enable")
    {
        if (found->second.criterion != "trigger")
        {
            output = "Only trigger objectives can be enabled";
            return false;
        }
        for (const std::string& who : holders(args[1], self)) m_enabledTriggers[objectiveName].insert(who);
        output = "Enabled trigger " + objectiveName;
        return true;
    }
    if (action == "operation")
    {
        if (args.size() < 6)
        {
            output = "/scoreboard players operation <targets> <objective> <operation> <source> <objective>";
            return false;
        }
        if (frozen || !m_objectives.count(args[5]))
        {
            output = "That objective is read-only or unknown";
            return false;
        }
        const std::string op = args[3];
        const std::vector<std::string> sources = holders(args[4], self);
        for (const std::string& who : holders(args[1], self))
        {
            int& left = m_scores[objectiveName][who];
            for (const std::string& from : sources)
            {
                int& right = m_scores[args[5]][from];
                if (op == "=") left = right;
                else if (op == "+=") left += right;
                else if (op == "-=") left -= right;
                else if (op == "*=") left *= right;
                else if (op == "/=") { if (right != 0) left = static_cast<int>(std::floor(static_cast<double>(left) / right)); }
                else if (op == "%=") { if (right != 0) left = ((left % right) + right) % right; }
                else if (op == "<") left = std::min(left, right);
                else if (op == ">") left = std::max(left, right);
                else if (op == "><") std::swap(left, right);
                else
                {
                    output = "Invalid operation";
                    return false;
                }
            }
        }
        output = "Set " + objectiveName + " for " + args[1];
        return true;
    }
    int value = 0;
    if (args.size() < 4 || !integer(args[3], value))
    {
        output = "Expected an integer";
        return false;
    }
    if (frozen)
    {
        output = "That objective is read-only";
        return false;
    }
    int last = 0;
    const std::vector<std::string> targets = holders(args[1], self);
    for (const std::string& who : targets)
    {
        int& held = m_scores[objectiveName][who];
        if (action == "set") held = value;
        else if (action == "add") held += value;
        else if (action == "remove") held -= value;
        else
        {
            output = "/scoreboard players set|add|remove ...";
            return false;
        }
        last = held;
    }
    output = "Set [" + found->second.displayName + "] for " + (targets.size() == 1 ? targets[0] : args[1]) + " to " + std::to_string(last);
    return true;
}

bool Scoreboard::team(const std::vector<std::string>& args, const std::string& self, std::string& output)
{
    const std::string action = args.empty() ? std::string() : args[0];
    if (action == "list")
    {
        if (args.size() >= 2)
        {
            const auto found = m_teams.find(args[1]);
            if (found == m_teams.end())
            {
                output = "Unknown team '" + args[1] + "'";
                return false;
            }
            output = found->second.members.empty() ? "There are no members on team [" + found->second.displayName + "]"
                                                   : "Team [" + found->second.displayName + "] has " + std::to_string(found->second.members.size()) + " member(s):";
            for (const std::string& member : found->second.members) output += " " + member;
            return true;
        }
        output = m_teams.empty() ? std::string("There are no teams") : "There are " + std::to_string(m_teams.size()) + " team(s):";
        for (const auto& [name, team] : m_teams) output += " [" + team.displayName + "]";
        return true;
    }
    if (action == "add" && args.size() >= 2)
    {
        if (m_teams.count(args[1]))
        {
            output = "A team already exists by that name";
            return false;
        }
        Team made;
        made.name = args[1];
        made.displayName = args.size() > 2 ? joined(args, 2) : args[1];
        m_teams[args[1]] = made;
        output = "Created team [" + made.displayName + "]";
        return true;
    }
    if (action == "leave" && args.size() >= 2)
    {
        int left = 0;
        for (const std::string& who : holders(args[1], self))
            for (auto& [name, team] : m_teams) left += static_cast<int>(team.members.erase(who));
        output = "Removed " + std::to_string(left) + " member(s) from any team";
        return left > 0;
    }
    if (args.size() < 2)
    {
        output = "/team add|remove|empty|join|leave|list|modify ...";
        return false;
    }
    const auto found = m_teams.find(args[1]);
    if (found == m_teams.end())
    {
        output = "Unknown team '" + args[1] + "'";
        return false;
    }
    Team& team = found->second;
    if (action == "remove")
    {
        output = "Removed team [" + team.displayName + "]";
        m_teams.erase(found);
        return true;
    }
    if (action == "empty")
    {
        output = "Removed " + std::to_string(team.members.size()) + " member(s) from team [" + team.displayName + "]";
        team.members.clear();
        return true;
    }
    if (action == "join")
    {
        const std::vector<std::string> joining = holders(args.size() >= 3 ? args[2] : "@s", self);
        for (const std::string& who : joining)
        {
            for (auto& [name, other] : m_teams) other.members.erase(who);
            team.members.insert(who);
        }
        output = "Added " + std::to_string(joining.size()) + " member(s) to team [" + team.displayName + "]";
        return true;
    }
    if (action == "modify" && args.size() >= 4)
    {
        const std::string& option = args[2];
        const std::string value = joined(args, 3);
        if (option == "displayName") team.displayName = value;
        else if (option == "prefix") team.prefix = value;
        else if (option == "suffix") team.suffix = value;
        else if (option == "friendlyFire") team.friendlyFire = value == "true";
        else if (option == "seeFriendlyInvisibles") team.seeFriendlyInvisibles = value == "true";
        else if (option == "color")
        {
            bool ok = false;
            for (const char* colour : COLOURS) ok |= value == colour;
            if (!ok)
            {
                output = "Unknown colour '" + value + "'";
                return false;
            }
            team.colour = value;
        }
        else if (option == "nametagVisibility" || option == "deathMessageVisibility" || option == "collisionRule") {}
        else
        {
            output = "Unknown team option '" + option + "'";
            return false;
        }
        output = "Updated the " + option + " of team [" + team.displayName + "]";
        return true;
    }
    output = "/team add|remove|empty|join|leave|list|modify ...";
    return false;
}

bool Scoreboard::tag(const std::vector<std::string>& args, std::string& output)
{
    if (args.size() < 2)
    {
        output = "/tag <targets> add|remove|list [name]";
        return false;
    }
    const std::string& action = args[1];
    if (action == "list")
    {
        output = m_tags.empty() ? std::string("You have no tags") : "You have " + std::to_string(m_tags.size()) + " tag(s):";
        for (const std::string& name : m_tags) output += " " + name;
        return true;
    }
    if (args.size() < 3)
    {
        output = "/tag <targets> add|remove <name>";
        return false;
    }
    if (action == "add")
    {
        if (!m_tags.insert(args[2]).second)
        {
            output = "Nothing changed. You already have that tag";
            return false;
        }
        output = "Added tag '" + args[2] + "'";
        return true;
    }
    if (action == "remove")
    {
        if (!m_tags.erase(args[2]))
        {
            output = "Nothing changed. You don't have that tag";
            return false;
        }
        output = "Removed tag '" + args[2] + "'";
        return true;
    }
    output = "/tag <targets> add|remove|list [name]";
    return false;
}

bool Scoreboard::trigger(const std::vector<std::string>& args, const std::string& self, std::string& output)
{
    if (args.empty())
    {
        output = "/trigger <objective> [add|set <value>]";
        return false;
    }
    const auto found = m_objectives.find(args[0]);
    if (found == m_objectives.end() || found->second.criterion != "trigger")
    {
        output = "Invalid trigger objective name";
        return false;
    }
    auto enabled = m_enabledTriggers.find(args[0]);
    if (enabled == m_enabledTriggers.end() || !enabled->second.count(self))
    {
        output = "You cannot trigger this objective yet";
        return false;
    }
    int value = 1;
    const bool set = args.size() >= 3 && args[1] == "set";
    if (args.size() >= 3 && !integer(args[2], value))
    {
        output = "Expected an integer";
        return false;
    }
    int& held = m_scores[args[0]][self];
    held = set ? value : held + value;
    enabled->second.erase(self);
    output = "Triggered [" + found->second.displayName + "]";
    return true;
}

std::string Scoreboard::save() const
{
    std::ostringstream out;
    for (const auto& [name, objective] : m_objectives)
        out << "objective\t" << name << '\t' << objective.criterion << '\t' << objective.displayName << '\n';
    for (const auto& [name, scores] : m_scores)
        for (const auto& [holder, value] : scores) out << "score\t" << name << '\t' << holder << '\t' << value << '\n';
    for (const auto& [slot, name] : m_display) out << "display\t" << slot << '\t' << name << '\n';
    for (const auto& [name, team] : m_teams)
    {
        out << "team\t" << name << '\t' << team.displayName << '\t' << team.colour << '\t' << team.prefix << '\t' << team.suffix << '\t'
            << (team.friendlyFire ? 1 : 0) << '\t' << (team.seeFriendlyInvisibles ? 1 : 0) << '\n';
        for (const std::string& member : team.members) out << "member\t" << name << '\t' << member << '\n';
    }
    for (const auto& [name, who] : m_enabledTriggers)
        for (const std::string& holder : who) out << "enabled\t" << name << '\t' << holder << '\n';
    for (const std::string& name : m_tags) out << "tag\t" << name << '\n';
    return out.str();
}

void Scoreboard::load(const std::string& text)
{
    clear();
    std::istringstream in(text);
    std::string row;
    while (std::getline(in, row))
    {
        std::vector<std::string> cells;
        std::string cell;
        std::istringstream line(row);
        while (std::getline(line, cell, '\t')) cells.push_back(cell);
        if (cells.empty()) continue;
        const std::string& kind = cells[0];
        if (kind == "objective" && cells.size() >= 4) m_objectives[cells[1]] = Objective{ cells[1], cells[2], cells[3] };
        else if (kind == "score" && cells.size() >= 4)
        {
            int value = 0;
            if (integer(cells[3], value)) m_scores[cells[1]][cells[2]] = value;
        }
        else if (kind == "display" && cells.size() >= 3) m_display[cells[1]] = cells[2];
        else if (kind == "team" && cells.size() >= 8)
        {
            Team& team = m_teams[cells[1]];
            team.name = cells[1];
            team.displayName = cells[2];
            team.colour = cells[3];
            team.prefix = cells[4];
            team.suffix = cells[5];
            team.friendlyFire = cells[6] == "1";
            team.seeFriendlyInvisibles = cells[7] == "1";
        }
        else if (kind == "member" && cells.size() >= 3) m_teams[cells[1]].members.insert(cells[2]);
        else if (kind == "enabled" && cells.size() >= 3) m_enabledTriggers[cells[1]].insert(cells[2]);
        else if (kind == "tag" && cells.size() >= 2) m_tags.insert(cells[1]);
    }
}

void Scoreboard::clear()
{
    m_objectives.clear();
    m_scores.clear();
    m_enabledTriggers.clear();
    m_display.clear();
    m_teams.clear();
    m_tags.clear();
}
