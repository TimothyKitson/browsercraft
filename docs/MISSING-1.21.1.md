# What Browsercraft does not have, against Minecraft 1.21.1

Generated from the 1.21.1 client source (Yarn mappings) by reading its own registries -- `Blocks`, `Items`, `EntityType`, `StatusEffects`, `Potions`, `Enchantments`, `BiomeKeys`, `StructureKeys`, `ScreenHandlerType` and the entity model classes -- and checking each id against what Browsercraft implements. Browsercraft's own names differ from Minecraft's in places (its `Planks` is `oak_planks`, its eight door states are one `oak_door`), so its side was mapped to 1.21.1 ids by hand.

## Summary

| | 1.21.1 | Missing |
|---|---:|---:|
| Blocks | 1054 | 841 |
| Items (not counting blocks) | 458 | 300 |
| Entities | 130 | 49 |
| Status effects | 39 | 28 |
| Potions | 46 | 39 |
| Enchantments | 42 | 35 |
| Biomes | 64 | 50 |
| Structures | 34 | 17 |
| Container screens | 25 | 15 |

## Blocks

1.21.1 has **1054**. Browsercraft has **213**. Missing: **841**.

Wall-mounted, potted, attached and lit forms are separate blocks in 1.21.1 (`wall_torch`, `potted_poppy`, `oak_wall_sign`) and are counted here as it counts them.

### Colour sets (16 dyes each) (207)

`white_bed`, `orange_bed`, `magenta_bed`, `light_blue_bed`, `yellow_bed`, `lime_bed`, `pink_bed`, `gray_bed`, `light_gray_bed`, `cyan_bed`, `purple_bed`, `blue_bed`, `brown_bed`, `green_bed`, `black_bed`, `white_stained_glass`, `orange_stained_glass`, `magenta_stained_glass`, `light_blue_stained_glass`, `yellow_stained_glass`, `lime_stained_glass`, `pink_stained_glass`, `gray_stained_glass`, `light_gray_stained_glass`, `cyan_stained_glass`, `purple_stained_glass`, `blue_stained_glass`, `brown_stained_glass`, `green_stained_glass`, `red_stained_glass`, `black_stained_glass`, `white_terracotta`, `orange_terracotta`, `magenta_terracotta`, `light_blue_terracotta`, `yellow_terracotta`, `lime_terracotta`, `pink_terracotta`, `gray_terracotta`, `light_gray_terracotta`, `cyan_terracotta`, `purple_terracotta`, `blue_terracotta`, `brown_terracotta`, `green_terracotta`, `red_terracotta`, `black_terracotta`, `white_stained_glass_pane`, `orange_stained_glass_pane`, `magenta_stained_glass_pane`, `light_blue_stained_glass_pane`, `yellow_stained_glass_pane`, `lime_stained_glass_pane`, `pink_stained_glass_pane`, `gray_stained_glass_pane`, `light_gray_stained_glass_pane`, `cyan_stained_glass_pane`, `purple_stained_glass_pane`, `blue_stained_glass_pane`, `brown_stained_glass_pane`, `green_stained_glass_pane`, `red_stained_glass_pane`, `black_stained_glass_pane`, `white_carpet`, `orange_carpet`, `magenta_carpet`, `light_blue_carpet`, `yellow_carpet`, `lime_carpet`, `pink_carpet`, `gray_carpet`, `light_gray_carpet`, `cyan_carpet`, `purple_carpet`, `blue_carpet`, `brown_carpet`, `green_carpet`, `red_carpet`, `black_carpet`, `white_banner`, `orange_banner`, `magenta_banner`, `light_blue_banner`, `yellow_banner`, `lime_banner`, `pink_banner`, `gray_banner`, `light_gray_banner`, `cyan_banner`, `purple_banner`, `blue_banner`, `brown_banner`, `green_banner`, `red_banner`, `black_banner`, `white_wall_banner`, `orange_wall_banner`, `magenta_wall_banner`, `light_blue_wall_banner`, `yellow_wall_banner`, `lime_wall_banner`, `pink_wall_banner`, `gray_wall_banner`, `light_gray_wall_banner`, `cyan_wall_banner`, `purple_wall_banner`, `blue_wall_banner`, `brown_wall_banner`, `green_wall_banner`, `red_wall_banner`, `black_wall_banner`, `white_shulker_box`, `orange_shulker_box`, `magenta_shulker_box`, `light_blue_shulker_box`, `yellow_shulker_box`, `lime_shulker_box`, `pink_shulker_box`, `gray_shulker_box`, `light_gray_shulker_box`, `cyan_shulker_box`, `purple_shulker_box`, `blue_shulker_box`, `brown_shulker_box`, `green_shulker_box`, `red_shulker_box`, `black_shulker_box`, `white_glazed_terracotta`, `orange_glazed_terracotta`, `magenta_glazed_terracotta`, `light_blue_glazed_terracotta`, `yellow_glazed_terracotta`, `lime_glazed_terracotta`, `pink_glazed_terracotta`, `gray_glazed_terracotta`, `light_gray_glazed_terracotta`, `cyan_glazed_terracotta`, `purple_glazed_terracotta`, `blue_glazed_terracotta`, `brown_glazed_terracotta`, `green_glazed_terracotta`, `red_glazed_terracotta`, `black_glazed_terracotta`, `white_concrete`, `orange_concrete`, `magenta_concrete`, `light_blue_concrete`, `yellow_concrete`, `lime_concrete`, `pink_concrete`, `gray_concrete`, `light_gray_concrete`, `cyan_concrete`, `purple_concrete`, `blue_concrete`, `brown_concrete`, `green_concrete`, `red_concrete`, `black_concrete`, `white_concrete_powder`, `orange_concrete_powder`, `magenta_concrete_powder`, `light_blue_concrete_powder`, `yellow_concrete_powder`, `lime_concrete_powder`, `pink_concrete_powder`, `gray_concrete_powder`, `light_gray_concrete_powder`, `cyan_concrete_powder`, `purple_concrete_powder`, `blue_concrete_powder`, `brown_concrete_powder`, `green_concrete_powder`, `red_concrete_powder`, `black_concrete_powder`, `white_candle`, `orange_candle`, `magenta_candle`, `light_blue_candle`, `yellow_candle`, `lime_candle`, `pink_candle`, `gray_candle`, `light_gray_candle`, `cyan_candle`, `purple_candle`, `blue_candle`, `brown_candle`, `green_candle`, `red_candle`, `black_candle`, `white_candle_cake`, `orange_candle_cake`, `magenta_candle_cake`, `light_blue_candle_cake`, `yellow_candle_cake`, `lime_candle_cake`, `pink_candle_cake`, `gray_candle_cake`, `light_gray_candle_cake`, `cyan_candle_cake`, `purple_candle_cake`, `blue_candle_cake`, `brown_candle_cake`, `green_candle_cake`, `red_candle_cake`, `black_candle_cake`

### Wood sets (190)

`birch_planks`, `cherry_planks`, `mangrove_planks`, `bamboo_planks`, `bamboo_mosaic`, `oak_sapling`, `spruce_sapling`, `birch_sapling`, `jungle_sapling`, `acacia_sapling`, `cherry_sapling`, `dark_oak_sapling`, `mangrove_propagule`, `cherry_log`, `mangrove_log`, `mangrove_roots`, `bamboo_block`, `stripped_spruce_log`, `stripped_birch_log`, `stripped_jungle_log`, `stripped_acacia_log`, `stripped_cherry_log`, `stripped_dark_oak_log`, `stripped_oak_log`, `stripped_mangrove_log`, `stripped_bamboo_block`, `oak_wood`, `spruce_wood`, `birch_wood`, `jungle_wood`, `acacia_wood`, `cherry_wood`, `dark_oak_wood`, `mangrove_wood`, `stripped_oak_wood`, `stripped_spruce_wood`, `stripped_birch_wood`, `stripped_jungle_wood`, `stripped_acacia_wood`, `stripped_cherry_wood`, `stripped_dark_oak_wood`, `stripped_mangrove_wood`, `cherry_leaves`, `mangrove_leaves`, `spruce_sign`, `birch_sign`, `acacia_sign`, `cherry_sign`, `jungle_sign`, `dark_oak_sign`, `mangrove_sign`, `bamboo_sign`, `spruce_wall_sign`, `birch_wall_sign`, `acacia_wall_sign`, `cherry_wall_sign`, `jungle_wall_sign`, `dark_oak_wall_sign`, `mangrove_wall_sign`, `bamboo_wall_sign`, `oak_hanging_sign`, `spruce_hanging_sign`, `birch_hanging_sign`, `acacia_hanging_sign`, `cherry_hanging_sign`, `jungle_hanging_sign`, `dark_oak_hanging_sign`, `crimson_hanging_sign`, `warped_hanging_sign`, `mangrove_hanging_sign`, `bamboo_hanging_sign`, `oak_wall_hanging_sign`, `spruce_wall_hanging_sign`, `birch_wall_hanging_sign`, `acacia_wall_hanging_sign`, `cherry_wall_hanging_sign`, `jungle_wall_hanging_sign`, `dark_oak_wall_hanging_sign`, `mangrove_wall_hanging_sign`, `crimson_wall_hanging_sign`, `warped_wall_hanging_sign`, `bamboo_wall_hanging_sign`, `oak_pressure_plate`, `spruce_pressure_plate`, `birch_pressure_plate`, `jungle_pressure_plate`, `acacia_pressure_plate`, `cherry_pressure_plate`, `dark_oak_pressure_plate`, `mangrove_pressure_plate`, `bamboo_pressure_plate`, `oak_trapdoor`, `spruce_trapdoor`, `birch_trapdoor`, `jungle_trapdoor`, `acacia_trapdoor`, `cherry_trapdoor`, `dark_oak_trapdoor`, `mangrove_trapdoor`, `bamboo_trapdoor`, `oak_fence_gate`, `spruce_stairs`, `birch_stairs`, `jungle_stairs`, `oak_button`, `spruce_button`, `birch_button`, `jungle_button`, `acacia_button`, `cherry_button`, `dark_oak_button`, `mangrove_button`, `bamboo_button`, `acacia_stairs`, `cherry_stairs`, `dark_oak_stairs`, `mangrove_stairs`, `bamboo_stairs`, `bamboo_mosaic_stairs`, `spruce_slab`, `birch_slab`, `jungle_slab`, `acacia_slab`, `cherry_slab`, `dark_oak_slab`, `mangrove_slab`, `bamboo_slab`, `bamboo_mosaic_slab`, `spruce_fence_gate`, `birch_fence_gate`, `jungle_fence_gate`, `acacia_fence_gate`, `cherry_fence_gate`, `dark_oak_fence_gate`, `mangrove_fence_gate`, `bamboo_fence_gate`, `spruce_fence`, `birch_fence`, `jungle_fence`, `acacia_fence`, `cherry_fence`, `dark_oak_fence`, `mangrove_fence`, `bamboo_fence`, `spruce_door`, `birch_door`, `jungle_door`, `acacia_door`, `cherry_door`, `dark_oak_door`, `mangrove_door`, `bamboo_door`, `bamboo_sapling`, `warped_stem`, `stripped_warped_stem`, `warped_hyphae`, `stripped_warped_hyphae`, `warped_nylium`, `warped_fungus`, `warped_wart_block`, `warped_roots`, `crimson_stem`, `stripped_crimson_stem`, `crimson_hyphae`, `stripped_crimson_hyphae`, `crimson_nylium`, `crimson_fungus`, `crimson_roots`, `crimson_planks`, `warped_planks`, `crimson_slab`, `warped_slab`, `crimson_pressure_plate`, `warped_pressure_plate`, `crimson_fence`, `warped_fence`, `crimson_trapdoor`, `warped_trapdoor`, `crimson_fence_gate`, `warped_fence_gate`, `crimson_stairs`, `warped_stairs`, `crimson_button`, `warped_button`, `crimson_door`, `warped_door`, `crimson_sign`, `warped_sign`, `crimson_wall_sign`, `warped_wall_sign`

### Copper (69)

`deepslate_copper_ore`, `oxidized_cut_copper`, `weathered_cut_copper`, `exposed_cut_copper`, `cut_copper`, `oxidized_chiseled_copper`, `weathered_chiseled_copper`, `exposed_chiseled_copper`, `chiseled_copper`, `waxed_oxidized_chiseled_copper`, `waxed_weathered_chiseled_copper`, `waxed_exposed_chiseled_copper`, `waxed_chiseled_copper`, `oxidized_cut_copper_stairs`, `weathered_cut_copper_stairs`, `exposed_cut_copper_stairs`, `cut_copper_stairs`, `oxidized_cut_copper_slab`, `weathered_cut_copper_slab`, `exposed_cut_copper_slab`, `cut_copper_slab`, `waxed_copper_block`, `waxed_weathered_copper`, `waxed_exposed_copper`, `waxed_oxidized_copper`, `waxed_oxidized_cut_copper`, `waxed_weathered_cut_copper`, `waxed_exposed_cut_copper`, `waxed_cut_copper`, `waxed_oxidized_cut_copper_stairs`, `waxed_weathered_cut_copper_stairs`, `waxed_exposed_cut_copper_stairs`, `waxed_cut_copper_stairs`, `waxed_oxidized_cut_copper_slab`, `waxed_weathered_cut_copper_slab`, `waxed_exposed_cut_copper_slab`, `waxed_cut_copper_slab`, `copper_door`, `exposed_copper_door`, `oxidized_copper_door`, `weathered_copper_door`, `waxed_copper_door`, `waxed_exposed_copper_door`, `waxed_oxidized_copper_door`, `waxed_weathered_copper_door`, `copper_trapdoor`, `exposed_copper_trapdoor`, `oxidized_copper_trapdoor`, `weathered_copper_trapdoor`, `waxed_copper_trapdoor`, `waxed_exposed_copper_trapdoor`, `waxed_oxidized_copper_trapdoor`, `waxed_weathered_copper_trapdoor`, `copper_grate`, `exposed_copper_grate`, `weathered_copper_grate`, `oxidized_copper_grate`, `waxed_copper_grate`, `waxed_exposed_copper_grate`, `waxed_weathered_copper_grate`, `waxed_oxidized_copper_grate`, `exposed_copper_bulb`, `weathered_copper_bulb`, `oxidized_copper_bulb`, `waxed_copper_bulb`, `waxed_exposed_copper_bulb`, `waxed_weathered_copper_bulb`, `waxed_oxidized_copper_bulb`, `raw_copper_block`

### Ores (11)

`deepslate_gold_ore`, `deepslate_iron_ore`, `deepslate_coal_ore`, `nether_gold_ore`, `deepslate_lapis_ore`, `deepslate_diamond_ore`, `deepslate_redstone_ore`, `deepslate_emerald_ore`, `nether_quartz_ore`, `raw_iron_block`, `raw_gold_block`

### Coral (30)

`dead_tube_coral_block`, `dead_brain_coral_block`, `dead_bubble_coral_block`, `dead_fire_coral_block`, `dead_horn_coral_block`, `dead_tube_coral`, `dead_brain_coral`, `dead_bubble_coral`, `dead_fire_coral`, `dead_horn_coral`, `dead_tube_coral_fan`, `dead_brain_coral_fan`, `dead_bubble_coral_fan`, `dead_fire_coral_fan`, `dead_horn_coral_fan`, `tube_coral_fan`, `brain_coral_fan`, `bubble_coral_fan`, `fire_coral_fan`, `horn_coral_fan`, `dead_tube_coral_wall_fan`, `dead_brain_coral_wall_fan`, `dead_bubble_coral_wall_fan`, `dead_fire_coral_wall_fan`, `dead_horn_coral_wall_fan`, `tube_coral_wall_fan`, `brain_coral_wall_fan`, `bubble_coral_wall_fan`, `fire_coral_wall_fan`, `horn_coral_wall_fan`

### Plants, crops and flowers (99)

`muddy_mangrove_roots`, `flowering_azalea_leaves`, `fern`, `dead_bush`, `tall_seagrass`, `torchflower`, `blue_orchid`, `allium`, `azure_bluet`, `red_tulip`, `orange_tulip`, `white_tulip`, `pink_tulip`, `oxeye_daisy`, `cornflower`, `wither_rose`, `lily_of_the_valley`, `brown_mushroom`, `red_mushroom`, `carved_pumpkin`, `jack_o_lantern`, `mossy_stone_bricks`, `infested_mossy_stone_bricks`, `brown_mushroom_block`, `red_mushroom_block`, `mushroom_stem`, `glow_lichen`, `mossy_cobblestone_wall`, `flower_pot`, `potted_torchflower`, `potted_oak_sapling`, `potted_spruce_sapling`, `potted_birch_sapling`, `potted_jungle_sapling`, `potted_acacia_sapling`, `potted_cherry_sapling`, `potted_dark_oak_sapling`, `potted_mangrove_propagule`, `potted_fern`, `potted_blue_orchid`, `potted_allium`, `potted_azure_bluet`, `potted_red_tulip`, `potted_orange_tulip`, `potted_white_tulip`, `potted_pink_tulip`, `potted_oxeye_daisy`, `potted_cornflower`, `potted_lily_of_the_valley`, `potted_wither_rose`, `potted_red_mushroom`, `potted_brown_mushroom`, `potted_dead_bush`, `potted_cactus`, `hay_block`, `sunflower`, `lilac`, `rose_bush`, `peony`, `tall_grass`, `large_fern`, `torchflower_crop`, `pitcher_crop`, `pitcher_plant`, `nether_wart_block`, `dried_kelp_block`, `sniffer_egg`, `potted_bamboo`, `mossy_stone_brick_stairs`, `mossy_cobblestone_stairs`, `mossy_stone_brick_slab`, `mossy_cobblestone_slab`, `mossy_stone_brick_wall`, `sweet_berry_bush`, `nether_sprouts`, `weeping_vines`, `weeping_vines_plant`, `twisting_vines`, `twisting_vines_plant`, `bee_nest`, `beehive`, `potted_crimson_fungus`, `potted_warped_fungus`, `potted_crimson_roots`, `potted_warped_roots`, `sculk_vein`, `cave_vines_plant`, `spore_blossom`, `azalea`, `flowering_azalea`, `moss_carpet`, `pink_petals`, `big_dripleaf`, `big_dripleaf_stem`, `small_dripleaf`, `hanging_roots`, `potted_azalea_bush`, `potted_flowering_azalea_bush`, `frogspawn`

### Redstone (19)

`powered_rail`, `detector_rail`, `moving_piston`, `stone_pressure_plate`, `redstone_wall_torch`, `repeater`, `tripwire_hook`, `tripwire`, `trapped_chest`, `light_weighted_pressure_plate`, `heavy_weighted_pressure_plate`, `comparator`, `daylight_detector`, `activator_rail`, `target`, `polished_blackstone_pressure_plate`, `polished_blackstone_button`, `calibrated_sculk_sensor`, `lightning_rod`

### Glass, ice and light (15)

`wall_torch`, `soul_torch`, `soul_wall_torch`, `glass_pane`, `light`, `packed_ice`, `end_rod`, `frosted_ice`, `blue_ice`, `conduit`, `soul_campfire`, `shroomlight`, `respawn_anchor`, `candle_cake`, `tinted_glass`

### Workstations and storage (14)

`chiseled_bookshelf`, `jukebox`, `lava_cauldron`, `powder_snow_cauldron`, `ender_chest`, `chipped_anvil`, `damaged_anvil`, `loom`, `cartography_table`, `fletching_table`, `smithing_table`, `stonecutter`, `bell`, `decorated_pot`

### Heads and skulls (13)

`skeleton_skull`, `skeleton_wall_skull`, `wither_skeleton_wall_skull`, `zombie_head`, `zombie_wall_head`, `player_head`, `player_wall_head`, `creeper_head`, `creeper_wall_head`, `dragon_head`, `dragon_wall_head`, `piglin_head`, `piglin_wall_head`

### Stone, brick and deepslate sets (134)

`polished_granite`, `polished_diorite`, `polished_andesite`, `chiseled_sandstone`, `cut_sandstone`, `polished_basalt`, `stone_bricks`, `cracked_stone_bricks`, `chiseled_stone_bricks`, `packed_mud`, `mud_bricks`, `infested_stone`, `infested_cobblestone`, `infested_stone_bricks`, `infested_cracked_stone_bricks`, `infested_chiseled_stone_bricks`, `brick_stairs`, `stone_brick_stairs`, `mud_brick_stairs`, `nether_brick_fence`, `nether_brick_stairs`, `cobblestone_wall`, `quartz_block`, `chiseled_quartz_block`, `quartz_pillar`, `quartz_stairs`, `prismarine_stairs`, `prismarine_brick_stairs`, `dark_prismarine_stairs`, `prismarine_slab`, `prismarine_brick_slab`, `dark_prismarine_slab`, `terracotta`, `red_sandstone`, `chiseled_red_sandstone`, `cut_red_sandstone`, `red_sandstone_stairs`, `smooth_stone_slab`, `cut_sandstone_slab`, `brick_slab`, `stone_brick_slab`, `mud_brick_slab`, `nether_brick_slab`, `quartz_slab`, `red_sandstone_slab`, `cut_red_sandstone_slab`, `purpur_slab`, `smooth_stone`, `smooth_sandstone`, `smooth_quartz`, `smooth_red_sandstone`, `purpur_stairs`, `red_nether_bricks`, `polished_granite_stairs`, `smooth_red_sandstone_stairs`, `polished_diorite_stairs`, `end_stone_brick_stairs`, `smooth_sandstone_stairs`, `smooth_quartz_stairs`, `granite_stairs`, `andesite_stairs`, `red_nether_brick_stairs`, `polished_andesite_stairs`, `diorite_stairs`, `polished_granite_slab`, `smooth_red_sandstone_slab`, `polished_diorite_slab`, `end_stone_brick_slab`, `smooth_sandstone_slab`, `smooth_quartz_slab`, `granite_slab`, `andesite_slab`, `red_nether_brick_slab`, `polished_andesite_slab`, `diorite_slab`, `brick_wall`, `prismarine_wall`, `red_sandstone_wall`, `granite_wall`, `stone_brick_wall`, `mud_brick_wall`, `nether_brick_wall`, `andesite_wall`, `red_nether_brick_wall`, `sandstone_wall`, `end_stone_brick_wall`, `diorite_wall`, `crying_obsidian`, `lodestone`, `blackstone_stairs`, `blackstone_wall`, `blackstone_slab`, `polished_blackstone`, `cracked_polished_blackstone_bricks`, `chiseled_polished_blackstone`, `polished_blackstone_brick_slab`, `polished_blackstone_brick_stairs`, `polished_blackstone_brick_wall`, `polished_blackstone_stairs`, `polished_blackstone_slab`, `polished_blackstone_wall`, `chiseled_nether_bricks`, `cracked_nether_bricks`, `quartz_bricks`, `tuff_slab`, `tuff_stairs`, `tuff_wall`, `polished_tuff`, `polished_tuff_slab`, `polished_tuff_stairs`, `polished_tuff_wall`, `chiseled_tuff`, `tuff_brick_slab`, `tuff_brick_stairs`, `tuff_brick_wall`, `chiseled_tuff_bricks`, `cobbled_deepslate`, `cobbled_deepslate_stairs`, `cobbled_deepslate_slab`, `cobbled_deepslate_wall`, `polished_deepslate`, `polished_deepslate_stairs`, `polished_deepslate_slab`, `polished_deepslate_wall`, `deepslate_tile_stairs`, `deepslate_tile_slab`, `deepslate_tile_wall`, `deepslate_brick_stairs`, `deepslate_brick_slab`, `deepslate_brick_wall`, `chiseled_deepslate`, `cracked_deepslate_bricks`, `cracked_deepslate_tiles`, `infested_deepslate`

### Everything else (40)

`coarse_dirt`, `podzol`, `suspicious_sand`, `red_sand`, `suspicious_gravel`, `cobweb`, `fire`, `soul_fire`, `iron_door`, `snow`, `soul_soil`, `iron_bars`, `chain`, `mycelium`, `dragon_egg`, `command_block`, `potted_dandelion`, `potted_poppy`, `barrier`, `iron_trapdoor`, `petrified_oak_slab`, `repeating_command_block`, `chain_command_block`, `magma_block`, `bone_block`, `structure_void`, `turtle_egg`, `void_air`, `cave_air`, `bubble_column`, `structure_block`, `jigsaw`, `honey_block`, `honeycomb_block`, `large_amethyst_bud`, `medium_amethyst_bud`, `small_amethyst_bud`, `powder_snow`, `rooted_dirt`, `heavy_core`

## Items (not counting blocks)

1.21.1 has **458**. Browsercraft has **158**. Missing: **300**.

### Spawn eggs (80)

`armadillo_spawn_egg`, `allay_spawn_egg`, `axolotl_spawn_egg`, `bat_spawn_egg`, `bee_spawn_egg`, `blaze_spawn_egg`, `bogged_spawn_egg`, `breeze_spawn_egg`, `cat_spawn_egg`, `camel_spawn_egg`, `cave_spider_spawn_egg`, `chicken_spawn_egg`, `cod_spawn_egg`, `cow_spawn_egg`, `creeper_spawn_egg`, `dolphin_spawn_egg`, `donkey_spawn_egg`, `drowned_spawn_egg`, `elder_guardian_spawn_egg`, `ender_dragon_spawn_egg`, `enderman_spawn_egg`, `endermite_spawn_egg`, `evoker_spawn_egg`, `fox_spawn_egg`, `frog_spawn_egg`, `ghast_spawn_egg`, `glow_squid_spawn_egg`, `goat_spawn_egg`, `guardian_spawn_egg`, `hoglin_spawn_egg`, `horse_spawn_egg`, `husk_spawn_egg`, `iron_golem_spawn_egg`, `llama_spawn_egg`, `magma_cube_spawn_egg`, `mooshroom_spawn_egg`, `mule_spawn_egg`, `ocelot_spawn_egg`, `panda_spawn_egg`, `parrot_spawn_egg`, `phantom_spawn_egg`, `pig_spawn_egg`, `piglin_spawn_egg`, `piglin_brute_spawn_egg`, `pillager_spawn_egg`, `polar_bear_spawn_egg`, `pufferfish_spawn_egg`, `rabbit_spawn_egg`, `ravager_spawn_egg`, `salmon_spawn_egg`, `sheep_spawn_egg`, `shulker_spawn_egg`, `silverfish_spawn_egg`, `skeleton_spawn_egg`, `skeleton_horse_spawn_egg`, `slime_spawn_egg`, `sniffer_spawn_egg`, `snow_golem_spawn_egg`, `spider_spawn_egg`, `squid_spawn_egg`, `stray_spawn_egg`, `strider_spawn_egg`, `tadpole_spawn_egg`, `trader_llama_spawn_egg`, `tropical_fish_spawn_egg`, `turtle_spawn_egg`, `vex_spawn_egg`, `villager_spawn_egg`, `vindicator_spawn_egg`, `wandering_trader_spawn_egg`, `warden_spawn_egg`, `witch_spawn_egg`, `wither_spawn_egg`, `wither_skeleton_spawn_egg`, `wolf_spawn_egg`, `zoglin_spawn_egg`, `zombie_spawn_egg`, `zombie_horse_spawn_egg`, `zombie_villager_spawn_egg`, `zombified_piglin_spawn_egg`

### Music discs (20)

`music_disc_13`, `music_disc_cat`, `music_disc_blocks`, `music_disc_chirp`, `music_disc_creator`, `music_disc_creator_music_box`, `music_disc_far`, `music_disc_mall`, `music_disc_mellohi`, `music_disc_stal`, `music_disc_strad`, `music_disc_ward`, `music_disc_11`, `music_disc_wait`, `music_disc_otherside`, `music_disc_relic`, `music_disc_5`, `music_disc_pigstep`, `music_disc_precipice`, `disc_fragment_5`

### Smithing templates (19)

`netherite_upgrade_smithing_template`, `sentry_armor_trim_smithing_template`, `dune_armor_trim_smithing_template`, `coast_armor_trim_smithing_template`, `wild_armor_trim_smithing_template`, `ward_armor_trim_smithing_template`, `eye_armor_trim_smithing_template`, `vex_armor_trim_smithing_template`, `tide_armor_trim_smithing_template`, `snout_armor_trim_smithing_template`, `rib_armor_trim_smithing_template`, `spire_armor_trim_smithing_template`, `wayfinder_armor_trim_smithing_template`, `shaper_armor_trim_smithing_template`, `silence_armor_trim_smithing_template`, `raiser_armor_trim_smithing_template`, `host_armor_trim_smithing_template`, `flow_armor_trim_smithing_template`, `bolt_armor_trim_smithing_template`

### Pottery sherds (23)

`angler_pottery_sherd`, `archer_pottery_sherd`, `arms_up_pottery_sherd`, `blade_pottery_sherd`, `brewer_pottery_sherd`, `burn_pottery_sherd`, `danger_pottery_sherd`, `explorer_pottery_sherd`, `flow_pottery_sherd`, `friend_pottery_sherd`, `guster_pottery_sherd`, `heart_pottery_sherd`, `heartbreak_pottery_sherd`, `howl_pottery_sherd`, `miner_pottery_sherd`, `mourner_pottery_sherd`, `plenty_pottery_sherd`, `prize_pottery_sherd`, `scrape_pottery_sherd`, `sheaf_pottery_sherd`, `shelter_pottery_sherd`, `skull_pottery_sherd`, `snort_pottery_sherd`

### Banner patterns (8)

`flower_banner_pattern`, `creeper_banner_pattern`, `skull_banner_pattern`, `mojang_banner_pattern`, `globe_banner_pattern`, `piglin_banner_pattern`, `flow_banner_pattern`, `guster_banner_pattern`

### Tools, weapons and armour (29)

`saddle`, `carrot_on_a_stick`, `warped_fungus_on_a_stick`, `turtle_helmet`, `turtle_scute`, `armadillo_scute`, `wolf_armor`, `chainmail_helmet`, `chainmail_chestplate`, `chainmail_leggings`, `chainmail_boots`, `compass`, `recovery_compass`, `fishing_rod`, `clock`, `spyglass`, `shears`, `firework_rocket`, `firework_star`, `iron_horse_armor`, `golden_horse_armor`, `diamond_horse_armor`, `leather_horse_armor`, `lead`, `name_tag`, `spectral_arrow`, `tipped_arrow`, `crossbow`, `brush`

### Food (19)

`mushroom_stew`, `golden_apple`, `enchanted_golden_apple`, `tropical_fish_bucket`, `tropical_fish`, `dried_kelp`, `rotten_flesh`, `spider_eye`, `fermented_spider_eye`, `glistering_melon_slice`, `poisonous_potato`, `golden_carrot`, `pumpkin_pie`, `rabbit_stew`, `chorus_fruit`, `popped_chorus_fruit`, `suspicious_stew`, `sweet_berries`, `honey_bottle`

### Boats and minecarts (22)

`chest_minecart`, `furnace_minecart`, `tnt_minecart`, `hopper_minecart`, `oak_chest_boat`, `spruce_boat`, `spruce_chest_boat`, `birch_boat`, `birch_chest_boat`, `jungle_boat`, `jungle_chest_boat`, `acacia_boat`, `acacia_chest_boat`, `cherry_boat`, `cherry_chest_boat`, `dark_oak_boat`, `dark_oak_chest_boat`, `mangrove_boat`, `mangrove_chest_boat`, `bamboo_raft`, `bamboo_chest_raft`, `command_block_minecart`

### Signs (21)

`spruce_sign`, `birch_sign`, `jungle_sign`, `acacia_sign`, `cherry_sign`, `dark_oak_sign`, `mangrove_sign`, `bamboo_sign`, `crimson_sign`, `warped_sign`, `oak_hanging_sign`, `spruce_hanging_sign`, `birch_hanging_sign`, `jungle_hanging_sign`, `acacia_hanging_sign`, `cherry_hanging_sign`, `dark_oak_hanging_sign`, `mangrove_hanging_sign`, `bamboo_hanging_sign`, `crimson_hanging_sign`, `warped_hanging_sign`

### Buckets (6)

`powder_snow_bucket`, `pufferfish_bucket`, `salmon_bucket`, `cod_bucket`, `axolotl_bucket`, `tadpole_bucket`

### Colour sets (16 dyes each) (16)

`white_banner`, `orange_banner`, `magenta_banner`, `light_blue_banner`, `yellow_banner`, `lime_banner`, `pink_banner`, `gray_banner`, `light_gray_banner`, `cyan_banner`, `purple_banner`, `blue_banner`, `brown_banner`, `green_banner`, `red_banner`, `black_banner`

### Plants, crops and flowers (2)

`torchflower_seeds`, `pitcher_pod`

### Workstations and storage (1)

`writable_book`

### Stone, brick and deepslate sets (4)

`quartz`, `brick`, `prismarine_shard`, `prismarine_crystals`

### Everything else (30)

`charcoal`, `amethyst_shard`, `raw_iron`, `raw_gold`, `painting`, `clay_ball`, `bundle`, `glow_ink_sac`, `filled_map`, `experience_bottle`, `fire_charge`, `written_book`, `glow_item_frame`, `map`, `enchanted_book`, `rabbit_foot`, `rabbit_hide`, `armor_stand`, `end_crystal`, `dragon_breath`, `splash_potion`, `lingering_potion`, `iron_nugget`, `knowledge_book`, `debug_stick`, `phantom_membrane`, `nautilus_shell`, `heart_of_the_sea`, `goat_horn`, `echo_shard`

## Entities

1.21.1 has **130**. Browsercraft has **81**. Missing: **49**.

Every mob, and the things that are entities without being mobs: projectiles, vehicles, item frames, paintings, armour stands, falling blocks, lightning.

`area_effect_cloud`, `armadillo`, `armor_stand`, `block_display`, `chest_boat`, `chest_minecart`, `command_block_minecart`, `donkey`, `dragon_fireball`, `egg`, `ender_pearl`, `evoker_fangs`, `experience_bottle`, `falling_block`, `firework_rocket`, `furnace_minecart`, `giant`, `glow_item_frame`, `glow_squid`, `hopper_minecart`, `illusioner`, `interaction`, `item_display`, `ominous_item_spawner`, `fireball`, `leash_knot`, `lightning_bolt`, `llama_spit`, `marker`, `mooshroom`, `mule`, `ocelot`, `painting`, `shulker_bullet`, `skeleton_horse`, `sniffer`, `snowball`, `spawner_minecart`, `spectral_arrow`, `tadpole`, `text_display`, `tnt_minecart`, `trader_llama`, `trident`, `tropical_fish`, `wither_skull`, `zoglin`, `zombie_horse`, `fishing_bobber`

## Status effects

1.21.1 has **39**. Browsercraft has **11**. Missing: **28**.

`haste`, `mining_fatigue`, `instant_health`, `instant_damage`, `nausea`, `resistance`, `invisibility`, `blindness`, `hunger`, `wither`, `health_boost`, `absorption`, `saturation`, `glowing`, `levitation`, `luck`, `unluck`, `slow_falling`, `conduit_power`, `dolphins_grace`, `hero_of_the_village`, `darkness`, `trial_omen`, `raid_omen`, `wind_charged`, `weaving`, `oozing`, `infested`

## Potions

1.21.1 has **46**. Browsercraft has **7**. Missing: **39**.

Long and strong forms (`long_swiftness`, `strong_swiftness`) are separate potions in 1.21.1.

`mundane`, `thick`, `long_night_vision`, `invisibility`, `long_invisibility`, `leaping`, `long_leaping`, `strong_leaping`, `long_fire_resistance`, `long_swiftness`, `strong_swiftness`, `slowness`, `long_slowness`, `strong_slowness`, `turtle_master`, `long_turtle_master`, `strong_turtle_master`, `water_breathing`, `long_water_breathing`, `healing`, `strong_healing`, `harming`, `strong_harming`, `poison`, `long_poison`, `strong_poison`, `long_regeneration`, `strong_regeneration`, `long_strength`, `strong_strength`, `weakness`, `long_weakness`, `luck`, `slow_falling`, `long_slow_falling`, `wind_charged`, `weaving`, `oozing`, `infested`

## Enchantments

1.21.1 has **42**. Browsercraft has **7**. Missing: **35**.

`fire_protection`, `feather_falling`, `blast_protection`, `projectile_protection`, `respiration`, `aqua_affinity`, `thorns`, `depth_strider`, `frost_walker`, `binding_curse`, `soul_speed`, `swift_sneak`, `smite`, `bane_of_arthropods`, `knockback`, `fire_aspect`, `looting`, `sweeping_edge`, `punch`, `flame`, `infinity`, `luck_of_the_sea`, `lure`, `loyalty`, `impaling`, `riptide`, `channeling`, `multishot`, `quick_charge`, `piercing`, `density`, `breach`, `wind_burst`, `mending`, `vanishing_curse`

## Biomes

1.21.1 has **64**. Browsercraft has **14**. Missing: **50**.

`the_void`, `sunflower_plains`, `ice_spikes`, `mangrove_swamp`, `flower_forest`, `birch_forest`, `dark_forest`, `old_growth_birch_forest`, `old_growth_pine_taiga`, `old_growth_spruce_taiga`, `taiga`, `snowy_taiga`, `savanna_plateau`, `windswept_gravelly_hills`, `windswept_forest`, `windswept_savanna`, `sparse_jungle`, `bamboo_jungle`, `badlands`, `eroded_badlands`, `wooded_badlands`, `meadow`, `cherry_grove`, `grove`, `snowy_slopes`, `frozen_peaks`, `jagged_peaks`, `stony_peaks`, `river`, `frozen_river`, `snowy_beach`, `stony_shore`, `warm_ocean`, `lukewarm_ocean`, `deep_lukewarm_ocean`, `deep_ocean`, `cold_ocean`, `deep_cold_ocean`, `frozen_ocean`, `deep_frozen_ocean`, `mushroom_fields`, `deep_dark`, `warped_forest`, `crimson_forest`, `soul_sand_valley`, `basalt_deltas`, `end_highlands`, `end_midlands`, `small_end_islands`, `end_barrens`

## Structures

1.21.1 has **34**. Browsercraft has **17**. Missing: **17**.

Village and ruined portal variants are listed separately in 1.21.1.

`mineshaft_mesa`, `shipwreck_beached`, `ocean_ruin_cold`, `ocean_ruin_warm`, `nether_fossil`, `buried_treasure`, `village_desert`, `village_savanna`, `village_snowy`, `village_taiga`, `ruined_portal_desert`, `ruined_portal_jungle`, `ruined_portal_swamp`, `ruined_portal_mountain`, `ruined_portal_ocean`, `ruined_portal_nether`, `trail_ruins`

## Container screens

1.21.1 has **25**. Browsercraft has **10**. Missing: **15**.

Dispensers, droppers, hoppers, shulker boxes and the beacon exist as blocks here but have no screen to open.

`generic_9x1`, `generic_9x2`, `generic_9x4`, `generic_9x5`, `generic_9x6`, `generic_3x3`, `beacon`, `grindstone`, `hopper`, `lectern`, `loom`, `shulker_box`, `smithing`, `cartography_table`, `stonecutter`

## Entity models

Every one of Browsercraft's 53 species has a model, but all of them were built from Mojang's **Bedrock** geometry (`bedrock-samples`) or by hand -- none from the 1.21.1 Java model classes in this source. Where the editions differ, Browsercraft has the Bedrock shape. 1.21.1 draws its entities with **87** model classes; these have no Browsercraft equivalent at all:

`ArmadilloEntityModel`, `SnifferEntityModel`, `OcelotEntityModel`, `DonkeyEntityModel (donkey, mule)`, `TadpoleEntityModel`, `SmallTropicalFishEntityModel`, `LargeTropicalFishEntityModel`, `SmallPufferfishEntityModel / MediumPufferfishEntityModel (only one puff state here)`, `GiantEntityModel`, `ArmorStandEntityModel`, `ArmorStandArmorEntityModel`, `EvokerFangsEntityModel`, `LlamaSpitEntityModel`, `ShulkerBulletEntityModel`, `LeashKnotEntityModel`, `TridentEntityModel (thrown)`, `ElytraEntityModel (worn)`, `ChestBoatEntityModel`, `RaftEntityModel`, `ChestRaftEntityModel`, `BookModel (the enchanting table's book)`, `SkullEntityModel / DragonHeadEntityModel / PiglinHeadEntityModel (heads as blocks)`, `ArmorEntityModel (worn armour drawn on mobs)`

And these are drawn here, but not from the 1.21.1 class: every mob model, `BoatEntityModel`, `MinecartEntityModel`, `ShieldEntityModel`, `SheepWoolEntityModel`, `PlayerEntityModel`.

