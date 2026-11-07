#pragma once
#include "Mesh.h"
#include "Game/Banners.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class Shader;
class World;

class BannerRenderer
{
public:
    struct Placed
    {
        glm::ivec3 at{ 0 };
        int colour = 0;
        bool wall = false;
        int facing = 0;
        Banners::Design design;
    };

    struct Cell
    {
        float u0 = 0.0f, v0 = 0.0f, size = 0.0f;
    };

    ~BannerRenderer();

    void render(Shader& shader, const World& world, const std::vector<Placed>& banners, float seconds);
    Cell cellFor(int colour, const Banners::Design& design);
    unsigned int texture();

    static constexpr int ATLAS = 1024;
    static constexpr int PER_ROW = ATLAS / Banners::SHEET;

private:
    void ensureLoaded();

    Banners::Sheets m_sheets;
    bool m_loaded = false;
    unsigned int m_texture = 0;
    std::unordered_map<std::string, int> m_cells;
    int m_nextCell = 0;
    Mesh m_mesh;
    std::vector<float> m_vertices;
    std::vector<uint8_t> m_scratch;
};
