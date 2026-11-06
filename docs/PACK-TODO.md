# Pack to-do

Everything in the 1.21 textures zip the game doesn't use yet, turned into work. Checked with `python3 tools/pack-audit.py assets/textures all` (954 of 3086 used when this was written). Each step ends with "works like Minecraft does it": the right look, the right behaviour, and the right recipe or drop from the 1.21.1 data zip.

## Part 1: art for things the game already has

1. Link the missing block faces (done)
   - I. Observer back, back lit and top. Dispenser and dropper vertical fronts. Hopper inside. Barrel open top and bottom.
   - II. Crafter faces for each direction, plus its triggered and crafting states.
   - III. Furnace side, smoker bottom, enchanting table bottom, brewing stand base, cactus bottom, sandstone bottom, campfire lit logs, cauldron top, reinforced deepslate bottom.
   - IV. Trial spawner and vault states: ominous, active, inactive and ejecting reward.
   - V. Sculk sensor bottom and tendrils, sculk catalyst bloom, sculk shrieker inner tops.
   - VI. Nether wart stages 0 and 1, chipped and damaged anvil, copper bulb lit and powered.
2. Flowing water and lava (done)
   - I. Use water_flow and lava_flow on moving fluid and on the sides of fluid.
   - II. Use water_overlay where water meets glass, leaves and other see-through blocks.
   - III. Make the flow scroll the way water flows downhill, at Minecraft's speed.
3. Pointed dripstone and amethyst growth
   - I. Build dripstone columns from base, frustum, middle, tip and tip-merge pieces, by where each piece sits in the column.
   - II. Grow small, medium and large amethyst buds on budding amethyst until they become a cluster.
   - III. Run drips through all three stages: hang, fall, land.
4. Redstone dust and the lever
   - I. Make dust join up like Minecraft's: a dot, straight lines or crosses, chosen from its neighbours, with the overlay on top.
   - II. Give the lever Minecraft's model: a cobblestone base and the pack's handle.
5. Item icons the game still paints itself
   - I. Armour icons (16 of them), with the leather pieces tinted by their dye.
   - II. Item icons for doors, campfire, cauldron, hopper, end crystal, pointed dripstone and bamboo.
   - III. Bow pulling frames 0 to 2 while drawing the bow.
   - IV. The potion overlay, coloured by what's brewed in it.
   - V. Empty-slot ghosts for every slot that has one: ingot, shard, sword, pickaxe, axe, hoe, shovel, redstone, quartz, diamond, emerald, and the smithing templates.
6. Extra layers for mobs the game has
   - I. Glowing eyes for enderman, spider, cave spider, phantom and dragon. Warden glow, heart and pulsing spots.
   - II. Iron golem cracks that grow as it loses health.
   - III. Wither armour below half health, and the blue invulnerable phase when it spawns.
   - IV. Ghast shooting face, breeze eyes and wind, drowned outer layer, bogged and stray overlays, charging vex.
   - V. Pig and strider saddles (needs the saddle in item 49), and the cold strider.
7. Variants for mobs the game has
   - I. Wolves: 9 coats, the tame and angry faces, a collar tinted by dye, wolf armour and its cracks.
   - II. Cats: 11 breeds plus a collar. Ocelots.
   - III. Horses: 7 colours times 5 markings. Donkey, mule, skeleton horse and zombie horse.
   - IV. Llamas: 4 coats, the 16 carpet decors and the trader llama.
   - V. Rabbits (7), parrots (5), axolotls (5), frogs (cold and warm), foxes (snowy and sleeping), pandas (7 moods), bees (angry, nectar, stinger), tropical fish (2 shapes, 6 patterns, colours).
   - VI. Mooshrooms, red and brown.
8. Villager jobs
   - I. Add the other 10 jobs, each with its job-site block and trades: armorer, butcher, cartographer, cleric, fisherman, leatherworker, mason, nitwit, shepherd, weaponsmith.
   - II. Zombie villagers wear the biome, job and rank layers.
   - III. Curing a zombie villager gives discounts, shown with the strikethrough price.
9. Beams and attack effects
   - I. The beacon beam.
   - II. The End gateway beam, and the End crystal beam that heals the dragon.
   - III. The guardian laser.
   - IV. The dragon fireball, and the dragon's death explosion.
   - V. The shulker bullet.
10. Enchanting table book
    - I. A floating book above the table.
    - II. It opens when a player is near, turns pages and follows the player, like Minecraft.
11. Elytra
    - I. Wings on the player's back when worn.
    - II. Use the broken elytra icon when it's out of durability.
12. Shadows: round shadows under mobs, items and players (misc/shadow).
13. Enchantment glint
    - I. A moving glint on enchanted items in slots and in the hand.
    - II. The glint on worn enchanted armour (entity glint).
14. Underwater overlay: the misc/underwater tint when your head is in water.
15. Player skins: offer the pack's 9 default skins in the character picker, in wide and slim.
16. Horse riding UI
    - I. The horse inventory screen: saddle, armour and chest slots (including the llama armour slot).
    - II. The horse's hearts in place of food while riding.
    - III. The jump bar: charge it, then jump.
17. HUD leftovers
    - I. The attack cooldown indicator, on the crosshair and on the hotbar.
    - II. A left-handed option, which moves the offhand slot to the right side.
    - III. The search icon in the creative search box, and the unfocused text field.
    - IV. Boss bar colours and notched styles for other bosses and raids.
18. Gamemode switcher: the F3+F4 overlay with its slots and selection.
19. Beacon screen
    - I. Pick primary and secondary powers with the beacon buttons, confirm and cancel.
    - II. Add Haste and Resistance so every beacon power exists.
20. Grindstone screen
    - I. Repair two of the same item.
    - II. Strip enchantments and pay back XP.
    - III. Show the error icon when it can't.
21. Fix the audit tool so names the game builds in code (destroy stages and other numbered textures) count as used.

## Part 2: new blocks

22. The six existing woods
    - I. Saplings that grow into each tree, the 2x2 dark oak and jungle trees included.
    - II. Trapdoors for all six woods. Doors for spruce, birch, jungle, acacia and dark oak.
    - III. Stripped logs, stripped by right-clicking with an axe.
    - IV. Birch planks.
    - V. Signs for every wood.
23. Cherry wood
    - I. Logs, leaves, planks, door, trapdoor, sapling and sign.
    - II. The cherry grove biome, pink petals and falling-petal particles.
24. Mangrove
    - I. Logs, roots, muddy roots, leaves, propagule (hanging and planted), planks, door and trapdoor.
    - II. Mud bricks and packed mud.
    - III. The mangrove swamp biome.
25. Bamboo wood: bamboo block (and stripped), planks, mosaic, door, trapdoor, sign, raft and chest raft.
26. Nether forests
    - I. Crimson and warped stems, nylium, fungus (grown into huge fungus with bone meal), roots, sprouts and wart blocks.
    - II. Weeping vines and twisting vines. Shroomlight.
    - III. Planks, doors, trapdoors and signs for both.
    - IV. The crimson forest and warped forest biomes.
27. Stone sets
    - I. Stone bricks, plus mossy, cracked and chiseled. Smooth stone and its slab.
    - II. Polished granite, diorite and andesite.
    - III. Deepslate: cobbled, polished, chiseled, plus cracked bricks and tiles.
    - IV. Tuff: polished, chiseled, chiseled bricks.
    - V. Blackstone: top face, polished, chiseled, cracked bricks.
    - VI. Red nether bricks, cracked and chiseled nether bricks.
28. Quartz: nether quartz ore, the quartz item, quartz block, bricks, pillar and chiseled.
29. Sand and sandstone: red sand, the full red sandstone set, cut and chiseled sandstone.
30. Ores: deepslate versions of all 8 ores, nether gold ore, raw iron and raw gold, and raw ore blocks.
31. Colours
    - I. Concrete and concrete powder, 16 colours each. Powder hardens in water.
    - II. Terracotta, plain and in 16 colours. Glazed terracotta in 16, which faces where you place it.
    - III. Stained glass and panes in 16 colours. Glass panes that connect like Minecraft's. Tinted glass.
32. Candles in 16 colours: lit and unlit, 1 to 4 to a block, and candle cakes.
33. Coloured shulker boxes in all 16 colours, plus shulker mobs in those colours.
34. Beds in all 16 colours.
35. Chests
    - I. Two chests side by side join into a double chest, using the left and right textures and the 54-slot screen.
    - II. Trapped chest, ender chest (shared storage), and the Christmas chest in late December.
36. Copper set
    - I. Cut, chiseled, grate, door and trapdoor, at all four weathering stages.
    - II. Copper bulbs at every stage, lit and powered.
    - III. Waxing with honeycomb, scraping with an axe, and the lightning rod with lightning.
37. Flowers and plants
    - I. Allium, azure bluet, blue orchid, cornflower, lily of the valley, oxeye daisy, four tulips, wither rose.
    - II. Torchflower (seeds and crop). Pitcher plant (pod and crop). Pink petals.
    - III. The two-block plants: sunflower, lilac, peony, rose bush, large fern, tall grass.
    - IV. Fern and dead bush. A flower pot that holds every plant.
38. Ground cover: mushrooms and huge mushrooms, mycelium, podzol, coarse dirt, rooted dirt and hanging roots.
39. Lush caves: cave vines with glow berries, spore blossom, big and small dripleaf (big ones tilt when stood on), azalea and flowering azalea, glow lichen.
40. Farm blocks
    - I. Pumpkin stems, attached and unattached, and pumpkin seeds.
    - II. Carved pumpkin and jack o'lantern. Wearing a carved pumpkin shows pumpkinblur.
    - III. Hay bale, sweet berry bush, dried kelp block, bone block.
    - IV. Bee nest and beehive with honey levels. Honey bottle, honey block and honeycomb block.
41. Ocean
    - I. Tall seagrass, coral fans and wall fans, dead corals and coral blocks.
    - II. The conduit and conduit power.
    - III. Turtle eggs that hatch, and the turtle helmet.
42. Ice and snow
    - I. Packed ice, blue ice, and frosted ice that melts in stages.
    - II. Powder snow: you sink into it, the outline overlay shows, freezing hearts, and the powder snow bucket.
43. Redstone
    - I. Repeater, comparator and daylight detector.
    - II. Target block, tripwire and tripwire hook.
    - III. Powered, detector and activator rails.
    - IV. Iron door, iron trapdoor and wooden trapdoors that open with redstone.
44. Utility and decoration
    - I. Iron bars, chain and end rod.
    - II. Soul torch, soul fire, soul campfire and soul soil.
    - III. Fire as a block that spreads, burns out and lights things (fire_0 and fire_1).
    - IV. Magma block, crying obsidian, respawn anchor (charges 0 to 4) and lodestone.
    - V. Bell, and the jukebox with music discs. Chiseled bookshelf. Cobweb.
    - VI. The dragon egg on the dragon's death. Heavy core.
45. Archaeology
    - I. Suspicious sand and gravel, and the brush.
    - II. Pottery sherds, and decorated pots with a pattern on each side.
    - III. The sniffer, sniffer eggs, torchflower and pitcher seeds.
    - IV. Trail ruins, from the data zip.
46. New workstations, each a block plus its screen
    - I. Loom: banners and patterns.
    - II. Stonecutter: the recipe list.
    - III. Smithing table: netherite upgrade and trims.
    - IV. Cartography table: maps.
    - V. Fletching table: just the block (it's the fletcher's job site).
47. Sculk extras: calibrated sculk sensor and sculk vein.
48. Creative-only blocks: command blocks (normal, chain, repeating), structure block, jigsaw, barrier, light block, structure void.

## Part 3: new items

49. Tools and gear
    - I. Shears, fishing rod with fishing and the hook, carrot and warped fungus on a stick.
    - II. Lead with its knot, name tag and saddle.
    - III. Spyglass with its scope overlay, compass (32 frames), clock (64 frames), recovery compass.
    - IV. Brush, goat horn, and the bundle with its tooltip.
50. Combat
    - I. Crossbow: pulling, loaded arrow and loaded firework.
    - II. Spectral and tipped arrows, item and entity.
    - III. Throwable trident with riptide.
    - IV. Chainmail armour with its layers.
    - V. Horse armour (4 kinds), wolf armour, armadillo scute.
51. Food
    - I. Golden apple (absorption hearts), golden carrot, glistering melon.
    - II. Mushroom stew, rabbit stew, suspicious stew, pumpkin pie.
    - III. Rotten flesh (hunger icons), spider eye, fermented spider eye, poisonous potato.
    - IV. Chorus fruit (it teleports you) and popped chorus fruit. Dried kelp, sweet berries, honey bottle, tropical fish.
52. Drops and materials: rabbit hide and foot, phantom membrane, prismarine shard and crystals, nautilus shell, heart of the sea, echo shard, disc fragment, dragon's breath, glow ink sac, iron nugget, charcoal, clay ball, quartz, amethyst shard, bottle o' enchanting.
53. Buckets: cod, salmon, pufferfish, tropical fish, axolotl and tadpole buckets, and the powder snow bucket.
54. Books: book and quill, written book (with the book screen and page buttons), enchanted books for the anvil, knowledge book.
55. Vehicles: chest, furnace, hopper, TNT and command block minecarts. A boat and chest boat for every wood.
56. Spawn eggs for every mob, coloured by mob.
57. Fireworks: rockets, stars, sparks, flash and glitter, and firing them from a crossbow.
58. Music discs (19), played in the jukebox.
59. Templates and patterns: banner pattern items, 18 armour trim templates and the netherite upgrade template.
60. Splash and lingering potions, with a colour for every brew.

## Part 4: new mobs

61. Armadillo: rolls up when scared, drops scutes.
62. Sniffer: digs up seeds, hatches from its egg.
63. Glow squid, which drops glow ink.
64. Zoglin, illusioner and evoker fangs.
65. Tadpole and trader llama look right (check they use the pack art), and llama spit.

## Part 5: decorations that are entities

66. Paintings
    - I. Get the geometry right: a frame of the right depth with the back texture behind.
    - II. Paintings are 1x1 up to 4x4 like Minecraft's, and placing one picks the biggest that fits.
    - III. Add all the paintings from the pack.
    - IV. Break and drop like Minecraft, and pop off when the wall goes.
67. Armour stands
    - I. Get the geometry and shape right: base, body, arms, head.
    - II. Make it work like an armour stand: wear armour, hold items, swap items with right-click, break in two hits.
    - III. Pose it, and let it be pushed, like Minecraft.
68. Item frames: add the glow item frame.
69. Banners
    - I. Standing and wall banners in 16 base colours.
    - II. The 42 patterns, layered with the loom.
    - III. Banner patterns on shields.
70. Signs: a sign for every wood, hanging signs, and the hanging sign editing screen.
71. Maps
    - I. Empty map to filled map, drawn from the world.
    - II. Map decorations: player, banners, structures and markers (37 of them).
    - III. Copying, locking and zooming at the cartography table. Maps in item frames.
72. Armour trims: 55 trim textures over armour, applied at the smithing table.

## Part 6: effects

73. The 28 new status effects, with icons, the potions or sources that give them, and what they do: absorption, blindness, conduit power, darkness, dolphin's grace, glowing, haste, health boost, hero of the village, hunger, infested, instant health, instant damage, invisibility, levitation, luck, bad luck, mining fatigue, nausea (with the screen overlay), oozing, raid omen, resistance, saturation, slow falling, trial omen, weaving, wind charged, wither (withered hearts).
74. Heart and food styles: absorbing, frozen and withered hearts, and the hunger food icons.

## Part 7: screens and menus

75. Recipe book: crafting and furnace books, filters, tabs, and the ghost recipe in the grid.
76. Advancements: the screen with its tabs, frames and backgrounds, plus toasts when you earn one.
77. Statistics screen.
78. Worlds
    - I. A world list with more than one save.
    - II. A Create World screen with tabs, checkboxes and presets (flat, isles and the others).
79. Server list: LAN and saved addresses, ping bars, join and reorder buttons, and the unknown-server icon.
80. Resource pack screen: pick and order installed packs (the transferable list), with the unknown-pack icon.
81. Tutorial toasts: movement keys, mouse, punch a tree, make planks, open the recipe book.
82. Spectator mode and its menu.
83. Language and accessibility buttons on the title screen, with their screens.
84. Smaller widgets
    - I. Scrolling lists with the pack scroller (controls screen) and the list backgrounds.
    - II. The difficulty lock button. Checkboxes. Cross buttons.

## Part 8: text

85. Fonts
    - I. Accented and other non-Latin characters in chat and on signs.
    - II. The galactic alphabet (ascii_sga) on enchanting offers, like Minecraft.
    - III. Illager runes (asciillager).

## Part 9: world

86. A world border, shown with the forcefield texture.
87. Find out what effect/dither is for, and use it.
88. Biomes from the data zip (it has 74, the game has 10). Many items above need theirs: cherry grove, mangrove swamp, nether forests, and the rest.

## Not planned on purpose

- The Minecraft logo, "Minceraft", Java Edition, Mojang Studios and Realms title art. The game is Browsercraft.
- Realms, social interactions and reporting, notifications, invites, popups, operator buttons, news and video links, demo mode. These are Mojang online services, and there's nothing to connect them to.
