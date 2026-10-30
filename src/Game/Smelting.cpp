#include "Smelting.h"
#include "Food.h"
#include "Player/Inventory.h"
#include <algorithm>

StackId Smelting::resultOf(StackId input)
{
    switch (input)
    {
        case Blocks::IronOre:   return Items::IronIngot;
        case Blocks::GoldOre:   return Items::GoldIngot;
        case Blocks::CopperOre: return Items::CopperIngot;
        case Items::RawCopper:  return Items::CopperIngot;
        case Blocks::Sand:      return Blocks::Glass;
        case Blocks::Cobblestone: return Blocks::Stone;
        case Blocks::Clay:      return Blocks::Bricks;

        // A cactus fired is green, which is Minecraft's and the only
        // dye in the game that comes out of a furnace.
        case Blocks::Cactus:    return Items::DyeGreen;

        // A wet sponge dried out, which is Minecraft's recipe and the
        // only way to get a dry one. Without it the sponge in a
        // monument was the one block in the game nobody could ever
        // have -- which is what the block audit was written to find.
        case Blocks::WetSponge: return Blocks::Sponge;

        // Netherrack fired is a nether brick, which is Minecraft's.
        // It used to give blaze powder here, standing in for a blaze
        // there was no way to meet; now there is one, and the powder
        // comes off its rods where it should.
        case Blocks::Netherrack: return Items::NetherBrick;

        // Ancient debris fired is scrap, which is the only thing it
        // is for and the only thing that makes it worth the dig.
        case Blocks::AncientDebris: return Items::NetheriteScrap;

        // Meat. Cooked food is worth more than raw, which is the whole
        // reason to carry a furnace about.
        case Items::RawBeef:    return Items::Steak;
        case Items::RawPorkchop: return Items::CookedPorkchop;
        case Items::RawCod:      return Items::CookedCod;
        case Items::RawSalmon:   return Items::CookedSalmon;
        case Items::RawChicken: return Items::CookedChicken;
        case Items::RawMutton:  return Items::CookedMutton;

        // The one crop worth cooking. Raw it is barely food; baked it
        // is worth as much as bread.
        case Items::Potato:     return Items::BakedPotato;

        default: return Blocks::Air;
    }
}

float Smelting::burnSeconds(StackId fuel)
{
    // Asked of the whole family rather than of two named blocks: with
    // more than one kind of tree in the world, spruce would not have
    // burned.
    if (isBlockStack(fuel))
    {
        const BlockId block = asBlock(fuel);
        if (isLog(block) || isPlanks(block)) return 15.0f;
        if (isLeaves(block)) return 5.0f;
    }

    switch (fuel)
    {
        case Items::Coal:        return 80.0f;   // eight items
        case Blocks::CoalOre:    return 80.0f;
        case Items::Stick:       return 5.0f;
        case Blocks::CraftingTable: return 15.0f;

        // A wooden tool is firewood once it is of no further use.
        case Items::WoodPickaxe:
        case Items::WoodAxe:
        case Items::WoodShovel:
        case Items::WoodSword:
        case Items::WoodHoe:     return 10.0f;

        default: return 0.0f;
    }
}

bool Smelting::suits(Kind kind, StackId input)
{
    if (resultOf(input) == Blocks::Air) return false;

    switch (kind)
    {
        case Kind::Furnace:
            return true;

        // A smoker cooks what you eat. Asked of Food rather than
        // listed here, so a food added later is cookable in one
        // without anybody having to remember this function -- which
        // is the mistake the block table has caught five times.
        case Kind::Smoker:
            return Food::isEdible(input);

        // A blast furnace smelts metal: what comes out of the ground
        // with a metal in it, and the raw metal itself.
        case Kind::BlastFurnace:
            return input == Blocks::IronOre || input == Blocks::GoldOre ||
                   input == Blocks::CopperOre || input == Items::RawCopper ||
                   input == Blocks::AncientDebris;
    }
    return false;
}

Smelting::Kind Smelting::kindOf(BlockId block)
{
    if (block == Blocks::Smoker || block == Blocks::SmokerLit) return Kind::Smoker;
    if (block == Blocks::BlastFurnace || block == Blocks::BlastFurnaceLit)
        return Kind::BlastFurnace;
    return Kind::Furnace;
}

float Furnace::cookFraction() const
{
    return std::clamp(cooked / Smelting::cookSeconds(kind), 0.0f, 1.0f);
}

float Furnace::burnFraction() const
{
    if (burnTotal <= 0.0f) return 0.0f;
    return std::clamp(burnLeft / burnTotal, 0.0f, 1.0f);
}

void Furnace::tick(float deltaTime)
{
    if (!input || !fuel || !output) return;

    const StackId making = input->empty() ? Blocks::Air
                                         : Smelting::resultIn(kind, input->id);

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
    if (cooked < Smelting::cookSeconds(kind)) return;

    cooked -= Smelting::cookSeconds(kind);

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
