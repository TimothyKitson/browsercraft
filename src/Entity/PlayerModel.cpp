#include "PlayerModel.h"
#include "PlayerAnimation.h"
#include "PlayerSkin.h"
#include "World/World.h"
#include "Renderer/Shader.h"
#include "Core/GLFunctions.h"
#include <cmath>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    // Model space is in skin pixels with the feet at y = 0:
    //   +x the player's right, +y up, +z the way they are facing.
    constexpr float HEAD_PIVOT_Y = 24.0f;
    constexpr float SHOULDER_Y = 22.0f;
    constexpr float HIP_Y = 12.0f;
    constexpr float SNEAK_TILT = 0.5f;      // radians the torso folds forward

    // Pulls the sample a hair inside the patch. The skin's six faces are
    // packed edge to edge, so sampling exactly on a boundary can land on
    // the neighbouring face and draw a stray line down the arm.
    constexpr float TEXEL_INSET = 0.02f;

    struct FaceDef
    {
        int corner[4][3];       // 0 picks the box minimum on that axis, 1 the maximum
        float normal[3];
        int rightAxis, rightSign;   // which way the skin image's +u runs
        int downAxis, downSign;     // ...and its +v
    };

    // Corners wound counter-clockwise seen from outside, so back-face
    // culling hides the inside of each box.
    //
    // The right/down columns are the whole of Minecraft's box unwrapping.
    // A front face reads right-to-left across the model because you are
    // looking at it from the front, which is why text painted onto a skin
    // comes out mirrored on the player.
    const FaceDef FACES[6] = {
        // +x, the player's right side
        { { {1,0,0}, {1,1,0}, {1,1,1}, {1,0,1} }, {  1.0f,  0.0f,  0.0f }, 2, -1, 1, -1 },
        // -x, their left
        { { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} }, { -1.0f,  0.0f,  0.0f }, 2,  1, 1, -1 },
        // +y, the top
        { { {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0} }, {  0.0f,  1.0f,  0.0f }, 0, -1, 2,  1 },
        // -y, the underside
        { { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} }, {  0.0f, -1.0f,  0.0f }, 0, -1, 2, -1 },
        // +z, the face they look out of
        { { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} }, {  0.0f,  0.0f,  1.0f }, 0, -1, 1, -1 },
        // -z, the back of the head
        { { {0,0,0}, {0,1,0}, {1,1,0}, {1,0,0} }, {  0.0f,  0.0f, -1.0f }, 0,  1, 1, -1 },
    };

    // One limb or slab: a box turned about a pivot, wrapped in the patch
    // of skin that starts at (u, v).
    struct Box
    {
        glm::vec3 min{ 0.0f }, max{ 0.0f };
        glm::vec3 pivot{ 0.0f };
        float pitch = 0.0f;     // radians about the model's right; swings limbs
        float yaw = 0.0f;       // radians about up, positive turns left
        int u = 0, v = 0;
        bool mirror = false;    // the old layout holds one arm and one leg; the others mirror it
        bool torso = false;     // folds forward with the body when sneaking
        float inflate = 0.0f;   // the hat and jacket sit just outside the limb they cover
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

    // The terrain mesher's per-face shading, so a player standing in a
    // field is lit on the same terms as the field.
    float shadeFor(const glm::vec3& normal)
    {
        const float ax = std::abs(normal.x), ay = std::abs(normal.y), az = std::abs(normal.z);
        if (ay >= ax && ay >= az) return normal.y > 0.0f ? 1.0f : 0.50f;
        if (ax >= az) return 0.72f;
        return 0.86f;
    }

    void buildBoxes(std::vector<Box>& boxes, const PlayerSkin& skin, const PlayerPose& pose)
    {
        const float arm = static_cast<float>(skin.armWidth());
        // The 64x64 layout carries a left arm and leg of their own, plus a
        // second layer over every limb. The old 64x32 one has neither.
        const bool twoLayer = skin.height() >= 64;

        const float phase = pose.limbPhase;
        const float amount = pose.limbAmount;
        const float halfTurn = PlayerAnimation::PI;

        const float headTurn = glm::radians(
            -PlayerAnimation::wrapDegrees(pose.headYaw - pose.bodyYaw));
        const float headTilt = glm::radians(-pose.headPitch);

        Box head;
        head.min = glm::vec3(-4.0f, 24.0f, -4.0f);
        head.max = glm::vec3(4.0f, 32.0f, 4.0f);
        head.pivot = glm::vec3(0.0f, HEAD_PIVOT_Y, 0.0f);
        head.pitch = headTilt;
        head.yaw = headTurn;
        head.u = 0; head.v = 0;
        head.torso = true;

        Box body;
        body.min = glm::vec3(-4.0f, 12.0f, -2.0f);
        body.max = glm::vec3(4.0f, 24.0f, 2.0f);
        body.pivot = glm::vec3(0.0f, HEAD_PIVOT_Y, 0.0f);
        body.u = 16; body.v = 16;
        body.torso = true;

        Box rightArm;
        rightArm.min = glm::vec3(4.0f, 10.0f, -2.0f);
        rightArm.max = glm::vec3(4.0f + arm, 22.0f, 2.0f);
        rightArm.pivot = glm::vec3(4.0f + arm * 0.5f, SHOULDER_Y, 0.0f);
        rightArm.pitch = PlayerAnimation::armAngle(phase, amount);
        rightArm.u = 40; rightArm.v = 16;
        rightArm.torso = true;

        Box leftArm = rightArm;
        leftArm.min = glm::vec3(-4.0f - arm, 10.0f, -2.0f);
        leftArm.max = glm::vec3(-4.0f, 22.0f, 2.0f);
        leftArm.pivot = glm::vec3(-(4.0f + arm * 0.5f), SHOULDER_Y, 0.0f);
        leftArm.pitch = PlayerAnimation::armAngle(phase + halfTurn, amount);
        if (twoLayer) { leftArm.u = 32; leftArm.v = 48; }
        else          { leftArm.mirror = true; }

        Box rightLeg;
        rightLeg.min = glm::vec3(0.0f, 0.0f, -2.0f);
        rightLeg.max = glm::vec3(4.0f, 12.0f, 2.0f);
        rightLeg.pivot = glm::vec3(2.0f, HIP_Y, 0.0f);
        rightLeg.pitch = PlayerAnimation::legAngle(phase, amount);
        rightLeg.u = 0; rightLeg.v = 16;

        Box leftLeg = rightLeg;
        leftLeg.min = glm::vec3(-4.0f, 0.0f, -2.0f);
        leftLeg.max = glm::vec3(0.0f, 12.0f, 2.0f);
        leftLeg.pivot = glm::vec3(-2.0f, HIP_Y, 0.0f);
        leftLeg.pitch = PlayerAnimation::legAngle(phase + halfTurn, amount);
        if (twoLayer) { leftLeg.u = 16; leftLeg.v = 48; }
        else          { leftLeg.mirror = true; }

        boxes.push_back(head);
        boxes.push_back(body);
        boxes.push_back(rightArm);
        boxes.push_back(leftArm);
        boxes.push_back(rightLeg);
        boxes.push_back(leftLeg);

        // The hat is in both layouts; everything else over the top of a
        // limb only exists in the modern one. Fully transparent pixels
        // are discarded by the shader, so an unused layer costs nothing
        // on screen.
        Box hat = head;
        hat.u = 32; hat.v = 0; hat.inflate = 0.5f;
        boxes.push_back(hat);

        if (!twoLayer) return;

        Box jacket = body;
        jacket.u = 16; jacket.v = 32; jacket.inflate = 0.25f;
        boxes.push_back(jacket);

        Box rightSleeve = rightArm;
        rightSleeve.u = 40; rightSleeve.v = 32; rightSleeve.inflate = 0.25f;
        boxes.push_back(rightSleeve);

        Box leftSleeve = leftArm;
        leftSleeve.u = 48; leftSleeve.v = 48; leftSleeve.inflate = 0.25f;
        boxes.push_back(leftSleeve);

        Box rightTrouser = rightLeg;
        rightTrouser.u = 0; rightTrouser.v = 32; rightTrouser.inflate = 0.25f;
        boxes.push_back(rightTrouser);

        Box leftTrouser = leftLeg;
        leftTrouser.u = 0; leftTrouser.v = 48; leftTrouser.inflate = 0.25f;
        boxes.push_back(leftTrouser);
    }
}

// Where each face's patch sits inside a box's own rectangle of skin.
// Reading across the row of side faces: right, front, left, back, with
// the top and bottom sitting above them. Same order as FACES.
void PlayerModel::facePatch(int face, int u, int v, int w, int h, int d,
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

void PlayerModel::build(const PlayerSkin& skin, const PlayerPose& pose,
                        float sky, float blockLight)
{
    m_vertices.clear();
    if (skin.width() <= 0 || skin.height() <= 0) return;

    std::vector<Box> boxes;
    boxes.reserve(12);
    buildBoxes(boxes, skin, pose);

    const float bodyRadians = glm::radians(pose.bodyYaw);
    const glm::vec3 forward(std::cos(bodyRadians), 0.0f, std::sin(bodyRadians));
    const glm::vec3 right(-forward.z, 0.0f, forward.x);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const float tilt = pose.sneaking ? SNEAK_TILT : 0.0f;
    const glm::vec3 hip(0.0f, HIP_Y, 0.0f);

    const float skinW = static_cast<float>(skin.width());
    const float skinH = static_cast<float>(skin.height());

    m_vertices.reserve(boxes.size() * 36 * FLOATS_PER_VERTEX);

    for (const Box& box : boxes)
    {
        const int w = static_cast<int>(box.max.x - box.min.x);
        const int h = static_cast<int>(box.max.y - box.min.y);
        const int d = static_cast<int>(box.max.z - box.min.z);

        const glm::vec3 lo = box.min - glm::vec3(box.inflate);
        const glm::vec3 hi = box.max + glm::vec3(box.inflate);

        for (int f = 0; f < 6; ++f)
        {
            const FaceDef& face = FACES[f];

            int px = 0, py = 0, pw = 0, ph = 0;
            PlayerModel::facePatch(f, box.u, box.v, w, h, d, px, py, pw, ph);

            const float u0 = (px + TEXEL_INSET) / skinW;
            const float u1 = (px + pw - TEXEL_INSET) / skinW;
            const float v0 = (py + TEXEL_INSET) / skinH;
            const float v1 = (py + ph - TEXEL_INSET) / skinH;

            // The normal is turned with the box, so the shading follows
            // the limb round rather than staying stuck to the skin.
            glm::vec3 normal(face.normal[0], face.normal[1], face.normal[2]);
            normal = rotateX(normal, box.pitch);
            if (box.yaw != 0.0f) normal = rotateY(normal, box.yaw);
            if (box.torso && tilt != 0.0f) normal = rotateX(normal, tilt);
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
                if (box.torso && tilt != 0.0f) local = rotateX(local - hip, tilt) + hip;

                local *= PIXEL;
                corners[k] = pose.feet + right * local.x + up * local.y + forward * local.z;

                const bool onRight = face.corner[k][face.rightAxis] == (face.rightSign > 0 ? 1 : 0);
                const bool onBottom = face.corner[k][face.downAxis] == (face.downSign > 0 ? 1 : 0);
                u[k] = (onRight != box.mirror) ? u1 : u0;
                v[k] = onBottom ? v1 : v0;
            }

            const int order[6] = { 0, 1, 2, 0, 2, 3 };
            for (int k : order)
            {
                m_vertices.push_back(corners[k].x);
                m_vertices.push_back(corners[k].y);
                m_vertices.push_back(corners[k].z);
                m_vertices.push_back(u[k]);
                m_vertices.push_back(v[k]);
                m_vertices.push_back(shade);
                m_vertices.push_back(sky);
                m_vertices.push_back(blockLight);
                m_vertices.push_back(6.0f); // face 6: skip tangent-space normal mapping
            }
        }
    }
}

void PlayerModel::render(Shader& chunkShader, const World& world,
                         const PlayerSkin& skin, const PlayerPose& pose)
{
    if (!skin.textureId()) return;

    // One light sample for the whole figure, taken at head height and
    // nudged out of anything solid: reading it inside their own body, or
    // inside the floor, renders the player almost black. Dropped items
    // had the same problem and take the same way out.
    int lx = static_cast<int>(std::floor(pose.feet.x));
    int ly = static_cast<int>(std::floor(pose.feet.y + 1.6f));
    int lz = static_cast<int>(std::floor(pose.feet.z));
    for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step)
        ++ly;

    build(skin, pose,
          static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f,
          static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f);

    if (m_vertices.empty()) return;

    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f)); // vertices are already in world space
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skin.textureId());
    m_mesh.draw(GL_TRIANGLES);
}
