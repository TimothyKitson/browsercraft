#pragma once
#include <algorithm>
#include "Game/Effects.h"
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

        // Where the camera is pointing, for the one thing that cares:
        // a glide goes where you look rather than where you walk.
        glm::vec3 look{ 0.0f, 0.0f, -1.0f };
        bool wearingElytra = false;
    };

    explicit Player(glm::vec3 feetPosition);

    void update(float deltaTime, const World& world, const Controls& controls);

    glm::vec3 eyePosition() const;
    AABB aabb() const { return aabbAt(position); }

    bool isInWater(const World& world) const;

    // Lava burns rather than drowns. It reads as a liquid everywhere
    // else -- it slows you down and you sink in it -- but what it does
    // to the player is its own thing.
    bool isInLava(const World& world) const;

    // Standing in a campfire, which burns you the way lava does but
    // for walking over rather than falling in.
    bool isInFire(const World& world) const;

    // Four a go, twice a second, which is Minecraft's. Fire resistance
    // takes all of it: swimming lava is the whole point of the drink.
    static constexpr int LAVA_DAMAGE = 4;
    static constexpr float LAVA_INTERVAL = 0.5f;

    // Returns true on the step a burn lands.
    bool burn(float deltaTime, bool inLava);

    // Set alight for a while: a blaze's charge, or climbing out of lava
    // still burning. One damage a second until it goes out, which is
    // Minecraft's -- fire out of lava is a nuisance, lava is a death.
    static constexpr int FIRE_DAMAGE = 1;
    static constexpr float FIRE_INTERVAL = 1.0f;
    static constexpr float LAVA_FIRE_SECONDS = 7.0f;

    void ignite(float seconds);
    void douse() { m_fireSeconds = 0.0f; m_fireTimer = 0.0f; }
    bool onFire() const { return m_fireSeconds > 0.0f; }
    float fireSeconds() const { return m_fireSeconds; }

    // Whether the player is up against a ladder. Climbing is the one
    // thing that overrides gravity without flying.
    bool onLadder(const World& world) const;
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

    // Air, and running out of it. Fifteen seconds' worth, as in
    // Minecraft, then two points -- one heart -- a second until you
    // surface or die. It refills far faster than it empties, so a dive
    // costs you the dive and not the next one as well.
    static constexpr float MAX_AIR_SECONDS = 15.0f;
    static constexpr float AIR_REFILL_RATE = 5.0f;   // times as fast as it empties
    static constexpr int DROWN_DAMAGE = 2;
    static constexpr float DROWN_INTERVAL = 1.0f;

    float air = MAX_AIR_SECONDS;

    // What is running on the player right now. Kept here beside the
    // health and the hunger because it changes all three, and because
    // --selftest can set it directly and watch what happens.
    Effects::Set effects;

    // Runs the effect clocks and applies what they pay out. Separate
    // from updateVitals so a test can step the effects without also
    // running hunger down. Returns true if a pulse changed the health.
    bool updateEffects(float deltaTime);

    // Takes whether the head is under, rather than a World, so the rule
    // can be run through a whole dive in --selftest without one.
    // Returns true on the step a lungful runs out and a blow lands.
    bool breathe(float deltaTime, bool headUnderwater);

    void addExhaustion(float amount);
    bool eat(int hungerPoints, float saturationPoints);
    bool canEat() const { return hunger < MAX_HUNGER; }

    // Hunger, healing and starvation. Called by update, and separately
    // by --selftest, which has no world to walk about in.
    void updateVitals(float deltaTime);

    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    bool onGround = false;
    // Holding a shield up. Set from what is in hand and whether Use is
    // down; read by the movement, which slows, and by whoever applies
    // a blow, which is where it actually stops anything.
    bool blocking = false;
    bool flying = false;

    // In the air on a pair of wings. Minecraft's glide, cut to what a
    // player can feel: you fall slowly, you go where you are looking,
    // and pointing the nose down trades height for speed.
    bool gliding = false;
    bool sprinting = false;
    bool sneaking = false;
    GameMode mode = GameMode::Survival;
    int health = MAX_HEALTH;

    // What is being worn, totted up from the armour slots once a frame.
    // Kept here rather than read from the inventory so that damage() --
    // the one place a blow is ever applied -- stays the one place the
    // rules about a blow live, and --selftest can set it directly.
    int armourPoints = 0;

    // The best protection across the suit, totted up once a frame
    // beside the points, and for the same reason: damage() is the one
    // place a blow is applied, so it is the one place the rules about
    // a blow should live.
    uint32_t protection = 0;
    int hunger = MAX_HUNGER;
    float saturation = 5.0f;

    // Experience, as one running total rather than a level and a bar:
    // the level and the bar are both functions of it, so keeping the
    // three in step is a job that cannot be got wrong if there is only
    // one of them. Experience.h turns it into either.
    //
    // Smelting pays out in fractions of a point, so what the furnace
    // owes is kept here in hundredths until it crosses a whole one.
    int experience = 0;
    int experienceOwedHundredths = 0;

    // Awards experience, and returns true if that crossed into a new
    // level, which is what the level-up chime hangs off.
    bool addExperience(int amount);

    // Pays in hundredths of a point, holding back the remainder.
    bool addExperienceHundredths(int hundredths);

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

    // Start the fall again from here. A wind charge that throws you
    // clear has taken the fall over, and what it cost before the shove
    // is not yours to pay on landing.
    void clearFall() { m_fallStartY = position.y; }

    // How far this fall has gone so far, which is what a mace is paid
    // on. Zero on the ground: a mace swung standing still is just a
    // heavy club.
    float fallenSoFar() const
    {
        return m_falling ? std::max(0.0f, m_fallStartY - position.y) : 0.0f;
    }

private:
    float m_fallStartY = 0.0f;
    bool m_falling = false;
    bool m_jumped = false;
    float m_regenTimer = 0.0f;
    float m_hurtCooldown = 0.0f;
    float m_exhaustion = 0.0f;
    float m_starveTimer = 0.0f;
    float m_drownTimer = 0.0f;
    float m_burnTimer = 0.0f;
    float m_fireSeconds = 0.0f;
    float m_fireTimer = 0.0f;
    glm::vec3 m_lastPosition{ 0.0f };

    AABB aabbAt(const glm::vec3& feetPosition) const;
    bool collides(const World& world, const AABB& box) const;
    bool stepUp(const World& world, const glm::vec3& blocked);
    bool supported(const World& world, const glm::vec3& feetPosition) const;
    void moveAxis(const World& world, float delta, int axis, bool preventLedgeFall);
    void applyFallDamage(float landingY);
    void regenerate(float deltaTime);
};
