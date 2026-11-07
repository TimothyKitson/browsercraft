#include "Fangs.h"
#include <cmath>

namespace Fangs
{
    int tick(Fang& fang)
    {
        int events = 0;
        if (--fang.warmup < 0)
        {
            if (fang.warmup == -8) events |= Bite;
            if (!fang.started)
            {
                fang.started = true;
                events |= Started;
            }
            if (--fang.ticksLeft < 0) events |= Gone;
            if (fang.ticksLeft == 14) events |= Crits;
        }
        return events;
    }

    float progress(const Fang& fang, float partial)
    {
        if (!fang.started) return 0.0f;
        const int i = fang.ticksLeft - 2;
        if (i <= 0) return 1.0f;
        return 1.0f - (static_cast<float>(i) - partial) / 20.0f;
    }

    std::vector<Spot> pattern(const glm::vec3& caster, const glm::vec3& target)
    {
        std::vector<Spot> spots;
        const float f = std::atan2(target.z - caster.z, target.x - caster.x);
        const glm::vec3 apart = target - caster;
        if (glm::dot(apart, apart) < 9.0f)
        {
            constexpr float PI = 3.14159265f;
            for (int i = 0; i < 5; ++i)
            {
                const float g = f + static_cast<float>(i) * PI * 0.4f;
                spots.push_back(Spot{ caster.x + std::cos(g) * 1.5f, caster.z + std::sin(g) * 1.5f, g, 0 });
            }
            for (int i = 0; i < 8; ++i)
            {
                const float g = f + static_cast<float>(i) * PI * 2.0f / 8.0f + PI * 2.0f / 5.0f;
                spots.push_back(Spot{ caster.x + std::cos(g) * 2.5f, caster.z + std::sin(g) * 2.5f, g, 3 });
            }
        }
        else
        {
            for (int i = 0; i < 16; ++i)
            {
                const float h = 1.25f * static_cast<float>(i + 1);
                spots.push_back(Spot{ caster.x + std::cos(f) * h, caster.z + std::sin(f) * h, f, i });
            }
        }
        return spots;
    }

    bool bites(const Fang& fang, const glm::vec3& feet, float halfWidth, float height)
    {
        constexpr float REACH = 0.25f + 0.2f;
        if (std::fabs(feet.x - fang.at.x) >= REACH + halfWidth) return false;
        if (std::fabs(feet.z - fang.at.z) >= REACH + halfWidth) return false;
        return feet.y < fang.at.y + 0.8f && feet.y + height > fang.at.y;
    }
}
