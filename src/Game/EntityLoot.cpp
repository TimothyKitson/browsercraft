#include "EntityLoot.h"
#include "RecipeBook.h"
#include "Smelting.h"
#include "Effects.h"
#include <algorithm>
#include <cmath>
#include <map>

#include "EntityLootData.inl"

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
        };

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
                        open.push_back(&entry);
                        total += std::max(1, entry.weight);
                    }
                    if (open.empty()) continue;
                    int pick = static_cast<int>(dice.next() % static_cast<uint32_t>(total));
                    const Entry* chosen = open.back();
                    for (const Entry* entry : open)
                    {
                        pick -= std::max(1, entry->weight);
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
                    stack.count = std::min(count, maxStackOf(id));
                    if (chosen->potion[0] != '\0')
                    {
                        const std::string potion = chosen->potion;
                        stack.damage = static_cast<int>(potion == "poison" ? Effects::Kind::Poison : Effects::Kind::Slowness);
                    }
                    if (chosen->ominousAmplifier) stack.damage = dice.between(0, 4);
                    out.push_back(stack);
                }
            }
        }
    }

    bool has(const std::string& table) { return find(table) != nullptr; }

    std::vector<ItemStack> roll(const std::string& table, Context context)
    {
        std::vector<ItemStack> out;
        const Table* found = find(table);
        if (!found) return out;
        Dice dice{ context.seed ? context.seed : 1u };
        run(*found, context, dice, out, 0);
        return out;
    }
}
