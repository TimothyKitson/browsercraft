#include "Chat.h"
#include "Effects.h"
#include "GameMode.h"
#include "Entity/MobType.h"
#include <cmath>
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
        std::string target = lower(wanted);
        if (target.rfind("minecraft:", 0) == 0) target = target.substr(10);

        for (int pass = 0; pass < 2; ++pass)
            for (int id = 1; id < static_cast<int>(Items::Count); ++id)
            {
                const StackId stack = static_cast<StackId>(id);
                if (!isItem(stack) && stack >= static_cast<StackId>(Blocks::Count)) continue;
                if (pass == 0 && !isItem(stack) && !isObtainable(asBlock(stack))) continue;

                std::string name = lower(displayName(stack));
                std::replace(name.begin(), name.end(), ' ', '_');
                if (name == target) return stack;

                std::string spaced = lower(displayName(stack));
                if (spaced == target) return stack;
            }
        const std::string suffix = "_block";
        if (target.size() > suffix.size() && target.compare(target.size() - suffix.size(), suffix.size(), suffix) == 0)
            return lookUp("block_of_" + target.substr(0, target.size() - suffix.size()));
        return Blocks::Air;
    }

    bool coordinate(const std::string& text, float base, float& out)
    {
        if (!text.empty() && text[0] == '~')
        {
            float offset = 0.0f;
            if (text.size() > 1 && !number(text.substr(1), offset)) return false;
            out = base + offset;
            return true;
        }
        return number(text, out);
    }

    int mobNamed(const std::string& wanted)
    {
        std::string target = lower(wanted);
        if (target.rfind("minecraft:", 0) == 0) target = target.substr(10);
        for (int m = 0; m < static_cast<int>(MobId::Count); ++m)
        {
            std::string name = lower(mobType(static_cast<MobId>(m)).name);
            if (name == target) return m;
            std::replace(name.begin(), name.end(), ' ', '_');
            if (name == target) return m;
        }
        return -1;
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

bool Chat::succeeded(const Command& command)
{
    return command.kind != Kind::Nothing && command.kind != Kind::Unknown && command.kind != Kind::Denied;
}

Chat::Command Chat::parse(const std::string& line, bool cheats, const glm::vec3& origin)
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
            ? "/gamemode /time /tp /give /kill /seed /say /setblock /summon /effect /help"
            : "/help  -  this world was made without cheats, so the rest are off";
        return out;
    }

    if (name == "gamemode" || name == "gm")
    {
        if (!cheats) return deny(name);
        if (words.size() < 2) return fail("/gamemode survival|creative|adventure|spectator|hardcore");

        const std::string wanted = lower(words[1]);
        if (wanted == "survival" || wanted == "s" || wanted == "0")
            out.mode = static_cast<int>(GameMode::Survival);
        else if (wanted == "creative" || wanted == "c" || wanted == "1")
            out.mode = static_cast<int>(GameMode::Creative);
        else if (wanted == "hardcore" || wanted == "h")
            out.mode = static_cast<int>(GameMode::Hardcore);
        else if (wanted == "adventure" || wanted == "a" || wanted == "2")
            out.mode = static_cast<int>(GameMode::Adventure);
        else if (wanted == "spectator" || wanted == "sp" || wanted == "3")
            out.mode = static_cast<int>(GameMode::Spectator);
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
        if (!coordinate(words[1], origin.x, x) || !coordinate(words[2], origin.y, y) || !coordinate(words[3], origin.z, z))
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

    if (name == "say")
    {
        if (!cheats) return deny(name);
        if (words.size() < 2) return fail("/say <message>");
        std::string message;
        for (size_t i = 1; i < words.size(); ++i) message += (i > 1 ? " " : "") + words[i];
        out.kind = Kind::Announce;
        out.text = message.substr(0, MAX_LENGTH);
        return out;
    }

    if (name == "setblock")
    {
        if (!cheats) return deny(name);
        if (words.size() < 5) return fail("/setblock <x> <y> <z> <block>");
        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (!coordinate(words[1], std::floor(origin.x), x) || !coordinate(words[2], std::floor(origin.y), y) ||
            !coordinate(words[3], std::floor(origin.z), z))
            return fail("/setblock wants three numbers.");
        const std::string wanted = lower(words[4]);
        const bool air = wanted == "air" || wanted == "minecraft:air";
        const StackId id = air ? static_cast<StackId>(Blocks::Air) : lookUp(words[4]);
        if (id == Blocks::Air && !air) return fail("No block called " + words[4] + ".");
        if (isItem(id)) return fail(words[4] + " is not a block.");
        out.kind = Kind::SetBlock;
        out.position = glm::vec3(std::floor(x), std::floor(y), std::floor(z));
        out.item = id;
        out.text = "Changed the block at " + std::to_string(static_cast<int>(std::floor(x))) + ", " +
                   std::to_string(static_cast<int>(std::floor(y))) + ", " + std::to_string(static_cast<int>(std::floor(z))) + ".";
        return out;
    }

    if (name == "summon")
    {
        if (!cheats) return deny(name);
        if (words.size() < 2) return fail("/summon <mob> [x] [y] [z]");
        const int mob = mobNamed(words[1]);
        if (mob < 0) return fail("No mob called " + words[1] + ".");
        glm::vec3 at = origin;
        if (words.size() >= 5 &&
            (!coordinate(words[2], origin.x, at.x) || !coordinate(words[3], origin.y, at.y) || !coordinate(words[4], origin.z, at.z)))
            return fail("/summon wants three numbers for where.");
        out.kind = Kind::Summon;
        out.mob = mob;
        out.position = at;
        out.text = std::string("Summoned a new ") + mobType(static_cast<MobId>(mob)).name + ".";
        return out;
    }

    if (name == "effect")
    {
        if (!cheats) return deny(name);
        if (words.size() < 2) return fail("/effect give <effect> [seconds] [amplifier] or /effect clear [effect]");
        const std::string action = lower(words[1]);
        size_t next = 2;
        if (next < words.size() && !words[next].empty() && words[next][0] == '@')
            ++next;
        else if (next + 1 < words.size())
        {
            Effects::Kind probe;
            if (!Effects::fromId(lower(words[next]), probe) && Effects::fromId(lower(words[next + 1]), probe)) ++next;
        }
        if (action == "clear")
        {
            out.kind = Kind::EffectClear;
            if (next < words.size())
            {
                Effects::Kind kind;
                if (!Effects::fromId(lower(words[next]), kind)) return fail("No effect called " + words[next] + ".");
                out.effect = static_cast<int>(kind);
                out.text = std::string("Removed ") + Effects::name(kind) + ".";
            }
            else
                out.text = "Removed every effect.";
            return out;
        }
        if (action != "give" || next >= words.size()) return fail("/effect give <effect> [seconds] [amplifier]");
        Effects::Kind kind;
        if (!Effects::fromId(lower(words[next]), kind)) return fail("No effect called " + words[next] + ".");
        float seconds = Effects::isInstant(kind) ? Effects::INSTANT_SECONDS : 30.0f;
        if (next + 1 < words.size())
        {
            if (lower(words[next + 1]) == "infinite") seconds = 1000000.0f;
            else if (!number(words[next + 1], seconds) || seconds < 1.0f) return fail("/effect wants a number of seconds.");
            seconds = std::min(seconds, 1000000.0f);
        }
        float amplifier = 0.0f;
        if (next + 2 < words.size() && (!number(words[next + 2], amplifier) || amplifier < 0.0f || amplifier > 255.0f))
            return fail("/effect wants an amplifier from 0 to 255.");
        out.kind = Kind::EffectGive;
        out.effect = static_cast<int>(kind);
        out.amplifier = static_cast<int>(amplifier);
        out.seconds = seconds;
        out.text = std::string("Applied ") + Effects::name(kind) + ".";
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
