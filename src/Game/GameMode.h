#pragma once
#include <cstdint>

enum class GameMode : uint8_t
{
    Survival = 0,
    Creative = 1,
    Hardcore = 2
};

inline const char* gameModeName(GameMode mode)
{
    switch (mode)
    {
        case GameMode::Survival: return "Survival";
        case GameMode::Creative: return "Creative";
        case GameMode::Hardcore: return "Hardcore";
    }
    return "Unknown";
}

// Creative players fly, mine instantly and never take damage.
inline bool modeIsCreative(GameMode mode) { return mode == GameMode::Creative; }

// Hardcore is survival with one life: dying ends the world for good.
inline bool modeIsHardcore(GameMode mode) { return mode == GameMode::Hardcore; }

inline bool modeTakesDamage(GameMode mode) { return mode != GameMode::Creative; }
