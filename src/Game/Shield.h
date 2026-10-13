#pragma once
#include <glm/glm.hpp>

// The shield: what it stops, and what stopping it costs.
//
// Arithmetic over two directions and a number, so --selftest can take
// every blow from every angle without a world or a player in one.
namespace Shield
{
    // A shield covers the front and nothing else. Minecraft's arc is
    // the half-plane you are facing; this is a little tighter, so
    // turning to meet a blow is worth doing rather than automatic.
    constexpr float COVER_COSINE = 0.20f;   // about 78 degrees either side

    // What it costs the shield to stop a blow, in points of wear. A
    // shield that stopped everything for nothing would end the game's
    // combat rather than change it.
    constexpr int WEAR_PER_BLOCK = 1;

    // How many blows one takes before it breaks, which is Minecraft's
    // number. Iron mends it, since iron is what the boss is made of.
    constexpr int MAX_DURABILITY = 336;

    // While it is up you move at this share of your usual speed.
    constexpr float RAISED_SPEED = 0.45f;

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
