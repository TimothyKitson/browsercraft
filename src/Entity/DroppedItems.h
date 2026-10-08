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

// Blocks you mine pop out as little tumbling cubes that fall, settle and
// get sucked towards you when you walk close. This is also the engine's
// first entity type -- the update/render split here is what mobs would
// slot into later.
class DroppedItems
{
public:
    static constexpr float SIZE = 0.30f;        // edge length of the dropped cube
    static constexpr float HOVER = 0.16f;       // how far it floats above where it rests
    static constexpr float PICKUP_RADIUS = 1.9f;
    static constexpr float COLLECT_RADIUS = 0.7f;
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
};
