#pragma once
#include "World/Block.h"
#include "Game/Items.h"
#include "Renderer/Mesh.h"
#include <vector>
#include <glm/glm.hpp>

class World;
class Shader;
class Player;
class Inventory;
class AudioEngine;

// Blocks you mine pop out, fall and settle where they land. A block
// tumbles as a little cube; an item, and anything drawn as crossed
// quads in the world, stands up as a flat sprite the way Minecraft
// draws them -- a flower rolled up into a cube reads as a coloured
// brick rather than a flower.
//
// Nothing is dragged towards you: a drop stays where it fell until you
// walk onto it. The only thing that moves one is another of the same
// kind landing beside it, which merges the two so a mined seam leaves
// one pile instead of thirty.
class DroppedItems
{
public:
    static constexpr float SIZE = 0.30f;        // edge length of the dropped cube
    static constexpr float SPRITE_SIZE = 0.42f; // height of a flat item sprite
    static constexpr float HOVER = 0.16f;       // how far it floats above where it rests
    static constexpr float COLLECT_RADIUS = 1.0f;
    static constexpr float PICKUP_DELAY = 2.0f; // before it can be walked back onto
    static constexpr float MERGE_RADIUS = 0.8f;
    static constexpr float LIFETIME_SECONDS = 300.0f;

    void spawn(const glm::vec3& position, StackId block, int count = 1);
    void spawnFromBrokenBlock(const glm::ivec3& blockPosition, StackId drop);

    void update(float deltaTime, const World& world, const Player& player,
                Inventory& inventory, AudioEngine& audio);

    // Rebuilds the geometry each frame: there are only ever a handful of
    // items, and they spin, so a dynamic buffer is simpler than instancing.
    void render(Shader& chunkShader, const World& world);

    void clear() { m_items.clear(); }
    int count() const { return static_cast<int>(m_items.size()); }

private:
    struct Item
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        StackId block = Blocks::Air;
        int count = 1;
        float age = 0.0f;
        float spin = 0.0f;
        bool resting = false;
    };

    std::vector<Item> m_items;
    Mesh m_mesh;
    std::vector<float> m_vertices;

    bool blocked(const World& world, const glm::vec3& centre) const;
    void mergeNearby();
};
