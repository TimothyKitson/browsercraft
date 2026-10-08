#pragma once
#include "World/Block.h"
#include "Renderer/Mesh.h"
#include <vector>
#include <glm/glm.hpp>

class World;
class Shader;

// Camera-facing textured sprites, used for the debris that flies off a
// block as you mine it, the burst when it finally breaks, and the puff
// under your feet when you land. Each particle samples a small patch of
// the source block's own texture, so debris always matches the material.
class Particles
{
public:
    void spawnBlockBreak(const glm::ivec3& blockPosition, BlockId block);
    void spawnBlockHit(const glm::vec3& position, const glm::ivec3& faceNormal, BlockId block);
    void spawnLandingPuff(const glm::vec3& feetPosition, BlockId ground);

    void update(float deltaTime, const World& world);
    void render(Shader& chunkShader, const World& world,
                const glm::vec3& cameraRight, const glm::vec3& cameraUp);

    void clear() { m_particles.clear(); }
    int count() const { return static_cast<int>(m_particles.size()); }

private:
    struct Particle
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        float size = 0.1f;
        float age = 0.0f;
        float lifetime = 1.0f;
        float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f; // patch of the block texture
        bool resting = false;
    };

    std::vector<Particle> m_particles;
    Mesh m_mesh;
    std::vector<float> m_vertices;
    uint32_t m_random = 1u;

    float nextRandom();
    void add(const glm::vec3& position, const glm::vec3& velocity, BlockId block,
             float size, float lifetime);
};
