#include "Composting.h"
#include "World/Block.h"

float Composting::chanceOf(StackId id)
{
    // Anything that grows. Asked of the families first, so a leaf or a
    // crop added later composts without anybody having to remember
    // this function.
    if (isBlockStack(id))
    {
        const BlockId block = asBlock(id);
        if (isLeaves(block)) return 0.3f;
        if (isWaterPlant(block)) return 0.3f;

        switch (block)
        {
            case Blocks::TallGrass:
            case Blocks::Vine:
            case Blocks::LilyPad:
            case Blocks::FlowerRed:
            case Blocks::FlowerYellow:
                return 0.3f;

            case Blocks::SugarCane:
            case Blocks::Cactus:
            case Blocks::Moss:
                return 0.5f;

            case Blocks::Pumpkin:
                return 0.65f;

            case Blocks::Bookshelf:
                return 0.3f;   // paper and wood, which is what it is

            default:
                return 0.0f;
        }
    }

    switch (id)
    {
        case Items::WheatSeeds:
            return 0.3f;

        case Items::Wheat:
        case Items::Carrot:
        case Items::Potato:
        case Items::Apple:
        case Items::GlowBerries:
        case Items::NetherWart:
            return 0.65f;

        // Something somebody has already done work to. Minecraft is
        // the same: the more a thing cost to make, the better it rots.
        case Items::Bread:
        case Items::BakedPotato:
            return 0.85f;

        default:
            return 0.0f;
    }
}

bool Composting::raises(StackId id, uint32_t roll)
{
    const float chance = chanceOf(id);
    if (chance <= 0.0f) return false;

    // A hundred faces, so the chances above read as percentages and a
    // 0.3 really does come up thirty times in a hundred. Rounded to a
    // whole percent first: compared as floats, a 0.3 that is really
    // 0.30000001 came up thirty-one times, which the test caught.
    const int percent = static_cast<int>(chance * 100.0f + 0.5f);
    return static_cast<int>(roll % 100u) < percent;
}
