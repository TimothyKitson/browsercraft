// The inventory screen, split out of Application.cpp because it is a
// self-contained piece of UI: creative tabs, the armour column and body
// preview, the crafting grid, and the stack that follows the cursor.

namespace
{
    // Creative tabs, in the order they appear. Blocks may show up in more
    // than one tab -- grass belongs with both the building materials and
    // the scenery, and hunting for it in the wrong place is worse than
    // listing it twice.
    const StackId TAB_BUILDING[] = {
        Blocks::Stone, Blocks::Cobblestone, Blocks::MossyCobblestone, Blocks::Bricks,
        Blocks::Planks, Blocks::SprucePlanks, Blocks::DarkOakPlanks,
        Blocks::SpruceLog, Blocks::DarkOakLog, Blocks::SpruceLeaves, Blocks::DarkOakLeaves,
        Blocks::Granite, Blocks::Diorite, Blocks::Andesite, Blocks::Deepslate,
        Blocks::Tuff, Blocks::Calcite, Blocks::Log, Blocks::BirchLog, Blocks::Sandstone,
        Blocks::Sand, Blocks::Gravel, Blocks::Dirt, Blocks::Grass,
        Blocks::SnowGrass, Blocks::Clay, Blocks::Obsidian, Blocks::Glass,
        Blocks::Wool, Blocks::Snow, Blocks::Ice,
        Blocks::StoneSlab, Blocks::CobblestoneSlab, Blocks::PlankSlab, Blocks::SandstoneSlab,
        Blocks::Tnt, Blocks::LadderNorth, Blocks::OakFence, Blocks::DoorLowShutNorth,
        Blocks::StoneStairsNorth, Blocks::CobbleStairsNorth,
        Blocks::PlankStairsNorth, Blocks::SandstoneStairsNorth,
        Blocks::BirchPlanks, Blocks::JunglePlanks, Blocks::AcaciaPlanks,
        Blocks::StrippedOakLog, Blocks::StrippedSpruceLog, Blocks::StrippedBirchLog,
        Blocks::StrippedJungleLog, Blocks::StrippedAcaciaLog, Blocks::StrippedDarkOakLog,
        Blocks::SpruceDoor, Blocks::BirchDoor, Blocks::JungleDoor, Blocks::AcaciaDoor, Blocks::DarkOakDoor,
        Blocks::OakTrapdoor, Blocks::SpruceTrapdoor, Blocks::BirchTrapdoor,
        Blocks::JungleTrapdoor, Blocks::AcaciaTrapdoor, Blocks::DarkOakTrapdoor,
        Blocks::CherryLog, Blocks::StrippedCherryLog, Blocks::CherryPlanks, Blocks::CherryDoor,
        Blocks::CherryTrapdoor,
        Blocks::MangroveLog, Blocks::StrippedMangroveLog, Blocks::MangrovePlanks, Blocks::MangroveDoor,
        Blocks::MangroveTrapdoor, Blocks::MangroveRoots, Blocks::MuddyMangroveRoots, Blocks::Mud,
        Blocks::PackedMud, Blocks::MudBricks,
        Blocks::BambooBlock, Blocks::StrippedBambooBlock, Blocks::BambooPlanks, Blocks::BambooMosaic,
        Blocks::BambooSlab, Blocks::BambooDoor, Blocks::BambooTrapdoor,
        Blocks::CrimsonStem, Blocks::StrippedCrimsonStem, Blocks::CrimsonPlanks, Blocks::CrimsonDoor,
        Blocks::CrimsonTrapdoor, Blocks::WarpedStem, Blocks::StrippedWarpedStem, Blocks::WarpedPlanks,
        Blocks::WarpedDoor, Blocks::WarpedTrapdoor, Blocks::CrimsonNylium, Blocks::WarpedNylium,
        Blocks::NetherWartBlock, Blocks::WarpedWartBlock, Blocks::Shroomlight,
        Blocks::StoneBricks, Blocks::MossyStoneBricks, Blocks::CrackedStoneBricks, Blocks::ChiseledStoneBricks, Blocks::SmoothStone, Blocks::SmoothStoneSlab, Blocks::StoneBrickSlab, Blocks::PolishedGranite, Blocks::PolishedDiorite, Blocks::PolishedAndesite, Blocks::CobbledDeepslate, Blocks::PolishedDeepslate, Blocks::ChiseledDeepslate, Blocks::CrackedDeepslateBricks, Blocks::CrackedDeepslateTiles, Blocks::CobbledDeepslateSlab, Blocks::PolishedTuff, Blocks::ChiseledTuff, Blocks::ChiseledTuffBricks, Blocks::TuffSlab, Blocks::TuffBrickSlab, Blocks::PolishedBlackstone, Blocks::ChiseledPolishedBlackstone, Blocks::CrackedPolishedBlackstoneBricks, Blocks::PolishedBlackstoneSlab, Blocks::RedNetherBricks, Blocks::CrackedNetherBricks, Blocks::ChiseledNetherBricks, Blocks::NetherBrickSlab,
        Blocks::QuartzBlock, Blocks::QuartzBricks, Blocks::QuartzPillar, Blocks::ChiseledQuartzBlock, Blocks::QuartzSlab,
        Blocks::RedSand, Blocks::RedSandstone, Blocks::CutRedSandstone, Blocks::ChiseledRedSandstone, Blocks::SmoothRedSandstone, Blocks::RedSandstoneSlab, Blocks::CutSandstone, Blocks::ChiseledSandstone, Blocks::SmoothSandstone,
        Blocks::Terracotta, Blocks::GlassPane, Blocks::TintedGlass,
        Blocks::ConcreteFirst + 0, Blocks::ConcreteFirst + 1, Blocks::ConcreteFirst + 2, Blocks::ConcreteFirst + 3,
        Blocks::ConcreteFirst + 4, Blocks::ConcreteFirst + 5, Blocks::ConcreteFirst + 6, Blocks::ConcreteFirst + 7,
        Blocks::ConcreteFirst + 8, Blocks::ConcreteFirst + 9, Blocks::ConcreteFirst + 10, Blocks::ConcreteFirst + 11,
        Blocks::ConcreteFirst + 12, Blocks::ConcreteFirst + 13, Blocks::ConcreteFirst + 14, Blocks::ConcreteFirst + 15,
        Blocks::ConcretePowderFirst + 0, Blocks::ConcretePowderFirst + 1, Blocks::ConcretePowderFirst + 2, Blocks::ConcretePowderFirst + 3,
        Blocks::ConcretePowderFirst + 4, Blocks::ConcretePowderFirst + 5, Blocks::ConcretePowderFirst + 6, Blocks::ConcretePowderFirst + 7,
        Blocks::ConcretePowderFirst + 8, Blocks::ConcretePowderFirst + 9, Blocks::ConcretePowderFirst + 10, Blocks::ConcretePowderFirst + 11,
        Blocks::ConcretePowderFirst + 12, Blocks::ConcretePowderFirst + 13, Blocks::ConcretePowderFirst + 14, Blocks::ConcretePowderFirst + 15,
        Blocks::TerracottaFirst + 0, Blocks::TerracottaFirst + 1, Blocks::TerracottaFirst + 2, Blocks::TerracottaFirst + 3,
        Blocks::TerracottaFirst + 4, Blocks::TerracottaFirst + 5, Blocks::TerracottaFirst + 6, Blocks::TerracottaFirst + 7,
        Blocks::TerracottaFirst + 8, Blocks::TerracottaFirst + 9, Blocks::TerracottaFirst + 10, Blocks::TerracottaFirst + 11,
        Blocks::TerracottaFirst + 12, Blocks::TerracottaFirst + 13, Blocks::TerracottaFirst + 14, Blocks::TerracottaFirst + 15,
        Blocks::GlazedTerracottaFirst + 0, Blocks::GlazedTerracottaFirst + 1, Blocks::GlazedTerracottaFirst + 2, Blocks::GlazedTerracottaFirst + 3,
        Blocks::GlazedTerracottaFirst + 4, Blocks::GlazedTerracottaFirst + 5, Blocks::GlazedTerracottaFirst + 6, Blocks::GlazedTerracottaFirst + 7,
        Blocks::GlazedTerracottaFirst + 8, Blocks::GlazedTerracottaFirst + 9, Blocks::GlazedTerracottaFirst + 10, Blocks::GlazedTerracottaFirst + 11,
        Blocks::GlazedTerracottaFirst + 12, Blocks::GlazedTerracottaFirst + 13, Blocks::GlazedTerracottaFirst + 14, Blocks::GlazedTerracottaFirst + 15,
        Blocks::StainedGlassFirst + 0, Blocks::StainedGlassFirst + 1, Blocks::StainedGlassFirst + 2, Blocks::StainedGlassFirst + 3,
        Blocks::StainedGlassFirst + 4, Blocks::StainedGlassFirst + 5, Blocks::StainedGlassFirst + 6, Blocks::StainedGlassFirst + 7,
        Blocks::StainedGlassFirst + 8, Blocks::StainedGlassFirst + 9, Blocks::StainedGlassFirst + 10, Blocks::StainedGlassFirst + 11,
        Blocks::StainedGlassFirst + 12, Blocks::StainedGlassFirst + 13, Blocks::StainedGlassFirst + 14, Blocks::StainedGlassFirst + 15,
        Blocks::StainedGlassPaneFirst + 0, Blocks::StainedGlassPaneFirst + 1, Blocks::StainedGlassPaneFirst + 2, Blocks::StainedGlassPaneFirst + 3,
        Blocks::StainedGlassPaneFirst + 4, Blocks::StainedGlassPaneFirst + 5, Blocks::StainedGlassPaneFirst + 6, Blocks::StainedGlassPaneFirst + 7,
        Blocks::StainedGlassPaneFirst + 8, Blocks::StainedGlassPaneFirst + 9, Blocks::StainedGlassPaneFirst + 10, Blocks::StainedGlassPaneFirst + 11,
        Blocks::StainedGlassPaneFirst + 12, Blocks::StainedGlassPaneFirst + 13, Blocks::StainedGlassPaneFirst + 14, Blocks::StainedGlassPaneFirst + 15,
    };

    const StackId TAB_NATURE[] = {
        Blocks::Grass, Blocks::SnowGrass, Blocks::Dirt, Blocks::TallGrass,
        Blocks::FlowerRed, Blocks::FlowerYellow, Blocks::Leaves, Blocks::BirchLeaves,
        Blocks::Log, Blocks::BirchLog, Blocks::Cactus, Blocks::Pumpkin,
        Blocks::Sand, Blocks::Snow, Blocks::Ice, Blocks::Clay,
        Blocks::SugarCane,
        Blocks::OakSapling, Blocks::SpruceSapling, Blocks::BirchSapling,
        Blocks::JungleSapling, Blocks::AcaciaSapling, Blocks::DarkOakSapling,
        Blocks::CherryLeaves, Blocks::CherrySapling, Blocks::PinkPetals1,
        Blocks::MangroveLeaves, Blocks::MangrovePropagule,
        Blocks::CrimsonFungus, Blocks::WarpedFungus, Blocks::CrimsonRoots, Blocks::WarpedRoots,
        Blocks::NetherSprouts, Blocks::WeepingVines, Blocks::TwistingVines,
    };

    const StackId TAB_ORES[] = {
        Blocks::CoalOre, Blocks::IronOre, Blocks::GoldOre, Blocks::DiamondOre,
        Blocks::EmeraldOre, Blocks::NetherQuartzOre, Blocks::NetherGoldOre,
        Blocks::DeepslateCoalOre, Blocks::DeepslateIronOre, Blocks::DeepslateCopperOre, Blocks::DeepslateGoldOre,
        Blocks::DeepslateRedstoneOre, Blocks::DeepslateEmeraldOre, Blocks::DeepslateLapisOre,
        Blocks::DeepslateDiamondOre, Blocks::RawIronBlock, Blocks::RawGoldBlock, Blocks::RawCopperBlock,
        Blocks::Stone, Blocks::Cobblestone, Blocks::Gravel, Blocks::Obsidian,
        Blocks::Bedrock,
    };

    const StackId TAB_MISC[] = {
        Blocks::Torch, Blocks::Glowstone, Blocks::Water, Blocks::Lava,
        Blocks::Ice, Blocks::Glass, Blocks::Wool, Blocks::Pumpkin,
        Blocks::Bedrock,
        Blocks::ShulkerBox,
        Blocks::ShulkerBoxFirst + 0, Blocks::ShulkerBoxFirst + 1, Blocks::ShulkerBoxFirst + 2, Blocks::ShulkerBoxFirst + 3,
        Blocks::ShulkerBoxFirst + 4, Blocks::ShulkerBoxFirst + 5, Blocks::ShulkerBoxFirst + 6, Blocks::ShulkerBoxFirst + 7,
        Blocks::ShulkerBoxFirst + 8, Blocks::ShulkerBoxFirst + 9, Blocks::ShulkerBoxFirst + 10, Blocks::ShulkerBoxFirst + 11,
        Blocks::ShulkerBoxFirst + 12, Blocks::ShulkerBoxFirst + 13, Blocks::ShulkerBoxFirst + 14, Blocks::ShulkerBoxFirst + 15,
        Blocks::Candle,
        Blocks::DyedCandleFirst + 0, Blocks::DyedCandleFirst + 8, Blocks::DyedCandleFirst + 16, Blocks::DyedCandleFirst + 24,
        Blocks::DyedCandleFirst + 32, Blocks::DyedCandleFirst + 40, Blocks::DyedCandleFirst + 48, Blocks::DyedCandleFirst + 56,
        Blocks::DyedCandleFirst + 64, Blocks::DyedCandleFirst + 72, Blocks::DyedCandleFirst + 80, Blocks::DyedCandleFirst + 88,
        Blocks::DyedCandleFirst + 96, Blocks::DyedCandleFirst + 104, Blocks::DyedCandleFirst + 112, Blocks::DyedCandleFirst + 120,
    };

    struct CreativeTab
    {
        const char* name;
        StackId icon;
        const StackId* items;   // null means "every obtainable block"
        int count;
    };

    const StackId TAB_ITEMS[] = {
        Items::Wheat, Items::WheatSeeds, Items::Leather, Items::RawBeef,
        Items::RawPorkchop, Items::RawChicken, Items::RawMutton,
        Items::Feather, Items::Bone, Items::StringItem, Items::Gunpowder, Items::Flint,
        Items::Bread, Items::Steak, Items::CookedPorkchop, Items::CookedChicken,
        Items::CookedMutton,
        Items::Paper, Items::Book, Items::Emerald, Items::EnderPearl, Items::EyeOfEnder, Items::NetherQuartz, Items::AmethystShard,
        Items::Sign, Items::SpruceSign, Items::BirchSign, Items::JungleSign, Items::AcaciaSign, Items::DarkOakSign,
        Items::CherrySign, Items::MangroveSign, Items::BambooSign, Items::CrimsonSign, Items::WarpedSign,
    };

    // The twenty-five tools, their materials, and the two benches you
    // need to make any of it.
    const StackId TAB_ARMOUR[] = {
        Items::LeatherHelmet, Items::LeatherChestplate, Items::LeatherLeggings, Items::LeatherBoots,
        Items::GoldHelmet, Items::GoldChestplate, Items::GoldLeggings, Items::GoldBoots,
        Items::IronHelmet, Items::IronChestplate, Items::IronLeggings, Items::IronBoots,
        Items::DiamondHelmet, Items::DiamondChestplate, Items::DiamondLeggings, Items::DiamondBoots,
    };

    const StackId TAB_TOOLS[] = {
        Blocks::CraftingTable, Blocks::Furnace, Blocks::Chest, Blocks::Bed,
        Blocks::Bookshelf, Blocks::EnchantingTable, Blocks::Anvil,
        Blocks::EndPortalFrame,
        Items::Stick, Items::Coal, Items::Bow, Items::Arrow, Items::Shield, Items::FlintAndSteel, Items::Boat, Items::Minecart,
        Blocks::RailNorthSouth,
        Items::Bucket, Items::WaterBucket, Items::LavaBucket,
        Items::IronIngot, Items::GoldIngot, Items::Diamond,
        Items::WoodPickaxe, Items::WoodAxe, Items::WoodShovel, Items::WoodSword, Items::WoodHoe,
        Items::StonePickaxe, Items::StoneAxe, Items::StoneShovel, Items::StoneSword, Items::StoneHoe,
        Items::IronPickaxe, Items::IronAxe, Items::IronShovel, Items::IronSword, Items::IronHoe,
        Items::GoldPickaxe, Items::GoldAxe, Items::GoldShovel, Items::GoldSword, Items::GoldHoe,
        Items::DiamondPickaxe, Items::DiamondAxe, Items::DiamondShovel, Items::DiamondSword,
        Items::DiamondHoe,
    };

    const CreativeTab CREATIVE_TABS[] = {
        { "Building Blocks",  Blocks::Bricks,     TAB_BUILDING, static_cast<int>(std::size(TAB_BUILDING)) },
        { "Nature",           Blocks::Grass,      TAB_NATURE,   static_cast<int>(std::size(TAB_NATURE)) },
        { "Ores and Stone",   Blocks::DiamondOre, TAB_ORES,     static_cast<int>(std::size(TAB_ORES)) },
        { "Light and Liquid", Blocks::Glowstone,  TAB_MISC,     static_cast<int>(std::size(TAB_MISC)) },
        { "Items",            Items::Wheat,       TAB_ITEMS,    static_cast<int>(std::size(TAB_ITEMS)) },
        { "Tools",            Items::IronPickaxe, TAB_TOOLS,    static_cast<int>(std::size(TAB_TOOLS)) },
        { "Armour",           Items::DiamondChestplate, TAB_ARMOUR, static_cast<int>(std::size(TAB_ARMOUR)) },
        { "Search Items",     Items::Book,        nullptr,      0 },
        { "Your Inventory",   Blocks::Chest,      nullptr,      0 },  // the player's own slots
    };

    constexpr int TAB_COUNT = static_cast<int>(std::size(CREATIVE_TABS));
    constexpr int INVENTORY_TAB = TAB_COUNT - 1;
    constexpr int SEARCH_TAB = TAB_COUNT - 2;
    constexpr int PALETTE_COLUMNS = 9;
    constexpr int PALETTE_ROWS = 5;

    // The four armour slots, top to bottom, and the ghost drawn in each
    // while it is empty.
    const GuiSprite ARMOR_GHOSTS[Inventory::ARMOR_SLOTS] = {
        GuiSprite::ArmorHelmet, GuiSprite::ArmorChestplate,
        GuiSprite::ArmorLeggings, GuiSprite::ArmorBoots,
    };

    // Everything you can hold: the obtainable blocks, then the items.
    const std::vector<StackId>& allStacks()
    {
        static const std::vector<StackId> everything = []() {
            std::vector<StackId> list;
            for (BlockId id = 1; id < Blocks::Count; ++id)
                if (isObtainable(id)) list.push_back(id);
            for (StackId id = Items::FIRST; id < Items::Count; ++id)
                list.push_back(id);
            return list;
        }();
        return everything;
    }

    void tabContents(int tab, const StackId*& items, int& count)
    {
        if (tab < 0 || tab >= TAB_COUNT || CREATIVE_TABS[tab].items == nullptr)
        {
            items = allStacks().data();
            count = static_cast<int>(allStacks().size());
            return;
        }
        items = CREATIVE_TABS[tab].items;
        count = CREATIVE_TABS[tab].count;
    }

    // What a stack is called in a tooltip. A potion is one item id with
    // the brew in its damage, so its name has to be read out of that.
    std::string stackName(const ItemStack& stack)
    {
        if (stack.id == Items::Potion) return Brewing::name(Brewing::fromDamage(stack.damage));
        return displayName(stack.id);
    }
}

// A cube seen from above and to one side: the top face as a rhombus,
// and two of the four sides below it -- Minecraft's own view of a block
// in a slot, numbers and all.
//
// The client draws a block in a slot as a cube ten pixels on an edge,
// turned 45 degrees about the upright and tipped 30 towards you. That
// makes the top a rhombus 10 * sqrt 2 = 14.1 across and half that tall,
// and the sides hang 10 * cos 30 = 8.7 below it: 14.1 wide and 15.7
// tall in all, a little taller than wide. Fitting the same cube into a
// square instead -- the sides only as tall as the top is deep -- is what
// made every block in the game look sat on.
bool Application::drawBlockIcon(float x, float y, float size, StackId id,
                                const glm::vec4& tint)
{
    if (isItem(id)) return false;

    const BlockId block = asBlock(id);
    const BlockInfo& info = blockInfo(block);
    if (flatIconFor(block) >= 0) return false;

    // Only things that really are a box. A flower is two crossed quads
    // and a liquid has no edges; both read better flat, and Minecraft
    // draws them flat too.
    if (info.render != RenderType::Cube && info.render != RenderType::Slab) return false;

    // Minecraft's proportions, scaled to the slot: 14.14 of 16 across.
    const float width = size * (14.142f / 16.0f);
    const float a = width * 0.5f;                       // half across, and the top's height
    const float fullSides = width * 0.6124f;             // cos 30 / sqrt 2 of the width
    const float tall = blockHeight(block);               // a slab stands half as high
    const float sides = fullSides * tall;

    // Centred in the slot as a full cube, and a slab sits where the
    // bottom of that cube would be rather than floating in the middle.
    const float margin = (size - (a + fullSides)) * 0.5f;
    const float top = y + size - margin - (a + sides);
    const float cx = x + size * 0.5f;

    const glm::vec2 A(cx, top);                  // the rhombus, clockwise
    const glm::vec2 B(cx + a, top + a * 0.5f);
    const glm::vec2 C(cx, top + a);
    const glm::vec2 D(cx - a, top + a * 0.5f);
    const glm::vec2 Bb(B.x, B.y + sides);        // and the two corners below
    const glm::vec2 Cb(C.x, C.y + sides);
    const glm::vec2 Db(D.x, D.y + sides);

    const TileUV topUV = tileUV(info.tileTop);
    const TileUV sideUV = tileUV(info.tileSide);

    const glm::vec2 square[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

    auto faceUV = [&](const TileUV& t, glm::vec2 out[4]) {
        for (int i = 0; i < 4; ++i)
            out[i] = glm::vec2(square[i].x == 0.0f ? t.u0 : t.u1,
                               square[i].y == 0.0f ? t.vTop : t.vBottom);
    };

    // The three faces are shaded apart, or the cube reads as a flat
    // hexagon of one colour. These are the same ratios the world uses
    // for a top, a north face and an east face.
    auto shaded = [&](float amount) {
        return glm::vec4(tint.r * amount, tint.g * amount, tint.b * amount, tint.a);
    };

    // Where the light comes from, worked into each face's own tangent
    // space so the shader can read the pack's normal map against it.
    // The icon is drawn as if seen from above, in front and to one
    // side, so the light is put up there too.
    const glm::vec3 sun = glm::normalize(glm::vec3(0.35f, 1.0f, 0.45f));

    auto tangentLight = [&](const glm::vec3& tangent, const glm::vec3& bitangent,
                            const glm::vec3& normal) {
        return glm::vec3(glm::dot(sun, tangent), glm::dot(sun, bitangent),
                         glm::dot(sun, normal));
    };

    // The same three frames the world uses: the icon's top face is the
    // block's +Y, its left is +Z and its right is +X.
    const glm::vec3 topLight = tangentLight(glm::vec3(1.0f, 0.0f, 0.0f),
                                            glm::vec3(0.0f, 0.0f, -1.0f),
                                            glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 leftLight = tangentLight(glm::vec3(1.0f, 0.0f, 0.0f),
                                             glm::vec3(0.0f, 1.0f, 0.0f),
                                             glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::vec3 rightLight = tangentLight(glm::vec3(0.0f, 0.0f, 1.0f),
                                              glm::vec3(0.0f, 1.0f, 0.0f),
                                              glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec2 uv[4];

    const glm::vec2 topFace[4] = { A, B, C, D };
    faceUV(topUV, uv);
    m_ui.texturedShape(m_atlas.textureId(), topFace, uv, shaded(1.0f), topLight);

    // Left, then right. Drawn after the top so a seam between them is
    // covered by the face in front rather than showing through.
    const glm::vec2 leftFace[4] = { D, C, Cb, Db };
    faceUV(info.tileFront >= 0 ? tileUV(info.tileFront) : sideUV, uv);
    m_ui.texturedShape(m_atlas.textureId(), leftFace, uv, shaded(0.78f), leftLight);

    const glm::vec2 rightFace[4] = { C, B, Bb, Cb };
    faceUV(sideUV, uv);
    m_ui.texturedShape(m_atlas.textureId(), rightFace, uv, shaded(0.60f), rightLight);

    return true;
}

unsigned int Application::glintTexture(bool entity)
{
    unsigned int& texture = entity ? m_glintEntity : m_glintItem;
    if (texture) return texture;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(entity ? "assets/textures/misc/enchanted_glint_entity.png"
                                           : "assets/textures/misc/enchanted_glint_item.png",
                                    &width, &height, &channels, 4);
    std::vector<unsigned char> painted;
    if (!data)
    {
        width = height = 16;
        painted.assign(16 * 16 * 4, 0);
        for (int i = 0; i < 16 * 16; ++i)
        {
            const int x = i % 16, y = i / 16;
            const unsigned char glow = ((x + y) % 8 < 3) ? 140 : 40;
            painted[i * 4 + 0] = glow;
            painted[i * 4 + 1] = static_cast<unsigned char>(glow / 3);
            painted[i * 4 + 2] = glow;
            painted[i * 4 + 3] = 255;
        }
    }
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data ? data : painted.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (data) stbi_image_free(data);
    return texture;
}

glm::vec2 Application::glintShift() const
{
    const double ms = static_cast<double>(SDL_GetTicks()) * 8.0;
    const float across = static_cast<float>(std::fmod(ms, 110000.0) / 110000.0);
    const float down = static_cast<float>(std::fmod(ms, 30000.0) / 30000.0);
    return glm::vec2(-across, down);
}

void Application::drawItem(float x, float y, float size, StackId id, int count,
                           const glm::vec4& tint, int damage, uint32_t enchantments)
{
    if (id == Blocks::Air) return;

    // Minecraft draws an item the full sixteen pixels of its slot's
    // inside, with the slot's own border the only margin. Shrunk by a
    // pixel a side on top of that, every icon came out a size smaller
    // than the slot it was in.
    if (id == Items::Potion)
    {
        const unsigned int rgb = Brewing::colour(Brewing::fromDamage(damage));
        const glm::vec4 liquidTint(((rgb >> 16) & 0xFF) / 255.0f * tint.r, ((rgb >> 8) & 0xFF) / 255.0f * tint.g,
                                   (rgb & 0xFF) / 255.0f * tint.b, tint.a);
        const TileUV liquid = tileUV(Tiles::PotionLiquid);
        const TileUV bottle = tileUV(Tiles::PotionBottle);
        m_ui.texturedQuad(m_atlas.textureId(), x, y, size, size,
                          liquid.u0, liquid.vTop, liquid.u1, liquid.vBottom, liquidTint);
        m_ui.texturedQuad(m_atlas.textureId(), x, y, size, size,
                          bottle.u0, bottle.vTop, bottle.u1, bottle.vBottom, tint);
    }
    else if (!drawBlockIcon(x, y, size, id, tint))
    {
        int tile = m_iconOverride >= 0 ? m_iconOverride : atlasTileFor(id);
        if (id == Items::Elytra && damage >= Armour::maxDurability(Items::Elytra)) tile = Tiles::ItemBrokenElytra;
        const TileUV uv = tileUV(tile);
        m_ui.texturedQuad(m_atlas.textureId(), x, y, size, size,
                          uv.u0, uv.vTop, uv.u1, uv.vBottom, tint);
        if (enchantments != 0)
        {
            m_ui.beginGlint(glintTexture(false), x, y, size, size, glintShift());
            m_ui.texturedQuad(m_atlas.textureId(), x, y, size, size,
                              uv.u0, uv.vTop, uv.u1, uv.vBottom, tint);
            m_ui.endGlint();
        }
    }

    // How much of a tool is left, as the bar across the bottom of the
    // icon: green when it is new, red by the time it is about to go.
    const int limit = Tools::maxDurability(Tools::tierOf(id));
    if (limit > 0 && damage > 0)
    {
        // The client's bar: a black strip 13 by 2 at 2 and 13, and the
        // coloured one a pixel tall along the top of it.
        const float left = std::clamp(1.0f - static_cast<float>(damage) / limit, 0.0f, 1.0f);
        const float px = size / 16.0f;
        const float barX = x + 2.0f * px;
        const float barY = y + 13.0f * px;

        m_ui.quad(barX, barY, 13.0f * px, 2.0f * px, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        m_ui.quad(barX, barY, std::round(13.0f * left) * px, px,
                  glm::vec4(1.0f - left, left, 0.0f, 1.0f));
    }

    // A single item shows no number, same as Minecraft.
    if (count > 1)
    {
        // Minecraft draws the count in the bottom-right of a 16-pixel
        // slot at the ordinary text size, so the scale follows the
        // slot rather than being a number of its own: a hotbar slot
        // and an inventory slot then carry the same stack the same
        // way round.
        const std::string text = std::to_string(count);
        const float scale = size / 16.0f;
        m_ui.textWithShadow(text,
                            x + size + scale - UIRenderer::textWidth(text, scale),
                            y + size - UIRenderer::textHeight(scale) + scale,
                            scale, TEXT_COLOR);
    }
}

// A flat, front-on view of the player built from the skin texture. The
// real article is a rotating 3D model; at this size the difference is a
// few pixels of perspective, and a flat doll needs no second camera, no
// depth buffer and no viewport juggling in the middle of a 2D pass.
void Application::drawPlayerDoll(const PlayerSkin& skin, float x, float y, float unit)
{
    const unsigned int texture = skin.textureId();
    if (!texture) return;

    const float arm = static_cast<float>(skin.armWidth());

    auto part = [&](float px, float py, float pw, float ph, PlayerSkin::Patch patch, bool mirror) {
        const float u0 = mirror ? patch.u1 : patch.u0;
        const float u1 = mirror ? patch.u0 : patch.u1;
        m_ui.texturedQuad(texture, x + px * unit, y + py * unit, pw * unit, ph * unit,
                          u0, patch.v0, u1, patch.v1, glm::vec4(1.0f));
    };

    // Measured in skin pixels from the top-left of the doll: the torso is
    // 8 wide, so the arms hang at -arm and +8 either side of it.
    part(arm,            0.0f,  8.0f, 8.0f,  skin.headFront(), false);
    part(arm,            8.0f,  8.0f, 12.0f, skin.bodyFront(), false);
    part(0.0f,           8.0f,  arm,  12.0f, skin.armFront(),  true);
    part(arm + 8.0f,     8.0f,  arm,  12.0f, skin.armFront(),  false);
    part(arm,            20.0f, 4.0f, 12.0f, skin.legFront(),  true);
    part(arm + 4.0f,     20.0f, 4.0f, 12.0f, skin.legFront(),  false);
}

void Application::drawPlayerFigure(const PlayerSkin& skin, float x1, float y1, float x2,
                                   float y2, float size)
{
    if (!skin.textureId()) return;

    // 1.21's: the middle of the box is the middle of the figure, which is
    // half its height and a sixteenth above its feet. The body turns a
    // twentieth of the way the cursor is, the head twice that, and the
    // head tips up or down to meet it -- all through an arctangent over
    // forty pixels -- and the whole figure leans about its feet a
    // twentieth of the way the cursor is above or below.
    const float centreX = (x1 + x2) * 0.5f;
    const float centreY = (y1 + y2) * 0.5f;
    const float feetY = centreY + (1.8f * 0.5f + 0.0625f) * size;
    const float blockPixels = size * 0.9375f;

    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    const float across = std::atan((centreX - mouseX) / 40.0f);
    const float upDown = std::atan((centreY - mouseY) / 40.0f);
    const float lean = glm::radians(upDown * 20.0f);
    const float leanCos = std::cos(lean);
    const float leanSin = std::sin(lean);

    PlayerPose pose;
    pose.bodyYaw = 90.0f + across * 20.0f;
    pose.headYaw = 90.0f + across * 40.0f;
    pose.headPitch = upDown * 20.0f;

    struct Triangle { glm::vec2 corner[3]; glm::vec2 uv[3]; float shade; float depth;
                      unsigned int texture; glm::vec3 tint; bool glint = false; };
    std::vector<Triangle> drawn;

    auto gather = [&](unsigned int texture, const glm::vec3& tint, float nudge, bool glint = false) {
        const std::vector<float>& v = m_dollModel.vertices();
        const int stride = PlayerModel::FLOATS_PER_VERTEX;
        const int triangles = static_cast<int>(v.size()) / (stride * 3);
        for (int t = 0; t < triangles; ++t)
        {
            const float* a = &v[static_cast<size_t>(t * 3 * stride)];
            auto leaned = [&](const float* q) {
                return glm::vec3(q[0], q[1] * leanCos - q[2] * leanSin,
                                 q[1] * leanSin + q[2] * leanCos);
            };
            const glm::vec3 p0 = leaned(a);
            const glm::vec3 p1 = leaned(a + stride);
            const glm::vec3 p2 = leaned(a + 2 * stride);
            const glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
            if (normal.z <= 0.0f) continue;

            Triangle tri;
            const glm::vec3 points[3] = { p0, p1, p2 };
            for (int k = 0; k < 3; ++k)
            {
                tri.corner[k] = glm::vec2(centreX + points[k].x * blockPixels,
                                          feetY - points[k].y * blockPixels);
                tri.uv[k] = glm::vec2(a[k * stride + 3], a[k * stride + 4]);
            }
            tri.shade = a[5];
            tri.depth = (p0.z + p1.z + p2.z) / 3.0f + nudge;
            tri.texture = texture;
            tri.tint = tint;
            tri.glint = glint;
            drawn.push_back(tri);
        }
    };

    m_dollModel.build(skin, pose, 1.0f, 0.0f);
    gather(skin.textureId(), glm::vec3(1.0f), 0.0f);

    static const char* LAYER_ONE[6] = { nullptr, "../models/armor/leather_layer_1", "../models/armor/gold_layer_1",
                                        "../models/armor/iron_layer_1", "../models/armor/diamond_layer_1",
                                        "../models/armor/netherite_layer_1" };
    static const char* LAYER_TWO[6] = { nullptr, "../models/armor/leather_layer_2", "../models/armor/gold_layer_2",
                                        "../models/armor/iron_layer_2", "../models/armor/diamond_layer_2",
                                        "../models/armor/netherite_layer_2" };
    for (int slot = 0; slot < Inventory::ARMOR_SLOTS; ++slot)
    {
        const ItemStack& worn = m_inventory.slot(Inventory::ARMOR_FIRST + slot);
        if (worn.empty()) continue;
        const int tier = static_cast<int>(Armour::tierOf(worn.id));
        const int piece = static_cast<int>(Armour::pieceOf(worn.id));
        if (tier <= 0 || tier > 5 || piece <= 0) continue;
        const GuiImage* image = m_gui.image(piece == 3 ? LAYER_TWO[tier] : LAYER_ONE[tier]);
        if (!image) continue;
        m_dollModel.buildArmour(pose, piece, 1.0f, 0.0f);
        const bool leather = tier == 1;
        gather(image->texture, leather ? glm::vec3(0.627f, 0.396f, 0.251f) : glm::vec3(1.0f), 0.01f,
               worn.enchantments != 0);
        if (leather)
        {
            const GuiImage* overlay = m_gui.image(piece == 3 ? "../models/armor/leather_layer_2_overlay"
                                                             : "../models/armor/leather_layer_1_overlay");
            if (overlay) gather(overlay->texture, glm::vec3(1.0f), 0.02f);
        }
    }

    {
        const ItemStack& chest = m_inventory.slot(Inventory::ARMOR_FIRST + Armour::slotFor(Items::Elytra));
        const GuiImage* wings = m_gui.image("../entity/elytra");
        if (!chest.empty() && chest.id == Items::Elytra && wings)
        {
            m_dollModel.buildElytra(pose, 1.0f, 0.0f);
            gather(wings->texture, glm::vec3(1.0f), -0.01f, chest.enchantments != 0);
        }
    }

    std::stable_sort(drawn.begin(), drawn.end(),
                     [](const Triangle& l, const Triangle& r) { return l.depth < r.depth; });

    for (const Triangle& tri : drawn)
        m_ui.texturedTriangle(tri.texture, tri.corner, tri.uv,
                              glm::vec4(tri.tint * tri.shade, 1.0f));

    glm::vec2 lo(1e9f), hi(-1e9f);
    for (const Triangle& tri : drawn)
        if (tri.glint)
            for (const glm::vec2& c : tri.corner) { lo = glm::min(lo, c); hi = glm::max(hi, c); }
    if (hi.x > lo.x)
    {
        m_ui.beginGlint(glintTexture(true), lo.x, lo.y, hi.x - lo.x, hi.y - lo.y, glintShift());
        for (const Triangle& tri : drawn)
            if (tri.glint) m_ui.texturedTriangle(tri.texture, tri.corner, tri.uv, glm::vec4(1.0f));
        m_ui.endGlint();
    }
}

void Application::drawMobFigure(const Mob& mob, float x1, float y1, float x2, float y2, float size)
{
    const auto layers = m_mobModel.figure(mob);
    if (layers.empty()) return;

    const float centreX = (x1 + x2) * 0.5f;
    const float centreY = (y1 + y2) * 0.5f;
    const float feetY = centreY + (mob.height() * 0.5f + 0.0625f) * size;
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float across = std::atan((centreX - mouseX) / 40.0f);
    const float turn = across * glm::radians(20.0f) + glm::radians(35.0f);
    const float turnCos = std::cos(turn), turnSin = std::sin(turn);

    const float yaw = glm::radians(mob.yaw());
    const glm::vec3 forward(std::cos(yaw), 0.0f, std::sin(yaw));
    const glm::vec3 right(-forward.z, 0.0f, forward.x);
    const glm::vec3 feet = mob.position();

    struct Triangle { glm::vec2 corner[3]; glm::vec2 uv[3]; float shade; float depth; unsigned int texture; };
    std::vector<Triangle> drawn;
    for (size_t layer = 0; layer < layers.size(); ++layer)
    {
        const std::vector<float>& v = layers[layer].second;
        const int stride = 9;
        const size_t triangles = v.size() / (stride * 3);
        for (size_t t = 0; t < triangles; ++t)
        {
            const float* a = &v[t * 3 * stride];
            glm::vec3 points[3];
            for (int k = 0; k < 3; ++k)
            {
                const glm::vec3 d = glm::vec3(a[k * stride], a[k * stride + 1], a[k * stride + 2]) - feet;
                const float x = -glm::dot(d, right), z = glm::dot(d, forward);
                points[k] = glm::vec3(x * turnCos + z * turnSin, d.y, -x * turnSin + z * turnCos);
            }
            const glm::vec3 normal = glm::cross(points[1] - points[0], points[2] - points[0]);
            if (normal.z <= 0.0f) continue;
            Triangle tri;
            for (int k = 0; k < 3; ++k)
            {
                tri.corner[k] = glm::vec2(centreX + points[k].x * size, feetY - points[k].y * size);
                tri.uv[k] = glm::vec2(a[k * stride + 3], a[k * stride + 4]);
            }
            tri.shade = a[5];
            tri.depth = (points[0].z + points[1].z + points[2].z) / 3.0f + static_cast<float>(layer) * 0.02f;
            tri.texture = layers[layer].first;
            drawn.push_back(tri);
        }
    }
    std::stable_sort(drawn.begin(), drawn.end(),
                     [](const Triangle& l, const Triangle& r) { return l.depth < r.depth; });
    for (const Triangle& tri : drawn)
        m_ui.texturedTriangle(tri.texture, tri.corner, tri.uv, glm::vec4(glm::vec3(tri.shade), 1.0f));
}

void Application::drawCraftArrow(float x, float y, float width, float height, float filled)
{
    // The client's arrow: a grey shaft into a head as tall as the arrow,
    // drawn in the slot grey so it reads as part of the panel. Built a
    // column at a time, which at this size is what the sprite is -- and
    // which is what lets a furnace's fill it white from the left as the
    // thing in it cooks.
    const float head = std::floor(height * 0.5f + 0.5f);
    const float headStart = x + width - head;
    const float shaftHeight = std::max(2.0f, std::floor(height / 3.0f));
    const float middle = y + std::floor(height * 0.5f);
    const float fillEdge = x + width * std::clamp(filled, 0.0f, 1.0f);

    for (float column = x; column < x + width; column += 1.0f)
    {
        const glm::vec4 colour = column < fillEdge ? glm::vec4(1.0f) : GUI_WELL;
        if (column < headStart)
        {
            m_ui.quad(column, middle - std::floor(shaftHeight * 0.5f), 1.0f, shaftHeight, colour);
        }
        else
        {
            const float half = std::floor((head - (column - headStart)) - 0.5f);
            m_ui.quad(column, middle - half, 1.0f, half * 2.0f + 1.0f, colour);
        }
    }
}

void Application::drawFurnaceFlame(float x, float y, float alight)
{
    // Fourteen columns of flame, tallest in the middle, yellow at the
    // root and red at the tips -- and shown from the bottom up only as
    // far as the fuel has left, so it burns down the way the client's
    // does rather than draining like a bar.
    static const float COLUMN[14] = { 4, 7, 9, 11, 12, 13, 14, 13, 14, 12, 10, 8, 6, 3 };
    const float level = 14.0f * std::clamp(alight, 0.0f, 1.0f);
    const glm::vec4 root(1.0f, 0.86f, 0.30f, 1.0f);
    const glm::vec4 tip(0.86f, 0.24f, 0.08f, 1.0f);

    for (int i = 0; i < 14; ++i)
    {
        const float shown = std::min(COLUMN[i], level);
        if (shown <= 0.0f) continue;
        const float top = y + 14.0f - shown;
        const float along = shown / COLUMN[i];
        m_ui.gradient(x + i, top, 1.0f, shown, glm::mix(root, tip, along), root);
    }
}

bool Application::creativeSearching() const
{
    return m_inventoryOpen && m_player.creative() && m_creativeTab == SEARCH_TAB &&
           !m_chestOpen && !m_crafterOpen && !m_furnaceOpen && m_inventory.craftSize() < 3 &&
           !m_tradeOpen && !m_enchantOpen && !m_brewOpen && !m_anvilOpen;
}

int Application::creativeInventoryTab()
{
    return INVENTORY_TAB;
}

void Application::renderInventoryScreen()
{
    // Interface pixels, the same ones the HUD is drawn in, and the same
    // ones the open-source client lays its containers out in.
    m_ui.setScale(static_cast<float>(guiScale()));

    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();

    m_slotBoxes.clear();
    m_hintBoxes.clear();

    // The wash behind every screen the client draws over a world: a
    // gradient from C0101010 at the top to D0101010 at the bottom, so
    // the world and the HUD stay faintly there under the panel.
    m_ui.gradient(0.0f, 0.0f, w, h,
                  glm::vec4(0.063f, 0.063f, 0.063f, 0.753f),
                  glm::vec4(0.063f, 0.063f, 0.063f, 0.816f));

    // Creative has a screen of its own for the player's own things.
    // A container opened in creative still shows the container's own
    // screen, as it does in Minecraft: the palette used to take over
    // there too, so a creative player could open a chest and never
    // see what was in it.
    const bool container = m_chestOpen || m_crafterOpen || m_furnaceOpen ||
                           m_inventory.craftSize() >= 3 || m_tradeOpen ||
                           m_enchantOpen || m_brewOpen || m_anvilOpen || m_horseOpen || m_beaconOpen || m_grindOpen;

    if (m_player.creative() && !container) renderCreativeScreen(w, h);
    else renderContainerScreen(w, h);

    finishInventoryScreen();
    m_ui.setScale(1.0f);
}

void Application::renderContainerScreen(float w, float h)
{
    // Every number below is one of the client's own: a panel 176 by
    // 166, the player's storage at 8 across and 84 down, the hotbar at
    // 142, the armour column at 8 and 8, the little crafting grid at
    // 88 and 26 with its result at 144 and 36. Lining up with them is
    // what makes a screenshot of this sit on top of a screenshot of
    // that.
    const bool bench = m_inventory.craftSize() >= 3;
    const BlockId chestKind = m_chestOpen && m_world
        ? m_world->getBlock(m_chestBlock.x, m_chestBlock.y, m_chestBlock.z) : Blocks::Air;
    const BlockId furnaceKind = m_furnaceOpen && m_world
        ? furnaceBase(m_world->getBlock(m_furnaceBlock.x, m_furnaceBlock.y, m_furnaceBlock.z)) : Blocks::Air;
    const bool hopper = chestKind == Blocks::Hopper;
    const bool thrower = chestKind == Blocks::Dropper || chestKind == Blocks::Dispenser;
    const bool shulker = isShulkerBox(chestKind);
    const bool bigChest = m_chestOpen && !hopper && !thrower && !shulker;

    const char* background = "container/inventory";
    float textureW = 256.0f;
    if (m_chestOpen)
        background = hopper ? "container/hopper" : thrower ? "container/dispenser"
                   : shulker ? "container/shulker_box" : "container/generic_54";
    else if (m_crafterOpen) background = "container/crafter";
    else if (m_horseOpen) background = "container/horse";
    else if (m_beaconOpen) background = "container/beacon";
    else if (m_grindOpen) background = "container/grindstone";
    else if (m_tradeOpen) { background = "container/villager"; textureW = 512.0f; }
    else if (m_enchantOpen) background = "container/enchanting_table";
    else if (m_brewOpen) background = "container/brewing_stand";
    else if (m_anvilOpen) background = "container/anvil";
    else if (m_furnaceOpen)
        background = (furnaceKind == Blocks::Smoker || furnaceKind == Blocks::SmokerLit) ? "container/smoker"
                   : (furnaceKind == Blocks::BlastFurnace || furnaceKind == Blocks::BlastFurnaceLit) ? "container/blast_furnace"
                   : "container/furnace";
    else if (bench) background = "container/crafting_table";
    const bool skin = m_gui.has(background);

    const char* litSprite = "sprites/container/furnace/lit_progress";
    const char* burnSprite = "sprites/container/furnace/burn_progress";
    if (std::strcmp(background, "container/smoker") == 0)
    {
        litSprite = "sprites/container/smoker/lit_progress";
        burnSprite = "sprites/container/smoker/burn_progress";
    }
    if (std::strcmp(background, "container/blast_furnace") == 0)
    {
        litSprite = "sprites/container/blast_furnace/lit_progress";
        burnSprite = "sprites/container/blast_furnace/burn_progress";
    }

    const float PANEL_W = m_beaconOpen ? 230.0f : 176.0f;
    const float PANEL_H = m_beaconOpen ? 219.0f : hopper ? 133.0f : bigChest ? 168.0f : 166.0f;

    const float TRADE_LIST_W = 100.0f;
    const float panelWidth = m_tradeOpen ? PANEL_W + TRADE_LIST_W : PANEL_W;
    const float panelX = std::floor((w - panelWidth) * 0.5f);
    const float panelY = std::floor((h - PANEL_H) * 0.5f);
    const float shift = m_tradeOpen && skin ? TRADE_LIST_W : 0.0f;

    const float storageY = panelY + (m_beaconOpen ? 137.0f : hopper ? 51.0f : bigChest ? 85.0f : 84.0f);
    const float hotbarY = panelY + (m_beaconOpen ? 195.0f : hopper ? 109.0f : bigChest ? 143.0f : 142.0f);
    const float invX = panelX + (m_beaconOpen ? 36.0f : 8.0f) + shift;

    auto well = [this, skin](float x, float y) {
        if (!skin) guiWell(x - 1.0f, y - 1.0f, PITCH, PITCH);
    };
    auto slotBox = [this](SlotBox::Kind kind, int index, float x, float y) {
        m_slotBoxes.push_back({ kind, index, x - 1.0f, y - 1.0f, PITCH });
    };
    auto drawSlot = [&](int index, float x, float y) {
        well(x, y);
        const ItemStack& stack = m_inventory.slot(index);
        drawItem(x, y, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage, stack.enchantments);
        slotBox(SlotBox::Kind::Slot, index, x, y);
    };
    auto drawOutputSlot = [&](int index, float x, float y) {
        if (!skin) guiWell(x - 5.0f, y - 5.0f, 26.0f, 26.0f);
        const ItemStack& stack = m_inventory.slot(index);
        drawItem(x, y, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage, stack.enchantments);
        slotBox(SlotBox::Kind::Slot, index, x, y);
    };
    auto centredTitle = [&](const std::string& text) {
        return panelX + std::floor((panelWidth - UIRenderer::textWidth(text, 1.0f)) * 0.5f);
    };

    if (skin && bigChest)
    {
        guiBlit(background, panelX, panelY, 0.0f, 0.0f, PANEL_W, 3.0f * PITCH + 17.0f);
        guiBlit(background, panelX, panelY + 3.0f * PITCH + 17.0f, 0.0f, 126.0f, PANEL_W, 96.0f);
    }
    else if (skin)
        guiBlit(background, panelX, panelY, 0.0f, 0.0f, panelWidth, PANEL_H, textureW, 256.0f);
    else
        guiPanel(panelX, panelY, panelWidth, PANEL_H);

    // Dark text with no shadow, at the client's own two corners: the
    // container's name eight in and six down, and the word for the
    // player's own rows ninety-four up from the bottom edge.
    // The player's own screen is the one the client gives no heading
    // at all: it labels the little grid "Crafting" over the top of it
    // and leaves the rest bare. Everything else gets its name in the
    // corner and the word for the player's rows above them.
    // A workbench's screen is the one with a three by three in it,
    // which is the difference that actually shows: the flag is set
    // when one is opened in the world and not when a dev screen asks
    // for the same grid.
    const bool plainInventory = !m_chestOpen && !m_crafterOpen &&
                                !m_furnaceOpen && !bench && !m_tradeOpen &&
                                !m_enchantOpen && !m_brewOpen && !m_anvilOpen && !m_horseOpen && !m_beaconOpen && !m_grindOpen;

    // Every screen is headed with the name of what was opened, in the
    // top-left corner, so a barrel says Barrel and a smoker says Smoker.
    if (!plainInventory && !m_beaconOpen)
    {
        std::string title = "Inventory";
        if (m_chestOpen && m_world)
            title = displayName(m_world->getBlock(m_chestBlock.x, m_chestBlock.y, m_chestBlock.z));
        else if (m_furnaceOpen && m_world)
            title = displayName(m_world->getBlock(m_furnaceBlock.x, m_furnaceBlock.y, m_furnaceBlock.z));
        else if (m_crafterOpen) title = "Crafter";
        else if (m_horseOpen) title = "Horse";
        else if (m_grindOpen) title = "Repair & Disenchant";
        else if (m_brewOpen) title = "Brewing Stand";
        else if (m_anvilOpen) title = "Anvil";
        else if (m_enchantOpen) title = "Enchanting Table";
        else if (m_tradeOpen) title.clear();
        else if (bench) title = "Crafting Table";

        if (!m_tradeOpen)
        {
            if (title.empty() || title == "Air") title = m_chestOpen ? "Chest" : "Furnace";
            float titleX = panelX + 8.0f;
            if (skin)
            {
                if (bench) titleX = panelX + 29.0f;
                else if (m_anvilOpen) titleX = panelX + 60.0f;
                else if (m_furnaceOpen || m_brewOpen || m_crafterOpen || thrower) titleX = centredTitle(title);
            }
            m_ui.text(title, titleX, panelY + 6.0f, 1.0f, GUI_TEXT);
        }
    }

    // The word over the player's own rows. The client leaves it off two
    // screens: an anvil's, so that its price has somewhere to go, and the
    // player's own, where it was drawn under the character and the tail
    // of its y poked out below the recess as a stray dash.
    if (!plainInventory && !m_beaconOpen)
        m_ui.text("Inventory", m_tradeOpen && skin ? panelX + 107.0f : panelX + 8.0f,
                  panelY + PANEL_H - 94.0f, 1.0f, GUI_TEXT);


    // --------------------------------------------- the open chest ---
    // Three rows of nine, eighteen down from the top of the panel,
    // directly above the player's own rows.
    if (m_chestOpen && hopper)
    {
        for (int column = 0; column < Hopper::HOPPER_SLOTS; ++column)
            drawSlot(Inventory::CHEST_FIRST + column, panelX + 44.0f + column * PITCH, panelY + 20.0f);
    }
    else if (m_chestOpen && thrower)
    {
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
                drawSlot(Inventory::CHEST_FIRST + row * 3 + column,
                         panelX + 62.0f + column * PITCH, panelY + 17.0f + row * PITCH);
    }
    else if (m_chestOpen)
    {
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < PALETTE_COLUMNS; ++column)
                drawSlot(Inventory::CHEST_FIRST + row * PALETTE_COLUMNS + column,
                         panelX + 8.0f + column * PITCH, panelY + 18.0f + row * PITCH);
    }

    if (m_horseOpen)
    {
        const float saddleX = panelX + 8.0f, saddleY = panelY + 18.0f;
        const float armourX = panelX + 8.0f, armourY = panelY + 36.0f;
        if (!guiSprite("sprites/container/horse/saddle_slot", saddleX - 1.0f, saddleY - 1.0f, PITCH, PITCH))
            well(saddleX, saddleY);
        if (!guiSprite("sprites/container/horse/armor_slot", armourX - 1.0f, armourY - 1.0f, PITCH, PITCH))
            well(armourX, armourY);
        const ItemStack& saddle = m_inventory.slot(Inventory::HORSE_SADDLE);
        const ItemStack& armour = m_inventory.slot(Inventory::HORSE_ARMOUR);
        if (!saddle.empty()) drawItem(saddleX, saddleY, SLOT, saddle.id, saddle.count, glm::vec4(1.0f));
        if (!armour.empty()) drawItem(armourX, armourY, SLOT, armour.id, armour.count, glm::vec4(1.0f));
        slotBox(SlotBox::Kind::Slot, Inventory::HORSE_SADDLE, saddleX, saddleY);
        slotBox(SlotBox::Kind::Slot, Inventory::HORSE_ARMOUR, armourX, armourY);
        for (Mob& mob : m_entities.mobs())
            if (mob.typeId() == MobId::Horse && mob.birthSeed() == m_horseSeed && mob.alive())
            {
                mob.setSaddled(!saddle.empty() && saddle.id == Items::Saddle);
                mob.setHorseArmour(!armour.empty() && isHorseArmour(armour.id) ? armour.id : 0);
                if (!skin) guiWell(panelX + 25.0f, panelY + 17.0f, 54.0f, 54.0f);
                drawMobFigure(mob, panelX + 26.0f, panelY + 18.0f, panelX + 78.0f, panelY + 70.0f, 17.0f);
                break;
            }
    }

    if (m_beaconOpen)
    {
        const int tiers = beaconTiers();
        const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
        const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
        static const char* ICONS[Effects::KIND_COUNT] = {
            "../mob_effect/speed", "../mob_effect/slowness", "../mob_effect/strength",
            "../mob_effect/weakness", "../mob_effect/regeneration", "../mob_effect/poison",
            "../mob_effect/fire_resistance", "../mob_effect/water_breathing",
            "../mob_effect/night_vision", "../mob_effect/jump_boost", "../mob_effect/bad_omen",
            "../mob_effect/haste", "../mob_effect/resistance",
        };
        const glm::vec4 label(0.878f, 0.878f, 0.878f, 1.0f);
        const std::string primaryLabel = "Primary Power";
        const std::string secondaryLabel = "Secondary Power";
        m_ui.textWithShadow(primaryLabel, panelX + 62.0f - std::floor(UIRenderer::textWidth(primaryLabel, 1.0f) * 0.5f),
                            panelY + 10.0f, 1.0f, label);
        m_ui.textWithShadow(secondaryLabel, panelX + 169.0f - std::floor(UIRenderer::textWidth(secondaryLabel, 1.0f) * 0.5f),
                            panelY + 10.0f, 1.0f, label);

        auto button = [&](float bx, float by, bool active, bool selected, const char* icon, int kind, int index,
                          const std::string& tip) {
            const bool over = mouseX >= bx && mouseX < bx + 22.0f && mouseY >= by && mouseY < by + 22.0f;
            const char* face = !active ? "sprites/container/beacon/button_disabled"
                             : selected ? "sprites/container/beacon/button_selected"
                             : over ? "sprites/container/beacon/button_highlighted"
                                    : "sprites/container/beacon/button";
            if (!guiSprite(face, bx, by, 22.0f, 22.0f))
                bevel(bx, by, 22.0f, 22.0f, selected ? GUI_PANEL_DARK : GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 1.0f);
            if (icon) guiSprite(icon, bx + 2.0f, by + 2.0f, 18.0f, 18.0f);
            if (active)
                m_slotBoxes.push_back({ static_cast<SlotBox::Kind>(kind), index, bx, by, 22.0f });
            if (over && !tip.empty()) addHint(bx, by, 22.0f, 22.0f, tip);
        };

        for (int tier = 0; tier < 3; ++tier)
        {
            const std::vector<Effects::Kind> powers = Beacon::powersAt(tier);
            const float count = static_cast<float>(powers.size());
            const float width = count * 22.0f + (count - 1.0f) * 2.0f;
            for (size_t k = 0; k < powers.size(); ++k)
            {
                const int power = static_cast<int>(powers[k]);
                button(panelX + 76.0f + static_cast<float>(k) * 24.0f - std::floor(width * 0.5f),
                       panelY + 22.0f + static_cast<float>(tier) * 25.0f, tiers > tier, m_beaconPrimary == power,
                       ICONS[power], static_cast<int>(SlotBox::Kind::BeaconPower), power,
                       Effects::name(powers[k]));
            }
        }

        {
            const float width = 2.0f * 22.0f + 2.0f;
            const int regen = static_cast<int>(Effects::Kind::Regeneration);
            const bool open = tiers >= Beacon::MAX_TIERS && m_beaconPrimary >= 0;
            button(panelX + 167.0f - std::floor(width * 0.5f), panelY + 47.0f, open, m_beaconSecondary == regen,
                   ICONS[regen], static_cast<int>(SlotBox::Kind::BeaconPower), 100 + regen, "Regeneration");
            if (m_beaconPrimary >= 0)
                button(panelX + 167.0f + 24.0f - std::floor(width * 0.5f), panelY + 47.0f, open,
                       m_beaconSecondary == m_beaconPrimary, ICONS[m_beaconPrimary],
                       static_cast<int>(SlotBox::Kind::BeaconPower), 100 + m_beaconPrimary,
                       std::string(Effects::name(static_cast<Effects::Kind>(m_beaconPrimary))) + " II");
        }

        const bool paid = !m_inventory.slot(Inventory::BEACON_PAYMENT).empty();
        button(panelX + 164.0f, panelY + 107.0f, paid && m_beaconPrimary >= 0, false,
               "sprites/container/beacon/confirm", static_cast<int>(SlotBox::Kind::BeaconConfirm), 0, "Done");
        button(panelX + 190.0f, panelY + 107.0f, true, false, "sprites/container/beacon/cancel",
               static_cast<int>(SlotBox::Kind::BeaconCancel), 0, "Cancel");

        drawItem(panelX + 20.0f, panelY + 109.0f, SLOT, Items::NetheriteIngot, 0, glm::vec4(1.0f));
        drawItem(panelX + 41.0f, panelY + 109.0f, SLOT, Items::Emerald, 0, glm::vec4(1.0f));
        drawItem(panelX + 63.0f, panelY + 109.0f, SLOT, Items::Diamond, 0, glm::vec4(1.0f));
        drawItem(panelX + 86.0f, panelY + 109.0f, SLOT, Items::GoldIngot, 0, glm::vec4(1.0f));
        drawItem(panelX + 108.0f, panelY + 109.0f, SLOT, Items::IronIngot, 0, glm::vec4(1.0f));
        drawSlot(Inventory::BEACON_PAYMENT, panelX + 136.0f, panelY + 110.0f);
    }

    if (m_grindOpen)
    {
        drawSlot(Inventory::GRIND_TOP, panelX + 49.0f, panelY + 19.0f);
        drawSlot(Inventory::GRIND_BOTTOM, panelX + 49.0f, panelY + 40.0f);
        const Grindstone::Result made = grindResult();
        const float rx = panelX + 129.0f, ry = panelY + 34.0f;
        well(rx, ry);
        if (made.valid)
        {
            drawItem(rx, ry, SLOT, made.id, made.count, glm::vec4(1.0f), made.damage, made.enchantments);
            m_slotBoxes.push_back({ SlotBox::Kind::GrindTake, Inventory::GRIND_RESULT, rx - 1.0f, ry - 1.0f, PITCH });
        }
        else if (!m_inventory.slot(Inventory::GRIND_TOP).empty() || !m_inventory.slot(Inventory::GRIND_BOTTOM).empty())
        {
            if (!guiSprite("sprites/container/grindstone/error", panelX + 92.0f, panelY + 31.0f, 28.0f, 21.0f))
            {
                m_ui.quad(panelX + 98.0f, panelY + 34.0f, 16.0f, 3.0f, glm::vec4(0.8f, 0.1f, 0.1f, 1.0f));
                m_ui.quad(panelX + 98.0f, panelY + 44.0f, 16.0f, 3.0f, glm::vec4(0.8f, 0.1f, 0.1f, 1.0f));
            }
        }
    }

    // ------------------------------------------ the open crafter ---
    // 1.21's: the three by three at 26 and 17, the redstone that says
    // whether it is powered at 97 and 35, and at 134 and 35 what the
    // next pulse will make. That slot only ever shows: a crafter is
    // worked by a pulse rather than by hand.
    if (m_crafterOpen)
    {
        ItemStack grid[9];
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
            {
                const int cell = row * 3 + column;
                const int index = Inventory::CHEST_FIRST + cell;
                const float sx = panelX + 26.0f + column * PITCH;
                const float sy = panelY + 17.0f + row * PITCH;
                drawSlot(index, sx, sy);
                grid[cell] = m_inventory.slot(index);
                if (crafterSlotDisabled(cell))
                {
                    if (!guiSprite("sprites/container/crafter/disabled_slot", sx - 1.0f, sy - 1.0f, PITCH, PITCH))
                        bevel(sx - 1.0f, sy - 1.0f, PITCH, PITCH, GUI_PANEL, GUI_PANEL_LIGHT,
                              GUI_PANEL_DARK, 1.0f);
                    addHint(sx - 1.0f, sy - 1.0f, PITCH, PITCH, "Disabled: click to enable");
                }
                else if (grid[cell].empty())
                    addHint(sx - 1.0f, sy - 1.0f, PITCH, PITCH, "Click to disable this slot");
            }

        const bool powered = m_world->getBlock(m_crafterBlock.x, m_crafterBlock.y,
                                              m_crafterBlock.z) == Blocks::CrafterLit;
        if (skin)
        {
            guiSprite(powered ? "sprites/container/crafter/powered_redstone"
                              : "sprites/container/crafter/unpowered_redstone",
                      panelX + 97.0f, panelY + 35.0f, 16.0f, 16.0f);
            addHint(panelX + 97.0f, panelY + 35.0f, 16.0f, 16.0f,
                    powered ? "Powered" : "Not powered: a redstone pulse crafts once");
        }
        else
        {
            drawCraftArrow(panelX + 94.0f, panelY + 35.0f, 22.0f, 15.0f);
            drawItem(panelX + 97.0f, panelY + 16.0f, SLOT, Items::Redstone, 1,
                     powered ? glm::vec4(1.0f) : glm::vec4(0.35f, 0.35f, 0.35f, 1.0f));
            addHint(panelX + 96.0f, panelY + 15.0f, PITCH, PITCH,
                    powered ? "Powered" : "Not powered: a redstone pulse crafts once");
        }
        addHint(panelX + 129.0f, panelY + 30.0f, 26.0f, 26.0f, "What the next pulse makes");

        const float ox = panelX + 134.0f;
        const float oy = panelY + 35.0f;
        if (!skin) guiWell(ox - 5.0f, oy - 5.0f, 26.0f, 26.0f);
        const CraftOutput next = Crafting::match(grid, 3);
        if (next.valid())
            drawItem(ox, oy, SLOT, next.id, next.count, glm::vec4(1.0f));
    }

    // ------------------------------- armour, body preview, crafting ---
    if (!m_chestOpen && !m_crafterOpen)
    {
        // The armour column and the character are the player's own
        // screen and nothing else: the client draws neither on a
        // workbench, a furnace, an anvil or a trade. They used to be
        // drawn on all of them, which is how a furnace came to have a
        // man standing in the middle of its fire.
        const bool otherBusiness = m_tradeOpen || m_enchantOpen || m_brewOpen ||
                                   m_anvilOpen || m_furnaceOpen || bench || m_horseOpen || m_beaconOpen || m_grindOpen;

        // Armour down the left edge, at 8 and 8, a slot apart.
        for (int i = 0; !otherBusiness && i < Inventory::ARMOR_SLOTS; ++i)
        {
            const float x = panelX + 8.0f;
            const float y = panelY + 8.0f + i * PITCH;
            const int index = Inventory::ARMOR_FIRST + i;

            well(x, y);

            const ItemStack& stack = m_inventory.slot(index);
            if (stack.empty())
            {
                // A faint silhouette of the piece that belongs here, the
                // way Minecraft marks its empty armour slots.
                m_ui.texturedQuad(m_gui.texture(ARMOR_GHOSTS[i]), x, y, SLOT, SLOT,
                                  0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
            }
            else
            {
                drawItem(x, y, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage, stack.enchantments);
            }

            slotBox(SlotBox::Kind::Slot, index, x, y);
        }

        // The character, in the recess the client leaves for it: a box
        // from 26 and 8 across to 76 and 78, with the figure standing
        // on 75.
        if (!otherBusiness)
        {
            const float offX = panelX + 77.0f;
            const float offY = panelY + 62.0f;
            well(offX, offY);
            const ItemStack& offhand = m_inventory.slot(Inventory::OFFHAND);
            if (offhand.empty())
                if (!guiSprite("sprites/container/slot/shield", offX, offY, SLOT, SLOT))
                    guiSprite("../item/empty_armor_slot_shield", offX, offY, SLOT, SLOT);
            else
                drawItem(offX, offY, SLOT, offhand.id, offhand.count, glm::vec4(1.0f), offhand.damage, offhand.enchantments);
            slotBox(SlotBox::Kind::Slot, Inventory::OFFHAND, offX, offY);
        }

        if (!otherBusiness)
        {
            // Thirty to the block, which is the client's scale for this
            // figure, less the sixteenth the player model is shrunk by.
            if (!skin) guiWell(panelX + 25.0f, panelY + 7.0f, 52.0f, 72.0f);
            drawPlayerFigure(activeSkin(), panelX + 26.0f, panelY + 8.0f, panelX + 75.0f,
                             panelY + 78.0f, 30.0f);
        }

        // Trading. The villager's rank and progress across the top, the
        // payment slots and what they buy in the middle, and the list of
        // every deal it will make down the right-hand side.
        if (m_tradeOpen && skin)
        {
            const std::vector<Trading::Deal> deals = Trading::dealsFor(m_tradeWith);
            TraderRecord& record = openTrader();
            const int rank = Trading::rankFor(record.xp);
            const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
            const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
            const bool chosen = m_tradeSelected >= 0 && m_tradeSelected < static_cast<int>(deals.size());

            const std::string header = std::string(Trading::name(m_tradeWith)) + " - " +
                                       Trading::rankName(rank);
            m_ui.text(header, panelX + 49.0f + 138.0f - std::floor(UIRenderer::textWidth(header, 1.0f) * 0.5f),
                      panelY + 6.0f, 1.0f, GUI_TEXT);
            const std::string tradesLabel = "Trades";
            m_ui.text(tradesLabel, panelX + 53.0f - std::floor(UIRenderer::textWidth(tradesLabel, 1.0f) * 0.5f),
                      panelY + 6.0f, 1.0f, GUI_TEXT);

            const float barX = panelX + 136.0f;
            const float barY = panelY + 16.0f;
            guiSprite("sprites/container/villager/experience_bar_background", barX, barY, 102.0f, 5.0f);
            std::string barTip = "Master: every trade is open";
            if (rank + 1 < Trading::RANKS)
            {
                const int from = Trading::xpForRank(rank);
                const int to = Trading::xpForRank(rank + 1);
                const float span = static_cast<float>(to - from);
                const float done = std::floor(102.0f * std::clamp(static_cast<float>(record.xp - from) / span, 0.0f, 1.0f));
                guiSpritePart("sprites/container/villager/experience_bar_current", 102.0f, 5.0f, 0.0f, 0.0f,
                              barX, barY, done, 5.0f);
                if (chosen)
                {
                    const float gain = std::floor(102.0f * static_cast<float>(
                        deals[static_cast<size_t>(m_tradeSelected)].villagerXp) / span);
                    guiSpritePart("sprites/container/villager/experience_bar_result", 102.0f, 5.0f, done, 0.0f,
                                  barX + done, barY, std::min(gain, 102.0f - done), 5.0f);
                }
                barTip = "Trades done: " + std::to_string(record.xp - from) + " of " +
                         std::to_string(to - from) + " to " + Trading::rankName(rank + 1);
            }
            else
            {
                guiSpritePart("sprites/container/villager/experience_bar_current", 102.0f, 5.0f, 0.0f, 0.0f,
                              barX, barY, 102.0f, 5.0f);
            }
            addHint(barX, barY - 1.0f, 102.0f, 7.0f, barTip);

            const float payY = panelY + 37.0f;
            drawSlot(Inventory::TRADE_PAY_A, panelX + 136.0f, payY);
            drawSlot(Inventory::TRADE_PAY_B, panelX + 162.0f, payY);
            if (chosen)
            {
                const Trading::Offer& offer = deals[static_cast<size_t>(m_tradeSelected)].offer;
                const glm::vec4 ghost(1.0f, 1.0f, 1.0f, 0.3f);
                if (m_inventory.slot(Inventory::TRADE_PAY_A).empty())
                    drawItem(panelX + 136.0f, payY, SLOT, offer.want, offer.wantCount, ghost);
                if (offer.also != Blocks::Air && m_inventory.slot(Inventory::TRADE_PAY_B).empty())
                    drawItem(panelX + 162.0f, payY, SLOT, offer.also, offer.alsoCount, ghost);
            }

            const bool soldOut = chosen &&
                                 record.uses[static_cast<size_t>(m_tradeSelected)] >=
                                     deals[static_cast<size_t>(m_tradeSelected)].maxUses;
            if (soldOut)
            {
                guiSprite("sprites/container/villager/out_of_stock", panelX + 182.0f, panelY + 35.0f, 28.0f, 21.0f);
                addHint(panelX + 182.0f, panelY + 35.0f, 28.0f, 21.0f, "Sold out: it restocks soon");
            }

            const float resultX = panelX + 220.0f;
            const ItemStack& made = m_inventory.slot(Inventory::TRADE_RESULT);
            drawItem(resultX, payY, SLOT, made.id, made.count, glm::vec4(1.0f), made.damage, made.enchantments);
            m_slotBoxes.push_back({ SlotBox::Kind::TradeTake, Inventory::TRADE_RESULT,
                                    resultX - 1.0f, payY - 1.0f, PITCH });

            std::vector<int> open;
            for (size_t i = 0; i < deals.size(); ++i)
                if (deals[i].rank <= rank) open.push_back(static_cast<int>(i));
            const int visibleRows = 7;
            const int maxScroll = std::max(0, static_cast<int>(open.size()) - visibleRows);

            const float listX = panelX + 5.0f;
            const float listTop = panelY + 17.0f;
            const float trackTop = panelY + 18.0f;
            if (m_draggingScroll && !m_input.isMouseButtonDown(SDL_BUTTON_LEFT)) m_draggingScroll = false;
            if (m_draggingScroll && maxScroll > 0)
            {
                const float along = std::clamp((mouseY - trackTop - 13.5f) / 113.0f, 0.0f, 1.0f);
                m_tradeScroll = static_cast<int>(std::lround(along * maxScroll));
            }
            m_tradeScroll = std::clamp(m_tradeScroll, 0, maxScroll);

            for (int r = 0; r < visibleRows; ++r)
            {
                const int at = r + m_tradeScroll;
                if (at >= static_cast<int>(open.size())) break;
                const int index = open[static_cast<size_t>(at)];
                const Trading::Deal& deal = deals[static_cast<size_t>(index)];
                const Trading::Offer& offer = deal.offer;
                const float ry = listTop + 20.0f * r;
                const bool out = record.uses[static_cast<size_t>(index)] >= deal.maxUses;
                const bool hovered = mouseX >= listX && mouseX < listX + 88.0f &&
                                     mouseY >= ry && mouseY < ry + 20.0f;

                guiSprite(hovered || index == m_tradeSelected ? "sprites/widget/button_highlighted"
                                                              : "sprites/widget/button",
                          listX, ry, 88.0f, 20.0f);

                drawItem(listX + 5.0f, ry + 2.0f, SLOT, offer.want, offer.wantCount, glm::vec4(1.0f));
                if (offer.also != Blocks::Air)
                    drawItem(listX + 35.0f, ry + 2.0f, SLOT, offer.also, offer.alsoCount, glm::vec4(1.0f));
                guiSprite(out ? "sprites/container/villager/trade_arrow_out_of_stock"
                              : "sprites/container/villager/trade_arrow",
                          listX + 55.0f, ry + 5.0f, 10.0f, 9.0f);
                drawItem(listX + 68.0f, ry + 2.0f, SLOT, offer.give, offer.giveCount, glm::vec4(1.0f), 0, offer.giveEnchantments);

                std::string tip = "Give " + std::to_string(offer.wantCount) + " " + displayName(offer.want);
                if (offer.also != Blocks::Air)
                    tip += " and " + std::to_string(offer.alsoCount) + " " + displayName(offer.also);
                tip += " for " + std::to_string(offer.giveCount) + " " + displayName(offer.give);
                if (offer.giveEnchantments != 0) tip += " (enchanted)";
                const int left = deal.maxUses - record.uses[static_cast<size_t>(index)];
                tip += out ? " - sold out" : " - " + std::to_string(left) + " left";
                addHint(listX, ry, 88.0f, 20.0f, tip);
                m_slotBoxes.push_back({ SlotBox::Kind::Trade, index, listX, ry, 20.0f, 88.0f });
            }

            const float thumbY = maxScroll > 0
                ? trackTop + std::floor(113.0f * static_cast<float>(m_tradeScroll) / static_cast<float>(maxScroll))
                : trackTop;
            guiSprite(maxScroll > 0 ? "sprites/container/villager/scroller"
                                    : "sprites/container/villager/scroller_disabled",
                      panelX + 94.0f, thumbY, 6.0f, 27.0f);
            if (maxScroll > 0)
                m_slotBoxes.push_back({ SlotBox::Kind::Scroll, 0, panelX + 94.0f, trackTop, 140.0f, 6.0f });
        }
        else if (m_tradeOpen)
        {
            const std::vector<Trading::Deal> deals = Trading::dealsFor(m_tradeWith);
            TraderRecord& record = openTrader();
            const int rank = Trading::rankFor(record.xp);
            const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
            const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();

            auto softBox = [&](float x, float y, float bw, float bh, const glm::vec4& top,
                               const glm::vec4& bottom, const glm::vec4& edge) {
                m_ui.quad(x + 1.0f, y, bw - 2.0f, bh, edge);
                m_ui.quad(x, y + 1.0f, bw, bh - 2.0f, edge);
                m_ui.gradient(x + 1.0f, y + 1.0f, bw - 2.0f, bh - 2.0f, top, bottom);
            };
            auto redCross = [&](float x, float y, float size) {
                const glm::vec4 red(0.85f, 0.18f, 0.15f, 1.0f);
                for (int k = 0; k < static_cast<int>(size); ++k)
                {
                    const float f = static_cast<float>(k);
                    m_ui.quad(x + f, y + f, 2.0f, 2.0f, red);
                    m_ui.quad(x + size - 2.0f - f, y + f, 2.0f, 2.0f, red);
                }
            };

            const std::string header = std::string(Trading::rankName(rank)) + " " +
                                       Trading::name(m_tradeWith);
            m_ui.text(header, panelX + PANEL_W * 0.5f - UIRenderer::textWidth(header, 1.0f) * 0.5f,
                      panelY + 6.0f, 1.0f, GUI_TEXT);

            const float barX = panelX + 38.0f;
            const float barY = panelY + 18.0f;
            const float barW = 100.0f;
            guiWell(barX - 1.0f, barY - 1.0f, barW + 2.0f, 7.0f);
            float progress = 1.0f;
            std::string barTip = "Master: every trade is open";
            if (rank + 1 < Trading::RANKS)
            {
                const int from = Trading::xpForRank(rank);
                const int to = Trading::xpForRank(rank + 1);
                progress = std::clamp(static_cast<float>(record.xp - from) / static_cast<float>(to - from),
                                      0.0f, 1.0f);
                barTip = "Trades done: " + std::to_string(record.xp - from) + " of " +
                         std::to_string(to - from) + " to " + Trading::rankName(rank + 1);
            }
            if (progress > 0.0f)
            {
                const bool master = rank + 1 >= Trading::RANKS;
                m_ui.gradient(barX, barY, std::floor(barW * progress), 5.0f,
                              master ? glm::vec4(1.0f, 0.86f, 0.35f, 1.0f) : glm::vec4(0.55f, 0.92f, 0.38f, 1.0f),
                              master ? glm::vec4(0.85f, 0.62f, 0.15f, 1.0f) : glm::vec4(0.28f, 0.66f, 0.18f, 1.0f));
            }
            addHint(barX - 1.0f, barY - 1.0f, barW + 2.0f, 7.0f, barTip);

            const bool chosen = m_tradeSelected >= 0 && m_tradeSelected < static_cast<int>(deals.size());
            const float payY = panelY + 40.0f;
            drawSlot(Inventory::TRADE_PAY_A, panelX + 36.0f, payY);
            drawSlot(Inventory::TRADE_PAY_B, panelX + 62.0f, payY);
            if (chosen)
            {
                const Trading::Offer& offer = deals[static_cast<size_t>(m_tradeSelected)].offer;
                const glm::vec4 ghost(1.0f, 1.0f, 1.0f, 0.3f);
                if (m_inventory.slot(Inventory::TRADE_PAY_A).empty())
                    drawItem(panelX + 36.0f, payY, SLOT, offer.want, offer.wantCount, ghost);
                if (offer.also != Blocks::Air && m_inventory.slot(Inventory::TRADE_PAY_B).empty())
                    drawItem(panelX + 62.0f, payY, SLOT, offer.also, offer.alsoCount, ghost);
            }

            drawCraftArrow(panelX + 88.0f, payY, 22.0f, 15.0f);
            const bool soldOut = chosen &&
                                 record.uses[static_cast<size_t>(m_tradeSelected)] >=
                                     deals[static_cast<size_t>(m_tradeSelected)].maxUses;
            if (soldOut)
            {
                redCross(panelX + 92.0f, payY - 1.0f, 15.0f);
                addHint(panelX + 88.0f, payY - 1.0f, 22.0f, 17.0f, "Sold out: it restocks soon");
            }

            const float resultX = panelX + 130.0f;
            guiWell(resultX - 5.0f, payY - 5.0f, 26.0f, 26.0f);
            const ItemStack& made = m_inventory.slot(Inventory::TRADE_RESULT);
            drawItem(resultX, payY, SLOT, made.id, made.count, glm::vec4(1.0f), made.damage, made.enchantments);
            m_slotBoxes.push_back({ SlotBox::Kind::TradeTake, Inventory::TRADE_RESULT,
                                    resultX - 1.0f, payY - 1.0f, PITCH });
            if (!chosen)
            {
                const std::string pick = "Pick a trade";
                m_ui.text(pick, panelX + PANEL_W * 0.5f - UIRenderer::textWidth(pick, 1.0f) * 0.5f,
                          panelY + 60.0f, 1.0f, TEXT_DIM);
            }

            const float listX = panelX + PANEL_W + 2.0f;
            const float listTop = panelY + 17.0f;
            const float rowH = 20.0f;
            const int visibleRows = 7;
            const std::string heading = "Trades";
            m_ui.text(heading, panelX + PANEL_W + 44.0f - UIRenderer::textWidth(heading, 1.0f) * 0.5f,
                      panelY + 6.0f, 1.0f, GUI_TEXT);

            std::vector<int> open;
            for (size_t i = 0; i < deals.size(); ++i)
                if (deals[i].rank <= rank) open.push_back(static_cast<int>(i));
            const int maxScroll = std::max(0, static_cast<int>(open.size()) - visibleRows);

            const float trackX = panelX + PANEL_W + 85.0f;
            const float trackH = rowH * visibleRows - 1.0f;
            if (m_draggingScroll && !m_input.isMouseButtonDown(SDL_BUTTON_LEFT)) m_draggingScroll = false;
            if (m_draggingScroll && maxScroll > 0)
            {
                const float along = std::clamp((mouseY - listTop - 7.5f) / (trackH - 15.0f), 0.0f, 1.0f);
                m_tradeScroll = static_cast<int>(std::lround(along * maxScroll));
            }
            m_tradeScroll = std::clamp(m_tradeScroll, 0, maxScroll);

            for (int r = 0; r < visibleRows; ++r)
            {
                const int at = r + m_tradeScroll;
                if (at >= static_cast<int>(open.size())) break;
                const int index = open[static_cast<size_t>(at)];
                const Trading::Deal& deal = deals[static_cast<size_t>(index)];
                const Trading::Offer& offer = deal.offer;
                const float ry = listTop + rowH * r;
                const float rw = 80.0f;
                const bool out = record.uses[static_cast<size_t>(index)] >= deal.maxUses;
                const bool selected = index == m_tradeSelected;
                const bool hovered = mouseX >= listX && mouseX < listX + rw &&
                                     mouseY >= ry && mouseY < ry + rowH - 1.0f;

                const glm::vec4 top = out ? glm::vec4(0.58f, 0.58f, 0.58f, 1.0f)
                                    : hovered ? glm::vec4(0.88f, 0.88f, 0.88f, 1.0f)
                                              : glm::vec4(0.78f, 0.78f, 0.78f, 1.0f);
                const glm::vec4 bottom = out ? glm::vec4(0.52f, 0.52f, 0.52f, 1.0f)
                                       : hovered ? glm::vec4(0.78f, 0.78f, 0.78f, 1.0f)
                                                 : glm::vec4(0.68f, 0.68f, 0.68f, 1.0f);
                softBox(listX, ry, rw, rowH - 1.0f, top, bottom,
                        selected ? glm::vec4(1.0f, 0.92f, 0.45f, 1.0f) : glm::vec4(0.40f, 0.40f, 0.40f, 1.0f));

                const glm::vec4 tint = out ? glm::vec4(1.0f, 1.0f, 1.0f, 0.5f) : glm::vec4(1.0f);
                drawItem(listX + 2.0f, ry + 1.5f, SLOT, offer.want, offer.wantCount, tint);
                if (offer.also != Blocks::Air)
                    drawItem(listX + 20.0f, ry + 1.5f, SLOT, offer.also, offer.alsoCount, tint);
                if (out) redCross(listX + 41.0f, ry + 3.0f, 12.0f);
                else drawCraftArrow(listX + 39.0f, ry + 4.0f, 15.0f, 11.0f);
                drawItem(listX + 60.0f, ry + 1.5f, SLOT, offer.give, offer.giveCount, tint, 0, offer.giveEnchantments);

                std::string tip = "Give " + std::to_string(offer.wantCount) + " " + displayName(offer.want);
                if (offer.also != Blocks::Air)
                    tip += " and " + std::to_string(offer.alsoCount) + " " + displayName(offer.also);
                tip += " for " + std::to_string(offer.giveCount) + " " + displayName(offer.give);
                if (offer.giveEnchantments != 0)
                    tip += " (enchanted)";
                const int left = deal.maxUses - record.uses[static_cast<size_t>(index)];
                tip += out ? " - sold out" : " - " + std::to_string(left) + " left";
                addHint(listX, ry, rw, rowH - 1.0f, tip);
                m_slotBoxes.push_back({ SlotBox::Kind::Trade, index, listX, ry, rowH - 1.0f, rw });
            }

            guiWell(trackX, listTop, 7.0f, trackH);
            if (maxScroll > 0)
            {
                const float along = static_cast<float>(m_tradeScroll) / static_cast<float>(maxScroll);
                const float thumbY = listTop + (trackH - 15.0f) * along;
                bevel(trackX + 1.0f, thumbY, 5.0f, 15.0f, GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 1.0f);
                m_slotBoxes.push_back({ SlotBox::Kind::Scroll, 0, trackX, listTop, trackH, 7.0f });
            }

            if (rank + 1 < Trading::RANKS)
            {
                int locked = 0;
                for (const Trading::Deal& deal : deals) if (deal.rank > rank) ++locked;
                if (locked > 0 && static_cast<int>(open.size()) < visibleRows)
                {
                    const std::string more = std::to_string(locked) + " more at " +
                                             Trading::rankName(rank + 1);
                    const float unit = static_cast<float>(guiScale());
                    const float small = std::max(0.5f, std::floor(unit * 0.75f) / unit);
                    m_ui.text(more, listX + 40.0f - UIRenderer::textWidth(more, small) * 0.5f,
                              listTop + rowH * static_cast<float>(open.size()) + 4.0f, small, TEXT_DIM);
                }
            }
        }
        else

        // The enchanting table. The item and the lapis that pays for it
        // sit side by side in the middle of the left-hand space, level
        // with the middle of the three offers beside them. Each offer
        // shows the level it needs in a green badge, what it puts on the
        // item, and the lapis it takes on the right.
        if (m_enchantOpen && skin)
        {
            const float slotY = panelY + 47.0f;
            drawSlot(Inventory::ENCHANT_SLOT, panelX + 15.0f, slotY);
            drawSlot(Inventory::ENCHANT_LAPIS, panelX + 35.0f, slotY);
            if (m_inventory.slot(Inventory::ENCHANT_LAPIS).empty())
                guiSprite("../item/empty_slot_lapis_lazuli", panelX + 35.0f, slotY, SLOT, SLOT);

            const ItemStack& subject = m_inventory.slot(Inventory::ENCHANT_SLOT);
            const ItemStack& lapis = m_inventory.slot(Inventory::ENCHANT_LAPIS);
            const int lapisHeld = lapis.id == Items::LapisLazuli ? lapis.count : 0;
            const int level = Experience::levelFor(m_player.experience);
            const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
            const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();

            static const char* LEVEL_ON[3] = { "sprites/container/enchanting_table/level_1",
                                               "sprites/container/enchanting_table/level_2",
                                               "sprites/container/enchanting_table/level_3" };
            static const char* LEVEL_OFF[3] = { "sprites/container/enchanting_table/level_1_disabled",
                                                "sprites/container/enchanting_table/level_2_disabled",
                                                "sprites/container/enchanting_table/level_3_disabled" };

            for (int row = 0; row < 3; ++row)
            {
                const float rx = panelX + 60.0f;
                const float ry = panelY + 14.0f + 19.0f * row;
                const Enchanting::Offer offer =
                    Enchanting::offerFor(row, m_enchantShelves, subject.id, m_enchantRoll);
                if (!offer.valid())
                {
                    guiSprite("sprites/container/enchanting_table/enchantment_slot_disabled", rx, ry, 108.0f, 19.0f);
                    continue;
                }

                const Enchanting::Price price = Enchanting::priceOf(row);
                const bool canBuy = Enchanting::affordable(offer, row, level, lapisHeld, m_player.creative());
                const bool hovered = mouseX >= rx && mouseY >= ry && mouseX < rx + 108.0f && mouseY < ry + 19.0f;
                const int dots = std::clamp(price.lapis, 1, 3) - 1;

                glm::vec4 nameColour(0.408f, 0.369f, 0.290f, 1.0f);
                if (!canBuy)
                {
                    guiSprite("sprites/container/enchanting_table/enchantment_slot_disabled", rx, ry, 108.0f, 19.0f);
                    guiSprite(LEVEL_OFF[dots], rx + 1.0f, ry + 1.0f, 16.0f, 16.0f);
                    nameColour = glm::vec4(0.204f, 0.184f, 0.145f, 1.0f);
                }
                else
                {
                    guiSprite(hovered ? "sprites/container/enchanting_table/enchantment_slot_highlighted"
                                      : "sprites/container/enchanting_table/enchantment_slot",
                              rx, ry, 108.0f, 19.0f);
                    guiSprite(LEVEL_ON[dots], rx + 1.0f, ry + 1.0f, 16.0f, 16.0f);
                    if (hovered) nameColour = glm::vec4(1.0f, 1.0f, 0.502f, 1.0f);
                }
                m_slotBoxes.push_back({ SlotBox::Kind::Enchant, row, rx, ry, 19.0f, 108.0f });

                std::string what = Enchanting::name(offer.kind);
                what += " ";
                what += std::to_string(offer.level);
                float scale = 1.0f;
                if (UIRenderer::textWidth(what, 1.0f) > 86.0f)
                {
                    const float unit = static_cast<float>(guiScale());
                    scale = std::max(0.5f, std::floor(unit * 86.0f / UIRenderer::textWidth(what, 1.0f)) / unit);
                }
                m_ui.text(what, rx + 20.0f, ry + 2.0f, scale, nameColour);

                const std::string need = std::to_string(offer.cost);
                m_ui.textWithShadow(need, rx + 20.0f + 86.0f - UIRenderer::textWidth(need, 1.0f), ry + 9.0f, 1.0f,
                                    canBuy ? glm::vec4(0.502f, 1.0f, 0.125f, 1.0f)
                                           : glm::vec4(0.251f, 0.498f, 0.063f, 1.0f));

                std::string tip = what + ": needs level " + std::to_string(offer.cost) +
                                  ", costs " + std::to_string(price.levels) +
                                  (price.levels == 1 ? " level and " : " levels and ") +
                                  std::to_string(price.lapis) + " lapis";
                addHint(rx, ry, 108.0f, 19.0f, tip);
            }
        }
        else if (m_enchantOpen)
        {
            const float slotY = panelY + 34.0f;
            drawSlot(Inventory::ENCHANT_SLOT, panelX + 15.0f, slotY);
            drawSlot(Inventory::ENCHANT_LAPIS, panelX + 35.0f, slotY);
            if (m_inventory.slot(Inventory::ENCHANT_LAPIS).empty())
                drawItem(panelX + 35.0f, slotY, SLOT, Items::LapisLazuli, 0,
                         glm::vec4(1.0f, 1.0f, 1.0f, 0.28f));
            addHint(panelX + 34.0f, slotY - 1.0f, PITCH, PITCH, "Lapis Lazuli goes here");

            const ItemStack& subject = m_inventory.slot(Inventory::ENCHANT_SLOT);
            const ItemStack& lapis = m_inventory.slot(Inventory::ENCHANT_LAPIS);
            const int lapisHeld = lapis.id == Items::LapisLazuli ? lapis.count : 0;
            const int level = Experience::levelFor(m_player.experience);

            const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
            const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();

            const glm::vec4 TEXT_BROWN(0.32f, 0.27f, 0.20f, 1.0f);
            const glm::vec4 TEXT_DEAD(0.42f, 0.39f, 0.35f, 1.0f);
            const glm::vec4 LEVEL_GREEN(0.62f, 0.95f, 0.42f, 1.0f);
            const glm::vec4 LEVEL_DARK(0.48f, 0.58f, 0.44f, 1.0f);

            auto softBox = [&](float x, float y, float bw, float bh, const glm::vec4& top,
                               const glm::vec4& bottom, const glm::vec4& edge) {
                m_ui.quad(x + 1.0f, y, bw - 2.0f, bh, edge);
                m_ui.quad(x, y + 1.0f, bw, bh - 2.0f, edge);
                m_ui.gradient(x + 1.0f, y + 1.0f, bw - 2.0f, bh - 2.0f, top, bottom);
            };

            if (subject.empty())
            {
                const std::string hint = "Put in an item";
                m_ui.text(hint, panelX + 60.0f + 54.0f - UIRenderer::textWidth(hint, 1.0f) * 0.5f,
                          panelY + 38.0f, 1.0f, TEXT_DIM);
            }

            for (int row = 0; row < 3; ++row)
            {
                const float rx = panelX + 60.0f;
                const float ry = panelY + 14.0f + 19.0f * row;
                const Enchanting::Offer offer =
                    Enchanting::offerFor(row, m_enchantShelves, subject.id, m_enchantRoll);

                if (!offer.valid())
                {
                    if (!subject.empty())
                        softBox(rx, ry, 108.0f, 18.0f, glm::vec4(0.60f, 0.60f, 0.60f, 1.0f),
                                glm::vec4(0.64f, 0.64f, 0.64f, 1.0f), glm::vec4(0.50f, 0.50f, 0.50f, 1.0f));
                    continue;
                }

                const Enchanting::Price price = Enchanting::priceOf(row);
                const bool canBuy = Enchanting::affordable(offer, row, level, lapisHeld,
                                                           m_player.creative());
                const bool hovered = mouseX >= rx && mouseY >= ry &&
                                     mouseX < rx + 108.0f && mouseY < ry + 18.0f;

                const glm::vec4 top = !canBuy ? glm::vec4(0.66f, 0.64f, 0.60f, 1.0f)
                                    : hovered ? glm::vec4(0.93f, 0.88f, 0.76f, 1.0f)
                                              : glm::vec4(0.86f, 0.80f, 0.69f, 1.0f);
                const glm::vec4 bottom = !canBuy ? glm::vec4(0.60f, 0.58f, 0.55f, 1.0f)
                                       : hovered ? glm::vec4(0.84f, 0.78f, 0.66f, 1.0f)
                                                 : glm::vec4(0.76f, 0.70f, 0.60f, 1.0f);
                softBox(rx, ry, 108.0f, 18.0f, top, bottom,
                        hovered && canBuy ? glm::vec4(0.95f, 0.90f, 0.70f, 1.0f)
                                          : glm::vec4(0.48f, 0.44f, 0.38f, 1.0f));
                m_slotBoxes.push_back({ SlotBox::Kind::Enchant, row, rx, ry, 18.0f, 108.0f });

                softBox(rx + 2.0f, ry + 2.0f, 16.0f, 14.0f,
                        canBuy ? glm::vec4(0.24f, 0.30f, 0.20f, 1.0f) : glm::vec4(0.36f, 0.36f, 0.34f, 1.0f),
                        canBuy ? glm::vec4(0.17f, 0.22f, 0.14f, 1.0f) : glm::vec4(0.32f, 0.32f, 0.30f, 1.0f),
                        canBuy ? glm::vec4(0.30f, 0.38f, 0.24f, 1.0f) : glm::vec4(0.40f, 0.40f, 0.38f, 1.0f));
                const std::string need = std::to_string(offer.cost);
                const float needScale = need.size() > 2 ? 0.75f : 1.0f;
                m_ui.text(need, rx + 10.0f - UIRenderer::textWidth(need, needScale) * 0.5f,
                          ry + 9.0f - UIRenderer::textHeight(needScale) * 0.5f, needScale,
                          canBuy ? LEVEL_GREEN : LEVEL_DARK);

                const float lapisX = rx + 108.0f - 18.0f;
                drawItem(lapisX, ry + 1.0f, SLOT, Items::LapisLazuli, price.lapis,
                         glm::vec4(1.0f, 1.0f, 1.0f, canBuy ? 1.0f : 0.5f));

                std::string what = Enchanting::name(offer.kind);
                what += " ";
                what += std::to_string(offer.level);
                const float room = lapisX - (rx + 21.0f) - 2.0f;
                float scale = 1.0f;
                if (UIRenderer::textWidth(what, 1.0f) > room)
                {
                    const float unit = static_cast<float>(guiScale());
                    scale = std::max(0.5f, std::floor(unit * room / UIRenderer::textWidth(what, 1.0f)) / unit);
                }
                m_ui.text(what, rx + 21.0f, ry + 9.0f - UIRenderer::textHeight(scale) * 0.5f,
                          scale, canBuy ? TEXT_BROWN : TEXT_DEAD);

                std::string tip = what + ": needs level " + std::to_string(offer.cost) +
                                  ", costs " + std::to_string(price.levels) +
                                  (price.levels == 1 ? " level and " : " levels and ") +
                                  std::to_string(price.lapis) + " lapis";
                addHint(rx, ry, 108.0f, 18.0f, tip);
            }

            const std::string shelves = std::to_string(m_enchantShelves) +
                                        (m_enchantShelves == 1 ? " shelf" : " shelves");
            m_ui.text(shelves, panelX + PANEL_W - 8.0f - UIRenderer::textWidth(shelves, 1.0f),
                      panelY + 6.0f, 1.0f, TEXT_DIM);
            addHint(panelX + PANEL_W - 8.0f - UIRenderer::textWidth(shelves, 1.0f), panelY + 5.0f,
                    UIRenderer::textWidth(shelves, 1.0f), 10.0f,
                    "Bookshelves around the table make stronger offers");
        }
        else

        // The brewing stand. The ingredient goes in at the top and the
        // arrow beside it runs down to the three bottles; the blaze
        // powder that fuels it has its gauge right underneath it.
        if (m_brewOpen && skin)
        {
            drawSlot(Inventory::BREW_INGREDIENT, panelX + 79.0f, panelY + 17.0f);
            drawSlot(Inventory::BREW_FUEL, panelX + 17.0f, panelY + 17.0f);
            const float bottleX[3] = { 56.0f, 79.0f, 102.0f };
            const float bottleY[3] = { 51.0f, 58.0f, 51.0f };
            for (int i = 0; i < Inventory::BREW_BOTTLES; ++i)
                drawSlot(Inventory::BREW_BOTTLE_FIRST + i, panelX + bottleX[i], panelY + bottleY[i]);

            const int fuelPixels = std::clamp((18 * m_brewFuel + Brewing::FUEL_PER_POWDER - 1) /
                                                  Brewing::FUEL_PER_POWDER, 0, 18);
            guiSpritePart("sprites/container/brewing_stand/fuel_length", 18.0f, 4.0f, 0.0f, 0.0f,
                          panelX + 60.0f, panelY + 44.0f, static_cast<float>(fuelPixels), 4.0f);
            addHint(panelX + 59.0f, panelY + 43.0f, 20.0f, 6.0f,
                    "Fuel: " + std::to_string(m_brewFuel) + " brews left");

            const float along = std::min(1.0f, m_brewProgress / Brewing::BREW_SECONDS);
            if (along > 0.0f)
            {
                const float filled = std::floor(28.0f * along);
                guiSpritePart("sprites/container/brewing_stand/brew_progress", 9.0f, 28.0f, 0.0f, 0.0f,
                              panelX + 97.0f, panelY + 16.0f, 9.0f, filled);
                static const int BUBBLE[7] = { 29, 24, 20, 16, 11, 6, 0 };
                const int ticksLeft = static_cast<int>((1.0f - along) * Brewing::BREW_SECONDS * 20.0f);
                const float shown = static_cast<float>(BUBBLE[(ticksLeft / 2) % 7]);
                guiSpritePart("sprites/container/brewing_stand/bubbles", 12.0f, 29.0f, 0.0f, 29.0f - shown,
                              panelX + 63.0f, panelY + 14.0f + 29.0f - shown, 12.0f, shown);
            }
        }
        else if (m_brewOpen)
        {
            drawSlot(Inventory::BREW_INGREDIENT, panelX + 79.0f, panelY + 17.0f);
            drawSlot(Inventory::BREW_FUEL, panelX + 17.0f, panelY + 17.0f);
            if (m_inventory.slot(Inventory::BREW_FUEL).empty())
                drawItem(panelX + 17.0f, panelY + 17.0f, SLOT, Items::BlazePowder, 0,
                         glm::vec4(1.0f, 1.0f, 1.0f, 0.28f));
            addHint(panelX + 16.0f, panelY + 16.0f, PITCH, PITCH, "Blaze Powder fuels the stand");

            const float bottleX[3] = { 56.0f, 79.0f, 102.0f };
            const float bottleY[3] = { 51.0f, 58.0f, 51.0f };
            for (int i = 0; i < Inventory::BREW_BOTTLES; ++i)
            {
                const int slot = Inventory::BREW_BOTTLE_FIRST + i;
                drawSlot(slot, panelX + bottleX[i], panelY + bottleY[i]);
                if (m_inventory.slot(slot).empty())
                    drawItem(panelX + bottleX[i], panelY + bottleY[i], SLOT, Items::GlassBottle, 0,
                             glm::vec4(1.0f, 1.0f, 1.0f, 0.28f));
            }

            const int fuelPixels = std::clamp((18 * m_brewFuel + Brewing::FUEL_PER_POWDER - 1) /
                                                  Brewing::FUEL_PER_POWDER, 0, 18);
            guiWell(panelX + 16.0f, panelY + 37.0f, 20.0f, 6.0f);
            if (fuelPixels > 0)
                m_ui.gradient(panelX + 17.0f, panelY + 38.0f, static_cast<float>(fuelPixels), 4.0f,
                              glm::vec4(1.0f, 0.80f, 0.25f, 1.0f), glm::vec4(0.85f, 0.45f, 0.10f, 1.0f));
            addHint(panelX + 16.0f, panelY + 37.0f, 20.0f, 6.0f,
                    "Fuel: " + std::to_string(m_brewFuel) + " brews left");

            const float along = std::min(1.0f, m_brewProgress / Brewing::BREW_SECONDS);
            static const int ARROW_WIDTH[28] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                                                 3, 3, 3, 3, 3, 3, 9, 9, 7, 7, 5, 5, 3, 1 };
            const int filledRows = static_cast<int>(std::floor(28.0f * along));
            for (int row = 0; row < 28; ++row)
            {
                const float rw = static_cast<float>(ARROW_WIDTH[row]);
                m_ui.quad(panelX + 97.0f + (9.0f - rw) * 0.5f, panelY + 16.0f + row, rw, 1.0f,
                          row < filledRows ? glm::vec4(1.0f) : GUI_WELL);
            }

            if (along > 0.0f)
            {
                static const int BUBBLE[7] = { 29, 24, 20, 16, 11, 6, 0 };
                const int ticksLeft = static_cast<int>((1.0f - along) * Brewing::BREW_SECONDS * 20.0f);
                const float shown = static_cast<float>(BUBBLE[(ticksLeft / 2) % 7]);
                for (float b = 0.0f; b < shown; b += 5.0f)
                {
                    const float by = panelY + 14.0f + 29.0f - b - 3.0f;
                    const float bx = panelX + 63.0f + (static_cast<int>(b / 5.0f) % 2 ? 6.0f : 2.0f);
                    m_ui.quad(bx, by, 3.0f, 3.0f, glm::vec4(0.75f, 0.90f, 1.0f, 0.85f));
                }
            }
        }
        else

        // The anvil: what is being worked on, plus what is put to it,
        // gives what comes out, with the name of the result over the top
        // and the price underneath.
        if (m_anvilOpen && skin)
        {
            const int slots[3] = { Inventory::ANVIL_LEFT, Inventory::ANVIL_RIGHT, Inventory::ANVIL_RESULT };
            const float xs[3] = { 27.0f, 76.0f, 134.0f };
            const float ay = panelY + 47.0f;
            const ItemStack& working = m_inventory.slot(Inventory::ANVIL_LEFT);
            const ItemStack& adding = m_inventory.slot(Inventory::ANVIL_RIGHT);
            const ItemStack& made = m_inventory.slot(Inventory::ANVIL_RESULT);

            guiSprite(working.empty() ? "sprites/container/anvil/text_field_disabled"
                                      : "sprites/container/anvil/text_field",
                      panelX + 59.0f, panelY + 20.0f, 110.0f, 16.0f);
            if (!working.empty())
            {
                const std::string name = stackName(made.empty() ? working : made);
                m_ui.textWithShadow(name, panelX + 62.0f, panelY + 24.0f, 1.0f,
                                    glm::vec4(0.878f, 0.878f, 0.878f, 1.0f));
            }

            for (int i = 0; i < 3; ++i)
            {
                const float x = panelX + xs[i];
                const ItemStack& stack = m_inventory.slot(slots[i]);
                drawItem(x, ay, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage, stack.enchantments);
                slotBox(i == 2 ? SlotBox::Kind::AnvilTake : SlotBox::Kind::Slot, slots[i], x, ay);
            }

            if ((!working.empty() || !adding.empty()) && made.empty())
                guiSprite("sprites/container/anvil/error", panelX + 99.0f, panelY + 45.0f, 28.0f, 21.0f);

            const Anvil::Result result = anvilResult();
            const int level = Experience::levelFor(m_player.experience);
            std::string price;
            glm::vec4 colour(0.502f, 1.0f, 0.125f, 1.0f);
            if (result.tooExpensive && !m_player.creative())
            {
                price = "Too Expensive!";
                colour = glm::vec4(1.0f, 0.376f, 0.376f, 1.0f);
            }
            else if (result.valid)
            {
                price = "Enchantment Cost: " + std::to_string(result.levelCost);
                if (!m_player.creative() && level < result.levelCost)
                    colour = glm::vec4(1.0f, 0.376f, 0.376f, 1.0f);
            }
            if (!price.empty())
            {
                const float right = panelX + PANEL_W - 8.0f;
                const float px = right - UIRenderer::textWidth(price, 1.0f) - 2.0f;
                m_ui.quad(px - 2.0f, panelY + 67.0f, right - (px - 2.0f), 12.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.31f));
                m_ui.textWithShadow(price, px, panelY + 69.0f, 1.0f, colour);
            }
        }
        else if (m_anvilOpen)
        {
            const int slots[3] = { Inventory::ANVIL_LEFT, Inventory::ANVIL_RIGHT,
                                   Inventory::ANVIL_RESULT };
            const float xs[3] = { 27.0f, 76.0f, 134.0f };
            const float ay = panelY + 36.0f;

            const ItemStack& working = m_inventory.slot(Inventory::ANVIL_LEFT);
            const ItemStack& made = m_inventory.slot(Inventory::ANVIL_RESULT);
            const std::string heading = !made.empty() ? stackName(made)
                                      : !working.empty() ? stackName(working)
                                      : std::string("Repair, combine or enchant");
            m_ui.text(heading, panelX + PANEL_W * 0.5f - UIRenderer::textWidth(heading, 1.0f) * 0.5f,
                      panelY + 22.0f, 1.0f, working.empty() ? TEXT_DIM : GUI_TEXT);

            for (int i = 0; i < 3; ++i)
            {
                const float x = panelX + xs[i];
                if (i == 2) guiWell(x - 5.0f, ay - 5.0f, 26.0f, 26.0f);
                else well(x, ay);
                const ItemStack& stack = m_inventory.slot(slots[i]);
                drawItem(x, ay, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage, stack.enchantments);
                slotBox(i == 2 ? SlotBox::Kind::AnvilTake : SlotBox::Kind::Slot, slots[i], x, ay);
            }

            m_ui.text("+", panelX + 58.0f, ay + 4.0f, 1.0f, GUI_TEXT);
            drawCraftArrow(panelX + 102.0f, ay, 22.0f, 15.0f);

            const Anvil::Result result = anvilResult();
            const int level = Experience::levelFor(m_player.experience);

            std::string price;
            glm::vec4 colour(0.502f, 1.0f, 0.125f, 1.0f);
            if (result.tooExpensive && !m_player.creative())
            {
                price = "Too Expensive!";
                colour = glm::vec4(1.0f, 0.376f, 0.376f, 1.0f);
            }
            else if (result.valid)
            {
                price = "Cost: " + std::to_string(result.levelCost) +
                        (result.levelCost == 1 ? " level" : " levels");
                if (!m_player.creative() && level < result.levelCost)
                    colour = glm::vec4(1.0f, 0.376f, 0.376f, 1.0f);
            }

            if (!price.empty())
            {
                const float px = panelX + PANEL_W * 0.5f - UIRenderer::textWidth(price, 1.0f) * 0.5f;
                m_ui.quad(px - 3.0f, panelY + 58.0f, UIRenderer::textWidth(price, 1.0f) + 6.0f, 12.0f,
                          glm::vec4(0.0f, 0.0f, 0.0f, 0.31f));
                m_ui.textWithShadow(price, px, panelY + 60.0f, 1.0f, colour);
            }
        }
        else

        // A furnace: the ore at 56 and 17, the fuel under it at 53, the
        // fire between them at 36 and the result off to the right at
        // 116 and 35. The client's own four, and the shape everybody
        // recognises.
        if (m_horseOpen || m_beaconOpen || m_grindOpen) {}
        else if (m_furnaceOpen)
        {
            const Furnaces::State& state = m_furnaces.at(m_furnaceBlock);

            Furnace gauge;
            gauge.burnLeft = state.burnLeft;
            gauge.burnTotal = state.burnTotal;
            gauge.cooked = state.cooked;

            drawSlot(Inventory::FURNACE_INPUT, panelX + 56.0f, panelY + 17.0f);
            drawSlot(Inventory::FURNACE_FUEL, panelX + 56.0f, panelY + 53.0f);

            if (skin)
            {
                if (gauge.burnLeft > 0.0f)
                {
                    const float lit = std::ceil(gauge.burnFraction() * 13.0f) + 1.0f;
                    guiSpritePart(litSprite, 14.0f, 14.0f, 0.0f, 14.0f - lit,
                                  panelX + 56.0f, panelY + 36.0f + 14.0f - lit, 14.0f, lit);
                }
                guiSpritePart(burnSprite, 24.0f, 16.0f, 0.0f, 0.0f,
                              panelX + 79.0f, panelY + 34.0f, std::ceil(gauge.cookFraction() * 24.0f), 16.0f);
            }
            else
            {
                drawFurnaceFlame(panelX + 57.0f, panelY + 36.0f, gauge.burnFraction());
                drawCraftArrow(panelX + 79.0f, panelY + 34.0f, 24.0f, 16.0f, gauge.cookFraction());
            }

            drawOutputSlot(Inventory::FURNACE_OUTPUT, panelX + 116.0f, panelY + 35.0f);
        }
        else
        {
            // Crafting. A workbench puts its three by three at 30 and
            // 17 with the result at 124 and 35; the player's own two by
            // two goes at 98 and 18 with the result at 154 and 28.
            const int size = m_inventory.craftSize();

            const float gridX = panelX + (bench ? 30.0f : 98.0f);
            const float gridY = panelY + (bench ? 17.0f : 18.0f);

            for (int row = 0; row < size; ++row)
                for (int column = 0; column < size; ++column)
                    // The grid is addressed as 3x3 even when only 2x2 shows.
                    drawSlot(Inventory::CRAFT_FIRST + row * 3 + column,
                             gridX + column * PITCH, gridY + row * PITCH);

            const float resultX = panelX + (bench ? 124.0f : 154.0f);
            const float resultY = panelY + (bench ? 35.0f : 28.0f);

            // The arrow between the grid and the result: 22 by 15 at 90
            // and 35 on a workbench, and squeezed into the narrower gap
            // the little grid leaves.
            if (skin) {}
            else if (bench) drawCraftArrow(panelX + 90.0f, panelY + 35.0f, 22.0f, 15.0f);
            else drawCraftArrow(panelX + 135.0f, panelY + 29.0f, 16.0f, 13.0f);
            if (bench) drawOutputSlot(Inventory::CRAFT_RESULT, resultX, resultY);
            else drawSlot(Inventory::CRAFT_RESULT, resultX, resultY);

            // "Crafting" sits over the little grid, at 97 and 6, which
            // is the only heading the player's own screen has.
            if (!bench)
                m_ui.text("Crafting", panelX + 97.0f, panelY + 6.0f, 1.0f, GUI_TEXT);
        }
    }

    // ------------------------------------------------------- main grid ---
    // Storage runs 9..35, directly above the hotbar.
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < PALETTE_COLUMNS; ++column)
            drawSlot(Inventory::HOTBAR_SLOTS + row * PALETTE_COLUMNS + column,
                     invX + column * PITCH, storageY + row * PITCH);

    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
        drawSlot(i, invX + i * PITCH, hotbarY);

    if (plainInventory) drawInventoryEffects(panelX + panelWidth, panelY, w);

}

void Application::renderCreativeScreen(float w, float h)
{
    // The client's creative screen, at its own numbers: a panel 195 by
    // 136, the palette nine by five at 9 and 18, the hotbar at 9 and
    // 112, the scrollbar's track at 175 and 18, and the tabs 28 by 32,
    // a row hanging behind the top edge and a row behind the bottom.
    const float PANEL_W = 195.0f;
    const float PANEL_H = 136.0f;

    const float panelX = std::floor((w - PANEL_W) * 0.5f);
    const float panelY = std::floor((h - PANEL_H) * 0.5f);

    const bool inventoryTab = m_creativeTab == INVENTORY_TAB;
    const bool searchTab = m_creativeTab == SEARCH_TAB;
    const char* background = inventoryTab ? "container/creative_inventory/tab_inventory"
                           : searchTab ? "container/creative_inventory/tab_item_search"
                                       : "container/creative_inventory/tab_items";
    const bool skin = m_gui.has(background);

    auto well = [this, skin](float x, float y) {
        if (!skin) guiWell(x - 1.0f, y - 1.0f, PITCH, PITCH);
    };
    auto drawSlot = [&](int index, float x, float y) {
        well(x, y);
        const ItemStack& stack = m_inventory.slot(index);
        drawItem(x, y, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage, stack.enchantments);
        m_slotBoxes.push_back({ SlotBox::Kind::Slot, index, x - 1.0f, y - 1.0f, PITCH });
    };

    // ------------------------------------------------------------ tabs ---
    // Where each tab goes. The first five run along the top the way the
    // client's do, and Everything takes the far corner of the top row
    // where its search tab sits; tools and armour go along the bottom,
    // and the player's own things take the far corner of that, which is
    // where the client keeps its survival-inventory tab.
    struct TabPlace { bool top; int column; };
    const TabPlace PLACES[TAB_COUNT] = {
        { true, 0 }, { true, 1 }, { true, 2 }, { true, 3 }, { true, 4 },
        { false, 0 }, { false, 1 }, { true, 6 }, { false, 6 },
    };

    const glm::vec4 TAB_BACK(0.62f, 0.62f, 0.62f, 1.0f);
    const glm::vec4 TAB_BACK_LIGHT(0.80f, 0.80f, 0.80f, 1.0f);
    const glm::vec4 OUTLINE(0.0f, 0.0f, 0.0f, 0.85f);

    auto drawTab = [&](int i) {
        const TabPlace place = PLACES[i];
        const bool open = i == m_creativeTab;

        // 1.21's: tabs 26 wide stepping 27 along, and the last two
        // columns counted back from the far edge.
        const float tx = panelX + (place.column >= 5 ? PANEL_W - 27.0f * (7 - place.column) + 1.0f
                                                     : place.column * 27.0f);
        const float ty = place.top ? panelY - 28.0f : panelY + PANEL_H - 4.0f;

        if (skin)
        {
            static const char* TOP_OPEN[7] = {
                "sprites/container/creative_inventory/tab_top_selected_1", "sprites/container/creative_inventory/tab_top_selected_2",
                "sprites/container/creative_inventory/tab_top_selected_3", "sprites/container/creative_inventory/tab_top_selected_4",
                "sprites/container/creative_inventory/tab_top_selected_5", "sprites/container/creative_inventory/tab_top_selected_6",
                "sprites/container/creative_inventory/tab_top_selected_7" };
            static const char* TOP_SHUT[7] = {
                "sprites/container/creative_inventory/tab_top_unselected_1", "sprites/container/creative_inventory/tab_top_unselected_2",
                "sprites/container/creative_inventory/tab_top_unselected_3", "sprites/container/creative_inventory/tab_top_unselected_4",
                "sprites/container/creative_inventory/tab_top_unselected_5", "sprites/container/creative_inventory/tab_top_unselected_6",
                "sprites/container/creative_inventory/tab_top_unselected_7" };
            static const char* BOTTOM_OPEN[7] = {
                "sprites/container/creative_inventory/tab_bottom_selected_1", "sprites/container/creative_inventory/tab_bottom_selected_2",
                "sprites/container/creative_inventory/tab_bottom_selected_3", "sprites/container/creative_inventory/tab_bottom_selected_4",
                "sprites/container/creative_inventory/tab_bottom_selected_5", "sprites/container/creative_inventory/tab_bottom_selected_6",
                "sprites/container/creative_inventory/tab_bottom_selected_7" };
            static const char* BOTTOM_SHUT[7] = {
                "sprites/container/creative_inventory/tab_bottom_unselected_1", "sprites/container/creative_inventory/tab_bottom_unselected_2",
                "sprites/container/creative_inventory/tab_bottom_unselected_3", "sprites/container/creative_inventory/tab_bottom_unselected_4",
                "sprites/container/creative_inventory/tab_bottom_unselected_5", "sprites/container/creative_inventory/tab_bottom_unselected_6",
                "sprites/container/creative_inventory/tab_bottom_unselected_7" };
            const int column = std::clamp(place.column, 0, 6);
            const char* sprite = place.top ? (open ? TOP_OPEN[column] : TOP_SHUT[column])
                                           : (open ? BOTTOM_OPEN[column] : BOTTOM_SHUT[column]);
            guiSprite(sprite, tx, ty, 26.0f, 32.0f);
            drawItem(tx + 5.0f, ty + (place.top ? 9.0f : 7.0f), SLOT, CREATIVE_TABS[i].icon, 0, glm::vec4(1.0f));
            m_slotBoxes.push_back({ SlotBox::Kind::Tab, i, tx, ty, 32.0f, 26.0f });
            return;
        }

        // An outline on the three sides that stand clear of the panel.
        m_ui.quad(tx - 1.0f, place.top ? ty - 1.0f : ty, 28.0f, 33.0f, OUTLINE);

        if (open)
        {
            bevel(tx, ty, 26.0f, 32.0f, GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 2.0f);

            // The open tab runs on into the panel with no seam: the edge
            // it shares with the panel is painted over in the panel's own
            // colour, border and all.
            if (place.top)
                m_ui.quad(tx + 2.0f, panelY - 1.0f, 22.0f, 6.0f, GUI_PANEL);
            else
                m_ui.quad(tx + 2.0f, panelY + PANEL_H - 5.0f, 22.0f, 6.0f, GUI_PANEL);
        }
        else
        {
            bevel(tx, ty, 26.0f, 32.0f, TAB_BACK, TAB_BACK_LIGHT, GUI_PANEL_DARK, 2.0f);
        }

        // The icon sits five in and nine down on the top row, seven down
        // on the bottom, so it is centred on the part of the tab you see.
        drawItem(tx + 5.0f, ty + (place.top ? 9.0f : 7.0f), SLOT,
                 CREATIVE_TABS[i].icon, 0, glm::vec4(1.0f));

        m_slotBoxes.push_back({ SlotBox::Kind::Tab, i, tx, ty, 32.0f, 26.0f });
    };

    // Every tab but the open one goes behind the panel.
    for (int i = 0; i < TAB_COUNT; ++i)
        if (i != m_creativeTab) drawTab(i);

    if (skin) guiBlit(background, panelX, panelY, 0.0f, 0.0f, PANEL_W, PANEL_H);
    else guiPanel(panelX, panelY, PANEL_W, PANEL_H);

    if (!inventoryTab)
    {
        // ------------------------------------------------- the palette ---
        // The tab's name in the corner, where the client writes it.
        if (!searchTab)
            m_ui.text(CREATIVE_TABS[m_creativeTab].name, panelX + 8.0f, panelY + 6.0f,
                      1.0f, GUI_TEXT);

        const StackId* items = nullptr;
        int count = 0;
        tabContents(m_creativeTab, items, count);

        static std::vector<StackId> found;
        if (searchTab)
        {
            if (!skin)
            {
                m_ui.text("Search Items", panelX + 8.0f, panelY + 6.0f, 1.0f, GUI_TEXT);
                m_ui.quad(panelX + 81.0f, panelY + 4.0f, 90.0f, 12.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            }
            const bool caretOn = ((SDL_GetTicks() / 500u) % 2u) == 0u;
            if (m_creativeSearch.empty() && guiSprite("sprites/icon/search", panelX + 81.0f, panelY + 4.0f, 12.0f, 12.0f))
            {
                m_ui.text("Search...", panelX + 95.0f, panelY + 6.0f, 1.0f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
                if (caretOn) m_ui.textWithShadow("_", panelX + 95.0f, panelY + 6.0f, 1.0f, glm::vec4(1.0f));
            }
            else
                m_ui.textWithShadow(m_creativeSearch + (caretOn ? "_" : ""), panelX + 83.0f, panelY + 6.0f, 1.0f,
                                    glm::vec4(1.0f));
            if (!m_creativeSearch.empty())
            {
                std::string want = m_creativeSearch;
                for (char& c : want) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                found.clear();
                for (int i = 0; i < count; ++i)
                {
                    std::string name = displayName(items[i]);
                    for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    if (name.find(want) != std::string::npos) found.push_back(items[i]);
                }
                items = found.data();
                count = static_cast<int>(found.size());
            }
        }

        // A tab with more in it than the grid holds scrolls on the wheel.
        const int rows = (count + PALETTE_COLUMNS - 1) / PALETTE_COLUMNS;
        const int maxRow = std::max(0, rows - PALETTE_ROWS);
        m_paletteRow = std::clamp(m_paletteRow, 0, maxRow);

        for (int row = 0; row < PALETTE_ROWS; ++row)
            for (int column = 0; column < PALETTE_COLUMNS; ++column)
            {
                const float x = panelX + 9.0f + column * PITCH;
                const float y = panelY + 18.0f + row * PITCH;
                well(x, y);

                const int cell = (row + m_paletteRow) * PALETTE_COLUMNS + column;
                if (cell >= count) continue;
                drawItem(x, y, SLOT, items[cell], 0, glm::vec4(1.0f));
                m_slotBoxes.push_back({ SlotBox::Kind::Palette, items[cell],
                                        x - 1.0f, y - 1.0f, PITCH });
            }

        // The scrollbar: a sunken track at 175 and 18, a hundred and
        // twelve tall, with the thumb twelve by fifteen running down it.
        // Greyed out when there is nothing to scroll to, as the client's
        // is.
        const float trackX = panelX + 174.0f;
        const float trackY = panelY + 17.0f;
        if (!skin) guiWell(trackX, trackY, 14.0f, 112.0f);

        // Dragged, the thumb follows the cursor down the track and the
        // palette follows the thumb, as the client's does; the wheel
        // still works as well. The press that starts it is the track's
        // own slot box below.
        if (m_draggingScroll && !m_input.isMouseButtonDown(SDL_BUTTON_LEFT))
            m_draggingScroll = false;
        if (m_draggingScroll && maxRow > 0)
        {
            const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
            const float fraction = std::clamp((mouseY - (panelY + 18.0f) - 7.5f) / (112.0f - 17.0f),
                                              0.0f, 1.0f);
            m_paletteRow = static_cast<int>(std::lround(fraction * maxRow));
        }
        if (maxRow > 0)
            m_slotBoxes.push_back({ SlotBox::Kind::Scroll, 0, trackX, trackY, 112.0f, 14.0f });

        const float along = maxRow > 0 ? static_cast<float>(m_paletteRow) / maxRow : 0.0f;
        const float thumbY = panelY + 18.0f + (112.0f - 17.0f) * along;
        if (skin)
            guiSprite(maxRow > 0 ? "sprites/container/creative_inventory/scroller"
                                 : "sprites/container/creative_inventory/scroller_disabled",
                      panelX + 175.0f, std::floor(thumbY), 12.0f, 15.0f);
        else if (maxRow > 0)
            bevel(trackX + 1.0f, thumbY, 12.0f, 15.0f,
                  GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 1.0f);
        else
            bevel(trackX + 1.0f, thumbY, 12.0f, 15.0f,
                  TAB_BACK, TAB_BACK_LIGHT, GUI_PANEL_DARK, 1.0f);
    }
    else
    {
        // ------------------------------------ the player's own things ---
        // 1.21's own layout for this tab, which is not the survival
        // screen's: the armour in two pairs at 54 and 108, six and
        // thirty-three down, the character standing between them, the
        // storage at 54, and no crafting grid at all.
        const float ARMOUR_X[4] = { 54.0f, 54.0f, 108.0f, 108.0f };
        const float ARMOUR_Y[4] = { 6.0f, 33.0f, 6.0f, 33.0f };

        for (int i = 0; i < Inventory::ARMOR_SLOTS; ++i)
        {
            const float x = panelX + ARMOUR_X[i];
            const float y = panelY + ARMOUR_Y[i];
            const int index = Inventory::ARMOR_FIRST + i;

            well(x, y);
            const ItemStack& stack = m_inventory.slot(index);
            if (stack.empty())
                m_ui.texturedQuad(m_gui.texture(ARMOR_GHOSTS[i]), x, y, SLOT, SLOT,
                                  0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
            else
                drawItem(x, y, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage, stack.enchantments);
            m_slotBoxes.push_back({ SlotBox::Kind::Slot, index, x - 1.0f, y - 1.0f, PITCH });
        }

        {
            const float offX = panelX + 35.0f;
            const float offY = panelY + 20.0f;
            well(offX, offY);
            const ItemStack& offhand = m_inventory.slot(Inventory::OFFHAND);
            if (offhand.empty())
                if (!guiSprite("sprites/container/slot/shield", offX, offY, SLOT, SLOT))
                    guiSprite("../item/empty_armor_slot_shield", offX, offY, SLOT, SLOT);
            else
                drawItem(offX, offY, SLOT, offhand.id, offhand.count, glm::vec4(1.0f), offhand.damage, offhand.enchantments);
            m_slotBoxes.push_back({ SlotBox::Kind::Slot, Inventory::OFFHAND, offX - 1.0f, offY - 1.0f, PITCH });
        }

        // The character in the box from 73 and 6 to 105 and 49, twenty
        // to a block -- two thirds the size it is on the survival screen.
        if (!skin) guiWell(panelX + 72.0f, panelY + 5.0f, 34.0f, 45.0f);
        drawPlayerFigure(activeSkin(), panelX + 73.0f, panelY + 6.0f, panelX + 105.0f,
                         panelY + 49.0f, 20.0f);

        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < PALETTE_COLUMNS; ++column)
                drawSlot(Inventory::HOTBAR_SLOTS + row * PALETTE_COLUMNS + column,
                         panelX + 9.0f + column * PITCH, panelY + 54.0f + row * PITCH);

        // The bin, at 173 and 112: whatever is dropped on it is gone.
        const float binX = panelX + 173.0f;
        const float binY = panelY + 112.0f;
        well(binX, binY);
        if (!skin) m_ui.text("x", binX + 6.0f, binY + 4.0f, 1.0f, glm::vec4(0.55f, 0.20f, 0.20f, 1.0f));
        m_slotBoxes.push_back({ SlotBox::Kind::Destroy, 0, binX - 1.0f, binY - 1.0f, PITCH });
    }

    // The hotbar, at 9 and 112 whichever tab is open.
    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
        drawSlot(i, panelX + 9.0f + i * PITCH, panelY + 112.0f);

    // And the open tab last, in front of the panel it belongs to.
    drawTab(m_creativeTab);
    drawInventoryEffects(panelX + PANEL_W, panelY, w);
}

void Application::drawInventoryEffects(float panelRight, float panelTop, float w)
{
    static const char* ICONS[Effects::KIND_COUNT] = {
        "../mob_effect/speed", "../mob_effect/slowness", "../mob_effect/strength",
        "../mob_effect/weakness", "../mob_effect/regeneration", "../mob_effect/poison",
        "../mob_effect/fire_resistance", "../mob_effect/water_breathing",
        "../mob_effect/night_vision", "../mob_effect/jump_boost", "../mob_effect/bad_omen",
        "../mob_effect/haste", "../mob_effect/resistance",
    };
    static const char* NUMERALS[6] = { "", " II", " III", " IV", " V", " VI" };

    std::vector<int> running;
    for (int i = 0; i < Effects::KIND_COUNT; ++i)
        if (m_player.effects.has(static_cast<Effects::Kind>(i))) running.push_back(i);
    if (running.empty()) return;

    const float x = panelRight + 2.0f;
    const float room = w - x;
    if (room < 32.0f) return;
    const bool wide = room >= 120.0f;
    const float step = running.size() > 5 ? 132.0f / static_cast<float>(running.size() - 1) : 33.0f;

    float y = panelTop;
    for (int i : running)
    {
        const Effects::Kind kind = static_cast<Effects::Kind>(i);
        const bool drew = wide ? guiSprite("sprites/container/inventory/effect_background_large", x, y, 120.0f, 32.0f)
                               : guiSprite("sprites/container/inventory/effect_background_small", x, y, 32.0f, 32.0f);
        if (!drew) guiPanel(x, y, wide ? 120.0f : 32.0f, 32.0f);
        guiSprite(ICONS[i], x + (wide ? 6.0f : 7.0f), y + 7.0f, 18.0f, 18.0f);
        if (wide)
        {
            const int amplifier = std::clamp(m_player.effects.amplifier(kind), 0, 5);
            m_ui.textWithShadow(std::string(Effects::name(kind)) + NUMERALS[amplifier], x + 28.0f, y + 6.0f,
                                1.0f, glm::vec4(1.0f));
            const int seconds = static_cast<int>(std::ceil(m_player.effects.secondsLeft(kind)));
            char clock[16];
            std::snprintf(clock, sizeof(clock), "%d:%02d", seconds / 60, seconds % 60);
            m_ui.textWithShadow(clock, x + 28.0f, y + 16.0f, 1.0f, glm::vec4(0.498f, 0.498f, 0.498f, 1.0f));
        }
        y += step;
    }
}

void Application::finishInventoryScreen()
{
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();

    // The slot under the cursor washes white, empty or not, which is
    // the client's: 80FFFFFF over the sixteen pixels the item sits in.
    std::string tip;
    if (const SlotBox* box = slotBoxAt(m_input.mouseX(), m_input.mouseY()))
    {
        const glm::vec4 wash(1.0f, 1.0f, 1.0f, 0.5f);
        auto highlight = [&]() { m_ui.quad(box->x + 1.0f, box->y + 1.0f, SLOT, SLOT, wash); };

        switch (box->kind)
        {
            case SlotBox::Kind::Slot:
            case SlotBox::Kind::AnvilTake:
            {
                highlight();
                const ItemStack& stack = m_inventory.slot(box->index);
                if (!stack.empty()) tip = stackName(stack);
                break;
            }
            case SlotBox::Kind::Palette:
                highlight();
                tip = displayName(static_cast<StackId>(box->index));
                break;
            case SlotBox::Kind::Scroll:
            m_draggingScroll = true;
            break;

        case SlotBox::Kind::Destroy:
                highlight();
                tip = "Destroy Item";
                break;
            case SlotBox::Kind::Tab:
                tip = CREATIVE_TABS[box->index].name;
                break;
            default:
                break;
        }
    }

    if (tip.empty())
        for (const HintBox& hint : m_hintBoxes)
            if (mouseX >= hint.x && mouseX < hint.x + hint.w && mouseY >= hint.y &&
                mouseY < hint.y + hint.h)
            {
                tip = hint.text;
                break;
            }

    // Whatever is on the cursor, drawn last, centred on it.
    const ItemStack& held = m_inventory.cursor();
    if (!held.empty())
        drawItem(mouseX - SLOT * 0.5f, mouseY - SLOT * 0.5f, SLOT,
                 held.id, held.count, glm::vec4(1.0f), held.damage, held.enchantments);

    // The client names what is under the cursor only while the cursor
    // is empty: with a stack in hand the tooltip would sit on top of it.
    if (!tip.empty() && held.empty()) drawTooltip(tip, mouseX, mouseY);
}

void Application::drawTooltip(const std::string& text, float mouseX, float mouseY)
{
    const float textWidth = UIRenderer::textWidth(text, 1.0f);
    const float textHeight = 8.0f;

    float x = mouseX + 12.0f;
    float y = mouseY - 12.0f;
    if (x + textWidth > m_ui.canvasWidth()) x -= 28.0f + textWidth;
    if (y + textHeight + 6.0f > m_ui.canvasHeight()) y = m_ui.canvasHeight() - textHeight - 6.0f;

    // F0100010 for the body, with its corners nicked off; and a border
    // that fades from 505000FF at the top to 5028007F at the bottom.
    const glm::vec4 body(0.063f, 0.0f, 0.063f, 0.94f);
    const glm::vec4 rimTop(0.314f, 0.0f, 1.0f, 0.314f);
    const glm::vec4 rimBottom(0.157f, 0.0f, 0.498f, 0.314f);

    m_ui.quad(x - 3.0f, y - 4.0f, textWidth + 6.0f, 1.0f, body);
    m_ui.quad(x - 3.0f, y + textHeight + 3.0f, textWidth + 6.0f, 1.0f, body);
    m_ui.quad(x - 3.0f, y - 3.0f, textWidth + 6.0f, textHeight + 6.0f, body);
    m_ui.quad(x - 4.0f, y - 3.0f, 1.0f, textHeight + 6.0f, body);
    m_ui.quad(x + textWidth + 3.0f, y - 3.0f, 1.0f, textHeight + 6.0f, body);

    m_ui.gradient(x - 3.0f, y - 2.0f, 1.0f, textHeight + 4.0f, rimTop, rimBottom);
    m_ui.gradient(x + textWidth + 2.0f, y - 2.0f, 1.0f, textHeight + 4.0f, rimTop, rimBottom);
    m_ui.quad(x - 3.0f, y - 3.0f, textWidth + 6.0f, 1.0f, rimTop);
    m_ui.quad(x - 3.0f, y + textHeight + 2.0f, textWidth + 6.0f, 1.0f, rimBottom);

    m_ui.textWithShadow(text, x, y, 1.0f, glm::vec4(1.0f));
}

const Application::SlotBox* Application::slotBoxAt(int mouseX, int mouseY) const
{
    // The boxes were recorded in interface pixels, so the cursor has
    // to be read in them too.
    const float unit = static_cast<float>(guiScale());
    const float x = static_cast<float>(mouseX) / unit;
    const float y = static_cast<float>(mouseY) / unit;

    for (const SlotBox& box : m_slotBoxes)
        if (x >= box.x && x < box.x + box.hitWidth() && y >= box.y && y < box.y + box.size)
            return &box;

    return nullptr;
}

void Application::handleInventoryClick(int mouseX, int mouseY, bool rightButton, bool shiftHeld)
{
    const SlotBox* box = slotBoxAt(mouseX, mouseY);
    if (!box)
    {
        // Clicking outside the panel throws the held stack into the world,
        // which is the only way to get rid of something in survival.
        if (!m_inventory.cursor().empty()) returnCursorToWorld();
        return;
    }

    switch (box->kind)
    {
        case SlotBox::Kind::Tab:
            m_creativeTab = box->index;
            m_paletteRow = 0;   // a new tab starts at the top
            m_audio.play(Sound::Click, 0.45f);
            break;

        case SlotBox::Kind::Palette:
            m_inventory.takeFromPalette(static_cast<BlockId>(box->index));
            m_audio.play(Sound::Click, 0.4f);
            break;

        case SlotBox::Kind::Enchant:
            buyEnchantment(box->index);
            break;

        case SlotBox::Kind::Trade:
            selectTrade(box->index);
            break;

        case SlotBox::Kind::TradeTake:
            takeTradeResult(shiftHeld);
            break;

        case SlotBox::Kind::AnvilTake:
            takeAnvilResult();
            break;

        case SlotBox::Kind::BeaconPower:
            if (box->index >= 100) m_beaconSecondary = box->index - 100;
            else
            {
                m_beaconPrimary = box->index;
                if (m_beaconSecondary >= 0 && m_beaconSecondary != static_cast<int>(Effects::Kind::Regeneration))
                    m_beaconSecondary = m_beaconPrimary;
            }
            m_audio.play(Sound::Click, 0.45f);
            break;

        case SlotBox::Kind::GrindTake:
            takeGrindResult(shiftHeld);
            break;

        case SlotBox::Kind::BeaconConfirm:
            confirmBeacon();
            break;

        case SlotBox::Kind::BeaconCancel:
            m_audio.play(Sound::Click, 0.45f);
            closeInventory();
            setMouseCaptured(true);
            break;

        case SlotBox::Kind::Destroy:
            // Whatever is in hand is gone, which is the bin's whole job.
            if (!m_inventory.cursor().empty())
            {
                m_inventory.cursor() = ItemStack{};
                m_audio.play(Sound::Click, 0.35f);
            }
            break;

        case SlotBox::Kind::Slot:
            if (m_crafterOpen && box->index >= Inventory::CHEST_FIRST &&
                box->index < Inventory::CHEST_FIRST + CRAFTER_SLOTS)
            {
                const int cell = box->index - Inventory::CHEST_FIRST;
                const bool handEmpty = m_inventory.cursor().empty();
                if (crafterSlotDisabled(cell))
                {
                    if (handEmpty && !shiftHeld) setCrafterSlotDisabled(cell, false);
                    m_audio.play(Sound::Click, 0.35f, handEmpty ? 1.0f : 0.6f);
                    break;
                }
                if (handEmpty && !shiftHeld && m_inventory.slot(box->index).empty())
                {
                    setCrafterSlotDisabled(cell, true);
                    m_audio.play(Sound::Click, 0.35f, 0.8f);
                    break;
                }
            }
            if (shiftHeld) m_inventory.quickMove(box->index);
            else if (rightButton) m_inventory.rightClick(box->index);
            else m_inventory.leftClick(box->index);
            // Any click can change what the grid adds up to, and working
            // out which ones can is not worth the bug it would cause.
            m_inventory.refreshCraftResult();
            refreshAnvilResult();
            refreshTradeResult();
            m_audio.play(Sound::Click, 0.35f);
            break;
    }
}

// Closing the screen must not swallow anything: the cursor stack and the
// crafting grid both go back to the player, and whatever will not fit is
// dropped at their feet.
void Application::openHorse(Mob& horse)
{
    m_horseOpen = true;
    m_horseSeed = horse.birthSeed();
    ItemStack saddle;
    if (horse.saddled()) { saddle.id = Items::Saddle; saddle.count = 1; }
    m_inventory.slot(Inventory::HORSE_SADDLE) = saddle;
    ItemStack armour;
    if (horse.horseArmour()) { armour.id = horse.horseArmour(); armour.count = 1; }
    m_inventory.slot(Inventory::HORSE_ARMOUR) = armour;
}

Grindstone::Result Application::grindResult() const
{
    auto input = [this](int index) {
        const ItemStack& stack = m_inventory.slot(index);
        Anvil::Input in;
        in.id = stack.id;
        in.count = stack.empty() ? 0 : stack.count;
        in.damage = stack.damage;
        in.enchantments = stack.enchantments;
        return in;
    };
    return Grindstone::combine(input(Inventory::GRIND_TOP), input(Inventory::GRIND_BOTTOM));
}

void Application::openGrindstone(const glm::ivec3& block)
{
    m_grindBlock = block;
    m_grindOpen = true;
    m_inventoryOpen = true;
    setMouseCaptured(false);
}

void Application::takeGrindResult(bool toInventory)
{
    if (!m_grindOpen) return;
    const Grindstone::Result made = grindResult();
    if (!made.valid) return;
    if (!toInventory && !m_inventory.cursor().empty()) return;

    ItemStack taken;
    taken.id = made.id;
    taken.count = made.count;
    taken.damage = made.damage;
    taken.enchantments = made.enchantments;
    if (toInventory)
    {
        const int left = m_inventory.addStack(taken);
        if (left > 0) return;
    }
    else
        m_inventory.cursor() = taken;

    ItemStack& top = m_inventory.slot(Inventory::GRIND_TOP);
    ItemStack& bottom = m_inventory.slot(Inventory::GRIND_BOTTOM);
    const int power = Grindstone::powerOf(top.empty() ? 0 : top.enchantments) +
                      Grindstone::powerOf(bottom.empty() ? 0 : bottom.enchantments);
    top.clear();
    bottom.clear();

    m_grindRoll ^= m_grindRoll << 13;
    m_grindRoll ^= m_grindRoll >> 17;
    m_grindRoll ^= m_grindRoll << 5;
    const int points = Grindstone::experienceFor(power, m_grindRoll);
    if (points > 0)
        m_orbs.spawnValue(glm::vec3(m_grindBlock) + glm::vec3(0.5f, 1.0f, 0.5f), points);
    m_audio.play(Sound::DigStone, 0.6f, 0.7f);
}

void Application::stowGrindstone()
{
    if (!m_grindOpen) return;
    m_grindOpen = false;
    for (int slot : { Inventory::GRIND_TOP, Inventory::GRIND_BOTTOM })
    {
        ItemStack& held = m_inventory.slot(slot);
        if (held.empty()) continue;
        ItemStack leftOver = held;
        leftOver.count = m_inventory.addStack(held);
        if (leftOver.count > 0) m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), leftOver);
        held.clear();
    }
}

int Application::beaconTiers() const
{
    if (!m_world) return 0;
    struct WorldGround : Beacon::Ground
    {
        const World* world = nullptr;
        BlockId at(int x, int y, int z) const override { return world->getBlock(x, y, z); }
    } ground;
    ground.world = m_world.get();
    return Beacon::tiersUnder(ground, m_beaconBlock.x, m_beaconBlock.y, m_beaconBlock.z);
}

void Application::openBeacon(const glm::ivec3& block)
{
    m_beaconBlock = block;
    m_beaconOpen = true;
    m_beaconPrimary = -1;
    m_beaconSecondary = -1;
    const auto chosen = m_beaconPowers.find({ static_cast<int>(m_dimension), block.x, block.y, block.z });
    if (chosen != m_beaconPowers.end())
    {
        m_beaconPrimary = chosen->second[0];
        m_beaconSecondary = chosen->second[1];
    }
    m_inventoryOpen = true;
    setMouseCaptured(false);
}

void Application::confirmBeacon()
{
    ItemStack& payment = m_inventory.slot(Inventory::BEACON_PAYMENT);
    if (payment.empty() || m_beaconPrimary < 0) return;
    const int tiers = beaconTiers();
    if (!Beacon::isPrimary(m_beaconPrimary, tiers)) return;
    const int secondary = Beacon::isSecondary(m_beaconSecondary, m_beaconPrimary, tiers) ? m_beaconSecondary : -1;
    m_beaconPowers[{ static_cast<int>(m_dimension), m_beaconBlock.x, m_beaconBlock.y, m_beaconBlock.z }] =
        { m_beaconPrimary, secondary };
    payment.count -= 1;
    if (payment.count <= 0) payment.clear();
    m_beaconTimer = 0.0f;
    m_audio.play(Sound::Click, 0.6f, 1.2f);
    closeInventory();
    setMouseCaptured(true);
}

void Application::stowBeacon()
{
    if (!m_beaconOpen) return;
    m_beaconOpen = false;
    ItemStack& held = m_inventory.slot(Inventory::BEACON_PAYMENT);
    if (held.empty()) return;
    ItemStack leftOver = held;
    leftOver.count = m_inventory.addStack(held);
    if (leftOver.count > 0) m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), leftOver);
    held.clear();
}

void Application::stowHorse()
{
    if (!m_horseOpen) return;
    m_horseOpen = false;
    Mob* horse = nullptr;
    for (Mob& mob : m_entities.mobs())
        if (mob.typeId() == MobId::Horse && mob.birthSeed() == m_horseSeed && mob.alive()) horse = &mob;
    ItemStack& saddle = m_inventory.slot(Inventory::HORSE_SADDLE);
    ItemStack& armour = m_inventory.slot(Inventory::HORSE_ARMOUR);
    if (horse)
    {
        horse->setSaddled(!saddle.empty() && saddle.id == Items::Saddle);
        horse->setHorseArmour(!armour.empty() && isHorseArmour(armour.id) ? armour.id : 0);
    }
    else
    {
        if (!saddle.empty() && m_inventory.addStack(saddle) > 0) m_drops.spawn(m_player.position, saddle.id, 1);
        if (!armour.empty() && m_inventory.addStack(armour) > 0) m_drops.spawn(m_player.position, armour.id, 1);
    }
    saddle.clear();
    armour.clear();
}

void Application::closeInventory()
{
    stowHorse();
    stowBeacon();
    stowGrindstone();
    stowChest();
    stowCrafter();
    stowFurnace();
    stowEnchantingTable();
    stowAnvil();
    stowBrewingStand();
    stowTrading();
    m_tradeOpen = false;
    returnCursorToWorld();
    m_inventoryOpen = false;
    m_benchOpen = false;
    m_draggingScroll = false;
    m_inventory.setCraftSize(2);
}

const Mob* Application::tradingVillager() const
{
    if (!m_tradeOpen) return nullptr;
    for (const Mob& mob : m_entities.mobs())
        if (mob.typeId() == MobId::Villager && mob.alive() && mob.birthSeed() == m_tradeSeed)
            return &mob;
    return nullptr;
}

void Application::openTrading(const Mob& villager)
{
    m_tradeOpen = true;
    m_tradeSelected = -1;
    m_tradeScroll = 0;
    m_tradeSeed = villager.birthSeed();
    // A villager that lives in a village keeps that building's trade.
    // One met out in the country has no building to take it from, so it
    // falls back to the one its birth seed draws -- which is what every
    // villager did before there were villages.
    m_tradeWith = (villager.professionSlot() >= 0)
                      ? static_cast<Trading::Profession>(villager.professionSlot())
                      : Trading::professionFor(villager.birthSeed());
    TraderRecord& record = openTrader();
    record.position = villager.position();
    record.dimension = static_cast<int>(m_dimension);
    m_inventoryOpen = true;
    setMouseCaptured(false);
}

Application::TraderRecord& Application::openTrader()
{
    TraderRecord& record = m_traders[m_tradeSeed];
    if (record.seed != m_tradeSeed)
    {
        record.seed = m_tradeSeed;
        record.profession = static_cast<int>(m_tradeWith);
        record.dimension = static_cast<int>(m_dimension);
    }
    const size_t deals = Trading::dealsFor(m_tradeWith).size();
    if (record.uses.size() != deals) record.uses.resize(deals, 0);
    return record;
}

// Picking a deal puts back whatever was waiting to pay with, then fills
// the payment slots from the pack -- as much as a stack will hold, so
// the same deal can be struck again and again by taking the result.
void Application::selectTrade(int deal)
{
    const std::vector<Trading::Deal> deals = Trading::dealsFor(m_tradeWith);
    if (deal < 0 || deal >= static_cast<int>(deals.size())) return;
    m_tradeSelected = deal;

    for (int slot : { Inventory::TRADE_PAY_A, Inventory::TRADE_PAY_B })
    {
        ItemStack& held = m_inventory.slot(slot);
        if (held.empty()) continue;
        const int leftOver = m_inventory.addStack(held);
        if (leftOver > 0)
        {
            ItemStack rest = held;
            rest.count = leftOver;
            m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), rest);
        }
        held.clear();
    }

    const Trading::Offer& offer = deals[static_cast<size_t>(deal)].offer;
    auto fill = [&](int slot, StackId want) {
        if (want == Blocks::Air) return;
        ItemStack& into = m_inventory.slot(slot);
        const int room = maxStackOf(want);
        for (int i = 0; i < Inventory::MAIN_SLOTS && into.count < room; ++i)
        {
            ItemStack& from = m_inventory.slot(i);
            if (from.empty() || from.id != want || from.enchantments != 0) continue;
            const int moved = std::min(from.count, room - into.count);
            if (into.empty())
            {
                into = from;
                into.count = 0;
            }
            into.count += moved;
            from.count -= moved;
            if (from.count <= 0) from.clear();
        }
    };
    fill(Inventory::TRADE_PAY_A, offer.want);
    fill(Inventory::TRADE_PAY_B, offer.also);
    refreshTradeResult();
    m_audio.play(Sound::Click, 0.35f);
}

void Application::refreshTradeResult()
{
    ItemStack& result = m_inventory.slot(Inventory::TRADE_RESULT);
    result.clear();
    if (!m_tradeOpen) return;

    const std::vector<Trading::Deal> deals = Trading::dealsFor(m_tradeWith);
    TraderRecord& record = openTrader();
    const int rank = Trading::rankFor(record.xp);
    const ItemStack& payA = m_inventory.slot(Inventory::TRADE_PAY_A);
    const ItemStack& payB = m_inventory.slot(Inventory::TRADE_PAY_B);

    if (m_tradeSelected < 0 && !payA.empty())
        for (size_t i = 0; i < deals.size(); ++i)
            if (deals[i].rank <= rank && deals[i].offer.want == payA.id)
            {
                m_tradeSelected = static_cast<int>(i);
                break;
            }

    if (m_tradeSelected < 0 || m_tradeSelected >= static_cast<int>(deals.size())) return;
    const Trading::Deal& deal = deals[static_cast<size_t>(m_tradeSelected)];
    if (deal.rank > rank) return;
    if (record.uses[static_cast<size_t>(m_tradeSelected)] >= deal.maxUses) return;

    const Trading::Offer& offer = deal.offer;
    if (payA.id != offer.want || payA.count < offer.wantCount) return;
    if (offer.also != Blocks::Air && (payB.id != offer.also || payB.count < offer.alsoCount)) return;

    result.id = offer.give;
    result.count = offer.giveCount;
    result.enchantments = offer.giveEnchantments;
}

void Application::takeTradeResult(bool all)
{
    if (!m_tradeOpen) return;

    for (int round = 0; round < 64; ++round)
    {
        refreshTradeResult();
        const ItemStack made = m_inventory.slot(Inventory::TRADE_RESULT);
        if (made.empty()) return;

        ItemStack& cursor = m_inventory.cursor();
        if (all)
        {
            if (m_inventory.addStack(made) > 0) return;
        }
        else
        {
            const bool fits = cursor.empty() ||
                              (cursor.id == made.id && cursor.enchantments == made.enchantments &&
                               cursor.count + made.count <= maxStackOf(made.id));
            if (!fits) return;
            if (cursor.empty()) cursor = made;
            else cursor.count += made.count;
        }

        const std::vector<Trading::Deal> deals = Trading::dealsFor(m_tradeWith);
        const Trading::Deal& deal = deals[static_cast<size_t>(m_tradeSelected)];
        ItemStack& payA = m_inventory.slot(Inventory::TRADE_PAY_A);
        ItemStack& payB = m_inventory.slot(Inventory::TRADE_PAY_B);
        payA.count -= deal.offer.wantCount;
        if (payA.count <= 0) payA.clear();
        if (deal.offer.also != Blocks::Air)
        {
            payB.count -= deal.offer.alsoCount;
            if (payB.count <= 0) payB.clear();
        }

        TraderRecord& record = openTrader();
        const int rankBefore = Trading::rankFor(record.xp);
        record.uses[static_cast<size_t>(m_tradeSelected)] += 1;
        if (const Mob* trader = tradingVillager())
            sprites(SpriteParticles::Kind::Happy, trader->position() + glm::vec3(0.0f, 1.9f, 0.0f), 5, 0.5f,
                    0.3f, 0.25f, 1.0f);
        record.xp += deal.villagerXp;
        for (const Mob& mob : m_entities.mobs())
            if (mob.typeId() == MobId::Villager && mob.birthSeed() == m_tradeSeed)
                record.position = mob.position();
        m_player.experience += deal.playerXp;

        const int rankAfter = Trading::rankFor(record.xp);
        if (rankAfter > rankBefore)
            m_audio.play(Sound::Pickup, 0.8f, 0.6f);
        else
            m_audio.play(Sound::MobVillagerSay, 0.7f, 1.3f);

        if (!all) break;
    }
    refreshTradeResult();
}

void Application::stowTrading()
{
    for (int slot : { Inventory::TRADE_PAY_A, Inventory::TRADE_PAY_B })
    {
        ItemStack& held = m_inventory.slot(slot);
        if (held.empty()) continue;
        const int leftOver = m_inventory.addStack(held);
        if (leftOver > 0)
        {
            ItemStack rest = held;
            rest.count = leftOver;
            m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), rest);
        }
        held.clear();
    }
    m_inventory.slot(Inventory::TRADE_RESULT).clear();

    if (m_tradeOpen)
    {
        auto found = m_traders.find(m_tradeSeed);
        if (found != m_traders.end())
        {
            bool touched = found->second.xp > 0;
            for (int used : found->second.uses) touched = touched || used > 0;
            if (!touched) m_traders.erase(found);
        }
    }
    m_tradeSelected = -1;
}

void Application::openEnchantingTable(const glm::ivec3& block)
{
    m_enchantBlock = block;
    m_enchantOpen = true;

    WorldEnchantRoom room;
    room.world = m_world.get();
    m_enchantShelves = Enchanting::shelvesAround(room, block.x, block.y, block.z);

    // One roll, held for as long as the screen is open, so the offers
    // stand still while you decide. Seeded from where the table is and
    // what you have earned, so two tables do not offer the same thing
    // and a table does not offer the same thing for ever.
    m_enchantRoll = static_cast<uint32_t>(block.x * 73856093 ^ block.y * 19349663 ^
                                          block.z * 83492791) ^
                    static_cast<uint32_t>(m_player.experience * 2654435761u);

    m_inventoryOpen = true;
    setMouseCaptured(false);
}

// Whatever was being enchanted goes back to the player, and the slot is
// emptied so the next thing to open does not find it there.
void Application::stowEnchantingTable()
{
    if (!m_enchantOpen) return;

    // The item and the lapis both go back into the pack if they fit, and
    // onto the floor if they do not: anything left here would vanish.
    for (int slot : { Inventory::ENCHANT_SLOT, Inventory::ENCHANT_LAPIS })
    {
        ItemStack& held = m_inventory.slot(slot);
        if (held.empty()) continue;

        ItemStack leftOver = held;
        leftOver.count = m_inventory.addStack(held);
        if (leftOver.count > 0)
            m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), leftOver);
        held.clear();
    }

    m_enchantOpen = false;
}

void Application::openBrewingStand(const glm::ivec3& block)
{
    m_brewBlock = block;
    m_brewOpen = true;
    m_brewProgress = 0.0f;

    m_inventoryOpen = true;
    setMouseCaptured(false);
}

void Application::stowBrewingStand()
{
    if (!m_brewOpen) return;

    for (int i = 0; i < Inventory::BREW_SLOTS; ++i)
    {
        ItemStack& stack = m_inventory.slot(Inventory::BREW_FIRST + i);
        if (stack.empty()) continue;

        const int leftOver = m_inventory.addStack(stack);
        if (leftOver > 0)
            m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), stack);
        stack.clear();
    }

    m_brewOpen = false;
    m_brewProgress = 0.0f;

    // The stand keeps nothing once it is shut here, its fuel included:
    // what was left of the powder burning in it goes with the screen.
    m_brewFuel = 0;
}

void Application::tickBrewing(float deltaTime)
{
    if (!m_brewOpen) { m_brewProgress = 0.0f; return; }

    // Out of fuel with powder to hand: one goes in and lasts twenty brews.
    if (m_brewFuel <= 0)
    {
        ItemStack& powder = m_inventory.slot(Inventory::BREW_FUEL);
        if (powder.id == Items::BlazePowder && powder.count > 0)
        {
            m_brewFuel = Brewing::FUEL_PER_POWDER;
            if (--powder.count <= 0) powder.clear();
        }
    }

    const ItemStack& ingredient = m_inventory.slot(Inventory::BREW_INGREDIENT);
    if (ingredient.empty() || !Brewing::isIngredient(ingredient.id))
    {
        m_brewProgress = 0.0f;
        return;
    }

    // Only bottles this ingredient would actually change count. A stand
    // with three finished potions in it and a spoon of sugar on top has
    // nothing left to do.
    int brewable = 0;
    for (int i = 0; i < Inventory::BREW_BOTTLES; ++i)
    {
        const ItemStack& bottle = m_inventory.slot(Inventory::BREW_BOTTLE_FIRST + i);
        if (bottle.id != Items::Potion) continue;
        if (Brewing::brew(Brewing::fromDamage(bottle.damage), ingredient.id) !=
            Brewing::Potion::Count)
            ++brewable;
    }

    if (brewable == 0) { m_brewProgress = 0.0f; return; }

    // A brew burns its fuel as it starts, and with none left it does not
    // start at all.
    if (m_brewProgress <= 0.0f)
    {
        if (m_brewFuel <= 0) return;
        --m_brewFuel;
    }

    m_brewProgress += deltaTime;
    if (m_brewProgress < Brewing::BREW_SECONDS) return;

    // One ingredient does every bottle at once, as Minecraft's does.
    for (int i = 0; i < Inventory::BREW_BOTTLES; ++i)
    {
        ItemStack& bottle = m_inventory.slot(Inventory::BREW_BOTTLE_FIRST + i);
        if (bottle.id != Items::Potion) continue;

        const Brewing::Potion made =
            Brewing::brew(Brewing::fromDamage(bottle.damage), ingredient.id);
        if (made == Brewing::Potion::Count) continue;

        bottle.damage = Brewing::toDamage(made);
    }

    ItemStack& spend = m_inventory.slot(Inventory::BREW_INGREDIENT);
    spend.count -= 1;
    if (spend.count <= 0) spend.clear();

    m_brewProgress = 0.0f;
    m_audio.play(Sound::Click, 0.5f, 1.4f);
}

void Application::openAnvil(const glm::ivec3& block)
{
    m_anvilBlock = block;
    m_anvilOpen = true;
    refreshAnvilResult();

    m_inventoryOpen = true;
    setMouseCaptured(false);
}

// Both inputs go back to the player. The result is only ever a preview
// of what they would make, so it is thrown away rather than returned --
// handing it over as well would mint a free item on every close.
void Application::stowAnvil()
{
    if (!m_anvilOpen) return;

    for (int slot : { Inventory::ANVIL_LEFT, Inventory::ANVIL_RIGHT })
    {
        ItemStack& stack = m_inventory.slot(slot);
        if (stack.empty()) continue;

        const int leftOver = m_inventory.addStack(stack);
        if (leftOver > 0)
            m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), stack);
        stack.clear();
    }

    m_inventory.slot(Inventory::ANVIL_RESULT).clear();
    m_anvilOpen = false;
}

Anvil::Result Application::anvilResult() const
{
    if (!m_anvilOpen) return Anvil::Result{};

    const ItemStack& left = m_inventory.slot(Inventory::ANVIL_LEFT);
    const ItemStack& right = m_inventory.slot(Inventory::ANVIL_RIGHT);

    return Anvil::combine({ left.id, left.count, left.damage, left.enchantments },
                          { right.id, right.count, right.damage, right.enchantments });
}

void Application::refreshAnvilResult()
{
    if (!m_anvilOpen) return;

    ItemStack& preview = m_inventory.slot(Inventory::ANVIL_RESULT);
    const Anvil::Result made = anvilResult();

    if (!made.valid)
    {
        preview.clear();
        return;
    }

    preview.id = made.id;
    preview.count = 1;
    preview.damage = made.damage;
    preview.enchantments = made.enchantments;
}

// Taking it is what spends the levels and the inputs. Until then the
// result slot is a price tag, and the player can change their mind.
void Application::takeAnvilResult()
{
    if (!m_anvilOpen) return;
    if (!m_inventory.cursor().empty()) return;   // hands full

    const Anvil::Result made = anvilResult();
    if (!made.valid) return;

    const int level = Experience::levelFor(m_player.experience);
    if (!m_player.creative() && level < made.levelCost) return;

    ItemStack taken;
    taken.id = made.id;
    taken.count = 1;
    taken.damage = made.damage;
    taken.enchantments = made.enchantments;
    m_inventory.cursor() = taken;

    // The left-hand item is always consumed whole; the right-hand one
    // only by as many as the work actually took.
    m_inventory.slot(Inventory::ANVIL_LEFT).clear();

    ItemStack& right = m_inventory.slot(Inventory::ANVIL_RIGHT);
    right.count -= made.used;
    if (right.count <= 0) right.clear();

    if (!m_player.creative())
    {
        // Spent the way the enchanting table spends them: whole levels
        // off, with the fraction carried across, since a level is not
        // worth the same number of points either side of the move.
        const float along = Experience::progress(m_player.experience);
        const int after = std::max(0, level - made.levelCost);
        m_player.experience =
            Experience::totalForLevel(after) +
            static_cast<int>(along * static_cast<float>(Experience::toNextLevel(after)));
    }

    m_audio.play(Sound::Click, 0.5f);
    if (!m_player.creative() && m_world)
    {
        m_wearSeed ^= m_wearSeed << 13;
        m_wearSeed ^= m_wearSeed >> 17;
        m_wearSeed ^= m_wearSeed << 5;
        if (m_wearSeed % 100u < 12u)
        {
            const BlockId here = m_world->getBlock(m_anvilBlock.x, m_anvilBlock.y, m_anvilBlock.z);
            const BlockId worn = here == Blocks::Anvil ? Blocks::AnvilChipped
                               : here == Blocks::AnvilChipped ? Blocks::AnvilDamaged : Blocks::Air;
            m_world->setBlock(m_anvilBlock.x, m_anvilBlock.y, m_anvilBlock.z, worn);
            m_net.onLocalBlockChange(m_anvilBlock.x, m_anvilBlock.y, m_anvilBlock.z, worn);
            m_audio.play(Sound::DigStone, 0.8f, worn == Blocks::Air ? 0.6f : 0.9f);
        }
    }
    refreshAnvilResult();
}

void Application::buyEnchantment(int row)
{
    if (!m_enchantOpen) return;

    ItemStack& subject = m_inventory.slot(Inventory::ENCHANT_SLOT);
    if (subject.empty()) return;

    const Enchanting::Offer offer =
        Enchanting::offerFor(row, m_enchantShelves, subject.id, m_enchantRoll);
    if (!offer.valid()) return;

    // The level shown on the row is what you must have reached; what
    // the row takes is its own number of levels and of lapis.
    const int level = Experience::levelFor(m_player.experience);
    ItemStack& lapis = m_inventory.slot(Inventory::ENCHANT_LAPIS);
    const int lapisHeld = lapis.id == Items::LapisLazuli ? lapis.count : 0;
    if (!Enchanting::affordable(offer, row, level, lapisHeld, m_player.creative())) return;
    const Enchanting::Price price = Enchanting::priceOf(row);

    // Already on it, and no better: nothing to sell.
    if (Enchanting::levelOf(subject.enchantments, offer.kind) >= offer.level) return;

    // Nothing that contradicts what it already carries.
    for (const auto& already : Enchanting::listOf(subject.enchantments))
        if (Enchanting::conflicts(already.first, offer.kind)) return;

    subject.enchantments = Enchanting::withLevel(subject.enchantments, offer.kind, offer.level);

    if (!m_player.creative())
    {
        // The levels are spent, not merely required. Minecraft takes
        // whole levels and leaves the bar where it stood, so the
        // fraction is carried across rather than rounded away -- the
        // cost of a level is not the same either side of the move.
        const float along = Experience::progress(m_player.experience);
        const int after = std::max(0, level - price.levels);
        m_player.experience =
            Experience::totalForLevel(after) +
            static_cast<int>(along * static_cast<float>(Experience::toNextLevel(after)));
        m_player.experienceOwedHundredths = 0;

        lapis.count -= price.lapis;
        if (lapis.count <= 0) lapis.clear();
    }

    // A fresh roll, so the next three offers are not the same three.
    m_enchantRoll = Enchanting::nextRoll(m_enchantRoll);
    m_audio.play(Sound::Pickup, 0.7f, 0.7f);
}

void Application::openBench(const glm::ivec3& block)
{
    m_benchBlock = block;
    m_benchOpen = true;
    m_inventory.setCraftSize(3);
    m_inventoryOpen = true;
    setMouseCaptured(false);
}

void Application::openChest(const glm::ivec3& block)
{
    m_chestBlock = block;
    m_chestOpen = true;
    if (m_world && m_world->getBlock(block.x, block.y, block.z) == Blocks::Barrel)
    {
        m_world->setBlock(block.x, block.y, block.z, Blocks::BarrelOpen);
        m_net.onLocalBlockChange(block.x, block.y, block.z, Blocks::BarrelOpen);
    }

    fillVillageChest(block);

    const Chests::Contents& held = m_chests.at(block);
    for (int i = 0; i < Inventory::CHEST_SLOTS; ++i)
        m_inventory.slot(Inventory::CHEST_FIRST + i) = held[i];

    m_inventoryOpen = true;
    setMouseCaptured(false);
}

// Whether this chest is one of a village's, and what is in it if so.
std::vector<VillageLoot::Stack> Application::villageChestLoot(const glm::ivec3& block) const
{
    const Village::Site site = Village::siteFor(Village::regionOf(block.x),
                                                Village::regionOf(block.z),
                                                m_world->seed());
    if (!Village::reaches(site, block.x, block.z, block.x, block.z)) return {};

    // The cheap tests passed; now it is worth asking whether the
    // village was actually built here.
    if (!m_world->generator().nearestVillage(block.x, block.z, 0).exists) return {};

    Village::Building building;
    if (!Village::buildingAt(site, m_world->seed(), block.x, block.z, building)) return {};
    if (!Village::isHouse(building.kind)) return {};

    return VillageLoot::rollFor(building.kind, block.x, block.y, block.z, m_world->seed());
}

// A chest the generator put down has never been opened before, so
// there is nothing in the table for it. What it holds is rolled from
// where it stands -- the same every time anyone finds it, and nothing
// is written to the save until the player takes something.
//
// Two places it can have come from: a village building, which is the
// cheapest to recognise and so is asked first, or one of the
// structures scattered about the world.
void Application::fillVillageChest(const glm::ivec3& block)
{
    if (!m_world) return;
    if (m_dimension != Dimension::Overworld) return;

    // Already known, so it has been opened before -- or the player
    // built it. Either way it is not ours to fill.
    if (m_chests.has(block)) return;

    std::vector<VillageLoot::Stack> loot = villageChestLoot(block);

    if (loot.empty())
    {
        const WorldGen::Hoard hoard =
            m_world->generator().hoardAt(block.x, block.y, block.z);
        loot = StructureLoot::rollFor(hoard, block.x, block.y, block.z, m_world->seed());
    }

    if (loot.empty()) return;

    Chests::Contents filled{};

    // Spread through the chest rather than packed into the first few
    // slots, which is how a found chest looks in Minecraft.
    for (size_t i = 0; i < loot.size() && i < static_cast<size_t>(Chests::SLOTS); ++i)
    {
        const uint32_t where = hashCoords(block.x + static_cast<int>(i), block.y, block.z,
                                          m_world->seed() + 61100);
        int slot = static_cast<int>(where % static_cast<uint32_t>(Chests::SLOTS));

        // Walk on to the next free one, so two draws landing on the
        // same slot do not lose one of the stacks.
        for (int step = 0; step < Chests::SLOTS && !filled[slot].empty(); ++step)
            slot = (slot + 1) % Chests::SLOTS;

        filled[slot].id = loot[i].id;
        filled[slot].count = loot[i].count;
    }

    m_chests.put(block, filled);
}

// A crafter is opened the same way a chest is, and for the same
// reason: the panel works on the inventory's own slots, so what the
// block holds is copied in while it is open and copied back after.
void Application::openCrafter(const glm::ivec3& block)
{
    m_crafterBlock = block;
    m_crafterOpen = true;

    const Chests::Contents& held = m_chests.at(block);
    for (int i = 0; i < CRAFTER_SLOTS; ++i)
        m_inventory.slot(Inventory::CHEST_FIRST + i) = held[i];

    m_inventoryOpen = true;
    setMouseCaptured(false);
}

bool Application::crafterSlotDisabled(int cell)
{
    if (!m_crafterOpen || cell < 0 || cell >= CRAFTER_SLOTS || !m_chests.has(m_crafterBlock))
        return false;
    const ItemStack& marker = m_chests.at(m_crafterBlock)[CRAFTER_MASK_SLOT];
    if (marker.id != Blocks::Bedrock || marker.count <= 0) return false;
    return ((marker.count - 1) >> cell) & 1;
}

void Application::setCrafterSlotDisabled(int cell, bool disabled)
{
    if (!m_crafterOpen || cell < 0 || cell >= CRAFTER_SLOTS) return;
    ItemStack& marker = m_chests.at(m_crafterBlock)[CRAFTER_MASK_SLOT];
    int mask = (marker.id == Blocks::Bedrock && marker.count > 0) ? marker.count - 1 : 0;
    if (disabled) mask |= 1 << cell;
    else mask &= ~(1 << cell);
    if (mask == 0) marker.clear();
    else
    {
        marker.id = Blocks::Bedrock;
        marker.count = mask + 1;
    }
}

void Application::stowCrafter()
{
    if (!m_crafterOpen) return;

    Chests::Contents& held = m_chests.at(m_crafterBlock);
    for (int i = 0; i < CRAFTER_SLOTS; ++i)
    {
        held[i] = m_inventory.slot(Inventory::CHEST_FIRST + i);
        m_inventory.slot(Inventory::CHEST_FIRST + i).clear();
    }

    m_chests.forgetIfEmpty(m_crafterBlock);
    m_crafterOpen = false;
}

void Application::stowChest()
{
    if (!m_chestOpen) return;
    if (m_world && m_world->getBlock(m_chestBlock.x, m_chestBlock.y, m_chestBlock.z) == Blocks::BarrelOpen)
    {
        m_world->setBlock(m_chestBlock.x, m_chestBlock.y, m_chestBlock.z, Blocks::Barrel);
        m_net.onLocalBlockChange(m_chestBlock.x, m_chestBlock.y, m_chestBlock.z, Blocks::Barrel);
    }

    Chests::Contents& held = m_chests.at(m_chestBlock);
    for (int i = 0; i < Inventory::CHEST_SLOTS; ++i)
    {
        held[i] = m_inventory.slot(Inventory::CHEST_FIRST + i);
        m_inventory.slot(Inventory::CHEST_FIRST + i).clear();
    }

    // An emptied chest is a plain block again; keeping a row of
    // twenty-seven nothings for it would bloat every save.
    m_chests.forgetIfEmpty(m_chestBlock);
    m_chestOpen = false;
}

// Write-through, so a save can see what an open container holds
// without the player losing the screen they are working in.
void Application::syncOpenContainers()
{
    if (m_chestOpen)
    {
        Chests::Contents& held = m_chests.at(m_chestBlock);
        for (int i = 0; i < Inventory::CHEST_SLOTS; ++i)
            held[i] = m_inventory.slot(Inventory::CHEST_FIRST + i);
    }

    if (m_crafterOpen)
    {
        Chests::Contents& held = m_chests.at(m_crafterBlock);
        for (int i = 0; i < CRAFTER_SLOTS; ++i)
            held[i] = m_inventory.slot(Inventory::CHEST_FIRST + i);
    }

    if (m_furnaceOpen)
    {
        Furnaces::State& state = m_furnaces.at(m_furnaceBlock);
        state.input = m_inventory.slot(Inventory::FURNACE_INPUT);
        state.fuel = m_inventory.slot(Inventory::FURNACE_FUEL);
        state.output = m_inventory.slot(Inventory::FURNACE_OUTPUT);
    }

    // The enchanting table has nowhere of its own to keep anything, so
    // what is in it is not saved at all. Closing the screen hands it
    // back; a save taken with the screen open simply does not see it,
    // the same as the crafting grid beside it.
}

void Application::openFurnace(const glm::ivec3& block)
{
    m_furnaceBlock = block;
    m_furnaceOpen = true;

    Furnaces::State& state = m_furnaces.at(block);

    // Which machine this is comes off the block, not off the record:
    // the record is made the first time one is opened, and a smoker
    // that remembered itself as a furnace would cook at half speed and
    // take ore.
    state.kind = Smelting::kindOf(m_world->getBlock(block.x, block.y, block.z));

    m_inventory.slot(Inventory::FURNACE_INPUT) = state.input;
    m_inventory.slot(Inventory::FURNACE_FUEL) = state.fuel;
    m_inventory.slot(Inventory::FURNACE_OUTPUT) = state.output;

    m_inventoryOpen = true;
    setMouseCaptured(false);
}

// Back into the furnace, and out of the slots the screen was using, so
// the next thing to open does not find yesterday's ore in them.
void Application::stowFurnace()
{
    if (!m_furnaceOpen) return;

    Furnaces::State& state = m_furnaces.at(m_furnaceBlock);
    state.input = m_inventory.slot(Inventory::FURNACE_INPUT);
    state.fuel = m_inventory.slot(Inventory::FURNACE_FUEL);
    state.output = m_inventory.slot(Inventory::FURNACE_OUTPUT);

    for (int i = 0; i < Inventory::FURNACE_SLOTS; ++i)
        m_inventory.slot(Inventory::FURNACE_FIRST + i).clear();

    m_furnaceOpen = false;
}

// Every furnace in the world steps forward, open or not -- a furnace you
// walked away from goes on smelting, which is the point of one. The open
// one is run from the slots the screen is editing.
void Application::tickFurnaces(float deltaTime)
{
    // Furnaces are read back before the chunks they stand in, so their
    // blocks cannot be set right until there are blocks to set.
    if (m_pendingFurnaceLight && m_worldReady)
    {
        m_pendingFurnaceLight = false;
        for (const auto& entry : m_furnaces.all())
        {
            // all() spans every dimension, since the level file holds
            // them all; only the one being stood in has blocks to set.
            if (entry.first[0] != static_cast<int>(m_dimension)) continue;
            setFurnaceLit(glm::ivec3(entry.first[1], entry.first[2], entry.first[3]),
                          entry.second.lit());
        }
    }

    if (m_furnaceOpen)
    {
        Furnaces::State& state = m_furnaces.at(m_furnaceBlock);
        const bool wasLit = state.lit();

        Furnace running;
        running.input = &m_inventory.slot(Inventory::FURNACE_INPUT);
        running.fuel = &m_inventory.slot(Inventory::FURNACE_FUEL);
        running.output = &m_inventory.slot(Inventory::FURNACE_OUTPUT);
        running.burnLeft = state.burnLeft;
        running.burnTotal = state.burnTotal;
        running.cooked = state.cooked;
        running.kind = state.kind;

        running.tick(deltaTime);

        state.burnLeft = running.burnLeft;
        state.burnTotal = running.burnTotal;
        state.cooked = running.cooked;

        if (state.lit() != wasLit) setFurnaceLit(m_furnaceBlock, state.lit());

        // Experience for what has been taken out since last frame.
        const ItemStack& made = m_inventory.slot(Inventory::FURNACE_OUTPUT);
        const int left = (!made.empty() && made.id == m_furnaceOutputId) ? made.count : 0;
        if (m_furnaceOutputId != Blocks::Air && left < m_furnaceOutputCount)
        {
            const int taken = m_furnaceOutputCount - left;
            if (m_player.addExperienceHundredths(
                    taken * Experience::smeltingHundredths(m_furnaceOutputId)))
                m_audio.play(Sound::Pickup, 0.7f, 0.55f);
        }
        m_furnaceOutputId = made.empty() ? Blocks::Air : made.id;
        m_furnaceOutputCount = made.empty() ? 0 : made.count;
    }

    const glm::ivec3* open = m_furnaceOpen ? &m_furnaceBlock : nullptr;
    for (const glm::ivec3& where : m_furnaces.tick(deltaTime, open))
        setFurnaceLit(where, m_furnaces.at(where).lit());
}

// The lit furnace is a separate block id, because a block here has
// nowhere to keep a flag. Swapping it is what makes the mouth glow and
// casts light on the floor in front of it.
void Application::setFurnaceLit(const glm::ivec3& block, bool lit)
{
    const BlockId now = m_world->getBlock(block.x, block.y, block.z);
    if (!isFurnaceBlock(now)) return;

    const BlockId wanted = lit ? furnaceLit(now) : furnaceCold(now);
    if (now == wanted) return;

    m_world->setBlock(block.x, block.y, block.z, wanted);
    m_net.onLocalBlockChange(block.x, block.y, block.z, wanted);
}

void Application::returnCursorToWorld()
{
    std::vector<ItemStack> spilled;
    m_inventory.clearCraftGrid(spilled);

    ItemStack& held = m_inventory.cursor();
    if (!held.empty())
    {
        if (m_player.creative())
        {
            held.clear();
        }
        else
        {
            // The whole stack through, so a tool on the cursor keeps
            // its wear and its enchantments: putting back only the id
            // and a count handed back a brand new one.
            ItemStack leftOver = held;
            leftOver.count = m_inventory.addStack(held);
            if (leftOver.count > 0) spilled.push_back(leftOver);
        }
        held.clear();
    }

    if (m_player.creative()) return;

    for (const ItemStack& stack : spilled)
        if (!stack.empty())
            m_drops.spawnStack(m_player.eyePosition() + m_camera.front * 0.8f, stack);
}
