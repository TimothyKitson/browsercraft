#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

class Scoreboard
{
public:
    struct Objective
    {
        std::string name;
        std::string criterion = "dummy";
        std::string displayName;
    };

    struct Team
    {
        std::string name;
        std::string displayName;
        std::string colour = "reset";
        std::string prefix;
        std::string suffix;
        bool friendlyFire = true;
        bool seeFriendlyInvisibles = true;
        std::set<std::string> members;
    };

    struct Live
    {
        int health = 20;
        int food = 20;
        int air = 300;
        int armor = 0;
        int level = 0;
        int xp = 0;
    };

    bool command(const std::string& name, const std::vector<std::string>& args, const std::string& self, std::string& output);

    const Objective* objective(const std::string& name) const;
    const Objective* displayed(const std::string& slot) const;
    std::vector<std::pair<std::string, int>> sidebar() const;
    bool has(const std::string& objective, const std::string& holder) const;
    int score(const std::string& objective, const std::string& holder) const;
    void setScore(const std::string& objective, const std::string& holder, int value);
    void addToCriterion(const std::string& criterion, const std::string& holder, int amount);
    void refreshLive(const std::string& holder, const Live& live);
    const Team* teamOf(const std::string& holder) const;
    const std::set<std::string>& tags() const { return m_tags; }
    bool tagged(const std::string& tag) const { return m_tags.count(tag) > 0; }

    std::string save() const;
    void load(const std::string& text);
    void clear();

private:
    std::map<std::string, Objective> m_objectives;
    std::map<std::string, std::map<std::string, int>> m_scores;
    std::map<std::string, std::set<std::string>> m_enabledTriggers;
    std::map<std::string, std::string> m_display;
    std::map<std::string, Team> m_teams;
    std::set<std::string> m_tags;

    bool objectives(const std::vector<std::string>& args, std::string& output);
    bool players(const std::vector<std::string>& args, const std::string& self, std::string& output);
    bool team(const std::vector<std::string>& args, const std::string& self, std::string& output);
    bool tag(const std::vector<std::string>& args, std::string& output);
    bool trigger(const std::vector<std::string>& args, const std::string& self, std::string& output);
    std::vector<std::string> holders(const std::string& target, const std::string& self) const;
};
