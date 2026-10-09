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
    };

    const StackId TAB_NATURE[] = {
        Blocks::Grass, Blocks::SnowGrass, Blocks::Dirt, Blocks::TallGrass,
        Blocks::FlowerRed, Blocks::FlowerYellow, Blocks::Leaves, Blocks::BirchLeaves,
        Blocks::Log, Blocks::BirchLog, Blocks::Cactus, Blocks::Pumpkin,
        Blocks::Sand, Blocks::Snow, Blocks::Ice, Blocks::Clay,
    };

    const StackId TAB_ORES[] = {
        Blocks::CoalOre, Blocks::IronOre, Blocks::GoldOre, Blocks::DiamondOre,
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
        Items::Feather, Items::Bone, Items::StringItem, Items::Gunpowder,
    };

    const CreativeTab CREATIVE_TABS[] = {
        { "BUILDING BLOCKS",  Blocks::Bricks,     TAB_BUILDING, static_cast<int>(std::size(TAB_BUILDING)) },
        { "NATURE",           Blocks::Grass,      TAB_NATURE,   static_cast<int>(std::size(TAB_NATURE)) },
        { "ORES AND STONE",   Blocks::DiamondOre, TAB_ORES,     static_cast<int>(std::size(TAB_ORES)) },
        { "LIGHT AND LIQUID", Blocks::Glowstone,  TAB_MISC,     static_cast<int>(std::size(TAB_MISC)) },
        { "ITEMS",            Items::Wheat,       TAB_ITEMS,    static_cast<int>(std::size(TAB_ITEMS)) },
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
                           const glm::vec4& tint)
{
    if (id == Blocks::Air) return;

    // Minecraft leaves a clear margin around an item in its slot; at
    // 11% the blocks looked wedged in.
    const float inset = size * 0.19f;
    const TileUV uv = tileUV(atlasTileFor(id));
    m_ui.texturedQuad(m_atlas.textureId(), x + inset, y + inset,
                      size - inset * 2.0f, size - inset * 2.0f,
                      uv.u0, uv.vTop, uv.u1, uv.vBottom, tint);

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
    const float topHeight = showPalette ? 0.0f : Inventory::ARMOR_SLOTS * step + SLOT_GAP;
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
                                 : (m_benchOpen ? "CRAFTING" : "INVENTORY");
    m_ui.text(title, panelX + 10.0f, panelY + 10.0f, 2.2f, GUI_TEXT);

    // ------------------------------- armour, body preview, crafting ---
    if (!showPalette)
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
                drawItem(x, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f));
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
                drawItem(x, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f));
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
        drawItem(resultX, arrowY, SLOT_SIZE, result.id, result.count, glm::vec4(1.0f));
        m_slotBoxes.push_back({ SlotBox::Kind::Slot, Inventory::CRAFT_RESULT,
                                resultX, arrowY, SLOT_SIZE });
    }

    // ------------------------------------------------------- main grid ---
    const float gridY = panelY + titleHeight + topHeight;
    const StackId* paletteItems = nullptr;
    int paletteCount = 0;
    if (showPalette) tabContents(m_creativeTab, paletteItems, paletteCount);

    for (int row = 0; row < gridRows; ++row)
    {
        for (int column = 0; column < PALETTE_COLUMNS; ++column)
        {
            const int cell = row * PALETTE_COLUMNS + column;
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
                // Storage runs 9..35, directly above the hotbar.
                const int index = Inventory::HOTBAR_SLOTS + cell;
                const ItemStack& stack = m_inventory.slot(index);
                drawItem(x, y, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f));
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
        drawItem(x, hotbarY, SLOT_SIZE, stack.id, stack.count, glm::vec4(1.0f));
        m_slotBoxes.push_back({ SlotBox::Kind::Slot, i, x, hotbarY, SLOT_SIZE });
    }

    // ------------------------- whatever is on the cursor, drawn last ---
    const ItemStack& held = m_inventory.cursor();
    if (!held.empty())
    {
        const float x = static_cast<float>(m_input.mouseX()) - SLOT_SIZE * 0.5f;
        const float y = static_cast<float>(m_input.mouseY()) - SLOT_SIZE * 0.5f;
        drawItem(x, y, SLOT_SIZE, held.id, held.count, glm::vec4(1.0f));
    }
}

const Application::SlotBox* Application::slotBoxAt(int mouseX, int mouseY) const
{
    const float x = static_cast<float>(mouseX);
    const float y = static_cast<float>(mouseY);

    for (const SlotBox& box : m_slotBoxes)
        if (x >= box.x && x < box.x + box.size && y >= box.y && y < box.y + box.size)
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
            m_audio.play(Sound::Click, 0.45f);
            break;

        case SlotBox::Kind::Palette:
            m_inventory.takeFromPalette(static_cast<BlockId>(box->index));
            m_audio.play(Sound::Click, 0.4f);
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
    returnCursorToWorld();
    m_inventoryOpen = false;
    m_benchOpen = false;
    m_inventory.setCraftSize(2);
}

void Application::openBench(const glm::ivec3& block)
{
    m_benchBlock = block;
    m_benchOpen = true;
    m_inventory.setCraftSize(3);
    m_inventoryOpen = true;
    setMouseCaptured(false);
}

void Application::returnCursorToWorld()
{
    std::vector<ItemStack> spilled;
    m_inventory.clearCraftGrid(spilled);

    ItemStack& held = m_inventory.cursor();
    if (!held.empty())
    {
        if (m_player.creative()) held.clear();
        else spilled.push_back(ItemStack{ held.id, m_inventory.add(held.id, held.count) });
        held.clear();
    }

    if (m_player.creative()) return;

    for (const ItemStack& stack : spilled)
        if (!stack.empty())
            m_drops.spawn(m_player.eyePosition() + m_camera.front * 0.8f, stack.id, stack.count);
}
