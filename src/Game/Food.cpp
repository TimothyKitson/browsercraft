#include "Food.h"

namespace
{
    struct Meal
    {
        StackId id;
        FoodValue value;
    };

    const Meal MEALS[] = {
        { Items::Bread,       { 5, 6.0f } },
        { Items::RawBeef,     { 3, 1.8f } },
        { Items::RawPorkchop, { 3, 1.8f } },
        { Items::RawMutton,   { 2, 1.2f } },
        { Items::RawChicken,  { 2, 1.2f } },

        // Cooking roughly doubles what a cut is worth, which is the
        // whole argument for carrying a furnace about.
        { Items::Steak,          { 8, 12.8f } },
        { Items::CookedPorkchop, { 8, 12.8f } },
        { Items::CookedMutton,   { 6, 9.6f } },
        { Items::CookedChicken,  { 6, 7.2f } },

        // Barely worth eating, which is Minecraft's: they are what you
        // find when you have nothing, and they light the cave on the
        // way to something better.
        { Items::GlowBerries,    { 2, 0.4f } },

        // Minecraft's apple: four hunger and not much behind it. It
        // is what you eat on the way somewhere, which is the whole
        // reason it falls out of trees.
        { Items::Apple,          { 4, 2.4f } },
    };
}

bool Food::isEdible(StackId id)
{
    for (const Meal& meal : MEALS)
        if (meal.id == id) return true;
    return false;
}

Food::Bite Food::chew(bool holdingUse, StackId held, bool hungry, float timer, float deltaTime)
{
    Bite bite;
    if (!holdingUse || !hungry || !isEdible(held)) return bite;

    bite.chewing = true;
    bite.timer = timer + deltaTime;

    if (bite.timer >= EAT_SECONDS)
    {
        bite.swallowed = true;
        bite.timer = 0.0f;
    }
    return bite;
}

FoodValue Food::valueOf(StackId id)
{
    for (const Meal& meal : MEALS)
        if (meal.id == id) return meal.value;
    return FoodValue{};
}
