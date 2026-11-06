#include "Trading.h"
#include "World/Block.h"
#include "Enchanting.h"

namespace Trading
{
    const char* name(Profession trade)
    {
        switch (trade)
        {
        case Profession::Farmer:    return "Farmer";
        case Profession::Librarian: return "Librarian";
        case Profession::Toolsmith: return "Toolsmith";
        case Profession::Fletcher:  return "Fletcher";
        case Profession::Armorer:   return "Armorer";
        case Profession::Butcher:   return "Butcher";
        case Profession::Cartographer: return "Cartographer";
        case Profession::Cleric:    return "Cleric";
        case Profession::Fisherman: return "Fisherman";
        case Profession::Leatherworker: return "Leatherworker";
        case Profession::Mason:     return "Mason";
        case Profession::Nitwit:    return "Nitwit";
        case Profession::Shepherd:  return "Shepherd";
        case Profession::Weaponsmith: return "Weaponsmith";
        default: break;
        }
        return "Villager";
    }

    const char* textureName(Profession trade)
    {
        static const char* NAMES[] = { "farmer", "librarian", "toolsmith", "fletcher", "armorer", "butcher",
                                       "cartographer", "cleric", "fisherman", "leatherworker", "mason",
                                       "nitwit", "shepherd", "weaponsmith" };
        const int index = static_cast<int>(trade);
        if (index < 0 || index >= static_cast<int>(Profession::Count)) return "farmer";
        return NAMES[index];
    }

    bool jobSite(BlockId block, Profession& trade)
    {
        if (isComposter(block)) { trade = Profession::Farmer; return true; }
        if (isLectern(block)) { trade = Profession::Librarian; return true; }
        if (isCauldron(block)) { trade = Profession::Leatherworker; return true; }
        switch (furnaceBase(block))
        {
            case Blocks::BlastFurnace: case Blocks::BlastFurnaceLit: trade = Profession::Armorer; return true;
            case Blocks::Smoker: case Blocks::SmokerLit: trade = Profession::Butcher; return true;
            default: break;
        }
        switch (block)
        {
            case Blocks::BrewingStand: trade = Profession::Cleric; return true;
            case Blocks::Barrel: case Blocks::BarrelOpen: trade = Profession::Fisherman; return true;
            case Blocks::Grindstone: trade = Profession::Weaponsmith; return true;
            default: return false;
        }
    }

    Profession professionFor(uint32_t seed)
    {
        // Stirred rather than taken straight: the seeds these come from
        // are small and consecutive, and the low bits of consecutive
        // numbers would hand out professions in a repeating cycle.
        uint32_t h = seed * 0x9E3779B9u;
        h ^= h >> 16; h *= 0x7FEB352Du;
        h ^= h >> 15; h *= 0x846CA68Bu;
        h ^= h >> 16;
        return static_cast<Profession>(h % static_cast<uint32_t>(Profession::Count));
    }

    const char* rankName(int rank)
    {
        static const char* NAMES[RANKS] = { "Novice", "Apprentice", "Journeyman", "Expert", "Master" };
        return NAMES[rank < 0 ? 0 : (rank >= RANKS ? RANKS - 1 : rank)];
    }

    int xpForRank(int rank)
    {
        static const int NEEDED[RANKS] = { 0, 12, 60, 140, 240 };
        return NEEDED[rank < 0 ? 0 : (rank >= RANKS ? RANKS - 1 : rank)];
    }

    int rankFor(int xp)
    {
        int rank = 0;
        while (rank + 1 < RANKS && xp >= xpForRank(rank + 1)) ++rank;
        return rank;
    }

    namespace
    {
        Deal buy(StackId want, int count, int rank, int uses = 16)
        {
            Deal d;
            d.offer = { want, count, Blocks::Air, 0, Items::Emerald, 1 };
            d.rank = rank;
            d.maxUses = uses;
            d.villagerXp = 2 + rank * 3;
            d.playerXp = 1 + rank;
            return d;
        }

        Deal sell(int emeralds, StackId give, int count, int rank, int uses = 12,
                  StackId also = Blocks::Air, int alsoCount = 0, uint32_t enchantments = 0)
        {
            Deal d;
            d.offer = { Items::Emerald, emeralds, also, alsoCount, give, count, enchantments };
            d.rank = rank;
            d.maxUses = uses;
            d.villagerXp = 2 + rank * 3;
            d.playerXp = 1 + rank;
            return d;
        }

        uint32_t enchanted(Enchanting::Kind kind, int level)
        {
            return Enchanting::withLevel(0u, kind, level);
        }
    }

    std::vector<Deal> dealsFor(Profession trade)
    {
        using Enchanting::Kind;
        switch (trade)
        {
        case Profession::Farmer:
            return {
                buy(Items::Wheat, 20, 0),
                sell(1, Items::Bread, 6, 0),
                buy(Items::Carrot, 22, 1),
                buy(Items::Potato, 26, 1),
                buy(Blocks::Pumpkin, 6, 2, 12),
                sell(1, Items::Apple, 4, 2),
                buy(Blocks::Melon, 4, 3, 12),
                sell(3, Items::Cookie, 18, 3),
                buy(Items::Beetroot, 15, 4),
                sell(1, Items::Cake, 1, 4, 6),
            };

        case Profession::Librarian:
            return {
                buy(Items::Paper, 24, 0),
                sell(9, Blocks::Bookshelf, 1, 0),
                buy(Items::Book, 4, 1, 12),
                sell(5, Items::Book, 1, 1, 8, Items::Book, 1, enchanted(Kind::Efficiency, 1)),
                buy(Items::InkSac, 5, 2, 12),
                sell(1, Blocks::Glass, 4, 2),
                sell(12, Items::Book, 1, 3, 6, Items::Book, 1, enchanted(Kind::Unbreaking, 2)),
                sell(1, Blocks::Lantern, 1, 3),
                sell(24, Blocks::EnchantingTable, 1, 4, 4, Items::Book, 1),
                sell(20, Items::Book, 1, 4, 4, Items::Book, 1, enchanted(Kind::SilkTouch, 1)),
            };

        case Profession::Toolsmith:
            return {
                buy(Items::Coal, 15, 0),
                sell(1, Items::StonePickaxe, 1, 0),
                buy(Items::IronIngot, 4, 1, 12),
                sell(2, Items::Bucket, 1, 1),
                buy(Items::Flint, 24, 2),
                sell(7, Items::IronPickaxe, 1, 2, 6),
                buy(Items::GoldIngot, 3, 3, 12),
                sell(8, Items::IronAxe, 1, 3, 4, Blocks::Air, 0, enchanted(Kind::Efficiency, 2)),
                sell(18, Items::DiamondPickaxe, 1, 4, 3, Items::Diamond, 1),
                sell(14, Items::DiamondAxe, 1, 4, 3, Items::Diamond, 1),
            };

        case Profession::Fletcher:
            return {
                buy(Items::Stick, 32, 0),
                sell(1, Items::Arrow, 8, 0),
                buy(Items::Feather, 16, 1),
                sell(2, Items::Bow, 1, 1),
                buy(Items::StringItem, 14, 2),
                sell(1, Items::Flint, 10, 2, 12, Blocks::Gravel, 10),
                sell(5, Items::Arrow, 24, 3),
                sell(4, Items::Bow, 1, 3, 4, Blocks::Air, 0, enchanted(Kind::Power, 1)),
                buy(Items::Feather, 24, 4),
                sell(9, Items::Bow, 1, 4, 3, Blocks::Air, 0, enchanted(Kind::Unbreaking, 3)),
            };

        case Profession::Armorer:
            return {
                buy(Items::Coal, 15, 0),
                sell(5, Items::IronHelmet, 1, 0),
                sell(9, Items::IronChestplate, 1, 0),
                sell(7, Items::IronLeggings, 1, 0),
                sell(4, Items::IronBoots, 1, 0),
                buy(Items::IronIngot, 4, 1, 12),
                buy(Items::LavaBucket, 1, 2, 12),
                buy(Items::Diamond, 1, 2, 12),
                sell(5, Items::Shield, 1, 3),
                sell(14, Items::DiamondLeggings, 1, 4, 3, Blocks::Air, 0, enchanted(Kind::Protection, 2)),
                sell(16, Items::DiamondChestplate, 1, 4, 3, Blocks::Air, 0, enchanted(Kind::Protection, 2)),
            };

        case Profession::Butcher:
            return {
                buy(Items::RawChicken, 14, 0),
                buy(Items::RawPorkchop, 7, 0),
                buy(Items::RawRabbit, 4, 0),
                sell(1, Items::CookedRabbit, 1, 0),
                buy(Items::Coal, 15, 1),
                sell(1, Items::CookedPorkchop, 5, 1),
                sell(1, Items::CookedChicken, 8, 1),
                buy(Items::RawMutton, 7, 2),
                buy(Items::RawBeef, 10, 2),
                sell(1, Items::CookedMutton, 6, 3),
                sell(1, Items::Steak, 6, 4),
            };

        case Profession::Cartographer:
            return {
                buy(Items::Paper, 24, 0),
                sell(7, Items::Book, 3, 0),
                buy(Blocks::Glass, 11, 1),
                sell(7, Items::ItemFrame, 1, 2),
                buy(Items::EyeOfEnder, 1, 3, 6),
                sell(3, Items::Paper, 18, 3),
                sell(8, Blocks::Bookshelf, 1, 4),
            };

        case Profession::Cleric:
            return {
                buy(Items::GoldIngot, 3, 0, 12),
                sell(1, Items::Redstone, 2, 0),
                buy(Items::GlassBottle, 9, 1),
                sell(1, Items::LapisLazuli, 1, 1),
                buy(Items::NetherWart, 22, 2),
                sell(4, Blocks::Glowstone, 1, 2),
                buy(Items::GhastTear, 2, 3, 12),
                sell(5, Items::EnderPearl, 1, 3),
                buy(Items::BlazePowder, 4, 4, 12),
                sell(3, Items::GlowstoneDust, 4, 4),
            };

        case Profession::Fisherman:
            return {
                buy(Items::StringItem, 20, 0),
                buy(Items::Coal, 10, 0),
                sell(1, Items::CookedCod, 6, 0, 16, Items::RawCod, 6),
                buy(Items::RawCod, 15, 1),
                sell(2, Blocks::Campfire, 1, 1),
                buy(Items::RawSalmon, 13, 2),
                sell(1, Items::CookedSalmon, 6, 2, 16, Items::RawSalmon, 6),
                buy(Items::Pufferfish, 4, 3, 12),
                buy(Items::Boat, 1, 4, 12),
            };

        case Profession::Leatherworker:
            return {
                buy(Items::Leather, 6, 0),
                sell(3, Items::LeatherLeggings, 1, 0),
                sell(7, Items::LeatherChestplate, 1, 0),
                buy(Items::Flint, 26, 1),
                sell(5, Items::LeatherHelmet, 1, 1),
                sell(4, Items::LeatherBoots, 1, 1),
                buy(Items::Leather, 9, 2),
                sell(7, Items::LeatherChestplate, 1, 3, 6, Blocks::Air, 0, enchanted(Kind::Protection, 1)),
                sell(5, Items::LeatherHelmet, 1, 4, 6, Blocks::Air, 0, enchanted(Kind::Protection, 1)),
                sell(6, Items::LeatherHorseArmor, 1, 3),
                sell(6, Items::Saddle, 1, 4),
            };

        case Profession::Mason:
            return {
                buy(Blocks::Stone, 20, 0),
                sell(1, Blocks::Bricks, 10, 0),
                buy(Blocks::Granite, 16, 1),
                buy(Blocks::Andesite, 16, 1),
                buy(Blocks::Diorite, 16, 2),
                sell(1, Blocks::Calcite, 4, 2),
                sell(1, Blocks::DripstoneBlock, 4, 3),
                sell(1, Blocks::Cobblestone, 16, 4),
            };

        case Profession::Nitwit:
            return {};

        case Profession::Shepherd:
            return {
                buy(Blocks::Wool, 18, 0),
                buy(Items::DyeWhite, 12, 1),
                buy(Items::DyeGray, 12, 1),
                buy(Items::DyeBlack, 12, 1),
                sell(1, Blocks::WoolOrange, 1, 1),
                sell(1, Blocks::WoolLightBlue, 1, 1),
                sell(1, Blocks::WoolLime, 1, 1),
                buy(Items::DyeLightBlue, 12, 2),
                buy(Items::DyeLime, 12, 2),
                sell(3, bedOf(0, 0, false), 1, 2),
                sell(1, Blocks::WoolMagenta, 1, 3),
                sell(1, Blocks::WoolYellow, 1, 3),
                sell(2, bedOf(14, 0, false), 1, 4),
            };

        case Profession::Weaponsmith:
            return {
                buy(Items::Coal, 15, 0),
                sell(3, Items::IronAxe, 1, 0),
                sell(2, Items::IronSword, 1, 0, 12, Blocks::Air, 0, enchanted(Kind::Sharpness, 1)),
                buy(Items::IronIngot, 4, 1, 12),
                buy(Items::Flint, 24, 2),
                buy(Items::Diamond, 1, 3, 12),
                sell(12, Items::DiamondAxe, 1, 3, 3, Blocks::Air, 0, enchanted(Kind::Sharpness, 2)),
                sell(8, Items::DiamondSword, 1, 4, 3, Blocks::Air, 0, enchanted(Kind::Sharpness, 3)),
            };

        default: break;
        }
        return {};
    }

    std::vector<Offer> offersFor(Profession trade)
    {
        std::vector<Offer> offers;
        for (const Deal& deal : dealsFor(trade)) offers.push_back(deal.offer);
        return offers;
    }

    bool affordable(const Offer& offer, int haveWant, int haveAlso)
    {
        if (!offer.valid()) return false;
        if (haveWant < offer.wantCount) return false;
        if (offer.also != Blocks::Air && haveAlso < offer.alsoCount) return false;
        return true;
    }
}
