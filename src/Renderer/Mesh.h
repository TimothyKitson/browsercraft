#pragma once
#include <vector>

// A VAO + VBO pair holding interleaved float vertices. The attribute layout
// is described by a list of component counts, e.g. {3, 2, 1, 1, 1} means
// location 0 is a vec3, location 1 a vec2, locations 2-4 floats.
class Mesh
{
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void upload(const std::vector<float>& vertices, const std::vector<int>& attributeSizes, bool dynamic = false);
    void draw(unsigned int primitive) const;

    int vertexCount() const { return m_vertexCount; }
    bool empty() const { return m_vertexCount == 0; }
    void destroy();

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    int m_vertexCount = 0;
    size_t m_capacityBytes = 0;
};
