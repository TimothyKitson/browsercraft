#pragma once
#include <cstddef>
#include <string>

// Status effects: what a potion leaves behind once it is drunk.
//
// A player carries at most one of each kind, so the set is one slot per
// kind rather than a list -- nothing has to be searched for, and the
// rule about a stronger one replacing a weaker one has somewhere
// obvious to live.
//
// All of it is arithmetic over a clock, so --selftest can run a whole
// potion's worth of regeneration without a world or a player.
namespace Effects
{
    // The ones worth having without a mob or a block to hang them on.
    // Order is what a saved player would store, so nothing goes in the
    // middle.
    enum class Kind : int
    {
        Speed = 0,
        Slowness,
        Strength,
        Weakness,
        Regeneration,
        Poison,
        FireResistance,
        WaterBreathing,
        NightVision,
        JumpBoost,

        // Not an effect so much as a flag you carry: it does nothing
        // to you at all, and everything to what a trial chamber does
        // to you. Drunk from an ominous bottle, it is the game's own
        // hard mode.
        BadOmen,
        Haste,
        Resistance,
        ConduitPower,
        Absorption,
        Hunger,
        Saturation,
        Blindness,
        Darkness,
        DolphinsGrace,
        Glowing,
        HealthBoost,
        HeroOfTheVillage,
        Infested,
        InstantHealth,
        InstantDamage,
        Invisibility,
        Levitation,
        Luck,
        BadLuck,
        MiningFatigue,
        Nausea,
        Oozing,
        RaidOmen,
        SlowFalling,
        TrialOmen,
        Weaving,
        WindCharged,
        Wither,

        Count
    };

    constexpr int KIND_COUNT = static_cast<int>(Kind::Count);

    const char* name(Kind kind);
    const char* icon(Kind kind);
    const char* id(Kind kind);
    bool fromId(const std::string& text, Kind& kind);
    unsigned int colour(Kind kind);
    bool isBeneficial(Kind kind);
    bool isNeutral(Kind kind);
    bool isInstant(Kind kind);
    std::string levelText(int amplifier);

    // Whether the effect is one you would rather not have. The milk
    // bucket does not exist yet, but the distinction is also what
    // decides which colour the UI gives it.
    bool isHarmful(Kind kind);

    // One running effect. The amplifier is Minecraft's: 0 reads as "I",
    // 1 as "II". A kind is present exactly when its time is above zero.
    struct Active
    {
        int amplifier = 0;
        float secondsLeft = 0.0f;
        // Regeneration and poison work in pulses rather than a trickle,
        // so each carries its own clock towards the next one.
        float pulseClock = 0.0f;
    };

    struct Set
    {
        Active slots[KIND_COUNT];

        bool has(Kind kind) const;
        // -1 when the effect is not running, so a caller can tell
        // "absent" from "the weakest there is".
        int amplifier(Kind kind) const;
        float secondsLeft(Kind kind) const;
        bool any() const;
    };

    // A stronger one replaces a weaker one outright; an equal one keeps
    // whichever lasts longer. That is Minecraft's rule, and it is what
    // stops a long weak drink cancelling a short strong one.
    void give(Set& set, Kind kind, int amplifier, float seconds);
    void remove(Set& set, Kind kind);
    void clear(Set& set);

    // Minecraft's pulse rates, halved for each step of the amplifier.
    constexpr float REGEN_INTERVAL = 2.5f;
    constexpr float POISON_INTERVAL = 1.25f;
    constexpr float WITHER_INTERVAL = 2.0f;
    constexpr float INSTANT_SECONDS = 0.05f;
    constexpr float HERO_SECONDS = 2400.0f;
    constexpr float RAID_OMEN_SECONDS = 30.0f;
    constexpr float TRIAL_OMEN_SECONDS = 900.0f;
    constexpr float DARKNESS_FADE_SECONDS = 1.1f;
    constexpr float INFESTED_CHANCE = 0.1f;

    // What one step of the clock produced. Pulses rather than a rate,
    // because that is how the player actually sees them: hearts going
    // up or down one at a time.
    struct Tick
    {
        int healPulses = 0;
        int poisonPulses = 0;
        int feedPulses = 0;
        int witherPulses = 0;
        int instantHeal = 0;
        int instantHarm = 0;
        float exhaustion = 0.0f;
    };

    // Runs every effect's clock down. Poison must never be the thing
    // that kills -- that rule belongs to whoever applies the pulses,
    // since only they know the health.
    Tick step(Set& set, float deltaTime);

    // --- what the effects actually do -------------------------------

    // Speed I is a tenth again, as in Minecraft, and slowness takes the
    // same away. They multiply, so drinking both leaves you near enough
    // where you started.
    float speedMultiplier(const Set& set);

    // Extra damage on a blow, and the weakness that subtracts from it.
    // Signed, and the caller still floors the total at zero.
    int attackBonus(const Set& set);

    float jumpMultiplier(const Set& set);

    bool fireImmune(const Set& set);
    float digMultiplier(const Set& set);
    int resisted(const Set& set, int amount);
    bool breathesWater(const Set& set);

    // The least daylight the world is allowed to look like while night
    // vision is up. 0 when it is not running.
    float nightVisionFloor(const Set& set);

    int maxHealthBonus(const Set& set);
    float levitationSpeed(const Set& set);
    bool fallsSlowly(const Set& set);
    float swimMultiplier(const Set& set);
    int luck(const Set& set);
    float detectionScale(const Set& set, int armourPieces);
    float cobwebDrag(const Set& set);
    float darknessPulse(float fade, float seconds);
    int instantAmount(Kind kind, int amplifier, float proximity);
}
