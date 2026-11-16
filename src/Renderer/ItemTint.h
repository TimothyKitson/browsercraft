#pragma once
#include <algorithm>
#include <cstdint>

namespace ItemTint
{
    constexpr float NEUTRAL = 1056832.0f;

    inline float packed(uint32_t rgb)
    {
        uint32_t out = 0u;
        for (int shift = 16, slot = 14; shift >= 0; shift -= 8, slot -= 7)
        {
            const float c = static_cast<float>((rgb >> shift) & 0xFFu);
            out |= static_cast<uint32_t>(std::clamp(c / 255.0f * 64.0f + 0.5f, 0.0f, 127.0f)) << slot;
        }
        return static_cast<float>(out);
    }
}
