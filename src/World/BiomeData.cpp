#include "Biomes.h"

namespace Biomes
{
    namespace
    {
        const Info INFO[COUNT] = {
            { "minecraft:badlands", "Badlands", 2.0f, 0.0f, false, false, 0x6EB1FF, 0xC0D8FF, 0x3F76E4, 0x050533, 9470285, 10387789, 0, "", 0.0f },
            { "minecraft:bamboo_jungle", "Bamboo Jungle", 0.95f, 0.9f, true, false, 0x77A8FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:basalt_deltas", "Basalt Deltas", 2.0f, 0.0f, false, false, 0x6EB1FF, 0x685F70, 0x3F76E4, 0x050533, -1, -1, 0, "white_ash", 0.118093334f },
            { "minecraft:beach", "Beach", 0.8f, 0.4f, true, false, 0x78A7FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:birch_forest", "Birch Forest", 0.6f, 0.6f, true, false, 0x7AA5FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:cherry_grove", "Cherry Grove", 0.5f, 0.8f, true, false, 0x7BA4FF, 0xC0D8FF, 0x5DB7EF, 0x5DB7EF, 11983713, 11983713, 0, "", 0.0f },
            { "minecraft:cold_ocean", "Cold Ocean", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x3D57D6, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:crimson_forest", "Crimson Forest", 2.0f, 0.0f, false, false, 0x6EB1FF, 0x330303, 0x3F76E4, 0x050533, -1, -1, 0, "crimson_spore", 0.025f },
            { "minecraft:dark_forest", "Dark Forest", 0.7f, 0.8f, true, false, 0x79A6FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 1, "", 0.0f },
            { "minecraft:deep_cold_ocean", "Deep Cold Ocean", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x3D57D6, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:deep_dark", "Deep Dark", 0.8f, 0.4f, true, false, 0x78A7FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:deep_frozen_ocean", "Deep Frozen Ocean", 0.5f, 0.5f, true, true, 0x7BA4FF, 0xC0D8FF, 0x3938C9, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:deep_lukewarm_ocean", "Deep Lukewarm Ocean", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x45ADF2, 0x041633, -1, -1, 0, "", 0.0f },
            { "minecraft:deep_ocean", "Deep Ocean", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:desert", "Desert", 2.0f, 0.0f, false, false, 0x6EB1FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:dripstone_caves", "Dripstone Caves", 0.8f, 0.4f, true, false, 0x78A7FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:end_barrens", "End Barrens", 0.5f, 0.5f, false, false, 0x000000, 0xA080A0, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:end_highlands", "End Highlands", 0.5f, 0.5f, false, false, 0x000000, 0xA080A0, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:end_midlands", "End Midlands", 0.5f, 0.5f, false, false, 0x000000, 0xA080A0, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:eroded_badlands", "Eroded Badlands", 2.0f, 0.0f, false, false, 0x6EB1FF, 0xC0D8FF, 0x3F76E4, 0x050533, 9470285, 10387789, 0, "", 0.0f },
            { "minecraft:flower_forest", "Flower Forest", 0.7f, 0.8f, true, false, 0x79A6FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:forest", "Forest", 0.7f, 0.8f, true, false, 0x79A6FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:frozen_ocean", "Frozen Ocean", 0.0f, 0.5f, true, true, 0x7FA1FF, 0xC0D8FF, 0x3938C9, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:frozen_peaks", "Frozen Peaks", -0.7f, 0.9f, true, false, 0x859DFF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:frozen_river", "Frozen River", 0.0f, 0.5f, true, false, 0x7FA1FF, 0xC0D8FF, 0x3938C9, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:grove", "Grove", -0.2f, 0.8f, true, false, 0x81A0FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:ice_spikes", "Ice Spikes", 0.0f, 0.5f, true, false, 0x7FA1FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:jagged_peaks", "Jagged Peaks", -0.7f, 0.9f, true, false, 0x859DFF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:jungle", "Jungle", 0.95f, 0.9f, true, false, 0x77A8FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:lukewarm_ocean", "Lukewarm Ocean", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x45ADF2, 0x041633, -1, -1, 0, "", 0.0f },
            { "minecraft:lush_caves", "Lush Caves", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:mangrove_swamp", "Mangrove Swamp", 0.8f, 0.9f, true, false, 0x78A7FF, 0xC0D8FF, 0x3A7A6A, 0x4D7A60, -1, 9285927, 2, "", 0.0f },
            { "minecraft:meadow", "Meadow", 0.5f, 0.8f, true, false, 0x7BA4FF, 0xC0D8FF, 0x0E4ECF, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:mushroom_fields", "Mushroom Fields", 0.9f, 1.0f, true, false, 0x77A8FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:nether_wastes", "Nether Wastes", 2.0f, 0.0f, false, false, 0x6EB1FF, 0x330808, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:ocean", "Ocean", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:old_growth_birch_forest", "Old Growth Birch Forest", 0.6f, 0.6f, true, false, 0x7AA5FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:old_growth_pine_taiga", "Old Growth Pine Taiga", 0.3f, 0.8f, true, false, 0x7CA3FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:old_growth_spruce_taiga", "Old Growth Spruce Taiga", 0.25f, 0.8f, true, false, 0x7DA3FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:plains", "Plains", 0.8f, 0.4f, true, false, 0x78A7FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:river", "River", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:savanna", "Savanna", 2.0f, 0.0f, false, false, 0x6EB1FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:savanna_plateau", "Savanna Plateau", 2.0f, 0.0f, false, false, 0x6EB1FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:small_end_islands", "Small End Islands", 0.5f, 0.5f, false, false, 0x000000, 0xA080A0, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:snowy_beach", "Snowy Beach", 0.05f, 0.3f, true, false, 0x7FA1FF, 0xC0D8FF, 0x3D57D6, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:snowy_plains", "Snowy Plains", 0.0f, 0.5f, true, false, 0x7FA1FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:snowy_slopes", "Snowy Slopes", -0.3f, 0.9f, true, false, 0x829FFF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:snowy_taiga", "Snowy Taiga", -0.5f, 0.4f, true, false, 0x839EFF, 0xC0D8FF, 0x3D57D6, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:soul_sand_valley", "Soul Sand Valley", 2.0f, 0.0f, false, false, 0x6EB1FF, 0x1B4745, 0x3F76E4, 0x050533, -1, -1, 0, "ash", 0.00625f },
            { "minecraft:sparse_jungle", "Sparse Jungle", 0.95f, 0.8f, true, false, 0x77A8FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:stony_peaks", "Stony Peaks", 1.0f, 0.3f, true, false, 0x76A8FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:stony_shore", "Stony Shore", 0.2f, 0.3f, true, false, 0x7DA2FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:sunflower_plains", "Sunflower Plains", 0.8f, 0.4f, true, false, 0x78A7FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:swamp", "Swamp", 0.8f, 0.9f, true, false, 0x78A7FF, 0xC0D8FF, 0x617B64, 0x232317, -1, 6975545, 2, "", 0.0f },
            { "minecraft:taiga", "Taiga", 0.25f, 0.8f, true, false, 0x7DA3FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:the_end", "The End", 0.5f, 0.5f, false, false, 0x000000, 0xA080A0, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:the_void", "The Void", 0.5f, 0.5f, false, false, 0x7BA4FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:warm_ocean", "Warm Ocean", 0.5f, 0.5f, true, false, 0x7BA4FF, 0xC0D8FF, 0x43D5EE, 0x041F33, -1, -1, 0, "", 0.0f },
            { "minecraft:warped_forest", "Warped Forest", 2.0f, 0.0f, false, false, 0x6EB1FF, 0x1A051A, 0x3F76E4, 0x050533, -1, -1, 0, "warped_spore", 0.01428f },
            { "minecraft:windswept_forest", "Windswept Forest", 0.2f, 0.3f, true, false, 0x7DA2FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:windswept_gravelly_hills", "Windswept Gravelly Hills", 0.2f, 0.3f, true, false, 0x7DA2FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:windswept_hills", "Windswept Hills", 0.2f, 0.3f, true, false, 0x7DA2FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:windswept_savanna", "Windswept Savanna", 2.0f, 0.0f, false, false, 0x6EB1FF, 0xC0D8FF, 0x3F76E4, 0x050533, -1, -1, 0, "", 0.0f },
            { "minecraft:wooded_badlands", "Wooded Badlands", 2.0f, 0.0f, false, false, 0x6EB1FF, 0xC0D8FF, 0x3F76E4, 0x050533, 9470285, 10387789, 0, "", 0.0f },
        };
    }

    const Info& info(Kind kind)
    {
        const int index = static_cast<int>(kind);
        return INFO[index >= 0 && index < COUNT ? index : static_cast<int>(Kind::Plains)];
    }
}
