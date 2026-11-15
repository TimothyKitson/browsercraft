#pragma once
#include <glm/glm.hpp>

// The shield: what it stops, and what stopping it costs.
//
// Arithmetic over two directions and a number, so --selftest can take
// every blow from every angle without a world or a player in one.
namespace Shield
{
    constexpr float COVER_COSINE = 0.0f;

    constexpr int WEAR_PER_BLOCK = 0;
    inline int wearFor(int damage) { return damage >= 3 ? 1 + damage : 0; }

    // How many blows one takes before it breaks, which is Minecraft's
    // number. Iron mends it, since iron is what the boss is made of.
    constexpr int MAX_DURABILITY = 336;

    constexpr float RAISED_SPEED = 0.2f;

    // Whether a blow coming from `attacker` towards `defender` lands on
    // the shield of someone looking along `facing`. Height is ignored:
    // raising a shield at something above you still covers you.
    bool covers(const glm::vec3& facing, const glm::vec3& defender,
                const glm::vec3& attacker);

    // What gets through. A raised shield stops a blow outright, which
    // is Minecraft's rule -- the cost is the wear and being slowed, not
    // a share of the damage.
    int damageThrough(int incoming, bool blocked);
}
