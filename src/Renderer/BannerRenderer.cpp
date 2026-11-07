#include "BannerRenderer.h"
#include "Core/GLFunctions.h"
#include "Entity/BoxMesh.h"
#include "Shader.h"
#include "World/World.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <string>
#include "Core/Packs.h"

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    Banners::Sheet loadSheet(const std::string& path)
    {
        Banners::Sheet sheet;
        int width = 0, height = 0, channels = 0;
        unsigned char* data = Packs::load(path.c_str(), &width, &height, &channels, 4);
        if (!data) return sheet;
        if (width >= Banners::SHEET && height >= Banners::SHEET)
        {
            sheet.width = width;
            sheet.height = height;
            sheet.rgba.assign(data, data + static_cast<size_t>(width) * height * 4);
        }
        stbi_image_free(data);
        return sheet;
    }

    BoxMesh::Box box(glm::vec3 from, glm::vec3 to, int u, int v)
    {
        BoxMesh::Box b;
        b.min = from;
        b.max = to;
        b.u = u;
        b.v = v;
        return b;
    }
}

BannerRenderer::~BannerRenderer()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

void BannerRenderer::ensureLoaded()
{
    if (m_loaded) return;
    m_loaded = true;
    m_sheets.frame = loadSheet("assets/textures/entity/banner_base.png");
    for (int p = 0; p < Banners::PatternCount; ++p)
        m_sheets.patterns[static_cast<size_t>(p)] =
            loadSheet(Banners::bannerTexture(p));
}

unsigned int BannerRenderer::texture()
{
    if (!m_texture)
    {
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        std::vector<uint8_t> blank(static_cast<size_t>(ATLAS) * ATLAS * 4, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, ATLAS, ATLAS, 0, GL_RGBA, GL_UNSIGNED_BYTE, blank.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    return m_texture;
}

BannerRenderer::Cell BannerRenderer::cellFor(int colour, const Banners::Design& design)
{
    ensureLoaded();
    const unsigned int atlas = texture();
    const std::string key = Banners::key(colour, design);
    auto found = m_cells.find(key);
    int index = 0;
    if (found != m_cells.end())
        index = found->second;
    else
    {
        if (m_nextCell >= PER_ROW * PER_ROW)
        {
            m_cells.clear();
            m_nextCell = 0;
        }
        index = m_nextCell++;
        m_cells[key] = index;
        Banners::compose(colour, design, m_sheets, m_scratch);
        glBindTexture(GL_TEXTURE_2D, atlas);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, (index % PER_ROW) * Banners::SHEET, (index / PER_ROW) * Banners::SHEET,
                        Banners::SHEET, Banners::SHEET, GL_RGBA, GL_UNSIGNED_BYTE, m_scratch.data());
    }
    Cell cell;
    cell.size = static_cast<float>(Banners::SHEET) / ATLAS;
    cell.u0 = static_cast<float>(index % PER_ROW) * cell.size;
    cell.v0 = static_cast<float>(index / PER_ROW) * cell.size;
    return cell;
}

void BannerRenderer::render(Shader& shader, const World& world, const std::vector<Placed>& banners, float seconds)
{
    if (banners.empty()) return;
    m_vertices.clear();
    constexpr float PI = 3.14159265f;
    for (const Placed& banner : banners)
    {
        const Cell cell = cellFor(banner.colour, banner.design);
        const glm::ivec3 at = banner.at;
        const float yaw = static_cast<float>((banner.facing & 3) - 1) * 90.0f;
        const glm::vec3 forward(std::cos(glm::radians(yaw)), 0.0f, std::sin(glm::radians(yaw)));

        BoxMesh::Frame frame;
        frame.textureWidth = static_cast<float>(Banners::SHEET);
        frame.textureHeight = static_cast<float>(Banners::SHEET);
        frame.scale = 2.0f / 3.0f;
        frame.bodyYaw = yaw;
        frame.sky = static_cast<float>(world.skyLight(at.x, at.y, at.z)) / 15.0f;
        frame.blockLight = static_cast<float>(world.blockLightAt(at.x, at.y, at.z)) / 15.0f;
        if (banner.wall)
            frame.feet = glm::vec3(at) + glm::vec3(0.5f, 0.0f, 0.5f) - forward * 0.4375f + glm::vec3(0.0f, -0.979f, 0.0f);
        else
            frame.feet = glm::vec3(at) + glm::vec3(0.5f, 0.0f, 0.5f);

        const float phase = std::fmod(static_cast<float>(at.x * 7 + at.y * 9 + at.z * 13) + seconds * 20.0f, 100.0f) / 100.0f;
        std::vector<BoxMesh::Box> parts;
        BoxMesh::Box flag = box({ -10.0f, 4.0f, 1.0f }, { 10.0f, 44.0f, 2.0f }, 0, 0);
        flag.pivot = glm::vec3(0.0f, 44.0f, 0.0f);
        flag.pitch = (-0.0125f + 0.01f * std::cos(2.0f * PI * phase)) * PI;
        parts.push_back(flag);
        parts.push_back(box({ -10.0f, 42.0f, -1.0f }, { 10.0f, 44.0f, 1.0f }, 0, 42));
        if (!banner.wall) parts.push_back(box({ -1.0f, 0.0f, -1.0f }, { 1.0f, 42.0f, 1.0f }, 44, 0));

        const size_t start = m_vertices.size();
        BoxMesh::appendAll(m_vertices, parts, frame);
        for (size_t i = start; i < m_vertices.size(); i += BoxMesh::FLOATS_PER_VERTEX)
        {
            m_vertices[i + 3] = cell.u0 + m_vertices[i + 3] * cell.size;
            m_vertices[i + 4] = cell.v0 + m_vertices[i + 4] * cell.size;
        }
    }
    if (m_vertices.empty()) return;
    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);
    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture());
    m_mesh.draw(GL_TRIANGLES);
    glEnable(GL_CULL_FACE);
}
