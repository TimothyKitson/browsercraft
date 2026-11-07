int Application::bookKind() const
{
    if (!m_inventoryOpen) return -1;
    if (m_chestOpen || m_crafterOpen || m_tradeOpen || m_enchantOpen || m_brewOpen || m_anvilOpen || m_horseOpen ||
        m_beaconOpen || m_grindOpen || m_cutOpen || m_smithOpen || m_cartoOpen || m_loomOpen)
        return -1;
    if (m_furnaceOpen)
    {
        const BlockId kind = m_world ? furnaceBase(m_world->getBlock(m_furnaceBlock.x, m_furnaceBlock.y, m_furnaceBlock.z))
                                     : static_cast<BlockId>(Blocks::Furnace);
        if (kind == Blocks::Smoker) return static_cast<int>(RecipeBook::Book::Smoker);
        if (kind == Blocks::BlastFurnace) return static_cast<int>(RecipeBook::Book::BlastFurnace);
        return static_cast<int>(RecipeBook::Book::Furnace);
    }
    if (m_inventory.craftSize() < 3 && m_player.creative()) return -1;
    return static_cast<int>(RecipeBook::Book::Crafting);
}

RecipeBook::Stock Application::bookStock() const
{
    std::vector<ItemStack> held;
    for (int i = 0; i < Inventory::MAIN_SLOTS; ++i) held.push_back(m_inventory.slot(i));
    if (m_furnaceOpen) held.push_back(m_inventory.slot(Inventory::FURNACE_INPUT));
    else
        for (int i = 0; i < Inventory::CRAFT_SLOTS; ++i) held.push_back(m_inventory.slot(Inventory::CRAFT_FIRST + i));
    return RecipeBook::stockOf(held.data(), static_cast<int>(held.size()));
}

std::vector<RecipeBook::Group> Application::bookTabs() const
{
    const int kind = bookKind();
    std::vector<RecipeBook::Group> shown;
    if (kind < 0) return shown;
    const RecipeBook::Book book = static_cast<RecipeBook::Book>(kind);
    for (RecipeBook::Group group : RecipeBook::tabs(book))
    {
        bool any = group == RecipeBook::Group::Search;
        for (const RecipeBook::Recipe& recipe : RecipeBook::recipes(book))
            if (!any && recipe.group == group && (book != RecipeBook::Book::Crafting || RecipeBook::fits(recipe, m_inventory.craftSize())))
                any = true;
        if (any) shown.push_back(group);
    }
    return shown;
}

std::vector<RecipeBook::Slot> Application::bookSlots() const
{
    const int kind = bookKind();
    if (kind < 0) return {};
    const std::vector<RecipeBook::Group> tabs = bookTabs();
    const RecipeBook::Group tab = tabs.empty() ? RecipeBook::Group::Search
                                               : tabs[static_cast<size_t>(std::clamp(m_recipeTab, 0, static_cast<int>(tabs.size()) - 1))];
    return RecipeBook::slots(static_cast<RecipeBook::Book>(kind), tab, m_inventory.craftSize(),
                             tab == RecipeBook::Group::Search ? m_recipeSearch : std::string(), m_recipeFilter[kind], bookStock());
}

bool Application::bookShown(float w) const
{
    return m_recipeOpen && bookKind() >= 0 && w > 0.0f;
}

float Application::bookPanelLeft(float w, float panelWidth) const
{
    if (!bookShown(w) || w < 379.0f) return std::floor((w - panelWidth) * 0.5f);
    return 177.0f + std::floor((w - panelWidth - 200.0f) * 0.5f);
}

void Application::takeForBook(StackId id, int count)
{
    for (int i = 0; i < Inventory::MAIN_SLOTS && count > 0; ++i)
    {
        ItemStack& slot = m_inventory.slot(i);
        if (slot.id != id || slot.empty() || slot.damage != 0 || slot.enchantments != 0) continue;
        const int taken = std::min(count, slot.count);
        slot.count -= taken;
        count -= taken;
        if (slot.count <= 0) slot.clear();
    }
}

void Application::bookChoose(int index, bool all)
{
    const int kind = bookKind();
    if (kind < 0) return;
    const RecipeBook::Book book = static_cast<RecipeBook::Book>(kind);
    const std::vector<RecipeBook::Recipe>& list = RecipeBook::recipes(book);
    if (index < 0 || index >= static_cast<int>(list.size())) return;
    const RecipeBook::Recipe& recipe = list[static_cast<size_t>(index)];
    m_audio.play(Sound::Click, 0.45f);

    if (RecipeBook::timesCraftable(recipe, bookStock(), 1) <= 0)
    {
        m_recipeGhost = index;
        m_recipeGhostBook = kind;
        return;
    }
    m_recipeGhost = -1;

    const int size = book == RecipeBook::Book::Crafting ? m_inventory.craftSize() : 1;
    std::vector<int> cells;
    for (size_t c = 0; c < recipe.cells.size(); ++c)
        cells.push_back(book == RecipeBook::Book::Crafting ? Inventory::CRAFT_FIRST + RecipeBook::cellIndex(recipe, static_cast<int>(c), size)
                                                           : Inventory::FURNACE_INPUT);

    int already = 0;
    {
        bool same = true;
        int sets = -1;
        for (size_t c = 0; c < recipe.cells.size() && same; ++c)
        {
            const ItemStack& cell = m_inventory.slot(cells[c]);
            if (recipe.cells[c].empty()) { same = cell.empty(); continue; }
            if (cell.empty() || std::find(recipe.cells[c].begin(), recipe.cells[c].end(), cell.id) == recipe.cells[c].end()) same = false;
            else sets = sets < 0 ? cell.count : std::min(sets, cell.count);
        }
        if (same && sets > 0) already = sets;
    }

    const int first = book == RecipeBook::Book::Crafting ? Inventory::CRAFT_FIRST : Inventory::FURNACE_INPUT;
    const int last = book == RecipeBook::Book::Crafting ? Inventory::CRAFT_FIRST + Inventory::CRAFT_SLOTS : Inventory::FURNACE_INPUT + 1;
    std::vector<ItemStack> saved;
    for (int i = first; i < last; ++i) saved.push_back(m_inventory.slot(i));
    for (int i = first; i < last; ++i)
    {
        ItemStack& cell = m_inventory.slot(i);
        if (cell.empty()) continue;
        const int left = m_inventory.addStack(cell);
        if (left > 0)
        {
            for (int k = first; k < last; ++k) m_inventory.slot(k) = saved[static_cast<size_t>(k - first)];
            return;
        }
        cell.clear();
    }

    std::vector<ItemStack> main;
    for (int i = 0; i < Inventory::MAIN_SLOTS; ++i) main.push_back(m_inventory.slot(i));
    const RecipeBook::Stock stock = RecipeBook::stockOf(main.data(), static_cast<int>(main.size()));
    int want = all ? 64 : already + 1;
    std::vector<StackId> picks;
    while (want > 0)
    {
        if (RecipeBook::allocate(recipe, stock, want, picks))
        {
            bool fitsStack = true;
            for (StackId pick : picks)
                if (pick != Blocks::Air && want > maxStackOf(pick)) fitsStack = false;
            if (fitsStack) break;
        }
        --want;
    }
    if (want <= 0) return;
    for (size_t c = 0; c < recipe.cells.size(); ++c)
    {
        if (picks[c] == Blocks::Air) continue;
        takeForBook(picks[c], want);
        ItemStack& cell = m_inventory.slot(cells[c]);
        cell.id = picks[c];
        cell.count = want;
        cell.damage = 0;
        cell.enchantments = 0;
    }
    m_inventory.refreshCraftResult();
}

void Application::bookClick(const SlotBox& box, bool rightButton, bool shiftHeld)
{
    const int kind = bookKind();
    switch (box.kind)
    {
        case SlotBox::Kind::BookToggle:
            m_recipeOpen = !m_recipeOpen;
            m_recipeAltSlot = -1;
            m_recipeSearching = false;
            saveOptions();
            m_audio.play(Sound::Click, 0.45f);
            break;
        case SlotBox::Kind::BookTab:
            if (m_recipeTab != box.index)
            {
                m_recipeTab = box.index;
                m_recipePage = 0;
            }
            m_audio.play(Sound::Click, 0.45f);
            break;
        case SlotBox::Kind::BookFilter:
            if (kind >= 0) m_recipeFilter[kind] = !m_recipeFilter[kind];
            m_recipePage = 0;
            saveOptions();
            m_audio.play(Sound::Click, 0.45f);
            break;
        case SlotBox::Kind::BookPage:
            m_recipePage = std::max(0, m_recipePage + box.index);
            m_audio.play(Sound::Click, 0.45f);
            break;
        case SlotBox::Kind::BookSearch:
            m_recipeSearching = true;
            break;
        case SlotBox::Kind::BookResult:
        {
            const std::vector<RecipeBook::Slot> slots = bookSlots();
            if (box.index < 0 || box.index >= static_cast<int>(slots.size()) || kind < 0) break;
            const RecipeBook::Slot& slot = slots[static_cast<size_t>(box.index)];
            const int shown = m_recipeFilter[kind] ? slot.craftable : static_cast<int>(slot.recipes.size());
            if (shown <= 0) break;
            if (rightButton)
            {
                if (shown > 1)
                {
                    m_recipeAltSlot = box.index;
                    m_recipeAltX = box.x;
                    m_recipeAltY = box.y;
                    m_audio.play(Sound::Click, 0.45f);
                }
                break;
            }
            const int pick = slot.recipes[static_cast<size_t>(static_cast<int>(m_elapsedSeconds / RecipeBook::CYCLE_SECONDS) % shown)];
            bookChoose(pick, shiftHeld);
            break;
        }
        case SlotBox::Kind::BookAlt:
            m_recipeAltSlot = -1;
            bookChoose(box.index, shiftHeld);
            break;
        default:
            break;
    }
}

void Application::renderRecipeBook(float w, float h, float panelX)
{
    const int kind = bookKind();
    if (kind < 0) return;
    const RecipeBook::Book book = static_cast<RecipeBook::Book>(kind);
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    auto over = [&](float x, float y, float bw, float bh) { return mouseX >= x && mouseX < x + bw && mouseY >= y && mouseY < y + bh; };
    auto push = [&](SlotBox::Kind boxKind, int index, float x, float y, float bw, float bh) {
        SlotBox box;
        box.kind = boxKind;
        box.index = index;
        box.x = x;
        box.y = y;
        box.size = bh;
        box.width = bw;
        m_slotBoxes.push_back(box);
    };

    const bool plain = !m_furnaceOpen && m_inventory.craftSize() < 3;
    const float buttonX = panelX + (m_furnaceOpen ? 20.0f : plain ? 104.0f : 5.0f);
    const float buttonY = std::floor(h * 0.5f) - (plain ? 22.0f : 49.0f);
    const bool hoverButton = over(buttonX, buttonY, 20.0f, 18.0f);
    if (!guiSprite(hoverButton ? "sprites/recipe_book/button_highlighted" : "sprites/recipe_book/button", buttonX, buttonY, 20.0f, 18.0f))
    {
        m_ui.quad(buttonX, buttonY, 20.0f, 18.0f, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        m_ui.quad(buttonX + 3.0f, buttonY + 2.0f, 14.0f, 14.0f, hoverButton ? glm::vec4(0.45f, 0.8f, 0.35f, 1.0f) : glm::vec4(0.3f, 0.62f, 0.25f, 1.0f));
        m_ui.quad(buttonX + 9.0f, buttonY + 2.0f, 2.0f, 14.0f, glm::vec4(0.82f, 0.74f, 0.55f, 1.0f));
    }
    push(SlotBox::Kind::BookToggle, 0, buttonX, buttonY, 20.0f, 18.0f);
    if (!m_recipeOpen) return;

    const float left = std::floor((w - 147.0f) * 0.5f) - (w >= 379.0f ? 86.0f : 0.0f);
    const float top = std::floor((h - 166.0f) * 0.5f);
    if (!guiBlit("recipe_book", left, top, 1.0f, 1.0f, 147.0f, 166.0f)) guiPanel(left, top, 147.0f, 166.0f);

    const std::vector<RecipeBook::Group> tabs = bookTabs();
    m_recipeTab = std::clamp(m_recipeTab, 0, std::max(0, static_cast<int>(tabs.size()) - 1));
    for (size_t n = 0; n < tabs.size(); ++n)
    {
        const bool chosen = static_cast<int>(n) == m_recipeTab;
        const float tabX = left - 30.0f;
        const float tabY = top + 3.0f + 27.0f * static_cast<float>(n);
        const float drawX = chosen ? tabX - 2.0f : tabX;
        if (!guiSprite(chosen ? "sprites/recipe_book/tab_selected" : "sprites/recipe_book/tab", drawX, tabY, 35.0f, 27.0f))
        {
            m_ui.quad(drawX, tabY, 35.0f, 27.0f, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
            m_ui.quad(drawX + 1.0f, tabY + 1.0f, 34.0f, 25.0f, chosen ? glm::vec4(0.78f, 0.78f, 0.78f, 1.0f) : glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));
        }
        const std::vector<StackId> icons = RecipeBook::tabIcons(book, tabs[n]);
        const float shift = chosen ? -2.0f : 0.0f;
        if (icons.size() == 1) drawItem(tabX + 9.0f + shift, tabY + 5.0f, SLOT, icons[0], 1, glm::vec4(1.0f));
        else if (icons.size() == 2)
        {
            drawItem(tabX + 3.0f + shift, tabY + 5.0f, SLOT, icons[0], 1, glm::vec4(1.0f));
            drawItem(tabX + 14.0f + shift, tabY + 5.0f, SLOT, icons[1], 1, glm::vec4(1.0f));
        }
        push(SlotBox::Kind::BookTab, static_cast<int>(n), tabX - 2.0f, tabY, 37.0f, 27.0f);
    }

    const float fieldX = left + 25.0f, fieldY = top + 13.0f;
    if (!guiNineSlice(m_recipeSearching ? "sprites/widget/text_field_highlighted" : "sprites/widget/text_field", fieldX, fieldY, 81.0f, 14.0f, 1.0f))
    {
        m_ui.quad(fieldX, fieldY, 81.0f, 14.0f, m_recipeSearching ? glm::vec4(1.0f) : glm::vec4(0.63f, 0.63f, 0.63f, 1.0f));
        m_ui.quad(fieldX + 1.0f, fieldY + 1.0f, 79.0f, 12.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    if (m_recipeSearch.empty() && !m_recipeSearching)
        m_ui.textWithShadow("Search...", fieldX + 4.0f, fieldY + 3.0f, 1.0f, glm::vec4(0.667f, 0.667f, 0.667f, 1.0f));
    else
    {
        std::string shown = m_recipeSearch;
        while (!shown.empty() && UIRenderer::textWidth(shown, 1.0f) > 72.0f) shown.erase(shown.begin());
        const bool caret = m_recipeSearching && static_cast<int>(m_elapsedSeconds * 3.0f) % 2 == 0;
        m_ui.textWithShadow(shown + (caret ? "_" : ""), fieldX + 4.0f, fieldY + 3.0f, 1.0f, glm::vec4(1.0f));
    }
    push(SlotBox::Kind::BookSearch, 0, fieldX, fieldY, 81.0f, 14.0f);

    const float filterX = left + 110.0f, filterY = top + 12.0f;
    const bool filtering = m_recipeFilter[kind];
    const bool hoverFilter = over(filterX, filterY, 26.0f, 16.0f);
    const char* filterSprite = filtering ? (hoverFilter ? "sprites/recipe_book/filter_enabled_highlighted" : "sprites/recipe_book/filter_enabled")
                                         : (hoverFilter ? "sprites/recipe_book/filter_disabled_highlighted" : "sprites/recipe_book/filter_disabled");
    if (!guiSprite(filterSprite, filterX, filterY, 26.0f, 16.0f))
    {
        m_ui.quad(filterX, filterY, 26.0f, 16.0f, glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
        m_ui.quad(filterX + 2.0f, filterY + 2.0f, 22.0f, 12.0f, filtering ? glm::vec4(0.35f, 0.7f, 0.3f, 1.0f) : glm::vec4(0.45f, 0.45f, 0.45f, 1.0f));
    }
    static const char* SHOWING[4] = { "Showing Craftable", "Showing Smeltable", "Showing Blastable", "Showing Smokable" };
    addHint(filterX, filterY, 26.0f, 16.0f, filtering ? SHOWING[kind] : "Showing All");
    push(SlotBox::Kind::BookFilter, 0, filterX, filterY, 26.0f, 16.0f);

    const std::vector<RecipeBook::Slot> slots = bookSlots();
    const std::vector<RecipeBook::Recipe>& list = RecipeBook::recipes(book);
    const int pages = std::max(1, (static_cast<int>(slots.size()) + RecipeBook::PER_PAGE - 1) / RecipeBook::PER_PAGE);
    m_recipePage = std::clamp(m_recipePage, 0, pages - 1);
    const int cycle = static_cast<int>(m_elapsedSeconds / RecipeBook::CYCLE_SECONDS);
    for (int k = 0; k < RecipeBook::PER_PAGE; ++k)
    {
        const int at = m_recipePage * RecipeBook::PER_PAGE + k;
        if (at >= static_cast<int>(slots.size())) break;
        const RecipeBook::Slot& slot = slots[static_cast<size_t>(at)];
        const int shown = filtering ? slot.craftable : static_cast<int>(slot.recipes.size());
        if (shown <= 0) continue;
        const bool craftable = slot.craftable > 0;
        const bool many = shown > 1;
        const float sx = left + 11.0f + 25.0f * static_cast<float>(k % 5);
        const float sy = top + 31.0f + 25.0f * static_cast<float>(k / 5);
        const char* sprite = many ? (craftable ? "sprites/recipe_book/slot_many_craftable" : "sprites/recipe_book/slot_many_uncraftable")
                                  : (craftable ? "sprites/recipe_book/slot_craftable" : "sprites/recipe_book/slot_uncraftable");
        if (!guiSprite(sprite, sx, sy, 25.0f, 25.0f))
        {
            m_ui.quad(sx, sy, 25.0f, 25.0f, glm::vec4(0.12f, 0.12f, 0.12f, 1.0f));
            m_ui.quad(sx + 1.0f, sy + 1.0f, 23.0f, 23.0f, craftable ? glm::vec4(0.55f, 0.55f, 0.55f, 1.0f) : glm::vec4(0.62f, 0.3f, 0.3f, 1.0f));
            if (many) m_ui.quad(sx + 3.0f, sy + 21.0f, 19.0f, 2.0f, glm::vec4(0.9f, 0.9f, 0.9f, 1.0f));
        }
        const RecipeBook::Recipe& recipe = list[static_cast<size_t>(slot.recipes[static_cast<size_t>(cycle % shown)])];
        drawItem(sx + 4.0f, sy + 4.0f, SLOT, recipe.result, recipe.count, glm::vec4(1.0f));
        addHint(sx, sy, 25.0f, 25.0f, displayName(recipe.result));
        push(SlotBox::Kind::BookResult, at, sx, sy, 25.0f, 25.0f);
    }

    if (pages > 1)
    {
        if (m_recipePage < pages - 1)
        {
            const float px = left + 93.0f, py = top + 137.0f;
            if (!guiSprite(over(px, py, 12.0f, 17.0f) ? "sprites/recipe_book/page_forward_highlighted" : "sprites/recipe_book/page_forward", px, py, 12.0f, 17.0f))
                m_ui.textWithShadow(">", px + 3.0f, py + 4.0f, 1.0f, glm::vec4(1.0f));
            push(SlotBox::Kind::BookPage, 1, px, py, 12.0f, 17.0f);
        }
        if (m_recipePage > 0)
        {
            const float px = left + 38.0f, py = top + 137.0f;
            if (!guiSprite(over(px, py, 12.0f, 17.0f) ? "sprites/recipe_book/page_backward_highlighted" : "sprites/recipe_book/page_backward", px, py, 12.0f, 17.0f))
                m_ui.textWithShadow("<", px + 3.0f, py + 4.0f, 1.0f, glm::vec4(1.0f));
            push(SlotBox::Kind::BookPage, -1, px, py, 12.0f, 17.0f);
        }
        const std::string page = std::to_string(m_recipePage + 1) + "/" + std::to_string(pages);
        m_ui.text(page, left - std::floor(UIRenderer::textWidth(page, 1.0f) * 0.5f) + 73.0f, top + 141.0f, 1.0f, glm::vec4(1.0f));
    }

    if (m_recipeAltSlot >= 0 && m_recipeAltSlot < static_cast<int>(slots.size()))
    {
        const RecipeBook::Slot& slot = slots[static_cast<size_t>(m_recipeAltSlot)];
        const int craftable = slot.craftable;
        const int total = filtering ? craftable : static_cast<int>(slot.recipes.size());
        const int across = total <= 16 ? 4 : 5;
        const int rows = (total + across - 1) / across;
        float altX = m_recipeAltX, altY = m_recipeAltY;
        const float centreX = left + 147.0f * 0.5f, centreY = top + 13.0f + 166.0f * 0.5f;
        const float rightEdge = altX + static_cast<float>(std::min(total, across)) * 25.0f;
        if (rightEdge > centreX + 50.0f) altX -= 25.0f * std::floor((rightEdge - (centreX + 50.0f)) / 25.0f);
        const float bottomEdge = altY + static_cast<float>(rows) * 25.0f;
        if (bottomEdge > centreY + 50.0f) altY -= 25.0f * std::ceil((bottomEdge - (centreY + 50.0f)) / 25.0f);
        if (altY < centreY - 100.0f) altY -= 25.0f * std::ceil((altY - (centreY - 100.0f)) / 25.0f);
        const float boxW = static_cast<float>(std::min(total, across)) * 25.0f + 8.0f;
        const float boxH = static_cast<float>(rows) * 25.0f + 8.0f;
        if (!guiNineSlice("sprites/recipe_book/overlay_recipe", altX, altY, boxW, boxH, 1.0f)) guiPanel(altX, altY, boxW, boxH);
        std::vector<SlotBox> altBoxes;
        const bool cooking = book != RecipeBook::Book::Crafting;
        for (int p = 0; p < total; ++p)
        {
            const int index = slot.recipes[static_cast<size_t>(p)];
            const RecipeBook::Recipe& recipe = list[static_cast<size_t>(index)];
            const bool can = p < craftable;
            const float bx = altX + 4.0f + 25.0f * static_cast<float>(p % across);
            const float by = altY + 5.0f + 25.0f * static_cast<float>(p / across);
            const bool lit = over(bx, by, 24.0f, 24.0f);
            const char* sprite = cooking ? (can ? (lit ? "sprites/recipe_book/furnace_overlay_highlighted" : "sprites/recipe_book/furnace_overlay")
                                                : (lit ? "sprites/recipe_book/furnace_overlay_disabled_highlighted" : "sprites/recipe_book/furnace_overlay_disabled"))
                                         : (can ? (lit ? "sprites/recipe_book/crafting_overlay_highlighted" : "sprites/recipe_book/crafting_overlay")
                                                : (lit ? "sprites/recipe_book/crafting_overlay_disabled_highlighted" : "sprites/recipe_book/crafting_overlay_disabled"));
            if (!guiSprite(sprite, bx, by, 24.0f, 24.0f))
            {
                m_ui.quad(bx, by, 24.0f, 24.0f, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
                m_ui.quad(bx + 1.0f, by + 1.0f, 22.0f, 22.0f, can ? glm::vec4(0.6f, 0.6f, 0.6f, 1.0f) : glm::vec4(0.62f, 0.32f, 0.32f, 1.0f));
            }
            if (cooking)
            {
                if (!recipe.cells.empty() && !recipe.cells[0].empty())
                    drawItem(bx + 9.0f, by + 9.0f, 6.0f, recipe.cells[0][static_cast<size_t>(cycle) % recipe.cells[0].size()], 1, glm::vec4(1.0f));
            }
            else
                for (size_t c = 0; c < recipe.cells.size(); ++c)
                {
                    if (recipe.cells[c].empty()) continue;
                    const int grid = RecipeBook::cellIndex(recipe, static_cast<int>(c), 3);
                    drawItem(bx + 2.0f + static_cast<float>(grid % 3) * 7.0f, by + 2.0f + static_cast<float>(grid / 3) * 7.0f, 6.0f,
                             recipe.cells[c][static_cast<size_t>(cycle) % recipe.cells[c].size()], 1, glm::vec4(1.0f));
                }
            SlotBox box;
            box.kind = SlotBox::Kind::BookAlt;
            box.index = index;
            box.x = bx;
            box.y = by;
            box.size = 24.0f;
            box.width = 24.0f;
            altBoxes.push_back(box);
            addHint(bx, by, 24.0f, 24.0f, displayName(recipe.result));
        }
        m_slotBoxes.insert(m_slotBoxes.begin(), altBoxes.begin(), altBoxes.end());
    }
}

void Application::drawGhostRecipe(float panelX, float panelY)
{
    const int kind = bookKind();
    if (m_recipeGhost < 0 || kind != m_recipeGhostBook) return;
    const RecipeBook::Book book = static_cast<RecipeBook::Book>(kind);
    const std::vector<RecipeBook::Recipe>& list = RecipeBook::recipes(book);
    if (m_recipeGhost >= static_cast<int>(list.size())) return;
    const RecipeBook::Recipe& recipe = list[static_cast<size_t>(m_recipeGhost)];
    const int cycle = static_cast<int>(m_elapsedSeconds / RecipeBook::CYCLE_SECONDS);
    const glm::vec4 red(1.0f, 0.0f, 0.0f, 48.0f / 255.0f);
    const glm::vec4 haze(1.0f, 1.0f, 1.0f, 48.0f / 255.0f);
    auto ghost = [&](float x, float y, StackId id, int count, bool big) {
        if (big) m_ui.quad(x - 4.0f, y - 4.0f, 24.0f, 24.0f, red);
        else m_ui.quad(x, y, 16.0f, 16.0f, red);
        drawItem(x, y, SLOT, id, count, glm::vec4(1.0f));
        m_ui.quad(x, y, 16.0f, 16.0f, haze);
        addHint(x, y, 16.0f, 16.0f, displayName(id));
    };
    if (book == RecipeBook::Book::Crafting)
    {
        const int size = m_inventory.craftSize();
        const bool bench = size >= 3;
        const float gridX = panelX + (bench ? 30.0f : 98.0f);
        const float gridY = panelY + (bench ? 17.0f : 18.0f);
        for (size_t c = 0; c < recipe.cells.size(); ++c)
        {
            if (recipe.cells[c].empty()) continue;
            const int at = RecipeBook::cellIndex(recipe, static_cast<int>(c), size);
            if (at % 3 >= size || at / 3 >= size || !m_inventory.slot(Inventory::CRAFT_FIRST + at).empty()) continue;
            ghost(gridX + static_cast<float>(at % 3) * PITCH, gridY + static_cast<float>(at / 3) * PITCH,
                  recipe.cells[c][static_cast<size_t>(cycle) % recipe.cells[c].size()], 1, false);
        }
        if (m_inventory.slot(Inventory::CRAFT_RESULT).empty())
            ghost(panelX + (bench ? 124.0f : 154.0f), panelY + (bench ? 35.0f : 28.0f), recipe.result, recipe.count, bench);
        return;
    }
    if (!recipe.cells.empty() && !recipe.cells[0].empty() && m_inventory.slot(Inventory::FURNACE_INPUT).empty())
        ghost(panelX + 56.0f, panelY + 17.0f, recipe.cells[0][static_cast<size_t>(cycle) % recipe.cells[0].size()], 1, false);
    if (m_inventory.slot(Inventory::FURNACE_FUEL).empty())
    {
        static const std::vector<StackId> burning = RecipeBook::fuels();
        if (!burning.empty()) ghost(panelX + 56.0f, panelY + 53.0f, burning[static_cast<size_t>(cycle) % burning.size()], 1, false);
    }
    if (m_inventory.slot(Inventory::FURNACE_OUTPUT).empty()) ghost(panelX + 116.0f, panelY + 35.0f, recipe.result, recipe.count, true);
}
