#pragma once
#include "Items.h"
#include "World/Block.h"
#include <cstdint>
#include <vector>

// Enchantments: what can be put on a thing, what it costs, and what it
// then does.
//
// An enchanted item has to carry its enchantments everywhere a bare id
// goes -- inventory slots, saved levels, dropped stacks -- and a slot
// has room for one number beside the id, not a list. So a whole set of
// enchantments is packed into one 32-bit word, three bits a kind, which
// is enough for every level Minecraft allows and leaves the thing that
// stores it no harder to save than the wear already beside it.
//
// All of it is arithmetic on ids and numbers, so --selftest can enchant
// the whole item table and read back what it did without a world.
namespace Enchanting
{
    // Minecraft's core set. Keep the order: a kind's place in it is
    // where its bits live, so inserting one in the middle would change
    // what every saved item says it has.
    enum class Kind : int
    {
        Efficiency = 0,   // breaks blocks faster
        Unbreaking,       // wears out slower
        Fortune,          // more out of a seam
        SilkTouch,        // the block itself rather than its drop
        Sharpness,        // hits harder
        Protection,       // takes less
        Power,            // arrows hit harder
        Count
    };

    // Three bits each, so no kind may go past seven.
    constexpr int BITS_PER_KIND = 3;
    constexpr int MAX_STORABLE_LEVEL = 7;

    using Set = uint32_t;

    static_assert(static_cast<int>(Kind::Count) * BITS_PER_KIND <= 32,
                  "the enchantment set no longer fits in one word");

    // The highest level Minecraft allows of each. Silk touch and
    // fortune are the odd pair: they do opposite things to the same
    // drop, so nothing may carry both.
    int maxLevel(Kind kind);
    const char* name(Kind kind);

    // Whether this kind means anything on this item. Efficiency on a
    // sword or sharpness on a helmet is not a weaker enchantment, it is
    // a meaningless one, and the table should never offer it.
    bool appliesTo(Kind kind, StackId item);

    // Two kinds that cannot sit on the same item.
    bool conflicts(Kind a, Kind b);

    // Reading and writing the packed word.
    int levelOf(Set set, Kind kind);
    Set withLevel(Set set, Kind kind, int level);
    bool has(Set set, Kind kind);
    bool any(Set set);

    // Every kind on the item, for showing it and for saving it.
    std::vector<std::pair<Kind, int>> listOf(Set set);

    // What a grindstone pays for taking this lot off, in hundredths of
    // an experience point -- the same unit the furnace pays in. Three
    // points a level, so stripping a good sword is worth doing and
    // stripping a bad one is not worth the walk.
    int grindHundredths(Set set);

    // --- the table -----------------------------------------------------

    // Shelves only count up to fifteen, and only when they stand one
    // cell clear of the table with nothing in the gap -- which is why
    // the room around a table is built the way it is.
    constexpr int MAX_SHELVES = 15;

    // What the three rows cost, in levels, for this many shelves and
    // this roll. Minecraft draws one number and derives all three from
    // it, so they move together: a table with no shelves never offers
    // anything worth having in any row.
    struct Offer
    {
        int cost = 0;        // levels it takes to buy
        Kind kind = Kind::Efficiency;
        int level = 0;       // of that enchantment
        bool valid() const { return cost > 0 && level > 0; }
    };

    // What counting shelves needs to know about the room. Named for
    // what it is rather than "Blocks", because inside anything deriving
    // from a type called Blocks the bare name Blocks:: finds the base
    // class instead of the block ids.
    struct Surroundings
    {
        virtual ~Surroundings() = default;
        virtual BlockId at(int x, int y, int z) const = 0;
    };

    // Shelves two cells out from the table, level with it or one above,
    // and only where the cell between them and the table is clear. The
    // gap is the rule people trip over: a shelf pushed right up against
    // the table counts for nothing.
    int shelvesAround(const Surroundings& room, int x, int y, int z);

    // The next roll after this one, for when an offer has been bought
    // and the three rows should not stay as they were.
    uint32_t nextRoll(uint32_t roll);

    // `row` is 0, 1 or 2 from the top.
    int costForRow(int row, int shelves, uint32_t roll);

    // The whole offer, including what you actually get. Returns an
    // invalid offer when nothing can go on this item.
    Offer offerFor(int row, int shelves, StackId item, uint32_t roll);

    // How enchantable the material is: gold takes the best enchantments
    // by far and wood is close behind, while diamond is middling, which
    // is the opposite of what people expect.
    int enchantability(StackId item);

    // --- what they do ----------------------------------------------------

    // Efficiency makes a tool dig faster. Minecraft adds level squared
    // plus one to the tool's speed, which is why the first level is a
    // big jump and the fifth barely shows.
    float digSpeedMultiplier(Set set);

    // Unbreaking skips wear some of the time rather than making the
    // tool tougher: at level n a point of damage lands about one time
    // in n+1. Returns whether this particular use should cost one.
    bool wearsThisTime(Set set, uint32_t roll);

    // Fortune multiplies what a seam gives up.
    int fortuneDrops(Set set, uint32_t roll);

    // Extra hearts of damage from sharpness, and from power on a bow.
    int sharpnessBonus(Set set);
    int powerBonus(Set set, int baseDamage);

    // Protection takes a slice off what gets through the armour, capped
    // the way Minecraft caps it so no amount of it makes you proof.
    int protectionReduced(Set set, int damage);
}
