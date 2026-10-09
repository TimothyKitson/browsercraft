#include "Smelting.h"
#include "Player/Inventory.h"
#include <algorithm>

StackId Smelting::resultOf(StackId input)
{
    switch (input)
    {
        case Blocks::IronOre:   return Items::IronIngot;
        case Blocks::GoldOre:   return Items::GoldIngot;
        case Blocks::Sand:      return Blocks::Glass;
        case Blocks::Cobblestone: return Blocks::Stone;
        case Blocks::Clay:      return Blocks::Bricks;

        // Meat. Cooked food is worth more than raw, which is the whole
        // reason to carry a furnace about.
        case Items::RawBeef:    return Items::Steak;
        case Items::RawPorkchop: return Items::CookedPorkchop;
        case Items::RawChicken: return Items::CookedChicken;
        case Items::RawMutton:  return Items::CookedMutton;

        default: return Blocks::Air;
    }
}

float Smelting::burnSeconds(StackId fuel)
{
    switch (fuel)
    {
        case Items::Coal:        return 80.0f;   // eight items
        case Blocks::CoalOre:    return 80.0f;
        case Items::Stick:       return 5.0f;
        case Blocks::Planks:     return 15.0f;
        case Blocks::Log:
        case Blocks::BirchLog:   return 15.0f;
        case Blocks::CraftingTable: return 15.0f;
        case Blocks::Leaves:
        case Blocks::BirchLeaves: return 5.0f;

        // A wooden tool is firewood once it is of no further use.
        case Items::WoodPickaxe:
        case Items::WoodAxe:
        case Items::WoodShovel:
        case Items::WoodSword:
        case Items::WoodHoe:     return 10.0f;

        default: return 0.0f;
    }
}

float Furnace::cookFraction() const
{
    return std::clamp(cooked / Smelting::COOK_SECONDS, 0.0f, 1.0f);
}

float Furnace::burnFraction() const
{
    if (burnTotal <= 0.0f) return 0.0f;
    return std::clamp(burnLeft / burnTotal, 0.0f, 1.0f);
}

void Furnace::tick(float deltaTime)
{
    if (!input || !fuel || !output) return;

    const StackId making = input->empty() ? Blocks::Air : Smelting::resultOf(input->id);

    // Is there room for what this would produce? A full output slot, or
    // one holding something else, stops the furnace dead.
    const bool roomForIt =
        making != Blocks::Air &&
        (output->empty() ||
         (output->id == making && output->count < maxStackOf(making)));

    if (burnLeft > 0.0f) burnLeft = std::max(0.0f, burnLeft - deltaTime);

    // Light it, but only when there is work for it to do: a furnace does
    // not eat its coal keeping warm.
    if (burnLeft <= 0.0f && roomForIt && !fuel->empty() && Smelting::isFuel(fuel->id))
    {
        burnTotal = Smelting::burnSeconds(fuel->id);
        burnLeft = burnTotal;

        fuel->count -= 1;
        if (fuel->count <= 0) fuel->clear();
    }

    if (!lit() || !roomForIt)
    {
        // Half-done work is lost when the fire goes out, the way it is
        // in Minecraft -- it creeps back rather than stopping dead.
        cooked = std::max(0.0f, cooked - deltaTime * 2.0f);
        return;
    }

    cooked += deltaTime;
    if (cooked < Smelting::COOK_SECONDS) return;

    cooked -= Smelting::COOK_SECONDS;

    if (output->empty())
    {
        output->id = making;
        output->count = 1;
        output->damage = 0;
    }
    else
    {
        output->count += 1;
    }

    input->count -= 1;
    if (input->count <= 0) input->clear();
}
