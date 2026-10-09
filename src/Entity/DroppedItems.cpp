#include "DroppedItems.h"
#include "World/World.h"
#include "Player/Player.h"
#include "Player/Inventory.h"
#include "Audio/AudioEngine.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Core/GLFunctions.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float GRAVITY = -22.0f;
    constexpr float BOUNCE_DAMPING = 0.35f;
    constexpr float GROUND_FRICTION = 6.0f;
    constexpr float SPIN_SPEED = 1.5f;      // radians per second
    constexpr float BOB_HEIGHT = 0.08f;

    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    // Unit cube faces in the same order and winding as the chunk mesher.
    struct FaceDef
    {
        int corner[4][3];
        float uv[4][2];
        float shade;
    };

    const FaceDef FACES[6] = {
        { { {1,0,0}, {1,1,0}, {1,1,1}, {1,0,1} }, { {0,1}, {0,0}, {1,0}, {1,1} }, 0.72f },
        { { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} }, { {0,1}, {1,1}, {1,0}, {0,0} }, 0.72f },
        { { {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0} }, { {0,0}, {0,1}, {1,1}, {1,0} }, 1.00f },
        { { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} }, { {0,0}, {1,0}, {1,1}, {0,1} }, 0.50f },
        { { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} }, { {0,1}, {1,1}, {1,0}, {0,0} }, 0.86f },
        { { {0,0,0}, {0,1,0}, {1,1,0}, {1,0,0} }, { {0,1}, {0,0}, {1,0}, {1,1} }, 0.86f },
    };

    // A block shows the right face of itself.
    int tileForFace(StackId id, int face)
    {
        const BlockInfo& info = blockInfo(asBlock(id));
        if (face == 2) return info.tileTop;
        if (face == 3) return info.tileBottom;
        return info.tileSide;
    }

    // Anything with one sprite rather than six faces: every item, and the
    // blocks the mesher draws as crossed quads.
    bool isFlatSprite(StackId id)
    {
        return isItem(id) || isCross(asBlock(id));
    }
}

void DroppedItems::spawn(const glm::vec3& position, StackId block, int count)
{
    if (block == Blocks::Air || count <= 0) return;
    if (m_items.size() > 400) return; // safety valve

    Item item;
    item.position = position;
    item.block = block;
    item.count = count;

    // A small random pop so several drops don't stack in one spot.
    const uint32_t h = hashCoords(static_cast<int>(position.x * 16.0f),
                                  static_cast<int>(position.y * 16.0f),
                                  static_cast<int>(position.z * 16.0f) + static_cast<int>(m_items.size()),
                                  12345u);
    item.velocity = glm::vec3((hashToFloat(h) - 0.5f) * 1.8f,
                              2.0f + hashToFloat(h >> 8) * 0.8f,
                              (hashToFloat(h >> 16) - 0.5f) * 1.8f);
    item.spin = hashToFloat(h >> 4) * 6.2831853f;

    m_items.push_back(item);
}

void DroppedItems::spawnFromBrokenBlock(const glm::ivec3& blockPosition, StackId drop)
{
    spawn(glm::vec3(blockPosition) + glm::vec3(0.5f), drop, 1);
}

bool DroppedItems::blocked(const World& world, const glm::vec3& centre) const
{
    const float half = SIZE * 0.5f;
    const int minX = static_cast<int>(std::floor(centre.x - half));
    const int maxX = static_cast<int>(std::floor(centre.x + half));
    const int minY = static_cast<int>(std::floor(centre.y - half));
    const int maxY = static_cast<int>(std::floor(centre.y + half));
    const int minZ = static_cast<int>(std::floor(centre.z - half));
    const int maxZ = static_cast<int>(std::floor(centre.z + half));

    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                if (isSolid(world.getBlock(x, y, z))) return true;

    return false;
}

void DroppedItems::update(float deltaTime, const World& world, const Player& player,
                          Inventory& inventory, AudioEngine& audio)
{
    const glm::vec3 playerCentre = player.position + glm::vec3(0.0f, Player::HEIGHT * 0.5f, 0.0f);

    for (size_t i = 0; i < m_items.size();)
    {
        Item& item = m_items[i];
        item.age += deltaTime;
        item.spin += SPIN_SPEED * deltaTime;

        bool collected = false;

        // Walk onto it to pick it up. The delay is what lets you drop
        // something on purpose without instantly taking it back.
        const float distance = glm::length(playerCentre - item.position);
        if (item.age > PICKUP_DELAY && distance < COLLECT_RADIUS)
        {
            const int leftOver = inventory.add(item.block, item.count);
            if (leftOver < item.count)
            {
                audio.play(Sound::Pickup, 0.45f, 0.95f + 0.1f * (i % 3));
                item.count = leftOver;
                collected = leftOver <= 0;
            }
        }

        if (collected || item.age > LIFETIME_SECONDS)
        {
            m_items[i] = m_items.back();
            m_items.pop_back();
            continue;
        }

        // --- physics ---
        if (!item.resting)
        {
            item.velocity.y += GRAVITY * deltaTime;

            // Resolve one axis at a time, same approach as the player.
            glm::vec3 next = item.position;

            next.x += item.velocity.x * deltaTime;
            if (blocked(world, next)) { next.x = item.position.x; item.velocity.x = 0.0f; }

            next.z += item.velocity.z * deltaTime;
            if (blocked(world, glm::vec3(next.x, item.position.y, next.z)))
            {
                next.z = item.position.z;
                item.velocity.z = 0.0f;
            }

            next.y += item.velocity.y * deltaTime;
            if (blocked(world, next))
            {
                next.y = item.position.y;
                if (item.velocity.y < 0.0f)
                {
                    // Settle rather than jitter once the bounce is tiny.
                    if (item.velocity.y > -1.2f) item.resting = true;
                    item.velocity.y *= -BOUNCE_DAMPING;
                }
                else
                {
                    item.velocity.y = 0.0f;
                }
            }

            item.position = next;

            const float friction = std::clamp(GROUND_FRICTION * deltaTime, 0.0f, 1.0f);
            item.velocity.x -= item.velocity.x * friction;
            item.velocity.z -= item.velocity.z * friction;
        }

        ++i;
    }

    mergeNearby();
}

// Two piles of the same thing within arm's length become one, which is
// the only way a settled drop ever moves. The older one wins the spot.
void DroppedItems::mergeNearby()
{
    for (size_t a = 0; a < m_items.size(); ++a)
    {
        for (size_t b = a + 1; b < m_items.size();)
        {
            Item& keep = m_items[a];
            Item& other = m_items[b];

            const int limit = maxStackOf(keep.block);
            if (other.block != keep.block || keep.count >= limit ||
                glm::length(other.position - keep.position) > MERGE_RADIUS)
            {
                ++b;
                continue;
            }

            const int moved = std::min(other.count, limit - keep.count);
            keep.count += moved;
            other.count -= moved;
            keep.age = std::max(keep.age, other.age);

            if (other.count > 0) { ++b; continue; }

            m_items[b] = m_items.back();
            m_items.pop_back();
        }
    }
}

void DroppedItems::render(Shader& chunkShader, const World& world)
{
    if (m_items.empty()) return;

    m_vertices.clear();
    m_vertices.reserve(m_items.size() * 36 * 9);


    const float half = SIZE * 0.5f;

    for (const Item& item : m_items)
    {

        // Float clear of the ground, then bob and spin. Resting the cube
        // exactly on the surface left it half sunk into the block below.
        const float bob = std::sin(item.age * 2.4f) * BOB_HEIGHT;
        const glm::vec3 centre = item.position + glm::vec3(0.0f, HOVER + bob, 0.0f);

        const float s = std::sin(item.spin);
        const float c = std::cos(item.spin);

        // Sample light from open air. Reading it at the item's own centre
        // picked up the solid block it was embedded in, which rendered the
        // item almost black.
        int lx = static_cast<int>(std::floor(centre.x));
        int ly = static_cast<int>(std::floor(centre.y));
        int lz = static_cast<int>(std::floor(centre.z));
        for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step)
            ++ly;

        const float sky = world.skyLight(lx, ly, lz) / 15.0f;
        const float blockLight = std::max<float>(world.blockLightAt(lx, ly, lz),
                                                 isItem(item.block) ? 0 : lightEmission(asBlock(item.block)))
                                       / 15.0f;

        auto push = [&](const glm::vec3& p, float tu, float tv, float shade) {
            m_vertices.push_back(p.x);
            m_vertices.push_back(p.y);
            m_vertices.push_back(p.z);
            m_vertices.push_back(tu);
            m_vertices.push_back(tv);
            m_vertices.push_back(shade);
            m_vertices.push_back(sky);
            m_vertices.push_back(blockLight);
            m_vertices.push_back(6.0f); // face 6: skip tangent-space normal mapping
        };

        if (isFlatSprite(item.block))
        {
            // Stood up on its bottom edge and turned on the spot, so it
            // reads as the thing it is from any angle. Drawn front and
            // back, because a single quad disappears edge-on and again
            // whenever the spin brings its back face round.
            const TileUV uv = tileUV(atlasTileFor(item.block));
            const float half = SPRITE_SIZE * 0.5f;
            const glm::vec3 foot = item.position + glm::vec3(0.0f, HOVER * 0.5f + bob, 0.0f);
            const glm::vec3 across(half * c, 0.0f, half * s);
            const glm::vec3 up(0.0f, SPRITE_SIZE, 0.0f);

            const glm::vec3 corner[4] = {
                foot - across, foot + across, foot + across + up, foot - across + up
            };
            const float tu[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
            const float tv[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };

            const int front[6] = { 0, 1, 2, 0, 2, 3 };
            const int back[6]  = { 0, 2, 1, 0, 3, 2 };
            for (int k : front) push(corner[k], tu[k], tv[k], 1.0f);
            for (int k : back)  push(corner[k], tu[k], tv[k], 0.86f);

            continue;
        }

        for (int f = 0; f < 6; ++f)
        {
            const FaceDef& face = FACES[f];
            const TileUV uv = tileUV(tileForFace(item.block, f));

            glm::vec3 corners[4];
            float u[4], v[4];
            for (int k = 0; k < 4; ++k)
            {
                // Corner in local space, centred on the origin.
                const glm::vec3 local(
                    (face.corner[k][0] - 0.5f) * SIZE,
                    (face.corner[k][1] - 0.5f) * SIZE,
                    (face.corner[k][2] - 0.5f) * SIZE);

                corners[k] = centre + glm::vec3(local.x * c - local.z * s,
                                                local.y,
                                                local.x * s + local.z * c);
                u[k] = (face.uv[k][0] == 0.0f) ? uv.u0 : uv.u1;
                v[k] = (face.uv[k][1] == 0.0f) ? uv.vTop : uv.vBottom;
            }

            const int order[6] = { 0, 1, 2, 0, 2, 3 };
            for (int k : order) push(corners[k], u[k], v[k], face.shade);
        }
    }

    (void)half;

    m_mesh.upload(m_vertices, CHUNK_LAYOUT, true);

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f)); // vertices are already in world space
    m_mesh.draw(GL_TRIANGLES);
}
