#pragma once
#include "Effects.h"
#include "Items.h"
#include "World/Block.h"
#include <string>
#include <vector>

// Brewing: what a stand makes of a bottle and an ingredient.
//
// Every drink in the game is one item id. Which one a stack holds rides
// in its damage field, the way wear does on a tool -- so a new potion
// costs no item id, and one left in a chest comes back from a save
// without the format changing.
//
// All of it is a lookup, so --selftest can brew every pairing there is
// without a stand or a world.
namespace Brewing
{
    // Order is what a saved stack stores in its damage field, so
    // nothing goes in the middle.
    enum class Potion : int
    {
        Water = 0,
        Awkward,      // what nether wart makes: the base of the rest
        Speed,
        Strength,
        NightVision,
        FireResistance,

        // What a ghast's tear makes. The only drink in the game that
        // puts hearts back rather than keeping them on.
        Regeneration,
        Leaping,
        Mundane,
        Thick,
        Slowness,
        Weakness,
        Poison,
        WaterBreathing,
        TurtleMaster,

        Count
    };

    enum class Form : int { Drink = 0, Splash, Lingering, Arrow };

    struct Brew
    {
        Potion potion = Potion::Water;
        bool extended = false;
        bool strong = false;
    };

    struct Dose
    {
        Effects::Kind kind = Effects::Kind::Speed;
        int amplifier = 0;
        float seconds = 0.0f;
    };

    constexpr int EXTENDED_BIT = 1 << 8;
    constexpr int STRONG_BIT = 1 << 9;

    Brew decode(int damage);
    int encode(const Brew& brew);
    bool canExtend(Potion potion);
    bool canStrengthen(Potion potion);
    std::vector<Dose> dosesOf(const Brew& brew);
    std::string fullName(const Brew& brew, Form form);
    unsigned int brewColour(const Brew& brew);
    bool brewWith(const Brew& base, StackId ingredient, Brew& out);
    StackId formChange(StackId bottle, StackId ingredient);
    std::string durationText(float seconds);

    const char* name(Potion potion);

    // Encoding to and from the stack's damage field.
    inline int toDamage(Potion potion) { return static_cast<int>(potion); }
    Potion fromDamage(int damage);
    unsigned int colour(Potion potion);

    // What drinking it leaves behind. Water and awkward leave nothing:
    // they are what you brew with, not what you drink. Returns false
    // for those two, and for anything out of range.
    bool effectOf(Potion potion, Effects::Kind& kind, int& amplifier, float& seconds);

    // What the stand turns this bottle into with this ingredient.
    // Potion::Count means the pairing makes nothing, which is also what
    // an unfinished stand shows: an empty output.
    Potion brew(Potion base, StackId ingredient);

    // Whether the stand will take this at all, for the slot to refuse
    // the rest before the timer ever starts.
    bool isIngredient(StackId id);

    // How long a brew takes. Minecraft's is twenty seconds.
    constexpr float BREW_SECONDS = 20.0f;

    // A stand runs on blaze powder: one powder is twenty brews, and each
    // brew burns one as it starts. 1.21's figures.
    constexpr int FUEL_PER_POWDER = 20;
}
