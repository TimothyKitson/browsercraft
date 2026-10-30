#pragma once
#include <glm/glm.hpp>

// The elytra: Minecraft's glide, cut down to what a player can feel.
//
// You fall slowly, you go where you are looking, and pointing the nose
// down trades height for speed. That exchange is the whole of how a
// glide is flown, and it is arithmetic -- so it lives here, pure, and
// --selftest can fly a whole descent without a world to fly it over.
namespace Glide
{
    // How fast a level glide sinks, and the fastest a dive may drop.
    constexpr float GRAVITY = -3.2f;
    constexpr float TERMINAL = -24.0f;

    // How much of the speed being carried the nose turns into climb
    // or dive, and how quickly the sink rate catches up with it. The
    // second one is what keeps a glide from snapping between
    // attitudes the moment the view moves.
    constexpr float LIFT = 1.1f;
    constexpr float SETTLE = 6.0f;
    constexpr float DIVE = 22.0f;     // how fast a dive builds speed
    constexpr float CLIMB = 14.0f;    // and how fast a climb bleeds it
    constexpr float MIN_SPEED = 4.0f;
    constexpr float MAX_SPEED = 26.0f;

    // Everything that decides whether a glide is on. Jumping in the
    // air with a working pair of wings starts one; the ground, water,
    // a ladder, creative flight or taking them off ends one.
    struct Conditions
    {
        bool wearingElytra = false;
        bool onGround = false;
        bool inWater = false;
        bool climbing = false;
        bool flying = false;
        bool jumpPressed = false;
        bool alreadyGliding = false;
        float velocityY = 0.0f;
    };
    bool holds(const Conditions& now);

    // One step. `look` is where the camera points and need not be
    // normalised; the velocity that comes back is the whole of the
    // glide's motion, horizontal and vertical together.
    glm::vec3 step(const glm::vec3& velocity, const glm::vec3& look, float deltaTime);
}
