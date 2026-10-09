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
class HeldItem
{
public:
    static constexpr float SWING_SECONDS = 0.25f;

    // Started when you swing at something, whether or not you hit it.
    void swing() { m_swing = SWING_SECONDS; }
    void update(float deltaTime);

    // `stack` is what the hotbar has selected; Blocks::Air draws a bare
    // hand. Sky and block light come from where the player is standing,
    // so your hand darkens with the room you are in.
    void render(Shader& chunkShader, const Camera& camera, const PlayerSkin& skin,
                const Atlas& atlas, StackId stack, float sky, float blockLight,
                bool sneaking);

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
