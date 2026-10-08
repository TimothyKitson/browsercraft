#include "Mesh.h"
#include "Core/GLFunctions.h"

Mesh::~Mesh() { destroy(); }

void Mesh::destroy()
{
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vbo = 0;
    m_vao = 0;
    m_vertexCount = 0;
    m_capacityBytes = 0;
}

void Mesh::upload(const std::vector<float>& vertices, const std::vector<int>& attributeSizes, bool dynamic)
{
    int floatsPerVertex = 0;
    for (int size : attributeSizes) floatsPerVertex += size;
    if (floatsPerVertex == 0) return;

    m_vertexCount = static_cast<int>(vertices.size()) / floatsPerVertex;

    if (m_vao == 0)
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    const size_t bytes = vertices.size() * sizeof(float);
    const GLenum usage = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    if (dynamic && bytes <= m_capacityBytes && bytes > 0)
    {
        // Reuse the existing allocation for per-frame buffers (UI, outlines).
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes), vertices.data());
    }
    else
    {
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bytes),
                     vertices.empty() ? nullptr : vertices.data(), usage);
        m_capacityBytes = bytes;
    }

    const GLsizei stride = floatsPerVertex * static_cast<GLsizei>(sizeof(float));
    size_t offset = 0;
    for (size_t i = 0; i < attributeSizes.size(); ++i)
    {
        glVertexAttribPointer(static_cast<GLuint>(i), attributeSizes[i], GL_FLOAT, GL_FALSE,
                              stride, reinterpret_cast<void*>(offset * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(i));
        offset += attributeSizes[i];
    }

    glBindVertexArray(0);
}

void Mesh::draw(unsigned int primitive) const
{
    if (m_vertexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawArrays(primitive, 0, m_vertexCount);
    glBindVertexArray(0);
}
