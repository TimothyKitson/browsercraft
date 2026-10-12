#include "EndPortal.h"
#include <cstdlib>

namespace EndPortal
{
    void frameOffsets(int out[FRAME_COUNT][2])
    {
        // The five-by-five ring with the four corners dropped: a corner
        // frame would touch the interior only diagonally, and Minecraft
        // leaves them out.
        int written = 0;
        for (int dx = -2; dx <= 2; ++dx)
        {
            for (int dz = -2; dz <= 2; ++dz)
            {
                if (std::abs(dx) != 2 && std::abs(dz) != 2) continue;  // inside
                if (std::abs(dx) == 2 && std::abs(dz) == 2) continue;  // corner
                out[written][0] = dx;
                out[written][1] = dz;
                ++written;
            }
        }
    }

    bool ringComplete(const Blocks& blocks, int centreX, int y, int centreZ)
    {
        int offsets[FRAME_COUNT][2];
        frameOffsets(offsets);

        for (const auto& offset : offsets)
            if (blocks.at(centreX + offset[0], y, centreZ + offset[1]) !=
                ::Blocks::EndPortalFrameFilled)
                return false;

        return true;
    }

    bool findRingAround(const Blocks& blocks, int x, int y, int z,
                        int& outCentreX, int& outCentreZ)
    {
        // Every centre this cell could be a frame of. Walking the twelve
        // offsets backwards from the cell is exact: trying every centre
        // within two would also test rings this frame is not part of.
        int offsets[FRAME_COUNT][2];
        frameOffsets(offsets);

        for (const auto& offset : offsets)
        {
            const int centreX = x - offset[0];
            const int centreZ = z - offset[1];
            if (!ringComplete(blocks, centreX, y, centreZ)) continue;

            outCentreX = centreX;
            outCentreZ = centreZ;
            return true;
        }

        return false;
    }
}
