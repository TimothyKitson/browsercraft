#pragma once
#include <glm/glm.hpp>

class World;

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

// A physical player: gravity, jumping, walking, and axis-aligned bounding
// box collision against the voxel World. This is what makes movement feel
// like Minecraft survival mode instead of a noclip spectator camera.
//
// Application owns one of these and keeps a Camera in sync with its eye
// position purely for looking around (see Application::handleInput).
class Player
{
public:
    static constexpr float WIDTH = 0.6f;       // collision box width/depth
    static constexpr float HEIGHT = 1.8f;      // collision box height
    static constexpr float EYE_HEIGHT = 1.62f; // eye position above feet, Minecraft-like

    explicit Player(glm::vec3 spawnFeetPosition);

    // wishDirection: desired horizontal movement direction in world space
    // (XZ only -- Y is ignored while walking). jumpPressed should be true
    // only on the frame jump was first pressed (edge-triggered). When
    // `flying` is true, gravity/collision are skipped and the player moves
    // freely in 3D, using verticalFlyInput (-1..1) for up/down.
    void update(float deltaTime, const World& world, const glm::vec3& wishDirection,
                bool jumpPressed, bool flying, float verticalFlyInput);

    glm::vec3 eyePosition() const;
    AABB aabb() const { return aabbAt(position); }

    glm::vec3 position; // feet position: bottom-center of the collision box
    glm::vec3 velocity{ 0.0f };
    bool onGround = false;

private:
    AABB aabbAt(const glm::vec3& feetPosition) const;
    bool collides(const World& world, const AABB& box) const;

    // Moves `pos` by `delta` along one axis (0=x, 1=y, 2=z) in small steps,
    // stopping just before it would overlap a solid block. See the .cpp for
    // why stepping instead of solving the exact contact point directly.
    void moveAxis(const World& world, glm::vec3& pos, float delta, int axis);
};
