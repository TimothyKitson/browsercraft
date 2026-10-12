#include "Cane.h"

namespace
{
    bool isPlantable(BlockId id)
    {
        return id == ::Blocks::Sand || id == ::Blocks::Dirt ||
               id == ::Blocks::Grass || id == ::Blocks::SnowGrass;
    }
}

namespace Cane
{
    bool groundIsWatered(const Blocks& blocks, int x, int y, int z)
    {
        if (!isPlantable(blocks.at(x, y, z))) return false;

        return blocks.at(x - 1, y, z) == ::Blocks::Water ||
               blocks.at(x + 1, y, z) == ::Blocks::Water ||
               blocks.at(x, y, z - 1) == ::Blocks::Water ||
               blocks.at(x, y, z + 1) == ::Blocks::Water;
    }

    int stackHeight(const Blocks& blocks, int x, int y, int z)
    {
        if (blocks.at(x, y, z) != ::Blocks::SugarCane) return 0;

        int height = 1;
        // Bounded by MAX_HEIGHT rather than walking down for ever: a
        // column of cane taller than the rules allow should still
        // report a height that stops it growing, not run to bedrock.
        while (height <= MAX_HEIGHT &&
               blocks.at(x, y - height, z) == ::Blocks::SugarCane)
            ++height;
        return height;
    }

    bool canStandAt(const Blocks& blocks, int x, int y, int z)
    {
        const BlockId below = blocks.at(x, y - 1, z);

        if (below == ::Blocks::SugarCane)
            return stackHeight(blocks, x, y - 1, z) < MAX_HEIGHT;

        return groundIsWatered(blocks, x, y - 1, z);
    }

    bool canGrowAt(const Blocks& blocks, int x, int y, int z)
    {
        if (blocks.at(x, y, z) != ::Blocks::SugarCane) return false;
        if (blocks.at(x, y + 1, z) != ::Blocks::Air) return false;
        return stackHeight(blocks, x, y, z) < MAX_HEIGHT;
    }
}
