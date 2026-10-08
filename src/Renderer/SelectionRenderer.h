#pragma once
#include "Shader.h"
#include "Mesh.h"
#include <glm/glm.hpp>

// Draws the wireframe box around the block you're looking at, plus the
// progressively-cracking overlay while you mine it.
class SelectionRenderer
{
public:
    SelectionRenderer();

    void drawOutline(const glm::mat4& view, const glm::mat4& projection, const glm::ivec3& block);

    // The red/green/blue axis cross Minecraft plants at your feet while
    // the debug screen is up: red is +X, green is +Y, blue is +Z.
    void drawAxes(const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& origin, float length = 1.0f);

    // Reuses the chunk shader (already bound with view/projection/atlas by
    // the caller) so the crack tiles sample the same atlas.
    void drawCrack(Shader& chunkShader, const glm::ivec3& block, int stage);

private:
    Shader m_lineShader;
    Mesh m_lineMesh;
    Mesh m_crackMesh;
};
