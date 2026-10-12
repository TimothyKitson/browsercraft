#pragma once
#include "Items.h"
#include <cstdint>
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
        Count
    };

    const char* name(Profession trade);

    // Which trade a villager seeded like this follows. Stable for a
    // given seed, so a villager keeps its profession across a save
    // without one being written down.
    Profession professionFor(uint32_t seed);

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

    // Whether a pack holding these counts could pay for this offer.
    // Taken as counts rather than an Inventory so the rule can be
    // checked without one.
    bool affordable(const Offer& offer, int haveWant, int haveAlso);
}
