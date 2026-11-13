namespace
{
    struct ParticleName { const char* id; SpriteParticles::Kind kind; };
    const ParticleName PARTICLE_NAMES[] = {
        { "explosion", SpriteParticles::Kind::Explosion }, { "explosion_emitter", SpriteParticles::Kind::Explosion },
        { "poof", SpriteParticles::Kind::Poof }, { "smoke", SpriteParticles::Kind::Smoke }, { "large_smoke", SpriteParticles::Kind::BigSmoke },
        { "campfire_cosy_smoke", SpriteParticles::Kind::BigSmoke }, { "flame", SpriteParticles::Kind::Flame },
        { "heart", SpriteParticles::Kind::Heart }, { "angry_villager", SpriteParticles::Kind::Angry },
        { "happy_villager", SpriteParticles::Kind::Happy }, { "crit", SpriteParticles::Kind::Crit },
        { "damage_indicator", SpriteParticles::Kind::Damage }, { "enchanted_hit", SpriteParticles::Kind::EnchantedHit },
        { "note", SpriteParticles::Kind::Note }, { "portal", SpriteParticles::Kind::Portal }, { "enchant", SpriteParticles::Kind::Enchant },
        { "bubble", SpriteParticles::Kind::Bubble }, { "bubble_pop", SpriteParticles::Kind::BubblePop },
        { "splash", SpriteParticles::Kind::Splash }, { "dripping_water", SpriteParticles::Kind::DripWater },
        { "dripping_lava", SpriteParticles::Kind::DripLava }, { "lava", SpriteParticles::Kind::Lava },
        { "effect", SpriteParticles::Kind::Effect }, { "entity_effect", SpriteParticles::Kind::Spell },
        { "sweep_attack", SpriteParticles::Kind::Sweep }, { "glow", SpriteParticles::Kind::Glow },
        { "gust", SpriteParticles::Kind::Gust }, { "small_gust", SpriteParticles::Kind::SmallGust },
        { "sonic_boom", SpriteParticles::Kind::SonicBoom }, { "trial_spawner_detection", SpriteParticles::Kind::TrialFlame },
        { "vibration", SpriteParticles::Kind::Vibration }, { "shriek", SpriteParticles::Kind::Shriek },
        { "sculk_soul", SpriteParticles::Kind::SculkSoul }, { "sculk_charge", SpriteParticles::Kind::SculkCharge },
        { "sculk_charge_pop", SpriteParticles::Kind::SculkChargePop }, { "flash", SpriteParticles::Kind::Flash },
        { "firework", SpriteParticles::Kind::Spark }, { "glow_squid_ink", SpriteParticles::Kind::Glow },
        { "soul", SpriteParticles::Kind::Soul }, { "soul_fire_flame", SpriteParticles::Kind::SoulFlame },
        { "cherry_leaves", SpriteParticles::Kind::Cherry }, { "nautilus", SpriteParticles::Kind::Nautilus },
        { "infested", SpriteParticles::Kind::Infested }, { "ash", SpriteParticles::Kind::Ash }, { "white_ash", SpriteParticles::Kind::WhiteAsh },
        { "crimson_spore", SpriteParticles::Kind::CrimsonSpore }, { "warped_spore", SpriteParticles::Kind::WarpedSpore },
        { "raid_omen", SpriteParticles::Kind::RaidOmen }, { "trial_omen", SpriteParticles::Kind::TrialOmen },
        { "ominous_spawning", SpriteParticles::Kind::OminousSpawning }, { "vault_connection", SpriteParticles::Kind::VaultConnection },
    };

    struct SoundName { const char* id; Sound sound; };
    const SoundName SOUND_NAMES[] = {
        { "entity.experience_orb.pickup", Sound::Pickup }, { "entity.item.pickup", Sound::Pickup },
        { "ui.button.click", Sound::Click }, { "block.lever.click", Sound::Click },
        { "entity.player.hurt", Sound::Hurt }, { "entity.generic.hurt", Sound::Hurt },
        { "entity.player.levelup", Sound::ChallengeComplete }, { "ui.toast.challenge_complete", Sound::ChallengeComplete },
        { "ui.toast.in", Sound::ToastIn }, { "ui.toast.out", Sound::ToastOut },
        { "entity.lightning_bolt.thunder", Sound::Thunder }, { "weather.rain", Sound::Rain },
        { "block.fire.ambient", Sound::Fire }, { "entity.generic.burn", Sound::Fire },
        { "entity.firework_rocket.launch", Sound::FireworkLaunch }, { "entity.firework_rocket.blast", Sound::FireworkBlast },
        { "entity.firework_rocket.large_blast", Sound::FireworkLargeBlast }, { "entity.firework_rocket.twinkle", Sound::FireworkTwinkle },
        { "event.raid.horn", Sound::RaidHorn }, { "block.sculk_shrieker.shriek", Sound::Shriek },
        { "block.sculk_sensor.clicking", Sound::SculkClick }, { "entity.evoker_fangs.attack", Sound::EvokerFangs },
        { "entity.wind_charge.wind_burst", Sound::WindBurst }, { "entity.llama.spit", Sound::LlamaSpit },
        { "entity.generic.explode", Sound::Thunder }, { "block.stone.break", Sound::DigStone }, { "block.wood.break", Sound::DigWood },
        { "block.grass.break", Sound::DigGrass }, { "block.glass.break", Sound::DigGlass }, { "block.wool.break", Sound::DigWool },
        { "block.sand.break", Sound::DigSand }, { "block.gravel.break", Sound::DigDirt }, { "block.stone.place", Sound::Place },
        { "entity.player.attack.strong", Sound::Hurt }, { "entity.generic.small_fall", Sound::Land }, { "entity.player.small_fall", Sound::Land },
        { "entity.generic.jump", Sound::Jump },
        { "block.note_block.harp", Sound::NoteFirst }, { "block.note_block.bass", static_cast<Sound>(static_cast<int>(Sound::NoteFirst) + 1) },
        { "block.note_block.basedrum", static_cast<Sound>(static_cast<int>(Sound::NoteFirst) + 2) },
        { "block.note_block.snare", static_cast<Sound>(static_cast<int>(Sound::NoteFirst) + 3) },
        { "block.note_block.hat", static_cast<Sound>(static_cast<int>(Sound::NoteFirst) + 4) },
        { "block.note_block.bell", static_cast<Sound>(static_cast<int>(Sound::NoteFirst) + 6) },
        { "block.note_block.chime", static_cast<Sound>(static_cast<int>(Sound::NoteFirst) + 8) },
        { "block.note_block.pling", static_cast<Sound>(static_cast<int>(Sound::NoteFirst) + 15) },
        { "entity.sheep.ambient", Sound::MobSheepSay }, { "entity.pig.ambient", Sound::MobPigSay }, { "entity.cow.ambient", Sound::MobCowSay },
        { "entity.chicken.ambient", Sound::MobChickenSay }, { "entity.zombie.ambient", Sound::MobZombieSay },
        { "entity.skeleton.ambient", Sound::MobSkeletonSay }, { "entity.creeper.primed", Sound::MobCreeperSay },
        { "entity.spider.ambient", Sound::MobSpiderSay }, { "entity.enderman.teleport", Sound::MobEndermanSay },
        { "entity.warden.sonic_boom", Sound::MobWardenSay }, { "entity.warden.ambient", Sound::MobWardenSay },
    };

    std::string bareId(std::string text)
    {
        for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (text.rfind("minecraft:", 0) == 0) text = text.substr(10);
        return text;
    }

    bool parseNumber(const std::string& text, float& out)
    {
        try
        {
            size_t used = 0;
            out = std::stof(text, &used);
            return used == text.size();
        }
        catch (...) { return false; }
    }

    bool coordinateNear(const std::string& text, float base, float& out)
    {
        if (!text.empty() && text[0] == '~')
        {
            float offset = 0.0f;
            if (text.size() > 1 && !parseNumber(text.substr(1), offset)) return false;
            out = base + offset;
            return true;
        }
        return parseNumber(text, out);
    }

    std::string selectorType(const std::string& selector)
    {
        const size_t at = selector.find("type=");
        if (at == std::string::npos) return std::string();
        size_t end = selector.find_first_of(",]", at);
        std::string type = selector.substr(at + 5, end == std::string::npos ? std::string::npos : end - at - 5);
        if (!type.empty() && type[0] == '!') return std::string();
        return bareId(type);
    }

    bool rangeMatches(const std::string& range, int value)
    {
        const size_t dots = range.find("..");
        try
        {
            if (dots == std::string::npos) return value == std::stoi(range);
            const std::string lo = range.substr(0, dots), hi = range.substr(dots + 2);
            if (!lo.empty() && value < std::stoi(lo)) return false;
            if (!hi.empty() && value > std::stoi(hi)) return false;
            return true;
        }
        catch (...) { return false; }
    }
}

Mob* Application::selectMob(const std::string& selector, const glm::vec3& from, float within)
{
    const std::string type = selectorType(selector);
    Mob* best = nullptr;
    float nearest = within;
    for (Mob& mob : m_entities.mobs())
    {
        if (!mob.alive()) continue;
        if (!type.empty())
        {
            std::string name = bareId(mobType(mob.typeId()).name);
            std::replace(name.begin(), name.end(), ' ', '_');
            if (name != type) continue;
        }
        const float gap = glm::length(mob.position() - from);
        if (gap < nearest)
        {
            nearest = gap;
            best = &mob;
        }
    }
    return best;
}

bool Application::runFunction(const std::string& id, int depth, std::string& output)
{
    if (depth > 16)
    {
        output = "Functions nest too deeply";
        return false;
    }
    std::string space = "minecraft", path = id;
    const size_t colon = id.find(':');
    if (colon != std::string::npos)
    {
        space = id.substr(0, colon);
        path = id.substr(colon + 1);
    }
    std::error_code ec;
    std::string file;
    const std::filesystem::path packs = std::filesystem::path(m_savePath) / "datapacks";
    if (std::filesystem::exists(packs, ec))
        for (const auto& pack : std::filesystem::directory_iterator(packs, ec))
            for (const char* folder : { "function", "functions" })
            {
                const std::filesystem::path candidate = pack.path() / "data" / space / folder / (path + ".mcfunction");
                if (std::filesystem::exists(candidate, ec)) file = candidate.string();
            }
    if (file.empty())
    {
        output = "Unknown function " + space + ":" + path;
        return false;
    }
    std::ifstream in(file);
    std::string line;
    int ran = 0;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#') continue;
        line = line.substr(start);
        if (line.rfind("return", 0) == 0) break;
        std::string said;
        if (line.rfind("function ", 0) == 0) runFunction(line.substr(9), depth + 1, said);
        else runCommand("/" + line, m_player.position, true, said);
        ++ran;
        if (!said.empty() && m_rules.on(GameRules::Rule::CommandBlockOutput)) m_chatLog.add(said);
    }
    output = "Executed " + std::to_string(ran) + " command(s) from function '" + space + ":" + path + "'";
    return true;
}

bool Application::runMoreCommand(const std::string& name, const std::vector<std::string>& args, const glm::vec3& origin, std::string& output)
{
    const std::string self = scoreHolder();
    auto arg = [&](size_t i) { return i < args.size() ? args[i] : std::string(); };

    if (name == "advancement")
    {
        const std::string action = arg(0), mode = arg(2);
        if ((action != "grant" && action != "revoke") || args.size() < 3)
        {
            output = "/advancement grant|revoke <targets> everything|only|from|through|until <advancement> [criterion]";
            return false;
        }
        const bool give = action == "grant";
        const std::vector<Advancements::Def>& all = Advancements::all();
        std::vector<std::string> ids;
        if (mode == "everything")
            for (const Advancements::Def& def : all) ids.emplace_back(def.id);
        else
        {
            std::string wanted = arg(3);
            if (wanted.rfind("minecraft:", 0) == 0) wanted = wanted.substr(10);
            if (Advancements::indexOf(wanted) < 0)
            {
                output = "No advancement was found by the name '" + wanted + "'";
                return false;
            }
            if (mode == "only") ids.push_back(wanted);
            else
            {
                auto parentOf = [&](const std::string& id) {
                    const int index = Advancements::indexOf(id);
                    return index >= 0 && all[static_cast<size_t>(index)].parent ? std::string(all[static_cast<size_t>(index)].parent) : std::string();
                };
                if (mode == "until" || mode == "through")
                    for (std::string at = wanted; !at.empty(); at = parentOf(at)) ids.push_back(at);
                if (mode == "from" || mode == "through")
                {
                    std::vector<std::string> frontier = { wanted };
                    while (!frontier.empty())
                    {
                        const std::string at = frontier.back();
                        frontier.pop_back();
                        if (std::find(ids.begin(), ids.end(), at) == ids.end()) ids.push_back(at);
                        for (const Advancements::Def& def : all)
                            if (def.parent && at == def.parent) frontier.emplace_back(def.id);
                    }
                }
            }
        }
        int changed = 0;
        for (const std::string& id : ids)
        {
            if (give)
            {
                if (mode == "only" && args.size() >= 5) changed += m_advancements.grant(id, args[4]) ? 1 : 0;
                else changed += m_advancements.grantAll(id) ? 1 : 0;
            }
            else changed += m_advancements.revoke(id, mode == "only" ? arg(4) : std::string()) ? 1 : 0;
        }
        output = std::string(give ? "Granted " : "Revoked ") + std::to_string(changed) + " advancement(s) " + (give ? "to " : "from ") + self;
        return changed > 0;
    }

    if (name == "bossbar")
    {
        const std::string action = arg(0);
        if (action == "list")
        {
            output = m_customBars.empty() ? std::string("There are no custom bossbars active") : "There are " + std::to_string(m_customBars.size()) + " custom bossbar(s) active:";
            for (const auto& [id, bar] : m_customBars) output += " [" + bar.name + "]";
            return true;
        }
        if (args.size() < 2)
        {
            output = "/bossbar add|remove|list|get|set ...";
            return false;
        }
        const std::string id = bareId(arg(1));
        if (action == "add")
        {
            if (m_customBars.count(id))
            {
                output = "A bossbar already exists with the ID '" + id + "'";
                return false;
            }
            CustomBar bar;
            bar.name = args.size() > 2 ? arg(2) : id;
            for (size_t i = 3; i < args.size(); ++i) bar.name += " " + args[i];
            std::string shown;
            if (Chat::textComponent(bar.name, shown)) bar.name = shown;
            m_customBars[id] = bar;
            output = "Created custom bossbar [" + bar.name + "]";
            return true;
        }
        auto found = m_customBars.find(id);
        if (found == m_customBars.end())
        {
            output = "No bossbar exists with the ID '" + id + "'";
            return false;
        }
        CustomBar& bar = found->second;
        if (action == "remove")
        {
            output = "Removed custom bossbar [" + bar.name + "]";
            m_customBars.erase(found);
            return true;
        }
        if (action == "get")
        {
            const std::string what = arg(2);
            if (what == "value") output = "Custom bossbar [" + bar.name + "] has a value of " + std::to_string(bar.value);
            else if (what == "max") output = "Custom bossbar [" + bar.name + "] has a maximum of " + std::to_string(bar.max);
            else if (what == "visible") output = "Custom bossbar [" + bar.name + "] is currently " + (bar.visible ? "shown" : "hidden");
            else output = "Custom bossbar [" + bar.name + "] has 1 player(s) currently online: " + self;
            return true;
        }
        if (action == "set" && args.size() >= 4)
        {
            const std::string what = arg(2), value = arg(3);
            float number = 0.0f;
            if (what == "name")
            {
                bar.name = value;
                for (size_t i = 4; i < args.size(); ++i) bar.name += " " + args[i];
                std::string shown;
                if (Chat::textComponent(bar.name, shown)) bar.name = shown;
            }
            else if (what == "color")
            {
                static const char* COLOURS[] = { "pink", "blue", "red", "green", "yellow", "purple", "white" };
                int picked = -1;
                for (int c = 0; c < 7; ++c) if (value == COLOURS[c]) picked = c;
                if (picked < 0) { output = "Unknown colour '" + value + "'"; return false; }
                bar.colour = picked;
            }
            else if (what == "style")
            {
                bar.notches = value == "notched_6" ? 6 : value == "notched_10" ? 10 : value == "notched_12" ? 12 : value == "notched_20" ? 20 : 0;
            }
            else if (what == "value" && parseNumber(value, number)) bar.value = std::clamp(static_cast<int>(number), 0, bar.max);
            else if (what == "max" && parseNumber(value, number)) bar.max = std::max(1, static_cast<int>(number));
            else if (what == "visible") bar.visible = value == "true";
            else if (what == "players") bar.visible = value.empty() ? false : true;
            else
            {
                output = "/bossbar set <id> name|color|style|value|max|visible|players <value>";
                return false;
            }
            output = "Custom bossbar [" + bar.name + "] has changed its " + what;
            return true;
        }
        output = "/bossbar add|remove|list|get|set ...";
        return false;
    }

    if (name == "item")
    {
        if (arg(0) != "replace" || args.size() < 5 || arg(1) != "entity")
        {
            output = "/item replace entity @s <slot> with <item> [count]";
            return false;
        }
        const std::string slot = arg(3);
        int index = -1;
        if (slot == "weapon.mainhand" || slot == "weapon") index = m_inventory.selectedSlot();
        else if (slot == "weapon.offhand") index = Inventory::OFFHAND;
        else if (slot == "armor.head") index = Inventory::ARMOR_FIRST;
        else if (slot == "armor.chest") index = Inventory::ARMOR_FIRST + 1;
        else if (slot == "armor.legs") index = Inventory::ARMOR_FIRST + 2;
        else if (slot == "armor.feet") index = Inventory::ARMOR_FIRST + 3;
        else if (slot.rfind("hotbar.", 0) == 0) index = std::clamp(std::atoi(slot.c_str() + 7), 0, 8);
        else if (slot.rfind("inventory.", 0) == 0) index = 9 + std::clamp(std::atoi(slot.c_str() + 10), 0, 26);
        if (index < 0 || arg(4) != "with")
        {
            output = "Unknown slot '" + slot + "'";
            return false;
        }
        const Chat::Command lookup = Chat::parse("/give " + arg(5) + " " + (args.size() > 6 ? arg(6) : std::string("1")), true);
        if (lookup.kind != Chat::Kind::Give && arg(5) != "air")
        {
            output = "Unknown item '" + arg(5) + "'";
            return false;
        }
        ItemStack put;
        if (arg(5) != "air" && arg(5) != "minecraft:air")
        {
            put.id = lookup.item;
            put.count = std::max(1, lookup.count);
        }
        m_inventory.slot(index) = put;
        output = "Replaced a slot on " + self + " with " + (put.empty() ? std::string("nothing") : displayName(put.id));
        return true;
    }

    if (name == "particle")
    {
        const std::string id = bareId(arg(0));
        const ParticleName* found = nullptr;
        for (const ParticleName& entry : PARTICLE_NAMES)
            if (id == entry.id) found = &entry;
        if (!found)
        {
            output = "Unknown particle: " + id;
            return false;
        }
        glm::vec3 at = origin, spread(0.0f);
        float speed = 0.0f, count = 1.0f;
        if (args.size() >= 4 && !(coordinateNear(arg(1), origin.x, at.x) && coordinateNear(arg(2), origin.y, at.y) && coordinateNear(arg(3), origin.z, at.z)))
        {
            output = "/particle <name> [x y z] [dx dy dz] [speed] [count]";
            return false;
        }
        if (args.size() >= 7) { parseNumber(arg(4), spread.x); parseNumber(arg(5), spread.y); parseNumber(arg(6), spread.z); }
        if (args.size() >= 8) parseNumber(arg(7), speed);
        if (args.size() >= 9) parseNumber(arg(8), count);
        const int many = std::clamp(static_cast<int>(count), 1, 500);
        sprites(found->kind, at, many, std::max({ spread.x, spread.y, spread.z, many > 1 ? 0.3f : 0.0f }), speed, 0.2f, 1.0f);
        output = "Displaying particle " + id;
        return true;
    }

    if (name == "playsound")
    {
        const std::string id = bareId(arg(0));
        const SoundName* found = nullptr;
        for (const SoundName& entry : SOUND_NAMES)
            if (id == entry.id) found = &entry;
        if (!found)
        {
            output = "Unknown sound '" + id + "'";
            return false;
        }
        float volume = 1.0f, pitch = 1.0f;
        if (args.size() >= 7) parseNumber(arg(6), volume);
        if (args.size() >= 8) parseNumber(arg(7), pitch);
        m_audio.play(found->sound, std::clamp(volume, 0.0f, 1.0f), std::clamp(pitch, 0.5f, 2.0f));
        output = "Played sound " + id + " to " + self;
        return true;
    }

    if (name == "stopsound")
    {
        m_audio.stopMusic();
        output = "Stopped all sounds";
        return true;
    }

    if (name == "random")
    {
        const std::string action = arg(0);
        const std::string range = arg(1);
        const size_t dots = range.find("..");
        if ((action != "value" && action != "roll") || dots == std::string::npos)
        {
            output = "/random value|roll <min>..<max>";
            return false;
        }
        int lo = 0, hi = 0;
        try
        {
            lo = std::stoi(range.substr(0, dots));
            hi = std::stoi(range.substr(dots + 2));
        }
        catch (...)
        {
            output = "Expected a range like 1..6";
            return false;
        }
        if (hi < lo) std::swap(lo, hi);
        m_commandRoll = m_commandRoll * 1664525u + 1013904223u + static_cast<uint32_t>(SDL_GetTicks());
        const int value = lo + static_cast<int>((m_commandRoll >> 8) % static_cast<uint32_t>(hi - lo + 1));
        output = action == "roll" ? self + " rolled " + std::to_string(value) + " (from " + std::to_string(lo) + " to " + std::to_string(hi) + ")"
                                  : "Randomized value was: " + std::to_string(value);
        return true;
    }

    if (name == "recipe")
    {
        const std::string action = arg(0), which = arg(2);
        if ((action != "give" && action != "take") || which.empty())
        {
            output = "/recipe give|take <targets> <recipe>|*";
            return false;
        }
        int changed = 0;
        if (which == "*")
        {
            if (action == "take") { changed = static_cast<int>(m_knownRecipes.size()); m_knownRecipes.clear(); }
            else
            {
                for (int id = 1; isBlockId(id); id = nextBlockId(id))
                    if (isObtainable(asBlock(static_cast<StackId>(id))) && m_knownRecipes.insert(static_cast<StackId>(id)).second) ++changed;
                for (int id = Items::FIRST; id < static_cast<int>(Items::Count); ++id)
                    if (m_knownRecipes.insert(static_cast<StackId>(id)).second) ++changed;
            }
        }
        else
        {
            const StackId recipe = RecipeBook::stackNamed(bareId(which));
            if (recipe == Blocks::Air)
            {
                output = "Unknown recipe '" + which + "'";
                return false;
            }
            changed = action == "give" ? (m_knownRecipes.insert(recipe).second ? 1 : 0) : static_cast<int>(m_knownRecipes.erase(recipe));
        }
        output = std::string(action == "give" ? "Unlocked " : "Took ") + std::to_string(changed) + " recipe(s) " + (action == "give" ? "for " : "from ") + self;
        return changed > 0;
    }

    if (name == "ride")
    {
        if (arg(1) == "dismount")
        {
            if (!riddenMob())
            {
                output = self + " is not riding any vehicle";
                return false;
            }
            dismount();
            output = self + " stopped riding";
            return true;
        }
        if (arg(1) != "mount" || args.size() < 3)
        {
            output = "/ride <target> mount <vehicle> | dismount";
            return false;
        }
        Mob* vehicle = selectMob(arg(2), m_player.position, 32.0f);
        if (!vehicle)
        {
            output = "No entity was found";
            return false;
        }
        if (Mob* already = riddenMob()) already->setRidden(false);
        vehicle->setRidden(true);
        m_player.velocity = glm::vec3(0.0f);
        output = self + " started riding " + mobType(vehicle->typeId()).name;
        return true;
    }

    if (name == "spectate")
    {
        if (!m_player.spectator())
        {
            output = "Only spectators can spectate";
            return false;
        }
        if (args.empty())
        {
            m_spectatingMob = 0;
            output = "Stopped spectating";
            return true;
        }
        Mob* target = selectMob(arg(0), m_player.position, 128.0f);
        if (!target)
        {
            output = "No entity was found";
            return false;
        }
        m_spectatingMob = target->serial();
        output = std::string("Now spectating ") + mobType(target->typeId()).name;
        return true;
    }

    if (name == "spreadplayers")
    {
        float cx = 0.0f, cz = 0.0f, range = 0.0f;
        if (args.size() < 4 || !coordinateNear(arg(0), origin.x, cx) || !coordinateNear(arg(1), origin.z, cz) || !parseNumber(arg(3), range))
        {
            output = "/spreadplayers <x> <z> <spreadDistance> <maxRange> [under <y>] <respectTeams> <targets>";
            return false;
        }
        if (!m_world) return false;
        m_commandRoll = m_commandRoll * 1664525u + 1013904223u + static_cast<uint32_t>(SDL_GetTicks());
        for (int attempt = 0; attempt < 32; ++attempt)
        {
            const float angle = static_cast<float>((m_commandRoll >> (attempt % 16)) % 6283u) * 0.001f;
            const float reach = range * static_cast<float>(((m_commandRoll >> 5) + static_cast<uint32_t>(attempt) * 977u) % 1000u) / 1000.0f;
            const int x = static_cast<int>(std::floor(cx + std::cos(angle) * reach));
            const int z = static_cast<int>(std::floor(cz + std::sin(angle) * reach));
            int y = m_world->generator().surfaceHeight(x, z) + 1;
            if (y <= 0 || y >= Chunk::SY - 2) continue;
            if (isLiquid(m_world->getBlock(x, y - 1, z)) || isLava(m_world->getBlock(x, y, z))) continue;
            m_player.position = glm::vec3(x + 0.5f, static_cast<float>(y), z + 0.5f);
            m_player.velocity = glm::vec3(0.0f);
            m_player.clearFall();
            output = "Spread 1 player(s) around " + std::to_string(static_cast<int>(cx)) + ", " + std::to_string(static_cast<int>(cz));
            return true;
        }
        output = "Could not spread the player around there";
        return false;
    }

    if (name == "teammsg" || name == "tm")
    {
        const Scoreboard::Team* team = m_scoreboard.teamOf(self);
        if (!team)
        {
            output = "You must be on a team to message your team";
            return false;
        }
        std::string text;
        for (size_t i = 0; i < args.size(); ++i) text += (i ? " " : "") + args[i];
        output = "-> [" + team->displayName + "] <" + self + "> " + text;
        return true;
    }

    if (name == "tick")
    {
        const std::string action = arg(0);
        float number = 0.0f;
        if (action == "query")
        {
            output = std::string("The game is ") + (m_tickFrozen ? "frozen" : "running normally") + "; target tick rate " + std::to_string(static_cast<int>(m_tickRate)) + " per second";
            return true;
        }
        if (action == "rate" && parseNumber(arg(1), number))
        {
            m_tickRate = std::clamp(number, 1.0f, 10000.0f);
            output = "Set the target tick rate to " + std::to_string(static_cast<int>(m_tickRate)) + " per second";
            return true;
        }
        if (action == "freeze" || action == "unfreeze")
        {
            m_tickFrozen = action == "freeze";
            m_tickStepSeconds = 0.0f;
            output = m_tickFrozen ? "The game is frozen" : "The game is running normally";
            return true;
        }
        if (action == "step")
        {
            if (!m_tickFrozen)
            {
                output = "The game must be frozen to step";
                return false;
            }
            float ticks = 1.0f;
            if (args.size() >= 2 && arg(1) != "stop" && !parseNumber(arg(1), ticks)) ticks = 1.0f;
            m_tickStepSeconds = arg(1) == "stop" ? 0.0f : ticks / 20.0f;
            output = arg(1) == "stop" ? "Stepping stopped" : "Stepping " + std::to_string(static_cast<int>(ticks)) + " tick(s)";
            return true;
        }
        if (action == "sprint")
        {
            float ticks = 0.0f;
            if (arg(1) == "stop") { m_tickSprintSeconds = 0.0f; output = "Sprint stopped"; return true; }
            if (!parseNumber(arg(1), ticks))
            {
                output = "/tick sprint <ticks>|stop";
                return false;
            }
            m_tickSprintSeconds = ticks / 20.0f;
            output = "Sprinting " + std::to_string(static_cast<int>(ticks)) + " tick(s)";
            return true;
        }
        output = "/tick query|rate|freeze|unfreeze|step|sprint ...";
        return false;
    }

    if (name == "function") return runFunction(arg(0), 0, output);

    if (name == "datapack")
    {
        std::error_code ec;
        std::vector<std::string> found;
        const std::filesystem::path packs = std::filesystem::path(m_savePath) / "datapacks";
        if (std::filesystem::exists(packs, ec))
            for (const auto& pack : std::filesystem::directory_iterator(packs, ec)) found.push_back(pack.path().filename().string());
        output = "There are " + std::to_string(found.size() + 1) + " data pack(s) enabled: [vanilla]";
        for (const std::string& pack : found) output += ", [file/" + pack + "]";
        return true;
    }

    if (name == "reload")
    {
        output = "Reloading!";
        return true;
    }

    if (name == "data" && arg(1) == "entity" && arg(2) != "@s" && !arg(2).empty() &&
        (arg(0) == "merge" || arg(0) == "get" || (arg(0) == "modify" && arg(4) == "set" && arg(5) == "value")))
    {
        Targets chosen;
        std::string error;
        if (!selectTargets(arg(2), origin, false, chosen, error) || chosen.mobs.empty())
        {
            output = error.empty() ? "No entity was found" : error;
            return false;
        }
        Mob& mob = m_entities.mobs()[chosen.mobs.front()];
        auto describe = [&]() {
            std::string text = "{Health: " + std::to_string(mob.health()) + ".0f, Rotation: [" + std::to_string(static_cast<int>(mob.yaw() - 90.0f)) +
                               ".0f, 0.0f], NoAI: " + (mob.noAI() ? "1b" : "0b") + ", Silent: " + (mob.silent() ? "1b" : "0b") +
                               ", Invulnerable: " + (mob.invulnerable() ? "1b" : "0b") + ", PersistenceRequired: " + (mob.persistent() ? "1b" : "0b") +
                               ", Age: " + (mob.baby() ? "-24000" : "0");
            if (!mob.customName().empty()) text += ", CustomName: '\"" + mob.customName() + "\"'";
            return text + "}";
        };
        const std::string who = mob.customName().empty() ? std::string(mob.type().name) : mob.customName();
        if (arg(0) == "get")
        {
            const std::string path = arg(3);
            const std::string all = describe();
            if (path.empty())
            {
                output = who + " has the following entity data: " + all;
                return true;
            }
            const size_t at = all.find(path + ": ");
            if (at == std::string::npos)
            {
                output = "Found no elements matching " + path;
                return false;
            }
            size_t end = at + path.size() + 2;
            int depth = 0;
            while (end < all.size() && (depth > 0 || (all[end] != ',' && all[end] != '}')))
            {
                if (all[end] == '[') ++depth;
                if (all[end] == ']') --depth;
                ++end;
            }
            output = who + " has the following entity data: " + all.substr(at + path.size() + 2, end - at - path.size() - 2);
            return true;
        }
        std::vector<std::pair<std::string, std::string>> tags;
        if (arg(0) == "modify") tags.push_back({ arg(3), arg(6) });
        else
        {
            std::string body;
            for (size_t k = 3; k < args.size(); ++k) body += (k > 3 ? " " : "") + args[k];
            if (body.size() < 2 || body.front() != '{' || body.back() != '}')
            {
                output = "Expected a compound tag like {NoAI:1b}";
                return false;
            }
            body = body.substr(1, body.size() - 2);
            size_t k = 0;
            while (k < body.size())
            {
                while (k < body.size() && (body[k] == ' ' || body[k] == ',')) ++k;
                const size_t colon = body.find(':', k);
                if (colon == std::string::npos) break;
                std::string key = body.substr(k, colon - k);
                while (!key.empty() && key.back() == ' ') key.pop_back();
                size_t v = colon + 1;
                while (v < body.size() && body[v] == ' ') ++v;
                size_t end = v;
                int depth = 0;
                char quote = 0;
                while (end < body.size())
                {
                    const char c = body[end];
                    if (quote) { if (c == quote) quote = 0; }
                    else if (c == '\'' || c == '"') quote = c;
                    else if (c == '[' || c == '{') ++depth;
                    else if (c == ']' || c == '}') --depth;
                    else if (c == ',' && depth == 0) break;
                    ++end;
                }
                tags.push_back({ key, body.substr(v, end - v) });
                k = end + 1;
            }
        }
        auto flag = [](const std::string& value) { return !value.empty() && value[0] != '0' && value != "false"; };
        auto number = [](std::string value) {
            while (!value.empty() && std::isalpha(static_cast<unsigned char>(value.back()))) value.pop_back();
            try { return std::stof(value); } catch (...) { return 0.0f; }
        };
        int applied = 0;
        for (const auto& [key, value] : tags)
        {
            if (key == "NoAI") mob.setNoAI(flag(value));
            else if (key == "Silent") mob.setSilent(flag(value));
            else if (key == "Invulnerable") mob.setInvulnerable(flag(value));
            else if (key == "PersistenceRequired") { if (flag(value)) mob.keepFromDespawning(); }
            else if (key == "Health") mob.setHealth(static_cast<int>(std::lround(number(value))));
            else if (key == "Age") mob.setBaby(number(value) < 0.0f);
            else if (key == "Glowing") { if (flag(value)) mob.giveEffect(Effects::Kind::Glowing, 0, 1.0e6f); }
            else if (key == "Rotation")
            {
                const size_t open = value.find('[');
                if (open != std::string::npos) mob.setYaw(number(value.substr(open + 1)) + 90.0f);
            }
            else if (key == "CustomName")
            {
                std::string text = value;
                if (text.size() >= 2 && (text.front() == '\'' || text.front() == '"') && text.back() == text.front()) text = text.substr(1, text.size() - 2);
                std::string plain;
                mob.setCustomName(Chat::textComponent(text, plain) ? plain : text);
            }
            else continue;
            ++applied;
        }
        if (applied == 0)
        {
            output = "Nothing changed. The specified properties already have these values";
            return false;
        }
        output = "Modified entity data of " + who;
        return true;
    }

    if (name == "data")
    {
        if (arg(0) != "get")
        {
            output = "Only /data get, and /data merge or modify on mobs, are supported: blocks keep no NBT here";
            return false;
        }
        if (arg(1) == "block" && args.size() >= 5 && m_world)
        {
            glm::vec3 at;
            if (!coordinateNear(arg(2), origin.x, at.x) || !coordinateNear(arg(3), origin.y, at.y) || !coordinateNear(arg(4), origin.z, at.z)) return false;
            const BlockId block = m_world->getBlock(static_cast<int>(std::floor(at.x)), static_cast<int>(std::floor(at.y)), static_cast<int>(std::floor(at.z)));
            output = std::to_string(static_cast<int>(std::floor(at.x))) + ", " + std::to_string(static_cast<int>(std::floor(at.y))) + ", " +
                     std::to_string(static_cast<int>(std::floor(at.z))) + " has the following block data: {id: \"" + RecipeBook::idOf(static_cast<StackId>(block)) + "\"}";
            return true;
        }
        if (arg(1) == "entity")
        {
            const std::string path = arg(3);
            const ItemStack& held = m_inventory.selected();
            if (path == "Health") output = self + " has the following entity data: " + std::to_string(m_player.health) + ".0f";
            else if (path == "foodLevel") output = self + " has the following entity data: " + std::to_string(m_player.hunger);
            else if (path == "XpLevel") output = self + " has the following entity data: " + std::to_string(Experience::levelFor(m_player.experience));
            else if (path == "Air") output = self + " has the following entity data: " + std::to_string(static_cast<int>(m_player.air * 20.0f)) + "s";
            else if (path == "Pos")
                output = self + " has the following entity data: [" + std::to_string(m_player.position.x) + "d, " + std::to_string(m_player.position.y) + "d, " +
                         std::to_string(m_player.position.z) + "d]";
            else if (path == "SelectedItem")
                output = self + " has the following entity data: {id: \"" + (held.empty() ? std::string("air") : RecipeBook::idOf(held.id)) + "\", count: " +
                         std::to_string(held.count) + "}";
            else
                output = self + " has the following entity data: {Health: " + std::to_string(m_player.health) + ".0f, foodLevel: " + std::to_string(m_player.hunger) +
                         ", XpLevel: " + std::to_string(Experience::levelFor(m_player.experience)) + ", playerGameType: " + std::to_string(static_cast<int>(m_player.mode)) + "}";
            return true;
        }
        output = "/data get entity @s [path] | /data get block <x> <y> <z>";
        return false;
    }

    if (name == "loot")
    {
        const bool give = arg(0) == "give";
        const bool spawn = arg(0) == "spawn";
        size_t at = give ? 2 : spawn ? 4 : 0;
        if ((!give && !spawn) || arg(at) != "loot" && arg(at) != "kill")
        {
            output = "/loot give <targets> loot <table> | /loot spawn <x y z> loot <table> | ... kill <target>";
            return false;
        }
        MobId killed = MobId::Count;
        if (arg(at) == "kill")
        {
            if (Mob* mob = selectMob(arg(at + 1), m_player.position, 64.0f)) killed = mob->typeId();
        }
        else
        {
            std::string table = bareId(arg(at + 1));
            if (table.rfind("entities/", 0) == 0)
            {
                const int mob = Chat::parse("/summon " + table.substr(9), true).mob;
                if (mob >= 0) killed = static_cast<MobId>(mob);
            }
        }
        if (killed == MobId::Count)
        {
            output = "Only entity loot tables (loot give @s loot minecraft:entities/<mob>) are known";
            return false;
        }
        const MobType& type = mobType(killed);
        glm::vec3 where = m_player.position + glm::vec3(0.0f, 0.5f, 0.0f);
        if (spawn) { coordinateNear(arg(1), origin.x, where.x); coordinateNear(arg(2), origin.y, where.y); coordinateNear(arg(3), origin.z, where.z); }
        int dropped = 0;
        auto drop = [&](StackId id, int count) {
            if (id == Blocks::Air || count <= 0) return;
            if (give) m_inventory.add(id, count);
            else m_drops.spawn(where, id, count);
            dropped += count;
        };
        drop(type.drop, type.dropCount);
        drop(type.secondDrop, type.secondDropCount);
        output = dropped > 0 ? "Dropped " + std::to_string(dropped) + " item(s)" : "That loot table is empty";
        return dropped > 0;
    }

    if (name == "place")
    {
        const std::string kind = arg(0), what = bareId(arg(1));
        glm::vec3 at = origin;
        if (args.size() >= 5 && !(coordinateNear(arg(2), origin.x, at.x) && coordinateNear(arg(3), origin.y, at.y) && coordinateNear(arg(4), origin.z, at.z)))
            return false;
        if (kind != "feature" || !m_world)
        {
            output = "Only /place feature <tree|huge_mushroom|huge_fungus> [x y z] is supported";
            return false;
        }
        struct WorldCells : Saplings::Cells
        {
            Application* app = nullptr;
            BlockId at(int x, int y, int z) const override { return app->m_world->getBlock(x, y, z); }
            void set(int x, int y, int z, BlockId id) override
            {
                app->m_world->setBlock(x, y, z, id);
                app->m_net.onLocalBlockChange(x, y, z, id);
            }
        } cells;
        cells.app = this;
        const glm::ivec3 spot = glm::ivec3(glm::floor(at));
        const uint32_t seed = static_cast<uint32_t>(spot.x * 73856093) ^ static_cast<uint32_t>(spot.z * 19349663) ^ SDL_GetTicks();
        bool placed = false;
        static const struct { const char* id; int wood; } TREES[] = {
            { "oak", 0 }, { "fancy_oak", 0 }, { "spruce", 1 }, { "pine", 1 }, { "birch", 2 }, { "super_birch_bees", 2 }, { "jungle_tree", 3 },
            { "acacia", 4 }, { "dark_oak", 5 }, { "cherry", 6 }, { "mangrove", 7 } };
        for (const auto& tree : TREES)
            if (what == tree.id)
            {
                const BlockId was = m_world->getBlock(spot.x, spot.y, spot.z);
                m_world->setBlock(spot.x, spot.y, spot.z, woodSet(tree.wood).sapling);
                placed = Saplings::grow(cells, spot, seed);
                if (!placed) m_world->setBlock(spot.x, spot.y, spot.z, was);
            }
        if (what == "azalea_tree") placed = Saplings::azaleaTree(cells, spot, seed, true);
        if (what == "huge_red_mushroom" || what == "huge_brown_mushroom") placed = Saplings::hugeMushroom(cells, spot, what == "huge_red_mushroom", seed, true);
        if (what == "crimson_fungus_planted" || what == "warped_fungus_planted" || what == "crimson_fungus" || what == "warped_fungus")
            placed = Saplings::hugeFungus(cells, spot, what.rfind("warped", 0) == 0, seed);
        output = placed ? "Placed feature \"minecraft:" + what + "\" at " + std::to_string(spot.x) + ", " + std::to_string(spot.y) + ", " + std::to_string(spot.z)
                        : "Failed to place feature";
        return placed;
    }

    if (name == "execute")
    {
        glm::vec3 at = origin;
        size_t i = 0;
        while (i < args.size())
        {
            const std::string& word = args[i];
            if (word == "run")
            {
                std::string rest;
                for (size_t k = i + 1; k < args.size(); ++k) rest += (k > i + 1 ? " " : "") + args[k];
                if (rest.empty()) return false;
                return runCommand("/" + rest, at, true, output);
            }
            if (word == "as" || word == "at")
            {
                if (word == "at")
                {
                    if (arg(i + 1).rfind("@e", 0) == 0)
                    {
                        Mob* mob = selectMob(arg(i + 1), at, 64.0f);
                        if (!mob) { output = "No entity was found"; return false; }
                        at = mob->position();
                    }
                    else at = m_player.position;
                }
                i += 2;
                continue;
            }
            if (word == "positioned")
            {
                if (arg(i + 1) == "as") { at = m_player.position; i += 3; continue; }
                glm::vec3 next;
                if (!coordinateNear(arg(i + 1), at.x, next.x) || !coordinateNear(arg(i + 2), at.y, next.y) || !coordinateNear(arg(i + 3), at.z, next.z))
                {
                    output = "Bad position after positioned";
                    return false;
                }
                at = next;
                i += 4;
                continue;
            }
            if (word == "align" || word == "anchored" || word == "facing" || word == "rotated" || word == "in" || word == "on" || word == "summon")
            {
                i += word == "facing" || word == "rotated" ? 3 : 2;
                continue;
            }
            if (word == "if" || word == "unless")
            {
                const bool want = word == "if";
                const std::string test = arg(i + 1);
                bool truth = false;
                size_t used = 2;
                if (test == "block" && m_world)
                {
                    glm::vec3 cell;
                    if (!coordinateNear(arg(i + 2), at.x, cell.x) || !coordinateNear(arg(i + 3), at.y, cell.y) || !coordinateNear(arg(i + 4), at.z, cell.z)) return false;
                    const BlockId there = m_world->getBlock(static_cast<int>(std::floor(cell.x)), static_cast<int>(std::floor(cell.y)), static_cast<int>(std::floor(cell.z)));
                    const Chat::Command lookup = Chat::parse("/setblock 0 0 0 " + arg(i + 5), true);
                    truth = lookup.kind == Chat::Kind::SetBlock ? there == asBlock(lookup.item) : bareId(arg(i + 5)) == "air" && there == Blocks::Air;
                    used = 6;
                }
                else if (test == "entity")
                {
                    const std::string selector = arg(i + 2);
                    truth = selector.rfind("@e", 0) == 0 ? selectMob(selector, at, 1e9f) != nullptr : true;
                    used = 3;
                }
                else if (test == "score")
                {
                    const std::string holder = arg(i + 2) == "@s" || arg(i + 2) == "@p" ? self : arg(i + 2);
                    if (arg(i + 4) == "matches")
                    {
                        truth = m_scoreboard.has(arg(i + 3), holder) && rangeMatches(arg(i + 5), m_scoreboard.score(arg(i + 3), holder));
                        used = 6;
                    }
                    else
                    {
                        const std::string op = arg(i + 4);
                        const std::string other = arg(i + 5) == "@s" || arg(i + 5) == "@p" ? self : arg(i + 5);
                        const int left = m_scoreboard.score(arg(i + 3), holder), right = m_scoreboard.score(arg(i + 6), other);
                        truth = op == "=" ? left == right : op == "<" ? left < right : op == "<=" ? left <= right : op == ">" ? left > right : op == ">=" ? left >= right : false;
                        used = 7;
                    }
                }
                else
                {
                    output = "/execute if|unless block|entity|score ...";
                    return false;
                }
                if (truth != want)
                {
                    output = "Test failed";
                    return false;
                }
                i += used;
                continue;
            }
            output = "Unknown /execute part '" + word + "'";
            return false;
        }
        output = "Test passed";
        return true;
    }

    if (name == "forceload")
    {
        const std::string action = arg(0);
        std::set<std::pair<int, int>>& forced = m_forcedChunks[static_cast<int>(m_dimension)];
        if (action == "query")
        {
            output = forced.empty() ? std::string("No force loaded chunks were found") : std::to_string(forced.size()) + " force loaded chunk(s)";
            return true;
        }
        if (action == "remove" && arg(1) == "all")
        {
            forced.clear();
            if (m_world) m_world->forcedChunks = forced;
            output = "Unmarked all force loaded chunks";
            return true;
        }
        float x = 0.0f, z = 0.0f;
        if ((action != "add" && action != "remove") || !coordinateNear(arg(1), origin.x, x) || !coordinateNear(arg(2), origin.z, z))
        {
            output = "/forceload add|remove <x> <z> | remove all | query";
            return false;
        }
        const std::pair<int, int> chunk(static_cast<int>(std::floor(x / 16.0f)), static_cast<int>(std::floor(z / 16.0f)));
        if (action == "add") forced.insert(chunk);
        else forced.erase(chunk);
        if (m_world) m_world->forcedChunks = forced;
        output = "Chunk at " + std::to_string(chunk.first) + ", " + std::to_string(chunk.second) + (action == "add" ? " is now force loaded" : " is no longer force loaded");
        return true;
    }

    if (name == "kick")
    {
        if (!m_net.active())
        {
            output = "No player was found";
            return false;
        }
        std::string reason;
        for (size_t i = 1; i < args.size(); ++i) reason += (i > 1 ? " " : "") + args[i];
        if (!m_net.kick(arg(0), reason))
        {
            output = "No player was found";
            return false;
        }
        output = "Kicked " + arg(0) + ": " + (reason.empty() ? std::string("Kicked by an operator") : reason);
        return true;
    }

    if (name == "ban" || name == "pardon" || name == "ban-ip" || name == "pardon-ip")
    {
        if (args.empty())
        {
            output = "/" + name + " <player>";
            return false;
        }
        if (name == "ban" || name == "ban-ip")
        {
            m_net.banned.insert(arg(0));
            m_net.kick(arg(0), "You are banned from this world");
            output = "Banned " + arg(0);
        }
        else
        {
            m_net.banned.erase(arg(0));
            output = "Unbanned " + arg(0);
        }
        return true;
    }

    if (name == "banlist")
    {
        output = m_net.banned.empty() ? std::string("There are no bans") : "There are " + std::to_string(m_net.banned.size()) + " ban(s):";
        for (const std::string& who : m_net.banned) output += " " + who;
        return true;
    }

    if (name == "whitelist")
    {
        const std::string action = arg(0);
        if (action == "on" || action == "off") { m_net.whitelistOn = action == "on"; output = std::string("Whitelist is now turned ") + action; return true; }
        if (action == "list")
        {
            output = "There are " + std::to_string(m_net.whitelist.size()) + " whitelisted player(s):";
            for (const std::string& who : m_net.whitelist) output += " " + who;
            return true;
        }
        if (action == "add" && !arg(1).empty()) { m_net.whitelist.insert(arg(1)); output = "Added " + arg(1) + " to the whitelist"; return true; }
        if (action == "remove" && !arg(1).empty()) { m_net.whitelist.erase(arg(1)); output = "Removed " + arg(1) + " from the whitelist"; return true; }
        if (action == "reload") { output = "Reloaded the whitelist"; return true; }
        output = "/whitelist on|off|list|add|remove|reload";
        return false;
    }

    if (name == "op" || name == "deop")
    {
        if (args.empty()) { output = "/" + name + " <player>"; return false; }
        if (name == "op") m_ops.insert(arg(0)); else m_ops.erase(arg(0));
        output = name == "op" ? "Made " + arg(0) + " a server operator" : "Made " + arg(0) + " no longer a server operator";
        return true;
    }

    if (name == "save-all")
    {
        if (m_world) m_world->saveAll();
        saveLevel();
        output = "Saved the game";
        return true;
    }

    if (name == "save-off" || name == "save-on")
    {
        m_autosaveOff = name == "save-off";
        output = m_autosaveOff ? "Automatic saving is now disabled" : "Automatic saving is now enabled";
        return true;
    }

    if (name == "stop")
    {
        m_pendingAction = MenuAction::QuitToTitle;
        output = "Stopping the server";
        return true;
    }

    if (name == "debug" || name == "perf")
    {
        const std::string action = arg(0);
        if (name == "perf" || action == "start")
        {
            m_debugStart = m_elapsedSeconds;
            m_debugFrames = 0;
            m_debugRunning = true;
            output = name == "perf" ? "Started 10 second performance profiling" : "Started the debug profiler";
            return true;
        }
        if (action == "stop")
        {
            if (!m_debugRunning) { output = "Can't stop profiling when we haven't started yet!"; return false; }
            m_debugRunning = false;
            const float seconds = std::max(0.001f, m_elapsedSeconds - m_debugStart);
            char text[128];
            std::snprintf(text, sizeof(text), "Stopped profiling after %.2f seconds and %d frames (%.2f frames per second)", static_cast<double>(seconds),
                          m_debugFrames, static_cast<double>(m_debugFrames / seconds));
            output = text;
            return true;
        }
        output = "/debug start|stop";
        return false;
    }

    if (name == "fillbiome")
    {
        float c[6];
        const float base[6] = { origin.x, origin.y, origin.z, origin.x, origin.y, origin.z };
        for (int k = 0; k < 6; ++k)
            if (!coordinateNear(arg(static_cast<size_t>(k)), base[k], c[k]))
            {
                output = "/fillbiome <from> <to> <biome> [replace <filter>]";
                return false;
            }
        std::string id = arg(6);
        if (id.rfind("minecraft:", 0) == 0) id = id.substr(10);
        if (!Biomes::known(id))
        {
            output = "Unknown biome: " + arg(6);
            return false;
        }
        const Biomes::Kind kind = Biomes::fromId(id);
        bool filtered = false;
        Biomes::Kind only = kind;
        if (arg(7) == "replace")
        {
            std::string filter = arg(8);
            if (filter.rfind("minecraft:", 0) == 0) filter = filter.substr(10);
            if (!Biomes::known(filter))
            {
                output = "Unknown biome: " + arg(8);
                return false;
            }
            filtered = true;
            only = Biomes::fromId(filter);
        }
        const glm::ivec3 low(static_cast<int>(std::floor(std::min(c[0], c[3]))), static_cast<int>(std::floor(std::min(c[1], c[4]))),
                             static_cast<int>(std::floor(std::min(c[2], c[5]))));
        const glm::ivec3 high(static_cast<int>(std::floor(std::max(c[0], c[3]))), static_cast<int>(std::floor(std::max(c[1], c[4]))),
                              static_cast<int>(std::floor(std::max(c[2], c[5]))));
        const glm::ivec3 q0(low.x >> 2, low.y >> 2, low.z >> 2), q1(high.x >> 2, high.y >> 2, high.z >> 2);
        const long long cells = static_cast<long long>(q1.x - q0.x + 1) * (q1.y - q0.y + 1) * (q1.z - q0.z + 1);
        if (cells > 32768)
        {
            output = "Too many blocks in the specified volume (maximum 32768, specified " + std::to_string(cells) + ")";
            return false;
        }
        const WorldGen& gen = m_world->generator();
        WorldGen::BiomeCells painted = gen.biomeCells();
        int changed = 0;
        for (int qx = q0.x; qx <= q1.x; ++qx)
            for (int qy = q0.y; qy <= q1.y; ++qy)
                for (int qz = q0.z; qz <= q1.z; ++qz)
                {
                    if (filtered && gen.kindIn(m_dimension, qx * 4, qy * 4, qz * 4) != only) continue;
                    painted[{ static_cast<int>(m_dimension), qx, qy, qz }] = kind;
                    ++changed;
                }
        gen.setBiomeCells(painted);
        m_biomeCells = painted;
        for (int cx = (low.x >> 4) - 1; cx <= (high.x >> 4) + 1; ++cx)
            for (int cz = (low.z >> 4) - 1; cz <= (high.z >> 4) + 1; ++cz)
                if (Chunk* chunk = m_world->chunkAt(cx, cz))
                {
                    gen.paintTints(*chunk);
                    m_world->remeshChunk(cx, cz);
                }
        output = std::to_string(changed) + (changed == 1 ? " biome entry set between " : " biome entries set between ") + std::to_string(low.x) +
                 ", " + std::to_string(low.y) + ", " + std::to_string(low.z) + " and " + std::to_string(high.x) + ", " + std::to_string(high.y) + ", " +
                 std::to_string(high.z);
        return true;
    }

    if (name == "transfer" || name == "setidletimeout" || name == "return")
    {
        if (name == "return") { output = "Returned"; return true; }
        if (name == "setidletimeout") { output = "The player idle timeout is now " + arg(0) + " minute(s)"; return !args.empty(); }
        output = "There is no other server to transfer to";
        return false;
    }

    output = "Unknown command: " + name;
    return false;
}
