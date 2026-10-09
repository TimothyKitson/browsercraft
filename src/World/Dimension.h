#pragma once
#include <cstdint>

// Which world a generator is building. Each has its own terrain shape, and
// chunks are saved under their own folder so the three never collide.
//
// In its own header because the mobs need it too, and a mob type has no
// business pulling in the whole terrain generator to find out what a
// dimension is.
enum class Dimension : uint8_t
{
    Overworld,
    Nether,
    End
};

const char* dimensionName(Dimension dimension);
