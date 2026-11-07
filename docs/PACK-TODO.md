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
3. Pointed dripstone and amethyst growth (done)
   - I. Build dripstone columns from base, frustum, middle, tip and tip-merge pieces, by where each piece sits in the column.
   - II. Grow small, medium and large amethyst buds on budding amethyst until they become a cluster.
   - III. Run drips through all three stages: hang, fall, land.
4. Redstone dust and the lever (done)
   - I. Make dust join up like Minecraft's: a dot, straight lines or crosses, chosen from its neighbours, with the overlay on top.
   - II. Give the lever Minecraft's model: a cobblestone base and the pack's handle.
5. Item icons the game still paints itself (done)
   - I. Armour icons (16 of them), with the leather pieces tinted by their dye.
   - II. Item icons for doors, campfire, cauldron, hopper, end crystal, pointed dripstone and bamboo.
   - III. Bow pulling frames 0 to 2 while drawing the bow.
   - IV. The potion overlay, coloured by what's brewed in it.
   - V. Empty-slot ghosts for every slot that has one: ingot, shard, sword, pickaxe, axe, hoe, shovel, redstone, quartz, diamond, emerald, and the smithing templates.
   - Left for later: the smithing ghosts go in with the smithing table (46), and dyed leather needs a colour on the item, which goes in with armour trims (72).
6. Extra layers for mobs the game has (done)
   - I. Glowing eyes for enderman, spider, cave spider, phantom and dragon. Warden glow, heart and pulsing spots.
   - II. Iron golem cracks that grow as it loses health.
   - III. Wither armour below half health, and the blue invulnerable phase when it spawns.
   - IV. Ghast shooting face, breeze eyes and wind, drowned outer layer, bogged and stray overlays, charging vex.
   - V. Pig and strider saddles (needs the saddle in item 49), and the cold strider.
   - Left for later: the saddles go in with the saddle (49), and the breeze's wind swirl is its own model, which goes in with the beams (9).
7. Variants for mobs the game has (done)
   - I. Wolves: 9 coats, the tame and angry faces, a collar tinted by dye, wolf armour and its cracks.
   - II. Cats: 11 breeds plus a collar. Ocelots.
   - III. Horses: 7 colours times 5 markings. Donkey, mule, skeleton horse and zombie horse.
   - IV. Llamas: 4 coats, the 16 carpet decors and the trader llama.
   - V. Rabbits (7), parrots (5), axolotls (5), frogs (cold and warm), foxes (snowy and sleeping), pandas (7 moods), bees (angry, nectar, stinger), tropical fish (2 shapes, 6 patterns, colours).
   - VI. Mooshrooms, red and brown.
   - Left for later, since each is a new mob rather than a new coat: ocelots, donkeys, mules, skeleton and zombie horses (with 61-65), mooshrooms (with 38), tropical fish (with 53). Llama carpets and the trader llama go with carpets (31), wolf armour with 50, and bee nectar with bees and honey (40).
8. Villager jobs (done)
   - I. Add the other 10 jobs, each with its job-site block and trades: armorer, butcher, cartographer, cleric, fisherman, leatherworker, mason, nitwit, shepherd, weaponsmith.
   - II. Zombie villagers wear the biome, job and rank layers.
   - III. Curing a zombie villager gives discounts, shown with the strikethrough price.
   - Left for later: curing needs the golden apple (51) and splash weakness (60), so it goes in with those. The jobs whose stations are not in yet (cartographer, mason, shepherd, toolsmith, fletcher) are handed out by seed until 46 adds the stations.
9. Beams and attack effects (done)
   - I. The beacon beam.
   - II. The End gateway beam, and the End crystal beam that heals the dragon.
   - III. The guardian laser.
   - IV. The dragon fireball, and the dragon's death explosion.
   - V. The shulker bullet.
   - Left for later: the breeze's wind swirl, which needs its own model.
10. Enchanting table book (done)
    - I. A floating book above the table.
    - II. It opens when a player is near, turns pages and follows the player, like Minecraft.
11. Elytra (done)
    - I. Wings on the player's back when worn.
    - II. Use the broken elytra icon when it's out of durability.
12. Shadows: round shadows under mobs, items and players (misc/shadow). (done)
13. Enchantment glint (done)
    - I. A moving glint on enchanted items in slots and in the hand.
    - II. The glint on worn enchanted armour (entity glint).
14. Underwater overlay: the misc/underwater tint when your head is in water. (done)
15. Player skins: offer the pack's 9 default skins in the character picker, in wide and slim. (done)
16. Horse riding UI (done)
    - I. The horse inventory screen: saddle, armour and chest slots (including the llama armour slot).
    - II. The horse's hearts in place of food while riding.
    - III. The jump bar: charge it, then jump.
    - Left for later: chest slots for donkeys and mules and the llama carpet slot (no donkeys, mules or llama riding yet); the saddle drawn on the horse model goes with item 49.
17. HUD leftovers (done)
    - I. The attack cooldown indicator, on the crosshair and on the hotbar.
    - II. A left-handed option, which moves the offhand slot to the right side.
    - III. The search icon in the creative search box, and the unfocused text field.
    - IV. Boss bar colours and notched styles for other bosses and raids.
    - Left for later: the creative search tab's compass icon waits for the compass (item 49); the plain text_field sprite gets used once a screen has more than one field (item 78); raids use the red notched bar once raids exist.
18. Gamemode switcher: the F3+F4 overlay with its slots and selection. (done)
    - Adventure and spectator now exist so all four slots work. Spectator here is the core of it (fly through blocks, no body, no hotbar, mobs ignore you); the spectator menu stays with item 82.
19. Beacon screen (done)
    - I. Pick primary and secondary powers with the beacon buttons, confirm and cancel.
    - II. Add Haste and Resistance so every beacon power exists.
20. Grindstone screen (done)
    - I. Repair two of the same item.
    - II. Strip enchantments and pay back XP.
    - III. Show the error icon when it can't.
21. Fix the audit tool so names the game builds in code (destroy stages and other numbered textures) count as used. (done)

## Part 2: new blocks

22. The six existing woods (done)
    - I. Saplings that grow into each tree, the 2x2 dark oak and jungle trees included.
    - II. Trapdoors for all six woods. Doors for spruce, birch, jungle, acacia and dark oak.
    - III. Stripped logs, stripped by right-clicking with an axe.
    - IV. Birch planks.
    - V. Signs for every wood.
    - Left for later: wooden stairs, slabs and fences for the five newer woods, and the mega spruce from four spruce saplings.
23. Cherry wood (done)
    - I. Logs, leaves, planks, door, trapdoor, sapling and sign.
    - II. The cherry grove biome, pink petals and falling-petal particles.
24. Mangrove (done)
    - I. Logs, roots, muddy roots, leaves, propagule (hanging and planted), planks, door and trapdoor.
    - II. Mud bricks and packed mud.
    - III. The mangrove swamp biome.
25. Bamboo wood: bamboo block (and stripped), planks, mosaic, door, trapdoor, sign, raft and chest raft. (done)
    - Left for later: the raft and chest raft go with the boats for every wood in item 55, which builds the raft's own flat model.
26. Nether forests (done)
    - I. Crimson and warped stems, nylium, fungus (grown into huge fungus with bone meal), roots, sprouts and wart blocks.
    - II. Weeping vines and twisting vines. Shroomlight.
    - III. Planks, doors, trapdoors and signs for both.
    - IV. The crimson forest and warped forest biomes.
27. Stone sets (done)
    - I. Stone bricks, plus mossy, cracked and chiseled. Smooth stone and its slab.
    - II. Polished granite, diorite and andesite.
    - III. Deepslate: cobbled, polished, chiseled, plus cracked bricks and tiles.
    - IV. Tuff: polished, chiseled, chiseled bricks.
    - V. Blackstone: top face, polished, chiseled, cracked bricks.
    - VI. Red nether bricks, cracked and chiseled nether bricks.
28. Quartz: nether quartz ore, the quartz item, quartz block, bricks, pillar and chiseled. (done)
29. Sand and sandstone: red sand, the full red sandstone set, cut and chiseled sandstone. (done)
    - A plain badlands biome now gives the red sand a home; its terracotta bands come with item 31.
30. Ores: deepslate versions of all 8 ores, nether gold ore, raw iron and raw gold, and raw ore blocks. (done)
31. Colours (done)
    - I. Concrete and concrete powder, 16 colours each. Powder hardens in water.
    - II. Terracotta, plain and in 16 colours. Glazed terracotta in 16, which faces where you place it.
    - III. Stained glass and panes in 16 colours. Glass panes that connect like Minecraft's. Tinted glass.
    - Notes: badlands now have terracotta bands under the red sand. Amethyst shards came with tinted glass (clusters drop 4, four make a block). Glazed terracotta has one facing for now; turning it to face the placer is still to do.
32. Candles in 16 colours: lit and unlit, 1 to 4 to a block, and candle cakes. (done)
    - Notes: candles are real 3D models now, with a baked flame when lit. Cakes got their proper half-height shape with bites taken out, since the candle cake needed it.
33. Coloured shulker boxes in all 16 colours, plus shulker mobs in those colours. (done)
    - Notes: shulkers carry a colour like 1.21's (undyed by default, as end cities spawn them). Boxes still spill their contents when broken rather than keeping them in the item; that needs item data the game does not have yet.
34. Beds in all 16 colours. (done)
    - Notes: beds are two blocks now, head and foot, facing the way you place them, drawn from the pack's bed sheets. Sleeping in the Nether or the End blows the bed up. Old single-block beds in saved worlds still work and break into a red bed.
35. Chests (done)
    - I. Two chests side by side join into a double chest, using the left and right textures and the 54-slot screen.
    - II. Trapped chest, ender chest (shared storage), and the Christmas chest in late December.
    - Notes: chests are 3D models cut from the pack's chest sheets and face the player. The tripwire hook arrived as a crafting item for the trapped chest; it becomes a placeable block with item 43. Lids do not swing open yet.
36. Copper set (done)
    - I. Cut, chiseled, grate, door and trapdoor, at all four weathering stages.
    - II. Copper bulbs at every stage, lit and powered.
    - III. Waxing with honeycomb, scraping with an axe, and the lightning rod with lightning.
    - Notes: cut copper slabs came along since chiseled copper is made from them; cut copper stairs are still to do. Storms now throw a visible bolt, and rods draw it, give a pulse of power and clean the copper they stand on. Lightning does not start fires or change mobs yet.
37. Flowers and plants (done)
    - I. Allium, azure bluet, blue orchid, cornflower, lily of the valley, oxeye daisy, four tulips, wither rose.
    - II. Torchflower (seeds and crop). Pitcher plant (pod and crop). Pink petals.
    - III. The two-block plants: sunflower, lilac, peony, rose bush, large fern, tall grass.
    - IV. Fern and dead bush. A flower pot that holds every plant.
    - Notes: clay balls and bricks came in for the flower pot, so a clay block now breaks into four balls and fires into terracotta. Torchflower seeds and pitcher pods wait for the sniffer (item 62). Wither roses grow where things die near the wither; the wither effect they give on touch comes with item 73. Mushrooms and azaleas get their pots with items 38 and 39.
38. Ground cover: mushrooms and huge mushrooms, mycelium, podzol, coarse dirt, rooted dirt and hanging roots. (done)
    - Notes: mushroom fields now rise as islands out of deep sea, with mycelium, huge mushrooms and no hostile spawns at night. Podzol patches lie in snowy and jungle ground, coarse dirt in savanna, and rooted dirt with hanging roots in lush cave roofs. Mooshrooms and mycelium spreading are still to do; there is no hoe yet for tilling coarse or rooted dirt.
39. Lush caves: cave vines with glow berries, spore blossom, big and small dripleaf (big ones tilt when stood on), azalea and flowering azalea, glow lichen. (done)
    - Notes: moss carpet came along for the cave floors. Azalea trees with rooted dirt now mark lush caves from the surface. Spore blossoms do not shed particles yet, and glow lichen drops itself until shears exist (item 49).
40. Farm blocks (done)
    - I. Pumpkin stems, attached and unattached, and pumpkin seeds.
    - II. Carved pumpkin and jack o'lantern. Wearing a carved pumpkin shows pumpkinblur.
    - III. Hay bale, sweet berry bush, dried kelp block, bone block.
    - IV. Bee nest and beehive with honey levels. Honey bottle, honey block and honeycomb block.
    - Notes: ripe melon and pumpkin stems now bend toward the fruit they grew and straighten when it is taken. Shears arrived early as an item: they carve pumpkins and collect honeycomb, but do not wear out or shear sheep until item 49. Dried kelp came in ahead of item 51 so the kelp block has something to be made from. Sweet berry bushes grow in snowy ground and on podzol, sting and slow you, and berries plant them. Bee nests hang under plains and forest trees; honey builds while bees are within 16 blocks, but bees do not fly into hives yet. Honey blocks slow you; they do not stick to walls or move with pistons yet.
41. Ocean (done)
    - I. Tall seagrass, coral fans and wall fans, dead corals and coral blocks.
    - II. The conduit and conduit power.
    - III. Turtle eggs that hatch, and the turtle helmet.
    - Notes: coral dies to its grey form when no water touches it; coral blocks break into dead blocks, and fans and plants need silk touch. Bone meal turns seagrass tall, and reefs now grow fans on their tops and sides. A conduit framed by at least sixteen prismarine or sea lantern blocks gives conduit power (water breathing and faster digging) in water or rain, and with the full 42 it strikes hostile mobs nearby; it is drawn as its shell only, without the turning cage and eye. Nautilus shells come from drowned and buried treasure always holds a heart of the sea. Turtles fed seagrass lay eggs on nearby sand, which crack and hatch there and can be trampled; a baby turtle sheds a scute when it grows up, and five scutes make the turtle shell, which gives water breathing on entering water.
42. Ice and snow (done)
    - I. Packed ice, blue ice, and frosted ice that melts in stages.
    - II. Powder snow: you sink into it, the outline overlay shows, freezing hearts, and the powder snow bucket.
    - Notes: every ice is slippery, blue ice most of all; nine ice pack into packed ice and nine of those into blue ice, and frozen seas carry floes of both. Snowy ground grows packed ice spires and powder snow patches. Frost Walker came in as a treasure enchantment (sold on iron boots by armorers) so frosted ice has a source. Leather boots let you stand on powder snow and climb out of it, and any leather piece stops freezing; frozen through, you take a point of damage every two seconds until you get out or catch fire. Mobs do not freeze or slide yet, and there is no powder snow cauldron.
43. Redstone (done)
    - I. Repeater, comparator and daylight detector.
    - II. Target block, tripwire and tripwire hook.
    - III. Powered, detector and activator rails.
    - IV. Iron door, iron trapdoor and wooden trapdoors that open with redstone.
    - Notes: power is still on or off rather than fifteen strengths, so a comparator compares and subtracts on that basis and reads a container, cauldron, composter, cake, lectern, hive or framed eye as full or empty. Repeaters point power one way after their one to four ticks and drive the block in front; they do not lock yet. Power now also comes from a struck target, a lit daylight detector (inverted by hand), a pulled tripwire hook and a detector rail with a cart on it. Powered rails push and brake carts, activator rails throw the rider out, but powered rails do not pass power down a line. The detector rail recipe takes a stone block until pressure plates exist. Iron doors and trapdoors open only with power.
44. Utility and decoration (done)
    - I. Iron bars, chain and end rod.
    - II. Soul torch, soul fire, soul campfire and soul soil.
    - III. Fire as a block that spreads, burns out and lights things (fire_0 and fire_1).
    - IV. Magma block, crying obsidian, respawn anchor (charges 0 to 4) and lodestone.
    - V. Bell, and the jukebox with music discs. Chiseled bookshelf. Cobweb.
    - VI. The dragon egg on the dragon's death. Heavy core.
    - Notes: iron nuggets and chorus fruit (with its teleport and popped form) came in early for the chain and end rod recipes. Fire spreads from flint and steel and from other fire, but lava does not start it yet and mobs standing in it do not burn. The respawn anchor is saved with the level and dying without one now always returns you to the overworld. The jukebox is ready but has nothing to play until the discs (item 58); the lodestone has no compass to point yet (item 49). Bells ring when used and are sold by armorers; they do not warn villagers. A chiseled bookshelf fills and empties its six slots in order rather than by the slot clicked, and holds plain books only. The dragon egg sits on a bedrock pillar over the exit portal and jumps away when used, but does not fall. Heavy cores come from ominous vaults.
45. Archaeology (done)
    - I. Suspicious sand and gravel, and the brush.
    - II. Pottery sherds, and decorated pots with a pattern on each side.
    - III. The sniffer, sniffer eggs, torchflower and pitcher seeds.
    - IV. Trail ruins, from the data zip.
    - Notes: what a brushed block gives follows the 1.21.1 archaeology tables for desert pyramids, desert wells, warm and cold ocean ruins and trail ruins; the armour trim templates and the relic disc from the rare ruin table wait for items 46 and 58. Simple ocean ruins came in so warm-water suspicious sand (and with it the sniffer egg) has somewhere to be. Trail ruins are generated in the same spirit as the real ones rather than from their structure files: buried terracotta, brick and mud brick rooms full of gravel in forests, jungles and snowy woods. Decorated pots keep their patterns in the level save; they do not hold items yet. The sniffer is done here, so item 62 is covered.
46. New workstations, each a block plus its screen (done)
    - I. Loom: banners and patterns.
    - II. Stonecutter: the recipe list.
    - III. Smithing table: netherite upgrade and trims.
    - IV. Cartography table: maps.
    - V. Fletching table: just the block (it's the fletcher's job site).
    - Notes: stonecutting recipes come from the 1.21.1 data zip. Templates arrived here with their loot (structure chests, trial vaults, archaeology, the elder guardian), so item 59 is partly covered; the rib template sits in bastions because fortresses have no chests yet. Trims show on icons and on the inventory doll but not yet on the player model in the world, and the grindstone leaves them alone. The compass came in as a still icon for the empty map recipe; item 49 makes it point. Maps fill in while held, draw in the corner of the screen and save one file each; the table zooms, copies and locks them. Banners come in all sixteen colours with the 1.21.1 patterns except Mojang's logo; designs are kept in the level save so identical banners still stack, and a water cauldron washes off the top layer. The creeper charge pattern waits for mob heads, and banner tooltips do not list their layers yet.
47. Sculk extras: calibrated sculk sensor and sculk vein. (done)
    - Notes: sensors used to be decoration, so this brings vibrations in: footsteps (not sneaking, not on wool), landing, mobs walking, blocks placed and broken, doors, trapdoors, levers, chests, eating, arrows landing, deaths and explosions, each with its 1.21.1 frequency. Wool between the two stops a vibration. A sensor hears 8 blocks, rings for 1.5 seconds and powers redstone while it does; the calibrated one hears 16 and rings for half a second. Redstone here is on or off, so a powered input counts as strength 15 and the calibrated sensor then only hears frequency 15 (deaths and explosions). Both sensors are now half a block high with tendrils, as in 1.21. Veins grow on any face, come back only with silk touch, line ancient city floors, and spread with fresh sculk from a catalyst when something dies near it. Shriekers and warden summoning are still not wired to sensors.
48. Creative-only blocks: command blocks (normal, chain, repeating), structure block, jigsaw, barrier, light block, structure void. (done)
    - Notes: command blocks run the game's own chat commands, which gained /say, /setblock and /summon and take ~ coordinates from wherever they run. Impulse fires on a rising signal, repeat every tick, chain when the block pointing into it runs, and conditional only when the block behind succeeded; output goes to chat as [@: ...]. The editor is Minecraft's: the command, the previous output and the three toggles. Structure blocks save and load up to 48 cubed to the world's structures folder, with structure void left untouched on load; corner and data modes are only the block for now. The jigsaw is just the block, since nothing generates from jigsaw pools yet. Barriers, lights and structure voids are invisible, with markers shown while one is held in creative; a light item steps a light block's level. /give now finds every block, not just the first 256. Block textures stored as animation strips now take their first frame instead of being squashed, which fixes sculk and the sculk sensor sides.

## Part 3: new items

49. Tools and gear (done)
    - I. Shears, fishing rod with fishing and the hook, carrot and warped fungus on a stick.
    - II. Lead with its knot, name tag and saddle.
    - III. Spyglass with its scope overlay, compass (32 frames), clock (64 frames), recovery compass.
    - IV. Brush, goat horn, and the bundle with its tooltip.
    - Notes: fishing follows the 1.21.1 fish, junk and treasure tables, though rotten flesh waits for item 51 and there is no Luck of the Sea or Lure yet. Pigs and striders are ridden with a saddle and steered with their stick. Name tags are named at the anvil, where only name tags can be renamed for now, and the names live in the level save; mobs are not saved, so a named or leashed mob is only kept around longer, not across a reload. Lead knots are drawn as a small block of rope rather than Minecraft's knot model. Compasses point to spawn or a bound lodestone; the recovery compass came with echo shards in ancient cities, ahead of item 52. Goats ram every so often and shed a horn when they hit stone, logs, ore or packed ice; one in fifty screams. The bundle uses the 1.21.2 recipe, since 1.21.1 kept it behind an experiment, and its contents are kept in the level like banner designs.
50. Combat (done)
    - I. Crossbow: pulling, loaded arrow and loaded firework.
    - II. Spectral and tipped arrows, item and entity.
    - III. Throwable trident with riptide.
    - IV. Chainmail armour with its layers.
    - V. Horse armour (4 kinds), wolf armour, armadillo scute.
    - Notes: the crossbow's loaded-firework icon is in, but loading a firework waits for rockets in item 57. Tipped arrows carry an effect rather than a potion, since only a handful of brews exist before item 60; strays drop arrows of slowness, and crafting them from lingering potions comes with item 60. Mobs can now glow, be slowed and be poisoned; glowing is drawn as full brightness rather than an outline. Trident enchantments are kept above the usual ones in the stack and come from the enchanting table, not books. Chainmail has no recipe, as in Minecraft, and is sold by armorers; armour still only shows on the inventory doll, not on the player in the world. The four horse armours were already in. Wolf armour and the armadillo scute go with the armadillo in item 61.
51. Food (done)
    - I. Golden apple (absorption hearts), golden carrot, glistering melon.
    - II. Mushroom stew, rabbit stew, suspicious stew, pumpkin pie.
    - III. Rotten flesh (hunger icons), spider eye, fermented spider eye, poisonous potato.
    - IV. Chorus fruit (it teleports you) and popped chorus fruit. Dried kelp, sweet berries, honey bottle, tropical fish.
    - Notes: part IV was already in from earlier items. Golden apples give absorption and regeneration with the 1.21.1 strengths, and the enchanted one also resistance and fire resistance and only turns up in desert temples and bastions, as it has no recipe. Absorption hearts sit above the health row and take blows first. Rotten flesh gives hunger 80% of the time, which drains food faster and turns the food row green; spider eyes poison and poisonous potatoes do 60% of the time. Zombies, husks, drowned and zombie villagers drop rotten flesh, spiders and cave spiders drop eyes, and a ripe potato crop is poisonous about one time in fifty. Stews and beetroot soup hand the bowl back. A suspicious stew remembers the flower it was made with and gives that flower's effect for 1.21.1's time; wither is the only one not in the game yet, so the wither rose gives poison until item 73. Blindness from the stew darkens the fog only on the simple renderer for now. Effect icons now come from one place, which also fixes the HUD for the new effects.
52. Drops and materials: rabbit hide and foot, phantom membrane, prismarine shard and crystals, nautilus shell, heart of the sea, echo shard, disc fragment, dragon's breath, glow ink sac, iron nugget, charcoal, clay ball, quartz, amethyst shard, bottle o' enchanting. (done)
    - Notes: nautilus shell, heart of the sea, echo shard, iron nugget, clay ball, quartz and amethyst shard were already in. Rabbits drop hide and one time in ten a foot, phantoms their membrane, and guardians shards with the 1.21.1 cod-or-crystals roll (the elder one a wet sponge, no longer a sea lantern). Shards and crystals make prismarine, bricks, dark prismarine and sea lanterns, and a sea lantern breaks into two or three crystals (glowstone now gives two to four dust). Four hides make leather. Logs fire into charcoal, which burns like coal and stands in for it in every recipe but the coal block. Bottles o' enchanting are thrown, arc, and break into 3 to 11 experience; a master cleric sells them and they turn up in outposts, shipwrecks and ancient cities, and the cleric's trades are now 1.21.1's. Disc fragments are in ancient city chests; the disc they make comes in item 58. Dragon's breath is bottled from the dragon's breath cloud, and waits for lingering potions in item 60. The rabbit's foot brews leaping and phantom membrane mends an elytra at the anvil; slow falling waits for item 60. Experience orbs are now tinted green as in Minecraft rather than drawn white. Glow ink sac moved to item 63, since the glow squid is its only source, and glowing sign text goes with it.
53. Buckets: cod, salmon, pufferfish, tropical fish, axolotl and tadpole buckets, and the powder snow bucket. (done)
    - Notes: the powder snow bucket was already in. A water bucket used on a cod, salmon, pufferfish, tropical fish, axolotl or tadpole scoops it up, and emptying the bucket pours the water and lets it out again with the same look; released ones do not wander off and despawn. Tropical fish are new: both of Minecraft's shapes with their models, the twelve patterns, sixteen body and sixteen pattern colours tinted onto the pack's sheets, nine in ten one of the twenty-two named fish, and the bucket's name says which (Bucket of Clownfish and so on). They spawn with the other sea life, since the oceans here have no warm and cold yet. Tadpoles are new too: frogs now breed with slimeballs as in Minecraft (they used to take magma cream) and lay frogspawn on still water instead of having a baby; the spawn hatches two to six tadpoles, which grow into a frog in twenty minutes, or sooner on slimeballs, and the frog that comes out suits the climate it grew up in. Frogspawn breaks if the water under it goes, and cannot be picked up. The fit check for mob sheets now understands Mojang's paper-thin fins, which hang off the edge of the sheet in the real art too.
54. Books: book and quill, written book (with the book screen and page buttons), enchanted books for the anvil, knowledge book. (done)
    - Notes: enchanted books are their own item now rather than a book carrying enchantments: the table enchants a plain book, fishing and librarians hand them out, two of them merge at the anvil, and the grindstone turns one back into a book; old enchanted books in saves still work. Tooltips now run to several lines, so every enchanted thing lists what is on it. A book and quill (book, ink sac and feather) opens Minecraft's book screen: type, Enter for a new line, the arrows or Page Up and Down to turn, and the forward arrow adds a page up to a hundred; Sign asks for a title and turns it into a written book by you. Written books open the same screen to read, stack to sixteen, show their author and generation, and are copied by crafting one with books and quill, two generations deep, the original staying in the grid. Both go on a lectern, which now remembers the book and opens it to read with a Take Book button (sneak to take it straight off); a lectern no longer takes a plain book. Book text and lectern books are kept in the level file. The knowledge book is creative only and is used up with a note, since there is no recipe book here for it to teach. Structure chests still hold plain books, as their loot has no enchantments yet. Also fixed the suspicious stew recipe reading the wrong cells in the two by two grid, and buckets of fish now keep their look through a save.
55. Vehicles: chest, furnace, hopper, TNT and command block minecarts. A boat and chest boat for every wood. (done)
    - Notes: every wood that floats has its boat and its boat with chest, crafted from that wood's planks (nether planks make none) and drawn in the pack's own boat and chest boat art; bamboo makes the raft and raft with chest, on Mojang's raft model. Sneak and use opens a chest boat; a plain use gets in. Minecarts take a chest, furnace, hopper or TNT in the grid, and carry the block where you can see it. Chest and hopper minecarts open like the block; a hopper minecart also picks up items it rolls over. A furnace minecart takes coal or charcoal, smokes, and pushes itself along the rails at half a ridden cart's speed. A TNT minecart lights on a powered activator rail or with flint and steel and goes off four seconds later. The command block minecart is creative only, edited with the command block's own screen, and runs its command on a powered activator rail. Broken vehicles give back the item they were made as, and spill what they carried. Boats and carts are now kept: they are saved with the level along with their wood, cargo, fuel and command, put away when you go far off or through a portal, and set back down once their ground has loaded. The old Boat item is now the Oak Boat.
56. Spawn eggs for every mob, coloured by mob. (done)
    - Notes: one spawn egg item that remembers its mob, drawn from the pack's egg and spot sheets tinted with each mob's two 1.21.1 colours (a painted egg in the same colours without the pack). Every living mob has one; boats, carts and the small slime do not, and the tropical fish egg hatches either shape. An egg used on the ground, or in water, puts the mob there; used on a grown animal of its own kind it makes a baby; used on a monster spawner it sets what the spawner makes, which is kept with the level. They are creative only, as in Minecraft, at the end of the Items tab and in search. The name above the hotbar now uses the full item name, so eggs, written book titles and fish buckets say what they are.
57. Fireworks: rockets, stars, sparks, flash and glitter, and firing them from a crossbow. (done)
    - Notes: firework stars and rockets use the 1.21.1 recipes: gunpowder and up to eight dyes, with a fire charge for a large ball, a gold nugget for a star, a feather for a burst, a diamond for a trail and glowstone for a twinkle; a star and dyes adds fade colours; paper, one to three gunpowder and up to seven stars make three rockets. Fire charges are in (blaze powder, gunpowder and coal or charcoal) and light fires like flint and steel. Designs are kept in the level the way banner designs are, and tooltips list the shape, colours, fade, trail, twinkle and flight. Rockets lift off from the ground on Minecraft's acceleration, leave a trail of sparks, and burst into small and large balls, stars and bursts of coloured sparks that slow, fall, fade to their fade colour, trail and twinkle, with the flash, the blast and large blast, and the twinkle a moment later. A burst with stars hurts anything within five blocks. A rocket used while gliding boosts the elytra. A crossbow loads a rocket from the off hand and fires it straight, bursting on what it hits. The creeper shape waits for mob heads. Sparks are drawn as glowing squares rather than the pack's spark sprite, since the sprite pass cannot be tinted.
58. Music discs (19), played in the jukebox. (done)
    - Notes: all nineteen discs with their pack art, their songs in the tooltip, and 1.21.1's lengths. A disc used on a jukebox goes in and plays, with notes rising from it and the volume falling off over 64 blocks; using the jukebox again or breaking it gives the disc back. Music is streamed from assets/sounds/records/<name>.ogg a piece at a time rather than loaded whole, since a song is far bigger than a sound effect; without the sound pack each disc plays its own short synthesised tune for the song's length. The twelve old discs drop from a creeper killed by a skeleton's arrow, which meant letting mob arrows hit creepers; 13 and cat are in mansion chests, otherside in ancient cities and strongholds, pigstep in bastions, relic in trail ruins, precipice and Creator (Music Box) in trial vaults and Creator in ominous ones, and disc 5 is crafted from nine fragments. Discs left in a jukebox are kept with the level but do not resume after a reload, only one jukebox is heard at a time, the comparator does not read a jukebox yet, and Now Playing goes to chat rather than the action bar.
59. Templates and patterns: banner pattern items, 18 armour trim templates and the netherite upgrade template. (done)
    - Notes: this came in with the smithing table, trial chambers and the loom in earlier items: all eighteen trim templates and the netherite upgrade template, each found where 1.21.1 hides it and copied with seven diamonds and its block, and the flower, skull, globe, piglin, flow and guster banner patterns. The creeper pattern waits for mob heads, and the Mojang pattern is left out on purpose.
60. Splash and lingering potions, with a colour for every brew. (done)
    - Notes: every brew whose effect is already in the game is in, with 1.21.1's ingredients and times: swiftness, slowness, strength, weakness, night vision (now from a golden carrot, as glowstone is the strengthener), fire resistance, regeneration, leaping, poison, water breathing and the turtle master, plus mundane and thick. Redstone makes a potion last longer and glowstone makes it stronger where Minecraft allows, a fermented spider eye corrupts swiftness and leaping into slowness and water into weakness, gunpowder makes a splash potion and dragon's breath a lingering one; old saved potions read as before. Splash potions are thrown and act on everything within four blocks, weaker further out, and a splash of water puts out a burning player. Lingering potions leave a coloured cloud for thirty seconds that shrinks as it is used, giving a quarter of the dose. Eight arrows round a lingering potion make eight tipped arrows. Every brew, length and strength in all three forms is in the creative Items tab, tooltips show each effect with its level and time, and every brew is drawn in its own colour, the turtle master in the mix of its two. Healing, harming, invisibility, slow falling and the other brews wait for their effects in item 73, and mobs only feel poison, slowness and regeneration from a potion.

## Part 4: new mobs

61. Armadillo: rolls up when scared, drops scutes. Wolf armour (from scutes) comes with it. (done)
    - Notes: the armadillo is on Mojang's model with the pack's art, lives in savannas and badlands, breeds on spider eyes, and rolls into its ball when a player sprints or rides near it, when the undead come close, or when it is hit; rolled up it does not move and takes half of a blow less one. It sheds a scute every five to ten minutes, and a brush takes one off it for sixteen points of wear. Six scutes make wolf armour, which the owner puts on a tamed wolf; it takes every blow until its sixty-four points run out, shows the pack's cracks as it wears, is mended a scute at a time, comes off with shears, and takes a dye in the grid. Wolf armour and its colour are kept only as long as the wolf is, since mobs are not saved yet, and dyeing takes one dye rather than mixing several.
62. Sniffer: digs up seeds, hatches from its egg. (done with item 45)
63. Glow squid, which drops glow ink. Glow ink sac item and glowing sign text (moved here from item 52). (done)
    - Notes: the glow squid is the squid's model in the pack's glow_squid skin, lights itself, and goes dark for five seconds when hurt, coming back over the last half second as in 1.21.1. It spawns in sea water with no light at all, which here is fifteen or more blocks under the surface, taking the squid's place and some of the others'; there is still no underground water for it to live in. It drops a glow ink sac, has its own spawn egg (1.21.1's colours) and a synthesised voice that the pack's mob/glow_squid files replace. Signs now keep a colour and a glow with their words, saved with the level: a dye recolours the text, a glow ink sac lights it and an ink sac puts it out, each used up, and only on a sign with something written on it; using one that would change nothing opens the editor as before. Sign text is drawn the way Minecraft draws it: plain black by default, dyed text at four tenths of its colour and shaded by the light where the sign hangs, glowing text at full colour with an outline (pale cream for black ink). Still to come: the glow particles around the squid and its ink squirt, glow item frames (the decorations items), honeycomb waxing, and the back face of a sign.
64. Zoglin, illusioner and evoker fangs. (done)
    - Notes: the evoker no longer throws potions: every five seconds it conjures fangs out of the floor the way 1.21.1 does, sixteen in a line a tick apart toward a far target, or a ring of five and a ring of eight round itself when the target is within three blocks. Each fang finds the floor between the two of them, rises on Mojang's model in the pack's evoker_fangs.png (painted when there is no pack), snaps shut with its sound and a puff of crits, and bites for six through armour eight ticks after it starts; it spares evokers and vexes. The zoglin is the hoglin's body in the pack's zoglin skin: forty health, hits for six and throws what it hits upward, drops rotten flesh, has 1.21.1's spawn egg, and goes for the nearest living thing that is not a zoglin or a creeper, the player included. A hoglin out of the Nether for fifteen seconds turns into one. The illusioner wears the pack's illusioner skin with its hood, has thirty-two health and a bow, and no spawn egg, as in 1.21.1. Once it sees you it casts its mirror spell for a minute (seventeen seconds between casts): it is drawn as four copies scattered up to three blocks round it, which fold back into it when it is hit and scatter again. It also blinds you for twenty seconds, once; Minecraft only does that on hard, and this engine has no difficulty yet. Mobs can now name a sheet in the pack directly, used when nothing is installed under skins/mob: the squid, glow squid, hoglin and evoker take theirs this way too, so the glow squid now really wears glow_squid.png, and glow particles drift off it. Still to come: the evoker's raised-arms casting pose and its wololo, the illusioner's cloud puffs, and the rest of the mobs moving onto their pack sheets (item 65).
65. Tadpole and trader llama look right (check they use the pack art), and llama spit. (done)
    - Notes: checking the tadpole showed it had never worn the pack art: it asked for entity/tadpole.png, and the pack keeps it at entity/tadpole/tadpole.png. Looking further, most mobs were the same, because only twenty-one sheets were ever copied into skins/mob by the install script and everything else painted itself. Every mob now names its own sheet in the pack and wears it whenever nothing is installed over it; all of them were looked at ten to a page (the zoo dev screen) and map onto Mojang's sheets, and the sniffer's 192-square sheet is now accepted. Entity art attached went from 229 to 277 of 524. The trader llama is a llama in its coat with the pack's trader_llama decor over it, 1.21.1's spawn egg, and no breeding; two come with every wandering trader and keep up with him. Llamas and trader llamas are neutral: hit one and it spits at you every two seconds from up to twenty blocks, for one point, with the spit drawn from the pack's spit.png (painted without the pack), arcing like an arrow and splashing on whatever it hits. Still to come: llamas spitting at wolves, trader llamas defending their trader, the leads between trader and llamas, and carpets on llamas (llama/decor, with the decorations items).

## Part 5: decorations that are entities

66. Paintings (done)
    - I. Get the geometry right: a frame of the right depth with the back texture behind.
    - II. Paintings are 1x1 up to 4x4 like Minecraft's, and placing one picks the biggest that fits.
    - III. Add all the paintings from the pack.
    - IV. Break and drop like Minecraft, and pop off when the wall goes.
    - Notes: all fifty of 1.21.1's paintings, at their sizes from one block to four by four, each drawn from the pack's painting/<name>.png (a painted stand-in of the same size without the pack). A painting is a sixteenth of a block deep and sits flat on the wall, with the pack's back.png on its back and round its edges, lit block by block like Minecraft's. Eight sticks round any wool make one; used on the side of a block it hangs the biggest of the forty-six hand-hung paintings that fits there, picked at random among those, with Minecraft's offsets for even sizes, and never over another painting. In creative every variant is in the items tab and the search, and a chosen one hangs only where it fits; its tooltip gives the title, artist and size (Cotan and Paradistrad lose their accents until the font has them, item 85). A punch takes it down and drops it, and it drops by itself when the wall behind goes or something fills the space in front, or in an explosion. They are saved with the level by name.
67. Armour stands (done)
    - I. Get the geometry and shape right: base, body, arms, head.
    - II. Make it work like an armour stand: wear armour, hold items, swap items with right-click, break in two hits.
    - III. Pose it, and let it be pushed, like Minecraft.
    - Notes: the stand is Mojang's model piece for piece (head stick, shoulders, body sticks, arms, legs and a base plate that stays square to the world) in the pack's armorstand/wood.png, painted without the pack. Six sticks and a smooth stone slab make one; it is set down on the block you click facing you, to the nearest eighth of a turn, and falls if what is under it goes. Armour is drawn on Mojang's armour model for the stand in the pack's armour layers (leather dyed its default brown), a carved pumpkin sits on its head face forward, and anything else is held in its hand. Using an item puts it where it belongs, and an empty hand takes back whatever is at the height you clicked, with 1.21.1's rules for stacks and creative. The first punch rocks it and a second within a quarter second breaks it, dropping it and everything on it; creative breaks it at once, and an explosion takes it too. Java gives a stand no arms and no posing without commands, so this goes the Bedrock way for those two: it always has arms, and sneaking with an empty hand cycles a dozen poses, starting from Java's default. As in Java nothing walks it about, but a piston shoves it along with the blocks. Carried blocks (a stand's pumpkin, a cart's furnace) now show their front face. Stands are saved with the level, gear and pose included.
68. Item frames: add the glow item frame. (done)
    - Notes: the glow item frame hangs four ways like the plain one in the pack's glow_item_frame block art (its own icon in the hand), holds and drops an item the same way, and draws what it holds at full brightness, so it reads in the dark. An item frame and a glow ink sac make one, and it breaks back into itself.
69. Banners (done)
    - I. Standing and wall banners in 16 base colours.
    - II. The 42 patterns, layered with the loom.
    - III. Banner patterns on shields.
    - Notes: I and II came in with the loom in item 46: standing and wall banners in all sixteen colours, and the forty-two patterns layered at the loom from the pack's banner sheets. This adds III: a shield and a banner in the grid make the shield with that banner on it, keeping the shield's wear and enchantments, and a shield that already has one takes no second, as in 1.21.1. The face is composed from the pack's shield_base.png and shield pattern sheets in the banner's colours, shown on the shield's icon and on the shield in hand (whose plain face now also comes from the pack when nothing is installed under skins). Banners and shields list their layers in the tooltip. The pattern art was always used by the banner renderer but built in a way the audit could not see; it now names its paths, and entity art attached went from 280 to 361. The shield on another player, and on your own back in third person, waits for held items on the player model.
70. Signs: a sign for every wood, hanging signs, and the hanging sign editing screen. (done)
    - Notes: every wood already had a sign; signs were flat wall tiles in the plank colour, so they are now Mojang's models in the pack's entity/signs sheets: the board on the wall, and a sign set on top of a block stands on its post, turned to face you in sixteen steps. Hanging signs came in for all eleven woods, two chains over six stripped logs making six: under a block one hangs on two crossed pairs of chains (or a V when it hangs from something thinner than a block), turned in sixteen steps; on the side of a block it hangs from a bar and faces along the wall, as in 1.21.1. Each kind sits where Minecraft puts it and drops when what holds it goes. The chunk mesher now leaves signs out and says where they are, and a renderer draws them from that. The editor shows the board you are writing on, post and all, in the wood's own art, with the words in the sign's ink; a hanging sign's editor shows the pack's gui/hanging_signs picture and takes shorter lines. Words still only go on the front of a sign.
71. Maps (done)
    - I. Empty map to filled map, drawn from the world.
    - II. Map decorations: player, banners, structures and markers (37 of them).
    - III. Copying, locking and zooming at the cartography table. Maps in item frames.
    - Notes: I and the cartography table came in with item 46. This adds the marks, drawn from the pack's map/decorations pictures the way Minecraft draws them: your arrow turns in sixteenths with you; off the edge it becomes the off-map dot at the border, and further off an ordinary map loses you while an explorer map keeps you as the off-limits dot; nobody shows from another dimension. Using a map on a banner marks it in the banner's colour and using it again takes the mark off. A map hung in an item frame is drawn on the wall over the whole block and shows a frame mark on itself where it hangs. The cartographer now trades as in 1.21.1, including the ocean, woodland and trial explorer maps for emeralds and a compass: each one points at the nearest monument, mansion or trial chambers when it reaches your inventory, zoomed out twice and marked. A held map now sits on the checkerboard background, as Minecraft's does. Still to come: buried treasure maps (there is no buried treasure yet), the village and temple marks (nothing hands out maps to them in 1.21.1 without experiments), and marks being lost when their banner is broken.
72. Armour trims: 55 trim textures over armour, applied at the smithing table. (done)
    - Notes: trims were put on at the smithing table in item 46 and shown as a flat tint. They are now coloured the way Minecraft colours them: each grey of the pack's trim sheet takes the matching colour from its material's palette in trims/color_palettes, and a trim of the armour's own metal (gold on gold, iron on iron, diamond on diamond, netherite on netherite) takes the darker palette. The icons, the inventory figure and armour stands all show them, and worn armour is now drawn on you in the world when the camera is behind or in front of you, leather in its own brown with its overlay, trims on top. All 55 trim textures are in use. Other players' armour waits for armour being sent over the network.

## Part 6: effects

73. The 28 new status effects, with icons, the potions or sources that give them, and what they do: absorption, blindness, conduit power, darkness, dolphin's grace, glowing, haste, health boost, hero of the village, hunger, infested, instant health, instant damage, invisibility, levitation, luck, bad luck, mining fatigue, nausea (with the screen overlay), oozing, raid omen, resistance, saturation, slow falling, trial omen, weaving, wind charged, wither (withered hearts). (done)
    - Notes: all thirty-nine of 1.21.1's effects are in, with the pack's mob_effect icons (a painted badge in the effect's colour without a pack), Mojang's colours and names, and what each one does. New brews with 1.21.1's ingredients and times: healing and harming (a spider eye turns healing, or poison, into harming, and the undead are hurt by healing and healed by harming), invisibility, slow falling, and 1.21's wind charging, weaving, oozing and infestation from a breeze rod, a cobweb, a slime block and stone; luck is only in the creative tab, as in 1.21.1. Tipped arrows keep the brew's level and are named for it. Where they come from: wither skeletons, and the wither's skulls, which now fly as skulls, wither you; a wither rose withers whatever stands in it and in a stew; husks bring hunger, cave spiders and bees poison, shulker bullets levitate; an elder guardian lays mining fatigue III on you once a minute, its face flashing up on the screen; the warden darkens everything near it, and so does a shrieker in an ancient city, which now shrieks when stepped on or when a sensor near it hears you and summons the warden at the fourth warning; dolphins give dolphin's grace to a swimmer; pufferfish adds hunger and nausea; witches pick slowness, poison, weakness or harming the way 1.21.1's do. Bad omen turns into trial omen beside a trial spawner (fifteen minutes a level) and into raid omen in a village, and raids are in: thirty seconds after the omen, five waves of pillagers, vindicators, evokers, witches and ravagers come under the red notched bar, with a bonus wave for a stronger omen; win and you are Hero of the Village, which cuts villager prices; ringing a bell makes raiders glow. Hearts wither, health boost adds rows of them, nausea warps the view and shows misc/nausea when the new Distortion Effects video setting is turned down, darkness closes the fog in and throbs, mobs see an invisible player only close up, and the ominous bottle's bad omen now lasts a hundred minutes. Oozing, weaving and wind charged go off when whatever carries them dies; infested lets out silverfish when it is hit. /effect give and /effect clear take any of them. Still to come: villagers' gifts to the hero, raid captains and their banners, outlines through walls for glowing (glowing mobs are lit up instead), the ambient frame for beacon and conduit effects, and the absorbing and frozen heart styles (item 74).
74. Heart and food styles: absorbing, frozen and withered hearts, and the hunger food icons. (done)
    - Notes: frozen hearts and the hunger food icons were already in, and withered hearts came with wither in item 73. The heart row is now laid out the way 1.21.1 lays it out: absorption hearts follow the red ones in the same rows rather than sitting in a row of their own, each in its container and drawn withered while wither is on; more rows squeeze closer together; a hit makes the hearts blink white, showing what you had a moment ago, and healing blinks them briefly too; with regeneration one heart at a time bobs up along the row; and the food row trembles when saturation runs out. Poisoned beats withered, which beats frozen, as in 1.21.1. Without a pack the hearts are tinted to match.

## Part 7: screens and menus

75. Recipe book: crafting and furnace books, filters, tabs, and the ghost recipe in the grid. (done)
    - Notes: the green book button sits where 1.21.1 puts it on the inventory, the crafting table and the furnace, smoker and blast furnace, and opening it slides the screen right with the book beside it, drawn from the pack's recipe_book art: the search box, the craftable filter ("Showing Craftable", "Showing Smeltable" and the rest), the tabs with 1.21.1's icons (all, building blocks, redstone, equipment and miscellaneous for crafting; food, blocks and miscellaneous for the furnaces, only what each one cooks), twenty results a page on green or red slots, and the page arrows. Every recipe the game makes is in it, filed under the tab 1.21.1 files it under (the categories come from Mojang's recipe data, with the same rules for the families it generates), with the wood and dye families sharing one slot that cycles through them; right-clicking such a slot opens the alternatives, each shown as a little grid of its ingredients. Clicking a recipe you can make lays the ingredients into the grid from your inventory, one set at a time or as many as a stack allows with shift; clicking one you cannot shows the ghost recipe in red until you touch the grid, fuel and all in a furnace. The book remembers whether it was open and how each filter was set. On a screen too narrow for both it covers the inventory, as 1.21.1's does. Every recipe is known from the start: unlocking recipes, and the toast that announces them, come with the advancements and toasts in item 76.
76. Advancements: the screen with its tabs, frames and backgrounds, plus toasts when you earn one. (done)
    - Notes: all 122 of 1.21.1's advancements are in, in their five trees (Minecraft, Nether, The End, Adventure, Husbandry), with Mojang's titles and descriptions, frames, icons, hidden ones and the same layout 1.21.1 works out for each tree. L opens the screen, as does a new Advancements button on the pause menu: the pack's window, the tabs above it with their root icons, the tiled backgrounds, the connecting lines, task, goal and challenge frames, and the hover box with its progress bar and an x/y count for the ones with many parts; the tree drags. What shows follows 1.21.1's rule (what you have done, and up to two steps beyond it, never a hidden one until it is done), so a new world shows nothing until the first crafting table. Earning one slides in Mojang's toast ("Advancement Made!", "Goal Reached!", "Challenge Complete!" with its fanfare) and says so in chat. Progress is kept per world. The recipe book now only shows recipes you have unlocked, which happens when you first hold one of a recipe's ingredients, with the "New Recipes Unlocked!" toast. Earned by: obtaining items, entering dimensions, finding fortresses, bastions, strongholds, end cities and trial chambers, biomes, killing (Monster Hunter and Monsters Hunted count each kind, Sniper Duel, Uneasy Alliance, Free the End, It Spreads), dying, sleeping, trading (and at the top of the world), enchanting, brewing, beacons, summoning golems and the wither, breeding and taming, eating (A Balanced Diet), planting, fishing and bucketing fish, axolotls and tadpoles, shields, totems, bows, crossbows and tridents, spyglasses, levitating fifty blocks, powder snow in leather boots, sneaking past sensors, every status effect at once, Hero of the Village, honey with a campfire, waxing and scraping copper, glowing signs, lodestones, striders, wolf armour, vault keys, armour trims and brushing armadillos. Still to come, because what triggers them is not in the game yet: zombie villager cures, returning a ghast's fireball, nether fast travel, bastion loot, piglin bartering, respawn anchors, end gateways, respawning the dragon, channeling, piercing and multishot kills, raid captains, honey slides, targets, lightning rods, meadows, falling the height of the world, sherds and decorated pots from the brush, chiseled bookshelves read by comparators, copper bulbs, wind charge kills and launches, the mace, crafters making crafters, axolotls helping, cat and wolf variants, allays, goats in boats, snifflets and frog variants on leads; Adventuring Time, Hot Tourist Destinations and Two by Two need biomes and animals the world does not have yet.
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
