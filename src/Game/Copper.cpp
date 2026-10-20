#include "Copper.h"
#include "World/Noise.h"

BlockId Copper::next(BlockId id)
{
    switch (id)
    {
        case Blocks::CopperBlock:     return Blocks::ExposedCopper;
        case Blocks::ExposedCopper:   return Blocks::WeatheredCopper;
        case Blocks::WeatheredCopper: return Blocks::OxidizedCopper;
        default:                      return id;
    }
}

bool Copper::ages(BlockId id)
{
    return next(id) != id;
}

bool Copper::ticks(int x, int y, int z, uint32_t seed, int tick)
{
    return hashToFloat(hashCoords(x + tick * 7919, y, z, seed + 99001)) < AGE_CHANCE;
}
