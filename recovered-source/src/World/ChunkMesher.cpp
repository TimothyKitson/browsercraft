#include "ChunkMesher.h"
#include "Renderer/Atlas.h"
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
        if (neighbour == self) return false;  // hide interior faces of water/glass/leaf volumes
        if (isCross(neighbour)) return true;
        return true;
    }

    float aoFactor(int occluders)
    {
        // 0 = fully open corner, 3 = tightly enclosed.
        static const float LEVELS[4] = { 1.0f, 0.78f, 0.58f, 0.42f };
        return LEVELS[occluders < 0 ? 0 : (occluders > 3 ? 3 : occluders)];
    }

    void pushVertex(std::vector<float>& out,
                    float x, float y, float z,
                    float u, float v,
                    float shade, float sky, float block)
    {
        out.push_back(x);
        out.push_back(y);
        out.push_back(z);
        out.push_back(u);
        out.push_back(v);
        out.push_back(shade);
        out.push_back(sky);
        out.push_back(block);
    }

    int tileForFace(const BlockInfo& info, int faceIndex)
    {
        if (faceIndex == 2) return info.tileTop;
        if (faceIndex == 3) return info.tileBottom;
        return info.tileSide;
    }

    void emitCubeFaces(const MeshSnapshot& snap, int lx, int y, int lz,
                       BlockId id, std::vector<float>& out)
    {
        const BlockInfo& info = blockInfo(id);
        const bool liquid = (info.render == RenderType::Liquid);

        // Liquids sit slightly below a full block unless covered by more liquid.
        float topY = 1.0f;
        if (liquid && !isLiquid(snap.block(lx, y + 1, lz)))
            topY = 0.875f;

        for (int f = 0; f < 6; ++f)
        {
            const FaceDef& face = FACES[f];
            const int nx = lx + face.nx;
            const int ny = y + face.ny;
            const int nz = lz + face.nz;

            if (!faceVisible(id, snap.block(nx, ny, nz))) continue;

            const TileUV uv = tileUV(tileForFace(info, f));
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
                    pushVertex(out, vx[c], vy[c], vz[c], vu[c], vv[c], vshade[c], vsky[c], vblock[c]);
                }
            }
        }
    }

    void emitCross(const MeshSnapshot& snap, int lx, int y, int lz,
                   BlockId id, std::vector<float>& out)
    {
        const TileUV uv = tileUV(blockInfo(id).tileSide);
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

                pushVertex(out, x0 + ax, y0, z0 + az, uv.u0, uv.vBottom, shade, sky, block);
                pushVertex(out, x0 + bx, y0, z0 + bz, uv.u1, uv.vBottom, shade, sky, block);
                pushVertex(out, x0 + bx, y0 + 1.0f, z0 + bz, uv.u1, uv.vTop, shade, sky, block);

                pushVertex(out, x0 + ax, y0, z0 + az, uv.u0, uv.vBottom, shade, sky, block);
                pushVertex(out, x0 + bx, y0 + 1.0f, z0 + bz, uv.u1, uv.vTop, shade, sky, block);
                pushVertex(out, x0 + ax, y0 + 1.0f, z0 + az, uv.u0, uv.vTop, shade, sky, block);
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

                if (isCross(id))
                {
                    emitCross(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else
                {
                    std::vector<float>& target = usesTransparentPass(id)
                        ? out.transparentVertices
                        : out.opaqueVertices;
                    emitCubeFaces(snapshot, lx, y, lz, id, target);
                }
            }
        }
    }
}
