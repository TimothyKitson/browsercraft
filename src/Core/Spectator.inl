std::vector<Application::SpectatorCommand> Application::spectatorGroup() const
{
    std::vector<SpectatorCommand> out;
    std::vector<std::pair<std::string, uint32_t>> players;
    for (const auto& [id, player] : m_net.players()) players.emplace_back(player.name, id);
    std::sort(players.begin(), players.end());
    if (m_spectatorGroup == 0)
    {
        out.push_back({ SpectatorCommand::Kind::TeleportGroup, "Teleport to Player", !players.empty(), 0 });
        out.push_back({ SpectatorCommand::Kind::TeamGroup, "Teleport to Team Member", false, 0 });
    }
    else
        for (const auto& [name, id] : players) out.push_back({ SpectatorCommand::Kind::Player, name, true, id });
    return out;
}

Application::SpectatorCommand Application::spectatorCommand(int slot) const
{
    const std::vector<SpectatorCommand> group = spectatorGroup();
    const int index = slot + m_spectatorPage * 6;
    if (m_spectatorPage > 0 && slot == 0) return { SpectatorCommand::Kind::PreviousPage, "Previous Page", true, 0 };
    if (slot == 7) return { SpectatorCommand::Kind::NextPage, "Next Page", index < static_cast<int>(group.size()), 0 };
    if (slot == 8) return { SpectatorCommand::Kind::Close, "Close Menu", true, 0 };
    if (index >= 0 && index < static_cast<int>(group.size())) return group[static_cast<size_t>(index)];
    return SpectatorCommand{};
}

void Application::useSpectatorSlot(int slot)
{
    const SpectatorCommand command = spectatorCommand(slot);
    if (command.kind == SpectatorCommand::Kind::Blank) return;
    if (m_spectatorSlot != slot || !command.enabled)
    {
        m_spectatorSlot = slot;
        return;
    }
    switch (command.kind)
    {
        case SpectatorCommand::Kind::TeleportGroup:
            m_spectatorGroup = 1;
            m_spectatorSlot = -1;
            m_spectatorPage = 0;
            break;
        case SpectatorCommand::Kind::PreviousPage: --m_spectatorPage; break;
        case SpectatorCommand::Kind::NextPage: ++m_spectatorPage; break;
        case SpectatorCommand::Kind::Close: closeSpectatorMenu(); break;
        case SpectatorCommand::Kind::Player:
        {
            const auto& players = m_net.players();
            const auto found = players.find(command.player);
            if (found != players.end())
            {
                m_spectatingMob = 0;
                m_player.position = found->second.position;
                m_player.velocity = glm::vec3(0.0f);
            }
            closeSpectatorMenu();
            break;
        }
        default: break;
    }
}

void Application::openSpectatorMenu()
{
    m_spectatorMenu = true;
    m_spectatorGroup = 0;
    m_spectatorSlot = -1;
    m_spectatorPage = 0;
}

void Application::closeSpectatorMenu()
{
    m_spectatorMenu = false;
    m_spectatorTouched = 0;
}

float Application::spectatorMenuShown() const
{
    if (!m_spectatorMenu) return 0.0f;
    const float left = static_cast<float>(static_cast<int64_t>(m_spectatorTouched) + 5000 - static_cast<int64_t>(SDL_GetTicks()));
    return std::clamp(left / 2000.0f, 0.0f, 1.0f);
}

void Application::updateSpectatorInput()
{
    if (!m_player.spectator())
    {
        m_spectatorMenu = false;
        m_spectatingMob = 0;
        return;
    }
    for (int i = 0; i < 9; ++i)
        if (m_input.wasKeyPressed(static_cast<SDL_Scancode>(SDL_SCANCODE_1 + i)))
        {
            m_spectatorTouched = SDL_GetTicks();
            if (m_spectatorMenu) useSpectatorSlot(i);
            else openSpectatorMenu();
        }
    const int wheel = m_input.wheelDelta();
    if (wheel != 0)
    {
        if (m_spectatorMenu)
        {
            const int step = wheel > 0 ? -1 : 1;
            int slot = m_spectatorSlot + step;
            while (slot >= 0 && slot <= 8 && (spectatorCommand(slot).kind == SpectatorCommand::Kind::Blank || !spectatorCommand(slot).enabled))
                slot += step;
            if (slot >= 0 && slot <= 8)
            {
                useSpectatorSlot(slot);
                m_spectatorTouched = SDL_GetTicks();
            }
        }
        else
            m_player.flySpeedScale = std::clamp(m_player.flySpeedScale + static_cast<float>(wheel) * 0.1f, 0.0f, 4.0f);
    }
    if (m_keys.pressed(m_input, Action::PickBlock))
    {
        m_spectatorTouched = SDL_GetTicks();
        if (m_spectatorMenu)
        {
            if (m_spectatorSlot != -1) useSpectatorSlot(m_spectatorSlot);
        }
        else
            openSpectatorMenu();
    }
    if (m_spectatorMenu && spectatorMenuShown() <= 0.0f) closeSpectatorMenu();

    if (m_spectatingMob == 0 && m_keys.pressed(m_input, Action::Attack))
        if (Mob* mob = m_entities.pick(m_camera.position, m_camera.front, ATTACK_REACH))
            if (mob->alive()) m_spectatingMob = mob->serial();
    if (m_spectatingMob != 0 && m_keys.pressed(m_input, Action::Sneak)) m_spectatingMob = 0;
}

bool Application::followSpectatedMob()
{
    if (m_spectatingMob == 0) return false;
    Mob* mob = mobBySerial(m_spectatingMob);
    if (!mob || !mob->alive())
    {
        m_spectatingMob = 0;
        return false;
    }
    const MobType& type = mob->type();
    const float eye = type.height * type.modelScale * 0.85f;
    m_player.position = mob->position();
    m_player.velocity = glm::vec3(0.0f);
    m_camera.position = mob->position() + glm::vec3(0.0f, eye, 0.0f);
    m_camera.yaw = mob->yaw();
    m_camera.pitch = 0.0f;
    m_camera.addLook(0.0f, 0.0f, 0.0f);
    return true;
}

void Application::renderSpectatorMenu(float w, float h)
{
    const float shown = spectatorMenuShown();
    if (shown <= 0.0f) return;
    const float cx = std::floor(w / 2.0f);
    const float y = std::floor(h - 22.0f * shown);
    const glm::vec4 fade(1.0f, 1.0f, 1.0f, shown);
    if (m_gui.fromPack(GuiSprite::Hotbar))
        m_ui.texturedQuad(m_gui.texture(GuiSprite::Hotbar), cx - 91.0f, y, 182.0f, 22.0f, 0.0f, 0.0f, 1.0f, 1.0f, fade);
    else
    {
        m_ui.quad(cx - 91.0f, y, 182.0f, 22.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.8f * shown));
        for (int i = 0; i < 9; ++i)
            m_ui.quad(cx - 89.0f + 20.0f * static_cast<float>(i), y + 2.0f, 18.0f, 18.0f, glm::vec4(0.35f, 0.35f, 0.35f, 0.8f * shown));
    }
    if (m_spectatorSlot >= 0)
    {
        const float sx = cx - 91.0f - 1.0f + static_cast<float>(m_spectatorSlot) * 20.0f;
        if (m_gui.fromPack(GuiSprite::HotbarSelection))
            m_ui.texturedQuad(m_gui.texture(GuiSprite::HotbarSelection), sx, y - 1.0f, 24.0f, 23.0f, 0.0f, 0.0f, 1.0f, 1.0f, fade);
        else
        {
            m_ui.quad(sx, y - 1.0f, 24.0f, 2.0f, fade);
            m_ui.quad(sx, y + 20.0f, 24.0f, 2.0f, fade);
            m_ui.quad(sx, y - 1.0f, 2.0f, 23.0f, fade);
            m_ui.quad(sx + 22.0f, y - 1.0f, 2.0f, 23.0f, fade);
        }
    }
    for (int i = 0; i < 9; ++i)
    {
        const SpectatorCommand command = spectatorCommand(i);
        if (command.kind == SpectatorCommand::Kind::Blank) continue;
        const float ix = cx - 90.0f + static_cast<float>(i) * 20.0f + 2.0f;
        const float iy = y + 3.0f;
        const float bright = command.enabled ? 1.0f : 0.25f;
        const glm::vec4 tint(bright, bright, bright, shown);
        const char* sprite = nullptr;
        switch (command.kind)
        {
            case SpectatorCommand::Kind::TeleportGroup: sprite = "sprites/spectator/teleport_to_player"; break;
            case SpectatorCommand::Kind::TeamGroup: sprite = "sprites/spectator/teleport_to_team"; break;
            case SpectatorCommand::Kind::PreviousPage: sprite = "sprites/spectator/scroll_left"; break;
            case SpectatorCommand::Kind::NextPage: sprite = "sprites/spectator/scroll_right"; break;
            case SpectatorCommand::Kind::Close: sprite = "sprites/spectator/close"; break;
            default: break;
        }
        if (command.kind == SpectatorCommand::Kind::Player)
        {
            const uint32_t hash = static_cast<uint32_t>(std::hash<std::string>{}(command.name));
            m_ui.quad(ix + 2.0f, iy + 2.0f, 12.0f, 12.0f,
                      glm::vec4(0.3f + 0.5f * static_cast<float>(hash & 255u) / 255.0f, 0.3f + 0.5f * static_cast<float>((hash >> 8) & 255u) / 255.0f,
                                0.3f + 0.5f * static_cast<float>((hash >> 16) & 255u) / 255.0f, shown));
            const std::string initial = command.name.empty() ? std::string("?") : command.name.substr(0, 1);
            m_ui.text(initial, ix + 6.0f, iy + 4.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, shown));
        }
        else if (sprite && !guiSprite(sprite, ix, iy, 16.0f, 16.0f, tint))
        {
            const glm::vec4 ink(0.9f * bright, 0.9f * bright, 0.9f * bright, shown);
            if (command.kind == SpectatorCommand::Kind::Close)
                for (int k = 0; k < 10; ++k)
                {
                    m_ui.quad(ix + 3.0f + static_cast<float>(k), iy + 3.0f + static_cast<float>(k), 1.0f, 1.0f, ink);
                    m_ui.quad(ix + 12.0f - static_cast<float>(k), iy + 3.0f + static_cast<float>(k), 1.0f, 1.0f, ink);
                }
            else if (command.kind == SpectatorCommand::Kind::PreviousPage || command.kind == SpectatorCommand::Kind::NextPage)
                for (int k = 0; k < 5; ++k)
                {
                    const float fk = static_cast<float>(k);
                    const float px = command.kind == SpectatorCommand::Kind::NextPage ? ix + 6.0f + fk : ix + 10.0f - fk;
                    m_ui.quad(px, iy + 3.0f + fk, 1.0f, 10.0f - 2.0f * fk, ink);
                }
            else
            {
                m_ui.quad(ix + 4.0f, iy + 2.0f, 8.0f, 8.0f, ink);
                m_ui.quad(ix + 2.0f, iy + 10.0f, 12.0f, 4.0f, ink);
            }
        }
        const int alpha = static_cast<int>(shown * 255.0f);
        if (alpha > 3 && command.enabled)
        {
            const std::string key = std::to_string(i + 1);
            m_ui.textWithShadow(key, ix + 19.0f - 2.0f - UIRenderer::textWidth(key, 1.0f), iy + 6.0f + 3.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, shown));
        }
    }
    const int alpha = static_cast<int>(shown * 255.0f);
    if (alpha > 3)
    {
        const SpectatorCommand selected = m_spectatorSlot >= 0 ? spectatorCommand(m_spectatorSlot) : SpectatorCommand{};
        const std::string text = selected.kind != SpectatorCommand::Kind::Blank ? selected.name
                                 : m_spectatorGroup == 0 ? std::string("Press a key to select a command, and again to use it.")
                                                         : std::string("Select a player to teleport to");
        const float tw = UIRenderer::textWidth(text, 1.0f);
        const float tx = std::floor((w - tw) / 2.0f);
        const float ty = h - 35.0f;
        m_ui.quad(tx - 2.0f, ty - 2.0f, tw + 4.0f, 13.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f * shown));
        m_ui.textWithShadow(text, tx, ty, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, shown));
    }
}
