#pragma once
#include "Mesh.h"
#include "Game/Paintings.h"
#include <vector>

class Shader;
class World;

class PaintingRenderer
{
public:
    ~PaintingRenderer();
    void render(Shader& shader, const World& world, const std::vector<Paintings::Placed>& paintings);

    static std::vector<uint8_t> paintVariant(int variant, int& width, int& height);
    static std::vector<uint8_t> paintBack();

private:
    unsigned int textureFor(int variant);
    unsigned int backTexture();
    void draw(unsigned int texture, std::vector<float>& vertices);

    std::vector<unsigned int> m_textures;
    std::vector<bool> m_tried;
    unsigned int m_back = 0;
    bool m_backTried = false;
    Mesh m_mesh;
};
