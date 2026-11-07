namespace
{
    const char* const TUTORIAL_STEP_IDS[6] = { "movement", "find_tree", "punch_tree", "open_inventory", "craft_planks", "none" };
    const char* const TUTORIAL_ICONS[5] = { "sprites/toast/movement_keys", "sprites/toast/mouse", "sprites/toast/tree",
                                            "sprites/toast/recipe_book", "sprites/toast/wooden_planks" };

    std::string tutorialKey(const Binding& binding)
    {
        if (!binding.valid()) return "Not Bound";
        if (binding.mouse)
        {
            if (binding.code == SDL_BUTTON_LEFT) return "Left Button";
            if (binding.code == SDL_BUTTON_RIGHT) return "Right Button";
            if (binding.code == SDL_BUTTON_MIDDLE) return "Middle Button";
            return "Button " + std::to_string(binding.code);
        }
        std::string name = SDL_GetScancodeName(static_cast<SDL_Scancode>(binding.code));
        if (name.size() <= 1)
        {
            for (char& c : name) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return name;
        }
        bool start = true;
        for (char& c : name)
        {
            c = static_cast<char>(start ? std::toupper(static_cast<unsigned char>(c)) : std::tolower(static_cast<unsigned char>(c)));
            start = c == ' ';
        }
        return name;
    }

    std::string bold(const std::string& text)
    {
        return "\x01" + text + "\x01";
    }
}

const char* Application::tutorialStepId(TutorialStep step)
{
    return TUTORIAL_STEP_IDS[std::clamp(static_cast<int>(step), 0, 5)];
}

Application::TutorialStep Application::tutorialStepFrom(const std::string& id)
{
    for (int i = 0; i < 6; ++i)
        if (id == TUTORIAL_STEP_IDS[i]) return static_cast<TutorialStep>(i);
    return TutorialStep::Movement;
}

void Application::drawBoldText(const std::string& text, float x, float y, const glm::vec4& colour)
{
    bool heavy = false;
    std::string run;
    auto flush = [&]() {
        if (run.empty()) return;
        m_ui.text(run, x, y, 1.0f, colour);
        if (heavy) m_ui.text(run, x + 1.0f, y, 1.0f, colour);
        x += UIRenderer::textWidth(run, 1.0f) + (heavy ? static_cast<float>(run.size()) : 0.0f);
        run.clear();
    };
    for (char c : text)
    {
        if (c == '\x01')
        {
            flush();
            heavy = !heavy;
            continue;
        }
        run.push_back(c);
    }
    flush();
}

uint32_t Application::addTutorialToast(int icon, const std::string& title, const std::string& description, bool progressBar)
{
    Toast toast;
    toast.tutorialIcon = icon;
    toast.tutorialTitle = title;
    toast.tutorialDescription = description;
    toast.progressBar = progressBar;
    toast.serial = ++m_toastSerial;
    m_toasts.push_back(toast);
    return toast.serial;
}

Application::Toast* Application::toastBySerial(uint32_t serial)
{
    if (serial == 0) return nullptr;
    for (Toast& toast : m_toasts)
        if (toast.serial == serial) return &toast;
    return nullptr;
}

void Application::hideTutorialToast(uint32_t& serial)
{
    if (Toast* toast = toastBySerial(serial))
    {
        if (!toast->hidden)
        {
            toast->hidden = true;
            toast->hiddenAt = toast->age;
        }
    }
    serial = 0;
}

void Application::setTutorialStep(TutorialStep step)
{
    hideTutorialToast(m_tutorialToast);
    hideTutorialToast(m_tutorialLookToast);
    m_tutorialStep = step;
    m_tutorialTicks = 0;
    m_tutorialMoved = 0;
    m_tutorialLooked = 0;
    m_tutorialMoveDone = -1;
    m_tutorialLookDone = -1;
    m_tutorialPunches = 0;
    m_tutorialLogs = minedTreeBlocks(true);
    saveOptions();
}

long long Application::minedTreeBlocks(bool logsOnly) const
{
    long long total = 0;
    for (const auto& [id, counts] : m_stats.items())
    {
        if (isItem(static_cast<StackId>(id))) continue;
        const BlockId block = asBlock(static_cast<StackId>(id));
        const bool tree = isLog(block) || (!logsOnly && (isLeaves(block) || block == Blocks::NetherWartBlock || block == Blocks::WarpedWartBlock));
        if (tree) total += counts[static_cast<size_t>(Stats::ItemStat::Mined)];
    }
    return total;
}

bool Application::inventoryHas(const std::function<bool(StackId)>& test) const
{
    for (int i = 0; i < Inventory::TOTAL_SLOTS; ++i)
        if (!m_inventory.slot(i).empty() && test(m_inventory.slot(i).id)) return true;
    return !m_inventory.slot(Inventory::OFFHAND).empty() && test(m_inventory.slot(Inventory::OFFHAND).id);
}

void Application::tickTutorial(float deltaTime)
{
    if (!m_world || m_menuWorld || m_tutorialStep == TutorialStep::None) return;
    if (m_automated && m_startupScreen.rfind("tutorial", 0) != 0 && !m_tutorialForced) return;
    m_tutorialClock += deltaTime;
    const bool moving = m_keys.down(m_input, Action::Forward) || m_keys.down(m_input, Action::Back) || m_keys.down(m_input, Action::Left) ||
                        m_keys.down(m_input, Action::Right) || m_keys.down(m_input, Action::Jump);
    const bool looking = std::fabs(m_camera.yaw - m_tutorialYaw) > 0.01f || std::fabs(m_camera.pitch - m_tutorialPitch) > 0.01f;
    m_tutorialMovedNow = m_tutorialMovedNow || moving;
    m_tutorialLookedNow = m_tutorialLookedNow || looking;
    m_tutorialYaw = m_camera.yaw;
    m_tutorialPitch = m_camera.pitch;
    const bool survival = m_gameMode == GameMode::Survival || m_gameMode == GameMode::Hardcore;
    auto isTreeStack = [](StackId id) {
        if (isItem(id)) return false;
        const BlockId block = asBlock(id);
        return isLog(block) || isLeaves(block) || block == Blocks::NetherWartBlock || block == Blocks::WarpedWartBlock;
    };
    auto isLogStack = [](StackId id) { return !isItem(id) && isLog(asBlock(id)); };
    auto isPlankStack = [](StackId id) { return !isItem(id) && isPlanks(asBlock(id)); };

    while (m_tutorialClock >= 0.05f && m_tutorialStep != TutorialStep::None)
    {
        m_tutorialClock -= 0.05f;
        ++m_tutorialTicks;
        switch (m_tutorialStep)
        {
            case TutorialStep::Movement:
            {
                if (m_tutorialMovedNow) ++m_tutorialMoved;
                if (m_tutorialLookedNow) ++m_tutorialLooked;
                m_tutorialMovedNow = m_tutorialLookedNow = false;
                if (m_tutorialMoveDone == -1 && m_tutorialMoved > 40)
                {
                    hideTutorialToast(m_tutorialToast);
                    m_tutorialMoveDone = m_tutorialTicks;
                }
                if (m_tutorialLookDone == -1 && m_tutorialLooked > 40)
                {
                    hideTutorialToast(m_tutorialLookToast);
                    m_tutorialLookDone = m_tutorialTicks;
                }
                if (m_tutorialMoveDone != -1 && m_tutorialLookDone != -1)
                {
                    setTutorialStep(survival ? TutorialStep::FindTree : TutorialStep::None);
                    break;
                }
                if (Toast* toast = toastBySerial(m_tutorialToast)) toast->progress = static_cast<float>(m_tutorialMoved) / 40.0f;
                if (Toast* toast = toastBySerial(m_tutorialLookToast)) toast->progress = static_cast<float>(m_tutorialLooked) / 40.0f;
                if (m_tutorialTicks >= 100)
                {
                    if (m_tutorialMoveDone == -1 && m_tutorialToast == 0)
                        m_tutorialToast = addTutorialToast(0,
                            "Move with " + bold(tutorialKey(m_keys.binding(Action::Forward))) + ", " + bold(tutorialKey(m_keys.binding(Action::Left))) +
                                ", " + bold(tutorialKey(m_keys.binding(Action::Back))) + " and " + bold(tutorialKey(m_keys.binding(Action::Right))),
                            "Jump with " + bold(tutorialKey(m_keys.binding(Action::Jump))), true);
                    else if (m_tutorialMoveDone != -1 && m_tutorialTicks - m_tutorialMoveDone >= 20 && m_tutorialLookDone == -1 && m_tutorialLookToast == 0)
                        m_tutorialLookToast = addTutorialToast(1, "Look around", "Use your mouse to turn", true);
                }
                break;
            }
            case TutorialStep::FindTree:
            {
                if (!survival) { setTutorialStep(TutorialStep::None); break; }
                if (m_tutorialTicks == 1 && (inventoryHas(isTreeStack) || minedTreeBlocks(false) > 0))
                {
                    setTutorialStep(TutorialStep::CraftPlanks);
                    break;
                }
                if (m_tutorialTicks >= 6000 && m_tutorialToast == 0)
                    m_tutorialToast = addTutorialToast(2, "Find a tree", "Punch it to collect wood", false);
                const RaycastHit hit = m_world->raycast(m_camera.position, m_camera.front, REACH_DISTANCE);
                if (hit.hit && isTreeStack(m_world->getBlock(hit.block.x, hit.block.y, hit.block.z)))
                {
                    setTutorialStep(TutorialStep::PunchTree);
                    break;
                }
                if (inventoryHas(isTreeStack)) setTutorialStep(TutorialStep::CraftPlanks);
                break;
            }
            case TutorialStep::PunchTree:
            {
                if (!survival) { setTutorialStep(TutorialStep::None); break; }
                if (m_tutorialTicks == 1 && (inventoryHas(isLogStack) || minedTreeBlocks(false) > 0))
                {
                    setTutorialStep(TutorialStep::CraftPlanks);
                    break;
                }
                if ((m_tutorialTicks >= 600 || m_tutorialPunches > 3) && m_tutorialToast == 0)
                    m_tutorialToast = addTutorialToast(2, "Destroy the tree", "Hold down " + bold(tutorialKey(m_keys.binding(Action::Attack))), true);
                if (minedTreeBlocks(true) > m_tutorialLogs)
                {
                    setTutorialStep(TutorialStep::OpenInventory);
                    break;
                }
                const bool onLog = m_hasTarget && isLog(m_world->getBlock(m_targetBlock.x, m_targetBlock.y, m_targetBlock.z));
                if (onLog && m_breakProgress > 0.0f)
                {
                    if (Toast* toast = toastBySerial(m_tutorialToast)) toast->progress = m_breakProgress;
                }
                else if (Toast* toast = toastBySerial(m_tutorialToast))
                    toast->progress = 0.0f;
                if (m_tutorialToast == 0 && onLog && m_keys.pressed(m_input, Action::Attack)) ++m_tutorialPunches;
                if (inventoryHas(isLogStack)) setTutorialStep(TutorialStep::CraftPlanks);
                break;
            }
            case TutorialStep::OpenInventory:
            {
                if (!survival) { setTutorialStep(TutorialStep::None); break; }
                if (m_tutorialTicks >= 600 && m_tutorialToast == 0)
                    m_tutorialToast = addTutorialToast(3, "Open your inventory", "Press " + bold(tutorialKey(m_keys.binding(Action::Inventory))), false);
                if (m_inventoryOpen) setTutorialStep(TutorialStep::CraftPlanks);
                break;
            }
            case TutorialStep::CraftPlanks:
            {
                if (!survival) { setTutorialStep(TutorialStep::None); break; }
                bool crafted = false;
                for (const auto& [id, counts] : m_stats.items())
                    crafted = crafted || (isPlankStack(static_cast<StackId>(id)) && counts[static_cast<size_t>(Stats::ItemStat::Crafted)] > 0);
                if (m_tutorialTicks == 1 && (inventoryHas(isPlankStack) || crafted))
                {
                    setTutorialStep(TutorialStep::None);
                    break;
                }
                if (m_tutorialTicks >= 1200 && m_tutorialToast == 0)
                    m_tutorialToast = addTutorialToast(4, "Craft wooden planks", "The recipe book can help", false);
                if (inventoryHas(isPlankStack)) setTutorialStep(TutorialStep::None);
                break;
            }
            default: break;
        }
    }
}

void Application::drawTutorialToast(const Toast& toast, float tx, float ty)
{
    if (!guiSprite("sprites/toast/tutorial", tx, ty, 160.0f, 32.0f))
    {
        m_ui.quad(tx, ty, 160.0f, 32.0f, glm::vec4(0.55f, 0.55f, 0.55f, 0.97f));
        m_ui.quad(tx + 1.0f, ty + 1.0f, 158.0f, 30.0f, glm::vec4(0.95f, 0.95f, 0.95f, 0.97f));
    }
    const int icon = std::clamp(toast.tutorialIcon, 0, 4);
    if (!guiSprite(TUTORIAL_ICONS[icon], tx + 6.0f, ty + 6.0f, 20.0f, 20.0f))
    {
        static const StackId FALLBACK[5] = { Blocks::Air, Blocks::Air, Blocks::Log, Blocks::CraftingTable, Blocks::Planks };
        if (FALLBACK[icon] != Blocks::Air) drawItem(tx + 8.0f, ty + 8.0f, SLOT, FALLBACK[icon], 1, glm::vec4(1.0f));
        else
        {
            m_ui.quad(tx + 7.0f, ty + 7.0f, 18.0f, 18.0f, glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));
            m_ui.text(icon == 0 ? "W" : "M", tx + 13.0f, ty + 12.0f, 1.0f, glm::vec4(1.0f));
        }
    }
    const glm::vec4 heading(0x50 / 255.0f, 0.0f, 0x50 / 255.0f, 1.0f);
    if (toast.tutorialDescription.empty()) drawBoldText(toast.tutorialTitle, tx + 30.0f, ty + 12.0f, heading);
    else
    {
        drawBoldText(toast.tutorialTitle, tx + 30.0f, ty + 7.0f, heading);
        drawBoldText(toast.tutorialDescription, tx + 30.0f, ty + 18.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    if (toast.progressBar)
    {
        m_ui.quad(tx + 3.0f, ty + 28.0f, 154.0f, 1.0f, glm::vec4(1.0f));
        const float shown = std::clamp(toast.shownProgress, 0.0f, 1.0f);
        const glm::vec4 colour = toast.progress >= toast.shownProgress ? glm::vec4(0.0f, 0x66 / 255.0f, 0.0f, 1.0f)
                                                                       : glm::vec4(0x55 / 255.0f, 0.0f, 0.0f, 1.0f);
        m_ui.quad(tx + 3.0f, ty + 28.0f, std::floor(154.0f * shown), 1.0f, colour);
    }
}
