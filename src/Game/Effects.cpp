#include "Effects.h"
#include <algorithm>
#include <cmath>

namespace
{
    int index(Effects::Kind kind) { return static_cast<int>(kind); }
}


namespace
{
    struct Info
    {
        const char* id;
        const char* name;
        const char* icon;
        unsigned int colour;
        int category;
    };

    const Info INFO[Effects::KIND_COUNT] = {
        { "speed", "Speed", "../mob_effect/speed", 0x33EBFF, 1 },
        { "slowness", "Slowness", "../mob_effect/slowness", 0x8BAFE0, -1 },
        { "strength", "Strength", "../mob_effect/strength", 0xFFC700, 1 },
        { "weakness", "Weakness", "../mob_effect/weakness", 0x484D48, -1 },
        { "regeneration", "Regeneration", "../mob_effect/regeneration", 0xCD5CAB, 1 },
        { "poison", "Poison", "../mob_effect/poison", 0x87A363, -1 },
        { "fire_resistance", "Fire Resistance", "../mob_effect/fire_resistance", 0xFF9900, 1 },
        { "water_breathing", "Water Breathing", "../mob_effect/water_breathing", 0x98DAC0, 1 },
        { "night_vision", "Night Vision", "../mob_effect/night_vision", 0xC2FF66, 1 },
        { "jump_boost", "Jump Boost", "../mob_effect/jump_boost", 0xFDFF84, 1 },
        { "bad_omen", "Bad Omen", "../mob_effect/bad_omen", 0x0B6138, 0 },
        { "haste", "Haste", "../mob_effect/haste", 0xD9C043, 1 },
        { "resistance", "Resistance", "../mob_effect/resistance", 0x9146F0, 1 },
        { "conduit_power", "Conduit Power", "../mob_effect/conduit_power", 0x1DC2D1, 1 },
        { "absorption", "Absorption", "../mob_effect/absorption", 0x2552A5, 1 },
        { "hunger", "Hunger", "../mob_effect/hunger", 0x587653, -1 },
        { "saturation", "Saturation", "../mob_effect/saturation", 0xF82423, 1 },
        { "blindness", "Blindness", "../mob_effect/blindness", 0x1F1F23, -1 },
        { "darkness", "Darkness", "../mob_effect/darkness", 0x292721, -1 },
        { "dolphins_grace", "Dolphin's Grace", "../mob_effect/dolphins_grace", 0x88A3BE, 1 },
        { "glowing", "Glowing", "../mob_effect/glowing", 0x94A061, 0 },
        { "health_boost", "Health Boost", "../mob_effect/health_boost", 0xF87D23, 1 },
        { "hero_of_the_village", "Hero of the Village", "../mob_effect/hero_of_the_village", 0x44FF44, 1 },
        { "infested", "Infested", "../mob_effect/infested", 0x8C9B8C, -1 },
        { "instant_health", "Instant Health", "../mob_effect/instant_health", 0xF82423, 1 },
        { "instant_damage", "Instant Damage", "../mob_effect/instant_damage", 0xA9656A, -1 },
        { "invisibility", "Invisibility", "../mob_effect/invisibility", 0xF6F6F6, 1 },
        { "levitation", "Levitation", "../mob_effect/levitation", 0xCEFFFF, -1 },
        { "luck", "Luck", "../mob_effect/luck", 0x59C106, 1 },
        { "unluck", "Bad Luck", "../mob_effect/unluck", 0xC0A44D, -1 },
        { "mining_fatigue", "Mining Fatigue", "../mob_effect/mining_fatigue", 0x4A4217, -1 },
        { "nausea", "Nausea", "../mob_effect/nausea", 0x551D4A, -1 },
        { "oozing", "Oozing", "../mob_effect/oozing", 0x99FFA3, -1 },
        { "raid_omen", "Raid Omen", "../mob_effect/raid_omen", 0xDE4058, 0 },
        { "slow_falling", "Slow Falling", "../mob_effect/slow_falling", 0xF3CFB9, 1 },
        { "trial_omen", "Trial Omen", "../mob_effect/trial_omen", 0x16A6A6, 0 },
        { "weaving", "Weaving", "../mob_effect/weaving", 0x78695A, -1 },
        { "wind_charged", "Wind Charged", "../mob_effect/wind_charged", 0xBDC9FF, -1 },
        { "wither", "Wither", "../mob_effect/wither", 0x736156, -1 },
    };

    const Info& info(Effects::Kind kind)
    {
        const int at = static_cast<int>(kind);
        return INFO[at >= 0 && at < Effects::KIND_COUNT ? at : 0];
    }
}

unsigned int Effects::colour(Kind kind) { return info(kind).colour; }
const char* Effects::icon(Kind kind) { return info(kind).icon; }
const char* Effects::name(Kind kind) { return kind == Kind::Count ? "" : info(kind).name; }
const char* Effects::id(Kind kind) { return info(kind).id; }

bool Effects::fromId(const std::string& text, Kind& kind)
{
    std::string wanted = text;
    if (wanted.rfind("minecraft:", 0) == 0) wanted = wanted.substr(10);
    for (int i = 0; i < KIND_COUNT; ++i)
        if (wanted == INFO[i].id)
        {
            kind = static_cast<Kind>(i);
            return true;
        }
    return false;
}

bool Effects::isBeneficial(Kind kind) { return kind != Kind::Count && info(kind).category > 0; }
bool Effects::isNeutral(Kind kind) { return kind != Kind::Count && info(kind).category == 0; }
bool Effects::isHarmful(Kind kind) { return kind != Kind::Count && !isBeneficial(kind); }
bool Effects::isInstant(Kind kind) { return kind == Kind::InstantHealth || kind == Kind::InstantDamage; }

std::string Effects::levelText(int amplifier)
{
    static const char* ROMAN[10] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X" };
    if (amplifier >= 0 && amplifier < 10) return ROMAN[amplifier];
    return std::to_string(amplifier + 1);
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

    if (isInstant(kind))
    {
        if (slot.secondsLeft <= 0.0f) slot = Active{};
        else amplifier = std::max(amplifier, slot.amplifier);
        slot.amplifier = amplifier;
        slot.secondsLeft = INSTANT_SECONDS;
        slot.pulseClock += 1.0f;
        return;
    }

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

        if (isInstant(kind))
        {
            const int times = std::max(1, static_cast<int>(std::lround(slot.pulseClock)));
            const int amount = times * instantAmount(kind, slot.amplifier, 1.0f);
            if (kind == Kind::InstantHealth) tick.instantHeal += amount;
            else tick.instantHarm += amount;
            slot = Active{};
            continue;
        }

        if (kind == Kind::Regeneration || kind == Kind::Poison || kind == Kind::Wither)
        {
            const float base = kind == Kind::Regeneration ? REGEN_INTERVAL
                             : kind == Kind::Poison ? POISON_INTERVAL
                                                    : WITHER_INTERVAL;
            // Each step of the amplifier halves the wait.
            const float interval = std::max(0.05f, base / static_cast<float>(1 << std::min(slot.amplifier, 8)));

            slot.pulseClock += alive;
            while (slot.pulseClock >= interval)
            {
                slot.pulseClock -= interval;
                if (kind == Kind::Regeneration) ++tick.healPulses;
                else if (kind == Kind::Poison) ++tick.poisonPulses;
                else ++tick.witherPulses;
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
    float multiplier = level < 0 ? 1.0f : 1.0f + 0.20f * static_cast<float>(level + 1);
    if (set.has(Kind::MiningFatigue))
    {
        static const float FATIGUE[4] = { 0.3f, 0.09f, 0.0027f, 0.00081f };
        multiplier *= FATIGUE[std::min(set.amplifier(Kind::MiningFatigue), 3)];
    }
    return multiplier;
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

int Effects::maxHealthBonus(const Set& set)
{
    return set.has(Kind::HealthBoost) ? 4 * (std::min(set.amplifier(Kind::HealthBoost), 255) + 1) : 0;
}

float Effects::levitationSpeed(const Set& set)
{
    return set.has(Kind::Levitation) ? static_cast<float>(set.amplifier(Kind::Levitation) + 1) : 0.0f;
}

bool Effects::fallsSlowly(const Set& set) { return set.has(Kind::SlowFalling); }

float Effects::swimMultiplier(const Set& set)
{
    return set.has(Kind::DolphinsGrace) ? 2.6f : 1.0f;
}

int Effects::luck(const Set& set)
{
    int total = 0;
    if (set.has(Kind::Luck)) total += set.amplifier(Kind::Luck) + 1;
    if (set.has(Kind::BadLuck)) total -= set.amplifier(Kind::BadLuck) + 1;
    return total;
}

float Effects::detectionScale(const Set& set, int armourPieces)
{
    if (!set.has(Kind::Invisibility)) return 1.0f;
    const float shown = std::max(0.1f, static_cast<float>(std::clamp(armourPieces, 0, 4)) / 4.0f);
    return 0.7f * shown;
}

float Effects::cobwebDrag(const Set& set) { return set.has(Kind::Weaving) ? 0.5f : 0.25f; }

float Effects::darknessPulse(float fade, float seconds)
{
    const float ticks = seconds * 20.0f;
    return std::max(0.0f, std::cos(ticks * 3.14159265f * 0.025f) * 0.45f * std::clamp(fade, 0.0f, 1.0f));
}

int Effects::instantAmount(Kind kind, int amplifier, float proximity)
{
    const int step = std::clamp(amplifier, 0, 8);
    const int full = kind == Kind::InstantHealth ? (4 << step) : kind == Kind::InstantDamage ? (6 << step) : 0;
    return static_cast<int>(std::clamp(proximity, 0.0f, 1.0f) * static_cast<float>(full) + 0.5f);
}
