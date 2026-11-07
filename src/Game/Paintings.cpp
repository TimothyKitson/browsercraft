#include "Paintings.h"
#include <algorithm>
#include <cmath>

namespace Paintings
{
    const std::vector<Variant>& variants()
    {
        static const std::vector<Variant> LIST = {
            { "kebab", 1, 1, true, "Kebab med tre pepperoni", "Kristoffer Zetterstrand" },
            { "aztec", 1, 1, true, "de_aztec", "Kristoffer Zetterstrand" },
            { "alban", 1, 1, true, "Albanian", "Kristoffer Zetterstrand" },
            { "aztec2", 1, 1, true, "de_aztec", "Kristoffer Zetterstrand" },
            { "bomb", 1, 1, true, "Target Successfully Bombed", "Kristoffer Zetterstrand" },
            { "plant", 1, 1, true, "Paradistrad", "Kristoffer Zetterstrand" },
            { "wasteland", 1, 1, true, "Wasteland", "Kristoffer Zetterstrand" },
            { "pool", 2, 1, true, "The Pool", "Kristoffer Zetterstrand" },
            { "courbet", 2, 1, true, "Bonjour Monsieur Courbet", "Kristoffer Zetterstrand" },
            { "sea", 2, 1, true, "Seaside", "Kristoffer Zetterstrand" },
            { "sunset", 2, 1, true, "sunset_dense", "Kristoffer Zetterstrand" },
            { "creebet", 2, 1, true, "Creebet", "Kristoffer Zetterstrand" },
            { "wanderer", 1, 2, true, "Wanderer", "Kristoffer Zetterstrand" },
            { "graham", 1, 2, true, "Graham", "Kristoffer Zetterstrand" },
            { "match", 2, 2, true, "Match", "Kristoffer Zetterstrand" },
            { "bust", 2, 2, true, "Bust", "Kristoffer Zetterstrand" },
            { "stage", 2, 2, true, "The Stage Is Set", "Kristoffer Zetterstrand" },
            { "void", 2, 2, true, "The void", "Kristoffer Zetterstrand" },
            { "skull_and_roses", 2, 2, true, "Skull and Roses", "Kristoffer Zetterstrand" },
            { "wither", 2, 2, true, "Wither", "Mojang" },
            { "fighters", 4, 2, true, "Fighters", "Kristoffer Zetterstrand" },
            { "pointer", 4, 4, true, "Pointer", "Kristoffer Zetterstrand" },
            { "pigscene", 4, 4, true, "Pigscene", "Kristoffer Zetterstrand" },
            { "burning_skull", 4, 4, true, "Skull On Fire", "Kristoffer Zetterstrand" },
            { "skeleton", 4, 3, true, "Mortal Coil", "Kristoffer Zetterstrand" },
            { "earth", 2, 2, false, "Earth", "Mojang" },
            { "wind", 2, 2, false, "Wind", "Mojang" },
            { "water", 2, 2, false, "Water", "Mojang" },
            { "fire", 2, 2, false, "Fire", "Mojang" },
            { "donkey_kong", 4, 3, true, "Kong", "Kristoffer Zetterstrand" },
            { "baroque", 2, 2, true, "Baroque", "Sarah Boeving" },
            { "humble", 2, 2, true, "Humble", "Sarah Boeving" },
            { "meditative", 1, 1, true, "Meditative", "Sarah Boeving" },
            { "prairie_ride", 1, 2, true, "Prairie Ride", "Sarah Boeving" },
            { "unpacked", 4, 4, true, "Unpacked", "Sarah Boeving" },
            { "backyard", 3, 4, true, "Backyard", "Kristoffer Zetterstrand" },
            { "bouquet", 3, 3, true, "Bouquet", "Kristoffer Zetterstrand" },
            { "cavebird", 3, 3, true, "Cavebird", "Kristoffer Zetterstrand" },
            { "changing", 4, 2, true, "Changing", "Kristoffer Zetterstrand" },
            { "cotan", 3, 3, true, "Cotan", "Kristoffer Zetterstrand" },
            { "endboss", 3, 3, true, "Endboss", "Kristoffer Zetterstrand" },
            { "fern", 3, 3, true, "Fern", "Kristoffer Zetterstrand" },
            { "finding", 4, 2, true, "Finding", "Kristoffer Zetterstrand" },
            { "lowmist", 4, 2, true, "Lowmist", "Kristoffer Zetterstrand" },
            { "orb", 4, 4, true, "Orb", "Kristoffer Zetterstrand" },
            { "owlemons", 3, 3, true, "Owlemons", "Kristoffer Zetterstrand" },
            { "passage", 4, 2, true, "Passage", "Kristoffer Zetterstrand" },
            { "pond", 3, 4, true, "Pond", "Kristoffer Zetterstrand" },
            { "sunflowers", 3, 3, true, "Sunflowers", "Kristoffer Zetterstrand" },
            { "tides", 3, 3, true, "Tides", "Kristoffer Zetterstrand" },
        };
        return LIST;
    }

    std::string texturePath(int variant)
    {
        return "assets/textures/painting/" + std::string(variants()[static_cast<size_t>(variant)].name) + ".png";
    }

    int count()
    {
        return static_cast<int>(variants().size());
    }

    int find(const std::string& name)
    {
        for (int i = 0; i < count(); ++i)
            if (name == variants()[static_cast<size_t>(i)].name) return i;
        return -1;
    }

    glm::ivec3 out(int facing)
    {
        static const glm::ivec3 OUT[4] = { { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 }, { 1, 0, 0 } };
        return OUT[facing & 3];
    }

    glm::ivec3 right(int facing)
    {
        static const glm::ivec3 RIGHT[4] = { { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 } };
        return RIGHT[facing & 3];
    }

    int facingFor(const glm::ivec3& normal)
    {
        if (normal.y != 0) return -1;
        return (normal.z > 0) ? 0 : (normal.x < 0) ? 1 : (normal.z < 0) ? 2 : 3;
    }

    std::vector<glm::ivec3> cells(const Placed& painting)
    {
        const Variant& v = variants()[static_cast<size_t>(std::clamp(painting.variant, 0, count() - 1))];
        std::vector<glm::ivec3> out;
        const glm::ivec3 side = right(painting.facing);
        for (int k = -((v.width - 1) / 2); k <= v.width / 2; ++k)
            for (int j = -((v.height - 1) / 2); j <= v.height / 2; ++j)
                out.push_back(painting.at + side * k + glm::ivec3(0, j, 0));
        return out;
    }

    bool fits(const Placed& painting, const Wall& wall)
    {
        const glm::ivec3 back = -out(painting.facing);
        for (const glm::ivec3& cell : cells(painting))
        {
            const glm::ivec3 behind = cell + back;
            if (!wall.solid(behind.x, behind.y, behind.z)) return false;
            if (!wall.open(cell.x, cell.y, cell.z)) return false;
        }
        return true;
    }

    bool overlaps(const Placed& a, const Placed& b)
    {
        if (a.facing != b.facing) return false;
        for (const glm::ivec3& one : cells(a))
            for (const glm::ivec3& two : cells(b))
                if (one == two) return true;
        return false;
    }

    int choose(const glm::ivec3& at, int facing, const Wall& wall, uint32_t roll, const std::vector<Placed>& others)
    {
        std::vector<int> fitting;
        int biggest = 0;
        for (int i = 0; i < count(); ++i)
        {
            const Variant& v = variants()[static_cast<size_t>(i)];
            if (!v.placeable) continue;
            const Placed trial{ at, facing, i };
            if (!fits(trial, wall)) continue;
            bool clash = false;
            for (const Placed& other : others)
                if (overlaps(trial, other)) clash = true;
            if (clash) continue;
            const int area = v.width * v.height;
            if (area > biggest)
            {
                biggest = area;
                fitting.clear();
            }
            if (area == biggest) fitting.push_back(i);
        }
        if (fitting.empty()) return -1;
        return fitting[roll % fitting.size()];
    }

    Box bounds(const Placed& painting)
    {
        const Variant& v = variants()[static_cast<size_t>(std::clamp(painting.variant, 0, count() - 1))];
        const glm::vec3 o(out(painting.facing));
        const glm::vec3 r(right(painting.facing));
        const glm::vec3 centre = glm::vec3(painting.at) + glm::vec3(0.5f) - o * 0.46875f +
                                 r * (v.width % 2 == 0 ? 0.5f : 0.0f) +
                                 glm::vec3(0.0f, v.height % 2 == 0 ? 0.5f : 0.0f, 0.0f);
        const glm::vec3 half = glm::abs(r) * (static_cast<float>(v.width) * 0.5f) +
                               glm::vec3(0.0f, static_cast<float>(v.height) * 0.5f, 0.0f) +
                               glm::abs(o) * 0.03125f;
        return Box{ centre - half, centre + half };
    }

    bool rayHits(const Box& box, const glm::vec3& origin, const glm::vec3& direction, float reach, float& distance)
    {
        float enter = 0.0f, leave = reach;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::fabs(direction[axis]) < 1e-6f)
            {
                if (origin[axis] < box.min[axis] || origin[axis] > box.max[axis]) return false;
                continue;
            }
            float a = (box.min[axis] - origin[axis]) / direction[axis];
            float b = (box.max[axis] - origin[axis]) / direction[axis];
            if (a > b) std::swap(a, b);
            enter = std::max(enter, a);
            leave = std::min(leave, b);
            if (enter > leave) return false;
        }
        distance = enter;
        return true;
    }
}

void PaintingStore::putIn(int dimension, const Paintings::Placed& painting)
{
    m_paintings[keyOf(dimension, painting)] = painting.variant;
}

bool PaintingStore::remove(const Paintings::Placed& painting)
{
    return m_paintings.erase(keyOf(m_dimension, painting)) > 0;
}

std::vector<Paintings::Placed> PaintingStore::here() const
{
    std::vector<Paintings::Placed> out;
    for (const auto& [key, variant] : m_paintings)
        if (key[0] == m_dimension)
            out.push_back(Paintings::Placed{ glm::ivec3(key[1], key[2], key[3]), key[4], variant });
    return out;
}

std::vector<Paintings::Placed> PaintingStore::near(const glm::vec3& around, float reach) const
{
    std::vector<Paintings::Placed> out;
    for (const Paintings::Placed& painting : here())
    {
        const glm::vec3 apart = glm::vec3(painting.at) + glm::vec3(0.5f) - around;
        if (glm::dot(apart, apart) <= reach * reach) out.push_back(painting);
    }
    return out;
}

std::vector<PaintingStore::Entry> PaintingStore::all() const
{
    std::vector<Entry> out;
    for (const auto& [key, variant] : m_paintings)
        out.push_back(Entry{ key[0], Paintings::Placed{ glm::ivec3(key[1], key[2], key[3]), key[4], variant } });
    return out;
}
