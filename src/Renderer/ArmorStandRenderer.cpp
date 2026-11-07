#include "ArmorStandRenderer.h"
#include "Core/GLFunctions.h"
#include "Game/Armour.h"
#include "Game/Items.h"
#include "Game/Smithing.h"
#include "Shader.h"
#include "World/World.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    BoxMesh::Box part(int u, int v, glm::vec3 from, glm::vec3 size, glm::vec3 pivot, const glm::vec3& degrees,
                      bool mirror = false, float inflate = 0.0f)
    {
        BoxMesh::Box box;
        box.min = glm::vec3(-(pivot.x + from.x + size.x), 24.0f - (pivot.y + from.y + size.y), -(pivot.z + from.z + size.z));
        box.max = box.min + size;
        box.pivot = glm::vec3(-pivot.x, 24.0f - pivot.y, -pivot.z);
        box.u = u;
        box.v = v;
        box.mirror = mirror;
        box.inflate = inflate;
        box.pitch = glm::radians(degrees.x);
        box.yaw = glm::radians(degrees.y);
        box.roll = glm::radians(degrees.z);
        return box;
    }

    unsigned int upload(const uint8_t* pixels, int width, int height)
    {
        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return texture;
    }

    unsigned int painted(int width, int height, uint8_t r, uint8_t g, uint8_t b)
    {
        std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 4));
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                const int n = static_cast<int>((static_cast<uint32_t>(x * 73 + y * 151) * 2654435761u) >> 28);
                uint8_t* p = &pixels[static_cast<size_t>((y * width + x) * 4)];
                p[0] = static_cast<uint8_t>(std::clamp(r + n * 2 - 12, 0, 255));
                p[1] = static_cast<uint8_t>(std::clamp(g + n * 2 - 12, 0, 255));
                p[2] = static_cast<uint8_t>(std::clamp(b + n * 2 - 12, 0, 255));
                p[3] = 255;
            }
        return upload(pixels.data(), width, height);
    }

    const char* tierName(StackId id)
    {
        if (id == Items::TurtleHelmet) return "turtle";
        switch (Armour::tierOf(id))
        {
            case Armour::Tier::Leather: return "leather";
            case Armour::Tier::Gold: return "gold";
            case Armour::Tier::Iron: return "iron";
            case Armour::Tier::Diamond: return "diamond";
            case Armour::Tier::Netherite: return "netherite";
            case Armour::Tier::Chainmail: return "chainmail";
            default: return nullptr;
        }
    }
}

ArmorStandRenderer::~ArmorStandRenderer()
{
    if (m_wood) glDeleteTextures(1, &m_wood);
    for (const auto& entry : m_armour)
        if (entry.second) glDeleteTextures(1, &entry.second);
}

std::vector<BoxMesh::Box> ArmorStandRenderer::standBoxes(const ArmorStands::Stand& stand)
{
    const ArmorStands::Angles& a = ArmorStands::poses()[static_cast<size_t>(
        std::clamp(stand.pose, 0, static_cast<int>(ArmorStands::poses().size()) - 1))].angles;
    std::vector<BoxMesh::Box> boxes;
    boxes.push_back(part(0, 0, { -1.0f, -7.0f, -1.0f }, { 2.0f, 7.0f, 2.0f }, { 0.0f, 1.0f, 0.0f }, a.head));
    boxes.push_back(part(0, 26, { -6.0f, 0.0f, -1.5f }, { 12.0f, 3.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, a.body));
    boxes.push_back(part(24, 0, { -2.0f, -2.0f, -1.0f }, { 2.0f, 12.0f, 2.0f }, { -5.0f, 2.0f, 0.0f }, a.rightArm));
    boxes.push_back(part(32, 16, { 0.0f, -2.0f, -1.0f }, { 2.0f, 12.0f, 2.0f }, { 5.0f, 2.0f, 0.0f }, a.leftArm, true));
    boxes.push_back(part(8, 0, { -1.0f, 0.0f, -1.0f }, { 2.0f, 11.0f, 2.0f }, { -1.9f, 12.0f, 0.0f }, a.rightLeg));
    boxes.push_back(part(40, 16, { -1.0f, 0.0f, -1.0f }, { 2.0f, 11.0f, 2.0f }, { 1.9f, 12.0f, 0.0f }, a.leftLeg, true));
    boxes.push_back(part(16, 0, { -3.0f, 3.0f, -1.0f }, { 2.0f, 7.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, a.body));
    boxes.push_back(part(48, 16, { 1.0f, 3.0f, -1.0f }, { 2.0f, 7.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, a.body));
    boxes.push_back(part(0, 48, { -4.0f, 10.0f, -1.0f }, { 8.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, a.body));
    boxes.push_back(part(0, 32, { -6.0f, 11.0f, -6.0f }, { 12.0f, 1.0f, 12.0f }, { 0.0f, 12.0f, 0.0f },
                         glm::vec3(0.0f, stand.yaw - 90.0f, 0.0f)));
    return boxes;
}

std::vector<BoxMesh::Box> ArmorStandRenderer::armourBoxes(const ArmorStands::Stand& stand, int slot)
{
    const ArmorStands::Angles& a = ArmorStands::poses()[static_cast<size_t>(
        std::clamp(stand.pose, 0, static_cast<int>(ArmorStands::poses().size()) - 1))].angles;
    const float inflate = slot == ArmorStands::Legs ? 0.5f : 1.0f;
    std::vector<BoxMesh::Box> boxes;
    if (slot == ArmorStands::Head)
        boxes.push_back(part(0, 0, { -4.0f, -8.0f, -4.0f }, { 8.0f, 8.0f, 8.0f }, { 0.0f, 1.0f, 0.0f }, a.head, false, inflate));
    if (slot == ArmorStands::Chest || slot == ArmorStands::Legs)
        boxes.push_back(part(16, 16, { -4.0f, 0.0f, -2.0f }, { 8.0f, 12.0f, 4.0f }, { 0.0f, 0.0f, 0.0f }, a.body, false, inflate));
    if (slot == ArmorStands::Chest)
    {
        boxes.push_back(part(40, 16, { -3.0f, -2.0f, -2.0f }, { 4.0f, 12.0f, 4.0f }, { -5.0f, 2.0f, 0.0f }, a.rightArm, false, inflate));
        boxes.push_back(part(40, 16, { -1.0f, -2.0f, -2.0f }, { 4.0f, 12.0f, 4.0f }, { 5.0f, 2.0f, 0.0f }, a.leftArm, true, inflate));
    }
    if (slot == ArmorStands::Legs || slot == ArmorStands::Feet)
    {
        boxes.push_back(part(0, 16, { -2.0f, 0.0f, -2.0f }, { 4.0f, 12.0f, 4.0f }, { -1.9f, 11.0f, 0.0f }, a.rightLeg, false, inflate));
        boxes.push_back(part(0, 16, { -2.0f, 0.0f, -2.0f }, { 4.0f, 12.0f, 4.0f }, { 1.9f, 11.0f, 0.0f }, a.leftLeg, true, inflate));
    }
    return boxes;
}

unsigned int ArmorStandRenderer::woodTexture()
{
    if (m_woodTried) return m_wood;
    m_woodTried = true;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load("assets/textures/entity/armorstand/wood.png", &width, &height, &channels, 4);
    if (data && width == 64 && height == 64) m_wood = upload(data, width, height);
    else m_wood = painted(64, 64, 166, 128, 78);
    if (data) stbi_image_free(data);
    return m_wood;
}

unsigned int ArmorStandRenderer::armourTexture(const std::string& name, unsigned int fallback)
{
    const auto found = m_armour.find(name);
    if (found != m_armour.end()) return found->second;
    int width = 0, height = 0, channels = 0;
    const std::string path = "assets/textures/models/armor/" + name + ".png";
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    unsigned int texture = 0;
    if (data && width > 0 && height > 0) texture = upload(data, width, height);
    else texture = painted(64, 32, static_cast<uint8_t>((fallback >> 16) & 0xFFu), static_cast<uint8_t>((fallback >> 8) & 0xFFu),
                           static_cast<uint8_t>(fallback & 0xFFu));
    if (data) stbi_image_free(data);
    m_armour[name] = texture;
    return texture;
}

void ArmorStandRenderer::draw(unsigned int texture, const std::vector<float>& vertices)
{
    if (vertices.empty() || !texture) return;
    m_mesh.upload(vertices, CHUNK_LAYOUT, true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    m_mesh.draw(GL_TRIANGLES);
}

void ArmorStandRenderer::render(Shader& shader, const World& world, const std::vector<ArmorStands::Stand>& stands)
{
    if (stands.empty()) return;

    std::vector<float> wood;
    std::map<unsigned int, std::vector<float>> layered;

    for (const ArmorStands::Stand& stand : stands)
    {
        const int lx = static_cast<int>(std::floor(stand.feet.x));
        const int ly = static_cast<int>(std::floor(stand.feet.y + 1.0f));
        const int lz = static_cast<int>(std::floor(stand.feet.z));

        BoxMesh::Frame frame;
        frame.feet = stand.feet;
        frame.bodyYaw = stand.yaw;
        const float ticks = stand.sinceHit / 0.05f;
        if (ticks < 5.0f) frame.bodyYaw += std::sin(ticks / 1.5f * 3.14159265f) * 3.0f;
        frame.sky = static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f;
        frame.blockLight = static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f;
        frame.textureWidth = 64.0f;
        frame.textureHeight = 64.0f;
        BoxMesh::appendAll(wood, standBoxes(stand), frame);

        frame.textureHeight = 32.0f;
        for (int slot = ArmorStands::Head; slot <= ArmorStands::Feet; ++slot)
        {
            const ItemStack& worn = stand.gear[static_cast<size_t>(slot)];
            if (worn.empty() || !Armour::isArmour(worn.id)) continue;
            const bool leggings = slot == ArmorStands::Legs;
            std::vector<BoxMesh::Box> boxes = armourBoxes(stand, slot);
            BoxMesh::appendAll(layered[m_textures.layer(worn.id, leggings)], boxes, frame);
            if (Armour::tierOf(worn.id) == Armour::Tier::Leather)
            {
                for (BoxMesh::Box& box : boxes) box.inflate += 0.01f;
                BoxMesh::appendAll(layered[m_textures.leatherOverlay(leggings)], boxes, frame);
            }
            if (Smithing::trimPattern(worn.enchantments) >= 0)
            {
                for (BoxMesh::Box& box : boxes) box.inflate += 0.02f;
                BoxMesh::appendAll(layered[m_textures.trim(Smithing::trimPattern(worn.enchantments), Smithing::trimMaterial(worn.enchantments),
                                                           worn.id, leggings)], boxes, frame);
            }
        }
    }

    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);
    draw(woodTexture(), wood);
    for (const auto& entry : layered)
        if (entry.first) draw(entry.first, entry.second);
    glEnable(GL_CULL_FACE);
}
