#include "Trial.h"
#include "World/Noise.h"
#include <algorithm>

namespace
{
    // What a chamber throws at you. The breeze belongs here and
    // nowhere else: in Minecraft a trial spawner is the only thing
    // that makes one.
    const MobId WAVE[] = { MobId::Zombie, MobId::Skeleton, MobId::Spider,
                           MobId::Breeze, MobId::Creeper };

    // What a vault is worth. Weighted by how far down the list it is,
    // so iron is common and a diamond is a find.
    struct Prize { StackId id; int low, high; float weight; };

    // What only an ominous vault has in it. A plain vault pays for the
    // trouble; this pays for the trouble you went looking for.
    const StackId OMINOUS_EXTRAS[] = { Items::Mace, Items::BreezeRod, Items::Diamond,
                                       Items::EnderPearl };

    const Prize PRIZES[] = {
        { Items::IronIngot,    2, 5, 1.00f },
        { Items::Emerald,      1, 3, 0.70f },
        { Items::GoldIngot,    1, 4, 0.60f },
        { Items::Diamond,      1, 2, 0.34f },
        { Items::EnderPearl,   1, 2, 0.30f },
        { Items::Book,         1, 1, 0.28f },
        { Items::BlazePowder,  1, 3, 0.26f },
        { Items::Redstone,     4, 9, 0.50f },

        // The bottle, which is what makes the next trip round the
        // chamber the hard one. Rare enough to be a decision.
        { Items::OminousBottle, 1, 1, 0.22f },
    };
}

MobId Trial::speciesFor(int x, int y, int z, uint32_t seed, int index)
{
    constexpr int COUNT = static_cast<int>(sizeof(WAVE) / sizeof(WAVE[0]));
    const uint32_t roll = hashCoords(x + index * 31, y, z, seed + 88001);
    return WAVE[static_cast<int>(hashToFloat(roll) * COUNT) % COUNT];
}

StackId Trial::keyFor(bool ominous)
{
    return ominous ? Items::OminousTrialKey : Items::TrialKey;
}

// The plain vault's roll, with more of it and one thing out of the
// short list that is only ever found this way. A bottle, too, so the
// loop closes: a plain vault pays the omen that makes the next chamber
// ominous, and an ominous one pays the things worth having.
std::vector<Trial::Stack> Trial::ominousVaultLoot(int x, int y, int z, uint32_t seed)
{
    std::vector<Stack> out = vaultLoot(x, y, z, seed + 4001);
    for (Stack& stack : out) stack.count *= 2;

    constexpr int COUNT = static_cast<int>(sizeof(OMINOUS_EXTRAS) / sizeof(OMINOUS_EXTRAS[0]));
    const float roll = hashToFloat(hashCoords(x, y, z, seed + 88010));
    out.push_back(Stack{ OMINOUS_EXTRAS[static_cast<int>(roll * COUNT) % COUNT], 1 });

    return out;
}

std::vector<Trial::Stack> Trial::vaultLoot(int x, int y, int z, uint32_t seed)
{
    std::vector<Stack> out;

    constexpr int COUNT = static_cast<int>(sizeof(PRIZES) / sizeof(PRIZES[0]));
    const int wanted = MIN_STACKS +
        static_cast<int>(hashToFloat(hashCoords(x, y, z, seed + 88002)) *
                         (MAX_STACKS - MIN_STACKS + 1));

    for (int slot = 0; slot < COUNT && static_cast<int>(out.size()) < wanted; ++slot)
    {
        const Prize& prize = PRIZES[slot];
        const float roll = hashToFloat(hashCoords(x + slot, y, z, seed + 88003 + slot));
        if (roll >= prize.weight) continue;

        const float amount = hashToFloat(hashCoords(x, y + slot, z, seed + 88004 + slot));
        const int span = prize.high - prize.low + 1;
        out.push_back(Stack{ prize.id,
                             prize.low + static_cast<int>(amount * static_cast<float>(span)) });
    }

    // Never empty. A vault you went to the trouble of opening and that
    // paid nothing is a bug however honest the dice were.
    if (out.empty()) out.push_back(Stack{ Items::IronIngot, 3 });

    return out;
}
