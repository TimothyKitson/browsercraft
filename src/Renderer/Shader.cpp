#include "Shader.h"
#include "Core/GLFunctions.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

std::string Shader::readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Shader: could not open " + path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

namespace
{
    // The shaders are written once as desktop GLSL 3.30. WebGL2 speaks
    // GLSL ES 3.00 instead, which is the same language apart from the
    // version line and mandatory precision qualifiers -- so patch those in
    // rather than maintaining two copies of every shader.
    std::string adaptForPlatform(const std::string& source)
    {
#ifdef __EMSCRIPTEN__
        const std::string desktopVersion = "#version 330 core";
        const std::string esHeader =
            "#version 300 es\n"
            "precision highp float;\n"
            "precision highp int;\n"
            "precision highp sampler2D;\n";

        const size_t at = source.find(desktopVersion);
        if (at == std::string::npos) return source;
        return esHeader + source.substr(at + desktopVersion.size());
#else
        return source;
#endif
    }
}

unsigned int Shader::compile(const std::string& rawSource, unsigned int type, const std::string& debugName)
{
    const std::string source = adaptForPlatform(rawSource);

    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 0 ? len : 512);
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        throw std::runtime_error("Shader compile error (" + debugName + "): " + log.data());
    }
    return shader;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    unsigned int vert = compile(readFile(vertexPath), GL_VERTEX_SHADER, vertexPath);
    unsigned int frag = compile(readFile(fragmentPath), GL_FRAGMENT_SHADER, fragmentPath);

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);

    GLint success = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint len = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 0 ? len : 512);
        glGetProgramInfoLog(m_id, static_cast<GLsizei>(log.size()), nullptr, log.data());
        throw std::runtime_error("Shader link error (" + vertexPath + "): " + log.data());
    }

    glDetachShader(m_id, vert);
    glDetachShader(m_id, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
}

Shader::~Shader()
{
    if (m_id) glDeleteProgram(m_id);
}

void Shader::bind() const { glUseProgram(m_id); }

int Shader::uniformLocation(const std::string& name)
{
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) return it->second;
    int loc = glGetUniformLocation(m_id, name.c_str());
    m_uniformCache.emplace(name, loc);
    return loc;
}

void Shader::setInt(const std::string& n, int v) { glUniform1i(uniformLocation(n), v); }
void Shader::setFloat(const std::string& n, float v) { glUniform1f(uniformLocation(n), v); }
void Shader::setVec2(const std::string& n, const glm::vec2& v) { glUniform2f(uniformLocation(n), v.x, v.y); }
void Shader::setVec3(const std::string& n, const glm::vec3& v) { glUniform3f(uniformLocation(n), v.x, v.y, v.z); }
void Shader::setVec4(const std::string& n, const glm::vec4& v) { glUniform4f(uniformLocation(n), v.x, v.y, v.z, v.w); }
void Shader::setMat4(const std::string& n, const glm::mat4& v) { glUniformMatrix4fv(uniformLocation(n), 1, GL_FALSE, &v[0][0]); }
