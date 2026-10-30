#pragma once
#include "Player/Inventory.h"
#include "World/Block.h"

// What a hopper does.
//
// One item at a time, pulled from whatever is above and pushed into
// whatever is below. That is the whole of it in Minecraft too -- the
// rest of a hopper's behaviour is the speed it does it at and which
// way it points, and this one points down, because everything in this
// game sits on top of something rather than on the side of it.
//
// The rule knows nothing about the world. It reads and writes through
// Holder, so --selftest can wire two chests to a hopper in a plain
// std::map and watch a stack move with no window and no chunks.
namespace Hopper
{
    // How long between one item and the next. Minecraft moves one
    // every eight ticks, which is this.
    constexpr float INTERVAL = 0.4f;

    // How many slots the three holders keep. A hopper has fewer than a
    // chest in Minecraft, and the difference matters: a five-slot
    // hopper fills and stops, which is how a sorter is built.
    constexpr int HOPPER_SLOTS = 5;
    constexpr int THROWER_SLOTS = 9;

    // Whatever can answer what is where and hand back its slots.
    struct Holder
    {
        virtual ~Holder() = default;
        virtual BlockId at(int x, int y, int z) const = 0;

        // The slots of the container at this point, or null if what is
        // there keeps nothing. The span is however many that kind of
        // block has.
        virtual ItemStack* slotsAt(int x, int y, int z, int& count) = 0;
    };

    // How many slots this kind of block keeps. Zero for anything that
    // is not a container.
    int slotsOf(BlockId id);

    // Moves one item into `into` if there is room for it, and says
    // whether it went. Stacks onto a matching slot first, which is
    // what stops a chest filling with single-item piles.
    bool give(ItemStack* into, int count, StackId id);

    // Takes one item out of the first slot holding anything, and says
    // what it was. Air if there was nothing.
    StackId take(ItemStack* from, int count);

    // One hopper, one step: pull from above, push below. Returns true
    // if anything moved, which is what tells the caller to make a
    // noise or redraw.
    bool step(Holder& holder, int x, int y, int z);
}
