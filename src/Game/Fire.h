#pragma once

#include "World/Block.h"

namespace Fire
{
    int burnChance(BlockId id);
    int spreadChance(BlockId id);
    inline bool feeds(BlockId id) { return burnChance(id) > 0; }
}
