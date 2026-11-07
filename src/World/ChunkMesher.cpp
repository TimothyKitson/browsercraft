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
        return isLiquid(id) || isGlassLike(id) || isPane(id) || id == Blocks::Ice;
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
            else if (water && (isLeaves(next) || isGlassLike(next) || isPane(next) || next == Blocks::Ice ||
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
                 const glm::vec3& lo, const glm::vec3& hi, std::vector<float>& out,
                 bool turnTop = false, bool cullSame = false)
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
            if (flush && cullSame && snap.block(lx + face.nx, y + face.ny, lz + face.nz) == id)
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
                else if (face.ny != 0 && turnTop)
                {
                    su = cz ? hi.z : lo.z;
                    sv = cx ? hi.x : lo.x;
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

    struct PartFace { int tile; float u0, v0, u1, v1; int turn = 0; bool flipU = false; };

    void emitPart(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id,
                  const glm::vec3& lo, const glm::vec3& hi, const PartFace faces[6],
                  std::vector<float>& out)
    {
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;
        const glm::vec3 base(static_cast<float>(lx), static_cast<float>(y), static_cast<float>(lz));

        for (int f = 0; f < 6; ++f)
        {
            if (faces[f].tile < 0) continue;
            const FaceDef& face = FACES[f];
            const bool flush =
                (face.nx < 0 && lo.x <= 0.0f) || (face.nx > 0 && hi.x >= 1.0f) ||
                (face.ny < 0 && lo.y <= 0.0f) || (face.ny > 0 && hi.y >= 1.0f) ||
                (face.nz < 0 && lo.z <= 0.0f) || (face.nz > 0 && hi.z >= 1.0f);
            if (flush && isOpaque(snap.block(lx + face.nx, y + face.ny, lz + face.nz))) continue;

            const TileUV uv = tileUV(faces[f].tile);
            float vx[4], vy[4], vz[4], vu[4], vv[4];
            for (int c = 0; c < 4; ++c)
            {
                vx[c] = base.x + (face.corner[c][0] ? hi.x : lo.x);
                vy[c] = base.y + (face.corner[c][1] ? hi.y : lo.y);
                vz[c] = base.z + (face.corner[c][2] ? hi.z : lo.z);
                const float wu = face.uv[c][0], wv = face.uv[c][1];
                float su = wu, sv = wv;
                switch (faces[f].turn & 3)
                {
                    case 1: su = wv; sv = 1.0f - wu; break;
                    case 2: su = 1.0f - wu; sv = 1.0f - wv; break;
                    case 3: su = 1.0f - wv; sv = wu; break;
                    default: break;
                }
                if (faces[f].flipU) su = 1.0f - su;
                const float pu = faces[f].u0 + (faces[f].u1 - faces[f].u0) * su;
                const float pv = faces[f].v0 + (faces[f].v1 - faces[f].v0) * sv;
                vu[c] = uv.u0 + (uv.u1 - uv.u0) * (pu / 16.0f);
                vv[c] = uv.vTop + (uv.vBottom - uv.vTop) * (pv / 16.0f);
            }
            const int order[6] = { 0, 1, 2, 0, 2, 3 };
            for (int k : order)
                pushVertex(out, vx[k], vy[k], vz[k], vu[k], vv[k], face.shade, sky, block, tagOf(f, id, face.corner[k][1] == 1));
        }
    }

    void emitTinyCross(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id,
                       float cx, float cz, float y0, float y1, float half, int tile,
                       float u0, float v0, float u1, float v1, std::vector<float>& out)
    {
        const TileUV uv = tileUV(tile);
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = std::max(static_cast<float>(snap.blockLight(lx, y, lz)),
                                     static_cast<float>(lightEmission(id))) / 15.0f;
        const float au = uv.u0 + (uv.u1 - uv.u0) * (u0 / 16.0f), bu = uv.u0 + (uv.u1 - uv.u0) * (u1 / 16.0f);
        const float tv = uv.vTop + (uv.vBottom - uv.vTop) * (v0 / 16.0f), bv = uv.vTop + (uv.vBottom - uv.vTop) * (v1 / 16.0f);
        const float x = static_cast<float>(lx) + cx, z = static_cast<float>(lz) + cz;
        const float ly = static_cast<float>(y) + y0, hy = static_cast<float>(y) + y1;
        const float d = half * 0.7071f;
        const float ends[2][4] = { { -d, -d, d, d }, { -d, d, d, -d } };
        for (const auto& e : ends)
            for (int side = 0; side < 2; ++side)
            {
                const float ax = x + (side ? e[2] : e[0]), az = z + (side ? e[3] : e[1]);
                const float bx = x + (side ? e[0] : e[2]), bz = z + (side ? e[1] : e[3]);
                pushVertex(out, ax, ly, az, au, bv, 0.95f, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, bx, ly, bz, bu, bv, 0.95f, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, bx, hy, bz, bu, tv, 0.95f, sky, block, tagOf(CROSS_FACE, id, true));
                pushVertex(out, ax, ly, az, au, bv, 0.95f, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, bx, hy, bz, bu, tv, 0.95f, sky, block, tagOf(CROSS_FACE, id, true));
                pushVertex(out, ax, hy, az, au, tv, 0.95f, sky, block, tagOf(CROSS_FACE, id, true));
            }
    }

    void emitOneCandle(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, int tile, bool lit,
                       int px, int pz, int baseY, int height, std::vector<float>& out)
    {
        const float h = static_cast<float>(height);
        const PartFace faces[6] = {
            { tile, 0.0f, 8.0f, 2.0f, 8.0f + h }, { tile, 0.0f, 8.0f, 2.0f, 8.0f + h },
            { tile, 0.0f, 6.0f, 2.0f, 8.0f },     { tile, 0.0f, 14.0f, 2.0f, 16.0f },
            { tile, 0.0f, 8.0f, 2.0f, 8.0f + h }, { tile, 0.0f, 8.0f, 2.0f, 8.0f + h } };
        const glm::vec3 lo(px / 16.0f, baseY / 16.0f, pz / 16.0f);
        const glm::vec3 hi((px + 2) / 16.0f, (baseY + height) / 16.0f, (pz + 2) / 16.0f);
        emitPart(snap, lx, y, lz, id, lo, hi, faces, out);
        const float cx = (px + 1) / 16.0f, cz = (pz + 1) / 16.0f;
        const float top = (baseY + height) / 16.0f;
        emitTinyCross(snap, lx, y, lz, id, cx, cz, top, top + 1.0f / 16.0f, 0.5f / 16.0f, tile, 0.0f, 5.0f, 1.0f, 6.0f, out);
        if (lit)
            emitTinyCross(snap, lx, y, lz, id, cx, cz, top + 0.5f / 16.0f, top + 3.0f / 16.0f, 1.25f / 16.0f,
                          Tiles::CandleFlameTile, 4.0f, 3.0f, 12.0f, 14.0f, out);
    }

    void emitCake(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int bites = cakeEaten(id);
        const float west = 1.0f + 2.0f * bites;
        const PartFace faces[6] = {
            { Tiles::CakeSide, 1.0f, 8.0f, 15.0f, 16.0f },
            { bites > 0 ? Tiles::CakeInner : Tiles::CakeSide, 1.0f, 8.0f, 15.0f, 16.0f },
            { Tiles::CakeTop, west, 1.0f, 15.0f, 15.0f },
            { Tiles::CakeBottom, west, 1.0f, 15.0f, 15.0f },
            { Tiles::CakeSide, west, 8.0f, 15.0f, 16.0f },
            { Tiles::CakeSide, west, 8.0f, 15.0f, 16.0f } };
        emitPart(snap, lx, y, lz, id, glm::vec3(west / 16.0f, 0.0f, 1.0f / 16.0f),
                 glm::vec3(15.0f / 16.0f, 0.5f, 15.0f / 16.0f), faces, out);
    }

    void emitRotated(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, int facing,
                     float x0, float y0, float z0, float x1, float y1, float z1, const PartFace local[6],
                     std::vector<float>& out)
    {
        auto toWorld = [facing](float x, float z, float& wx, float& wz) {
            switch (facing & 3)
            {
                case 1: wx = 1.0f - z; wz = x; break;
                case 2: wx = 1.0f - x; wz = 1.0f - z; break;
                case 3: wx = z; wz = 1.0f - x; break;
                default: wx = x; wz = z; break;
            }
        };
        auto turnDir = [facing](int dx, int dz, int& wx, int& wz) {
            switch (facing & 3)
            {
                case 1: wx = -dz; wz = dx; break;
                case 2: wx = -dx; wz = -dz; break;
                case 3: wx = dz; wz = -dx; break;
                default: wx = dx; wz = dz; break;
            }
        };
        float ax, az, bx, bz;
        toWorld(x0 / 16.0f, z0 / 16.0f, ax, az);
        toWorld(x1 / 16.0f, z1 / 16.0f, bx, bz);
        const glm::vec3 lo(std::min(ax, bx), y0 / 16.0f, std::min(az, bz));
        const glm::vec3 hi(std::max(ax, bx), y1 / 16.0f, std::max(az, bz));
        PartFace world[6];
        for (int k = 0; k < 6; ++k) world[k] = PartFace{ -1, 0.0f, 0.0f, 0.0f, 0.0f };
        world[2] = local[2]; world[2].turn = (local[2].turn + facing) & 3;
        world[3] = local[3]; world[3].turn = (local[3].turn + facing) & 3;
        for (int k : { 0, 1, 4, 5 })
        {
            int nx = 0, nz = 0;
            turnDir(FACES[k].nx, FACES[k].nz, nx, nz);
            int w = 0;
            for (int j : { 0, 1, 4, 5 })
                if (FACES[j].nx == nx && FACES[j].nz == nz) w = j;
            world[w] = local[k];
            int ux = 0, uz = 0;
            if (k == 0 || k == 1) turnDir(0, 1, ux, uz);
            else turnDir(1, 0, ux, uz);
            world[w].flipU = local[k].flipU != ((w == 0 || w == 1) ? uz < 0 : ux < 0);
        }
        emitPart(snap, lx, y, lz, id, lo, hi, world, out);
    }

    void emitRod(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int facing = lightningRodFacing(id);
        const int tile = blockInfo(id).tileSide;
        auto place = [facing](float x, float y, float z) {
            switch (facing)
            {
                case 1: return glm::vec3(x, 16.0f - y, z);
                case 2: return glm::vec3(x, z, 16.0f - y);
                case 3: return glm::vec3(y, z, x);
                case 4: return glm::vec3(x, z, y);
                case 5: return glm::vec3(16.0f - y, z, x);
                default: return glm::vec3(x, y, z);
            }
        };
        auto box = [&](float x0, float y0, float z0, float x1, float y1, float z1, float u0, float v0, float u1, float v1) {
            const glm::vec3 a = place(x0, y0, z0), b = place(x1, y1, z1);
            const glm::vec3 lo = glm::min(a, b) / 16.0f, hi = glm::max(a, b) / 16.0f;
            PartFace faces[6];
            for (int k = 0; k < 6; ++k) faces[k] = PartFace{ tile, u0, v0, u1, v1 };
            emitPart(snap, lx, y, lz, id, lo, hi, faces, out);
        };
        box(7.0f, 0.0f, 7.0f, 9.0f, 12.0f, 9.0f, 0.0f, 4.0f, 2.0f, 16.0f);
        box(6.0f, 12.0f, 6.0f, 10.0f, 16.0f, 10.0f, 0.0f, 0.0f, 4.0f, 4.0f);
    }

    void emitSunflowerHead(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = static_cast<float>(snap.blockLight(lx, y, lz)) / 15.0f;
        const float x0 = static_cast<float>(lx) + 0.56f, x1 = static_cast<float>(lx) + 0.68f;
        const float y0 = static_cast<float>(y) + 0.12f, y1 = static_cast<float>(y) + 0.92f;
        const float z0 = static_cast<float>(lz) + 0.1f, z1 = static_cast<float>(lz) + 0.9f;
        const TileUV front = tileUV(Tiles::SunflowerFrontTile);
        const TileUV back = tileUV(Tiles::SunflowerBackTile);
        const float tag = tagOf(CROSS_FACE, id);
        pushVertex(out, x1, y0, z0, front.u0, front.vBottom, 0.9f, sky, block, tag);
        pushVertex(out, x1, y0, z1, front.u1, front.vBottom, 0.9f, sky, block, tag);
        pushVertex(out, x0, y1, z1, front.u1, front.vTop, 0.9f, sky, block, tag);
        pushVertex(out, x1, y0, z0, front.u0, front.vBottom, 0.9f, sky, block, tag);
        pushVertex(out, x0, y1, z1, front.u1, front.vTop, 0.9f, sky, block, tag);
        pushVertex(out, x0, y1, z0, front.u0, front.vTop, 0.9f, sky, block, tag);
        pushVertex(out, x1, y0, z1, back.u0, back.vBottom, 0.7f, sky, block, tag);
        pushVertex(out, x1, y0, z0, back.u1, back.vBottom, 0.7f, sky, block, tag);
        pushVertex(out, x0, y1, z0, back.u1, back.vTop, 0.7f, sky, block, tag);
        pushVertex(out, x1, y0, z1, back.u0, back.vBottom, 0.7f, sky, block, tag);
        pushVertex(out, x0, y1, z0, back.u1, back.vTop, 0.7f, sky, block, tag);
        pushVertex(out, x0, y1, z1, back.u0, back.vTop, 0.7f, sky, block, tag);
    }

    void emitPot(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int pot = Tiles::FlowerPotTile;
        const PartFace walls[6] = {
            { pot, 5.0f, 10.0f, 11.0f, 16.0f }, { pot, 5.0f, 10.0f, 11.0f, 16.0f },
            { pot, 5.0f, 5.0f, 11.0f, 11.0f }, { pot, 5.0f, 10.0f, 11.0f, 16.0f },
            { pot, 5.0f, 10.0f, 11.0f, 16.0f }, { pot, 5.0f, 10.0f, 11.0f, 16.0f } };
        emitPart(snap, lx, y, lz, id, glm::vec3(5.0f, 0.0f, 5.0f) / 16.0f, glm::vec3(11.0f, 6.0f, 11.0f) / 16.0f, walls, out);
        const BlockId plant = pottedPlant(id);
        if (plant == Blocks::Air) return;
        const BlockInfo& info = blockInfo(plant);
        if (plant == Blocks::Cactus)
        {
            PartFace sides[6];
            for (int k = 0; k < 6; ++k) sides[k] = PartFace{ info.tileSide, 6.0f, 0.0f, 10.0f, 12.0f };
            sides[2] = PartFace{ info.tileTop, 6.0f, 6.0f, 10.0f, 10.0f };
            emitPart(snap, lx, y, lz, id, glm::vec3(6.0f, 4.0f, 6.0f) / 16.0f, glm::vec3(10.0f, 16.0f, 10.0f) / 16.0f, sides, out);
            return;
        }
        emitTinyCross(snap, lx, y, lz, id, 0.5f, 0.5f, 0.25f, 1.25f, 0.42f, info.tileSide, 0.0f, 0.0f, 16.0f, 16.0f, out);
    }

    void pushQuad(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, const glm::vec3 corner[4], int tile,
                  float u0, float v0, float u1, float v1, float shade, bool twoSided, std::vector<float>& out)
    {
        const TileUV uv = tileUV(tile);
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = std::max(static_cast<float>(snap.blockLight(lx, y, lz)),
                                     static_cast<float>(lightEmission(id))) / 15.0f;
        const glm::vec3 base(static_cast<float>(lx), static_cast<float>(y), static_cast<float>(lz));
        const float us[4] = { u0, u1, u1, u0 };
        const float vs[4] = { v1, v1, v0, v0 };
        float au[4], av[4];
        for (int c = 0; c < 4; ++c)
        {
            au[c] = uv.u0 + (uv.u1 - uv.u0) * (us[c] / 16.0f);
            av[c] = uv.vTop + (uv.vBottom - uv.vTop) * (vs[c] / 16.0f);
        }
        const float tag = tagOf(CROSS_FACE, id);
        const int front[6] = { 0, 1, 2, 0, 2, 3 };
        const int back[6] = { 0, 2, 1, 0, 3, 2 };
        for (int k : front)
        {
            const glm::vec3 p = base + corner[k];
            pushVertex(out, p.x, p.y, p.z, au[k], av[k], shade, sky, block, tag);
        }
        if (!twoSided) return;
        for (int k : back)
        {
            const glm::vec3 p = base + corner[k];
            pushVertex(out, p.x, p.y, p.z, au[k], av[k], shade * 0.8f, sky, block, tag);
        }
    }

    void emitLush(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        auto flat = [&](float yy, float x0, float z0, float x1, float z1, int tile, float shade) {
            const glm::vec3 c[4] = { glm::vec3(x0, yy, z1), glm::vec3(x1, yy, z1), glm::vec3(x1, yy, z0), glm::vec3(x0, yy, z0) };
            pushQuad(snap, lx, y, lz, id, c, tile, x0 * 16.0f, z0 * 16.0f, x1 * 16.0f, z1 * 16.0f, shade, true, out);
        };
        if (isAttachedStem(id))
        {
            static const int DX[4] = { 0, 1, 0, -1 };
            static const int DZ[4] = { -1, 0, 1, 0 };
            const int f = attachedStemFacing(id);
            const glm::vec3 mid(0.5f, 0.0f, 0.5f), d(DX[f] * 0.5f, 0.0f, DZ[f] * 0.5f), up(0.0f, 1.0f, 0.0f);
            const glm::vec3 c[4] = { mid + d, mid - d, mid - d + up, mid + d + up };
            pushQuad(snap, lx, y, lz, id, c, blockInfo(id).tileSide, 0.0f, 0.0f, 16.0f, 16.0f, 0.9f, true, out);
            return;
        }
        if (id == Blocks::SporeBlossom)
        {
            flat(15.9f / 16.0f, 0.0f, 0.0f, 1.0f, 1.0f, Tiles::SporeBlossomBaseTile, 0.8f);
            flat(13.0f / 16.0f, 0.0f, 0.0f, 1.0f, 1.0f, Tiles::SporeBlossomTile, 0.9f);
            return;
        }
        if (id == Blocks::MossCarpet)
        {
            const int moss = blockInfo(id).tileTop;
            const PartFace faces[6] = { { moss, 0, 15, 16, 16 }, { moss, 0, 15, 16, 16 }, { moss, 0, 0, 16, 16 },
                                        { moss, 0, 0, 16, 16 }, { moss, 0, 15, 16, 16 }, { moss, 0, 15, 16, 16 } };
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f, 1.0f / 16.0f, 1.0f), faces, out);
            return;
        }
        if (isGlowLichen(id))
        {
            const int tile = Tiles::GlowLichenTile;
            const float e = 0.02f;
            switch (glowLichenFace(id))
            {
                case 0: flat(e, 0, 0, 1, 1, tile, 1.0f); break;
                case 1: flat(1.0f - e, 0, 0, 1, 1, tile, 0.6f); break;
                case 2: { const glm::vec3 c[4] = { { 1, 0, e }, { 0, 0, e }, { 0, 1, e }, { 1, 1, e } };
                          pushQuad(snap, lx, y, lz, id, c, tile, 0, 0, 16, 16, 0.86f, true, out); break; }
                case 3: { const glm::vec3 c[4] = { { 1 - e, 0, 1 }, { 1 - e, 0, 0 }, { 1 - e, 1, 0 }, { 1 - e, 1, 1 } };
                          pushQuad(snap, lx, y, lz, id, c, tile, 0, 0, 16, 16, 0.72f, true, out); break; }
                case 4: { const glm::vec3 c[4] = { { 0, 0, 1 - e }, { 1, 0, 1 - e }, { 1, 1, 1 - e }, { 0, 1, 1 - e } };
                          pushQuad(snap, lx, y, lz, id, c, tile, 0, 0, 16, 16, 0.86f, true, out); break; }
                default: { const glm::vec3 c[4] = { { e, 0, 0 }, { e, 0, 1 }, { e, 1, 1 }, { e, 1, 0 } };
                           pushQuad(snap, lx, y, lz, id, c, tile, 0, 0, 16, 16, 0.72f, true, out); break; }
            }
            return;
        }
        if (isAzaleaBush(id))
        {
            const BlockInfo& info = blockInfo(id);
            const PartFace faces[6] = { { info.tileSide, 0, 0, 16, 16 }, { info.tileSide, 0, 0, 16, 16 },
                                        { info.tileTop, 0, 0, 16, 16 }, { -1, 0, 0, 0, 0 },
                                        { info.tileSide, 0, 0, 16, 16 }, { info.tileSide, 0, 0, 16, 16 } };
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f), faces, out);
            emitTinyCross(snap, lx, y, lz, id, 0.5f, 0.5f, 0.0f, 1.0f, 0.5f, Tiles::AzaleaPlantTile, 0, 0, 16, 16, out);
            return;
        }
        if (isBigDripleafStem(id))
        {
            emitTinyCross(snap, lx, y, lz, id, 0.5f, 0.5f, 0.0f, 1.0f, 0.35f, Tiles::BigDripleafStemTile, 0, 0, 16, 16, out);
            return;
        }
        if (isSmallDripleaf(id))
        {
            const bool upper = smallDripleafUpper(id);
            emitTinyCross(snap, lx, y, lz, id, 0.5f, 0.5f, 0.0f, upper ? 0.8f : 1.0f, 0.3f,
                          upper ? Tiles::SmallDripleafStemTopTile : Tiles::SmallDripleafStemBottomTile, 0, 0, 16, upper ? 13.0f : 16.0f, out);
            if (upper)
            {
                flat(13.0f / 16.0f, 0.05f, 0.05f, 0.55f, 0.55f, Tiles::SmallDripleafTopTile, 1.0f);
                flat(12.0f / 16.0f, 0.45f, 0.45f, 0.95f, 0.95f, Tiles::SmallDripleafTopTile, 1.0f);
            }
            return;
        }
        if (isBigDripleaf(id))
        {
            emitTinyCross(snap, lx, y, lz, id, 0.5f, 0.5f, 0.0f, 15.0f / 16.0f, 0.35f, Tiles::BigDripleafStemTile, 0, 0, 16, 15.0f, out);
            const DripleafTilt tilt = dripleafTilt(id);
            const float angle = tilt == DripleafTilt::Partial ? 0.3927f : tilt == DripleafTilt::Full ? 0.7854f : 0.0f;
            static const int AWAY[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
            const int facing = dripleafFacing(id);
            const glm::vec2 dir(static_cast<float>(AWAY[facing][0]), static_cast<float>(AWAY[facing][1]));
            const float top = 15.0f / 16.0f;
            glm::vec3 c[4] = { { 0, top, 1 }, { 1, top, 1 }, { 1, top, 0 }, { 0, top, 0 } };
            for (glm::vec3& p : c)
            {
                const float along = (p.x - 0.5f) * dir.x + (p.z - 0.5f) * dir.y;
                if (along > 0.0f) p.y -= std::sin(angle) * along;
            }
            pushQuad(snap, lx, y, lz, id, c, Tiles::BigDripleafTopTile, 0, 0, 16, 16, 1.0f, true, out);
        }
    }

    void emitChest(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const ChestState state = chestState(id);
        const int set = state.kind == 2 ? 3 : (christmasChests() ? 2 : state.kind);
        const int half = state.kind == 2 ? 0 : state.half;
        const int base = Tiles::ChestTileFirst + (set * 3 + half) * 5;
        const float x0 = half == 2 ? 0.0f : 1.0f;
        const float x1 = half == 1 ? 16.0f : 15.0f;
        const float z0 = 1.0f, z1 = 15.0f, top = 14.0f;
        PartFace faces[6];
        faces[0] = PartFace{ half == 1 ? -1 : base + 2, 16.0f - z0, 2.0f, 16.0f - z1, 16.0f };
        faces[1] = PartFace{ half == 2 ? -1 : base + 2, z0, 2.0f, z1, 16.0f };
        faces[2] = PartFace{ base + 0, 16.0f - x0, 16.0f - z0, 16.0f - x1, 16.0f - z1 };
        faces[3] = PartFace{ base + 4, x0, z0, x1, z1 };
        faces[4] = PartFace{ base + 3, x0, 2.0f, x1, 16.0f };
        faces[5] = PartFace{ base + 1, 16.0f - x0, 2.0f, 16.0f - x1, 16.0f };
        emitRotated(snap, lx, y, lz, id, state.facing, x0, 0.0f, z0, x1, top, z1, faces, out);

        const float latchX0 = half == 0 ? 7.0f : (half == 1 ? 15.0f : 0.0f);
        const float latchX1 = half == 0 ? 9.0f : (half == 1 ? 16.0f : 1.0f);
        PartFace latch[6];
        for (int k = 0; k < 6; ++k) latch[k] = PartFace{ base + 1, 0.0f, 0.0f, 2.0f, 2.0f };
        if (half == 1) latch[0].tile = -1;
        if (half == 2) latch[1].tile = -1;
        emitRotated(snap, lx, y, lz, id, state.facing, latchX0, 7.0f, 0.0f, latchX1, 11.0f, z0, latch, out);
    }

    void emitBed(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int colour = bedColour(id);
        const int facing = bedFacing(id);
        const bool head = bedIsHead(id);

        auto emitLocal = [&](float x0, float y0, float z0, float x1, float y1, float z1, const PartFace local[6]) {
            emitRotated(snap, lx, y, lz, id, facing, x0, y0, z0, x1, y1, z1, local, out);
        };

        const int topTile = head ? Tiles::BedHeadTopTile + colour : Tiles::BedFootTopTile + colour;
        const int sides = Tiles::BedSidesTile + colour;
        const int ends = Tiles::BedEndsTile + colour;
        const float row = head ? 0.0f : 6.0f;
        const PartFace mattress[6] = {
            { sides, 0.0f, row, 16.0f, row + 6.0f },
            { sides, 0.0f, row, 16.0f, row + 6.0f },
            { topTile, 0.0f, 0.0f, 16.0f, 16.0f },
            { ends, 0.0f, 12.0f, 16.0f, 16.0f },
            { head ? -1 : ends, 0.0f, 6.0f, 16.0f, 12.0f },
            { head ? ends : -1, 0.0f, 0.0f, 16.0f, 6.0f } };
        emitLocal(0.0f, 3.0f, 0.0f, 16.0f, 9.0f, 16.0f, mattress);

        PartFace leg[6];
        for (int k = 0; k < 6; ++k) leg[k] = PartFace{ sides, 0.0f, 12.0f, 3.0f, 15.0f };
        const float lz0 = head ? 0.0f : 13.0f;
        emitLocal(0.0f, 0.0f, lz0, 3.0f, 3.0f, lz0 + 3.0f, leg);
        emitLocal(13.0f, 0.0f, lz0, 16.0f, 3.0f, lz0 + 3.0f, leg);
    }

    void emitCandles(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        if (isCandleCake(id))
        {
            emitCake(snap, lx, y, lz, id, out);
            const int shade = candleCakeShade(id);
            const bool lit = candleCakeLit(id);
            const int tile = blockInfo(candleOf(shade, 1, lit)).tileSide;
            emitOneCandle(snap, lx, y, lz, id, tile, lit, 7, 7, 8, 6, out);
            return;
        }
        struct Spot { int x, z, h; };
        static const Spot SPOTS[4][4] = {
            { { 7, 7, 6 } },
            { { 5, 7, 5 }, { 9, 6, 6 } },
            { { 7, 9, 3 }, { 5, 7, 5 }, { 8, 6, 6 } },
            { { 6, 8, 3 }, { 9, 8, 5 }, { 5, 5, 5 }, { 8, 5, 6 } } };
        const int count = candleCount(id);
        const bool lit = candleLit(id);
        const int tile = blockInfo(id).tileSide;
        for (int k = 0; k < count; ++k)
            emitOneCandle(snap, lx, y, lz, id, tile, lit, SPOTS[count - 1][k].x, SPOTS[count - 1][k].z, 0,
                          SPOTS[count - 1][k].h, out);
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

    void emitPane(const MeshSnapshot& snap, int lx, int y, int lz,
                  BlockId id, std::vector<float>& out)
    {
        constexpr float LO = 0.4375f, HI = 0.5625f;
        const int links = paneLinks([&](int dx, int dz) { return snap.block(lx + dx, y, lz + dz); });

        emitBox(snap, lx, y, lz, id, glm::vec3(LO, 0.0f, LO), glm::vec3(HI, 1.0f, HI), out, false, true);
        if (links & 1)
            emitBox(snap, lx, y, lz, id, glm::vec3(LO, 0.0f, 0.0f), glm::vec3(HI, 1.0f, LO), out, false, true);
        if (links & 2)
            emitBox(snap, lx, y, lz, id, glm::vec3(LO, 0.0f, HI), glm::vec3(HI, 1.0f, 1.0f), out, false, true);
        if (links & 4)
            emitBox(snap, lx, y, lz, id, glm::vec3(0.0f, 0.0f, LO), glm::vec3(LO, 1.0f, HI), out, true, true);
        if (links & 8)
            emitBox(snap, lx, y, lz, id, glm::vec3(HI, 0.0f, LO), glm::vec3(1.0f, 1.0f, HI), out, true, true);
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
                    if (id == doublePlantOf(DoublePlant::Sunflower, true))
                        emitSunflowerHead(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isFlowerPot(id))
                {
                    emitPot(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (blockInfo(id).render == RenderType::Lush)
                {
                    emitLush(snapshot, lx, y, lz, id, out.opaqueVertices);
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
                else if (isPane(id))
                {
                    emitPane(snapshot, lx, y, lz, id, out.transparentVertices);
                }
                else if (isCandle(id) || isCandleCake(id))
                {
                    emitCandles(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isCake(id))
                {
                    emitCake(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isColourBed(id))
                {
                    emitBed(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isChestFamily(id))
                {
                    emitChest(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isLightningRod(id))
                {
                    emitRod(snapshot, lx, y, lz, id, out.opaqueVertices);
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
