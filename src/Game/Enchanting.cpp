#include "Enchanting.h"
#include "Tools.h"
#include "Armour.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <array>
#include <initializer_list>
#include <map>
#include <sstream>

namespace Enchanting
{
    namespace
    {
        int shift(Kind kind) { return static_cast<int>(kind) * BITS_PER_KIND; }

        // A roll stirred into a different number, so one draw can serve
        // several decisions without them marching in step.
        uint32_t stir(uint32_t value, uint32_t salt)
        {
            value ^= salt * 0x9E3779B9u;
            value ^= value >> 16; value *= 0x7FEB352Du;
            value ^= value >> 15; value *= 0x846CA68Bu;
            value ^= value >> 16;
            return value;
        }
    }

    namespace
    {
        struct Info
        {
            const char* name;
            const char* id;
            int maxLevel;
            bool treasure;
        };

        const Info INFO[KIND_COUNT] = {
            { "Efficiency", "efficiency", 5, false },
            { "Unbreaking", "unbreaking", 3, false },
            { "Fortune", "fortune", 3, false },
            { "Silk Touch", "silk_touch", 1, false },
            { "Sharpness", "sharpness", 5, false },
            { "Protection", "protection", 4, false },
            { "Power", "power", 5, false },
            { "Frost Walker", "frost_walker", 2, true },
            { "Fire Protection", "fire_protection", 4, false },
            { "Feather Falling", "feather_falling", 4, false },
            { "Blast Protection", "blast_protection", 4, false },
            { "Projectile Protection", "projectile_protection", 4, false },
            { "Respiration", "respiration", 3, false },
            { "Aqua Affinity", "aqua_affinity", 1, false },
            { "Thorns", "thorns", 3, false },
            { "Depth Strider", "depth_strider", 3, false },
            { "Curse of Binding", "binding_curse", 1, true },
            { "Soul Speed", "soul_speed", 3, true },
            { "Swift Sneak", "swift_sneak", 3, true },
            { "Smite", "smite", 5, false },
            { "Bane of Arthropods", "bane_of_arthropods", 5, false },
            { "Knockback", "knockback", 2, false },
            { "Fire Aspect", "fire_aspect", 2, false },
            { "Looting", "looting", 3, false },
            { "Sweeping Edge", "sweeping_edge", 3, false },
            { "Punch", "punch", 2, false },
            { "Flame", "flame", 1, false },
            { "Infinity", "infinity", 1, false },
            { "Luck of the Sea", "luck_of_the_sea", 3, false },
            { "Lure", "lure", 3, false },
            { "Multishot", "multishot", 1, false },
            { "Quick Charge", "quick_charge", 3, false },
            { "Piercing", "piercing", 4, false },
            { "Density", "density", 5, false },
            { "Breach", "breach", 4, false },
            { "Wind Burst", "wind_burst", 3, true },
            { "Mending", "mending", 1, true },
            { "Curse of Vanishing", "vanishing_curse", 1, true },
        };

        using Levels = std::array<uint8_t, KIND_COUNT>;

        constexpr uint32_t LIBRARY_MARK = 7u << 21;
        constexpr uint32_t LIBRARY_INDEX = (1u << 21) - 1u;

        struct Library
        {
            std::vector<Levels> sets;
            std::map<Levels, uint32_t> index;
        };

        Library& library()
        {
            static Library shelf;
            return shelf;
        }

        bool shelved(Set set) { return (set & (7u << 21)) == LIBRARY_MARK; }

        Levels decode(Set set)
        {
            Levels levels{};
            if (shelved(set))
            {
                const uint32_t at = set & LIBRARY_INDEX;
                if (at < library().sets.size()) levels = library().sets[at];
                return levels;
            }
            for (int k = 0; k < PACKED_KINDS; ++k)
                levels[static_cast<size_t>(k)] = static_cast<uint8_t>((set >> (k * BITS_PER_KIND)) & 0x7u);
            return levels;
        }

        Set encode(const Levels& levels, Set top)
        {
            bool packs = levels[static_cast<size_t>(Kind::FrostWalker)] < 7;
            for (int k = PACKED_KINDS; k < KIND_COUNT && packs; ++k)
                if (levels[static_cast<size_t>(k)] != 0) packs = false;
            if (packs)
            {
                Set out = top & ~KIND_BITS;
                for (int k = 0; k < PACKED_KINDS; ++k)
                    out |= static_cast<uint32_t>(levels[static_cast<size_t>(k)] & 0x7u) << (k * BITS_PER_KIND);
                return out;
            }
            Library& shelf = library();
            auto found = shelf.index.find(levels);
            uint32_t at = 0;
            if (found != shelf.index.end()) at = found->second;
            else
            {
                at = static_cast<uint32_t>(shelf.sets.size());
                if (at > LIBRARY_INDEX) return top;
                shelf.sets.push_back(levels);
                shelf.index.emplace(levels, at);
            }
            return (top & ~KIND_BITS) | LIBRARY_MARK | at;
        }
    }

    int maxLevel(Kind kind)
    {
        const int k = static_cast<int>(kind);
        return k >= 0 && k < KIND_COUNT ? INFO[k].maxLevel : 0;
    }

    bool isTreasure(Kind kind)
    {
        const int k = static_cast<int>(kind);
        return k >= 0 && k < KIND_COUNT && INFO[k].treasure;
    }

    bool isCurse(Kind kind) { return kind == Kind::BindingCurse || kind == Kind::VanishingCurse; }

    const char* name(Kind kind)
    {
        const int k = static_cast<int>(kind);
        return k >= 0 && k < KIND_COUNT ? INFO[k].name : "";
    }

    const char* idOf(Kind kind)
    {
        const int k = static_cast<int>(kind);
        return k >= 0 && k < KIND_COUNT ? INFO[k].id : "";
    }

    bool appliesTo(Kind kind, StackId item)
    {
        if (item == Items::Book || item == Items::EnchantedBook) return kind < Kind::Count;
        const Tools::Kind tool = Tools::kindOf(item);
        const bool digs = tool == Tools::Kind::Pickaxe || tool == Tools::Kind::Axe ||
                          tool == Tools::Kind::Shovel || tool == Tools::Kind::Hoe;
        const bool sword = tool == Tools::Kind::Sword;
        const bool armour = Armour::slotFor(item) >= 0 && item != Items::Elytra;
        const Armour::Piece piece = Armour::pieceOf(item);
        const bool wears = itemDurability(item) > 0 || Tools::isTool(item) || Armour::slotFor(item) >= 0 || item == Items::Bow ||
                           item == Items::Shield || item == Items::Shears || item == Items::FlintAndSteel || item == Items::Elytra ||
                           item == Items::Mace || item == Items::Brush;
        const bool wearable = Armour::slotFor(item) >= 0 || item == Items::Elytra ||
                              (!isItem(item) && (isHead(asBlock(item)) || asBlock(item) == ::Blocks::WitherSkull ||
                                                  (asBlock(item) >= ::Blocks::CarvedPumpkinFirst && asBlock(item) <= ::Blocks::CarvedPumpkinLast)));

        switch (kind)
        {
        case Kind::Unbreaking:
        case Kind::Mending: return wears;
        case Kind::Efficiency: return digs || item == Items::Shears;
        case Kind::Fortune:
        case Kind::SilkTouch:  return digs;
        case Kind::Sharpness:  return sword || tool == Tools::Kind::Axe;
        case Kind::Smite:
        case Kind::BaneOfArthropods: return sword || tool == Tools::Kind::Axe || item == Items::Mace;
        case Kind::Knockback:
        case Kind::Looting:
        case Kind::SweepingEdge: return sword;
        case Kind::FireAspect: return sword || item == Items::Mace;
        case Kind::Protection:
        case Kind::FireProtection:
        case Kind::BlastProtection:
        case Kind::ProjectileProtection:
        case Kind::Thorns: return armour;
        case Kind::Respiration:
        case Kind::AquaAffinity: return armour && piece == Armour::Piece::Helmet;
        case Kind::SwiftSneak: return armour && piece == Armour::Piece::Leggings;
        case Kind::FeatherFalling:
        case Kind::DepthStrider:
        case Kind::SoulSpeed:
        case Kind::FrostWalker: return armour && piece == Armour::Piece::Boots;
        case Kind::BindingCurse: return wearable;
        case Kind::VanishingCurse: return wears || wearable || item == Items::Compass || item == Items::RecoveryCompass;
        case Kind::Power:
        case Kind::Punch:
        case Kind::Flame:
        case Kind::Infinity: return item == Items::Bow;
        case Kind::LuckOfTheSea:
        case Kind::Lure: return item == Items::FishingRod;
        case Kind::Multishot:
        case Kind::QuickCharge:
        case Kind::Piercing: return item == Items::Crossbow;
        case Kind::Density:
        case Kind::Breach:
        case Kind::WindBurst: return item == Items::Mace;
        default: break;
        }
        return false;
    }

    bool conflicts(Kind a, Kind b)
    {
        if (a == b) return false;
        auto pair = [&](Kind x, Kind y) { return (a == x && b == y) || (a == y && b == x); };
        auto within = [&](std::initializer_list<Kind> group) {
            bool hasA = false, hasB = false;
            for (Kind k : group) { hasA |= k == a; hasB |= k == b; }
            return hasA && hasB;
        };
        if (within({ Kind::Protection, Kind::FireProtection, Kind::BlastProtection, Kind::ProjectileProtection })) return true;
        if (within({ Kind::Sharpness, Kind::Smite, Kind::BaneOfArthropods })) return true;
        if (within({ Kind::Density, Kind::Breach, Kind::Smite, Kind::BaneOfArthropods })) return true;
        return pair(Kind::Fortune, Kind::SilkTouch) || pair(Kind::DepthStrider, Kind::FrostWalker) ||
               pair(Kind::Infinity, Kind::Mending) || pair(Kind::Multishot, Kind::Piercing);
    }

    int levelOf(Set set, Kind kind)
    {
        const int k = static_cast<int>(kind);
        if (k < 0 || k >= KIND_COUNT) return 0;
        if (!shelved(set))
            return k < PACKED_KINDS ? static_cast<int>((set >> shift(kind)) & 0x7u) : 0;
        return decode(set)[static_cast<size_t>(k)];
    }

    Set withLevel(Set set, Kind kind, int level)
    {
        const int k = static_cast<int>(kind);
        if (k < 0 || k >= KIND_COUNT) return set;
        const int capped = std::clamp(level, 0, std::min(maxLevel(kind), MAX_STORABLE_LEVEL));
        Levels levels = decode(set);
        levels[static_cast<size_t>(k)] = static_cast<uint8_t>(capped);
        return encode(levels, set);
    }

    std::string librarySave()
    {
        std::string out;
        for (const Levels& levels : library().sets)
        {
            for (int k = 0; k < KIND_COUNT; ++k)
            {
                if (k) out += ' ';
                out += std::to_string(levels[static_cast<size_t>(k)]);
            }
            out += '\n';
        }
        return out;
    }

    void libraryClear()
    {
        library().sets.clear();
        library().index.clear();
    }

    void libraryLoad(const std::string& text)
    {
        libraryClear();
        std::istringstream in(text);
        std::string row;
        while (std::getline(in, row))
        {
            std::istringstream line(row);
            Levels levels{};
            int value = 0;
            for (int k = 0; k < KIND_COUNT && line >> value; ++k)
                levels[static_cast<size_t>(k)] = static_cast<uint8_t>(std::clamp(value, 0, maxLevel(static_cast<Kind>(k))));
            library().index.emplace(levels, static_cast<uint32_t>(library().sets.size()));
            library().sets.push_back(levels);
        }
    }

    size_t librarySize() { return library().sets.size(); }

    bool has(Set set, Kind kind) { return levelOf(set, kind) > 0; }
    bool any(Set set) { return (set & KIND_BITS) != 0u; }

    bool glints(StackId id, Set set)
    {
        if (!isItem(id) && isDecoratedPot(asBlock(id))) return false;
        if (id == Items::Trident) return set != 0u;
        if (id == Items::EnchantedGoldenApple || id == Items::ExperienceBottle || id == Items::EnchantedBook ||
            id == Items::WrittenBook) return true;
        return (set & KIND_BITS) != 0u;
    }

    namespace
    {
        const int TRIDENT_SHIFT[4] = { 24, 26, 28, 29 };
        const uint32_t TRIDENT_MASK[4] = { 3u, 3u, 1u, 7u };
    }

    int tridentLevel(Set set, TridentKind kind)
    {
        const int k = static_cast<int>(kind);
        if (k < 0 || k >= 4) return 0;
        return static_cast<int>((set >> TRIDENT_SHIFT[k]) & TRIDENT_MASK[k]);
    }

    Set withTrident(Set set, TridentKind kind, int level)
    {
        const int k = static_cast<int>(kind);
        if (k < 0 || k >= 4) return set;
        const uint32_t clamped = static_cast<uint32_t>(std::clamp(level, 0, tridentMax(kind)));
        return (set & ~(TRIDENT_MASK[k] << TRIDENT_SHIFT[k])) | (clamped << TRIDENT_SHIFT[k]);
    }

    int tridentMax(TridentKind kind)
    {
        switch (kind)
        {
            case TridentKind::Loyalty: return 3;
            case TridentKind::Riptide: return 3;
            case TridentKind::Channeling: return 1;
            case TridentKind::Impaling: return 5;
            default: return 0;
        }
    }

    const char* tridentName(TridentKind kind)
    {
        switch (kind)
        {
            case TridentKind::Loyalty: return "Loyalty";
            case TridentKind::Riptide: return "Riptide";
            case TridentKind::Channeling: return "Channeling";
            case TridentKind::Impaling: return "Impaling";
            default: return "";
        }
    }

    bool tridentConflicts(Set set, TridentKind kind)
    {
        if (kind == TridentKind::Riptide)
            return tridentLevel(set, TridentKind::Loyalty) > 0 || tridentLevel(set, TridentKind::Channeling) > 0;
        if (kind == TridentKind::Loyalty || kind == TridentKind::Channeling)
            return tridentLevel(set, TridentKind::Riptide) > 0;
        return false;
    }

    int grindHundredths(Set set)
    {
        int levels = 0;
        for (int k = 0; k < static_cast<int>(Kind::Count); ++k)
            levels += levelOf(set, static_cast<Kind>(k));
        return levels * 300;
    }

    std::vector<std::pair<Kind, int>> listOf(Set set)
    {
        std::vector<std::pair<Kind, int>> out;
        for (int i = 0; i < static_cast<int>(Kind::Count); ++i)
        {
            const Kind kind = static_cast<Kind>(i);
            const int level = levelOf(set, kind);
            if (level > 0) out.emplace_back(kind, level);
        }
        return out;
    }

    // Minecraft's own numbers, and they are not the ranking anyone
    // expects: gold takes the best enchantments by a distance and wood
    // is close behind it, while diamond is middling and stone is the
    // worst thing in the game to enchant.
    int enchantability(StackId item)
    {
        // Armour is asked about first. It is not a tool, so Tools::
        // tierOf would call every piece of it Tier::None and hand the
        // whole wardrobe the same answer.
        if (item == Items::TurtleHelmet) return 9;
        if (Armour::isArmour(item))
        {
            switch (Armour::tierOf(item))
            {
            case Armour::Tier::Leather: return 15;
            case Armour::Tier::Gold:    return 25;
            case Armour::Tier::Iron:    return 9;
            case Armour::Tier::Chainmail: return 12;
            case Armour::Tier::Diamond: return 10;
            default: break;
            }
            return 0;
        }

        if (item == Items::Trident || item == Items::Book) return 1;
        if (Tools::isTool(item))
        {
            switch (Tools::tierOf(item))
            {
            case Tools::Tier::Wood:    return 15;
            case Tools::Tier::Stone:   return 5;
            case Tools::Tier::Iron:    return 14;
            case Tools::Tier::Gold:    return 22;
            case Tools::Tier::Diamond: return 10;
            default: break;
            }
            return 0;
        }

        if (item == Items::Bow || item == Items::FishingRod || item == Items::Crossbow || item == Items::Trident) return 1;
        return 0;
    }

    int shelvesAround(const Surroundings& room, int x, int y, int z)
    {
        int found = 0;

        // The ring two cells out, at the table's own level and one
        // above it, which is where a shelf two blocks high stands.
        for (int dy = 0; dy <= 1; ++dy)
        {
            for (int dx = -2; dx <= 2; ++dx)
            {
                for (int dz = -2; dz <= 2; ++dz)
                {
                    // Only the ring: the cells nearer than two are the
                    // gap, not a place a shelf counts from.
                    if (std::max(std::abs(dx), std::abs(dz)) != 2) continue;
                    if (room.at(x + dx, y + dy, z + dz) != ::Blocks::Bookshelf) continue;

                    // The cell between, stepped one towards the table.
                    // Anything standing in it blocks the shelf, which is
                    // how a wall between you and your library stops it
                    // helping.
                    const int betweenX = x + dx / 2;
                    const int betweenZ = z + dz / 2;
                    if (room.at(betweenX, y + dy, betweenZ) != ::Blocks::Air) continue;

                    ++found;
                }
            }
        }

        return std::min(found, MAX_SHELVES);
    }

    uint32_t nextRoll(uint32_t roll) { return stir(roll, 7777u); }

    int costForRow(int row, int shelves, uint32_t roll)
    {
        if (row < 0 || row > 2) return 0;
        const int capped = std::clamp(shelves, 0, MAX_SHELVES);

        // Minecraft draws one base number and derives all three rows
        // from it, which is why a poor table is poor in every row
        // rather than only sometimes.
        const int draw = 1 + static_cast<int>(stir(roll, 1u) % 8u);
        const int bonus = static_cast<int>(stir(roll, 2u) % static_cast<uint32_t>(capped + 1));
        const int base = draw + capped / 2 + bonus;

        switch (row)
        {
        case 0:  return std::max(base / 3, 1);
        case 1:  return (base * 2) / 3 + 1;
        default: return std::max(base, capped * 2);
        }
    }

    Offer offerFor(int row, int shelves, StackId item, uint32_t roll)
    {
        Offer offer;
        if (enchantability(item) <= 0) return offer;

        offer.cost = costForRow(row, shelves, roll);
        if (offer.cost <= 0) return offer;

        if (item == Items::Trident)
        {
            int order[5] = { -1, 0, 1, 2, 3 };
            for (int i = 4; i > 0; --i) std::swap(order[i], order[stir(roll, static_cast<uint32_t>(i) + 40u) % static_cast<uint32_t>(i + 1)]);
            const int pick = order[row % 5];
            const int reach = (offer.cost * (1 + enchantability(item) / 8)) / 12;
            if (pick < 0)
            {
                offer.kind = Kind::Unbreaking;
                offer.level = std::clamp(1 + reach, 1, maxLevel(Kind::Unbreaking));
            }
            else
            {
                offer.special = pick;
                offer.level = std::clamp(1 + reach, 1, tridentMax(static_cast<TridentKind>(pick)));
            }
            return offer;
        }

        // Everything that could go on this item, so the pick is made
        // from what fits rather than rolled and thrown away.
        std::vector<Kind> fits;
        for (int i = 0; i < static_cast<int>(Kind::Count); ++i)
        {
            const Kind kind = static_cast<Kind>(i);
            if (appliesTo(kind, item) && !isTreasure(kind)) fits.push_back(kind);
        }
        if (fits.empty())
        {
            offer.cost = 0;
            return offer;
        }

        // Shuffled once for the whole table, then each row takes its
        // own place in the order. Picking a row's kind independently
        // let two rows land on the same one, and a table offering silk
        // touch twice has really only two offers on it.
        for (size_t i = fits.size(); i > 1; --i)
        {
            const size_t j = stir(roll, static_cast<uint32_t>(i) + 20u) % i;
            std::swap(fits[i - 1], fits[j]);
        }
        offer.kind = fits[static_cast<size_t>(row) % fits.size()];

        // A dearer row buys a higher level, scaled by how well the
        // material takes it: the same thirty levels put more on gold
        // than on diamond.
        const int ceiling = maxLevel(offer.kind);
        const int reach = (offer.cost * (1 + enchantability(item) / 8)) / 12;
        offer.level = std::clamp(1 + reach, 1, ceiling);
        return offer;
    }

    float digSpeedMultiplier(Set set)
    {
        const int level = levelOf(set, Kind::Efficiency);
        if (level <= 0) return 1.0f;
        // Minecraft adds level^2 + 1 to the tool's speed, so the first
        // level is a big jump and the fifth barely shows.
        return 1.0f + static_cast<float>(level * level + 1) * 0.25f;
    }

    bool wearsThisTime(Set set, uint32_t roll)
    {
        const int level = levelOf(set, Kind::Unbreaking);
        if (level <= 0) return true;
        // A point lands about one time in level+1.
        return (stir(roll, 3u) % static_cast<uint32_t>(level + 1)) == 0u;
    }

    int fortuneDrops(Set set, uint32_t roll)
    {
        const int level = levelOf(set, Kind::Fortune);
        if (level <= 0) return 1;
        // Minecraft rolls a bonus of 0..level and discards the worst,
        // which averages a little under doubling at level three.
        const int bonus = static_cast<int>(stir(roll, 4u) % static_cast<uint32_t>(level + 1));
        return 1 + std::max(0, bonus);
    }

    int sharpnessBonus(Set set)
    {
        const int level = levelOf(set, Kind::Sharpness);
        if (level <= 0) return 0;
        // Half a heart for the first, a quarter-heart more for each
        // after it, rounded as Minecraft rounds it.
        return 1 + (level - 1) / 2;
    }

    int powerBonus(Set set, int baseDamage)
    {
        const int level = levelOf(set, Kind::Power);
        if (level <= 0) return 0;
        return std::max(1, (baseDamage * (level + 1)) / 4);
    }

    int protectionReduced(Set set, int damage)
    {
        const int level = levelOf(set, Kind::Protection);
        if (level <= 0 || damage <= 0) return damage;

        // Four per level, out of twenty-five, and capped at four fifths
        // so no amount of it makes you proof.
        const int points = std::min(level * 4, 20);
        const int through = damage - (damage * points) / 25;
        return std::max(1, through);
    }
}

int Enchanting::protectionFactor(Set set, Hurt hurt)
{
    if (hurt == Hurt::Unprotected) return 0;
    int factor = levelOf(set, Kind::Protection);
    if (hurt == Hurt::Fire) factor += 2 * levelOf(set, Kind::FireProtection);
    if (hurt == Hurt::Blast) factor += 2 * levelOf(set, Kind::BlastProtection);
    if (hurt == Hurt::Projectile) factor += 2 * levelOf(set, Kind::ProjectileProtection);
    if (hurt == Hurt::Fall) factor += 3 * levelOf(set, Kind::FeatherFalling);
    return factor;
}

int Enchanting::reducedByFactor(int factor, int damage)
{
    if (factor <= 0 || damage <= 0) return damage;
    const int capped = std::min(factor, 20);
    const int through = damage - (damage * capped * 4 + 50) / 100;
    return std::max(1, through);
}

float Enchanting::burnScale(Set set)
{
    return std::max(0.0f, 1.0f - 0.15f * static_cast<float>(levelOf(set, Kind::FireProtection)));
}

bool Enchanting::breathHeld(Set helmet, uint32_t roll)
{
    const int level = levelOf(helmet, Kind::Respiration);
    if (level <= 0) return false;
    return (roll % static_cast<uint32_t>(level + 1)) != 0u;
}

float Enchanting::extraDamageAgainst(Set set, bool undead, bool arthropod)
{
    float extra = 0.0f;
    if (undead) extra += 2.5f * static_cast<float>(levelOf(set, Kind::Smite));
    if (arthropod) extra += 2.5f * static_cast<float>(levelOf(set, Kind::BaneOfArthropods));
    return extra;
}

Enchanting::Price Enchanting::priceOf(int row)
{
    Price price;
    price.levels = row + 1;
    price.lapis = row + 1;
    return price;
}

bool Enchanting::affordable(const Offer& offer, int row, int playerLevel, int lapis, bool creative)
{
    if (!offer.valid()) return false;
    if (creative) return true;
    return playerLevel >= offer.cost && lapis >= priceOf(row).lapis;
}

std::string Enchanting::phrase(uint32_t seed, int row)
{
    static const char* WORDS[] = {
        "arcane", "ember", "veil", "lumen", "quill", "gale", "frost", "bloom", "aether", "glimmer",
        "hollow", "drift", "spark", "tide", "stone", "ash", "dusk", "dawn", "mire", "wisp",
        "thorn", "vapor", "rift", "echo", "lore", "sigil", "bind", "weave", "kindle", "shroud",
        "grove", "ward", "orbit", "zephyr", "quartz", "nimbus", "fable", "cinder", "brine", "loam",
        "hearth", "spindle", "marrow", "lantern", "riddle", "hush", "gleam", "verdant", "keel", "omen",
        "wane", "wax", "knot", "rook", "fen", "crest", "dirge", "sable", "tallow", "umber",
    };
    constexpr uint32_t COUNT = sizeof(WORDS) / sizeof(WORDS[0]);
    uint32_t state = seed * 2654435761u + static_cast<uint32_t>(row + 1) * 40503u + 0x6A09E667u;
    auto next = [&state]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    for (int i = 0; i < 3; ++i) next();
    const int words = 3 + static_cast<int>(next() % 2u);
    std::string out;
    for (int i = 0; i < words; ++i)
    {
        if (i) out += ' ';
        out += WORDS[next() % COUNT];
    }
    return out;
}
