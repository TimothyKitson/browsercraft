#pragma once
#include <cstddef>
#include <vector>

// A GPU vertex buffer with a caller-described attribute layout.
//
// upload() takes the interleaved float data plus the size of each attribute,
// so the same class serves chunk geometry (position.xyz, uv, shade, skylight,
// blocklight -- see assets/shaders/chunk.vert), the UI, and the selection
// outline without any of them agreeing on a vertex format.
//
// No index buffer is used on purpose: each visible block face is pushed as
// 6 raw vertices (2 triangles) by buildChunkMesh(). That wastes some vertex
// memory (shared corners are duplicated) but keeps mesh generation simple to
// read. Switching to indexed quads is a good later optimization.
class Mesh
{
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Uploads vertex data, replacing whatever was there before. `dynamic`
    // marks per-frame buffers, which reuse their allocation when the new
    // data still fits.
    void upload(const std::vector<float>& vertices,
                const std::vector<int>& attributeSizes,
                bool dynamic = false);

    void draw(unsigned int primitive) const;

    int vertexCount() const { return m_vertexCount; }
    bool empty() const { return m_vertexCount == 0; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    int m_vertexCount = 0;
    size_t m_capacityBytes = 0;

    void destroy();
};
