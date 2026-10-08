#include "Inventory.h"
#include "Game/Crafting.h"
#include <algorithm>

int Inventory::add(StackId id, int count)
{
    if (id == Blocks::Air || count <= 0) return count;

    // Top up existing stacks first, then fall back to empty slots.
    for (int pass = 0; pass < 2 && count > 0; ++pass)
    {
        for (int i = 0; i < MAIN_SLOTS; ++i)
        {
            ItemStack& stack = m_slots[i];
            if (count <= 0) break;

            const bool matching = (!stack.empty() && stack.id == id && stack.count < maxStackOf(id));
            const bool emptySlot = stack.empty();

            if ((pass == 0 && matching) || (pass == 1 && emptySlot))
            {
                if (emptySlot)
                {
                    stack.id = id;
                    stack.count = 0;
                }
                const int space = maxStackOf(id) - stack.count;
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

int Inventory::countOf(StackId id) const
{
    int total = 0;
    for (int i = 0; i < MAIN_SLOTS; ++i)
        if (!m_slots[i].empty() && m_slots[i].id == id) total += m_slots[i].count;
    return total;
}

void Inventory::clear()
{
    for (ItemStack& stack : m_slots) stack.clear();
    m_cursor.clear();
}

// --------------------------------------------------------- crafting ---

void Inventory::refreshCraftResult()
{
    const CraftOutput output = Crafting::match(&m_slots[CRAFT_FIRST], m_craftSize);
    m_slots[CRAFT_RESULT].id = output.id;
    m_slots[CRAFT_RESULT].count = output.count;
}

void Inventory::clearCraftGrid(std::vector<ItemStack>& spilled)
{
    for (int i = 0; i < CRAFT_SLOTS; ++i)
    {
        ItemStack& cell = m_slots[CRAFT_FIRST + i];
        if (cell.empty()) continue;

        const int leftOver = add(cell.id, cell.count);
        if (leftOver > 0) spilled.push_back(ItemStack{ cell.id, leftOver });
        cell.clear();
    }
    m_slots[CRAFT_RESULT].clear();
}

// Taking the result is its own thing: the slot is not storage, it is a
// preview, so you can only remove from it and doing so spends ingredients.
bool Inventory::takeCraftResult()
{
    ItemStack& result = m_slots[CRAFT_RESULT];
    if (result.empty()) return false;

    if (m_cursor.empty())
    {
        m_cursor = result;
    }
    else if (m_cursor.id == result.id && m_cursor.count + result.count <= maxStackOf(result.id))
    {
        m_cursor.count += result.count;
    }
    else
    {
        return false;   // hands full of something else
    }

    Crafting::consume(&m_slots[CRAFT_FIRST], m_craftSize);
    refreshCraftResult();
    return true;
}

// ------------------------------------------------------------- clicks ---

void Inventory::mergeInto(ItemStack& from, ItemStack& into)
{
    const int space = maxStackOf(into.id) - into.count;
    const int moved = (from.count < space) ? from.count : space;
    into.count += moved;
    from.count -= moved;
    if (from.count <= 0) from.clear();
}

int Inventory::findDestination(const ItemStack& stack, int begin, int end) const
{
    // Topping up a stack you already have beats scattering across empties.
    for (int i = begin; i < end; ++i)
        if (!m_slots[i].empty() && m_slots[i].id == stack.id && m_slots[i].count < maxStackOf(stack.id))
            return i;
    for (int i = begin; i < end; ++i)
        if (m_slots[i].empty()) return i;
    return -1;
}

void Inventory::leftClick(int index)
{
    if (index < 0 || index >= SLOT_COUNT) return;
    if (index == CRAFT_RESULT) { takeCraftResult(); return; }

    ItemStack& target = m_slots[index];

    if (m_cursor.empty())
    {
        m_cursor = target;
        target.clear();
        return;
    }

    if (target.empty())
    {
        target = m_cursor;
        m_cursor.clear();
        return;
    }

    if (target.id == m_cursor.id)
    {
        mergeInto(m_cursor, target);  // leftovers stay on the cursor
        return;
    }

    std::swap(target, m_cursor);
}

void Inventory::rightClick(int index)
{
    if (index < 0 || index >= SLOT_COUNT) return;
    if (index == CRAFT_RESULT) { takeCraftResult(); return; }

    ItemStack& target = m_slots[index];

    if (m_cursor.empty())
    {
        if (target.empty()) return;
        // The larger half comes with you, as in Minecraft.
        const int taken = (target.count + 1) / 2;
        m_cursor.id = target.id;
        m_cursor.count = taken;
        target.count -= taken;
        if (target.count <= 0) target.clear();
        return;
    }

    if (target.empty())
    {
        target.id = m_cursor.id;
        target.count = 1;
        m_cursor.count -= 1;
        if (m_cursor.count <= 0) m_cursor.clear();
        return;
    }

    if (target.id == m_cursor.id && target.count < maxStackOf(target.id))
    {
        target.count += 1;
        m_cursor.count -= 1;
        if (m_cursor.count <= 0) m_cursor.clear();
    }
}

void Inventory::quickMove(int index)
{
    if (index < 0 || index >= SLOT_COUNT) return;

    // Shift-clicking the result slot crafts as many times as the
    // ingredients and the free space allow -- the usual way to turn a
    // stack of logs into a stack of planks.
    if (index == CRAFT_RESULT)
    {
        while (!m_slots[CRAFT_RESULT].empty())
        {
            const ItemStack result = m_slots[CRAFT_RESULT];
            if (findDestination(result, 0, MAIN_SLOTS) < 0) break;
            if (add(result.id, result.count) > 0) break;
            Crafting::consume(&m_slots[CRAFT_FIRST], m_craftSize);
            refreshCraftResult();
        }
        return;
    }

    ItemStack& source = m_slots[index];
    if (source.empty()) return;

    // Hotbar sends to storage and storage sends to the hotbar; armour and
    // the crafting grid always send back to the main inventory.
    const bool fromHotbar = index < HOTBAR_SLOTS;
    const bool fromMain = index < MAIN_SLOTS;
    const int begin = !fromMain ? 0 : (fromHotbar ? HOTBAR_SLOTS : 0);
    const int end = !fromMain ? MAIN_SLOTS : (fromHotbar ? MAIN_SLOTS : HOTBAR_SLOTS);

    while (!source.empty())
    {
        const int destination = findDestination(source, begin, end);
        if (destination < 0) break;   // the other half is full

        ItemStack& into = m_slots[destination];
        if (into.empty())
        {
            into.id = source.id;
            into.count = 0;
        }
        mergeInto(source, into);
    }
}

void Inventory::takeFromPalette(BlockId id)
{
    // Holding something and clicking the palette throws it away: the
    // palette is where creative items come from, so it is also the bin.
    if (!m_cursor.empty())
    {
        m_cursor.clear();
        return;
    }

    if (id == Blocks::Air) return;
    m_cursor.id = id;
    m_cursor.count = maxStackOf(m_cursor.id);
}
