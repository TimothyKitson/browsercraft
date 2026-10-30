#include "HeldItem.h"
#include "BoxMesh.h"
#include "PlayerSkin.h"
#include "Renderer/Atlas.h"
#include "Renderer/Camera.h"
#include "Renderer/Shader.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <stb_image.h>

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
    constexpr float BLOCK_SIZE = 0.4f;
    constexpr float ITEM_SCALE = 0.4f;
    constexpr float SPRITE_SCALE = ITEM_SCALE * 1.5f;

    // And a flat item is a slab a sixteenth of a block thick rather
    // than a card with no edge, which is what you see when you turn one
    // side-on.
    constexpr float SPRITE_DEPTH = 1.0f / 16.0f;

    constexpr float PI = 3.14159265f;

    float degrees(float d) { return d * PI / 180.0f; }

    // The shield's own sheet is 64x64 and its model is measured in the
    // same sixteenths-of-a-block as everything Mojang draws. Held in
    // the hand it is scaled down to about the size of the card it
    // replaces, or a 22-pixel plank fills a third of the screen.
    constexpr float SHIELD_SHEET = 64.0f;
    // How big a model unit is, for the shield alone.
    //
    // Mojang's shield.json asks for 1.25 of an item's usual size, but
    // that 1.25 is against Minecraft's own first-person item, which is
    // drawn a good deal larger than a held block is here -- so taking
    // the quarter-again against BLOCK_SIZE, which is what this used to
    // do, came out far too small.
    //
    // Measured off a vanilla screenshot instead: there the shield fills
    // half the screen's width and half its height AND is cut off by two
    // edges, so for a plate twelve units wide by twenty-two tall the
    // whole thing stands about one and a half screens high. You only
    // ever see its top corner. This is the per-unit size that puts
    // twelve units across about half the view.
    constexpr float SHIELD_SCALE = 0.032f;
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

    int cubeTile(StackId id, int face)
    {
        const BlockInfo& info = blockInfo(asBlock(id));
        if (face == 2) return info.tileTop;
        if (face == 3) return info.tileBottom;
        return info.tileSide;
    }

    bool drawsFlat(StackId id)
    {
        return isItem(id) || isCross(asBlock(id));
    }
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

// Minecraft's own first-person pose. Every number here is out of the
// vanilla client's ItemRenderer:
//
//   translate(-sin(sqrt(p)*PI) * 0.4, sin(sqrt(p)*PI*2) * 0.2, -sin(p*PI) * 0.2)
//   translate(0.56, -0.52 - (1 - equipped) * 0.6, -0.72)
//   rotateY(45)
//   rotateY(-sin(p*p*PI) * 20), rotateZ(-sin(sqrt(p)*PI) * 20),
//   rotateX(-sin(sqrt(p)*PI) * 80)
//
// Its camera looks down -z and ours looks down +z, so every depth and
// every angle changes sign on the way across and nothing else does.
glm::mat4 HeldItem::pose(const glm::mat4& camera, bool item, bool sneaking,
                         bool blocking) const
{
    const float p = swingProgress();
    const float arc = std::sin(std::sqrt(p) * PI);        // out and back
    const float lift = std::sin(std::sqrt(p) * PI * 2.0f); // up, then down
    const float punch = std::sin(p * PI);                  // forward

    float right = (item ? ITEM_RIGHT : HAND_RIGHT) - arc * (item ? 0.4f : 0.3f);
    float down = (item ? ITEM_DOWN : HAND_DOWN) + (1.0f - m_equip) * EQUIP_DROP
                 - lift * (item ? 0.2f : 0.4f);
    float forward = (item ? ITEM_FORWARD : HAND_FORWARD) + punch * (item ? 0.2f : 0.4f);

    // Sneaking drops the whole hand with the camera, and a raised
    // shield comes up and in across the front of you.
    if (sneaking) down += 0.08f;
    if (blocking) { down -= 0.26f; right -= 0.26f; forward -= 0.18f; }

    const glm::vec3 origin = glm::vec3(camera[3]) +
                             glm::vec3(camera[0]) * right +
                             glm::vec3(camera[1]) * -down +
                             glm::vec3(camera[2]) * forward;

    // The turn. Forty-five degrees about the upright to begin with, so
    // a block is seen corner-on, and then the three the swing adds.
    // Crossing between Minecraft's frame and ours flips the forward
    // axis, which negates a turn about the upright and a turn about
    // the sideways one -- both of them mix z -- and leaves a turn in
    // the plane of the screen alone, because that one mixes x and y
    // and neither of those changed. Getting the last of those three
    // wrong is what held a pickaxe out past the corner of the screen.
    const float yaw = degrees(-45.0f + (item ? std::sin(p * p * PI) * 20.0f
                                             : -arc * 70.0f));
    const float roll = degrees(item ? -arc * 20.0f : -std::sin(p * p * PI) * 20.0f);
    const float pitch = degrees(item ? arc * 80.0f : 0.0f);

    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float cr = std::cos(roll), sr = std::sin(roll);
    const float cp = std::cos(pitch), sp = std::sin(pitch);

    glm::mat3 turn(1.0f);
    {
        const glm::mat3 Ry(cy, 0.0f, -sy,  0.0f, 1.0f, 0.0f,  sy, 0.0f, cy);
        const glm::mat3 Rz(cr, sr, 0.0f,  -sr, cr, 0.0f,  0.0f, 0.0f, 1.0f);
        const glm::mat3 Rx(1.0f, 0.0f, 0.0f,  0.0f, cp, sp,  0.0f, -sp, cp);
        turn = Ry * Rz * Rx;
    }

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

void HeldItem::appendBlock(const glm::mat4& toWorld, StackId stack, float sky, float blockLight)
{
    const float half = BLOCK_SIZE * 0.5f;

    for (int face = 0; face < 6; ++face)
    {
        const TileUV uv = tileUV(cubeTile(stack, face));

        Quad q;
        q.shade = CUBE_SHADE[face];
        for (int k = 0; k < 4; ++k)
        {
            const int* c = CUBE_FACE[face][k];
            q.corner[k] = place(toWorld,
                                (c[0] ? half : -half),
                                (c[1] ? half : -half),
                                (c[2] ? half : -half));
            q.u[k] = (CUBE_UV[face][k][0] == 0.0f) ? uv.u0 : uv.u1;
            q.v[k] = (CUBE_UV[face][k][1] == 0.0f) ? uv.vTop : uv.vBottom;
        }
        pushQuad(m_vertices, q, sky, blockLight, DERIVED_BASIS);
    }
}

unsigned int HeldItem::shieldTexture()
{
    // Looked for once. Without the sheet the shield falls back to its
    // item card, which is what a copy of the game with no art installed
    // has always drawn.
    if (m_shieldLookedFor) return m_shieldTexture;
    m_shieldLookedFor = true;

    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(SHIELD_TEXTURE, &w, &h, &channels, 4);
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

// One box of the shield, unwrapped at Mojang's own offsets: the six
// faces of a box sit in the sheet exactly where BoxMesh says they do,
// so the panel and the mobs read their art the same way.
void HeldItem::appendShieldBox(const glm::mat4& toWorld, int u, int v,
                               float w, float h, float d,
                               float ox, float oy, float oz,
                               float sky, float blockLight)
{
    const float hw = w * 0.5f * SHIELD_SCALE;
    const float hh = h * 0.5f * SHIELD_SCALE;
    const float hd = d * 0.5f * SHIELD_SCALE;

    for (int face = 0; face < 6; ++face)
    {
        int px = 0, py = 0, pw = 0, ph = 0;
        BoxMesh::facePatch(face, u, v, static_cast<int>(w), static_cast<int>(h),
                           static_cast<int>(d), px, py, pw, ph);

        const float u0 = px / SHIELD_SHEET;
        const float u1 = (px + pw) / SHIELD_SHEET;
        const float v0 = py / SHIELD_SHEET;
        const float v1 = (py + ph) / SHIELD_SHEET;

        Quad q;
        q.shade = CUBE_SHADE[face];
        for (int k = 0; k < 4; ++k)
        {
            const int* c = CUBE_FACE[face][k];
            q.corner[k] = place(toWorld,
                                ox + (c[0] ? hw : -hw),
                                oy + (c[1] ? hh : -hh),
                                oz + (c[2] ? hd : -hd));
            q.u[k] = (CUBE_UV[face][k][0] == 0.0f) ? u0 : u1;
            q.v[k] = (CUBE_UV[face][k][1] == 0.0f) ? v0 : v1;
        }

        // No maps, the same as the arm and the item sprites. The shield
        // has a sheet of its own and nothing in the block atlas, so
        // asking the shader for a derived tangent basis set it marching
        // the parallax through the normal atlas at the shield's own
        // texture coordinates -- which dragged the UVs wherever that
        // data happened to lead and smeared the plank face into a
        // mottled smudge with a dark ring in the middle of it.
        pushQuad(m_vertices, q, sky, blockLight, NO_NORMAL_MAP);
    }
}

void HeldItem::appendShield(const glm::mat4& toWorld, float sky, float blockLight)
{
    // Mojang's own two boxes, read off shield_base_nopattern.png: a
    // plate of 12x22x1 at (0,0) and a handle of 2x6x6 at (26,0).
    //
    // Built around the handle rather than around the plate, because the
    // handle is the part the hand holds: put at the fist, the whole
    // thing hangs off it the way a carried shield does. Centring the
    // plate instead left the shield floating in mid-air beside the arm
    // with nothing holding it.
    appendShieldBox(toWorld, 26, 0, 2.0f, 6.0f, 6.0f,
                    0.0f, 0.0f, 0.0f, sky, blockLight);

    // The plate stands off the far side of the grip in the model's own
    // space. The pose below turns the whole thing about, which is what
    // Mojang's rotation of [0,180,_] means, so this ends up nearest the
    // eye with its front towards you -- the studded side. Drawn without
    // that turn you get the plain back of the shield, which is what the
    // first version showed and what made it look wrong against a
    // screenshot of the real thing.
    appendShieldBox(toWorld, 0, 0, 12.0f, 22.0f, 1.0f,
                    0.0f, 0.0f, 3.5f * SHIELD_SCALE, sky, blockLight);
}

// A flat item in the hand. Minecraft does not hang the sprite square
// in front of you: out of its ItemRenderer, after the pose above it
//
//   translate(0, -0.3, 0); scale(1.5); rotateY(50); rotateZ(335);
//   translate(-0.9375, -0.0625, 0)
//
// and then draws the sprite as a unit square a sixteenth of a block
// thick, which is why a pickaxe held in Minecraft has an edge you can
// see when you turn.
void HeldItem::appendSprite(const glm::mat4& toWorld, StackId stack, float sky, float blockLight)
{
    const TileUV uv = tileUV(atlasTileFor(stack));

    const float yaw = degrees(-50.0f);       // our forward axis is the other way
    const float roll = degrees(335.0f);      // but a turn in the screen's plane is not
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float cr = std::cos(roll), sr = std::sin(roll);

    const glm::mat3 Ry(cy, 0.0f, -sy,  0.0f, 1.0f, 0.0f,  sy, 0.0f, cy);
    const glm::mat3 Rz(cr, sr, 0.0f,  -sr, cr, 0.0f,  0.0f, 0.0f, 1.0f);
    const glm::mat3 turn = Ry * Rz;

    const glm::mat3 basis{ glm::vec3(toWorld[0]), glm::vec3(toWorld[1]), glm::vec3(toWorld[2]) };
    const glm::mat3 axes = basis * turn;

    // The 0.3 drop happens before the turn and inside Minecraft's own
    // 0.4 scale, so it is that much of a block and not a third of one;
    // the last translation comes after the turn, in the card's own
    // frame, and is what puts the handle of a tool in the fist rather
    // than its middle.
    const glm::vec3 origin = glm::vec3(toWorld[3]) + glm::vec3(toWorld[1]) * (-0.3f * ITEM_SCALE);

    glm::mat4 card(1.0f);
    card[0] = glm::vec4(axes[0] * SPRITE_SCALE, 0.0f);
    card[1] = glm::vec4(axes[1] * SPRITE_SCALE, 0.0f);
    card[2] = glm::vec4(axes[2] * SPRITE_SCALE, 0.0f);
    card[3] = glm::vec4(origin + axes[0] * (-0.9375f * SPRITE_SCALE) +
                            axes[1] * (-0.0625f * SPRITE_SCALE), 1.0f);

    const float d = SPRITE_DEPTH;

    // The two faces, back to back a sixteenth apart.
    Quad front;
    front.shade = 1.0f;
    front.corner[0] = place(card, 0.0f, 0.0f, 0.0f);
    front.corner[1] = place(card, 1.0f, 0.0f, 0.0f);
    front.corner[2] = place(card, 1.0f, 1.0f, 0.0f);
    front.corner[3] = place(card, 0.0f, 1.0f, 0.0f);
    const float tu[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
    const float tv[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };
    for (int k = 0; k < 4; ++k) { front.u[k] = tu[k]; front.v[k] = tv[k]; }
    pushQuad(m_vertices, front, sky, blockLight);

    Quad back = front;
    back.shade = 0.86f;
    for (int k = 0; k < 4; ++k)
        back.corner[k] = place(card,
                               (k == 1 || k == 2) ? 1.0f : 0.0f,
                               (k >= 2) ? 1.0f : 0.0f,
                               d);
    std::swap(back.corner[1], back.corner[3]);
    std::swap(back.u[1], back.u[3]);
    std::swap(back.v[1], back.v[3]);
    pushQuad(m_vertices, back, sky, blockLight);

    // And the four edges between them, each taking the strip of sprite
    // it runs along, so the slab reads as solid from the side rather
    // than as two cards with daylight between them.
    struct Edge { float x0, y0, x1, y1; float u0, v0, u1, v1; float shade; };
    const Edge edges[4] = {
        { 0.0f, 1.0f, 1.0f, 1.0f, uv.u0, uv.vTop,    uv.u1, uv.vTop,    1.00f },
        { 0.0f, 0.0f, 1.0f, 0.0f, uv.u0, uv.vBottom, uv.u1, uv.vBottom, 0.50f },
        { 0.0f, 0.0f, 0.0f, 1.0f, uv.u0, uv.vBottom, uv.u0, uv.vTop,    0.72f },
        { 1.0f, 0.0f, 1.0f, 1.0f, uv.u1, uv.vBottom, uv.u1, uv.vTop,    0.72f },
    };

    for (const Edge& e : edges)
    {
        Quad q;
        q.shade = e.shade;
        q.corner[0] = place(card, e.x0, e.y0, 0.0f);
        q.corner[1] = place(card, e.x1, e.y1, 0.0f);
        q.corner[2] = place(card, e.x1, e.y1, d);
        q.corner[3] = place(card, e.x0, e.y0, d);
        q.u[0] = e.u0; q.v[0] = e.v0;
        q.u[1] = e.u1; q.v[1] = e.v1;
        q.u[2] = e.u1; q.v[2] = e.v1;
        q.u[3] = e.u0; q.v[3] = e.v0;
        pushQuad(m_vertices, q, sky, blockLight);
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

    if (skin.textureId())
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
        // Mojang's own first-person pose, out of shield.json: well out
        // to the left, well forward of the hand, a quarter again life
        // size, and square to the eye rather than angled -- you look at
        // the face of your own shield, which is how you can see the
        // banner on one. Carried it leans five degrees; raised it
        // straightens up, rises a little and comes closer.
        //
        //   firstperson_righthand  rotation [0,180,5]  translation [-10,2,-10]
        //   shield_blocking        rotation [0,180,0]  translation [-10,2.5,-11]
        // Placed against the camera rather than hung off the hand: a
        // shield is far bigger than the fist holding it, and the two
        // numbers that matter are where its face sits on the screen
        // and how square to you it is. Carried, the near edge is out
        // at the bottom right; raised, it comes across in front of
        // you and most of the view is shield.
        glm::mat4 panel = held;
        const float outward = blocking ? 0.19f : 0.45f;
        const float downward = blocking ? 0.49f : 0.80f;
        const float along = blocking ? 0.61f : 0.82f;
        panel[3] = glm::vec4(camera.position +
                                 camera.right * outward +
                                 camera.up * -downward +
                                 camera.front * along, 1.0f);

        const float roll = blocking ? 0.0f : 0.0873f;   // five degrees
        const float cr = std::cos(roll), sr = std::sin(roll);
        const glm::vec3 right = camera.right * cr + camera.up * sr;
        const glm::vec3 up = -camera.right * sr + camera.up * cr;

        // Turned about its own vertical axis, which negates right and
        // forward. This is the half-turn in Mojang's [0,180,5]: without
        // it you are looking at the back of your own shield.
        panel[0] = glm::vec4(-right, 0.0f);
        panel[1] = glm::vec4(up, 0.0f);
        panel[2] = glm::vec4(-camera.front, 0.0f);

        m_vertices.clear();
        appendShield(panel, sky, blockLight);
        m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shieldTexture());
        m_mesh.draw(GL_TRIANGLES);

        atlas.bind(0, 1, 2);
        return;
    }

    if (drawsFlat(showing)) appendSprite(held, showing, sky, blockLight);
    else                    appendBlock(held, showing, sky, blockLight);

    atlas.bind(0, 1, 2);
    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    m_mesh.draw(GL_TRIANGLES);
}
