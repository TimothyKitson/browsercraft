#include "ShaderPipeline.h"
#include "Core/GLFunctions.h"
#include "World/Block.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include "Core/Packs.h"

namespace
{
    enum Material : uint8_t
    {
        SOLID = 0,
        PLANT = 1,
        LEAVES = 2,
        WATER = 3,
        LAVA = 4,
        GLASS = 5,
        ICE = 6,
        SNOW = 7,
        HANGING = 8,
        METAL = 9,
        ORE = 10,
        GLOWING = 11,
        POLISHED = 12,
        FLAME = 13,
        PORTAL = 14,
        FLOATING = 15,
        UNDERWATER_PLANT = 16,
        TALL_PLANT = 17
    };

    bool has(const std::string& name, const char* part)
    {
        return name.find(part) != std::string::npos;
    }

    struct Entry
    {
        uint8_t material = SOLID;
        uint8_t emission = 0;
        uint8_t smoothness = 0;
        uint8_t metalness = 0;
    };

    Entry classify(BlockId id)
    {
        const BlockInfo& info = blockInfo(id);
        const std::string name = info.name ? info.name : "";
        const int light = info.lightEmission;
        Entry e;

        if (info.render == RenderType::None) return e;

        if (isLiquid(id))
        {
            if (has(name, "Lava")) { e.material = LAVA; e.emission = 255; }
            else { e.material = WATER; e.smoothness = 255; }
            return e;
        }

        if (isLeaves(id)) { e.material = LEAVES; return e; }

        if (has(name, "Portal") && !has(name, "Frame")) { e.material = PORTAL; e.emission = 230; return e; }
        if (has(name, "End Gateway")) { e.material = PORTAL; e.emission = 230; return e; }

        if (has(name, "Lily Pad")) { e.material = FLOATING; return e; }
        if (has(name, "Kelp") || has(name, "Seagrass")) { e.material = UNDERWATER_PLANT; return e; }
        if (has(name, "Vines") || has(name, "Glow Berries"))
        {
            e.material = HANGING;
            if (light > 0) e.emission = static_cast<uint8_t>(light * 10);
            return e;
        }

        if (info.render == RenderType::Cross)
        {
            if (light > 0)
            {
                e.material = FLAME;
                e.emission = static_cast<uint8_t>(std::min(255, light * 17));
                return e;
            }
            if (has(name, "Sugar Cane") || has(name, "Bamboo") || has(name, "Dripstone") ||
                has(name, "Cobweb") || has(name, "Coral") || has(name, "Amethyst") ||
                has(name, "Rail") || has(name, "Redstone"))
                return e;
            e.material = PLANT;
            return e;
        }

        if (has(name, "Glass")) { e.material = GLASS; e.smoothness = 235; return e; }
        if (name == "Ice" || has(name, "Packed Ice") || has(name, "Blue Ice") || has(name, "Frosted Ice"))
        {
            e.material = ICE;
            e.smoothness = 220;
            return e;
        }
        if (name == "Snow" || has(name, "Snow Block") || has(name, "Powder Snow"))
        {
            e.material = SNOW;
            return e;
        }

        if (light >= 6 && info.render != RenderType::Liquid)
        {
            e.material = GLOWING;
            e.emission = static_cast<uint8_t>(std::min(255, light * 15));
            return e;
        }

        if ((has(name, "Ore") && !has(name, "Coal")) || has(name, "Ancient Debris") ||
            has(name, "Gilded"))
        {
            e.material = ORE;
            e.emission = 60;
            e.smoothness = 140;
            return e;
        }

        if (has(name, "Block of Iron") || has(name, "Block of Gold") ||
            has(name, "Block of Netherite") || has(name, "Copper") || has(name, "Anvil") ||
            has(name, "Hopper") || has(name, "Cauldron") || has(name, "Lantern") ||
            has(name, "Chain") || has(name, "Bell"))
        {
            e.material = METAL;
            e.smoothness = has(name, "Oxidized") ? 70 : has(name, "Weathered") ? 110 : 190;
            e.metalness = 220;
            return e;
        }

        if (has(name, "Block of Diamond") || has(name, "Block of Emerald") ||
            has(name, "Block of Lapis") || has(name, "Block of Amethyst") ||
            has(name, "Block of Redstone"))
        {
            e.material = POLISHED;
            e.smoothness = 200;
            e.metalness = 40;
            return e;
        }

        if (has(name, "Polished") || has(name, "Smooth") || has(name, "Quartz") ||
            has(name, "Prismarine") || has(name, "Purpur") || has(name, "Obsidian") ||
            has(name, "Calcite") || has(name, "Tiles") || has(name, "Terracotta") ||
            has(name, "Sea Lantern") || has(name, "Slime"))
        {
            e.material = POLISHED;
            e.smoothness = 140;
            return e;
        }

        if (has(name, "Tall Grass") || has(name, "Large Fern")) { e.material = TALL_PLANT; return e; }

        return e;
    }

    unsigned int makeTexture(int width, int height, GLenum internal, GLenum format, GLenum type,
                             GLenum filter)
    {
        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), width, height, 0, format,
                     type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return texture;
    }

    unsigned int makeFramebuffer(unsigned int colour, unsigned int depth)
    {
        unsigned int fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        if (colour)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colour, 0);
            const GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, buffers);
        }
        else
        {
            const GLenum none[1] = { GL_NONE };
            glDrawBuffers(1, none);
            glReadBuffer(GL_NONE);
        }
        if (depth)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
        return fbo;
    }

    bool complete()
    {
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    uint32_t hash(uint32_t x)
    {
        x ^= x >> 16;
        x *= 0x7feb352dU;
        x ^= x >> 15;
        x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    glm::vec3 mixColour(const glm::vec3& a, const glm::vec3& b, float t)
    {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }

    float smooth(float edge0, float edge1, float x)
    {
        const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
}

ShaderPipeline::ShaderPipeline()
{
    try
    {
        build();
        createMaterials();
        createNoise();
        createDither();
        for (int which = 0; which < 2; ++which)
        {
            int width = 0, height = 0, channels = 0;
            unsigned char* data = Packs::load(which == 0 ? "assets/textures/environment/sun.png"
                                                       : "assets/textures/environment/moon_phases.png",
                                            &width, &height, &channels, 4);
            if (!data) continue;
            unsigned int& texture = which == 0 ? m_sunTexture : m_moonTexture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glBindTexture(GL_TEXTURE_2D, 0);
            stbi_image_free(data);
        }
        glGenVertexArrays(1, &m_emptyVao);

        m_shadowDepth = makeTexture(SHADOW_RESOLUTION, SHADOW_RESOLUTION, GL_DEPTH_COMPONENT24,
                                    GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        m_shadowFbo = makeFramebuffer(0, m_shadowDepth);
        if (!complete()) throw std::runtime_error("the shadow map could not be made");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_ready = true;
    }
    catch (const std::exception& error)
    {
        m_failure = error.what();
        m_ready = false;
        std::printf("Shaders unavailable: %s\n", m_failure.c_str());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShaderPipeline::~ShaderPipeline()
{
    destroyTargets();
    if (m_shadowFbo) glDeleteFramebuffers(1, &m_shadowFbo);
    if (m_shadowDepth) glDeleteTextures(1, &m_shadowDepth);
    if (m_materials) glDeleteTextures(1, &m_materials);
    if (m_noise) glDeleteTextures(1, &m_noise);
    if (m_dither) glDeleteTextures(1, &m_dither);
    if (m_emptyVao) glDeleteVertexArrays(1, &m_emptyVao);
}

void ShaderPipeline::build()
{
    const std::string dir = "assets/shaders/fx/";
    m_shadow = std::make_unique<Shader>(dir + "shadow.vert", dir + "shadow.frag");
    m_world = std::make_unique<Shader>(dir + "world.vert", dir + "world.frag");
    m_sky = std::make_unique<Shader>(dir + "screen.vert", dir + "sky.frag");
    m_composite = std::make_unique<Shader>(dir + "screen.vert", dir + "composite.frag");
    m_bloomDown = std::make_unique<Shader>(dir + "screen.vert", dir + "bloom_down.frag");
    m_bloomUp = std::make_unique<Shader>(dir + "screen.vert", dir + "bloom_up.frag");
    m_grade = std::make_unique<Shader>(dir + "screen.vert", dir + "grade.frag");
    m_smooth = std::make_unique<Shader>(dir + "screen.vert", dir + "smooth.frag");
}

void ShaderPipeline::createMaterials()
{
    constexpr int WIDTH = 1024;
    std::vector<uint8_t> data(WIDTH * 4, 0);
    const int count = std::min<int>(WIDTH, static_cast<int>(Blocks::Count));
    for (int id = 0; id < count; ++id)
    {
        const Entry e = classify(static_cast<BlockId>(id));
        data[id * 4 + 0] = e.material;
        data[id * 4 + 1] = e.emission;
        data[id * 4 + 2] = e.smoothness;
        data[id * 4 + 3] = e.metalness;
    }

    glGenTextures(1, &m_materials);
    glBindTexture(GL_TEXTURE_2D, m_materials);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void ShaderPipeline::createNoise()
{
    constexpr int SIZE = 256;
    constexpr int CELLS[4] = { 8, 16, 32, 64 };
    std::vector<uint8_t> data(SIZE * SIZE * 4, 0);

    for (int channel = 0; channel < 4; ++channel)
    {
        const int cells = CELLS[channel];
        const float step = static_cast<float>(SIZE) / cells;
        auto lattice = [&](int x, int y) {
            x = (x % cells + cells) % cells;
            y = (y % cells + cells) % cells;
            return static_cast<float>(hash(static_cast<uint32_t>(x * 7919 + y * 104729 +
                                                                 channel * 1299709)) & 0xffffu) /
                   65535.0f;
        };

        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x)
            {
                const float fx = x / step;
                const float fy = y / step;
                const int ix = static_cast<int>(std::floor(fx));
                const int iy = static_cast<int>(std::floor(fy));
                float tx = fx - ix;
                float ty = fy - iy;
                tx = tx * tx * (3.0f - 2.0f * tx);
                ty = ty * ty * (3.0f - 2.0f * ty);
                const float a = lattice(ix, iy);
                const float b = lattice(ix + 1, iy);
                const float c = lattice(ix, iy + 1);
                const float d = lattice(ix + 1, iy + 1);
                const float value = (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
                data[(y * SIZE + x) * 4 + channel] = static_cast<uint8_t>(value * 255.0f + 0.5f);
            }
    }

    glGenTextures(1, &m_noise);
    glBindTexture(GL_TEXTURE_2D, m_noise);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void ShaderPipeline::paintDither(unsigned char out[16])
{
    static const int BASE[2][2] = { { 0, 2 }, { 3, 1 } };
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            out[y * 4 + x] = static_cast<unsigned char>((4 * BASE[y % 2][x % 2] + BASE[y / 2][x / 2]) * 17);
}

void ShaderPipeline::createDither()
{
    std::vector<unsigned char> pixels;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load("assets/textures/effect/dither.png", &width, &height, &channels, 4);
    m_ditherFromPack = data != nullptr;
    if (data)
    {
        pixels.assign(data, data + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        stbi_image_free(data);
    }
    else
    {
        unsigned char levels[16];
        paintDither(levels);
        width = height = 4;
        pixels.resize(64);
        for (int i = 0; i < 16; ++i)
        {
            pixels[static_cast<size_t>(i * 4)] = pixels[static_cast<size_t>(i * 4 + 1)] = pixels[static_cast<size_t>(i * 4 + 2)] = levels[i];
            pixels[static_cast<size_t>(i * 4 + 3)] = 255;
        }
    }
    glGenTextures(1, &m_dither);
    glBindTexture(GL_TEXTURE_2D, m_dither);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ShaderPipeline::destroyTargets()
{
    auto dropFbo = [](unsigned int& fbo) { if (fbo) glDeleteFramebuffers(1, &fbo); fbo = 0; };
    auto dropTex = [](unsigned int& tex) { if (tex) glDeleteTextures(1, &tex); tex = 0; };

    dropFbo(m_sceneFbo);
    dropTex(m_sceneColour);
    dropTex(m_sceneDepth);
    dropFbo(m_copyFbo);
    dropTex(m_copyColour);
    dropTex(m_copyDepth);
    dropFbo(m_compositeFbo);
    dropTex(m_compositeColour);
    for (int i = 0; i < BLOOM_LEVELS; ++i)
    {
        dropFbo(m_bloomFbo[i]);
        dropTex(m_bloomColour[i]);
    }
    dropFbo(m_gradeFbo);
    dropTex(m_gradeColour);
    m_width = 0;
    m_height = 0;
}

void ShaderPipeline::createTargets(int width, int height)
{
    destroyTargets();
    m_width = width;
    m_height = height;

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        const GLenum hdr = m_halfFloat ? GL_RGBA16F : GL_RGBA8;
        const GLenum hdrType = m_halfFloat ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE;

        m_sceneColour = makeTexture(width, height, hdr, GL_RGBA, hdrType, GL_LINEAR);
        m_sceneDepth = makeTexture(width, height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT,
                                   GL_UNSIGNED_INT, GL_NEAREST);
        m_sceneFbo = makeFramebuffer(m_sceneColour, m_sceneDepth);
        const bool sceneOk = complete();

        if (sceneOk || !m_halfFloat) break;

        destroyTargets();
        m_width = width;
        m_height = height;
        m_halfFloat = false;
    }

    const GLenum hdr = m_halfFloat ? GL_RGBA16F : GL_RGBA8;
    const GLenum hdrType = m_halfFloat ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE;

    m_copyColour = makeTexture(width, height, hdr, GL_RGBA, hdrType, GL_LINEAR);
    m_copyDepth = makeTexture(width, height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT,
                              GL_UNSIGNED_INT, GL_NEAREST);
    m_copyFbo = makeFramebuffer(m_copyColour, m_copyDepth);

    m_compositeColour = makeTexture(width, height, hdr, GL_RGBA, hdrType, GL_LINEAR);
    m_compositeFbo = makeFramebuffer(m_compositeColour, 0);

    int w = width;
    int h = height;
    for (int i = 0; i < BLOOM_LEVELS; ++i)
    {
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);
        m_bloomWidth[i] = w;
        m_bloomHeight[i] = h;
        m_bloomColour[i] = makeTexture(w, h, hdr, GL_RGBA, hdrType, GL_LINEAR);
        m_bloomFbo[i] = makeFramebuffer(m_bloomColour[i], 0);
    }

    m_gradeColour = makeTexture(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);
    m_gradeFbo = makeFramebuffer(m_gradeColour, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShaderPipeline::computeLight()
{
    const glm::vec3 sun = glm::normalize(m_frame.sunDirection);
    const bool sunUp = sun.y > -0.04f;
    m_lightDirection = sunUp ? sun : -sun;

    const float height = std::abs(m_lightDirection.y);
    const float rain = std::clamp(m_frame.rain, 0.0f, 1.0f);

    if (m_frame.dimension != 0)
    {
        m_shadowStrength = 0.0f;
        m_lightColour = glm::vec3(0.0f);
        m_skyAmbient = m_frame.dimension == 1 ? glm::vec3(0.62f, 0.34f, 0.26f)
                                              : glm::vec3(0.40f, 0.33f, 0.52f);
        return;
    }

    m_shadowStrength = smooth(0.0f, 0.12f, height) * (1.0f - rain * 0.85f);

    const float noon = smooth(0.05f, 0.45f, sun.y);
    const glm::vec3 sunset(1.00f, 0.52f, 0.24f);
    const glm::vec3 day(1.00f, 0.92f, 0.80f);
    const glm::vec3 moon(0.50f, 0.62f, 0.95f);

    if (sunUp)
        m_lightColour = mixColour(sunset * 0.95f, day * 1.10f, noon) * smooth(-0.04f, 0.10f, sun.y);
    else
        m_lightColour = moon * 0.11f * smooth(0.0f, 0.15f, height);

    const glm::vec3 skyDay(0.58f, 0.72f, 0.98f);
    const glm::vec3 skyDusk(0.62f, 0.48f, 0.55f);
    const glm::vec3 skyNight(0.08f, 0.11f, 0.22f);
    const float dusk = smooth(-0.15f, 0.15f, sun.y);
    m_skyAmbient = sun.y > 0.0f ? mixColour(skyDusk * 0.26f, skyDay * 0.44f, noon)
                                : mixColour(skyNight * 0.30f, skyDusk * 0.26f, dusk);

    const glm::vec3 grey(0.55f, 0.58f, 0.62f);
    const float grey_luma = 0.3f * m_lightColour.r + 0.59f * m_lightColour.g + 0.11f * m_lightColour.b;
    m_lightColour = mixColour(m_lightColour, grey * grey_luma, rain * 0.6f) * (1.0f - rain * 0.55f);
    const float ambientLuma = 0.3f * m_skyAmbient.r + 0.59f * m_skyAmbient.g + 0.11f * m_skyAmbient.b;
    m_skyAmbient = mixColour(m_skyAmbient, grey * ambientLuma, rain * 0.7f) * (1.0f - rain * 0.3f);
}

void ShaderPipeline::prepare(int width, int height, const Frame& frame)
{
    if (!m_ready) return;
    m_frame = frame;
    if (width != m_width || height != m_height) createTargets(width, height);
    computeLight();

    const float target = std::clamp(1.0f / (0.38f + 0.62f * std::clamp(frame.eyeLight, 0.0f, 1.0f)),
                                    1.0f, 2.4f);
    const float rate = 1.0f - std::exp(-frame.deltaTime * 1.6f);
    m_exposure += (target - m_exposure) * rate;

    const float snap = 2.0f;
    m_shadowCentre = glm::vec3(std::floor(frame.cameraPosition.x / snap) * snap,
                               std::floor(frame.cameraPosition.y / snap) * snap,
                               std::floor(frame.cameraPosition.z / snap) * snap);

    const glm::vec3 up = std::abs(m_lightDirection.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f)
                                                              : glm::vec3(0.0f, 1.0f, 0.0f);
    const float depthReach = 256.0f;
    const glm::mat4 lightView = glm::lookAt(m_shadowCentre + m_lightDirection * (depthReach * 0.5f),
                                            m_shadowCentre, up);
    const glm::mat4 lightProjection = glm::ortho(-SHADOW_REACH, SHADOW_REACH, -SHADOW_REACH,
                                                 SHADOW_REACH, 0.0f, depthReach);
    m_shadowViewProjection = lightProjection * lightView;
}

void ShaderPipeline::beginShadows()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);
    glViewport(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_shadow->bind();
    m_shadow->setMat4("uShadowViewProj", m_shadowViewProjection);
    m_shadow->setInt("uAtlas", 0);
    m_shadow->setInt("uMaterials", 3);
    m_shadow->setFloat("uTime", m_frame.time);
    m_shadow->setFloat("uRain", m_frame.rain);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, m_materials);
    glActiveTexture(GL_TEXTURE0);
}

void ShaderPipeline::endShadows()
{
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShaderPipeline::setWorldUniforms(int pass)
{
    Shader& s = *m_world;
    s.bind();
    s.setMat4("uView", m_frame.view);
    s.setMat4("uProjection", m_frame.projection);
    s.setMat4("uShadowViewProj", m_shadowViewProjection);
    s.setInt("uAtlas", 0);
    s.setInt("uNormalAtlas", 1);
    s.setInt("uSpecularAtlas", 2);
    s.setInt("uMaterials", 3);
    s.setInt("uShadowMap", 4);
    s.setInt("uNoise", 5);
    s.setInt("uSceneCopy", 6);
    s.setInt("uDepthCopy", 7);
    s.setVec3("uCameraPos", m_frame.cameraPosition);
    s.setVec3("uSunDirection", glm::normalize(m_frame.sunDirection));
    s.setVec3("uLightDir", m_lightDirection);
    s.setVec3("uLightColour", m_lightColour);
    s.setVec3("uSkyAmbient", m_skyAmbient);
    s.setFloat("uShadowStrength", m_shadowStrength);
    s.setFloat("uDaylight", m_frame.dimension == 0 ? m_frame.daylight : 0.0f);
    s.setFloat("uAmbient", m_frame.ambient);
    s.setFloat("uRain", m_frame.rain);
    s.setFloat("uTime", m_frame.time);
    s.setInt("uDimension", m_frame.dimension);
    s.setInt("uPass", pass);
    s.setInt("uUnderwater", m_frame.underwater ? 1 : 0);
    s.setInt("uPbrEnabled", m_frame.pbr ? 1 : 0);
    s.setVec2("uScreenSize", glm::vec2(static_cast<float>(m_width), static_cast<float>(m_height)));
    s.setFloat("uNear", m_frame.nearPlane);
    s.setFloat("uFar", m_frame.farPlane);
    s.setVec3("uChunkOffset", glm::vec3(0.0f));

    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, m_materials);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, m_shadowDepth);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_2D, m_noise);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, m_copyColour);
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, m_copyDepth);
    glActiveTexture(GL_TEXTURE0);
}

void ShaderPipeline::drawScreen()
{
    glBindVertexArray(m_emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void ShaderPipeline::beginScene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
    glViewport(0, 0, m_width, m_height);

    const glm::vec3 back = m_frame.dimension == 0 ? glm::vec3(0.0f)
                                                  : glm::pow(m_frame.backdrop, glm::vec3(2.2f));
    glClearColor(back.r, back.g, back.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_frame.dimension == 0)
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_sky->bind();
        m_sky->setMat4("uInvViewProj", glm::inverse(m_frame.projection * m_frame.view));
        m_sky->setVec3("uCameraPos", m_frame.cameraPosition);
        m_sky->setVec3("uSunDirection", glm::normalize(m_frame.sunDirection));
        m_sky->setFloat("uTime", m_frame.time);
        m_sky->setFloat("uRain", m_frame.rain);
        m_sky->setVec3("uLightColour", m_lightColour);
        m_sky->setVec3("uSkyAmbient", m_skyAmbient);
        m_sky->setInt("uNoise", 5);
        m_sky->setInt("uMoonPhase", ((m_frame.moonPhase % 8) + 8) % 8);
        m_sky->setInt("uHasSun", m_sunTexture ? 1 : 0);
        m_sky->setInt("uHasMoon", m_moonTexture ? 1 : 0);
        m_sky->setInt("uSunTex", 6);
        m_sky->setInt("uMoonTex", 7);
        glActiveTexture(GL_TEXTURE0 + 5);
        glBindTexture(GL_TEXTURE_2D, m_noise);
        glActiveTexture(GL_TEXTURE0 + 6);
        glBindTexture(GL_TEXTURE_2D, m_sunTexture);
        glActiveTexture(GL_TEXTURE0 + 7);
        glBindTexture(GL_TEXTURE_2D, m_moonTexture);
        glActiveTexture(GL_TEXTURE0);
        drawScreen();
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }

    setWorldUniforms(0);
}

void ShaderPipeline::copyScene(bool colour)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sceneFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_copyFbo);
    if (colour)
        glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
}

void ShaderPipeline::beginTransparent()
{
    copyScene(true);
    setWorldUniforms(1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
}

void ShaderPipeline::endTransparent()
{
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void ShaderPipeline::beginHand()
{
    copyScene(false);
    glClear(GL_DEPTH_BUFFER_BIT);
    setWorldUniforms(2);
}

void ShaderPipeline::finish()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    const glm::mat4 invProjection = glm::inverse(m_frame.projection);
    const glm::mat4 invView = glm::inverse(m_frame.view);

    glBindFramebuffer(GL_FRAMEBUFFER, m_compositeFbo);
    glViewport(0, 0, m_width, m_height);
    m_composite->bind();
    m_composite->setInt("uScene", 0);
    m_composite->setInt("uDepth", 1);
    m_composite->setInt("uHandDepth", 2);
    m_composite->setInt("uShadowMap", 4);
    m_composite->setInt("uNoise", 5);
    m_composite->setMat4("uInvProjection", invProjection);
    m_composite->setMat4("uInvView", invView);
    m_composite->setMat4("uShadowViewProj", m_shadowViewProjection);
    m_composite->setVec3("uCameraPos", m_frame.cameraPosition);
    m_composite->setVec3("uSunDirection", glm::normalize(m_frame.sunDirection));
    m_composite->setVec3("uLightDir", m_lightDirection);
    m_composite->setVec3("uLightColour", m_lightColour);
    m_composite->setVec3("uSkyAmbient", m_skyAmbient);
    m_composite->setVec3("uBackdrop", glm::pow(m_frame.backdrop, glm::vec3(2.2f)));
    m_composite->setFloat("uShadowStrength", m_shadowStrength);
    m_composite->setFloat("uRain", m_frame.rain);
    m_composite->setFloat("uTime", m_frame.time);
    m_composite->setFloat("uRenderDistance", m_frame.renderDistance);
    m_composite->setFloat("uEyeSkyLight", m_frame.eyeSkyLight);
    m_composite->setInt("uDimension", m_frame.dimension);
    m_composite->setInt("uUnderwater", m_frame.underwater ? 1 : 0);
    m_composite->setInt("uInLava", m_frame.inLava ? 1 : 0);
    m_composite->setVec2("uScreenSize", glm::vec2(static_cast<float>(m_width),
                                                  static_cast<float>(m_height)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneColour);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, m_copyDepth);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, m_sceneDepth);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, m_shadowDepth);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_2D, m_noise);
    drawScreen();

    unsigned int source = m_compositeColour;
    int sourceW = m_width;
    int sourceH = m_height;
    m_bloomDown->bind();
    m_bloomDown->setInt("uSource", 0);
    for (int i = 0; i < BLOOM_LEVELS; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[i]);
        glViewport(0, 0, m_bloomWidth[i], m_bloomHeight[i]);
        m_bloomDown->setVec2("uTexel", glm::vec2(1.0f / sourceW, 1.0f / sourceH));
        m_bloomDown->setInt("uFirst", i == 0 ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, source);
        drawScreen();
        source = m_bloomColour[i];
        sourceW = m_bloomWidth[i];
        sourceH = m_bloomHeight[i];
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    m_bloomUp->bind();
    m_bloomUp->setInt("uSource", 0);
    for (int i = BLOOM_LEVELS - 1; i > 0; --i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[i - 1]);
        glViewport(0, 0, m_bloomWidth[i - 1], m_bloomHeight[i - 1]);
        m_bloomUp->setVec2("uTexel", glm::vec2(1.0f / m_bloomWidth[i], 1.0f / m_bloomHeight[i]));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_bloomColour[i]);
        drawScreen();
    }
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, m_gradeFbo);
    glViewport(0, 0, m_width, m_height);
    m_grade->bind();
    m_grade->setInt("uScene", 0);
    m_grade->setInt("uBloom", 1);
    m_grade->setFloat("uExposure", m_exposure);
    m_grade->setFloat("uTime", m_frame.time);
    m_grade->setInt("uUnderwater", m_frame.underwater ? 1 : 0);
    m_grade->setInt("uDimension", m_frame.dimension);
    m_grade->setFloat("uRain", m_frame.rain);
    m_grade->setInt("uDither", 2);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, m_dither);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_compositeColour);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, m_bloomColour[0]);
    drawScreen();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    m_smooth->bind();
    m_smooth->setInt("uImage", 0);
    m_smooth->setVec2("uTexel", glm::vec2(1.0f / m_width, 1.0f / m_height));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_gradeColour);
    drawScreen();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
}
