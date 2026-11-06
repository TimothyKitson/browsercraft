#include "Beacon.h"
#include <algorithm>

int Beacon::tiersUnder(const Ground& ground, int x, int y, int z)
{
    int complete = 0;

    for (int tier = 1; tier <= MAX_TIERS; ++tier)
    {
        const int half = tier;
        const int layerY = y - tier;

        bool whole = true;
        for (int dx = -half; dx <= half && whole; ++dx)
            for (int dz = -half; dz <= half && whole; ++dz)
                if (!isBeaconBase(ground.at(x + dx, layerY, z + dz))) whole = false;

        // The first gap ends it. A pyramid with its bottom layer
        // missing is not a three-tier pyramid floating in the air.
        if (!whole) break;
        complete = tier;
    }

    return complete;
}

int Beacon::rangeFor(int tiers)
{
    if (tiers <= 0) return 0;
    return RANGE_BASE + RANGE_PER_TIER * std::min(tiers, MAX_TIERS);
}

bool Beacon::seesSky(const Ground& ground, int x, int y, int z, int worldTop)
{
    for (int above = y + 1; above < worldTop; ++above)
        if (isOpaque(ground.at(x, above, z))) return false;
    return true;
}

std::vector<Effects::Kind> Beacon::powersAt(int tier)
{
    switch (tier)
    {
        case 0: return { Effects::Kind::Speed, Effects::Kind::Haste };
        case 1: return { Effects::Kind::Resistance, Effects::Kind::JumpBoost };
        case 2: return { Effects::Kind::Strength };
        case 3: return { Effects::Kind::Regeneration };
        default: return {};
    }
}

bool Beacon::isPrimary(int power, int tiers)
{
    for (int tier = 0; tier < 3 && tier < tiers; ++tier)
        for (Effects::Kind kind : powersAt(tier))
            if (static_cast<int>(kind) == power) return true;
    return false;
}

bool Beacon::isSecondary(int power, int primary, int tiers)
{
    if (tiers < MAX_TIERS || !isPrimary(primary, tiers)) return false;
    return power == primary || power == static_cast<int>(Effects::Kind::Regeneration);
}

float Beacon::seconds(int tiers)
{
    return 9.0f + 2.0f * static_cast<float>(tiers);
}
