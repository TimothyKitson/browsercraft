#include "Signs.h"
#include <vector>

Signs::Text& Signs::at(const glm::ivec3& block)
{
    return m_signs[key(block)];
}

bool Signs::has(const glm::ivec3& block) const
{
    return m_signs.find(key(block)) != m_signs.end();
}

const Signs::Text* Signs::find(const glm::ivec3& block) const
{
    const auto it = m_signs.find(key(block));
    return (it == m_signs.end()) ? nullptr : &it->second;
}

void Signs::remove(const glm::ivec3& block)
{
    m_signs.erase(key(block));
}

void Signs::putIn(int dimension, const glm::ivec3& block, const Text& text)
{
    m_signs[{ dimension, block.x, block.y, block.z }] = text;
}

bool Signs::blank(const Text& text)
{
    for (const std::string& line : text)
        if (!line.empty()) return false;
    return true;
}

std::vector<Signs::Near> Signs::near(const glm::vec3& around, float reach) const
{
    std::vector<Near> out;
    const float squared = reach * reach;

    for (const auto& entry : m_signs)
    {
        if (entry.first[0] != m_dimension) continue;

        const glm::vec3 middle(static_cast<float>(entry.first[1]) + 0.5f,
                               static_cast<float>(entry.first[2]) + 0.5f,
                               static_cast<float>(entry.first[3]) + 0.5f);
        const glm::vec3 away = middle - around;
        if (glm::dot(away, away) > squared) continue;

        out.push_back(Near{ glm::ivec3(entry.first[1], entry.first[2], entry.first[3]),
                            &entry.second });
    }
    return out;
}
