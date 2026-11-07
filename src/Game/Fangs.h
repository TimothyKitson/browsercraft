#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace Fangs
{
    constexpr float TICK = 0.05f;
    constexpr int DAMAGE = 6;

    struct Fang
    {
        glm::vec3 at{ 0.0f };
        float yaw = 0.0f;
        int warmup = 0;
        int ticksLeft = 22;
        bool started = false;
        int damage = DAMAGE;
    };

    enum Event
    {
        Started = 1,
        Bite = 2,
        Crits = 4,
        Gone = 8,
    };

    int tick(Fang& fang);
    float progress(const Fang& fang, float partial);

    struct Spot
    {
        float x = 0.0f, z = 0.0f;
        float yaw = 0.0f;
        int warmup = 0;
    };
    std::vector<Spot> pattern(const glm::vec3& caster, const glm::vec3& target);

    bool bites(const Fang& fang, const glm::vec3& feet, float halfWidth, float height);
}
