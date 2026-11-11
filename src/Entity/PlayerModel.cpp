#include "PlayerModel.h"
#include "PlayerAnimation.h"
#include "PlayerSkin.h"
#include "BoxMesh.h"
#include "World/World.h"
#include "Renderer/Shader.h"
#include "Core/GLFunctions.h"
#include <cmath>
#include <algorithm>
#include <stb_image.h>
#include "Core/Packs.h"

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

    void buildBoxes(std::vector<Box>& boxes, float arm, bool twoLayer, const PlayerPose& pose)
    {

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
    buildBoxes(boxes, static_cast<float>(skin.armWidth()), skin.height() >= 64, pose);

    BoxMesh::Frame frame;
    frame.feet = pose.feet;
    frame.bodyYaw = pose.bodyYaw;
    frame.sky = sky;
    frame.blockLight = blockLight;
    frame.textureWidth = static_cast<float>(skin.width());
    frame.textureHeight = static_cast<float>(skin.height());
    frame.jointAngle = pose.sneaking ? SNEAK_TILT : 0.0f;
    frame.jointPivot = glm::vec3(0.0f, HIP_Y, 0.0f);
    if (pose.swimming)
    {
        frame.bodyPitch = glm::radians(pose.inWater ? std::clamp(90.0f - pose.headPitch, 10.0f, 170.0f) : 90.0f);
        frame.bodyPivot = glm::vec3(0.0f, 16.0f, 0.0f);
        frame.bodyShift = glm::vec3(0.0f, -11.0f, 0.0f);
    }

    BoxMesh::appendAll(m_vertices, boxes, frame);
}

void PlayerModel::buildArmour(const PlayerPose& pose, int piece, float sky, float blockLight, float inflateExtra)
{
    m_vertices.clear();
    std::vector<Box> all;
    buildBoxes(all, 4.0f, false, pose);

    std::vector<Box> boxes;
    const float inflate = (piece == 3 ? 0.5f : 1.0f) + inflateExtra;
    auto take = [&](int index) {
        Box box = all[static_cast<size_t>(index)];
        box.inflate = inflate;
        if (index == 3 || index == 5) box.mirror = true;
        if (index == 3) { box.u = 40; box.v = 16; }
        if (index == 5) { box.u = 0; box.v = 16; }
        boxes.push_back(box);
    };
    if (piece == 1) take(0);
    if (piece == 2) { take(1); take(2); take(3); }
    if (piece == 3) { take(1); take(4); take(5); }
    if (piece == 4) { take(4); take(5); }

    BoxMesh::Frame frame;
    frame.feet = pose.feet;
    frame.bodyYaw = pose.bodyYaw;
    frame.sky = sky;
    frame.blockLight = blockLight;
    frame.textureWidth = 64.0f;
    frame.textureHeight = 32.0f;
    frame.jointAngle = pose.sneaking ? SNEAK_TILT : 0.0f;
    frame.jointPivot = glm::vec3(0.0f, HIP_Y, 0.0f);
    if (pose.swimming)
    {
        frame.bodyPitch = glm::radians(pose.inWater ? std::clamp(90.0f - pose.headPitch, 10.0f, 170.0f) : 90.0f);
        frame.bodyPivot = glm::vec3(0.0f, 16.0f, 0.0f);
        frame.bodyShift = glm::vec3(0.0f, -11.0f, 0.0f);
    }
    BoxMesh::appendAll(m_vertices, boxes, frame);
}

void PlayerModel::buildElytra(const PlayerPose& pose, float sky, float blockLight)
{
    m_vertices.clear();

    float pitch = 0.2617994f;
    float roll = -0.2617994f;
    float drop = 0.0f;
    float yaw = 0.0f;
    if (pose.gliding)
    {
        const float open = 1.0f - std::pow(std::clamp(pose.fall, 0.0f, 1.0f), 1.5f);
        pitch = open * 0.34906584f + (1.0f - open) * pitch;
        roll = open * -1.5707964f + (1.0f - open) * roll;
    }
    else if (pose.sneaking)
    {
        pitch = 0.6981317f;
        roll = -0.7853982f;
        drop = 3.0f;
        yaw = 0.08726646f;
    }

    std::vector<Box> wings;
    for (int side = 0; side < 2; ++side)
    {
        const float s = side == 0 ? -1.0f : 1.0f;
        Box wing;
        wing.min = glm::vec3(-5.0f, 4.0f - drop, -4.0f);
        wing.max = glm::vec3(5.0f, 24.0f - drop, -2.0f);
        wing.pivot = glm::vec3(5.0f * s, 24.0f - drop, -2.0f);
        wing.u = 22;
        wing.v = 0;
        wing.inflate = 1.0f;
        wing.mirror = side == 1;
        wing.pitch = -pitch;
        wing.roll = roll * s;
        wing.yaw = yaw * s;
        wing.jointed = true;
        wings.push_back(wing);
    }

    BoxMesh::Frame frame;
    frame.feet = pose.feet;
    frame.bodyYaw = pose.bodyYaw;
    frame.sky = sky;
    frame.blockLight = blockLight;
    frame.textureWidth = 64.0f;
    frame.textureHeight = 32.0f;
    frame.jointAngle = pose.sneaking ? SNEAK_TILT : 0.0f;
    frame.jointPivot = glm::vec3(0.0f, HIP_Y, 0.0f);
    if (pose.swimming)
    {
        frame.bodyPitch = glm::radians(pose.inWater ? std::clamp(90.0f - pose.headPitch, 10.0f, 170.0f) : 90.0f);
        frame.bodyPivot = glm::vec3(0.0f, 16.0f, 0.0f);
        frame.bodyShift = glm::vec3(0.0f, -11.0f, 0.0f);
    }
    BoxMesh::appendAll(m_vertices, wings, frame);
}

void PlayerModel::renderElytra(Shader& chunkShader, const World& world, const PlayerPose& pose,
                               unsigned int glint, const glm::vec2& shift)
{
    if (!m_elytraLoaded)
    {
        m_elytraLoaded = true;
        int width = 0, height = 0, channels = 0;
        unsigned char* data = Packs::load("assets/textures/entity/elytra.png", &width, &height, &channels, 4);
        if (data)
        {
            glGenTextures(1, &m_elytraTexture);
            glBindTexture(GL_TEXTURE_2D, m_elytraTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            stbi_image_free(data);
        }
    }
    if (!m_elytraTexture) return;

    int lx = static_cast<int>(std::floor(pose.feet.x));
    int ly = static_cast<int>(std::floor(pose.feet.y + 1.6f));
    int lz = static_cast<int>(std::floor(pose.feet.z));
    for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step) ++ly;
    buildElytra(pose, static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f,
                static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f);
    if (m_vertices.empty()) return;

    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_elytraTexture);
    m_mesh.draw(GL_TRIANGLES);
    if (glint)
    {
        chunkShader.setInt("uGlint", 1);
        chunkShader.setInt("uGlintTexture", 5);
        chunkShader.setVec2("uGlintShift", shift);
        chunkShader.setFloat("uGlintScale", 1.0f);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, glint);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        m_mesh.draw(GL_TRIANGLES);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
        chunkShader.setInt("uGlint", 0);
    }
    glEnable(GL_CULL_FACE);
}

void PlayerModel::renderArmour(Shader& chunkShader, const World& world, const PlayerPose& pose, int piece,
                               unsigned int texture, float inflateExtra)
{
    if (!texture) return;
    int lx = static_cast<int>(std::floor(pose.feet.x));
    int ly = static_cast<int>(std::floor(pose.feet.y + 1.6f));
    int lz = static_cast<int>(std::floor(pose.feet.z));
    for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step) ++ly;
    buildArmour(pose, piece, static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f,
                static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f, inflateExtra);
    if (m_vertices.empty()) return;
    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    m_mesh.draw(GL_TRIANGLES);
    glEnable(GL_CULL_FACE);
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
