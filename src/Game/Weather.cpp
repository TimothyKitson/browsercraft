#include "Weather.h"
#include <algorithm>

namespace Weather
{
    namespace
    {
        uint32_t stir(uint32_t& seed)
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            return seed;
        }

        float between(uint32_t& seed, float low, float high)
        {
            const float t = static_cast<float>(stir(seed) & 0xFFFFFF) /
                            static_cast<float>(0x1000000);
            return low + t * (high - low);
        }

        void roll(State& state, uint32_t& seed)
        {
            if (state.kind == Kind::Clear)
            {
                // A wet spell, and one in four of those has thunder.
                const bool thunder = (stir(seed) % static_cast<uint32_t>(STORM_IN)) == 0u;
                state.kind = thunder ? Kind::Storm : Kind::Rain;
                state.secondsLeft = thunder ? between(seed, STORM_MIN_SECONDS, STORM_MAX_SECONDS)
                                            : between(seed, RAIN_MIN_SECONDS, RAIN_MAX_SECONDS);
                state.secondsToFlash = between(seed, FLASH_MIN_SECONDS, FLASH_MAX_SECONDS);
                return;
            }

            state.kind = Kind::Clear;
            state.secondsLeft = between(seed, CLEAR_MIN_SECONDS, CLEAR_MAX_SECONDS);
            state.flashLeft = 0.0f;
        }
    }

    void begin(State& state, Kind kind, uint32_t& seed)
    {
        state.kind = kind;
        state.flashLeft = 0.0f;

        switch (kind)
        {
        case Kind::Rain:
            state.secondsLeft = between(seed, RAIN_MIN_SECONDS, RAIN_MAX_SECONDS);
            break;
        case Kind::Storm:
            state.secondsLeft = between(seed, STORM_MIN_SECONDS, STORM_MAX_SECONDS);
            break;
        default:
            state.secondsLeft = between(seed, CLEAR_MIN_SECONDS, CLEAR_MAX_SECONDS);
            break;
        }

        state.secondsToFlash = between(seed, FLASH_MIN_SECONDS, FLASH_MAX_SECONDS);
    }

    bool step(State& state, float deltaTime, uint32_t& seed)
    {
        if (deltaTime <= 0.0f) return false;

        // A world that has never had weather starts the clock rather
        // than turning wet on its first step.
        if (state.secondsLeft <= 0.0f && state.kind == Kind::Clear && state.depth <= 0.0f)
        {
            state.secondsLeft = between(seed, CLEAR_MIN_SECONDS, CLEAR_MAX_SECONDS);
            return false;
        }

        state.secondsLeft -= deltaTime;
        if (state.secondsLeft <= 0.0f) roll(state, seed);

        // The sky follows the weather rather than jumping to it.
        const float target = state.raining() ? 1.0f : 0.0f;
        const float travel = deltaTime / FADE_SECONDS;
        state.depth = (state.depth < target) ? std::min(target, state.depth + travel)
                                             : std::max(target, state.depth - travel);

        if (state.flashLeft > 0.0f) state.flashLeft = std::max(0.0f, state.flashLeft - deltaTime);

        // Lightning only once the storm has properly arrived: a flash
        // out of a sky that is still clearing looks like a glitch.
        if (!state.storming() || state.depth < 0.5f) return false;

        state.secondsToFlash -= deltaTime;
        if (state.secondsToFlash > 0.0f) return false;

        state.secondsToFlash = between(seed, FLASH_MIN_SECONDS, FLASH_MAX_SECONDS);
        state.flashLeft = FLASH_SECONDS;
        return true;
    }

    float lightScale(const State& state)
    {
        // Interpolated by depth, so the world darkens as the cloud
        // arrives rather than the instant the clock turns over.
        const float full = (state.kind == Kind::Storm) ? 0.68f : 0.80f;
        return 1.0f - (1.0f - full) * std::clamp(state.depth, 0.0f, 1.0f);
    }

    float flashBoost(const State& state)
    {
        if (state.flashLeft <= 0.0f) return 0.0f;

        // Bright at the front and gone quickly, which is what makes it
        // read as a flash rather than a lamp being switched on.
        const float along = state.flashLeft / FLASH_SECONDS;
        return 0.85f * along * along;
    }
}
