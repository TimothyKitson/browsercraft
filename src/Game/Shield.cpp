#include "Shield.h"
#include <algorithm>

bool Shield::covers(const glm::vec3& facing, const glm::vec3& defender,
                    const glm::vec3& attacker)
{
    // Flattened: a shield held against something standing above you
    // still covers you, and the pitch of the head should not decide
    // whether a blow lands.
    const glm::vec2 look(facing.x, facing.z);
    const glm::vec2 toward(attacker.x - defender.x, attacker.z - defender.z);

    const float lookLength = glm::length(look);
    const float towardLength = glm::length(toward);
    if (lookLength < 0.0001f || towardLength < 0.0001f) return false;

    return glm::dot(look / lookLength, toward / towardLength) >= COVER_COSINE;
}

int Shield::damageThrough(int incoming, bool blocked)
{
    if (!blocked) return incoming;
    return 0;
}
