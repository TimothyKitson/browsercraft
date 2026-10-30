#include "Glide.h"
#include <algorithm>
#include <cmath>

bool Glide::holds(const Conditions& now)
{
    if (!now.wearingElytra || now.onGround || now.inWater || now.climbing ||
        now.flying)
        return false;

    if (now.alreadyGliding) return true;

    // Only on the way down, or near enough: pressing jump on the way
    // up out of a jump would turn every hop into a flight.
    return now.jumpPressed && now.velocityY < 1.0f;
}

glm::vec3 Glide::step(const glm::vec3& velocity, const glm::vec3& look, float deltaTime)
{
    glm::vec3 out = velocity;

    const float length = glm::length(look);
    const glm::vec3 heading = (length > 0.0001f) ? look / length
                                                 : glm::vec3(0.0f, 0.0f, -1.0f);

    const glm::vec2 flat(heading.x, heading.z);
    const float flatLength = glm::length(flat);

    float speed = glm::length(glm::vec2(out.x, out.z));

    // Nose down and you go faster; nose up and you trade it back.
    if (heading.y < 0.0f) speed += -heading.y * DIVE * deltaTime;
    else speed -= heading.y * CLIMB * deltaTime;
    speed = std::clamp(speed, MIN_SPEED, MAX_SPEED);

    if (flatLength > 0.01f)
    {
        const glm::vec2 along = flat / flatLength;
        out.x = along.x * speed;
        out.z = along.y * speed;
    }

    // A glide does not pile up downward speed the way a fall does: it
    // settles at the rate the wings allow, and the nose sets that
    // rate. Pointing down drops you fast, pointing up buys height
    // with the speed the climb is spending, and level sinks slowly.
    // Integrating gravity instead made a dive and a level glide lose
    // height at exactly the same rate, which is not a glide at all --
    // and the test said so the moment it was written.
    const float sink = GRAVITY + heading.y * speed * LIFT;
    out.y += (sink - out.y) * std::clamp(SETTLE * deltaTime, 0.0f, 1.0f);
    out.y = std::max(out.y, TERMINAL);

    return out;
}
