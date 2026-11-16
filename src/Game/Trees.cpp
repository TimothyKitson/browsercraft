#include "Trees.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <functional>
#include <set>
#include <unordered_map>

namespace
{
    using Trees::Shape;

    struct Rng
    {
        uint64_t state;
        explicit Rng(uint32_t seed)
            : state((static_cast<uint64_t>(seed) + 1ull) * 0x9E3779B97F4A7C15ull ^ 0xD1B54A32D192ED03ull)
        {
            if (state == 0) state = 0x2545F4914F6CDD1Dull;
            for (int i = 0; i < 4; ++i) next();
        }
        uint32_t next()
        {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            return static_cast<uint32_t>((state * 0x2545F4914F6CDD1Dull) >> 32);
        }
        int nextInt(int bound) { return bound <= 0 ? 0 : static_cast<int>(next() % static_cast<uint32_t>(bound)); }
        int between(int lo, int hi) { return lo + nextInt(hi - lo + 1); }
        float nextFloat() { return static_cast<float>(next() >> 8) / 16777216.0f; }
        bool nextBoolean() { return (next() & 1u) != 0u; }
    };

    const glm::ivec3 DIRS[4] = { { 0, 0, -1 }, { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 } };
    constexpr float PI = 3.14159265358979f;

    int axisOf(int dir) { return (dir & 1) ? 1 : 2; }

    struct Node
    {
        glm::ivec3 centre;
        int radius;
        bool giant;
    };

    enum Cell : uint8_t { Empty = 0, Log = 1, Leaf = 2, Extra = 3 };

    struct Builder
    {
        Trees::Plan& plan;
        Rng& rng;
        BlockId log = Blocks::Log;
        BlockId leaves = Blocks::Leaves;
        BlockId flowering = Blocks::Air;
        std::unordered_map<uint64_t, std::pair<Cell, size_t>> cells;

        Builder(Trees::Plan& p, Rng& r) : plan(p), rng(r) {}

        static uint64_t key(const glm::ivec3& p)
        {
            return (static_cast<uint64_t>(static_cast<uint32_t>(p.x) & 0x3FFFFFu) << 42) |
                   (static_cast<uint64_t>(static_cast<uint32_t>(p.y + 4096) & 0xFFFFFu) << 22) |
                   static_cast<uint64_t>(static_cast<uint32_t>(p.z) & 0x3FFFFFu);
        }
        Cell kind(const glm::ivec3& p) const
        {
            const auto found = cells.find(key(p));
            return found == cells.end() ? Empty : found->second.first;
        }
        bool air(const glm::ivec3& p) const { return kind(p) == Empty; }
        bool setLog(const glm::ivec3& p, int axis = 0)
        {
            const auto found = cells.find(key(p));
            if (found != cells.end())
            {
                if (found->second.first != Leaf) return false;
                plan.leaves[found->second.second].id = Blocks::Air;
            }
            cells[key(p)] = { Log, plan.logs.size() };
            plan.logs.push_back({ p, withAxis(log, axis) });
            return true;
        }
        bool setLeaf(const glm::ivec3& p)
        {
            if (kind(p) != Empty) return false;
            BlockId id = leaves;
            if (flowering != Blocks::Air && rng.nextInt(4) == 0) id = flowering;
            cells[key(p)] = { Leaf, plan.leaves.size() };
            plan.leaves.push_back({ p, id });
            return true;
        }
        bool placedLeaf(const glm::ivec3& p) const { return kind(p) == Leaf; }
        void setExtra(const glm::ivec3& p, BlockId id)
        {
            cells[key(p)] = { Extra, plan.extras.size() };
            plan.extras.push_back({ p, id });
        }
        void vine(const glm::ivec3& p, int bit)
        {
            const auto found = cells.find(key(p));
            if (found != cells.end())
            {
                if (found->second.first != Extra) return;
                Trees::Placed& there = plan.extras[found->second.second];
                if (isVine(there.id)) there.id = vineWith(vineMask(there.id) | (1 << bit));
                return;
            }
            setExtra(p, vineWith(1 << bit));
        }
        void dirt(const glm::ivec3& p) { plan.dirt.push_back(p); }
    };

    using Invalid = std::function<bool(int j, int y, int k, int radius, bool giant)>;

    int norm(int v, bool giant) { return giant ? std::min(std::abs(v), std::abs(v - 1)) : std::abs(v); }

    void square(Builder& b, const glm::ivec3& c, int radius, int y, bool giant, const Invalid& invalid)
    {
        const int extra = giant ? 1 : 0;
        for (int j = -radius; j <= radius + extra; ++j)
            for (int k = -radius; k <= radius + extra; ++k)
                if (!invalid(j, y, k, radius, giant)) b.setLeaf(c + glm::ivec3(j, y, k));
    }

    enum class Trunk { Straight, Forking, DarkOak, Fancy, Giant, MegaJungle, Cherry, Upward, Bending };
    enum class Foliage { Blob, Fancy, Acacia, DarkOak, Spruce, Pine, MegaPine, Jungle, Cherry, Spread, Bush };

    struct Spec
    {
        Trunk trunk;
        int base, a, b;
        Foliage foliage;
        int radiusMin, radiusMax;
        int offsetMin, offsetMax;
        int heightMin, heightMax;
        int extraMin, extraMax;
        int attempts;
        int wood;
    };

    Spec specOf(Shape shape)
    {
        switch (shape)
        {
            case Shape::Oak: return { Trunk::Straight, 4, 2, 0, Foliage::Blob, 2, 2, 0, 0, 3, 3, 0, 0, 0, 0 };
            case Shape::FancyOak: return { Trunk::Fancy, 3, 11, 0, Foliage::Fancy, 2, 2, 4, 4, 4, 4, 0, 0, 0, 0 };
            case Shape::Birch: return { Trunk::Straight, 5, 2, 0, Foliage::Blob, 2, 2, 0, 0, 3, 3, 0, 0, 0, 2 };
            case Shape::SuperBirch: return { Trunk::Straight, 5, 2, 6, Foliage::Blob, 2, 2, 0, 0, 3, 3, 0, 0, 0, 2 };
            case Shape::Spruce: return { Trunk::Straight, 5, 2, 1, Foliage::Spruce, 2, 3, 0, 2, 0, 0, 1, 2, 0, 1 };
            case Shape::Pine: return { Trunk::Straight, 6, 4, 0, Foliage::Pine, 1, 1, 1, 1, 3, 4, 0, 0, 0, 1 };
            case Shape::MegaSpruce: return { Trunk::Giant, 13, 2, 14, Foliage::MegaPine, 0, 0, 0, 0, 13, 17, 0, 0, 0, 1 };
            case Shape::MegaPine: return { Trunk::Giant, 13, 2, 14, Foliage::MegaPine, 0, 0, 0, 0, 3, 7, 0, 0, 0, 1 };
            case Shape::Jungle: case Shape::JungleNoVine:
                return { Trunk::Straight, 4, 8, 0, Foliage::Blob, 2, 2, 0, 0, 3, 3, 0, 0, 0, 3 };
            case Shape::MegaJungle: return { Trunk::MegaJungle, 10, 2, 19, Foliage::Jungle, 2, 2, 0, 0, 2, 2, 0, 0, 0, 3 };
            case Shape::JungleBush: return { Trunk::Straight, 1, 0, 0, Foliage::Bush, 2, 2, 1, 1, 2, 2, 0, 0, 0, 3 };
            case Shape::Acacia: return { Trunk::Forking, 5, 2, 2, Foliage::Acacia, 2, 2, 0, 0, 0, 0, 0, 0, 0, 4 };
            case Shape::DarkOak: return { Trunk::DarkOak, 6, 2, 1, Foliage::DarkOak, 0, 0, 0, 0, 4, 4, 0, 0, 0, 5 };
            case Shape::Cherry: return { Trunk::Cherry, 7, 1, 0, Foliage::Cherry, 4, 4, 0, 0, 5, 5, 0, 0, 0, 6 };
            case Shape::Mangrove: return { Trunk::Upward, 2, 1, 4, Foliage::Spread, 3, 3, 0, 0, 2, 2, 0, 0, 70, 7 };
            case Shape::TallMangrove: return { Trunk::Upward, 4, 1, 9, Foliage::Spread, 3, 3, 0, 0, 2, 2, 0, 0, 70, 7 };
            case Shape::SwampOak: return { Trunk::Straight, 5, 3, 0, Foliage::Blob, 3, 3, 0, 0, 3, 3, 0, 0, 0, 0 };
            case Shape::Azalea: return { Trunk::Bending, 4, 2, 0, Foliage::Spread, 3, 3, 0, 0, 2, 2, 0, 0, 50, 0 };
        }
        return { Trunk::Straight, 4, 2, 0, Foliage::Blob, 2, 2, 0, 0, 3, 3, 0, 0, 0, 0 };
    }

    std::vector<Node> straight(Builder& b, const glm::ivec3& base, int height)
    {
        b.dirt(base + glm::ivec3(0, -1, 0));
        for (int i = 0; i < height; ++i) b.setLog(base + glm::ivec3(0, i, 0));
        return { { base + glm::ivec3(0, height, 0), 0, false } };
    }

    std::vector<Node> forking(Builder& b, const glm::ivec3& base, int height)
    {
        Rng& r = b.rng;
        b.dirt(base + glm::ivec3(0, -1, 0));
        std::vector<Node> nodes;
        const int dir = r.nextInt(4);
        const int i = height - r.nextInt(4) - 1;
        int j = 3 - r.nextInt(3);
        int k = base.x, l = base.z;
        int top = INT_MIN;
        for (int m = 0; m < height; ++m)
        {
            const int n = base.y + m;
            if (m >= i && j > 0)
            {
                k += DIRS[dir].x;
                l += DIRS[dir].z;
                --j;
            }
            if (b.setLog({ k, n, l })) top = n + 1;
        }
        if (top != INT_MIN) nodes.push_back({ { k, top, l }, 1, false });
        k = base.x;
        l = base.z;
        const int dir2 = r.nextInt(4);
        if (dir2 != dir)
        {
            const int start = i - r.nextInt(2) - 1;
            int o = 1 + r.nextInt(3);
            top = INT_MIN;
            for (int p = start; p < height && o > 0; --o)
            {
                if (p >= 1)
                {
                    const int q = base.y + p;
                    k += DIRS[dir2].x;
                    l += DIRS[dir2].z;
                    if (b.setLog({ k, q, l })) top = q + 1;
                }
                ++p;
            }
            if (top != INT_MIN) nodes.push_back({ { k, top, l }, 0, false });
        }
        return nodes;
    }

    std::vector<Node> darkOak(Builder& b, const glm::ivec3& base, int height)
    {
        Rng& r = b.rng;
        std::vector<Node> nodes;
        for (const glm::ivec3& d : { glm::ivec3(0), glm::ivec3(1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(1, 0, 1) })
            b.dirt(base + d + glm::ivec3(0, -1, 0));
        const int dir = r.nextInt(4);
        const int i = height - r.nextInt(4);
        int j = 2 - r.nextInt(3);
        const int k = base.x, l = base.y, m = base.z;
        int n = k, o = m;
        const int p = l + height - 1;
        for (int q = 0; q < height; ++q)
        {
            if (q >= i && j > 0)
            {
                n += DIRS[dir].x;
                o += DIRS[dir].z;
                --j;
            }
            const glm::ivec3 at(n, l + q, o);
            const Cell here = b.kind(at);
            if (here == Empty || here == Leaf)
            {
                b.setLog(at);
                b.setLog(at + glm::ivec3(1, 0, 0));
                b.setLog(at + glm::ivec3(0, 0, 1));
                b.setLog(at + glm::ivec3(1, 0, 1));
            }
        }
        nodes.push_back({ { n, p, o }, 0, true });
        for (int q = -1; q <= 2; ++q)
            for (int s = -1; s <= 2; ++s)
                if ((q < 0 || q > 1 || s < 0 || s > 1) && r.nextInt(3) <= 0)
                {
                    const int t = r.nextInt(3) + 2;
                    for (int u = 0; u < t; ++u) b.setLog({ k + q, p - u - 1, m + s });
                    nodes.push_back({ { n + q, p, o + s }, 0, false });
                }
        return nodes;
    }

    float branchChance(int treeHeight, int height)
    {
        if (static_cast<float>(height) < static_cast<float>(treeHeight) * 0.3f) return -1.0f;
        const float f = static_cast<float>(treeHeight) / 2.0f;
        const float g = f - static_cast<float>(height);
        float h = std::sqrt(std::max(0.0f, f * f - g * g));
        if (g == 0.0f) h = f;
        else if (std::abs(g) >= f) return 0.0f;
        return h * 0.5f;
    }

    void branchLine(Builder& b, const glm::ivec3& from, const glm::ivec3& to)
    {
        const glm::ivec3 off = to - from;
        const int longest = std::max(std::abs(off.x), std::max(std::abs(off.y), std::abs(off.z)));
        if (longest == 0)
        {
            b.setLog(from);
            return;
        }
        const float fx = static_cast<float>(off.x) / longest, fy = static_cast<float>(off.y) / longest, fz = static_cast<float>(off.z) / longest;
        for (int j = 0; j <= longest; ++j)
        {
            const glm::ivec3 at = from + glm::ivec3(static_cast<int>(std::floor(0.5f + j * fx)), static_cast<int>(std::floor(0.5f + j * fy)),
                                                    static_cast<int>(std::floor(0.5f + j * fz)));
            const int ax = std::abs(at.x - from.x), az = std::abs(at.z - from.z);
            const int most = std::max(ax, az);
            const int axis = most > 0 ? (ax == most ? 1 : 2) : 0;
            b.setLog(at, axis);
        }
    }

    std::vector<Node> fancy(Builder& b, const glm::ivec3& base, int height)
    {
        Rng& r = b.rng;
        const int treeHeight = height + 2;
        const int k = static_cast<int>(std::floor(treeHeight * 0.618));
        b.dirt(base + glm::ivec3(0, -1, 0));
        const int m = base.y + k;
        int n = treeHeight - 5;
        struct Branch
        {
            glm::ivec3 node;
            int endY;
        };
        std::vector<Branch> branches;
        branches.push_back({ base + glm::ivec3(0, n, 0), m });
        for (; n >= 0; --n)
        {
            const float f = branchChance(treeHeight, n);
            if (f < 0.0f) continue;
            const double g = 1.0 * f * (r.nextFloat() + 0.328);
            const double h = r.nextFloat() * 2.0 * PI;
            const double p = g * std::sin(h) + 0.5;
            const double q = g * std::cos(h) + 0.5;
            const glm::ivec3 tip = base + glm::ivec3(static_cast<int>(std::floor(p)), n - 1, static_cast<int>(std::floor(q)));
            const int dx = base.x - tip.x, dz = base.z - tip.z;
            const double t = tip.y - std::sqrt(static_cast<double>(dx * dx + dz * dz)) * 0.381;
            const int u = t > m ? m : static_cast<int>(t);
            branches.push_back({ tip, u });
        }
        branchLine(b, base, base + glm::ivec3(0, k, 0));
        for (const Branch& branch : branches)
        {
            const glm::ivec3 start(base.x, branch.endY, base.z);
            if (start != branch.node && branch.endY - base.y >= treeHeight * 0.2) branchLine(b, start, branch.node);
        }
        std::vector<Node> nodes;
        for (const Branch& branch : branches)
            if (branch.endY - base.y >= treeHeight * 0.2) nodes.push_back({ branch.node, 0, false });
        return nodes;
    }

    std::vector<Node> giantTrunk(Builder& b, const glm::ivec3& base, int height)
    {
        for (const glm::ivec3& d : { glm::ivec3(0), glm::ivec3(1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(1, 0, 1) })
            b.dirt(base + d + glm::ivec3(0, -1, 0));
        for (int i = 0; i < height; ++i)
        {
            b.setLog(base + glm::ivec3(0, i, 0));
            if (i < height - 1)
            {
                b.setLog(base + glm::ivec3(1, i, 0));
                b.setLog(base + glm::ivec3(1, i, 1));
                b.setLog(base + glm::ivec3(0, i, 1));
            }
        }
        return { { base + glm::ivec3(0, height, 0), 0, true } };
    }

    std::vector<Node> megaJungle(Builder& b, const glm::ivec3& base, int height)
    {
        Rng& r = b.rng;
        std::vector<Node> nodes = giantTrunk(b, base, height);
        for (int i = height - 2 - r.nextInt(4); i > height / 2; i -= 2 + r.nextInt(4))
        {
            const float f = r.nextFloat() * PI * 2.0f;
            int j = 0, k = 0;
            for (int l = 0; l < 5; ++l)
            {
                j = static_cast<int>(1.5f + std::cos(f) * l);
                k = static_cast<int>(1.5f + std::sin(f) * l);
                b.setLog(base + glm::ivec3(j, i - 3 + l / 2, k));
            }
            nodes.push_back({ base + glm::ivec3(j, i, k), -2, false });
        }
        return nodes;
    }

    Node cherryBranch(Builder& b, int height, const glm::ivec3& base, int dir, int start, bool below)
    {
        Rng& r = b.rng;
        glm::ivec3 at = base + glm::ivec3(0, start, 0);
        const int i = height - 1 + r.between(-1, 0);
        const bool longer = below || i < start;
        const int length = r.between(2, 4) + (longer ? 1 : 0);
        const glm::ivec3 target = base + DIRS[dir] * length + glm::ivec3(0, i, 0);
        const int steps = longer ? 2 : 1;
        for (int l = 0; l < steps; ++l)
        {
            at += DIRS[dir];
            b.setLog(at, axisOf(dir));
        }
        const glm::ivec3 vertical(0, target.y > at.y ? 1 : -1, 0);
        for (int guard = 0; guard < 64; ++guard)
        {
            const int m = std::abs(target.x - at.x) + std::abs(target.y - at.y) + std::abs(target.z - at.z);
            if (m == 0) break;
            const float f = static_cast<float>(std::abs(target.y - at.y)) / static_cast<float>(m);
            const bool up = r.nextFloat() < f;
            at += up ? vertical : DIRS[dir];
            b.setLog(at, up ? 0 : axisOf(dir));
        }
        return { target + glm::ivec3(0, 1, 0), 0, false };
    }

    std::vector<Node> cherry(Builder& b, const glm::ivec3& base, int height)
    {
        Rng& r = b.rng;
        b.dirt(base + glm::ivec3(0, -1, 0));
        const int i = std::max(0, height - 1 + r.between(-4, -3));
        int j = std::max(0, height - 1 + r.between(-4, -4));
        if (j >= i) ++j;
        const int count = 1 + r.nextInt(3);
        const bool three = count == 3;
        const bool two = count >= 2;
        const int l = three ? height : two ? std::max(i, j) + 1 : i + 1;
        for (int m = 0; m < l; ++m) b.setLog(base + glm::ivec3(0, m, 0));
        std::vector<Node> nodes;
        if (three) nodes.push_back({ base + glm::ivec3(0, l, 0), 0, false });
        const int dir = r.nextInt(4);
        nodes.push_back(cherryBranch(b, height, base, dir, i, i < l - 1));
        if (two) nodes.push_back(cherryBranch(b, height, base, (dir + 2) & 3, j, j < l - 1));
        return nodes;
    }

    std::vector<Node> upward(Builder& b, const glm::ivec3& base, int height)
    {
        Rng& r = b.rng;
        std::vector<Node> nodes;
        for (int i = 0; i < height; ++i)
        {
            const int j = base.y + i;
            if (b.setLog({ base.x, j, base.z }) && i < height - 1 && r.nextFloat() < 0.5f)
            {
                const int dir = r.nextInt(4);
                const int k = r.between(0, 1);
                const int length = std::max(0, k - r.between(0, 1) - 1);
                int steps = r.between(1, 4);
                int top = j + length;
                int x = base.x, z = base.z;
                int l = length;
                while (l < height && steps > 0)
                {
                    if (l >= 1)
                    {
                        const int y = j + l;
                        x += DIRS[dir].x;
                        z += DIRS[dir].z;
                        top = y;
                        if (b.setLog({ x, y, z })) top = y + 1;
                        nodes.push_back({ { x, y, z }, 0, false });
                    }
                    ++l;
                    --steps;
                }
                if (top - j > 1)
                {
                    nodes.push_back({ { x, top, z }, 0, false });
                    nodes.push_back({ { x, top - 2, z }, 0, false });
                }
            }
            if (i == height - 1) nodes.push_back({ { base.x, j + 1, base.z }, 0, false });
        }
        return nodes;
    }

    std::vector<Node> bending(Builder& b, const glm::ivec3& base, int height)
    {
        Rng& r = b.rng;
        const int dir = r.nextInt(4);
        const int i = height - 1;
        glm::ivec3 at = base;
        b.dirt(base + glm::ivec3(0, -1, 0));
        std::vector<Node> nodes;
        for (int j = 0; j <= i; ++j)
        {
            if (j + 1 >= i + r.nextInt(2)) at += DIRS[dir];
            if (b.kind(at) != Log) b.setLog(at);
            if (j >= 3) nodes.push_back({ at, 0, false });
            at.y += 1;
        }
        const int bend = r.between(1, 2);
        for (int k = 0; k <= bend; ++k)
        {
            if (b.kind(at) != Log) b.setLog(at);
            nodes.push_back({ at, 0, false });
            at += DIRS[dir];
        }
        return nodes;
    }

    void hangingSquare(Builder& b, const glm::ivec3& c, int radius, int y, bool giant, const Invalid& invalid, float chance, float extension)
    {
        square(b, c, radius, y, giant, invalid);
        const int extra = giant ? 1 : 0;
        const glm::ivec3 origin = c + glm::ivec3(0, -1, 0);
        auto place = [&](float odds, const glm::ivec3& at) {
            const glm::ivec3 d = at - origin;
            if (std::abs(d.x) + std::abs(d.y) + std::abs(d.z) >= 7) return false;
            if (b.rng.nextFloat() > odds) return false;
            return b.setLeaf(at);
        };
        for (int dir = 0; dir < 4; ++dir)
        {
            const int clockwise = (dir + 1) & 3;
            const bool positive = DIRS[clockwise].x > 0 || DIRS[clockwise].z > 0;
            const int reach = positive ? radius + extra : radius;
            glm::ivec3 at = c + glm::ivec3(0, y - 1, 0) + DIRS[clockwise] * reach - DIRS[dir] * radius;
            for (int k = -radius; k < radius + extra; ++k)
            {
                const bool above = b.placedLeaf(at + glm::ivec3(0, 1, 0));
                if (above && place(chance, at)) place(extension, at + glm::ivec3(0, -1, 0));
                at += DIRS[dir];
            }
        }
    }

    void foliage(Builder& b, const Spec& spec, const Node& node, int trunkHeight, int foliageHeight, int radius)
    {
        Rng& r = b.rng;
        const int offset = r.between(spec.offsetMin, spec.offsetMax);
        const glm::ivec3 c = node.centre;
        const bool giant = node.giant;
        auto corner = [](int j, int y, int k, int rad, bool g) {
            (void)y;
            return norm(j, g) == rad && norm(k, g) == rad && rad > 0;
        };
        switch (spec.foliage)
        {
            case Foliage::Blob:
                for (int i = offset; i >= offset - foliageHeight; --i)
                {
                    const int j = std::max(radius + node.radius - 1 - i / 2, 0);
                    square(b, c, j, i, giant, [&](int a, int y, int k, int rad, bool g) {
                        return norm(a, g) == rad && norm(k, g) == rad && (r.nextInt(2) == 0 || y == 0);
                    });
                }
                break;
            case Foliage::Fancy:
                for (int i = offset; i >= offset - foliageHeight; --i)
                {
                    const int j = radius + ((i != offset && i != offset - foliageHeight) ? 1 : 0);
                    square(b, c, j, i, giant, [](int a, int, int k, int rad, bool g) {
                        const float fx = norm(a, g) + 0.5f, fz = norm(k, g) + 0.5f;
                        return fx * fx + fz * fz > static_cast<float>(rad * rad);
                    });
                }
                break;
            case Foliage::Acacia:
            {
                const glm::ivec3 top = c + glm::ivec3(0, offset, 0);
                const Invalid invalid = [](int a, int y, int k, int rad, bool g) {
                    const int dx = norm(a, g), dz = norm(k, g);
                    return y == 0 ? ((dx > 1 || dz > 1) && dx != 0 && dz != 0) : (dx == rad && dz == rad && rad > 0);
                };
                square(b, top, radius + node.radius, -1 - foliageHeight, giant, invalid);
                square(b, top, radius - 1, -foliageHeight, giant, invalid);
                square(b, top, radius + node.radius - 1, 0, giant, invalid);
                break;
            }
            case Foliage::DarkOak:
            {
                const glm::ivec3 top = c + glm::ivec3(0, offset, 0);
                const Invalid invalid = [](int a, int y, int k, int rad, bool g) {
                    if (y == 0 && g && (a == -rad || a >= rad) && (k == -rad || k >= rad)) return true;
                    const int dx = norm(a, g), dz = norm(k, g);
                    if (y == -1 && !g) return dx == rad && dz == rad;
                    return y == 1 ? dx + dz > rad * 2 - 2 : false;
                };
                if (giant)
                {
                    square(b, top, radius + 2, -1, giant, invalid);
                    square(b, top, radius + 3, 0, giant, invalid);
                    square(b, top, radius + 2, 1, giant, invalid);
                    if (r.nextBoolean()) square(b, top, radius, 2, giant, invalid);
                }
                else
                {
                    square(b, top, radius + 2, -1, giant, invalid);
                    square(b, top, radius + 1, 0, giant, invalid);
                }
                break;
            }
            case Foliage::Spruce:
            {
                int i = r.nextInt(2);
                int j = 1;
                int k = 0;
                for (int l = offset; l >= -foliageHeight; --l)
                {
                    square(b, c, i, l, giant, corner);
                    if (i >= j)
                    {
                        i = k;
                        k = 1;
                        j = std::min(j + 1, radius + node.radius);
                    }
                    else
                        ++i;
                }
                break;
            }
            case Foliage::Pine:
            {
                int i = 0;
                for (int j = offset; j >= offset - foliageHeight; --j)
                {
                    square(b, c, i, j, giant, corner);
                    if (i >= 1 && j == offset - foliageHeight + 1) --i;
                    else if (i < radius + node.radius) ++i;
                }
                break;
            }
            case Foliage::MegaPine:
            {
                int previous = 0;
                for (int j = c.y - foliageHeight + offset; j <= c.y + offset; ++j)
                {
                    const int k = c.y - j;
                    const int l = radius + node.radius + static_cast<int>(std::floor(static_cast<float>(k) / foliageHeight * 3.5f));
                    const int m = (k > 0 && l == previous && (j & 1) == 0) ? l + 1 : l;
                    square(b, glm::ivec3(c.x, j, c.z), m, 0, giant, [](int a, int, int kk, int rad, bool g) {
                        const int dx = norm(a, g), dz = norm(kk, g);
                        return dx + dz >= 7 ? true : dx * dx + dz * dz > rad * rad;
                    });
                    previous = l;
                }
                break;
            }
            case Foliage::Jungle:
            {
                const int layers = giant ? foliageHeight : 1 + r.nextInt(2);
                for (int j = offset; j >= offset - layers; --j)
                {
                    const int k = radius + node.radius + 1 - j;
                    square(b, c, k, j, giant, [](int a, int, int kk, int rad, bool g) {
                        const int dx = norm(a, g), dz = norm(kk, g);
                        return dx + dz >= 7 ? true : dx * dx + dz * dz > rad * rad;
                    });
                }
                break;
            }
            case Foliage::Cherry:
            {
                const glm::ivec3 top = c + glm::ivec3(0, offset, 0);
                const int i = radius + node.radius - 1;
                const Invalid invalid = [&](int a, int y, int k, int rad, bool g) {
                    const int dx = norm(a, g), dz = norm(k, g);
                    if (y == -1 && (dx == rad || dz == rad) && r.nextFloat() < 0.25f) return true;
                    const bool cornerCell = dx == rad && dz == rad;
                    if (rad > 2) return cornerCell || (dx + dz > rad * 2 - 2 && r.nextFloat() < 0.5f);
                    return cornerCell && r.nextFloat() < 0.5f;
                };
                square(b, top, i - 2, foliageHeight - 3, giant, invalid);
                square(b, top, i - 1, foliageHeight - 4, giant, invalid);
                for (int j = foliageHeight - 5; j >= 0; --j) square(b, top, i, j, giant, invalid);
                hangingSquare(b, top, i, -1, giant, invalid, 0.16666667f, 0.33333334f);
                hangingSquare(b, top, i - 1, -2, giant, invalid, 0.16666667f, 0.33333334f);
                break;
            }
            case Foliage::Spread:
                for (int i = 0; i < spec.attempts; ++i)
                {
                    const int dx = r.nextInt(radius) - r.nextInt(radius);
                    const int dy = r.nextInt(foliageHeight) - r.nextInt(foliageHeight);
                    const int dz = r.nextInt(radius) - r.nextInt(radius);
                    b.setLeaf(c + glm::ivec3(dx, dy, dz));
                }
                break;
            case Foliage::Bush:
                for (int i = offset; i >= offset - foliageHeight; --i)
                {
                    const int j = radius + node.radius - 1 - i;
                    square(b, c, j, i, giant, [&](int a, int, int k, int rad, bool g) {
                        return norm(a, g) == rad && norm(k, g) == rad && r.nextInt(2) == 0;
                    });
                }
                break;
        }
        (void)trunkHeight;
    }

    std::vector<glm::ivec3> sorted(const std::vector<Trees::Placed>& list)
    {
        std::vector<glm::ivec3> out;
        for (const Trees::Placed& p : list)
            if (p.id != Blocks::Air) out.push_back(p.at);
        std::stable_sort(out.begin(), out.end(), [](const glm::ivec3& a, const glm::ivec3& b) { return a.y < b.y; });
        return out;
    }

    void trunkVines(Builder& b)
    {
        for (const glm::ivec3& p : sorted(b.plan.logs))
        {
            if (b.rng.nextInt(3) > 0 && b.air(p + glm::ivec3(-1, 0, 0))) b.vine(p + glm::ivec3(-1, 0, 0), 1);
            if (b.rng.nextInt(3) > 0 && b.air(p + glm::ivec3(1, 0, 0))) b.vine(p + glm::ivec3(1, 0, 0), 3);
            if (b.rng.nextInt(3) > 0 && b.air(p + glm::ivec3(0, 0, -1))) b.vine(p + glm::ivec3(0, 0, -1), 2);
            if (b.rng.nextInt(3) > 0 && b.air(p + glm::ivec3(0, 0, 1))) b.vine(p + glm::ivec3(0, 0, 1), 0);
        }
    }

    void leafVines(Builder& b, float chance)
    {
        auto hang = [&](glm::ivec3 at, int bit) {
            b.vine(at, bit);
            at.y -= 1;
            for (int i = 4; i > 0 && b.air(at); --i)
            {
                b.vine(at, bit);
                at.y -= 1;
            }
        };
        for (const glm::ivec3& p : sorted(b.plan.leaves))
        {
            if (b.rng.nextFloat() < chance && b.air(p + glm::ivec3(-1, 0, 0))) hang(p + glm::ivec3(-1, 0, 0), 1);
            if (b.rng.nextFloat() < chance && b.air(p + glm::ivec3(1, 0, 0))) hang(p + glm::ivec3(1, 0, 0), 3);
            if (b.rng.nextFloat() < chance && b.air(p + glm::ivec3(0, 0, -1))) hang(p + glm::ivec3(0, 0, -1), 2);
            if (b.rng.nextFloat() < chance && b.air(p + glm::ivec3(0, 0, 1))) hang(p + glm::ivec3(0, 0, 1), 0);
        }
    }

    void cocoa(Builder& b, float chance)
    {
        if (b.rng.nextFloat() >= chance) return;
        const std::vector<glm::ivec3> logs = sorted(b.plan.logs);
        if (logs.empty()) return;
        const int lowest = logs.front().y;
        for (const glm::ivec3& p : logs)
        {
            if (p.y - lowest > 2) continue;
            for (int dir = 0; dir < 4; ++dir)
            {
                if (b.rng.nextFloat() > 0.25f) continue;
                const glm::ivec3 at = p - DIRS[dir];
                if (b.air(at)) b.setExtra(at, cocoaWith(dir, b.rng.nextInt(3)));
            }
        }
    }

    void beehive(Builder& b, float chance)
    {
        if (chance <= 0.0f || b.rng.nextFloat() >= chance) return;
        const std::vector<glm::ivec3> logs = sorted(b.plan.logs);
        const std::vector<glm::ivec3> leaves = sorted(b.plan.leaves);
        if (logs.empty()) return;
        const int y = !leaves.empty() ? std::max(leaves.front().y - 1, logs.front().y + 1)
                                      : std::min(logs.front().y + 1 + b.rng.nextInt(3), logs.back().y);
        std::vector<glm::ivec3> spots;
        for (const glm::ivec3& p : logs)
            if (p.y == y)
                for (int dir : { 1, 2, 3 }) spots.push_back(p + DIRS[dir]);
        for (size_t i = spots.size(); i > 1; --i) std::swap(spots[i - 1], spots[static_cast<size_t>(b.rng.nextInt(static_cast<int>(i)))]);
        for (const glm::ivec3& spot : spots)
            if (b.air(spot) && b.air(spot + DIRS[2]))
            {
                b.setExtra(spot, hiveOf(true, 2, 0));
                return;
            }
    }

    void podzol(Builder& b)
    {
        const std::vector<glm::ivec3> logs = sorted(b.plan.logs);
        if (logs.empty()) return;
        const int lowest = logs.front().y;
        std::set<std::tuple<int, int, int>> seen;
        auto area = [&](const glm::ivec3& at) {
            for (int i = -2; i <= 2; ++i)
                for (int j = -2; j <= 2; ++j)
                    if (std::abs(i) != 2 || std::abs(j) != 2)
                    {
                        const glm::ivec3 column = at + glm::ivec3(i, 0, j);
                        if (seen.insert({ column.x, column.y, column.z }).second) b.plan.podzol.push_back(column);
                    }
        };
        for (const glm::ivec3& p : logs)
        {
            if (p.y != lowest) continue;
            area(p + glm::ivec3(-1, 0, -1));
            area(p + glm::ivec3(2, 0, -1));
            area(p + glm::ivec3(-1, 0, 2));
            area(p + glm::ivec3(2, 0, 2));
            for (int i = 0; i < 5; ++i)
            {
                const int j = b.rng.nextInt(64);
                const int k = j % 8, l = j / 8;
                if (k == 0 || k == 7 || l == 0 || l == 7) area(p + glm::ivec3(-3 + k, 0, -3 + l));
            }
        }
    }

    void propagules(Builder& b, float chance)
    {
        std::vector<glm::ivec3> leaves;
        for (const Trees::Placed& p : b.plan.leaves)
            if (p.id != Blocks::Air) leaves.push_back(p.at);
        for (size_t i = leaves.size(); i > 1; --i) std::swap(leaves[i - 1], leaves[static_cast<size_t>(b.rng.nextInt(static_cast<int>(i)))]);
        std::set<std::tuple<int, int, int>> excluded;
        for (const glm::ivec3& leaf : leaves)
        {
            b.rng.nextInt(1);
            const glm::ivec3 below = leaf + glm::ivec3(0, -1, 0);
            if (excluded.count({ below.x, below.y, below.z })) continue;
            if (b.rng.nextFloat() >= chance) continue;
            if (!b.air(below) || !b.air(below + glm::ivec3(0, -1, 0))) continue;
            for (int dx = -1; dx <= 1; ++dx)
                for (int dz = -1; dz <= 1; ++dz) excluded.insert({ below.x + dx, below.y, below.z + dz });
            b.setExtra(below, Blocks::MangrovePropaguleHanging);
        }
    }

    bool mangroveRoots(Builder& b, const glm::ivec3& origin, const glm::ivec3& trunk, const Trees::Ground& ground)
    {
        auto through = [&](const glm::ivec3& p) { return ground.open(p.x, p.y, p.z) || ground.muddy(p.x, p.y, p.z); };
        for (glm::ivec3 at = origin; at.y < trunk.y; ++at.y)
            if (!through(at)) return false;
        std::vector<glm::ivec3> roots{ trunk + glm::ivec3(0, -1, 0) };
        std::function<bool(const glm::ivec3&, int, std::vector<glm::ivec3>&, int)> grow =
            [&](const glm::ivec3& at, int dir, std::vector<glm::ivec3>& offshoots, int length) -> bool {
            if (length == 15 || offshoots.size() > 15) return false;
            const glm::ivec3 down = at + glm::ivec3(0, -1, 0);
            const glm::ivec3 side = at + DIRS[dir];
            const int distance = std::abs(at.x - trunk.x) + std::abs(at.y - trunk.y) + std::abs(at.z - trunk.z);
            std::vector<glm::ivec3> next;
            if (distance > 5 && distance <= 8) next = b.rng.nextFloat() < 0.2f ? std::vector<glm::ivec3>{ down, side + glm::ivec3(0, -1, 0) }
                                                                                : std::vector<glm::ivec3>{ down };
            else if (distance > 8) next = { down };
            else if (b.rng.nextFloat() < 0.2f) next = { down };
            else next = b.rng.nextBoolean() ? std::vector<glm::ivec3>{ side } : std::vector<glm::ivec3>{ down };
            for (const glm::ivec3& p : next)
                if (through(p))
                {
                    offshoots.push_back(p);
                    if (!grow(p, dir, offshoots, length + 1)) return false;
                }
            return true;
        };
        for (int dir = 0; dir < 4; ++dir)
        {
            const glm::ivec3 start = trunk + DIRS[dir];
            std::vector<glm::ivec3> offshoots;
            if (!grow(start, dir, offshoots, 0)) return false;
            roots.insert(roots.end(), offshoots.begin(), offshoots.end());
            roots.push_back(start);
        }
        for (const glm::ivec3& p : roots)
        {
            if (!through(p)) continue;
            b.plan.roots.push_back({ p, ground.muddy(p.x, p.y, p.z) ? Blocks::MuddyMangroveRoots : Blocks::MangroveRoots });
            const glm::ivec3 above = p + glm::ivec3(0, 1, 0);
            if (b.rng.nextFloat() < 0.5f && ground.open(above.x, above.y, above.z) && b.air(above))
                b.setExtra(above, Blocks::MossCarpet);
        }
        return true;
    }
}

bool Trees::giant(Shape shape)
{
    return shape == Shape::MegaSpruce || shape == Shape::MegaPine || shape == Shape::MegaJungle || shape == Shape::DarkOak;
}

Trees::Plan Trees::plan(Shape shape, const glm::ivec3& origin, uint32_t seed, float bees, const Ground* ground)
{
    Plan out;
    Rng rng(seed);
    Builder b(out, rng);
    const Spec spec = specOf(shape);
    b.log = logOfWood(spec.wood);
    b.leaves = leavesOfWood(spec.wood);
    if (shape == Shape::JungleBush) b.leaves = Blocks::Leaves;
    if (shape == Shape::Azalea)
    {
        b.leaves = Blocks::AzaleaLeaves;
        b.flowering = Blocks::FloweringAzaleaLeaves;
        out.dirtBlock = Blocks::RootedDirt;
        out.forceDirt = true;
    }

    const int height = spec.base + rng.nextInt(spec.a + 1) + rng.nextInt(spec.b + 1);
    int foliageHeight = spec.heightMin;
    switch (spec.foliage)
    {
        case Foliage::Acacia: foliageHeight = 0; break;
        case Foliage::DarkOak: foliageHeight = 4; break;
        case Foliage::Spruce: foliageHeight = std::max(4, height - rng.between(spec.extraMin, spec.extraMax)); break;
        case Foliage::Pine: case Foliage::MegaPine: case Foliage::Cherry: case Foliage::Spread:
            foliageHeight = rng.between(spec.heightMin, spec.heightMax);
            break;
        default: break;
    }
    int radius = rng.between(spec.radiusMin, spec.radiusMax);
    if (spec.foliage == Foliage::Pine) radius += rng.nextInt(std::max(height - foliageHeight + 1, 1));

    glm::ivec3 base = origin;
    const bool mangrove = shape == Shape::Mangrove || shape == Shape::TallMangrove;
    if (mangrove) base.y += rng.between(1, 3);
    if (mangrove && ground && !mangroveRoots(b, origin, base, *ground))
    {
        out.ok = false;
        return out;
    }

    std::vector<Node> nodes;
    switch (spec.trunk)
    {
        case Trunk::Straight: nodes = straight(b, base, height); break;
        case Trunk::Forking: nodes = forking(b, base, height); break;
        case Trunk::DarkOak: nodes = darkOak(b, base, height); break;
        case Trunk::Fancy: nodes = fancy(b, base, height); break;
        case Trunk::Giant: nodes = giantTrunk(b, base, height); break;
        case Trunk::MegaJungle: nodes = megaJungle(b, base, height); break;
        case Trunk::Cherry: nodes = cherry(b, base, height); break;
        case Trunk::Upward: nodes = upward(b, base, height); break;
        case Trunk::Bending: nodes = bending(b, base, height); break;
    }
    for (const Node& node : nodes) foliage(b, spec, node, height, foliageHeight, radius);

    switch (shape)
    {
        case Shape::Jungle:
            cocoa(b, 0.2f);
            trunkVines(b);
            leafVines(b, 0.25f);
            break;
        case Shape::MegaJungle:
            trunkVines(b);
            leafVines(b, 0.25f);
            break;
        case Shape::SwampOak: leafVines(b, 0.25f); break;
        case Shape::MegaSpruce: case Shape::MegaPine: podzol(b); break;
        case Shape::Mangrove: case Shape::TallMangrove:
            leafVines(b, 0.125f);
            propagules(b, 0.14f);
            beehive(b, 0.01f);
            break;
        default: break;
    }
    if (bees > 0.0f) beehive(b, bees);
    out.base = base;
    out.height = height;
    return out;
}

bool Trees::replaceable(BlockId id)
{
    const BlockId bare = static_cast<BlockId>(id & ~WATERLOGGED);
    return bare == Blocks::Air || isLeaves(bare) || isVine(bare) || isWater(bare) || bare == Blocks::TallGrass || bare == Blocks::Fern ||
           bare == Blocks::DeadBush || isDoublePlant(bare) || bare == Blocks::Seagrass || isTallSeagrass(bare) || isGlowLichen(bare) ||
           bare == Blocks::HangingRoots || isSnowLayer(bare) || isSapling(bare) || bare == Blocks::MossCarpet ||
           bare == Blocks::MangrovePropaguleHanging || bare == Blocks::MangroveRoots;
}

bool Trees::fits(Shape shape, const Plan& plan, const Cells& cells)
{
    if (!plan.ok) return false;
    int limit = 1, lower = 0, upper = 1;
    switch (shape)
    {
        case Shape::FancyOak: case Shape::JungleBush: limit = 0; lower = 0; upper = 0; break;
        case Shape::Spruce: case Shape::Pine: case Shape::Mangrove: limit = 2; lower = 0; upper = 2; break;
        case Shape::TallMangrove: limit = 3; lower = 0; upper = 2; break;
        case Shape::Acacia: case Shape::Cherry: limit = 1; lower = 0; upper = 2; break;
        case Shape::MegaSpruce: case Shape::MegaPine: case Shape::MegaJungle: limit = 1; lower = 1; upper = 2; break;
        default: break;
    }
    for (int i = 0; i <= plan.height + 1; ++i)
    {
        int radius = i < limit ? lower : upper;
        if (shape == Shape::DarkOak) radius = i < 1 ? 0 : (i >= plan.height - 1 ? 2 : 1);
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
            {
                const BlockId here = cells.at(plan.base.x + dx, plan.base.y + i, plan.base.z + dz);
                if (!replaceable(here) && !isLog(static_cast<BlockId>(here & ~WATERLOGGED))) return false;
            }
    }
    return true;
}

void Trees::write(const Plan& plan, Cells& cells)
{
    auto soaked = [](BlockId here, BlockId put) {
        return isWater(static_cast<BlockId>(here & ~WATERLOGGED)) && canWaterlog(put) ? static_cast<BlockId>(put | WATERLOGGED) : put;
    };
    for (const glm::ivec3& p : plan.dirt)
    {
        const BlockId here = cells.at(p.x, p.y, p.z);
        if (plan.forceDirt) cells.set(p.x, p.y, p.z, plan.dirtBlock);
        else if (here == Blocks::Grass || here == Blocks::SnowGrass || here == Blocks::Mycelium || isFarmland(here))
            cells.set(p.x, p.y, p.z, Blocks::Dirt);
    }
    for (const Placed& root : plan.roots)
    {
        const BlockId here = cells.at(root.at.x, root.at.y, root.at.z);
        if (replaceable(here) || here == Blocks::Mud || here == Blocks::MuddyMangroveRoots)
            cells.set(root.at.x, root.at.y, root.at.z, soaked(here, root.id));
    }
    for (const Placed& leaf : plan.leaves)
    {
        if (leaf.id == Blocks::Air) continue;
        const BlockId here = cells.at(leaf.at.x, leaf.at.y, leaf.at.z);
        if (replaceable(here) && !isLeaves(static_cast<BlockId>(here & ~WATERLOGGED))) cells.set(leaf.at.x, leaf.at.y, leaf.at.z, soaked(here, leaf.id));
    }
    for (const Placed& log : plan.logs)
    {
        const BlockId here = cells.at(log.at.x, log.at.y, log.at.z);
        if (replaceable(here)) cells.set(log.at.x, log.at.y, log.at.z, log.id);
    }
    for (const Placed& extra : plan.extras)
        if (cells.at(extra.at.x, extra.at.y, extra.at.z) == Blocks::Air) cells.set(extra.at.x, extra.at.y, extra.at.z, extra.id);
    for (const glm::ivec3& column : plan.podzol)
        for (int i = 2; i >= -3; --i)
        {
            const BlockId here = cells.at(column.x, column.y + i, column.z);
            if (here == Blocks::Grass || here == Blocks::Dirt || here == Blocks::SnowGrass || here == Blocks::Podzol || here == Blocks::CoarseDirt ||
                here == Blocks::Mycelium || here == Blocks::RootedDirt || here == Blocks::Mud)
            {
                cells.set(column.x, column.y + i, column.z, Blocks::Podzol);
                break;
            }
            if (here != Blocks::Air && i < 0) break;
        }
}
