#include "Hopper.h"

int Hopper::slotsOf(BlockId id)
{
    if (id == Blocks::Hopper) return HOPPER_SLOTS;
    if (id == Blocks::Dropper || id == Blocks::Dispenser) return THROWER_SLOTS;
    if (id == Blocks::Chest) return 27;
    return 0;
}

bool Hopper::give(ItemStack* into, int count, StackId id)
{
    if (!into || count <= 0 || id == Blocks::Air) return false;

    // Onto a matching stack first. Without this a chest fed by a
    // hopper fills with twenty-seven piles of one.
    for (int i = 0; i < count; ++i)
        if (into[i].id == id && into[i].count < maxStackOf(id))
        {
            into[i].count += 1;
            return true;
        }

    for (int i = 0; i < count; ++i)
        if (into[i].empty())
        {
            into[i].id = id;
            into[i].count = 1;
            return true;
        }

    return false;
}

StackId Hopper::take(ItemStack* from, int count)
{
    if (!from || count <= 0) return Blocks::Air;

    for (int i = 0; i < count; ++i)
    {
        if (from[i].empty()) continue;

        const StackId id = from[i].id;
        from[i].count -= 1;
        if (from[i].count <= 0) from[i].clear();
        return id;
    }

    return Blocks::Air;
}

bool Hopper::step(Holder& holder, int x, int y, int z)
{
    if (holder.at(x, y, z) != Blocks::Hopper) return false;

    int mine = 0;
    ItemStack* self = holder.slotsAt(x, y, z, mine);
    if (!self) return false;

    bool moved = false;

    // Push first, then pull. The other way round, an item pulled in
    // this step could be pushed out in the same one, and a chain of
    // hoppers would move a stack its whole length in a single tick.
    int belowCount = 0;
    if (ItemStack* below = holder.slotsAt(x, y - 1, z, belowCount))
    {
        // Look at what is there before taking it, so nothing is lost
        // when the thing below turns out to be full.
        for (int i = 0; i < mine && !moved; ++i)
        {
            if (self[i].empty()) continue;
            if (!give(below, belowCount, self[i].id)) continue;

            self[i].count -= 1;
            if (self[i].count <= 0) self[i].clear();
            moved = true;
        }
    }

    int aboveCount = 0;
    if (ItemStack* above = holder.slotsAt(x, y + 1, z, aboveCount))
    {
        for (int i = 0; i < aboveCount; ++i)
        {
            if (above[i].empty()) continue;
            if (!give(self, mine, above[i].id)) break;   // full: stop trying

            above[i].count -= 1;
            if (above[i].count <= 0) above[i].clear();
            moved = true;
            break;
        }
    }

    return moved;
}
