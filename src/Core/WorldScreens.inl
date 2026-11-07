namespace
{
    const glm::vec4 WORLD_WHITE(1.0f);
    const glm::vec4 WORLD_GRAY(0x80 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f, 1.0f);
    const glm::vec4 WORLD_LABEL(0xA0 / 255.0f, 0xA0 / 255.0f, 0xA0 / 255.0f, 1.0f);
    const glm::vec4 WORLD_DARK(0x55 / 255.0f, 0x55 / 255.0f, 0x55 / 255.0f, 1.0f);
    const glm::vec4 WORLD_RED(1.0f, 0x55 / 255.0f, 0x55 / 255.0f, 1.0f);
    const glm::vec4 WORLD_FIELD(0xE0 / 255.0f, 0xE0 / 255.0f, 0xE0 / 255.0f, 1.0f);
    const GameMode CREATE_MODES[3] = { GameMode::Survival, GameMode::Hardcore, GameMode::Creative };
    const char* const CREATE_MODE_NAMES[3] = { "Survival", "Hardcore", "Creative" };
    const char* const CREATE_MODE_INFO[3] = {
        "Explore a mysterious world where you build, collect, craft, and fight monsters.",
        "Survival Mode locked to 'Hard' difficulty. You can't respawn if you die.",
        "Create, build, and explore without limits. You can fly, have endless materials, and can't be hurt by monsters." };
    const char* const CREATE_TABS[3] = { "Game", "World", "More" };

    std::string modeDetail(int mode)
    {
        switch (static_cast<GameMode>(mode))
        {
            case GameMode::Creative: return "Creative Mode";
            case GameMode::Hardcore: return "Hardcore Mode!";
            case GameMode::Adventure: return "Adventure Mode";
            case GameMode::Spectator: return "Spectator Mode";
            default: return "Survival Mode";
        }
    }

    std::vector<std::string> wrapText(const std::string& text, float width)
    {
        std::vector<std::string> out;
        std::istringstream words(text);
        std::string word, line;
        while (words >> word)
        {
            const std::string attempt = line.empty() ? word : line + " " + word;
            if (!line.empty() && UIRenderer::textWidth(attempt, 1.0f) > width)
            {
                out.push_back(line);
                line = word;
            }
            else
                line = attempt;
        }
        if (!line.empty()) out.push_back(line);
        return out;
    }

    std::string shortBlockId(const std::string& id)
    {
        return id.rfind("minecraft:", 0) == 0 ? id.substr(10) : id;
    }
}

void Application::menuScreenBackdrop(float w, float h)
{
    const bool inWorld = m_world && !m_menuWorld;
    if (!guiTiled(inWorld ? "inworld_menu_background" : "menu_background", 0.0f, 0.0f, w, h, 32.0f))
        m_ui.quad(0.0f, 0.0f, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
}

bool Application::guiButton(float x, float y, float w, float h, const std::string& label, bool enabled)
{
    const float unit = m_ui.scale();
    m_ui.setScale(unit * MENU_UNIT);
    const bool hit = menuButton({ x / MENU_UNIT, y / MENU_UNIT, w / MENU_UNIT, h / MENU_UNIT }, label, enabled);
    m_ui.setScale(unit);
    if (hit) m_audio.play(Sound::Click, 0.45f);
    return hit;
}

bool Application::guiCycle(float x, float y, float w, const std::string& label, bool enabled)
{
    return guiButton(x, y, w, 20.0f, label, enabled);
}

void Application::guiField(GuiField& field, float x, float y, float w, float h, const std::string& placeholder)
{
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    if (!m_automated && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT))
    {
        const bool inside = mouseX >= x && mouseX < x + w && mouseY >= y && mouseY < y + h;
        if (inside && !field.focused) field.focusTime = SDL_GetTicks();
        field.focused = inside;
    }
    if (field.focused)
    {
        Utf8::type(field.text, m_input.typedText(), field.maxLength);
        for (int i = 0; i < m_input.backspaces() && !field.text.empty(); ++i) Utf8::popBack(field.text);
    }
    if (!guiNineSlice(field.focused ? "sprites/widget/text_field_highlighted" : "sprites/widget/text_field", x, y, w, h, 1.0f))
    {
        m_ui.quad(x, y, w, h, field.focused ? glm::vec4(1.0f) : glm::vec4(0.627f, 0.627f, 0.627f, 1.0f));
        m_ui.quad(x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    const float tx = x + 4.0f;
    const float ty = y + std::floor((h - 8.0f) / 2.0f);
    std::string shown = field.text;
    while (!shown.empty() && UIRenderer::textWidth(shown, 1.0f) > w - 8.0f)
    {
        if (field.focused) Utf8::popFront(shown);
        else Utf8::popBack(shown);
    }
    float end = tx;
    if (!shown.empty())
    {
        m_ui.textWithShadow(shown, tx, ty, 1.0f, WORLD_FIELD);
        end = tx + UIRenderer::textWidth(shown, 1.0f);
    }
    if (!placeholder.empty() && field.text.empty() && !field.focused) m_ui.textWithShadow(placeholder, tx, ty, 1.0f, WORLD_DARK);
    const bool blink = field.focused && ((SDL_GetTicks() - field.focusTime) / 300u) % 2u == 0u;
    if (blink)
    {
        if (field.text.size() >= field.maxLength) m_ui.quad(end - 1.0f, ty - 1.0f, 1.0f, 10.0f, glm::vec4(0xD0 / 255.0f, 0xD0 / 255.0f, 0xD0 / 255.0f, 1.0f));
        else m_ui.textWithShadow("_", end, ty, 1.0f, WORLD_FIELD);
    }
}

void Application::guiList(GuiList& list, float top, float height, float rowHeight, float rowWidth, int count, float header,
                          const std::function<void(int, float, float, bool)>& drawRow, float listX, float listW)
{
    const bool boxed = listW > 0.0f;
    const float w = boxed ? listW : m_ui.canvasWidth();
    const float left = boxed ? listX : 0.0f;
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    const bool clicked = !m_automated && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT);
    const bool held = !m_automated && m_input.isMouseButtonDown(SDL_BUTTON_LEFT);
    list.clicked = -1;
    list.doubleClicked = false;
    const float bottom = top + height;
    const float maxPosition = static_cast<float>(count) * rowHeight + header;
    const float maxScroll = std::max(0.0f, maxPosition - (height - 4.0f));
    const float boxLeft = left + std::floor(w / 2.0f) - std::floor(rowWidth / 2.0f);
    const float rowLeft = boxLeft + 2.0f;
    const float scrollbarX = boxed ? left + w - 6.0f : boxLeft + rowWidth + 10.0f;
    const bool over = mouseY >= top && mouseY <= bottom && mouseX >= left && mouseX <= left + w;
    if (over && m_input.wheelDelta() != 0 && !m_automated) list.scroll -= static_cast<float>(m_input.wheelDelta()) * rowHeight / 2.0f;
    const float thumbH = maxPosition > 0.0f ? std::clamp(std::floor(height * height / maxPosition), 32.0f, std::max(32.0f, height - 8.0f)) : height;
    if (clicked) list.dragging = over && maxScroll > 0.0f && mouseX >= scrollbarX && mouseX < scrollbarX + 6.0f;
    if (!held) list.dragging = false;
    if (list.dragging)
    {
        if (mouseY < top) list.scroll = 0.0f;
        else if (mouseY > bottom) list.scroll = maxScroll;
        else list.scroll += (mouseY - list.grabY) * std::max(1.0f, std::max(1.0f, maxScroll) / std::max(1.0f, height - thumbH));
    }
    list.grabY = mouseY;
    list.scroll = std::clamp(list.scroll, 0.0f, maxScroll);
    const float scroll = std::floor(list.scroll);
    int hovered = -1;
    if (over && mouseX >= boxLeft && mouseX <= boxLeft + rowWidth)
    {
        const int m = static_cast<int>(std::floor(mouseY - top)) - static_cast<int>(header) + static_cast<int>(scroll) - 4;
        const int n = m / static_cast<int>(rowHeight);
        if (m >= 0 && n >= 0 && n < count) hovered = n;
    }
    if (clicked && hovered >= 0 && !list.dragging)
    {
        const uint32_t now = SDL_GetTicks();
        list.doubleClicked = list.clickIndex == hovered && now - list.clickTime < 250u;
        list.clickTime = now;
        list.clickIndex = hovered;
        list.selected = hovered;
        list.clicked = hovered;
    }
    if (list.selected >= count) list.selected = -1;
    list.hovered = hovered;

    const bool inWorld = m_world && !m_menuWorld;
    const char* background = inWorld ? "inworld_menu_list_background" : "menu_list_background";
    if (guiHas(background))
    {
        m_ui.setClip(left, top, w, height);
        const float shiftX = std::fmod(left + w, 32.0f);
        const float shiftY = std::fmod(bottom + scroll, 32.0f);
        for (float ty = top - shiftY; ty < bottom; ty += 32.0f)
            for (float tx = left - shiftX; tx < left + w; tx += 32.0f) guiBlit(background, tx, ty, 0.0f, 0.0f, 32.0f, 32.0f, 32.0f, 32.0f);
        m_ui.clearClip();
    }
    else
        m_ui.quad(left, top, w, height, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));

    m_ui.setClip(left, top, w, height);
    for (int i = 0; i < count; ++i)
    {
        const float y = top + 4.0f - scroll + static_cast<float>(i) * rowHeight + header;
        if (y + rowHeight < top || y > bottom) continue;
        if (i == list.selected)
        {
            const float boxX = boxed && maxScroll > 0.0f ? rowLeft - 2.0f : left + std::floor((w - rowWidth) / 2.0f);
            const float boxW = boxed && maxScroll > 0.0f ? left + w - 7.0f - boxX : rowWidth;
            m_ui.quad(boxX, y - 2.0f, boxW, rowHeight, glm::vec4(1.0f));
            m_ui.quad(boxX + 1.0f, y - 1.0f, boxW - 2.0f, rowHeight - 2.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        drawRow(i, rowLeft, y, i == hovered);
    }
    m_ui.clearClip();

    auto separator = [&](const char* path, float y, bool upper) {
        if (guiHas(path))
        {
            for (float tx = left; tx < left + w; tx += 32.0f) guiBlit(path, tx, y, 0.0f, 0.0f, std::min(32.0f, left + w - tx), 2.0f, 32.0f, 2.0f);
            return;
        }
        m_ui.quad(left, y, w, 1.0f, upper ? glm::vec4(0.0f, 0.0f, 0.0f, 0.75f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.25f));
        m_ui.quad(left, y + 1.0f, w, 1.0f, upper ? glm::vec4(1.0f, 1.0f, 1.0f, 0.25f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));
    };
    separator(inWorld ? "inworld_header_separator" : "header_separator", top - 2.0f, true);
    separator(inWorld ? "inworld_footer_separator" : "footer_separator", bottom, false);
    if (maxScroll > 0.0f)
    {
        const float thumbY = std::max(top, std::floor(scroll * (height - thumbH) / maxScroll + top));
        if (!guiSprite("sprites/widget/scroller_background", scrollbarX, top, 6.0f, height))
            m_ui.quad(scrollbarX, top, 6.0f, height, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        if (!guiSprite("sprites/widget/scroller", scrollbarX, thumbY, 6.0f, thumbH))
        {
            m_ui.quad(scrollbarX, thumbY, 6.0f, thumbH, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
            m_ui.quad(scrollbarX, thumbY, 5.0f, thumbH - 1.0f, glm::vec4(0.75f, 0.75f, 0.75f, 1.0f));
        }
    }
}

void Application::wrappedTooltip(const std::string& text, float mouseX, float mouseY, float width)
{
    const std::vector<std::string> lines = wrapText(text, width);
    if (lines.empty()) return;
    std::string joined;
    for (size_t i = 0; i < lines.size(); ++i) joined += (i ? "\n" : "") + lines[i];
    drawTooltip(joined, mouseX, mouseY, true);
}

void Application::migrateLegacySave()
{
    std::error_code ec;
    if (!std::filesystem::exists(m_configPath + "/level.dat", ec)) return;
    const std::string folder = Worlds::uniqueFolder(savesDirectory(), "World");
    const std::string target = savesDirectory() + "/" + folder;
    std::filesystem::create_directories(target, ec);
    for (const auto& item : std::filesystem::directory_iterator(m_configPath, ec))
    {
        const std::string name = item.path().filename().string();
        if (name == "saves" || name == "backups" || name == "guest" || name == "profile.txt" || name == "options.txt" ||
            name.rfind("keys", 0) == 0)
            continue;
        std::filesystem::rename(item.path(), target + "/" + name, ec);
    }
    Worlds::Meta meta;
    meta.name = "World";
    LevelState state;
    if (WorldSave::loadLevel(target, state))
    {
        meta.mode = state.gameMode;
        meta.cheats = state.cheats;
    }
    meta.lastPlayed = Worlds::nowMillis();
    Worlds::writeMeta(target, meta);
}

void Application::refreshWorlds()
{
    m_worldEntries = Worlds::list(savesDirectory());
    for (const Worlds::Entry& entry : m_worldEntries) m_gui.forget("file:" + entry.path + "/icon.png");
}

std::string Application::defaultWorldFolder()
{
    const std::vector<Worlds::Entry> worlds = Worlds::list(savesDirectory());
    if (!worlds.empty()) return worlds.front().folder;
    return Worlds::uniqueFolder(savesDirectory(), "New World");
}

void Application::openWorldList()
{
    refreshWorlds();
    m_worldSearch.text.clear();
    m_worldSearch.focused = true;
    m_worldSearch.focusTime = SDL_GetTicks();
    m_worldList = GuiList{};
    if (m_worldEntries.empty())
    {
        openCreateWorld(nullptr);
        m_create.fromTitle = true;
        return;
    }
    m_screen = Screen::Singleplayer;
}

void Application::playWorld(const std::string& folder)
{
    m_savePath = savesDirectory() + "/" + folder;
    Worlds::Meta meta;
    if (!Worlds::readMeta(m_savePath, meta)) meta.name = folder;
    m_worldMeta = meta;
    startWorld(static_cast<GameMode>(meta.mode), false);
}

bool Application::createCheats() const
{
    if (m_create.mode == 1) return false;
    if (m_create.cheats >= 0) return m_create.cheats == 1;
    return m_create.mode == 2;
}

void Application::openCreateWorld(const Worlds::Entry* from)
{
    m_create = CreateWorldState{};
    m_createName = GuiField{};
    m_createName.text = "New World";
    m_createName.focused = true;
    m_createName.focusTime = SDL_GetTicks();
    m_createSeed = GuiField{};
    if (from)
    {
        const Worlds::Meta& meta = from->meta;
        m_createName.text = meta.name;
        const GameMode mode = static_cast<GameMode>(meta.mode);
        m_create.mode = mode == GameMode::Hardcore ? 1 : mode == GameMode::Creative ? 2 : 0;
        m_create.cheats = meta.cheats ? 1 : 0;
        m_create.difficulty = meta.difficulty;
        m_create.type = meta.type;
        m_create.structures = meta.structures;
        m_create.bonusChest = meta.bonusChest;
        if (meta.type == Worlds::Type::Flat)
        {
            m_create.layers = meta.layers;
            m_create.flatBiome = meta.biome;
        }
        if (meta.type == Worlds::Type::SingleBiome) m_create.biome = meta.biome;
        LevelState state;
        if (WorldSave::loadLevel(from->path, state) && state.seed != 0) m_createSeed.text = std::to_string(state.seed);
    }
    m_screen = Screen::CreateWorld;
}

std::vector<std::pair<BlockId, int>> Application::flatLayers(const std::string& text) const
{
    std::vector<std::pair<BlockId, int>> out;
    for (const Worlds::Layer& layer : Worlds::parseLayers(text))
    {
        const std::string id = shortBlockId(layer.block);
        StackId stack = id == "air" ? static_cast<StackId>(Blocks::Air) : stackNamed(id);
        if (id == "water") stack = Blocks::Water;
        if (id == "lava") stack = Blocks::Lava;
        if (isItem(stack)) stack = Blocks::Air;
        out.emplace_back(asBlock(stack), layer.count);
    }
    return out;
}

WorldGen::Options Application::generatorOptions(const Worlds::Meta& meta) const
{
    WorldGen::Options options;
    options.structures = meta.structures;
    switch (meta.type)
    {
        case Worlds::Type::Flat:
            options.type = WorldGen::Options::Type::Flat;
            options.layers = flatLayers(meta.layers);
            options.biome = biomeFromId(meta.biome);
            break;
        case Worlds::Type::LargeBiomes: options.type = WorldGen::Options::Type::LargeBiomes; break;
        case Worlds::Type::Amplified: options.type = WorldGen::Options::Type::Amplified; break;
        case Worlds::Type::SingleBiome:
            options.type = WorldGen::Options::Type::SingleBiome;
            options.biome = biomeFromId(meta.biome);
            break;
        default: break;
    }
    return options;
}

void Application::writeWorldMeta()
{
    if (m_menuWorld || m_net.role() == Net::Role::Guest || m_savePath.empty()) return;
    m_worldMeta.lastPlayed = Worlds::nowMillis();
    m_worldMeta.mode = static_cast<int>(m_gameMode);
    m_worldMeta.cheats = m_allowCheats;
    m_worldMeta.border = m_border.isDefault() ? std::string() : m_border.save();
    Worlds::writeMeta(m_savePath, m_worldMeta);
}

void Application::createWorldFromScreen()
{
    const std::string folder = Worlds::uniqueFolder(savesDirectory(), m_createName.text);
    m_savePath = savesDirectory() + "/" + folder;
    Worlds::Meta meta;
    std::string name = m_createName.text;
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) name.pop_back();
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front()))) name.erase(0, 1);
    meta.name = name.empty() ? std::string("New World") : name;
    meta.mode = static_cast<int>(CREATE_MODES[std::clamp(m_create.mode, 0, 2)]);
    meta.cheats = createCheats();
    meta.type = m_create.type;
    meta.structures = m_create.structures;
    meta.bonusChest = m_create.bonusChest && m_create.mode != 1;
    meta.difficulty = m_create.mode == 1 ? Worlds::Difficulty::Hard : m_create.difficulty;
    if (meta.type == Worlds::Type::Flat)
    {
        meta.layers = m_create.layers;
        meta.biome = m_create.flatBiome;
    }
    else if (meta.type == Worlds::Type::SingleBiome)
        meta.biome = m_create.biome;
    meta.lastPlayed = Worlds::nowMillis();
    Worlds::writeMeta(m_savePath, meta);
    m_worldMeta = meta;
    m_allowCheats = meta.cheats;
    const uint32_t random = static_cast<uint32_t>(SDL_GetPerformanceCounter() ^ 0x9E3779B9u);
    m_seedChoice = Worlds::seedFrom(m_createSeed.text, random);
    m_seedChosen = true;
    startWorld(static_cast<GameMode>(meta.mode), true);
}

void Application::placeBonusChest()
{
    m_bonusChestPending = false;
    if (!m_world) return;
    const glm::ivec3 feet = glm::ivec3(glm::floor(m_player.position));
    glm::ivec3 spot(0);
    bool found = false;
    for (int r = 2; r <= 6 && !found; ++r)
        for (int step = 0; step < 8 && !found; ++step)
        {
            const int x = feet.x + static_cast<int>(std::round(std::cos(step * 0.785f) * r));
            const int z = feet.z + static_cast<int>(std::round(std::sin(step * 0.785f) * r));
            for (int y = feet.y + 3; y >= feet.y - 4; --y)
                if (isSolid(m_world->getBlock(x, y - 1, z)) && !isSolid(m_world->getBlock(x, y, z)) &&
                    !isLiquid(m_world->getBlock(x, y, z)) && !isLiquid(m_world->getBlock(x, y - 1, z)))
                {
                    spot = glm::ivec3(x, y, z);
                    found = true;
                    break;
                }
        }
    if (!found) return;
    m_world->setBlock(spot.x, spot.y, spot.z, Blocks::Chest);
    m_net.onLocalBlockChange(spot.x, spot.y, spot.z, Blocks::Chest);
    const glm::ivec3 sides[4] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    for (const glm::ivec3& side : sides)
    {
        const glm::ivec3 at = spot + side;
        if (!isSolid(m_world->getBlock(at.x, at.y, at.z)) && isSolid(m_world->getBlock(at.x, at.y - 1, at.z)) &&
            m_world->getBlock(at.x, at.y, at.z) == Blocks::Air)
        {
            m_world->setBlock(at.x, at.y, at.z, Blocks::Torch);
            m_net.onLocalBlockChange(at.x, at.y, at.z, Blocks::Torch);
        }
    }
    uint32_t roll = m_world->seed() * 2654435761u + 0x51ED27u;
    auto next = [&](uint32_t range) {
        roll ^= roll << 13;
        roll ^= roll >> 17;
        roll ^= roll << 5;
        return range ? roll % range : 0u;
    };
    Chests::Contents& contents = m_chests.at(spot);
    std::vector<ItemStack> loot;
    loot.push_back(ItemStack{ next(4) == 0 ? static_cast<StackId>(Items::StoneAxe) : static_cast<StackId>(Items::WoodAxe), 1 });
    loot.push_back(ItemStack{ next(4) == 0 ? static_cast<StackId>(Items::StonePickaxe) : static_cast<StackId>(Items::WoodPickaxe), 1 });
    const StackId FOOD[3] = { Items::Apple, Items::Bread, Items::RawSalmon };
    const uint32_t FOOD_WEIGHT[3] = { 5, 3, 3 };
    for (int i = 0; i < 3; ++i)
    {
        uint32_t pick = next(11);
        int k = 0;
        while (pick >= FOOD_WEIGHT[k]) pick -= FOOD_WEIGHT[k++];
        loot.push_back(ItemStack{ FOOD[k], 1 + static_cast<int>(next(2)) });
    }
    const StackId LOGS[7] = { Blocks::Log, Blocks::SpruceLog, Blocks::BirchLog, Blocks::JungleLog, Blocks::AcaciaLog, Blocks::DarkOakLog, Blocks::MangroveLog };
    for (int i = 0; i < 4; ++i)
    {
        const uint32_t pick = next(41);
        if (pick < 10) loot.push_back(ItemStack{ Items::Stick, 1 + static_cast<int>(next(12)) });
        else if (pick < 20) loot.push_back(ItemStack{ Blocks::Planks, 1 + static_cast<int>(next(12)) });
        else loot.push_back(ItemStack{ LOGS[std::min<uint32_t>(6, (pick - 20) / 3)], 1 + static_cast<int>(next(3)) });
    }
    for (const ItemStack& stack : loot)
    {
        for (int tries = 0; tries < 40; ++tries)
        {
            ItemStack& slot = contents[next(static_cast<uint32_t>(contents.size()))];
            if (!slot.empty()) continue;
            slot = stack;
            break;
        }
    }
}

void Application::renderSingleplayerScreen()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    menuScreenBackdrop(w, h);

    std::vector<int> shown;
    for (size_t i = 0; i < m_worldEntries.size(); ++i)
        if (m_worldSearch.text.empty() || Worlds::matches(m_worldEntries[i], m_worldSearch.text)) shown.push_back(static_cast<int>(i));

    const float half = std::floor(w / 2.0f);
    const std::string before = m_worldSearch.text;
    guiList(m_worldList, 48.0f, std::max(0.0f, h - 112.0f), 36.0f, 270.0f, static_cast<int>(shown.size()), 0.0f,
            [&](int index, float x, float y, bool hovered) {
                const Worlds::Entry& entry = m_worldEntries[static_cast<size_t>(shown[static_cast<size_t>(index)])];
                const std::string icon = entry.icon ? "file:" + entry.path + "/icon.png" : std::string("file:assets/textures/misc/unknown_server.png");
                if (!guiSprite(icon.c_str(), x, y, 32.0f, 32.0f))
                {
                    m_ui.quad(x, y, 32.0f, 32.0f, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
                    m_ui.quad(x + 1.0f, y + 1.0f, 30.0f, 30.0f, glm::vec4(0.35f, 0.35f, 0.35f, 1.0f));
                    m_ui.text("?", x + 14.0f, y + 12.0f, 1.0f, WORLD_WHITE);
                }
                std::string name = entry.meta.name;
                if (name.empty()) name = "World " + std::to_string(index + 1);
                std::string folder = entry.folder;
                if (entry.meta.lastPlayed > 0) folder += " (" + Worlds::formatDate(entry.meta.lastPlayed) + ")";
                m_ui.text(name, x + 35.0f, y + 1.0f, 1.0f, WORLD_WHITE);
                m_ui.text(folder, x + 35.0f, y + 12.0f, 1.0f, WORLD_GRAY);
                const bool hardcore = static_cast<GameMode>(entry.meta.mode) == GameMode::Hardcore;
                const std::string mode = modeDetail(entry.meta.mode);
                const std::string rest = std::string(entry.meta.cheats ? ", Cheats" : "") + ", Version: 0.1";
                m_ui.text(mode, x + 35.0f, y + 21.0f, 1.0f, hardcore ? WORLD_RED : WORLD_GRAY);
                m_ui.text(rest, x + 35.0f + UIRenderer::textWidth(mode, 1.0f), y + 21.0f, 1.0f, WORLD_GRAY);
                if (hovered)
                {
                    m_ui.quad(x, y, 32.0f, 32.0f, glm::vec4(0x90 / 255.0f, 0x90 / 255.0f, 0x90 / 255.0f, 0xA0 / 255.0f));
                    const bool onIcon = mouseX - x < 32.0f;
                    if (!guiSprite(onIcon ? "sprites/world_list/join_highlighted" : "sprites/world_list/join", x, y, 32.0f, 32.0f))
                        for (int r = 0; r < 7; ++r)
                            m_ui.quad(x + 12.0f + static_cast<float>(r), y + 9.0f + static_cast<float>(r), 1.0f,
                                      14.0f - 2.0f * static_cast<float>(r), onIcon ? WORLD_WHITE : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
                }
            });
    const int selected = m_worldList.selected >= 0 && m_worldList.selected < static_cast<int>(shown.size())
                             ? shown[static_cast<size_t>(m_worldList.selected)] : -1;
    if (m_worldList.clicked >= 0 && selected >= 0)
    {
        const float rowLeft = half - 135.0f + 2.0f;
        if (m_worldList.doubleClicked || mouseX - rowLeft <= 32.0f)
        {
            m_audio.play(Sound::Click, 0.45f);
            m_pendingWorldFolder = m_worldEntries[static_cast<size_t>(selected)].folder;
            m_pendingAction = MenuAction::PlayWorld;
        }
    }
    if (m_input.wasKeyPressed(SDL_SCANCODE_RETURN) && selected >= 0 && !m_automated)
    {
        m_pendingWorldFolder = m_worldEntries[static_cast<size_t>(selected)].folder;
        m_pendingAction = MenuAction::PlayWorld;
    }

    const std::string title = "Select World";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 8.0f, 1.0f, WORLD_WHITE);
    guiField(m_worldSearch, half - 100.0f, 22.0f, 200.0f, 20.0f);
    if (m_worldSearch.text != before)
    {
        m_worldList.selected = -1;
        m_worldList.scroll = 0.0f;
    }

    if (guiButton(half - 154.0f, h - 52.0f, 150.0f, 20.0f, "Play Selected World", selected >= 0))
    {
        m_pendingWorldFolder = m_worldEntries[static_cast<size_t>(selected)].folder;
        m_pendingAction = MenuAction::PlayWorld;
    }
    if (guiButton(half + 4.0f, h - 52.0f, 150.0f, 20.0f, "Create New World")) openCreateWorld(nullptr);
    if (guiButton(half - 154.0f, h - 28.0f, 72.0f, 20.0f, "Edit", selected >= 0))
    {
        const Worlds::Entry& entry = m_worldEntries[static_cast<size_t>(selected)];
        m_editFolder = entry.folder;
        m_editName = GuiField{};
        m_editName.text = entry.meta.name;
        m_editName.focused = true;
        m_editName.focusTime = SDL_GetTicks();
        m_screen = Screen::EditWorld;
    }
    if (guiButton(half - 76.0f, h - 28.0f, 72.0f, 20.0f, "Delete", selected >= 0))
    {
        m_editFolder = m_worldEntries[static_cast<size_t>(selected)].folder;
        m_editNote = m_worldEntries[static_cast<size_t>(selected)].meta.name;
        m_screen = Screen::ConfirmDelete;
    }
    if (guiButton(half + 4.0f, h - 28.0f, 72.0f, 20.0f, "Re-Create", selected >= 0))
        openCreateWorld(&m_worldEntries[static_cast<size_t>(selected)]);
    if (guiButton(half + 82.0f, h - 28.0f, 72.0f, 20.0f, "Back")) m_pendingAction = MenuAction::BackToTitle;
    m_ui.setScale(1.0f);
}

void Application::renderConfirmDelete()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const std::string title = m_deletingServer ? "Are you sure you want to remove this server?" : "Are you sure you want to delete this world?";
    const std::vector<std::string> lines = wrapText("'" + m_editNote + "' will be lost forever! (A long time!)", w - 50.0f);
    const float messagesHeight = static_cast<float>(lines.size()) * 9.0f;
    const float titleY = std::clamp(std::floor((h - messagesHeight) / 2.0f) - 29.0f, 10.0f, 80.0f);
    const float messageY = titleY + 20.0f;
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), titleY, 1.0f, WORLD_WHITE);
    for (size_t i = 0; i < lines.size(); ++i)
        m_ui.textWithShadow(lines[i], std::floor((w - UIRenderer::textWidth(lines[i], 1.0f)) / 2.0f), messageY + 9.0f * static_cast<float>(i), 1.0f, WORLD_WHITE);
    const float buttonsY = std::clamp(messageY + messagesHeight + 20.0f, std::floor(h / 6.0f) + 96.0f, h - 24.0f);
    const float half = std::floor(w / 2.0f);
    if (m_deletingServer)
    {
        if (guiButton(half - 155.0f, buttonsY, 150.0f, 20.0f, "Delete"))
        {
            if (m_editServer >= 0 && m_editServer < static_cast<int>(m_servers.size()))
                m_servers.erase(m_servers.begin() + m_editServer);
            saveServers();
            refreshPings();
            m_serverList.selected = -1;
            m_deletingServer = false;
            m_screen = Screen::Multiplayer;
        }
        if (guiButton(half + 5.0f, buttonsY, 150.0f, 20.0f, "Cancel"))
        {
            m_deletingServer = false;
            m_screen = Screen::Multiplayer;
        }
        m_ui.setScale(1.0f);
        return;
    }
    if (guiButton(half - 155.0f, buttonsY, 150.0f, 20.0f, "Delete"))
    {
        std::error_code ec;
        std::filesystem::remove_all(savesDirectory() + "/" + m_editFolder, ec);
        refreshWorlds();
        m_worldList.selected = -1;
        flushSaveStorage();
        m_screen = Screen::Singleplayer;
    }
    if (guiButton(half + 5.0f, buttonsY, 150.0f, 20.0f, "Cancel")) m_screen = Screen::Singleplayer;
    m_ui.setScale(1.0f);
}

void Application::renderEditWorld()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const std::string path = savesDirectory() + "/" + m_editFolder;
    std::error_code ec;
    const bool hasIcon = std::filesystem::exists(path + "/icon.png", ec);
    const float x0 = std::floor((w - 200.0f) / 2.0f);
    const float y0 = std::floor((h - 234.0f) / 2.0f);
    const std::string title = "Edit World";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 15.0f, 1.0f, WORLD_WHITE);
    m_ui.textWithShadow("World Name", x0, y0 + 25.0f, 1.0f, WORLD_WHITE);
    guiField(m_editName, x0, y0 + 39.0f, 200.0f, 20.0f);
#ifdef __EMSCRIPTEN__
    const bool folders = false;
#else
    const bool folders = true;
#endif
    if (guiButton(x0, y0 + 64.0f, 200.0f, 20.0f, "Reset Icon", hasIcon))
    {
        std::filesystem::remove(path + "/icon.png", ec);
        m_gui.forget("file:" + path + "/icon.png");
        flushSaveStorage();
    }
    if (guiButton(x0, y0 + 89.0f, 200.0f, 20.0f, "Open World Folder", folders))
    {
#ifndef __EMSCRIPTEN__
        const std::string url = "file://" + std::filesystem::absolute(path, ec).string();
        SDL_OpenURL(url.c_str());
#endif
    }
    if (guiButton(x0, y0 + 114.0f, 200.0f, 20.0f, "Make Backup"))
    {
        char stamp[32];
        const std::time_t now = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &local);
        const std::string target = m_configPath + "/backups/" + stamp + "_" + m_editFolder;
        std::filesystem::create_directories(target, ec);
        std::filesystem::copy(path, target, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
        Toast toast;
        toast.systemTitle = ec ? "Backup failed" : "Backed up: " + m_editName.text;
        m_toasts.push_back(toast);
        flushSaveStorage();
        refreshWorlds();
        m_screen = Screen::Singleplayer;
    }
    if (guiButton(x0, y0 + 139.0f, 200.0f, 20.0f, "Open Backups Folder", folders))
    {
        std::filesystem::create_directories(m_configPath + "/backups", ec);
#ifndef __EMSCRIPTEN__
        const std::string url = "file://" + std::filesystem::absolute(m_configPath + "/backups", ec).string();
        SDL_OpenURL(url.c_str());
#endif
    }
    guiButton(x0, y0 + 164.0f, 200.0f, 20.0f, "Optimize World", false);
    bool blank = true;
    for (char c : m_editName.text) blank = blank && std::isspace(static_cast<unsigned char>(c));
    const bool save = guiButton(x0, y0 + 214.0f, 98.0f, 20.0f, "Save", !blank) ||
                      (!blank && !m_automated && m_input.wasKeyPressed(SDL_SCANCODE_RETURN));
    if (save)
    {
        Worlds::Meta meta;
        if (!Worlds::readMeta(path, meta)) meta.name = m_editFolder;
        meta.name = m_editName.text;
        Worlds::writeMeta(path, meta);
        flushSaveStorage();
        refreshWorlds();
        m_screen = Screen::Singleplayer;
    }
    if (guiButton(x0 + 102.0f, y0 + 214.0f, 98.0f, 20.0f, "Cancel")) m_screen = Screen::Singleplayer;
    m_ui.setScale(1.0f);
}

void Application::renderCreateWorld()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    const bool clicked = !m_automated && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT);
    const bool inWorld = m_world && !m_menuWorld;

    if (!guiTiled("tab_header_background", 0.0f, 0.0f, w, 24.0f, 16.0f))
        m_ui.quad(0.0f, 0.0f, w, 24.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));
    if (!guiTiled(inWorld ? "inworld_menu_background" : "menu_background", 0.0f, 24.0f, w, h - 24.0f, 32.0f))
        m_ui.quad(0.0f, 24.0f, w, h - 24.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));

    const float navWidth = std::min(400.0f, w) - 28.0f;
    const float tabWidth = std::ceil(std::floor(navWidth / 3.0f) / 2.0f) * 2.0f;
    const float navX = std::ceil(std::floor((w - navWidth) / 2.0f) / 2.0f) * 2.0f;
    auto separator = [&](const char* path, float x, float y, float sw) {
        if (sw <= 0.0f) return;
        if (guiHas(path))
        {
            for (float tx = x; tx < x + sw; tx += 32.0f) guiBlit(path, tx, y, 0.0f, 0.0f, std::min(32.0f, x + sw - tx), 2.0f, 32.0f, 2.0f);
            return;
        }
        m_ui.quad(x, y, sw, 1.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));
        m_ui.quad(x, y + 1.0f, sw, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.25f));
    };
    separator("header_separator", 0.0f, 22.0f, navX);
    separator("header_separator", navX + tabWidth * 3.0f, 22.0f, w - navX - tabWidth * 3.0f);
    for (int t = 0; t < 3; ++t)
    {
        const float tx = navX + tabWidth * static_cast<float>(t);
        const bool current = m_create.tab == t;
        const bool over = mouseX >= tx && mouseX < tx + tabWidth && mouseY >= 0.0f && mouseY < 24.0f;
        if (over && clicked && !current)
        {
            m_create.tab = t;
            m_audio.play(Sound::Click, 0.45f);
        }
        const char* sprite = current ? (over ? "sprites/widget/tab_selected_highlighted" : "sprites/widget/tab_selected")
                                     : (over ? "sprites/widget/tab_highlighted" : "sprites/widget/tab");
        if (!guiSprite(sprite, tx, 0.0f, tabWidth, 24.0f))
        {
            m_ui.quad(tx, 0.0f, tabWidth, 24.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
            m_ui.quad(tx + 1.0f, 1.0f, tabWidth - 2.0f, 23.0f, current ? glm::vec4(0.3f, 0.3f, 0.3f, 0.9f) : glm::vec4(0.18f, 0.18f, 0.18f, 0.9f));
        }
        const float labelW = UIRenderer::textWidth(CREATE_TABS[t], 1.0f);
        const float ty = current ? 8.0f : 10.0f;
        m_ui.textWithShadow(CREATE_TABS[t], std::floor(tx + (tabWidth - labelW) / 2.0f), ty, 1.0f, WORLD_WHITE);
        if (current)
        {
            const float lineW = std::min(labelW, tabWidth - 4.0f);
            m_ui.quad(std::floor(tx + (tabWidth - lineW) / 2.0f), 22.0f, lineW, 1.0f, WORLD_WHITE);
        }
    }
    if (!m_automated && m_input.wasKeyPressed(SDL_SCANCODE_TAB) &&
        (m_input.isKeyDown(SDL_SCANCODE_LCTRL) || m_input.isKeyDown(SDL_SCANCODE_RCTRL)))
        m_create.tab = (m_create.tab + 1) % 3;

    const float areaTop = 24.0f;
    const float areaH = h - 33.0f - areaTop;
    std::string tip;
    float tipWidth = 170.0f;
    const bool hardcore = m_create.mode == 1;
    if (m_create.tab == 0)
    {
        const float gridH = 117.0f;
        const float gx = std::floor((w - 210.0f) / 2.0f);
        const float gy = areaTop + std::floor((areaH - gridH) / 6.0f);
        m_ui.textWithShadow("World Name", gx + 1.0f, gy, 1.0f, WORLD_WHITE);
        guiField(m_createName, gx + 1.0f, gy + 13.0f, 208.0f, 20.0f);
        if (mouseX >= gx + 1.0f && mouseX < gx + 209.0f && mouseY >= gy + 13.0f && mouseY < gy + 33.0f)
            tip = "Save folder: " + Worlds::uniqueFolder(savesDirectory(), m_createName.text);
        const float modeY = gy + 41.0f;
        if (guiCycle(gx, modeY, 210.0f, std::string("Game Mode: ") + CREATE_MODE_NAMES[m_create.mode]))
            m_create.mode = (m_create.mode + 1) % 3;
        if (mouseX >= gx && mouseX < gx + 210.0f && mouseY >= modeY && mouseY < modeY + 20.0f) tip = CREATE_MODE_INFO[m_create.mode];
        const Worlds::Difficulty shownDifficulty = hardcore ? Worlds::Difficulty::Hard : m_create.difficulty;
        const float difficultyY = modeY + 28.0f;
        if (guiCycle(gx, difficultyY, 210.0f, std::string("Difficulty: ") + Worlds::difficultyName(shownDifficulty), !hardcore))
            m_create.difficulty = static_cast<Worlds::Difficulty>((static_cast<int>(m_create.difficulty) + 1) % 4);
        if (mouseX >= gx && mouseX < gx + 210.0f && mouseY >= difficultyY && mouseY < difficultyY + 20.0f)
            tip = Worlds::difficultyInfo(shownDifficulty);
        const float cheatsY = difficultyY + 28.0f;
        if (guiCycle(gx, cheatsY, 210.0f, std::string("Allow Commands: ") + (createCheats() ? "ON" : "OFF"), !hardcore))
            m_create.cheats = createCheats() ? 0 : 1;
        if (mouseX >= gx && mouseX < gx + 210.0f && mouseY >= cheatsY && mouseY < cheatsY + 20.0f)
            tip = "Commands like /gamemode, /experience";
    }
    else if (m_create.tab == 1)
    {
        const float gridH = 117.0f;
        const float gx = std::floor((w - 310.0f) / 2.0f);
        const float gy = areaTop + std::floor((areaH - gridH) / 6.0f);
        if (guiCycle(gx, gy, 150.0f, std::string("World Type: ") + Worlds::typeName(m_create.type)))
            m_create.type = static_cast<Worlds::Type>((static_cast<int>(m_create.type) + 1) % Worlds::TYPE_COUNT);
        if (m_create.type == Worlds::Type::Amplified && mouseX >= gx && mouseX < gx + 150.0f && mouseY >= gy && mouseY < gy + 20.0f)
            tip = "Notice: Just for fun! Requires a beefy computer.";
        const bool customizable = m_create.type == Worlds::Type::Flat || m_create.type == Worlds::Type::SingleBiome;
        if (guiButton(gx + 160.0f, gy, 150.0f, 20.0f, "Customize", customizable))
        {
            if (m_create.type == Worlds::Type::Flat)
            {
                m_flatDraft = m_create.layers;
                m_flatBiomeDraft = m_create.flatBiome;
                m_flatList = GuiList{};
                m_screen = Screen::CustomizeFlat;
            }
            else
            {
                m_biomeDraft = m_create.biome;
                m_biomeList = GuiList{};
                m_screen = Screen::CustomizeBiome;
            }
        }
        const float seedY = gy + 28.0f;
        m_ui.textWithShadow("Seed for the world generator", gx + 1.0f, seedY, 1.0f, WORLD_WHITE);
        guiField(m_createSeed, gx + 1.0f, seedY + 13.0f, 308.0f, 20.0f, "Leave blank for a random seed");
        const float optionsY = seedY + 41.0f;
        const char* const NAMES[2] = { "Generate Structures", "Bonus Chest" };
        for (int i = 0; i < 2; ++i)
        {
            const float rowY = optionsY + 4.0f + 24.0f * static_cast<float>(i);
            const bool value = i == 0 ? m_create.structures : (m_create.bonusChest && !hardcore);
            const bool enabled = i == 0 || !hardcore;
            m_ui.textWithShadow(NAMES[i], gx, rowY + 6.0f, 1.0f, WORLD_WHITE);
            if (guiButton(gx + 266.0f, rowY, 44.0f, 20.0f, value ? "ON" : "OFF", enabled))
            {
                if (i == 0) m_create.structures = !m_create.structures;
                else m_create.bonusChest = !m_create.bonusChest;
            }
            if (i == 0 && mouseX >= gx + 266.0f && mouseX < gx + 310.0f && mouseY >= rowY && mouseY < rowY + 20.0f)
                tip = "Villages, Shipwrecks, etc.";
        }
    }
    else
    {
        const float gridH = 76.0f;
        const float gx = std::floor((w - 210.0f) / 2.0f);
        const float gy = areaTop + std::floor((areaH - gridH) / 6.0f);
        guiButton(gx, gy, 210.0f, 20.0f, "Game Rules", false);
        guiButton(gx, gy + 28.0f, 210.0f, 20.0f, "Experiments", false);
        guiButton(gx, gy + 56.0f, 210.0f, 20.0f, "Data Packs", false);
    }

    separator("footer_separator", 0.0f, h - 33.0f - 2.0f, w);
    const float footerX = std::floor((w - 308.0f) / 2.0f);
    const float footerY = h - 27.0f;
    const bool create = guiButton(footerX, footerY, 150.0f, 20.0f, "Create New World") ||
                        (!m_automated && m_input.wasKeyPressed(SDL_SCANCODE_RETURN));
    if (create) m_pendingAction = MenuAction::CreateWorldNow;
    if (guiButton(footerX + 158.0f, footerY, 150.0f, 20.0f, "Cancel"))
    {
        if (m_create.fromTitle) m_pendingAction = MenuAction::BackToTitle;
        else
        {
            refreshWorlds();
            m_screen = Screen::Singleplayer;
        }
    }
    if (!tip.empty()) wrappedTooltip(tip, mouseX, mouseY, tipWidth);
    m_ui.setScale(1.0f);
}

void Application::renderCustomizeFlat()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    std::vector<Worlds::Layer> layers = Worlds::parseLayers(m_flatDraft);
    const int count = static_cast<int>(layers.size());
    guiList(m_flatList, 43.0f, std::max(0.0f, h - 103.0f), 24.0f, 220.0f, count, 0.0f, [&](int index, float x, float y, bool) {
        const Worlds::Layer& layer = layers[static_cast<size_t>(count - index - 1)];
        const std::string id = shortBlockId(layer.block);
        StackId stack = id == "air" ? static_cast<StackId>(Blocks::Air) : stackNamed(id);
        if (id == "water") stack = Items::WaterBucket;
        if (id == "lava") stack = Items::LavaBucket;
        if (!guiSprite("sprites/container/slot", x + 1.0f, y + 1.0f, 18.0f, 18.0f)) guiWell(x + 1.0f, y + 1.0f, 18.0f, 18.0f);
        if (stack != Blocks::Air) drawItem(x + 2.0f, y + 2.0f, SLOT, stack, 1, glm::vec4(1.0f));
        const std::string name = stack != Blocks::Air ? std::string(displayName(id == "water" ? static_cast<StackId>(Blocks::Water)
                                                                    : id == "lava" ? static_cast<StackId>(Blocks::Lava) : stack))
                                                      : std::string("Air");
        m_ui.text(name, x + 23.0f, y + 3.0f, 1.0f, WORLD_WHITE);
        std::string thickness = std::to_string(layer.count);
        if (index == 0) thickness = "Top - " + thickness;
        else if (index == count - 1) thickness = "Bottom - " + thickness;
        m_ui.text(thickness, x + 2.0f + 213.0f - UIRenderer::textWidth(thickness, 1.0f), y + 3.0f, 1.0f, WORLD_WHITE);
    });
    const std::string title = "Superflat Customization";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 8.0f, 1.0f, WORLD_WHITE);
    const float left = std::floor(w / 2.0f) - 92.0f - 16.0f;
    m_ui.textWithShadow("Layer Material", left, 32.0f, 1.0f, WORLD_WHITE);
    m_ui.textWithShadow("Height", left + 2.0f + 213.0f - UIRenderer::textWidth("Height", 1.0f), 32.0f, 1.0f, WORLD_WHITE);
    const float half = std::floor(w / 2.0f);
    const bool chosen = m_flatList.selected >= 0 && m_flatList.selected < count;
    if (guiButton(half - 155.0f, h - 52.0f, 150.0f, 20.0f, "Remove Layer", chosen))
    {
        const int index = m_flatList.selected;
        layers.erase(layers.begin() + (count - index - 1));
        m_flatDraft = Worlds::formatLayers(layers);
        m_flatList.selected = layers.empty() ? -1 : std::min(index, static_cast<int>(layers.size()) - 1);
    }
    if (guiButton(half + 5.0f, h - 52.0f, 150.0f, 20.0f, "Presets"))
    {
        m_presetField = GuiField{};
        m_presetField.maxLength = 1230;
        m_presetField.text = Worlds::presetCode(m_flatDraft, m_flatBiomeDraft);
        m_presetList = GuiList{};
        m_screen = Screen::FlatPresets;
    }
    if (guiButton(half - 155.0f, h - 28.0f, 150.0f, 20.0f, "Done"))
    {
        m_create.layers = m_flatDraft;
        m_create.flatBiome = m_flatBiomeDraft;
        m_screen = Screen::CreateWorld;
    }
    if (guiButton(half + 5.0f, h - 28.0f, 150.0f, 20.0f, "Cancel")) m_screen = Screen::CreateWorld;
    m_ui.setScale(1.0f);
}

void Application::renderFlatPresets()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const std::vector<Worlds::FlatPreset>& presets = Worlds::flatPresets();
    guiList(m_presetList, 80.0f, std::max(0.0f, h - 117.0f), 24.0f, 220.0f, static_cast<int>(presets.size()), 0.0f,
            [&](int index, float x, float y, bool) {
                const Worlds::FlatPreset& preset = presets[static_cast<size_t>(index)];
                if (!guiSprite("sprites/container/slot", x + 1.0f, y + 1.0f, 18.0f, 18.0f)) guiWell(x + 1.0f, y + 1.0f, 18.0f, 18.0f);
                const StackId icon = stackNamed(preset.icon);
                if (icon != Blocks::Air) drawItem(x + 2.0f, y + 2.0f, SLOT, icon, 1, glm::vec4(1.0f));
                m_ui.text(preset.name, x + 23.0f, y + 6.0f, 1.0f, WORLD_WHITE);
            });
    if (m_presetList.clicked >= 0)
    {
        const Worlds::FlatPreset& preset = presets[static_cast<size_t>(m_presetList.clicked)];
        m_presetField.text = Worlds::presetCode(preset.layers, preset.biome);
    }
    const std::string title = "Select a Preset";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 8.0f, 1.0f, WORLD_WHITE);
    m_ui.textWithShadow("Want to share your preset with someone? Use the box below!", 51.0f, 30.0f, 1.0f, WORLD_LABEL);
    m_ui.textWithShadow("Alternatively, here's some we made earlier!", 51.0f, 68.0f, 1.0f, WORLD_LABEL);
    guiField(m_presetField, 50.0f, 40.0f, w - 100.0f, 20.0f);
    const float half = std::floor(w / 2.0f);
    std::string layers, biome;
    const bool valid = Worlds::parsePresetCode(m_presetField.text, layers, biome);
    if (guiButton(half - 155.0f, h - 28.0f, 150.0f, 20.0f, "Use Preset", m_presetList.selected >= 0 || valid) && valid)
    {
        m_flatDraft = layers;
        m_flatBiomeDraft = biome;
        m_flatList = GuiList{};
        m_screen = Screen::CustomizeFlat;
    }
    if (guiButton(half + 5.0f, h - 28.0f, 150.0f, 20.0f, "Cancel")) m_screen = Screen::CustomizeFlat;
    m_ui.setScale(1.0f);
}

void Application::renderCustomizeBiome()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    std::vector<Biome> biomes = { Biome::Badlands, Biome::Beach, Biome::CherryGrove, Biome::CrimsonForest, Biome::Desert, Biome::Forest,
                                  Biome::Jungle, Biome::MangroveSwamp, Biome::MushroomFields, Biome::NetherWastes, Biome::Ocean,
                                  Biome::Plains, Biome::Savanna, Biome::Snowy, Biome::Swamp, Biome::WarpedForest, Biome::Mountains };
    std::sort(biomes.begin(), biomes.end(), [](Biome a, Biome b) { return std::string(biomeTitle(a)) < std::string(biomeTitle(b)); });
    if (m_biomeList.selected < 0)
        for (size_t i = 0; i < biomes.size(); ++i)
            if (biomeId(biomes[i]) == m_biomeDraft) m_biomeList.selected = static_cast<int>(i);
    guiList(m_biomeList, 40.0f, std::max(0.0f, h - 77.0f), 16.0f, 220.0f, static_cast<int>(biomes.size()), 0.0f,
            [&](int index, float x, float y, bool) {
                m_ui.textWithShadow(biomeTitle(biomes[static_cast<size_t>(index)]), x + 5.0f, y + 2.0f, 1.0f, WORLD_WHITE);
            });
    if (m_biomeList.selected >= 0) m_biomeDraft = biomeId(biomes[static_cast<size_t>(m_biomeList.selected)]);
    const std::string title = "Buffet world customization";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 8.0f, 1.0f, WORLD_WHITE);
    const std::string prompt = "Please select a biome";
    m_ui.textWithShadow(prompt, std::floor((w - UIRenderer::textWidth(prompt, 1.0f)) / 2.0f), 28.0f, 1.0f, WORLD_GRAY);
    const float half = std::floor(w / 2.0f);
    if (guiButton(half - 155.0f, h - 28.0f, 150.0f, 20.0f, "Done", m_biomeList.selected >= 0))
    {
        m_create.biome = m_biomeDraft;
        m_screen = Screen::CreateWorld;
    }
    if (guiButton(half + 5.0f, h - 28.0f, 150.0f, 20.0f, "Cancel")) m_screen = Screen::CreateWorld;
    m_ui.setScale(1.0f);
}
