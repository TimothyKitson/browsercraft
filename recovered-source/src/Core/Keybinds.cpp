#include "Keybinds.h"
#include <SDL.h>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace
{
    struct Default
    {
        Keybinds::Device device;
        int code;
        const char* id;
        const char* label;
    };

    // Scancodes rather than keycodes, so the bindings follow physical keys and
    // do not move about on a non-QWERTY layout.
    const Default DEFAULTS[Keybinds::Count] = {
        { Keybinds::Keyboard, SDL_SCANCODE_W,            "forward",    "Walk Forward" },
        { Keybinds::Keyboard, SDL_SCANCODE_S,            "back",       "Walk Backwards" },
        { Keybinds::Keyboard, SDL_SCANCODE_A,            "left",       "Strafe Left" },
        { Keybinds::Keyboard, SDL_SCANCODE_D,            "right",      "Strafe Right" },
        { Keybinds::Keyboard, SDL_SCANCODE_SPACE,        "jump",       "Jump" },
        { Keybinds::Keyboard, SDL_SCANCODE_LCTRL,        "sneak",      "Sneak" },
        { Keybinds::Keyboard, SDL_SCANCODE_LSHIFT,       "sprint",     "Sprint" },
        { Keybinds::Mouse,    SDL_BUTTON_LEFT,           "attack",     "Attack / Mine" },
        { Keybinds::Mouse,    SDL_BUTTON_RIGHT,          "use",        "Use / Place" },
        { Keybinds::Mouse,    SDL_BUTTON_MIDDLE,         "pick",       "Pick Block" },
        { Keybinds::Keyboard, SDL_SCANCODE_Q,            "drop",       "Drop Item" },
        { Keybinds::Keyboard, SDL_SCANCODE_E,            "inventory",  "Inventory" },
        { Keybinds::Keyboard, SDL_SCANCODE_1,            "hotbar1",    "Hotbar Slot 1" },
        { Keybinds::Keyboard, SDL_SCANCODE_2,            "hotbar2",    "Hotbar Slot 2" },
        { Keybinds::Keyboard, SDL_SCANCODE_3,            "hotbar3",    "Hotbar Slot 3" },
        { Keybinds::Keyboard, SDL_SCANCODE_4,            "hotbar4",    "Hotbar Slot 4" },
        { Keybinds::Keyboard, SDL_SCANCODE_5,            "hotbar5",    "Hotbar Slot 5" },
        { Keybinds::Keyboard, SDL_SCANCODE_6,            "hotbar6",    "Hotbar Slot 6" },
        { Keybinds::Keyboard, SDL_SCANCODE_7,            "hotbar7",    "Hotbar Slot 7" },
        { Keybinds::Keyboard, SDL_SCANCODE_8,            "hotbar8",    "Hotbar Slot 8" },
        { Keybinds::Keyboard, SDL_SCANCODE_9,            "hotbar9",    "Hotbar Slot 9" },
        { Keybinds::Keyboard, SDL_SCANCODE_F2,           "screenshot", "Screenshot" },
        { Keybinds::Keyboard, SDL_SCANCODE_F1,           "hidehud",    "Hide HUD" },
        { Keybinds::Keyboard, SDL_SCANCODE_F11,          "fullscreen", "Fullscreen" },
        { Keybinds::Keyboard, SDL_SCANCODE_P,            "pbr",        "Fancy Lighting" },
        { Keybinds::Keyboard, SDL_SCANCODE_M,            "mute",       "Mute Sound" },
        { Keybinds::Keyboard, SDL_SCANCODE_LEFTBRACKET,  "distdown",   "Render Distance -" },
        { Keybinds::Keyboard, SDL_SCANCODE_RIGHTBRACKET, "distup",     "Render Distance +" },
    };
}

Keybinds::Keybinds()
{
    resetToDefaults();
}

void Keybinds::resetToDefaults()
{
    for (int i = 0; i < Count; ++i)
    {
        m_bindings[i].device = DEFAULTS[i].device;
        m_bindings[i].code = DEFAULTS[i].code;
    }
    m_dirty = true;
}

const char* Keybinds::actionId(Action action)
{
    return (action >= 0 && action < Count) ? DEFAULTS[action].id : "";
}

const char* Keybinds::actionLabel(Action action)
{
    return (action >= 0 && action < Count) ? DEFAULTS[action].label : "";
}

void Keybinds::set(Action action, Device device, int code)
{
    if (action < 0 || action >= Count) return;
    m_bindings[action].device = device;
    m_bindings[action].code = code;
    m_dirty = true;
}

void Keybinds::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return; // untouched controls keep their defaults

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream parts(line);
        std::string id;
        int device = 0;
        int code = 0;
        if (!(parts >> id >> device >> code)) continue;

        for (int i = 0; i < Count; ++i)
        {
            if (id == DEFAULTS[i].id)
            {
                m_bindings[i].device = (device == Mouse) ? Mouse : Keyboard;
                m_bindings[i].code = code;
                break;
            }
        }
    }
    m_dirty = false;
}

bool Keybinds::save(const std::string& path) const
{
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open())
    {
        std::fprintf(stderr, "[Keybinds] could not write %s\n", path.c_str());
        return false;
    }

    for (int i = 0; i < Count; ++i)
        file << DEFAULTS[i].id << ' ' << static_cast<int>(m_bindings[i].device)
             << ' ' << m_bindings[i].code << '\n';

    return true;
}

std::string Keybinds::bindingLabel(Action action) const
{
    if (action < 0 || action >= Count) return "";

    const Binding& binding = m_bindings[action];
    if (binding.device == Mouse)
    {
        switch (binding.code)
        {
            case SDL_BUTTON_LEFT:   return "Left Click";
            case SDL_BUTTON_RIGHT:  return "Right Click";
            case SDL_BUTTON_MIDDLE: return "Middle Click";
            default: return "Mouse " + std::to_string(binding.code);
        }
    }

    const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(binding.code));
    return (name && *name) ? name : "Unbound";
}
