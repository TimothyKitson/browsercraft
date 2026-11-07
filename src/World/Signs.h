#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// What every sign in the world says, by where it stands.
//
// A block here is a bare id with nowhere to put words, so a sign's
// lines live here instead, keyed by position the way a chest's
// contents are. They are saved with the level, and a sign whose block
// is gone is forgotten along with what it said.
class Signs
{
public:
    static constexpr int LINES = 4;
    static constexpr size_t MAX_LINE = 15;

    using Text = std::array<std::string, LINES>;

    // The dimension is part of the key for the same reason it is for
    // the chests: a sign in the Nether at matching coordinates is a
    // different sign, and keyed on position alone the two shared what
    // they said.
    void setDimension(int dimension) { m_dimension = dimension; }

    Text& at(const glm::ivec3& block);
    bool has(const glm::ivec3& block) const;
    const Text* find(const glm::ivec3& block) const;

    void remove(const glm::ivec3& block);
    void clear() { m_signs.clear(); m_looks.clear(); }

    struct Look
    {
        uint8_t colour = 15;
        bool glow = false;
    };
    static constexpr uint8_t PLAIN_COLOUR = 15;
    static bool plain(const Look& look) { return look.colour == PLAIN_COLOUR && !look.glow; }
    Look look(const glm::ivec3& block) const;
    void setLook(const glm::ivec3& block, const Look& look);
    const std::map<std::array<int, 4>, Look>& looks() const { return m_looks; }
    void lookIn(int dimension, const glm::ivec3& block, const Look& look);

    static unsigned int textRgb(int colour);
    static unsigned int shadedRgb(const Look& look);

    // For saving. The order is stable, which keeps the file diffable.
    const std::map<std::array<int, 4>, Text>& all() const { return m_signs; }
    void putIn(int dimension, const glm::ivec3& block, const Text& text);

    // Every sign in this dimension within `reach` of a point, for the
    // pass that draws what they say. There are only ever a handful, so
    // this walks the lot rather than keeping an index.
    struct Near
    {
        glm::ivec3 block;
        const Text* text;
    };
    std::vector<Near> near(const glm::vec3& around, float reach) const;

    // Whether anything on it is worth keeping. A sign with four blank
    // lines is a board, and a board needs no row.
    static bool blank(const Text& text);

private:
    std::array<int, 4> key(const glm::ivec3& block) const
    {
        return { m_dimension, block.x, block.y, block.z };
    }

    int m_dimension = 0;
    std::map<std::array<int, 4>, Text> m_signs;
    std::map<std::array<int, 4>, Look> m_looks;
};
