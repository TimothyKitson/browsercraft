#include "Grindstone.h"
#include <algorithm>

int Grindstone::minPower(Enchanting::Kind kind, int level)
{
    const int extra = std::max(0, level - 1);
    switch (kind)
    {
        case Enchanting::Kind::Efficiency: return 1 + 10 * extra;
        case Enchanting::Kind::Unbreaking: return 5 + 8 * extra;
        case Enchanting::Kind::Fortune:    return 15 + 9 * extra;
        case Enchanting::Kind::SilkTouch:  return 15;
        case Enchanting::Kind::Sharpness:  return 1 + 11 * extra;
        case Enchanting::Kind::Protection: return 1 + 11 * extra;
        case Enchanting::Kind::Power:      return 1 + 10 * extra;
        default:                           return 0;
    }
}

int Grindstone::powerOf(Enchanting::Set set)
{
    int total = 0;
    for (const auto& [kind, level] : Enchanting::listOf(set)) total += minPower(kind, level);
    return total;
}

int Grindstone::experienceFor(int power, uint32_t roll)
{
    if (power <= 0) return 0;
    const int half = (power + 1) / 2;
    return half + static_cast<int>(roll % static_cast<uint32_t>(half));
}

bool Grindstone::accepts(StackId id, Enchanting::Set enchantments)
{
    return Anvil::wearsOut(id) || Enchanting::any(enchantments);
}

Grindstone::Result Grindstone::combine(const Anvil::Input& top, const Anvil::Input& bottom)
{
    Result out;
    const bool hasTop = top.count > 0 && top.id != Blocks::Air;
    const bool hasBottom = bottom.count > 0 && bottom.id != Blocks::Air;
    if (!hasTop && !hasBottom) return out;

    if (hasTop != hasBottom)
    {
        const Anvil::Input& only = hasTop ? top : bottom;
        if (!Enchanting::any(only.enchantments)) return out;
        out.valid = true;
        out.id = only.id == Items::EnchantedBook ? static_cast<StackId>(Items::Book) : only.id;
        out.count = only.count;
        out.damage = only.damage;
        return out;
    }

    if (top.id != bottom.id) return out;
    const int most = Anvil::maxDurability(top.id);
    if (most <= 0) return out;

    const int left = most - top.damage;
    const int right = most - bottom.damage;
    const int healed = left + right + most * 5 / 100;
    out.valid = true;
    out.id = top.id;
    out.count = 1;
    out.damage = std::max(most - healed, 0);
    return out;
}
