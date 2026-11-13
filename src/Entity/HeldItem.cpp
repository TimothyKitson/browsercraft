#include "HeldItem.h"
#include "BoxMesh.h"
#include "PlayerSkin.h"
#include "Renderer/Atlas.h"
#include "Renderer/Camera.h"
#include "Renderer/Shader.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdio>
#include <string>
#include <stb_image.h>
#include "Core/Packs.h"

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    // Where the hand goes, in the camera's own frame, and it is
    // Minecraft's: the vanilla client translates to
    //
    //     (0.7, -0.65, -0.9) * 0.8    with something held
    //     (0.8, -0.75, -0.9) * 0.8    with an empty hand
    //
    // which is what the three pairs below come to. Its forward axis is
    // -z and ours is +z, so the depth changes sign and nothing else
    // does.
    constexpr float ITEM_RIGHT = 0.7f * 0.8f;
    constexpr float ITEM_DOWN = 0.65f * 0.8f;
    constexpr float ITEM_FORWARD = 0.9f * 0.8f;

    // The empty hand is not simply the pose above with nothing in it.
    // Mojang puts the arm through a stack of its own after it --
    //
    //     translate(-1, 3.6, 3.5)  rotateZ(120)  rotateX(200)
    //     rotateY(-135)  translate(5.6, 0, 0)
    //
    // -- and then draws the biped's right arm, a 4x12x4 box pinned at
    // (-5, 2, 0), at a sixteenth scale. Walking the fist end of that
    // box through the whole chain puts it at (0.74, -0.48, -1.14) in
    // the camera's frame, which is where these three come from: a
    // fist about three quarters of the way across the screen and four
    // fifths of the way down it, which is exactly where Minecraft's
    // is. The arm below is built out from that point, so what matters
    // is that the point is right rather than that the chain is
    // replayed in full.
    constexpr float HAND_RIGHT = 0.7355f;
    constexpr float HAND_DOWN = 0.4800f;
    constexpr float HAND_FORWARD = 1.1379f;

    // How far the hand drops while something new is being raised.
    constexpr float EQUIP_DROP = 0.6f;

    // The arm's own origin is its fist, so the offsets above place the
    // fist directly and there is nothing further to nudge.
    constexpr float HAND_LOCAL_X = 0.0f;
    constexpr float HAND_LOCAL_Y = 0.0f;
    constexpr float HAND_LOCAL_Z = 0.0f;

    // Minecraft's first-person arm is a slim thing tucked into the
    // bottom corner -- about a tenth of the screen across -- long
    // enough that the shoulder runs off the bottom edge, so only the
    // fist end is ever in shot.
    constexpr float ARM_HALF_WIDTH = 0.100f;
    constexpr float ARM_HALF_DEPTH = 0.100f;

    // An arm's texture patch is four pixels across and twelve tall, so
    // the box has to be three times as long as it is wide or the texels
    // come out taller than they are wide and the whole thing reads as
    // stretched.
    constexpr float ARM_TEXEL_RATIO = 3.0f;   // 12 tall / 4 wide
    constexpr float ARM_LENGTH = ARM_HALF_WIDTH * 2.0f * ARM_TEXEL_RATIO;

    // Radians, leaning the arm so it rises from the bottom-right corner
    // towards the middle of the screen. It has to be zero: a lean tips
    // the cut end of the fist sideways, and what should be a level edge
    // across the knuckles comes out as a slant.
    constexpr float ARM_LEAN = 0.0f;

    // How far past edge-on to carry the cut end, in radians. Zero, now
    // that the end caps take the fist's patch rather than the sleeve's.
    constexpr float ARM_CAP_MARGIN = 0.0f;

    // Minecraft scales a held block to four tenths of a block and
    // draws it about its own middle, so it is 0.4 across however the
    // hand is turned. A flat item is that and then half as much again,
    // which is Mojang's 1.5 in renderItem.

    // And a flat item is a slab a sixteenth of a block thick rather
    // than a card with no edge, which is what you see when you turn one
    // side-on.

    constexpr float PI = 3.14159265f;

    float degrees(float d) { return d * PI / 180.0f; }

    constexpr float SHIELD_SHEET = 64.0f;
    const char* SHIELD_TEXTURE = "assets/skins/shield.png";

    // Face 6 tells the chunk shader to skip tangent-space normal mapping,
    // which neither a skin nor an item sprite has maps for.
    constexpr float NO_NORMAL_MAP = 6.0f;

    // The held block does have maps, but its faces are turned by the
    // arm, so the world-space frames the shader keeps for the six cube
    // faces are no use to it. Seven tells the shader to work the frame
    // out from the triangle itself, which holds whichever way it is
    // pointing.
    constexpr float DERIVED_BASIS = 7.0f;

    struct Quad
    {
        glm::vec3 corner[4];
        float u[4], v[4];
        float shade;
    };

    void pushQuad(std::vector<float>& out, const Quad& q, float sky, float blockLight,
                  float face = NO_NORMAL_MAP)
    {
        // Reversed, for the same reason BoxMesh reverses its own: these
        // corners are given counter-clockwise seen from outside in the
        // camera's own frame, and that frame -- right, up, forward -- is
        // left-handed, because a camera's right is up cross forward
        // rather than forward cross up. Wound the obvious way round, every
        // face comes out clockwise in world space and back-face culling
        // throws away the side facing you, leaving the inside of the arm
        // and of whatever it is holding on show.
        const int order[6] = { 0, 2, 1, 0, 3, 2 };
        for (int k : order)
        {
            out.push_back(q.corner[k].x);
            out.push_back(q.corner[k].y);
            out.push_back(q.corner[k].z);
            out.push_back(q.u[k]);
            out.push_back(q.v[k]);
            out.push_back(q.shade);
            out.push_back(sky);
            out.push_back(blockLight);
            out.push_back(face);
        }
    }

    glm::vec3 place(const glm::mat4& m, float x, float y, float z)
    {
        return glm::vec3(m * glm::vec4(x, y, z, 1.0f));
    }

    // The same, wound so the quad faces away from `centre` whatever the
    // transform did to its handedness: the held things are built in
    // Minecraft's frame and carried into ours through a mirror.
    void pushFacing(std::vector<float>& out, const Quad& q, const glm::vec3& centre,
                    float sky, float blockLight, float face = NO_NORMAL_MAP)
    {
        const glm::vec3 normal = glm::cross(q.corner[1] - q.corner[0], q.corner[2] - q.corner[0]);
        const glm::vec3 middle = (q.corner[0] + q.corner[1] + q.corner[2] + q.corner[3]) * 0.25f;
        const bool outward = glm::dot(normal, middle - centre) >= 0.0f;
        const int forwards[6] = { 0, 1, 2, 0, 2, 3 };
        const int backwards[6] = { 0, 2, 1, 0, 3, 2 };
        for (int k : (outward ? forwards : backwards))
        {
            out.push_back(q.corner[k].x);
            out.push_back(q.corner[k].y);
            out.push_back(q.corner[k].z);
            out.push_back(q.u[k]);
            out.push_back(q.v[k]);
            out.push_back(q.shade);
            out.push_back(sky);
            out.push_back(blockLight);
            out.push_back(face);
        }
    }

    // A turn about one of Minecraft's axes, in degrees, as its
    // RotationAxis gives one.
    glm::mat4 mcRotate(const glm::vec3& axis, float degreesTurned)
    {
        return glm::rotate(glm::mat4(1.0f), glm::radians(degreesTurned), axis);
    }

    // Minecraft's camera looks down -z and ours down +z.
    const glm::mat4 FLIP_FORWARD = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -1.0f));

    const int CUBE_FACE[6][4][3] = {
        { {1,0,0}, {1,1,0}, {1,1,1}, {1,0,1} },
        { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} },
        { {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0} },
        { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} },
        { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} },
        { {0,0,0}, {0,1,0}, {1,1,0}, {1,0,0} },
    };

    const float CUBE_UV[6][4][2] = {
        { {0,1}, {0,0}, {1,0}, {1,1} },
        { {0,1}, {1,1}, {1,0}, {0,0} },
        { {0,0}, {0,1}, {1,1}, {1,0} },
        { {0,0}, {1,0}, {1,1}, {0,1} },
        { {0,1}, {1,1}, {1,0}, {0,0} },
        { {0,1}, {0,0}, {1,0}, {1,1} },
    };

    const float CUBE_SHADE[6] = { 0.72f, 0.72f, 1.00f, 0.50f, 0.86f, 0.86f };

    // A model's first-person transform, as its JSON gives it: degrees,
    // sixteenths of a block, and a scale.
    struct Display
    {
        glm::vec3 rotation;
        glm::vec3 translation;
        glm::vec3 scale;
    };

    const Display BLOCK_DISPLAY{ { 0.0f, 45.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, glm::vec3(0.4f) };
    const Display ORIENTABLE_DISPLAY{ { 0.0f, 135.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, glm::vec3(0.4f) };
    const Display CHEST_DISPLAY{ { 0.0f, 315.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, glm::vec3(0.4f) };
    const Display ITEM_DISPLAY{ { 0.0f, -90.0f, 25.0f }, { 1.13f, 3.2f, 1.13f }, glm::vec3(0.68f) };
    const Display MACE_DISPLAY{ { 0.0f, -90.0f, 25.0f }, { 0.0f, 3.0f, 0.8f }, glm::vec3(0.9f) };
    const Display SHIELD_DISPLAY{ { 0.0f, 180.0f, 5.0f }, { -10.0f, 2.0f, -10.0f }, glm::vec3(1.25f) };
    const Display SPYGLASS_DISPLAY{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, glm::vec3(1.0f) };
    const Display SHIELD_BLOCKING_DISPLAY{ { 0.0f, 180.0f, -5.0f }, { -15.0f, 5.0f, -11.0f },
                                           glm::vec3(1.25f) };

    glm::mat4 displayMatrix(const Display& d)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), d.translation / 16.0f);
        m = glm::rotate(m, glm::radians(d.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, glm::radians(d.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, glm::radians(d.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, d.scale);
        return glm::translate(m, glm::vec3(-0.5f));
    }

    // The blocks whose item model turns their face to you: the
    // orientable ones at 135 degrees, and a chest the other way at 315.
    bool orientable(BlockId id)
    {
        switch (id)
        {
            case Blocks::Furnace: case Blocks::FurnaceLit:
            case Blocks::BlastFurnace: case Blocks::BlastFurnaceLit:
            case Blocks::Smoker: case Blocks::SmokerLit:
            case Blocks::Dispenser: case Blocks::Dropper:
            case Blocks::Pumpkin: case Blocks::Lectern:
                return true;
            default:
                return false;
        }
    }

    bool drawsFlat(StackId id)
    {
        return isItem(id) || isCross(asBlock(id)) || isBanner(asBlock(id));
    }
}

bool HeldItem::flatInHand(StackId id)
{
    return drawsFlat(id);
}

void HeldItem::update(float deltaTime, StackId held)
{
    if (m_swing > 0.0f) m_swing = std::max(0.0f, m_swing - deltaTime);

    // Swapping what you are carrying drops the hand out of shot and
    // raises the new thing in its place, which is Minecraft's only
    // cue that the swap happened. What is drawn changes at the bottom
    // of the dip, so you never see one item turn into another.
    const float want = (held == m_shown) ? 1.0f : 0.0f;
    const float step = EQUIP_RATE * deltaTime;
    m_equip = std::clamp(m_equip + std::clamp(want - m_equip, -step, step), 0.0f, 1.0f);
    if (m_equip < 0.1f) m_shown = held;
}

float HeldItem::swingProgress() const
{
    if (m_swing <= 0.0f) return 0.0f;
    return 1.0f - (m_swing / SWING_SECONDS);
}

// Minecraft's own first-person pose, out of 1.21's HeldItemRenderer.
// With something held:
//
//   translate(-0.4 sin(sqrt(p) PI), 0.2 sin(sqrt(p) 2 PI), -0.2 sin(p PI))
//   translate(0.56, -0.52 - 0.6 (1 - equipped), -0.72)
//   rotateY(45 - 20 sin(p p PI))  rotateZ(-20 sin(sqrt(p) PI))
//   rotateX(-80 sin(sqrt(p) PI))  rotateY(-45)
//
// built in Minecraft's own frame, which looks down -z, and carried into
// ours by flipping z once at the end. An empty hand keeps the fist pose
// worked out from the arm's own stack, which 1.21 has unchanged.
glm::mat4 HeldItem::pose(const glm::mat4& camera, bool item, bool, bool) const
{
    const float p = swingProgress();
    const float arc = std::sin(std::sqrt(p) * PI);
    const float lift = std::sin(std::sqrt(p) * PI * 2.0f);
    const float punch = std::sin(p * PI);

    if (item)
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(-0.4f * arc, 0.2f * lift, -0.2f * punch));
        m = glm::translate(m, glm::vec3(ITEM_RIGHT, -ITEM_DOWN - EQUIP_DROP * (1.0f - m_equip),
                                        -ITEM_FORWARD));
        m = m * mcRotate(glm::vec3(0.0f, 1.0f, 0.0f), 45.0f - 20.0f * std::sin(p * p * PI));
        m = m * mcRotate(glm::vec3(0.0f, 0.0f, 1.0f), -20.0f * arc);
        m = m * mcRotate(glm::vec3(1.0f, 0.0f, 0.0f), -80.0f * arc);
        m = m * mcRotate(glm::vec3(0.0f, 1.0f, 0.0f), -45.0f);
        return camera * FLIP_FORWARD * m;
    }

    const float right = HAND_RIGHT - arc * 0.3f;
    const float down = HAND_DOWN + (1.0f - m_equip) * EQUIP_DROP - lift * 0.4f;
    const float forward = HAND_FORWARD + punch * 0.4f;

    const glm::vec3 origin = glm::vec3(camera[3]) +
                             glm::vec3(camera[0]) * right +
                             glm::vec3(camera[1]) * -down +
                             glm::vec3(camera[2]) * forward;

    const float yaw = degrees(-45.0f - arc * 70.0f);
    const float roll = degrees(-std::sin(p * p * PI) * 20.0f);

    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float cr = std::cos(roll), sr = std::sin(roll);

    const glm::mat3 Ry(cy, 0.0f, -sy,  0.0f, 1.0f, 0.0f,  sy, 0.0f, cy);
    const glm::mat3 Rz(cr, sr, 0.0f,  -sr, cr, 0.0f,  0.0f, 0.0f, 1.0f);
    const glm::mat3 turn = Ry * Rz;

    const glm::mat3 basis{ glm::vec3(camera[0]), glm::vec3(camera[1]), glm::vec3(camera[2]) };
    const glm::mat3 axes = basis * turn;

    glm::mat4 out(1.0f);
    out[0] = glm::vec4(axes[0], 0.0f);
    out[1] = glm::vec4(axes[1], 0.0f);
    out[2] = glm::vec4(axes[2], 0.0f);
    out[3] = glm::vec4(origin, 1.0f);
    return out;
}

// Up and back on the way out, down and forward on the way in, which is
// what makes a swing read as a swing rather than a twitch.
float HeldItem::swingAngle() const
{
    if (m_swing <= 0.0f) return 0.0f;
    const float t = 1.0f - (m_swing / SWING_SECONDS);
    return std::sin(t * 3.14159265f);
}

void HeldItem::appendArm(const glm::mat4& toWorld, const PlayerSkin& skin,
                         float sky, float blockLight)
{
    const float w = static_cast<float>(skin.armWidth());
    const float texW = static_cast<float>(skin.width());
    const float texH = static_cast<float>(skin.height());

    // The right arm out of the skin, turned in the plane of the screen
    // so it rises from the bottom-right corner with the fist towards
    // the middle.
    //
    // Turning it in the screen's plane alone is not enough to hide the
    // cut end of the box. The hand sits well off to the right, so
    // perspective lets you see round it and the cross-section shows at
    // the knuckles -- an arm that looks like a tube you are peering
    // down. The cure is to pitch the arm until that end is edge-on
    // *from the eye*, which also happens to be the truth of it: your
    // fist really is further from your eye than your shoulder is.
    //
    // The angle for that is not a number to guess at -- it falls out of
    // where the hand is, so it is worked out here and follows the hand
    // if it ever moves.
    //
    // Held exactly edge-on, and with no lean, the end of the fist faces
    // straight ahead: the square cut projects to a level line across the
    // knuckles, which is the flat end Minecraft shows. Any margin past
    // edge-on hides the cut behind the arm, and what bounds the top of
    // the arm instead is one edge of that square -- turned out of true
    // by the spin below, so the fist tapers off at an angle.
    const float cl = std::cos(ARM_LEAN), sl = std::sin(ARM_LEAN);

    const float tilt = std::atan2(cl * -HAND_DOWN - sl * HAND_RIGHT, HAND_FORWARD)
                       - ARM_CAP_MARGIN;
    const float ct = std::cos(tilt), st = std::sin(tilt);

    glm::mat4 lean(1.0f);
    lean[0] = glm::vec4(cl, sl, 0.0f, 0.0f);
    lean[1] = glm::vec4(-sl * ct, cl * ct, -st, 0.0f);
    lean[2] = glm::vec4(-sl * st, cl * st, ct, 0.0f);
    lean[3] = glm::vec4(HAND_LOCAL_X, HAND_LOCAL_Y, HAND_LOCAL_Z, 1.0f);
    // Spun about its own length so two of the long faces show at once
    // and the arm reads as a box seen corner-on rather than as a flat
    // slab facing you. Measured against the line from the eye to the
    // hand, like the block's turn, because the hand sits far enough off
    // to the side that that line and the camera's axis differ. Spinning
    // about the arm's own axis leaves the end cap pointing where it was,
    // so the tilt above still hides it.
    // A further quarter turn on top of the eighth: the cross-section is
    // square, so it leaves the silhouette alone and only swaps which pair
    // of faces is which, putting the brighter one on the left where
    // Minecraft has it rather than on the right.
    const float spinAngle = std::atan2(HAND_FORWARD, HAND_RIGHT) + 0.7853982f;
    const float cs = std::cos(spinAngle), ss = std::sin(spinAngle);
    glm::mat4 spin(1.0f);
    spin[0] = glm::vec4(cs, 0.0f, ss, 0.0f);
    spin[2] = glm::vec4(-ss, 0.0f, cs, 0.0f);

    const glm::mat4 arm = toWorld * lean * spin;

    const float halfW = ARM_HALF_WIDTH * (w / 4.0f);
    const float halfD = ARM_HALF_DEPTH;
    const float length = ARM_LENGTH;

    for (int face = 0; face < 6; ++face)
    {
        // The box has the fist at its top and the shoulder at its
        // bottom; the skin has them the other way up. That applies to
        // the two end caps as much as to the long faces, so they swap
        // patches -- otherwise the end of the fist is textured with the
        // top of the sleeve, and the sliver of it you can see at the
        // knuckles comes out teal.
        const int patchFace = (face == 2) ? 3 : (face == 3) ? 2 : face;

        int px = 0, py = 0, pw = 0, ph = 0;
        BoxMesh::facePatch(patchFace, 40, 16, static_cast<int>(w), 12, 4, px, py, pw, ph);

        // The four long faces take their v from where the corner
        // actually is along the arm, rather than from the cube's own
        // table: the box has the hand at its top and the skin has the
        // hand at the bottom of its patch, and reading the table
        // straight is what put the cuff on the knuckles.
        const bool longFace = (face != 2 && face != 3);

        Quad q;
        q.shade = CUBE_SHADE[face];
        for (int k = 0; k < 4; ++k)
        {
            const int* c = CUBE_FACE[face][k];
            q.corner[k] = place(arm,
                                (c[0] ? halfW : -halfW),
                                (c[1] ? 0.0f : -length),
                                (c[2] ? halfD : -halfD));

            // c[1] is 1 at the hand end of the box, 0 at the shoulder.
            const float vEdge = longFace
                                    ? FirstPersonArm::patchV(c[1] ? 1.0f : 0.0f)
                                    : CUBE_UV[face][k][1];

            q.u[k] = (px + CUBE_UV[face][k][0] * pw) / texW;
            q.v[k] = (py + vEdge * ph) / texH;
        }
        pushQuad(m_vertices, q, sky, blockLight);
    }
}

// A block in the hand, at block.json's first-person transform: turned
// 45 degrees about the upright and scaled to 0.4, about its own middle.
// The faces take their textures the way Minecraft's model faces do.
void HeldItem::appendBlock(const glm::mat4& toWorld, StackId stack, float sky, float blockLight)
{
    const BlockId block = asBlock(stack);
    const bool turned = orientable(block);
    const bool chest = isChestFamily(block);
    const Display& display = turned ? ORIENTABLE_DISPLAY : chest ? CHEST_DISPLAY : BLOCK_DISPLAY;

    const glm::mat4 model = toWorld * displayMatrix(display);
    const glm::vec3 centre = place(model, 0.5f, 0.5f, 0.5f);

    struct Face { glm::vec3 corner[4]; int tile; float shade; };
    const BlockInfo& info = blockInfo(block);

    // Where the face goes: north on an orientable model, which the turn
    // above brings round to you; south on a chest; and west on any other
    // cube with a face, which is the side Minecraft's 45 degrees shows.
    const bool hasFront = info.tileFront >= 0;
    const int front = hasFront ? info.tileFront : info.tileSide;
    const int westTile = (hasFront && !turned && !chest) ? front : info.tileSide;
    const int northTile = (hasFront && turned) ? front : info.tileSide;
    const int southTile = (hasFront && chest) ? front : info.tileSide;

    auto side = [&](int axis, float at, float shade, int tile) {
        Face f;
        f.tile = tile;
        f.shade = shade;
        const int ua = axis == 0 ? 2 : 0;
        const int va = axis == 1 ? 2 : 1;
        const float cu[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
        const float cv[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        for (int k = 0; k < 4; ++k)
        {
            glm::vec3 c(0.0f);
            c[axis] = at;
            c[ua] = cu[k];
            c[va] = cv[k];
            f.corner[k] = c;
        }
        return f;
    };

    const Face faces[6] = {
        side(0, 1.0f, 0.6f, info.tileSide),
        side(0, 0.0f, 0.6f, westTile),
        side(1, 1.0f, 1.0f, info.tileTop),
        side(1, 0.0f, 0.5f, info.tileBottom),
        side(2, 1.0f, 0.8f, southTile),
        side(2, 0.0f, 0.8f, northTile),
    };
    const int axisOf[6] = { 0, 0, 1, 1, 2, 2 };
    const bool positive[6] = { true, false, true, false, true, false };

    for (int i = 0; i < 6; ++i)
    {
        const Face& f = faces[i];
        const TileUV uv = tileUV(f.tile);
        Quad q;
        q.shade = f.shade;
        for (int k = 0; k < 4; ++k)
        {
            const glm::vec3 c = f.corner[k];
            float u = 0.0f, v = 0.0f;
            switch (axisOf[i])
            {
                case 0: u = positive[i] ? 1.0f - c.z : c.z; v = 1.0f - c.y; break;
                case 1: u = c.x; v = positive[i] ? c.z : 1.0f - c.z; break;
                default: u = positive[i] ? c.x : 1.0f - c.x; v = 1.0f - c.y; break;
            }
            q.corner[k] = place(model, c.x, c.y, c.z);
            q.u[k] = uv.u0 + u * (uv.u1 - uv.u0);
            q.v[k] = uv.vTop + v * (uv.vBottom - uv.vTop);
        }
        pushFacing(m_vertices, q, centre, sky, blockLight, DERIVED_BASIS);
    }
}

unsigned int HeldItem::shieldLookTexture()
{
    const unsigned int look = lookTexture(m_lookColour, m_lookLayers);
    return look ? look : shieldTexture();
}

unsigned int HeldItem::lookTexture(int baseColour, const Banners::Design& layers)
{
    if (!m_shieldSheetsLoaded)
    {
        m_shieldSheetsLoaded = true;
        auto load = [](const std::string& path, Banners::Sheet& sheet) {
            int w = 0, h = 0, channels = 0;
            unsigned char* data = Packs::load(path.c_str(), &w, &h, &channels, 4);
            if (!data) return;
            sheet.width = w;
            sheet.height = h;
            sheet.rgba.assign(data, data + static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
            stbi_image_free(data);
        };
        load("assets/textures/entity/shield_base.png", m_shieldSheets.frame);
        for (int p = 0; p < Banners::PatternCount; ++p)
            load(Banners::shieldTexture(p), m_shieldSheets.patterns[static_cast<size_t>(p)]);
    }
    if (!m_shieldSheets.frame.loaded() || m_shieldSheets.frame.width != Banners::SHEET) return 0;

    const std::string key = Banners::key(baseColour, layers);
    const auto found = m_shieldLooks.find(key);
    if (found != m_shieldLooks.end()) return found->second;

    std::vector<uint8_t> pixels;
    Banners::compose(baseColour, layers, m_shieldSheets, pixels);
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Banners::SHEET, Banners::SHEET, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_shieldLooks[key] = texture;
    return texture;
}

unsigned int HeldItem::shieldTexture()
{
    // Looked for once. Without the sheet the shield falls back to its
    // item card, which is what a copy of the game with no art installed
    // has always drawn.
    if (m_shieldLookedFor) return m_shieldTexture;
    m_shieldLookedFor = true;

    int w = 0, h = 0, channels = 0;
    unsigned char* data = Packs::load(SHIELD_TEXTURE, &w, &h, &channels, 4);
    if (!data) data = Packs::load("assets/textures/entity/shield_base_nopattern.png", &w, &h, &channels, 4);
    if (!data) return 0;

    if (w != 64 || h != 64)
    {
        std::printf("Shield texture ignored: expected 64x64, got %dx%d\n", w, h);
        stbi_image_free(data);
        return 0;
    }

    glGenTextures(1, &m_shieldTexture);
    glBindTexture(GL_TEXTURE_2D, m_shieldTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    std::printf("Shield: loaded %s (%dx%d)\n", SHIELD_TEXTURE, w, h);
    return m_shieldTexture;
}

// One of the shield's two boxes, built the way Minecraft's ModelPart
// builds a cuboid: corners in model pixels, and each face's patch of the
// 64-square sheet where its uv rule puts it.
void HeldItem::appendShieldBox(const glm::mat4& toWorld, int u, int v,
                               float w, float h, float d,
                               float ox, float oy, float oz,
                               float sky, float blockLight)
{
    const float x0 = ox, y0 = oy, z0 = oz;
    const float x1 = ox + w, y1 = oy + h, z1 = oz + d;

    const glm::vec3 p[8] = {
        { x0, y0, z0 }, { x1, y0, z0 }, { x1, y1, z0 }, { x0, y1, z0 },
        { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 },
    };

    const float j = static_cast<float>(u);
    const float k = u + d;
    const float l = u + d + w;
    const float m = u + d + w + w;
    const float n = u + d + w + d;
    const float o = u + d + w + d + w;
    const float pv = static_cast<float>(v);
    const float q = v + d;
    const float r = v + d + h;

    struct Side { int corner[4]; float u1, v1, u2, v2; float shade; };
    const Side sides[6] = {
        { { 5, 4, 0, 1 }, k, pv, l, q, 1.0f },
        { { 2, 3, 7, 6 }, l, q, m, pv, 0.5f },
        { { 0, 4, 7, 3 }, j, q, k, r, 0.6f },
        { { 1, 0, 3, 2 }, k, q, l, r, 0.8f },
        { { 5, 1, 2, 6 }, l, q, n, r, 0.6f },
        { { 4, 5, 6, 7 }, n, q, o, r, 0.8f },
    };

    const glm::vec3 centre = place(toWorld, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, (z0 + z1) * 0.5f);

    for (const Side& side : sides)
    {
        const float us[4] = { side.u2, side.u1, side.u1, side.u2 };
        const float vs[4] = { side.v1, side.v1, side.v2, side.v2 };

        Quad quad;
        quad.shade = side.shade;
        for (int c = 0; c < 4; ++c)
        {
            const glm::vec3& at = p[side.corner[c]];
            quad.corner[c] = place(toWorld, at.x, at.y, at.z);
            quad.u[c] = us[c] / SHIELD_SHEET;
            quad.v[c] = vs[c] / SHIELD_SHEET;
        }
        pushFacing(m_vertices, quad, centre, sky, blockLight, NO_NORMAL_MAP);
    }
}

// The shield, out of 1.21: its item model's first-person transform,
// then the builtin renderer's flip of y and z, then ShieldEntityModel's
// plate of 12x22x1 at (-6,-11,-2) off (0,0) and handle of 2x6x6 at
// (-1,-3,-1) off (26,0), in sixteenths of a block.
void HeldItem::appendShield(const glm::mat4& toWorld, float sky, float blockLight)
{
    appendShieldBox(toWorld, 0, 0, 12.0f, 22.0f, 1.0f, -6.0f, -11.0f, -2.0f, sky, blockLight);
    appendShieldBox(toWorld, 26, 0, 2.0f, 6.0f, 6.0f, -1.0f, -3.0f, -1.0f, sky, blockLight);
}

// A flat item in the hand, at item/generated.json's first-person
// transform -- translation (1.13, 3.2, 1.13) sixteenths, rotation
// (0, -90, 25), scale 0.68 -- and then Minecraft's own sprite slab: the
// sixteen pixels square across the model, a pixel thick about its middle,
// with its edges between the two faces.
void HeldItem::renderGlint(Shader& chunkShader, unsigned int glint, const glm::vec2& shift)
{
    if (m_vertices.empty() || !glint) return;
    chunkShader.bind();
    chunkShader.setInt("uGlint", 1);
    chunkShader.setInt("uGlintTexture", 5);
    chunkShader.setVec2("uGlintShift", shift);
    chunkShader.setFloat("uGlintScale", 6.0f);
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

void HeldItem::appendSpyglass(const glm::mat4& toWorld, float sky, float blockLight)
{
    const TileUV uv = tileUV(Tiles::ItemSpyglassModel);
    const glm::mat4 model = toWorld * displayMatrix(SPYGLASS_DISPLAY);
    auto box = [&](glm::vec3 a, glm::vec3 b, const glm::vec4& side, const glm::vec4& up, const glm::vec4* down) {
        a /= 16.0f;
        b /= 16.0f;
        const glm::vec3 mid = (a + b) * 0.5f;
        const glm::vec3 centre = place(model, mid.x, mid.y, mid.z);
        auto quad = [&](glm::vec3 c0, glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, const glm::vec4& r, float shade) {
            const glm::vec3 cs[4] = { c0, c1, c2, c3 };
            const float us[4] = { r.x, r.z, r.z, r.x };
            const float vs[4] = { r.w, r.w, r.y, r.y };
            Quad q;
            q.shade = shade;
            for (int k = 0; k < 4; ++k)
            {
                q.corner[k] = place(model, cs[k].x, cs[k].y, cs[k].z);
                q.u[k] = uv.u0 + (uv.u1 - uv.u0) * us[k] / 16.0f;
                q.v[k] = uv.vTop + (uv.vBottom - uv.vTop) * vs[k] / 16.0f;
            }
            pushFacing(m_vertices, q, centre, sky, blockLight);
        };
        quad({ b.x, a.y, a.z }, { a.x, a.y, a.z }, { a.x, b.y, a.z }, { b.x, b.y, a.z }, side, 0.8f);
        quad({ a.x, a.y, b.z }, { b.x, a.y, b.z }, { b.x, b.y, b.z }, { a.x, b.y, b.z }, side, 0.8f);
        quad({ a.x, a.y, a.z }, { a.x, a.y, b.z }, { a.x, b.y, b.z }, { a.x, b.y, a.z }, side, 0.6f);
        quad({ b.x, a.y, b.z }, { b.x, a.y, a.z }, { b.x, b.y, a.z }, { b.x, b.y, b.z }, side, 0.6f);
        quad({ a.x, b.y, b.z }, { b.x, b.y, b.z }, { b.x, b.y, a.z }, { a.x, b.y, a.z }, up, 1.0f);
        if (down) quad({ a.x, a.y, a.z }, { b.x, a.y, a.z }, { b.x, a.y, b.z }, { a.x, a.y, b.z }, *down, 0.5f);
    };
    box({ 7.0f, 8.5f, 7.0f }, { 9.0f, 13.5f, 9.0f }, { 0.0f, 2.0f, 2.0f, 7.0f }, { 0.0f, 0.0f, 2.0f, 2.0f }, nullptr);
    const glm::vec4 eyepiece{ 0.0f, 13.0f, 2.0f, 15.0f };
    box({ 6.9f, 2.4f, 6.9f }, { 9.1f, 8.6f, 9.1f }, { 0.0f, 7.0f, 2.0f, 13.0f }, { 0.0f, 5.0f, 2.0f, 7.0f }, &eyepiece);
}

void HeldItem::appendSprite(const glm::mat4& toWorld, StackId stack, float sky, float blockLight)
{
    const TileUV uv = tileUV(m_tileOverride >= 0 ? m_tileOverride : atlasTileFor(stack));

    const glm::mat4 model = toWorld * displayMatrix(stack == Items::Mace ? MACE_DISPLAY
                                                                         : ITEM_DISPLAY);

    const int tile = m_tileOverride >= 0 ? m_tileOverride : atlasTileFor(stack);
    (void)uv;
    for (const SpriteFace& face : Atlas::extrudedSprite(tile))
    {
        Quad q;
        q.shade = face.shade;
        for (int k = 0; k < 4; ++k)
        {
            q.corner[k] = place(model, face.corner[k].x, face.corner[k].y, face.corner[k].z);
            q.u[k] = face.u[k];
            q.v[k] = face.v[k];
        }
        const glm::vec3 middle = (q.corner[0] + q.corner[1] + q.corner[2] + q.corner[3]) * 0.25f;
        const glm::vec3 local = (face.corner[0] + face.corner[2]) * 0.5f;
        const glm::vec3 inside = place(model, local.x - face.normal.x, local.y - face.normal.y, local.z - face.normal.z);
        pushFacing(m_vertices, q, middle + (inside - middle) * 0.01f, sky, blockLight);
    }
}

void HeldItem::render(Shader& chunkShader, const Camera& camera, const PlayerSkin& skin,
                      const Atlas& atlas, StackId stack, float sky, float blockLight,
                      bool sneaking, bool blocking)
{
    // Camera axes as a basis, so the pose is written in the obvious
    // local coordinates: +x right, +y up, +z forwards.
    glm::mat4 eye(1.0f);
    eye[0] = glm::vec4(camera.right, 0.0f);
    eye[1] = glm::vec4(camera.up, 0.0f);
    eye[2] = glm::vec4(camera.front, 0.0f);
    eye[3] = glm::vec4(camera.position, 1.0f);
    struct Mirror
    {
        bool on;
        explicit Mirror(bool left) : on(left) { if (on) glFrontFace(GL_CW); }
        ~Mirror() { if (on) glFrontFace(GL_CCW); }
    } mirror(m_leftHanded);
    if (m_leftHanded) eye[0] = -eye[0];

    // What is drawn is what has finished being raised rather than what
    // the hotbar says: halfway through a swap the old one is still on
    // its way down, and swapping the drawing at the top of the dip is
    // what would make one item turn into another in mid-air.
    const StackId showing = (m_shown == Blocks::Air && stack != Blocks::Air && m_equip >= 1.0f)
                                ? stack : m_shown;
    const bool empty = showing == Blocks::Air;

    // The hand is skin-textured and anything it holds comes off the atlas,
    // so they are two passes. Depth is cleared once, before either.
    glClear(GL_DEPTH_BUFFER_BIT);
    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));

    if (skin.textureId() && empty)
    {
        m_vertices.clear();
        appendArm(pose(eye, false, sneaking, blocking), skin, sky, blockLight);
        m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skin.textureId());
        m_mesh.draw(GL_TRIANGLES);
    }

    if (empty)
    {
        atlas.bind(0, 1, 2);
        return;
    }

    m_vertices.clear();

    const glm::mat4 held = pose(eye, true, sneaking, blocking);

    // The shield is its own pass: it has a sheet rather than an atlas
    // tile, and it is a panel rather than a card.
    if (showing == Items::Shield && shieldTexture() != 0)
    {
        // Raised, 1.21 holds it at the equip offset alone, with no swing,
        // and turns to shield_blocking's transform.
        glm::mat4 hand = held;
        if (blocking)
            hand = eye * FLIP_FORWARD *
                   glm::translate(glm::mat4(1.0f),
                                  glm::vec3(ITEM_RIGHT, -ITEM_DOWN - EQUIP_DROP * (1.0f - m_equip),
                                            -ITEM_FORWARD));

        const glm::mat4 panel = hand * displayMatrix(blocking ? SHIELD_BLOCKING_DISPLAY
                                                              : SHIELD_DISPLAY) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, -1.0f) / 16.0f);

        m_vertices.clear();
        appendShield(panel, sky, blockLight);
        m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_hasLook ? shieldLookTexture() : shieldTexture());
        m_mesh.draw(GL_TRIANGLES);

        atlas.bind(0, 1, 2);
        return;
    }

    if (showing == Items::Spyglass) appendSpyglass(held, sky, blockLight);
    else if (drawsFlat(showing)) appendSprite(held, showing, sky, blockLight);
    else                    appendBlock(held, showing, sky, blockLight);

    atlas.bind(0, 1, 2);
    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    m_mesh.draw(GL_TRIANGLES);
}
