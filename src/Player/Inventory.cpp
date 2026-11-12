#include "Game/Anvil.h"
#include "Inventory.h"
#include "Game/Enchanting.h"
#include "Game/Armour.h"
#include "Game/Crafting.h"
#include "Game/Items.h"
#include "Game/Banners.h"
#include "Game/Archaeology.h"
#include <algorithm>

int Inventory::addStack(const ItemStack& stack)
{
    if (stack.empty()) return 0;

    // Anything carrying wear or enchantments is a thing that does not
    // stack, so it wants a slot of its own rather than topping up one
    // that is already there: merging would quietly throw away whichever
    // of the two sets of enchantments lost.
    if (stack.damage != 0 || stack.enchantments != 0 || maxStackOf(stack.id) == 1)
    {
        int left = stack.count;
        if (maxStackOf(stack.id) > 1)
            for (int i = 0; i < MAIN_SLOTS && left > 0; ++i)
            {
                ItemStack& slot = m_slots[i];
                if (slot.empty() || !slot.sameAs(stack) || slot.count >= maxStackOf(stack.id)) continue;
                const int moved = std::min(left, maxStackOf(stack.id) - slot.count);
                slot.count += moved;
                left -= moved;
            }
        if (left <= 0) return 0;
        for (int i = 0; i < MAIN_SLOTS; ++i)
        {
            if (!m_slots[i].empty()) continue;
            m_slots[i] = stack;
            m_slots[i].count = left;
            return 0;
        }
        return left;
    }

    return add(stack.id, stack.count);
}

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

            const bool matching = (!stack.empty() && stack.id == id && stack.damage == 0 && stack.enchantments == 0 &&
                                   stack.count < maxStackOf(id));
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

    // Almost every recipe makes something brand new; the netherite
    // upgrade hands back the tool you put in, wear and enchantments
    // and all, and dropping those here would make upgrading a way of
    // throwing enchantments away.
    m_slots[CRAFT_RESULT].damage = output.damage;
    m_slots[CRAFT_RESULT].enchantments = output.enchantments;
}

void Inventory::clearCraftGrid(std::vector<ItemStack>& spilled)
{
    for (int i = 0; i < CRAFT_SLOTS; ++i)
    {
        ItemStack& cell = m_slots[CRAFT_FIRST + i];
        if (cell.empty()) continue;

        // addStack rather than add, so a tool left in the grid comes
        // back worn and enchanted as it went in. Nothing that stacks
        // notices the difference.
        ItemStack leftOver = cell;
        leftOver.count = addStack(cell);
        if (leftOver.count > 0) spilled.push_back(leftOver);
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
    else if (m_cursor.sameAs(result) && m_cursor.count + result.count <= maxStackOf(result.id))
    {
        m_cursor.count += result.count;
    }
    else
    {
        return false;   // hands full of something else
    }

    m_crafted.emplace_back(result.id, result.count);
    noteSherdPot(result);
    const int bottles = Crafting::consume(&m_slots[CRAFT_FIRST], m_craftSize);
    if (bottles > 0) add(Items::GlassBottle, bottles);
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
        if (!m_slots[i].empty() && m_slots[i].sameAs(stack) && m_slots[i].count < maxStackOf(stack.id))
            return i;
    for (int i = begin; i < end; ++i)
        if (m_slots[i].empty()) return i;
    return -1;
}

bool Inventory::has(StackId id) const
{
    for (int i = 0; i < MAIN_SLOTS; ++i)
        if (m_slots[i].id == id && m_slots[i].count > 0) return true;
    return false;
}

bool Inventory::take(StackId id)
{
    // The hotbar first, so what you can see going is what goes.
    for (int i = 0; i < MAIN_SLOTS; ++i)
    {
        ItemStack& slot = m_slots[i];
        if (slot.id != id || slot.count <= 0) continue;

        slot.count -= 1;
        if (slot.count <= 0) slot.clear();
        return true;
    }
    return false;
}

// Right-clicking a piece in your hand puts it on, which is how
// Minecraft does it and saves dragging it across the panel.
bool Inventory::wear(ItemStack& from)
{
    if (from.empty()) return false;

    const int piece = Armour::slotFor(from.id);
    if (piece < 0 || piece >= ARMOR_SLOTS) return false;

    if (bound(ARMOR_FIRST + piece)) return false;
    ItemStack& worn = m_slots[ARMOR_FIRST + piece];

    // What was there comes off in exchange, so swapping one helmet for
    // another never destroys the old one.
    std::swap(worn, from);
    return true;
}

bool Inventory::bound(int index) const
{
    if (!bindingHolds || index < ARMOR_FIRST || index >= ARMOR_FIRST + ARMOR_SLOTS) return false;
    const ItemStack& worn = m_slots[index];
    return !worn.empty() && Enchanting::has(worn.enchantments, Enchanting::Kind::BindingCurse);
}

void Inventory::leftClick(int index)
{
    if (index < 0 || index >= SLOT_COUNT) return;
    if (bound(index)) return;
    if (index == CRAFT_RESULT) { takeCraftResult(); return; }

    ItemStack& target = m_slots[index];

    // What a furnace has made is yours to take and not a place to put
    // things: dropping ore into the output would be a way of smelting
    // nothing at all.
    if (index == FURNACE_OUTPUT && !m_cursor.empty())
    {
        if (!target.empty() && target.sameAs(m_cursor)) mergeInto(target, m_cursor);
        return;
    }

    // Boots go on feet, lapis goes in the lapis slot: a slot made for one
    // thing takes that thing and nothing else, so none of them can be
    // used as an extra pocket.
    if (!m_cursor.empty() && !belongsIn(index, m_cursor.id))
        return;

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

    if (target.sameAs(m_cursor))
    {
        mergeInto(m_cursor, target);  // leftovers stay on the cursor
        return;
    }

    std::swap(target, m_cursor);
}

void Inventory::rightClick(int index)
{
    if (index < 0 || index >= SLOT_COUNT) return;
    if (bound(index)) return;
    if (index == CRAFT_RESULT) { takeCraftResult(); return; }
    if (index == FURNACE_OUTPUT && !m_cursor.empty()) return;
    if (!m_cursor.empty() && !belongsIn(index, m_cursor.id))
        return;

    ItemStack& target = m_slots[index];

    if (m_cursor.empty())
    {
        if (target.empty()) return;
        // The larger half comes with you, as in Minecraft.
        const int taken = (target.count + 1) / 2;
        m_cursor = target;
        m_cursor.count = taken;
        target.count -= taken;
        if (target.count <= 0) target.clear();
        return;
    }

    if (target.empty())
    {
        target = m_cursor;
        target.count = 1;
        m_cursor.count -= 1;
        if (m_cursor.count <= 0) m_cursor.clear();
        return;
    }

    if (target.sameAs(m_cursor) && target.count < maxStackOf(target.id))
    {
        target.count += 1;
        m_cursor.count -= 1;
        if (m_cursor.count <= 0) m_cursor.clear();
    }
}

bool Inventory::canSpreadInto(int index) const
{
    if (index < 0 || index >= SLOT_COUNT || m_cursor.empty()) return false;
    if (isOutputSlot(index) || bound(index) || !belongsIn(index, m_cursor.id)) return false;
    const ItemStack& target = m_slots[index];
    return target.empty() || (target.sameAs(m_cursor) && target.count < maxStackOf(target.id));
}

bool Inventory::spread(const std::vector<int>& slots, bool one)
{
    std::vector<int> into;
    for (int index : slots)
        if (canSpreadInto(index) && std::find(into.begin(), into.end(), index) == into.end()) into.push_back(index);
    if (into.empty()) return false;
    while (static_cast<int>(into.size()) > m_cursor.count) into.pop_back();
    const int each = one ? 1 : m_cursor.count / static_cast<int>(into.size());
    if (each <= 0) return false;
    const ItemStack carried = m_cursor;
    int left = m_cursor.count;
    for (int index : into)
    {
        ItemStack& target = m_slots[index];
        const int had = target.empty() ? 0 : target.count;
        const int put = std::min(each, maxStackOf(carried.id) - had);
        if (put <= 0) continue;
        if (target.empty())
        {
            target = carried;
            target.count = 0;
        }
        target.count += put;
        left -= put;
    }
    m_cursor.count = left;
    if (m_cursor.count <= 0) m_cursor.clear();
    return true;
}

int Inventory::gather(const std::vector<int>& slots)
{
    if (m_cursor.empty()) return 0;
    const int most = maxStackOf(m_cursor.id);
    int taken = 0;
    for (int pass = 0; pass < 2 && m_cursor.count < most; ++pass)
        for (int index : slots)
        {
            if (m_cursor.count >= most) break;
            if (index < 0 || index >= SLOT_COUNT || isOutputSlot(index) || bound(index)) continue;
            ItemStack& from = m_slots[index];
            if (from.empty() || !from.sameAs(m_cursor)) continue;
            if (pass == 0 && from.count >= maxStackOf(from.id)) continue;
            const int moved = std::min(from.count, most - m_cursor.count);
            m_cursor.count += moved;
            from.count -= moved;
            taken += moved;
            if (from.count <= 0) from.clear();
        }
    return taken;
}

bool Inventory::swapSlots(int index, int other)
{
    if (index < 0 || index >= SLOT_COUNT || other < 0 || other >= SLOT_COUNT || index == other) return false;
    if (bound(index) || bound(other) || isOutputSlot(other)) return false;
    ItemStack& a = m_slots[index];
    ItemStack& b = m_slots[other];
    if (a.empty() && b.empty()) return false;
    if (isOutputSlot(index))
    {
        if (a.empty() || index == CRAFT_RESULT) return false;
        if (b.empty())
        {
            b = a;
            a.clear();
            return true;
        }
        if (!b.sameAs(a) || b.count >= maxStackOf(b.id)) return false;
        mergeInto(a, b);
        return true;
    }
    if (!a.empty() && !belongsIn(other, a.id)) return false;
    if (!b.empty() && !belongsIn(index, b.id)) return false;
    std::swap(a, b);
    return true;
}

ItemStack Inventory::throwFrom(int index, bool whole)
{
    ItemStack out;
    if (index < 0 || index >= SLOT_COUNT || bound(index) || index == CRAFT_RESULT) return out;
    if (!m_cursor.empty()) return out;
    ItemStack& from = m_slots[index];
    if (from.empty()) return out;
    out = from;
    out.count = whole ? from.count : 1;
    from.count -= out.count;
    if (from.count <= 0) from.clear();
    return out;
}

void Inventory::quickMove(int index)
{
    if (index < 0 || index >= SLOT_COUNT) return;
    if (bound(index)) return;

    // Shift-clicking the result slot crafts as many times as the
    // ingredients and the free space allow -- the usual way to turn a
    // stack of logs into a stack of planks.
    if (index == CRAFT_RESULT)
    {
        while (!m_slots[CRAFT_RESULT].empty())
        {
            const ItemStack result = m_slots[CRAFT_RESULT];
            if (findDestination(result, 0, MAIN_SLOTS) < 0) break;
            if (addStack(result) > 0) break;
            m_crafted.emplace_back(result.id, result.count);
            noteSherdPot(result);
            const int bottles = Crafting::consume(&m_slots[CRAFT_FIRST], m_craftSize);
            if (bottles > 0) add(Items::GlassBottle, bottles);
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
            // The whole stack, not just its id: mergeInto below moves
            // the count and nothing else, so setting only the id left
            // the wear and the enchantments behind. Shift-clicking a
            // worn tool out of a chest repaired it, and shift-clicking
            // an enchanted one stripped it.
            into = source;
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

bool Inventory::belongsIn(int index, StackId id)
{
    if (index == ENCHANT_LAPIS) return id == Items::LapisLazuli;
    if (index == BREW_FUEL) return id == Items::BlazePowder;
    if (index == TRADE_RESULT) return false;
    if (index == HORSE_SADDLE) return id == Items::Saddle;
    if (index == HORSE_ARMOUR) return isHorseArmour(id);
    if (index == GRIND_TOP || index == GRIND_BOTTOM) return Anvil::wearsOut(id) || id == Items::Book;
    if (index == GRIND_RESULT) return false;
    if (index == CUT_INPUT) return !isItem(id);
    if (index == CARTO_MAP) return id == Items::FilledMap;
    if (index == CARTO_EXTRA) return id == Items::Paper || id == Items::EmptyMap || id == Blocks::GlassPane;
    if (index == LOOM_BANNER) return !isItem(id) && isBanner(asBlock(id));
    if (index == LOOM_DYE) return isDye(id);
    if (index == LOOM_PATTERN) return Banners::isPatternItem(id);
    if (index == BEACON_PAYMENT)
        return id == Items::IronIngot || id == Items::GoldIngot || id == Items::Emerald ||
               id == Items::Diamond || id == Items::NetheriteIngot;
    if (!isArmourSlot(index)) return true;
    return Armour::slotFor(id) == index - ARMOR_FIRST;
}

std::vector<ItemStack> Inventory::evictMisfitArmour()
{
    std::vector<ItemStack> overflow;
    for (int i = ARMOR_FIRST; i < ARMOR_FIRST + ARMOR_SLOTS; ++i)
    {
        ItemStack& worn = m_slots[i];
        if (worn.empty() || belongsIn(i, worn.id)) continue;

        ItemStack moving = worn;
        worn.clear();
        moving.count = addStack(moving);
        if (moving.count > 0) overflow.push_back(moving);
    }
    return overflow;
}

void Inventory::noteSherdPot(const ItemStack& result)
{
    if (result.id < Blocks::DecoratedPotFirst || result.id > Blocks::DecoratedPotLast) return;
    for (int side = 0; side < 4; ++side)
        if (Archaeology::potSide(static_cast<uint32_t>(result.enchantments), side) < 0) return;
    ++m_sherdPots;
}
