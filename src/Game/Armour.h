#pragma once
#include "Items.h"

// Armour: which slot a piece belongs in, how much it stops, and how long
// it lasts.
//
// Pure functions of an item id, like the tools, so --selftest can dress
// a player in every combination and check the sums without a world.
namespace Armour
{
    // In the order the slots run down the side of the inventory.
    enum class Piece { None, Helmet, Chestplate, Leggings, Boots };
    enum class Tier { None, Leather, Gold, Iron, Diamond };

    Piece pieceOf(StackId id);
    Tier tierOf(StackId id);

    inline bool isArmour(StackId id) { return pieceOf(id) != Piece::None; }

    // Which of the four armour slots this belongs in, or -1.
    int slotFor(StackId id);

    // Minecraft's armour points: the whole suit of leather is seven,
    // diamond is twenty.
    int defencePoints(StackId id);
    int maxDurability(StackId id);

    // What `incoming` becomes after `points` of armour. Minecraft takes
    // four percent off per point, so twenty points -- a full diamond
    // suit -- is eighty percent off. A blow never drops below one.
    int reduce(int incoming, int points);
}
