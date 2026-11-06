#include "Copper.h"
#include "World/Noise.h"

BlockId Copper::next(BlockId id)
{
    const CopperForm form = copperForm(id);
    if (form.family == CopperFamily::None || form.waxed || form.stage >= 3) return id;
    return copperWith(id, form.stage + 1, false);
}

BlockId Copper::waxed(BlockId id)
{
    const CopperForm form = copperForm(id);
    if (form.family == CopperFamily::None || form.waxed) return id;
    return copperWith(id, form.stage, true);
}

BlockId Copper::scraped(BlockId id)
{
    const CopperForm form = copperForm(id);
    if (form.family == CopperFamily::None) return id;
    if (form.waxed) return copperWith(id, form.stage, false);
    if (form.stage > 0) return copperWith(id, form.stage - 1, false);
    return id;
}

BlockId Copper::cleaned(BlockId id)
{
    const CopperForm form = copperForm(id);
    if (form.family == CopperFamily::None || form.waxed) return id;
    return copperWith(id, 0, false);
}

bool Copper::ages(BlockId id)
{
    return next(id) != id;
}

bool Copper::ticks(int x, int y, int z, uint32_t seed, int tick)
{
    return hashToFloat(hashCoords(x + tick * 7919, y, z, seed + 99001)) < AGE_CHANCE;
}
