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
        Blocks::Planks, Blocks::Log, Blocks::BirchLog, Blocks::Sandstone,
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
        Blocks::Bookshelf, Blocks::EnchantingTable, Blocks::EndPortalFrame,
        Items::Stick, Items::Coal, Items::Bow, Items::Arrow, Items::FlintAndSteel,
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
        { "BUILDING BLOCKS",  Blocks::Bricks,     TAB_BUILDING, static_cast<int>(std::size(TAB_BUILDING)) },
        { "NATURE",           Blocks::Grass,      TAB_NATURE,   static_cast<int>(std::size(TAB_NATURE)) },
        { "ORES AND STONE",   Blocks::DiamondOre, TAB_ORES,     static_cast<int>(std::size(TAB_ORES)) },
        { "LIGHT AND LIQUID", Blocks::Glowstone,  TAB_MISC,     static_cast<int>(std::size(TAB_MISC)) },
        { "ITEMS",            Items::Wheat,       TAB_ITEMS,    static_cast<int>(std::size(TAB_ITEMS)) },
        { "TOOLS",            Items::IronPickaxe, TAB_TOOLS,    static_cast<int>(std::size(TAB_TOOLS)) },
        { "ARMOUR",           Items::DiamondChestplate, TAB_ARMOUR, static_cast<int>(std::size(TAB_ARMOUR)) },
        { "EVERYTHING",       Blocks::Cobblestone,nullptr,      0 },
        { "YOUR INVENTORY",   Blocks::Wool,       nullptr,      0 },  // the player's own slots
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

void Application::drawItem(float x, float y, float size, StackId id, int count,
                           const glm::vec4& tint, int damage)
{
    if (id == Blocks::Air) return;

    // Minecraft leaves a clear margin around an item in its slot; at
    // 11% the blocks looked wedged in.
    const float inset = size * 0.19f;
    const TileUV uv = tileUV(atlasTileFor(id));
    m_ui.texturedQuad(m_atlas.textureId(), x + inset, y + inset,
                      size - inset * 2.0f, size - inset * 2.0f,
                      uv.u0, uv.vTop, uv.u1, uv.vBottom, tint);

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
        const std::string text = std::to_string(count);
        const float scale = 1.8f;
        m_ui.textWithShadow(text,
                            x + size - UIRenderer::textWidth(text, scale) - 4.0f,
                            y + size - UIRenderer::textHeight(scale) - 3.0f,
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
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());
    const bool creative = m_player.creative();

    m_slotBoxes.clear();

    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.62f));

    const float step = SLOT_SIZE + SLOT_GAP;
    const float gridWidth = PALETTE_COLUMNS * step + SLOT_GAP;
    const float panelX = w * 0.5f - gridWidth * 0.5f;

    // Creative item tabs show a palette; the inventory tab (and all of
    // survival) shows the character, armour, crafting and storage.
    const bool showPalette = creative && m_creativeTab != INVENTORY_TAB;
    const int gridRows = showPalette ? PALETTE_ROWS : 3;

    const float titleHeight = 34.0f;
    // The top of the panel holds the armour and the character, or, with
    // a chest open, the chest's own three rows -- which are one row
    // shorter, so the panel closes up rather than leaving a gap.
    const float topRows = m_chestOpen ? 3.0f : static_cast<float>(Inventory::ARMOR_SLOTS);
    const float topHeight = showPalette ? 0.0f : topRows * step + SLOT_GAP;
    const float gridHeight = gridRows * step + SLOT_GAP;
    const float hotbarHeight = step + SLOT_GAP;
    const float panelHeight = titleHeight + topHeight + gridHeight + 18.0f + hotbarHeight;
    const float tabHeight = creative ? SLOT_SIZE + 6.0f : 0.0f;

    float panelY = h * 0.5f - (panelHeight + tabHeight) * 0.5f + tabHeight;
    panelY = std::max(panelY, tabHeight + 10.0f);

    // ------------------------------------------------------------ tabs ---
    if (creative)
    {
        const float tabWidth = std::min(SLOT_SIZE + 4.0f, (gridWidth - 8.0f) / TAB_COUNT);
        for (int i = 0; i < TAB_COUNT; ++i)
        {
            const float tabX = panelX + 4.0f + i * (tabWidth + 2.0f);
            const float tabY = panelY - tabHeight;
            const bool active = (i == m_creativeTab);

            // The active tab is lifted and lit so it reads as part of the
            // panel below it rather than a button floating above it.
            // The selected tab is raised and the same colour as the panel,
            // so it joins onto it; the rest sit back and darker.
            if (active)
                bevel(tabX, tabY, tabWidth, tabHeight + 6.0f,
                      GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 3.0f);
            else
                bevel(tabX, tabY + 4.0f, tabWidth, tabHeight,
                      glm::vec4(0.569f, 0.569f, 0.569f, 1.0f),
                      glm::vec4(0.69f, 0.69f, 0.69f, 1.0f), GUI_PANEL_DARK, 2.0f);
            drawItem(tabX + 2.0f, tabY + (active ? 2.0f : 6.0f), tabWidth - 4.0f,
                     CREATIVE_TABS[i].icon, 0,
                     active ? glm::vec4(1.0f) : glm::vec4(0.72f, 0.72f, 0.72f, 0.9f));

            m_slotBoxes.push_back({ SlotBox::Kind::Tab, i, tabX, tabY, tabWidth });
        }
    }

    guiPanel(panelX, panelY, gridWidth, panelHeight);

    // Dark text with no shadow: on a light panel a drop shadow just looks
    // like a smudge, and vanilla does not use one here either.
    const char* title = creative ? CREATIVE_TABS[m_creativeTab].name
                                 : (m_chestOpen ? "CHEST"
                                   : (m_furnaceOpen ? "FURNACE"
                                   : (m_benchOpen ? "CRAFTING" : "INVENTORY")));
    m_ui.text(title, panelX + 10.0f, panelY + 10.0f, 2.2f, GUI_TEXT);

    // --------------------------------------------- the open chest ---
    // A chest is the one container too big for a corner, so it takes the
    // whole top of the panel: the same nine across as the storage below
    // it, which is what makes the two read as one tall grid.
    if (m_chestOpen && !showPalette)
    {
        const float topY = panelY + titleHeight;

        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < PALETTE_COLUMNS; ++column)
            {
                const float x = panelX + SLOT_GAP + column * step;
                const float y = topY + SLOT_GAP + row * step;
                const int index = Inventory::CHEST_FIRST + row * PALETTE_COLUMNS + column;

                guiWell(x, y, SLOT_SIZE, SLOT_SIZE);
                const ItemStack& stack = m_inventory.slot(index);
                drawItem(x, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
                m_slotBoxes.push_back({ SlotBox::Kind::Slot, index, x, y, SLOT_SIZE });
            }
    }

    // ------------------------------- armour, body preview, crafting ---
    if (!showPalette && !m_chestOpen)
    {
        const float topY = panelY + titleHeight;

        // Armour down the left edge.
        for (int i = 0; i < Inventory::ARMOR_SLOTS; ++i)
        {
            const float x = panelX + SLOT_GAP;
            const float y = topY + SLOT_GAP + i * step;
            const int index = Inventory::ARMOR_FIRST + i;

            guiWell(x, y, SLOT_SIZE, SLOT_SIZE);

            const ItemStack& stack = m_inventory.slot(index);
            if (stack.empty())
            {
                // A faint silhouette of the piece that belongs here, the
                // way Minecraft marks its empty armour slots.
                const float inset = SLOT_SIZE * 0.16f;
                m_ui.texturedQuad(m_gui.texture(ARMOR_GHOSTS[i]),
                                  x + inset, y + inset,
                                  SLOT_SIZE - inset * 2.0f, SLOT_SIZE - inset * 2.0f,
                                  0.0f, 0.0f, 1.0f, 1.0f, glm::vec4(1.0f));
            }
            else
            {
                drawItem(x, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
            }

            m_slotBoxes.push_back({ SlotBox::Kind::Slot, index, x, y, SLOT_SIZE });
        }

        // The character, beside the armour column.
        const float dollHeight = Inventory::ARMOR_SLOTS * step - SLOT_GAP * 2.0f;
        const float unit = dollHeight / 32.0f;   // the doll is 32 skin pixels tall
        const float dollWidth = (activeSkin().armWidth() * 2.0f + 8.0f) * unit;
        guiWell(panelX + SLOT_GAP + step, topY + SLOT_GAP,
                dollWidth + 28.0f, dollHeight + SLOT_GAP * 2.0f);
        drawPlayerDoll(activeSkin(), panelX + SLOT_GAP + step + 14.0f,
                       topY + SLOT_GAP * 2.0f, unit);

        // Trading: the villager's deals, one to a row. Each shows what
        // it wants on the left and what it gives on the right, drawn as
        // the items themselves rather than named, because the names are
        // long and the panel is not wide.
        if (m_tradeOpen)
        {
            const std::vector<Trading::Offer> offers = Trading::offersFor(m_tradeWith);

            const float tx = panelX + gridWidth * 0.36f;
            const float ty = topY + SLOT_GAP;

            // Four rows have to fit beside the player doll, and at full
            // slot size the last one ran down into the storage grid, so
            // the trade slots are drawn a little smaller than the rest.
            const float cell = SLOT_SIZE * 0.78f;
            const float rowHeight = cell + 3.0f;

            m_ui.text(Trading::name(m_tradeWith), tx, ty, 1.6f, TEXT_COLOR);

            for (size_t row = 0; row < offers.size(); ++row)
            {
                const Trading::Offer& offer = offers[row];
                const float ry = ty + 18.0f + static_cast<float>(row) * rowHeight;

                const int haveWant = m_inventory.countOf(offer.want);
                const int haveAlso = (offer.also == Blocks::Air)
                                         ? 0 : m_inventory.countOf(offer.also);
                const bool canPay = m_player.creative() ||
                                    Trading::affordable(offer, haveWant, haveAlso);

                const glm::vec4 tint = canPay ? glm::vec4(1.0f)
                                              : glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);

                float cx = tx;
                guiWell(cx, ry, cell, cell);
                drawItem(cx, ry, cell, offer.want, offer.wantCount, tint);
                cx += cell + 3.0f;

                if (offer.also != Blocks::Air)
                {
                    guiWell(cx, ry, cell, cell);
                    drawItem(cx, ry, cell, offer.also, offer.alsoCount, tint);
                }
                cx += cell + 3.0f;

                m_ui.text("->", cx, ry + cell * 0.28f, 1.6f,
                          canPay ? TEXT_COLOR : TEXT_DIM);
                cx += 22.0f;

                guiWell(cx, ry, cell, cell);
                drawItem(cx, ry, cell, offer.give, offer.giveCount, tint);

                // The whole row is the button, out to the far edge of
                // what it gives you, so a deal can be struck by
                // clicking anywhere along it rather than only on the
                // one slot that happens to be the price.
                if (canPay)
                {
                    const float rowWidth = (cx + cell) - tx;
                    m_slotBoxes.push_back({ SlotBox::Kind::Trade, static_cast<int>(row),
                                            tx, ry, cell, rowWidth });
                }
            }
        }
        else

        // The enchanting table: the item on the left, three offers down
        // the right. Each row shows what it costs in levels and, once
        // you could afford it, what it would put on the item -- which
        // is the one place this is kinder than Minecraft, where the
        // rows are written in a rune alphabet nobody can read.
        if (m_enchantOpen)
        {
            // Over on the right, clear of the player doll and the
            // armour slots down the left-hand side: the first attempt
            // put the item being enchanted straight on top of them.
            const float ex = panelX + gridWidth * 0.42f;
            const float ey = topY + SLOT_GAP + step * 0.5f;

            guiWell(ex, ey, SLOT_SIZE, SLOT_SIZE);
            const ItemStack& subject = m_inventory.slot(Inventory::ENCHANT_SLOT);
            drawItem(ex, ey, SLOT_SIZE, subject.id, subject.count, glm::vec4(1.0f),
                     subject.damage);
            m_slotBoxes.push_back({ SlotBox::Kind::Slot, Inventory::ENCHANT_SLOT,
                                    ex, ey, SLOT_SIZE });

            const float rowX = ex + step + 24.0f;
            const float rowWidth = gridWidth - (rowX - panelX) - SLOT_GAP * 2.0f;
            const float rowHeight = 30.0f;
            const int level = Experience::levelFor(m_player.experience);

            for (int row = 0; row < 3; ++row)
            {
                const float ry = ey + row * (rowHeight + 6.0f);
                const Enchanting::Offer offer =
                    Enchanting::offerFor(row, m_enchantShelves, subject.id, m_enchantRoll);

                // Affordable in creative whatever the price, as in
                // Minecraft.
                const bool affordable = offer.valid() &&
                                        (m_player.creative() || level >= offer.cost);

                guiWell(rowX, ry, rowWidth, rowHeight);
                if (offer.valid())
                    m_slotBoxes.push_back({ SlotBox::Kind::Enchant, row, rowX, ry,
                                            rowHeight, rowWidth });

                if (!offer.valid())
                {
                    m_ui.text("-", rowX + 10.0f, ry + 8.0f, 1.8f, TEXT_DIM);
                    continue;
                }

                // The price in the green the experience bar uses, dimmed
                // when it is out of reach.
                const glm::vec4 priceColour = affordable
                                                  ? glm::vec4(0.49f, 0.98f, 0.24f, 1.0f)
                                                  : glm::vec4(0.55f, 0.30f, 0.30f, 1.0f);
                const std::string price = std::to_string(offer.cost);
                m_ui.text(price, rowX + 8.0f, ry + 8.0f, 2.0f, priceColour);

                std::string what = Enchanting::name(offer.kind);
                what += " ";
                what += std::to_string(offer.level);
                m_ui.text(what, rowX + 42.0f, ry + 9.0f, 1.6f,
                          affordable ? TEXT_COLOR : TEXT_DIM);
            }

            // Under the offers rather than beside the item: to the left
            // of the slot it ran straight across the player doll.
            const std::string shelves = std::to_string(m_enchantShelves) + " shelves";
            m_ui.text(shelves, rowX + 2.0f, ey + 3.0f * (rowHeight + 6.0f) + 4.0f,
                      1.5f, TEXT_DIM);
        }
        else

        // A furnace takes the same corner the crafting grid does: ore on
        // top, fuel below it, the fire between them and the result off
        // to the right, which is the shape Minecraft uses.
        if (m_furnaceOpen)
        {
            const Furnaces::State& state = m_furnaces.at(m_furnaceBlock);

            Furnace gauge;
            gauge.burnLeft = state.burnLeft;
            gauge.burnTotal = state.burnTotal;
            gauge.cooked = state.cooked;

            const float resultWidth = step + 34.0f;
            const float fx = panelX + gridWidth - SLOT_GAP - step - resultWidth;
            const float fy = topY + SLOT_GAP + step * 0.5f;

            const int SLOTS[3] = { Inventory::FURNACE_INPUT, Inventory::FURNACE_FUEL,
                                   Inventory::FURNACE_OUTPUT };

            // Input above, fuel below.
            for (int i = 0; i < 2; ++i)
            {
                const float y = fy + i * step * 1.4f;
                guiWell(fx, y, SLOT_SIZE, SLOT_SIZE);
                const ItemStack& stack = m_inventory.slot(SLOTS[i]);
                drawItem(fx, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
                m_slotBoxes.push_back({ SlotBox::Kind::Slot, SLOTS[i], fx, y, SLOT_SIZE });
            }

            // The fire between them, burning down as the fuel goes.
            const float flameSize = SLOT_SIZE * 0.5f;
            const float flameX = fx + (SLOT_SIZE - flameSize) * 0.5f;
            const float flameY = fy + step + (step * 0.4f - flameSize) * 0.5f;
            const float alight = gauge.burnFraction();

            m_ui.quad(flameX, flameY, flameSize, flameSize, glm::vec4(0.0f, 0.0f, 0.0f, 0.25f));
            if (alight > 0.0f)
                m_ui.quad(flameX, flameY + flameSize * (1.0f - alight),
                          flameSize, flameSize * alight,
                          glm::vec4(1.0f, 0.65f, 0.15f, 1.0f));

            // The arrow, filling as the item cooks.
            const float arrowX = fx + step + 4.0f;
            const float arrowY = fy + step * 0.7f;
            const float arrowWidth = 26.0f;
            const float arrowHeight = 8.0f;

            m_ui.quad(arrowX, arrowY, arrowWidth, arrowHeight, glm::vec4(0.0f, 0.0f, 0.0f, 0.25f));
            m_ui.quad(arrowX, arrowY, arrowWidth * gauge.cookFraction(), arrowHeight,
                      glm::vec4(0.92f, 0.92f, 0.92f, 1.0f));

            const float outX = arrowX + arrowWidth + 8.0f;
            bevel(outX - 4.0f, arrowY - SLOT_SIZE * 0.5f - 4.0f, SLOT_SIZE + 8.0f, SLOT_SIZE + 8.0f,
                  GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 2.0f);
            const float outY = arrowY - SLOT_SIZE * 0.5f;
            guiWell(outX, outY, SLOT_SIZE, SLOT_SIZE);
            const ItemStack& made = m_inventory.slot(Inventory::FURNACE_OUTPUT);
            drawItem(outX, outY, SLOT_SIZE, made.id, made.count, glm::vec4(1.0f), made.damage);
            m_slotBoxes.push_back({ SlotBox::Kind::Slot, Inventory::FURNACE_OUTPUT,
                                    outX, outY, SLOT_SIZE });
        }
        else
        {

        // Crafting, over on the right: a 2x2 grid, an arrow, the result.
        const float craftSize = static_cast<float>(m_inventory.craftSize());
        const float resultWidth = step + 34.0f;
        const float craftX = panelX + gridWidth - SLOT_GAP - craftSize * step - resultWidth;
        const float craftY = topY + SLOT_GAP + step * 0.5f;

        for (int row = 0; row < m_inventory.craftSize(); ++row)
            for (int column = 0; column < m_inventory.craftSize(); ++column)
            {
                const float x = craftX + column * step;
                const float y = craftY + row * step;
                // The grid is addressed as 3x3 even when only 2x2 shows.
                const int index = Inventory::CRAFT_FIRST + row * 3 + column;

                guiWell(x, y, SLOT_SIZE, SLOT_SIZE);
                const ItemStack& stack = m_inventory.slot(index);
                drawItem(x, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
                m_slotBoxes.push_back({ SlotBox::Kind::Slot, index, x, y, SLOT_SIZE });
            }

        const float arrowX = craftX + craftSize * step + 4.0f;
        const float arrowY = craftY + (craftSize * step - SLOT_SIZE) * 0.5f;
        m_ui.text("->", arrowX, arrowY + SLOT_SIZE * 0.5f - UIRenderer::textHeight(2.4f) * 0.5f,
                  2.4f, GUI_TEXT);

        const float resultX = arrowX + 30.0f;
        // The result slot is drawn bigger, the way vanilla does.
        bevel(resultX - 4.0f, arrowY - 4.0f, SLOT_SIZE + 8.0f, SLOT_SIZE + 8.0f,
              GUI_PANEL, GUI_PANEL_LIGHT, GUI_PANEL_DARK, 2.0f);
        guiWell(resultX, arrowY, SLOT_SIZE, SLOT_SIZE);
        const ItemStack& result = m_inventory.slot(Inventory::CRAFT_RESULT);
        drawItem(resultX, arrowY, SLOT_SIZE, result.id, result.count, glm::vec4(1.0f), result.damage);
        m_slotBoxes.push_back({ SlotBox::Kind::Slot, Inventory::CRAFT_RESULT,
                                resultX, arrowY, SLOT_SIZE });
        }
    }

    // ------------------------------------------------------- main grid ---
    const float gridY = panelY + titleHeight + topHeight;
    const StackId* paletteItems = nullptr;
    int paletteCount = 0;
    if (showPalette) tabContents(m_creativeTab, paletteItems, paletteCount);

    // A tab with more in it than the grid holds scrolls on the wheel.
    // Without this the tools, and most of EVERYTHING, were simply not
    // reachable: the grid drew the first forty-five and stopped.
    const int paletteRows = showPalette
        ? (paletteCount + PALETTE_COLUMNS - 1) / PALETTE_COLUMNS : 0;
    const int maxRow = std::max(0, paletteRows - PALETTE_ROWS);
    m_paletteRow = std::clamp(m_paletteRow, 0, maxRow);

    for (int row = 0; row < gridRows; ++row)
    {
        for (int column = 0; column < PALETTE_COLUMNS; ++column)
        {
            const int cell = (row + m_paletteRow) * PALETTE_COLUMNS + column;
            const float x = panelX + SLOT_GAP + column * step;
            const float y = gridY + SLOT_GAP + row * step;

            guiWell(x, y, SLOT_SIZE, SLOT_SIZE);

            if (showPalette)
            {
                if (cell >= paletteCount) continue;
                drawItem(x, y, SLOT_SIZE, paletteItems[cell], 0, glm::vec4(1.0f));
                m_slotBoxes.push_back({ SlotBox::Kind::Palette, paletteItems[cell], x, y, SLOT_SIZE });
            }
            else
            {
                // Storage runs 9..35, directly above the hotbar. It does
                // not scroll, so it counts from the row on screen rather
                // than from the palette's offset into a longer list.
                const int index = Inventory::HOTBAR_SLOTS + row * PALETTE_COLUMNS + column;
                const ItemStack& stack = m_inventory.slot(index);
                drawItem(x, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
                m_slotBoxes.push_back({ SlotBox::Kind::Slot, index, x, y, SLOT_SIZE });
            }
        }
    }

    // --------------------------------- hotbar, always the bottom row ---
    const float hotbarY = gridY + gridHeight + 18.0f;
    for (int i = 0; i < Inventory::HOTBAR_SLOTS; ++i)
    {
        const float x = panelX + SLOT_GAP + i * step;
        const bool selected = (i == m_inventory.selectedSlot());

        if (selected)
            m_ui.quad(x - 3.0f, hotbarY - 3.0f, SLOT_SIZE + 6.0f, SLOT_SIZE + 6.0f, SELECTED_COLOR);
        guiWell(x, hotbarY, SLOT_SIZE, SLOT_SIZE);

        const ItemStack& stack = m_inventory.slot(i);
        drawItem(x, hotbarY, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f), stack.damage);
        m_slotBoxes.push_back({ SlotBox::Kind::Slot, i, x, hotbarY, SLOT_SIZE });
    }

    // ------------------------- whatever is on the cursor, drawn last ---
    const ItemStack& held = m_inventory.cursor();
    if (!held.empty())
    {
        const float x = static_cast<float>(m_input.mouseX()) - SLOT_SIZE * 0.5f;
        const float y = static_cast<float>(m_input.mouseY()) - SLOT_SIZE * 0.5f;
        drawItem(x, y, SLOT_SIZE, held.id, held.count, glm::vec4(1.0f), held.damage);
    }
}

const Application::SlotBox* Application::slotBoxAt(int mouseX, int mouseY) const
{
    const float x = static_cast<float>(mouseX);
    const float y = static_cast<float>(mouseY);

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

        case SlotBox::Kind::Slot:
            if (shiftHeld) m_inventory.quickMove(box->index);
            else if (rightButton) m_inventory.rightClick(box->index);
            else m_inventory.leftClick(box->index);
            // Any click can change what the grid adds up to, and working
            // out which ones can is not worth the bug it would cause.
            m_inventory.refreshCraftResult();
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
    stowFurnace();
    stowEnchantingTable();
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
    m_tradeWith = Trading::professionFor(villager.birthSeed());
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

    const Chests::Contents& held = m_chests.at(block);
    for (int i = 0; i < Inventory::CHEST_SLOTS; ++i)
        m_inventory.slot(Inventory::CHEST_FIRST + i) = held[i];

    m_inventoryOpen = true;
    setMouseCaptured(false);
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

    const Furnaces::State& state = m_furnaces.at(block);
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
            setFurnaceLit(glm::ivec3(entry.first[0], entry.first[1], entry.first[2]),
                          entry.second.lit());
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
    if (now != Blocks::Furnace && now != Blocks::FurnaceLit) return;

    const BlockId wanted = lit ? Blocks::FurnaceLit : Blocks::Furnace;
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
