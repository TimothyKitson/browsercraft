#include "StructureLoot.h"
#include "World/Noise.h"
#include <algorithm>

namespace
{
    struct Entry
    {
        StackId id;
        int least;
        int most;
    };

    // Gold, bones, and the thing nobody wants to be standing on.
    const Entry PYRAMID[] = {
        { Items::GoldIngot,   1, 4 },
        { Items::Bone,        2, 6 },
        { Items::Gunpowder,   1, 4 },
        { Items::StringItem,  1, 4 },
        { Items::Emerald,     1, 2 },
        { Items::Diamond,     1, 1 },
        { Blocks::Sandstone,  2, 8 },
        { Items::EnderPearl,  1, 1 },
    };

    // The one everybody walks out of carrying something good, which is
    // what makes it worth the tripwire it is famous for.
    const Entry TEMPLE[] = {
        { Items::Diamond,     1, 2 },
        { Items::Emerald,     1, 3 },
        { Items::GoldIngot,   1, 3 },
        { Items::Bone,        1, 5 },
        { Blocks::JunglePlanks, 2, 8 },
        { Items::IronIngot,   1, 4 },
        { Items::Arrow,       2, 6 },
    };

    // Somebody was living here: a meal, a fire, and not much else.
    const Entry IGLOO[] = {
        { Items::Bread,       1, 3 },
        { Items::Coal,        1, 4 },
        { Items::Wheat,       1, 5 },
        { Items::Emerald,     1, 1 },
        { Items::StoneAxe,    1, 1 },
        { Items::GoldIngot,   1, 1 },
    };

    // All gold, because that is what a ruined portal is for: it is the
    // one place in the overworld the Nether's metal turns up.
    const Entry RUIN[] = {
        { Items::GoldIngot,   1, 6 },
        { Items::GoldHelmet,  1, 1 },
        { Items::GoldBoots,   1, 1 },
        { Items::GoldSword,   1, 1 },
        { Items::FlintAndSteel, 1, 1 },
        { Blocks::Obsidian,   1, 3 },
        { Items::GlowstoneDust, 1, 4 },
    };

    // A raiding party's stores.
    const Entry OUTPOST[] = {
        { Items::Arrow,       4, 12 },
        { Items::Bow,         1, 1 },
        { Blocks::DarkOakPlanks, 2, 8 },
        { Items::Wheat,       1, 5 },
        { Items::IronIngot,   1, 2 },
        { Items::Emerald,     1, 2 },
    };

    // A mansion is somebody's house, and a rich one: it is the
    // longest walk to any structure in the game, so it pays like it.
    const Entry MANSION[] = {
        { Items::Diamond,     1, 3 },
        { Items::Emerald,     2, 6 },
        { Items::GoldIngot,   1, 5 },
        { Items::IronIngot,   2, 6 },
        { Blocks::DarkOakPlanks, 2, 8 },
        { Items::Book,        1, 3 },
        { Items::Bread,       1, 4 },
        { Items::DiamondHoe,  1, 1 },
    };

    // Somebody's voyage, and what they never got to spend.
    const Entry SHIPWRECK[] = {
        { Items::Emerald,     1, 4 },
        { Items::IronIngot,   1, 4 },
        { Items::GoldIngot,   1, 3 },
        { Items::Paper,       1, 6 },
        { Items::Bread,       1, 4 },
        { Items::WheatSeeds,  1, 6 },
        { Items::Leather,     1, 3 },
        { Items::Diamond,     1, 1 },
    };

    // The best chest in the overworld, and the only one that needs a
    // map. It is allowed to be worth the dig.
    const Entry TREASURE[] = {
        { Items::Diamond,     1, 3 },
        { Items::Emerald,     2, 6 },
        { Items::GoldIngot,   2, 8 },
        { Items::IronIngot,   2, 6 },
        { Items::CookedPorkchop, 1, 4 },
        { Items::Potion,      1, 1 },
    };

    // What is behind the dark prismarine. Minecraft keeps eight blocks
    // of gold in there; there is no block of gold here, so it is the
    // ingots and a sponge instead.
    const Entry MONUMENT[] = {
        { Items::GoldIngot,   4, 10 },
        { Blocks::Sponge,     1, 2 },
        { Blocks::SeaLantern, 1, 4 },
        { Items::Emerald,     1, 4 },
        { Items::Diamond,     1, 2 },
        { Blocks::Prismarine, 2, 8 },
    };

    // What the miners left behind.
    const Entry MINESHAFT[] = {
        { Blocks::RailEastWest, 2, 8 },
        { Items::Coal,        2, 8 },
        { Items::IronIngot,   1, 4 },
        { Items::GoldIngot,   1, 2 },
        { Items::Diamond,     1, 1 },
        { Blocks::Torch,      2, 6 },
        { Items::Bread,       1, 3 },
        { Items::StringItem,  1, 4 },
    };

    // A stronghold is a library with a portal in it, so its store room
    // holds what a library runs on and what the walk down there costs.
    const Entry STRONGHOLD[] = {
        { Items::Book,        1, 3 },
        { Items::Paper,       2, 7 },
        { Items::EnderPearl,  1, 2 },
        { Items::IronIngot,   1, 5 },
        { Items::Diamond,     1, 2 },
        { Items::Bread,       1, 3 },
        { Items::IronPickaxe, 1, 1 },
    };

    // The deepest chest in the world, and the one with the longest
    // walk home from it. Minecraft puts its best enchanted books down
    // there; there is no enchanted book here, so it is the diamonds
    // and the echo of one.
    const Entry CITY[] = {
        { Items::Diamond,     2, 5 },
        { Items::Emerald,     2, 8 },
        { Items::Book,        1, 4 },
        { Items::DiamondHoe,  1, 1 },
        { Blocks::Sculk,      2, 8 },
        { Items::EnderPearl,  1, 3 },
        { Items::GoldIngot,   2, 6 },
        { Items::Potion,      1, 2 },
    };

    // A supply room, which is exactly what it sounds like: what you
    // want in your hands before the next spawner starts.
    const Entry SUPPLY[] = {
        { Items::Arrow,       4, 12 },
        { Items::Bread,       1, 4 },
        { Items::Potion,      1, 2 },
        { Items::StonePickaxe, 1, 1 },
        { Blocks::TuffBricks, 2, 8 },
        { Items::CopperIngot, 1, 4 },
    };

    struct Table
    {
        const Entry* entries;
        int count;
    };

    Table tableFor(WorldGen::Hoard hoard)
    {
        switch (hoard)
        {
            case WorldGen::Hoard::DesertPyramid:
                return { PYRAMID, static_cast<int>(std::size(PYRAMID)) };
            case WorldGen::Hoard::JungleTemple:
                return { TEMPLE, static_cast<int>(std::size(TEMPLE)) };
            case WorldGen::Hoard::Igloo:
                return { IGLOO, static_cast<int>(std::size(IGLOO)) };
            case WorldGen::Hoard::RuinedPortal:
                return { RUIN, static_cast<int>(std::size(RUIN)) };
            case WorldGen::Hoard::PillagerOutpost:
                return { OUTPOST, static_cast<int>(std::size(OUTPOST)) };
            case WorldGen::Hoard::WoodlandMansion:
                return { MANSION, static_cast<int>(std::size(MANSION)) };
            case WorldGen::Hoard::Shipwreck:
                return { SHIPWRECK, static_cast<int>(std::size(SHIPWRECK)) };
            case WorldGen::Hoard::BuriedTreasure:
                return { TREASURE, static_cast<int>(std::size(TREASURE)) };
            case WorldGen::Hoard::Monument:
                return { MONUMENT, static_cast<int>(std::size(MONUMENT)) };
            case WorldGen::Hoard::Mineshaft:
                return { MINESHAFT, static_cast<int>(std::size(MINESHAFT)) };
            case WorldGen::Hoard::Stronghold:
                return { STRONGHOLD, static_cast<int>(std::size(STRONGHOLD)) };
            case WorldGen::Hoard::AncientCity:
                return { CITY, static_cast<int>(std::size(CITY)) };
            case WorldGen::Hoard::TrialChamber:
                return { SUPPLY, static_cast<int>(std::size(SUPPLY)) };
            case WorldGen::Hoard::None:
                break;
        }
        return { nullptr, 0 };
    }
}

StructureLoot::Stack StructureLoot::centrepiece(WorldGen::Hoard hoard,
                                                int x, int y, int z, uint32_t seed)
{
    Stack out;
    switch (hoard)
    {
        case WorldGen::Hoard::DesertPyramid:   out.id = Items::GoldIngot; out.count = 3; break;
        case WorldGen::Hoard::JungleTemple:    out.id = Items::Diamond;   out.count = 1; break;
        case WorldGen::Hoard::Igloo:           out.id = Items::Bread;     out.count = 2; break;
        case WorldGen::Hoard::RuinedPortal:    out.id = Items::GoldIngot; out.count = 4; break;
        case WorldGen::Hoard::PillagerOutpost: out.id = Items::Arrow;     out.count = 8; break;
        case WorldGen::Hoard::WoodlandMansion: out.id = Items::Emerald;   out.count = 4; break;
        case WorldGen::Hoard::Shipwreck:       out.id = Items::Emerald;   out.count = 2; break;
        case WorldGen::Hoard::BuriedTreasure:  out.id = Items::Diamond;   out.count = 2; break;
        case WorldGen::Hoard::Monument:        out.id = Items::GoldIngot; out.count = 8; break;
        case WorldGen::Hoard::Mineshaft:       out.id = Blocks::RailEastWest; out.count = 6; break;
        case WorldGen::Hoard::Stronghold:      out.id = Items::Book;      out.count = 2; break;
        case WorldGen::Hoard::AncientCity:     out.id = Items::Diamond;   out.count = 3; break;
        case WorldGen::Hoard::TrialChamber:    out.id = Items::Arrow;     out.count = 8; break;
        case WorldGen::Hoard::None:            return out;
    }

    // A little spread on the count, so two of the same kind are not
    // identical to the item.
    const int wobble =
        static_cast<int>(hashCoords(x, y, z, seed + 62010) % 3u);
    out.count = std::max(1, out.count + wobble - 1);
    return out;
}

std::vector<StructureLoot::Stack> StructureLoot::rollFor(WorldGen::Hoard hoard,
                                                         int x, int y, int z, uint32_t seed)
{
    std::vector<Stack> out;

    const Table table = tableFor(hoard);
    if (table.count == 0) return out;

    // The thing this kind of place is known for goes in first and is
    // never rolled for. A pyramid with no gold in it is a hole.
    const Stack signature = centrepiece(hoard, x, y, z, seed);
    if (signature.count > 0) out.push_back(signature);

    const int spread = MAX_STACKS - MIN_STACKS + 1;
    const int wanted = MIN_STACKS +
        static_cast<int>(hashCoords(x, y, z, seed + 62001) % static_cast<uint32_t>(spread));

    // Shuffled and taken off the front, the way the village chests do
    // it: drawing each line on its own repeats them, and a chest
    // holding three separate lots of bone reads as a bug.
    std::vector<int> order(static_cast<size_t>(table.count));
    for (int i = 0; i < table.count; ++i) order[static_cast<size_t>(i)] = i;

    uint32_t state = hashCoords(x, y, z, seed + 62002) | 1u;
    for (int i = table.count - 1; i > 0; --i)
    {
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
        if (entry.most <= 0) continue;
        if (entry.id == signature.id) continue;

        const int range = entry.most - entry.least + 1;
        const int count = entry.least +
            static_cast<int>(hashCoords(x + taken, y, z, seed + 62004) %
                             static_cast<uint32_t>(std::max(1, range)));

        Stack stack;
        stack.id = entry.id;
        stack.count = std::min(count, maxStackOf(entry.id));
        if (stack.count > 0) out.push_back(stack);
    }

    return out;
}
