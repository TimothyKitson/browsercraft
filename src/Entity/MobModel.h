#pragma once
#include "MobSkin.h"
#include "MobType.h"
#include "Renderer/Mesh.h"
#include <functional>
#include <memory>
#include <string>
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
    void build(const MobSkin& skin, const Mob& mob, float sky, float blockLight, int layer = 0,
               float extraInflate = 0.0f);
    const std::vector<float>& vertices() const { return m_vertices; }
    std::vector<std::pair<unsigned int, std::vector<float>>> figure(const Mob& mob);

    // Built on demand and kept; painting one costs well under a
    // millisecond and there are only ever eight.
    const MobSkin& skinFor(MobId id);

    std::function<int(const Mob&)> professionOf;
    std::function<int(const Mob&)> rankOf;
    ~MobModel();

private:
    unsigned int villagerTexture(const char* path);
    void renderExtras(MobId id, const MobSkin& skin, const World& world, const EntityManager& entities);
    std::string variantOf(const Mob& mob) const;
    std::vector<std::string> coatOverlaysOf(const Mob& mob) const;
    std::vector<std::pair<std::string, unsigned int>> m_villagerTextures;
    std::vector<std::unique_ptr<MobSkin>> m_skins;
    Mesh m_mesh;
    std::vector<float> m_vertices;
};
