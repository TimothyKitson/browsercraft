#include "Armour.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int PIECES_PER_TIER = 4;

    bool inNetheriteRange(StackId id)
    {
        return id >= Items::NetheriteHelmet && id <= Items::NetheriteBoots;
    }

    bool inArmourRange(StackId id)
    {
        return (id >= Items::LeatherHelmet && id <= Items::DiamondBoots) ||
               inNetheriteRange(id);
    }

    // Points per piece, by tier then piece: helmet, chestplate, leggings,
    // boots. These are Minecraft's own numbers.
    const int POINTS[5][PIECES_PER_TIER] = {
        { 1, 3, 2, 1 },   // leather:  7
        { 2, 5, 3, 1 },   // gold:    11
        { 2, 6, 5, 2 },   // iron:    15
        { 3, 8, 6, 3 },   // diamond: 20
        // The same twenty. Netherite is not better armour in
        // Minecraft either -- it is armour that does not wear out and
        // does not burn, and that is the whole of what it buys.
        { 3, 8, 6, 3 },   // netherite: 20
    };

    // How many hits each tier takes, as a multiplier on the piece's own
    // base. Minecraft gives each piece a different figure; these are
    // close enough that the ordering a player actually feels is right.
    const int DURABILITY[5] = { 80, 112, 240, 528, 666 };
    const float PIECE_SHARE[PIECES_PER_TIER] = { 0.69f, 1.0f, 0.94f, 0.81f };
}

Armour::Piece Armour::pieceOf(StackId id)
{
    // The elytra is worn where a chestplate goes and is not one: it
    // stops nothing, it is not made of anything, and it is the only
    // thing in the armour slots that is not armour.
    if (id == Items::Elytra) return Piece::Chestplate;

    // The four netherite pieces are at the end of the item list
    // rather than after the diamond ones, so their offset is taken
    // from their own start.
    if (inNetheriteRange(id))
        return static_cast<Piece>((id - Items::NetheriteHelmet) + 1);

    if (!inArmourRange(id)) return Piece::None;
    const int offset = (id - Items::LeatherHelmet) % PIECES_PER_TIER;
    return static_cast<Piece>(offset + 1);
}

Armour::Tier Armour::tierOf(StackId id)
{
    if (inNetheriteRange(id)) return Tier::Netherite;

    if (!inArmourRange(id)) return Tier::None;
    const int step = (id - Items::LeatherHelmet) / PIECES_PER_TIER;
    return static_cast<Tier>(step + 1);
}

int Armour::slotFor(StackId id)
{
    const Piece piece = pieceOf(id);
    if (piece == Piece::None) return -1;
    return static_cast<int>(piece) - 1;
}

int Armour::defencePoints(StackId id)
{
    if (!inArmourRange(id)) return 0;
    const int tier = static_cast<int>(tierOf(id)) - 1;
    const int piece = static_cast<int>(pieceOf(id)) - 1;
    return POINTS[tier][piece];
}

int Armour::maxDurability(StackId id)
{
    // Minecraft's: four hundred and thirty-two seconds of flight, and
    // it wears while gliding rather than while being hit.
    if (id == Items::Elytra) return 432;

    if (!inArmourRange(id)) return 0;
    const int tier = static_cast<int>(tierOf(id)) - 1;
    const int piece = static_cast<int>(pieceOf(id)) - 1;
    return static_cast<int>(static_cast<float>(DURABILITY[tier]) * PIECE_SHARE[piece]);
}

int Armour::reduce(int incoming, int points)
{
    if (incoming <= 0) return 0;

    const int capped = std::clamp(points, 0, 20);
    const float left = 1.0f - static_cast<float>(capped) * 0.04f;
    const int after = static_cast<int>(std::lround(static_cast<float>(incoming) * left));

    // Armour softens a blow; it never stops one outright.
    return std::max(1, after);
}
