#include "Brewing.h"
#include <algorithm>

const char* Brewing::name(Potion potion)
{
    switch (potion)
    {
        case Potion::Water:       return "Water Bottle";
        case Potion::Awkward:     return "Awkward Potion";
        case Potion::Speed:       return "Potion of Swiftness";
        case Potion::Strength:    return "Potion of Strength";
        case Potion::NightVision: return "Potion of Night Vision";
        case Potion::FireResistance: return "Potion of Fire Resistance";
        case Potion::Regeneration: return "Potion of Regeneration";
        case Potion::Count:       break;
    }
    return "Potion";
}

Brewing::Potion Brewing::fromDamage(int damage)
{
    if (damage < 0 || damage >= static_cast<int>(Potion::Count)) return Potion::Water;
    return static_cast<Potion>(damage);
}

bool Brewing::effectOf(Potion potion, Effects::Kind& kind, int& amplifier, float& seconds)
{
    switch (potion)
    {
        case Potion::Speed:
            kind = Effects::Kind::Speed; amplifier = 0; seconds = 180.0f; return true;
        case Potion::Strength:
            kind = Effects::Kind::Strength; amplifier = 0; seconds = 180.0f; return true;
        case Potion::NightVision:
            kind = Effects::Kind::NightVision; amplifier = 0; seconds = 180.0f; return true;
        case Potion::FireResistance:
            kind = Effects::Kind::FireResistance; amplifier = 0; seconds = 180.0f; return true;

        // Shorter than the rest, because what it does is worth more:
        // forty-five seconds is Minecraft's and is about four hearts.
        case Potion::Regeneration:
            kind = Effects::Kind::Regeneration; amplifier = 0; seconds = 45.0f; return true;

        // Nothing to drink for: these are stages on the way.
        case Potion::Water:
        case Potion::Awkward:
        case Potion::Count:
            break;
    }
    return false;
}

Brewing::Potion Brewing::brew(Potion base, StackId ingredient)
{
    // Nether wart is the only thing water takes, and the only thing
    // that turns a bottle into something worth adding to.
    if (base == Potion::Water)
        return (ingredient == Items::NetherWart) ? Potion::Awkward : Potion::Count;

    if (base != Potion::Awkward) return Potion::Count;   // already finished

    switch (ingredient)
    {
        case Items::Sugar:         return Potion::Speed;
        case Items::BlazePowder:   return Potion::Strength;
        case Items::GlowstoneDust: return Potion::NightVision;
        case Items::MagmaCream:    return Potion::FireResistance;
        case Items::GhastTear:     return Potion::Regeneration;
        default:                   return Potion::Count;
    }
}

bool Brewing::isIngredient(StackId id)
{
    return id == Items::NetherWart || id == Items::Sugar ||
           id == Items::BlazePowder || id == Items::GlowstoneDust ||
           id == Items::MagmaCream || id == Items::GhastTear;
}
