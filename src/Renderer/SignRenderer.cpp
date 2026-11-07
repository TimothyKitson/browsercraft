#include "SignRenderer.h"
#include "Core/GLFunctions.h"
#include "Shader.h"
#include "World/World.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };
    constexpr float QUARTER = 1.5707964f;

    BoxMesh::Box part(int u, int v, glm::vec3 from, glm::vec3 size, glm::vec3 pivot = glm::vec3(0.0f), float yaw = 0.0f)
    {
        BoxMesh::Box box;
        box.min = glm::vec3(-(pivot.x + from.x + size.x), 24.0f - (pivot.y + from.y + size.y), -(pivot.z + from.z + size.z));
        box.max = box.min + size;
        box.pivot = glm::vec3(-pivot.x, 24.0f - pivot.y, -pivot.z);
        box.u = u;
        box.v = v;
        box.mirror = true;
        box.yaw = yaw;
        return box;
    }

    const glm::vec3 WOOD_RGB[WOOD_COUNT] = {
        { 162, 130, 78 }, { 114, 84, 48 }, { 196, 178, 122 }, { 160, 115, 80 }, { 168, 90, 50 }, { 66, 43, 20 },
        { 226, 178, 172 }, { 117, 54, 48 }, { 194, 174, 82 }, { 101, 48, 70 }, { 43, 104, 99 },
    };
}

SignRenderer::~SignRenderer()
{
    for (unsigned int texture : m_sheets)
        if (texture) glDeleteTextures(1, &texture);
    for (unsigned int texture : m_gui)
        if (texture) glDeleteTextures(1, &texture);
}

const char* SignRenderer::woodName(int wood)
{
    static const char* NAMES[WOOD_COUNT] = { "oak", "spruce", "birch", "jungle", "acacia", "dark_oak",
                                             "cherry", "mangrove", "bamboo", "crimson", "warped" };
    return NAMES[std::clamp(wood, 0, WOOD_COUNT - 1)];
}

unsigned int SignRenderer::load(const std::string& path, int wood, bool hanging, int width, int height)
{
    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    std::vector<uint8_t> painted;
    if (!data || w <= 0 || h <= 0)
    {
        if (data) stbi_image_free(data);
        data = nullptr;
        w = width;
        h = height;
        painted.assign(static_cast<size_t>(w * h * 4), 0);
        const glm::vec3 base = WOOD_RGB[std::clamp(wood, 0, WOOD_COUNT - 1)];
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const float grain = static_cast<float>(((x * 7 + y * 31) * 2654435761u) >> 29) * 4.0f;
                const bool chain = hanging && y >= 6 && y < 12 && x < 26 && height == 32;
                const glm::vec3 c = chain ? glm::vec3(80.0f, 82.0f, 92.0f) : glm::clamp(base + glm::vec3(grain - 12.0f), 0.0f, 255.0f);
                uint8_t* p = &painted[static_cast<size_t>((y * w + x) * 4)];
                p[0] = static_cast<uint8_t>(c.r);
                p[1] = static_cast<uint8_t>(c.g);
                p[2] = static_cast<uint8_t>(c.b);
                p[3] = 255;
            }
    }
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data ? data : painted.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (data) stbi_image_free(data);
    return texture;
}

unsigned int SignRenderer::sheet(int wood, bool hanging)
{
    wood = std::clamp(wood, 0, WOOD_COUNT - 1);
    const size_t index = static_cast<size_t>(wood * 2 + (hanging ? 1 : 0));
    if (m_tried[index]) return m_sheets[index];
    m_tried[index] = true;
    const std::string path = hanging ? "assets/textures/entity/signs/hanging/" + std::string(woodName(wood)) + ".png"
                                     : "assets/textures/entity/signs/" + std::string(woodName(wood)) + ".png";
    m_sheets[index] = load(path, wood, hanging, 64, 32);
    return m_sheets[index];
}

unsigned int SignRenderer::editorBackground(int wood)
{
    wood = std::clamp(wood, 0, WOOD_COUNT - 1);
    const size_t index = static_cast<size_t>(wood);
    if (m_guiTried[index]) return m_gui[index];
    m_guiTried[index] = true;
    m_gui[index] = load("assets/textures/gui/hanging_signs/" + std::string(woodName(wood)) + ".png", wood, false, 16, 16);
    return m_gui[index];
}

std::vector<BoxMesh::Box> SignRenderer::boxesFor(BlockId block, bool attached)
{
    std::vector<BoxMesh::Box> boxes;
    if (isHangingSign(block))
    {
        boxes.push_back(part(0, 12, { -7.0f, 0.0f, -1.0f }, { 14.0f, 10.0f, 2.0f }));
        if (isWallHangingSign(block)) boxes.push_back(part(0, 0, { -8.0f, -6.0f, -2.0f }, { 16.0f, 2.0f, 4.0f }));
        if (isCeilingHangingSign(block) && attached)
            boxes.push_back(part(14, 6, { -6.0f, -6.0f, 0.0f }, { 12.0f, 6.0f, 0.0f }));
        else
        {
            boxes.push_back(part(0, 6, { -1.5f, 0.0f, 0.0f }, { 3.0f, 6.0f, 0.0f }, { -5.0f, -6.0f, 0.0f }, -QUARTER * 0.5f));
            boxes.push_back(part(6, 6, { -1.5f, 0.0f, 0.0f }, { 3.0f, 6.0f, 0.0f }, { -5.0f, -6.0f, 0.0f }, QUARTER * 0.5f));
            boxes.push_back(part(0, 6, { -1.5f, 0.0f, 0.0f }, { 3.0f, 6.0f, 0.0f }, { 5.0f, -6.0f, 0.0f }, -QUARTER * 0.5f));
            boxes.push_back(part(6, 6, { -1.5f, 0.0f, 0.0f }, { 3.0f, 6.0f, 0.0f }, { 5.0f, -6.0f, 0.0f }, QUARTER * 0.5f));
        }
        return boxes;
    }
    boxes.push_back(part(0, 0, { -12.0f, -14.0f, -1.0f }, { 24.0f, 12.0f, 2.0f }));
    if (isStandingSign(block)) boxes.push_back(part(0, 14, { -1.0f, -2.0f, -1.0f }, { 2.0f, 14.0f, 2.0f }));
    return boxes;
}

glm::vec3 SignRenderer::facingVector(BlockId block)
{
    const float yaw = glm::radians(static_cast<float>(signRotation(block)) * 22.5f + 90.0f);
    return glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw));
}

BoxMesh::Frame SignRenderer::frameFor(const glm::ivec3& at, BlockId block)
{
    BoxMesh::Frame frame;
    const glm::vec3 forward = facingVector(block);
    frame.bodyYaw = static_cast<float>(signRotation(block)) * 22.5f + 90.0f;
    frame.textureWidth = 64.0f;
    frame.textureHeight = 32.0f;
    const glm::vec3 bottom = glm::vec3(at) + glm::vec3(0.5f, 0.0f, 0.5f);
    if (isHangingSign(block))
    {
        frame.scale = 1.0f;
        frame.feet = bottom - glm::vec3(0.0f, 0.875f, 0.0f);
    }
    else
    {
        frame.scale = 2.0f / 3.0f;
        frame.feet = bottom - glm::vec3(0.0f, 0.5f, 0.0f);
        if (isWallSign(block)) frame.feet += glm::vec3(0.0f, -0.3125f, 0.0f) - forward * 0.4375f;
    }
    return frame;
}

glm::vec3 SignRenderer::textAnchor(const glm::ivec3& at, BlockId block)
{
    const glm::vec3 forward = facingVector(block);
    const glm::vec3 bottom = glm::vec3(at) + glm::vec3(0.5f, 0.0f, 0.5f);
    if (isHangingSign(block)) return bottom + glm::vec3(0.0f, 0.3125f, 0.0f) + forward * 0.07f;
    if (isWallSign(block)) return bottom + glm::vec3(0.0f, 0.52f, 0.0f) - forward * 0.39f;
    return bottom + glm::vec3(0.0f, 0.833f, 0.0f) + forward * 0.05f;
}

void SignRenderer::render(Shader& shader, const World& world, const std::vector<glm::ivec3>& signs)
{
    if (signs.empty()) return;
    std::vector<std::vector<float>> batches(static_cast<size_t>(WOOD_COUNT * 2));
    for (const glm::ivec3& at : signs)
    {
        const BlockId block = world.getBlock(at.x, at.y, at.z);
        if (!isSign(block)) continue;
        const BlockId above = world.getBlock(at.x, at.y + 1, at.z);
        const bool attached = isCeilingHangingSign(block) && !(isSolid(above) && isOpaque(above));
        BoxMesh::Frame frame = frameFor(at, block);
        frame.sky = static_cast<float>(world.skyLight(at.x, at.y, at.z)) / 15.0f;
        frame.blockLight = static_cast<float>(world.blockLightAt(at.x, at.y, at.z)) / 15.0f;
        const size_t index = static_cast<size_t>(signWood(block) * 2 + (isHangingSign(block) ? 1 : 0));
        BoxMesh::appendAll(batches[index], boxesFor(block, attached), frame);
    }

    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);
    for (size_t i = 0; i < batches.size(); ++i)
    {
        if (batches[i].empty()) continue;
        const unsigned int texture = sheet(static_cast<int>(i / 2), (i % 2) == 1);
        if (!texture) continue;
        m_mesh.upload(batches[i], CHUNK_LAYOUT, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        m_mesh.draw(GL_TRIANGLES);
    }
    glEnable(GL_CULL_FACE);
}
