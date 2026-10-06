#pragma once
#include <string>

// Rebindable controls, stored as /world/keybinds.txt.
//
// The file is one binding per line: "<action> <device> <code>", where device
// 0 is an SDL scancode and 1 a mouse button. Actions missing from the file
// keep their default, so a partial or older file still loads, and the file is
// only written once something has actually been rebound.
//
//   forward 0 26
//   sneak 0 29
//   attack 1 1
class Keybinds
{
public:
    enum Action
    {
        Forward, Back, Left, Right,
        Jump, Sneak, Sprint,
        Attack, Use, Pick,
        Drop, Inventory, Fly,
        Screenshot, Debug, HideHud, Fullscreen,
        Pbr, Mute, DistanceDown, DistanceUp,
        Count
    };

    enum Device : int { Keyboard = 0, Mouse = 1 };

    Keybinds();

    // Reads the file if it exists. Missing or unreadable means defaults.
    void load(const std::string& path);

    // Writes every binding. Called after a rebind, not on every frame.
    bool save(const std::string& path) const;

    void set(Action action, Device device, int code);
    void resetToDefaults();

    Device device(Action action) const { return m_bindings[action].device; }
    int code(Action action) const { return m_bindings[action].code; }

    // Stable identifier used in the file, e.g. "forward".
    static const char* actionId(Action action);
    // Human label for the controls screen, e.g. "Walk Forward".
    static const char* actionLabel(Action action);
    // How a binding reads on screen, e.g. "Left Shift" or "Left Click".
    std::string bindingLabel(Action action) const;

    bool dirty() const { return m_dirty; }

private:
    struct Binding
    {
        Device device = Keyboard;
        int code = 0;
    };

    Binding m_bindings[Count];
    bool m_dirty = false;
};
