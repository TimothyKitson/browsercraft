#pragma once
#include "Renderer/Mesh.h"
#include "Game/Items.h"
#include <glm/glm.hpp>
#include <vector>

class Shader;
class Camera;
class PlayerSkin;
class Atlas;

// The arm in the bottom corner of the screen, and whatever it is holding.
//
// Built in world space against the camera's own axes rather than through
// a separate view matrix, so it goes through the terrain shader with
// everything else. It is drawn last, after the depth buffer is cleared,
// which is what stops your own hand being swallowed by the wall you are
// standing against.
// Where along its texture patch a point on the first-person arm sits.
//
// The arm box is built with the hand at its top and the shoulder below,
// but a skin draws an arm the other way up: sleeve at the top of the
// patch, hand at the bottom. Getting this backwards puts the cuff on
// the knuckles, which is exactly what it did, so the rule is named and
// tested rather than buried in a UV table.
//
// `alongArm` is 0 at the shoulder and 1 at the hand. The result is the
// fraction down the patch to sample.
namespace FirstPersonArm
{
    inline float patchV(float alongArm) { return alongArm; }
}

class HeldItem
{
public:
    static constexpr float SWING_SECONDS = 0.25f;

    // Started when you swing at something, whether or not you hit it.
    void swing() { m_swing = SWING_SECONDS; }

    // Whether one is still running. Mining asks before starting
    // another: calling swing() every frame, as it used to, reset the
    // timer before it could run down and pinned the arm near the start
    // of its arc, so holding the button showed a hand held still rather
    // than one swinging over and over.
    bool swinging() const { return m_swing > 0.0f; }
    void update(float deltaTime);

    // `stack` is what the hotbar has selected; Blocks::Air draws a bare
    // hand. Sky and block light come from where the player is standing,
    // so your hand darkens with the room you are in.
    void render(Shader& chunkShader, const Camera& camera, const PlayerSkin& skin,
                const Atlas& atlas, StackId stack, float sky, float blockLight,
                bool sneaking, bool blocking = false);

private:
    Mesh m_mesh;
    std::vector<float> m_vertices;
    float m_swing = 0.0f;

    float swingAngle() const;
    void appendArm(const glm::mat4& toWorld, const PlayerSkin& skin,
                   float sky, float blockLight);
    void appendBlock(const glm::mat4& toWorld, StackId stack, float sky, float blockLight);
    void appendSprite(const glm::mat4& toWorld, StackId stack, float sky, float blockLight);
};
