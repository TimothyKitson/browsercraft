#pragma once
#include "Anvil.h"
#include "Enchanting.h"
#include <cstdint>

namespace Grindstone
{
    struct Result
    {
        bool valid = false;
        StackId id = Blocks::Air;
        int count = 1;
        int damage = 0;
        Enchanting::Set enchantments = 0;
    };

    int minPower(Enchanting::Kind kind, int level);
    int powerOf(Enchanting::Set set);
    int experienceFor(int power, uint32_t roll);
    bool accepts(StackId id, Enchanting::Set enchantments);
    Result combine(const Anvil::Input& top, const Anvil::Input& bottom);
}
