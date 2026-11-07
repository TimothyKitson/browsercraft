#include "Crafting.h"
#include "Player/Inventory.h"
#include "Items.h"
#include "Tools.h"
#include "Armour.h"
#include <algorithm>

namespace
{
    constexpr StackId _ = Blocks::Air;   // an empty cell, readably

    struct Recipe
    {
        bool shapeless;
        int width, height;        // shaped only
        StackId pattern[9];       // shaped: row-major; shapeless: the ingredient list
        int inputCount;           // shapeless only
        StackId result;
        int resultCount;
    };

    // The recipe book is still short: there are items now, but no
    // sticks, ingots or tools, so most of Minecraft's recipes have no
    // ingredients to be made from yet.
    const Recipe RECIPES[] = {
        // One log becomes four planks of its own wood. Birch has no
        // plank of its own yet, so it gives oak -- planksFromLog() says
        // so in one place rather than this table saying it four times.
        { true,  0, 0, { Blocks::Log },        1, Blocks::Planks,        4 },
        { true,  0, 0, { Blocks::BirchLog },   1, Blocks::BirchPlanks,   4 },
        { true,  0, 0, { Blocks::StrippedOakLog },     1, Blocks::Planks,        4 },
        { true,  0, 0, { Blocks::StrippedSpruceLog },  1, Blocks::SprucePlanks,  4 },
        { true,  0, 0, { Blocks::StrippedBirchLog },   1, Blocks::BirchPlanks,   4 },
        { true,  0, 0, { Blocks::StrippedJungleLog },  1, Blocks::JunglePlanks,  4 },
        { true,  0, 0, { Blocks::StrippedAcaciaLog },  1, Blocks::AcaciaPlanks,  4 },
        { true,  0, 0, { Blocks::StrippedDarkOakLog }, 1, Blocks::DarkOakPlanks, 4 },
        { true,  0, 0, { Blocks::CherryLog },          1, Blocks::CherryPlanks,  4 },
        { true,  0, 0, { Blocks::StrippedCherryLog },  1, Blocks::CherryPlanks,  4 },
        { true,  0, 0, { Blocks::PinkPetals1 },        1, Items::DyePink,        1 },
        { true,  0, 0, { Blocks::MangroveLog },         1, Blocks::MangrovePlanks, 4 },
        { true,  0, 0, { Blocks::StrippedMangroveLog }, 1, Blocks::MangrovePlanks, 4 },
        { true,  0, 0, { Blocks::BambooBlock },         1, Blocks::BambooPlanks,   2 },
        { true,  0, 0, { Blocks::StrippedBambooBlock }, 1, Blocks::BambooPlanks,   2 },
        { false, 2, 2, { Blocks::Stone, Blocks::Stone, Blocks::Stone, Blocks::Stone },
          0, Blocks::StoneBricks, 4 },
        { true,  0, 0, { Blocks::StoneBricks, Blocks::Vine }, 2, Blocks::MossyStoneBricks, 1 },
        { false, 3, 1, { Blocks::StoneBricks, Blocks::StoneBricks, Blocks::StoneBricks },
          0, Blocks::StoneBrickSlab, 6 },
        { false, 1, 2, { Blocks::StoneBrickSlab, Blocks::StoneBrickSlab },
          0, Blocks::ChiseledStoneBricks, 1 },
        { false, 3, 1, { Blocks::SmoothStone, Blocks::SmoothStone, Blocks::SmoothStone },
          0, Blocks::SmoothStoneSlab, 6 },
        { false, 2, 2, { Blocks::Granite, Blocks::Granite, Blocks::Granite, Blocks::Granite },
          0, Blocks::PolishedGranite, 4 },
        { false, 2, 2, { Blocks::Diorite, Blocks::Diorite, Blocks::Diorite, Blocks::Diorite },
          0, Blocks::PolishedDiorite, 4 },
        { false, 2, 2, { Blocks::Andesite, Blocks::Andesite, Blocks::Andesite, Blocks::Andesite },
          0, Blocks::PolishedAndesite, 4 },
        { false, 2, 2, { Blocks::CobbledDeepslate, Blocks::CobbledDeepslate, Blocks::CobbledDeepslate, Blocks::CobbledDeepslate },
          0, Blocks::PolishedDeepslate, 4 },
        { false, 2, 2, { Blocks::PolishedDeepslate, Blocks::PolishedDeepslate, Blocks::PolishedDeepslate, Blocks::PolishedDeepslate },
          0, Blocks::DeepslateBricks, 4 },
        { false, 2, 2, { Blocks::DeepslateBricks, Blocks::DeepslateBricks, Blocks::DeepslateBricks, Blocks::DeepslateBricks },
          0, Blocks::DeepslateTiles, 4 },
        { false, 3, 1, { Blocks::CobbledDeepslate, Blocks::CobbledDeepslate, Blocks::CobbledDeepslate },
          0, Blocks::CobbledDeepslateSlab, 6 },
        { false, 1, 2, { Blocks::CobbledDeepslateSlab, Blocks::CobbledDeepslateSlab },
          0, Blocks::ChiseledDeepslate, 1 },
        { false, 2, 2, { Blocks::Tuff, Blocks::Tuff, Blocks::Tuff, Blocks::Tuff },
          0, Blocks::PolishedTuff, 4 },
        { false, 2, 2, { Blocks::PolishedTuff, Blocks::PolishedTuff, Blocks::PolishedTuff, Blocks::PolishedTuff },
          0, Blocks::TuffBricks, 4 },
        { false, 3, 1, { Blocks::Tuff, Blocks::Tuff, Blocks::Tuff },
          0, Blocks::TuffSlab, 6 },
        { false, 1, 2, { Blocks::TuffSlab, Blocks::TuffSlab },
          0, Blocks::ChiseledTuff, 1 },
        { false, 3, 1, { Blocks::TuffBricks, Blocks::TuffBricks, Blocks::TuffBricks },
          0, Blocks::TuffBrickSlab, 6 },
        { false, 1, 2, { Blocks::TuffBrickSlab, Blocks::TuffBrickSlab },
          0, Blocks::ChiseledTuffBricks, 1 },
        { false, 2, 2, { Blocks::Blackstone, Blocks::Blackstone, Blocks::Blackstone, Blocks::Blackstone },
          0, Blocks::PolishedBlackstone, 4 },
        { false, 2, 2, { Blocks::PolishedBlackstone, Blocks::PolishedBlackstone, Blocks::PolishedBlackstone, Blocks::PolishedBlackstone },
          0, Blocks::PolishedBlackstoneBricks, 4 },
        { false, 3, 1, { Blocks::PolishedBlackstone, Blocks::PolishedBlackstone, Blocks::PolishedBlackstone },
          0, Blocks::PolishedBlackstoneSlab, 6 },
        { false, 1, 2, { Blocks::PolishedBlackstoneSlab, Blocks::PolishedBlackstoneSlab },
          0, Blocks::ChiseledPolishedBlackstone, 1 },
        { false, 2, 2, { Items::NetherWart, Items::NetherBrick, Items::NetherBrick, Items::NetherWart },
          0, Blocks::RedNetherBricks, 1 },
        { false, 3, 1, { Blocks::NetherBricks, Blocks::NetherBricks, Blocks::NetherBricks },
          0, Blocks::NetherBrickSlab, 6 },
        { false, 1, 2, { Blocks::NetherBrickSlab, Blocks::NetherBrickSlab },
          0, Blocks::ChiseledNetherBricks, 1 },
        { false, 2, 2, { Items::NetherQuartz, Items::NetherQuartz, Items::NetherQuartz, Items::NetherQuartz },
          0, Blocks::QuartzBlock, 1 },
        { false, 2, 2, { Blocks::QuartzBlock, Blocks::QuartzBlock, Blocks::QuartzBlock, Blocks::QuartzBlock },
          0, Blocks::QuartzBricks, 4 },
        { false, 1, 2, { Blocks::QuartzBlock, Blocks::QuartzBlock },
          0, Blocks::QuartzPillar, 2 },
        { false, 3, 1, { Blocks::QuartzBlock, Blocks::QuartzBlock, Blocks::QuartzBlock },
          0, Blocks::QuartzSlab, 6 },
        { false, 1, 2, { Blocks::QuartzSlab, Blocks::QuartzSlab },
          0, Blocks::ChiseledQuartzBlock, 1 },
        { false, 2, 2, { Blocks::RedSand, Blocks::RedSand, Blocks::RedSand, Blocks::RedSand },
          0, Blocks::RedSandstone, 1 },
        { false, 2, 2, { Blocks::RedSandstone, Blocks::RedSandstone, Blocks::RedSandstone, Blocks::RedSandstone },
          0, Blocks::CutRedSandstone, 4 },
        { false, 3, 1, { Blocks::RedSandstone, Blocks::RedSandstone, Blocks::RedSandstone },
          0, Blocks::RedSandstoneSlab, 6 },
        { false, 1, 2, { Blocks::RedSandstoneSlab, Blocks::RedSandstoneSlab },
          0, Blocks::ChiseledRedSandstone, 1 },
        { false, 2, 2, { Blocks::Sandstone, Blocks::Sandstone, Blocks::Sandstone, Blocks::Sandstone },
          0, Blocks::CutSandstone, 4 },
        { false, 1, 2, { Blocks::SandstoneSlab, Blocks::SandstoneSlab },
          0, Blocks::ChiseledSandstone, 1 },
        { false, 3, 3, { Items::RawIron, Items::RawIron, Items::RawIron, Items::RawIron, Items::RawIron, Items::RawIron, Items::RawIron, Items::RawIron, Items::RawIron },
          0, Blocks::RawIronBlock, 1 },
        { true,  0, 0, { Blocks::RawIronBlock }, 1, Items::RawIron, 9 },
        { false, 3, 3, { Items::RawGold, Items::RawGold, Items::RawGold, Items::RawGold, Items::RawGold, Items::RawGold, Items::RawGold, Items::RawGold, Items::RawGold },
          0, Blocks::RawGoldBlock, 1 },
        { true,  0, 0, { Blocks::RawGoldBlock }, 1, Items::RawGold, 9 },
        { false, 3, 3, { Items::RawCopper, Items::RawCopper, Items::RawCopper, Items::RawCopper, Items::RawCopper, Items::RawCopper, Items::RawCopper, Items::RawCopper, Items::RawCopper },
          0, Blocks::RawCopperBlock, 1 },
        { true,  0, 0, { Blocks::RawCopperBlock }, 1, Items::RawCopper, 9 },
        { true,  0, 0, { Blocks::CrimsonStem },         1, Blocks::CrimsonPlanks,  4 },
        { true,  0, 0, { Blocks::StrippedCrimsonStem }, 1, Blocks::CrimsonPlanks,  4 },
        { true,  0, 0, { Blocks::WarpedStem },          1, Blocks::WarpedPlanks,   4 },
        { true,  0, 0, { Blocks::StrippedWarpedStem },  1, Blocks::WarpedPlanks,   4 },
        { false, 3, 3, { Items::NetherWart, Items::NetherWart, Items::NetherWart,
                         Items::NetherWart, Items::NetherWart, Items::NetherWart,
                         Items::NetherWart, Items::NetherWart, Items::NetherWart },
          0, Blocks::NetherWartBlock, 1 },
        { false, 3, 3, { Blocks::Bamboo, Blocks::Bamboo, Blocks::Bamboo,
                         Blocks::Bamboo, Blocks::Bamboo, Blocks::Bamboo,
                         Blocks::Bamboo, Blocks::Bamboo, Blocks::Bamboo },
          0, Blocks::BambooBlock, 1 },
        { false, 1, 2, { Blocks::BambooSlab,
                         Blocks::BambooSlab },
          0, Blocks::BambooMosaic, 1 },
        { true,  0, 0, { Blocks::Mud, Items::Wheat },   2, Blocks::PackedMud,     1 },
        { true,  0, 0, { Blocks::Mud, Blocks::MangroveRoots }, 2, Blocks::MuddyMangroveRoots, 1 },
        { false, 2, 2, { Blocks::PackedMud, Blocks::PackedMud,
                         Blocks::PackedMud, Blocks::PackedMud },
          0, Blocks::MudBricks, 4 },
        { true,  0, 0, { Blocks::SpruceLog },  1, Blocks::SprucePlanks,  4 },
        { true,  0, 0, { Blocks::DarkOakLog }, 1, Blocks::DarkOakPlanks, 4 },
        { true,  0, 0, { Blocks::JungleLog },  1, Blocks::JunglePlanks,  4 },
        { true,  0, 0, { Blocks::AcaciaLog },  1, Blocks::AcaciaPlanks,  4 },

        // Three wheat becomes a loaf. Minecraft wants them in a row,
        // which needs a bench this game has not got yet, so it takes
        // them in any arrangement for the same three wheat.
        { true,  0, 0, { Items::Wheat, Items::Wheat, Items::Wheat }, 3, Items::Bread, 1 },

        // An eye of ender: a pearl and the powder off a blaze's rod,
        // which is Minecraft's. It used to take gunpowder, because
        // there was no blaze to get the powder from.
        { true,  0, 0, { Items::EnderPearl, Items::BlazePowder }, 2, Items::EyeOfEnder, 1 },

        // The frames. In Minecraft these are only found in
        // strongholds, and there are none; without a recipe the End is
        // somewhere you can only reach from the command line.
        { false, 3, 3, { Blocks::EndStone,  Blocks::EndStone,  Blocks::EndStone,
                         Blocks::EndStone,  Items::EnderPearl, Blocks::EndStone,
                         Blocks::EndStone,  Blocks::EndStone,  Blocks::EndStone },
          0, Blocks::EndPortalFrame, 4 },

        // A shield: planks round an iron boss.
        { false, 3, 3, { Blocks::Planks,    Items::IronIngot,  Blocks::Planks,
                         Blocks::Planks,    Blocks::Planks,    Blocks::Planks,
                         _,                 Blocks::Planks,    _ },
          0, Items::Shield, 1 },

        // A boat: planks round three sides, open at the top.
        { false, 3, 2, { Blocks::Planks,    _,                 Blocks::Planks,
                         Blocks::Planks,    Blocks::Planks,    Blocks::Planks },
          0, Items::Boat, 1 },

        // A minecart: five iron round an open top, the same shape the
        // boat is made in.
        { false, 3, 2, { Items::IronIngot,  _,                 Items::IronIngot,
                         Items::IronIngot,  Items::IronIngot,  Items::IronIngot },
          0, Items::Minecart, 1 },

        // Rails: six iron down the sides with a stick between them.
        { false, 3, 3, { Items::IronIngot,  _,                 Items::IronIngot,
                         Items::IronIngot,  Items::Stick,      Items::IronIngot,
                         Items::IronIngot,  _,                 Items::IronIngot },
          0, Blocks::RailNorthSouth, 16 },

        // Brewing's glassware and the sweetener. One cane makes one
        // sugar; three glass makes three bottles.
        { true,  0, 0, { Blocks::SugarCane },
          1, Items::Sugar, 1 },
        { false, 3, 2, { Blocks::Glass,     _,                 Blocks::Glass,
                         _,                 Blocks::Glass,     _ },
          0, Items::GlassBottle, 3 },

        // A rod ground down. Two powders a rod, which is Minecraft's,
        // and it is the whole supply: every potion in the game is
        // downstream of something a blaze was carrying.
        { true,  0, 0, { Items::BlazeRod },
          1, Items::BlazePowder, 2 },

        // Four nether bricks, which is the only thing to build with
        // down there that is not the ground itself.
        { false, 2, 2, { Items::NetherBrick, Items::NetherBrick,
                         Items::NetherBrick, Items::NetherBrick },
          0, Blocks::NetherBricks, 1 },

        // 1.21's two. A rod makes four charges, which is Minecraft's;
        // the mace wants a heavy core with its rod and there are no
        // vaults here to find one in, so it takes an iron ingot
        // instead -- the one other thing in the game that is nothing
        // but weight.
        { true,  0, 0, { Items::BreezeRod },
          1, Items::WindCharge, 4 },
        { true,  0, 0, { Items::BreezeRod, Items::IronIngot },
          2, Items::Mace, 1 },

        // Copper: nine ingots into a block and back out again, and the
        // bulb, which is Minecraft's -- copper round a blaze rod with
        // redstone behind it.
        { false, 3, 3, { Items::CopperIngot, Items::CopperIngot, Items::CopperIngot,
                         Items::CopperIngot, Items::CopperIngot, Items::CopperIngot,
                         Items::CopperIngot, Items::CopperIngot, Items::CopperIngot },
          0, Blocks::CopperBlock, 1 },
        { true,  0, 0, { Blocks::CopperBlock },
          1, Items::CopperIngot, 9 },
        { false, 2, 2, { Blocks::CopperBlock, Blocks::CopperBlock, Blocks::CopperBlock, Blocks::CopperBlock }, 0, Blocks::CutCopperFirst + 0, 4 },
        { false, 3, 3, { _, Blocks::CopperBlock, _, Blocks::CopperBlock, _, Blocks::CopperBlock, _, Blocks::CopperBlock, _ }, 0, Blocks::CopperGrateFirst + 0, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 0, Blocks::CutCopperFirst + 0, Blocks::CutCopperFirst + 0 }, 0, Blocks::CutCopperSlabFirst + 0, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 0, Blocks::CutCopperSlabFirst + 0 }, 0, Blocks::ChiseledCopperFirst + 0, 1 },
        { false, 3, 3, { _, Blocks::CopperBlock, _, Blocks::CopperBlock, Items::BlazeRod, Blocks::CopperBlock, _, Items::Redstone, _ }, 0, Blocks::CopperBulb, 4 },
        { false, 2, 2, { Blocks::ExposedCopper, Blocks::ExposedCopper, Blocks::ExposedCopper, Blocks::ExposedCopper }, 0, Blocks::CutCopperFirst + 1, 4 },
        { false, 3, 3, { _, Blocks::ExposedCopper, _, Blocks::ExposedCopper, _, Blocks::ExposedCopper, _, Blocks::ExposedCopper, _ }, 0, Blocks::CopperGrateFirst + 1, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 1, Blocks::CutCopperFirst + 1, Blocks::CutCopperFirst + 1 }, 0, Blocks::CutCopperSlabFirst + 1, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 1, Blocks::CutCopperSlabFirst + 1 }, 0, Blocks::ChiseledCopperFirst + 1, 1 },
        { false, 3, 3, { _, Blocks::ExposedCopper, _, Blocks::ExposedCopper, Items::BlazeRod, Blocks::ExposedCopper, _, Items::Redstone, _ }, 0, Blocks::CopperBulbRunFirst + 0, 4 },
        { false, 2, 2, { Blocks::WeatheredCopper, Blocks::WeatheredCopper, Blocks::WeatheredCopper, Blocks::WeatheredCopper }, 0, Blocks::CutCopperFirst + 2, 4 },
        { false, 3, 3, { _, Blocks::WeatheredCopper, _, Blocks::WeatheredCopper, _, Blocks::WeatheredCopper, _, Blocks::WeatheredCopper, _ }, 0, Blocks::CopperGrateFirst + 2, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 2, Blocks::CutCopperFirst + 2, Blocks::CutCopperFirst + 2 }, 0, Blocks::CutCopperSlabFirst + 2, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 2, Blocks::CutCopperSlabFirst + 2 }, 0, Blocks::ChiseledCopperFirst + 2, 1 },
        { false, 3, 3, { _, Blocks::WeatheredCopper, _, Blocks::WeatheredCopper, Items::BlazeRod, Blocks::WeatheredCopper, _, Items::Redstone, _ }, 0, Blocks::CopperBulbRunFirst + 4, 4 },
        { false, 2, 2, { Blocks::OxidizedCopper, Blocks::OxidizedCopper, Blocks::OxidizedCopper, Blocks::OxidizedCopper }, 0, Blocks::CutCopperFirst + 3, 4 },
        { false, 3, 3, { _, Blocks::OxidizedCopper, _, Blocks::OxidizedCopper, _, Blocks::OxidizedCopper, _, Blocks::OxidizedCopper, _ }, 0, Blocks::CopperGrateFirst + 3, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 3, Blocks::CutCopperFirst + 3, Blocks::CutCopperFirst + 3 }, 0, Blocks::CutCopperSlabFirst + 3, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 3, Blocks::CutCopperSlabFirst + 3 }, 0, Blocks::ChiseledCopperFirst + 3, 1 },
        { false, 3, 3, { _, Blocks::OxidizedCopper, _, Blocks::OxidizedCopper, Items::BlazeRod, Blocks::OxidizedCopper, _, Items::Redstone, _ }, 0, Blocks::CopperBulbRunFirst + 8, 4 },
        { false, 2, 2, { Blocks::WaxedCopperFirst + 0, Blocks::WaxedCopperFirst + 0, Blocks::WaxedCopperFirst + 0, Blocks::WaxedCopperFirst + 0 }, 0, Blocks::CutCopperFirst + 4, 4 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 0, _, Blocks::WaxedCopperFirst + 0, _, Blocks::WaxedCopperFirst + 0, _, Blocks::WaxedCopperFirst + 0, _ }, 0, Blocks::CopperGrateFirst + 4, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 4, Blocks::CutCopperFirst + 4, Blocks::CutCopperFirst + 4 }, 0, Blocks::CutCopperSlabFirst + 4, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 4, Blocks::CutCopperSlabFirst + 4 }, 0, Blocks::ChiseledCopperFirst + 4, 1 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 0, _, Blocks::WaxedCopperFirst + 0, Items::BlazeRod, Blocks::WaxedCopperFirst + 0, _, Items::Redstone, _ }, 0, Blocks::CopperBulbRunFirst + 12, 4 },
        { false, 2, 2, { Blocks::WaxedCopperFirst + 1, Blocks::WaxedCopperFirst + 1, Blocks::WaxedCopperFirst + 1, Blocks::WaxedCopperFirst + 1 }, 0, Blocks::CutCopperFirst + 5, 4 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 1, _, Blocks::WaxedCopperFirst + 1, _, Blocks::WaxedCopperFirst + 1, _, Blocks::WaxedCopperFirst + 1, _ }, 0, Blocks::CopperGrateFirst + 5, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 5, Blocks::CutCopperFirst + 5, Blocks::CutCopperFirst + 5 }, 0, Blocks::CutCopperSlabFirst + 5, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 5, Blocks::CutCopperSlabFirst + 5 }, 0, Blocks::ChiseledCopperFirst + 5, 1 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 1, _, Blocks::WaxedCopperFirst + 1, Items::BlazeRod, Blocks::WaxedCopperFirst + 1, _, Items::Redstone, _ }, 0, Blocks::CopperBulbRunFirst + 16, 4 },
        { false, 2, 2, { Blocks::WaxedCopperFirst + 2, Blocks::WaxedCopperFirst + 2, Blocks::WaxedCopperFirst + 2, Blocks::WaxedCopperFirst + 2 }, 0, Blocks::CutCopperFirst + 6, 4 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 2, _, Blocks::WaxedCopperFirst + 2, _, Blocks::WaxedCopperFirst + 2, _, Blocks::WaxedCopperFirst + 2, _ }, 0, Blocks::CopperGrateFirst + 6, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 6, Blocks::CutCopperFirst + 6, Blocks::CutCopperFirst + 6 }, 0, Blocks::CutCopperSlabFirst + 6, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 6, Blocks::CutCopperSlabFirst + 6 }, 0, Blocks::ChiseledCopperFirst + 6, 1 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 2, _, Blocks::WaxedCopperFirst + 2, Items::BlazeRod, Blocks::WaxedCopperFirst + 2, _, Items::Redstone, _ }, 0, Blocks::CopperBulbRunFirst + 20, 4 },
        { false, 2, 2, { Blocks::WaxedCopperFirst + 3, Blocks::WaxedCopperFirst + 3, Blocks::WaxedCopperFirst + 3, Blocks::WaxedCopperFirst + 3 }, 0, Blocks::CutCopperFirst + 7, 4 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 3, _, Blocks::WaxedCopperFirst + 3, _, Blocks::WaxedCopperFirst + 3, _, Blocks::WaxedCopperFirst + 3, _ }, 0, Blocks::CopperGrateFirst + 7, 4 },
        { false, 3, 1, { Blocks::CutCopperFirst + 7, Blocks::CutCopperFirst + 7, Blocks::CutCopperFirst + 7 }, 0, Blocks::CutCopperSlabFirst + 7, 6 },
        { false, 1, 2, { Blocks::CutCopperSlabFirst + 7, Blocks::CutCopperSlabFirst + 7 }, 0, Blocks::ChiseledCopperFirst + 7, 1 },
        { false, 3, 3, { _, Blocks::WaxedCopperFirst + 3, _, Blocks::WaxedCopperFirst + 3, Items::BlazeRod, Blocks::WaxedCopperFirst + 3, _, Items::Redstone, _ }, 0, Blocks::CopperBulbRunFirst + 24, 4 },
        { true, 0, 0, { Blocks::CopperBlock, Items::Honeycomb }, 2, Blocks::WaxedCopperFirst + 0, 1 },
        { true, 0, 0, { Blocks::CutCopperFirst + 0, Items::Honeycomb }, 2, Blocks::CutCopperFirst + 4, 1 },
        { true, 0, 0, { Blocks::ChiseledCopperFirst + 0, Items::Honeycomb }, 2, Blocks::ChiseledCopperFirst + 4, 1 },
        { true, 0, 0, { Blocks::CopperGrateFirst + 0, Items::Honeycomb }, 2, Blocks::CopperGrateFirst + 4, 1 },
        { true, 0, 0, { Blocks::CutCopperSlabFirst + 0, Items::Honeycomb }, 2, Blocks::CutCopperSlabFirst + 4, 1 },
        { true, 0, 0, { Blocks::CopperDoorFirst + 0, Items::Honeycomb }, 2, Blocks::CopperDoorFirst + 64, 1 },
        { true, 0, 0, { Blocks::CopperTrapdoorFirst + 0, Items::Honeycomb }, 2, Blocks::CopperTrapdoorFirst + 64, 1 },
        { true, 0, 0, { Blocks::CopperBulb, Items::Honeycomb }, 2, Blocks::CopperBulbRunFirst + 12, 1 },
        { true, 0, 0, { Blocks::ExposedCopper, Items::Honeycomb }, 2, Blocks::WaxedCopperFirst + 1, 1 },
        { true, 0, 0, { Blocks::CutCopperFirst + 1, Items::Honeycomb }, 2, Blocks::CutCopperFirst + 5, 1 },
        { true, 0, 0, { Blocks::ChiseledCopperFirst + 1, Items::Honeycomb }, 2, Blocks::ChiseledCopperFirst + 5, 1 },
        { true, 0, 0, { Blocks::CopperGrateFirst + 1, Items::Honeycomb }, 2, Blocks::CopperGrateFirst + 5, 1 },
        { true, 0, 0, { Blocks::CutCopperSlabFirst + 1, Items::Honeycomb }, 2, Blocks::CutCopperSlabFirst + 5, 1 },
        { true, 0, 0, { Blocks::CopperDoorFirst + 16, Items::Honeycomb }, 2, Blocks::CopperDoorFirst + 80, 1 },
        { true, 0, 0, { Blocks::CopperTrapdoorFirst + 16, Items::Honeycomb }, 2, Blocks::CopperTrapdoorFirst + 80, 1 },
        { true, 0, 0, { Blocks::CopperBulbRunFirst + 0, Items::Honeycomb }, 2, Blocks::CopperBulbRunFirst + 16, 1 },
        { true, 0, 0, { Blocks::WeatheredCopper, Items::Honeycomb }, 2, Blocks::WaxedCopperFirst + 2, 1 },
        { true, 0, 0, { Blocks::CutCopperFirst + 2, Items::Honeycomb }, 2, Blocks::CutCopperFirst + 6, 1 },
        { true, 0, 0, { Blocks::ChiseledCopperFirst + 2, Items::Honeycomb }, 2, Blocks::ChiseledCopperFirst + 6, 1 },
        { true, 0, 0, { Blocks::CopperGrateFirst + 2, Items::Honeycomb }, 2, Blocks::CopperGrateFirst + 6, 1 },
        { true, 0, 0, { Blocks::CutCopperSlabFirst + 2, Items::Honeycomb }, 2, Blocks::CutCopperSlabFirst + 6, 1 },
        { true, 0, 0, { Blocks::CopperDoorFirst + 32, Items::Honeycomb }, 2, Blocks::CopperDoorFirst + 96, 1 },
        { true, 0, 0, { Blocks::CopperTrapdoorFirst + 32, Items::Honeycomb }, 2, Blocks::CopperTrapdoorFirst + 96, 1 },
        { true, 0, 0, { Blocks::CopperBulbRunFirst + 4, Items::Honeycomb }, 2, Blocks::CopperBulbRunFirst + 20, 1 },
        { true, 0, 0, { Blocks::OxidizedCopper, Items::Honeycomb }, 2, Blocks::WaxedCopperFirst + 3, 1 },
        { true, 0, 0, { Blocks::CutCopperFirst + 3, Items::Honeycomb }, 2, Blocks::CutCopperFirst + 7, 1 },
        { true, 0, 0, { Blocks::ChiseledCopperFirst + 3, Items::Honeycomb }, 2, Blocks::ChiseledCopperFirst + 7, 1 },
        { true, 0, 0, { Blocks::CopperGrateFirst + 3, Items::Honeycomb }, 2, Blocks::CopperGrateFirst + 7, 1 },
        { true, 0, 0, { Blocks::CutCopperSlabFirst + 3, Items::Honeycomb }, 2, Blocks::CutCopperSlabFirst + 7, 1 },
        { true, 0, 0, { Blocks::CopperDoorFirst + 48, Items::Honeycomb }, 2, Blocks::CopperDoorFirst + 112, 1 },
        { true, 0, 0, { Blocks::CopperTrapdoorFirst + 48, Items::Honeycomb }, 2, Blocks::CopperTrapdoorFirst + 112, 1 },
        { true, 0, 0, { Blocks::CopperBulbRunFirst + 8, Items::Honeycomb }, 2, Blocks::CopperBulbRunFirst + 24, 1 },
        { false, 2, 3, { Items::CopperIngot, Items::CopperIngot, Items::CopperIngot, Items::CopperIngot,
                         Items::CopperIngot, Items::CopperIngot }, 0, Blocks::CopperDoorFirst, 3 },
        { false, 2, 2, { Items::CopperIngot, Items::CopperIngot, Items::CopperIngot, Items::CopperIngot },
          0, Blocks::CopperTrapdoorFirst, 1 },
        { false, 1, 3, { Items::CopperIngot, Items::CopperIngot, Items::CopperIngot }, 0, Blocks::LightningRodFirst, 1 },
        { true, 0, 0, { Blocks::WaxedCopperFirst }, 1, Items::CopperIngot, 9 },

        // The crafter: a bench that works on its own. Minecraft wants a
        // dropper with the iron and the redstone, and there are none
        // here, so it takes a piston -- the one other thing in the game
        // whose whole job is moving something along when it is told to.
        { false, 3, 3, { Items::IronIngot, Items::IronIngot,     Items::IronIngot,
                         Items::IronIngot, Blocks::CraftingTable, Items::IronIngot,
                         Items::Redstone,  Blocks::PistonNorth,   Items::Redstone },
          0, Blocks::Crafter, 1 },

        // A piston: planks over cobble with an iron plate and the dust
        // that drives it, which is Minecraft's. Sticky wants a
        // slimeball and there are no slimes here, so it takes the
        // magma cream off a magma cube -- the one other thing in the
        // game that is sticky at all.
        { false, 3, 3, { Blocks::Planks,      Blocks::Planks,    Blocks::Planks,
                         Blocks::Cobblestone, Items::IronIngot,  Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Redstone,   Blocks::Cobblestone },
          0, Blocks::PistonNorth, 1 },
        { true,  0, 0, { Blocks::PistonNorth, Items::MagmaCream },
          2, Blocks::StickyPistonNorth, 1 },

        // Redstone. A torch is dust on a stick, a lever is a stick on
        // a cobble, a button is one stone, a lamp is glowstone wrapped
        // in dust, and nine dust packs into a block -- all Minecraft's,
        // except that the lever takes the cobble beside the stick
        // rather than under it, which this grid cannot tell apart.
        { false, 1, 2, { Items::Redstone,
                         Items::Stick },
          0, Blocks::RedstoneTorch, 1 },
        { true,  0, 0, { Items::Stick, Blocks::Cobblestone },
          2, Blocks::LeverOff, 1 },
        { true,  0, 0, { Blocks::Stone },
          1, Blocks::ButtonOff, 1 },
        { false, 3, 3, { _,                 Items::Redstone,   _,
                         Items::Redstone,   Blocks::Glowstone, Items::Redstone,
                         _,                 Items::Redstone,   _ },
          0, Blocks::RedstoneLamp, 1 },
        { false, 3, 3, { Items::Redstone,   Items::Redstone,   Items::Redstone,
                         Items::Redstone,   Items::Redstone,   Items::Redstone,
                         Items::Redstone,   Items::Redstone,   Items::Redstone },
          0, Blocks::RedstoneBlock, 1 },
        { true,  0, 0, { Blocks::RedstoneBlock },
          1, Items::Redstone, 9 },

        // The stand: a blaze powder sat on three cobblestone.
        { false, 3, 2, { _,                 Items::BlazePowder, _,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::BrewingStand, 1 },

        // Seven iron: a slab across the top, a stem, and a foot.
        // Minecraft spends three iron blocks on the top row, but there
        // is no iron block here to spend, so it is ingots all the way.
        { false, 3, 3, { Items::IronIngot,  Items::IronIngot,  Items::IronIngot,
                         _,                 Items::IronIngot,  _,
                         Items::IronIngot,  Items::IronIngot,  Items::IronIngot },
          0, Blocks::Anvil, 1 },

        // Four obsidian, two diamond and a book. The book is why the
        // whole cane chain above has to exist.
        { false, 3, 3, { _,                 Items::Book,       _,
                         Items::Diamond,    Blocks::Obsidian,  Items::Diamond,
                         Blocks::Obsidian,  Blocks::Obsidian,  Blocks::Obsidian },
          0, Blocks::EnchantingTable, 1 },

        // Nine of a thing into one block, and the one block back
        // into nine. Minecraft's oldest way of tidying a chest, and
        // what a beacon's pyramid is built out of.
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot },
          0, Blocks::IronBlock, 1 },
        { true, 0, 0, { Blocks::IronBlock }, 1, Items::IronIngot, 9 },
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot,
                         Items::GoldIngot, Items::GoldIngot, Items::GoldIngot,
                         Items::GoldIngot, Items::GoldIngot, Items::GoldIngot },
          0, Blocks::GoldBlock, 1 },
        { true, 0, 0, { Blocks::GoldBlock }, 1, Items::GoldIngot, 9 },
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond,
                         Items::Diamond, Items::Diamond, Items::Diamond,
                         Items::Diamond, Items::Diamond, Items::Diamond },
          0, Blocks::DiamondBlock, 1 },
        { true, 0, 0, { Blocks::DiamondBlock }, 1, Items::Diamond, 9 },
        { false, 3, 3, { Items::Emerald, Items::Emerald, Items::Emerald,
                         Items::Emerald, Items::Emerald, Items::Emerald,
                         Items::Emerald, Items::Emerald, Items::Emerald },
          0, Blocks::EmeraldBlock, 1 },
        { true, 0, 0, { Blocks::EmeraldBlock }, 1, Items::Emerald, 9 },
        { false, 3, 3, { Items::Coal, Items::Coal, Items::Coal,
                         Items::Coal, Items::Coal, Items::Coal,
                         Items::Coal, Items::Coal, Items::Coal },
          0, Blocks::CoalBlock, 1 },
        { true, 0, 0, { Blocks::CoalBlock }, 1, Items::Coal, 9 },

        // A lantern: iron round a torch. A barrel: planks and slabs,
        // which is Minecraft's and the cheapest chest in the game. A
        // campfire: logs under a stick frame, with coal to light it.
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Blocks::Torch,    Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot },
          0, Blocks::Lantern, 1 },

        { false, 3, 3, { Blocks::Planks,   Blocks::PlankSlab, Blocks::Planks,
                         Blocks::Planks,   Blocks::Air,       Blocks::Planks,
                         Blocks::Planks,   Blocks::PlankSlab, Blocks::Planks },
          0, Blocks::Barrel, 1 },

        { false, 3, 3, { Blocks::Air,    Items::Stick,  Blocks::Air,
                         Items::Stick,   Items::Coal,   Items::Stick,
                         Blocks::Log,    Blocks::Log,   Blocks::Log },
          0, Blocks::Campfire, 1 },

        // A bone ground up makes three meal, which is Minecraft's.
        { true, 0, 0, { Items::Bone }, 1, Items::BoneMeal, 3 },

        // Three planks in a V. The bowl is the only container in the
        // game that is not a block, and the only way to carry a soup.
        { false, 3, 3, { Blocks::Planks, Blocks::Air,    Blocks::Planks,
                         Blocks::Air,    Blocks::Planks, Blocks::Air,
                         Blocks::Air,    Blocks::Air,    Blocks::Air },
          0, Items::Bowl, 4 },

        // Six beets in a bowl, which is Minecraft's and is the one
        // thing a beetroot is for.
        { false, 3, 3, { Items::Beetroot, Items::Beetroot, Items::Beetroot,
                         Items::Beetroot, Items::Beetroot, Items::Beetroot,
                         Blocks::Air,     Items::Bowl,     Blocks::Air },
          0, Items::BeetrootSoup, 1 },

        // Nine slices make a melon back, and one slice makes the seed
        // that grows another. Both are Minecraft's.
        { false, 3, 3, { Items::MelonSlice, Items::MelonSlice, Items::MelonSlice,
                         Items::MelonSlice, Items::MelonSlice, Items::MelonSlice,
                         Items::MelonSlice, Items::MelonSlice, Items::MelonSlice },
          0, Blocks::Melon, 1 },

        { true, 0, 0, { Items::MelonSlice }, 1, Items::MelonSeeds, 1 },
        { true, 0, 0, { Blocks::Pumpkin }, 1, Items::PumpkinSeeds, 4 },
        { false, 3, 3, { Blocks::Ice, Blocks::Ice, Blocks::Ice, Blocks::Ice, Blocks::Ice,
                         Blocks::Ice, Blocks::Ice, Blocks::Ice, Blocks::Ice }, 0, Blocks::PackedIce, 1 },
        { false, 3, 3, { Blocks::PackedIce, Blocks::PackedIce, Blocks::PackedIce, Blocks::PackedIce, Blocks::PackedIce,
                         Blocks::PackedIce, Blocks::PackedIce, Blocks::PackedIce, Blocks::PackedIce }, 0, Blocks::BlueIce, 1 },
        { false, 3, 3, { Items::NautilusShell, Items::NautilusShell, Items::NautilusShell,
                         Items::NautilusShell, Items::HeartOfTheSea, Items::NautilusShell,
                         Items::NautilusShell, Items::NautilusShell, Items::NautilusShell }, 0, Blocks::Conduit, 1 },
        { false, 3, 2, { Items::TurtleScute, Items::TurtleScute, Items::TurtleScute,
                         Items::TurtleScute, Blocks::Air, Items::TurtleScute }, 0, Items::TurtleHelmet, 1 },
        { false, 1, 2, { Blocks::CarvedPumpkinFirst + 2, Blocks::Torch }, 0, Blocks::JackOLanternFirst + 2, 1 },
        { false, 2, 2, { Blocks::Air, Items::IronIngot, Items::IronIngot, Blocks::Air }, 0, Items::Shears, 1 },
        { false, 3, 3, { Items::Wheat, Items::Wheat, Items::Wheat, Items::Wheat, Items::Wheat, Items::Wheat,
                         Items::Wheat, Items::Wheat, Items::Wheat }, 0, Blocks::HayBale, 1 },
        { true, 0, 0, { Blocks::HayBale }, 1, Items::Wheat, 9 },
        { false, 3, 3, { Items::BoneMeal, Items::BoneMeal, Items::BoneMeal, Items::BoneMeal, Items::BoneMeal,
                         Items::BoneMeal, Items::BoneMeal, Items::BoneMeal, Items::BoneMeal }, 0, Blocks::BoneBlock, 1 },
        { true, 0, 0, { Blocks::BoneBlock }, 1, Items::BoneMeal, 9 },
        { false, 3, 3, { Items::DriedKelp, Items::DriedKelp, Items::DriedKelp, Items::DriedKelp, Items::DriedKelp,
                         Items::DriedKelp, Items::DriedKelp, Items::DriedKelp, Items::DriedKelp }, 0, Blocks::DriedKelpBlock, 1 },
        { true, 0, 0, { Blocks::DriedKelpBlock }, 1, Items::DriedKelp, 9 },
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Items::Honeycomb, Items::Honeycomb, Items::Honeycomb,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks }, 0, Blocks::BeehiveFirst + 12, 1 },
        { false, 2, 2, { Items::HoneyBottle, Items::HoneyBottle, Items::HoneyBottle, Items::HoneyBottle }, 0, Blocks::HoneyBlock, 1 },
        { true, 0, 0, { Blocks::HoneyBlock, Items::GlassBottle, Items::GlassBottle, Items::GlassBottle, Items::GlassBottle },
          5, Items::HoneyBottle, 4 },
        { true, 0, 0, { Items::HoneyBottle }, 1, Items::Sugar, 3 },
        { false, 2, 2, { Items::Honeycomb, Items::Honeycomb, Items::Honeycomb, Items::Honeycomb }, 0, Blocks::HoneycombBlock, 1 },

        // Six planks and a stick make three signs, which is
        // Minecraft's and the cheapest way to label anything.
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Blocks::Air,    Items::Stick,   Blocks::Air },
          0, Items::Sign, 3 },

        // Four scrap and four gold make an ingot, which is
        // Minecraft's and is why netherite costs a trip rather than a
        // dig. Nine of those make the block, and the block gives them
        // back, like every other metal.
        { false, 3, 3, { Items::NetheriteScrap, Items::NetheriteScrap, Items::NetheriteScrap,
                         Items::NetheriteScrap, Items::GoldIngot,      Items::GoldIngot,
                         Items::GoldIngot,      Items::GoldIngot,      Blocks::Air },
          0, Items::NetheriteIngot, 1 },

        { false, 3, 3, { Items::NetheriteIngot, Items::NetheriteIngot, Items::NetheriteIngot,
                         Items::NetheriteIngot, Items::NetheriteIngot, Items::NetheriteIngot,
                         Items::NetheriteIngot, Items::NetheriteIngot, Items::NetheriteIngot },
          0, Blocks::NetheriteBlock, 1 },
        { true, 0, 0, { Blocks::NetheriteBlock }, 1, Items::NetheriteIngot, 9 },

        // --- the sixteen colours ---------------------------------
        //
        // Where each dye comes from. Minecraft's sources, with the
        // two it has no flower for here taken from the ore and the
        // squid instead -- which is also Minecraft's, just the other
        // half of it.
        { true, 0, 0, { Items::BoneMeal },    1, Items::DyeWhite,  1 },
        { true, 0, 0, { Blocks::FlowerYellow }, 1, Items::DyeYellow, 1 },
        { true, 0, 0, { Blocks::FlowerRed },  1, Items::DyeRed,    1 },
        { true, 0, 0, { Blocks::Allium }, 1, Items::DyeMagenta, 1 },
        { true, 0, 0, { Blocks::AzureBluet }, 1, Items::DyeLightGray, 1 },
        { true, 0, 0, { Blocks::BlueOrchid }, 1, Items::DyeLightBlue, 1 },
        { true, 0, 0, { Blocks::Cornflower }, 1, Items::DyeBlue, 1 },
        { true, 0, 0, { Blocks::LilyOfTheValley }, 1, Items::DyeWhite, 1 },
        { true, 0, 0, { Blocks::OxeyeDaisy }, 1, Items::DyeLightGray, 1 },
        { true, 0, 0, { Blocks::RedTulip }, 1, Items::DyeRed, 1 },
        { true, 0, 0, { Blocks::OrangeTulip }, 1, Items::DyeOrange, 1 },
        { true, 0, 0, { Blocks::WhiteTulip }, 1, Items::DyeLightGray, 1 },
        { true, 0, 0, { Blocks::PinkTulip }, 1, Items::DyePink, 1 },
        { true, 0, 0, { Blocks::WitherRose }, 1, Items::DyeBlack, 1 },
        { true, 0, 0, { Blocks::Torchflower }, 1, Items::DyeOrange, 1 },
        { true, 0, 0, { Blocks::DoublePlantFirst + 0 }, 1, Items::DyeYellow, 2 },
        { true, 0, 0, { Blocks::DoublePlantFirst + 2 }, 1, Items::DyeMagenta, 2 },
        { true, 0, 0, { Blocks::DoublePlantFirst + 4 }, 1, Items::DyeRed, 2 },
        { true, 0, 0, { Blocks::DoublePlantFirst + 6 }, 1, Items::DyePink, 2 },
        { true, 0, 0, { Blocks::DoublePlantFirst + 12 }, 1, Items::DyeCyan, 2 },
        { false, 3, 2, { Items::Brick, _, Items::Brick, _, Items::Brick, _ }, 0, Blocks::FlowerPot, 1 },
        { false, 2, 2, { Items::Brick, Items::Brick, Items::Brick, Items::Brick }, 0, Blocks::Bricks, 1 },
        { false, 2, 2, { Items::ClayBall, Items::ClayBall, Items::ClayBall, Items::ClayBall }, 0, Blocks::Clay, 1 },
        { false, 2, 2, { Blocks::Dirt, Blocks::Gravel, Blocks::Gravel, Blocks::Dirt }, 0, Blocks::CoarseDirt, 4 },
        { false, 2, 1, { Blocks::Moss, Blocks::Moss }, 0, Blocks::MossCarpet, 3 },
        { true, 0, 0, { Items::LapisLazuli }, 1, Items::DyeBlue,   1 },
        { true, 0, 0, { Items::InkSac },      1, Items::DyeBlack,  1 },
        { true, 0, 0, { Items::CocoaBeans },  1, Items::DyeBrown,  1 },

        // And the ones that are made by mixing, which is the whole of
        // why there are sixteen rather than six.
        { true, 0, 0, { Items::DyeRed, Items::DyeYellow },  2, Items::DyeOrange,    2 },
        { true, 0, 0, { Items::DyeRed, Items::DyeBlue },    2, Items::DyePurple,    2 },
        { true, 0, 0, { Items::DyeRed, Items::DyeWhite },   2, Items::DyePink,      2 },
        { true, 0, 0, { Items::DyeBlue, Items::DyeGreen },  2, Items::DyeCyan,      2 },
        { true, 0, 0, { Items::DyeBlue, Items::DyeWhite },  2, Items::DyeLightBlue, 2 },
        { true, 0, 0, { Items::DyeGreen, Items::DyeWhite }, 2, Items::DyeLime,      2 },
        { true, 0, 0, { Items::DyeBlack, Items::DyeWhite }, 2, Items::DyeGray,      2 },
        { true, 0, 0, { Items::DyeGray, Items::DyeWhite },  2, Items::DyeLightGray, 2 },
        { true, 0, 0, { Items::DyePurple, Items::DyePink }, 2, Items::DyeMagenta,   2 },

        // Green is the odd one: it is a cactus out of a furnace
        // rather than anything mixed, which is Minecraft's.

        // Nine lapis to a block and back, like every other mineral.
        { false, 3, 3, { Items::LapisLazuli, Items::LapisLazuli, Items::LapisLazuli,
                         Items::LapisLazuli, Items::LapisLazuli, Items::LapisLazuli,
                         Items::LapisLazuli, Items::LapisLazuli, Items::LapisLazuli },
          0, Blocks::LapisBlock, 1 },
        { true, 0, 0, { Blocks::LapisBlock }, 1, Items::LapisLazuli, 9 },

        // Slime. Nine balls to a block and back, Minecraft's own
        // sticky piston rather than the magma-cream stand-in it had
        // while there were no slimes, and the cream itself.
        { false, 3, 3, { Items::Slimeball, Items::Slimeball, Items::Slimeball,
                         Items::Slimeball, Items::Slimeball, Items::Slimeball,
                         Items::Slimeball, Items::Slimeball, Items::Slimeball },
          0, Blocks::SlimeBlock, 1 },
        { true, 0, 0, { Blocks::SlimeBlock }, 1, Items::Slimeball, 9 },
        { false, 1, 3, { Items::IronIngot, Items::Stick, Blocks::Planks }, 0, Items::TripwireHook, 2 },
        { true, 0, 0, { Blocks::Chest, Items::TripwireHook }, 2, Blocks::ChestRunFirst + 17, 1 },
        { false, 3, 3, { Blocks::Obsidian, Blocks::Obsidian, Blocks::Obsidian,
                         Blocks::Obsidian, Items::EyeOfEnder, Blocks::Obsidian,
                         Blocks::Obsidian, Blocks::Obsidian, Blocks::Obsidian },
          0, Blocks::ChestRunFirst + 37, 1 },
        { false, 3, 2, { Blocks::Glass, Blocks::Glass, Blocks::Glass,
                         Blocks::Glass, Blocks::Glass, Blocks::Glass },
          0, Blocks::GlassPane, 16 },
        { false, 3, 3, { _, Items::AmethystShard, _,
                         Items::AmethystShard, Blocks::Glass, Items::AmethystShard,
                         _, Items::AmethystShard, _ },
          0, Blocks::TintedGlass, 2 },
        { false, 2, 2, { Items::AmethystShard, Items::AmethystShard,
                         Items::AmethystShard, Items::AmethystShard },
          0, Blocks::AmethystBlock, 1 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 0, Blocks::StainedGlassFirst + 0, Blocks::StainedGlassFirst + 0,
                         Blocks::StainedGlassFirst + 0, Blocks::StainedGlassFirst + 0, Blocks::StainedGlassFirst + 0 },
          0, Blocks::StainedGlassPaneFirst + 0, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 1, Blocks::StainedGlassFirst + 1, Blocks::StainedGlassFirst + 1,
                         Blocks::StainedGlassFirst + 1, Blocks::StainedGlassFirst + 1, Blocks::StainedGlassFirst + 1 },
          0, Blocks::StainedGlassPaneFirst + 1, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 2, Blocks::StainedGlassFirst + 2, Blocks::StainedGlassFirst + 2,
                         Blocks::StainedGlassFirst + 2, Blocks::StainedGlassFirst + 2, Blocks::StainedGlassFirst + 2 },
          0, Blocks::StainedGlassPaneFirst + 2, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 3, Blocks::StainedGlassFirst + 3, Blocks::StainedGlassFirst + 3,
                         Blocks::StainedGlassFirst + 3, Blocks::StainedGlassFirst + 3, Blocks::StainedGlassFirst + 3 },
          0, Blocks::StainedGlassPaneFirst + 3, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 4, Blocks::StainedGlassFirst + 4, Blocks::StainedGlassFirst + 4,
                         Blocks::StainedGlassFirst + 4, Blocks::StainedGlassFirst + 4, Blocks::StainedGlassFirst + 4 },
          0, Blocks::StainedGlassPaneFirst + 4, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 5, Blocks::StainedGlassFirst + 5, Blocks::StainedGlassFirst + 5,
                         Blocks::StainedGlassFirst + 5, Blocks::StainedGlassFirst + 5, Blocks::StainedGlassFirst + 5 },
          0, Blocks::StainedGlassPaneFirst + 5, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 6, Blocks::StainedGlassFirst + 6, Blocks::StainedGlassFirst + 6,
                         Blocks::StainedGlassFirst + 6, Blocks::StainedGlassFirst + 6, Blocks::StainedGlassFirst + 6 },
          0, Blocks::StainedGlassPaneFirst + 6, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 7, Blocks::StainedGlassFirst + 7, Blocks::StainedGlassFirst + 7,
                         Blocks::StainedGlassFirst + 7, Blocks::StainedGlassFirst + 7, Blocks::StainedGlassFirst + 7 },
          0, Blocks::StainedGlassPaneFirst + 7, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 8, Blocks::StainedGlassFirst + 8, Blocks::StainedGlassFirst + 8,
                         Blocks::StainedGlassFirst + 8, Blocks::StainedGlassFirst + 8, Blocks::StainedGlassFirst + 8 },
          0, Blocks::StainedGlassPaneFirst + 8, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 9, Blocks::StainedGlassFirst + 9, Blocks::StainedGlassFirst + 9,
                         Blocks::StainedGlassFirst + 9, Blocks::StainedGlassFirst + 9, Blocks::StainedGlassFirst + 9 },
          0, Blocks::StainedGlassPaneFirst + 9, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 10, Blocks::StainedGlassFirst + 10, Blocks::StainedGlassFirst + 10,
                         Blocks::StainedGlassFirst + 10, Blocks::StainedGlassFirst + 10, Blocks::StainedGlassFirst + 10 },
          0, Blocks::StainedGlassPaneFirst + 10, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 11, Blocks::StainedGlassFirst + 11, Blocks::StainedGlassFirst + 11,
                         Blocks::StainedGlassFirst + 11, Blocks::StainedGlassFirst + 11, Blocks::StainedGlassFirst + 11 },
          0, Blocks::StainedGlassPaneFirst + 11, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 12, Blocks::StainedGlassFirst + 12, Blocks::StainedGlassFirst + 12,
                         Blocks::StainedGlassFirst + 12, Blocks::StainedGlassFirst + 12, Blocks::StainedGlassFirst + 12 },
          0, Blocks::StainedGlassPaneFirst + 12, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 13, Blocks::StainedGlassFirst + 13, Blocks::StainedGlassFirst + 13,
                         Blocks::StainedGlassFirst + 13, Blocks::StainedGlassFirst + 13, Blocks::StainedGlassFirst + 13 },
          0, Blocks::StainedGlassPaneFirst + 13, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 14, Blocks::StainedGlassFirst + 14, Blocks::StainedGlassFirst + 14,
                         Blocks::StainedGlassFirst + 14, Blocks::StainedGlassFirst + 14, Blocks::StainedGlassFirst + 14 },
          0, Blocks::StainedGlassPaneFirst + 14, 16 },
        { false, 3, 2, { Blocks::StainedGlassFirst + 15, Blocks::StainedGlassFirst + 15, Blocks::StainedGlassFirst + 15,
                         Blocks::StainedGlassFirst + 15, Blocks::StainedGlassFirst + 15, Blocks::StainedGlassFirst + 15 },
          0, Blocks::StainedGlassPaneFirst + 15, 16 },
        { true, 0, 0, { Blocks::PistonNorth, Items::Slimeball },
          2, Blocks::StickyPistonNorth, 1 },
        { true, 0, 0, { Items::Slimeball, Items::BlazePowder },
          2, Items::MagmaCream, 1 },

        // Nine nuggets to an ingot and back. The only thing in the
        // game that turns what a mob leaves into a metal.
        { false, 3, 3, { Items::GoldNugget, Items::GoldNugget, Items::GoldNugget,
                         Items::GoldNugget, Items::GoldNugget, Items::GoldNugget,
                         Items::GoldNugget, Items::GoldNugget, Items::GoldNugget },
          0, Items::GoldIngot, 1 },
        { true, 0, 0, { Items::GoldIngot }, 1, Items::GoldNugget, 9 },

        // Four snowballs pack back into the block they came off.
        { false, 2, 2, { Items::Snowball, Items::Snowball,
                         Items::Snowball, Items::Snowball },
          0, Blocks::Snow, 1 },

        // Cobblestone and a vine. Minecraft's recipe, and the only
        // way to mossy cobble that does not involve finding a village
        // built in a swamp -- which is where the one in this world
        // was, and the block audit said so.
        { true, 0, 0, { Blocks::Cobblestone, Blocks::Vine }, 2,
          Blocks::MossyCobblestone, 1 },

        // Two shells round a chest, which is Minecraft's.
        { false, 1, 3, { Items::ShulkerShell,
                         Blocks::Chest,
                         Items::ShulkerShell }, 0, Blocks::ShulkerBox, 1 },

        // Sticks round a hide, which is Minecraft's. A frame is the
        // other way to label a room: a sign says what is in the chest
        // and a frame shows it.
        { false, 3, 3, { Items::Stick,   Items::Stick,   Items::Stick,
                         Items::Stick,   Items::Leather, Items::Stick,
                         Items::Stick,   Items::Stick,   Items::Stick },
          0, Items::ItemFrame, 1 },

        // A cake: milk along the top, an egg between two sugars, and
        // wheat under it. Minecraft's, buckets and all -- and the
        // buckets come back, which is handled where the grid is
        // emptied rather than here.
        { false, 3, 3, { Items::MilkBucket, Items::MilkBucket, Items::MilkBucket,
                         Items::Sugar,      Items::Egg,        Items::Sugar,
                         Items::Wheat,      Items::Wheat,      Items::Wheat },
          0, Items::Cake, 1 },

        // Two wheat round a bean makes eight cookies, which is
        // Minecraft's and the reason cocoa is worth climbing for.
        { false, 3, 3, { Items::Wheat, Items::CocoaBeans, Items::Wheat,
                         Blocks::Air,  Blocks::Air,       Blocks::Air,
                         Blocks::Air,  Blocks::Air,       Blocks::Air },
          0, Items::Cookie, 8 },

        // Six canes and a string make six scaffolding, which is
        // Minecraft's and the only thing bamboo is really for.
        { false, 3, 3, { Blocks::Bamboo, Items::StringItem, Blocks::Bamboo,
                         Blocks::Bamboo, Blocks::Air,       Blocks::Bamboo,
                         Blocks::Bamboo, Blocks::Air,       Blocks::Bamboo },
          0, Blocks::Scaffolding, 6 },

        // A wick in wax. Minecraft's own recipe, now that there are
        // bees to take the wax off: it used to stand on wool instead,
        // which was at least white and the same shape.
        { false, 1, 2, { Items::StringItem,
                         Items::Honeycomb }, 0, Blocks::Candle, 1 },

        // The workstations. A smoker is a furnace with logs built
        // round it; a blast furnace is one with iron over it and
        // smooth stone under. Both are Minecraft's, and both are the
        // same trade: half the diet for twice the speed.
        { false, 3, 3, { Blocks::Air,  Blocks::Log,     Blocks::Air,
                         Blocks::Log,  Blocks::Furnace, Blocks::Log,
                         Blocks::Air,  Blocks::Log,     Blocks::Air },
          0, Blocks::Smoker, 1 },

        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot,
                         Items::IronIngot, Blocks::Furnace,  Items::IronIngot,
                         Blocks::Stone,    Blocks::Stone,    Blocks::Stone },
          0, Blocks::BlastFurnace, 1 },

        // A composter is a slatted box: seven slabs with the top open.
        { false, 3, 3, { Blocks::PlankSlab, Blocks::Air,       Blocks::PlankSlab,
                         Blocks::PlankSlab, Blocks::Air,       Blocks::PlankSlab,
                         Blocks::PlankSlab, Blocks::PlankSlab, Blocks::PlankSlab },
          0, Blocks::Composter0, 1 },

        // A grindstone is a wheel slung between two posts.
        { false, 3, 3, { Items::Stick,     Blocks::StoneSlab, Items::Stick,
                         Blocks::Planks,   Blocks::Air,       Blocks::Planks,
                         Blocks::Air,      Blocks::Air,       Blocks::Air },
          0, Blocks::Grindstone, 1 },

        // A lectern is a bookshelf on a stand, which is what it is.
        { false, 3, 3, { Blocks::PlankSlab, Blocks::PlankSlab, Blocks::PlankSlab,
                         Blocks::Air,       Blocks::Bookshelf, Blocks::Air,
                         Blocks::Air,       Blocks::PlankSlab, Blocks::Air },
          0, Blocks::Lectern, 1 },

        // A cauldron is seven iron round an open top.
        { false, 3, 3, { Items::IronIngot, Blocks::Air,      Items::IronIngot,
                         Items::IronIngot, Blocks::Air,      Items::IronIngot,
                         Items::IronIngot, Items::IronIngot, Items::IronIngot },
          0, Blocks::Cauldron0, 1 },

        // A hopper: iron round a chest, which is Minecraft's. A
        // dropper is cobble round nothing with redstone behind it,
        // and a dispenser is a dropper with a bow in it.
        { false, 3, 3, { Items::IronIngot,    Blocks::Air,     Items::IronIngot,
                         Items::IronIngot,    Blocks::Chest,   Items::IronIngot,
                         Blocks::Air,         Items::IronIngot, Blocks::Air },
          0, Blocks::Hopper, 1 },

        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Blocks::Cobblestone, Blocks::Air,         Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Redstone,     Blocks::Cobblestone },
          0, Blocks::Dropper, 1 },

        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Bow,          Blocks::Cobblestone,
                         Blocks::Cobblestone, Items::Redstone,     Blocks::Cobblestone },
          0, Blocks::Dispenser, 1 },

        // A note block: planks round redstone. An observer: cobble
        // and redstone round a lump of quartz -- there is no quartz
        // here, so a diamond stands in as the one thing precious
        // enough to make it feel earned.
        { false, 3, 3, { Blocks::Planks, Blocks::Planks,  Blocks::Planks,
                         Blocks::Planks, Items::Redstone, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks,  Blocks::Planks },
          0, Blocks::NoteBlock, 1 },

        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Items::Redstone,     Items::Redstone,     Items::Diamond,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::Observer, 1 },

        // The beacon: glass over the star, obsidian under it. The one
        // recipe in the game you can only make once you have killed
        // something, which is the whole point of a nether star.
        { false, 3, 3, { Blocks::Glass,    Blocks::Glass,      Blocks::Glass,
                         Blocks::Glass,    Items::NetherStar,  Blocks::Glass,
                         Blocks::Obsidian, Blocks::Obsidian,   Blocks::Obsidian },
          0, Blocks::Beacon, 1 },

        // Three cane in a row makes three paper, three paper and a
        // hide makes a book, and six planks round three books makes a
        // shelf. All three want a bench: every one of them is three
        // wide, which the two-by-two grid cannot reach.
        { false, 3, 1, { Blocks::SugarCane, Blocks::SugarCane, Blocks::SugarCane },
          0, Items::Paper, 3 },
        { true,  0, 0, { Items::Paper, Items::Paper, Items::Paper, Items::Leather },
          4, Items::Book, 1 },
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Items::Book,    Items::Book,    Items::Book,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::Bookshelf, 1 },

        // Four sand in a square becomes one sandstone.
        { false, 2, 2, { Blocks::Sand, Blocks::Sand,
                         Blocks::Sand, Blocks::Sand }, 0, Blocks::Sandstone, 1 },

        // Four planks make the bench, which is the only way to reach a
        // 3x3 grid and so the only way to reach anything three wide.
        { false, 2, 2, { Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks }, 0, Blocks::CraftingTable, 1 },

        // Eight cobblestone round an empty middle make a furnace.
        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Blocks::Cobblestone, _,                   Blocks::Cobblestone,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::Furnace, 1 },

        // Eight planks round an empty middle make a chest.
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, _,              Blocks::Planks,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::Chest, 1 },

        // Five gunpowder and four sand, crossed, makes TNT.
        { false, 3, 3, { Items::Gunpowder, Blocks::Sand,     Items::Gunpowder,
                         Blocks::Sand,     Items::Gunpowder, Blocks::Sand,
                         Items::Gunpowder, Blocks::Sand,     Items::Gunpowder },
          0, Blocks::Tnt, 1 },

        // Six of a block in a staircase shape makes four stairs.
        { false, 3, 3, { Blocks::Stone, _, _,
                         Blocks::Stone, Blocks::Stone, _,
                         Blocks::Stone, Blocks::Stone, Blocks::Stone },
          0, Blocks::StoneStairsNorth, 4 },
        { false, 3, 3, { Blocks::Cobblestone, _, _,
                         Blocks::Cobblestone, Blocks::Cobblestone, _,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::CobbleStairsNorth, 4 },
        { false, 3, 3, { Blocks::Planks, _, _,
                         Blocks::Planks, Blocks::Planks, _,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::PlankStairsNorth, 4 },
        { false, 3, 3, { Blocks::Sandstone, _, _,
                         Blocks::Sandstone, Blocks::Sandstone, _,
                         Blocks::Sandstone, Blocks::Sandstone, Blocks::Sandstone },
          0, Blocks::SandstoneStairsNorth, 4 },

        // Six planks make three doors.
        { false, 2, 3, { Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks },
          0, Blocks::DoorLowShutNorth, 3 },
        { false, 3, 2, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::OakTrapdoor, 2 },

        // Four planks and two sticks make three lengths of fence.
        { false, 3, 2, { Blocks::Planks, Items::Stick, Blocks::Planks,
                         Blocks::Planks, Items::Stick, Blocks::Planks },
          0, Blocks::OakFence, 3 },

        // Seven sticks make three ladders.
        { false, 3, 3, { Items::Stick, _,            Items::Stick,
                         Items::Stick, Items::Stick, Items::Stick,
                         Items::Stick, _,            Items::Stick },
          0, Blocks::LadderNorth, 3 },

        // Three of a block in a row makes six slabs of it.
        { false, 3, 1, { Blocks::Stone, Blocks::Stone, Blocks::Stone },
          0, Blocks::StoneSlab, 6 },
        { false, 3, 1, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::CobblestoneSlab, 6 },
        { false, 3, 1, { Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::PlankSlab, 6 },
        { false, 3, 1, { Blocks::Sandstone, Blocks::Sandstone, Blocks::Sandstone },
          0, Blocks::SandstoneSlab, 6 },

        // Three iron in a V makes a bucket.
        { false, 3, 2, { Items::IronIngot, _,                Items::IronIngot,
                         _,                Items::IronIngot, _ },
          0, Items::Bucket, 1 },

        // Iron and flint make the thing that lights a portal.
        { true, 0, 0, { Items::IronIngot, Items::Flint }, 2, Items::FlintAndSteel, 1 },

        // Three sticks and three string make a bow.
        { false, 3, 3, { _,                Items::Stick,      Items::StringItem,
                         Items::Stick,     _,                 Items::StringItem,
                         _,                Items::Stick,      Items::StringItem },
          0, Items::Bow, 1 },

        // Flint, stick and feather make four arrows.
        { false, 1, 3, { Items::Flint,
                         Items::Stick,
                         Items::Feather }, 0, Items::Arrow, 4 },

        // Coal over a stick makes four torches, which is the recipe the
        // whole early game turns on -- there was no way to light a cave
        // before it.
        { false, 1, 2, { Items::Coal,
                         Items::Stick }, 0, Blocks::Torch, 4 },

        // Four string makes a block of wool, so a bed does not depend on
        // finding sheep.
        { false, 2, 2, { Items::StringItem, Items::StringItem,
                         Items::StringItem, Items::StringItem }, 0, Blocks::Wool, 1 },

        // Three wool over three planks makes a bed.
        { false, 3, 2, { Blocks::Wool, Blocks::Wool, Blocks::Wool,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 0, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 0, Blocks::WoolOrange + 0, Blocks::WoolOrange + 0,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 8, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 1, Blocks::WoolOrange + 1, Blocks::WoolOrange + 1,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 16, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 2, Blocks::WoolOrange + 2, Blocks::WoolOrange + 2,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 24, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 3, Blocks::WoolOrange + 3, Blocks::WoolOrange + 3,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 32, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 4, Blocks::WoolOrange + 4, Blocks::WoolOrange + 4,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 40, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 5, Blocks::WoolOrange + 5, Blocks::WoolOrange + 5,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 48, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 6, Blocks::WoolOrange + 6, Blocks::WoolOrange + 6,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 56, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 7, Blocks::WoolOrange + 7, Blocks::WoolOrange + 7,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 64, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 8, Blocks::WoolOrange + 8, Blocks::WoolOrange + 8,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 72, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 9, Blocks::WoolOrange + 9, Blocks::WoolOrange + 9,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 80, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 10, Blocks::WoolOrange + 10, Blocks::WoolOrange + 10,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 88, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 11, Blocks::WoolOrange + 11, Blocks::WoolOrange + 11,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 96, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 12, Blocks::WoolOrange + 12, Blocks::WoolOrange + 12,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 104, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 13, Blocks::WoolOrange + 13, Blocks::WoolOrange + 13,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 112, 1 },
        { false, 3, 2, { Blocks::WoolOrange + 14, Blocks::WoolOrange + 14, Blocks::WoolOrange + 14,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::BedFirst + 120, 1 },

        // Two planks, one above the other, make four sticks.
        { false, 1, 2, { Blocks::Planks,
                         Blocks::Planks }, 0, Items::Stick, 4 },


        // Armour. Every piece is its material beaten into shape;
        // all four are three wide, so all four want the bench.
        { false, 3, 2, { Items::Leather, Items::Leather, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherHelmet, 1 },
        { false, 3, 3, { Items::Leather, _, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherHorseArmor, 1 },
        { false, 3, 3, { Items::Leather, _, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather }, 0, Items::LeatherChestplate, 1 },
        { false, 3, 3, { Items::Leather, Items::Leather, Items::Leather, Items::Leather, _, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherLeggings, 1 },
        { false, 3, 2, { Items::Leather, _, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherBoots, 1 },

        { false, 3, 2, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldHelmet, 1 },
        { false, 3, 3, { Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot }, 0, Items::GoldChestplate, 1 },
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldLeggings, 1 },
        { false, 3, 2, { Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldBoots, 1 },

        { false, 3, 2, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronHelmet, 1 },
        { false, 3, 3, { Items::IronIngot, _, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot }, 0, Items::IronChestplate, 1 },
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronLeggings, 1 },
        { false, 3, 2, { Items::IronIngot, _, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronBoots, 1 },

        { false, 3, 2, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondHelmet, 1 },
        { false, 3, 3, { Items::Diamond, _, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond }, 0, Items::DiamondChestplate, 1 },
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondLeggings, 1 },
        { false, 3, 2, { Items::Diamond, _, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondBoots, 1 },

        // Tools. Every one is its head material over a stick or two,
        // which is why none of them could be made before the bench --
        // all but the shovel and the sword are wider than a 2x2 grid.
        // Wood
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::WoodPickaxe, 1 },
        { false, 2, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks, Items::Stick, _, Items::Stick }, 0, Items::WoodAxe, 1 },
        { false, 1, 3, { Blocks::Planks, Items::Stick, Items::Stick }, 0, Items::WoodShovel, 1 },
        { false, 1, 3, { Blocks::Planks, Blocks::Planks, Items::Stick }, 0, Items::WoodSword, 1 },
        { false, 2, 3, { Blocks::Planks, Blocks::Planks, _, Items::Stick, _, Items::Stick }, 0, Items::WoodHoe, 1 },

        // Stone
        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::StonePickaxe, 1 },
        { false, 2, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone, Items::Stick, _, Items::Stick }, 0, Items::StoneAxe, 1 },
        { false, 1, 3, { Blocks::Cobblestone, Items::Stick, Items::Stick }, 0, Items::StoneShovel, 1 },
        { false, 1, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Items::Stick }, 0, Items::StoneSword, 1 },
        { false, 2, 3, { Blocks::Cobblestone, Blocks::Cobblestone, _, Items::Stick, _, Items::Stick }, 0, Items::StoneHoe, 1 },

        // Iron
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::IronPickaxe, 1 },
        { false, 2, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::Stick, _, Items::Stick }, 0, Items::IronAxe, 1 },
        { false, 1, 3, { Items::IronIngot, Items::Stick, Items::Stick }, 0, Items::IronShovel, 1 },
        { false, 1, 3, { Items::IronIngot, Items::IronIngot, Items::Stick }, 0, Items::IronSword, 1 },
        { false, 2, 3, { Items::IronIngot, Items::IronIngot, _, Items::Stick, _, Items::Stick }, 0, Items::IronHoe, 1 },

        // Gold
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::GoldPickaxe, 1 },
        { false, 2, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::Stick, _, Items::Stick }, 0, Items::GoldAxe, 1 },
        { false, 1, 3, { Items::GoldIngot, Items::Stick, Items::Stick }, 0, Items::GoldShovel, 1 },
        { false, 1, 3, { Items::GoldIngot, Items::GoldIngot, Items::Stick }, 0, Items::GoldSword, 1 },
        { false, 2, 3, { Items::GoldIngot, Items::GoldIngot, _, Items::Stick, _, Items::Stick }, 0, Items::GoldHoe, 1 },

        // Diamond
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::DiamondPickaxe, 1 },
        { false, 2, 3, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Stick, _, Items::Stick }, 0, Items::DiamondAxe, 1 },
        { false, 1, 3, { Items::Diamond, Items::Stick, Items::Stick }, 0, Items::DiamondShovel, 1 },
        { false, 1, 3, { Items::Diamond, Items::Diamond, Items::Stick }, 0, Items::DiamondSword, 1 },
        { false, 2, 3, { Items::Diamond, Items::Diamond, _, Items::Stick, _, Items::Stick }, 0, Items::DiamondHoe, 1 },
    };

    // Shrinks the used area of the grid down to its bounding box, so a
    // recipe placed in the bottom-right corner still matches.
    bool trim(const ItemStack* grid, int size, int& outX, int& outY, int& outW, int& outH)
    {
        int minX = size, minY = size, maxX = -1, maxY = -1;

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
                if (!grid[y * 3 + x].empty())
                {
                    minX = std::min(minX, x); maxX = std::max(maxX, x);
                    minY = std::min(minY, y); maxY = std::max(maxY, y);
                }

        if (maxX < 0) return false;   // nothing in the grid

        outX = minX; outY = minY;
        outW = maxX - minX + 1;
        outH = maxY - minY + 1;
        return true;
    }

    // Whether what is in the cell will do for what the recipe asked
    // for. Exact, except that any plank stands in for any other: in
    // Minecraft a spruce bench is a bench. Written once here rather
    // than by listing every wood in all thirty-odd recipes that take
    // planks, which is how a new wood ends up unable to make anything.
    bool satisfies(StackId wanted, StackId have)
    {
        if (have == wanted) return true;
        if (!isBlockStack(wanted) || !isBlockStack(have)) return false;
        return isPlanks(asBlock(wanted)) && isPlanks(asBlock(have));
    }

    StackId inWoodOf(StackId result, const ItemStack* grid, int size)
    {
        int wood = -1;
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const ItemStack& cell = grid[y * 3 + x];
                if (cell.empty() || !isBlockStack(cell.id) || !isPlanks(asBlock(cell.id))) continue;
                int here = 0;
                for (int w = 0; w < WOOD_COUNT; ++w)
                    if (planksOfWood(w) == asBlock(cell.id)) here = w;
                if (wood >= 0 && wood != here) return result;
                wood = here;
            }
        if (wood <= 0) return result;
        if (result == Blocks::DoorLowShutNorth) return doorBaseOf(wood);
        if (result == Blocks::OakTrapdoor) return trapdoorBaseOf(wood);
        if (result == Items::Sign) return signItemOfWood(wood);
        if (result == Blocks::PlankSlab && wood == 8) return Blocks::BambooSlab;
        return result;
    }

    bool matchesShaped(const Recipe& recipe, const ItemStack* grid, int size)
    {
        int x0 = 0, y0 = 0, width = 0, height = 0;
        if (!trim(grid, size, x0, y0, width, height)) return false;
        if (width != recipe.width || height != recipe.height) return false;

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                const ItemStack& cell = grid[(y0 + y) * 3 + (x0 + x)];
                const StackId wanted = recipe.pattern[y * recipe.width + x];
                if (wanted == _ && !cell.empty()) return false;
                if (wanted != _ && (cell.empty() || !satisfies(wanted, cell.id))) return false;
            }

        return true;
    }

    bool matchesShapeless(const Recipe& recipe, const ItemStack* grid, int size)
    {
        // Every filled cell has to be accounted for by exactly one
        // ingredient, and every ingredient has to be found.
        bool used[9] = { false };
        for (int i = 0; i < recipe.inputCount; ++i)
        {
            bool found = false;
            for (int y = 0; y < size && !found; ++y)
                for (int x = 0; x < size && !found; ++x)
                {
                    const int cell = y * 3 + x;
                    if (used[cell]) continue;
                    if (grid[cell].empty() || !satisfies(recipe.pattern[i], grid[cell].id)) continue;
                    used[cell] = true;
                    found = true;
                }
            if (!found) return false;
        }

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const int cell = y * 3 + x;
                if (!grid[cell].empty() && !used[cell]) return false;
            }

        return true;
    }
}

namespace Crafting
{
    // A diamond tool or piece of armour and a netherite ingot, in any
    // arrangement. Minecraft wants a smithing table; this is the same
    // trade at a bench, and the thing that matters is kept: what
    // comes out is the tool you had, not a new one.
    CraftOutput matchUpgrade(const ItemStack* grid, int size)
    {
        const ItemStack* diamond = nullptr;
        int ingots = 0;
        int others = 0;

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const ItemStack& cell = grid[y * 3 + x];
                if (cell.empty()) continue;

                if (cell.id == Items::NetheriteIngot) { ingots += cell.count; continue; }
                if (!diamond && Tools::upgradeOf(cell.id) != Blocks::Air &&
                    cell.count == 1)
                { diamond = &cell; continue; }
                ++others;
            }

        if (!diamond || ingots != 1 || others > 0) return CraftOutput{};

        CraftOutput out;
        out.id = Tools::upgradeOf(diamond->id);
        out.count = 1;

        // The wear carries across as a fraction, not a number: a
        // diamond pickaxe half gone comes out a netherite one half
        // gone, rather than one that is nearly new because netherite
        // lasts longer.
        const int wasMax = (Tools::kindOf(diamond->id) != Tools::Kind::None)
                               ? Tools::maxDurability(Tools::tierOf(diamond->id))
                               : Armour::maxDurability(diamond->id);
        const int nowMax = (Tools::kindOf(out.id) != Tools::Kind::None)
                               ? Tools::maxDurability(Tools::tierOf(out.id))
                               : Armour::maxDurability(out.id);
        out.damage = (wasMax > 0)
                         ? static_cast<int>(static_cast<long long>(diamond->damage) *
                                            nowMax / wasMax)
                         : 0;
        out.enchantments = diamond->enchantments;
        return out;
    }

    // Wool and a dye make wool of that colour. One rule rather than
    // sixteen recipes, because the dyes and the wools are numbered in
    // the same order: a dye's colour is the wool it makes.
    CraftOutput matchDyeing(const ItemStack* grid, int size)
    {
        int wools = 0, terracotta = 0, glass = 0, panes = 0, sand = 0, gravel = 0, candles = 0, boxes = 0;
        int dyes = 0;
        int colour = 0;
        int others = 0;

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const ItemStack& cell = grid[y * 3 + x];
                if (cell.empty()) continue;

                if (isDye(cell.id)) { ++dyes; colour = dyeColour(cell.id); continue; }
                if (!isBlockStack(cell.id)) { ++others; continue; }
                const BlockId block = asBlock(cell.id);
                if (isWool(block)) ++wools;
                else if (block == Blocks::Terracotta) ++terracotta;
                else if (block == Blocks::Glass) ++glass;
                else if (block == Blocks::GlassPane) ++panes;
                else if (block == Blocks::Sand) ++sand;
                else if (block == Blocks::Gravel) ++gravel;
                else if (block == Blocks::Candle) ++candles;
                else if (isShulkerBox(block)) ++boxes;
                else ++others;
            }

        if (dyes != 1 || others > 0) return CraftOutput{};

        const int families = (wools > 0) + (terracotta > 0) + (glass > 0) + (panes > 0) + (candles > 0) + (boxes > 0) +
                             ((sand > 0 || gravel > 0) ? 1 : 0);
        if (families != 1) return CraftOutput{};

        CraftOutput out;
        if (wools >= 1 && wools <= 8) { out.id = woolOfColour(colour); out.count = wools; }
        else if (terracotta == 8) { out.id = colourIn(Blocks::TerracottaFirst, colour); out.count = 8; }
        else if (glass == 8) { out.id = colourIn(Blocks::StainedGlassFirst, colour); out.count = 8; }
        else if (panes == 8) { out.id = colourIn(Blocks::StainedGlassPaneFirst, colour); out.count = 8; }
        else if (boxes == 1) { out.id = colourIn(Blocks::ShulkerBoxFirst, colour); out.count = 1; }
        else if (candles == 1) { out.id = candleOf(colour + 1, 1, false); out.count = 1; }
        else if (sand == 4 && gravel == 4) { out.id = colourIn(Blocks::ConcretePowderFirst, colour); out.count = 8; }
        return out;
    }

    CraftOutput match(const ItemStack* grid, int size)
    {
        for (const Recipe& recipe : RECIPES)
        {
            const bool hit = recipe.shapeless ? matchesShapeless(recipe, grid, size)
                                              : matchesShaped(recipe, grid, size);
            if (hit) return CraftOutput{ inWoodOf(recipe.result, grid, size), recipe.resultCount };
        }
        const CraftOutput dyed = matchDyeing(grid, size);
        if (dyed.valid()) return dyed;

        return matchUpgrade(grid, size);
    }

    int consume(ItemStack* grid, int size)
    {
        int bottles = 0;
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                ItemStack& cell = grid[y * 3 + x];
                if (cell.empty()) continue;

                // A recipe uses what is in the bucket, not the bucket.
                // Without this a cake cost three iron as well as three
                // milk, which is Minecraft's recipe at four times the
                // price. Only a single bucket can be in a slot, so
                // there is never a count to carry across.
                if (cell.id == Items::MilkBucket || cell.id == Items::WaterBucket ||
                    cell.id == Items::LavaBucket)
                {
                    cell.id = Items::Bucket;
                    cell.count = 1;
                    cell.damage = 0;
                    cell.enchantments = 0;
                    continue;
                }
                if (cell.id == Items::HoneyBottle)
                {
                    if (cell.count == 1) { cell.id = Items::GlassBottle; continue; }
                    ++bottles;
                }

                cell.count -= 1;
                if (cell.count <= 0) cell.clear();
            }
        return bottles;
    }
}


std::vector<StackId> Crafting::everyOutput()
{
    std::vector<StackId> out;
    out.reserve(std::size(RECIPES) + 9);
    for (const Recipe& recipe : RECIPES) out.push_back(recipe.result);

    // The sixteen wools a dye makes, which is likewise a rule rather
    // than a table: an audit that could not see it called fifteen of
    // the colours unmakeable.
    for (int colour = 0; colour < DYE_COLOURS; ++colour)
    {
        out.push_back(woolOfColour(colour));
        out.push_back(colourIn(Blocks::TerracottaFirst, colour));
        out.push_back(colourIn(Blocks::StainedGlassFirst, colour));
        out.push_back(colourIn(Blocks::StainedGlassPaneFirst, colour));
        out.push_back(colourIn(Blocks::ConcretePowderFirst, colour));
        out.push_back(candleOf(colour + 1, 1, false));
        out.push_back(colourIn(Blocks::ShulkerBoxFirst, colour));
    }

    for (int wood = 1; wood < WOOD_COUNT; ++wood)
    {
        out.push_back(doorBaseOf(wood));
        out.push_back(trapdoorBaseOf(wood));
        out.push_back(signItemOfWood(wood));
    }
    out.push_back(Blocks::BambooSlab);

    // And the nine the upgrade makes. It is a recipe the table cannot
    // hold -- it takes whichever diamond thing you give it -- but it
    // is still a recipe, and an audit that could not see it called
    // every netherite tool in the game unmakeable.
    for (StackId id = Items::DiamondPickaxe; id <= Items::DiamondHoe; ++id)
        out.push_back(Tools::upgradeOf(id));
    for (StackId id = Items::DiamondHelmet; id <= Items::DiamondBoots; ++id)
        out.push_back(Tools::upgradeOf(id));

    return out;
}

std::vector<StackId> Crafting::everyIngredient()
{
    std::vector<StackId> out;

    // What the upgrade asks for, which is likewise not in the table,
    // and what dyeing asks for.
    out.push_back(Items::NetheriteIngot);
    for (int colour = 0; colour < DYE_COLOURS; ++colour)
        out.push_back(dyeOfColour(colour));
    out.push_back(Blocks::Wool);
    out.push_back(Blocks::Terracotta);
    out.push_back(Blocks::Candle);
    out.push_back(Blocks::ShulkerBox);
    out.push_back(Blocks::GlassPane);
    out.push_back(Blocks::Sand);
    out.push_back(Blocks::Gravel);

    for (const Recipe& recipe : RECIPES)
    {
        const int many = recipe.shapeless ? recipe.inputCount : 9;
        for (int i = 0; i < many; ++i)
            if (recipe.pattern[i] != Blocks::Air) out.push_back(recipe.pattern[i]);
    }
    return out;
}
