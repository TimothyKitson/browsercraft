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
    // Six ticks, which is what Minecraft gives a swing.
    static constexpr float SWING_SECONDS = 0.30f;

    // How fast something new is raised into the hand. Minecraft moves
    // it four tenths of the way each tick, twenty ticks a second, and
    // swaps what is drawn once it is nearly all the way down.
    static constexpr float EQUIP_RATE = 8.0f;

    // Started when you swing at something, whether or not you hit it.
    void swing() { m_swing = SWING_SECONDS; }

    // Whether one is still running. Mining asks before starting
    // another: calling swing() every frame, as it used to, reset the
    // timer before it could run down and pinned the arm near the start
    // of its arc, so holding the button showed a hand held still rather
    // than one swinging over and over.
    bool swinging() const { return m_swing > 0.0f; }

    // `held` is what the hotbar has selected. Passed in so the hand can
    // dip and come back up when it changes, which is Minecraft's only
    // cue that you have swapped what you are carrying.
    void update(float deltaTime, StackId held = Blocks::Air);

    // 0 at rest, 1 at the far end of the swing. Minecraft hangs every
    // part of the first-person pose off this one number.
    float swingProgress() const;

    // 0 while something new is being raised, 1 once it is up.
    float equipProgress() const { return m_equip; }

    // Minecraft's own first-person pose, in the camera's own frame:
    // where the hand goes, how far it is turned, and what the swing
    // does to both. `item` picks the pose for something held over the
    // pose for an empty hand -- Minecraft has two, and they differ.
    // Public so --selftest can walk a swing through it without a
    // window: an identity camera makes the answer camera-space.
    glm::mat4 pose(const glm::mat4& camera, bool item, bool sneaking = false,
                   bool blocking = false) const;

    // Only --selftest sets these directly, to stand the hand at a
    // chosen point in a swing or a swap rather than waiting one out.
    void setSwingForTest(float secondsLeft) { m_swing = secondsLeft; }
    void setEquipForTest(float raised) { m_equip = raised; }

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
    float m_equip = 1.0f;
    StackId m_shown = Blocks::Air;

    float swingAngle() const;
    void appendArm(const glm::mat4& toWorld, const PlayerSkin& skin,
                   float sky, float blockLight);
    void appendBlock(const glm::mat4& toWorld, StackId stack, float sky, float blockLight);
    void appendSprite(const glm::mat4& toWorld, StackId stack, float sky, float blockLight);

    // The shield, which is the one held thing that is not a block and
    // not a flat card: a plank panel with a handle behind it, off its
    // own sheet. Drawn as a panel because a shield seen edge-on while
    // you block should have an edge.
    void appendShield(const glm::mat4& toWorld, float sky, float blockLight);
    void appendShieldBox(const glm::mat4& toWorld, int u, int v,
                         float w, float h, float d,
                         float ox, float oy, float oz, float sky, float blockLight);
    unsigned int shieldTexture();

    unsigned int m_shieldTexture = 0;
    bool m_shieldLookedFor = false;
};
