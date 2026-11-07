#include "Effects.h"
#include <algorithm>
#include <cmath>

namespace
{
    int index(Effects::Kind kind) { return static_cast<int>(kind); }
}

unsigned int Effects::colour(Kind kind)
{
    switch (kind)
    {
        case Kind::Speed: return 0x33EBFF;
        case Kind::Slowness: return 0x8BAFE0;
        case Kind::Strength: return 0xFFC700;
        case Kind::Weakness: return 0x484D48;
        case Kind::Regeneration: return 0xCD5CAB;
        case Kind::Poison: return 0x87A363;
        case Kind::FireResistance: return 0xFF9900;
        case Kind::WaterBreathing: return 0x98DAC0;
        case Kind::NightVision: return 0xC2FF66;
        case Kind::JumpBoost: return 0xFDFF84;
        case Kind::BadOmen: return 0x0B6138;
        case Kind::Haste: return 0xD9C043;
        case Kind::Resistance: return 0x9146F0;
        case Kind::ConduitPower: return 0x1DC2D1;
        case Kind::Absorption: return 0x2552A5;
        case Kind::Hunger: return 0x587653;
        case Kind::Saturation: return 0xF82423;
        case Kind::Blindness: return 0x1F1F23;
        default: return 0x385DC6;
    }
}

const char* Effects::icon(Kind kind)
{
    switch (kind)
    {
        case Kind::Speed:          return "../mob_effect/speed";
        case Kind::Slowness:       return "../mob_effect/slowness";
        case Kind::Strength:       return "../mob_effect/strength";
        case Kind::Weakness:       return "../mob_effect/weakness";
        case Kind::Regeneration:   return "../mob_effect/regeneration";
        case Kind::Poison:         return "../mob_effect/poison";
        case Kind::FireResistance: return "../mob_effect/fire_resistance";
        case Kind::WaterBreathing: return "../mob_effect/water_breathing";
        case Kind::NightVision:    return "../mob_effect/night_vision";
        case Kind::JumpBoost:      return "../mob_effect/jump_boost";
        case Kind::BadOmen:        return "../mob_effect/bad_omen";
        case Kind::Haste:          return "../mob_effect/haste";
        case Kind::Resistance:     return "../mob_effect/resistance";
        case Kind::ConduitPower:   return "../mob_effect/conduit_power";
        case Kind::Absorption:     return "../mob_effect/absorption";
        case Kind::Hunger:         return "../mob_effect/hunger";
        case Kind::Saturation:     return "../mob_effect/saturation";
        case Kind::Blindness:      return "../mob_effect/blindness";
        case Kind::Count:          break;
    }
    return "../mob_effect/speed";
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
        case Kind::Haste:          return "Haste";
        case Kind::Resistance:     return "Resistance";
        case Kind::ConduitPower:   return "Conduit Power";
        case Kind::Absorption:     return "Absorption";
        case Kind::Hunger:         return "Hunger";
        case Kind::Saturation:     return "Saturation";
        case Kind::Blindness:      return "Blindness";
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
           kind == Kind::Poison || kind == Kind::BadOmen || kind == Kind::Hunger || kind == Kind::Blindness;
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

        if (kind == Kind::Hunger) tick.exhaustion += 0.1f * static_cast<float>(slot.amplifier + 1) * alive;
        if (kind == Kind::Saturation)
        {
            slot.pulseClock += alive;
            while (slot.pulseClock >= 0.05f)
            {
                slot.pulseClock -= 0.05f;
                tick.feedPulses += slot.amplifier + 1;
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

float Effects::digMultiplier(const Set& set)
{
    const int level = std::max(set.amplifier(Kind::Haste), set.amplifier(Kind::ConduitPower));
    if (level < 0) return 1.0f;
    return 1.0f + 0.20f * static_cast<float>(level + 1);
}

int Effects::resisted(const Set& set, int amount)
{
    if (!set.has(Kind::Resistance) || amount <= 0) return amount;
    const float kept = std::max(0.0f, 1.0f - 0.20f * static_cast<float>(set.amplifier(Kind::Resistance) + 1));
    return static_cast<int>(std::lround(static_cast<float>(amount) * kept));
}
bool Effects::breathesWater(const Set& set) { return set.has(Kind::WaterBreathing) || set.has(Kind::ConduitPower); }

float Effects::nightVisionFloor(const Set& set)
{
    return set.has(Kind::NightVision) ? 0.72f : 0.0f;
}
