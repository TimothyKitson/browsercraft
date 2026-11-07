#pragma once
#include "Mesh.h"
#include "Entity/BoxMesh.h"
#include "Game/ArmorStands.h"
#include <map>
#include <string>
#include <vector>

class Shader;
class World;

class ArmorStandRenderer
{
public:
    ~ArmorStandRenderer();
    void render(Shader& shader, const World& world, const std::vector<ArmorStands::Stand>& stands);

    static std::vector<BoxMesh::Box> standBoxes(const ArmorStands::Stand& stand);
    static std::vector<BoxMesh::Box> armourBoxes(const ArmorStands::Stand& stand, int slot);

private:
    unsigned int woodTexture();
    unsigned int armourTexture(const std::string& name, unsigned int fallback);
    void draw(unsigned int texture, const std::vector<float>& vertices);

    unsigned int m_wood = 0;
    bool m_woodTried = false;
    std::map<std::string, unsigned int> m_armour;
    Mesh m_mesh;
};
