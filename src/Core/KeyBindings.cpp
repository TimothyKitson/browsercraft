#include "KeyBindings.h"
#include "Input.h"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <filesystem>

namespace
{
    struct Entry
    {
        Action action;
        const char* name;
        const char* key;    // for saving: a stable name, not a number
        Binding fallback;
    };

    Binding key(SDL_Scancode code) { return Binding{ false, static_cast<int>(code) }; }
    Binding mouse(int button) { return Binding{ true, button }; }

    // Minecraft's defaults, with three additions at the bottom that
    // Minecraft keeps in Options rather than on a key. They are bound
    // here because this game has no options for them yet.
    const Entry ENTRIES[] = {
        { Action::Forward,   "WALK FORWARD",     "forward",   key(SDL_SCANCODE_W) },
        { Action::Back,      "WALK BACKWARDS",   "back",      key(SDL_SCANCODE_S) },
        { Action::Left,      "STRAFE LEFT",      "left",      key(SDL_SCANCODE_A) },
        { Action::Right,     "STRAFE RIGHT",     "right",     key(SDL_SCANCODE_D) },
        { Action::Jump,      "JUMP",             "jump",      key(SDL_SCANCODE_SPACE) },
        { Action::Sneak,     "SNEAK",            "sneak",     key(SDL_SCANCODE_LSHIFT) },
        { Action::Sprint,    "SPRINT",           "sprint",    key(SDL_SCANCODE_LCTRL) },
        { Action::Attack,    "ATTACK / MINE",    "attack",    mouse(SDL_BUTTON_LEFT) },
        { Action::Use,       "USE / PLACE",      "use",       mouse(SDL_BUTTON_RIGHT) },
        { Action::PickBlock, "PICK BLOCK",       "pick",      mouse(SDL_BUTTON_MIDDLE) },
        { Action::Drop,      "DROP ITEM",        "drop",      key(SDL_SCANCODE_Q) },
        { Action::Inventory, "INVENTORY",        "inventory", key(SDL_SCANCODE_E) },
        { Action::Fly,       "TOGGLE FLY",       "fly",       key(SDL_SCANCODE_F) },
        { Action::Perspective, "TOGGLE PERSPECTIVE", "perspective", key(SDL_SCANCODE_F5) },
        { Action::Screenshot,"SCREENSHOT",       "screenshot",key(SDL_SCANCODE_F2) },
        { Action::Debug,     "DEBUG INFO",       "debug",     key(SDL_SCANCODE_F3) },
        { Action::HideHud,   "HIDE HUD",         "hidehud",   key(SDL_SCANCODE_F1) },
        { Action::Fullscreen,"FULLSCREEN",       "fullscreen",key(SDL_SCANCODE_F11) },
        { Action::FancyLighting, "FANCY LIGHTING", "pbr",     key(SDL_SCANCODE_P) },
        { Action::Mute,      "MUTE SOUND",       "mute",      key(SDL_SCANCODE_M) },
        { Action::RenderDistanceDown, "RENDER DISTANCE -", "distdown", key(SDL_SCANCODE_LEFTBRACKET) },
        { Action::RenderDistanceUp,   "RENDER DISTANCE +", "distup",   key(SDL_SCANCODE_RIGHTBRACKET) },
    };

    static_assert(sizeof(ENTRIES) / sizeof(ENTRIES[0]) == static_cast<size_t>(Action::Count),
                  "every action needs a row in ENTRIES");

    const Entry& entry(Action action)
    {
        // The table is written in enum order, so this is a direct index;
        // the static_assert above is what keeps that true.
        return ENTRIES[static_cast<size_t>(action)];
    }
}

KeyBindings::KeyBindings() { resetToDefaults(); }

void KeyBindings::resetToDefaults()
{
    for (const Entry& e : ENTRIES)
        m_bindings[index(e.action)] = e.fallback;
}

void KeyBindings::setBinding(Action action, const Binding& binding)
{
    m_bindings[index(action)] = binding;
}

const char* KeyBindings::name(Action action) { return entry(action).name; }

bool KeyBindings::down(const Input& input, Action action) const
{
    const Binding& b = m_bindings[index(action)];
    if (!b.valid()) return false;
    return b.mouse ? input.isMouseButtonDown(static_cast<Uint8>(b.code))
                   : input.isKeyDown(static_cast<SDL_Scancode>(b.code));
}

bool KeyBindings::pressed(const Input& input, Action action) const
{
    const Binding& b = m_bindings[index(action)];
    if (!b.valid()) return false;
    return b.mouse ? input.wasMouseButtonPressed(static_cast<Uint8>(b.code))
                   : input.wasKeyPressed(static_cast<SDL_Scancode>(b.code));
}

bool KeyBindings::conflicts(Action action) const
{
    const Binding& mine = m_bindings[index(action)];
    if (!mine.valid()) return false;

    for (size_t i = 0; i < COUNT; ++i)
        if (i != index(action) && m_bindings[i] == mine) return true;

    return false;
}

std::string KeyBindings::describe(const Binding& binding)
{
    if (!binding.valid()) return "NONE";

    if (binding.mouse)
    {
        switch (binding.code)
        {
            case SDL_BUTTON_LEFT:   return "LEFT CLICK";
            case SDL_BUTTON_RIGHT:  return "RIGHT CLICK";
            case SDL_BUTTON_MIDDLE: return "MIDDLE CLICK";
            default:                return "MOUSE " + std::to_string(binding.code);
        }
    }

    // SDL's own name is already close to what a player expects ("Left
    // Shift", "F3", "Space"); it only needs upper-casing for this font.
    std::string text = SDL_GetScancodeName(static_cast<SDL_Scancode>(binding.code));
    if (text.empty()) return "KEY " + std::to_string(binding.code);
    for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

// --- persistence -------------------------------------------------------
//
// Saved by name rather than by number so the file survives the enum
// being reordered, and so it is readable if anyone wants to edit it.

void KeyBindings::load(const std::string& directory)
{
    std::FILE* file = std::fopen((directory + "/keybinds.txt").c_str(), "rb");
    if (!file) return;

    resetToDefaults();

    char line[128];
    while (std::fgets(line, sizeof(line), file))
    {
        char name[64] = { 0 };
        int isMouse = 0, code = 0;
        if (std::sscanf(line, "%63s %d %d", name, &isMouse, &code) != 3) continue;

        for (const Entry& e : ENTRIES)
            if (std::strcmp(e.key, name) == 0)
            {
                m_bindings[index(e.action)] = Binding{ isMouse != 0, code };
                break;
            }
    }

    std::fclose(file);
}

void KeyBindings::save(const std::string& directory) const
{
    // The controls screen is reachable from the title, before any world
    // has been created, so this folder may not exist yet.
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);

    std::FILE* file = std::fopen((directory + "/keybinds.txt").c_str(), "wb");
    if (!file) return;

    for (const Entry& e : ENTRIES)
    {
        const Binding& b = m_bindings[index(e.action)];
        std::fprintf(file, "%s %d %d\n", e.key, b.mouse ? 1 : 0, b.code);
    }

    std::fclose(file);
}
