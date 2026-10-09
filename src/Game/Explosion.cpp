#include "Explosion.h"
#include <algorithm>

float Explosion::falloff(float distance, float radius)
{
    if (radius <= 0.0f) return 0.0f;
    if (distance >= radius) return 0.0f;
    if (distance <= 0.0f) return 1.0f;

    // Linear from the middle out. Minecraft uses rays and a squared
    // term; the difference at this scale is a block either way, and
    // this is the version anyone can reason about.
    return 1.0f - distance / radius;
}

bool Explosion::destroys(BlockId block, float distance, float radius)
{
    if (block == Blocks::Air) return false;
    if (distance >= radius) return false;

    // Blast resistance is not mining time. In Minecraft they are two
    // separate numbers -- obsidian is twelve hundred against stone's
    // six, but only four times slower to mine -- so the handful of
    // blocks that shrug off a blast are named rather than derived from
    // a hardness that says nothing about it.
    switch (block)
    {
        case Blocks::Bedrock:
        case Blocks::Obsidian:
            return false;
        default: break;
    }

    // A liquid absorbs a blast instead of being thrown about by it,
    // which is why standing in water saves your house.
    if (isLiquid(block)) return false;

    const float strength = falloff(distance, radius);

    // The harder the block, the closer the blast has to be. Obsidian at
    // fifty is past anything a creeper can manage; stone at one and a
    // half goes anywhere but the very edge.
    const float hardness = blockInfo(block).hardness;
    if (hardness < 0.0f) return false;          // unbreakable

    return strength > hardness / 12.0f;
}
