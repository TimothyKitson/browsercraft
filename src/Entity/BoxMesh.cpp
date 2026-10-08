#include "BoxMesh.h"
#include <cmath>

namespace
{
    // Pulls the sample a hair inside the patch. A sheet's six faces are
    // packed edge to edge, so sampling exactly on a boundary can land on
    // the neighbouring face and draw a stray line down the arm.
    constexpr float TEXEL_INSET = 0.02f;

    struct FaceDef
    {
        int corner[4][3];       // 0 picks the box minimum on that axis, 1 the maximum
        float normal[3];
        int rightAxis, rightSign;   // which way the sheet's +u runs
        int downAxis, downSign;     // ...and its +v
    };

    // Corners wound counter-clockwise seen from outside *in model space*,
    // where +x is the figure's right, +y is up and +z is the way it
    // faces. That triple is left-handed -- a figure's right hand is
    // forward cross up -- so the winding comes out the other way round
    // once the box is placed in the world, and the triangles below are
    // emitted in the order that puts it back.
    //
    // The right/down columns are the whole of Minecraft's box unwrapping.
    // A front face reads right-to-left across the model because you are
    // looking at it from the front, which is why text painted onto a skin
    // comes out mirrored on the figure wearing it.
    const FaceDef FACES[6] = {
        // +x, the figure's right side
        { { {1,0,0}, {1,1,0}, {1,1,1}, {1,0,1} }, {  1.0f,  0.0f,  0.0f }, 2, -1, 1, -1 },
        // -x, its left
        { { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} }, { -1.0f,  0.0f,  0.0f }, 2,  1, 1, -1 },
        // +y, the top
        { { {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0} }, {  0.0f,  1.0f,  0.0f }, 0, -1, 2,  1 },
        // -y, the underside
        { { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} }, {  0.0f, -1.0f,  0.0f }, 0, -1, 2, -1 },
        // +z, the face it looks out of
        { { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} }, {  0.0f,  0.0f,  1.0f }, 0, -1, 1, -1 },
        // -z, the back of the head
        { { {0,0,0}, {0,1,0}, {1,1,0}, {1,0,0} }, {  0.0f,  0.0f, -1.0f }, 0,  1, 1, -1 },
    };

    glm::vec3 rotateX(const glm::vec3& p, float a)
    {
        const float s = std::sin(a), c = std::cos(a);
        return glm::vec3(p.x, p.y * c - p.z * s, p.y * s + p.z * c);
    }

    glm::vec3 rotateY(const glm::vec3& p, float a)
    {
        const float s = std::sin(a), c = std::cos(a);
        return glm::vec3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c);
    }

    glm::vec3 rotateZ(const glm::vec3& p, float a)
    {
        const float s = std::sin(a), c = std::cos(a);
        return glm::vec3(p.x * c - p.y * s, p.x * s + p.y * c, p.z);
    }

    // The terrain mesher's per-face shading, so anything standing in a
    // field is lit on the same terms as the field.
    float shadeFor(const glm::vec3& normal)
    {
        const float ax = std::abs(normal.x), ay = std::abs(normal.y), az = std::abs(normal.z);
        if (ay >= ax && ay >= az) return normal.y > 0.0f ? 1.0f : 0.50f;
        if (ax >= az) return 0.72f;
        return 0.86f;
    }
}

// Reading across the row of side faces: right, front, left, back, with
// the top and bottom sitting above them.
void BoxMesh::facePatch(int face, int u, int v, int w, int h, int d,
                        int& outX, int& outY, int& outW, int& outH)
{
    switch (face)
    {
        case Right:  outX = u;             outY = v + d; outW = d; outH = h; break;
        case Left:   outX = u + d + w;     outY = v + d; outW = d; outH = h; break;
        case Top:    outX = u + d;         outY = v;     outW = w; outH = d; break;
        case Bottom: outX = u + d + w;     outY = v;     outW = w; outH = d; break;
        case Front:  outX = u + d;         outY = v + d; outW = w; outH = h; break;
        default:     outX = u + d + w + d; outY = v + d; outW = w; outH = h; break;
    }
}

void BoxMesh::append(std::vector<float>& out, const Box& box, const Frame& frame)
{
    const float bodyRadians = glm::radians(frame.bodyYaw);
    const glm::vec3 forward(std::cos(bodyRadians), 0.0f, std::sin(bodyRadians));
    const glm::vec3 right(-forward.z, 0.0f, forward.x);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    const bool jointed = box.jointed && frame.jointAngle != 0.0f;

    const int w = static_cast<int>(box.max.x - box.min.x);
    const int h = static_cast<int>(box.max.y - box.min.y);
    const int d = static_cast<int>(box.max.z - box.min.z);

    const glm::vec3 lo = box.min - glm::vec3(box.inflate);
    const glm::vec3 hi = box.max + glm::vec3(box.inflate);

    for (int f = 0; f < 6; ++f)
    {
        const FaceDef& face = FACES[f];

        int px = 0, py = 0, pw = 0, ph = 0;
        facePatch(f, box.u, box.v, w, h, d, px, py, pw, ph);

        const float u0 = (px + TEXEL_INSET) / frame.textureWidth;
        const float u1 = (px + pw - TEXEL_INSET) / frame.textureWidth;
        const float v0 = (py + TEXEL_INSET) / frame.textureHeight;
        const float v1 = (py + ph - TEXEL_INSET) / frame.textureHeight;

        // The normal is turned with the box, so the shading follows the
        // limb round rather than staying stuck to the texture.
        glm::vec3 normal(face.normal[0], face.normal[1], face.normal[2]);
        normal = rotateX(normal, box.pitch);
        if (box.yaw != 0.0f) normal = rotateY(normal, box.yaw);
        if (box.roll != 0.0f) normal = rotateZ(normal, box.roll);
        if (jointed) normal = rotateX(normal, frame.jointAngle);
        const float shade = shadeFor(right * normal.x + up * normal.y + forward * normal.z);

        glm::vec3 corners[4];
        float u[4], v[4];

        for (int k = 0; k < 4; ++k)
        {
            glm::vec3 local(face.corner[k][0] ? hi.x : lo.x,
                            face.corner[k][1] ? hi.y : lo.y,
                            face.corner[k][2] ? hi.z : lo.z);

            local = rotateX(local - box.pivot, box.pitch) + box.pivot;
            if (box.yaw != 0.0f) local = rotateY(local - box.pivot, box.yaw) + box.pivot;
            if (box.roll != 0.0f) local = rotateZ(local - box.pivot, box.roll) + box.pivot;
            if (jointed) local = rotateX(local - frame.jointPivot, frame.jointAngle) + frame.jointPivot;

            local *= frame.scale / 16.0f;
            corners[k] = frame.feet + right * local.x + up * local.y + forward * local.z;

            const bool onRight = face.corner[k][face.rightAxis] == (face.rightSign > 0 ? 1 : 0);
            const bool onBottom = face.corner[k][face.downAxis] == (face.downSign > 0 ? 1 : 0);
            u[k] = (onRight != box.mirror) ? u1 : u0;
            v[k] = onBottom ? v1 : v0;
        }

        const int order[6] = { 0, 2, 1, 0, 3, 2 };
        for (int k : order)
        {
            out.push_back(corners[k].x);
            out.push_back(corners[k].y);
            out.push_back(corners[k].z);
            out.push_back(u[k]);
            out.push_back(v[k]);
            out.push_back(shade);
            out.push_back(frame.sky);
            out.push_back(frame.blockLight);
            out.push_back(FACE_NO_NORMAL_MAP);
        }
    }
}

void BoxMesh::appendAll(std::vector<float>& out, const std::vector<Box>& boxes, const Frame& frame)
{
    out.reserve(out.size() + boxes.size() * 36 * FLOATS_PER_VERTEX);
    for (const Box& box : boxes) append(out, box, frame);
}
