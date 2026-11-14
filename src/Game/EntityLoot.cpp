#include "EntityLoot.h"
#include "RecipeBook.h"
#include "Smelting.h"
#include "Effects.h"
#include "Brewing.h"
#include "Enchanting.h"
#include "Food.h"
#include "Maps.h"
#include "Tools.h"
#include "Armour.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <numeric>

#include "LootData.inl"

namespace EntityLoot
{
    namespace
    {
        struct Dice
        {
            uint32_t state;
            uint32_t next()
            {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                return state;
            }
            float unit() { return static_cast<float>(next() % 1000000u) / 1000000.0f; }
            int between(int low, int high) { return high <= low ? low : low + static_cast<int>(next() % static_cast<uint32_t>(high - low + 1)); }
            int below(int bound) { return bound <= 1 ? 0 : static_cast<int>(next() % static_cast<uint32_t>(bound)); }
            bool coin() { return (next() >> 7) & 1u; }
        };

        uint32_t scramble(uint32_t seed)
        {
            uint32_t z = seed + 0x9E3779B9u;
            z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
            z = (z ^ (z >> 13)) * 0xC2B2AE35u;
            z ^= z >> 16;
            return z ? z : 1u;
        }

        const Table* find(const std::string& name)
        {
            for (int i = 0; i < TABLE_COUNT; ++i)
                if (name == TABLES[i].name) return &TABLES[i];
            return nullptr;
        }

        bool passes(const Rule* rules, int count, const Context& context, Dice& dice)
        {
            for (int i = 0; i < count; ++i)
            {
                const Rule& rule = rules[i];
                switch (rule.kind)
                {
                    case Rule::None: break;
                    case Rule::Player: if (!context.byPlayer) return false; break;
                    case Rule::Chance: if (dice.unit() >= rule.a) return false; break;
                    case Rule::LootingChance:
                    {
                        const float chance = context.looting > 0 ? rule.c + rule.b * static_cast<float>(context.looting - 1) : rule.a;
                        if (dice.unit() >= chance) return false;
                        break;
                    }
                    case Rule::Fire: if (!context.burning && !context.fireAspect) return false; break;
                    case Rule::KillerSkeleton: if (!context.killerSkeleton) return false; break;
                    case Rule::Size: if (context.size < static_cast<int>(rule.a) || context.size > static_cast<int>(rule.b)) return false; break;
                    case Rule::Captain: if (!context.captain) return false; break;
                    case Rule::Lightning: if (!context.lightning) return false; break;
                    case Rule::Frog: if (context.frog < 0) return false; break;
                    case Rule::FrogTemperate: if (context.frog != 0) return false; break;
                    case Rule::FrogWarm: if (context.frog != 1) return false; break;
                    case Rule::FrogCold: if (context.frog != 2) return false; break;
                    case Rule::NotFrog: if (context.frog >= 0) return false; break;
                    case Rule::OpenWater: if (!context.openWater) return false; break;
                    case Rule::Jungle: if (!context.jungle) return false; break;
                    case Rule::Never: return false;
                }
            }
            return true;
        }

        StackId stackOf(const char* id)
        {
            static std::map<std::string, StackId> cache;
            const auto found = cache.find(id);
            if (found != cache.end()) return found->second;
            const StackId stack = RecipeBook::stackNamed(id);
            cache[id] = stack;
            return stack;
        }

        struct Ref
        {
            bool trident = false;
            int kind = -1;
        };

        Ref refNamed(const std::string& id)
        {
            Ref ref;
            for (int k = 0; k < Enchanting::KIND_COUNT; ++k)
                if (id == Enchanting::idOf(static_cast<Enchanting::Kind>(k)))
                {
                    ref.kind = k;
                    return ref;
                }
            static const char* const TRIDENT[] = { "loyalty", "riptide", "channeling", "impaling" };
            for (int k = 0; k < 4; ++k)
                if (id == TRIDENT[k])
                {
                    ref.trident = true;
                    ref.kind = k;
                    return ref;
                }
            return ref;
        }

        const Ref& refOf(int index)
        {
            static std::vector<Ref> refs;
            if (refs.empty())
                for (int i = 0; i < ENCHANT_COUNT; ++i) refs.push_back(refNamed(ENCHANTS[i].id));
            return refs[static_cast<size_t>(index)];
        }

        int enchantIndex(const std::string& id)
        {
            for (int i = 0; i < ENCHANT_COUNT; ++i)
                if (id == ENCHANTS[i].id) return i;
            return -1;
        }

        bool isBook(StackId id) { return id == Items::Book || id == Items::EnchantedBook; }

        bool supports(const Ref& ref, StackId item)
        {
            if (ref.kind < 0) return false;
            if (isBook(item)) return true;
            if (ref.trident) return item == Items::Trident;
            return Enchanting::appliesTo(static_cast<Enchanting::Kind>(ref.kind), item);
        }

        bool primary(const Ref& ref, StackId item)
        {
            if (!supports(ref, item)) return false;
            if (ref.trident || isBook(item)) return true;
            using K = Enchanting::Kind;
            const K kind = static_cast<K>(ref.kind);
            if (kind == K::Sharpness || kind == K::Smite || kind == K::BaneOfArthropods || kind == K::FireAspect)
                return Tools::kindOf(item) == Tools::Kind::Sword;
            if (kind == K::Thorns) return Armour::pieceOf(item) == Armour::Piece::Chestplate;
            return true;
        }

        const char* const ARMOR_SET[] = { "protection", "blast_protection", "fire_protection", "projectile_protection", nullptr };
        const char* const BOOTS_SET[] = { "frost_walker", "depth_strider", nullptr };
        const char* const BOW_SET[] = { "infinity", "mending", nullptr };
        const char* const CROSSBOW_SET[] = { "multishot", "piercing", nullptr };
        const char* const DAMAGE_SET[] = { "sharpness", "smite", "bane_of_arthropods", "impaling", "density", "breach", nullptr };
        const char* const MINING_SET[] = { "fortune", "silk_touch", nullptr };
        const char* const RIPTIDE_SET[] = { "loyalty", "channeling", nullptr };

        const char* const* exclusiveOf(const std::string& id)
        {
            if (id == "protection" || id == "blast_protection" || id == "fire_protection" || id == "projectile_protection") return ARMOR_SET;
            if (id == "frost_walker" || id == "depth_strider") return BOOTS_SET;
            if (id == "infinity") return BOW_SET;
            if (id == "multishot" || id == "piercing") return CROSSBOW_SET;
            if (id == "sharpness" || id == "smite" || id == "bane_of_arthropods" || id == "impaling" || id == "density" || id == "breach") return DAMAGE_SET;
            if (id == "fortune" || id == "silk_touch") return MINING_SET;
            if (id == "riptide") return RIPTIDE_SET;
            return nullptr;
        }

        bool listed(const char* const* set, const std::string& id)
        {
            if (!set) return false;
            for (; *set; ++set)
                if (id == *set) return true;
            return false;
        }

        bool combinable(const std::string& a, const std::string& b)
        {
            return a != b && !listed(exclusiveOf(a), b) && !listed(exclusiveOf(b), a);
        }

        int minCost(const Enchant& e, int level) { return e.minBase + e.minPer * (level - 1); }
        int maxCost(const Enchant& e, int level) { return e.maxBase + e.maxPer * (level - 1); }

        void addEnchant(ItemStack& stack, int index, int level)
        {
            const Ref& ref = refOf(index);
            if (ref.kind < 0 || level <= 0) return;
            if (stack.id == Items::Book) stack.id = Items::EnchantedBook;
            if (ref.trident)
                stack.enchantments = Enchanting::withTrident(stack.enchantments, static_cast<Enchanting::TridentKind>(ref.kind), level);
            else
                stack.enchantments = Enchanting::withLevel(stack.enchantments, static_cast<Enchanting::Kind>(ref.kind), level);
        }

        void enchantLevels(ItemStack& stack, int level, Dice& dice, bool randomLoot)
        {
            const int enchantability = Enchanting::enchantability(stack.id);
            if (enchantability <= 0) return;
            level += 1 + dice.below(enchantability / 4 + 1) + dice.below(enchantability / 4 + 1);
            const float spread = (dice.unit() + dice.unit() - 1.0f) * 0.15f;
            level = std::max(1, static_cast<int>(std::floor(static_cast<float>(level) + static_cast<float>(level) * spread + 0.5f)));

            struct Option { int index; int level; };
            std::vector<Option> options;
            for (int i = 0; i < ENCHANT_COUNT; ++i)
            {
                const Enchant& e = ENCHANTS[i];
                if (randomLoot ? !e.randomLoot : !e.nonTreasure) continue;
                if (!primary(refOf(i), stack.id)) continue;
                for (int l = e.maxLevel; l >= 1; --l)
                    if (level >= minCost(e, l) && level <= maxCost(e, l))
                    {
                        options.push_back({ i, l });
                        break;
                    }
            }
            if (options.empty()) return;

            std::vector<Option> chosen;
            auto pick = [&]() {
                int total = 0;
                for (const Option& o : options) total += ENCHANTS[o.index].weight;
                if (total <= 0) return;
                int roll = dice.below(total);
                for (const Option& o : options)
                {
                    roll -= ENCHANTS[o.index].weight;
                    if (roll < 0)
                    {
                        chosen.push_back(o);
                        return;
                    }
                }
            };

            pick();
            while (dice.below(50) <= level)
            {
                if (!chosen.empty())
                {
                    const std::string last = ENCHANTS[chosen.back().index].id;
                    options.erase(std::remove_if(options.begin(), options.end(),
                                                 [&](const Option& o) { return !combinable(last, ENCHANTS[o.index].id); }),
                                  options.end());
                }
                if (options.empty()) break;
                pick();
                level /= 2;
            }

            for (const Option& o : chosen) addEnchant(stack, o.index, o.level);
        }

        void enchantRandomly(ItemStack& stack, const char* options, Dice& dice)
        {
            std::vector<int> pool;
            if (std::strcmp(options, "*") == 0)
            {
                for (int i = 0; i < ENCHANT_COUNT; ++i)
                    if (ENCHANTS[i].randomLoot) pool.push_back(i);
            }
            else
            {
                std::string list = options;
                size_t start = 0;
                while (start <= list.size())
                {
                    const size_t comma = list.find(',', start);
                    const std::string id = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                    const int index = enchantIndex(id);
                    if (index >= 0) pool.push_back(index);
                    if (comma == std::string::npos) break;
                    start = comma + 1;
                }
            }
            if (!isBook(stack.id))
                pool.erase(std::remove_if(pool.begin(), pool.end(), [&](int i) { return !supports(refOf(i), stack.id); }), pool.end());
            if (pool.empty()) return;
            const int index = pool[static_cast<size_t>(dice.below(static_cast<int>(pool.size())))];
            addEnchant(stack, index, dice.between(1, ENCHANTS[index].maxLevel));
        }

        void setEnchantments(ItemStack& stack, const char* fixed)
        {
            std::string list = fixed;
            size_t start = 0;
            while (start < list.size())
            {
                const size_t comma = list.find(',', start);
                const std::string part = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                const size_t colon = part.find(':');
                if (colon != std::string::npos)
                {
                    const int index = enchantIndex(part.substr(0, colon));
                    if (index >= 0) addEnchant(stack, index, std::atoi(part.c_str() + colon + 1));
                }
                if (comma == std::string::npos) break;
                start = comma + 1;
            }
            if (stack.id == Items::Book) stack.id = Items::EnchantedBook;
        }

        bool brewNamed(std::string name, Brewing::Brew& brew)
        {
            static const char* const IDS[] = {
                "water", "awkward", "swiftness", "strength", "night_vision", "fire_resistance", "regeneration", "leaping",
                "mundane", "thick", "slowness", "weakness", "poison", "water_breathing", "turtle_master", "invisibility",
                "healing", "harming", "slow_falling", "luck", "wind_charged", "weaving", "oozing", "infested"
            };
            if (name.rfind("long_", 0) == 0)
            {
                brew.extended = true;
                name = name.substr(5);
            }
            else if (name.rfind("strong_", 0) == 0)
            {
                brew.strong = true;
                name = name.substr(7);
            }
            const int count = static_cast<int>(sizeof(IDS) / sizeof(IDS[0]));
            for (int i = 0; i < count && i < static_cast<int>(Brewing::Potion::Count); ++i)
                if (name == IDS[i])
                {
                    brew.potion = static_cast<Brewing::Potion>(i);
                    return true;
                }
            return false;
        }

        void setPotion(ItemStack& stack, const char* name)
        {
            Brewing::Brew brew;
            if (!brewNamed(name, brew)) return;
            if (stack.id == Items::TippedArrow)
            {
                const std::vector<Brewing::Dose> doses = Brewing::dosesOf(brew);
                if (!doses.empty()) stack.damage = Brewing::tippedDamage(doses.front().kind, doses.front().amplifier);
                return;
            }
            stack.damage = Brewing::encode(brew);
        }

        int flowerForEffect(const std::string& effect)
        {
            static const char* const BY_FLOWER[] = {
                "fire_resistance", "blindness", "saturation", "saturation", "jump_boost", "poison",
                "regeneration", "night_vision", "weakness", "wither"
            };
            for (int i = 0; i < static_cast<int>(sizeof(BY_FLOWER) / sizeof(BY_FLOWER[0])); ++i)
                if (effect == BY_FLOWER[i]) return i;
            return -1;
        }

        void setStew(ItemStack& stack, const char* stew, Dice& dice)
        {
            std::vector<std::string> effects;
            std::string list = stew;
            size_t start = 0;
            while (start < list.size())
            {
                const size_t semi = list.find(';', start);
                const std::string part = list.substr(start, semi == std::string::npos ? std::string::npos : semi - start);
                effects.push_back(part.substr(0, part.find(':')));
                if (semi == std::string::npos) break;
                start = semi + 1;
            }
            if (effects.empty()) return;
            const int flower = flowerForEffect(effects[static_cast<size_t>(dice.below(static_cast<int>(effects.size())))]);
            if (flower >= 0) stack.damage = flower;
        }

        void run(const Table& table, const Context& context, Dice& dice, std::vector<ItemStack>& out, int depth)
        {
            for (int p = 0; p < table.poolCount; ++p)
            {
                const Pool& pool = POOLS[table.firstPool + p];
                if (!passes(pool.rules, pool.ruleCount, context, dice)) continue;
                const int rolls = dice.between(static_cast<int>(std::floor(pool.rollsMin)), static_cast<int>(std::floor(pool.rollsMax)));
                for (int r = 0; r < rolls; ++r)
                {
                    std::vector<const Entry*> open;
                    int total = 0;
                    for (int e = 0; e < pool.count; ++e)
                    {
                        const Entry& entry = ENTRIES[pool.first + e];
                        if (!passes(entry.rules, entry.ruleCount, context, dice)) continue;
                        const int weight = static_cast<int>(std::floor(static_cast<float>(entry.weight) + static_cast<float>(entry.quality) * context.luck));
                        if (weight <= 0) continue;
                        open.push_back(&entry);
                        total += weight;
                    }
                    if (open.empty()) continue;
                    int pick = static_cast<int>(dice.next() % static_cast<uint32_t>(total));
                    const Entry* chosen = open.back();
                    for (const Entry* entry : open)
                    {
                        pick -= static_cast<int>(std::floor(static_cast<float>(entry->weight) + static_cast<float>(entry->quality) * context.luck));
                        if (pick < 0) { chosen = entry; break; }
                    }
                    if (chosen->table[0] != '\0')
                    {
                        if (depth < 4)
                            if (const Table* inner = find(chosen->table)) run(*inner, context, dice, out, depth + 1);
                        continue;
                    }
                    if (chosen->item[0] == '\0') continue;
                    StackId id = stackOf(chosen->item);
                    if (id == Blocks::Air) continue;
                    int count = dice.between(static_cast<int>(std::floor(chosen->countMin)), static_cast<int>(std::floor(chosen->countMax)));
                    if (context.looting > 0 && (chosen->lootingMax > 0.0f || chosen->lootingMin > 0.0f))
                    {
                        const float extra = static_cast<float>(context.looting) *
                                            (chosen->lootingMin + dice.unit() * (chosen->lootingMax - chosen->lootingMin));
                        count += static_cast<int>(std::lround(extra));
                        if (chosen->lootingLimit > 0) count = std::min(count, chosen->lootingLimit);
                    }
                    if (count <= 0) continue;
                    if (chosen->smelt && (context.burning || context.fireAspect))
                    {
                        const StackId cooked = Smelting::resultOf(id);
                        if (cooked != Blocks::Air) id = cooked;
                    }
                    ItemStack stack;
                    stack.id = id;
                    stack.count = count;
                    if (chosen->potion[0] != '\0') setPotion(stack, chosen->potion);
                    if (chosen->ampMin >= 0.0f)
                        stack.damage = dice.between(static_cast<int>(std::lround(chosen->ampMin)), static_cast<int>(std::lround(chosen->ampMax)));
                    if (chosen->damageMin >= 0.0f && itemDurability(stack.id) > 0)
                    {
                        const float left = chosen->damageMin + dice.unit() * (chosen->damageMax - chosen->damageMin);
                        stack.damage = static_cast<int>(std::floor((1.0f - left) * static_cast<float>(itemDurability(stack.id))));
                    }
                    if (chosen->levelsMin >= 0.0f)
                        enchantLevels(stack, dice.between(static_cast<int>(std::lround(chosen->levelsMin)), static_cast<int>(std::lround(chosen->levelsMax))), dice, true);
                    if (chosen->enchantOptions[0] != '\0') enchantRandomly(stack, chosen->enchantOptions, dice);
                    if (chosen->fixedEnchant[0] != '\0') setEnchantments(stack, chosen->fixedEnchant);
                    if (chosen->stew[0] != '\0') setStew(stack, chosen->stew, dice);
                    if (chosen->map)
                    {
                        stack.id = Items::FilledMap;
                        stack.enchantments = static_cast<uint32_t>(Maps::TreasureExplorer) << 24;
                    }
                    if (chosen->instrument) stack.damage = dice.below(4);
                    out.push_back(stack);
                }
            }
        }

        std::vector<ItemStack> split(const std::vector<ItemStack>& stacks)
        {
            std::vector<ItemStack> out;
            for (const ItemStack& stack : stacks)
            {
                const int most = std::max(1, maxStackOf(stack.id));
                int left = stack.count;
                while (left > 0)
                {
                    ItemStack part = stack;
                    part.count = std::min(most, left);
                    left -= part.count;
                    out.push_back(part);
                }
            }
            return out;
        }

        template <typename T>
        void shuffle(std::vector<T>& items, Dice& dice)
        {
            for (int i = static_cast<int>(items.size()); i > 1; --i)
                std::swap(items[static_cast<size_t>(i - 1)], items[static_cast<size_t>(dice.below(i))]);
        }
    }

    bool has(const std::string& table) { return find(table) != nullptr; }
    int tableCount() { return TABLE_COUNT; }

    std::vector<std::string> unknownItems()
    {
        std::vector<std::string> out;
        for (const Entry& entry : ENTRIES)
            if (entry.item[0] != '\0' && stackOf(entry.item) == Blocks::Air &&
                std::find(out.begin(), out.end(), entry.item) == out.end())
                out.push_back(entry.item);
        for (const Entry& entry : ENTRIES)
            if (entry.table[0] != '\0' && !find(entry.table)) out.push_back(std::string("table ") + entry.table);
        return out;
    }
    const char* tableName(int index) { return index >= 0 && index < TABLE_COUNT ? TABLES[index].name : ""; }

    std::vector<ItemStack> roll(const std::string& table, Context context)
    {
        std::vector<ItemStack> out;
        const Table* found = find(table);
        if (!found) return out;
        Dice dice{ context.seed ? context.seed : 1u };
        run(*found, context, dice, out, 0);
        return split(out);
    }

    std::vector<ItemStack> fillChest(const std::string& table, uint32_t seed, int slots)
    {
        std::vector<ItemStack> out(static_cast<size_t>(std::max(0, slots)));
        const Table* found = find(table);
        if (!found || slots <= 0) return out;
        Dice dice{ scramble(seed) };
        std::vector<ItemStack> drops;
        run(*found, Context{}, dice, drops, 0);
        drops = split(drops);

        std::vector<int> free(static_cast<size_t>(slots));
        std::iota(free.begin(), free.end(), 0);
        shuffle(free, dice);

        std::vector<ItemStack> single;
        std::vector<ItemStack> many;
        for (const ItemStack& stack : drops)
        {
            if (stack.empty()) continue;
            if (stack.count > 1) many.push_back(stack);
            else single.push_back(stack);
        }
        while (slots - static_cast<int>(single.size()) - static_cast<int>(many.size()) > 0 && !many.empty())
        {
            const size_t at = static_cast<size_t>(dice.between(0, static_cast<int>(many.size()) - 1));
            ItemStack whole = many[at];
            many.erase(many.begin() + static_cast<long>(at));
            ItemStack part = whole;
            part.count = dice.between(1, whole.count / 2);
            whole.count -= part.count;
            if (whole.count > 1 && dice.coin()) many.push_back(whole);
            else single.push_back(whole);
            if (part.count > 1 && dice.coin()) many.push_back(part);
            else single.push_back(part);
        }
        single.insert(single.end(), many.begin(), many.end());
        shuffle(single, dice);

        for (const ItemStack& stack : single)
        {
            if (free.empty()) break;
            out[static_cast<size_t>(free.back())] = stack;
            free.pop_back();
        }
        return out;
    }

    ItemStack enchantWithLevels(ItemStack stack, int levels, uint32_t seed, bool treasure)
    {
        Dice dice{ scramble(seed) };
        enchantLevels(stack, levels, dice, treasure);
        return stack;
    }
}
