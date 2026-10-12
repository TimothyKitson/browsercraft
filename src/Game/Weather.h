#pragma once
#include <cstdint>

// Rain, snow and thunderstorms: how long each lasts, how dark it gets,
// and when the lightning comes.
//
// Minecraft keeps its weather as a pair of countdowns rather than as a
// forecast, and so does this: when the clock runs out the sky changes to
// whatever it was not. That is the whole of it, which is why all of it
// is pure arithmetic over a seed and --selftest can run a week of
// weather through in a few thousand steps and check it behaves.
namespace Weather
{
    enum class Kind : uint8_t
    {
        Clear,
        Rain,    // falling water, a dimmed sky
        Storm,   // the same, darker, with lightning in it
    };

    // Minecraft's own spans, in seconds. A day here is twenty minutes,
    // the same as there, so these carry over unchanged: clear spells run
    // from half a day to nearly nine, and wet ones are shorter.
    constexpr float CLEAR_MIN_SECONDS = 600.0f;
    constexpr float CLEAR_MAX_SECONDS = 10800.0f;
    constexpr float RAIN_MIN_SECONDS = 600.0f;
    constexpr float RAIN_MAX_SECONDS = 7800.0f;
    constexpr float STORM_MIN_SECONDS = 180.0f;
    constexpr float STORM_MAX_SECONDS = 780.0f;

    // One spell in four is a thunderstorm rather than plain rain.
    constexpr int STORM_IN = 4;

    // How long the sky takes to darken when the weather turns, and to
    // clear again. Minecraft fades it rather than switching, which is
    // most of why weather there feels like weather.
    constexpr float FADE_SECONDS = 5.0f;

    // Seconds between flashes in a storm, picked between these.
    constexpr float FLASH_MIN_SECONDS = 4.0f;
    constexpr float FLASH_MAX_SECONDS = 26.0f;

    // How long a flash lights the sky for.
    constexpr float FLASH_SECONDS = 0.45f;

    struct State
    {
        Kind kind = Kind::Clear;
        float secondsLeft = 0.0f;

        // 0 when the sky is clear, 1 when the weather is fully in.
        // Climbs while it is raining and falls once it stops, so the two
        // need not be in step: a spell that ends while the sky is still
        // darkening simply turns round and lightens from where it got to.
        float depth = 0.0f;

        float secondsToFlash = 0.0f;
        float flashLeft = 0.0f;   // counting down through a flash

        bool raining() const { return kind != Kind::Clear; }
        bool storming() const { return kind == Kind::Storm; }
        bool flashing() const { return flashLeft > 0.0f; }
    };

    // One step. `seed` is stirred as it goes, so a caller keeps one and
    // passes it every time.
    //
    // Returns true on the step a flash begins, which is the caller's cue
    // to play the thunder.
    bool step(State& state, float deltaTime, uint32_t& seed);

    // Starts a spell now, as the /weather command does.
    void begin(State& state, Kind kind, uint32_t& seed);

    // What the daylight is multiplied by. Rain takes a fifth off and a
    // storm nearly a third, which is enough that a storm at noon lets
    // the things that only come out at night come out.
    float lightScale(const State& state);

    // Added to the daylight while a flash is in the sky. A flash is
    // brief and very bright, so it is added rather than multiplied.
    float flashBoost(const State& state);
}
