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
    };

    const StackId TAB_NATURE[] = {
        Blocks::Grass, Blocks::SnowGrass, Blocks::Dirt, Blocks::TallGrass,
        Blocks::FlowerRed, Blocks::FlowerYellow, Blocks::Leaves, Blocks::BirchLeaves,
        Blocks::Log, Blocks::BirchLog, Blocks::Cactus, Blocks::Pumpkin,
        Blocks::Sand, Blocks::Snow, Blocks::Ice, Blocks::Clay,
        Blocks::SugarCane,
    };

    const StackId TAB_ORES[] = {
        Blocks::CoalOre, Blocks::IronOre, Blocks::GoldOre, Blocks::DiamondOre,
        Blocks::EmeraldOre,
        Blocks::Stone, Blocks::Cobblestone, Blocks::Gravel, Blocks::Obsidian,
        Blocks::Bedrock,
    };

    const StackId TAB_MISC[] = {
        Blocks::Torch, Blocks::Glowstone, Blocks::Water, Blocks::Lava,
        Blocks::Ice, Blocks::Glass, Blocks::Wool, Blocks::Pumpkin,
        Blocks::Bedrock,
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
        Items::Paper, Items::Book, Items::Emerald, Items::EnderPearl, Items::EyeOfEnder,
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
        { "Everything",       Blocks::Cobblestone,nullptr,      0 },
        { "Your Inventory",   Blocks::Wool,       nullptr,      0 },  // the player's own slots
    };

    constexpr int TAB_COUNT = static_cast<int>(std::size(CREATIVE_TABS));
    constexpr int INVENTORY_TAB = TAB_COUNT - 1;
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
}

// A cube seen from above and to one side: the top face as a rhombus,
// and two of the four sides below it. It is the same two-to-one
// isometric Minecraft uses, which is why a block in a slot there looks
// like a block rather than like a square of its own top texture.
//
// Laid out so the whole thing fits a square of `size` on a side. With
// `a` for the half-width, the rhombus is `a` wide and `a` tall between
// its top and bottom corners, and the sides hang `a` below it, which
// comes to `2a` -- the full square -- in all.
bool Application::drawBlockIcon(float x, float y, float size, StackId id,
                                const glm::vec4& tint)
{
    if (isItem(id)) return false;

    const BlockId block = asBlock(id);
    const BlockInfo& info = blockInfo(block);

    // Only things that really are a box. A flower is two crossed quads
    // and a liquid has no edges; both read better flat, and Minecraft
    // draws them flat too.
    if (info.render != RenderType::Cube && info.render != RenderType::Slab) return false;

    const float a = size * 0.5f;
    const float tall = blockHeight(block);     // a slab stands half as high
    const float sides = a * tall;

    // Bottom-aligned, so a slab sits in the bottom of the slot with the
    // space above it rather than floating in the middle.
    const float top = y + (size - (a + sides));
    const float cx = x + a;

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
    faceUV(sideUV, uv);
    m_ui.texturedShape(m_atlas.textureId(), leftFace, uv, shaded(0.78f), leftLight);

    const glm::vec2 rightFace[4] = { C, B, Bb, Cb };
    faceUV(sideUV, uv);
    m_ui.texturedShape(m_atlas.textureId(), rightFace, uv, shaded(0.60f), rightLight);

    return true;
}

void Application::drawItem(float x, float y, float size, StackId id, int count,
                           const glm::vec4& tint, int damage)
{
    if (id == Blocks::Air) return;

    // Minecraft draws a 16-pixel item inside an 18-pixel slot, so the
    // only margin is the slot's own border. At 19 per cent the icons
    // sat in the middle of their slots like stamps on envelopes.
    const float inset = size * 0.0625f;

    if (!drawBlockIcon(x + inset, y + inset, size - inset * 2.0f, id, tint))
    {
        const TileUV uv = tileUV(atlasTileFor(id));
        m_ui.texturedQuad(m_atlas.textureId(), x + inset, y + inset,
                          size - inset * 2.0f, size - inset * 2.0f,
                          uv.u0, uv.vTop, uv.u1, uv.vBottom, tint);
    }

    // How much of a tool is left, as the bar across the bottom of the
    // icon: green when it is new, red by the time it is about to go.
    const int limit = Tools::maxDurability(Tools::tierOf(id));
    if (limit > 0 && damage > 0)
    {
        const float left = std::clamp(1.0f - static_cast<float>(damage) / limit, 0.0f, 1.0f);
        const float barX = x + size * 0.16f;
        const float barY = y + size * 0.78f;
        const float barWidth = size * 0.68f;
        const float barHeight = std::max(2.0f, size * 0.08f);

        m_ui.quad(barX, barY, barWidth, barHeight, glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
        m_ui.quad(barX, barY, barWidth * left, barHeight,
                  glm::vec4(1.0f - left, left, 0.12f, 1.0f));
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

void Application::renderInventoryScreen()
{
    // Interface pixels, the same ones the HUD is drawn in, and the same
    // ones the open-source client lays its containers out in. Every
    // number below is one of its own: a panel 176 by 166, the player's
    // storage at 8 across and 84 down, the hotbar at 142, the armour
    // column at 8 and 8, the little crafting grid at 88 and 26 with its
    // result at 144 and 36. Lining up with them is what makes a
    // screenshot of this sit on top of a screenshot of that.
    m_ui.setScale(static_cast<float>(guiScale()));

    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const bool creative = m_player.creative();

    m_slotBoxes.clear();

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.62f));

    // Creative item tabs show a palette; the inventory tab (and all of
    // survival) shows the character, armour, crafting and storage.
    const bool showPalette = creative && m_creativeTab != INVENTORY_TAB;

    // A chest's panel is a row taller than the rest, and the creative
    // palette's is shorter: it has no storage rows under it, only the
    // hotbar. Both are the client's own figures.
    const float PANEL_W = 176.0f;
    const float PANEL_H = showPalette ? 136.0f : (m_chestOpen ? 168.0f : 166.0f);

    const float tabHeight = creative ? 16.0f : 0.0f;
    const float panelX = std::floor((w - PANEL_W) * 0.5f);
    float panelY = std::floor((h - PANEL_H) * 0.5f);
    panelY = std::max(panelY, tabHeight + 2.0f);

    // Where the player's own rows go, which is the same in every
    // container the game has: a chest pushes them down a row, the
    // palette has none at all.
    // A chest's own rows push the player's down, but not by a whole
    // row: the client works it out as 103 + (rows - 4) * 18, which for
    // the three rows a chest has comes to 85, and the hotbar to 143.
    const float storageY = panelY + (m_chestOpen ? 85.0f : 84.0f);
    const float hotbarY = panelY + (showPalette ? 112.0f
                                                : (m_chestOpen ? 143.0f : 142.0f));

    // One slot, at the item's own top-left the way the client counts
    // them: the well is the eighteen-pixel box drawn round it.
    auto well = [this](float x, float y) {
        guiWell(x - 1.0f, y - 1.0f, PITCH, PITCH);
    };
    auto slotBox = [this](SlotBox::Kind kind, int index, float x, float y) {
        m_slotBoxes.push_back({ kind, index, x - 1.0f, y - 1.0f, PITCH });
    };
    auto drawSlot = [&](int index, float x, float y) {
        well(x, y);
        const ItemStack& stack = m_inventory.slot(index);
        drawItem(x, y, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
        slotBox(SlotBox::Kind::Slot, index, x, y);
    };

    // ------------------------------------------------------------ tabs ---
    if (creative)
    {
        const float tabWidth = std::min(20.0f, (PANEL_W - 4.0f) / TAB_COUNT);
        for (int i = 0; i < TAB_COUNT; ++i)
        {
            const float tabX = panelX + 2.0f + i * tabWidth;
            const float tabY = panelY - tabHeight;
            const bool active = (i == m_creativeTab);

            // The selected tab is raised and the same colour as the
            // panel, so it joins onto it; the rest sit back and darker.
            if (active)
                bevel(tabX, tabY, tabWidth - 1.0f, tabHeight + 3.0f,
                      GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 1.0f);
            else
                bevel(tabX, tabY + 2.0f, tabWidth - 1.0f, tabHeight,
                      glm::vec4(0.569f, 0.569f, 0.569f, 1.0f),
                      glm::vec4(0.69f, 0.69f, 0.69f, 1.0f), GUI_PANEL_DARK, 1.0f);

            drawItem(tabX + 2.0f, tabY + (active ? 1.0f : 3.0f), tabWidth - 5.0f,
                     CREATIVE_TABS[i].icon, 0,
                     active ? glm::vec4(1.0f) : glm::vec4(0.72f, 0.72f, 0.72f, 0.9f));

            m_slotBoxes.push_back({ SlotBox::Kind::Tab, i, tabX, tabY, tabHeight + 3.0f,
                                    tabWidth });
        }
    }

    guiPanel(panelX, panelY, PANEL_W, PANEL_H);

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
    const bool bench = m_inventory.craftSize() >= 3;
    const bool plainInventory = !creative && !m_chestOpen && !m_crafterOpen &&
                                !m_furnaceOpen && !bench && !m_tradeOpen &&
                                !m_enchantOpen && !m_brewOpen && !m_anvilOpen;

    // Each screen's heading goes where the client puts that screen's
    // heading: a furnace's and a brewing stand's are centred, an
    // anvil's starts at 60, an enchanting table's at 12, and the rest
    // sit in the corner at 8.
    if (!plainInventory)
    {
        const char* title = creative ? CREATIVE_TABS[m_creativeTab].name
                                     : (m_chestOpen ? "Chest"
                                       : (m_crafterOpen ? "Crafter"
                                       : (m_furnaceOpen ? "Furnace"
                                       : (m_brewOpen ? "Brewing"
                                       : (m_anvilOpen ? "Repair & Name"
                                       : (m_enchantOpen ? "Enchant"
                                       : (m_tradeOpen ? Trading::name(m_tradeWith)
                                       : (bench ? "Crafting" : "Inventory"))))))));

        const float titleX = (m_furnaceOpen || m_brewOpen)
                                 ? panelX + PANEL_W * 0.5f
                                       - UIRenderer::textWidth(title, 1.0f) * 0.5f
                                 : panelX + (m_anvilOpen ? 60.0f
                                                         : (m_enchantOpen ? 12.0f : 8.0f));
        m_ui.text(title, titleX, panelY + (m_enchantOpen ? 5.0f : 6.0f), 1.0f, GUI_TEXT);
    }

    // The word over the player's own rows. An anvil is the one screen
    // the client leaves it off, so that its price has somewhere to go.
    if (!showPalette && !m_anvilOpen)
        m_ui.text("Inventory", panelX + 8.0f, panelY + PANEL_H - 94.0f, 1.0f, GUI_TEXT);

    // A line of their own for the two screens that have something to
    // say and nowhere to say it: the shelf count and what is in the
    // bottle. Right-aligned in the heading row, clear of everything.
    auto noteOnTheRight = [&](const std::string& text) {
        m_ui.text(text, panelX + PANEL_W - 8.0f - UIRenderer::textWidth(text, 1.0f),
                  panelY + 6.0f, 1.0f, TEXT_DIM);
    };

    // --------------------------------------------- the open chest ---
    // Three rows of nine, eighteen down from the top of the panel,
    // directly above the player's own rows.
    if (m_chestOpen && !showPalette)
    {
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < PALETTE_COLUMNS; ++column)
                drawSlot(Inventory::CHEST_FIRST + row * PALETTE_COLUMNS + column,
                         panelX + 8.0f + column * PITCH, panelY + 18.0f + row * PITCH);
    }

    // ------------------------------------------ the open crafter ---
    // Three by three where a workbench puts its grid. There is no
    // result slot: a crafter is worked by a pulse rather than by hand,
    // and a slot that only ever looked back at you would be a lie.
    if (m_crafterOpen && !showPalette)
    {
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
                drawSlot(Inventory::CHEST_FIRST + row * 3 + column,
                         panelX + 30.0f + column * PITCH, panelY + 17.0f + row * PITCH);

        m_ui.text("Power it to craft", panelX + 95.0f, panelY + 38.0f, 1.0f, GUI_TEXT);
    }

    // ------------------------------- armour, body preview, crafting ---
    if (!showPalette && !m_chestOpen && !m_crafterOpen)
    {
        // The armour column and the character are the player's own
        // screen and nothing else: the client draws neither on a
        // workbench, a furnace, an anvil or a trade. They used to be
        // drawn on all of them, which is how a furnace came to have a
        // man standing in the middle of its fire.
        const bool otherBusiness = m_tradeOpen || m_enchantOpen || m_brewOpen ||
                                   m_anvilOpen || m_furnaceOpen || bench;

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
                drawItem(x, y, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
            }

            slotBox(SlotBox::Kind::Slot, index, x, y);
        }

        // The character, in the recess the client leaves for it: a box
        // from 26 and 8 across to 76 and 78, with the figure standing
        // on 75.
        if (!otherBusiness)
        {
            guiWell(panelX + 25.0f, panelY + 7.0f, 52.0f, 72.0f);
            const float unit = 51.0f / 32.0f;   // the doll is 32 skin pixels tall
            const float dollWidth = (activeSkin().armWidth() * 2.0f + 8.0f) * unit;
            drawPlayerDoll(activeSkin(), panelX + 51.0f - dollWidth * 0.5f,
                           panelY + 75.0f - 51.0f, unit);
        }

        // Trading: the villager's deals, one to a row. Each shows what
        // it wants on the left and what it gives on the right, drawn as
        // the items themselves rather than named, because the names are
        // long and the panel is not wide.
        if (m_tradeOpen)
        {
            const std::vector<Trading::Offer> offers = Trading::offersFor(m_tradeWith);

            const float tx = panelX + 80.0f;
            const float ty = panelY + 6.0f;

            for (size_t row = 0; row < offers.size(); ++row)
            {
                const Trading::Offer& offer = offers[row];
                const float ry = ty + 12.0f + static_cast<float>(row) * PITCH;

                const int haveWant = m_inventory.countOf(offer.want);
                const int haveAlso = (offer.also == Blocks::Air)
                                         ? 0 : m_inventory.countOf(offer.also);
                const bool canPay = m_player.creative() ||
                                    Trading::affordable(offer, haveWant, haveAlso);

                const glm::vec4 tint = canPay ? glm::vec4(1.0f)
                                              : glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);

                float cx = tx;
                well(cx, ry);
                drawItem(cx, ry, SLOT, offer.want, offer.wantCount, tint);
                cx += PITCH;

                if (offer.also != Blocks::Air)
                {
                    well(cx, ry);
                    drawItem(cx, ry, SLOT, offer.also, offer.alsoCount, tint);
                }
                cx += PITCH;

                m_ui.text("->", cx, ry + 5.0f, 1.0f, canPay ? TEXT_COLOR : TEXT_DIM);
                cx += 14.0f;

                well(cx, ry);
                drawItem(cx, ry, SLOT, offer.give, offer.giveCount, tint);

                // The whole row is the button, out to the far edge of
                // what it gives you, so a deal can be struck by
                // clicking anywhere along it rather than only on the
                // one slot that happens to be the price.
                if (canPay)
                    m_slotBoxes.push_back({ SlotBox::Kind::Trade, static_cast<int>(row),
                                            tx - 1.0f, ry - 1.0f, PITCH,
                                            (cx + SLOT) - tx + 1.0f });
            }
        }
        else

        // The enchanting table: the item at 25 and 47, three offers
        // down the right at 60 and 14, nineteen apart -- the client's
        // own numbers. Each row shows what it costs in levels and what
        // it would put on the item, which is the one place this is
        // kinder than Minecraft, where the rows are written in a rune
        // alphabet nobody can read.
        if (m_enchantOpen)
        {
            const float ex = panelX + 25.0f;
            const float ey = panelY + 47.0f;

            well(ex, ey);
            const ItemStack& subject = m_inventory.slot(Inventory::ENCHANT_SLOT);
            drawItem(ex, ey, SLOT, subject.id, subject.count, glm::vec4(1.0f),
                     subject.damage);
            slotBox(SlotBox::Kind::Slot, Inventory::ENCHANT_SLOT, ex, ey);

            const float rowX = panelX + 60.0f;
            const float rowWidth = 108.0f;
            const float rowHeight = 19.0f;
            const int level = Experience::levelFor(m_player.experience);

            for (int row = 0; row < 3; ++row)
            {
                const float ry = panelY + 14.0f + row * rowHeight;
                const Enchanting::Offer offer =
                    Enchanting::offerFor(row, m_enchantShelves, subject.id, m_enchantRoll);

                // Affordable in creative whatever the price, as in
                // Minecraft.
                const bool affordable = offer.valid() &&
                                        (m_player.creative() || level >= offer.cost);

                guiWell(rowX, ry, rowWidth, rowHeight - 1.0f);
                if (offer.valid())
                    m_slotBoxes.push_back({ SlotBox::Kind::Enchant, row, rowX, ry,
                                            rowHeight - 1.0f, rowWidth });

                if (!offer.valid())
                {
                    m_ui.text("-", rowX + 5.0f, ry + 5.0f, 1.0f, TEXT_DIM);
                    continue;
                }

                // The price in the green the experience bar uses, dimmed
                // when it is out of reach.
                const glm::vec4 priceColour = affordable
                                                  ? glm::vec4(0.49f, 0.98f, 0.24f, 1.0f)
                                                  : glm::vec4(0.55f, 0.30f, 0.30f, 1.0f);
                const std::string price = std::to_string(offer.cost);
                m_ui.text(price, rowX + 4.0f, ry + 5.0f, 1.0f, priceColour);

                std::string what = Enchanting::name(offer.kind);
                what += " ";
                what += std::to_string(offer.level);
                m_ui.text(what, rowX + 18.0f, ry + 5.0f, 1.0f,
                          affordable ? TEXT_COLOR : TEXT_DIM);
            }

            noteOnTheRight(std::to_string(m_enchantShelves) + " shelves");
        }
        else

        // The brewing stand, at the client's own four: the ingredient
        // on top at 79 and 17, and the three bottles under it at 56,
        // 79 and 102, the outer two a notch higher than the middle.
        if (m_brewOpen)
        {
            drawSlot(Inventory::BREW_INGREDIENT, panelX + 79.0f, panelY + 17.0f);

            const float bottleX[3] = { 56.0f, 79.0f, 102.0f };
            const float bottleY[3] = { 46.0f, 53.0f, 46.0f };
            for (int i = 0; i < Inventory::BREW_BOTTLES; ++i)
                drawSlot(Inventory::BREW_BOTTLE_FIRST + i,
                         panelX + bottleX[i], panelY + bottleY[i]);

            // The bubbles run down between the ingredient and the
            // bottles, which is where the client draws them.
            const float barX = panelX + 86.0f;
            const float barTop = panelY + 39.0f;
            const float barHeight = 10.0f;
            const float along = std::min(1.0f, m_brewProgress / Brewing::BREW_SECONDS);
            guiWell(barX, barTop, 4.0f, barHeight);
            if (along > 0.0f)
                m_ui.quad(barX, barTop + barHeight * (1.0f - along), 4.0f, barHeight * along,
                          glm::vec4(0.62f, 0.38f, 0.86f, 1.0f));

            // What the bottle under the cursor actually holds, since
            // every potion is the same item id with a different number.
            // Above the panel rather than on it. Minecraft says what
            // is in a bottle in a tooltip and there is no tooltip
            // here, and every clear strip inside a brewing stand's
            // panel is shorter than the longest name that goes in it.
            const ItemStack& first = m_inventory.slot(Inventory::BREW_BOTTLE_FIRST);
            if (first.id == Items::Potion)
                m_ui.textWithShadow(Brewing::name(Brewing::fromDamage(first.damage)),
                                    panelX + 8.0f, panelY - 12.0f, 1.0f, TEXT_COLOR);
        }
        else

        // The anvil: two in at 27 and 76, one out at 134, all on 47.
        // Which way round the two go matters, so the plus between them
        // is not decoration.
        if (m_anvilOpen)
        {
            const int slots[3] = { Inventory::ANVIL_LEFT, Inventory::ANVIL_RIGHT,
                                   Inventory::ANVIL_RESULT };
            const float xs[3] = { 27.0f, 76.0f, 134.0f };
            const float ay = panelY + 47.0f;

            for (int i = 0; i < 3; ++i)
            {
                const float x = panelX + xs[i];
                well(x, ay);
                const ItemStack& stack = m_inventory.slot(slots[i]);
                drawItem(x, ay, SLOT, stack.id, stack.count, glm::vec4(1.0f), stack.damage);

                // The result is taken, never filled, so it gets its own
                // click rather than the ordinary slot one.
                slotBox(i == 2 ? SlotBox::Kind::AnvilTake : SlotBox::Kind::Slot,
                        slots[i], x, ay);
            }

            m_ui.text("+", panelX + 60.0f, ay + 5.0f, 1.0f, TEXT_DIM);
            m_ui.text("->", panelX + 103.0f, ay + 5.0f, 1.0f, TEXT_DIM);

            // The price, right-aligned on 67 with a shadow behind it,
            // which is where and how the client draws an anvil's.
            const Anvil::Result made = anvilResult();
            const int level = Experience::levelFor(m_player.experience);

            std::string price;
            glm::vec4 colour(0.502f, 1.0f, 0.125f, 1.0f);     // the client's green

            if (made.tooExpensive)
            {
                price = "Too expensive!";
                colour = glm::vec4(1.0f, 0.439f, 0.376f, 1.0f);   // and its red
            }
            else if (made.valid)
            {
                price = "Repair cost: " + std::to_string(made.levelCost);
                if (!m_player.creative() && level < made.levelCost)
                    colour = glm::vec4(1.0f, 0.439f, 0.376f, 1.0f);
            }

            if (!price.empty())
            {
                const float px = panelX + PANEL_W - 8.0f
                                 - UIRenderer::textWidth(price, 1.0f);
                const float py = panelY + 67.0f;
                m_ui.text(price, px + 1.0f, py + 1.0f, 1.0f,
                          glm::vec4(colour.r * 0.25f, colour.g * 0.25f,
                                    colour.b * 0.25f, 1.0f));
                m_ui.text(price, px, py, 1.0f, colour);
            }
        }
        else

        // A furnace: the ore at 56 and 17, the fuel under it at 53, the
        // fire between them at 36 and the result off to the right at
        // 116 and 35. The client's own four, and the shape everybody
        // recognises.
        if (m_furnaceOpen)
        {
            const Furnaces::State& state = m_furnaces.at(m_furnaceBlock);

            Furnace gauge;
            gauge.burnLeft = state.burnLeft;
            gauge.burnTotal = state.burnTotal;
            gauge.cooked = state.cooked;

            drawSlot(Inventory::FURNACE_INPUT, panelX + 56.0f, panelY + 17.0f);
            drawSlot(Inventory::FURNACE_FUEL, panelX + 56.0f, panelY + 53.0f);

            // The fire between them, burning down as the fuel goes:
            // fourteen square at 56 and 36.
            const float flameX = panelX + 57.0f;
            const float flameY = panelY + 36.0f;
            const float flameSize = 14.0f;
            const float alight = gauge.burnFraction();

            m_ui.quad(flameX, flameY, flameSize, flameSize, glm::vec4(0.0f, 0.0f, 0.0f, 0.25f));
            if (alight > 0.0f)
                m_ui.quad(flameX, flameY + flameSize * (1.0f - alight),
                          flameSize, flameSize * alight,
                          glm::vec4(1.0f, 0.65f, 0.15f, 1.0f));

            // The arrow, filling as the item cooks: twenty-four by
            // sixteen at 79 and 34.
            const float arrowX = panelX + 79.0f;
            const float arrowY = panelY + 38.0f;
            const float arrowWidth = 24.0f;
            const float arrowHeight = 8.0f;

            m_ui.quad(arrowX, arrowY, arrowWidth, arrowHeight, glm::vec4(0.0f, 0.0f, 0.0f, 0.25f));
            m_ui.quad(arrowX, arrowY, arrowWidth * gauge.cookFraction(), arrowHeight,
                      glm::vec4(0.92f, 0.92f, 0.92f, 1.0f));

            drawSlot(Inventory::FURNACE_OUTPUT, panelX + 116.0f, panelY + 35.0f);
        }
        else
        {
            // Crafting. A workbench puts its three by three at 30 and
            // 17 with the result at 124 and 35; the player's own two by
            // two goes at 88 and 26 with the result at 144 and 36.
            const int size = m_inventory.craftSize();

            const float gridX = panelX + (bench ? 30.0f : 88.0f);
            const float gridY = panelY + (bench ? 17.0f : 26.0f);

            for (int row = 0; row < size; ++row)
                for (int column = 0; column < size; ++column)
                    // The grid is addressed as 3x3 even when only 2x2 shows.
                    drawSlot(Inventory::CRAFT_FIRST + row * 3 + column,
                             gridX + column * PITCH, gridY + row * PITCH);

            const float resultX = panelX + (bench ? 124.0f : 144.0f);
            const float resultY = panelY + (bench ? 35.0f : 36.0f);

            m_ui.text("->", resultX - 20.0f, resultY + 5.0f, 1.0f, GUI_TEXT);
            drawSlot(Inventory::CRAFT_RESULT, resultX, resultY);

            // "Crafting" sits over the little grid rather than along
            // the top, which is where the client puts it.
            if (!bench)
                m_ui.text("Crafting", panelX + 86.0f, panelY + 16.0f, 1.0f, GUI_TEXT);
        }
    }

    // ------------------------------------------------------- main grid ---
    const StackId* paletteItems = nullptr;
    int paletteCount = 0;
    if (showPalette) tabContents(m_creativeTab, paletteItems, paletteCount);

    // A tab with more in it than the grid holds scrolls on the wheel.
    // Without this the tools, and most of Everything, were simply not
    // reachable: the grid drew the first forty-five and stopped.
    const int paletteRows = showPalette
        ? (paletteCount + PALETTE_COLUMNS - 1) / PALETTE_COLUMNS : 0;
    const int maxRow = std::max(0, paletteRows - PALETTE_ROWS);
    m_paletteRow = std::clamp(m_paletteRow, 0, maxRow);

    const int gridRows = showPalette ? PALETTE_ROWS : 3;

    for (int row = 0; row < gridRows; ++row)
    {
        for (int column = 0; column < PALETTE_COLUMNS; ++column)
        {
            const float x = panelX + 8.0f + column * PITCH;
            const float y = showPalette ? panelY + 18.0f + row * PITCH
                                        : storageY + row * PITCH;

            if (showPalette)
            {
                well(x, y);
                const int cell = (row + m_paletteRow) * PALETTE_COLUMNS + column;
                if (cell >= paletteCount) continue;
                drawItem(x, y, SLOT, paletteItems[cell], 0, glm::vec4(1.0f));
                m_slotBoxes.push_back({ SlotBox::Kind::Palette, paletteItems[cell],
                                        x - 1.0f, y - 1.0f, PITCH });
            }
            else
            {
                // Storage runs 9..35, directly above the hotbar.
                drawSlot(Inventory::HOTBAR_SLOTS + row * PALETTE_COLUMNS + column, x, y);
            }
        }
    }

    // --------------------------------- hotbar, always the bottom row ---
    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
    {
        const float x = panelX + 8.0f + i * PITCH;

        if (i == m_inventory.selectedSlot())
            m_ui.quad(x - 2.0f, hotbarY - 2.0f, PITCH + 2.0f, PITCH + 2.0f, SELECTED_COLOR);

        drawSlot(i, x, hotbarY);
    }

    // ------------------------- whatever is on the cursor, drawn last ---
    const ItemStack& held = m_inventory.cursor();
    if (!held.empty())
    {
        const float x = static_cast<float>(m_input.mouseX()) / m_ui.scale() - SLOT * 0.5f;
        const float y = static_cast<float>(m_input.mouseY()) / m_ui.scale() - SLOT * 0.5f;
        drawItem(x, y, SLOT, held.id, held.count, glm::vec4(1.0f), held.damage);
    }

    m_ui.setScale(1.0f);
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
            acceptTrade(box->index);
            break;

        case SlotBox::Kind::AnvilTake:
            takeAnvilResult();
            break;

        case SlotBox::Kind::Slot:
            if (shiftHeld) m_inventory.quickMove(box->index);
            else if (rightButton) m_inventory.rightClick(box->index);
            else m_inventory.leftClick(box->index);
            // Any click can change what the grid adds up to, and working
            // out which ones can is not worth the bug it would cause.
            m_inventory.refreshCraftResult();
            refreshAnvilResult();
            m_audio.play(Sound::Click, 0.35f);
            break;
    }
}

// Closing the screen must not swallow anything: the cursor stack and the
// crafting grid both go back to the player, and whatever will not fit is
// dropped at their feet.
void Application::closeInventory()
{
    stowChest();
    stowCrafter();
    stowFurnace();
    stowEnchantingTable();
    stowAnvil();
    stowBrewingStand();
    m_tradeOpen = false;
    returnCursorToWorld();
    m_inventoryOpen = false;
    m_benchOpen = false;
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
    m_tradeSeed = villager.birthSeed();
    // A villager that lives in a village keeps that building's trade.
    // One met out in the country has no building to take it from, so it
    // falls back to the one its birth seed draws -- which is what every
    // villager did before there were villages.
    m_tradeWith = (villager.professionSlot() >= 0)
                      ? static_cast<Trading::Profession>(villager.professionSlot())
                      : Trading::professionFor(villager.birthSeed());
    m_inventoryOpen = true;
    setMouseCaptured(false);
}

void Application::acceptTrade(int row)
{
    if (!m_tradeOpen) return;

    const std::vector<Trading::Offer> offers = Trading::offersFor(m_tradeWith);
    if (row < 0 || row >= static_cast<int>(offers.size())) return;

    const Trading::Offer& offer = offers[static_cast<size_t>(row)];

    const int haveWant = m_inventory.countOf(offer.want);
    const int haveAlso = (offer.also == Blocks::Air) ? 0 : m_inventory.countOf(offer.also);
    if (!m_player.creative() && !Trading::affordable(offer, haveWant, haveAlso)) return;

    // Taken before it is given. The other way round, a trade that
    // filled the last free slot would be paid for out of a pack that no
    // longer had room for what it was paying with.
    if (!m_player.creative())
    {
        for (int i = 0; i < offer.wantCount; ++i) m_inventory.take(offer.want);
        for (int i = 0; i < offer.alsoCount; ++i) m_inventory.take(offer.also);
    }

    const int leftOver = m_inventory.add(offer.give, offer.giveCount);
    if (leftOver > 0)
        m_drops.spawn(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f),
                      offer.give, leftOver);

    m_audio.play(Sound::MobVillagerSay, 0.7f, 1.3f);
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

    ItemStack& subject = m_inventory.slot(Inventory::ENCHANT_SLOT);
    if (!subject.empty())
    {
        // Back into the pack if it fits, and onto the floor if it does
        // not: an item left here would simply vanish.
        const int leftOver = m_inventory.addStack(subject);
        if (leftOver > 0)
            m_drops.spawnStack(m_player.position + glm::vec3(0.0f, 0.6f, 0.0f), subject);
        subject.clear();
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
}

void Application::tickBrewing(float deltaTime)
{
    if (!m_brewOpen) { m_brewProgress = 0.0f; return; }

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

    const int level = Experience::levelFor(m_player.experience);
    if (!m_player.creative() && level < offer.cost) return;

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
        const int after = std::max(0, level - offer.cost);
        m_player.experience =
            Experience::totalForLevel(after) +
            static_cast<int>(along * static_cast<float>(Experience::toNextLevel(after)));
        m_player.experienceOwedHundredths = 0;
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
