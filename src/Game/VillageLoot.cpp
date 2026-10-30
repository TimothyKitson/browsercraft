#include "VillageLoot.h"
#include "World/Noise.h"
#include <algorithm>

namespace
{
    // One line of a trade's table: what it is, and the fewest and most
    // of it that ever turn up at once.
    struct Entry
    {
        StackId id;
        int least;
        int most;
    };

    // A blacksmith works metal, so that is what is in the box: the
    // ingots, the coal to work them, and now and then something already
    // made. The diamond is the reason to look.
    const Entry SMITH[] = {
        { Items::IronIngot,     1, 5 },
        { Items::Coal,          2, 8 },
        { Items::GoldIngot,     1, 3 },
        { Items::Diamond,       1, 2 },
        { Items::IronPickaxe,   1, 1 },
        { Items::IronSword,     1, 1 },
        { Items::IronHelmet,    1, 1 },
        { Items::IronChestplate,1, 1 },
        { Items::Flint,         1, 3 },
        { Blocks::Obsidian,     1, 2 },
    };

    // A library keeps what it is made of, and what the table next door
    // runs on.
    const Entry LIBRARY[] = {
        { Items::Paper,         3, 9 },
        { Items::Book,          1, 3 },
        { Blocks::Bookshelf,    1, 2 },
        { Items::Emerald,       1, 2 },
        { Blocks::SugarCane,    1, 4 },
        { Items::Leather,       1, 3 },
        { Items::WheatSeeds,    1, 5 },
    };

    // A butcher keeps meat, and the coal to cook it with.
    const Entry BUTCHER[] = {
        { Items::RawBeef,       1, 4 },
        { Items::RawPorkchop,   1, 4 },
        { Items::RawChicken,    1, 4 },
        { Items::RawMutton,     1, 4 },
        { Items::Steak,         1, 3 },
        { Items::CookedPorkchop,1, 3 },
        { Items::Coal,          1, 4 },
        { Items::Leather,       1, 4 },
    };

    // An ordinary household: what anybody would have about the place.
    const Entry HOME[] = {
        { Items::Bread,         1, 3 },
        // Apples are in a Minecraft village house chest too. They are
        // also the only route into the game the block audit can see:
        // the other one is a chance drop off oak leaves, which is not
        // something blockDrop can say.
        { Items::Apple,         1, 3 },
        { Items::WheatSeeds,    1, 6 },
        { Items::Wheat,         1, 4 },
        { Items::Stick,         1, 6 },
        { Blocks::Planks,       2, 8 },
        { Blocks::Torch,        1, 6 },
        { Items::Coal,          1, 3 },
        { Items::StringItem,    1, 3 },
        { Items::Feather,       1, 3 },
    };

    struct Table
    {
        const Entry* entries;
        int count;
    };

    Table tableFor(Village::Kind kind)
    {
        switch (kind)
        {
            case Village::Kind::Blacksmith:
                return { SMITH, static_cast<int>(std::size(SMITH)) };
            case Village::Kind::Library:
                return { LIBRARY, static_cast<int>(std::size(LIBRARY)) };
            case Village::Kind::Butcher:
                return { BUTCHER, static_cast<int>(std::size(BUTCHER)) };
            case Village::Kind::Home:
                return { HOME, static_cast<int>(std::size(HOME)) };
            default:
                return { nullptr, 0 };
        }
    }
}

std::vector<VillageLoot::Stack> VillageLoot::rollFor(Village::Kind kind,
                                                     int x, int y, int z, uint32_t seed)
{
    std::vector<Stack> out;

    const Table table = tableFor(kind);
    if (table.count == 0) return out;

    // How many things are in it, which is what makes one chest worth
    // more than the next.
    const int spread = MAX_STACKS - MIN_STACKS + 1;
    const int wanted = MIN_STACKS +
        static_cast<int>(hashCoords(x, y, z, seed + 61001) % static_cast<uint32_t>(spread));

    // Shuffle the table and take off the front. Drawing each line
    // independently would repeat them, and a chest holding three
    // separate lots of coal reads as a bug rather than a windfall.
    // A stride through the table has the same trouble whenever the
    // stride and the table share a factor, which is why this shuffles
    // rather than steps.
    std::vector<int> order(static_cast<size_t>(table.count));
    for (int i = 0; i < table.count; ++i) order[static_cast<size_t>(i)] = i;

    uint32_t state = hashCoords(x, y, z, seed + 61002) | 1u;
    for (int i = table.count - 1; i > 0; --i)
    {
        // splitmix, so successive draws off one position do not march
        // in step the way a plain multiply-and-shift does.
        state += 0x9E3779B9u;
        uint32_t mixed = state;
        mixed = (mixed ^ (mixed >> 16)) * 0x21F0AAADu;
        mixed = (mixed ^ (mixed >> 15)) * 0x735A2D97u;
        mixed ^= mixed >> 15;

        const int j = static_cast<int>(mixed % static_cast<uint32_t>(i + 1));
        std::swap(order[static_cast<size_t>(i)], order[static_cast<size_t>(j)]);
    }

    const int taking = std::min(wanted, table.count);
    for (int taken = 0; taken < taking; ++taken)
    {
        const Entry& entry = table.entries[order[static_cast<size_t>(taken)]];

        const int range = entry.most - entry.least + 1;
        const int count = entry.least +
            static_cast<int>(hashCoords(x + taken, y, z, seed + 61004) %
                             static_cast<uint32_t>(std::max(1, range)));

        out.push_back({ entry.id, count });
    }

    return out;
}
