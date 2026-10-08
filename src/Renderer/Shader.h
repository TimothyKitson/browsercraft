#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

// Compiles + links a GLSL vertex/fragment pair from disk and caches uniform
// locations.
class Shader
{
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void bind() const;

    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const glm::vec2& value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setMat4(const std::string& name, const glm::mat4& value);

private:
    unsigned int m_id = 0;
    std::unordered_map<std::string, int> m_uniformCache;

    int uniformLocation(const std::string& name);
    static std::string readFile(const std::string& path);
    static unsigned int compile(const std::string& rawSource, unsigned int type, const std::string& debugName);
};
