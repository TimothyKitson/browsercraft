#include "ChunkMesher.h"
#include "Renderer/Atlas.h"
#include "Renderer/AtlasTiles.h"
#include <array>
#include <cmath>

namespace
{
    // Face order: +X, -X, +Y, -Y, +Z, -Z.
    // Corners are listed counter-clockwise seen from outside the block, so
    // back-face culling keeps the outward side.
    struct FaceDef
    {
        int nx, ny, nz;          // outward normal
        int corner[4][3];        // 4 corners, each 0/1 per axis
        float uv[4][2];          // matching uv weights (0 = u0/vTop edge, 1 = u1/vBottom edge)
        float shade;             // fixed directional shading
    };

    const std::array<FaceDef, 6> FACES = { {
        // +X
        { 1, 0, 0,
          { {1,0,0}, {1,1,0}, {1,1,1}, {1,0,1} },
          { {0,1}, {0,0}, {1,0}, {1,1} },
          0.72f },
        // -X
        { -1, 0, 0,
          { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} },
          { {0,1}, {1,1}, {1,0}, {0,0} },
          0.72f },
        // +Y (top)
        { 0, 1, 0,
          { {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0} },
          { {0,0}, {0,1}, {1,1}, {1,0} },
          1.0f },
        // -Y (bottom)
        { 0, -1, 0,
          { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} },
          { {0,0}, {1,0}, {1,1}, {0,1} },
          0.5f },
        // +Z
        { 0, 0, 1,
          { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} },
          { {0,1}, {1,1}, {1,0}, {0,0} },
          0.86f },
        // -Z
        { 0, 0, -1,
          { {0,0,0}, {0,1,0}, {1,1,0}, {1,0,0} },
          { {0,1}, {0,0}, {1,0}, {1,1} },
          0.86f },
    } };

    bool usesTransparentPass(BlockId id)
    {
        return isLiquid(id) || id == Blocks::Glass || id == Blocks::Ice;
    }

    // Should the face between `self` and `neighbour` be drawn?
    bool faceVisible(BlockId self, BlockId neighbour)
    {
        if (neighbour == Blocks::Air) return true;
        if (isOpaque(neighbour)) return false;

        // Two slabs side by side each show the face between them: one is
        // not hidden by the other the way two full blocks would be, and
        // hiding it would leave a hole through a slab floor.
        if (isSlab(self) || isSlab(neighbour)) return true;

        // Leaves are the exception to hiding the inside of a volume. A
        // canopy meshed as a hollow shell is one layer of cut-out tiles
        // with nothing behind it, so you see through every gap in it to
        // the trunk. Drawing every leaf face means the layers behind
        // cover each other's holes and the tree reads as solid.
        if (isLeaves(self) && isLeaves(neighbour)) return true;

        if (neighbour == self) return false;  // hide interior faces of water and glass

        // The sea does not draw a face against what grows in it. Kelp
        // and seagrass stand in the water rather than beside it, and
        // without this every blade gets a glassy box round it where
        // the water surfaces meet it.
        if (isLiquid(self) && isWaterPlant(neighbour)) return false;

        if (isCross(neighbour)) return true;
        return true;
    }

    float aoFactor(int occluders)
    {
        // 0 = fully open corner, 3 = tightly enclosed.
        static const float LEVELS[4] = { 1.0f, 0.78f, 0.58f, 0.42f };
        return LEVELS[occluders < 0 ? 0 : (occluders > 3 ? 3 : occluders)];
    }

    // The last attribute carries which way the face looks and what it
    // is a face of: face + 8 * block id, so the shaders can treat water,
    // leaves and lava differently and light a face by its real normal.
    // Six is a plant's crossed quads, which have no single normal.
    constexpr int CROSS_FACE = 6;
    float tagOf(int face, BlockId id, bool top = false)
    {
        return static_cast<float>(face + 8 * static_cast<int>(id) + (top ? 32768 : 0));
    }

    void pushVertex(std::vector<float>& out,
                    float x, float y, float z,
                    float u, float v,
                    float shade, float sky, float block, float tag)
    {
        out.push_back(x);
        out.push_back(y);
        out.push_back(z);
        out.push_back(u);
        out.push_back(v);
        out.push_back(shade);
        out.push_back(sky);
        out.push_back(block);
        out.push_back(tag);
    }

    int tileForFace(const BlockInfo& info, int faceIndex)
    {
        if (faceIndex == 2) return info.tileTop;
        if (faceIndex == 3) return info.tileBottom;
        return info.tileSide;
    }

    // The same, for a block that points: the one side it points at
    // gets its own texture. Which side that is comes off the face's
    // own normal rather than a table, so it cannot drift out of step
    // with the mesher's face order.
    int tileForFace(const BlockInfo& info, int faceIndex, BlockId id)
    {
        if (info.tileFront < 0) return tileForFace(info, faceIndex);
        if (faceIndex == 2 || faceIndex == 3) return tileForFace(info, faceIndex);

        const FaceDef& face = FACES[faceIndex];
        const int facing = (face.nz < 0) ? 0 : (face.nx > 0) ? 1 : (face.nz > 0) ? 2 : 3;
        if (facing == blockFacing(id)) return info.tileFront;
        if (info.tileBack >= 0 && facing == ((blockFacing(id) + 2) & 3)) return info.tileBack;
        return info.tileSide;
    }

    float fluidHeightAt(const MeshSnapshot& snap, int x, int y, int z, BlockId base)
    {
        const BlockId here = snap.block(x, y, z);
        if (fluidBase(here) == base)
            return fluidBase(snap.block(x, y + 1, z)) == base ? 1.0f : fluidAmount(here) / 9.0f;
        return blockInfo(here).collides ? -1.0f : 0.0f;
    }

    float fluidCorner(const MeshSnapshot& snap, int x, int y, int z, BlockId base,
                      float origin, int sx, int sz)
    {
        const float northSouth = fluidHeightAt(snap, x, y, z + sz, base);
        const float eastWest = fluidHeightAt(snap, x + sx, y, z, base);
        if (northSouth >= 1.0f || eastWest >= 1.0f) return 1.0f;

        float sum = 0.0f;
        float weight = 0.0f;
        auto add = [&](float h) {
            if (h >= 0.8f) { sum += h * 10.0f; weight += 10.0f; }
            else if (h >= 0.0f) { sum += h; weight += 1.0f; }
        };
        if (northSouth > 0.0f || eastWest > 0.0f)
        {
            const float diagonal = fluidHeightAt(snap, x + sx, y, z + sz, base);
            if (diagonal >= 1.0f) return 1.0f;
            add(diagonal);
        }
        add(origin);
        add(northSouth);
        add(eastWest);
        return weight > 0.0f ? sum / weight : origin;
    }

    glm::vec2 fluidFlow(const MeshSnapshot& snap, int x, int y, int z, BlockId id)
    {
        const BlockId base = fluidBase(id);
        const float self = fluidAmount(id) / 9.0f;
        static const int DIRS[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        glm::vec2 flow(0.0f);
        for (const auto& d : DIRS)
        {
            const BlockId next = snap.block(x + d[0], y, z + d[1]);
            if (isFluid(next) && fluidBase(next) != base) continue;
            const float height = fluidBase(next) == base ? fluidAmount(next) / 9.0f : 0.0f;
            float push = 0.0f;
            if (height == 0.0f)
            {
                if (blockInfo(next).collides) continue;
                const BlockId below = snap.block(x + d[0], y - 1, z + d[1]);
                if (!isFluid(below) || fluidBase(below) == base)
                {
                    const float under = fluidBase(below) == base ? fluidAmount(below) / 9.0f : 0.0f;
                    if (under > 0.0f) push = self - (under - 8.0f / 9.0f);
                }
            }
            else
            {
                push = self - height;
            }
            flow += glm::vec2(static_cast<float>(d[0]), static_cast<float>(d[1])) * push;
        }
        return flow;
    }

    void emitFluid(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const BlockId base = fluidBase(id);
        const BlockInfo& still = blockInfo(base);
        const bool water = base == Blocks::Water;
        const bool covered = fluidBase(snap.block(lx, y + 1, lz)) == base;

        const float origin = fluidHeightAt(snap, lx, y, lz, base);
        float corner[2][2];
        for (int cx = 0; cx < 2; ++cx)
            for (int cz = 0; cz < 2; ++cz)
                corner[cx][cz] = covered ? 1.0f
                                         : fluidCorner(snap, lx, y, lz, base, origin, cx ? 1 : -1, cz ? 1 : -1);

        auto lightOf = [&](int x, int yy, int z, float& sky, float& block) {
            sky = static_cast<float>(snap.skyLight(x, yy, z)) / 15.0f;
            block = static_cast<float>(snap.blockLight(x, yy, z)) / 15.0f;
            const float selfSky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
            const float selfBlock = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;
            sky = std::max(sky, selfSky);
            block = std::max(block, selfBlock);
        };

        auto emitQuad = [&](int f, const float px[4], const float py[4], const float pz[4],
                            const float pu[4], const float pv[4], float sky, float block) {
            const float shade = FACES[f].shade;
            const int order[6] = { 0, 1, 2, 0, 2, 3 };
            for (int k = 0; k < 6; ++k)
            {
                const int c = order[k];
                pushVertex(out, px[c], py[c], pz[c], pu[c], pv[c], shade, sky, block,
                           tagOf(f, id, py[c] > static_cast<float>(y) + 0.01f));
            }
        };

        if (!covered)
        {
            const glm::vec2 flow = isFluidSource(id) ? glm::vec2(0.0f) : fluidFlow(snap, lx, y, lz, id);
            const bool moving = std::abs(flow.x) > 1e-4f || std::abs(flow.y) > 1e-4f;
            const TileUV uv = tileUV(moving ? still.tileSide : still.tileTop);
            int along = 2;
            if (moving)
            {
                if (std::abs(flow.x) > std::abs(flow.y)) along = flow.x > 0.0f ? 1 : 3;
                else along = flow.y > 0.0f ? 2 : 0;
            }

            float px[4], py[4], pz[4], pu[4], pv[4];
            const FaceDef& top = FACES[2];
            for (int c = 0; c < 4; ++c)
            {
                const int cx = top.corner[c][0];
                const int cz = top.corner[c][2];
                px[c] = static_cast<float>(lx + cx);
                pz[c] = static_cast<float>(lz + cz);
                py[c] = static_cast<float>(y) + corner[cx][cz];
                float u = static_cast<float>(cx);
                float v = static_cast<float>(cz);
                switch (along)
                {
                    case 0: u = 1.0f - cx; v = 1.0f - cz; break;
                    case 1: u = 1.0f - cz; v = static_cast<float>(cx); break;
                    case 3: u = static_cast<float>(cz); v = 1.0f - cx; break;
                    default: break;
                }
                pu[c] = uv.u0 + (uv.u1 - uv.u0) * u;
                pv[c] = uv.vTop + (uv.vBottom - uv.vTop) * v;
            }
            float sky = 0.0f, block = 0.0f;
            lightOf(lx, y + 1, lz, sky, block);
            emitQuad(2, px, py, pz, pu, pv, sky, block);
        }

        for (int f = 0; f < 6; ++f)
        {
            if (f == 2) continue;
            const FaceDef& face = FACES[f];
            const int nx = lx + face.nx;
            const int ny = y + face.ny;
            const int nz = lz + face.nz;
            const BlockId next = snap.block(nx, ny, nz);
            if (fluidBase(next) == base) continue;
            if (isOpaque(next)) continue;
            if (water && isWaterPlant(next)) continue;

            int tile = still.tileSide;
            if (f == 3) tile = still.tileBottom;
            else if (water && (isLeaves(next) || next == Blocks::Glass || next == Blocks::Ice ||
                               next == Blocks::SlimeBlock))
                tile = Tiles::WaterOverlay;
            const TileUV uv = tileUV(tile);

            float px[4], py[4], pz[4], pu[4], pv[4];
            for (int c = 0; c < 4; ++c)
            {
                const int cx = face.corner[c][0];
                const int cy = face.corner[c][1];
                const int cz = face.corner[c][2];
                px[c] = static_cast<float>(lx + cx);
                pz[c] = static_cast<float>(lz + cz);
                const float h = (f == 3) ? 0.0f : (cy ? corner[cx][cz] : 0.0f);
                py[c] = static_cast<float>(y) + ((f == 3) ? 0.0f : h);
                pu[c] = (face.uv[c][0] == 0.0f) ? uv.u0 : uv.u1;
                if (f == 3)
                    pv[c] = (face.uv[c][1] == 0.0f) ? uv.vTop : uv.vBottom;
                else
                    pv[c] = cy ? uv.vBottom + (uv.vTop - uv.vBottom) * h : uv.vBottom;
            }
            float sky = 0.0f, block = 0.0f;
            lightOf(nx, ny, nz, sky, block);
            emitQuad(f, px, py, pz, pu, pv, sky, block);
        }
    }

    void emitCubeFaces(const MeshSnapshot& snap, int lx, int y, int lz,
                       BlockId id, std::vector<float>& out)
    {
        const BlockInfo& info = blockInfo(id);
        const bool liquid = (info.render == RenderType::Liquid);

        // Liquids sit slightly below a full block unless covered by more
        // liquid; a slab stands half as tall as its cell.
        float topY = blockHeight(id);
        if (liquid && !isLiquid(snap.block(lx, y + 1, lz)))
            topY = 0.875f;

        for (int f = 0; f < 6; ++f)
        {
            const FaceDef& face = FACES[f];
            const int nx = lx + face.nx;
            const int ny = y + face.ny;
            const int nz = lz + face.nz;

            if (!faceVisible(id, snap.block(nx, ny, nz))) continue;

            const TileUV uv = tileUV(tileForFace(info, f, id));
            const float baseSky = static_cast<float>(snap.skyLight(nx, ny, nz)) / 15.0f;
            const float baseBlock = static_cast<float>(snap.blockLight(nx, ny, nz)) / 15.0f;

            float vx[4], vy[4], vz[4], vu[4], vv[4], vshade[4], vsky[4], vblock[4];

            for (int c = 0; c < 4; ++c)
            {
                const int cx = face.corner[c][0];
                const int cy = face.corner[c][1];
                const int cz = face.corner[c][2];

                // Ambient occlusion + smooth light: look at the three cells
                // touching this corner on the outside of the face.
                int occluders = 0;
                float skySum = baseSky * 15.0f;
                float blockSum = baseBlock * 15.0f;
                int samples = 1;

                // Directions along the two axes that aren't the face normal.
                int dx[3] = { 0, 0, 0 };
                int dy[3] = { 0, 0, 0 };
                int dz[3] = { 0, 0, 0 };
                int count = 0;

                if (face.nx == 0) { dx[count] = (cx == 1) ? 1 : -1; ++count; }
                if (face.ny == 0) { dy[count] = (cy == 1) ? 1 : -1; ++count; }
                if (face.nz == 0) { dz[count] = (cz == 1) ? 1 : -1; ++count; }

                // Two side neighbours and the diagonal corner.
                const int offsets[3][3] = {
                    { dx[0], dy[0], dz[0] },
                    { dx[1], dy[1], dz[1] },
                    { dx[0] + dx[1], dy[0] + dy[1], dz[0] + dz[1] }
                };

                bool side1Opaque = false;
                bool side2Opaque = false;
                for (int s = 0; s < 3; ++s)
                {
                    const int sx = nx + offsets[s][0];
                    const int sy = ny + offsets[s][1];
                    const int sz = nz + offsets[s][2];
                    const BlockId neighbour = snap.block(sx, sy, sz);

                    if (isOpaque(neighbour))
                    {
                        if (s == 0) side1Opaque = true;
                        else if (s == 1) side2Opaque = true;
                        else if (!(side1Opaque && side2Opaque)) ++occluders;
                        if (s < 2) ++occluders;
                    }
                    else
                    {
                        skySum += snap.skyLight(sx, sy, sz);
                        blockSum += snap.blockLight(sx, sy, sz);
                        ++samples;
                    }
                }

                // Two touching sides fully enclose the corner.
                if (side1Opaque && side2Opaque) occluders = 3;

                vx[c] = static_cast<float>(lx + cx);
                vy[c] = static_cast<float>(y) + ((cy == 1) ? topY : 0.0f);
                vz[c] = static_cast<float>(lz + cz);
                vu[c] = (face.uv[c][0] == 0.0f) ? uv.u0 : uv.u1;
                vv[c] = (face.uv[c][1] == 0.0f) ? uv.vTop : uv.vBottom;

                // A side of something shorter than its cell shows only
                // the lower part of its texture. Stretching the whole
                // tile over half the height is what would make a slab
                // look like a squashed block rather than a cut one.
                if (face.ny == 0 && topY < 1.0f && face.uv[c][1] == 0.0f)
                    vv[c] = uv.vBottom + (uv.vTop - uv.vBottom) * topY;
                vshade[c] = face.shade * aoFactor(occluders);
                vsky[c] = (skySum / samples) / 15.0f;
                vblock[c] = (blockSum / samples) / 15.0f;
            }

            // Split the quad along the darker diagonal to avoid the classic
            // AO "flipped corner" artefact.
            const bool flip = (vshade[0] + vshade[2]) < (vshade[1] + vshade[3]);
            const int order[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };
            const int orderFlipped[2][3] = { { 1, 2, 3 }, { 1, 3, 0 } };

            for (int tri = 0; tri < 2; ++tri)
            {
                for (int k = 0; k < 3; ++k)
                {
                    const int c = flip ? orderFlipped[tri][k] : order[tri][k];
                    pushVertex(out, vx[c], vy[c], vz[c], vu[c], vv[c], vshade[c], vsky[c], vblock[c], tagOf(f, id, face.corner[c][1] == 1));
                }
            }
        }
    }

    // One axis-aligned box inside a cell, with its own tiles and flat
    // lighting. Stairs are two of these. It is deliberately separate
    // from emitCubeFaces: that one carries the ambient-occlusion work
    // for every ordinary block in the world, and a partial box has no
    // business reaching into it.
    void emitBox(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id,
                 const glm::vec3& lo, const glm::vec3& hi, std::vector<float>& out)
    {
        const BlockInfo& info = blockInfo(id);

        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;

        const glm::vec3 base(static_cast<float>(lx), static_cast<float>(y),
                             static_cast<float>(lz));

        for (int f = 0; f < 6; ++f)
        {
            const FaceDef& face = FACES[f];

            // A face flush with the cell wall is hidden by a solid
            // neighbour the same way a whole block's would be; a face
            // inside the cell is always drawn.
            const bool flush =
                (face.nx < 0 && lo.x <= 0.0f) || (face.nx > 0 && hi.x >= 1.0f) ||
                (face.ny < 0 && lo.y <= 0.0f) || (face.ny > 0 && hi.y >= 1.0f) ||
                (face.nz < 0 && lo.z <= 0.0f) || (face.nz > 0 && hi.z >= 1.0f);

            if (flush && isOpaque(snap.block(lx + face.nx, y + face.ny, lz + face.nz)))
                continue;

            const TileUV uv = tileUV(tileForFace(info, f));

            float vx[4], vy[4], vz[4], vu[4], vv[4];
            for (int c = 0; c < 4; ++c)
            {
                const int cx = face.corner[c][0];
                const int cy = face.corner[c][1];
                const int cz = face.corner[c][2];

                vx[c] = base.x + (cx ? hi.x : lo.x);
                vy[c] = base.y + (cy ? hi.y : lo.y);
                vz[c] = base.z + (cz ? hi.z : lo.z);

                // The texture is cut from the part of the tile the box
                // actually covers, so a half-height side shows half the
                // texture rather than the whole of it squashed. Which
                // two axes those are depends on which way the face
                // looks.
                float su = 0.0f, sv = 0.0f;
                if (face.nx != 0)
                {
                    su = cz ? hi.z : lo.z;
                    sv = cy ? hi.y : lo.y;
                }
                else if (face.ny != 0)
                {
                    su = cx ? hi.x : lo.x;
                    sv = cz ? hi.z : lo.z;
                }
                else
                {
                    su = cx ? hi.x : lo.x;
                    sv = cy ? hi.y : lo.y;
                }

                vu[c] = uv.u0 + (uv.u1 - uv.u0) * su;
                vv[c] = uv.vBottom + (uv.vTop - uv.vBottom) * sv;
            }

            const int order[6] = { 0, 1, 2, 0, 2, 3 };
            for (int k : order)
                pushVertex(out, vx[k], vy[k], vz[k], vu[k], vv[k], face.shade, sky, block, tagOf(f, id, face.corner[k][1] == 1));
        }
    }

    // Stairs: the bottom half of the cell, plus a step on the back half
    // of the top. "Facing" is the low side -- the way you walk up.
    void emitStairs(const MeshSnapshot& snap, int lx, int y, int lz,
                    BlockId id, std::vector<float>& out)
    {
        emitBox(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f, 0.5f, 1.0f), out);

        glm::vec3 lo(0.0f, 0.5f, 0.0f);
        glm::vec3 hi(1.0f, 1.0f, 1.0f);

        switch (blockFacing(id))
        {
            case 0: lo.z = 0.5f; break;   // faces north, step at the south end
            case 1: hi.x = 0.5f; break;   // faces east, step at the west end
            case 2: hi.z = 0.5f; break;   // faces south, step at the north end
            default: lo.x = 0.5f; break;  // faces west, step at the east end
        }

        emitBox(snap, lx, y, lz, id, lo, hi, out);
    }

    // A door: a thin slab on one edge of its cell. Shut, it lies across
    // the way you came; open, it has swung a quarter turn to the side.
    void emitDoor(const MeshSnapshot& snap, int lx, int y, int lz,
                  BlockId id, std::vector<float>& out)
    {
        constexpr float THICK = 0.1875f;

        // Shut, the panel sits on the face it was hung from. Open, it
        // sits on the face a quarter turn clockwise from it.
        const int facing = blockFacing(id);
        const int resting = doorIsOpen(id) ? ((facing + 1) & 3) : facing;

        glm::vec3 lo(0.0f), hi(1.0f);
        switch (resting)
        {
            case 0: hi.z = THICK; break;            // against the north edge
            case 1: lo.x = 1.0f - THICK; break;     // against the east edge
            case 2: lo.z = 1.0f - THICK; break;     // against the south edge
            default: hi.x = THICK; break;           // against the west edge
        }

        emitBox(snap, lx, y, lz, id, lo, hi, out);
    }

    void emitTrapdoor(const MeshSnapshot& snap, int lx, int y, int lz,
                      BlockId id, std::vector<float>& out)
    {
        constexpr float THICK = 0.1875f;
        glm::vec3 lo(0.0f), hi(1.0f);
        if (!trapdoorIsOpen(id))
        {
            if (trapdoorIsTop(id)) lo.y = 1.0f - THICK;
            else hi.y = THICK;
        }
        else
        {
            switch ((trapdoorFacing(id) + 2) & 3)
            {
                case 0: hi.z = THICK; break;
                case 1: lo.x = 1.0f - THICK; break;
                case 2: lo.z = 1.0f - THICK; break;
                default: hi.x = THICK; break;
            }
        }
        emitBox(snap, lx, y, lz, id, lo, hi, out);
    }

    // A fence: a post in the middle of the cell, with an arm reaching
    // towards each neighbour worth joining on to. Two rails per arm,
    // which is what makes a run of fence read as a fence rather than a
    // row of posts.
    void emitFence(const MeshSnapshot& snap, int lx, int y, int lz,
                   BlockId id, std::vector<float>& out)
    {
        constexpr float POST_LO = 0.375f, POST_HI = 0.625f;
        constexpr float RAIL_LO = 0.4375f, RAIL_HI = 0.5625f;

        emitBox(snap, lx, y, lz, id, glm::vec3(POST_LO, 0.0f, POST_LO),
                glm::vec3(POST_HI, 1.5f, POST_HI), out);

        // Anything solid is worth joining on to, which is what lets a
        // fence meet a wall without leaving a gap.
        auto joins = [&](int dx, int dz) {
            const BlockId other = snap.block(lx + dx, y, lz + dz);
            return isFence(other) || isOpaque(other);
        };

        // The two rails of an arm, at the heights Minecraft puts them.
        const float railY[2][2] = { { 0.375f, 0.5625f }, { 0.8125f, 1.0f } };

        for (int rail = 0; rail < 2; ++rail)
        {
            const float lo = railY[rail][0];
            const float hi = railY[rail][1];

            if (joins(0, -1))
                emitBox(snap, lx, y, lz, id, glm::vec3(RAIL_LO, lo, 0.0f),
                        glm::vec3(RAIL_HI, hi, POST_LO), out);
            if (joins(0, 1))
                emitBox(snap, lx, y, lz, id, glm::vec3(RAIL_LO, lo, POST_HI),
                        glm::vec3(RAIL_HI, hi, 1.0f), out);
            if (joins(-1, 0))
                emitBox(snap, lx, y, lz, id, glm::vec3(0.0f, lo, RAIL_LO),
                        glm::vec3(POST_LO, hi, RAIL_HI), out);
            if (joins(1, 0))
                emitBox(snap, lx, y, lz, id, glm::vec3(POST_HI, lo, RAIL_LO),
                        glm::vec3(1.0f, hi, RAIL_HI), out);
        }
    }

    // A ladder: one flat panel hard against the wall it hangs on, drawn
    // from both sides so it does not vanish when you climb past it.
    void emitLadder(const MeshSnapshot& snap, int lx, int y, int lz,
                    BlockId id, std::vector<float>& out)
    {
        const TileUV uv = tileUV(blockInfo(id).tileSide);

        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;

        // A hair off the wall, so it does not fight the wall's own face
        // for the same pixels.
        constexpr float GAP = 0.03f;

        const float x0 = static_cast<float>(lx);
        const float y0 = static_cast<float>(y);
        const float z0 = static_cast<float>(lz);

        glm::vec3 corner[4];

        switch (blockFacing(id))
        {
            case 0: // hanging on the wall to the north, facing south
                corner[0] = { x0,        y0,        z0 + GAP };
                corner[1] = { x0 + 1.0f, y0,        z0 + GAP };
                corner[2] = { x0 + 1.0f, y0 + 1.0f, z0 + GAP };
                corner[3] = { x0,        y0 + 1.0f, z0 + GAP };
                break;
            case 1: // on the wall to the east, facing west
                corner[0] = { x0 + 1.0f - GAP, y0,        z0 };
                corner[1] = { x0 + 1.0f - GAP, y0,        z0 + 1.0f };
                corner[2] = { x0 + 1.0f - GAP, y0 + 1.0f, z0 + 1.0f };
                corner[3] = { x0 + 1.0f - GAP, y0 + 1.0f, z0 };
                break;
            case 2: // on the wall to the south, facing north
                corner[0] = { x0 + 1.0f, y0,        z0 + 1.0f - GAP };
                corner[1] = { x0,        y0,        z0 + 1.0f - GAP };
                corner[2] = { x0,        y0 + 1.0f, z0 + 1.0f - GAP };
                corner[3] = { x0 + 1.0f, y0 + 1.0f, z0 + 1.0f - GAP };
                break;
            default: // on the wall to the west, facing east
                corner[0] = { x0 + GAP, y0,        z0 + 1.0f };
                corner[1] = { x0 + GAP, y0,        z0 };
                corner[2] = { x0 + GAP, y0 + 1.0f, z0 };
                corner[3] = { x0 + GAP, y0 + 1.0f, z0 + 1.0f };
                break;
        }

        const float u[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
        const float v[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };

        const int front[6] = { 0, 1, 2, 0, 2, 3 };
        const int back[6]  = { 0, 2, 1, 0, 3, 2 };

        for (int k : front)
            pushVertex(out, corner[k].x, corner[k].y, corner[k].z, u[k], v[k], 1.0f, sky, block, tagOf(CROSS_FACE, id));
        for (int k : back)
            pushVertex(out, corner[k].x, corner[k].y, corner[k].z, u[k], v[k], 0.86f, sky, block, tagOf(CROSS_FACE, id));
    }

    // A rail: one flat panel just off the floor, turned so the strips
    // run the way the shape says. Drawn from above only -- there is
    // nothing to see from under a sleeper.
    void emitRail(const MeshSnapshot& snap, int lx, int y, int lz,
                  BlockId id, std::vector<float>& out)
    {
        const TileUV uv = tileUV(blockInfo(id).tileSide);

        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;

        // Clear of the floor it lies on, or the two fight for the same
        // pixels and the rail flickers in and out as you walk.
        constexpr float LIFT = 0.0625f;

        const float x0 = static_cast<float>(lx);
        const float y0 = static_cast<float>(y) + LIFT;
        const float z0 = static_cast<float>(lz);

        const glm::vec3 corner[4] = {
            { x0,        y0, z0        },
            { x0 + 1.0f, y0, z0        },
            { x0 + 1.0f, y0, z0 + 1.0f },
            { x0,        y0, z0 + 1.0f },
        };

        // The tile is painted running north to south, and the corner
        // one bent from its north edge round to its east. Every other
        // shape is one of those two turned a quarter at a time.
        int turn = 0;
        switch (id)
        {
            case Blocks::RailEastWest:  turn = 1; break;
            case Blocks::RailSouthEast: turn = 1; break;
            case Blocks::RailSouthWest: turn = 2; break;
            case Blocks::RailNorthWest: turn = 3; break;
            default: break;
        }

        const float u[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
        const float v[4] = { uv.vTop, uv.vTop, uv.vBottom, uv.vBottom };

        const int top[6] = { 0, 2, 1, 0, 3, 2 };
        for (int k : top)
        {
            const int t = (k + turn) & 3;
            pushVertex(out, corner[k].x, corner[k].y, corner[k].z, u[t], v[t], 1.0f, sky, block, tagOf(2, id));
        }
    }

    bool isDust(BlockId id) { return id == Blocks::RedstoneDust || id == Blocks::RedstoneDustLit; }

    bool dustLinks(BlockId id)
    {
        return isDust(id) || id == Blocks::RedstoneTorch || id == Blocks::RedstoneTorchOff ||
               id == Blocks::LeverOff || id == Blocks::LeverOn || id == Blocks::ButtonOff ||
               id == Blocks::ButtonOn || id == Blocks::RedstoneBlock || id == Blocks::Observer ||
               id == Blocks::ObserverLit;
    }

    void emitDust(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const bool lit = id == Blocks::RedstoneDustLit;
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;
        static const int SIDE[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };

        bool linked[4] = { false, false, false, false };
        bool climbs[4] = { false, false, false, false };
        const bool roofed = isOpaque(snap.block(lx, y + 1, lz));
        for (int s = 0; s < 4; ++s)
        {
            const int nx = lx + SIDE[s][0];
            const int nz = lz + SIDE[s][1];
            const BlockId beside = snap.block(nx, y, nz);
            if (dustLinks(beside)) linked[s] = true;
            else if (isOpaque(beside) && !roofed && isDust(snap.block(nx, y + 1, nz))) linked[s] = climbs[s] = true;
            else if (!isOpaque(beside) && isDust(snap.block(nx, y - 1, nz))) linked[s] = true;
        }

        int count = 0;
        for (bool l : linked) count += l ? 1 : 0;
        if (count == 0) for (bool& l : linked) l = true;
        if (count == 1)
            for (int s = 0; s < 4; ++s)
                if (linked[s]) { linked[(s + 2) & 3] = true; break; }

        const float x0 = static_cast<float>(lx);
        const float z0 = static_cast<float>(lz);
        const float y0 = static_cast<float>(y);

        auto flat = [&](int tile, float ax, float az, float bx, float bz, bool across, float lift) {
            const TileUV uv = tileUV(tile);
            const float xs[4] = { ax, bx, bx, ax };
            const float zs[4] = { az, az, bz, bz };
            float us[4], vs[4];
            for (int c = 0; c < 4; ++c)
            {
                const float a = across ? zs[c] : xs[c];
                const float b = across ? xs[c] : zs[c];
                us[c] = uv.u0 + (uv.u1 - uv.u0) * a;
                vs[c] = uv.vTop + (uv.vBottom - uv.vTop) * b;
            }
            const int order[12] = { 0, 2, 1, 0, 3, 2, 0, 1, 2, 0, 2, 3 };
            for (int k : order)
                pushVertex(out, x0 + xs[k], y0 + lift, z0 + zs[k], us[k], vs[k], 1.0f, sky, block, tagOf(2, id));
        };

        const int line0 = lit ? Tiles::DustLine0On : Tiles::DustLine0Off;
        const int line1 = lit ? Tiles::DustLine1On : Tiles::DustLine1Off;
        if (linked[0]) flat(line0, 0.0f, 0.0f, 1.0f, 0.5f, false, 0.030f);
        if (linked[2]) flat(line0, 0.0f, 0.5f, 1.0f, 1.0f, false, 0.030f);
        if (linked[1]) flat(line1, 0.5f, 0.0f, 1.0f, 1.0f, true, 0.031f);
        if (linked[3]) flat(line1, 0.0f, 0.0f, 0.5f, 1.0f, true, 0.031f);
        if ((linked[0] || linked[2]) && (linked[1] || linked[3]))
            flat(lit ? Tiles::DustDotOn : Tiles::DustDotOff, 0.0f, 0.0f, 1.0f, 1.0f, false, 0.032f);

        for (int s = 0; s < 4; ++s)
        {
            if (!climbs[s]) continue;
            const TileUV uv = tileUV(line0);
            const float inset = 0.02f;
            glm::vec3 p[4];
            if (s == 0 || s == 2)
            {
                const float z = s == 0 ? inset : 1.0f - inset;
                p[0] = { 0.0f, 0.0f, z }; p[1] = { 1.0f, 0.0f, z }; p[2] = { 1.0f, 1.0f, z }; p[3] = { 0.0f, 1.0f, z };
            }
            else
            {
                const float x = s == 3 ? inset : 1.0f - inset;
                p[0] = { x, 0.0f, 0.0f }; p[1] = { x, 0.0f, 1.0f }; p[2] = { x, 1.0f, 1.0f }; p[3] = { x, 1.0f, 0.0f };
            }
            const float us[4] = { uv.u0, uv.u1, uv.u1, uv.u0 };
            const float vs[4] = { uv.vBottom, uv.vBottom, uv.vTop, uv.vTop };
            const int order[12] = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
            for (int k : order)
                pushVertex(out, x0 + p[k].x, y0 + p[k].y, z0 + p[k].z, us[k], vs[k], 0.8f, sky, block, tagOf(7, id));
        }
    }

    void emitPart(std::vector<float>& out, const glm::vec3& cell, const glm::vec3& from, const glm::vec3& to,
                  int tile, const float (*faceUV)[4], float angle, const glm::vec3& pivot,
                  float sky, float block, BlockId id, bool skipBottom)
    {
        const TileUV uv = tileUV(tile);
        const float c = std::cos(glm::radians(angle));
        const float s = std::sin(glm::radians(angle));
        auto place = [&](glm::vec3 p) {
            if (angle != 0.0f)
            {
                const glm::vec3 q = p - pivot;
                p = pivot + glm::vec3(q.x, q.y * c - q.z * s, q.y * s + q.z * c);
            }
            return cell + p / 16.0f;
        };
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        static const float SHADE[6] = { 0.72f, 0.72f, 1.0f, 0.5f, 0.86f, 0.86f };

        for (int f = 0; f < 6; ++f)
        {
            if (f == 3 && skipBottom) continue;
            float r[4];
            if (faceUV) { for (int k = 0; k < 4; ++k) r[k] = faceUV[f][k]; }
            else if (f < 2) { r[0] = from.z; r[1] = 16.0f - to.y; r[2] = to.z; r[3] = 16.0f - from.y; }
            else if (f < 4) { r[0] = from.x; r[1] = from.z; r[2] = to.x; r[3] = to.z; }
            else { r[0] = from.x; r[1] = 16.0f - to.y; r[2] = to.x; r[3] = 16.0f - from.y; }

            glm::vec3 p[4];
            float us[4], vs[4];
            const float st[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
            for (int k = 0; k < 4; ++k)
            {
                const float sa = st[k][0], ta = st[k][1];
                const float x = lerp(from.x, to.x, sa), z = lerp(from.z, to.z, sa);
                const float yy = lerp(from.y, to.y, ta);
                switch (f)
                {
                    case 0: p[k] = { to.x, yy, z }; break;
                    case 1: p[k] = { from.x, yy, z }; break;
                    case 2: p[k] = { x, to.y, lerp(from.z, to.z, ta) }; break;
                    case 3: p[k] = { x, from.y, lerp(from.z, to.z, ta) }; break;
                    case 4: p[k] = { x, yy, to.z }; break;
                    default: p[k] = { x, yy, from.z }; break;
                }
                us[k] = uv.u0 + (uv.u1 - uv.u0) * lerp(r[0], r[2], sa) / 16.0f;
                const float tv = (f == 2 || f == 3) ? ta : 1.0f - ta;
                vs[k] = uv.vTop + (uv.vBottom - uv.vTop) * lerp(r[1], r[3], tv) / 16.0f;
                p[k] = place(p[k]);
            }
            const int order[12] = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
            for (int k : order)
                pushVertex(out, p[k].x, p[k].y, p[k].z, us[k], vs[k], SHADE[f], sky, block,
                           tagOf(angle != 0.0f ? 7 : f, id));
        }
    }

    void emitLever(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;
        const glm::vec3 cell(static_cast<float>(lx), static_cast<float>(y), static_cast<float>(lz));
        static const float BASE_UV[6][4] = {
            { 4, 0, 12, 3 }, { 4, 0, 12, 3 }, { 5, 4, 11, 12 }, { 5, 4, 11, 12 }, { 5, 0, 11, 3 }, { 5, 0, 11, 3 },
        };
        static const float HANDLE_UV[6][4] = {
            { 7, 6, 9, 16 }, { 7, 6, 9, 16 }, { 7, 6, 9, 8 }, { 7, 6, 9, 8 }, { 7, 6, 9, 16 }, { 7, 6, 9, 16 },
        };
        emitPart(out, cell, glm::vec3(5.0f, 0.0f, 4.0f), glm::vec3(11.0f, 3.0f, 12.0f), Tiles::Cobblestone,
                 BASE_UV, 0.0f, glm::vec3(0.0f), sky, block, id, false);
        emitPart(out, cell, glm::vec3(7.0f, 1.0f, 7.0f), glm::vec3(9.0f, 11.0f, 9.0f), Tiles::LeverHandle,
                 HANDLE_UV, id == Blocks::LeverOn ? 45.0f : -45.0f, glm::vec3(8.0f, 1.0f, 8.0f),
                 sky, block, id, true);
    }

    int dripstoneTile(const MeshSnapshot& snap, int x, int y, int z, BlockId id)
    {
        const bool down = id == Blocks::DripstoneDown;
        const int step = down ? -1 : 1;
        const BlockId opposite = down ? Blocks::DripstoneUp : Blocks::DripstoneDown;
        const BlockId next = snap.block(x, y + step, z);
        if (next == opposite) return down ? Tiles::DripstoneDownTipMerge : Tiles::DripstoneUpTipMerge;
        if (next != id) return down ? Tiles::DripstoneSpike : Tiles::DripstoneUpTip;
        if (snap.block(x, y + 2 * step, z) != id) return down ? Tiles::DripstoneDownFrustum : Tiles::DripstoneUpFrustum;
        if (snap.block(x, y - step, z) == id) return down ? Tiles::DripstoneDownMiddle : Tiles::DripstoneUpMiddle;
        return down ? Tiles::DripstoneDownBase : Tiles::DripstoneUpBase;
    }

    int growthFacing(const MeshSnapshot& snap, int x, int y, int z)
    {
        static const int SUPPORT[6][3] = { { 0, -1, 0 }, { 0, 1, 0 }, { -1, 0, 0 },
                                           { 1, 0, 0 }, { 0, 0, -1 }, { 0, 0, 1 } };
        for (int pass = 0; pass < 2; ++pass)
            for (int i = 0; i < 6; ++i)
            {
                const BlockId b = snap.block(x + SUPPORT[i][0], y + SUPPORT[i][1], z + SUPPORT[i][2]);
                if (pass == 0 ? b == Blocks::BuddingAmethyst : isOpaque(b)) return i;
            }
        return 0;
    }

    glm::vec3 turnCross(int facing, float a, float h, float b)
    {
        switch (facing)
        {
            case 1: return { a, 1.0f - h, b };
            case 2: return { h, a, b };
            case 3: return { 1.0f - h, a, b };
            case 4: return { a, b, h };
            case 5: return { a, b, 1.0f - h };
            default: return { a, h, b };
        }
    }

    void emitCross(const MeshSnapshot& snap, int lx, int y, int lz,
                   BlockId id, std::vector<float>& out)
    {
        int tile = blockInfo(id).tileSide;
        int facing = 0;
        if (isPointedDripstone(id)) tile = dripstoneTile(snap, lx, y, lz, id);
        if (isAmethystGrowth(id)) facing = growthFacing(snap, lx, y, lz);
        const TileUV uv = tileUV(tile);
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = std::max(static_cast<float>(snap.blockLight(lx, y, lz)),
                                     static_cast<float>(lightEmission(id))) / 15.0f;
        const float shade = 0.95f;

        const float x0 = static_cast<float>(lx);
        const float y0 = static_cast<float>(y);
        const float z0 = static_cast<float>(lz);
        const float inset = 0.146f; // pull the quads in so they fit the block

        struct Quad { float ax, az, bx, bz; };
        const Quad quads[2] = {
            { inset, inset, 1.0f - inset, 1.0f - inset },
            { inset, 1.0f - inset, 1.0f - inset, inset }
        };

        for (const Quad& q : quads)
        {
            // Both windings so the plant is visible from either side.
            for (int side = 0; side < 2; ++side)
            {
                const float ax = side == 0 ? q.ax : q.bx;
                const float az = side == 0 ? q.az : q.bz;
                const float bx = side == 0 ? q.bx : q.ax;
                const float bz = side == 0 ? q.bz : q.az;

                const glm::vec3 base(x0, y0, z0);
                const glm::vec3 p0 = base + turnCross(facing, ax, 0.0f, az);
                const glm::vec3 p1 = base + turnCross(facing, bx, 0.0f, bz);
                const glm::vec3 p2 = base + turnCross(facing, bx, 1.0f, bz);
                const glm::vec3 p3 = base + turnCross(facing, ax, 1.0f, az);

                pushVertex(out, p0.x, p0.y, p0.z, uv.u0, uv.vBottom, shade, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, p1.x, p1.y, p1.z, uv.u1, uv.vBottom, shade, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, p2.x, p2.y, p2.z, uv.u1, uv.vTop, shade, sky, block, tagOf(CROSS_FACE, id, true));

                pushVertex(out, p0.x, p0.y, p0.z, uv.u0, uv.vBottom, shade, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, p2.x, p2.y, p2.z, uv.u1, uv.vTop, shade, sky, block, tagOf(CROSS_FACE, id, true));
                pushVertex(out, p3.x, p3.y, p3.z, uv.u0, uv.vTop, shade, sky, block, tagOf(CROSS_FACE, id, true));
            }
        }
    }
}

void buildChunkMesh(const MeshSnapshot& snapshot, MeshResult& out)
{
    out.pos = snapshot.pos;
    out.opaqueVertices.clear();
    out.transparentVertices.clear();
    out.opaqueVertices.reserve(8192);

    for (int y = 0; y < Chunk::SY; ++y)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            for (int lx = 0; lx < Chunk::SX; ++lx)
            {
                const BlockId id = snapshot.block(lx, y, lz);
                if (!isVisible(id)) continue;

                if (id == Blocks::LeverOff || id == Blocks::LeverOn)
                {
                    emitLever(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isCross(id))
                {
                    emitCross(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isDust(id))
                {
                    emitDust(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isWallPanel(id))
                {
                    emitLadder(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isPinkPetals(id))
                {
                    const int amount = id - Blocks::PinkPetals1 + 1;
                    static const float QUARTERS[4][2] = { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 0.5f }, { 0.0f, 0.5f } };
                    for (int q = 0; q < amount; ++q)
                        emitBox(snapshot, lx, y, lz, id, glm::vec3(QUARTERS[q][0], 0.0f, QUARTERS[q][1]),
                                glm::vec3(QUARTERS[q][0] + 0.5f, 0.0625f, QUARTERS[q][1] + 0.5f), out.opaqueVertices);
                }
                else if (isRail(id) || isFlat(id))
                {
                    emitRail(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isStairs(id))
                {
                    emitStairs(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isFence(id))
                {
                    emitFence(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isDoor(id))
                {
                    emitDoor(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isTrapdoor(id))
                {
                    emitTrapdoor(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else
                {
                    std::vector<float>& target = usesTransparentPass(id)
                        ? out.transparentVertices
                        : out.opaqueVertices;
                    if (isFluid(id)) emitFluid(snapshot, lx, y, lz, id, target);
                    else emitCubeFaces(snapshot, lx, y, lz, id, target);
                }
            }
        }
    }
}
