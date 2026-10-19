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
    // What is in the air. An arrow drops and sticks; a blaze's charge
    // flies flat and sets what it hits alight; a wind charge flies flat
    // and bursts, shoving everything near it.
    enum class Kind : uint8_t { Shaft, Fire, Wind };

    // How far a wind charge's burst reaches, and how hard it pushes at
    // the middle of it. Pure arithmetic over two points, so --selftest
    // can check the shove without anything to shove.
    constexpr float WIND_RADIUS = 3.5f;
    constexpr float WIND_PUSH = 14.0f;

    glm::vec3 windImpulse(const glm::vec3& burst, const glm::vec3& target);

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

    // A blaze's fire charge, or a wind charge, rather than an arrow:
    // both fly flat and burst against whatever they hit instead of
    // sticking.
    Arrows::Kind kind = Arrows::Kind::Shaft;

    bool flies() const { return kind != Arrows::Kind::Shaft; }
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

        // The same, for something that is not a plain arrow. Defaulted,
        // so anything that only cares about arrows -- which is most of
        // what shoots at a target -- needs the one above.
        virtual bool strike(const glm::vec3& point, int damage, bool fromPlayer,
                            Arrows::Kind kind)
        {
            (void)kind;
            return strike(point, damage, fromPlayer);
        }

        // Where a charge went off, whether or not it hit anything. A
        // wind charge does most of its work here rather than on impact:
        // what it is for is the shove, and the shove is round the
        // burst, not on whatever stopped it.
        virtual void burst(const glm::vec3& at, Arrows::Kind kind, bool fromPlayer)
        {
            (void)at; (void)kind; (void)fromPlayer;
        }
    };

    // What the arrow flies through. Only one question is ever asked of
    // the world, so that is all this takes -- which lets --selftest
    // shoot across an empty sky with no world to build.
    struct Blocks
    {
        virtual ~Blocks() = default;
        virtual bool solidAt(int x, int y, int z) const = 0;
    };

    void spawn(const glm::vec3& from, const glm::vec3& velocity, int damage, bool fromPlayer,
               Arrows::Kind kind = Arrows::Kind::Shaft);

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
