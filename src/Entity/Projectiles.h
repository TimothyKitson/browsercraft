#pragma once
#include "Game/Items.h"
#include <glm/glm.hpp>
#include "Renderer/Mesh.h"
#include <vector>

class World;
class Shader;
class Atlas;
class Camera;

// Arrows in flight, from the player's bow and from a skeleton's.
//
// The flight itself is arithmetic -- a launch velocity, gravity, and a
// box to test against -- so the parts worth getting right are pure and
// reachable from --selftest without a world to shoot across.
namespace Arrows
{
    // How fast a fully drawn bow sends one, in blocks per second. A bow
    // drawn for less sends it slower and it drops sooner.
    constexpr float MAX_SPEED = 42.0f;
    constexpr float MIN_SPEED = 12.0f;
    constexpr float DRAW_SECONDS = 1.0f;
    constexpr float GRAVITY = -20.0f;

    // Arrows that hit nothing are cleared up rather than left to fall
    // for ever.
    constexpr float LIFETIME = 30.0f;

    // 0..1 for how long the bow has been held. A bow barely drawn is
    // worth almost nothing, which is what stops it being a machine gun.
    float drawFraction(float heldSeconds);
    float speedFor(float drawnFraction);

    // What a hit is worth. Minecraft scales the damage with the draw.
    int damageFor(float drawnFraction);

    // Whether a draw has gone on long enough to be worth loosing at all.
    inline bool worthLoosing(float drawnFraction) { return drawnFraction > 0.1f; }
}

struct Arrow
{
    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    float age = 0.0f;
    int damage = 1;
    bool fromPlayer = true;

    // Set once it has buried itself in a block: it stops moving and
    // hangs about for a while so you can see where it went.
    bool stuck = false;
};

class Projectiles
{
public:
    // What an arrow can hit besides a block. Projectiles knows nothing
    // about mobs or the player, so whoever owns them answers this --
    // the same arrangement the pathfinder uses for the ground.
    //
    // Return true to say the arrow struck something and should stop.
    struct Targets
    {
        virtual ~Targets() = default;

        // `point` is where the arrow is now; `fromPlayer` says who shot
        // it, so an arrow never hits whoever loosed it.
        virtual bool strike(const glm::vec3& point, int damage, bool fromPlayer) = 0;
    };

    // What the arrow flies through. Only one question is ever asked of
    // the world, so that is all this takes -- which lets --selftest
    // shoot across an empty sky with no world to build.
    struct Blocks
    {
        virtual ~Blocks() = default;
        virtual bool solidAt(int x, int y, int z) const = 0;
    };

    void spawn(const glm::vec3& from, const glm::vec3& velocity, int damage, bool fromPlayer);

    // Moves every arrow. A block stops one dead; anything living is
    // asked about through `targets`.
    void update(float deltaTime, const Blocks& blocks, Targets& targets);

    // The same, against a real world.
    void update(float deltaTime, const World& world, Targets& targets);

    void clear() { m_arrows.clear(); }
    const std::vector<Arrow>& all() const { return m_arrows; }

    void render(Shader& shader, const Camera& camera, const Atlas& atlas, const World& world);

private:
    std::vector<Arrow> m_arrows;
    std::vector<float> m_vertices;
    Mesh m_mesh;
};
