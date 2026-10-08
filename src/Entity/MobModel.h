#pragma once
#include "MobSkin.h"
#include "MobType.h"
#include "Renderer/Mesh.h"
#include <memory>
#include <vector>

class Shader;
class World;
class EntityManager;
class Mob;

// Draws every mob, one species at a time.
//
// Each species carries its own painted hide, so the draw is batched per
// species rather than per mob: eight textures a frame at worst, however
// many animals are standing about.
class MobModel
{
public:
    void render(Shader& chunkShader, const World& world, const EntityManager& entities);

    // The geometry for one mob, in world space, with no GL involved --
    // which is what lets --selftest check the models stand on the ground
    // and face the way they are walking.
    void build(const MobSkin& skin, const Mob& mob, float sky, float blockLight);
    const std::vector<float>& vertices() const { return m_vertices; }

    // Built on demand and kept; painting one costs well under a
    // millisecond and there are only ever eight.
    const MobSkin& skinFor(MobId id);

private:
    std::vector<std::unique_ptr<MobSkin>> m_skins;
    Mesh m_mesh;
    std::vector<float> m_vertices;
};
