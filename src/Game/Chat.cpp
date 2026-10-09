#include "Chat.h"
#include "GameMode.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace
{
    std::vector<std::string> split(const std::string& line)
    {
        std::vector<std::string> words;
        std::istringstream stream(line);
        std::string word;
        while (stream >> word) words.push_back(word);
        return words;
    }

    std::string lower(std::string text)
    {
        for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    }

    bool number(const std::string& text, float& out)
    {
        try { out = std::stof(text); }
        catch (...) { return false; }
        return true;
    }

    // Blocks and items alike, by the name the inventory shows, with
    // spaces allowed to be underscores the way Minecraft writes them.
    StackId lookUp(const std::string& wanted)
    {
        const std::string target = lower(wanted);

        for (int id = 1; id < static_cast<int>(Items::Count); ++id)
        {
            const StackId stack = static_cast<StackId>(id);
            if (!isItem(stack) && stack >= 256) continue;

            std::string name = lower(displayName(stack));
            std::replace(name.begin(), name.end(), ' ', '_');
            if (name == target) return stack;

            std::string spaced = lower(displayName(stack));
            if (spaced == lower(wanted)) return stack;
        }
        return Blocks::Air;
    }

    Chat::Command deny(const std::string& name)
    {
        Chat::Command c;
        c.kind = Chat::Kind::Denied;
        c.text = "/" + name + " needs cheats, which this world was made without.";
        return c;
    }

    Chat::Command fail(const std::string& message)
    {
        Chat::Command c;
        c.kind = Chat::Kind::Unknown;
        c.text = message;
        return c;
    }
}

Chat::Command Chat::parse(const std::string& line, bool cheats)
{
    Command out;

    std::string trimmed = line;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
        trimmed.erase(trimmed.begin());
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
        trimmed.pop_back();

    if (trimmed.empty()) return out;

    if (trimmed[0] != '/')
    {
        out.kind = Kind::Say;
        out.text = trimmed.substr(0, MAX_LENGTH);
        return out;
    }

    const std::vector<std::string> words = split(trimmed.substr(1));
    if (words.empty()) return fail("Type a command after the slash. /help lists them.");

    const std::string name = lower(words[0]);

    // Help works whether or not cheats do, and says which it is.
    if (name == "help")
    {
        out.kind = Kind::Help;
        out.text = cheats
            ? "/gamemode /time /tp /give /kill /seed /help"
            : "/help  -  this world was made without cheats, so the rest are off";
        return out;
    }

    if (name == "gamemode" || name == "gm")
    {
        if (!cheats) return deny(name);
        if (words.size() < 2) return fail("/gamemode survival|creative|hardcore");

        const std::string wanted = lower(words[1]);
        if (wanted == "survival" || wanted == "s" || wanted == "0")
            out.mode = static_cast<int>(GameMode::Survival);
        else if (wanted == "creative" || wanted == "c" || wanted == "1")
            out.mode = static_cast<int>(GameMode::Creative);
        else if (wanted == "hardcore" || wanted == "h")
            out.mode = static_cast<int>(GameMode::Hardcore);
        else
            return fail("No game mode called " + words[1] + ".");

        out.kind = Kind::GameMode;
        out.text = "Game mode set to " + wanted + ".";
        return out;
    }

    if (name == "time")
    {
        if (!cheats) return deny(name);
        if (words.size() < 3 || lower(words[1]) != "set")
            return fail("/time set day|noon|night|midnight");

        const std::string when = lower(words[2]);
        if (when == "day") out.time = 0.25f;
        else if (when == "noon") out.time = 0.5f;
        else if (when == "night") out.time = 0.75f;
        else if (when == "midnight") out.time = 0.0f;
        else
        {
            float ticks = 0.0f;
            if (!number(when, ticks)) return fail("No time called " + words[2] + ".");
            // Minecraft counts 24000 ticks to the day, starting at dawn.
            out.time = std::fmod(ticks / 24000.0f + 0.25f, 1.0f);
        }

        out.kind = Kind::TimeSet;
        out.text = "Time set to " + when + ".";
        return out;
    }

    if (name == "tp" || name == "teleport")
    {
        if (!cheats) return deny(name);
        if (words.size() < 4) return fail("/tp <x> <y> <z>");

        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (!number(words[1], x) || !number(words[2], y) || !number(words[3], z))
            return fail("/tp wants three numbers.");

        out.kind = Kind::Teleport;
        out.position = glm::vec3(x, y, z);
        out.text = "Teleported.";
        return out;
    }

    if (name == "give")
    {
        if (!cheats) return deny(name);
        if (words.size() < 2) return fail("/give <name> [count]");

        const StackId id = lookUp(words[1]);
        if (id == Blocks::Air) return fail("Nothing called " + words[1] + ".");

        int count = 1;
        if (words.size() >= 3)
        {
            float asked = 1.0f;
            if (!number(words[2], asked)) return fail("/give wants a number for the count.");
            count = std::clamp(static_cast<int>(asked), 1, 64 * 9);
        }

        out.kind = Kind::Give;
        out.item = id;
        out.count = count;
        out.text = "Gave " + std::to_string(count) + " " + displayName(id) + ".";
        return out;
    }

    if (name == "kill")
    {
        if (!cheats) return deny(name);
        out.kind = Kind::Kill;
        out.text = "Ouch.";
        return out;
    }

    if (name == "seed")
    {
        if (!cheats) return deny(name);
        out.kind = Kind::Seed;
        return out;
    }

    return fail("No command called /" + words[0] + ". /help lists them.");
}

void Chat::Log::add(const std::string& text)
{
    if (text.empty()) return;

    m_lines.push_back(Line{ text, 0.0f });

    // Only the last few are ever shown, so nothing is gained by keeping
    // a transcript of the whole session in memory.
    if (m_lines.size() > static_cast<size_t>(VISIBLE_LINES) * 4)
        m_lines.erase(m_lines.begin(), m_lines.begin() + VISIBLE_LINES);
}

void Chat::Log::update(float deltaTime)
{
    for (Line& line : m_lines) line.age += deltaTime;
}
