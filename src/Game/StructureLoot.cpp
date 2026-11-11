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
        { Items::Saddle,      1, 1 },
        { Items::IronHorseArmor, 1, 1 },
        { Items::GoldHorseArmor, 1, 1 },
        { Items::DiamondHorseArmor, 1, 1 },
        { Items::DuneTemplate, 1, 1 },
        { Items::NameTag, 1, 1 },
        { Items::EnchantedGoldenApple, 1, 1 },
        { Items::GoldenApple, 1, 1 },
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
        { Items::Saddle,      1, 1 },
        { Items::IronHorseArmor, 1, 1 },
        { Items::GoldHorseArmor, 1, 1 },
        { Items::DiamondHorseArmor, 1, 1 },
        { Items::WildTemplate, 1, 1 },
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
        { Items::SentryTemplate, 1, 1 },
        { Items::GoatHorn,    1, 1 },
        { Items::ExperienceBottle, 1, 3 },
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
        { Items::VexTemplate, 1, 1 },
        { Items::MusicDiscFirst + 0, 1, 1 },
        { Items::MusicDiscFirst + 1, 1, 1 },
    };

    // Somebody's voyage, and what they never got to spend.
    const Entry SHIPWRECK[] = {
        { Items::IronIngot,   1, 5 },
        { Items::GoldIngot,   1, 5 },
        { Items::Emerald,     1, 5 },
        { Items::Diamond,     1, 1 },
        { Items::ExperienceBottle, 1, 1 },
        { Items::IronNugget,  1, 10 },
        { Items::GoldNugget,  1, 10 },
        { Items::LapisLazuli, 1, 10 },
        { Items::CoastTemplate, 1, 1 },
    };

    const Entry OCEAN_RUIN[] = {
        { Items::Coal,        1, 4 },
        { Items::GoldNugget,  1, 3 },
        { Items::Emerald,     1, 1 },
        { Items::Wheat,       2, 3 },
        { Items::StoneAxe,    1, 1 },
        { Items::RottenFlesh, 1, 1 },
        { Items::GoldenApple, 1, 1 },
        { Items::EnchantedBook, 1, 1 },
        { Items::LeatherChestplate, 1, 1 },
        { Items::FishingRod,  1, 1 },
        { Items::FilledMap,   1, 1 },
    };

    const Entry WRECK_SUPPLY[] = {
        { Items::Paper,       1, 12 },
        { Items::Potato,      2, 6 },
        { Items::PoisonousPotato, 2, 6 },
        { Items::Carrot,      4, 8 },
        { Items::Wheat,       8, 21 },
        { Items::SuspiciousStew, 1, 1 },
        { Items::Coal,        2, 8 },
        { Items::RottenFlesh, 5, 24 },
        { Items::Gunpowder,   1, 5 },
        { Items::LeatherHelmet, 1, 1 },
        { Items::LeatherChestplate, 1, 1 },
        { Items::CoastTemplate, 1, 1 },
    };

    const Entry WRECK_MAP[] = {
        { Items::Compass,     1, 1 },
        { Items::EmptyMap,    1, 1 },
        { Items::Clock,       1, 1 },
        { Items::Paper,       1, 10 },
        { Items::Feather,     1, 5 },
        { Items::Book,        1, 5 },
        { Items::CoastTemplate, 1, 1 },
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
        { Items::PrismarineCrystals, 1, 5 },
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

    const Entry DUNGEON[] = {
        { Items::Saddle,      1, 1 },
        { Items::GoldenApple, 1, 1 },
        { Items::Bread,       1, 3 },
        { Items::Wheat,       1, 4 },
        { Items::Gunpowder,   1, 4 },
        { Items::StringItem,  1, 4 },
        { Items::Bucket,      1, 1 },
        { Items::IronIngot,   1, 4 },
        { Items::GoldIngot,   1, 4 },
        { Items::Redstone,    1, 4 },
        { Items::NameTag,     1, 1 },
        { Items::IronHorseArmor, 1, 1 },
        { Items::Bone,        1, 8 },
        { Items::RottenFlesh, 1, 8 },
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
        { Items::NameTag,     1, 1 },
        { Items::Lead,        1, 1 },
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
        { Items::EyeTemplate, 1, 1 },
        { Items::MusicDiscFirst + 12, 1, 1 },
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
        { Items::WardTemplate, 1, 1 },
        { Items::SilenceTemplate, 1, 1 },
        { Items::Lead,        1, 1 },
        { Items::EchoShard,   1, 3 },
        { Items::ExperienceBottle, 1, 3 },
        { Items::DiscFragment, 1, 3 },
        { Items::MusicDiscFirst + 12, 1, 1 },
    };

    // A witch's hut. What she brews with, and what she has brewed.
    const Entry HUT[] = {
        { Items::GlassBottle, 2, 5 },
        { Items::Sugar,       1, 4 },
        { Items::Redstone,    1, 4 },
        { Items::GlowstoneDust, 1, 3 },
        { Items::NetherWart,  1, 2 },
        { Items::Potion,      1, 2 },
        { Items::StringItem,  1, 4 },
    };

    // The End's city. Minecraft keeps elytra and enchanted diamond
    // armour out there; there are no elytra here, so it is the armour
    // and the purpur.
    const Entry ENDCITY[] = {
        { Items::DiamondChestplate, 1, 1 },
        { Items::DiamondHelmet, 1, 1 },
        { Items::Diamond,     2, 5 },
        { Items::Emerald,     2, 7 },
        { Blocks::PurpurBlock, 2, 8 },
        { Items::EnderPearl,  2, 5 },
        { Items::GoldIngot,   2, 6 },
        { Items::SpireTemplate, 1, 1 },
    };

    // A bastion, which is gold and nothing but: it is the one place
    // in the Nether worth raiding and the only reason to go.
    const Entry BASTION_TREASURE[] = {
        { Items::NetheriteIngot, 1, 1 },
        { Blocks::AncientDebris, 1, 2 },
        { Items::NetheriteScrap, 1, 2 },
        { Items::DiamondSword, 1, 1 },
        { Items::DiamondChestplate, 1, 1 },
        { Items::DiamondHelmet, 1, 1 },
        { Items::DiamondLeggings, 1, 1 },
        { Items::DiamondBoots, 1, 1 },
        { Items::Diamond, 2, 6 },
        { Items::EnchantedGoldenApple, 1, 1 },
        { Items::SpectralArrow, 12, 25 },
        { Blocks::GoldBlock, 2, 5 },
        { Blocks::IronBlock, 2, 5 },
        { Items::GoldIngot, 3, 9 },
        { Blocks::CryingObsidian, 3, 5 },
        { Items::NetherQuartz, 8, 23 },
        { Items::MagmaCream, 2, 8 },
        { Items::SnoutTemplate, 1, 1 },
        { Items::NetheriteUpgradeTemplate, 1, 1 },
    };

    const Entry BASTION_BRIDGE[] = {
        { Blocks::Lodestone, 1, 1 },
        { Items::Crossbow, 1, 1 },
        { Items::SpectralArrow, 10, 28 },
        { Blocks::GildedBlackstone, 8, 12 },
        { Blocks::CryingObsidian, 3, 8 },
        { Blocks::GoldBlock, 1, 1 },
        { Items::GoldIngot, 4, 9 },
        { Items::IronIngot, 4, 9 },
        { Items::GoldSword, 1, 1 },
        { Items::GoldChestplate, 1, 1 },
        { Items::GoldHelmet, 1, 1 },
        { Items::GoldLeggings, 1, 1 },
        { Items::GoldBoots, 1, 1 },
        { Items::GoldAxe, 1, 1 },
        { Items::StringItem, 1, 6 },
        { Items::Leather, 1, 3 },
        { Items::Arrow, 5, 17 },
        { Items::SnoutTemplate, 1, 1 },
        { Items::NetheriteUpgradeTemplate, 1, 1 },
    };

    const Entry BASTION_STABLE[] = {
        { Items::DiamondShovel, 1, 1 },
        { Items::DiamondPickaxe, 1, 1 },
        { Items::NetheriteScrap, 1, 1 },
        { Blocks::AncientDebris, 1, 2 },
        { Items::Saddle, 1, 1 },
        { Blocks::GoldBlock, 2, 4 },
        { Items::GoldenCarrot, 8, 17 },
        { Items::GoldenApple, 1, 1 },
        { Items::GoldAxe, 1, 1 },
        { Blocks::CryingObsidian, 1, 5 },
        { Blocks::Glowstone, 3, 6 },
        { Blocks::GildedBlackstone, 2, 5 },
        { Blocks::SoulSand, 2, 7 },
        { Blocks::CrimsonNylium, 2, 7 },
        { Items::Leather, 1, 3 },
        { Items::Arrow, 5, 17 },
        { Items::StringItem, 3, 8 },
        { Items::RawPorkchop, 2, 5 },
        { Items::CookedPorkchop, 2, 5 },
        { Items::SnoutTemplate, 1, 1 },
        { Items::NetheriteUpgradeTemplate, 1, 1 },
    };

    const Entry BASTION[] = {
        { Items::GoldIngot,   4, 12 },
        { Blocks::GildedBlackstone, 1, 4 },
        { Items::GoldSword,   1, 1 },
        { Items::GoldChestplate, 1, 1 },
        { Items::Diamond,     1, 2 },
        { Items::MagmaCream,  1, 3 },
        { Blocks::Blackstone, 2, 8 },
        { Items::NetheriteUpgradeTemplate, 1, 1 },
        { Items::SnoutTemplate, 1, 1 },
        { Items::RibTemplate, 1, 1 },
        { Items::PiglinBannerPattern, 1, 1 },
        { Items::EnchantedGoldenApple, 1, 1 },
        { Items::MusicDiscFirst + 14, 1, 1 },
    };

    // A supply room, which is exactly what it sounds like: what you
    // want in your hands before the next spawner starts.
    const Entry LIBRARY[] = {
        { Items::Book,    1, 3 },
        { Items::Paper,   2, 7 },
        { Items::EmptyMap, 1, 1 },
        { Items::Compass, 1, 1 },
        { Items::EnchantedBook, 1, 1 },
    };

    const Entry FORTRESS[] = {
        { Items::Diamond, 1, 3 },
        { Items::IronIngot, 1, 5 },
        { Items::GoldIngot, 1, 3 },
        { Items::GoldSword, 1, 1 },
        { Items::GoldChestplate, 1, 1 },
        { Items::FlintAndSteel, 1, 1 },
        { Items::NetherWart, 3, 7 },
        { Items::Saddle, 1, 1 },
        { Items::GoldHorseArmor, 1, 1 },
        { Items::IronHorseArmor, 1, 1 },
        { Items::DiamondHorseArmor, 1, 1 },
        { Blocks::Obsidian, 2, 4 },
    };

    const Entry TRAP[] = {
        { Items::Arrow, 2, 7 },
    };

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
            case WorldGen::Hoard::Dungeon:
                return { DUNGEON, static_cast<int>(std::size(DUNGEON)) };
            case WorldGen::Hoard::Stronghold:
                return { STRONGHOLD, static_cast<int>(std::size(STRONGHOLD)) };
            case WorldGen::Hoard::SwampHut:
                return { HUT, static_cast<int>(std::size(HUT)) };
            case WorldGen::Hoard::EndCity:
                return { ENDCITY, static_cast<int>(std::size(ENDCITY)) };
            case WorldGen::Hoard::Bastion:
                return { BASTION, static_cast<int>(std::size(BASTION)) };
            case WorldGen::Hoard::AncientCity:
                return { CITY, static_cast<int>(std::size(CITY)) };
            case WorldGen::Hoard::TrialChamber:
                return { SUPPLY, static_cast<int>(std::size(SUPPLY)) };
            case WorldGen::Hoard::TrapDispenser:
                return { TRAP, static_cast<int>(std::size(TRAP)) };
            case WorldGen::Hoard::Fortress:
                return { FORTRESS, static_cast<int>(std::size(FORTRESS)) };
            case WorldGen::Hoard::StrongholdLibrary:
                return { LIBRARY, static_cast<int>(std::size(LIBRARY)) };
            case WorldGen::Hoard::BastionTreasure:
                return { BASTION_TREASURE, static_cast<int>(std::size(BASTION_TREASURE)) };
            case WorldGen::Hoard::BastionBridge:
                return { BASTION_BRIDGE, static_cast<int>(std::size(BASTION_BRIDGE)) };
            case WorldGen::Hoard::BastionStable:
                return { BASTION_STABLE, static_cast<int>(std::size(BASTION_STABLE)) };
            case WorldGen::Hoard::OceanRuin:
                return { OCEAN_RUIN, static_cast<int>(std::size(OCEAN_RUIN)) };
            case WorldGen::Hoard::ShipwreckSupply:
                return { WRECK_SUPPLY, static_cast<int>(std::size(WRECK_SUPPLY)) };
            case WorldGen::Hoard::ShipwreckMap:
                return { WRECK_MAP, static_cast<int>(std::size(WRECK_MAP)) };
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
        case WorldGen::Hoard::BuriedTreasure:  out.id = Items::HeartOfTheSea; out.count = 1; break;
        case WorldGen::Hoard::Monument:        out.id = Items::GoldIngot; out.count = 8; break;
        case WorldGen::Hoard::Mineshaft:       out.id = Blocks::RailEastWest; out.count = 6; break;
        case WorldGen::Hoard::Dungeon:         out.id = static_cast<StackId>(Items::MusicDiscFirst + static_cast<int>(hashCoords(x, y, z, seed) % 3u)); out.count = 1; break;
        case WorldGen::Hoard::Stronghold:      out.id = Items::Book;      out.count = 2; break;
        case WorldGen::Hoard::SwampHut:        out.id = Items::GlassBottle; out.count = 3; break;
        case WorldGen::Hoard::EndCity:         out.id = Items::Diamond;   out.count = 2; break;
        case WorldGen::Hoard::Bastion:         out.id = Items::GoldIngot; out.count = 8; break;
        case WorldGen::Hoard::AncientCity:     out.id = Items::Diamond;   out.count = 3; break;
        case WorldGen::Hoard::TrialChamber:    out.id = Items::Arrow;     out.count = 8; break;
        case WorldGen::Hoard::TrapDispenser:   out.id = Items::Arrow;     out.count = 5; break;
        case WorldGen::Hoard::Fortress:        out.id = Items::GoldIngot; out.count = 2; break;
        case WorldGen::Hoard::StrongholdLibrary: out.id = Items::Book;    out.count = 3; break;
        case WorldGen::Hoard::ShipwreckSupply: out.id = Items::Wheat;     out.count = 9; break;
        case WorldGen::Hoard::OceanRuin:       out.id = Items::Coal;      out.count = 2; break;
        case WorldGen::Hoard::BastionTreasure: out.id = Items::NetheriteIngot; out.count = 1; break;
        case WorldGen::Hoard::BastionBridge:   out.id = Items::GoldIngot; out.count = 6; break;
        case WorldGen::Hoard::BastionStable:   out.id = Items::GoldenCarrot; out.count = 9; break;
        case WorldGen::Hoard::ShipwreckMap:    out.id = Items::FilledMap; out.count = 1; return out;
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
