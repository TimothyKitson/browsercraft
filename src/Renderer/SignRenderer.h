#pragma once
#include "Mesh.h"
#include "Entity/BoxMesh.h"
#include "World/Block.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader;
class World;

class SignRenderer
{
public:
    ~SignRenderer();
    void render(Shader& shader, const World& world, const std::vector<glm::ivec3>& signs);

    static const char* woodName(int wood);
    static std::vector<BoxMesh::Box> boxesFor(BlockId block, bool attached);
    static BoxMesh::Frame frameFor(const glm::ivec3& at, BlockId block);
    static glm::vec3 textAnchor(const glm::ivec3& at, BlockId block);
    static glm::vec3 facingVector(BlockId block);

    unsigned int sheet(int wood, bool hanging);
    unsigned int editorBackground(int wood);

private:
    unsigned int load(const std::string& path, int wood, bool hanging, int width, int height);

    std::vector<unsigned int> m_sheets = std::vector<unsigned int>(WOOD_COUNT * 2, 0u);
    std::vector<bool> m_tried = std::vector<bool>(WOOD_COUNT * 2, false);
    std::vector<unsigned int> m_gui = std::vector<unsigned int>(WOOD_COUNT, 0u);
    std::vector<bool> m_guiTried = std::vector<bool>(WOOD_COUNT, false);
    Mesh m_mesh;
};
