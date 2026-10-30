#pragma once
#include "Items.h"

struct FoodValue
{
    int hunger = 0;
    float saturation = 0.0f;
};

namespace Food
{
    constexpr int MAX = 20;
    constexpr float EAT_SECONDS = 1.6f;

    bool isEdible(StackId id);
    FoodValue valueOf(StackId id);

    // Whether swallowing it makes you ill. Whoever applies the bite
    // owns the effect; this only says which food deserves one.
    bool poisons(StackId id);

    // How long the poison lasts, in seconds, and how strong. Minecraft
    // gives a pufferfish poison for a quarter of a minute.
    constexpr float POISON_SECONDS = 15.0f;
    constexpr int POISON_LEVEL = 1;

    struct Bite
    {
        float timer = 0.0f;
        bool chewing = false;
        bool swallowed = false;
    };

    // One frame of holding the use button with something in hand. Kept
    // out of the input handler so --selftest can drive it: the rule that
    // matters is that a bite only lands at the end of the chew, and that
    // letting go or running out of food starts it again from nothing.
    Bite chew(bool holdingUse, StackId held, bool hungry, float timer, float deltaTime);
}
