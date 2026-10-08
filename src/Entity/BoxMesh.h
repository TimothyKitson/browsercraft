#pragma once
#include <glm/glm.hpp>
#include <vector>

// Minecraft builds every character out of textured boxes unwrapped onto a
// flat sheet, and so does this engine: players wear a skin, mobs wear a
// painted hide, and both arrive here. Keeping one copy of the unwrapping
// is the point -- it is fiddly enough that two copies would drift.
namespace BoxMesh
{
    // Nine floats a vertex, the layout the chunk shader expects:
    // position, uv, shade, sky light, block light, face.
    constexpr int FLOATS_PER_VERTEX = 9;

    // Face 6 is what the mesher marks cross-shaped plants with, and it is
    // the value that tells the chunk shader to skip tangent-space normal
    // mapping -- which neither a skin nor a hide has maps for.
    constexpr float FACE_NO_NORMAL_MAP = 6.0f;

    enum Face { Right, Left, Top, Bottom, Front, Back };

    // Where one face of a box lands on the sheet, given the box's own
    // top-left corner (u, v) and its size in texture pixels.
    void facePatch(int face, int u, int v, int w, int h, int d,
                   int& outX, int& outY, int& outW, int& outH);

    // One limb or slab. Coordinates are texture pixels, sixteen to the
    // block, measured from the figure's feet.
    struct Box
    {
        glm::vec3 min{ 0.0f }, max{ 0.0f };
        glm::vec3 pivot{ 0.0f };
        float pitch = 0.0f;     // radians about the figure's right; swings limbs
        float yaw = 0.0f;       // radians about up, positive turns left
        int u = 0, v = 0;
        bool mirror = false;    // the old skin layout holds one arm; the other mirrors it
        float inflate = 0.0f;   // hats and jackets sit just outside what they cover
        // A second rotation applied after the box's own, about a shared
        // pivot. The player's torso folds forward on this when sneaking;
        // mobs leave it alone.
        bool jointed = false;
    };

    // Where the figure stands and which way it faces, plus the lighting
    // every vertex of it is given.
    struct Frame
    {
        glm::vec3 feet{ 0.0f };
        float bodyYaw = 0.0f;       // degrees, the same convention as Camera::yaw
        float sky = 1.0f;           // 0..1
        float blockLight = 0.0f;    // 0..1
        float textureWidth = 64.0f;
        float textureHeight = 64.0f;
        float jointAngle = 0.0f;    // radians, applied to jointed boxes
        glm::vec3 jointPivot{ 0.0f };
    };

    // Appends one box, in world space, to a vertex buffer.
    void append(std::vector<float>& out, const Box& box, const Frame& frame);

    // Appends a whole figure.
    void appendAll(std::vector<float>& out, const std::vector<Box>& boxes, const Frame& frame);
}
