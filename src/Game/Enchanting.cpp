#include "Enchanting.h"
#include "Tools.h"
#include "Armour.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

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

    int maxLevel(Kind kind)
    {
        switch (kind)
        {
        case Kind::Efficiency: return 5;
        case Kind::Unbreaking: return 3;
        case Kind::Fortune:    return 3;
        case Kind::SilkTouch:  return 1;
        case Kind::Sharpness:  return 5;
        case Kind::Protection: return 4;
        case Kind::Power:      return 5;
        default: break;
        }
        return 0;
    }

    const char* name(Kind kind)
    {
        switch (kind)
        {
        case Kind::Efficiency: return "Efficiency";
        case Kind::Unbreaking: return "Unbreaking";
        case Kind::Fortune:    return "Fortune";
        case Kind::SilkTouch:  return "Silk Touch";
        case Kind::Sharpness:  return "Sharpness";
        case Kind::Protection: return "Protection";
        case Kind::Power:      return "Power";
        default: break;
        }
        return "";
    }

    bool appliesTo(Kind kind, StackId item)
    {
        const Tools::Kind tool = Tools::kindOf(item);
        const bool digs = tool == Tools::Kind::Pickaxe || tool == Tools::Kind::Axe ||
                          tool == Tools::Kind::Shovel;

        switch (kind)
        {
        // Unbreaking goes on anything that wears out, which is every
        // tool, every piece of armour and the bow.
        case Kind::Unbreaking:
            return Tools::isTool(item) || Armour::slotFor(item) >= 0 || item == Items::Bow;

        case Kind::Efficiency: return digs;

        // Fortune and silk touch are for what comes out of the ground,
        // so they go on the three digging tools and nothing else.
        case Kind::Fortune:
        case Kind::SilkTouch:  return digs;

        case Kind::Sharpness:  return tool == Tools::Kind::Sword || tool == Tools::Kind::Axe;
        case Kind::Protection: return Armour::slotFor(item) >= 0;
        case Kind::Power:      return item == Items::Bow;
        default: break;
        }
        return false;
    }

    bool conflicts(Kind a, Kind b)
    {
        if (a == b) return false;
        // One keeps the block whole, the other multiplies what it
        // breaks into. An item carrying both has no sensible answer.
        return (a == Kind::Fortune && b == Kind::SilkTouch) ||
               (a == Kind::SilkTouch && b == Kind::Fortune);
    }

    int levelOf(Set set, Kind kind)
    {
        if (kind >= Kind::Count) return 0;
        return static_cast<int>((set >> shift(kind)) & 0x7u);
    }

    Set withLevel(Set set, Kind kind, int level)
    {
        if (kind >= Kind::Count) return set;
        const int capped = std::clamp(level, 0, std::min(maxLevel(kind), MAX_STORABLE_LEVEL));
        const uint32_t mask = 0x7u << shift(kind);
        return (set & ~mask) | (static_cast<uint32_t>(capped) << shift(kind));
    }

    bool has(Set set, Kind kind) { return levelOf(set, kind) > 0; }
    bool any(Set set) { return set != 0u; }

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
        if (Armour::isArmour(item))
        {
            switch (Armour::tierOf(item))
            {
            case Armour::Tier::Leather: return 15;
            case Armour::Tier::Gold:    return 25;
            case Armour::Tier::Iron:    return 9;
            case Armour::Tier::Diamond: return 10;
            default: break;
            }
            return 0;
        }

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

        if (item == Items::Bow) return 1;
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

        // Everything that could go on this item, so the pick is made
        // from what fits rather than rolled and thrown away.
        std::vector<Kind> fits;
        for (int i = 0; i < static_cast<int>(Kind::Count); ++i)
        {
            const Kind kind = static_cast<Kind>(i);
            if (appliesTo(kind, item)) fits.push_back(kind);
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
