#include "PlayerModel.h"
#include "PlayerAnimation.h"
#include "PlayerSkin.h"
#include "BoxMesh.h"
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

    using Box = BoxMesh::Box;

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
        head.jointed = true;

        Box body;
        body.min = glm::vec3(-4.0f, 12.0f, -2.0f);
        body.max = glm::vec3(4.0f, 24.0f, 2.0f);
        body.pivot = glm::vec3(0.0f, HEAD_PIVOT_Y, 0.0f);
        body.u = 16; body.v = 16;
        body.jointed = true;

        Box rightArm;
        rightArm.min = glm::vec3(4.0f, 10.0f, -2.0f);
        rightArm.max = glm::vec3(4.0f + arm, 22.0f, 2.0f);
        rightArm.pivot = glm::vec3(4.0f + arm * 0.5f, SHOULDER_Y, 0.0f);
        rightArm.pitch = PlayerAnimation::armAngle(phase, amount);
        rightArm.u = 40; rightArm.v = 16;
        rightArm.jointed = true;

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

void PlayerModel::facePatch(int face, int u, int v, int w, int h, int d,
                            int& outX, int& outY, int& outW, int& outH)
{
    BoxMesh::facePatch(face, u, v, w, h, d, outX, outY, outW, outH);
}

void PlayerModel::build(const PlayerSkin& skin, const PlayerPose& pose,
                        float sky, float blockLight)
{
    m_vertices.clear();
    if (skin.width() <= 0 || skin.height() <= 0) return;

    std::vector<Box> boxes;
    boxes.reserve(12);
    buildBoxes(boxes, skin, pose);

    BoxMesh::Frame frame;
    frame.feet = pose.feet;
    frame.bodyYaw = pose.bodyYaw;
    frame.sky = sky;
    frame.blockLight = blockLight;
    frame.textureWidth = static_cast<float>(skin.width());
    frame.textureHeight = static_cast<float>(skin.height());
    frame.jointAngle = pose.sneaking ? SNEAK_TILT : 0.0f;
    frame.jointPivot = glm::vec3(0.0f, HIP_Y, 0.0f);

    BoxMesh::appendAll(m_vertices, boxes, frame);
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
