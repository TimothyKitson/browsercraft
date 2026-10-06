#pragma once
#include <glm/glm.hpp>

class World;

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

// A physical player: gravity, jumping, walking, swimming, and axis-aligned
// bounding box collision against the voxel World. This is what makes movement
// feel like Minecraft survival mode instead of a noclip spectator camera.
//
// Application owns one of these and keeps a Camera in sync with its eye
// position purely for looking around (see Application::handleInput).
class Player
{
public:
    static constexpr float WIDTH = 0.6f;             // collision box width/depth
    static constexpr float HEIGHT = 1.8f;            // collision box height
    static constexpr float EYE_HEIGHT = 1.62f;       // eye above feet, Minecraft-like
    static constexpr float SNEAK_EYE_HEIGHT = 1.54f; // crouched eye height
    static constexpr int MAX_HEALTH = 20;            // ten hearts

    // One frame of intent, filled in by Application from the keymap so the
    // player has no idea which physical keys are bound to what.
    struct Controls
    {
        glm::vec3 wishDirection{ 0.0f }; // desired horizontal direction, XZ only
        bool jump = false;               // edge-triggered: true only on the press
        bool jumpHeld = false;           // held: swim up, or fly up
        bool sneak = false;
        bool sprint = false;
        bool descend = false;            // fly down
    };

    explicit Player(glm::vec3 feetPosition);

    void update(float deltaTime, const World& world, const Controls& controls);

    glm::vec3 eyePosition() const;
    AABB aabb() const { return aabbAt(position); }

    bool isInWater(const World& world) const;
    bool isHeadUnderwater(const World& world) const;

    void damage(int amount);
    void heal(int amount);
    void respawn(glm::vec3 feetPosition);

    glm::vec3 position;              // feet position: bottom-center of the box
    glm::vec3 velocity{ 0.0f };
    bool onGround = false;
    bool flying = false;
    bool creative = false;
    bool sneaking = false;
    bool sprinting = false;
    int health = MAX_HEALTH;
    float damageFlash = 0.0f;        // seconds left on the red hurt overlay

private:
    AABB aabbAt(const glm::vec3& feetPosition) const;
    bool collides(const World& world, const AABB& box) const;
    bool supported(const World& world, const glm::vec3& feetPosition) const;

    // Moves along one axis (0=x, 1=y, 2=z) in small steps, stopping just
    // before it would overlap a solid block. See the .cpp for why stepping
    // instead of solving the exact contact point directly.
    void moveAxis(const World& world, float delta, int axis, bool preventLedgeFall);
    void applyFallDamage(float landingY);

    bool m_falling = false;
    float m_fallStartY = 0.0f;
};
