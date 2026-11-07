#include "Food.h"
#include <algorithm>

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
        { Items::SweetBerries,   { 2, 0.4f } },
        { Items::HoneyBottle,    { 6, 1.2f } },
        { Items::DriedKelp,      { 1, 0.6f } },
        { Items::ChorusFruit,    { 4, 2.4f } },

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

        // Out of the sea. Cooking a fish is worth as much as cooking
        // anything else, and a pufferfish is worth nothing at all --
        // it is the one food in the game that is a mistake.
        { Items::RawCod,         { 2, 0.4f } },
        { Items::CookedCod,      { 5, 6.0f } },
        { Items::RawSalmon,      { 2, 0.4f } },
        { Items::CookedSalmon,   { 6, 9.6f } },
        { Items::Pufferfish,     { 1, 0.2f } },
        { Items::TropicalFish,   { 1, 0.2f } },
        { Items::GoldenApple,    { 4, 9.6f } },
        { Items::EnchantedGoldenApple, { 4, 9.6f } },
        { Items::GoldenCarrot,   { 6, 14.4f } },
        { Items::MushroomStew,   { 6, 7.2f } },
        { Items::RabbitStew,     { 10, 12.0f } },
        { Items::SuspiciousStew, { 6, 7.2f } },
        { Items::PumpkinPie,     { 8, 4.8f } },
        { Items::RottenFlesh,    { 4, 0.8f } },
        { Items::SpiderEye,      { 2, 3.2f } },
        { Items::PoisonousPotato, { 2, 1.2f } },

        // Rabbit. Minecraft's is worth a little less than pork and a
        // rabbit is a good deal harder to catch.
        { Items::RawRabbit,      { 3, 1.8f } },
        { Items::CookedRabbit,   { 5, 6.0f } },
    };
}

bool Food::poisons(StackId id)
{
    // One. Minecraft's pufferfish is the only food that is worse than
    // not eating, and that is the whole of what it is for.
    return id == Items::Pufferfish;
}

bool Food::alwaysEdible(StackId id)
{
    return id == Items::GoldenApple || id == Items::EnchantedGoldenApple || id == Items::ChorusFruit || id == Items::SuspiciousStew;
}

bool Food::leavesBowl(StackId id)
{
    return id == Items::MushroomStew || id == Items::RabbitStew || id == Items::SuspiciousStew || id == Items::BeetrootSoup;
}

Food::StewEffect Food::stewEffect(int flower)
{
    static const StewEffect EFFECTS[] = {
        { Effects::Kind::FireResistance, 3.0f },
        { Effects::Kind::Blindness, 11.0f },
        { Effects::Kind::Saturation, 0.35f },
        { Effects::Kind::Saturation, 0.35f },
        { Effects::Kind::JumpBoost, 5.0f },
        { Effects::Kind::Poison, 11.0f },
        { Effects::Kind::Regeneration, 7.0f },
        { Effects::Kind::NightVision, 5.0f },
        { Effects::Kind::Weakness, 7.0f },
        { Effects::Kind::Poison, 7.0f },
        { Effects::Kind::NightVision, 5.0f },
    };
    const int count = static_cast<int>(sizeof(EFFECTS) / sizeof(EFFECTS[0]));
    return EFFECTS[std::clamp(flower, 0, count - 1)];
}

int Food::stewFlower(StackId flower)
{
    switch (flower)
    {
        case Blocks::Allium: return 0;
        case Blocks::AzureBluet: return 1;
        case Blocks::BlueOrchid: return 2;
        case Blocks::FlowerYellow: return 3;
        case Blocks::Cornflower: return 4;
        case Blocks::LilyOfTheValley: return 5;
        case Blocks::OxeyeDaisy: return 6;
        case Blocks::FlowerRed: return 7;
        case Blocks::RedTulip: case Blocks::OrangeTulip: case Blocks::WhiteTulip: case Blocks::PinkTulip: return 8;
        case Blocks::WitherRose: return 9;
        case Blocks::Torchflower: return 10;
        default: return -1;
    }
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
