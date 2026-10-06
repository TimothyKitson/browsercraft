#include "Inventory.h"
#include <algorithm>

int Inventory::add(BlockId id, int count)
{
    if (id == Blocks::Air || count <= 0) return count;

    // Top up existing stacks first, then fall back to empty slots.
    for (int pass = 0; pass < 2 && count > 0; ++pass)
    {
        for (ItemStack& stack : m_slots)
        {
            if (count <= 0) break;

            const bool matching = (!stack.empty() && stack.id == id && stack.count < MAX_STACK);
            const bool emptySlot = stack.empty();

            if ((pass == 0 && matching) || (pass == 1 && emptySlot))
            {
                if (emptySlot)
                {
                    stack.id = id;
                    stack.count = 0;
                }
                const int space = MAX_STACK - stack.count;
                const int moved = std::min(space, count);
                stack.count += moved;
                count -= moved;
            }
        }
    }

    return count;
}

bool Inventory::removeOne(int slot)
{
    if (slot < 0 || slot >= TOTAL_SLOTS) return false;
    ItemStack& stack = m_slots[slot];
    if (stack.empty()) return false;

    stack.count -= 1;
    if (stack.count <= 0) stack.clear();
    return true;
}

void Inventory::setSelectedSlot(int index)
{
    m_selectedSlot = std::clamp(index, 0, HOTBAR_SLOTS - 1);
}

void Inventory::scrollSelection(int delta)
{
    if (delta == 0) return;
    // Scrolling up (positive) moves left along the hotbar, like Minecraft.
    int next = (m_selectedSlot - delta) % HOTBAR_SLOTS;
    if (next < 0) next += HOTBAR_SLOTS;
    m_selectedSlot = next;
}

int Inventory::countOf(BlockId id) const
{
    int total = 0;
    for (const ItemStack& stack : m_slots)
        if (!stack.empty() && stack.id == id) total += stack.count;
    return total;
}

void Inventory::clear()
{
    for (ItemStack& stack : m_slots) stack.clear();
}
