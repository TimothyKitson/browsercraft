namespace
{
    const char* const LOADING_FRAMES[4] = { "O o o", "o O o", "o o O", "o O o" };
    const glm::vec4 SERVER_ADDRESS(0x30 / 255.0f, 0x30 / 255.0f, 0x30 / 255.0f, 1.0f);
}

void Application::saveServers()
{
    std::error_code ec;
    std::filesystem::create_directories(m_configPath, ec);
    Servers::save(m_configPath + "/servers.txt", m_servers);
    flushSaveStorage();
}

void Application::refreshPings()
{
    m_pings.assign(m_servers.size(), Net::Pinger{});
    for (size_t i = 0; i < m_servers.size(); ++i) m_pings[i].start(m_servers[i].address);
}

void Application::openServerList()
{
    m_servers = Servers::load(m_configPath + "/servers.txt");
    refreshPings();
    m_serverList = GuiList{};
    m_screen = Screen::Multiplayer;
#ifndef __EMSCRIPTEN__
    m_discovery.startListening();
#endif
}

void Application::joinAddress(const std::string& address)
{
    m_addressDraft = address;
    m_pendingAction = MenuAction::JoinTyped;
}

void Application::renderMultiplayerScreen()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    menuScreenBackdrop(w, h);
    if (m_pings.size() != m_servers.size()) refreshPings();

#ifndef __EMSCRIPTEN__
    const std::vector<Net::LanDiscovery::Game>& games = m_discovery.games();
    const int lanCount = static_cast<int>(games.size());
#else
    const int lanCount = 0;
#endif
    const int saved = static_cast<int>(m_servers.size());
    const int count = saved + 1 + lanCount;
    std::string tip;
    int moveFrom = -1, moveTo = -1;
    guiList(m_serverList, 32.0f, std::max(0.0f, h - 96.0f), 36.0f, 305.0f, count, 0.0f, [&](int index, float x, float y, bool hovered) {
        if (index < saved)
        {
            const Servers::Entry& server = m_servers[static_cast<size_t>(index)];
            const Net::PingResult ping = m_pings[static_cast<size_t>(index)].result();
            if (!guiSprite("file:assets/textures/misc/unknown_server.png", x, y, 32.0f, 32.0f))
            {
                m_ui.quad(x, y, 32.0f, 32.0f, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
                m_ui.quad(x + 1.0f, y + 1.0f, 30.0f, 30.0f, glm::vec4(0.35f, 0.35f, 0.35f, 1.0f));
                m_ui.text("?", x + 14.0f, y + 12.0f, 1.0f, glm::vec4(1.0f));
            }
            m_ui.text(server.name, x + 35.0f, y + 1.0f, 1.0f, glm::vec4(1.0f));
            std::string label;
            glm::vec4 labelColour = glm::vec4(0x80 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f, 1.0f);
            std::string sprite;
            std::string statusTip;
            std::string count;
            glm::vec4 countColour = labelColour;
            switch (ping.state)
            {
                case Net::PingResult::State::Pinging:
                {
                    label = "Pinging...";
                    int frame = static_cast<int>((SDL_GetTicks() / 100u + static_cast<uint32_t>(index) * 2u) & 7u);
                    if (frame > 4) frame = 8 - frame;
                    sprite = "sprites/server_list/pinging_" + std::to_string(frame + 1);
                    statusTip = "Pinging...";
                    break;
                }
                case Net::PingResult::State::Online:
                    label = ping.motd;
                    sprite = "sprites/server_list/ping_" + std::to_string(Net::pingBars(ping.ms));
                    statusTip = std::to_string(ping.ms) + " ms";
                    count = std::to_string(ping.online) + "/" + std::to_string(ping.max);
                    break;
                case Net::PingResult::State::Incompatible:
                    label = ping.motd;
                    sprite = "sprites/server_list/incompatible";
                    statusTip = "Incompatible version!";
                    count = ping.version;
                    countColour = glm::vec4(1.0f, 0x55 / 255.0f, 0x55 / 255.0f, 1.0f);
                    break;
                case Net::PingResult::State::Unresolved:
                    label = "Can't resolve hostname";
                    labelColour = glm::vec4(1.0f, 0x55 / 255.0f, 0x55 / 255.0f, 1.0f);
                    sprite = "sprites/server_list/unreachable";
                    statusTip = "(no connection)";
                    break;
                default:
                    label = "Can't connect to server";
                    labelColour = glm::vec4(1.0f, 0x55 / 255.0f, 0x55 / 255.0f, 1.0f);
                    sprite = "sprites/server_list/unreachable";
                    statusTip = "(no connection)";
                    break;
            }
            const std::vector<std::string> lines = wrapText(label, 305.0f - 32.0f - 2.0f);
            for (size_t i = 0; i < lines.size() && i < 2; ++i)
                m_ui.text(lines[i], x + 35.0f, y + 12.0f + 9.0f * static_cast<float>(i), 1.0f, labelColour);
            const float iconX = x + 305.0f - 15.0f;
            if (!guiSprite(sprite.c_str(), iconX, y, 10.0f, 8.0f))
            {
                const int bars = ping.state == Net::PingResult::State::Online ? Net::pingBars(ping.ms) : 0;
                for (int b = 0; b < 5; ++b)
                {
                    const float bh = 2.0f + static_cast<float>(b) * 1.5f;
                    const glm::vec4 colour = ping.state == Net::PingResult::State::Unreachable || ping.state == Net::PingResult::State::Unresolved
                                                 ? glm::vec4(0.8f, 0.1f, 0.1f, 1.0f)
                                                 : b < bars ? glm::vec4(0.2f, 0.9f, 0.2f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
                    m_ui.quad(iconX + 2.0f * static_cast<float>(b), y + 8.0f - bh, 1.0f, bh, colour);
                }
            }
            if (!count.empty())
            {
                const float countW = UIRenderer::textWidth(count, 1.0f);
                m_ui.text(count, iconX - countW - 5.0f, y + 1.0f, 1.0f, countColour);
            }
            if (mouseX >= iconX && mouseX <= iconX + 10.0f && mouseY >= y && mouseY <= y + 8.0f) tip = statusTip;
            if (hovered)
            {
                m_ui.quad(x, y, 32.0f, 32.0f, glm::vec4(0x90 / 255.0f, 0x90 / 255.0f, 0x90 / 255.0f, 0xA0 / 255.0f));
                const float l = mouseX - x, m = mouseY - y;
                const bool canJoin = ping.state == Net::PingResult::State::Online;
                auto overlay = [&](const char* plain, const char* bright, bool lit, int shape) {
                    if (guiSprite(lit ? bright : plain, x, y, 32.0f, 32.0f)) return;
                    const glm::vec4 colour = lit ? glm::vec4(1.0f) : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                    for (int r = 0; r < 5; ++r)
                    {
                        if (shape == 0) m_ui.quad(x + 18.0f + static_cast<float>(r), y + 11.0f + static_cast<float>(r), 1.0f, 10.0f - 2.0f * static_cast<float>(r), colour);
                        if (shape == 1) m_ui.quad(x + 7.0f - static_cast<float>(r), y + 4.0f + static_cast<float>(r), 1.0f + 2.0f * static_cast<float>(r), 1.0f, colour);
                        if (shape == 2) m_ui.quad(x + 7.0f - static_cast<float>(r), y + 27.0f - static_cast<float>(r), 1.0f + 2.0f * static_cast<float>(r), 1.0f, colour);
                    }
                };
                if (canJoin) overlay("sprites/server_list/join", "sprites/server_list/join_highlighted", l < 32.0f && l > 16.0f, 0);
                if (index > 0) overlay("sprites/server_list/move_up", "sprites/server_list/move_up_highlighted", l < 16.0f && m < 16.0f, 1);
                if (index < saved - 1)
                    overlay("sprites/server_list/move_down", "sprites/server_list/move_down_highlighted", l < 16.0f && m > 16.0f, 2);
            }
        }
        else if (index == saved)
        {
            const std::string scanning = "Scanning for games on your local network";
            const float ty = y + 16.0f - 4.0f;
            m_ui.text(scanning, std::floor((w - UIRenderer::textWidth(scanning, 1.0f)) / 2.0f), ty, 1.0f, glm::vec4(1.0f));
            const std::string frames = LOADING_FRAMES[(SDL_GetTicks() / 300u) % 4u];
            m_ui.text(frames, std::floor((w - UIRenderer::textWidth(frames, 1.0f)) / 2.0f), ty + 9.0f, 1.0f,
                      glm::vec4(0x80 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f, 1.0f));
        }
        else
        {
#ifndef __EMSCRIPTEN__
            const Net::LanDiscovery::Game& game = games[static_cast<size_t>(index - saved - 1)];
            m_ui.text("LAN World", x + 35.0f, y + 1.0f, 1.0f, glm::vec4(1.0f));
            m_ui.text(game.name, x + 35.0f, y + 12.0f, 1.0f, glm::vec4(0x80 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f, 1.0f));
            m_ui.text(game.address + ":" + std::to_string(game.port), x + 35.0f, y + 23.0f, 1.0f, SERVER_ADDRESS);
#endif
        }
    });

    const int selected = m_serverList.selected;
    if (selected == saved) m_serverList.selected = -1;
    const float rowLeft = std::floor(w / 2.0f) - std::floor(305.0f / 2.0f) + 2.0f;
    if (m_serverList.clicked >= 0 && m_serverList.clicked < saved)
    {
        const int index = m_serverList.clicked;
        const float rowTop = 32.0f + 4.0f - std::floor(m_serverList.scroll) + 36.0f * static_cast<float>(index);
        const float l = mouseX - rowLeft, m = mouseY - rowTop;
        const bool canJoin = m_pings[static_cast<size_t>(index)].result().state == Net::PingResult::State::Online;
        if (l < 32.0f && l > 16.0f && canJoin) joinAddress(m_servers[static_cast<size_t>(index)].address);
        else if (l < 16.0f && m < 16.0f && index > 0) { moveFrom = index; moveTo = index - 1; }
        else if (l < 16.0f && m > 16.0f && index < saved - 1) { moveFrom = index; moveTo = index + 1; }
        else if (m_serverList.doubleClicked) joinAddress(m_servers[static_cast<size_t>(index)].address);
    }
#ifndef __EMSCRIPTEN__
    if (m_serverList.clicked > saved && m_serverList.doubleClicked)
    {
        m_joinChoice = m_serverList.clicked - saved - 1;
        m_pendingAction = MenuAction::JoinDiscovered;
    }
#endif
    if (moveFrom >= 0)
    {
        Servers::move(m_servers, moveFrom, moveTo);
        std::swap(m_pings[static_cast<size_t>(moveFrom)], m_pings[static_cast<size_t>(moveTo)]);
        m_serverList.selected = moveTo;
        m_serverList.clickIndex = -1;
        saveServers();
    }

    const std::string title = "Play Multiplayer";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 20.0f, 1.0f, glm::vec4(1.0f));

    const bool serverChosen = m_serverList.selected >= 0 && m_serverList.selected < saved;
    const bool lanChosen = m_serverList.selected > saved && m_serverList.selected < count;
    const float x0 = std::floor((w - 308.0f) / 2.0f);
    const float row1 = h - 54.0f, row2 = h - 30.0f;
    if (guiButton(x0, row1, 100.0f, 20.0f, "Join Server", serverChosen || lanChosen) ||
        (!m_automated && m_input.wasKeyPressed(SDL_SCANCODE_RETURN) && (serverChosen || lanChosen)))
    {
        if (serverChosen) joinAddress(m_servers[static_cast<size_t>(m_serverList.selected)].address);
#ifndef __EMSCRIPTEN__
        else
        {
            m_joinChoice = m_serverList.selected - saved - 1;
            m_pendingAction = MenuAction::JoinDiscovered;
        }
#endif
    }
    if (guiButton(x0 + 104.0f, row1, 100.0f, 20.0f, "Direct Connection"))
    {
        m_directAddress = GuiField{};
        m_directAddress.maxLength = 128;
        m_directAddress.text = m_lastDirect;
        m_directAddress.focused = true;
        m_directAddress.focusTime = SDL_GetTicks();
        m_screen = Screen::DirectConnect;
    }
    if (guiButton(x0 + 208.0f, row1, 100.0f, 20.0f, "Add Server"))
    {
        m_editServer = -1;
        m_serverName = GuiField{};
        m_serverName.text = "Minecraft Server";
        m_serverName.focused = true;
        m_serverName.focusTime = SDL_GetTicks();
        m_serverAddress = GuiField{};
        m_serverAddress.maxLength = 128;
        m_serverPacks = Servers::Packs::Prompt;
        m_screen = Screen::EditServer;
    }
    if (guiButton(x0, row2, 74.0f, 20.0f, "Edit", serverChosen))
    {
        const Servers::Entry& server = m_servers[static_cast<size_t>(m_serverList.selected)];
        m_editServer = m_serverList.selected;
        m_serverName = GuiField{};
        m_serverName.text = server.name;
        m_serverName.focused = true;
        m_serverName.focusTime = SDL_GetTicks();
        m_serverAddress = GuiField{};
        m_serverAddress.maxLength = 128;
        m_serverAddress.text = server.address;
        m_serverPacks = server.packs;
        m_screen = Screen::EditServer;
    }
    if (guiButton(x0 + 78.0f, row2, 74.0f, 20.0f, "Delete", serverChosen))
    {
        m_deletingServer = true;
        m_editServer = m_serverList.selected;
        m_editNote = m_servers[static_cast<size_t>(m_serverList.selected)].name;
        m_screen = Screen::ConfirmDelete;
    }
    if (guiButton(x0 + 156.0f, row2, 74.0f, 20.0f, "Refresh"))
    {
        m_servers = Servers::load(m_configPath + "/servers.txt");
        refreshPings();
    }
    if (guiButton(x0 + 234.0f, row2, 74.0f, 20.0f, "Back")) m_pendingAction = MenuAction::BackToTitle;
    if (!tip.empty()) drawTooltip(tip, mouseX, mouseY, true);
    m_ui.setScale(1.0f);
}

void Application::renderEditServer()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const float half = std::floor(w / 2.0f);
    const std::string title = "Edit Server Info";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 17.0f, 1.0f, glm::vec4(1.0f));
    const glm::vec4 label(0xA0 / 255.0f, 0xA0 / 255.0f, 0xA0 / 255.0f, 1.0f);
    m_ui.textWithShadow("Server Name", half - 99.0f, 53.0f, 1.0f, label);
    guiField(m_serverName, half - 100.0f, 66.0f, 200.0f, 20.0f);
    m_ui.textWithShadow("Server Address", half - 99.0f, 94.0f, 1.0f, label);
    guiField(m_serverAddress, half - 100.0f, 106.0f, 200.0f, 20.0f);
    if (guiButton(half - 100.0f, std::floor(h / 4.0f) + 72.0f, 200.0f, 20.0f,
                  std::string("Server Resource Packs: ") + Servers::packsName(m_serverPacks)))
        m_serverPacks = static_cast<Servers::Packs>((static_cast<int>(m_serverPacks) + 1) % 3);
    std::string host;
    uint16_t port = 0;
    const bool valid = Net::splitAddress(m_serverAddress.text, host, port);
    const bool done = guiButton(half - 100.0f, std::floor(h / 4.0f) + 96.0f + 18.0f, 200.0f, 20.0f, "Done", valid) ||
                      (valid && !m_automated && m_input.wasKeyPressed(SDL_SCANCODE_RETURN));
    if (done)
    {
        Servers::Entry entry;
        entry.name = m_serverName.text;
        entry.address = m_serverAddress.text;
        entry.packs = m_serverPacks;
        if (m_editServer >= 0 && m_editServer < static_cast<int>(m_servers.size())) m_servers[static_cast<size_t>(m_editServer)] = entry;
        else m_servers.push_back(entry);
        saveServers();
        refreshPings();
        m_screen = Screen::Multiplayer;
    }
    if (guiButton(half - 100.0f, std::floor(h / 4.0f) + 120.0f + 18.0f, 200.0f, 20.0f, "Cancel")) m_screen = Screen::Multiplayer;
    m_ui.setScale(1.0f);
}

void Application::renderDirectConnect()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const float half = std::floor(w / 2.0f);
    const std::string title = "Direct Connection";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 20.0f, 1.0f, glm::vec4(1.0f));
    m_ui.textWithShadow("Server Address", half - 99.0f, 100.0f, 1.0f, glm::vec4(0xA0 / 255.0f, 0xA0 / 255.0f, 0xA0 / 255.0f, 1.0f));
    guiField(m_directAddress, half - 100.0f, 116.0f, 200.0f, 20.0f);
    std::string host;
    uint16_t port = 0;
    const bool valid = Net::splitAddress(m_directAddress.text, host, port);
    const bool join = guiButton(half - 100.0f, std::floor(h / 4.0f) + 96.0f + 12.0f, 200.0f, 20.0f, "Join Server", valid) ||
                      (valid && !m_automated && m_input.wasKeyPressed(SDL_SCANCODE_RETURN));
    if (join)
    {
        m_lastDirect = m_directAddress.text;
        joinAddress(m_directAddress.text);
    }
    if (guiButton(half - 100.0f, std::floor(h / 4.0f) + 120.0f + 12.0f, 200.0f, 20.0f, "Cancel"))
    {
        m_lastDirect = m_directAddress.text;
        m_screen = Screen::Multiplayer;
    }
    m_ui.setScale(1.0f);
}

void Application::renderConnecting()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const bool failed = m_net.status() == Net::Status::Failed || (m_net.role() == Net::Role::Offline && m_net.status() == Net::Status::Idle);
    if (!failed)
    {
        const std::string status = m_net.status() == Net::Status::Connecting ? "Connecting to the server..." : "Joining world...";
        m_ui.textWithShadow(status, std::floor((w - UIRenderer::textWidth(status, 1.0f)) / 2.0f), std::floor(h / 2.0f) - 50.0f, 1.0f, glm::vec4(1.0f));
        if (guiButton(std::floor(w / 2.0f) - 100.0f, std::floor(h / 4.0f) + 120.0f + 12.0f, 200.0f, 20.0f, "Cancel"))
        {
            m_net.leave();
            m_screen = Screen::Multiplayer;
        }
    }
    else
    {
        const std::string title = "Failed to connect to the server";
        const std::string reason = m_net.message().empty() ? std::string("Can't connect to server") : m_net.message();
        const std::vector<std::string> lines = wrapText(reason, w - 50.0f);
        const float total = 73.0f + 9.0f * static_cast<float>(lines.size());
        const float y0 = std::floor((h - total) / 2.0f);
        m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), y0 + 10.0f, 1.0f, glm::vec4(1.0f));
        for (size_t i = 0; i < lines.size(); ++i)
            m_ui.textWithShadow(lines[i], std::floor((w - UIRenderer::textWidth(lines[i], 1.0f)) / 2.0f), y0 + 39.0f + 9.0f * static_cast<float>(i), 1.0f,
                                glm::vec4(0xA0 / 255.0f, 0xA0 / 255.0f, 0xA0 / 255.0f, 1.0f));
        if (guiButton(std::floor(w / 2.0f) - 100.0f, y0 + 51.0f + 9.0f * static_cast<float>(lines.size()), 200.0f, 20.0f, "Back to Server List"))
        {
            m_net.leave();
            m_screen = Screen::Multiplayer;
        }
    }
    m_ui.setScale(1.0f);
}
