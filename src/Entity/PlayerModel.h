#pragma once
#include "Renderer/Mesh.h"
#include "BoxMesh.h"
#include <glm/glm.hpp>
#include <vector>

class Shader;
class World;
class PlayerSkin;

// Where a player is and what they are doing with their limbs. Filled in
// from the network for everyone else and from the local player in third
// person, so both go through the same renderer.
struct PlayerPose
{
    glm::vec3 feet{ 0.0f };
    float bodyYaw = 0.0f;      // degrees, same convention as Camera::yaw
    float headYaw = 0.0f;
    float headPitch = 0.0f;
    float limbPhase = 0.0f;    // radians around the walk cycle
    float limbAmount = 0.0f;   // 0 standing still, 1 at walking pace
    bool sneaking = false;
};

// A Minecraft player: six boxes wrapped in a 64x32 or 64x64 skin, built
// in world space each frame and drawn through the terrain shader.
//
// There is no separate entity shader because there does not need to be
// one. A skin is just another texture, and giving each vertex face 6 --
// the value the mesher uses for cross-shaped plants -- tells the chunk
// shader to skip tangent-space normal mapping, which a skin has no maps
// for. Dropped items reach the screen the same way.
class PlayerModel
{
public:
    // Skins are measured in pixels, sixteen to the block, which makes the
    // finished figure 32 pixels -- two blocks -- tall. Minecraft's own
    // model is the same height, a little taller than the 1.8 collision
    // box it belongs to.
    static constexpr float PIXEL = 1.0f / 16.0f;
    static constexpr float HEIGHT = 32.0f * PIXEL;

    // The six faces of a box, in the order the renderer walks them.
    enum Face { Right = BoxMesh::Right, Left = BoxMesh::Left, Top = BoxMesh::Top,
                Bottom = BoxMesh::Bottom, Front = BoxMesh::Front, Back = BoxMesh::Back };

    // Where one face of a box lands in the skin image, given the box's
    // own top-left corner (u, v) and its size in pixels. This is the
    // layout every Minecraft skin is painted to; it is public because
    // --selftest checks it against the flat paper doll, which is drawn
    // from PlayerSkin's own patch coordinates and has been eyeballed.
    static void facePatch(int face, int u, int v, int w, int h, int d,
                          int& outX, int& outY, int& outW, int& outH);

    // Rebuilt per player per frame: there are at most eight of them and
    // every limb moves, so a dynamic buffer beats keeping meshes around.
    void render(Shader& chunkShader, const World& world,
                const PlayerSkin& skin, const PlayerPose& pose);

    // The geometry on its own, in world space, with no GL anywhere near
    // it -- which is what lets --selftest check that the model is the
    // right size and facing the right way without a window.
    //
    // Nine floats a vertex, the layout the chunk shader expects:
    // position, uv, shade, sky light, block light, face.
    static constexpr int FLOATS_PER_VERTEX = BoxMesh::FLOATS_PER_VERTEX;
    void build(const PlayerSkin& skin, const PlayerPose& pose, float sky, float blockLight);
    const std::vector<float>& vertices() const { return m_vertices; }

private:
    Mesh m_mesh;
    std::vector<float> m_vertices;
};
