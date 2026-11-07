#include "Raids.h"
#include <algorithm>
#include <cmath>

namespace
{
    struct Member
    {
        MobId type;
        int counts[8];
    };

    const Member MEMBERS[] = {
        { MobId::Vindicator, { 0, 0, 2, 0, 1, 4, 2, 5 } },
        { MobId::Evoker, { 0, 0, 0, 0, 0, 1, 1, 2 } },
        { MobId::Pillager, { 0, 4, 3, 3, 4, 4, 4, 2 } },
        { MobId::Witch, { 0, 0, 0, 0, 3, 0, 0, 1 } },
        { MobId::Ravager, { 0, 0, 0, 1, 0, 1, 0, 2 } },
    };
}

int Raids::waveCount(int difficulty)
{
    switch (difficulty)
    {
        case 1: return 3;
        case 2: return 5;
        case 3: return 7;
        default: return 0;
    }
}

int Raids::baseCount(MobId type, int wave)
{
    const int at = std::clamp(wave, 0, 7);
    for (const Member& member : MEMBERS)
        if (member.type == type) return member.counts[at];
    return 0;
}

int Raids::bonusCount(MobId type, int wave, int difficulty, bool extra, uint32_t roll)
{
    const bool easy = difficulty == 1;
    const bool normal = difficulty == 2;
    int spread = 0;
    switch (type)
    {
        case MobId::Vindicator:
        case MobId::Pillager:
            spread = easy ? static_cast<int>(roll % 2u) : normal ? 1 : 2;
            roll >>= 3;
            break;
        case MobId::Witch:
            if (easy || wave <= 2 || wave == 4) return 0;
            spread = 1;
            break;
        case MobId::Ravager:
            spread = !easy && extra ? 1 : 0;
            break;
        default:
            return 0;
    }
    return spread > 0 ? static_cast<int>((roll >> 4) % static_cast<uint32_t>(spread + 1)) : 0;
}

std::vector<MobId> Raids::party(int wave, int waves, int difficulty, bool extra, uint32_t seed)
{
    std::vector<MobId> out;
    for (const Member& member : MEMBERS)
    {
        seed = seed * 1664525u + 1013904223u;
        const int count = baseCount(member.type, extra ? waves : wave) +
                          bonusCount(member.type, wave, difficulty, extra, seed >> 6);
        for (int k = 0; k < count; ++k) out.push_back(member.type);
    }
    return out;
}

bool Raids::isRaider(MobId type)
{
    return type == MobId::Pillager || type == MobId::Vindicator || type == MobId::Evoker ||
           type == MobId::Witch || type == MobId::Ravager || type == MobId::Illusioner;
}

int Raids::heroPrice(int count, int heroLevel)
{
    if (count <= 0) return count;
    const double share = 0.3 + 0.0625 * static_cast<double>(std::max(0, heroLevel));
    const int cut = std::max(1, static_cast<int>(std::floor(share * static_cast<double>(count))));
    return std::max(1, count - cut);
}

Raids::Raid Raids::begin(const glm::vec3& centre, int omenLevel, int difficulty)
{
    Raid raid;
    raid.active = true;
    raid.centre = centre;
    raid.omenLevel = std::clamp(omenLevel, 1, MAX_OMEN);
    raid.waves = waveCount(difficulty);
    return raid;
}

void Raids::addOmen(Raid& raid, int omenLevel)
{
    raid.omenLevel = std::clamp(raid.omenLevel + omenLevel, 1, MAX_OMEN);
}

std::string Raids::barName(const Raid& raid, int raidersLeft)
{
    if (raid.status == Status::Victory) return "Raid - Victory";
    if (raid.status == Status::Loss) return "Raid - Defeat";
    if (raidersLeft > 0 && raidersLeft <= 2) return "Raid - Raiders Remaining: " + std::to_string(raidersLeft);
    return "Raid";
}
