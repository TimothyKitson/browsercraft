#pragma once
#include "Items.h"
#include <cstdint>
#include <string>
#include <vector>

// Villager trades: what each sort of villager will deal in, and at what
// price.
//
// A villager's trade list is worked out from its profession and a seed
// rather than stored, so two villagers of the same trade in the same
// world always offer the same things and nothing has to be saved with
// the mob. All of it is arithmetic on item ids, so --selftest can walk
// every profession's whole list without a world to stand them in.
namespace Trading
{
    // The professions, in no particular order beyond keeping Farmer
    // first so an unseeded villager is the harmless one.
    enum class Profession : uint8_t
    {
        Farmer,
        Librarian,
        Toolsmith,
        Fletcher,
        Armorer,
        Butcher,
        Cartographer,
        Cleric,
        Fisherman,
        Leatherworker,
        Mason,
        Nitwit,
        Shepherd,
        Weaponsmith,
        Count
    };

    const char* name(Profession trade);

    // Which trade a villager seeded like this follows. Stable for a
    // given seed, so a villager keeps its profession across a save
    // without one being written down.
    Profession professionFor(uint32_t seed);
    const char* textureName(Profession trade);
    bool jobSite(BlockId block, Profession& trade);

    // One deal: hand over `wantCount` of `want` -- and, where there is
    // one, `alsoCount` of `also` -- and get `giveCount` of `give`.
    struct Offer
    {
        StackId want = Blocks::Air;
        int wantCount = 0;
        StackId also = Blocks::Air;
        int alsoCount = 0;
        StackId give = Blocks::Air;
        int giveCount = 0;
        uint32_t giveEnchantments = 0;
        int giveDamage = 0;

        bool valid() const
        {
            return want != Blocks::Air && wantCount > 0 &&
                   give != Blocks::Air && giveCount > 0;
        }
    };

    // Every deal this profession offers. The list is short on purpose:
    // four trades a villager, which is about what a Minecraft villager
    // shows before it has been levelled up.
    std::vector<Offer> offersFor(Profession trade);

    // A villager's standing: it starts a Novice and works up to Master
    // by trading, and each rank opens more of its list.
    constexpr int RANKS = 5;
    const char* rankName(int rank);
    int xpForRank(int rank);
    int rankFor(int xp);

    // One line of a villager's list: the deal, the rank it opens at,
    // how many times it can be struck before the villager runs out, and
    // the experience it earns the villager and the player.
    struct Deal
    {
        Offer offer;
        int rank = 0;
        int maxUses = 12;
        int villagerXp = 2;
        int playerXp = 1;
        float multiplier = 0.05f;
    };

    enum class FactoryKind : uint8_t { Buy, Sell, Process, Book, Tool, Dyed, Map, Tipped, Stew, Boat };
    struct Factory
    {
        uint8_t profession;
        uint8_t level;
        FactoryKind kind;
        const char* item;
        int count;
        int price;
        int maxUses;
        int xp;
        float multiplier;
        const char* item2;
        int count2;
        int extra;
    };
    constexpr int VILLAGER_TYPES = 7;
    constexpr int TRADES_PER_LEVEL = 2;

    std::vector<Deal> dealsFor(Profession trade);
    std::vector<Deal> dealsFor(Profession trade, uint32_t seed, int villagerType);
    std::vector<Deal> wandererDeals(uint32_t seed);
    std::vector<std::string> unknownTradeItems();

    // How long a villager takes to restock everything it has sold out
    // of, in seconds of play.
    constexpr float RESTOCK_SECONDS = 600.0f;

    // Whether a pack holding these counts could pay for this offer.
    // Taken as counts rather than an Inventory so the rule can be
    // checked without one.
    bool affordable(const Offer& offer, int haveWant, int haveAlso);
}
