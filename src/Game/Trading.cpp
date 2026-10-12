#include "Trading.h"
#include "World/Block.h"

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

    std::vector<Offer> offersFor(Profession trade)
    {
        switch (trade)
        {
        case Profession::Farmer:
            return {
                // Buys what a field produces, sells what you would
                // rather not spend a field's worth of time on.
                { Items::Wheat, 18, Blocks::Air, 0, Items::Emerald, 1 },
                { Blocks::Pumpkin, 6, Blocks::Air, 0, Items::Emerald, 1 },
                { Items::Emerald, 1, Blocks::Air, 0, Items::Bread, 6 },
                { Items::Emerald, 1, Blocks::Air, 0, Items::WheatSeeds, 8 },
            };

        case Profession::Librarian:
            return {
                { Items::Paper, 24, Blocks::Air, 0, Items::Emerald, 1 },
                { Items::Book, 4, Blocks::Air, 0, Items::Emerald, 1 },
                { Items::Emerald, 9, Blocks::Air, 0, Blocks::Bookshelf, 1 },
                // The one trade that matters: a table costs two diamond
                // and an age of mining, or this.
                { Items::Emerald, 24, Items::Book, 1, Blocks::EnchantingTable, 1 },
            };

        case Profession::Toolsmith:
            return {
                { Items::Coal, 15, Blocks::Air, 0, Items::Emerald, 1 },
                { Items::IronIngot, 4, Blocks::Air, 0, Items::Emerald, 1 },
                { Items::Emerald, 7, Blocks::Air, 0, Items::IronPickaxe, 1 },
                { Items::Emerald, 18, Items::Diamond, 1, Items::DiamondPickaxe, 1 },
            };

        case Profession::Fletcher:
            return {
                { Items::Stick, 32, Blocks::Air, 0, Items::Emerald, 1 },
                { Items::Feather, 16, Blocks::Air, 0, Items::Emerald, 1 },
                { Items::Emerald, 1, Blocks::Air, 0, Items::Arrow, 8 },
                { Items::Emerald, 3, Blocks::Air, 0, Items::Bow, 1 },
            };

        default: break;
        }
        return {};
    }

    bool affordable(const Offer& offer, int haveWant, int haveAlso)
    {
        if (!offer.valid()) return false;
        if (haveWant < offer.wantCount) return false;
        if (offer.also != Blocks::Air && haveAlso < offer.alsoCount) return false;
        return true;
    }
}
