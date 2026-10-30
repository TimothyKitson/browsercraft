#include "Frames.h"

ItemStack& Frames::at(const glm::ivec3& block)
{
    return m_frames[key(block)];
}

bool Frames::has(const glm::ivec3& block) const
{
    return m_frames.find(key(block)) != m_frames.end();
}

const ItemStack* Frames::find(const glm::ivec3& block) const
{
    const auto it = m_frames.find(key(block));
    return (it == m_frames.end()) ? nullptr : &it->second;
}

void Frames::remove(const glm::ivec3& block)
{
    m_frames.erase(key(block));
}

void Frames::putIn(int dimension, const glm::ivec3& block, const ItemStack& held)
{
    m_frames[{ dimension, block.x, block.y, block.z }] = held;
}

std::vector<Frames::Near> Frames::near(const glm::vec3& around, float reach) const
{
    std::vector<Near> out;
    const float squared = reach * reach;

    for (const auto& entry : m_frames)
    {
        if (entry.first[0] != m_dimension) continue;
        if (entry.second.empty()) continue;

        const glm::vec3 middle(static_cast<float>(entry.first[1]) + 0.5f,
                               static_cast<float>(entry.first[2]) + 0.5f,
                               static_cast<float>(entry.first[3]) + 0.5f);
        const glm::vec3 away = middle - around;
        if (glm::dot(away, away) > squared) continue;

        out.push_back(Near{ glm::ivec3(entry.first[1], entry.first[2], entry.first[3]),
                            entry.second });
    }
    return out;
}
