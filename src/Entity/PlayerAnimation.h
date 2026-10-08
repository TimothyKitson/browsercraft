#pragma once
#include <algorithm>
#include <cmath>

// How a player model moves: the walk cycle and the way the body follows
// the head. Separated from the renderer so --selftest can check it
// without a window, and so the local player in third person and a remote
// player on the wire are animated by exactly the same code.
namespace PlayerAnimation
{
    constexpr float PI = 3.14159265358979f;

    // Folded into -180..180, which is where every angle comparison here
    // wants its input: yaw arrives off the network unbounded.
    inline float wrapDegrees(float degrees)
    {
        degrees = std::fmod(degrees + 180.0f, 360.0f);
        if (degrees < 0.0f) degrees += 360.0f;
        return degrees - 180.0f;
    }

    // The walk cycle advances by distance covered rather than by time,
    // so the feet keep step with the ground at any frame rate and a
    // sprinting player's legs go round faster. The constant is
    // Minecraft's: four phase units per block, scaled by its 0.6662.
    constexpr float PHASE_PER_BLOCK = 4.0f * 0.6662f;
    constexpr float WALK_SPEED = 4.3f;          // blocks per second
    constexpr float LEG_SWING = 1.4f;           // radians at a full run
    constexpr float ARM_SWING = 1.0f;

    inline float legAngle(float phase, float amount) { return std::cos(phase) * LEG_SWING * amount; }

    // The arms swing opposite the legs on the same side.
    inline float armAngle(float phase, float amount) { return std::cos(phase + PI) * ARM_SWING * amount; }

    // 0 standing, 1 at walking pace or faster. Eased rather than snapped
    // so stopping settles the legs instead of freezing them mid-stride.
    inline float approach(float current, float target, float rate, float deltaTime)
    {
        return current + (target - current) * std::clamp(rate * deltaTime, 0.0f, 1.0f);
    }

    inline float swingAmountFor(float horizontalSpeed)
    {
        return std::clamp(horizontalSpeed / WALK_SPEED, 0.0f, 1.0f);
    }

    // The neck only twists so far: past this the body comes round to
    // meet the head, which is what stops a player who spins on the spot
    // from looking over their own shoulder.
    constexpr float MAX_NECK_TWIST = 45.0f;

    inline float followHead(float bodyYaw, float headYaw, float deltaTime, bool moving)
    {
        bodyYaw += wrapDegrees(headYaw - bodyYaw) *
                   std::clamp((moving ? 12.0f : 3.0f) * deltaTime, 0.0f, 1.0f);

        const float twist = wrapDegrees(headYaw - bodyYaw);
        if (twist > MAX_NECK_TWIST) bodyYaw = headYaw - MAX_NECK_TWIST;
        else if (twist < -MAX_NECK_TWIST) bodyYaw = headYaw + MAX_NECK_TWIST;

        return wrapDegrees(bodyYaw);
    }
}
