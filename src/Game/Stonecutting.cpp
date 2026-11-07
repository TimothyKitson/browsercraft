#include "Stonecutting.h"

namespace
{
    struct Cut
    {
        BlockId input;
        BlockId result;
        int count;
    };

    const Cut& cutAt(int index);
    int cutCount();

    const Cut CUTS[] = {
        { copperBlockOf(CopperFamily::Block, 0, false), copperBlockOf(CopperFamily::Chiseled, 0, false), 4 },
        { copperBlockOf(CopperFamily::Cut, 0, false), copperBlockOf(CopperFamily::Chiseled, 0, false), 1 },
        { Blocks::CobbledDeepslate, Blocks::ChiseledDeepslate, 1 },
        { Blocks::NetherBricks, Blocks::ChiseledNetherBricks, 1 },
        { Blocks::Blackstone, Blocks::ChiseledPolishedBlackstone, 1 },
        { Blocks::PolishedBlackstone, Blocks::ChiseledPolishedBlackstone, 1 },
        { Blocks::QuartzBlock, Blocks::ChiseledQuartzBlock, 1 },
        { Blocks::RedSandstone, Blocks::ChiseledRedSandstone, 1 },
        { Blocks::Sandstone, Blocks::ChiseledSandstone, 1 },
        { Blocks::StoneBricks, Blocks::ChiseledStoneBricks, 1 },
        { Blocks::Stone, Blocks::ChiseledStoneBricks, 1 },
        { Blocks::PolishedTuff, Blocks::ChiseledTuffBricks, 1 },
        { Blocks::TuffBricks, Blocks::ChiseledTuffBricks, 1 },
        { Blocks::Tuff, Blocks::ChiseledTuffBricks, 1 },
        { Blocks::Tuff, Blocks::ChiseledTuff, 1 },
        { Blocks::CobbledDeepslate, Blocks::CobbledDeepslateSlab, 2 },
        { Blocks::Cobblestone, Blocks::CobblestoneSlab, 2 },
        { Blocks::Cobblestone, Blocks::CobbleStairsNorth, 1 },
        { copperBlockOf(CopperFamily::Block, 0, false), copperBlockOf(CopperFamily::Grate, 0, false), 4 },
        { copperBlockOf(CopperFamily::Block, 0, false), copperBlockOf(CopperFamily::Cut, 0, false), 4 },
        { copperBlockOf(CopperFamily::Block, 0, false), copperBlockOf(CopperFamily::Slab, 0, false), 8 },
        { copperBlockOf(CopperFamily::Cut, 0, false), copperBlockOf(CopperFamily::Slab, 0, false), 2 },
        { Blocks::RedSandstone, Blocks::CutRedSandstone, 1 },
        { Blocks::Sandstone, Blocks::CutSandstone, 1 },
        { Blocks::CobbledDeepslate, Blocks::DeepslateBricks, 1 },
        { Blocks::PolishedDeepslate, Blocks::DeepslateBricks, 1 },
        { Blocks::CobbledDeepslate, Blocks::DeepslateTiles, 1 },
        { Blocks::DeepslateBricks, Blocks::DeepslateTiles, 1 },
        { Blocks::PolishedDeepslate, Blocks::DeepslateTiles, 1 },
        { Blocks::EndStone, Blocks::EndStoneBricks, 1 },
        { copperBlockOf(CopperFamily::Block, 1, false), copperBlockOf(CopperFamily::Chiseled, 1, false), 4 },
        { copperBlockOf(CopperFamily::Cut, 1, false), copperBlockOf(CopperFamily::Chiseled, 1, false), 1 },
        { copperBlockOf(CopperFamily::Block, 1, false), copperBlockOf(CopperFamily::Grate, 1, false), 4 },
        { copperBlockOf(CopperFamily::Block, 1, false), copperBlockOf(CopperFamily::Cut, 1, false), 4 },
        { copperBlockOf(CopperFamily::Block, 1, false), copperBlockOf(CopperFamily::Slab, 1, false), 8 },
        { copperBlockOf(CopperFamily::Cut, 1, false), copperBlockOf(CopperFamily::Slab, 1, false), 2 },
        { Blocks::NetherBricks, Blocks::NetherBrickSlab, 2 },
        { copperBlockOf(CopperFamily::Block, 3, false), copperBlockOf(CopperFamily::Chiseled, 3, false), 4 },
        { copperBlockOf(CopperFamily::Cut, 3, false), copperBlockOf(CopperFamily::Chiseled, 3, false), 1 },
        { copperBlockOf(CopperFamily::Block, 3, false), copperBlockOf(CopperFamily::Grate, 3, false), 4 },
        { copperBlockOf(CopperFamily::Block, 3, false), copperBlockOf(CopperFamily::Cut, 3, false), 4 },
        { copperBlockOf(CopperFamily::Block, 3, false), copperBlockOf(CopperFamily::Slab, 3, false), 8 },
        { copperBlockOf(CopperFamily::Cut, 3, false), copperBlockOf(CopperFamily::Slab, 3, false), 2 },
        { Blocks::Andesite, Blocks::PolishedAndesite, 1 },
        { Blocks::Blackstone, Blocks::PolishedBlackstoneBricks, 1 },
        { Blocks::PolishedBlackstone, Blocks::PolishedBlackstoneBricks, 1 },
        { Blocks::Blackstone, Blocks::PolishedBlackstone, 1 },
        { Blocks::Blackstone, Blocks::PolishedBlackstoneSlab, 2 },
        { Blocks::PolishedBlackstone, Blocks::PolishedBlackstoneSlab, 2 },
        { Blocks::CobbledDeepslate, Blocks::PolishedDeepslate, 1 },
        { Blocks::Diorite, Blocks::PolishedDiorite, 1 },
        { Blocks::Granite, Blocks::PolishedGranite, 1 },
        { Blocks::Tuff, Blocks::PolishedTuff, 1 },
        { Blocks::PurpurBlock, Blocks::PurpurPillar, 1 },
        { Blocks::QuartzBlock, Blocks::QuartzBricks, 1 },
        { Blocks::QuartzBlock, Blocks::QuartzPillar, 1 },
        { Blocks::QuartzBlock, Blocks::QuartzSlab, 2 },
        { Blocks::RedSandstone, Blocks::RedSandstoneSlab, 2 },
        { Blocks::Sandstone, Blocks::SandstoneSlab, 2 },
        { Blocks::Sandstone, Blocks::SandstoneStairsNorth, 1 },
        { Blocks::SmoothStone, Blocks::SmoothStoneSlab, 2 },
        { Blocks::StoneBricks, Blocks::StoneBrickSlab, 2 },
        { Blocks::Stone, Blocks::StoneBrickSlab, 2 },
        { Blocks::Stone, Blocks::StoneBricks, 1 },
        { Blocks::Stone, Blocks::StoneSlab, 2 },
        { Blocks::Stone, Blocks::StoneStairsNorth, 1 },
        { Blocks::PolishedTuff, Blocks::TuffBrickSlab, 2 },
        { Blocks::TuffBricks, Blocks::TuffBrickSlab, 2 },
        { Blocks::Tuff, Blocks::TuffBrickSlab, 2 },
        { Blocks::PolishedTuff, Blocks::TuffBricks, 1 },
        { Blocks::Tuff, Blocks::TuffBricks, 1 },
        { Blocks::Tuff, Blocks::TuffSlab, 2 },
        { copperBlockOf(CopperFamily::Block, 0, true), copperBlockOf(CopperFamily::Chiseled, 0, true), 4 },
        { copperBlockOf(CopperFamily::Cut, 0, true), copperBlockOf(CopperFamily::Chiseled, 0, true), 1 },
        { copperBlockOf(CopperFamily::Block, 0, true), copperBlockOf(CopperFamily::Grate, 0, true), 4 },
        { copperBlockOf(CopperFamily::Block, 0, true), copperBlockOf(CopperFamily::Cut, 0, true), 4 },
        { copperBlockOf(CopperFamily::Block, 0, true), copperBlockOf(CopperFamily::Slab, 0, true), 8 },
        { copperBlockOf(CopperFamily::Cut, 0, true), copperBlockOf(CopperFamily::Slab, 0, true), 2 },
        { copperBlockOf(CopperFamily::Block, 1, true), copperBlockOf(CopperFamily::Chiseled, 1, true), 4 },
        { copperBlockOf(CopperFamily::Cut, 1, true), copperBlockOf(CopperFamily::Chiseled, 1, true), 1 },
        { copperBlockOf(CopperFamily::Block, 1, true), copperBlockOf(CopperFamily::Grate, 1, true), 4 },
        { copperBlockOf(CopperFamily::Block, 1, true), copperBlockOf(CopperFamily::Cut, 1, true), 4 },
        { copperBlockOf(CopperFamily::Block, 1, true), copperBlockOf(CopperFamily::Slab, 1, true), 8 },
        { copperBlockOf(CopperFamily::Cut, 1, true), copperBlockOf(CopperFamily::Slab, 1, true), 2 },
        { copperBlockOf(CopperFamily::Block, 3, true), copperBlockOf(CopperFamily::Chiseled, 3, true), 4 },
        { copperBlockOf(CopperFamily::Cut, 3, true), copperBlockOf(CopperFamily::Chiseled, 3, true), 1 },
        { copperBlockOf(CopperFamily::Block, 3, true), copperBlockOf(CopperFamily::Grate, 3, true), 4 },
        { copperBlockOf(CopperFamily::Block, 3, true), copperBlockOf(CopperFamily::Cut, 3, true), 4 },
        { copperBlockOf(CopperFamily::Block, 3, true), copperBlockOf(CopperFamily::Slab, 3, true), 8 },
        { copperBlockOf(CopperFamily::Cut, 3, true), copperBlockOf(CopperFamily::Slab, 3, true), 2 },
        { copperBlockOf(CopperFamily::Block, 2, true), copperBlockOf(CopperFamily::Chiseled, 2, true), 4 },
        { copperBlockOf(CopperFamily::Cut, 2, true), copperBlockOf(CopperFamily::Chiseled, 2, true), 1 },
        { copperBlockOf(CopperFamily::Block, 2, true), copperBlockOf(CopperFamily::Grate, 2, true), 4 },
        { copperBlockOf(CopperFamily::Block, 2, true), copperBlockOf(CopperFamily::Cut, 2, true), 4 },
        { copperBlockOf(CopperFamily::Block, 2, true), copperBlockOf(CopperFamily::Slab, 2, true), 8 },
        { copperBlockOf(CopperFamily::Cut, 2, true), copperBlockOf(CopperFamily::Slab, 2, true), 2 },
        { copperBlockOf(CopperFamily::Block, 2, false), copperBlockOf(CopperFamily::Chiseled, 2, false), 4 },
        { copperBlockOf(CopperFamily::Cut, 2, false), copperBlockOf(CopperFamily::Chiseled, 2, false), 1 },
        { copperBlockOf(CopperFamily::Block, 2, false), copperBlockOf(CopperFamily::Grate, 2, false), 4 },
        { copperBlockOf(CopperFamily::Block, 2, false), copperBlockOf(CopperFamily::Cut, 2, false), 4 },
        { copperBlockOf(CopperFamily::Block, 2, false), copperBlockOf(CopperFamily::Slab, 2, false), 8 },
        { copperBlockOf(CopperFamily::Cut, 2, false), copperBlockOf(CopperFamily::Slab, 2, false), 2 },
    };

    const Cut& cutAt(int index) { return CUTS[index]; }
    int cutCount() { return static_cast<int>(sizeof(CUTS) / sizeof(CUTS[0])); }
}

std::vector<Stonecutting::Option> Stonecutting::optionsFor(StackId input)
{
    std::vector<Option> out;
    if (isItem(input)) return out;
    for (int i = 0; i < cutCount(); ++i)
        if (cutAt(i).input == asBlock(input)) out.push_back(Option{ cutAt(i).result, cutAt(i).count });
    return out;
}

std::vector<StackId> Stonecutting::everyOutput()
{
    std::vector<StackId> out;
    for (int i = 0; i < cutCount(); ++i) out.push_back(cutAt(i).result);
    return out;
}
