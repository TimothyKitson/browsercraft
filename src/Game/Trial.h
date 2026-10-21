#pragma once
#include "Items.h"
#include "Entity/MobType.h"
#include <cstdint>
#include <vector>

// What a trial spawner does and what a vault pays out.
//
// Rules only: nothing here knows about chunks, mobs or the player. The
// spawner's state lives in its block id and the rest is arithmetic over
// a position and a seed, so --selftest can run a whole trial without a
// world to run it in.
namespace Trial
{
    // How near you have to be for a spawner to notice you, and how far
    // its wave can get before it counts as somebody else's problem.
    constexpr float WAKE_RANGE = 11.0f;
    constexpr float WAVE_RANGE = 16.0f;

    // How many it sends out at once, and how long it waits before it
    // will believe the wave is dead -- without that, a spawner reports
    // victory in the same frame it armed, before its mobs have been
    // added to the world.
    constexpr int WAVE_SIZE = 3;

    // What an ominous spawner sends instead, and how long the bottle
    // that makes one lasts. Minecraft's bad omen runs for a hundred
    // seconds, which is about as long as it takes to climb down.
    constexpr int OMINOUS_WAVE_SIZE = 5;
    constexpr float OMEN_SECONDS = 100.0f;
    constexpr float SETTLE_SECONDS = 2.0f;

    // What one spawner is worth. Minecraft pays a key a spawner, and
    // the keys are the only way into the vaults.
    constexpr int KEYS_PER_SPAWNER = 1;

    // Which species this spawner sends. Drawn from where it stands, so
    // the same spawner always sends the same thing and two players in
    // one world fight the same fight.
    MobId speciesFor(int x, int y, int z, uint32_t seed, int index);

    // How many a spawner sends, which is the whole difference an omen
    // makes to the fight.
    inline int waveSize(bool ominous) { return ominous ? OMINOUS_WAVE_SIZE : WAVE_SIZE; }

    // Which key it pays. The two are not interchangeable: an ominous
    // vault takes the ominous key and nothing else.
    StackId keyFor(bool ominous);

    // What a vault holds. The spread matters more than the total: a
    // vault that always paid the same would be a vending machine.
    struct Stack
    {
        StackId id = Blocks::Air;
        int count = 0;
    };

    constexpr int MIN_STACKS = 2;
    constexpr int MAX_STACKS = 4;

    std::vector<Stack> vaultLoot(int x, int y, int z, uint32_t seed);

    // What an ominous vault holds: everything a plain one can, more of
    // it, and the things that are only in there.
    std::vector<Stack> ominousVaultLoot(int x, int y, int z, uint32_t seed);
}
