#include "Composting.h"
#include "RecipeBook.h"
#include "World/Block.h"
#include <map>

float Composting::chanceOf(StackId id)
{
    static const std::pair<const char*, int> TABLE[] = {
        { "acacia_leaves", 30 }, { "acacia_sapling", 30 }, { "allium", 65 }, { "apple", 65 }, { "azalea", 65 }, {
        "azalea_leaves", 30 }, { "azure_bluet", 65 }, { "baked_potato", 85 }, { "beetroot", 65 }, { "beetroot_seeds", 30 }, {
        "big_dripleaf", 65 }, { "birch_leaves", 30 }, { "birch_sapling", 30 }, { "blue_orchid", 65 }, { "bread", 85 }, {
        "brown_mushroom", 65 }, { "brown_mushroom_block", 85 }, { "cactus", 50 }, { "cake", 100 }, { "carrot", 65 }, {
        "carved_pumpkin", 65 }, { "cherry_leaves", 30 }, { "cherry_sapling", 30 }, { "cocoa_beans", 65 }, { "cookie", 85 }, {
        "cornflower", 65 }, { "crimson_fungus", 65 }, { "crimson_roots", 65 }, { "dandelion", 65 }, { "dark_oak_leaves", 30 }, {
        "dark_oak_sapling", 30 }, { "dried_kelp", 30 }, { "dried_kelp_block", 50 }, { "fern", 65 }, { "flowering_azalea", 85 },
        { "flowering_azalea_leaves", 50 }, { "glow_berries", 30 }, { "glow_lichen", 50 }, { "hanging_roots", 30 }, {
        "hay_block", 85 }, { "jungle_leaves", 30 }, { "jungle_sapling", 30 }, { "kelp", 30 }, { "large_fern", 65 }, { "lilac",
        65 }, { "lily_of_the_valley", 65 }, { "lily_pad", 65 }, { "mangrove_leaves", 30 }, { "mangrove_propagule", 30 }, {
        "mangrove_roots", 30 }, { "melon", 65 }, { "melon_seeds", 30 }, { "melon_slice", 50 }, { "moss_block", 65 }, {
        "moss_carpet", 30 }, { "mushroom_stem", 65 }, { "nether_sprouts", 50 }, { "nether_wart", 65 }, { "nether_wart_block", 85
        }, { "oak_leaves", 30 }, { "oak_sapling", 30 }, { "orange_tulip", 65 }, { "oxeye_daisy", 65 }, { "peony", 65 }, {
        "pink_petals", 30 }, { "pink_tulip", 65 }, { "pitcher_plant", 85 }, { "pitcher_pod", 30 }, { "poppy", 65 }, { "potato",
        65 }, { "pumpkin", 65 }, { "pumpkin_pie", 100 }, { "pumpkin_seeds", 30 }, { "red_mushroom", 65 }, {
        "red_mushroom_block", 85 }, { "red_tulip", 65 }, { "rose_bush", 65 }, { "seagrass", 30 }, { "sea_pickle", 65 }, {
        "short_grass", 30 }, { "shroomlight", 65 }, { "small_dripleaf", 30 }, { "spore_blossom", 65 }, { "spruce_leaves", 30 },
        { "spruce_sapling", 30 }, { "sugar_cane", 50 }, { "sunflower", 65 }, { "sweet_berries", 30 }, { "tall_grass", 50 }, {
        "torchflower", 85 }, { "torchflower_seeds", 30 }, { "twisting_vines", 50 }, { "vine", 50 }, { "warped_fungus", 65 }, {
        "warped_roots", 65 }, { "warped_wart_block", 85 }, { "weeping_vines", 50 }, { "wheat", 65 }, { "wheat_seeds", 30 }, {
        "white_tulip", 65 }, { "wither_rose", 65 }
    };
    static std::map<StackId, int> byStack;
    if (byStack.empty())
        for (const auto& entry : TABLE)
        {
            const StackId stack = RecipeBook::stackNamed(entry.first);
            if (stack != Blocks::Air) byStack[stack] = entry.second;
        }
    const auto found = byStack.find(id);
    return found == byStack.end() ? 0.0f : static_cast<float>(found->second) / 100.0f;
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
