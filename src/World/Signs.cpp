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
    m_looks.erase(key(block));
}

Signs::Look Signs::look(const glm::ivec3& block) const
{
    const auto it = m_looks.find(key(block));
    return it == m_looks.end() ? Look{} : it->second;
}

void Signs::setLook(const glm::ivec3& block, const Look& look)
{
    if (plain(look)) m_looks.erase(key(block));
    else m_looks[key(block)] = look;
}

void Signs::lookIn(int dimension, const glm::ivec3& block, const Look& look)
{
    if (plain(look)) return;
    m_looks[{ dimension, block.x, block.y, block.z }] = look;
}

unsigned int Signs::textRgb(int colour)
{
    static const unsigned int TEXT[16] = {
        0xFFFFFFu, 0xFF681Fu, 0xFF00FFu, 0x9AC0CDu, 0xFFFF00u, 0xBFFF00u, 0xFF69B4u, 0x808080u,
        0xD3D3D3u, 0x00FFFFu, 0xA020F0u, 0x0000FFu, 0x8B4513u, 0x00FF00u, 0xFF0000u, 0x000000u,
    };
    return TEXT[colour < 0 || colour > 15 ? 15 : colour];
}

unsigned int Signs::shadedRgb(const Look& look)
{
    const unsigned int rgb = textRgb(look.colour);
    if (look.colour == PLAIN_COLOUR && look.glow) return 0xF0EBCCu;
    const unsigned int r = static_cast<unsigned int>(((rgb >> 16) & 0xFFu) * 0.4);
    const unsigned int g = static_cast<unsigned int>(((rgb >> 8) & 0xFFu) * 0.4);
    const unsigned int b = static_cast<unsigned int>((rgb & 0xFFu) * 0.4);
    return (r << 16) | (g << 8) | b;
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
