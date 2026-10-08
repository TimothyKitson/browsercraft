#pragma once
#include "Game/GameMode.h"
#include <glm/glm.hpp>

class World;

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

// The physical player: gravity, jumping, sprinting, sneaking, swimming,
// fall damage and axis-aligned collision against the voxel grid.
class Player
{
public:
    static constexpr float WIDTH = 0.6f;
    static constexpr float HEIGHT = 1.8f;
    static constexpr float EYE_HEIGHT = 1.62f;
    static constexpr float SNEAK_EYE_HEIGHT = 1.45f;
    static constexpr int MAX_HEALTH = 20;

    struct Controls
    {
        glm::vec3 wishDirection{ 0.0f }; // horizontal, world space, unit length
        bool jump = false;               // edge-triggered
        bool jumpHeld = false;
        bool sprint = false;
        bool sneak = false;
        bool descend = false;            // flying only
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

    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    bool onGround = false;
    bool flying = false;
    bool sprinting = false;
    bool sneaking = false;
    GameMode mode = GameMode::Survival;
    int health = MAX_HEALTH;

    bool creative() const { return modeIsCreative(mode); }
    float damageFlash = 0.0f; // seconds remaining on the red hurt overlay

private:
    float m_fallStartY = 0.0f;
    bool m_falling = false;
    float m_regenTimer = 0.0f;

    AABB aabbAt(const glm::vec3& feetPosition) const;
    bool collides(const World& world, const AABB& box) const;
    bool supported(const World& world, const glm::vec3& feetPosition) const;
    void moveAxis(const World& world, float delta, int axis, bool preventLedgeFall);
    void applyFallDamage(float landingY);
};
