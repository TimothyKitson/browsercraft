#pragma once
#include <vector>

// A GPU vertex buffer for chunk geometry. Each vertex is 6 floats:
//   position.xyz, texCoord.uv, brightness
// (see assets/shaders/chunk.vert for the matching attribute layout).
//
// No index buffer is used on purpose: each visible block face is pushed as
// 6 raw vertices (2 triangles) by Chunk::generateMesh(). That wastes some
// vertex memory (shared corners are duplicated) but keeps mesh generation
// simple to read. Switching to indexed quads is a good later optimization
// once you understand how this version works.
class Mesh
{
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Uploads vertex data, replacing whatever was there before.
    void upload(const std::vector<float>& vertices);

    void draw() const;

    int vertexCount() const { return m_vertexCount; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    int m_vertexCount = 0;

    void destroy();
};
