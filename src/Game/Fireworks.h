#pragma once
#include "Items.h"
#include "Crafting.h"
#include <string>
#include <vector>
#include <cstdint>

struct ItemStack;

namespace Fireworks
{
    enum class Shape : int { SmallBall = 0, LargeBall, Star, Creeper, Burst, Count };

    struct Explosion
    {
        Shape shape = Shape::SmallBall;
        std::vector<int> colours;
        std::vector<int> fades;
        bool trail = false;
        bool twinkle = false;
        bool operator==(const Explosion& other) const
        {
            return shape == other.shape && colours == other.colours && fades == other.fades &&
                   trail == other.trail && twinkle == other.twinkle;
        }
    };

    struct Rocket
    {
        int flight = 1;
        std::vector<int> explosions;
        bool operator==(const Rocket& other) const { return flight == other.flight && explosions == other.explosions; }
    };

    class Library
    {
    public:
        int explosion(const Explosion& design);
        int rocket(const Rocket& design);
        const Explosion* explosionAt(int id) const;
        const Rocket* rocketAt(int id) const;
        std::vector<Explosion>& explosions() { return m_explosions; }
        std::vector<Rocket>& rockets() { return m_rockets; }
        std::vector<int>& crossbowSlots() { return m_crossbowSlots; }
        int crossbowSlot(int rocketId);
        int crossbowRocket(int slot) const;
        void clear() { m_explosions.clear(); m_rockets.clear(); m_crossbowSlots.clear(); m_nextSlot = 0; }

    private:
        std::vector<Explosion> m_explosions;
        std::vector<Rocket> m_rockets;
        std::vector<int> m_crossbowSlots;
        int m_nextSlot = 0;
    };

    Library& library();

    constexpr int MAX_DYES = 8;
    constexpr int MAX_STARS = 7;
    constexpr int MAX_FLIGHT = 3;

    CraftOutput matchStar(const ItemStack* grid, int size);
    CraftOutput matchFade(const ItemStack* grid, int size);
    CraftOutput matchRocket(const ItemStack* grid, int size);
    CraftOutput match(const ItemStack* grid, int size);

    const char* shapeName(Shape shape);
    unsigned int fireworkRgb(int dye);
    unsigned int starColour(const ItemStack& star);
    std::vector<std::string> describeStar(const Explosion& explosion);
    std::vector<std::string> describeRocket(const ItemStack& rocket);
    float lifetimeSeconds(int flight, uint32_t roll);
}
