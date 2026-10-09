#include "Chests.h"

void Chests::forgetIfEmpty(const glm::ivec3& block)
{
    const auto it = m_chests.find(key(block));
    if (it == m_chests.end()) return;

    for (const ItemStack& stack : it->second)
        if (!stack.empty()) return;

    m_chests.erase(it);
}
