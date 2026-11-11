#pragma once
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace Advancements
{
    enum class Tab : uint8_t { Story, Nether, End, Adventure, Husbandry };
    enum class Frame : uint8_t { Task, Goal, Challenge };

    constexpr int TAB_COUNT = 5;

    struct Criterion
    {
        const char* name;
        const char* items;
    };

    struct Def
    {
        const char* id;
        Tab tab;
        const char* parent;
        const char* icon;
        const char* title;
        const char* description;
        Frame frame;
        bool toast;
        bool hidden;
        float x;
        float y;
        bool any;
        std::vector<Criterion> criteria;
    };

    const std::vector<Def>& all();
    int indexOf(const std::string& id);
    int parentOf(int index);
    int rootOf(Tab tab);
    const char* tabBackground(Tab tab);
    const char* frameText(Frame frame);
    unsigned int frameColour(Frame frame);

    class Tracker
    {
    public:
        Tracker();
        bool grant(const std::string& id, const std::string& criterion);
        bool grantAll(const std::string& id);
        bool revoke(const std::string& id, const std::string& criterion = std::string());
        bool done(int index) const;
        bool has(int index, const std::string& criterion) const;
        int criteriaDone(int index) const;
        int criteriaNeeded(int index) const;
        bool visible(int index) const;
        bool tabVisible(Tab tab) const;
        void reset();
        std::vector<std::string> lines() const;
        void load(const std::vector<std::string>& lines);
        std::vector<int> takeNew();

    private:
        std::vector<std::set<std::string>> m_done;
        std::vector<int> m_new;
        bool complete(int index) const;
        bool reachable(int index) const;
    };
}
