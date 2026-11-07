#include "Fireworks.h"
#include "Banners.h"
#include "Player/Inventory.h"
#include <algorithm>

namespace
{
    const unsigned int FIREWORK_RGB[16] = {
        0xF0F0F0u, 0xEB8844u, 0xC354CDu, 0x6689D3u, 0xDECF2Au, 0x41CD34u, 0xD88198u, 0x434343u,
        0xABABABu, 0x287697u, 0x7B2FBEu, 0x253192u, 0x51301Au, 0x3B511Au, 0xB3312Cu, 0x1E1B1Bu,
    };

    int dyeIndex(StackId id)
    {
        if (id < Items::DyeWhite || id > Items::DyeBlack) return -1;
        return static_cast<int>(id - Items::DyeWhite);
    }

    template <typename Visit>
    void eachCell(const ItemStack* grid, int size, Visit visit)
    {
        for (int i = 0; i < 9; ++i)
        {
            if (i % 3 >= size || i / 3 >= size) continue;
            if (!grid[i].empty()) visit(grid[i]);
        }
    }
}

namespace Fireworks
{
    int Library::explosion(const Explosion& design)
    {
        for (size_t i = 0; i < m_explosions.size(); ++i)
            if (m_explosions[i] == design) return static_cast<int>(i) + 1;
        if (m_explosions.size() >= 60000u) return 0;
        m_explosions.push_back(design);
        return static_cast<int>(m_explosions.size());
    }

    int Library::rocket(const Rocket& design)
    {
        for (size_t i = 0; i < m_rockets.size(); ++i)
            if (m_rockets[i] == design) return static_cast<int>(i) + 1;
        if (m_rockets.size() >= 60000u) return 0;
        m_rockets.push_back(design);
        return static_cast<int>(m_rockets.size());
    }

    const Explosion* Library::explosionAt(int id) const
    {
        if (id <= 0 || id > static_cast<int>(m_explosions.size())) return nullptr;
        return &m_explosions[static_cast<size_t>(id - 1)];
    }

    const Rocket* Library::rocketAt(int id) const
    {
        if (id <= 0 || id > static_cast<int>(m_rockets.size())) return nullptr;
        return &m_rockets[static_cast<size_t>(id - 1)];
    }

    int Library::crossbowSlot(int rocketId)
    {
        for (size_t i = 0; i < m_crossbowSlots.size(); ++i)
            if (m_crossbowSlots[i] == rocketId) return static_cast<int>(i) + 1;
        if (m_crossbowSlots.size() < 31u)
        {
            m_crossbowSlots.push_back(rocketId);
            return static_cast<int>(m_crossbowSlots.size());
        }
        m_nextSlot = (m_nextSlot + 1) % 31;
        m_crossbowSlots[static_cast<size_t>(m_nextSlot)] = rocketId;
        return m_nextSlot + 1;
    }

    int Library::crossbowRocket(int slot) const
    {
        if (slot <= 0 || slot > static_cast<int>(m_crossbowSlots.size())) return 0;
        return m_crossbowSlots[static_cast<size_t>(slot - 1)];
    }

    Library& library()
    {
        static Library shared;
        return shared;
    }

    CraftOutput matchStar(const ItemStack* grid, int size)
    {
        CraftOutput out;
        int gunpowder = 0, shapes = 0, diamonds = 0, glowstone = 0, others = 0;
        Explosion design;
        eachCell(grid, size, [&](const ItemStack& cell) {
            const int dye = dyeIndex(cell.id);
            if (dye >= 0) design.colours.push_back(dye);
            else if (cell.id == Items::Gunpowder) ++gunpowder;
            else if (cell.id == Items::FireCharge) { ++shapes; design.shape = Shape::LargeBall; }
            else if (cell.id == Items::GoldNugget) { ++shapes; design.shape = Shape::Star; }
            else if (cell.id == Items::Feather) { ++shapes; design.shape = Shape::Burst; }
            else if (cell.id == Items::Diamond) { ++diamonds; design.trail = true; }
            else if (cell.id == Items::GlowstoneDust) { ++glowstone; design.twinkle = true; }
            else ++others;
        });
        if (gunpowder != 1 || shapes > 1 || diamonds > 1 || glowstone > 1 || others > 0) return out;
        if (design.colours.empty() || static_cast<int>(design.colours.size()) > MAX_DYES) return out;
        const int id = library().explosion(design);
        if (id <= 0) return out;
        out.id = Items::FireworkStar;
        out.count = 1;
        out.damage = id;
        return out;
    }

    CraftOutput matchFade(const ItemStack* grid, int size)
    {
        CraftOutput out;
        const ItemStack* star = nullptr;
        int stars = 0, others = 0;
        std::vector<int> fades;
        eachCell(grid, size, [&](const ItemStack& cell) {
            const int dye = dyeIndex(cell.id);
            if (dye >= 0) fades.push_back(dye);
            else if (cell.id == Items::FireworkStar) { ++stars; star = &cell; }
            else ++others;
        });
        if (stars != 1 || others > 0 || fades.empty() || static_cast<int>(fades.size()) > MAX_DYES) return out;
        const Explosion* before = library().explosionAt(star->damage);
        if (!before) return out;
        Explosion design = *before;
        design.fades = fades;
        const int id = library().explosion(design);
        if (id <= 0) return out;
        out.id = Items::FireworkStar;
        out.count = 1;
        out.damage = id;
        return out;
    }

    CraftOutput matchRocket(const ItemStack* grid, int size)
    {
        CraftOutput out;
        int paper = 0, gunpowder = 0, others = 0;
        Rocket design;
        eachCell(grid, size, [&](const ItemStack& cell) {
            if (cell.id == Items::Paper) ++paper;
            else if (cell.id == Items::Gunpowder) ++gunpowder;
            else if (cell.id == Items::FireworkStar && library().explosionAt(cell.damage)) design.explosions.push_back(cell.damage);
            else ++others;
        });
        if (paper != 1 || gunpowder < 1 || gunpowder > MAX_FLIGHT || others > 0) return out;
        if (static_cast<int>(design.explosions.size()) > MAX_STARS) return out;
        design.flight = gunpowder;
        const int id = library().rocket(design);
        if (id <= 0) return out;
        out.id = Items::FireworkRocket;
        out.count = 3;
        out.damage = id;
        return out;
    }

    CraftOutput match(const ItemStack* grid, int size)
    {
        bool fireworkPart = false;
        eachCell(grid, size, [&](const ItemStack& cell) {
            if (cell.id == Items::Gunpowder || cell.id == Items::FireworkStar) fireworkPart = true;
        });
        if (!fireworkPart) return CraftOutput{};
        const CraftOutput rocket = matchRocket(grid, size);
        if (rocket.valid()) return rocket;
        const CraftOutput star = matchStar(grid, size);
        if (star.valid()) return star;
        return matchFade(grid, size);
    }

    const char* shapeName(Shape shape)
    {
        switch (shape)
        {
            case Shape::SmallBall: return "Small Ball";
            case Shape::LargeBall: return "Large Ball";
            case Shape::Star: return "Star-shaped";
            case Shape::Creeper: return "Creeper-shaped";
            case Shape::Burst: return "Burst";
            case Shape::Count: break;
        }
        return "Unknown Shape";
    }

    unsigned int fireworkRgb(int dye)
    {
        return FIREWORK_RGB[std::clamp(dye, 0, 15)];
    }

    unsigned int starColour(const ItemStack& star)
    {
        const Explosion* design = library().explosionAt(star.damage);
        if (!design || design->colours.empty()) return 0x8A8A8Au;
        unsigned int r = 0, g = 0, b = 0;
        for (int dye : design->colours)
        {
            const unsigned int c = fireworkRgb(dye);
            r += (c >> 16) & 255u;
            g += (c >> 8) & 255u;
            b += c & 255u;
        }
        const unsigned int n = static_cast<unsigned int>(design->colours.size());
        return ((r / n) << 16) | ((g / n) << 8) | (b / n);
    }

    std::vector<std::string> describeStar(const Explosion& explosion)
    {
        std::vector<std::string> lines;
        lines.push_back(shapeName(explosion.shape));
        std::string colours;
        for (size_t i = 0; i < explosion.colours.size(); ++i)
            colours += (i ? ", " : "") + std::string(Banners::colourName(explosion.colours[i]));
        if (!colours.empty()) lines.push_back(colours);
        std::string fades;
        for (size_t i = 0; i < explosion.fades.size(); ++i)
            fades += (i ? ", " : "") + std::string(Banners::colourName(explosion.fades[i]));
        if (!fades.empty()) lines.push_back("Fade to " + fades);
        if (explosion.trail) lines.push_back("Trail");
        if (explosion.twinkle) lines.push_back("Twinkle");
        return lines;
    }

    std::vector<std::string> describeRocket(const ItemStack& rocket)
    {
        std::vector<std::string> lines;
        const Rocket* design = library().rocketAt(rocket.damage);
        lines.push_back("Flight Duration: " + std::to_string(design ? design->flight : 1));
        if (!design) return lines;
        for (int id : design->explosions)
            if (const Explosion* explosion = library().explosionAt(id))
                for (const std::string& line : describeStar(*explosion)) lines.push_back("  " + line);
        return lines;
    }

    float lifetimeSeconds(int flight, uint32_t roll)
    {
        const int ticks = 10 * (std::clamp(flight, 1, MAX_FLIGHT) + 1) + static_cast<int>(roll % 6u) + static_cast<int>((roll >> 8) % 7u);
        return static_cast<float>(ticks) / 20.0f;
    }
}
