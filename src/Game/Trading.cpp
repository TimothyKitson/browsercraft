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
        default: break;
        }
        return "Villager";
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
