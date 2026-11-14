#pragma once
#include "World/Block.h"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

namespace Explosion
{
    constexpr float CREEPER_RADIUS = 3.0f;
    constexpr int CREEPER_DAMAGE = 24;

    float falloff(float distance, float radius);

    inline int damageAt(float distance, float radius, int fullDamage)
    {
        return static_cast<int>(static_cast<float>(fullDamage) * falloff(distance, radius));
    }

    bool destroys(BlockId block, float distance, float radius);

    float resistance(BlockId block);

    using BlockAt = std::function<BlockId(int, int, int)>;
    std::vector<glm::ivec3> rays(const glm::vec3& centre, float power, uint32_t seed, const BlockAt& blockAt);
    float exposure(const glm::vec3& centre, const glm::vec3& boxMin, const glm::vec3& boxMax, const BlockAt& blockAt);
    float impact(float distance, float power, float exposed);
    int entityDamage(float distance, float power, float exposed);
}
