#pragma once
#include <SDL.h>
#include <array>
#include <string>

// Every rebindable control, with Minecraft's own defaults.
//
// A binding is a keyboard scancode or a mouse button, because Minecraft
// lets you put "mine" on a key and "jump" on a mouse button if you want
// to, and a control scheme that only half works is worse than none.
enum class Action
{
    Forward, Back, Left, Right,
    Jump, Sneak, Sprint,
    Attack, Use, PickBlock,
    Drop, Inventory,
    Fly,
    Screenshot, Debug, HideHud, Fullscreen,
    FancyLighting, Mute,
    RenderDistanceDown, RenderDistanceUp,
    Count
};

struct Binding
{
    bool mouse = false;   // true: code is an SDL_BUTTON_*; false: an SDL_Scancode
    int code = 0;

    bool operator==(const Binding& o) const { return mouse == o.mouse && code == o.code; }
    bool valid() const { return code != 0; }
};

class Input;

class KeyBindings
{
public:
    KeyBindings();

    // Held down right now / pressed this frame.
    bool down(const Input& input, Action action) const;
    bool pressed(const Input& input, Action action) const;

    const Binding& binding(Action action) const { return m_bindings[index(action)]; }
    void setBinding(Action action, const Binding& binding);
    void resetToDefaults();

    // "LEFT SHIFT", "LEFT CLICK", "F3" -- what the controls screen shows.
    static std::string describe(const Binding& binding);
    static const char* name(Action action);

    // True when some other action already uses this binding, which the
    // controls screen marks in red the way Minecraft does.
    bool conflicts(Action action) const;

    // Stored next to the save so it travels with the browser profile.
    void load(const std::string& directory);
    void save(const std::string& directory) const;

private:
    static constexpr size_t COUNT = static_cast<size_t>(Action::Count);
    static size_t index(Action action) { return static_cast<size_t>(action); }

    std::array<Binding, COUNT> m_bindings{};
};
