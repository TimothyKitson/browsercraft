#include "Smelting.h"
#include "Food.h"
#include "Player/Inventory.h"
#include <algorithm>

StackId Smelting::resultOf(StackId input)
{
    if (isBlockStack(input)) input = oreBase(static_cast<BlockId>(input));
    if (isBlockStack(input) && isColouredTerracotta(static_cast<BlockId>(input)))
        return colourIn(Blocks::GlazedTerracottaFirst, runColour(static_cast<BlockId>(input), Blocks::TerracottaFirst));
    switch (input)
    {
        case Blocks::IronOre:   return Items::IronIngot;
        case Blocks::GoldOre:   return Items::GoldIngot;
        case Blocks::CopperOre: return Items::CopperIngot;
        case Items::RawCopper:  return Items::CopperIngot;
        case Items::RawIron:    return Items::IronIngot;
        case Items::RawGold:    return Items::GoldIngot;
        case Blocks::Sand:      return Blocks::Glass;
        case Blocks::Cobblestone: return Blocks::Stone;
        case Blocks::Stone:     return Blocks::SmoothStone;
        case Blocks::StoneBricks: return Blocks::CrackedStoneBricks;
        case Blocks::CobbledDeepslate: return Blocks::Deepslate;
        case Blocks::DeepslateBricks: return Blocks::CrackedDeepslateBricks;
        case Blocks::DeepslateTiles: return Blocks::CrackedDeepslateTiles;
        case Blocks::PolishedBlackstoneBricks: return Blocks::CrackedPolishedBlackstoneBricks;
        case Blocks::NetherBricks: return Blocks::CrackedNetherBricks;
        case Blocks::NetherQuartzOre: return Items::NetherQuartz;
        case Blocks::CoalOre:   return Items::Coal;
        case Blocks::DiamondOre: return Items::Diamond;
        case Blocks::EmeraldOre: return Items::Emerald;
        case Blocks::LapisOre:  return Items::LapisLazuli;
        case Blocks::RedstoneOre: return Items::Redstone;
        case Blocks::NetherGoldOre: return Items::GoldNugget;
        case Blocks::Basalt:    return Blocks::SmoothBasalt;
        case Blocks::SeaPickle: return Items::DyeLime;
        case Blocks::RedSand: return Blocks::Glass;
        case Blocks::Sandstone: return Blocks::SmoothSandstone;
        case Blocks::QuartzBlock: return Blocks::SmoothQuartz;
        case Blocks::RedSandstone: return Blocks::SmoothRedSandstone;
        case Blocks::Clay:      return Blocks::Terracotta;
        case Items::ClayBall:   return Items::Brick;

        // A cactus fired is green, which is Minecraft's and the only
        // dye in the game that comes out of a furnace.
        case Blocks::Cactus:    return Items::DyeGreen;
        case Blocks::Kelp:      return Items::DriedKelp;
        case Items::ChorusFruit: return Items::PoppedChorusFruit;
        case Blocks::Log: case Blocks::BirchLog: case Blocks::SpruceLog: case Blocks::DarkOakLog:
        case Blocks::JungleLog: case Blocks::AcaciaLog: case Blocks::CherryLog: case Blocks::MangroveLog:
        case Blocks::StrippedMangroveLog: case Blocks::StrippedCherryLog:
        case Blocks::StrippedOakLog: case Blocks::StrippedSpruceLog: case Blocks::StrippedBirchLog:
        case Blocks::StrippedJungleLog: case Blocks::StrippedAcaciaLog: case Blocks::StrippedDarkOakLog: return Items::Charcoal;

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
        case Items::RawRabbit:   return Items::CookedRabbit;
        case Items::RawChicken: return Items::CookedChicken;
        case Items::RawMutton:  return Items::CookedMutton;

        // The one crop worth cooking. Raw it is barely food; baked it
        // is worth as much as bread.
        case Items::Potato:     return Items::BakedPotato;

        default: break;
    }
    switch (input)
    {
        case Items::IronPickaxe: case Items::IronAxe: case Items::IronShovel: case Items::IronSword: case Items::IronHoe:
        case Items::IronHelmet: case Items::IronChestplate: case Items::IronLeggings: case Items::IronBoots:
        case Items::ChainmailHelmet: case Items::ChainmailChestplate: case Items::ChainmailLeggings: case Items::ChainmailBoots:
        case Items::IronHorseArmor:
            return Items::IronNugget;
        case Items::GoldPickaxe: case Items::GoldAxe: case Items::GoldShovel: case Items::GoldSword: case Items::GoldHoe:
        case Items::GoldHelmet: case Items::GoldChestplate: case Items::GoldLeggings: case Items::GoldBoots:
        case Items::GoldHorseArmor:
            return Items::GoldNugget;
        default: return Blocks::Air;
    }
}

float Smelting::burnSeconds(StackId fuel)
{
    auto netherWood = [](int wood) { return wood == 9 || wood == 10; };
    if (isBlockStack(fuel))
    {
        const BlockId block = asBlock(fuel);
        if (isPlanks(block)) return block == Blocks::CrimsonPlanks || block == Blocks::WarpedPlanks ? 0.0f : 15.0f;
        if (isLog(block))
        {
            const BlockId planks = planksFromLog(block);
            return planks == Blocks::CrimsonPlanks || planks == Blocks::WarpedPlanks ? 0.0f : 15.0f;
        }
        if (const int wood = shapeWood(block); wood >= 0)
        {
            if (netherWood(wood)) return 0.0f;
            if (isFamilyButton(block)) return 5.0f;
            return slabMaterial(block) >= 0 ? 7.5f : 15.0f;
        }
        if (isDoor(block)) { const int wood = doorWood(block); return wood >= 0 && !netherWood(wood) ? 10.0f : 0.0f; }
        if (isTrapdoor(block)) { const int wood = trapdoorWood(block); return wood >= 0 && !netherWood(wood) ? 15.0f : 0.0f; }
        if (isHangingSign(block)) return netherWood(signWood(block)) ? 0.0f : 40.0f;
        if (isSign(block)) return netherWood(signWood(block)) ? 0.0f : 10.0f;
        if (isWool(block)) return 5.0f;
        if (isCarpet(block)) return block == Blocks::MossCarpet ? 0.0f : 3.35f;
        if (isBanner(block)) return 15.0f;
        if (isSapling(block)) return 5.0f;
        if (isChestFamily(block) || isChiseledBookshelf(block) || isLectern(block) || isNoteBlock(block) || isLadder(block) ||
            isBarrel(block) || isComposter(block) || isDaylightDetector(block) || isLoom(block))
            return 15.0f;
        switch (block)
        {
            case Blocks::CoalBlock: return 800.0f;
            case Blocks::DriedKelpBlock: return 200.05f;
            case Blocks::BambooMosaic: return 15.0f;
            case Blocks::Bookshelf:
            case Blocks::Jukebox:
            case Blocks::CraftingTable:
            case Blocks::CartographyTable:
            case Blocks::FletchingTable:
            case Blocks::SmithingTable:
            case Blocks::MangroveRoots: return 15.0f;
            case Blocks::Azalea:
            case Blocks::FloweringAzalea:
            case Blocks::DeadBush: return 5.0f;
            case Blocks::Bamboo:
            case Blocks::Scaffolding: return 2.5f;
            default: return 0.0f;
        }
    }

    switch (fuel)
    {
        case Items::LavaBucket:  return 1000.0f;
        case Items::BlazeRod:    return 120.0f;
        case Items::Coal:        return 80.0f;
        case Items::Charcoal:    return 80.0f;
        case Items::Bow:
        case Items::FishingRod:
        case Items::Crossbow:    return 15.0f;
        case Items::WoodPickaxe:
        case Items::WoodAxe:
        case Items::WoodShovel:
        case Items::WoodSword:
        case Items::WoodHoe:     return 10.0f;
        case Items::Stick:
        case Items::Bowl:        return 5.0f;
        default: break;
    }
    for (int wood = 0; wood < BOAT_WOODS; ++wood)
        if (fuel == boatOfWood(wood, false) || fuel == boatOfWood(wood, true)) return 60.0f;
    return 0.0f;
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
            return (Food::isEdible(input) && input != Items::ChorusFruit) || input == Blocks::Kelp;

        // A blast furnace smelts metal: what comes out of the ground
        // with a metal in it, and the raw metal itself.
        case Kind::BlastFurnace:
        {
            if (isBlockStack(input)) input = oreBase(static_cast<BlockId>(input));
            const StackId made = resultOf(input);
            return input == Blocks::IronOre || input == Blocks::GoldOre ||
                   input == Blocks::CopperOre || input == Items::RawCopper ||
                   input == Items::RawIron || input == Items::RawGold ||
                   input == Blocks::AncientDebris || input == Blocks::CoalOre || input == Blocks::DiamondOre ||
                   input == Blocks::EmeraldOre || input == Blocks::LapisOre || input == Blocks::RedstoneOre ||
                   input == Blocks::NetherQuartzOre || input == Blocks::NetherGoldOre ||
                   (!isBlockStack(input) && (made == Items::IronNugget || made == Items::GoldNugget));
        }
    }
    return false;
}

Smelting::Kind Smelting::kindOf(BlockId block)
{
    block = furnaceBase(block);
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
        burnTotal = Smelting::burnSeconds(fuel->id) * (kind == Smelting::Kind::Furnace ? 1.0f : 0.5f);
        burnLeft = burnTotal;

        if (fuel->id == Items::LavaBucket)
        {
            fuel->id = Items::Bucket;
            fuel->count = 1;
        }
        else
        {
            fuel->count -= 1;
            if (fuel->count <= 0) fuel->clear();
        }
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
