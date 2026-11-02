#pragma once
#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

class ShaderPipeline
{
public:
    struct Frame
    {
        glm::mat4 view{ 1.0f };
        glm::mat4 projection{ 1.0f };
        glm::vec3 cameraPosition{ 0.0f };
        glm::vec3 sunDirection{ 0.0f, 1.0f, 0.0f };
        float daylight = 1.0f;
        float time = 0.0f;
        float rain = 0.0f;
        int dimension = 0;
        bool underwater = false;
        bool inLava = false;
        float eyeLight = 1.0f;
        float eyeSkyLight = 1.0f;
        float renderDistance = 128.0f;
        float ambient = 0.07f;
        float nearPlane = 0.05f;
        float farPlane = 1000.0f;
        glm::vec3 backdrop{ 0.0f };
        bool pbr = false;
        float deltaTime = 0.016f;
    };

    ShaderPipeline();
    ~ShaderPipeline();

    ShaderPipeline(const ShaderPipeline&) = delete;
    ShaderPipeline& operator=(const ShaderPipeline&) = delete;

    bool ready() const { return m_ready; }
    const std::string& failure() const { return m_failure; }

    void prepare(int width, int height, const Frame& frame);

    bool shadowsWanted() const { return m_shadowStrength > 0.001f; }
    glm::vec3 shadowCentre() const { return m_shadowCentre; }
    float shadowReach() const { return SHADOW_REACH; }
    glm::vec3 lightDirection() const { return m_lightDirection; }

    Shader& shadowShader() { return *m_shadow; }
    Shader& worldShader() { return *m_world; }

    void beginShadows();
    void endShadows();
    void beginScene();
    void beginTransparent();
    void endTransparent();
    void beginHand();
    void finish();

    static constexpr float SHADOW_REACH = 96.0f;
    static constexpr int SHADOW_RESOLUTION = 2048;

private:
    bool m_ready = false;
    std::string m_failure;

    std::unique_ptr<Shader> m_shadow;
    std::unique_ptr<Shader> m_world;
    std::unique_ptr<Shader> m_sky;
    std::unique_ptr<Shader> m_composite;
    std::unique_ptr<Shader> m_bloomDown;
    std::unique_ptr<Shader> m_bloomUp;
    std::unique_ptr<Shader> m_grade;
    std::unique_ptr<Shader> m_smooth;

    Frame m_frame;
    int m_width = 0;
    int m_height = 0;
    bool m_halfFloat = true;

    glm::mat4 m_shadowViewProjection{ 1.0f };
    glm::vec3 m_shadowCentre{ 0.0f };
    glm::vec3 m_lightDirection{ 0.0f, 1.0f, 0.0f };
    glm::vec3 m_lightColour{ 1.0f };
    glm::vec3 m_skyAmbient{ 1.0f };
    float m_shadowStrength = 0.0f;
    float m_exposure = 1.0f;

    unsigned int m_emptyVao = 0;
    unsigned int m_shadowFbo = 0;
    unsigned int m_shadowDepth = 0;
    unsigned int m_sceneFbo = 0;
    unsigned int m_sceneColour = 0;
    unsigned int m_sceneDepth = 0;
    unsigned int m_copyFbo = 0;
    unsigned int m_copyColour = 0;
    unsigned int m_copyDepth = 0;
    unsigned int m_compositeFbo = 0;
    unsigned int m_compositeColour = 0;
    static constexpr int BLOOM_LEVELS = 5;
    unsigned int m_bloomFbo[BLOOM_LEVELS] = {};
    unsigned int m_bloomColour[BLOOM_LEVELS] = {};
    int m_bloomWidth[BLOOM_LEVELS] = {};
    int m_bloomHeight[BLOOM_LEVELS] = {};
    unsigned int m_gradeFbo = 0;
    unsigned int m_gradeColour = 0;
    unsigned int m_materials = 0;
    unsigned int m_noise = 0;

    void build();
    void createTargets(int width, int height);
    void destroyTargets();
    void createMaterials();
    void createNoise();
    void computeLight();
    void setWorldUniforms(int pass);
    void drawScreen();
    void copyScene(bool colour);
};
