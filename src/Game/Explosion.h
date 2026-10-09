#pragma once
#include "World/Block.h"
#include <glm/glm.hpp>

// What a blast does.
//
// Both questions it answers -- how hard something is hit at a given
// distance, and whether a block survives -- are pure, so --selftest can
// set off a creeper in an empty room and check the shape of the damage
// without a world to flatten.
namespace Explosion
{
    // A creeper's blast, in blocks.
    constexpr float CREEPER_RADIUS = 3.2f;
    constexpr int CREEPER_DAMAGE = 24;

    // How much of the full damage something takes at `distance` from the
    // middle. One at the centre, nothing at the edge, and falling off
    // the whole way between.
    float falloff(float distance, float radius);

    inline int damageAt(float distance, float radius, int fullDamage)
    {
        return static_cast<int>(static_cast<float>(fullDamage) * falloff(distance, radius));
    }

    // Whether a blast at this strength takes the block out. Bedrock
    // never goes, obsidian needs more than a creeper has, and water
    // smothers a blast rather than being blown away by it.
    bool destroys(BlockId block, float distance, float radius);
}
