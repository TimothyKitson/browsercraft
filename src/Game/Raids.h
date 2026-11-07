#pragma once
#include "Entity/MobType.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Raids
{
    constexpr float PRE_RAID_SECONDS = 15.0f;
    constexpr float MAX_SECONDS = 2400.0f;
    constexpr float FINISH_SECONDS = 30.0f;
    constexpr float VICTORY_DELAY = 2.0f;
    constexpr float VILLAGE_RADIUS = 32.0f;
    constexpr float BAR_RANGE = 96.0f;
    constexpr float SPAWN_DISTANCE = 26.0f;
    constexpr int MAX_OMEN = 5;

    enum class Status { Ongoing, Victory, Loss, Stopped };

    int waveCount(int difficulty);
    int baseCount(MobId type, int wave);
    int bonusCount(MobId type, int wave, int difficulty, bool extra, uint32_t roll);
    std::vector<MobId> party(int wave, int waves, int difficulty, bool extra, uint32_t seed);
    bool isRaider(MobId type);
    int heroPrice(int count, int heroLevel);

    struct Raid
    {
        bool active = false;
        glm::vec3 centre{ 0.0f };
        int omenLevel = 0;
        int waves = 0;
        int spawned = 0;
        float preRaid = PRE_RAID_SECONDS;
        float elapsed = 0.0f;
        float postRaid = 0.0f;
        float finish = 0.0f;
        float totalHealth = 0.0f;
        bool started = false;
        Status status = Status::Ongoing;
        std::vector<uint32_t> raiders;

        bool extraWave() const { return omenLevel > 1; }
        int groups() const { return waves + (extraWave() ? 1 : 0); }
        bool moreGroups() const { return spawned < groups(); }
        bool finished() const { return status != Status::Ongoing; }
    };

    Raid begin(const glm::vec3& centre, int omenLevel, int difficulty);
    void addOmen(Raid& raid, int omenLevel);
    std::string barName(const Raid& raid, int raidersLeft);
}
