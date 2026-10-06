#include "Chunk.h"
#include "../Renderer/Texture.h"
#include <array>

namespace
{
    // The 8 corners of a unit cube, local to a block's (0,0,0) origin.
    constexpr glm::vec3 c0(0, 0, 0), c1(1, 0, 0), c2(1, 1, 0), c3(0, 1, 0);
    constexpr glm::vec3 c4(0, 0, 1), c5(1, 0, 1), c6(1, 1, 1), c7(0, 1, 1);

    struct FaceDef
    {
        glm::vec3 corners[4]; // in order around the quad; UVs are (0,0)(1,0)(1,1)(0,1)
        int dx, dy, dz;       // offset to the neighboring block this face touches
        float brightness;     // crude fixed directional shading, like Minecraft's
    };

    // Face order: +Y (top), -Y (bottom), +X, -X, +Z, -Z
    const std::array<FaceDef, 6> FACES = { {
        { { c3, c2, c6, c7 },  0,  1,  0, 1.0f },
        { { c0, c4, c5, c1 },  0, -1,  0, 0.5f },
        { { c1, c2, c6, c5 },  1,  0,  0, 0.8f },
        { { c0, c4, c7, c3 }, -1,  0,  0, 0.8f },
        { { c4, c5, c6, c7 },  0,  0,  1, 0.6f },
        { { c0, c1, c2, c3 },  0,  0, -1, 0.6f },
    } };
}

Chunk::Chunk(ChunkCoord coord)
    : m_coord(coord), m_blocks(SIZE_X * SIZE_Y * SIZE_Z, BlockType::Air)
{
}

bool Chunk::inBounds(int x, int y, int z)
{
    return x >= 0 && x < SIZE_X && y >= 0 && y < SIZE_Y && z >= 0 && z < SIZE_Z;
}

int Chunk::index(int x, int y, int z)
{
    return (x * SIZE_Y + y) * SIZE_Z + z;
}

BlockType Chunk::getBlock(int x, int y, int z) const
{
    if (!inBounds(x, y, z)) return BlockType::Air;
    return m_blocks[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType type)
{
    if (!inBounds(x, y, z)) return;
    m_blocks[index(x, y, z)] = type;
}

glm::vec3 Chunk::worldOffset() const
{
    return glm::vec3(m_coord.x * SIZE_X, 0.0f, m_coord.z * SIZE_Z);
}

void Chunk::addFace(std::vector<float>& vertices, const glm::vec3& blockPos, int faceIndex, int tileIndex) const
{
    const FaceDef& face = FACES[faceIndex];

    float u0, v0, u1, v1;
    getAtlasTileUV(tileIndex, u0, v0, u1, v1);
    const float uvs[4][2] = { { u0, v0 }, { u1, v0 }, { u1, v1 }, { u0, v1 } };

    // Two triangles: (0,1,2) and (0,2,3).
    static constexpr int triIndices[6] = { 0, 1, 2, 0, 2, 3 };

    for (int t : triIndices)
    {
        glm::vec3 pos = blockPos + face.corners[t];
        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);
        vertices.push_back(uvs[t][0]);
        vertices.push_back(uvs[t][1]);
        vertices.push_back(face.brightness);
    }
}

void Chunk::generateMesh()
{
    std::vector<float> vertices;
    vertices.reserve(4096);

    for (int x = 0; x < SIZE_X; ++x)
    {
        for (int y = 0; y < SIZE_Y; ++y)
        {
            for (int z = 0; z < SIZE_Z; ++z)
            {
                BlockType type = getBlock(x, y, z);
                if (!isBlockSolid(type)) continue;

                BlockFaceTiles tiles = getBlockFaceTiles(type);
                glm::vec3 blockPos(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

                for (int f = 0; f < 6; ++f)
                {
                    const FaceDef& face = FACES[f];
                    // NOTE: only checks neighbors inside this chunk -- see the
                    // class comment in Chunk.h about chunk-boundary faces.
                    BlockType neighbor = getBlock(x + face.dx, y + face.dy, z + face.dz);
                    if (isBlockSolid(neighbor)) continue; // hidden face, skip it

                    int tileIndex = (f == 0) ? tiles.top : (f == 1) ? tiles.bottom : tiles.side;
                    addFace(vertices, blockPos, f, tileIndex);
                }
            }
        }
    }

    m_mesh.upload(vertices);
}
