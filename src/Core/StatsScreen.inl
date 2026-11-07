namespace
{
    constexpr float STATS_HEADER = 33.0f;
    constexpr float STATS_FOOTER = 58.0f;
    constexpr float STATS_ROW_WIDTH = 280.0f;
    constexpr float STATS_ROW_HEIGHT[3] = { 14.0f, 22.0f, 36.0f };
    constexpr float STATS_LIST_HEADER[3] = { 0.0f, 22.0f, 0.0f };
    const char* const STATS_ICONS[Stats::ITEM_STATS] = {
        "sprites/statistics/block_mined", "sprites/statistics/item_broken", "sprites/statistics/item_crafted",
        "sprites/statistics/item_used", "sprites/statistics/item_picked_up", "sprites/statistics/item_dropped" };
    const char* const STATS_TYPE_NAMES[Stats::ITEM_STATS] = {
        "Times Mined", "Times Broken", "Times Crafted", "Times Used", "Picked Up", "Dropped" };
    const glm::vec4 STATS_WHITE(1.0f);
    const glm::vec4 STATS_ALT(0xBA / 255.0f, 0xBA / 255.0f, 0xBA / 255.0f, 1.0f);
    const glm::vec4 STATS_GRAY(0x80 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f, 1.0f);

    float statsColumn(int i) { return 75.0f + 40.0f * static_cast<float>(i); }
}

void Application::statBroke(StackId id)
{
    if (id != Blocks::Air) m_stats.item(Stats::ItemStat::Broken, id);
}

StackId Application::statBlockItem(BlockId block) const
{
    if (isObtainable(block)) return block;
    const StackId drop = blockDrop(block);
    return drop != Blocks::Air ? drop : static_cast<StackId>(block);
}

void Application::statOpened(BlockId block)
{
    using C = Stats::Custom;
    C stat = C::Count;
    if (isShulkerBox(block)) stat = C::OpenShulkerBox;
    else if (isEnderChest(block)) stat = C::OpenEnderchest;
    else if (isTrappedChest(block)) stat = C::TriggerTrappedChest;
    else if (block == Blocks::Barrel || block == Blocks::BarrelOpen) stat = C::OpenBarrel;
    else if (block == Blocks::Hopper) stat = C::InspectHopper;
    else if (block == Blocks::Dropper) stat = C::InspectDropper;
    else if (block == Blocks::Dispenser) stat = C::InspectDispenser;
    else if (isStorageChest(block)) stat = C::OpenChest;
    else if (isFurnaceBlock(block))
    {
        const int kind = (furnaceSlot(block) % 6) / 2;
        stat = kind == 1 ? C::InteractWithSmoker : kind == 2 ? C::InteractWithBlastFurnace : C::InteractWithFurnace;
    }
    else if (block == Blocks::CraftingTable) stat = C::InteractWithCraftingTable;
    else if (isAnvil(block)) stat = C::InteractWithAnvil;
    else if (block == Blocks::Grindstone) stat = C::InteractWithGrindstone;
    else if (block == Blocks::Stonecutter) stat = C::InteractWithStonecutter;
    else if (block == Blocks::SmithingTable) stat = C::InteractWithSmithingTable;
    else if (block == Blocks::CartographyTable) stat = C::InteractWithCartographyTable;
    else if (isLoom(block)) stat = C::InteractWithLoom;
    else if (block == Blocks::BrewingStand) stat = C::InteractWithBrewingstand;
    else if (block == Blocks::Beacon) stat = C::InteractWithBeacon;
    else if (block == Blocks::Lectern || block == Blocks::LecternBook) stat = C::InteractWithLectern;
    if (stat != C::Count) m_stats.add(stat);
}

void Application::statUsed(const ItemStack& before, const ItemStack& after)
{
    if (before.empty()) return;
    const bool same = !after.empty() && after.id == before.id;
    if (same && after.count < before.count) m_stats.item(Stats::ItemStat::Used, before.id, before.count - after.count);
    else if (same && after.damage > before.damage) m_stats.item(Stats::ItemStat::Used, before.id);
    else if (!same) m_stats.item(Stats::ItemStat::Used, before.id);
}

void Application::drainStatEvents()
{
    for (const auto& [id, count] : m_inventory.takeCrafted()) m_stats.item(Stats::ItemStat::Crafted, id, count);
    for (const auto& [id, count] : m_drops.takePicked()) m_stats.item(Stats::ItemStat::PickedUp, id, count);
    const Player::Tally tally = m_player.takeTally();
    if (tally.taken > 0) m_stats.add(Stats::Custom::DamageTaken, tally.taken * 10);
    if (tally.absorbed > 0) m_stats.add(Stats::Custom::DamageAbsorbed, tally.absorbed * 10);
    if (tally.resisted > 0) m_stats.add(Stats::Custom::DamageResisted, tally.resisted * 10);
}

void Application::tickStats(float deltaTime)
{
    using C = Stats::Custom;
    drainStatEvents();
    m_lastAttackerAge += deltaTime;
    if (!m_statsPlaced)
    {
        m_statsFrom = m_player.position;
        m_statsPlaced = true;
    }

    const bool inWater = m_player.isInWater(*m_world);
    if (inWater || m_player.onGround)
    {
        if (!inWater && m_statsFall >= 2.0f && !m_player.creative() && !m_player.spectator())
            m_stats.add(C::FallOneCm, std::lround(m_statsFall * 100.0f));
        m_statsFall = 0.0f;
    }
    else
        m_statsFall = std::max(m_statsFall, m_player.fallenSoFar());

    m_statsClock += deltaTime;
    int ticks = 0;
    while (m_statsClock >= 0.05f)
    {
        m_statsClock -= 0.05f;
        ++ticks;
    }
    if (ticks == 0) return;
    m_stats.add(C::PlayTime, ticks);
    m_stats.add(C::TotalWorldTime, ticks);
    if (m_player.health > 0) m_stats.add(C::TimeSinceDeath, ticks);
    if (m_player.sneaking) m_stats.add(C::SneakTime, ticks);
    m_stats.add(C::TimeSinceRest, ticks);

    const glm::vec3 delta = m_player.position - m_statsFrom;
    m_statsFrom = m_player.position;
    if (glm::length(delta) > 10.0f * static_cast<float>(ticks) || glm::length(delta) < 1.0e-5f) return;
    const long long whole = std::lround(glm::length(delta) * 100.0f);
    const long long flat = std::lround(glm::length(glm::vec2(delta.x, delta.z)) * 100.0f);

    if (const Mob* mount = riddenMob())
    {
        const MobId kind = mount->typeId();
        if (kind == MobId::Minecart) m_stats.add(C::MinecartOneCm, whole);
        else if (isBoatMob(kind)) m_stats.add(C::BoatOneCm, whole);
        else if (kind == MobId::Pig) m_stats.add(C::PigOneCm, whole);
        else if (kind == MobId::Horse || kind == MobId::Llama || kind == MobId::TraderLlama || kind == MobId::Camel)
            m_stats.add(C::HorseOneCm, whole);
        else if (kind == MobId::Strider) m_stats.add(C::StriderOneCm, whole);
        return;
    }

    const bool under = m_player.isHeadUnderwater(*m_world);
    if (inWater && under && m_player.sprinting) m_stats.add(C::SwimOneCm, whole);
    else if (under) m_stats.add(C::WalkUnderWaterOneCm, whole);
    else if (inWater) m_stats.add(C::WalkOnWaterOneCm, flat);
    else if (m_player.onLadder(*m_world))
    {
        if (delta.y > 0.0f) m_stats.add(C::ClimbOneCm, std::lround(delta.y * 100.0f));
    }
    else if (m_player.onGround)
    {
        if (m_player.sprinting) m_stats.add(C::SprintOneCm, flat);
        else if (m_player.sneaking) m_stats.add(C::CrouchOneCm, flat);
        else m_stats.add(C::WalkOneCm, flat);
    }
    else if (m_player.gliding) m_stats.add(C::AviateOneCm, whole);
    else if (flat > 25 * ticks) m_stats.add(C::FlyOneCm, flat);
}

void Application::openStatistics()
{
    drainStatEvents();
    m_screen = Screen::Statistics;
    m_statsTab = 0;
    m_statsScroll = 0.0f;
    m_statsScrolling = false;
    m_statsSelected = -1;
    setMouseCaptured(false);
}

std::vector<StackId> Application::statItemRows() const
{
    std::vector<StackId> rows;
    for (const auto& [id, counts] : m_stats.items())
    {
        bool any = false;
        for (long long count : counts) any = any || count > 0;
        if (any && id != Blocks::Air) rows.push_back(static_cast<StackId>(id));
    }
    if (m_statsSort >= 0 && m_statsOrder != 0)
    {
        const Stats::ItemStat stat = static_cast<Stats::ItemStat>(m_statsSort);
        auto value = [&](StackId id) -> long long {
            if (stat == Stats::ItemStat::Mined && isItem(id)) return -1;
            return m_stats.count(stat, id);
        };
        std::stable_sort(rows.begin(), rows.end(), [&](StackId a, StackId b) {
            const long long va = value(a), vb = value(b);
            if (va == vb) return m_statsOrder * (a < b ? -1 : (a > b ? 1 : 0)) < 0;
            return m_statsOrder * (va < vb ? -1 : 1) < 0;
        });
    }
    return rows;
}

std::vector<int> Application::statMobRows() const
{
    std::vector<int> rows;
    for (const auto& [mob, counts] : m_stats.mobs())
        if ((counts[0] > 0 || counts[1] > 0) && mob >= 0 && mob < mobTypeCount()) rows.push_back(mob);
    return rows;
}

void Application::renderStatsScreen()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    const bool clicked = !m_automated && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT);
    const bool held = !m_automated && m_input.isMouseButtonDown(SDL_BUTTON_LEFT);

    if (!guiTiled("inworld_menu_background", 0.0f, 0.0f, w, h, 32.0f))
        m_ui.quad(0.0f, 0.0f, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));

    const std::vector<StackId> items = statItemRows();
    const std::vector<int> mobs = statMobRows();
    if ((m_statsTab == 1 && items.empty()) || (m_statsTab == 2 && mobs.empty())) m_statsTab = 0;
    const int tab = m_statsTab;
    const int count = tab == 0 ? Stats::CUSTOM_COUNT : tab == 1 ? static_cast<int>(items.size()) : static_cast<int>(mobs.size());

    const float listY = STATS_HEADER;
    const float listH = std::max(0.0f, h - STATS_HEADER - STATS_FOOTER);
    const float listBottom = listY + listH;
    const float rowH = STATS_ROW_HEIGHT[tab];
    const float headH = STATS_LIST_HEADER[tab];
    const float maxPosition = static_cast<float>(count) * rowH + headH;
    const float maxScroll = std::max(0.0f, maxPosition - (listH - 4.0f));
    const float boxLeft = std::floor(w / 2.0f) - STATS_ROW_WIDTH / 2.0f;
    const float rowLeft = boxLeft + 2.0f;
    const float scrollbarX = boxLeft + STATS_ROW_WIDTH + 10.0f;
    const bool overList = mouseY >= listY && mouseY <= listBottom && mouseX >= 0.0f && mouseX <= w;

    if (overList && m_input.wheelDelta() != 0 && !m_automated)
        m_statsScroll -= static_cast<float>(m_input.wheelDelta()) * rowH / 2.0f;
    const float thumbH = maxPosition > 0.0f ? std::clamp(std::floor(listH * listH / maxPosition), 32.0f, std::max(32.0f, listH - 8.0f)) : listH;
    if (clicked) m_statsScrolling = overList && maxScroll > 0.0f && mouseX >= scrollbarX && mouseX < scrollbarX + 6.0f;
    if (!held) m_statsScrolling = false;
    if (m_statsScrolling)
    {
        if (mouseY < listY) m_statsScroll = 0.0f;
        else if (mouseY > listBottom) m_statsScroll = maxScroll;
        else m_statsScroll += (mouseY - m_statsGrabY) * std::max(1.0f, std::max(1.0f, maxScroll) / std::max(1.0f, listH - thumbH));
    }
    m_statsGrabY = mouseY;
    m_statsScroll = std::clamp(m_statsScroll, 0.0f, maxScroll);
    const float scroll = std::floor(m_statsScroll);

    auto rowTop = [&](int index) { return listY + 4.0f - scroll + static_cast<float>(index) * rowH + headH; };
    int hovered = -1;
    if (overList && mouseX >= boxLeft && mouseX <= boxLeft + STATS_ROW_WIDTH)
    {
        const int m = static_cast<int>(std::floor(mouseY - listY)) - static_cast<int>(headH) + static_cast<int>(scroll) - 4;
        const int n = m / static_cast<int>(rowH);
        if (m >= 0 && n >= 0 && n < count) hovered = n;
    }

    if (clicked && overList && !m_statsScrolling)
    {
        if (hovered >= 0) m_statsSelected = hovered;
        else if (tab == 1)
        {
            const float relX = mouseX - boxLeft;
            const float relY = mouseY - listY + scroll - 4.0f;
            if (relY >= 0.0f && relY < headH)
                for (int i = 0; i < Stats::ITEM_STATS; ++i)
                {
                    const float j = relX - statsColumn(i);
                    if (j >= -36.0f && j <= 0.0f)
                    {
                        m_statsPressed = i;
                        if (m_statsSort != i) { m_statsSort = i; m_statsOrder = -1; }
                        else if (m_statsOrder == -1) m_statsOrder = 1;
                        else { m_statsSort = -1; m_statsOrder = 0; }
                        m_audio.play(Sound::Click, 0.45f);
                        break;
                    }
                }
        }
    }
    if (!held) m_statsPressed = -1;

    if (!guiHas("inworld_menu_list_background"))
        m_ui.quad(0.0f, listY, w, listH, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));
    m_ui.setClip(0.0f, listY, w, listH);
    if (guiHas("inworld_menu_list_background"))
    {
        const float shiftX = std::fmod(w, 32.0f);
        const float shiftY = std::fmod(listBottom + scroll, 32.0f);
        for (float ty = listY - shiftY; ty < listBottom; ty += 32.0f)
            for (float tx = -shiftX; tx < w; tx += 32.0f)
                guiBlit("inworld_menu_list_background", tx, ty, 0.0f, 0.0f, 32.0f, 32.0f, 32.0f, 32.0f);
    }

    if (tab == 1)
    {
        const float hx = rowLeft;
        const float hy = listY + 4.0f - scroll;
        for (int i = 0; i < Stats::ITEM_STATS; ++i)
        {
            const float cx = hx + statsColumn(i) - 18.0f;
            const bool pressed = m_statsPressed == i;
            if (!guiSprite(pressed ? "sprites/container/slot" : "sprites/statistics/header", cx, hy + 1.0f, 18.0f, 18.0f))
            {
                if (pressed) guiWell(cx, hy + 1.0f, 18.0f, 18.0f);
                else
                {
                    m_ui.quad(cx, hy + 1.0f, 18.0f, 18.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                    m_ui.quad(cx + 1.0f, hy + 2.0f, 16.0f, 16.0f, glm::vec4(1.0f));
                    m_ui.quad(cx + 2.0f, hy + 3.0f, 15.0f, 15.0f, glm::vec4(0.33f, 0.33f, 0.33f, 1.0f));
                    m_ui.quad(cx + 2.0f, hy + 3.0f, 14.0f, 14.0f, glm::vec4(0.776f, 0.776f, 0.776f, 1.0f));
                }
            }
        }
        if (m_statsSort >= 0)
        {
            const float ax = hx + statsColumn(m_statsSort) - 36.0f;
            if (!guiSprite(m_statsOrder == 1 ? "sprites/statistics/sort_up" : "sprites/statistics/sort_down", ax, hy + 1.0f, 18.0f, 18.0f))
            {
                const bool up = m_statsOrder == 1;
                for (int r = 0; r < 4; ++r)
                {
                    const float span = static_cast<float>(1 + r * 2);
                    const float ry = up ? hy + 7.0f + static_cast<float>(r) : hy + 13.0f - static_cast<float>(r);
                    m_ui.quad(ax + 9.0f - std::floor(span / 2.0f) - 1.0f, ry, span + 1.0f, 1.0f, glm::vec4(0.9f, 0.9f, 0.9f, 1.0f));
                }
            }
        }
        for (int i = 0; i < Stats::ITEM_STATS; ++i)
        {
            const float shift = m_statsPressed == i ? 1.0f : 0.0f;
            const float ix = hx + statsColumn(i) - 18.0f + shift;
            const float iy = hy + 1.0f + shift;
            if (guiSprite(STATS_ICONS[i], ix, iy, 18.0f, 18.0f)) continue;
            const glm::vec4 ink(0.25f, 0.25f, 0.25f, 1.0f);
            switch (i)
            {
                case 0:
                    m_ui.quad(ix + 5.0f, iy + 5.0f, 8.0f, 8.0f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
                    m_ui.quad(ix + 5.0f, iy + 5.0f, 8.0f, 2.0f, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
                    break;
                case 1:
                    m_ui.quad(ix + 4.0f, iy + 8.0f, 4.0f, 2.0f, ink);
                    m_ui.quad(ix + 10.0f, iy + 8.0f, 4.0f, 2.0f, ink);
                    m_ui.quad(ix + 8.0f, iy + 6.0f, 2.0f, 2.0f, ink);
                    break;
                case 2:
                    for (int gx = 0; gx < 2; ++gx)
                        for (int gy = 0; gy < 2; ++gy)
                            m_ui.quad(ix + 5.0f + 5.0f * static_cast<float>(gx), iy + 5.0f + 5.0f * static_cast<float>(gy), 3.0f, 3.0f, ink);
                    break;
                case 3:
                    m_ui.quad(ix + 8.0f, iy + 4.0f, 2.0f, 10.0f, ink);
                    m_ui.quad(ix + 5.0f, iy + 4.0f, 8.0f, 2.0f, ink);
                    break;
                default:
                {
                    const bool up = i == 4;
                    m_ui.quad(ix + 8.0f, iy + 4.0f, 2.0f, 10.0f, ink);
                    for (int r = 0; r < 3; ++r)
                    {
                        const float ry = up ? iy + 5.0f + static_cast<float>(r) : iy + 12.0f - static_cast<float>(r);
                        m_ui.quad(ix + 6.0f + static_cast<float>(r), ry, 6.0f - 2.0f * static_cast<float>(r), 1.0f, ink);
                    }
                    break;
                }
            }
        }
    }

    std::vector<int> order(static_cast<size_t>(tab == 0 ? Stats::CUSTOM_COUNT : 0));
    if (tab == 0)
    {
        for (int i = 0; i < Stats::CUSTOM_COUNT; ++i) order[static_cast<size_t>(i)] = i;
        std::stable_sort(order.begin(), order.end(), [](int a, int b) {
            return std::string(Stats::name(static_cast<Stats::Custom>(a))) < std::string(Stats::name(static_cast<Stats::Custom>(b)));
        });
    }

    for (int index = 0; index < count; ++index)
    {
        const float y = rowTop(index);
        if (y + rowH < listY || y > listBottom) continue;
        const float entryH = rowH - 4.0f;
        if (index == m_statsSelected)
        {
            const float left = std::floor((w - STATS_ROW_WIDTH) / 2.0f);
            m_ui.quad(left, y - 2.0f, STATS_ROW_WIDTH, entryH + 4.0f, glm::vec4(1.0f));
            m_ui.quad(left + 1.0f, y - 1.0f, STATS_ROW_WIDTH - 2.0f, entryH + 2.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        const glm::vec4& colour = index % 2 == 0 ? STATS_WHITE : STATS_ALT;
        if (tab == 0)
        {
            const Stats::Custom stat = static_cast<Stats::Custom>(order[static_cast<size_t>(index)]);
            const float ty = y + std::floor(entryH / 2.0f) - 4.0f;
            m_ui.textWithShadow(Stats::name(stat), rowLeft + 2.0f, ty, 1.0f, colour);
            const std::string value = Stats::formatted(Stats::format(stat), m_stats.get(stat));
            m_ui.textWithShadow(value, rowLeft + STATS_ROW_WIDTH - UIRenderer::textWidth(value, 1.0f) - 4.0f, ty, 1.0f, colour);
        }
        else if (tab == 1)
        {
            const StackId id = items[static_cast<size_t>(index)];
            if (!guiSprite("sprites/container/slot", rowLeft, y, 18.0f, 18.0f)) guiWell(rowLeft, y, 18.0f, 18.0f);
            drawItem(rowLeft + 1.0f, y + 1.0f, SLOT, id, 1, glm::vec4(1.0f));
            const float ty = y + std::floor(entryH / 2.0f) - 4.0f;
            for (int i = 0; i < Stats::ITEM_STATS; ++i)
            {
                const Stats::ItemStat stat = static_cast<Stats::ItemStat>(i);
                const std::string text = (stat == Stats::ItemStat::Mined && isItem(id))
                                             ? std::string("-")
                                             : Stats::formatted(Stats::Format::Plain, m_stats.count(stat, id));
                m_ui.textWithShadow(text, rowLeft + statsColumn(i) - UIRenderer::textWidth(text, 1.0f), ty, 1.0f, colour);
            }
        }
        else
        {
            const int mob = mobs[static_cast<size_t>(index)];
            const std::string name = mobType(static_cast<MobId>(mob)).name;
            const long long killed = m_stats.kills(mob);
            const long long killedBy = m_stats.deathsBy(mob);
            m_ui.textWithShadow(name, rowLeft + 2.0f, y + 1.0f, 1.0f, STATS_WHITE);
            const std::string first = killed > 0 ? "You killed " + std::to_string(killed) + " " + name : "You have never killed " + name;
            const std::string second = killedBy > 0 ? name + " killed you " + std::to_string(killedBy) + " time(s)"
                                                    : "You have never been killed by " + name;
            m_ui.textWithShadow(first, rowLeft + 12.0f, y + 10.0f, 1.0f, killed > 0 ? STATS_ALT : STATS_GRAY);
            m_ui.textWithShadow(second, rowLeft + 12.0f, y + 19.0f, 1.0f, killedBy > 0 ? STATS_ALT : STATS_GRAY);
        }
    }
    m_ui.clearClip();

    auto separator = [&](const char* path, float y, bool top) {
        if (guiHas(path))
        {
            for (float tx = 0.0f; tx < w; tx += 32.0f)
                guiBlit(path, tx, y, 0.0f, 0.0f, std::min(32.0f, w - tx), 2.0f, 32.0f, 2.0f);
            return;
        }
        m_ui.quad(0.0f, y, w, 1.0f, top ? glm::vec4(0.0f, 0.0f, 0.0f, 0.75f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.25f));
        m_ui.quad(0.0f, y + 1.0f, w, 1.0f, top ? glm::vec4(1.0f, 1.0f, 1.0f, 0.25f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));
    };
    separator("inworld_header_separator", listY - 2.0f, true);
    separator("inworld_footer_separator", listBottom, false);

    if (maxScroll > 0.0f)
    {
        const float thumbY = std::max(listY, std::floor(scroll * (listH - thumbH) / maxScroll + listY));
        if (!guiSprite("sprites/widget/scroller_background", scrollbarX, listY, 6.0f, listH))
            m_ui.quad(scrollbarX, listY, 6.0f, listH, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        if (!guiSprite("sprites/widget/scroller", scrollbarX, thumbY, 6.0f, thumbH))
        {
            m_ui.quad(scrollbarX, thumbY, 6.0f, thumbH, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
            m_ui.quad(scrollbarX, thumbY, 5.0f, thumbH - 1.0f, glm::vec4(0.75f, 0.75f, 0.75f, 1.0f));
        }
    }

    std::string tip;
    if (tab == 1 && mouseY >= listY && mouseY <= listBottom)
    {
        if (hovered >= 0)
        {
            if (mouseX >= rowLeft && mouseX <= rowLeft + 18.0f) tip = displayName(items[static_cast<size_t>(hovered)]);
        }
        else
        {
            const float j = mouseX - rowLeft;
            for (int k = 0; k < Stats::ITEM_STATS && tip.empty(); ++k)
                if (j >= statsColumn(k) - 18.0f && j <= statsColumn(k)) tip = STATS_TYPE_NAMES[k];
        }
    }

    const std::string title = "Statistics";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 12.0f, 1.0f, STATS_WHITE);

    const float rowY = h - STATS_FOOTER + 6.0f;
    const float rowX = std::floor((w - 370.0f) / 2.0f);
    m_ui.setScale(unit * MENU_UNIT);
    auto button = [&](float x, float y, float bw, const char* label, bool enabled) {
        return menuButton({ x / MENU_UNIT, y / MENU_UNIT, bw / MENU_UNIT, 20.0f / MENU_UNIT }, label, enabled);
    };
    if (button(rowX, rowY, 120.0f, "General", true)) { m_statsTab = 0; m_statsScroll = 0.0f; m_statsSelected = -1; m_audio.play(Sound::Click, 0.45f); }
    if (button(rowX + 125.0f, rowY, 120.0f, "Items", !items.empty())) { m_statsTab = 1; m_statsScroll = 0.0f; m_statsSelected = -1; m_audio.play(Sound::Click, 0.45f); }
    if (button(rowX + 250.0f, rowY, 120.0f, "Mobs", !mobs.empty())) { m_statsTab = 2; m_statsScroll = 0.0f; m_statsSelected = -1; m_audio.play(Sound::Click, 0.45f); }
    if (button(std::floor((w - 200.0f) / 2.0f), rowY + 25.0f, 200.0f, "Done", true))
    {
        m_audio.play(Sound::Click, 0.45f);
        m_screen = Screen::Paused;
    }
    m_ui.setScale(unit);
    if (!tip.empty()) drawTooltip(tip, mouseX, mouseY);
    m_ui.setScale(1.0f);
}

void Application::saveStats()
{
    if (std::FILE* file = std::fopen((m_savePath + "/stats.txt").c_str(), "wb"))
    {
        for (const std::string& line : m_stats.lines()) std::fprintf(file, "%s\n", line.c_str());
        std::fclose(file);
    }
}

void Application::loadStats()
{
    std::vector<std::string> lines;
    if (std::FILE* file = std::fopen((m_savePath + "/stats.txt").c_str(), "rb"))
    {
        char line[256];
        while (std::fgets(line, sizeof(line), file))
        {
            std::string text(line);
            while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
            if (!text.empty()) lines.push_back(text);
        }
        std::fclose(file);
    }
    m_stats.load(lines);
    m_statsPlaced = false;
    m_statsFall = 0.0f;
    m_statsClock = 0.0f;
    m_lastAttacker = -1;
}
