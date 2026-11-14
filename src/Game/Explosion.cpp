#include "Explosion.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>

#include "BlastData.inl"

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

float Explosion::resistance(BlockId block)
{
    static std::vector<float> cache;
    static std::vector<uint8_t> known;
    const size_t at = static_cast<size_t>(block);
    if (at >= cache.size())
    {
        cache.resize(at + 1, 0.0f);
        known.resize(at + 1, 0);
    }
    if (known[at]) return cache[at];
    float value = blockInfo(block).hardness < 0.0f ? 3600000.0f : blockInfo(block).hardness;
    if (block == Blocks::Air) value = 0.0f;
    else if (isLiquid(block) || isFluid(block)) value = 100.0f;
    else
    {
        std::string name = blockInfo(block).name ? blockInfo(block).name : "";
        for (char& c : name) c = c == ' ' ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        name.erase(std::remove(name.begin(), name.end(), '\''), name.end());
        for (int i = 0; i < RESISTANCE_COUNT; ++i)
            if (name == RESISTANCES[i].id)
            {
                value = RESISTANCES[i].value;
                break;
            }
    }
    cache[at] = value;
    known[at] = 1;
    return value;
}

std::vector<glm::ivec3> Explosion::rays(const glm::vec3& centre, float power, uint32_t seed, const BlockAt& blockAt)
{
    std::vector<glm::ivec3> out;
    std::unordered_set<int64_t> seen;
    uint32_t state = seed ? seed : 1u;
    auto unit = [&]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(state >> 8) / 16777216.0f;
    };
    for (int j = 0; j < 16; ++j)
        for (int k = 0; k < 16; ++k)
            for (int l = 0; l < 16; ++l)
            {
                if (j != 0 && j != 15 && k != 0 && k != 15 && l != 0 && l != 15) continue;
                glm::vec3 step(static_cast<float>(j) / 15.0f * 2.0f - 1.0f, static_cast<float>(k) / 15.0f * 2.0f - 1.0f,
                               static_cast<float>(l) / 15.0f * 2.0f - 1.0f);
                step = glm::normalize(step) * 0.3f;
                float left = power * (0.7f + unit() * 0.6f);
                glm::vec3 p = centre;
                for (; left > 0.0f; left -= 0.22500001f)
                {
                    const glm::ivec3 cell(static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y)), static_cast<int>(std::floor(p.z)));
                    const BlockId here = blockAt(cell.x, cell.y, cell.z);
                    if (here != Blocks::Air) left -= (resistance(here) + 0.3f) * 0.3f;
                    if (left > 0.0f && here != Blocks::Air && !isLiquid(here) && !isFluid(here))
                    {
                        const int64_t key = (static_cast<int64_t>(cell.x) & 0x3FFFFF) | ((static_cast<int64_t>(cell.y) & 0xFFFFF) << 22) |
                                            ((static_cast<int64_t>(cell.z) & 0x3FFFFF) << 42);
                        if (seen.insert(key).second) out.push_back(cell);
                    }
                    p += step;
                }
            }
    return out;
}

float Explosion::exposure(const glm::vec3& centre, const glm::vec3& boxMin, const glm::vec3& boxMax, const BlockAt& blockAt)
{
    const glm::vec3 size = boxMax - boxMin;
    const float dx = 1.0f / (size.x * 2.0f + 1.0f), dy = 1.0f / (size.y * 2.0f + 1.0f), dz = 1.0f / (size.z * 2.0f + 1.0f);
    if (dx < 0.0f || dy < 0.0f || dz < 0.0f) return 0.0f;
    const float ox = (1.0f - std::floor(1.0f / dx) * dx) / 2.0f, oz = (1.0f - std::floor(1.0f / dz) * dz) / 2.0f;
    int clear = 0, total = 0;
    for (float a = 0.0f; a <= 1.0f; a += dx)
        for (float b = 0.0f; b <= 1.0f; b += dy)
            for (float c = 0.0f; c <= 1.0f; c += dz)
            {
                const glm::vec3 from(boxMin.x + (boxMax.x - boxMin.x) * a + ox, boxMin.y + (boxMax.y - boxMin.y) * b,
                                     boxMin.z + (boxMax.z - boxMin.z) * c + oz);
                const glm::vec3 span = centre - from;
                const float length = glm::length(span);
                const int steps = std::max(1, static_cast<int>(length * 4.0f));
                bool blocked = false;
                for (int s = 1; s < steps && !blocked; ++s)
                {
                    const glm::vec3 p = from + span * (static_cast<float>(s) / static_cast<float>(steps));
                    const BlockId here = blockAt(static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y)), static_cast<int>(std::floor(p.z)));
                    if (here != Blocks::Air && isSolid(here) && !isLiquid(here)) blocked = true;
                }
                if (!blocked) ++clear;
                ++total;
            }
    return total > 0 ? static_cast<float>(clear) / static_cast<float>(total) : 0.0f;
}

float Explosion::impact(float distance, float power, float exposed)
{
    const float reach = power * 2.0f;
    if (reach <= 0.0f || distance > reach) return 0.0f;
    return (1.0f - distance / reach) * exposed;
}

int Explosion::entityDamage(float distance, float power, float exposed)
{
    const float hit = impact(distance, power, exposed);
    if (hit <= 0.0f) return 0;
    return static_cast<int>((hit * hit + hit) / 2.0f * 7.0f * power * 2.0f + 1.0f);
}

