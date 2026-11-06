#pragma once
#include "Mesh.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

class Shader;
class World;

class BookRenderer
{
public:
    struct Book
    {
        float rotation = 0.0f;
        float lastRotation = 0.0f;
        float target = 0.0f;
        float open = 0.0f;
        float lastOpen = 0.0f;
        float flip = 0.0f;
        float lastFlip = 0.0f;
        float flipTurn = 0.0f;
        float flipRandom = 0.0f;
        int ticks = 0;
    };

    ~BookRenderer();

    void update(float deltaTime, const World& world, const glm::vec3& player);
    void render(Shader& shader, const World& world);
    size_t count() const { return m_books.size(); }
    const Book* bookAt(const glm::ivec3& at) const;

    static void step(Book& book, bool playerNear, float dx, float dz, uint32_t& seed);

private:
    std::map<std::tuple<int, int, int>, Book> m_books;
    float m_clock = 0.0f;
    float m_scanTimer = 0.0f;
    uint32_t m_seed = 0x2545F491u;
    unsigned int m_texture = 0;
    bool m_loaded = false;
    Mesh m_mesh;
    std::vector<float> m_vertices;
    glm::vec3 m_player{ 0.0f };
};
