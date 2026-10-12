#pragma once
#include "Effects.h"
#include "Items.h"
#include "World/Block.h"

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
        Count
    };

    const char* name(Potion potion);

    // Encoding to and from the stack's damage field.
    inline int toDamage(Potion potion) { return static_cast<int>(potion); }
    Potion fromDamage(int damage);

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
}
