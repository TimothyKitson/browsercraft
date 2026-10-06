#pragma once
#include "World/Block.h"
#include <array>

struct ItemStack
{
    BlockId id = Blocks::Air;
    int count = 0;

    bool empty() const { return count <= 0 || id == Blocks::Air; }
    void clear() { id = Blocks::Air; count = 0; }
};

// 9 hotbar slots plus 27 storage slots, same shape as Minecraft's.
class Inventory
{
public:
    static constexpr int HOTBAR_SLOTS = 9;
    static constexpr int STORAGE_SLOTS = 27;
    static constexpr int TOTAL_SLOTS = HOTBAR_SLOTS + STORAGE_SLOTS;
    static constexpr int MAX_STACK = 64;

    // Returns the number of items that didn't fit.
    int add(BlockId id, int count);
    bool removeOne(int slot);

    ItemStack& slot(int index) { return m_slots[index]; }
    const ItemStack& slot(int index) const { return m_slots[index]; }

    ItemStack& selected() { return m_slots[m_selectedSlot]; }
    const ItemStack& selected() const { return m_slots[m_selectedSlot]; }

    int selectedSlot() const { return m_selectedSlot; }
    void setSelectedSlot(int index);
    void scrollSelection(int delta);

    int countOf(BlockId id) const;
    void clear();

private:
    std::array<ItemStack, TOTAL_SLOTS> m_slots{};
    int m_selectedSlot = 0;
};
