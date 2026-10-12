#include "Anvil.h"
#include "Armour.h"
#include "Tools.h"
#include <algorithm>

namespace
{
    // What a book's enchantment costs to move, per level, against what
    // the same one costs when it comes off another item. Books are the
    // cheap path in Minecraft, and that is most of why they are worth
    // keeping rather than using the table on the tool directly.
    constexpr int COST_PER_LEVEL_FROM_BOOK = 1;
    constexpr int COST_PER_LEVEL_FROM_ITEM = 2;

    // What joining two worn items costs on top of their enchantments.
    constexpr int COST_OF_JOINING = 2;

    int repairStep(int max)
    {
        // Rounded up, so four units always cover a whole item. Rounding
        // down left a sliver behind -- an iron pickaxe worn to its last
        // point wanted a fifth ingot to close the final two.
        const int step = (max * Anvil::REPAIR_PERCENT + 99) / 100;
        return std::max(1, step);   // or a one-use item could never be mended
    }

    // Folds one kind of the right-hand item onto the result, and says
    // what it added to the bill. Returns false when it does not belong
    // on this item at all, which costs nothing and changes nothing.
    bool foldIn(Enchanting::Set& into, StackId target,
                Enchanting::Kind kind, int rightLevel, int perLevel, int& cost)
    {
        if (rightLevel <= 0) return false;
        if (!Enchanting::appliesTo(kind, target)) return false;

        // Fortune and silk touch undo each other. Whichever is already
        // there keeps the slot.
        for (int k = 0; k < static_cast<int>(Enchanting::Kind::Count); ++k)
        {
            const Enchanting::Kind other = static_cast<Enchanting::Kind>(k);
            if (other == kind) continue;
            if (!Enchanting::has(into, other)) continue;
            if (Enchanting::conflicts(kind, other)) return false;
        }

        const int leftLevel = Enchanting::levelOf(into, kind);

        // Two of the same level make the next one up; otherwise the
        // better of the two wins and the worse is simply absorbed.
        const int merged = (leftLevel == rightLevel)
                               ? std::min(leftLevel + 1, Enchanting::maxLevel(kind))
                               : std::max(leftLevel, rightLevel);

        if (merged == leftLevel && leftLevel > 0)
        {
            // Nothing gained -- the right-hand one was weaker, or both
            // were already at the ceiling. Still consumed, still free.
            return false;
        }

        into = Enchanting::withLevel(into, kind, merged);
        cost += merged * perLevel;
        return true;
    }
}

StackId Anvil::repairMaterial(StackId item)
{
    switch (Tools::tierOf(item))
    {
        case Tools::Tier::Wood:    return Blocks::Planks;
        case Tools::Tier::Stone:   return Blocks::Cobblestone;
        case Tools::Tier::Iron:    return Items::IronIngot;
        case Tools::Tier::Gold:    return Items::GoldIngot;
        case Tools::Tier::Diamond: return Items::Diamond;
        case Tools::Tier::None:    break;
    }

    switch (Armour::tierOf(item))
    {
        case Armour::Tier::Leather: return Items::Leather;
        case Armour::Tier::Iron:    return Items::IronIngot;
        case Armour::Tier::Gold:    return Items::GoldIngot;
        case Armour::Tier::Diamond: return Items::Diamond;
        case Armour::Tier::None:    break;
    }

    return Blocks::Air;
}

int Anvil::maxDurability(StackId item)
{
    if (Tools::isTool(item)) return Tools::maxDurability(Tools::tierOf(item));
    if (Armour::tierOf(item) != Armour::Tier::None) return Armour::maxDurability(item);
    return 0;
}

Anvil::Result Anvil::combine(const Input& left, const Input& right)
{
    Result out;
    if (left.id == Blocks::Air || left.count <= 0) return out;
    if (right.id == Blocks::Air || right.count <= 0) return out;

    const int max = maxDurability(left.id);
    if (max <= 0) return out;   // nothing the anvil knows how to work on

    out.id = left.id;
    out.damage = left.damage;
    out.enchantments = left.enchantments;

    int cost = 0;
    bool changed = false;

    if (isEnchantedBook(right))
    {
        // A book gives up everything on it that the item can take.
        out.used = 1;
        for (const auto& [kind, level] : Enchanting::listOf(right.enchantments))
            changed |= foldIn(out.enchantments, out.id, kind, level,
                              COST_PER_LEVEL_FROM_BOOK, cost);
    }
    else if (right.id == left.id)
    {
        // Two of the same thing. What is left of both, plus a little
        // for the trouble, and the enchantments of each.
        out.used = 1;

        const int leftLeft = max - left.damage;
        const int rightLeft = max - right.damage;
        const int bonus = max * 12 / 100;
        const int mended = std::min(max, leftLeft + rightLeft + bonus);
        const int newDamage = std::max(0, max - mended);

        if (newDamage < out.damage)
        {
            out.damage = newDamage;
            cost += COST_OF_JOINING;
            changed = true;
        }

        for (const auto& [kind, level] : Enchanting::listOf(right.enchantments))
            changed |= foldIn(out.enchantments, out.id, kind, level,
                              COST_PER_LEVEL_FROM_ITEM, cost);
    }
    else if (right.id == repairMaterial(left.id))
    {
        // Raw material. Takes only as many as the damage needs, so
        // putting a stack of sixty-four against a nearly-whole pickaxe
        // spends one ingot rather than the lot.
        if (left.damage <= 0) return out;   // nothing to mend

        const int step = repairStep(max);
        const int wanted = (left.damage + step - 1) / step;
        const int units = std::min(wanted, right.count);

        out.used = units;
        out.damage = std::max(0, left.damage - units * step);
        cost += units;
        changed = true;
    }
    else
    {
        return out;   // these two do not go together
    }

    if (!changed) return out;   // nothing gained: the anvil offers nothing

    out.levelCost = cost;
    if (cost >= TOO_EXPENSIVE)
    {
        out.tooExpensive = true;
        return out;
    }

    out.valid = true;
    return out;
}
