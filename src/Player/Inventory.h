#pragma once
#include "World/Block.h"
#include "Game/Items.h"
#include <array>
#include <vector>

struct ItemStack
{
    StackId id = Blocks::Air;
    int count = 0;

    // How worn a tool is, 0 when new. Meaningless on everything else,
    // which is why it is never compared when stacks are merged -- only
    // tools carry it, and a tool never stacks.
    int damage = 0;

    bool empty() const { return count <= 0 || id == Blocks::Air; }
    void clear() { id = Blocks::Air; count = 0; damage = 0; }
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

    // The open furnace's three slots, borrowed the same way the craft
    // grid is: what you click on is an inventory slot, so every click
    // rule below applies to a furnace without knowing one exists. The
    // contents are copied in when it is opened and back out when it is
    // shut, and live with the furnace in between.
    static constexpr int FURNACE_FIRST = CRAFT_RESULT + 1;            // 50..52
    static constexpr int FURNACE_INPUT = FURNACE_FIRST;
    static constexpr int FURNACE_FUEL = FURNACE_FIRST + 1;
    static constexpr int FURNACE_OUTPUT = FURNACE_FIRST + 2;
    static constexpr int FURNACE_SLOTS = 3;

    // The open chest's twenty-seven, borrowed the same way. A chest is
    // the one container big enough that the screen gives it the whole
    // top of the panel rather than a corner of it.
    static constexpr int CHEST_FIRST = FURNACE_FIRST + FURNACE_SLOTS;  // 53..79
    static constexpr int CHEST_SLOTS = 27;

    static constexpr int SLOT_COUNT = CHEST_FIRST + CHEST_SLOTS;      // 80

    // The ceiling. What a particular thing actually stacks to comes from
    // maxStackOf(), since items need not agree with blocks.
    static constexpr int MAX_STACK = 64;

    // Whether the player is carrying any of something, and taking one
    // of it. Used by the bow, which spends arrows from wherever in the
    // pack they happen to be rather than from the hand.
    bool has(StackId id) const;
    bool take(StackId id);

    static bool isArmourSlot(int index)
    {
        return index >= ARMOR_FIRST && index < ARMOR_FIRST + ARMOR_SLOTS;
    }

    // Puts a piece of armour on, swapping whatever that slot already
    // holds onto the cursor. Returns false if it is not armour, or if
    // the slot it belongs in is somehow out of range.
    bool wear(ItemStack& from);

    // Returns the number of items that didn't fit.
    int add(StackId id, int count);
    bool removeOne(int slot);

    ItemStack& slot(int index) { return m_slots[index]; }
    const ItemStack& slot(int index) const { return m_slots[index]; }

    ItemStack& selected() { return m_slots[m_selectedSlot]; }
    const ItemStack& selected() const { return m_slots[m_selectedSlot]; }

    int selectedSlot() const { return m_selectedSlot; }
    void setSelectedSlot(int index);
    void scrollSelection(int delta);

    int countOf(StackId id) const;
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
