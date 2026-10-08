#pragma once
#include "Items.h"
#include "World/Block.h"
#include <cstdint>
#include <glm/glm.hpp>

class World;

// Tilling, planting, growing and harvesting wheat.
//
// Crops keep their whole state in the block id -- eight ids, one per
// stage -- so a field saves and loads for free with the chunks it stands
// in, and there is no list of growing things to keep in step with the
// world.
namespace Farming
{
    // How far from the player crops are ticked. Beyond this a field sits
    // still, which is the one thing this does differently from Minecraft
    // and is invisible unless you stand and watch.
    //
    // The numbers are chosen together so a crop ripens in a couple of
    // minutes rather than Minecraft's twenty: a pass samples
    // TICKS_PER_PASS positions out of a box of (2R)^2 * HEIGHT, which
    // gives each crop roughly a one-in-thirty-five chance per pass, and
    // there are two passes a second. Sampling rather than scanning is
    // what keeps the cost in the hundreds of lookups rather than the
    // tens of thousands.
    constexpr int TICK_RADIUS = 24;
    constexpr int TICK_HEIGHT = 4;          // levels either side of the player
    constexpr float TICK_INTERVAL = 0.5f;   // seconds between growth passes
    constexpr int TICKS_PER_PASS = 600;
    constexpr int LIGHT_TO_GROW = 9;

    // Dirt and grass can be tilled; nothing else can.
    bool isTillable(BlockId id);

    // What a crop leaves when it is broken, which depends on how far
    // along it was. A ripe one gives wheat and seed to sow again.
    struct Harvest
    {
        StackId first = Blocks::Air;
        int firstCount = 0;
        StackId second = Blocks::Air;
        int secondCount = 0;
    };
    Harvest harvestOf(BlockId crop, uint32_t roll);

    // Turns the block into farmland and plants on top of it. False when
    // the ground is wrong or there is no room above it.
    bool plant(World& world, const glm::ivec3& ground);

    // Advances a scatter of crops near the player. `seed` is stirred
    // each call so the same columns are not picked every time.
    void grow(World& world, const glm::vec3& around, uint32_t& seed);

    // Whether this block would grow if it were ticked: a crop, not yet
    // ripe, standing on farmland in enough light.
    bool canGrowAt(const World& world, const glm::ivec3& at);
}
