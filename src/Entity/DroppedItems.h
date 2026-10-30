#pragma once
#include "World/Block.h"
#include "Game/Items.h"
#include "Player/Inventory.h"
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

    // The same, for a whole stack: a dropped tool carries its wear and
    // its enchantments, and spawn() above would hand back a brand new
    // unenchanted one when you walked over it.
    void spawnStack(const glm::vec3& position, const ItemStack& stack);
    void spawnFromBrokenBlock(const glm::ivec3& blockPosition, StackId drop, int count = 1);

    void update(float deltaTime, const World& world, const Player& player,
                Inventory& inventory, AudioEngine& audio);

    // Rebuilds the geometry each frame: there are only ever a handful of
    // items, and they spin, so a dynamic buffer is simpler than instancing.
    void render(Shader& chunkShader, const World& world);

    // An item held in a frame on a wall. Not a drop: it does not
    // fall, does not merge, cannot be walked onto and never ages out.
    // It is drawn here because this is the one place an item is drawn
    // standing in the world, and a second copy of that would drift.
    //
    // Refilled every frame by whoever owns the frames, the way the
    // nameplates are: a frame can be written to, broken or loaded
    // between one frame and the next.
    struct Pinned
    {
        glm::vec3 position{ 0.0f };   // the middle of the frame's block
        int facing = 0;               // the ladder's four
        StackId item = Blocks::Air;
    };
    void clearPinned() { m_pinned.clear(); }
    void pin(const Pinned& one) { m_pinned.push_back(one); }

    void clear() { m_items.clear(); m_pinned.clear(); }
    int count() const { return static_cast<int>(m_items.size()); }

private:
    struct Item
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        StackId block = Blocks::Air;
        int count = 1;
        int damage = 0;
        uint32_t enchantments = 0;
        float age = 0.0f;
        float spin = 0.0f;
        bool resting = false;
    };

    std::vector<Item> m_items;
    std::vector<Pinned> m_pinned;
    Mesh m_mesh;
    std::vector<float> m_vertices;

    bool blocked(const World& world, const glm::vec3& centre) const;
    void mergeNearby();
};
