#pragma once
#include "Mesh.h"
#include <glm/glm.hpp>
#include <vector>

class EntityManager;
class Shader;
class World;

// Draws every mob as a small stack of textured boxes.
//
// Mob geometry uses the same vertex layout as chunk geometry, so it goes
// through the chunk shader and the block atlas rather than needing a pipeline
// of its own. All the mobs in view are packed into one dynamic mesh each
// frame, which is cheap at the few dozen boxes this ever amounts to.
class MobRenderer
{
public:
    void render(const EntityManager& entities, const World& world, Shader& chunkShader);

private:
    void appendBox(std::vector<float>& out, const glm::vec3& centre, const glm::vec3& halfSize,
                   float yaw, const glm::vec3& origin, int tile, float sky, float block, float tint);

    Mesh m_mesh;
    std::vector<float> m_vertices;
};
