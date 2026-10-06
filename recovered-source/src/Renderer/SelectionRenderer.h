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

    // Reuses the chunk shader (already bound with view/projection/atlas by
    // the caller) so the crack tiles sample the same atlas.
    void drawCrack(Shader& chunkShader, const glm::ivec3& block, int stage);

private:
    Shader m_lineShader;
    Mesh m_lineMesh;
    Mesh m_crackMesh;
};
