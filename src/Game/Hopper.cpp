#include "Hopper.h"

int Hopper::slotsOf(BlockId id)
{
    if (isHopper(id)) return HOPPER_SLOTS;
    if (isDropper(id) || isDispenser(id)) return THROWER_SLOTS;
    if (isStorageChest(id)) return 27;
    if (isDecoratedPot(id)) return 1;
    if (isBarrel(id) || isShulkerBox(id)) return 27;
    return 0;
}

bool Hopper::give(ItemStack* into, int count, StackId id)
{
    ItemStack one;
    one.id = id;
    one.count = 1;
    return give(into, count, one);
}

bool Hopper::give(ItemStack* into, int count, const ItemStack& one)
{
    if (!into || count <= 0 || one.id == Blocks::Air) return false;

    for (int i = 0; i < count; ++i)
        if (!into[i].empty() && into[i].sameAs(one) && into[i].count < maxStackOf(one.id))
        {
            into[i].count += 1;
            return true;
        }

    for (int i = 0; i < count; ++i)
        if (into[i].empty())
        {
            into[i] = one;
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
    const BlockId self_ = holder.at(x, y, z);
    if (!isHopper(self_)) return false;
    static const int STEP[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    const int facing = hopperFacing(self_);
    const int tx = facing < 0 ? x : x + STEP[facing][0];
    const int ty = facing < 0 ? y - 1 : y;
    const int tz = facing < 0 ? z : z + STEP[facing][1];

    int mine = 0;
    ItemStack* self = holder.slotsAt(x, y, z, mine);
    if (!self) return false;

    bool moved = false;

    // Push first, then pull. The other way round, an item pulled in
    // this step could be pushed out in the same one, and a chain of
    // hoppers would move a stack its whole length in a single tick.
    int belowCount = 0;
    if (ItemStack* below = holder.slotsAt(tx, ty, tz, belowCount))
    {
        // Look at what is there before taking it, so nothing is lost
        // when the thing below turns out to be full.
        for (int i = 0; i < mine && !moved; ++i)
        {
            if (self[i].empty()) continue;
            if (!give(below, belowCount, self[i])) continue;

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
            if (!give(self, mine, above[i])) continue;

            above[i].count -= 1;
            if (above[i].count <= 0) above[i].clear();
            moved = true;
            break;
        }
    }

    return moved;
}
