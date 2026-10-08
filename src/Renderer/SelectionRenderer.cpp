#include "SelectionRenderer.h"
#include "Atlas.h"
#include "Core/GLFunctions.h"
#include <vector>

#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif

namespace
{
    const std::vector<int> LINE_LAYOUT = { 3 };
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    void addLine(std::vector<float>& out, glm::vec3 a, glm::vec3 b)
    {
        out.insert(out.end(), { a.x, a.y, a.z, b.x, b.y, b.z });
    }
}

SelectionRenderer::SelectionRenderer()
    : m_lineShader("assets/shaders/line.vert", "assets/shaders/line.frag")
{
}

void SelectionRenderer::drawOutline(const glm::mat4& view, const glm::mat4& projection, const glm::ivec3& block)
{
    const glm::vec3 base(block);
    const float e = 0.002f; // nudge outwards so the lines don't z-fight
    const glm::vec3 lo = base - glm::vec3(e);
    const glm::vec3 hi = base + glm::vec3(1.0f + e);

    std::vector<float> vertices;
    vertices.reserve(24 * 3);

    const glm::vec3 corners[8] = {
        { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z }, { hi.x, lo.y, hi.z }, { lo.x, lo.y, hi.z },
        { lo.x, hi.y, lo.z }, { hi.x, hi.y, lo.z }, { hi.x, hi.y, hi.z }, { lo.x, hi.y, hi.z },
    };

    for (int i = 0; i < 4; ++i)
    {
        addLine(vertices, corners[i], corners[(i + 1) % 4]);         // bottom ring
        addLine(vertices, corners[4 + i], corners[4 + (i + 1) % 4]); // top ring
        addLine(vertices, corners[i], corners[4 + i]);               // vertical edge
    }

    m_lineMesh.upload(vertices, LINE_LAYOUT, true);

    m_lineShader.bind();
    m_lineShader.setMat4("uView", view);
    m_lineShader.setMat4("uProjection", projection);
    m_lineShader.setVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    m_lineMesh.draw(GL_LINES);
    glDisable(GL_BLEND);
}

void SelectionRenderer::drawAxes(const glm::mat4& view, const glm::mat4& projection,
                                 const glm::vec3& origin, float length)
{
    m_lineShader.bind();
    m_lineShader.setMat4("uView", view);
    m_lineShader.setMat4("uProjection", projection);

    const glm::vec3 directions[3] = {
        { length, 0.0f, 0.0f }, { 0.0f, length, 0.0f }, { 0.0f, 0.0f, length },
    };
    const glm::vec4 colours[3] = {
        { 1.0f, 0.25f, 0.25f, 1.0f },   // +X
        { 0.30f, 1.0f, 0.30f, 1.0f },   // +Y
        { 0.35f, 0.55f, 1.0f, 1.0f },   // +Z
    };

    // Drawn through the terrain, as in Minecraft -- an axis cross you
    // can only see in open air would be useless for orienting yourself.
    glDisable(GL_DEPTH_TEST);
    glLineWidth(3.0f);

    for (int axis = 0; axis < 3; ++axis)
    {
        std::vector<float> vertices;
        addLine(vertices, origin, origin + directions[axis]);
        m_lineMesh.upload(vertices, LINE_LAYOUT, true);
        m_lineShader.setVec4("uColor", colours[axis]);
        m_lineMesh.draw(GL_LINES);
    }

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

void SelectionRenderer::drawCrack(Shader& chunkShader, const glm::ivec3& block, int stage)
{
    if (stage < 0) return;
    if (stage >= Tiles::CrackStages) stage = Tiles::CrackStages - 1;

    const TileUV uv = tileUV(Tiles::CrackFirst + stage);

    // A unit cube in chunk-local space; uChunkOffset places it in the world.
    const float o = 0.003f; // slight inflation to sit just outside the block
    const float lo = -o;
    const float hi = 1.0f + o;

    struct Face { float x[4], y[4], z[4]; };
    const Face faces[6] = {
        { { hi, hi, hi, hi }, { lo, hi, hi, lo }, { lo, lo, hi, hi } }, // +X
        { { lo, lo, lo, lo }, { lo, lo, hi, hi }, { lo, hi, hi, lo } }, // -X
        { { lo, lo, hi, hi }, { hi, hi, hi, hi }, { lo, hi, hi, lo } }, // +Y
        { { lo, hi, hi, lo }, { lo, lo, lo, lo }, { lo, lo, hi, hi } }, // -Y
        { { lo, hi, hi, lo }, { lo, lo, hi, hi }, { hi, hi, hi, hi } }, // +Z
        { { lo, lo, hi, hi }, { lo, hi, hi, lo }, { lo, lo, lo, lo } }, // -Z
    };
    const float cornerU[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
    const float cornerV[4] = { 1.0f, 1.0f, 0.0f, 0.0f };

    std::vector<float> vertices;
    vertices.reserve(6 * 6 * 8);

    for (const Face& face : faces)
    {
        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        for (int i : order)
        {
            vertices.push_back(face.x[i]);
            vertices.push_back(face.y[i]);
            vertices.push_back(face.z[i]);
            vertices.push_back(cornerU[i] == 0.0f ? uv.u0 : uv.u1);
            vertices.push_back(cornerV[i] == 0.0f ? uv.vTop : uv.vBottom);
            vertices.push_back(1.0f); // shade
            vertices.push_back(1.0f); // skylight
            vertices.push_back(1.0f); // block light
            vertices.push_back(6.0f); // face 6 = skip normal mapping
        }
    }

    m_crackMesh.upload(vertices, CHUNK_LAYOUT, true);

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(block));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    glDisable(GL_CULL_FACE); // the overlay cube is drawn from both sides

    m_crackMesh.draw(GL_TRIANGLES);

    glEnable(GL_CULL_FACE);
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
}
