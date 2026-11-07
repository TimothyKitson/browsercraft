namespace
{
    struct BindRow
    {
        const char* category;
        Action action;
        const char* name;
    };

    const std::vector<BindRow>& bindRows()
    {
        static const std::vector<BindRow> ROWS = {
            { "Movement", Action::Jump, "Jump" }, { "Movement", Action::Sneak, "Sneak" }, { "Movement", Action::Sprint, "Sprint" },
            { "Movement", Action::Left, "Strafe Left" }, { "Movement", Action::Right, "Strafe Right" },
            { "Movement", Action::Back, "Walk Backwards" }, { "Movement", Action::Forward, "Walk Forwards" },
            { "Movement", Action::Fly, "Fly" },
            { "Gameplay", Action::Attack, "Attack/Destroy" }, { "Gameplay", Action::PickBlock, "Pick Block" },
            { "Gameplay", Action::Use, "Use Item/Place Block" },
            { "Inventory", Action::Drop, "Drop Selected Item" }, { "Inventory", Action::Inventory, "Open/Close Inventory" },
            { "Inventory", Action::SwapHands, "Swap Item With Offhand" },
            { "Multiplayer", Action::Chat, "Open Chat" }, { "Multiplayer", Action::SlashCommand, "Open Command" },
            { "Miscellaneous", Action::Advancements, "Advancements" }, { "Miscellaneous", Action::Screenshot, "Take Screenshot" },
            { "Miscellaneous", Action::Perspective, "Toggle Perspective" }, { "Miscellaneous", Action::Fullscreen, "Toggle Fullscreen" },
            { "Miscellaneous", Action::HideHud, "Hide HUD" }, { "Miscellaneous", Action::Debug, "Debug Screen" },
            { "Miscellaneous", Action::FancyLighting, "Fancy Lighting" }, { "Miscellaneous", Action::Mute, "Mute Sound" },
            { "Miscellaneous", Action::RenderDistanceDown, "Render Distance Down" },
            { "Miscellaneous", Action::RenderDistanceUp, "Render Distance Up" },
        };
        return ROWS;
    }
}

void Application::renderControlsScreen()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    const bool wasAutomated = m_automated;
    if (m_bindingCaptured) m_automated = true;
    m_controlsHover = -1;
    menuScreenBackdrop(w, h);
    const std::string title = "Key Binds";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 12.0f, 1.0f, glm::vec4(1.0f));

    struct Entry
    {
        const char* category = nullptr;
        const BindRow* row = nullptr;
    };
    std::vector<Entry> entries;
    const char* last = nullptr;
    for (const BindRow& row : bindRows())
    {
        if (!last || std::string(last) != row.category)
        {
            entries.push_back({ row.category, nullptr });
            last = row.category;
        }
        entries.push_back({ nullptr, &row });
    }
    KeyBindings defaults;
    std::string tip;
    m_controlsList.selected = -1;
    const float scrollbarX = std::floor(w / 2.0f) - 170.0f + 340.0f + 10.0f;
    guiList(m_controlsList, 33.0f, std::max(0.0f, h - 66.0f), 20.0f, 340.0f, static_cast<int>(entries.size()), 0.0f,
            [&](int index, float x, float y, bool) {
                const Entry& entry = entries[static_cast<size_t>(index)];
                if (entry.category)
                {
                    const std::string text = entry.category;
                    m_ui.text(text, std::floor((w - UIRenderer::textWidth(text, 1.0f)) / 2.0f), y + 16.0f - 9.0f - 1.0f, 1.0f, glm::vec4(1.0f));
                    return;
                }
                const Action action = entry.row->action;
                const float resetX = scrollbarX - 50.0f - 10.0f;
                const float editX = resetX - 5.0f - 75.0f;
                m_ui.textWithShadow(entry.row->name, x, y + 8.0f - 4.0f, 1.0f, glm::vec4(1.0f));
                const Binding& binding = m_keys.binding(action);
                std::string label = tutorialKey(binding);
                std::string clashes;
                if (binding.valid())
                    for (const BindRow& other : bindRows())
                        if (other.action != action && m_keys.binding(other.action) == binding)
                            clashes += (clashes.empty() ? "" : ", ") + std::string(other.name);
                if (m_rebinding == action) label = "> " + label + " <";
                else if (!clashes.empty()) label = "[ " + label + " ]";
                if (guiButton(editX, y - 2.0f, 75.0f, 20.0f, label) && m_rebinding == Action::Count)
                {
                    m_rebinding = action;
                    m_bindingCaptured = true;
                }
                if (m_rebinding == action)
                    m_ui.quad(editX + 2.0f, y - 2.0f + 19.0f - 2.0f, 71.0f, 1.0f, glm::vec4(1.0f, 1.0f, 0.33f, 1.0f));
                if (!clashes.empty())
                {
                    m_ui.quad(editX - 6.0f, y - 1.0f, 3.0f, 17.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
                    if (mouseX >= editX && mouseX < editX + 75.0f && mouseY >= y - 2.0f && mouseY < y + 18.0f)
                        tip = "This key is also used for:\n" + clashes;
                }
                if (guiButton(resetX, y - 2.0f, 50.0f, 20.0f, "Reset", !(binding == defaults.binding(action))))
                {
                    m_keys.setBinding(action, defaults.binding(action));
                    saveBindings();
                }
            });
    bool changed = false;
    for (const BindRow& row : bindRows()) changed = changed || !(m_keys.binding(row.action) == defaults.binding(row.action));
    const float x0 = std::floor((w - 308.0f) / 2.0f);
    if (guiButton(x0, h - 27.0f, 150.0f, 20.0f, "Reset Keys", changed)) m_pendingAction = MenuAction::ResetBindings;
    if (guiButton(x0 + 158.0f, h - 27.0f, 150.0f, 20.0f, "Done")) m_pendingAction = MenuAction::GoSettings;
    if (!tip.empty()) drawTooltip(tip, mouseX, mouseY, true);
    m_automated = wasAutomated;
    m_bindingCaptured = false;
    m_ui.setScale(1.0f);
}

void Application::setDifficulty(Worlds::Difficulty difficulty)
{
    m_worldMeta.difficulty = difficulty;
    m_player.difficulty = modeIsHardcore(m_gameMode) ? 3 : static_cast<int>(difficulty);
    m_entities.setPeaceful(m_player.difficulty == 0);
    writeWorldMeta();
}

bool Application::lockButton(float x, float y, float size, bool locked, bool enabled)
{
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    const bool over = enabled && mouseX >= x && mouseX < x + size && mouseY >= y && mouseY < y + size;
    const char* sprite = locked ? (!enabled ? "sprites/widget/locked_button_disabled" : over ? "sprites/widget/locked_button_highlighted" : "sprites/widget/locked_button")
                                : (!enabled ? "sprites/widget/unlocked_button_disabled" : over ? "sprites/widget/unlocked_button_highlighted" : "sprites/widget/unlocked_button");
    if (!guiSprite(sprite, x, y, size, size))
    {
        m_ui.quad(x, y, size, size, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        m_ui.quad(x + size * 0.05f, y + size * 0.05f, size * 0.9f, size * 0.9f, over ? glm::vec4(0.55f, 0.55f, 0.65f, 1.0f) : glm::vec4(0.42f, 0.42f, 0.42f, 1.0f));
        const glm::vec4 ink = enabled ? glm::vec4(1.0f, 0.85f, 0.3f, 1.0f) : glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
        m_ui.quad(x + size * 0.3f, y + size * 0.45f, size * 0.4f, size * 0.35f, ink);
        m_ui.quad(x + size * 0.33f, y + size * 0.22f, size * 0.08f, size * 0.25f, ink);
        m_ui.quad(locked ? x + size * 0.59f : x + size * 0.68f, y + size * 0.22f, size * 0.08f, size * (locked ? 0.25f : 0.12f), ink);
        m_ui.quad(x + size * 0.33f, y + size * 0.2f, size * (locked ? 0.34f : 0.43f), size * 0.07f, ink);
    }
    const bool hit = over && !m_automated && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT);
    if (hit) m_audio.play(Sound::Click, 0.45f);
    return hit;
}

void Application::renderLockConfirm()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const std::string title = "Lock World Difficulty";
    const std::string message = std::string("Are you sure you want to lock the difficulty of this world? This will set this world to always be ") +
                                Worlds::difficultyName(m_worldMeta.difficulty) + ", and you will never be able to change that again.";
    const std::vector<std::string> lines = wrapText(message, w - 50.0f);
    const float messagesHeight = static_cast<float>(lines.size()) * 9.0f;
    const float titleY = std::clamp(std::floor((h - messagesHeight) / 2.0f) - 29.0f, 10.0f, 80.0f);
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), titleY, 1.0f, glm::vec4(1.0f));
    for (size_t i = 0; i < lines.size(); ++i)
        m_ui.textWithShadow(lines[i], std::floor((w - UIRenderer::textWidth(lines[i], 1.0f)) / 2.0f), titleY + 20.0f + 9.0f * static_cast<float>(i), 1.0f, glm::vec4(1.0f));
    const float buttonsY = std::clamp(titleY + 20.0f + messagesHeight + 20.0f, std::floor(h / 6.0f) + 96.0f, h - 24.0f);
    const float half = std::floor(w / 2.0f);
    if (guiButton(half - 155.0f, buttonsY, 150.0f, 20.0f, "Yes"))
    {
        m_worldMeta.difficultyLocked = true;
        writeWorldMeta();
        m_screen = Screen::Settings;
    }
    if (guiButton(half + 5.0f, buttonsY, 150.0f, 20.0f, "No")) m_screen = Screen::Settings;
    m_ui.setScale(1.0f);
}
