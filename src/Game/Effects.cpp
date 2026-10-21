#include "Effects.h"
#include <algorithm>

namespace
{
    int index(Effects::Kind kind) { return static_cast<int>(kind); }
}

const char* Effects::name(Kind kind)
{
    switch (kind)
    {
        case Kind::Speed:          return "Speed";
        case Kind::Slowness:       return "Slowness";
        case Kind::Strength:       return "Strength";
        case Kind::Weakness:       return "Weakness";
        case Kind::Regeneration:   return "Regeneration";
        case Kind::Poison:         return "Poison";
        case Kind::FireResistance: return "Fire Resistance";
        case Kind::WaterBreathing: return "Water Breathing";
        case Kind::NightVision:    return "Night Vision";
        case Kind::JumpBoost:      return "Jump Boost";
        case Kind::BadOmen:        return "Bad Omen";
        case Kind::Count:          break;
    }
    return "";
}

bool Effects::isHarmful(Kind kind)
{
    // Bad omen does nothing to the body at all. It counts as harmful
    // because of what it is for and because the UI should paint it the
    // colour of trouble.
    return kind == Kind::Slowness || kind == Kind::Weakness ||
           kind == Kind::Poison || kind == Kind::BadOmen;
}

bool Effects::Set::has(Kind kind) const
{
    return slots[index(kind)].secondsLeft > 0.0f;
}

int Effects::Set::amplifier(Kind kind) const
{
    return has(kind) ? slots[index(kind)].amplifier : -1;
}

float Effects::Set::secondsLeft(Kind kind) const
{
    return has(kind) ? slots[index(kind)].secondsLeft : 0.0f;
}

bool Effects::Set::any() const
{
    for (int i = 0; i < KIND_COUNT; ++i)
        if (slots[i].secondsLeft > 0.0f) return true;
    return false;
}

void Effects::give(Set& set, Kind kind, int amplifier, float seconds)
{
    if (seconds <= 0.0f) return;
    amplifier = std::max(0, amplifier);

    Active& slot = set.slots[index(kind)];

    if (slot.secondsLeft > 0.0f)
    {
        // Weaker than what is already running, and no longer: ignored
        // outright, so a thrown-together drink cannot cut short a good one.
        if (amplifier < slot.amplifier) return;
        if (amplifier == slot.amplifier)
        {
            slot.secondsLeft = std::max(slot.secondsLeft, seconds);
            return;
        }
    }

    slot.amplifier = amplifier;
    slot.secondsLeft = seconds;
    slot.pulseClock = 0.0f;
}

void Effects::remove(Set& set, Kind kind)
{
    set.slots[index(kind)] = Active{};
}

void Effects::clear(Set& set)
{
    for (int i = 0; i < KIND_COUNT; ++i) set.slots[i] = Active{};
}

Effects::Tick Effects::step(Set& set, float deltaTime)
{
    Tick tick;
    if (deltaTime <= 0.0f) return tick;

    for (int i = 0; i < KIND_COUNT; ++i)
    {
        Active& slot = set.slots[i];
        if (slot.secondsLeft <= 0.0f) continue;

        const Kind kind = static_cast<Kind>(i);

        // The clock runs only over the part of the step the effect was
        // still up for. A long frame must not pay out pulses the player
        // did not live through.
        const float alive = std::min(deltaTime, slot.secondsLeft);

        if (kind == Kind::Regeneration || kind == Kind::Poison)
        {
            const float base = (kind == Kind::Regeneration) ? REGEN_INTERVAL
                                                            : POISON_INTERVAL;
            // Each step of the amplifier halves the wait.
            const float interval = std::max(0.05f, base / static_cast<float>(1 << slot.amplifier));

            slot.pulseClock += alive;
            while (slot.pulseClock >= interval)
            {
                slot.pulseClock -= interval;
                if (kind == Kind::Regeneration) ++tick.healPulses;
                else ++tick.poisonPulses;
            }
        }

        slot.secondsLeft -= deltaTime;
        if (slot.secondsLeft <= 0.0f) slot = Active{};
    }

    return tick;
}

float Effects::speedMultiplier(const Set& set)
{
    float multiplier = 1.0f;
    if (set.has(Kind::Speed))
        multiplier *= 1.0f + 0.20f * static_cast<float>(set.amplifier(Kind::Speed) + 1);
    if (set.has(Kind::Slowness))
        multiplier *= std::max(0.2f,
            1.0f - 0.15f * static_cast<float>(set.amplifier(Kind::Slowness) + 1));
    return multiplier;
}

int Effects::attackBonus(const Set& set)
{
    int bonus = 0;
    if (set.has(Kind::Strength)) bonus += 3 * (set.amplifier(Kind::Strength) + 1);
    if (set.has(Kind::Weakness)) bonus -= 4 * (set.amplifier(Kind::Weakness) + 1);
    return bonus;
}

float Effects::jumpMultiplier(const Set& set)
{
    if (!set.has(Kind::JumpBoost)) return 1.0f;
    return 1.0f + 0.10f * static_cast<float>(set.amplifier(Kind::JumpBoost) + 1);
}

bool Effects::fireImmune(const Set& set) { return set.has(Kind::FireResistance); }
bool Effects::breathesWater(const Set& set) { return set.has(Kind::WaterBreathing); }

float Effects::nightVisionFloor(const Set& set)
{
    return set.has(Kind::NightVision) ? 0.72f : 0.0f;
}
