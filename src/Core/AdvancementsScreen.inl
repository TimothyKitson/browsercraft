StackId Application::stackNamed(const std::string& id) const
{
    return RecipeBook::stackNamed(id);
}

void Application::award(const char* id, const char* criterion)
{
    if (m_player.spectator() || !m_world) return;
    m_advancements.grant(id, criterion);
}

void Application::awardAll(const char* id)
{
    if (m_player.spectator() || !m_world) return;
    m_advancements.grantAll(id);
}

bool Application::inventoryHolds(const std::string& items) const
{
    std::istringstream in(items);
    std::string word;
    bool any = false;
    while (in >> word)
    {
        any = true;
        bool found = false;
        std::istringstream alternatives(word);
        std::string option;
        while (std::getline(alternatives, option, '|') && !found)
        {
            const StackId wanted = stackNamed(option);
            if (wanted == Blocks::Air) continue;
            for (int i = 0; i < Inventory::TOTAL_SLOTS && !found; ++i)
                if (m_inventory.slot(i).id == wanted && !m_inventory.slot(i).empty()) found = true;
            if (!found && m_inventory.slot(Inventory::OFFHAND).id == wanted && !m_inventory.slot(Inventory::OFFHAND).empty()) found = true;
            if (!found && m_inventory.cursor().id == wanted && !m_inventory.cursor().empty()) found = true;
        }
        if (!found) return false;
    }
    return any;
}

void Application::tickAdvancements(float deltaTime)
{
    if (!m_world) return;
    if (const Mob* boat = riddenMob(); boat && (boat->typeId() == MobId::Boat || boat->typeId() == MobId::Raft))
        for (const Mob& goat : m_entities.mobs())
            if (goat.typeId() == MobId::Goat && goat.alive() && goat.mount() == boat->serial())
                award("husbandry/ride_a_boat_with_a_goat", "ride_a_boat_with_a_goat");
    if (m_entities.takeAxolotlAssists() > 0 && m_player.health > 0)
    {
        Effects::give(m_player.effects, Effects::Kind::Regeneration, 0, 5.0f);
        Effects::remove(m_player.effects, Effects::Kind::MiningFatigue);
        award("husbandry/kill_axolotl_target", "kill_axolotl_target");
    }
    if (m_player.honeySliding)
    {
        m_player.honeySliding = false;
        award("adventure/honey_block_slide", "honey_block_slide");
    }
    if (m_windLaunched)
    {
        m_windLaunchAge += deltaTime;
        if (m_player.position.y - m_windLaunchY >= 8.0f) award("adventure/who_needs_rockets", "who_needs_rockets");
        if ((m_player.onGround && m_windLaunchAge > 0.3f) || m_windLaunchAge > 10.0f) m_windLaunched = false;
    }
    if (!m_player.onGround && !m_player.flying && !m_player.isInWater(*m_world))
    {
        if (!m_fallTracking) { m_fallTracking = true; m_fallPeakY = m_player.position.y; }
        m_fallPeakY = std::max(m_fallPeakY, m_player.position.y);
    }
    else if (m_fallTracking)
    {
        m_fallTracking = false;
        if (m_player.onGround && m_player.health > 0 && m_fallPeakY >= static_cast<float>(Chunk::SY - 1) && m_player.position.y <= 6.0f)
            award("adventure/fall_from_world_height", "fall_from_world_height");
    }
    if (Mob* mount = riddenMob(); mount && mount->typeId() == MobId::Strider && m_dimension == Dimension::Overworld &&
                                  isLava(m_world->getBlock(static_cast<int>(std::floor(mount->position().x)),
                                                           static_cast<int>(std::floor(mount->position().y)),
                                                           static_cast<int>(std::floor(mount->position().z)))))
    {
        if (!m_striderRiding) { m_striderRiding = true; m_striderStart = mount->position(); }
        if (glm::length(glm::vec2(mount->position().x - m_striderStart.x, mount->position().z - m_striderStart.z)) >= 50.0f)
            award("nether/ride_strider_in_overworld_lava", "ride_entity_distance");
    }
    else if (!riddenMob() || riddenMob()->typeId() != MobId::Strider)
        m_striderRiding = false;
    {
        const int dimension = static_cast<int>(m_dimension);
        if (m_dimension == Dimension::Overworld && m_lastAdvDimension == static_cast<int>(Dimension::Nether) && m_netherEntryKnown &&
            glm::length(glm::vec2(m_player.position.x, m_player.position.z) - m_netherEntry) >= 7000.0f)
            award("nether/fast_travel", "travelled");
        if (m_dimension == Dimension::Nether && m_lastAdvDimension == static_cast<int>(Dimension::Overworld))
        {
            m_netherEntry = m_overworldSpot;
            m_netherEntryKnown = true;
        }
        if (m_dimension == Dimension::Overworld) m_overworldSpot = glm::vec2(m_player.position.x, m_player.position.z);
        m_lastAdvDimension = dimension;
    }
    m_advCheck += deltaTime;
    if (m_advCheck >= 0.5f)
    {
        m_advCheck = 0.0f;
        const std::vector<Advancements::Def>& defs = Advancements::all();
        for (size_t i = 0; i < defs.size(); ++i)
            for (const Advancements::Criterion& criterion : defs[i].criteria)
                if (criterion.items[0] != '\0' && !m_advancements.has(static_cast<int>(i), criterion.name) && inventoryHolds(criterion.items))
                    award(defs[i].id, criterion.name);

        if (m_dimension == Dimension::Nether)
        {
            award("story/enter_the_nether", "entered_nether");
            award("nether/root", "entered_nether");
        }
        if (m_dimension == Dimension::End)
        {
            award("story/enter_the_end", "entered_end");
            award("end/root", "entered_end");
        }

        using Effects::Kind;
        auto holding = [&](std::initializer_list<Kind> kinds) {
            for (Kind kind : kinds)
                if (!m_player.effects.has(kind)) return false;
            return true;
        };
        if (holding({ Kind::Speed, Kind::Slowness, Kind::Strength, Kind::JumpBoost, Kind::Regeneration, Kind::FireResistance,
                      Kind::WaterBreathing, Kind::Invisibility, Kind::NightVision, Kind::Weakness, Kind::Poison, Kind::SlowFalling,
                      Kind::Resistance, Kind::Oozing, Kind::Infested, Kind::WindCharged, Kind::Weaving }))
            award("nether/all_potions", "all_effects");
        if (holding({ Kind::Speed, Kind::Slowness, Kind::Strength, Kind::JumpBoost, Kind::Regeneration, Kind::FireResistance,
                      Kind::WaterBreathing, Kind::Invisibility, Kind::NightVision, Kind::Weakness, Kind::Poison, Kind::Wither,
                      Kind::Haste, Kind::MiningFatigue, Kind::Levitation, Kind::Glowing, Kind::Absorption, Kind::Hunger, Kind::Nausea,
                      Kind::Resistance, Kind::SlowFalling, Kind::ConduitPower, Kind::DolphinsGrace, Kind::Blindness, Kind::BadOmen,
                      Kind::HeroOfTheVillage, Kind::Darkness }))
            award("nether/all_effects", "all_effects");
        if (m_player.effects.has(Kind::HeroOfTheVillage)) award("adventure/hero_of_the_village", "hero_of_the_village");

        const int px = static_cast<int>(std::floor(m_player.position.x));
        const int py = static_cast<int>(std::floor(m_player.position.y));
        const int pz = static_cast<int>(std::floor(m_player.position.z));
        const uint32_t seed = m_world->seed();
        if (m_dimension == Dimension::Overworld)
        {
            const Stronghold::Site hold = Stronghold::nearest(px, pz, seed);
            if (hold.exists && std::abs(px - hold.x) <= Stronghold::MAX_RADIUS && std::abs(pz - hold.z) <= Stronghold::MAX_RADIUS &&
                py >= hold.y - 1 && py <= hold.y + Stronghold::HEADROOM + 2)
                award("story/follow_ender_eye", "in_stronghold");
            const TrialChamber::Site chamber = TrialChamber::siteFor(TrialChamber::regionOf(px), TrialChamber::regionOf(pz), seed);
            if (chamber.exists && std::abs(px - chamber.x) <= TrialChamber::MAX_RADIUS && std::abs(pz - chamber.z) <= TrialChamber::MAX_RADIUS &&
                py >= chamber.y - 1 && py <= chamber.y + TrialChamber::HEADROOM + 2)
                award("adventure/minecraft_trials_edition", "minecraft_trials_edition");
        }
        if (m_dimension == Dimension::Nether)
        {
            bool bricks = false;
            for (int dy = -2; dy <= 2 && !bricks; ++dy)
                for (int dx = -2; dx <= 2 && !bricks; ++dx)
                    for (int dz = -2; dz <= 2 && !bricks; ++dz)
                        if (m_world->getBlock(px + dx, py + dy, pz + dz) == Blocks::NetherBricks) bricks = true;
            if (bricks) award("nether/find_fortress", "fortress");
            const Structures::Spot bastion = m_world->generator().nearestBastion(px, pz, 1);
            if (bastion.exists)
            {
                const int bx = bastion.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int bz = bastion.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
                if (std::abs(px - bx) <= 40 && std::abs(pz - bz) <= 40) award("nether/find_bastion", "bastion");
            }
        }
        if (m_dimension == Dimension::End)
        {
            const Structures::Spot city = m_world->generator().nearestEndCity(px, pz, 1);
            if (city.exists)
            {
                const int cx = city.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
                const int cz = city.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
                if (std::abs(px - cx) <= 40 && std::abs(pz - cz) <= 40) award("end/find_end_city", "in_city");
            }
        }
        if (m_player.effects.has(Kind::Levitation))
        {
            if (!m_levitating)
            {
                m_levitating = true;
                m_levitateFrom = m_player.position.y;
            }
            if (m_player.position.y - m_levitateFrom >= 50.0f) award("end/levitate", "levitated");
        }
        else
            m_levitating = false;
        if (m_inventory.slot(Inventory::ARMOR_FIRST + 3).id == Items::LeatherBoots &&
            m_world->getBlock(px, static_cast<int>(std::floor(m_player.position.y - 0.05f)), pz) == Blocks::PowderSnow)
            award("adventure/walk_on_powder_snow_with_leather_boots", "walk_on_powder_snow_with_leather_boots");
        if (m_player.sneaking && glm::length(glm::vec2(m_player.velocity.x, m_player.velocity.z)) > 0.5f)
            for (const glm::ivec3& sensor : m_sensorSpots)
                if (glm::length(glm::vec3(sensor) + glm::vec3(0.5f) - m_player.position) <= 8.0f)
                {
                    award("adventure/avoid_vibration", "avoid_vibration");
                    break;
                }
        if (m_scoping)
            if (const Mob* seen = m_entities.pick(m_camera.position, m_camera.front, 96.0f))
            {
                if (seen->typeId() == MobId::Parrot) award("adventure/spyglass_at_parrot", "spyglass_at_parrot");
                if (seen->typeId() == MobId::Ghast) award("adventure/spyglass_at_ghast", "spyglass_at_ghast");
                if (seen->typeId() == MobId::EnderDragon) award("adventure/spyglass_at_dragon", "spyglass_at_dragon");
            }

        static const std::pair<Biome, const char*> BIOMES[] = {
            { Biome::Plains, "plains" }, { Biome::Desert, "desert" }, { Biome::Forest, "forest" }, { Biome::Swamp, "swamp" },
            { Biome::Jungle, "jungle" }, { Biome::Savanna, "savanna" }, { Biome::Badlands, "badlands" }, { Biome::Snowy, "snowy_plains" },
            { Biome::Ocean, "ocean" }, { Biome::Beach, "beach" }, { Biome::Mountains, "windswept_hills" },
            { Biome::MushroomFields, "mushroom_fields" }, { Biome::CherryGrove, "cherry_grove" }, { Biome::MangroveSwamp, "mangrove_swamp" },
            { Biome::NetherWastes, "nether_wastes" }, { Biome::CrimsonForest, "crimson_forest" }, { Biome::WarpedForest, "warped_forest" },
        };
        if (m_dimension != Dimension::End)
        {
            const Biome here = m_world->generator().biomeAt(static_cast<int>(std::floor(m_player.position.x)),
                                                            static_cast<int>(std::floor(m_player.position.z)));
            for (const auto& entry : BIOMES)
                if (entry.first == here)
                    award(m_dimension == Dimension::Nether ? "nether/explore_nether" : "adventure/adventuring_time", entry.second);
            if (m_dimension == Dimension::Overworld && insideAncientCity(m_player.position)) award("adventure/adventuring_time", "deep_dark");
        }
    }

    for (int index : m_advancements.takeNew())
    {
        const Advancements::Def& def = Advancements::all()[static_cast<size_t>(index)];
        if (def.toast)
        {
            Toast toast;
            toast.advancement = index;
            m_toasts.push_back(toast);
        }
        if (def.toast && m_rules.on(GameRules::Rule::AnnounceAdvancements))
        {
            const char* verb = def.frame == Advancements::Frame::Challenge ? " has completed the challenge ["
                             : def.frame == Advancements::Frame::Goal ? " has reached the goal [" : " has made the advancement [";
            m_chatLog.add((m_playerName.empty() ? std::string("Player") : m_playerName) + verb + def.title + "]");
        }
    }
    for (Toast& toast : m_toasts) toast.age += deltaTime;
}

void Application::openAdvancements()
{
    if (m_inventoryOpen) closeInventory();
    m_advOpen = true;
    m_advDragging = false;
    setMouseCaptured(false);
}

void Application::closeAdvancements()
{
    m_advOpen = false;
    m_advDragging = false;
    setMouseCaptured(true);
}

std::vector<int> Application::advancementTabs() const
{
    std::vector<int> out;
    for (int tab = 0; tab < Advancements::TAB_COUNT; ++tab)
        if (m_advancements.tabVisible(static_cast<Advancements::Tab>(tab))) out.push_back(tab);
    return out;
}

void Application::updateAdvancementsInput()
{
    if (!m_advOpen) return;
    if (m_input.wasKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        closeAdvancements();
        return;
    }
    const float unit = static_cast<float>(guiScale());
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    const float w = static_cast<float>(m_window.width()) / unit;
    const float h = static_cast<float>(m_window.height()) / unit;
    const float x = std::floor((w - 252.0f) * 0.5f);
    const float y = std::floor((h - 140.0f) * 0.5f);
    const std::vector<int> tabs = advancementTabs();
    if (m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT) && !m_automated)
    {
        if (tabs.size() > 1)
            for (size_t i = 0; i < tabs.size(); ++i)
            {
                const float tabX = x + 32.0f * static_cast<float>(i);
                const float tabY = y - 28.0f;
                if (mouseX > tabX && mouseX < tabX + 28.0f && mouseY > tabY && mouseY < tabY + 32.0f) m_advTab = tabs[i];
            }
        m_advDragging = mouseX >= x + 9.0f && mouseX < x + 243.0f && mouseY >= y + 18.0f && mouseY < y + 131.0f;
        m_advDragX = mouseX;
        m_advDragY = mouseY;
    }
    if (!m_input.isMouseButtonDown(SDL_BUTTON_LEFT)) m_advDragging = false;
    if (m_advDragging && m_advTab >= 0)
    {
        const int tab = m_advTab;
        m_advOriginX[tab] += mouseX - m_advDragX;
        m_advOriginY[tab] += mouseY - m_advDragY;
        m_advDragX = mouseX;
        m_advDragY = mouseY;
    }
}

void Application::renderAdvancementsScreen()
{
    m_ui.setScale(static_cast<float>(guiScale()));
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    m_ui.gradient(0.0f, 0.0f, w, h, glm::vec4(0.063f, 0.063f, 0.063f, 0.753f), glm::vec4(0.063f, 0.063f, 0.063f, 0.816f));
    const float x = std::floor((w - 252.0f) * 0.5f);
    const float y = std::floor((h - 140.0f) * 0.5f);
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    const std::vector<int> tabs = advancementTabs();
    if (std::find(tabs.begin(), tabs.end(), m_advTab) == tabs.end()) m_advTab = tabs.empty() ? -1 : tabs[0];
    const std::vector<Advancements::Def>& defs = Advancements::all();
    const float pageX = x + 9.0f, pageY = y + 18.0f;
    std::string hoverTitle;
    int hovered = -1;
    std::function<void()> tooltip;

    if (m_advTab < 0)
    {
        m_ui.quad(pageX, pageY, 234.0f, 113.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        const std::string empty = "There doesn't seem to be anything here...";
        m_ui.textWithShadow(empty, std::floor(pageX + 117.0f - UIRenderer::textWidth(empty, 1.0f) * 0.5f), pageY + 56.0f - 4.0f, 1.0f, glm::vec4(1.0f));
        m_ui.textWithShadow(":(", std::floor(pageX + 117.0f - UIRenderer::textWidth(":(", 1.0f) * 0.5f), pageY + 113.0f - 9.0f, 1.0f, glm::vec4(1.0f));
    }
    else
    {
        const Advancements::Tab tab = static_cast<Advancements::Tab>(m_advTab);
        std::vector<int> shown;
        int minX = 1 << 30, maxX = -(1 << 30), minY = 1 << 30, maxY = -(1 << 30);
        for (size_t i = 0; i < defs.size(); ++i)
        {
            if (defs[i].tab != tab || !m_advancements.visible(static_cast<int>(i))) continue;
            shown.push_back(static_cast<int>(i));
            const int wx = static_cast<int>(std::floor(defs[i].x * 28.0f));
            const int wy = static_cast<int>(std::floor(defs[i].y * 27.0f));
            minX = std::min(minX, wx);
            maxX = std::max(maxX, wx + 28);
            minY = std::min(minY, wy);
            maxY = std::max(maxY, wy + 27);
        }
        if (!m_advPlaced[m_advTab])
        {
            m_advOriginX[m_advTab] = static_cast<float>(117 - (maxX + minX) / 2);
            m_advOriginY[m_advTab] = static_cast<float>(56 - (maxY + minY) / 2);
            m_advPlaced[m_advTab] = true;
        }
        if (maxX - minX > 234) m_advOriginX[m_advTab] = std::clamp(m_advOriginX[m_advTab], static_cast<float>(-(maxX - 234)), 0.0f);
        else if (!m_advDragging) {}
        if (maxY - minY > 113) m_advOriginY[m_advTab] = std::clamp(m_advOriginY[m_advTab], static_cast<float>(-(maxY - 113)), 0.0f);
        const int ox = static_cast<int>(std::floor(m_advOriginX[m_advTab]));
        const int oy = static_cast<int>(std::floor(m_advOriginY[m_advTab]));

        m_ui.setClip(pageX, pageY, 234.0f, 113.0f);
        const int k = ox % 16;
        const int l = oy % 16;
        const char* background = Advancements::tabBackground(tab);
        for (int m = -1; m <= 15; ++m)
            for (int n = -1; n <= 8; ++n)
            {
                const float tx = pageX + static_cast<float>(k + 16 * m);
                const float ty = pageY + static_cast<float>(l + 16 * n);
                if (!guiSprite(background, tx, ty, 16.0f, 16.0f))
                {
                    const float shade = ((m + n) & 1) ? 0.33f : 0.36f;
                    m_ui.quad(tx, ty, 16.0f, 16.0f, glm::vec4(shade, shade, shade, 1.0f));
                }
            }

        auto hline = [&](int x1, int x2, int ly, const glm::vec4& colour) {
            const int lo = std::min(x1, x2), hi = std::max(x1, x2);
            m_ui.quad(pageX + static_cast<float>(lo), pageY + static_cast<float>(ly), static_cast<float>(hi - lo + 1), 1.0f, colour);
        };
        auto vline = [&](int lx, int y1, int y2, const glm::vec4& colour) {
            const int lo = std::min(y1, y2), hi = std::max(y1, y2);
            if (hi - lo - 1 > 0) m_ui.quad(pageX + static_cast<float>(lx), pageY + static_cast<float>(lo + 1), 1.0f, static_cast<float>(hi - lo - 1), colour);
        };
        for (int pass = 0; pass < 2; ++pass)
            for (int index : shown)
            {
                const int parent = Advancements::parentOf(index);
                if (parent < 0 || std::find(shown.begin(), shown.end(), parent) == shown.end()) continue;
                const int px = static_cast<int>(std::floor(defs[static_cast<size_t>(parent)].x * 28.0f));
                const int py = static_cast<int>(std::floor(defs[static_cast<size_t>(parent)].y * 27.0f));
                const int cx = static_cast<int>(std::floor(defs[static_cast<size_t>(index)].x * 28.0f));
                const int cy = static_cast<int>(std::floor(defs[static_cast<size_t>(index)].y * 27.0f));
                const int i0 = ox + px + 13, j0 = ox + px + 26 + 4, k0 = oy + py + 13, l0 = ox + cx + 13, m0 = oy + cy + 13;
                if (pass == 0)
                {
                    const glm::vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
                    hline(j0, i0, k0 - 1, black);
                    hline(j0 + 1, i0, k0, black);
                    hline(j0, i0, k0 + 1, black);
                    hline(l0, j0 - 1, m0 - 1, black);
                    hline(l0, j0 - 1, m0, black);
                    hline(l0, j0 - 1, m0 + 1, black);
                    vline(j0 - 1, m0, k0, black);
                    vline(j0 + 1, m0, k0, black);
                }
                else
                {
                    const glm::vec4 white(1.0f);
                    hline(j0, i0, k0, white);
                    hline(l0, j0, m0, white);
                    vline(j0, m0, k0, white);
                }
            }

        static const char* FRAMES[3][2] = {
            { "advancements/task_frame_unobtained", "advancements/task_frame_obtained" },
            { "advancements/goal_frame_unobtained", "advancements/goal_frame_obtained" },
            { "advancements/challenge_frame_unobtained", "advancements/challenge_frame_obtained" } };
        for (int index : shown)
        {
            const Advancements::Def& def = defs[static_cast<size_t>(index)];
            const float wx = pageX + static_cast<float>(ox + static_cast<int>(std::floor(def.x * 28.0f)));
            const float wy = pageY + static_cast<float>(oy + static_cast<int>(std::floor(def.y * 27.0f)));
            const bool got = m_advancements.done(index);
            if (!guiSprite(FRAMES[static_cast<int>(def.frame)][got ? 1 : 0], wx + 3.0f, wy, 26.0f, 26.0f))
            {
                m_ui.quad(wx + 3.0f, wy, 26.0f, 26.0f, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
                m_ui.quad(wx + 5.0f, wy + 2.0f, 22.0f, 22.0f, got ? glm::vec4(0.85f, 0.7f, 0.2f, 1.0f) : glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));
            }
            drawItem(wx + 8.0f, wy + 5.0f, SLOT, stackNamed(def.icon), 1, glm::vec4(1.0f));
            if (mouseX > pageX && mouseX < pageX + 234.0f && mouseY > pageY && mouseY < pageY + 113.0f &&
                mouseX >= wx + 3.0f && mouseX < wx + 29.0f && mouseY >= wy && mouseY < wy + 26.0f && hovered < 0)
                hovered = index;
            if (index == m_devAdvHover) hovered = index;
        }
        m_ui.clearClip();

        m_advHoverFade = hovered >= 0 ? std::min(0.3f, m_advHoverFade + 0.02f) : std::max(0.0f, m_advHoverFade - 0.04f);
        if (m_advHoverFade > 0.0f) m_ui.quad(pageX, pageY, 234.0f, 113.0f, glm::vec4(0.0f, 0.0f, 0.0f, m_advHoverFade));
        if (hovered >= 0)
            tooltip = [&, hovered, ox, oy]()
        {
            const Advancements::Def& def = defs[static_cast<size_t>(hovered)];
            const int wx = ox + static_cast<int>(std::floor(def.x * 28.0f));
            const int wy = oy + static_cast<int>(std::floor(def.y * 27.0f));
            const int needed = m_advancements.criteriaNeeded(hovered);
            const int have = m_advancements.criteriaDone(hovered);
            const std::string progress = needed > 1 ? std::to_string(have) + "/" + std::to_string(needed) : std::string();
            const float progressWidth = progress.empty() ? 0.0f : UIRenderer::textWidth(std::to_string(needed) + "/" + std::to_string(needed), 1.0f) + 8.0f;
            float width = 29.0f + UIRenderer::textWidth(def.title, 1.0f) + progressWidth;
            auto wrap = [&](float limit) {
                std::vector<std::string> out;
                std::istringstream words(def.description);
                std::string word, line;
                while (words >> word)
                {
                    const std::string attempt = line.empty() ? word : line + " " + word;
                    if (!line.empty() && UIRenderer::textWidth(attempt, 1.0f) > limit) { out.push_back(line); line = word; }
                    else line = attempt;
                }
                if (!line.empty()) out.push_back(line);
                return out;
            };
            std::vector<std::string> lines;
            float closest = 1.0e9f;
            for (float offset : { 0.0f, 10.0f, -10.0f, 25.0f, -25.0f })
            {
                const std::vector<std::string> tried = wrap(width - offset);
                float widest = 0.0f;
                for (const std::string& line : tried) widest = std::max(widest, UIRenderer::textWidth(line, 1.0f));
                const float gap = std::fabs(widest - width);
                if (gap <= 10.0f) { lines = tried; break; }
                if (gap < closest) { closest = gap; lines = tried; }
            }
            for (const std::string& line : lines) width = std::max(width, UIRenderer::textWidth(line, 1.0f));
            width = std::floor(width) + 8.0f;
            const bool flip = x + 9.0f + static_cast<float>(wx) + width + 26.0f >= w;
            const bool above = 113 - oy - static_cast<int>(std::floor(def.y * 27.0f)) - 26 <= 6 + static_cast<int>(lines.size()) * 9;
            const float fraction = needed > 0 ? static_cast<float>(have) / static_cast<float>(needed) : 0.0f;
            float split = std::floor(fraction * width);
            bool leftGot = true, rightGot = false, frameGot = false;
            if (fraction >= 1.0f) { split = std::floor(width / 2.0f); leftGot = rightGot = frameGot = true; }
            else if (split < 2.0f) { split = std::floor(width / 2.0f); leftGot = false; }
            else if (split > width - 2.0f) { split = std::floor(width / 2.0f); rightGot = true; }
            const float boxX = pageX + static_cast<float>(flip ? wx - static_cast<int>(width) + 26 + 6 : wx);
            const float boxY = pageY + static_cast<float>(wy);
            const float tall = 32.0f + static_cast<float>(lines.size()) * 9.0f;
            if (!lines.empty() && !guiNineSlice("advancements/title_box", boxX, above ? boxY + 26.0f - tall : boxY, width, tall, 1.0f))
                m_ui.quad(boxX, above ? boxY + 26.0f - tall : boxY, width, tall, glm::vec4(0.0f, 0.0f, 0.0f, 0.9f));
            const glm::vec4 got(0.85f, 0.65f, 0.1f, 1.0f), notGot(0.0f, 0.35f, 0.55f, 1.0f);
            if (!guiSpritePart(leftGot ? "advancements/box_obtained" : "advancements/box_unobtained", 200.0f, 26.0f, 0.0f, 0.0f, boxX, boxY, split, 26.0f))
                m_ui.quad(boxX, boxY, split, 26.0f, leftGot ? got : notGot);
            const float rest = width - split;
            if (!guiSpritePart(rightGot ? "advancements/box_obtained" : "advancements/box_unobtained", 200.0f, 26.0f, 200.0f - rest, 0.0f, boxX + split, boxY, rest, 26.0f))
                m_ui.quad(boxX + split, boxY, rest, 26.0f, rightGot ? got : notGot);
            const float frameX = pageX + static_cast<float>(wx) + 3.0f;
            if (!guiSprite(FRAMES[static_cast<int>(def.frame)][frameGot ? 1 : 0], frameX, boxY, 26.0f, 26.0f))
                m_ui.quad(frameX, boxY, 26.0f, 26.0f, frameGot ? got : glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));
            if (flip)
            {
                m_ui.textWithShadow(def.title, boxX + 5.0f, boxY + 9.0f, 1.0f, glm::vec4(1.0f));
                if (!progress.empty())
                    m_ui.textWithShadow(progress, pageX + static_cast<float>(wx) - UIRenderer::textWidth(progress, 1.0f), boxY + 9.0f, 1.0f, glm::vec4(1.0f));
            }
            else
            {
                m_ui.textWithShadow(def.title, pageX + static_cast<float>(wx) + 32.0f, boxY + 9.0f, 1.0f, glm::vec4(1.0f));
                if (!progress.empty())
                    m_ui.textWithShadow(progress, pageX + static_cast<float>(wx) + width - UIRenderer::textWidth(progress, 1.0f) - 5.0f, boxY + 9.0f, 1.0f, glm::vec4(1.0f));
            }
            const unsigned int c = Advancements::frameColour(def.frame);
            const glm::vec4 descColour(((c >> 16) & 255u) / 255.0f, ((c >> 8) & 255u) / 255.0f, (c & 255u) / 255.0f, 1.0f);
            for (size_t o = 0; o < lines.size(); ++o)
            {
                const float ly = above ? boxY + 26.0f - tall + 7.0f + 9.0f * static_cast<float>(o) : boxY + 9.0f + 17.0f + 9.0f * static_cast<float>(o);
                m_ui.text(lines[o], boxX + 5.0f, ly, 1.0f, descColour);
            }
            drawItem(pageX + static_cast<float>(wx) + 8.0f, boxY + 5.0f, SLOT, stackNamed(def.icon), 1, glm::vec4(1.0f));
        };
    }

    if (!guiBlit("advancements/window", x, y, 0.0f, 0.0f, 252.0f, 140.0f))
    {
        m_ui.quad(x, y, 252.0f, 18.0f, glm::vec4(0.78f, 0.78f, 0.78f, 1.0f));
        m_ui.quad(x, y + 131.0f, 252.0f, 9.0f, glm::vec4(0.78f, 0.78f, 0.78f, 1.0f));
        m_ui.quad(x, y + 18.0f, 9.0f, 113.0f, glm::vec4(0.78f, 0.78f, 0.78f, 1.0f));
        m_ui.quad(x + 243.0f, y + 18.0f, 9.0f, 113.0f, glm::vec4(0.78f, 0.78f, 0.78f, 1.0f));
    }
    if (tabs.size() > 1)
    {
        for (size_t i = 0; i < tabs.size(); ++i)
        {
            const bool chosen = tabs[i] == m_advTab;
            const float tabX = x + 32.0f * static_cast<float>(i);
            const float tabY = y - 28.0f;
            const char* sprite = i == 0 ? (chosen ? "advancements/tab_above_left_selected" : "advancements/tab_above_left")
                                        : (chosen ? "advancements/tab_above_middle_selected" : "advancements/tab_above_middle");
            if (!guiSprite(sprite, tabX, tabY, 28.0f, 32.0f))
            {
                m_ui.quad(tabX, tabY, 28.0f, 32.0f, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
                m_ui.quad(tabX + 1.0f, tabY + 1.0f, 26.0f, 31.0f, chosen ? glm::vec4(0.78f, 0.78f, 0.78f, 1.0f) : glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));
            }
        }
        for (size_t i = 0; i < tabs.size(); ++i)
        {
            const float tabX = x + 32.0f * static_cast<float>(i);
            const float tabY = y - 28.0f;
            const int root = Advancements::rootOf(static_cast<Advancements::Tab>(tabs[i]));
            if (root >= 0) drawItem(tabX + 6.0f, tabY + 9.0f, SLOT, stackNamed(defs[static_cast<size_t>(root)].icon), 1, glm::vec4(1.0f));
            if (mouseX > tabX && mouseX < tabX + 28.0f && mouseY > tabY && mouseY < tabY + 32.0f && root >= 0)
                hoverTitle = defs[static_cast<size_t>(root)].title;
        }
    }
    const int root = m_advTab >= 0 ? Advancements::rootOf(static_cast<Advancements::Tab>(m_advTab)) : -1;
    m_ui.text(root >= 0 ? defs[static_cast<size_t>(root)].title : "Advancements", x + 8.0f, y + 6.0f, 1.0f, GUI_TEXT);
    if (tooltip) tooltip();
    if (!hoverTitle.empty()) drawTooltip(hoverTitle, mouseX, mouseY);
    m_ui.setScale(1.0f);
}

void Application::renderToasts()
{
    if (m_toasts.empty()) return;
    m_ui.setScale(static_cast<float>(guiScale()));
    const float w = m_ui.canvasWidth();
    int slot = 0;
    for (size_t i = 0; i < m_toasts.size() && slot < 5;)
    {
        Toast& toast = m_toasts[i];
        const bool tutorial = toast.tutorialIcon >= 0;
        if ((!tutorial && toast.age >= 5.6f) || (tutorial && toast.hidden && toast.age - toast.hiddenAt >= 0.6f))
        {
            m_toasts.erase(m_toasts.begin() + static_cast<long>(i));
            continue;
        }
        if (!toast.sounded)
        {
            toast.sounded = true;
            m_audio.play(Sound::ToastIn, 0.7f, 1.0f);
            if (toast.advancement >= 0 && Advancements::all()[static_cast<size_t>(toast.advancement)].frame == Advancements::Frame::Challenge)
                m_audio.play(Sound::ChallengeComplete, 0.8f, 1.0f);
        }
        const float shown = tutorial ? (toast.hidden ? std::max(0.0f, 1.0f - (toast.age - toast.hiddenAt) / 0.6f) : std::min(1.0f, toast.age / 0.6f))
                          : toast.age < 0.6f ? toast.age / 0.6f : toast.age > 5.0f ? std::max(0.0f, 1.0f - (toast.age - 5.0f) / 0.6f) : 1.0f;
        if (tutorial) toast.shownProgress += (toast.progress - toast.shownProgress) * 0.25f;
        const float wide = toast.systemTitle.empty() ? 160.0f : std::max(160.0f, 30.0f + UIRenderer::textWidth(toast.systemTitle, 1.0f));
        const float tx = w - wide * shown;
        const float ty = 32.0f * static_cast<float>(slot);
        if (tutorial) drawTutorialToast(toast, tx, ty);
        else if (!toast.systemTitle.empty())
        {
            if (!guiSprite("sprites/toast/system", tx, ty, wide, 32.0f))
            {
                m_ui.quad(tx, ty, wide, 32.0f, glm::vec4(0.13f, 0.13f, 0.13f, 0.95f));
                m_ui.quad(tx, ty, 2.0f, 32.0f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
            }
            m_ui.text(toast.systemTitle, tx + 18.0f, ty + 12.0f, 1.0f, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        }
        else if (toast.advancement >= 0)
        {
            const Advancements::Def& def = Advancements::all()[static_cast<size_t>(toast.advancement)];
            if (!guiSprite("toast/advancement", tx, ty, 160.0f, 32.0f))
            {
                m_ui.quad(tx, ty, 160.0f, 32.0f, glm::vec4(0.13f, 0.13f, 0.13f, 0.95f));
                m_ui.quad(tx + 1.0f, ty + 1.0f, 158.0f, 30.0f, glm::vec4(0.25f, 0.25f, 0.25f, 0.95f));
            }
            const glm::vec4 heading = def.frame == Advancements::Frame::Challenge ? glm::vec4(1.0f, 0.533f, 1.0f, 1.0f) : glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            m_ui.text(Advancements::frameText(def.frame), tx + 30.0f, ty + 7.0f, 1.0f, heading);
            std::string title = def.title;
            while (UIRenderer::textWidth(title, 1.0f) > 125.0f && title.size() > 3) Utf8::popBack(title);
            m_ui.text(title, tx + 30.0f, ty + 18.0f, 1.0f, glm::vec4(1.0f));
            drawItem(tx + 8.0f, ty + 8.0f, SLOT, stackNamed(def.icon), 1, glm::vec4(1.0f));
        }
        else
        {
            if (!guiSprite("toast/recipe", tx, ty, 160.0f, 32.0f))
            {
                m_ui.quad(tx, ty, 160.0f, 32.0f, glm::vec4(0.75f, 0.75f, 0.75f, 0.95f));
                m_ui.quad(tx + 1.0f, ty + 1.0f, 158.0f, 30.0f, glm::vec4(0.9f, 0.9f, 0.9f, 0.95f));
            }
            m_ui.text("New Recipes Unlocked!", tx + 30.0f, ty + 7.0f, 1.0f, glm::vec4(0.314f, 0.0f, 0.314f, 1.0f));
            m_ui.text("Check your recipe book", tx + 30.0f, ty + 18.0f, 1.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            if (!toast.recipes.empty())
            {
                const size_t pick = static_cast<size_t>(toast.age / std::max(0.1f, 5.0f / static_cast<float>(toast.recipes.size()))) % toast.recipes.size();
                drawItem(tx + 1.8f, ty + 1.8f, 9.6f, Blocks::CraftingTable, 1, glm::vec4(1.0f));
                drawItem(tx + 8.0f, ty + 8.0f, SLOT, toast.recipes[pick], 1, glm::vec4(1.0f));
            }
        }
        ++slot;
        ++i;
    }
    m_ui.setScale(1.0f);
}

void Application::saveAdvancements()
{
    if (!m_world || m_net.role() == Net::Role::Guest) return;
    std::error_code ec;
    std::filesystem::create_directories(m_savePath, ec);
    if (std::FILE* file = std::fopen((m_savePath + "/advancements.txt").c_str(), "wb"))
    {
        for (const std::string& line : m_advancements.lines()) std::fprintf(file, "%s\n", line.c_str());
        std::fclose(file);
    }
    if (std::FILE* file = std::fopen((m_savePath + "/recipes.txt").c_str(), "wb"))
    {
        for (StackId id : m_knownRecipes) std::fprintf(file, "%d\n", static_cast<int>(id));
        std::fclose(file);
    }
    saveStats();
}

void Application::loadAdvancements(bool existing)
{
    std::vector<std::string> lines;
    if (std::FILE* file = std::fopen((m_savePath + "/advancements.txt").c_str(), "rb"))
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
    m_advancements.load(lines);
    m_knownRecipes.clear();
    m_recipesSeeded = !existing;
    if (std::FILE* file = std::fopen((m_savePath + "/recipes.txt").c_str(), "rb"))
    {
        int id = 0;
        while (std::fscanf(file, "%d", &id) == 1) m_knownRecipes.insert(static_cast<StackId>(id));
        std::fclose(file);
        m_recipesSeeded = true;
    }
    m_toasts.clear();
    m_tutorialToast = 0;
    m_tutorialLookToast = 0;
    m_tutorialTicks = 0;
    loadStats();
}

void Application::unlockRecipes(float deltaTime)
{
    if (!m_world) return;
    m_recipeClock += deltaTime;
    if (m_recipeClock < 0.5f) return;
    m_recipeClock = 0.0f;
    std::set<StackId> held;
    for (int i = 0; i < Inventory::TOTAL_SLOTS; ++i)
        if (!m_inventory.slot(i).empty()) held.insert(m_inventory.slot(i).id);
    if (!m_inventory.cursor().empty()) held.insert(m_inventory.cursor().id);
    std::vector<StackId> fresh;
    for (RecipeBook::Book book : { RecipeBook::Book::Crafting, RecipeBook::Book::Furnace, RecipeBook::Book::BlastFurnace, RecipeBook::Book::Smoker })
        for (const RecipeBook::Recipe& recipe : RecipeBook::recipes(book))
        {
            if (m_knownRecipes.count(recipe.result)) continue;
            bool opens = false;
            for (const std::vector<StackId>& cell : recipe.cells)
                for (StackId option : cell)
                    if (held.count(option)) opens = true;
            if (!opens) continue;
            m_knownRecipes.insert(recipe.result);
            fresh.push_back(recipe.result);
        }
    if (fresh.empty()) return;
    if (!m_recipesSeeded)
    {
        m_recipesSeeded = true;
        return;
    }
    for (Toast& toast : m_toasts)
        if (toast.advancement < 0 && toast.age < 5.0f)
        {
            toast.recipes.insert(toast.recipes.end(), fresh.begin(), fresh.end());
            return;
        }
    Toast toast;
    toast.recipes = fresh;
    m_toasts.push_back(toast);
}

std::string Application::entityId(MobId id) const
{
    switch (id)
    {
        case MobId::ZombiePigman: return "zombified_piglin";
        case MobId::SlimeSmall: return "slime";
        case MobId::TropicalFishLarge: return "tropical_fish";
        case MobId::TraderLlama: return "trader_llama";
        default: break;
    }
    std::string out;
    for (const char* c = mobType(id).name; c && *c; ++c)
    {
        const unsigned char ch = static_cast<unsigned char>(*c);
        if (std::isalnum(ch)) out.push_back(static_cast<char>(std::tolower(ch)));
        else if (ch == ' ' || ch == '-') out.push_back('_');
    }
    return out;
}
