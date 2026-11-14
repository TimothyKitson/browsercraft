#include <algorithm>
#include "Trading.h"
#include "Maps.h"
#include "World/Block.h"
#include "Enchanting.h"
#include "EntityLoot.h"
#include "RecipeBook.h"
#include "Brewing.h"
#include <map>
#include <string>

#include "TradeData.inl"

namespace Trading
{
    const char* name(Profession trade)
    {
        switch (trade)
        {
        case Profession::Farmer:    return "Farmer";
        case Profession::Librarian: return "Librarian";
        case Profession::Toolsmith: return "Toolsmith";
        case Profession::Fletcher:  return "Fletcher";
        case Profession::Armorer:   return "Armorer";
        case Profession::Butcher:   return "Butcher";
        case Profession::Cartographer: return "Cartographer";
        case Profession::Cleric:    return "Cleric";
        case Profession::Fisherman: return "Fisherman";
        case Profession::Leatherworker: return "Leatherworker";
        case Profession::Mason:     return "Mason";
        case Profession::Nitwit:    return "Nitwit";
        case Profession::Shepherd:  return "Shepherd";
        case Profession::Weaponsmith: return "Weaponsmith";
        default: break;
        }
        return "Villager";
    }

    const char* textureName(Profession trade)
    {
        static const char* NAMES[] = { "farmer", "librarian", "toolsmith", "fletcher", "armorer", "butcher",
                                       "cartographer", "cleric", "fisherman", "leatherworker", "mason",
                                       "nitwit", "shepherd", "weaponsmith" };
        const int index = static_cast<int>(trade);
        if (index < 0 || index >= static_cast<int>(Profession::Count)) return "farmer";
        return NAMES[index];
    }

    bool jobSite(BlockId block, Profession& trade)
    {
        if (isComposter(block)) { trade = Profession::Farmer; return true; }
        if (isLectern(block)) { trade = Profession::Librarian; return true; }
        if (isAnyCauldron(block)) { trade = Profession::Leatherworker; return true; }
        if (isLoom(block)) { trade = Profession::Shepherd; return true; }
        if (isBarrel(block)) { trade = Profession::Fisherman; return true; }
        if (isGrindstone(block)) { trade = Profession::Weaponsmith; return true; }
        if (isStonecutter(block)) { trade = Profession::Mason; return true; }
        switch (furnaceBase(block))
        {
            case Blocks::BlastFurnace: case Blocks::BlastFurnaceLit: trade = Profession::Armorer; return true;
            case Blocks::Smoker: case Blocks::SmokerLit: trade = Profession::Butcher; return true;
            default: break;
        }
        switch (block)
        {
            case Blocks::BrewingStand: trade = Profession::Cleric; return true;
            case Blocks::FletchingTable: trade = Profession::Fletcher; return true;
            case Blocks::SmithingTable: trade = Profession::Toolsmith; return true;
            case Blocks::CartographyTable: trade = Profession::Cartographer; return true;
            default: return false;
        }
    }

    Profession professionFor(uint32_t seed)
    {
        // Stirred rather than taken straight: the seeds these come from
        // are small and consecutive, and the low bits of consecutive
        // numbers would hand out professions in a repeating cycle.
        uint32_t h = seed * 0x9E3779B9u;
        h ^= h >> 16; h *= 0x7FEB352Du;
        h ^= h >> 15; h *= 0x846CA68Bu;
        h ^= h >> 16;
        return static_cast<Profession>(h % static_cast<uint32_t>(Profession::Count));
    }

    const char* rankName(int rank)
    {
        static const char* NAMES[RANKS] = { "Novice", "Apprentice", "Journeyman", "Expert", "Master" };
        return NAMES[rank < 0 ? 0 : (rank >= RANKS ? RANKS - 1 : rank)];
    }

    int xpForRank(int rank)
    {
        static const int NEEDED[RANKS] = { 0, 12, 60, 140, 240 };
        return NEEDED[rank < 0 ? 0 : (rank >= RANKS ? RANKS - 1 : rank)];
    }

    int rankFor(int xp)
    {
        int rank = 0;
        while (rank + 1 < RANKS && xp >= xpForRank(rank + 1)) ++rank;
        return rank;
    }

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
            int below(int bound) { return bound <= 1 ? 0 : static_cast<int>(next() % static_cast<uint32_t>(bound)); }
            float unit() { return static_cast<float>(next() % 1000000u) / 1000000.0f; }
        };

        uint32_t mix(uint32_t a, uint32_t b)
        {
            uint32_t h = a * 0x9E3779B9u ^ (b + 0x7F4A7C15u);
            h ^= h >> 16; h *= 0x7FEB352Du;
            h ^= h >> 15; h *= 0x846CA68Bu;
            h ^= h >> 16;
            return h ? h : 1u;
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

        std::vector<Brewing::Brew> arrowBrews()
        {
            std::vector<Brewing::Brew> out;
            for (int p = 0; p < static_cast<int>(Brewing::Potion::Count); ++p)
            {
                const Brewing::Potion potion = static_cast<Brewing::Potion>(p);
                if (potion == Brewing::Potion::Water || potion == Brewing::Potion::Awkward || potion == Brewing::Potion::Mundane ||
                    potion == Brewing::Potion::Thick || potion == Brewing::Potion::Luck)
                    continue;
                out.push_back(Brewing::Brew{ potion, false, false });
                if (Brewing::canExtend(potion)) out.push_back(Brewing::Brew{ potion, true, false });
                if (Brewing::canStrengthen(potion)) out.push_back(Brewing::Brew{ potion, false, true });
            }
            return out;
        }

        Deal resolve(const Factory& f, Dice& dice, int villagerType)
        {
            Deal d;
            d.rank = std::max(0, static_cast<int>(f.level) - 1);
            d.maxUses = f.maxUses;
            d.villagerXp = f.xp;
            d.playerXp = 0;
            d.multiplier = f.multiplier;
            Offer& o = d.offer;
            const StackId item = f.item[0] ? stackOf(f.item) : static_cast<StackId>(Blocks::Air);
            switch (f.kind)
            {
            case FactoryKind::Buy:
                o.want = item; o.wantCount = f.count; o.give = Items::Emerald; o.giveCount = f.price;
                break;
            case FactoryKind::Sell:
            case FactoryKind::Dyed:
                o.want = Items::Emerald; o.wantCount = f.price; o.give = item; o.giveCount = f.count;
                break;
            case FactoryKind::Process:
            case FactoryKind::Tipped:
                o.want = Items::Emerald; o.wantCount = f.price; o.also = stackOf(f.item2); o.alsoCount = f.count2;
                o.give = item; o.giveCount = f.count;
                if (f.kind == FactoryKind::Tipped)
                {
                    const std::vector<Brewing::Brew> brews = arrowBrews();
                    const Brewing::Brew brew = brews[static_cast<size_t>(dice.below(static_cast<int>(brews.size())))];
                    const std::vector<Brewing::Dose> doses = Brewing::dosesOf(brew);
                    if (!doses.empty()) o.giveDamage = Brewing::tippedDamage(doses.front().kind, doses.front().amplifier);
                }
                break;
            case FactoryKind::Book:
            {
                std::vector<EntityLoot::EnchantInfo> options;
                for (const EntityLoot::EnchantInfo& info : EntityLoot::enchantList())
                    if (info.nonTreasure || info.id == "binding_curse" || info.id == "vanishing_curse" ||
                        info.id == "frost_walker" || info.id == "mending")
                        options.push_back(info);
                ItemStack book{ static_cast<StackId>(Items::Book), 1 };
                int price = 1;
                if (!options.empty())
                {
                    const EntityLoot::EnchantInfo& pick = options[static_cast<size_t>(dice.below(static_cast<int>(options.size())))];
                    const int level = 1 + dice.below(std::max(1, pick.maxLevel));
                    book = EntityLoot::withEnchant(book, pick.id, level);
                    price = 2 + dice.below(5 + level * 10) + 3 * level;
                    if (!pick.nonTreasure) price *= 2;
                    price = std::min(price, 64);
                }
                o.want = Items::Emerald; o.wantCount = price; o.also = Items::Book; o.alsoCount = 1;
                o.give = book.id; o.giveCount = 1; o.giveEnchantments = book.enchantments;
                d.maxUses = 12;
                break;
            }
            case FactoryKind::Tool:
            {
                const int level = 5 + dice.below(15);
                ItemStack tool{ item, 1 };
                tool = EntityLoot::enchantWithLevels(tool, level, dice.next(), false);
                o.want = Items::Emerald; o.wantCount = std::min(f.price + level, 64);
                o.give = tool.id; o.giveCount = 1; o.giveEnchantments = tool.enchantments;
                break;
            }
            case FactoryKind::Map:
                o.want = Items::Emerald; o.wantCount = f.price; o.also = Items::Compass; o.alsoCount = 1;
                o.give = Items::FilledMap; o.giveCount = 1; o.giveEnchantments = static_cast<uint32_t>(f.extra) << 24;
                break;
            case FactoryKind::Stew:
            {
                static const int FLOWERS[6] = { 7, 4, 8, 1, 5, 2 };
                o.want = Items::Emerald; o.wantCount = 1; o.give = Items::SuspiciousStew; o.giveCount = 1;
                o.giveDamage = FLOWERS[std::clamp(f.extra, 0, 5)];
                break;
            }
            case FactoryKind::Boat:
            {
                static const char* const BOATS[VILLAGER_TYPES] = { "oak_boat", "jungle_boat", "jungle_boat", "acacia_boat",
                                                                   "spruce_boat", "dark_oak_boat", "spruce_boat" };
                o.want = stackOf(BOATS[std::clamp(villagerType, 0, VILLAGER_TYPES - 1)]); o.wantCount = f.count;
                o.give = Items::Emerald; o.giveCount = 1;
                break;
            }
            }
            if (o.want != Blocks::Air) o.wantCount = std::clamp(o.wantCount, 1, maxStackOf(o.want));
            if (o.also != Blocks::Air) o.alsoCount = std::clamp(o.alsoCount, 1, maxStackOf(o.also));
            return d;
        }

        std::vector<const Factory*> poolOf(int profession, int level)
        {
            std::vector<const Factory*> pool;
            for (int i = 0; i < FACTORY_COUNT; ++i)
                if (FACTORIES[i].profession == profession && FACTORIES[i].level == level) pool.push_back(&FACTORIES[i]);
            return pool;
        }

        void pickFrom(std::vector<const Factory*> pool, int count, Dice& dice, int villagerType, std::vector<Deal>& out)
        {
            int taken = 0;
            while (taken < count && !pool.empty())
            {
                const size_t at = static_cast<size_t>(dice.below(static_cast<int>(pool.size())));
                const Factory* chosen = pool[at];
                pool.erase(pool.begin() + static_cast<long>(at));
                const Deal deal = resolve(*chosen, dice, villagerType);
                if (!deal.offer.valid()) continue;
                out.push_back(deal);
                ++taken;
            }
        }
    }

    std::vector<Deal> wandererDeals(uint32_t seed)
    {
        std::vector<Deal> out;
        Dice dice{ mix(seed, 0xA11Du) };
        pickFrom(poolOf(255, 1), 5, dice, 0, out);
        pickFrom(poolOf(255, 2), 1, dice, 0, out);
        return out;
    }

    std::vector<Deal> dealsFor(Profession trade, uint32_t seed, int villagerType)
    {
        std::vector<Deal> out;
        for (int level = 1; level <= RANKS; ++level)
        {
            Dice dice{ mix(seed, static_cast<uint32_t>(level) * 977u + static_cast<uint32_t>(trade)) };
            pickFrom(poolOf(static_cast<int>(trade), level), TRADES_PER_LEVEL, dice, villagerType, out);
        }
        return out;
    }

    std::vector<Deal> dealsFor(Profession trade)
    {
        std::vector<Deal> out;
        Dice dice{ 0x5EEDu };
        for (int i = 0; i < FACTORY_COUNT; ++i)
            if (FACTORIES[i].profession == static_cast<int>(trade))
            {
                const Deal deal = resolve(FACTORIES[i], dice, 0);
                if (deal.offer.valid()) out.push_back(deal);
            }
        return out;
    }

    std::vector<std::string> unknownTradeItems()
    {
        std::vector<std::string> out;
        for (int i = 0; i < FACTORY_COUNT; ++i)
            for (const char* id : { FACTORIES[i].item, FACTORIES[i].item2 })
                if (id[0] && stackOf(id) == Blocks::Air && std::find(out.begin(), out.end(), id) == out.end()) out.push_back(id);
        for (const char* boat : { "oak_boat", "jungle_boat", "acacia_boat", "spruce_boat", "dark_oak_boat" })
            if (stackOf(boat) == Blocks::Air) out.push_back(boat);
        return out;
    }

    std::vector<Offer> offersFor(Profession trade)
    {
        std::vector<Offer> offers;
        for (const Deal& deal : dealsFor(trade)) offers.push_back(deal.offer);
        return offers;
    }

    bool affordable(const Offer& offer, int haveWant, int haveAlso)
    {
        if (!offer.valid()) return false;
        if (haveWant < offer.wantCount) return false;
        if (offer.also != Blocks::Air && haveAlso < offer.alsoCount) return false;
        return true;
    }
}
