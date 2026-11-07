#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Paintings
{
    struct Variant
    {
        const char* name;
        int width;
        int height;
        bool placeable;
        const char* title;
        const char* author;
    };

    const std::vector<Variant>& variants();
    std::string texturePath(int variant);
    int count();
    int find(const std::string& name);

    struct Placed
    {
        glm::ivec3 at{ 0 };
        int facing = 0;
        int variant = 0;
    };

    glm::ivec3 out(int facing);
    glm::ivec3 right(int facing);
    int facingFor(const glm::ivec3& normal);
    std::vector<glm::ivec3> cells(const Placed& painting);

    struct Wall
    {
        virtual ~Wall() = default;
        virtual bool solid(int x, int y, int z) const = 0;
        virtual bool open(int x, int y, int z) const = 0;
    };

    bool fits(const Placed& painting, const Wall& wall);
    int choose(const glm::ivec3& at, int facing, const Wall& wall, uint32_t roll,
               const std::vector<Placed>& others = {});
    bool overlaps(const Placed& a, const Placed& b);

    struct Box
    {
        glm::vec3 min{ 0.0f }, max{ 0.0f };
    };
    Box bounds(const Placed& painting);
    bool rayHits(const Box& box, const glm::vec3& origin, const glm::vec3& direction, float reach, float& distance);
}

class PaintingStore
{
public:
    void setDimension(int dimension) { m_dimension = dimension; }
    void clear() { m_paintings.clear(); }

    void put(const Paintings::Placed& painting) { putIn(m_dimension, painting); }
    void putIn(int dimension, const Paintings::Placed& painting);
    bool remove(const Paintings::Placed& painting);

    std::vector<Paintings::Placed> here() const;
    std::vector<Paintings::Placed> near(const glm::vec3& around, float reach) const;

    struct Entry
    {
        int dimension = 0;
        Paintings::Placed painting;
    };
    std::vector<Entry> all() const;

private:
    using Key = std::array<int, 5>;
    static Key keyOf(int dimension, const Paintings::Placed& painting)
    {
        return { dimension, painting.at.x, painting.at.y, painting.at.z, painting.facing };
    }

    int m_dimension = 0;
    std::map<Key, int> m_paintings;
};
