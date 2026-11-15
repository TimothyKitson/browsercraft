#include "Anvil.h"
#include "Armour.h"
#include "Shield.h"
#include "Tools.h"
#include <algorithm>

StackId Anvil::repairMaterial(StackId item)
{
    if (item == Items::Shield) return Blocks::Planks;
    if (item == Items::Mace) return Items::BreezeRod;
    if (item == Items::WolfArmor) return Items::ArmadilloScute;
    if (item == Items::TurtleHelmet) return Items::TurtleScute;
    if (item == Items::Elytra) return Items::PhantomMembrane;

    switch (Tools::tierOf(item))
    {
        case Tools::Tier::Wood:    return Blocks::Planks;
        case Tools::Tier::Stone:   return Blocks::Cobblestone;
        case Tools::Tier::Iron:    return Items::IronIngot;
        case Tools::Tier::Gold:    return Items::GoldIngot;
        case Tools::Tier::Diamond: return Items::Diamond;
        case Tools::Tier::Netherite: return Items::NetheriteIngot;
        case Tools::Tier::None:    break;
    }

    switch (Armour::tierOf(item))
    {
        case Armour::Tier::Leather: return Items::Leather;
        case Armour::Tier::Iron:    return Items::IronIngot;
        case Armour::Tier::Chainmail: return Items::IronIngot;
        case Armour::Tier::Gold:    return Items::GoldIngot;
        case Armour::Tier::Diamond: return Items::Diamond;
        case Armour::Tier::Netherite: return Items::NetheriteIngot;
        case Armour::Tier::None:    break;
    }

    return Blocks::Air;
}

bool Anvil::repairsWith(StackId item, StackId material)
{
    const StackId wanted = repairMaterial(item);
    if (wanted == Blocks::Air || material == Blocks::Air) return false;
    if (wanted == static_cast<StackId>(Blocks::Planks)) return !isItem(material) && isPlanks(asBlock(material));
    if (wanted == static_cast<StackId>(Blocks::Cobblestone))
        return material == static_cast<StackId>(Blocks::Cobblestone) || material == static_cast<StackId>(Blocks::Blackstone) ||
               material == static_cast<StackId>(Blocks::CobbledDeepslate);
    return material == wanted;
}

int Anvil::maxDurability(StackId item)
{
    return itemDurability(item);
}

Anvil::Result Anvil::combine(const Input& left, const Input& right)
{
    Result out;
    if (left.id == Blocks::Air || left.count <= 0) return out;
    if (right.id == Blocks::Air || right.count <= 0) return out;

    const bool leftBook = isEnchantedBook(left) || left.id == Items::EnchantedBook;
    const bool book = isEnchantedBook(right);
    const int max = maxDurability(left.id);
    const long base = static_cast<long>(Enchanting::priorWork(left.enchantments)) + Enchanting::priorWork(right.enchantments);

    out.id = left.id;
    out.damage = left.damage;
    out.enchantments = left.enchantments;
    int cost = 0;

    if (max > 0 && repairsWith(left.id, right.id))
    {
        int step = std::min(out.damage, max / 4);
        if (step <= 0) return out;
        int units = 0;
        for (; step > 0 && units < right.count; ++units)
        {
            out.damage -= step;
            ++cost;
            step = std::min(out.damage, max / 4);
        }
        out.used = units;
    }
    else
    {
        if (!book && (right.id != left.id || (max <= 0 && !leftBook))) return out;
        out.used = 1;
        if (max > 0 && !book)
        {
            const int kept = (max - left.damage) + (max - right.damage) + max * 12 / 100;
            const int worn = std::max(0, max - kept);
            if (worn < out.damage)
            {
                out.damage = worn;
                cost += 2;
            }
        }
        bool applied = false, refused = false;
        for (const auto& [kind, level] : Enchanting::listOf(right.enchantments))
        {
            const int had = Enchanting::levelOf(out.enchantments, kind);
            int merged = had == level ? level + 1 : std::max(level, had);
            bool fits = leftBook || Enchanting::appliesTo(kind, left.id);
            for (const auto& [other, otherLevel] : Enchanting::listOf(out.enchantments))
                if (other != kind && Enchanting::conflicts(kind, other))
                {
                    fits = false;
                    ++cost;
                }
            if (!fits)
            {
                refused = true;
                continue;
            }
            applied = true;
            merged = std::min(merged, Enchanting::maxLevel(kind));
            out.enchantments = Enchanting::withLevel(out.enchantments, kind, merged);
            const int each = book ? std::max(1, Enchanting::anvilCost(kind) / 2) : Enchanting::anvilCost(kind);
            cost += each * merged;
            if (left.count > 1) cost = TOO_EXPENSIVE;
        }
        if (refused && !applied) return out;
    }

    if (cost <= 0) return out;
    out.levelCost = static_cast<int>(std::min<long>(base + cost, 1000000L));
    if (out.levelCost >= TOO_EXPENSIVE)
    {
        out.tooExpensive = true;
        return out;
    }
    if (left.id != Items::Trident)
    {
        const int work = std::max(Enchanting::priorWork(left.enchantments), Enchanting::priorWork(right.enchantments));
        out.enchantments = Enchanting::withPriorWork(out.enchantments, std::min(255, work * 2 + 1));
    }
    out.valid = true;
    return out;
}
