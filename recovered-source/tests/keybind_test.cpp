#include "Core/Keybinds.h"
#include <SDL.h>
#include <cstdio>
#include <fstream>

static int failures = 0;

static void check(bool ok, const char* what)
{
    std::printf("  %s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++failures;
}

int main()
{
    const std::string path = "keybinds_test.txt";

    std::printf("defaults\n");
    Keybinds fresh;
    check(fresh.device(Keybinds::Sneak) == Keybinds::Keyboard &&
          fresh.code(Keybinds::Sneak) == SDL_SCANCODE_Z, "sneak defaults to Z");
    check(fresh.code(Keybinds::Sprint) == SDL_SCANCODE_LSHIFT, "sprint defaults to Left Shift");
    check(fresh.device(Keybinds::Attack) == Keybinds::Mouse &&
          fresh.code(Keybinds::Attack) == SDL_BUTTON_LEFT, "attack defaults to left mouse");

    std::printf("a file on disk overrides the defaults\n");
    {
        std::ofstream f(path, std::ios::trunc);
        f << "sneak 0 " << SDL_SCANCODE_C << "\n";
        f << "jump 0 " << SDL_SCANCODE_T << "\n";
        f << "attack 1 " << static_cast<int>(SDL_BUTTON_RIGHT) << "\n";
        f << "garbage line that should be skipped\n";
        f << "notanaction 0 5\n";
    }
    Keybinds loaded;
    loaded.load(path);
    check(loaded.code(Keybinds::Sneak) == SDL_SCANCODE_C, "sneak picked up C from the file");
    check(loaded.code(Keybinds::Jump) == SDL_SCANCODE_T, "jump picked up T from the file");
    check(loaded.device(Keybinds::Attack) == Keybinds::Mouse &&
          loaded.code(Keybinds::Attack) == SDL_BUTTON_RIGHT, "attack moved to right mouse");
    check(loaded.code(Keybinds::Sprint) == SDL_SCANCODE_LSHIFT,
          "sprint, absent from the file, kept its default");
    check(loaded.code(Keybinds::Forward) == SDL_SCANCODE_W,
          "forward, absent from the file, kept its default");

    std::printf("save then load returns the same bindings\n");
    loaded.set(Keybinds::Fly, Keybinds::Keyboard, SDL_SCANCODE_G);
    check(loaded.save(path), "save wrote the file");
    Keybinds reloaded;
    reloaded.load(path);
    bool identical = true;
    for (int i = 0; i < Keybinds::Count; ++i)
    {
        const auto a = static_cast<Keybinds::Action>(i);
        if (reloaded.device(a) != loaded.device(a) || reloaded.code(a) != loaded.code(a))
        {
            identical = false;
            std::printf("    mismatch on %s\n", Keybinds::actionId(a));
        }
    }
    check(identical, "every one of the 21 bindings round-tripped");

    std::printf("a missing file leaves the defaults alone\n");
    Keybinds absent;
    absent.load("no_such_file_here.txt");
    check(absent.code(Keybinds::Sneak) == SDL_SCANCODE_Z, "sneak still Z after a failed load");

    std::printf("reset restores the defaults\n");
    loaded.resetToDefaults();
    check(loaded.code(Keybinds::Sneak) == SDL_SCANCODE_Z, "reset put sneak back to Z");
    check(loaded.code(Keybinds::Fly) == SDL_SCANCODE_F, "reset put fly back to F");

    std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
    return failures ? 1 : 0;
}
