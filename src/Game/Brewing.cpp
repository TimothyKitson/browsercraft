#include "Brewing.h"
#include <algorithm>
#include <cstdio>

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
        case Potion::Leaping:     return "Potion of Leaping";
        case Potion::Mundane:     return "Mundane Potion";
        case Potion::Thick:       return "Thick Potion";
        case Potion::Slowness:    return "Potion of Slowness";
        case Potion::Weakness:    return "Potion of Weakness";
        case Potion::Poison:      return "Potion of Poison";
        case Potion::WaterBreathing: return "Potion of Water Breathing";
        case Potion::TurtleMaster: return "Potion of the Turtle Master";
        case Potion::Count:       break;
    }
    return "Potion";
}

Brewing::Potion Brewing::fromDamage(int damage)
{
    const int potion = damage & 0xFF;
    if (damage < 0 || potion >= static_cast<int>(Potion::Count)) return Potion::Water;
    return static_cast<Potion>(potion);
}

Brewing::Brew Brewing::decode(int damage)
{
    Brew brew;
    brew.potion = fromDamage(damage);
    brew.extended = damage >= 0 && (damage & EXTENDED_BIT) != 0 && canExtend(brew.potion);
    brew.strong = damage >= 0 && (damage & STRONG_BIT) != 0 && canStrengthen(brew.potion);
    if (brew.extended && brew.strong) brew.extended = false;
    return brew;
}

int Brewing::encode(const Brew& brew)
{
    return static_cast<int>(brew.potion) | (brew.extended ? EXTENDED_BIT : 0) | (brew.strong ? STRONG_BIT : 0);
}

bool Brewing::canExtend(Potion potion)
{
    switch (potion)
    {
        case Potion::Speed: case Potion::Slowness: case Potion::Strength: case Potion::Weakness:
        case Potion::NightVision: case Potion::FireResistance: case Potion::Regeneration: case Potion::Leaping:
        case Potion::Poison: case Potion::WaterBreathing: case Potion::TurtleMaster:
            return true;
        default:
            return false;
    }
}

bool Brewing::canStrengthen(Potion potion)
{
    switch (potion)
    {
        case Potion::Speed: case Potion::Slowness: case Potion::Strength: case Potion::Regeneration:
        case Potion::Leaping: case Potion::Poison: case Potion::TurtleMaster:
            return true;
        default:
            return false;
    }
}

std::vector<Brewing::Dose> Brewing::dosesOf(const Brew& brew)
{
    std::vector<Dose> doses;
    auto add = [&](Effects::Kind kind, float base, float longer, float stronger, int strongAmplifier) {
        Dose dose;
        dose.kind = kind;
        dose.seconds = brew.strong ? stronger : brew.extended ? longer : base;
        dose.amplifier = brew.strong ? strongAmplifier : 0;
        doses.push_back(dose);
    };
    switch (brew.potion)
    {
        case Potion::Speed: add(Effects::Kind::Speed, 180.0f, 480.0f, 90.0f, 1); break;
        case Potion::Slowness: add(Effects::Kind::Slowness, 90.0f, 240.0f, 20.0f, 3); break;
        case Potion::Strength: add(Effects::Kind::Strength, 180.0f, 480.0f, 90.0f, 1); break;
        case Potion::Weakness: add(Effects::Kind::Weakness, 90.0f, 240.0f, 90.0f, 0); break;
        case Potion::NightVision: add(Effects::Kind::NightVision, 180.0f, 480.0f, 180.0f, 0); break;
        case Potion::FireResistance: add(Effects::Kind::FireResistance, 180.0f, 480.0f, 180.0f, 0); break;
        case Potion::Regeneration: add(Effects::Kind::Regeneration, 45.0f, 90.0f, 22.5f, 1); break;
        case Potion::Leaping: add(Effects::Kind::JumpBoost, 180.0f, 480.0f, 90.0f, 1); break;
        case Potion::Poison: add(Effects::Kind::Poison, 45.0f, 90.0f, 21.6f, 1); break;
        case Potion::WaterBreathing: add(Effects::Kind::WaterBreathing, 180.0f, 480.0f, 180.0f, 0); break;
        case Potion::TurtleMaster:
            add(Effects::Kind::Slowness, 20.0f, 40.0f, 20.0f, 5);
            add(Effects::Kind::Resistance, 20.0f, 40.0f, 20.0f, 3);
            if (!brew.strong)
            {
                doses[0].amplifier = 3;
                doses[1].amplifier = 2;
            }
            break;
        default: break;
    }
    return doses;
}

std::string Brewing::fullName(const Brew& brew, Form form)
{
    static const char* PREFIX[4] = { "", "Splash ", "Lingering ", "" };
    const int which = static_cast<int>(form);
    if (form == Form::Arrow)
    {
        switch (brew.potion)
        {
            case Potion::Water: return "Arrow of Splashing";
            case Potion::Awkward: case Potion::Mundane: case Potion::Thick: return "Tipped Arrow";
            default: break;
        }
        std::string base = name(brew.potion);
        return "Arrow of " + base.substr(base.find(" of ") + 4);
    }
    if (brew.potion == Potion::Water) return std::string(PREFIX[which]) + "Water Bottle";
    return std::string(PREFIX[which]) + name(brew.potion);
}

unsigned int Brewing::brewColour(const Brew& brew)
{
    const std::vector<Dose> doses = dosesOf(brew);
    if (doses.empty()) return 0x385DC6;
    unsigned int r = 0, g = 0, b = 0;
    for (const Dose& dose : doses)
    {
        const unsigned int c = Effects::colour(dose.kind);
        r += (c >> 16) & 255u;
        g += (c >> 8) & 255u;
        b += c & 255u;
    }
    const unsigned int n = static_cast<unsigned int>(doses.size());
    return ((r / n) << 16) | ((g / n) << 8) | (b / n);
}

bool Brewing::brewWith(const Brew& base, StackId ingredient, Brew& out)
{
    out = base;
    if (ingredient == Items::Redstone && canExtend(base.potion) && !base.extended)
    {
        out.extended = true;
        out.strong = false;
        return true;
    }
    if (ingredient == Items::GlowstoneDust && canStrengthen(base.potion) && !base.strong)
    {
        out.strong = true;
        out.extended = false;
        return true;
    }
    if (ingredient == Items::FermentedSpiderEye)
    {
        out.extended = base.extended;
        out.strong = false;
        if (base.potion == Potion::Water) { out = Brew{ Potion::Weakness, false, false }; return true; }
        if (base.potion == Potion::Speed || base.potion == Potion::Leaping)
        {
            out.potion = Potion::Slowness;
            return true;
        }
        return false;
    }
    if (base.extended || base.strong) return false;
    const Potion made = brew(base.potion, ingredient);
    if (made == Potion::Count) return false;
    out = Brew{ made, false, false };
    return true;
}

StackId Brewing::formChange(StackId bottle, StackId ingredient)
{
    if (ingredient == Items::Gunpowder && bottle == Items::Potion) return Items::SplashPotion;
    if (ingredient == Items::DragonsBreath && bottle == Items::SplashPotion) return Items::LingeringPotion;
    return Blocks::Air;
}

std::string Brewing::durationText(float seconds)
{
    const int whole = static_cast<int>(seconds);
    char text[16];
    std::snprintf(text, sizeof(text), "%d:%02d", whole / 60, whole % 60);
    return text;
}

unsigned int Brewing::colour(Potion potion)
{
    if (potion == Potion::TurtleMaster) return brewColour(Brew{ potion, false, false });
    Effects::Kind kind = Effects::Kind::Count;
    int amplifier = 0;
    float seconds = 0.0f;
    if (!effectOf(potion, kind, amplifier, seconds)) return 0x385DC6;
    return Effects::colour(kind);
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
        case Potion::Leaping:
            kind = Effects::Kind::JumpBoost; amplifier = 0; seconds = 180.0f; return true;
        case Potion::Slowness:
            kind = Effects::Kind::Slowness; amplifier = 0; seconds = 90.0f; return true;
        case Potion::Weakness:
            kind = Effects::Kind::Weakness; amplifier = 0; seconds = 90.0f; return true;
        case Potion::Poison:
            kind = Effects::Kind::Poison; amplifier = 0; seconds = 45.0f; return true;
        case Potion::WaterBreathing:
            kind = Effects::Kind::WaterBreathing; amplifier = 0; seconds = 180.0f; return true;
        case Potion::TurtleMaster:
            kind = Effects::Kind::Resistance; amplifier = 2; seconds = 20.0f; return true;
        case Potion::Mundane:
        case Potion::Thick:

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
    {
        if (ingredient == Items::NetherWart) return Potion::Awkward;
        if (ingredient == Items::GlowstoneDust) return Potion::Thick;
        if (ingredient == Items::Redstone || ingredient == Items::GlisteringMelon || ingredient == Items::GhastTear ||
            ingredient == Items::RabbitFoot || ingredient == Items::BlazePowder || ingredient == Items::SpiderEye ||
            ingredient == Items::Sugar || ingredient == Items::MagmaCream)
            return Potion::Mundane;
        if (ingredient == Items::FermentedSpiderEye) return Potion::Weakness;
        return Potion::Count;
    }

    if (base != Potion::Awkward) return Potion::Count;   // already finished

    switch (ingredient)
    {
        case Items::Sugar:         return Potion::Speed;
        case Items::BlazePowder:   return Potion::Strength;
        case Items::GoldenCarrot:  return Potion::NightVision;
        case Items::SpiderEye:     return Potion::Poison;
        case Items::Pufferfish:    return Potion::WaterBreathing;
        case Items::TurtleHelmet:  return Potion::TurtleMaster;
        case Items::MagmaCream:    return Potion::FireResistance;
        case Items::GhastTear:     return Potion::Regeneration;
        case Items::RabbitFoot:    return Potion::Leaping;
        default:                   return Potion::Count;
    }
}

bool Brewing::isIngredient(StackId id)
{
    return id == Items::NetherWart || id == Items::Sugar ||
           id == Items::BlazePowder || id == Items::GlowstoneDust ||
           id == Items::MagmaCream || id == Items::GhastTear || id == Items::RabbitFoot ||
           id == Items::GoldenCarrot || id == Items::SpiderEye || id == Items::Pufferfish || id == Items::TurtleHelmet ||
           id == Items::Redstone || id == Items::FermentedSpiderEye || id == Items::GlisteringMelon ||
           id == Items::Gunpowder || id == Items::DragonsBreath;
}
