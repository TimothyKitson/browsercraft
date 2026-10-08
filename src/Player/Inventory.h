#pragma once
#include "World/Block.h"
#include <array>
#include <vector>

struct ItemStack
{
    BlockId id = Blocks::Air;
    int count = 0;

    bool empty() const { return count <= 0 || id == Blocks::Air; }
    void clear() { id = Blocks::Air; count = 0; }
};

// 9 hotbar slots plus 27 storage slots, same shape as Minecraft's, and the
// same slot numbering: 0-8 are the hotbar, 9-35 the three storage rows.
//
// The stack "on the mouse" lives here too rather than in the UI, because
// every click rule below is really a rule about where items are allowed to
// end up -- keeping it next to the slots is what makes those rules short.
class Inventory
{
public:
    static constexpr int HOTBAR_SLOTS = 9;
    static constexpr int STORAGE_SLOTS = 27;
    // Where picked-up items go, and the only range add() ever fills.
    static constexpr int MAIN_SLOTS = HOTBAR_SLOTS + STORAGE_SLOTS;   // 0..35

    static constexpr int ARMOR_FIRST = MAIN_SLOTS;                    // 36..39
    static constexpr int ARMOR_SLOTS = 4;

    // What gets written to level.dat. The crafting grid deliberately
    // stops here: Minecraft empties it when you close the screen, and
    // saving it would resurrect half-finished recipes on load.
    static constexpr int TOTAL_SLOTS = MAIN_SLOTS + ARMOR_SLOTS;      // 40

    static constexpr int CRAFT_FIRST = TOTAL_SLOTS;                   // 40..48
    static constexpr int CRAFT_SLOTS = 9;
    static constexpr int CRAFT_RESULT = CRAFT_FIRST + CRAFT_SLOTS;    // 49
    static constexpr int SLOT_COUNT = CRAFT_RESULT + 1;               // 50

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

    // --- crafting ---
    // The grid is addressed as 3x3 even while only the top-left 2x2 is
    // reachable, so opening a bigger grid later changes one number.
    int craftSize() const { return m_craftSize; }
    void setCraftSize(int size) { m_craftSize = size; }
    // Recomputes the result slot. Call after anything touches the grid.
    void refreshCraftResult();
    // Empties the grid back into the inventory; returns what did not fit.
    void clearCraftGrid(std::vector<ItemStack>& spilled);
    // Moves one batch of the result onto the cursor, spending ingredients.
    bool takeCraftResult();

    // --- the stack being carried by the cursor ---
    ItemStack& cursor() { return m_cursor; }
    const ItemStack& cursor() const { return m_cursor; }

    // --- click handling, Minecraft's rules ---
    //
    // Left: pick the whole stack up, put the whole stack down, merge into
    // a matching stack, or swap. Right: pick up half, or put one down.
    void leftClick(int index);
    void rightClick(int index);
    // Shift-click teleports a stack between the hotbar and the storage
    // rows, which is how anyone actually moves things around.
    void quickMove(int index);

    // Creative only: the palette is an endless supply, so taking from it
    // just fills the cursor, and dropping onto it throws things away.
    void takeFromPalette(BlockId id);

private:
    std::array<ItemStack, SLOT_COUNT> m_slots{};
    int m_craftSize = 2;
    ItemStack m_cursor;
    int m_selectedSlot = 0;

    // Moves as much of `from` into `into` as the stack limit allows.
    static void mergeInto(ItemStack& from, ItemStack& into);
    // First slot in [begin, end) that `stack` could move into, preferring
    // a partial stack of the same block over an empty slot.
    int findDestination(const ItemStack& stack, int begin, int end) const;
};
