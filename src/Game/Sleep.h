#pragma once

// Going to bed.
//
// The rule about when you may is a pure function of the time of day, so
// --selftest can walk a whole day round and check where the window opens
// and shuts without a bed, a world or a clock.
namespace Sleep
{
    // Dawn, which is where a night's sleep puts the clock.
    constexpr float MORNING = 0.25f;

    // Minecraft lets you sleep from dusk until dawn. timeOfDay runs
    // 0.25 sunrise, 0.5 noon, 0.75 sunset, so the night is the stretch
    // that wraps past 1.0 and round to the next sunrise.
    constexpr float DUSK = 0.74f;
    constexpr float DAWN = 0.22f;

    inline bool isNight(float timeOfDay)
    {
        return timeOfDay >= DUSK || timeOfDay < DAWN;
    }

    // What the clock becomes after a night in a bed.
    inline float wakeTime() { return MORNING; }
}
