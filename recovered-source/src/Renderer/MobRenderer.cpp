#include "MobRenderer.h"
#include "Atlas.h"
#include "Core/GLFunctions.h"
#include "Entity/EntityManager.h"
#include "Shader.h"
#include "World/World.h"
#include <cmath>

namespace
{
    // Same layout the chunk mesher emits: position, uv, shade, skylight, blocklight.
    const std::vector<int> MOB_LAYOUT = { 3, 2, 1, 1, 1 };

    // Fixed per-face brightness, matching the block mesher so a mob standing in
    // a chunk is lit like the blocks around it.
    constexpr float FACE_SHADE[6] = { 1.0f, 0.5f, 0.8f, 0.8f, 0.6f, 0.6f };

    // Corner offsets for the six faces of a unit cube centred on the origin,
    // in the order +Y, -Y, +X, -X, +Z, -Z.
    const glm::vec3 FACE_CORNERS[6][4] = {
        { { -1,  1, -1 }, {  1,  1, -1 }, {  1,  1,  1 }, { -1,  1,  1 } },
        { { -1, -1,  1 }, {  1, -1,  1 }, {  1, -1, -1 }, { -1, -1, -1 } },
        { {  1, -1,  1 }, {  1,  1,  1 }, {  1,  1, -1 }, {  1, -1, -1 } },
        { { -1, -1, -1 }, { -1,  1, -1 }, { -1,  1,  1 }, { -1, -1,  1 } },
        { { -1, -1,  1 }, { -1,  1,  1 }, {  1,  1,  1 }, {  1, -1,  1 } },
        { {  1, -1, -1 }, {  1,  1, -1 }, { -1,  1, -1 }, { -1, -1, -1 } },
    };
}

void MobRenderer::appendBox(std::vector<float>& out, const glm::vec3& centre, const glm::vec3& halfSize,
                            float yaw, const glm::vec3& origin, int tile, float sky, float block, float tint)
{
    const TileUV uv = tileUV(tile);
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);

    auto place = [&](const glm::vec3& corner) {
        const glm::vec3 local = centre + corner * halfSize;
        // Rotate about the mob's own vertical axis, then move into the world.
        return glm::vec3(
            origin.x + local.x * cosYaw + local.z * sinYaw,
            origin.y + local.y,
            origin.z - local.x * sinYaw + local.z * cosYaw);
    };

    for (int face = 0; face < 6; ++face)
    {
        const float shade = FACE_SHADE[face] * tint;

        const glm::vec3 p0 = place(FACE_CORNERS[face][0]);
        const glm::vec3 p1 = place(FACE_CORNERS[face][1]);
        const glm::vec3 p2 = place(FACE_CORNERS[face][2]);
        const glm::vec3 p3 = place(FACE_CORNERS[face][3]);

        const glm::vec3 quad[6] = { p0, p1, p2, p0, p2, p3 };
        const float us[6] = { uv.u0, uv.u1, uv.u1, uv.u0, uv.u1, uv.u0 };
        const float vs[6] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vBottom, uv.vTop, uv.vTop };

        for (int i = 0; i < 6; ++i)
        {
            out.push_back(quad[i].x);
            out.push_back(quad[i].y);
            out.push_back(quad[i].z);
            out.push_back(us[i]);
            out.push_back(vs[i]);
            out.push_back(shade);
            out.push_back(sky);
            out.push_back(block);
        }
    }
}

void MobRenderer::render(const EntityManager& entities, const World& world, Shader& chunkShader)
{
    m_vertices.clear();

    for (const Mob& mob : entities.mobs())
    {
        const MobType& type = mob.type();
        const glm::vec3 origin = mob.position();

        // One light sample for the whole mob: they are small enough that
        // per-box sampling would cost more than it shows.
        const int lx = static_cast<int>(std::floor(origin.x));
        const int ly = static_cast<int>(std::floor(origin.y + type.height * 0.5f));
        const int lz = static_cast<int>(std::floor(origin.z));
        const float sky = world.skyLight(lx, ly, lz) / 15.0f;
        const float block = world.blockLightAt(lx, ly, lz) / 15.0f;

        // Flash white-hot briefly when hurt, and sink into the ground on death.
        const float tint = 1.0f + mob.hurtFlash() * 1.6f;

        float sink = 0.0f;
        if (!mob.alive()) sink = -(1.0f - std::max(0.0f, mob.hurtFlash())) * 0.0f;

        // Legs swing fore and aft as the mob walks.
        const float swing = std::sin(mob.gait()) * 0.35f;

        int boxIndex = 0;
        for (const ModelBox& box : type.model)
        {
            glm::vec3 centre = box.center;
            centre.y += sink;

            // Boxes after the body and head are legs: rock them around their top.
            if (boxIndex >= 2)
            {
                const float direction = (boxIndex % 2 == 0) ? 1.0f : -1.0f;
                centre.z += swing * direction * box.halfSize.y;
            }

            appendBox(m_vertices, centre, box.halfSize, mob.yaw(), origin, box.tile, sky, block, tint);
            ++boxIndex;
        }
    }

    if (m_vertices.empty()) return;

    m_mesh.upload(m_vertices, MOB_LAYOUT, true);

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));
    m_mesh.draw(GL_TRIANGLES);
}
