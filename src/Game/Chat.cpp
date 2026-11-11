#include "Chat.h"
#include "World/Biomes.h"
#include "Effects.h"
#include "GameMode.h"
#include "Entity/MobType.h"
#include "Core/Utf8.h"
#include "GameRules.h"
#include "Enchanting.h"
#include "Attributes.h"
#include "Renderer/FontMaps.h"
#include <cmath>
#include <cstdio>
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

        std::vector<StackId> stacks;
        for (int id = 1; isBlockId(id); id = nextBlockId(id)) stacks.push_back(static_cast<StackId>(id));
        for (int id = Items::FIRST; id < static_cast<int>(Items::Count); ++id) stacks.push_back(static_cast<StackId>(id));
        for (int pass = 0; pass < 2; ++pass)
            for (const StackId stack : stacks)
            {
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

    struct ComponentReader
    {
        const std::string& json;
        size_t at = 0;
        std::string out;
        FontFace written = FontFace::Default;

        void space()
        {
            while (at < json.size() && std::isspace(static_cast<unsigned char>(json[at]))) ++at;
        }

        bool literal(const char* word)
        {
            const size_t length = std::char_traits<char>::length(word);
            if (json.compare(at, length, word) != 0) return false;
            at += length;
            return true;
        }

        bool hex(char32_t& value)
        {
            if (at + 4 > json.size()) return false;
            value = 0;
            for (int i = 0; i < 4; ++i)
            {
                const char c = json[at++];
                value <<= 4;
                if (c >= '0' && c <= '9') value |= static_cast<char32_t>(c - '0');
                else if (c >= 'a' && c <= 'f') value |= static_cast<char32_t>(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') value |= static_cast<char32_t>(c - 'A' + 10);
                else return false;
            }
            return true;
        }

        bool string(std::string& value)
        {
            space();
            if (at >= json.size() || json[at] != '"') return false;
            ++at;
            value.clear();
            while (at < json.size())
            {
                const char c = json[at++];
                if (c == '"') return true;
                if (c != '\\')
                {
                    value.push_back(c);
                    continue;
                }
                if (at >= json.size()) return false;
                const char e = json[at++];
                switch (e)
                {
                    case 'n': value.push_back('\n'); break;
                    case 't': value.push_back('\t'); break;
                    case 'b': case 'f': case 'r': break;
                    case 'u':
                    {
                        char32_t c32 = 0;
                        if (!hex(c32)) return false;
                        if (c32 >= 0xD800 && c32 < 0xDC00 && json.compare(at, 2, "\\u") == 0)
                        {
                            at += 2;
                            char32_t low = 0;
                            if (!hex(low)) return false;
                            c32 = 0x10000 + ((c32 - 0xD800) << 10) + (low - 0xDC00);
                        }
                        Utf8::append(value, c32);
                        break;
                    }
                    default: value.push_back(e); break;
                }
            }
            return false;
        }

        static FontFace faceNamed(std::string name)
        {
            if (name.rfind("minecraft:", 0) == 0) name = name.substr(10);
            if (name == "alt") return FontFace::Galactic;
            if (name == "illageralt") return FontFace::Illager;
            return FontFace::Default;
        }

        void emit(const std::string& text, FontFace face)
        {
            if (text.empty()) return;
            if (face != written)
            {
                Utf8::append(out, FONT_SWITCH + static_cast<char32_t>(face));
                written = face;
            }
            for (char32_t c : Utf8::decode(text))
                if (c != '\n' && !isFontSwitch(c)) Utf8::append(out, c);
        }

        bool scalar(std::string& text)
        {
            space();
            const size_t start = at;
            if (literal("true") || literal("false")) { text = json.substr(start, at - start); return true; }
            while (at < json.size() && (std::isdigit(static_cast<unsigned char>(json[at])) || json[at] == '-' || json[at] == '.' || json[at] == 'e' || json[at] == 'E' || json[at] == '+')) ++at;
            text = json.substr(start, at - start);
            return at > start;
        }

        bool skip()
        {
            space();
            if (at >= json.size()) return false;
            if (json[at] == '"') { std::string ignored; return string(ignored); }
            if (json[at] == '{' || json[at] == '[')
            {
                const char close = json[at] == '{' ? '}' : ']';
                ++at;
                space();
                if (at < json.size() && json[at] == close) { ++at; return true; }
                while (true)
                {
                    if (close == '}')
                    {
                        std::string key;
                        if (!string(key)) return false;
                        space();
                        if (at >= json.size() || json[at++] != ':') return false;
                    }
                    if (!skip()) return false;
                    space();
                    if (at < json.size() && json[at] == ',') { ++at; continue; }
                    if (at < json.size() && json[at] == close) { ++at; return true; }
                    return false;
                }
            }
            std::string ignored;
            return scalar(ignored) || literal("null");
        }

        bool component(FontFace face, int depth)
        {
            if (depth > 16) return false;
            space();
            if (at >= json.size()) return false;
            const char c = json[at];
            if (c == '"')
            {
                std::string text;
                if (!string(text)) return false;
                emit(text, face);
                return true;
            }
            if (c == '[')
            {
                ++at;
                space();
                if (at < json.size() && json[at] == ']') { ++at; return true; }
                while (true)
                {
                    if (!component(face, depth + 1)) return false;
                    space();
                    if (at < json.size() && json[at] == ',') { ++at; continue; }
                    if (at < json.size() && json[at] == ']') { ++at; return true; }
                    return false;
                }
            }
            if (c == '{')
            {
                ++at;
                std::string text;
                size_t extraAt = std::string::npos;
                FontFace own = face;
                space();
                if (at < json.size() && json[at] == '}') { ++at; return true; }
                while (true)
                {
                    std::string key;
                    if (!string(key)) return false;
                    space();
                    if (at >= json.size() || json[at++] != ':') return false;
                    space();
                    if (key == "text")
                    {
                        if (at < json.size() && json[at] == '"') { if (!string(text)) return false; }
                        else if (!scalar(text)) return false;
                    }
                    else if (key == "font")
                    {
                        std::string name;
                        if (!string(name)) return false;
                        own = faceNamed(name);
                    }
                    else if (key == "extra")
                    {
                        extraAt = at;
                        if (!skip()) return false;
                    }
                    else if (!skip())
                        return false;
                    space();
                    if (at < json.size() && json[at] == ',') { ++at; continue; }
                    if (at < json.size() && json[at] == '}') { ++at; break; }
                    return false;
                }
                emit(text, own);
                if (extraAt != std::string::npos)
                {
                    const size_t after = at;
                    at = extraAt;
                    if (!component(own, depth + 1)) return false;
                    at = after;
                }
                return true;
            }
            std::string text;
            if (!scalar(text)) return false;
            emit(text, face);
            return true;
        }
    };

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

bool Chat::textComponent(const std::string& json, std::string& out)
{
    ComponentReader reader{ json };
    if (!reader.component(FontFace::Default, 0)) return false;
    reader.space();
    if (reader.at != json.size()) return false;
    out = reader.out;
    return true;
}

bool Chat::applyWorldBorder(const Command& command, WorldBorder& border, std::string& message)
{
    char buffer[160];
    auto say = [&](const char* format, auto... values) {
        std::snprintf(buffer, sizeof(buffer), format, values...);
        message = buffer;
    };
    const std::string& sub = command.sub;
    if (sub == "get")
    {
        say("The world border is currently %.0f block(s) wide", border.size());
        return true;
    }
    if (sub == "set" || sub == "add")
    {
        const double current = border.size();
        const double distance = sub == "add" ? current + command.value : command.value;
        const long long seconds = static_cast<long long>(command.seconds);
        if (current == distance) { message = "Nothing changed. The world border is already that size"; return false; }
        if (distance < 1.0) { message = "World border cannot be smaller than 1 block wide"; return false; }
        if (distance > WorldBorder::MAX_SIZE) { say("World border cannot be bigger than %.0f blocks wide", WorldBorder::MAX_SIZE); return false; }
        if (seconds > 0)
        {
            border.lerpTo(current, distance, static_cast<double>(seconds));
            if (distance > current) say("Growing the world border to %.1f blocks wide over %lld seconds", distance, seconds);
            else say("Shrinking the world border to %.1f block(s) wide over %lld second(s)", distance, seconds);
        }
        else
        {
            border.setSize(distance);
            say("Set the world border to %.1f block(s) wide", distance);
        }
        return true;
    }
    if (sub == "center")
    {
        if (border.centerX() == command.value && border.centerZ() == command.value2)
        {
            message = "Nothing changed. The world border is already centered there";
            return false;
        }
        if (std::abs(command.value) > WorldBorder::MAX_CENTER || std::abs(command.value2) > WorldBorder::MAX_CENTER)
        {
            say("World border cannot be further out than %.0f blocks", WorldBorder::MAX_CENTER);
            return false;
        }
        border.setCenter(command.value, command.value2);
        say("Set the center of the world border to %.2f, %.2f", command.value, command.value2);
        return true;
    }
    if (sub == "damage amount")
    {
        if (border.damagePerBlock == command.value) { message = "Nothing changed. The world border damage is already that amount"; return false; }
        border.damagePerBlock = command.value;
        say("Set the world border damage to %.2f per block each second", command.value);
        return true;
    }
    if (sub == "damage buffer")
    {
        if (border.safeZone == command.value) { message = "Nothing changed. The world border damage buffer is already that distance"; return false; }
        border.safeZone = command.value;
        say("Set the world border damage buffer to %.2f block(s)", command.value);
        return true;
    }
    if (sub == "warning distance")
    {
        if (border.warningBlocks == static_cast<int>(command.value)) { message = "Nothing changed. The world border warning is already that distance"; return false; }
        border.warningBlocks = static_cast<int>(command.value);
        say("Set the world border warning distance to %d block(s)", border.warningBlocks);
        return true;
    }
    if (sub == "warning time")
    {
        if (border.warningTime == static_cast<int>(command.value)) { message = "Nothing changed. The world border warning is already that amount of time"; return false; }
        border.warningTime = static_cast<int>(command.value);
        say("Set the world border warning time to %d second(s)", border.warningTime);
        return true;
    }
    message = "/worldborder <add|set|center|damage|get|warning> ...";
    return false;
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
        out.text = Utf8::prefix(trimmed, MAX_LENGTH);
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
            ? "/gamemode /time /tp /give /kill /seed /say /me /msg /list /tellraw /title /setblock /fill /clone /summon /effect /enchant /xp /clear /weather /difficulty /defaultgamemode /setworldspawn /spawnpoint /locate /damage /scoreboard /team /tag /trigger /attribute /worldborder /gamerule /help"
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

    if (name == "time" && !(words.size() >= 2 && (lower(words[1]) == "add" || lower(words[1]) == "query")))
    {
        if (!cheats) return deny(name);
        if (words.size() < 3 || lower(words[1]) != "set")
            return fail("/time set|add|query ...");

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
        out.text = Utf8::prefix(message, MAX_LENGTH);
        return out;
    }

    if (name == "worldborder")
    {
        if (!cheats) return deny(name);
        const std::string usage = "/worldborder <add|set|center|damage|get|warning> ...";
        if (words.size() < 2) return fail(usage);
        const std::string sub = lower(words[1]);
        out.kind = Kind::WorldBorder;
        out.sub = sub;
        auto whole = [](const std::string& text, int& value) {
            char* end = nullptr;
            const long parsed = std::strtol(text.c_str(), &end, 10);
            if (end == text.c_str() || *end || parsed < 0 || parsed > 2147483647L) return false;
            value = static_cast<int>(parsed);
            return true;
        };
        if (sub == "get") return out;
        if (sub == "set" || sub == "add")
        {
            float distance = 0.0f;
            if (words.size() < 3 || words.size() > 4 || !number(words[2], distance) || std::abs(distance) > WorldBorder::MAX_SIZE)
                return fail("/worldborder " + sub + " <distance> [time]");
            int time = 0;
            if (words.size() == 4 && !whole(words[3], time)) return fail("/worldborder " + sub + " <distance> [time]");
            out.value = std::stod(words[2]);
            out.seconds = static_cast<float>(time);
            return out;
        }
        if (sub == "center")
        {
            float x = 0.0f, z = 0.0f;
            if (words.size() != 4 || !coordinate(words[2], origin.x, x) || !coordinate(words[3], origin.z, z))
                return fail("/worldborder center <x> <z>");
            out.value = x;
            out.value2 = z;
            return out;
        }
        if (sub == "damage" || sub == "warning")
        {
            const std::string what = words.size() > 2 ? lower(words[2]) : std::string();
            const bool damage = sub == "damage";
            if (words.size() != 4 || (damage ? (what != "amount" && what != "buffer") : (what != "distance" && what != "time")))
                return fail(damage ? "/worldborder damage <amount|buffer> <value>" : "/worldborder warning <distance|time> <value>");
            out.sub = sub + " " + what;
            if (damage)
            {
                float amount = 0.0f;
                if (!number(words[3], amount) || amount < 0.0f) return fail("/worldborder damage " + what + " <value>");
                out.value = amount;
            }
            else
            {
                int amount = 0;
                if (!whole(words[3], amount)) return fail("/worldborder warning " + what + " <value>");
                out.value = amount;
            }
            return out;
        }
        return fail(usage);
    }

    if (name == "tellraw")
    {
        if (!cheats) return deny(name);
        if (words.size() < 3) return fail("/tellraw <targets> <message>");
        const size_t afterName = trimmed.find(words[0]) + words[0].size();
        const size_t targetAt = trimmed.find(words[1], afterName);
        const std::string json = trimmed.substr(targetAt + words[1].size());
        std::string text;
        if (!textComponent(json, text)) return fail("That message isn't valid text. Try /tellraw @a {\"text\":\"Hello\"}");
        out.kind = Kind::Tellraw;
        out.text = text;
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

    {
        size_t at = 1;
        auto target = [&]() { if (at < words.size() && !words[at].empty() && words[at][0] == '@') ++at; };
        auto rest = [&](size_t from) {
            std::string text;
            for (size_t k = from; k < words.size(); ++k) text += (k > from ? " " : "") + words[k];
            return text;
        };
        auto triple = [&](size_t from, const glm::vec3& base, glm::vec3& outPos) {
            if (from + 2 >= words.size()) return false;
            return coordinate(words[from], base.x, outPos.x) && coordinate(words[from + 1], base.y, outPos.y) &&
                   coordinate(words[from + 2], base.z, outPos.z);
        };
        if (name == "me")
        {
            if (words.size() < 2) return fail("/me <action>");
            out.kind = Kind::Me;
            out.text = rest(1);
            return out;
        }
        if (name == "msg" || name == "tell" || name == "w")
        {
            if (words.size() < 3) return fail("/msg <player> <message>");
            out.kind = Kind::Whisper;
            out.sub = words[1];
            out.text = rest(2);
            return out;
        }
        if (name == "list")
        {
            out.kind = Kind::List;
            return out;
        }
        if (name == "weather")
        {
            if (!cheats) return deny(name);
            if (words.size() < 2) return fail("/weather clear|rain|thunder [seconds]");
            const std::string kind = lower(words[1]);
            if (kind != "clear" && kind != "rain" && kind != "thunder") return fail("No weather called " + words[1] + ".");
            out.kind = Kind::Weather;
            out.sub = kind;
            out.seconds = 0.0f;
            if (words.size() >= 3) number(words[2], out.seconds);
            out.text = kind == "clear" ? "Set the weather to clear" : kind == "rain" ? "Set the weather to rain" : "Set the weather to rain & thunder";
            return out;
        }
        if (name == "difficulty")
        {
            out.kind = Kind::Difficulty;
            out.mode = -1;
            if (words.size() >= 2)
            {
                if (!cheats) return deny(name);
                const std::string level = lower(words[1]);
                out.mode = level == "peaceful" || level == "0" ? 0 : level == "easy" || level == "1" ? 1
                         : level == "normal" || level == "2" ? 2 : level == "hard" || level == "3" ? 3 : -2;
                if (out.mode == -2) return fail("No difficulty called " + words[1] + ".");
                out.text = "The difficulty has been set to " + level;
            }
            return out;
        }
        if (name == "xp" || name == "experience")
        {
            if (!cheats) return deny(name);
            std::string action = words.size() >= 2 ? lower(words[1]) : "";
            size_t index = 2;
            if (action != "add" && action != "set" && action != "query") { action = "add"; index = 1; }
            at = index;
            target();
            index = at;
            if (action == "query")
            {
                out.kind = Kind::Experience;
                out.sub = "query";
                out.mode = index < words.size() && lower(words[index]) == "levels" ? 1 : 0;
                return out;
            }
            if (index >= words.size()) return fail("/xp add|set <amount> [points|levels]");
            std::string amount = words[index];
            bool levels = false;
            if (!amount.empty() && (amount.back() == 'L' || amount.back() == 'l')) { levels = true; amount.pop_back(); }
            float value = 0.0f;
            if (!number(amount, value)) return fail("/xp wants a number.");
            if (index + 1 < words.size() && lower(words[index + 1]) == "levels") levels = true;
            out.kind = Kind::Experience;
            out.sub = action;
            out.count = static_cast<int>(value);
            out.mode = levels ? 1 : 0;
            out.text = (action == "set" ? "Set " : "Gave ") + std::to_string(out.count) + (levels ? " experience levels" : " experience points");
            return out;
        }
        if (name == "clear")
        {
            if (!cheats) return deny(name);
            target();
            out.kind = Kind::Clear;
            out.item = Blocks::Air;
            out.count = -1;
            if (at < words.size())
            {
                out.item = lookUp(words[at]);
                if (out.item == Blocks::Air) return fail("Nothing called " + words[at] + ".");
                if (at + 1 < words.size())
                {
                    float most = 0.0f;
                    if (number(words[at + 1], most)) out.count = static_cast<int>(most);
                }
            }
            return out;
        }
        if (name == "setworldspawn" || name == "spawnpoint")
        {
            if (!cheats) return deny(name);
            if (name == "spawnpoint") target();
            out.kind = name == "setworldspawn" ? Kind::SetWorldSpawn : Kind::SpawnPoint;
            out.position = origin;
            if (at < words.size() && !triple(at, origin, out.position)) return fail("/" + name + " [x y z]");
            out.text = "Set the spawn point to " + std::to_string(static_cast<int>(std::floor(out.position.x))) + ", " +
                       std::to_string(static_cast<int>(std::floor(out.position.y))) + ", " +
                       std::to_string(static_cast<int>(std::floor(out.position.z)));
            return out;
        }
        if (name == "defaultgamemode")
        {
            if (!cheats) return deny(name);
            if (words.size() < 2) return fail("/defaultgamemode survival|creative|adventure|spectator");
            const std::string wanted = lower(words[1]);
            out.mode = wanted == "creative" || wanted == "1" ? static_cast<int>(GameMode::Creative)
                     : wanted == "adventure" || wanted == "2" ? static_cast<int>(GameMode::Adventure)
                     : wanted == "spectator" || wanted == "3" ? static_cast<int>(GameMode::Spectator) : static_cast<int>(GameMode::Survival);
            out.kind = Kind::DefaultGameMode;
            out.text = "The default game mode is now " + wanted;
            return out;
        }
        if (name == "fill")
        {
            if (!cheats) return deny(name);
            if (words.size() < 8 || !triple(1, origin, out.position) || !triple(4, origin, out.position2))
                return fail("/fill <x1> <y1> <z1> <x2> <y2> <z2> <block> [replace|keep|destroy|hollow|outline]");
            out.item = lookUp(words[7]);
            if (words[7] == "air" || words[7] == "minecraft:air") out.item = Blocks::Air;
            else if (out.item == Blocks::Air || isItem(out.item)) return fail("No block called " + words[7] + ".");
            out.sub = words.size() >= 9 ? lower(words[8]) : "replace";
            out.kind = Kind::Fill;
            return out;
        }
        if (name == "clone")
        {
            if (!cheats) return deny(name);
            if (words.size() < 10 || !triple(1, origin, out.position) || !triple(4, origin, out.position2) || !triple(7, origin, out.position3))
                return fail("/clone <x1> <y1> <z1> <x2> <y2> <z2> <x> <y> <z>");
            out.kind = Kind::Clone;
            return out;
        }
        if (name == "time" && words.size() >= 2 && (lower(words[1]) == "add" || lower(words[1]) == "query"))
        {
            if (!cheats) return deny(name);
            if (lower(words[1]) == "query")
            {
                out.kind = Kind::TimeQuery;
                out.sub = words.size() >= 3 ? lower(words[2]) : "daytime";
                return out;
            }
            if (words.size() < 3) return fail("/time add <ticks>");
            std::string amount = words[2];
            float scale = 1.0f;
            if (!amount.empty() && (amount.back() == 'd' || amount.back() == 's' || amount.back() == 't'))
            {
                scale = amount.back() == 'd' ? 24000.0f : amount.back() == 's' ? 20.0f : 1.0f;
                amount.pop_back();
            }
            float ticks = 0.0f;
            if (!number(amount, ticks)) return fail("/time add wants a number.");
            out.kind = Kind::TimeAdd;
            out.value = ticks * scale;
            out.text = "Set the time";
            return out;
        }
        if (name == "title")
        {
            if (!cheats) return deny(name);
            target();
            if (at >= words.size()) return fail("/title <player> title|subtitle|actionbar|clear|reset <text>");
            out.kind = Kind::Title;
            out.sub = lower(words[at]);
            std::string raw = rest(at + 1);
            std::string shown;
            out.text = textComponent(raw, shown) ? shown : raw;
            return out;
        }
        if (name == "enchant")
        {
            if (!cheats) return deny(name);
            target();
            if (at >= words.size()) return fail("/enchant <player> <enchantment> [level]");
            std::string wanted = lower(words[at]);
            if (wanted.rfind("minecraft:", 0) == 0) wanted = wanted.substr(10);
            int found = -1;
            for (int k = 0; k < static_cast<int>(Enchanting::Kind::Count); ++k)
            {
                if (Enchanting::idOf(static_cast<Enchanting::Kind>(k)) == wanted) found = k;
            }
            if (found < 0) return fail("No enchantment called " + words[at] + ".");
            out.kind = Kind::Enchant;
            out.effect = found;
            out.amplifier = 1;
            if (at + 1 < words.size())
            {
                float level = 1.0f;
                if (number(words[at + 1], level)) out.amplifier = static_cast<int>(level);
            }
            return out;
        }
        if (name == "locate")
        {
            if (!cheats) return deny(name);
            if (words.size() < 3) return fail("/locate structure|biome <name>");
            out.kind = Kind::Locate;
            out.sub = lower(words[1]);
            out.text = lower(words[2]);
            if (out.text.rfind("minecraft:", 0) == 0) out.text = out.text.substr(10);
            return out;
        }
        if (name == "attribute")
        {
            if (!cheats) return deny(name);
            out.kind = Kind::AttributeCommand;
            out.args.assign(words.begin() + 1, words.end());
            return out;
        }
        if (name == "scoreboard" || name == "team" || name == "tag" || name == "trigger")
        {
            if (!cheats && name != "trigger") return deny(name);
            out.kind = Kind::ScoreboardCommand;
            out.sub = name;
            out.args.assign(words.begin() + 1, words.end());
            return out;
        }
        if (name == "damage")
        {
            if (!cheats) return deny(name);
            target();
            if (at >= words.size()) return fail("/damage <target> <amount>");
            float amount = 0.0f;
            if (!number(words[at], amount)) return fail("/damage wants a number.");
            out.kind = Kind::Damage;
            out.count = static_cast<int>(amount);
            out.text = "Applied " + std::to_string(out.count) + " damage";
            return out;
        }
    }

    if (name == "gamerule")
    {
        if (!cheats) return deny(name);
        if (words.size() < 2) return fail("/gamerule wants the name of a rule.");
        GameRules::Rule rule;
        if (!GameRules::find(words[1], rule)) return fail("Incorrect argument for command: " + words[1]);
        out.mob = static_cast<int>(rule);
        if (words.size() < 3)
        {
            out.kind = Kind::GameRuleQuery;
            return out;
        }
        const GameRules::Info& info = GameRules::info(rule);
        int value = 0;
        if (info.flag)
        {
            const std::string word = lower(words[2]);
            if (word != "true" && word != "false") return fail("Invalid boolean, expected 'true' or 'false' but found '" + words[2] + "'");
            value = word == "true" ? 1 : 0;
        }
        else
        {
            float amount = 0.0f;
            if (!number(words[2], amount)) return fail("Invalid integer '" + words[2] + "'");
            value = static_cast<int>(amount);
        }
        out.kind = Kind::GameRuleSet;
        out.count = value;
        out.text = std::string("Gamerule ") + info.name + " is now set to: " + (info.flag ? (value ? "true" : "false") : std::to_string(value));
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

std::vector<std::string> Chat::suggest(const std::string& draft)
{
    std::vector<std::string> out;
    if (draft.empty() || draft[0] != '/') return out;
    const std::string body = draft.substr(1);
    const size_t lastSpace = body.rfind(' ');
    const std::string word = lower(lastSpace == std::string::npos ? body : body.substr(lastSpace + 1));
    const std::vector<std::string> words = split(body);
    int index = static_cast<int>(words.size());
    if (!body.empty() && body.back() != ' ') --index;
    if (index < 0) index = 0;
    const std::string name = words.empty() ? std::string() : lower(words[0]);
    const std::string sub = words.size() > 1 ? lower(words[1]) : std::string();

    static const std::vector<std::string> COMMANDS = {
        "clear", "clone", "damage", "defaultgamemode", "difficulty", "effect", "enchant", "experience", "fill", "gamemode",
        "gamerule", "give", "help", "kill", "list", "locate", "me", "msg", "say", "seed", "setblock", "setworldspawn",
        "spawnpoint", "summon", "tell", "tellraw", "time", "title", "tp", "w", "weather", "worldborder", "xp",
        "scoreboard", "team", "tag", "trigger", "attribute" };
    static std::vector<std::string> items, blocks, mobs, effects, enchants, rules, biomes;
    if (items.empty())
    {
        for (int id = 1; isBlockId(id); id = nextBlockId(id))
        {
            const StackId stack = static_cast<StackId>(id);
            if (!isObtainable(asBlock(stack))) continue;
            std::string label = lower(displayName(stack));
            std::replace(label.begin(), label.end(), ' ', '_');
            items.push_back(label);
            blocks.push_back(label);
        }
        for (int id = Items::FIRST; id < static_cast<int>(Items::Count); ++id)
        {
            std::string label = lower(displayName(static_cast<StackId>(id)));
            std::replace(label.begin(), label.end(), ' ', '_');
            items.push_back(label);
        }
        for (int m = 0; m < static_cast<int>(MobId::Count); ++m)
        {
            std::string label = lower(mobType(static_cast<MobId>(m)).name);
            std::replace(label.begin(), label.end(), ' ', '_');
            mobs.push_back(label);
        }
        for (int k = 0; k < Effects::KIND_COUNT; ++k) effects.push_back(Effects::id(static_cast<Effects::Kind>(k)));
        for (int k = 0; k < Enchanting::KIND_COUNT; ++k) enchants.push_back(Enchanting::idOf(static_cast<Enchanting::Kind>(k)));
        for (int r = 0; r < GameRules::COUNT; ++r) rules.push_back(GameRules::info(static_cast<GameRules::Rule>(r)).name);
        for (int b = 0; b < Biomes::COUNT; ++b)
        {
            std::string id = Biomes::info(static_cast<Biomes::Kind>(b)).id;
            if (id.rfind("minecraft:", 0) == 0) id = id.substr(10);
            biomes.push_back(id);
        }
        for (auto* list : { &items, &blocks, &mobs, &effects, &enchants, &rules, &biomes })
        {
            std::sort(list->begin(), list->end());
            list->erase(std::unique(list->begin(), list->end()), list->end());
        }
    }
    static const std::vector<std::string> SELECTORS = { "@a", "@e", "@p", "@r", "@s" };
    static const std::vector<std::string> MODES = { "adventure", "creative", "spectator", "survival" };
    static const std::vector<std::string> STRUCTURES = { "ancient_city", "bastion_remnant", "desert_pyramid", "end_city", "igloo",
        "jungle_pyramid", "mansion", "monument", "pillager_outpost", "ruined_portal", "shipwreck", "stronghold", "swamp_hut",
        "trial_chambers", "village" };

    const std::vector<std::string>* pool = nullptr;
    std::vector<std::string> local;
    auto use = [&](std::initializer_list<const char*> list) { for (const char* entry : list) local.emplace_back(entry); pool = &local; };
    if (index == 0) pool = &COMMANDS;
    else if (!word.empty() && word[0] == '@') pool = &SELECTORS;
    else if (name == "give" || (name == "clear" && index >= 2)) pool = &items;
    else if (name == "summon" && index == 1) pool = &mobs;
    else if (name == "setblock" && index == 4) pool = &blocks;
    else if (name == "fill" && index == 7) pool = &blocks;
    else if (name == "fill" && index == 8) use({ "destroy", "hollow", "keep", "outline", "replace" });
    else if (name == "effect" && index == 1) use({ "clear", "give" });
    else if (name == "effect" && index >= 2) pool = &effects;
    else if (name == "enchant" && index >= 2) pool = &enchants;
    else if ((name == "gamemode" || name == "gm" || name == "defaultgamemode") && index == 1) pool = &MODES;
    else if (name == "weather" && index == 1) use({ "clear", "rain", "thunder" });
    else if (name == "difficulty" && index == 1) use({ "easy", "hard", "normal", "peaceful" });
    else if (name == "time" && index == 1) use({ "add", "query", "set" });
    else if (name == "time" && index == 2 && sub == "set") use({ "day", "midnight", "night", "noon" });
    else if (name == "time" && index == 2 && sub == "query") use({ "day", "daytime", "gametime" });
    else if ((name == "xp" || name == "experience") && index == 1) use({ "add", "query", "set" });
    else if (name == "locate" && index == 1) use({ "biome", "structure" });
    else if (name == "locate" && index == 2) pool = sub == "biome" ? &biomes : &STRUCTURES;
    else if (name == "gamerule" && index == 1) pool = &rules;
    else if (name == "gamerule" && index == 2) use({ "false", "true" });
    else if (name == "title" && index == 2) use({ "actionbar", "clear", "reset", "subtitle", "title" });
    else if (name == "worldborder" && index == 1) use({ "add", "center", "damage", "get", "set", "warning" });
    else if (name == "scoreboard" && index == 1) use({ "objectives", "players" });
    else if (name == "scoreboard" && index == 2 && sub == "objectives") use({ "add", "list", "modify", "remove", "setdisplay" });
    else if (name == "scoreboard" && index == 2 && sub == "players") use({ "add", "enable", "get", "list", "operation", "remove", "reset", "set" });
    else if (name == "scoreboard" && index == 4 && words.size() > 2 && lower(words[2]) == "add") use({ "air", "armor", "deathCount", "dummy", "food", "health", "level", "playerKillCount", "totalKillCount", "trigger", "xp" });
    else if (name == "scoreboard" && index == 3 && words.size() > 2 && lower(words[2]) == "setdisplay") use({ "below_name", "list", "sidebar" });
    else if (name == "team" && index == 1) use({ "add", "empty", "join", "leave", "list", "modify", "remove" });
    else if (name == "team" && index == 3 && sub == "modify") use({ "collisionRule", "color", "deathMessageVisibility", "displayName", "friendlyFire", "nametagVisibility", "prefix", "seeFriendlyInvisibles", "suffix" });
    else if (name == "tag" && index == 2) use({ "add", "list", "remove" });
    else if (name == "attribute" && index == 2)
    {
        for (int a = 0; a < Attributes::COUNT; ++a) local.emplace_back(Attributes::info(static_cast<Attributes::Kind>(a)).id);
        pool = &local;
    }
    else if (name == "attribute" && index == 3) use({ "base", "get", "modifier" });
    else if (name == "attribute" && index == 4 && words.size() > 3 && lower(words[3]) == "base") use({ "get", "reset", "set" });
    else if (name == "attribute" && index == 4 && words.size() > 3 && lower(words[3]) == "modifier") use({ "add", "remove", "value" });
    else if (name == "attribute" && index == 7) use({ "add_multiplied_base", "add_multiplied_total", "add_value" });
    if (!pool) return out;
    std::string bare = word;
    if (bare.rfind("minecraft:", 0) == 0) bare = bare.substr(10);
    for (const std::string& entry : *pool)
        if (entry.rfind(bare, 0) == 0 && entry != bare) out.push_back(entry);
    if (out.size() > 50) out.resize(50);
    return out;
}

std::string Chat::completeWith(const std::string& draft, const std::string& choice)
{
    const size_t lastSpace = draft.rfind(' ');
    if (lastSpace == std::string::npos) return "/" + choice;
    return draft.substr(0, lastSpace + 1) + choice;
}
