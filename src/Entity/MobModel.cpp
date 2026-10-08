#include "MobModel.h"
#include "BoxMesh.h"
#include "EntityManager.h"
#include "Mob.h"
#include "PlayerAnimation.h"
#include "Core/GLFunctions.h"
#include "Renderer/Shader.h"
#include "World/World.h"
#include <cmath>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    // How far a limb swings at a full run, in radians. Less than the
    // player's: four short legs covering the same ground do not need to
    // reach as far as two long ones.
    constexpr float LEG_SWING = 0.9f;

    // A quadruped's diagonal pairs swing together, the way a real animal
    // walks. For the bipeds the arms are filed as the front pair, so the
    // same rule gives them an arm swinging opposite each leg.
    float swingFor(Part part, float phase, float amount)
    {
        switch (part)
        {
            case Part::LegFrontLeft:
            case Part::LegBackRight:
                return std::cos(phase) * LEG_SWING * amount;
            case Part::LegFrontRight:
            case Part::LegBackLeft:
                return std::cos(phase + PlayerAnimation::PI) * LEG_SWING * amount;
            default:
                return 0.0f;
        }
    }
}

const MobSkin& MobModel::skinFor(MobId id)
{
    if (m_skins.empty()) m_skins.resize(static_cast<size_t>(MobId::Count));

    const size_t index = static_cast<size_t>(id);
    if (!m_skins[index])
    {
        m_skins[index] = std::make_unique<MobSkin>();
        m_skins[index]->build(id);
    }

    return *m_skins[index];
}

void MobModel::build(const MobSkin& skin, const Mob& mob, float sky, float blockLight)
{
    const MobType& type = mob.type();

    BoxMesh::Frame frame;
    frame.feet = mob.position();
    frame.bodyYaw = mob.yaw();
    frame.sky = sky;
    frame.blockLight = blockLight;
    frame.textureWidth = static_cast<float>(skin.width());
    frame.textureHeight = static_cast<float>(skin.height());

    // A dead one keels over sideways rather than vanishing mid-step.
    const float fade = mob.deathFade();
    if (fade < 1.0f)
    {
        frame.jointAngle = 0.0f;
        frame.feet.y -= (1.0f - fade) * 0.15f;
    }

    const float phase = mob.gait();
    const float amount = mob.gaitAmount();

    std::vector<BoxMesh::Box> boxes;
    boxes.reserve(skin.boxes().size());

    for (const MobBox& source : skin.boxes())
    {
        BoxMesh::Box box;
        box.min = glm::vec3(source.origin) * mob.scale();
        box.max = glm::vec3(source.origin + source.size) * mob.scale();
        box.u = source.u;
        box.v = source.v;

        // Limbs hinge at their top, where they meet the body.
        box.pivot = glm::vec3(static_cast<float>(source.origin.x) + source.size.x * 0.5f,
                              static_cast<float>(source.origin.y + source.size.y),
                              static_cast<float>(source.origin.z) + source.size.z * 0.5f) * mob.scale();
        box.pitch = swingFor(source.part, phase, amount);

        boxes.push_back(box);
    }

    m_vertices.clear();
    BoxMesh::appendAll(m_vertices, boxes, frame);
}

void MobModel::render(Shader& chunkShader, const World& world, const EntityManager& entities)
{
    if (entities.mobs().empty()) return;

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f)); // already in world space

    // One pass per species, so each hide is bound once however many of
    // them are wandering about.
    for (int speciesIndex = 0; speciesIndex < mobTypeCount(); ++speciesIndex)
    {
        const MobId id = static_cast<MobId>(speciesIndex);

        bool any = false;
        for (const Mob& mob : entities.mobs())
            if (mob.typeId() == id) { any = true; break; }
        if (!any) continue;

        const MobSkin& skin = skinFor(id);
        if (!skin.textureId()) continue;

        std::vector<float> batch;

        for (const Mob& mob : entities.mobs())
        {
            if (mob.typeId() != id) continue;

            // One light sample per mob, taken at its middle and nudged
            // clear of anything solid -- read inside its own body, or
            // inside the floor, it comes out almost black.
            const glm::vec3 centre = mob.position();
            int lx = static_cast<int>(std::floor(centre.x));
            int ly = static_cast<int>(std::floor(centre.y + mob.height() * 0.5f));
            int lz = static_cast<int>(std::floor(centre.z));
            for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step)
                ++ly;

            float sky = static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f;
            float blockLight = static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f;

            // Flash while it is being hit, and dim as it dies.
            blockLight = std::min(1.0f, blockLight + mob.hurtFlash() * 1.6f);
            const float fade = mob.deathFade();
            sky *= fade;
            blockLight *= fade;

            build(skin, mob, sky, blockLight);
            batch.insert(batch.end(), m_vertices.begin(), m_vertices.end());
        }

        if (batch.empty()) continue;

        m_mesh.upload(batch, CHUNK_LAYOUT, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skin.textureId());
        m_mesh.draw(GL_TRIANGLES);
    }
}
