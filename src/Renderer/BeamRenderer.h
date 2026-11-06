#pragma once
#include "Mesh.h"
#include <glm/glm.hpp>
#include <vector>

class Shader;

class BeamRenderer
{
public:
    enum class Look
    {
        Beacon,
        BeaconHaze,
        Gateway,
        Crystal,
        GuardianDim,
        GuardianWarm,
        GuardianHot,
        GuardianFull,
        Ray,
        Lightning,
        Count
    };

    ~BeamRenderer();

    void clear();
    void beam(Look look, const glm::vec3& from, const glm::vec3& to, float radius, float scroll, float spin);
    void render(Shader& shader);
    bool empty() const;

private:
    unsigned int m_textures[static_cast<int>(Look::Count)] = {};
    bool m_loaded = false;
    std::vector<float> m_batches[static_cast<int>(Look::Count)];
    Mesh m_mesh;

    void load();
};
