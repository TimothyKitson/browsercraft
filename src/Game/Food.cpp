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

        // The two other crops. A raw potato is the worst food in the
        // game and a baked one is among the best, which is the whole
        // reason the furnace is worth walking back to.
        { Items::Carrot,         { 3, 3.6f } },
        { Items::Potato,         { 1, 0.6f } },
        { Items::BakedPotato,    { 5, 6.0f } },

        // A beet is barely worth eating; six of them in a bowl is a
        // meal, which is the whole reason the bowl exists.
        { Items::Beetroot,       { 1, 1.2f } },
        { Items::BeetrootSoup,   { 6, 7.2f } },

        // Melon: two hunger a slice and a great many slices to a
        // melon, which is what makes one worth planting.
        { Items::MelonSlice,     { 2, 1.2f } },

        // Two wheat and a bean, and the cheapest food in the game to
        // carry in bulk: eight of them from one handful.
        { Items::Cookie,         { 2, 0.4f } },
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
