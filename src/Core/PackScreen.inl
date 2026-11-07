namespace
{
    std::string trimmedName(const std::string& name)
    {
        if (UIRenderer::textWidth(name, 1.0f) <= 157.0f) return name;
        std::string out = name;
        while (!out.empty() && UIRenderer::textWidth(out + "...", 1.0f) > 157.0f) Utf8::popBack(out);
        return out + "...";
    }
}

void Application::openPacks()
{
    const std::vector<Packs::Info> found = Packs::discover(m_configPath);
    m_packsSelected.clear();
    m_packsAvailable.clear();
    for (const std::string& id : m_packSelection)
        for (const Packs::Info& info : found)
            if (info.id == id) m_packsSelected.push_back(info);
    for (const Packs::Info& info : found)
    {
        bool chosen = false;
        for (const Packs::Info& picked : m_packsSelected) chosen = chosen || picked.id == info.id;
        if (!chosen) m_packsAvailable.push_back(info);
    }
    m_availableList = GuiList{};
    m_selectedList = GuiList{};
    m_packConfirm = -1;
    m_packsReturn = m_screen;
    m_screen = Screen::Packs;
}

void Application::applyPacks()
{
    std::vector<std::string> ids;
    for (const Packs::Info& info : m_packsSelected) ids.push_back(info.id);
    const bool changed = ids != m_packSelection;
    m_packSelection = ids;
    saveOptions();
    if (changed) requestReload();
    else m_screen = m_packsReturn == Screen::Packs ? Screen::Settings : m_packsReturn;
}

void Application::requestReload()
{
    m_reloadWorld.clear();
    if (m_world && !m_menuWorld && m_net.role() != Net::Role::Guest)
    {
        saveLevel();
        m_reloadWorld = std::filesystem::path(m_savePath).filename().string();
    }
    m_net.leave();
    m_reloadRequested = true;
    m_running = false;
}

void Application::renderPacks()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    menuScreenBackdrop(w, h);
    const float half = std::floor(w / 2.0f);

    for (const std::string& dropped : m_input.droppedFiles())
    {
        std::error_code ec;
        const std::filesystem::path source(dropped);
        if (!std::filesystem::is_directory(source, ec) || !std::filesystem::exists(source / "pack.mcmeta", ec)) continue;
        const std::filesystem::path target = std::filesystem::path(Packs::directory(m_configPath)) / source.filename();
        std::filesystem::copy(source, target, std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing, ec);
        const std::vector<Packs::Info> found = Packs::discover(m_configPath);
        for (const Packs::Info& info : found)
        {
            bool known = false;
            for (const Packs::Info& have : m_packsAvailable) known = known || have.id == info.id;
            for (const Packs::Info& have : m_packsSelected) known = known || have.id == info.id;
            if (!known) m_packsAvailable.push_back(info);
        }
    }

    if (m_packConfirm >= 0 && m_packConfirm < static_cast<int>(m_packsAvailable.size()))
    {
        const Packs::Info& pack = m_packsAvailable[static_cast<size_t>(m_packConfirm)];
        const std::string title = "Are you sure you want to load this pack?";
        const std::string message = pack.format < Packs::PACK_FORMAT
                                        ? "This pack was made for an older version of Minecraft and may no longer work correctly."
                                        : "This pack was made for a newer version of Minecraft and may not work correctly.";
        const std::vector<std::string> lines = wrapText(message, w - 50.0f);
        const float messagesHeight = static_cast<float>(lines.size()) * 9.0f;
        const float titleY = std::clamp(std::floor((h - messagesHeight) / 2.0f) - 29.0f, 10.0f, 80.0f);
        m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), titleY, 1.0f, glm::vec4(1.0f));
        for (size_t i = 0; i < lines.size(); ++i)
            m_ui.textWithShadow(lines[i], std::floor((w - UIRenderer::textWidth(lines[i], 1.0f)) / 2.0f), titleY + 20.0f + 9.0f * static_cast<float>(i), 1.0f, glm::vec4(1.0f));
        const float buttonsY = std::clamp(titleY + 20.0f + messagesHeight + 20.0f, std::floor(h / 6.0f) + 96.0f, h - 24.0f);
        if (guiButton(half - 155.0f, buttonsY, 150.0f, 20.0f, "Yes"))
        {
            m_packsSelected.insert(m_packsSelected.begin(), pack);
            m_packsAvailable.erase(m_packsAvailable.begin() + m_packConfirm);
            m_packConfirm = -1;
        }
        else if (guiButton(half + 5.0f, buttonsY, 150.0f, 20.0f, "No"))
            m_packConfirm = -1;
        m_ui.setScale(1.0f);
        return;
    }

    const std::string title = "Select Resource Packs";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 5.0f, 1.0f, glm::vec4(1.0f));
    const std::string drop = "Drag and drop files into this window to add packs";
    m_ui.textWithShadow(drop, std::floor((w - UIRenderer::textWidth(drop, 1.0f)) / 2.0f), 19.0f, 1.0f, glm::vec4(0xAA / 255.0f, 0xAA / 255.0f, 0xAA / 255.0f, 1.0f));

    const float listTop = 33.0f;
    const float listH = std::max(0.0f, h - 66.0f);
    const float header = 13.0f;
    auto drawPack = [&](const Packs::Info& pack, float x, float y, bool hovered, bool selectedSide, int index, int movable) {
        const bool compatible = pack.builtIn || pack.format == Packs::PACK_FORMAT;
        if (!compatible) m_ui.quad(x - 1.0f, y - 1.0f, 198.0f, 34.0f, glm::vec4(0x77 / 255.0f, 0.0f, 0.0f, 1.0f));
        if (pack.builtIn) drawItem(x, y, 32.0f, Blocks::Grass, 1, glm::vec4(1.0f));
        else
        {
            const std::string icon = pack.icon ? "file:" + pack.root + "/pack.png" : std::string("file:assets/textures/misc/unknown_pack.png");
            if (!guiSprite(icon.c_str(), x, y, 32.0f, 32.0f))
            {
                m_ui.quad(x, y, 32.0f, 32.0f, glm::vec4(0.3f, 0.2f, 0.35f, 1.0f));
                m_ui.text("?", x + 14.0f, y + 12.0f, 1.0f, glm::vec4(1.0f));
            }
        }
        std::string name = trimmedName(pack.name);
        std::string description = pack.description;
        if (hovered && !pack.builtIn)
        {
            m_ui.quad(x, y, 32.0f, 32.0f, glm::vec4(0x90 / 255.0f, 0x90 / 255.0f, 0x90 / 255.0f, 0xA0 / 255.0f));
            if (!compatible)
            {
                name = "Incompatible";
                description = pack.format < Packs::PACK_FORMAT ? "(Made for an older version of Minecraft)" : "(Made for a newer version of Minecraft)";
            }
            const float d = mouseX - x, e = mouseY - y;
            auto overlay = [&](const char* plain, const char* bright, bool lit, int shape) {
                if (guiSprite(lit ? bright : plain, x, y, 32.0f, 32.0f)) return;
                const glm::vec4 colour = lit ? glm::vec4(1.0f) : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                for (int r = 0; r < 5; ++r)
                {
                    const float fr = static_cast<float>(r);
                    if (shape == 0) m_ui.quad(x + 12.0f + fr, y + 11.0f + fr, 1.0f, 10.0f - 2.0f * fr, colour);
                    if (shape == 1) m_ui.quad(x + 9.0f - fr, y + 11.0f + fr, 1.0f, 10.0f - 2.0f * fr, colour);
                    if (shape == 2) m_ui.quad(x + 23.0f - fr, y + 4.0f + fr, 1.0f + 2.0f * fr, 1.0f, colour);
                    if (shape == 3) m_ui.quad(x + 23.0f - fr, y + 27.0f - fr, 1.0f + 2.0f * fr, 1.0f, colour);
                }
            };
            if (!selectedSide) overlay("sprites/transferable_list/select", "sprites/transferable_list/select_highlighted", d < 32.0f, 0);
            else
            {
                overlay("sprites/transferable_list/unselect", "sprites/transferable_list/unselect_highlighted", d < 16.0f, 1);
                if (index > 0)
                    overlay("sprites/transferable_list/move_up", "sprites/transferable_list/move_up_highlighted", d < 32.0f && d > 16.0f && e < 16.0f, 2);
                if (index < movable - 1)
                    overlay("sprites/transferable_list/move_down", "sprites/transferable_list/move_down_highlighted", d < 32.0f && d > 16.0f && e > 16.0f, 3);
            }
        }
        m_ui.textWithShadow(name, x + 34.0f, y + 1.0f, 1.0f, glm::vec4(1.0f));
        const std::vector<std::string> lines = wrapText(description, 157.0f);
        for (size_t i = 0; i < lines.size() && i < 2; ++i)
            m_ui.textWithShadow(lines[i], x + 34.0f, y + 12.0f + 10.0f * static_cast<float>(i), 1.0f,
                                glm::vec4(0x80 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f, 1.0f));
    };

    Packs::Info vanilla;
    vanilla.id = "vanilla";
    vanilla.name = "Default";
    vanilla.description = "The default look and feel of Minecraft";
    vanilla.builtIn = true;

    const float leftX = half - 215.0f;
    const float rightX = half + 15.0f;
    const int available = static_cast<int>(m_packsAvailable.size());
    const int chosen = static_cast<int>(m_packsSelected.size());
    guiList(m_availableList, listTop, listH, 36.0f, 200.0f, available, header, [&](int index, float x, float y, bool hovered) {
        drawPack(m_packsAvailable[static_cast<size_t>(index)], x, y, hovered, false, index, available);
    }, leftX, 200.0f);
    guiList(m_selectedList, listTop, listH, 36.0f, 200.0f, chosen + 1, header, [&](int index, float x, float y, bool hovered) {
        drawPack(index < chosen ? m_packsSelected[static_cast<size_t>(index)] : vanilla, x, y, hovered, true, index, chosen);
    }, rightX, 200.0f);
    auto heading = [&](const char* text, float x, const GuiList& list) {
        const float y = std::min(listTop + 3.0f, listTop + 4.0f - std::floor(list.scroll));
        if (y < listTop) return;
        const float tw = UIRenderer::textWidth(text, 1.0f) + 1.0f;
        const float tx = std::floor(x + 100.0f - tw / 2.0f);
        m_ui.text(text, tx, y, 1.0f, glm::vec4(1.0f));
        m_ui.text(text, tx + 1.0f, y, 1.0f, glm::vec4(1.0f));
        m_ui.quad(tx, y + 9.0f, tw, 1.0f, glm::vec4(1.0f));
    };
    heading("Available", leftX, m_availableList);
    heading("Selected", rightX, m_selectedList);

    if (m_availableList.clicked >= 0)
    {
        const int index = m_availableList.clicked;
        const float rowTop = listTop + 4.0f - std::floor(m_availableList.scroll) + 36.0f * static_cast<float>(index) + header;
        (void)rowTop;
        if (mouseX - (leftX + 2.0f) <= 32.0f)
        {
            const Packs::Info pack = m_packsAvailable[static_cast<size_t>(index)];
            if (pack.format != Packs::PACK_FORMAT) m_packConfirm = index;
            else
            {
                m_packsSelected.insert(m_packsSelected.begin(), pack);
                m_packsAvailable.erase(m_packsAvailable.begin() + index);
            }
            m_availableList.selected = -1;
        }
    }
    if (m_selectedList.clicked >= 0 && m_selectedList.clicked < chosen)
    {
        const int index = m_selectedList.clicked;
        const float rowTop = listTop + 4.0f - std::floor(m_selectedList.scroll) + 36.0f * static_cast<float>(index) + header;
        const float d = mouseX - (rightX + 2.0f), e = mouseY - rowTop;
        if (d <= 32.0f)
        {
            if (d < 16.0f)
            {
                m_packsAvailable.push_back(m_packsSelected[static_cast<size_t>(index)]);
                std::sort(m_packsAvailable.begin(), m_packsAvailable.end(), [](const Packs::Info& a, const Packs::Info& b) { return a.name < b.name; });
                m_packsSelected.erase(m_packsSelected.begin() + index);
            }
            else if (e < 16.0f && index > 0)
                std::swap(m_packsSelected[static_cast<size_t>(index)], m_packsSelected[static_cast<size_t>(index - 1)]);
            else if (e > 16.0f && index < chosen - 1)
                std::swap(m_packsSelected[static_cast<size_t>(index)], m_packsSelected[static_cast<size_t>(index + 1)]);
            m_selectedList.selected = -1;
        }
    }
    if (m_selectedList.clicked == chosen) m_selectedList.selected = -1;

    const float footerX = std::floor((w - 308.0f) / 2.0f);
    const float footerY = h - 27.0f;
#ifdef __EMSCRIPTEN__
    const bool folders = false;
#else
    const bool folders = true;
#endif
    if (guiButton(footerX, footerY, 150.0f, 20.0f, "Open Pack Folder", folders))
    {
#ifndef __EMSCRIPTEN__
        std::error_code ec;
        std::filesystem::create_directories(Packs::directory(m_configPath), ec);
        const std::string url = "file://" + std::filesystem::absolute(Packs::directory(m_configPath), ec).string();
        SDL_OpenURL(url.c_str());
#endif
    }
    if (guiButton(footerX + 158.0f, footerY, 150.0f, 20.0f, "Done")) applyPacks();
    if (mouseX >= footerX && mouseX < footerX + 150.0f && mouseY >= footerY && mouseY < footerY + 20.0f)
        drawTooltip("(Place pack files here)", mouseX, mouseY, true);
    m_ui.setScale(1.0f);
}
