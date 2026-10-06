#include "Shader.h"
#include "Core/GLFunctions.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cstdio>

std::string Shader::readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Shader: could not open file " + path);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

unsigned int Shader::compile(const std::string& source, unsigned int type, const std::string& debugName)
{
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength > 0 ? logLength : 512);
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        throw std::runtime_error("Shader compile error (" + debugName + "): " + std::string(log.data()));
    }
    return shader;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertSrc = readFile(vertexPath);
    std::string fragSrc = readFile(fragmentPath);

    unsigned int vert = compile(vertSrc, GL_VERTEX_SHADER, vertexPath);
    unsigned int frag = compile(fragSrc, GL_FRAGMENT_SHADER, fragmentPath);

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);

    GLint success = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint logLength = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength > 0 ? logLength : 512);
        glGetProgramInfoLog(m_id, static_cast<GLsizei>(log.size()), nullptr, log.data());
        throw std::runtime_error("Shader link error: " + std::string(log.data()));
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

void Shader::bind() const
{
    glUseProgram(m_id);
}

int Shader::uniformLocation(const std::string& name)
{
    // Not caching locations: with only a handful of uniforms set once per
    // frame, the lookup cost doesn't matter yet. Worth caching in a
    // std::unordered_map<std::string,int> if you add many more uniforms.
    return glGetUniformLocation(m_id, name.c_str());
}

void Shader::setInt(const std::string& name, int value)
{
    glUniform1i(uniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value)
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3f(uniformLocation(name), value.x, value.y, value.z);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value)
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, &value[0][0]);
}
