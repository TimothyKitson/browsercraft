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

    // Returns true when the blow actually landed. Half a second of
    // invulnerability follows each one, the way Minecraft does it --
    // without it a mob standing in your face deals its damage once per
    // frame and kills you instantly.
    static constexpr float HURT_IMMUNITY = 0.5f;
    bool damage(int amount);
    bool invulnerable() const { return m_hurtCooldown > 0.0f; }
    void heal(int amount);
    void respawn(glm::vec3 feetPosition);

    static constexpr int MAX_HUNGER = 20;
    static constexpr float EXHAUSTION_PER_DRAIN = 4.0f;
    static constexpr int SPRINT_HUNGER = 7;
    static constexpr int REGEN_HUNGER = 18;

    void addExhaustion(float amount);
    bool eat(int hungerPoints, float saturationPoints);
    bool canEat() const { return hunger < MAX_HUNGER; }

    // Hunger, healing and starvation. Called by update, and separately
    // by --selftest, which has no world to walk about in.
    void updateVitals(float deltaTime);

    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    bool onGround = false;
    bool flying = false;
    bool sprinting = false;
    bool sneaking = false;
    GameMode mode = GameMode::Survival;
    int health = MAX_HEALTH;

    // What is being worn, totted up from the armour slots once a frame.
    // Kept here rather than read from the inventory so that damage() --
    // the one place a blow is ever applied -- stays the one place the
    // rules about a blow live, and --selftest can set it directly.
    int armourPoints = 0;
    int hunger = MAX_HUNGER;
    float saturation = 5.0f;

    bool creative() const { return modeIsCreative(mode); }

    // True for the one update in which the player left the ground under
    // their own power. The audio reads this rather than the jump key,
    // which is held down for many frames and is down just as often when
    // the jump does not happen at all.
    bool jumpedThisUpdate() const { return m_jumped; }

    // Drops the half second of immunity after a blow. Only --selftest
    // wants this: it lands a run of blows with no clock to wait on.
    void clearHurtCooldown() { m_hurtCooldown = 0.0f; }
    float damageFlash = 0.0f; // seconds remaining on the red hurt overlay

private:
    float m_fallStartY = 0.0f;
    bool m_falling = false;
    bool m_jumped = false;
    float m_regenTimer = 0.0f;
    float m_hurtCooldown = 0.0f;
    float m_exhaustion = 0.0f;
    float m_starveTimer = 0.0f;
    glm::vec3 m_lastPosition{ 0.0f };

    AABB aabbAt(const glm::vec3& feetPosition) const;
    bool collides(const World& world, const AABB& box) const;
    bool supported(const World& world, const glm::vec3& feetPosition) const;
    void moveAxis(const World& world, float delta, int axis, bool preventLedgeFall);
    void applyFallDamage(float landingY);
    void regenerate(float deltaTime);
};
