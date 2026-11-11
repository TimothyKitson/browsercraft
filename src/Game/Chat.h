#pragma once
#include "Items.h"
#include "World/WorldBorder.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

// The chat line and the commands typed into it.
//
// Parsing is a pure function that returns what the player asked for
// rather than doing it, so --selftest can type every command and read
// back what it meant without a world, a player or a clock. Application
// is left with a switch that carries the request out.
namespace Chat
{
    constexpr size_t MAX_LENGTH = 100;
    constexpr float LINE_SECONDS = 10.0f;
    constexpr int VISIBLE_LINES = 10;

    enum class Kind
    {
        Nothing,      // blank line: say nothing
        Say,          // ordinary chat
        Unknown,      // a slash command nobody recognises
        Denied,       // a real command, but cheats are off
        Help,
        GameMode,
        TimeSet,
        Teleport,
        Give,
        Kill,
        Seed,
        Announce,
        SetBlock,
        Summon,
        EffectGive,
        EffectClear,
        Tellraw,
        WorldBorder,
        GameRuleQuery,
        GameRuleSet,
        Weather,
        Difficulty,
        Experience,
        Clear,
        SetWorldSpawn,
        SpawnPoint,
        Me,
        Whisper,
        List,
        DefaultGameMode,
        Fill,
        Clone,
        TimeAdd,
        TimeQuery,
        Title,
        Enchant,
        Locate,
        Damage,
        ScoreboardCommand,
        AttributeCommand,
        MoreCommand,
    };

    struct Command
    {
        Kind kind = Kind::Nothing;
        std::string text;          // what to print, or what was said

        int mode = 0;              // GameMode, for GameMode
        float time = 0.0f;         // 0..1 through the day, for TimeSet
        glm::vec3 position{ 0.0f };
        StackId item = Blocks::Air;
        int count = 1;
        int mob = -1;
        int effect = -1;
        int amplifier = 0;
        float seconds = 0.0f;
        std::string sub;
        double value = 0.0;
        double value2 = 0.0;
        glm::vec3 position2{ 0.0f };
        glm::vec3 position3{ 0.0f };
        std::vector<std::string> args;
        std::string target;
    };

    // `cheats` decides whether anything beyond chat and /help is allowed.
    // A command refused for that reason comes back as Denied rather than
    // Unknown, so the player is told which of the two it was.
    Command parse(const std::string& line, bool cheats, const glm::vec3& origin = glm::vec3(0.0f));
    std::vector<std::string> suggest(const std::string& draft);
    std::string completeWith(const std::string& draft, const std::string& choice);
    bool succeeded(const Command& command);
    bool textComponent(const std::string& json, std::string& out);
    bool applyWorldBorder(const Command& command, WorldBorder& border, std::string& message);

    // One scrolling log. Lines fade out on their own, and are kept so the
    // last few can be shown while the chat line is closed.
    class Log
    {
    public:
        void add(const std::string& text);
        void update(float deltaTime);
        void clear() { m_lines.clear(); }

        struct Line
        {
            std::string text;
            float age = 0.0f;
        };

        const std::vector<Line>& lines() const { return m_lines; }

    private:
        std::vector<Line> m_lines;
    };
}
