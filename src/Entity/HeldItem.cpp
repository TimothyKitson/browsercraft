#include "HeldItem.h"
#include "BoxMesh.h"
#include "PlayerSkin.h"
#include "Renderer/Atlas.h"
#include "Renderer/Camera.h"
#include "Renderer/Shader.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    // Where the hand sits in front of the camera, in blocks: out to the
    // right, down out of the way of the crosshair, and far enough forward
    // to clear the near plane.
    // Far enough forward that perspective shrinks it to about a quarter
    // of the screen, which is roughly where Minecraft puts it.
    // Where the hand sits, as a fraction of the screen. The camera is
    // 70 degrees vertical, so at HAND_FORWARD the visible half-height is
    // tan(35) = 0.700 and the half-width is that times the aspect. Those
    // two numbers are what turn "about four fifths across and two
    // thirds down", measured off a Java screenshot, into the offsets
    // below -- rather than nudging them and re-rendering.
    constexpr float HAND_RIGHT = 0.78f;
    constexpr float HAND_DOWN = 0.50f;
    constexpr float HAND_FORWARD = 1.00f;

    // The arm's own origin is its fist, so the offsets above place the
    // fist directly and there is nothing further to nudge. Anything
    // held hangs off this, not off the camera, or it floats in the
    // middle of the screen on its own.
    constexpr float HAND_LOCAL_X = 0.0f;
    constexpr float HAND_LOCAL_Y = 0.0f;
    constexpr float HAND_LOCAL_Z = 0.0f;

    // Minecraft's first-person arm is a slim thing tucked into the
    // bottom corner -- about a tenth of the screen across -- long
    // enough that the shoulder runs off the bottom edge, so only the
    // fist end is ever in shot.
    constexpr float ARM_HALF_WIDTH = 0.080f;
    constexpr float ARM_HALF_DEPTH = 0.080f;

    // An arm's texture patch is four pixels across and twelve tall, so
    // the box has to be three times as long as it is wide or the texels
    // come out taller than they are wide and the whole thing reads as
    // stretched. At 0.46 they were eighteen per cent off.
    constexpr float ARM_TEXEL_RATIO = 3.0f;   // 12 tall / 4 wide
    constexpr float ARM_LENGTH = ARM_HALF_WIDTH * 2.0f * ARM_TEXEL_RATIO;
    // Radians, leaning the arm so it rises from the bottom-right corner
    // towards the middle of the screen. The sign matters: the other way
    // round puts the shoulder in the middle and the hand out at the
    // corner, which is the arm on backwards.
    constexpr float ARM_LEAN = 0.62f;

    // How far past edge-on to carry the cut end, in radians. Exactly
    // edge-on projects to a zero-width sliver that can still catch a
    // pixel; a little past it is safely behind.
    constexpr float ARM_CAP_MARGIN = 0.16f;

    // Scaled with the arm: a block held in a fist this size looked like
    // a pebble at the old figures.
    constexpr float BLOCK_SIZE = 0.23f;
    constexpr float SPRITE_SIZE = 0.30f;

    // Face 6 tells the chunk shader to skip tangent-space normal mapping,
    // which neither a skin nor an item sprite has maps for.
    constexpr float NO_NORMAL_MAP = 6.0f;

    struct Quad
    {
        glm::vec3 corner[4];
        float u[4], v[4];
        float shade;
    };

    void pushQuad(std::vector<float>& out, const Quad& q, float sky, float blockLight)
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
            out.push_back(NO_NORMAL_MAP);
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

void HeldItem::update(float deltaTime)
{
    if (m_swing > 0.0f) m_swing = std::max(0.0f, m_swing - deltaTime);
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
    const float cl = std::cos(ARM_LEAN), sl = std::sin(ARM_LEAN);

    const float tilt = std::atan2(cl * -HAND_DOWN - sl * HAND_RIGHT, HAND_FORWARD)
                       - ARM_CAP_MARGIN;
    const float ct = std::cos(tilt), st = std::sin(tilt);

    glm::mat4 lean(1.0f);
    lean[0] = glm::vec4(cl, sl, 0.0f, 0.0f);
    lean[1] = glm::vec4(-sl * ct, cl * ct, -st, 0.0f);
    lean[2] = glm::vec4(-sl * st, cl * st, ct, 0.0f);
    lean[3] = glm::vec4(HAND_LOCAL_X, HAND_LOCAL_Y, HAND_LOCAL_Z, 1.0f);
    const glm::mat4 arm = toWorld * lean;

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
        pushQuad(m_vertices, q, sky, blockLight);
    }
}

void HeldItem::appendSprite(const glm::mat4& toWorld, StackId stack, float sky, float blockLight)
{
    const TileUV uv = tileUV(atlasTileFor(stack));
    const float half = SPRITE_SIZE * 0.5f;

    Quad front;
    front.shade = 1.0f;
    front.corner[0] = place(toWorld, -half, -half, 0.0f);
    front.corner[1] = place(toWorld, half, -half, 0.0f);
    front.corner[2] = place(toWorld, half, half, 0.0f);
    front.corner[3] = place(toWorld, -half, half, 0.0f);
    const float tu[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
    const float tv[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };
    for (int k = 0; k < 4; ++k) { front.u[k] = tu[k]; front.v[k] = tv[k]; }
    pushQuad(m_vertices, front, sky, blockLight);

    Quad back = front;
    back.shade = 0.86f;
    std::swap(back.corner[1], back.corner[3]);
    std::swap(back.u[1], back.u[3]);
    std::swap(back.v[1], back.v[3]);
    pushQuad(m_vertices, back, sky, blockLight);
}

void HeldItem::render(Shader& chunkShader, const Camera& camera, const PlayerSkin& skin,
                      const Atlas& atlas, StackId stack, float sky, float blockLight,
                      bool sneaking)
{
    const float swing = swingAngle();

    // The hand drops and swings back as you strike, and sits lower when
    // you are sneaking, the same as the camera does.
    const float down = HAND_DOWN + swing * 0.22f + (sneaking ? 0.08f : 0.0f);
    const float forward = HAND_FORWARD - swing * 0.14f;
    const float across = HAND_RIGHT - swing * 0.05f;

    const glm::vec3 origin = camera.position + camera.front * forward +
                             camera.right * across - camera.up * down;

    // Camera axes as a basis, so everything below is written in the
    // obvious local coordinates: +x right, +y up, +z forwards.
    glm::mat4 toWorld(1.0f);
    toWorld[0] = glm::vec4(camera.right, 0.0f);
    toWorld[1] = glm::vec4(camera.up, 0.0f);
    toWorld[2] = glm::vec4(camera.front, 0.0f);
    toWorld[3] = glm::vec4(origin, 1.0f);

    const bool empty = stack == Blocks::Air;

    // The hand is skin-textured and anything it holds comes off the atlas,
    // so they are two passes. Depth is cleared once, before either.
    glClear(GL_DEPTH_BUFFER_BIT);
    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f));

    if (skin.textureId())
    {
        m_vertices.clear();
        appendArm(toWorld, skin, sky, blockLight);
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

    // Held things sit at the end of the arm, turned a little so a block
    // is seen corner-on rather than as a flat face.
    glm::mat4 held = toWorld;
    // Just beyond the fist, and a little above it, so the block sits in
    // the hand rather than inside it.
    held[3] = glm::vec4(place(toWorld, HAND_LOCAL_X, HAND_LOCAL_Y + 0.05f,
                              HAND_LOCAL_Z + 0.02f), 1.0f);

    if (drawsFlat(stack))
    {
        appendSprite(held, stack, sky, blockLight);
    }
    else
    {
        // Turned so the two visible sides come out the same width, which
        // is what makes a block read as corner-on rather than as a flat
        // square facing you. The angle is measured against the line from
        // the eye to the block, not against the camera's forward axis:
        // the hand sits nearly forty degrees off to the side, far enough
        // out that the two differ, and turning it the obvious
        // forty-five degrees from forward lands it face-on instead.
        const float a = std::atan2(forward + HAND_LOCAL_Z + 0.02f,
                                   across + HAND_LOCAL_X) - 0.7853982f;
        const glm::vec3 r(camera.right * std::cos(a) + camera.front * std::sin(a));
        const glm::vec3 f(-camera.right * std::sin(a) + camera.front * std::cos(a));
        held[0] = glm::vec4(r, 0.0f);
        held[2] = glm::vec4(f, 0.0f);
        appendBlock(held, stack, sky, blockLight);
    }

    atlas.bind(0, 1, 2);
    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);
    m_mesh.draw(GL_TRIANGLES);
}
