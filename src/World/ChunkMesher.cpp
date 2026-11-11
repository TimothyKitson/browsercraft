#include "ChunkMesher.h"
#include "Renderer/Atlas.h"
#include "Renderer/AtlasTiles.h"
#include "BiomeColours.h"
#include <algorithm>
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
        return isLiquid(id) || isGlassLike(id) || isPane(id) || id == Blocks::Ice || isFrostedIce(id);
    }

    // Should the face between `self` and `neighbour` be drawn?
    bool faceVisible(BlockId self, BlockId neighbour)
    {
        if (neighbour == Blocks::Air) return true;
        if (isOpaque(neighbour)) return false;

        // Two slabs side by side each show the face between them: one is
        // not hidden by the other the way two full blocks would be, and
        // hiding it would leave a hole through a slab floor.
        if (isSlab(self) || isSlab(neighbour) || isTopSlab(self) || isTopSlab(neighbour)) return true;

        // Leaves are the exception to hiding the inside of a volume. A
        // canopy meshed as a hollow shell is one layer of cut-out tiles
        // with nothing behind it, so you see through every gap in it to
        // the trunk. Drawing every leaf face means the layers behind
        // cover each other's holes and the tree reads as solid.
        if (isLeaves(self) && isLeaves(neighbour)) return true;

        if (neighbour == self) return false;  // hide interior faces of water and glass
        if ((self == Blocks::Ice || isFrostedIce(self)) && (neighbour == Blocks::Ice || isFrostedIce(neighbour))) return false;

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
        return static_cast<float>(face + 8 * static_cast<int>(shaderStandIn(id)) + (top ? 32768 : 0));
    }

    struct ColumnTint
    {
        uint32_t grass = 0;
        uint32_t foliage = 0;
        uint32_t water = 0;
        uint32_t grassBase = 1;
        uint32_t foliageBase = 1;
        uint32_t waterBase = 1;
    };
    thread_local ColumnTint t_tint;
    constexpr float NEUTRAL_TINT = 1056832.0f;

    enum TintClass : uint8_t { NO_TINT, GRASS_TINT, FOLIAGE_TINT, WATER_TINT, GRASS_BLOCK };

    const std::vector<uint8_t>& tintClasses()
    {
        static const std::vector<uint8_t> TABLE = [] {
            std::vector<uint8_t> table(static_cast<size_t>(Blocks::ExtCount), NO_TINT);
            const BlockId grassy[] = { Blocks::TallGrass, Blocks::Fern, Blocks::SugarCane, doublePlantOf(DoublePlant::TallGrass, false),
                                       doublePlantOf(DoublePlant::TallGrass, true), doublePlantOf(DoublePlant::LargeFern, false),
                                       doublePlantOf(DoublePlant::LargeFern, true) };
            for (BlockId id : grassy) table[static_cast<size_t>(id)] = GRASS_TINT;
            const BlockId leafy[] = { Blocks::Leaves, Blocks::JungleLeaves, Blocks::AcaciaLeaves, Blocks::DarkOakLeaves, Blocks::MangroveLeaves, Blocks::Vine };
            for (BlockId id : leafy) table[static_cast<size_t>(id)] = FOLIAGE_TINT;
            for (int id = 0; isBlockId(id); id = nextBlockId(id))
                if (isWater(static_cast<BlockId>(id))) table[static_cast<size_t>(id)] = WATER_TINT;
            table[static_cast<size_t>(Blocks::Grass)] = GRASS_BLOCK;
            return table;
        }();
        return TABLE;
    }

    float packTint(uint32_t colour, uint32_t base, bool selective)
    {
        if (colour == 0 || base == 0) return NEUTRAL_TINT;
        uint32_t packed = selective ? (1u << 21) : 0u;
        for (int shift = 16, slot = 14; shift >= 0; shift -= 8, slot -= 7)
        {
            const float c = static_cast<float>((colour >> shift) & 0xFFu);
            const float b = std::max(1.0f, static_cast<float>((base >> shift) & 0xFFu));
            const uint32_t q = static_cast<uint32_t>(std::clamp(c / b * 64.0f + 0.5f, 0.0f, 127.0f));
            packed |= q << slot;
        }
        return static_cast<float>(packed);
    }

    float tintFor(float tag)
    {
        const int value = static_cast<int>(tag);
        const int face = value & 7;
        const int id = (value >> 3) & 4095;
        const std::vector<uint8_t>& table = tintClasses();
        if (id < 0 || id >= static_cast<int>(table.size())) return NEUTRAL_TINT;
        switch (table[static_cast<size_t>(id)])
        {
            case GRASS_TINT: return packTint(t_tint.grass, t_tint.grassBase, false);
            case FOLIAGE_TINT: return packTint(t_tint.foliage, t_tint.foliageBase, false);
            case WATER_TINT: return packTint(t_tint.water, t_tint.waterBase, false);
            case GRASS_BLOCK:
                if (face == 2) return packTint(t_tint.grass, t_tint.grassBase, false);
                if (face == 0 || face == 1 || face == 4 || face == 5) return packTint(t_tint.grass, t_tint.grassBase, true);
                return NEUTRAL_TINT;
            default: return NEUTRAL_TINT;
        }
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
        out.push_back(tintFor(tag));
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
        if (isPiston(id))
        {
            static const int PISTON_DIR[6] = { 1, 3, 4, 5, 2, 0 };
            const int f = pistonFacing(id);
            const int dir = PISTON_DIR[faceIndex];
            const int opposite = f < 4 ? (f + 2) % 4 : 9 - f;
            if (dir == f) return isPistonBody(id) && pistonOut(id) ? Tiles::PistonArm : info.tileFront;
            if (dir == opposite && isPistonBody(id)) return Tiles::PistonBack;
            if (f < 4 && dir >= 4) return info.tileTop;
            return info.tileSide;
        }
        if (isDispenser(id) || isDropper(id) || isObserver(id))
        {
            static const int FACE_DIR[6] = { 1, 3, 4, 5, 2, 0 };
            const int f = sixFacing(id);
            const int dir = FACE_DIR[faceIndex];
            const int opposite = f < 4 ? (f + 2) % 4 : 9 - f;
            const bool vertical = f >= 4;
            if (isObserver(id))
            {
                const bool lit = observerLit(id);
                if (dir == f) return Tiles::ObserverTop;
                if (dir == opposite) return lit ? Tiles::ObserverBackLit : Tiles::ObserverBack;
                const bool arrowFace = vertical ? (dir == 0 || dir == 2) : (dir == 4 || dir == 5);
                return arrowFace ? Tiles::ObserverArrow : Tiles::ObserverSide;
            }
            const bool dispenser = isDispenser(id);
            if (dir == f) return vertical ? (dispenser ? Tiles::DispenserFront : Tiles::DropperFront)
                                          : (dispenser ? Tiles::DispenserFrontSide : Tiles::DropperFrontSide);
            if (vertical || dir >= 4) return Tiles::DispenserSide;
            return Tiles::FurnaceSide;
        }
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
            else if (water && (isLeaves(next) || isGlassLike(next) || isPane(next) || next == Blocks::Ice || isFrostedIce(next) ||
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

            const int axis = blockAxis(id);
            int faceTile = tileForFace(info, f, id);
            bool turned = false;
            const int quarter = glazedTurn(id);
            if (axis == 1)
            {
                faceTile = f == 0 ? info.tileTop : f == 1 ? info.tileBottom : info.tileSide;
                turned = f >= 2;
            }
            else if (axis == 2)
            {
                faceTile = f == 4 ? info.tileTop : f == 5 ? info.tileBottom : info.tileSide;
                turned = f <= 1;
            }
            const TileUV uv = tileUV(faceTile);
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
                float su = turned ? face.uv[c][1] : face.uv[c][0];
                float sv = turned ? 1.0f - face.uv[c][0] : face.uv[c][1];
                for (int q = 0; q < quarter; ++q)
                {
                    const float ou = su;
                    su = sv;
                    sv = 1.0f - ou;
                }
                vu[c] = (su == 0.0f) ? uv.u0 : uv.u1;
                vv[c] = (sv == 0.0f) ? uv.vTop : uv.vBottom;

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
        if (id == Blocks::Stonecutter)
        {
            const BlockInfo& info = blockInfo(id);
            const PartFace side{ info.tileSide, 0.0f, 7.0f, 16.0f, 16.0f };
            const PartFace faces[6] = { side, side, { info.tileTop, 0.0f, 0.0f, 16.0f, 16.0f }, { info.tileBottom, 0.0f, 0.0f, 16.0f, 16.0f }, side, side };
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f, 9.0f / 16.0f, 1.0f), faces, out);
            const glm::vec3 saw[4] = { { 1.0f / 16.0f, 9.0f / 16.0f, 0.5f }, { 15.0f / 16.0f, 9.0f / 16.0f, 0.5f },
                                       { 15.0f / 16.0f, 1.0f, 0.5f }, { 1.0f / 16.0f, 1.0f, 0.5f } };
            pushQuad(snap, lx, y, lz, id, saw, Tiles::StonecutterSawTile, 1.0f, 9.0f, 15.0f, 16.0f, 0.9f, true, out);
            return;
        }
        if (isSnifferEgg(id))
        {
            const int first = Tiles::SnifferEggTileFirst + (id - Blocks::SnifferEgg0) * 6;
            const PartFace faces[6] = { { first + 1, 0.0f, 0.0f, 12.0f, 16.0f }, { first + 3, 0.0f, 0.0f, 12.0f, 16.0f },
                                        { first + 4, 0.0f, 0.0f, 14.0f, 12.0f }, { first + 5, 0.0f, 0.0f, 14.0f, 12.0f },
                                        { first + 2, 0.0f, 0.0f, 14.0f, 16.0f }, { first + 0, 0.0f, 0.0f, 14.0f, 16.0f } };
            emitPart(snap, lx, y, lz, id, glm::vec3(1.0f, 0.0f, 2.0f) / 16.0f, glm::vec3(15.0f, 16.0f, 14.0f) / 16.0f, faces, out);
            return;
        }
        if (isDecoratedPot(id))
        {
            const BlockInfo& info = blockInfo(id);
            const int base = info.tileTop, side = info.tileSide;
            const float lo = 1.0f / 16.0f, hi = 15.0f / 16.0f;
            const glm::vec3 walls[4][4] = {
                { { hi, 0.0f, lo }, { lo, 0.0f, lo }, { lo, 1.0f, lo }, { hi, 1.0f, lo } },
                { { hi, 0.0f, hi }, { hi, 0.0f, lo }, { hi, 1.0f, lo }, { hi, 1.0f, hi } },
                { { lo, 0.0f, hi }, { hi, 0.0f, hi }, { hi, 1.0f, hi }, { lo, 1.0f, hi } },
                { { lo, 0.0f, lo }, { lo, 0.0f, hi }, { lo, 1.0f, hi }, { lo, 1.0f, lo } },
            };
            for (int k = 0; k < 4; ++k)
                pushQuad(snap, lx, y, lz, id, walls[k], side, 1.0f, 0.0f, 15.0f, 16.0f, k % 2 ? 0.8f : 0.9f, false, out);
            const glm::vec3 top[4] = { { lo, 1.0f, hi }, { hi, 1.0f, hi }, { hi, 1.0f, lo }, { lo, 1.0f, lo } };
            pushQuad(snap, lx, y, lz, id, top, base, 0.0f, 6.5f, 7.0f, 13.5f, 1.0f, false, out);
            const glm::vec3 bottom[4] = { { lo, 0.0f, lo }, { hi, 0.0f, lo }, { hi, 0.0f, hi }, { lo, 0.0f, hi } };
            pushQuad(snap, lx, y, lz, id, bottom, base, 7.0f, 6.5f, 14.0f, 13.5f, 0.5f, false, out);
            const PartFace neckSide{ base, 4.0f, 4.0f, 8.0f, 5.5f }, neckTop{ base, 4.0f, 0.0f, 8.0f, 4.0f };
            const PartFace neck[6] = { neckSide, neckSide, neckTop, neckTop, neckSide, neckSide };
            emitPart(snap, lx, y, lz, id, glm::vec3(4.0f, 17.0f, 4.0f) / 16.0f, glm::vec3(12.0f, 20.0f, 12.0f) / 16.0f, neck, out);
            const PartFace lipSide{ base, 3.0f, 5.5f, 6.0f, 6.0f }, lipTop{ base, 3.0f, 2.5f, 6.0f, 5.5f };
            const PartFace lip[6] = { lipSide, lipSide, lipTop, lipTop, lipSide, lipSide };
            emitPart(snap, lx, y, lz, id, glm::vec3(5.0f, 16.0f, 5.0f) / 16.0f, glm::vec3(11.0f, 17.0f, 11.0f) / 16.0f, lip, out);
            return;
        }
        if (isBell(id))
        {
            const BlockInfo& info = blockInfo(id);
            const int facing = id - Blocks::BellFirst;
            const PartFace bar{ Tiles::DarkOakPlanks, 2.0f, 2.0f, 14.0f, 4.0f };
            const PartFace bars[6] = { bar, bar, bar, bar, bar, bar };
            emitRotated(snap, lx, y, lz, id, facing, 2, 13, 7, 14, 15, 9, bars, out);
            const PartFace post{ Tiles::Stone, 0.0f, 1.0f, 4.0f, 16.0f }, cap{ Tiles::Stone, 0.0f, 0.0f, 2.0f, 4.0f };
            const PartFace posts[6] = { post, post, cap, cap, post, post };
            emitRotated(snap, lx, y, lz, id, facing, 0, 0, 6, 2, 16, 10, posts, out);
            emitRotated(snap, lx, y, lz, id, facing, 14, 0, 6, 16, 16, 10, posts, out);
            const PartFace side{ info.tileSide, 0.0f, 0.0f, 16.0f, 16.0f };
            const PartFace body[6] = { side, side, { info.tileTop, 0.0f, 0.0f, 16.0f, 16.0f }, { info.tileBottom, 0.0f, 0.0f, 16.0f, 16.0f }, side, side };
            emitRotated(snap, lx, y, lz, id, facing, 5, 6, 5, 11, 12, 11, body, out);
            emitRotated(snap, lx, y, lz, id, facing, 4, 4, 4, 12, 6, 12, body, out);
            emitRotated(snap, lx, y, lz, id, facing, 7, 12, 7, 9, 13, 9, body, out);
            return;
        }
        if (isChiseledBookshelf(id))
        {
            const BlockInfo& info = blockInfo(id);
            const int facing = shelfFacing(id);
            const PartFace side{ info.tileSide, 0.0f, 0.0f, 16.0f, 16.0f }, top{ info.tileTop, 0.0f, 0.0f, 16.0f, 16.0f };
            const PartFace shell[6] = { side, side, top, top, side, { -1, 0.0f, 0.0f, 0.0f, 0.0f } };
            emitRotated(snap, lx, y, lz, id, facing, 0, 0, 0, 16, 16, 16, shell, out);
            struct Slot { float x0, x1, y0, y1, u0, u1, v0, v1; };
            static const Slot SLOTS[6] = {
                { 10, 16, 8, 16, 0, 6, 0, 8 }, { 5, 10, 8, 16, 6, 11, 0, 8 }, { 0, 5, 8, 16, 11, 16, 0, 8 },
                { 10, 16, 0, 8, 0, 6, 8, 16 }, { 5, 10, 0, 8, 6, 11, 8, 16 }, { 0, 5, 0, 8, 11, 16, 8, 16 },
            };
            for (int i = 0; i < 6; ++i)
            {
                const Slot& s = SLOTS[i];
                const int tile = (shelfMask(id) & (1 << i)) ? Tiles::ShelfOccupiedTile : Tiles::ShelfEmptyTile;
                const PartFace face{ tile, s.u0, s.v0, s.u1, s.v1 };
                PartFace faces[6];
                for (PartFace& f : faces) f = PartFace{ -1, 0.0f, 0.0f, 0.0f, 0.0f };
                faces[5] = face;
                emitRotated(snap, lx, y, lz, id, facing, s.x0, s.y0, 0, s.x1, s.y1, 0, faces, out);
            }
            return;
        }
        if (id == Blocks::DragonEgg)
        {
            const int tile = blockInfo(id).tileSide;
            struct Layer { float lo, y0, hi, y1; };
            static const Layer LAYERS[8] = { { 6, 15, 10, 16 }, { 5, 14, 11, 15 }, { 5, 13, 11, 14 }, { 3, 11, 13, 13 },
                                             { 2, 8, 14, 11 }, { 1, 3, 15, 8 }, { 2, 1, 14, 3 }, { 3, 0, 13, 1 } };
            for (const Layer& l : LAYERS)
            {
                const PartFace side{ tile, l.lo, 16.0f - l.y1, l.hi, 16.0f - l.y0 }, flat{ tile, l.lo, l.lo, l.hi, l.hi };
                const PartFace faces[6] = { side, side, flat, flat, side, side };
                emitPart(snap, lx, y, lz, id, glm::vec3(l.lo, l.y0, l.lo) / 16.0f, glm::vec3(l.hi, l.y1, l.hi) / 16.0f, faces, out);
            }
            return;
        }
        if (id == Blocks::HeavyCore)
        {
            const int tile = blockInfo(id).tileSide;
            const PartFace side{ tile, 0.0f, 8.0f, 8.0f, 16.0f };
            const PartFace faces[6] = { side, side, { tile, 0.0f, 0.0f, 8.0f, 8.0f }, { tile, 8.0f, 0.0f, 16.0f, 8.0f }, side, side };
            emitPart(snap, lx, y, lz, id, glm::vec3(0.25f, 0.0f, 0.25f), glm::vec3(0.75f, 0.5f, 0.75f), faces, out);
            return;
        }
        if (isFire(id))
        {
            const BlockInfo& info = blockInfo(id);
            const float lean = 0.3f, h = 1.25f;
            const glm::vec3 walls[4][4] = {
                { { 0.0f, 0.0f, 0.02f }, { 1.0f, 0.0f, 0.02f }, { 1.0f, h, lean }, { 0.0f, h, lean } },
                { { 1.0f, 0.0f, 0.98f }, { 0.0f, 0.0f, 0.98f }, { 0.0f, h, 1.0f - lean }, { 1.0f, h, 1.0f - lean } },
                { { 0.02f, 0.0f, 1.0f }, { 0.02f, 0.0f, 0.0f }, { lean, h, 0.0f }, { lean, h, 1.0f } },
                { { 0.98f, 0.0f, 0.0f }, { 0.98f, 0.0f, 1.0f }, { 1.0f - lean, h, 1.0f }, { 1.0f - lean, h, 0.0f } },
            };
            for (int k = 0; k < 4; ++k)
                pushQuad(snap, lx, y, lz, id, walls[k], k < 2 ? info.tileTop : info.tileBottom, 0.0f, 0.0f, 16.0f, 16.0f, 1.0f, true, out);
            const glm::vec3 crossA[4] = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } };
            const glm::vec3 crossB[4] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } };
            pushQuad(snap, lx, y, lz, id, crossA, info.tileTop, 0.0f, 0.0f, 16.0f, 16.0f, 1.0f, true, out);
            pushQuad(snap, lx, y, lz, id, crossB, info.tileBottom, 0.0f, 0.0f, 16.0f, 16.0f, 1.0f, true, out);
            return;
        }
        if (isChain(id))
        {
            const int tile = blockInfo(id).tileSide;
            const int axis = chainAxis(id);
            auto place = [axis](float a, float along, float b) {
                if (axis == 1) return glm::vec3(along, a, b);
                if (axis == 2) return glm::vec3(a, b, along);
                return glm::vec3(a, along, b);
            };
            const float d = 1.5f / 16.0f * 0.7071f;
            for (int k = 0; k < 2; ++k)
            {
                const float sx = k == 0 ? d : d, sz = k == 0 ? d : -d;
                const glm::vec3 c[4] = { place(0.5f - sx, 0.0f, 0.5f - sz), place(0.5f + sx, 0.0f, 0.5f + sz),
                                         place(0.5f + sx, 1.0f, 0.5f + sz), place(0.5f - sx, 1.0f, 0.5f - sz) };
                pushQuad(snap, lx, y, lz, id, c, tile, k == 0 ? 0.0f : 3.0f, 0.0f, k == 0 ? 3.0f : 6.0f, 16.0f, 0.9f, true, out);
            }
            return;
        }
        if (isEndRod(id))
        {
            const int tile = blockInfo(id).tileSide;
            const int f = endRodFacing(id);
            auto box = [&](float a0, float h0, float b0, float a1, float h1, float b1, const PartFace faces[6]) {
                if (f == 1) { h0 = 16.0f - h0; h1 = 16.0f - h1; std::swap(h0, h1); }
                glm::vec3 lo(a0, h0, b0), hi(a1, h1, b1);
                if (f >= 2)
                {
                    const bool towardsMinus = f == 2 || f == 5;
                    const float n0 = towardsMinus ? 16.0f - h1 : h0, n1 = towardsMinus ? 16.0f - h0 : h1;
                    if (f == 2 || f == 4) { lo = glm::vec3(a0, b0, n0); hi = glm::vec3(a1, b1, n1); }
                    else { lo = glm::vec3(n0, a0, b0); hi = glm::vec3(n1, a1, b1); }
                }
                emitPart(snap, lx, y, lz, id, lo / 16.0f, hi / 16.0f, faces, out);
            };
            const PartFace rodSide{ tile, 0.0f, 0.0f, 2.0f, 15.0f }, rodEnd{ tile, 2.0f, 0.0f, 4.0f, 2.0f };
            const PartFace baseSide{ tile, 2.0f, 6.0f, 6.0f, 7.0f }, baseEnd{ tile, 2.0f, 2.0f, 6.0f, 6.0f };
            const PartFace rod[6] = { rodSide, rodSide, rodEnd, rodEnd, rodSide, rodSide };
            const PartFace base[6] = { baseSide, baseSide, baseEnd, baseEnd, baseSide, baseSide };
            box(7, 1, 7, 9, 16, 9, rod);
            box(6, 0, 6, 10, 1, 10, base);
            return;
        }
        if (isDiode(id))
        {
            const BlockInfo& info = blockInfo(id);
            const int facing = diodeOut(id);
            const PartFace slabSide{ info.tileSide, 0.0f, 14.0f, 16.0f, 16.0f };
            const PartFace base[6] = { slabSide, slabSide, { info.tileTop, 0.0f, 0.0f, 16.0f, 16.0f },
                                       { info.tileSide, 0.0f, 0.0f, 16.0f, 16.0f }, slabSide, slabSide };
            emitRotated(snap, lx, y, lz, id, facing, 0, 0, 0, 16, 2, 16, base, out);
            auto torch = [&](float x0, float z0, float height, bool on) {
                const int tile = on ? Tiles::RedstoneTorch : Tiles::RedstoneTorchOff;
                const PartFace side{ tile, 7.0f, 6.0f, 9.0f, 6.0f + height };
                const PartFace faces[6] = { side, side, { tile, 7.0f, 6.0f, 9.0f, 8.0f }, { tile, 7.0f, 13.0f, 9.0f, 15.0f }, side, side };
                emitRotated(snap, lx, y, lz, id, facing, x0, 2, z0, x0 + 2, 2 + height, z0 + 2, faces, out);
            };
            if (isRepeater(id))
            {
                torch(7.0f, 2.0f, 5.0f, repeaterLit(id));
                torch(7.0f, 6.0f + (repeaterDelay(id) - 1) * 2.0f, 5.0f, repeaterLit(id));
            }
            else
            {
                torch(4.0f, 11.0f, 5.0f, comparatorLit(id));
                torch(10.0f, 11.0f, 5.0f, comparatorLit(id));
                torch(7.0f, 2.0f, 2.0f, comparatorSubtracts(id));
            }
            return;
        }
        if (isDaylightDetector(id))
        {
            const BlockInfo& info = blockInfo(id);
            const PartFace side{ info.tileSide, 0.0f, 10.0f, 16.0f, 16.0f };
            const PartFace faces[6] = { side, side, { info.tileTop, 0.0f, 0.0f, 16.0f, 16.0f },
                                        { info.tileSide, 0.0f, 0.0f, 16.0f, 16.0f }, side, side };
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f, 6.0f / 16.0f, 1.0f), faces, out);
            return;
        }
        if (isTripwireHook(id))
        {
            const BlockInfo& info = blockInfo(id);
            const int wood = info.tileBottom, hook = info.tileTop, facing = hookFacing(id);
            const PartFace plateSide{ wood, 6.0f, 7.0f, 10.0f, 15.0f };
            const PartFace plate[6] = { plateSide, plateSide, { wood, 6.0f, 0.0f, 10.0f, 2.0f }, { wood, 6.0f, 14.0f, 10.0f, 16.0f },
                                        plateSide, plateSide };
            emitRotated(snap, lx, y, lz, id, facing, 6, 1, 14, 10, 9, 16, plate, out);
            const PartFace stickSide{ wood, 7.0f, 9.0f, 9.0f, 11.0f };
            const PartFace stick[6] = { stickSide, stickSide, { wood, 7.0f, 2.0f, 9.0f, 7.0f }, { wood, 7.0f, 9.0f, 9.0f, 14.0f },
                                        stickSide, stickSide };
            emitRotated(snap, lx, y, lz, id, facing, 7.4f, 5.2f, 10, 8.8f, 6.8f, 14, stick, out);
            const float drop = hookAttached(id) ? 0.0f : 1.0f;
            const PartFace ringSide{ hook, 5.0f, 3.0f, 11.0f, 4.0f };
            const PartFace ring[6] = { ringSide, ringSide, { hook, 5.0f, 3.0f, 11.0f, 9.0f }, { hook, 5.0f, 3.0f, 11.0f, 9.0f },
                                       ringSide, ringSide };
            emitRotated(snap, lx, y, lz, id, facing, 6.2f, 3.8f + drop, 7.9f, 9.8f, 4.6f + drop, 11.5f, ring, out);
            return;
        }
        if (isTripwire(id))
        {
            const int tile = blockInfo(id).tileTop;
            auto joins = [&](int dx, int dz) {
                const BlockId n = snap.block(lx + dx, y, lz + dz);
                return isTripwire(n) || isTripwireHook(n);
            };
            const bool alongX = joins(1, 0) || joins(-1, 0);
            const float h = (isTripwire(id) && id == Blocks::TripwireActive ? 1.0f : 1.5f) / 16.0f;
            const float a = 7.75f / 16.0f, b = 8.25f / 16.0f;
            if (alongX)
            {
                const glm::vec3 c[4] = { { 0.0f, h, a }, { 1.0f, h, a }, { 1.0f, h, b }, { 0.0f, h, b } };
                pushQuad(snap, lx, y, lz, id, c, tile, 0.0f, 0.0f, 16.0f, 2.0f, 1.0f, true, out);
            }
            else
            {
                const glm::vec3 c[4] = { { a, h, 1.0f }, { a, h, 0.0f }, { b, h, 0.0f }, { b, h, 1.0f } };
                pushQuad(snap, lx, y, lz, id, c, tile, 0.0f, 0.0f, 16.0f, 2.0f, 1.0f, true, out);
            }
            return;
        }
        if (isAnyCoralFan(id))
        {
            const int tile = blockInfo(id).tileSide;
            const float reach = 0.85f, lift = 0.383f, spread = 0.924f;
            static const int DX[4] = { 1, -1, 0, 0 };
            static const int DZ[4] = { 0, 0, 1, -1 };
            const glm::vec3 hinge(0.5f, 0.01f, 0.5f);
            for (int k = 0; k < 4; ++k)
            {
                const glm::vec3 d(static_cast<float>(DX[k]), 0.0f, static_cast<float>(DZ[k]));
                const glm::vec3 side(-d.z * 0.5f, 0.0f, d.x * 0.5f);
                const glm::vec3 tip = hinge + d * (spread * reach) + glm::vec3(0.0f, lift * reach, 0.0f);
                const glm::vec3 c[4] = { hinge - side, hinge + side, tip + side, tip - side };
                pushQuad(snap, lx, y, lz, id, c, tile, 0.0f, 0.0f, 16.0f, 16.0f, 0.9f, true, out);
            }
            return;
        }
        if (isAnyCoralWallFan(id))
        {
            const int tile = blockInfo(id).tileSide;
            const int f = coralWallFanFacing(id);
            auto turn = [f](glm::vec3 p) {
                switch (f)
                {
                    case 1: return glm::vec3(1.0f - p.z, p.y, p.x);
                    case 2: return glm::vec3(1.0f - p.x, p.y, 1.0f - p.z);
                    case 3: return glm::vec3(p.z, p.y, 1.0f - p.x);
                    default: return p;
                }
            };
            const float reach = 0.875f;
            for (int k = 0; k < 2; ++k)
            {
                const float tilt = (k == 0 ? 0.383f : -0.383f) * reach;
                const glm::vec3 h0(0.0f, 0.5f, 0.9375f), h1(1.0f, 0.5f, 0.9375f);
                const glm::vec3 t0 = h0 + glm::vec3(0.0f, tilt, -0.924f * reach), t1 = h1 + glm::vec3(0.0f, tilt, -0.924f * reach);
                const glm::vec3 c[4] = { turn(h0), turn(h1), turn(t1), turn(t0) };
                pushQuad(snap, lx, y, lz, id, c, tile, 0.0f, 0.0f, 16.0f, 16.0f, 0.9f, true, out);
            }
            return;
        }
        if (id == Blocks::Conduit)
        {
            const PartFace shell{ Tiles::ConduitTile, 2.0f, 2.0f, 14.0f, 14.0f };
            const PartFace faces[6] = { shell, shell, shell, shell, shell, shell };
            emitPart(snap, lx, y, lz, id, glm::vec3(5.0f) / 16.0f, glm::vec3(11.0f) / 16.0f, faces, out);
            return;
        }
        if (isTurtleEgg(id))
        {
            struct Egg { float x0, y1, z0, x1, z1; float tu0, tv0, tu1, tv1, su0, sv0, su1, sv1; };
            static const Egg EGGS[4] = {
                { 5, 7, 4, 9, 8, 0, 0, 4, 4, 1, 4, 5, 11 },
                { 1, 5, 7, 5, 11, 6, 7, 10, 11, 10, 10, 14, 15 },
                { 11, 4, 7, 14, 10, 5, 0, 8, 3, 8, 3, 11, 7 },
                { 6, 4, 9, 10, 13, 0, 11, 4, 15, 4, 11, 8, 15 },
            };
            const int tile = blockInfo(id).tileSide;
            for (int e = 0; e < turtleEggCount(id); ++e)
            {
                const Egg& g = EGGS[e];
                const PartFace side{ tile, g.su0, g.sv0, g.su1, g.sv1 };
                const PartFace top{ tile, g.tu0, g.tv0, g.tu1, g.tv1 };
                const PartFace faces[6] = { side, side, top, top, side, side };
                emitPart(snap, lx, y, lz, id, glm::vec3(g.x0, 0.0f, g.z0) / 16.0f, glm::vec3(g.x1, g.y1, g.z1) / 16.0f, faces, out);
            }
            return;
        }
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
        if (isSnowLayer(id))
        {
            const int layers = snowLayers(id);
            const int snow = blockInfo(id).tileTop;
            const float v = 16.0f - 2.0f * static_cast<float>(layers);
            const PartFace faces[6] = { { snow, 0, v, 16, 16 }, { snow, 0, v, 16, 16 }, { snow, 0, 0, 16, 16 },
                                        { snow, 0, 0, 16, 16 }, { snow, 0, v, 16, 16 }, { snow, 0, v, 16, 16 } };
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f, static_cast<float>(layers) / 8.0f, 1.0f), faces, out);
            return;
        }
        if (id == Blocks::MossCarpet || isCarpet(id))
        {
            const int moss = blockInfo(id).tileTop;
            const PartFace faces[6] = { { moss, 0, 15, 16, 16 }, { moss, 0, 15, 16, 16 }, { moss, 0, 0, 16, 16 },
                                        { moss, 0, 0, 16, 16 }, { moss, 0, 15, 16, 16 }, { moss, 0, 15, 16, 16 } };
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f, 1.0f / 16.0f, 1.0f), faces, out);
            return;
        }
        if (isCommandBlock(id) || isJigsaw(id))
        {
            static const int TURN[6][6] = {
                { 3, 3, 0, 0, 0, 0 }, { 0, 0, 1, 1, 1, 1 }, { 1, 1, 2, 2, 0, 0 },
                { 0, 0, 3, 3, 3, 3 }, { 0, 0, 0, 0, 0, 0 }, { 2, 2, 0, 0, 2, 2 },
            };
            static const int FRONT_FACE[6] = { 5, 0, 4, 1, 2, 3 };
            const int facing = isCommandBlock(id) ? commandFacing(id) : jigsawFacing(id);
            const int front = FRONT_FACE[facing];
            const int back = front ^ 1;
            int frontTile = 0, backTile = 0, sideTile = 0, lockTile = -1;
            if (isCommandBlock(id))
            {
                const int base = Tiles::CommandTileFirst + commandKind(id) * 4;
                frontTile = base;
                backTile = base + 1;
                sideTile = commandConditional(id) ? base + 3 : base + 2;
            }
            else
            {
                frontTile = Tiles::JigsawTopTile;
                backTile = Tiles::JigsawBottomTile;
                sideTile = Tiles::JigsawSideTile;
                lockTile = Tiles::JigsawLockTile;
            }
            PartFace faces[6];
            for (int f = 0; f < 6; ++f)
            {
                const int tile = f == front ? frontTile : f == back ? backTile : sideTile;
                faces[f] = PartFace{ tile, 0.0f, 0.0f, 16.0f, 16.0f, TURN[facing][f] };
            }
            if (lockTile >= 0)
            {
                const int lockFace = facing >= 4 ? 5 : 2;
                faces[lockFace] = PartFace{ lockTile, 0.0f, 0.0f, 16.0f, 16.0f, facing >= 4 ? 0 : (4 - facing) & 3 };
            }
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f), faces, out);
            return;
        }
        if (isAnySculkSensor(id))
        {
            const BlockInfo& info = blockInfo(id);
            const PartFace side{ info.tileSide, 0.0f, 8.0f, 16.0f, 16.0f };
            PartFace faces[6] = { side, side, { info.tileTop, 0.0f, 0.0f, 16.0f, 16.0f },
                                  { info.tileBottom, 0.0f, 0.0f, 16.0f, 16.0f }, side, side };
            if (isCalibratedSensor(id))
            {
                static const int BACK[4] = { 4, 1, 5, 0 };
                faces[BACK[calibratedFacing(id)]] = PartFace{ Tiles::CalibratedInputTile, 0.0f, 8.0f, 16.0f, 16.0f };
            }
            emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f, 0.5f, 1.0f), faces, out);
            const int tendril = sculkSensorLit(id) ? Tiles::SculkTendrilActiveTile : Tiles::SculkTendrilTile;
            static const float CORNERS[4][2] = { { 3.0f, 3.0f }, { 13.0f, 3.0f }, { 13.0f, 13.0f }, { 3.0f, 13.0f } };
            for (const auto& corner : CORNERS)
                emitTinyCross(snap, lx, y, lz, id, corner[0] / 16.0f, corner[1] / 16.0f, 0.5f, 1.0f, 0.25f, tendril,
                              4.0f, 8.0f, 12.0f, 16.0f, out);
            if (isCalibratedSensor(id))
                emitTinyCross(snap, lx, y, lz, id, 0.5f, 0.5f, 0.5f, 1.25f, 0.7071f, Tiles::CalibratedAmethystTile,
                              0.0f, 4.0f, 16.0f, 16.0f, out);
            return;
        }
        if (isGlowLichen(id) || isSculkVein(id))
        {
            const int tile = isSculkVein(id) ? Tiles::SculkVeinTile : Tiles::GlowLichenTile;
            const float e = 0.02f;
            switch (isSculkVein(id) ? sculkVeinFace(id) : glowLichenFace(id))
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
        const bool top = stairsUpsideDown(id);
        const float slabLo = top ? 0.5f : 0.0f;
        const float stepLo = top ? 0.0f : 0.5f;
        emitBox(snap, lx, y, lz, id, glm::vec3(0.0f, slabLo, 0.0f), glm::vec3(1.0f, slabLo + 0.5f, 1.0f), out);

        const int quadrants = stairsQuadrantsAt(id, [&](int dx, int dz) { return snap.block(lx + dx, y, lz + dz); });
        for (int side = 0; side < 4; ++side)
            if (quadrants == stairsSideQuadrants(side))
            {
                glm::vec3 lo(0.0f, stepLo, 0.0f), hi(1.0f, stepLo + 0.5f, 1.0f);
                if (side == 0) hi.z = 0.5f;
                else if (side == 1) lo.x = 0.5f;
                else if (side == 2) lo.z = 0.5f;
                else hi.x = 0.5f;
                emitBox(snap, lx, y, lz, id, lo, hi, out);
                return;
            }
        for (int q = 0; q < 4; ++q)
        {
            if (!(quadrants & (1 << q))) continue;
            const float x0 = (q & 1) ? 0.5f : 0.0f;
            const float z0 = (q & 2) ? 0.5f : 0.0f;
            emitBox(snap, lx, y, lz, id, glm::vec3(x0, stepLo, z0), glm::vec3(x0 + 0.5f, stepLo + 0.5f, z0 + 0.5f), out);
        }
    }

    void emitGate(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int facing = gateFacing(id);
        const bool across = (facing & 1) == 0;
        const bool flip = facing == 0 || facing == 3;
        auto box = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
            if (flip)
            {
                const float a = 16.0f - z1, b = 16.0f - z0;
                z0 = a;
                z1 = b;
            }
            glm::vec3 lo(x0, y0, z0), hi(x1, y1, z1);
            if (!across)
            {
                lo = glm::vec3(z0, y0, x0);
                hi = glm::vec3(z1, y1, x1);
            }
            emitBox(snap, lx, y, lz, id, lo / 16.0f, hi / 16.0f, out);
        };
        box(0, 5, 7, 2, 16, 9);
        box(14, 5, 7, 16, 16, 9);
        if (!gateOpen(id))
        {
            box(6, 6, 7, 8, 15, 9);
            box(8, 6, 7, 10, 15, 9);
            box(2, 6, 7, 6, 9, 9);
            box(2, 12, 7, 6, 15, 9);
            box(10, 6, 7, 14, 9, 9);
            box(10, 12, 7, 14, 15, 9);
        }
        else
        {
            box(0, 6, 13, 2, 15, 15);
            box(0, 6, 9, 2, 9, 13);
            box(0, 12, 9, 2, 15, 13);
            box(14, 6, 13, 16, 15, 15);
            box(14, 6, 9, 16, 9, 13);
            box(14, 12, 9, 16, 15, 13);
        }
    }

    void emitWall(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int links = shapeLinks(id, [&](int dx, int dz) { return snap.block(lx + dx, y, lz + dz); });
        const BlockId above = snap.block(lx, y + 1, lz);
        const bool straight = links == 3 || links == 12;
        const bool tall = isWall(above) || (blockInfo(above).opaque && isSolid(above));
        if (!straight || links == 0 || isWall(above))
            emitBox(snap, lx, y, lz, id, glm::vec3(0.25f, 0.0f, 0.25f), glm::vec3(0.75f, 1.0f, 0.75f), out);
        const float height = tall ? 1.0f : 0.875f;
        if (links & 1) emitBox(snap, lx, y, lz, id, glm::vec3(0.3125f, 0.0f, 0.0f), glm::vec3(0.6875f, height, 0.5f), out);
        if (links & 2) emitBox(snap, lx, y, lz, id, glm::vec3(0.3125f, 0.0f, 0.5f), glm::vec3(0.6875f, height, 1.0f), out);
        if (links & 4) emitBox(snap, lx, y, lz, id, glm::vec3(0.0f, 0.0f, 0.3125f), glm::vec3(0.5f, height, 0.6875f), out);
        if (links & 8) emitBox(snap, lx, y, lz, id, glm::vec3(0.5f, 0.0f, 0.3125f), glm::vec3(1.0f, height, 0.6875f), out);
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
                glm::vec3(POST_HI, 1.0f, POST_HI), out);

        auto joins = [&](int dx, int dz) { return fenceJoins(id, snap.block(lx + dx, y, lz + dz)); };

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

        glm::vec3 corner[4] = {
            { x0,        y0, z0        },
            { x0 + 1.0f, y0, z0        },
            { x0 + 1.0f, y0, z0 + 1.0f },
            { x0,        y0, z0 + 1.0f },
        };
        static const int RAISED[4][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 } };
        const int high = railSlopeDir(id);
        if (high >= 0)
        {
            corner[RAISED[high][0]].y += 1.0f;
            corner[RAISED[high][1]].y += 1.0f;
        }

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
            default:
                if (isRailSlope(id)) turn = (railSlopeDir(id) & 1) ? 1 : 0;
                else if (isSpecialRail(id) && specialRailEastWest(id)) turn = 1;
                break;
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
               id == Blocks::ButtonOn || id == Blocks::RedstoneBlock || isObserver(id) || isFamilyButton(id) || isPlate(id) || isFamilyLever(id) ||
               (isWallTorch(id) && wallTorchKind(id) >= 2);
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

    void reorient(std::vector<float>& out, size_t start, int lx, int y, int lz, int face, int facing)
    {
        for (size_t i = start; i + 10 <= out.size(); i += 10)
        {
            const float x = out[i] - static_cast<float>(lx);
            const float h = out[i + 1] - static_cast<float>(y);
            const float z = out[i + 2] - static_cast<float>(lz);
            float nx = x, ny = h, nz = z;
            if (face == 1)
            {
                switch (facing & 3)
                {
                    case 0: nx = x; ny = z; nz = 1.0f - h; break;
                    case 1: nx = h; ny = z; nz = x; break;
                    case 2: nx = 1.0f - x; ny = z; nz = h; break;
                    default: nx = 1.0f - h; ny = z; nz = 1.0f - x; break;
                }
            }
            else
            {
                switch (facing & 3)
                {
                    case 1: nx = z; nz = 1.0f - x; break;
                    case 2: nx = 1.0f - x; nz = 1.0f - z; break;
                    case 3: nx = 1.0f - z; nz = x; break;
                    default: break;
                }
                if (face == 2)
                {
                    ny = 1.0f - ny;
                    nz = 1.0f - nz;
                }
            }
            out[i] = nx + static_cast<float>(lx);
            out[i + 1] = ny + static_cast<float>(y);
            out[i + 2] = nz + static_cast<float>(lz);
        }
    }

    void autoBox(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, glm::vec3 lo, glm::vec3 hi,
                 int top, int bottom, int side, std::vector<float>& out)
    {
        const PartFace f[6] = { { side, 16.0f - hi.z, 16.0f - hi.y, 16.0f - lo.z, 16.0f - lo.y },
                                { side, lo.z, 16.0f - hi.y, hi.z, 16.0f - lo.y },
                                { top, lo.x, lo.z, hi.x, hi.z },
                                { bottom, lo.x, 16.0f - hi.z, hi.x, 16.0f - lo.z },
                                { side, lo.x, 16.0f - hi.y, hi.x, 16.0f - lo.y },
                                { side, 16.0f - hi.x, 16.0f - hi.y, 16.0f - lo.x, 16.0f - lo.y } };
        emitPart(snap, lx, y, lz, id, lo / 16.0f, hi / 16.0f, f, out);
    }

    void emitCross(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out);

    void emitWorkBlock(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const BlockInfo& info = blockInfo(id);
        auto box = [&](float x0, float y0, float z0, float x1, float y1, float z1, int top, int bottom, int side) {
            autoBox(snap, lx, y, lz, id, glm::vec3(x0, y0, z0), glm::vec3(x1, y1, z1), top, bottom, side, out);
        };
        if (isAnvil(id))
        {
            const int body = info.tileSide;
            box(2, 0, 2, 14, 4, 14, body, body, body);
            box(4, 4, 3, 12, 5, 13, body, body, body);
            box(6, 5, 4, 10, 10, 12, body, body, body);
            box(3, 10, 0, 13, 16, 16, info.tileTop, body, body);
            return;
        }
        if (id == Blocks::EnchantingTable)
        {
            box(0, 0, 0, 16, 12, 16, info.tileTop, info.tileBottom, info.tileSide);
            return;
        }
        if (id == Blocks::EndPortalFrame || id == Blocks::EndPortalFrameFilled)
        {
            box(0, 0, 0, 16, 13, 16, Tiles::EndFrameTop, info.tileBottom, info.tileSide);
            if (id == Blocks::EndPortalFrameFilled)
            {
                const int eye = Tiles::EndFrameEye;
                const PartFace f[6] = { { eye, 4, 0, 12, 3 }, { eye, 4, 0, 12, 3 }, { eye, 4, 4, 12, 12 },
                                        { -1, 0, 0, 16, 16 }, { eye, 4, 0, 12, 3 }, { eye, 4, 0, 12, 3 } };
                emitPart(snap, lx, y, lz, id, glm::vec3(4.0f, 13.0f, 4.0f) / 16.0f, glm::vec3(12.0f, 16.0f, 12.0f) / 16.0f, f, out);
            }
            return;
        }
        if (isHopper(id))
        {
            const int side = info.tileSide;
            box(0, 10, 0, 16, 16, 16, info.tileTop, side, side);
            box(4, 4, 4, 12, 10, 12, side, side, side);
            switch (hopperFacing(id))
            {
                case 0: box(6, 4, 0, 10, 8, 4, side, side, side); break;
                case 1: box(12, 4, 6, 16, 8, 10, side, side, side); break;
                case 2: box(6, 4, 12, 10, 8, 16, side, side, side); break;
                case 3: box(0, 4, 6, 4, 8, 10, side, side, side); break;
                default: box(6, 0, 6, 10, 4, 10, side, side, side); break;
            }
            return;
        }
        if (isComposter(id))
        {
            const int side = info.tileSide, bottom = info.tileBottom, rim = Tiles::ComposterTop;
            box(0, 0, 0, 2, 16, 16, rim, bottom, side);
            box(14, 0, 0, 16, 16, 16, rim, bottom, side);
            box(2, 0, 0, 14, 16, 2, rim, bottom, side);
            box(2, 0, 14, 14, 16, 16, rim, bottom, side);
            const int level = compostLevel(id);
            const float height = level == 0 ? 2.0f : level >= 8 ? 15.0f : 1.0f + 2.0f * static_cast<float>(level);
            box(2, 0, 2, 14, height, 14, level == 0 ? bottom : info.tileTop, bottom, side);
            return;
        }
        if (id == Blocks::BrewingStand)
        {
            const int base = Tiles::BrewingBaseTile;
            box(2, 0, 1, 8, 2, 7, base, base, base);
            box(2, 0, 9, 8, 2, 15, base, base, base);
            box(9, 0, 5, 15, 2, 11, base, base, base);
            emitCross(snap, lx, y, lz, id, out);
        }
    }

    void emitCauldron(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const BlockInfo& info = blockInfo(id);
        const int side = Tiles::CauldronSide, inner = Tiles::CauldronTop, rim = Tiles::CauldronRimTile, bottom = Tiles::CauldronBottom;
        auto wall = [&](float x0, float z0, float x1, float z1) {
            const PartFace f[6] = { { side, 16 - z1, 0, 16 - z0, 13 }, { side, z0, 0, z1, 13 }, { rim, x0, z0, x1, z1 },
                                    { bottom, x0, z0, x1, z1 }, { side, x0, 0, x1, 13 }, { side, 16 - x1, 0, 16 - x0, 13 } };
            PartFace g[6];
            for (int k = 0; k < 6; ++k) g[k] = f[k];
            if (x0 > 0.0f) g[1] = PartFace{ inner, z0, 0, z1, 12 };
            if (x1 < 16.0f) g[0] = PartFace{ inner, 16 - z1, 0, 16 - z0, 12 };
            if (z0 > 0.0f) g[5] = PartFace{ inner, 16 - x1, 0, 16 - x0, 12 };
            if (z1 < 16.0f) g[4] = PartFace{ inner, x0, 0, x1, 12 };
            emitPart(snap, lx, y, lz, id, glm::vec3(x0, 3.0f, z0) / 16.0f, glm::vec3(x1, 16.0f, z1) / 16.0f, g, out);
        };
        wall(0.0f, 0.0f, 2.0f, 16.0f);
        wall(14.0f, 0.0f, 16.0f, 16.0f);
        wall(2.0f, 0.0f, 14.0f, 2.0f);
        wall(2.0f, 14.0f, 14.0f, 16.0f);
        const PartFace floor[6] = { { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { inner, 2, 2, 14, 14 },
                                    { bottom, 2, 2, 14, 14 }, { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 } };
        emitPart(snap, lx, y, lz, id, glm::vec3(2.0f, 3.0f, 2.0f) / 16.0f, glm::vec3(14.0f, 4.0f, 14.0f) / 16.0f, floor, out);
        static const float LEGS[4][2] = { { 0, 0 }, { 12, 0 }, { 0, 12 }, { 12, 12 } };
        for (const auto& leg : LEGS)
        {
            const PartFace f[6] = { { side, 0, 13, 4, 16 }, { side, 0, 13, 4, 16 }, { -1, 0, 0, 16, 16 },
                                    { bottom, leg[0], leg[1], leg[0] + 4, leg[1] + 4 }, { side, 0, 13, 4, 16 }, { side, 0, 13, 4, 16 } };
            emitPart(snap, lx, y, lz, id, glm::vec3(leg[0], 0.0f, leg[1]) / 16.0f, glm::vec3(leg[0] + 4.0f, 3.0f, leg[1] + 4.0f) / 16.0f, f, out);
        }
        float level = 0.0f;
        if (isCauldron(id)) level = cauldronLevel(id) == 0 ? 0.0f : cauldronLevel(id) == 1 ? 9.0f : cauldronLevel(id) == 2 ? 12.0f : 15.0f;
        else if (id == Blocks::LavaCauldron) level = 15.0f;
        else level = powderCauldronLevel(id) == 1 ? 9.0f : powderCauldronLevel(id) == 2 ? 12.0f : 15.0f;
        if (level <= 0.0f) return;
        const PartFace surface[6] = { { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { info.tileTop, 2, 2, 14, 14 },
                                      { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 } };
        emitPart(snap, lx, y, lz, id, glm::vec3(2.0f, 4.0f, 2.0f) / 16.0f, glm::vec3(14.0f, level, 14.0f) / 16.0f, surface, out);
    }

    void emitLantern(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int t = blockInfo(id).tileSide;
        const float lift = lanternHangs(id) ? 1.0f : 0.0f;
        const PartFace body[6] = { { t, 0, 2, 6, 9 }, { t, 0, 2, 6, 9 }, { t, 0, 9, 6, 15 }, { t, 0, 9, 6, 15 }, { t, 0, 2, 6, 9 }, { t, 0, 2, 6, 9 } };
        const PartFace cap[6] = { { t, 1, 0, 5, 2 }, { t, 1, 0, 5, 2 }, { t, 1, 10, 5, 14 }, { -1, 0, 0, 16, 16 }, { t, 1, 0, 5, 2 }, { t, 1, 0, 5, 2 } };
        emitPart(snap, lx, y, lz, id, glm::vec3(5.0f, lift, 5.0f) / 16.0f, glm::vec3(11.0f, lift + 7.0f, 11.0f) / 16.0f, body, out);
        emitPart(snap, lx, y, lz, id, glm::vec3(6.0f, lift + 7.0f, 6.0f) / 16.0f, glm::vec3(10.0f, lift + 9.0f, 10.0f) / 16.0f, cap, out);
        const float h0 = (lift + 9.0f) / 16.0f;
        const float h1 = lanternHangs(id) ? 1.0f : (lift + 11.0f) / 16.0f;
        const float v1 = lanternHangs(id) ? 7.0f : 3.0f;
        const glm::vec3 a[4] = { { 6.5f / 16.0f, h0, 0.5f }, { 9.5f / 16.0f, h0, 0.5f }, { 9.5f / 16.0f, h1, 0.5f }, { 6.5f / 16.0f, h1, 0.5f } };
        const glm::vec3 b[4] = { { 0.5f, h0, 6.5f / 16.0f }, { 0.5f, h0, 9.5f / 16.0f }, { 0.5f, h1, 9.5f / 16.0f }, { 0.5f, h1, 6.5f / 16.0f } };
        pushQuad(snap, lx, y, lz, id, a, t, 11.0f, 1.0f, 14.0f, v1, 0.9f, true, out);
        pushQuad(snap, lx, y, lz, id, b, t, 11.0f, 1.0f, 14.0f, v1, 0.9f, true, out);
    }

    void emitCampfire(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const bool lit = isCampfire(id);
        const bool soul = isSoulCampfire(id);
        const int log = Tiles::CampfireLog;
        const int ember = lit ? (soul ? Tiles::SoulCampfireLogLit : Tiles::CampfireLogLit) : Tiles::CampfireLog;
        const PartFace alongZ[6] = { { log, 0, 0, 16, 4 }, { log, 0, 0, 16, 4 }, { log, 0, 0, 16, 4, 1 },
                                     { log, 0, 0, 16, 4, 1 }, { log, 0, 4, 4, 8 }, { log, 0, 4, 4, 8 } };
        const PartFace alongX[6] = { { log, 0, 4, 4, 8 }, { log, 0, 4, 4, 8 }, { log, 0, 0, 16, 4 },
                                     { log, 0, 0, 16, 4 }, { log, 0, 0, 16, 4 }, { log, 0, 0, 16, 4 } };
        const PartFace bed[6] = { { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { ember, 0, 8, 16, 14, 1 },
                                  { -1, 0, 0, 16, 16 }, { ember, 0, 8, 6, 9 }, { ember, 0, 8, 6, 9 } };
        auto box = [&](float x0, float y0, float z0, float x1, float y1, float z1, const PartFace* f) {
            emitPart(snap, lx, y, lz, id, glm::vec3(x0, y0, z0) / 16.0f, glm::vec3(x1, y1, z1) / 16.0f, f, out);
        };
        box(1.0f, 0.0f, 0.0f, 5.0f, 4.0f, 16.0f, alongZ);
        box(11.0f, 0.0f, 0.0f, 15.0f, 4.0f, 16.0f, alongZ);
        box(0.0f, 3.0f, 1.0f, 16.0f, 7.0f, 5.0f, alongX);
        box(0.0f, 3.0f, 11.0f, 16.0f, 7.0f, 15.0f, alongX);
        box(5.0f, 0.0f, 0.0f, 11.0f, 1.0f, 16.0f, bed);
        if (!lit) return;
        const int fire = soul ? Tiles::SoulCampfireTop : Tiles::CampfireTop;
        const float lo = 1.0f / 16.0f;
        const glm::vec3 a[4] = { { 0.05f, lo, 0.05f }, { 0.95f, lo, 0.95f }, { 0.95f, 1.0f + lo, 0.95f }, { 0.05f, 1.0f + lo, 0.05f } };
        const glm::vec3 b[4] = { { 0.95f, lo, 0.05f }, { 0.05f, lo, 0.95f }, { 0.05f, 1.0f + lo, 0.95f }, { 0.95f, 1.0f + lo, 0.05f } };
        pushQuad(snap, lx, y, lz, id, a, fire, 0.0f, 0.0f, 16.0f, 16.0f, 1.0f, true, out);
        pushQuad(snap, lx, y, lz, id, b, fire, 0.0f, 0.0f, 16.0f, 16.0f, 1.0f, true, out);
    }

    void emitCactus(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const BlockInfo& info = blockInfo(id);
        const PartFace caps[6] = { { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { info.tileTop, 0, 0, 16, 16 },
                                   { info.tileBottom, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 } };
        emitPart(snap, lx, y, lz, id, glm::vec3(0.0f), glm::vec3(1.0f), caps, out);
        const PartFace eastWest[6] = { { info.tileSide, 0, 0, 16, 16 }, { info.tileSide, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 },
                                       { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 } };
        emitPart(snap, lx, y, lz, id, glm::vec3(1.0f / 16.0f, 0.0f, 0.0f), glm::vec3(15.0f / 16.0f, 1.0f, 1.0f), eastWest, out);
        const PartFace northSouth[6] = { { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 }, { -1, 0, 0, 16, 16 },
                                         { -1, 0, 0, 16, 16 }, { info.tileSide, 0, 0, 16, 16 }, { info.tileSide, 0, 0, 16, 16 } };
        emitPart(snap, lx, y, lz, id, glm::vec3(0.0f, 0.0f, 1.0f / 16.0f), glm::vec3(1.0f, 1.0f, 15.0f / 16.0f), northSouth, out);
    }

    void emitBamboo(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int stalk = Tiles::BambooStalkTile;
        const PartFace faces[6] = { { stalk, 0, 0, 3, 16 }, { stalk, 0, 0, 3, 16 }, { stalk, 13, 0, 16, 3 },
                                    { stalk, 13, 0, 16, 3 }, { stalk, 0, 0, 3, 16 }, { stalk, 0, 0, 3, 16 } };
        emitPart(snap, lx, y, lz, id, glm::vec3(6.5f / 16.0f, 0.0f, 6.5f / 16.0f), glm::vec3(9.5f / 16.0f, 1.0f, 9.5f / 16.0f), faces, out);
        int leaves = -1;
        if (snap.block(lx, y + 1, lz) != Blocks::Bamboo) leaves = blockInfo(id).tileSide;
        else if (snap.block(lx, y + 2, lz) != Blocks::Bamboo) leaves = Tiles::BambooSmallLeavesTile;
        if (leaves < 0) return;
        const glm::vec3 a[4] = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } };
        const glm::vec3 b[4] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } };
        pushQuad(snap, lx, y, lz, id, a, leaves, 0.0f, 0.0f, 16.0f, 16.0f, 0.9f, true, out);
        pushQuad(snap, lx, y, lz, id, b, leaves, 0.0f, 0.0f, 16.0f, 16.0f, 0.9f, true, out);
    }

    void emitHead(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const int kind = headKind(id);
        const bool wall = headOnWall(id);
        const int t = Tiles::HeadTileFirst + kind * 6;
        const int extra = Tiles::HeadTileFirst + HEAD_KINDS * 6;
        const PartFace faces[6] = { { t + 2, 0, 0, 16, 16, 0, true }, { t + 4, 0, 0, 16, 16 }, { t + 0, 0, 0, 16, 16 },
                                    { t + 1, 0, 0, 16, 16 }, { t + 5, 0, 0, 16, 16 }, { t + 3, 0, 0, 16, 16, 0, true } };
        const PartFace snout[6] = { { extra, 0, 0, 16, 16 }, { extra, 0, 0, 16, 16 }, { extra, 0, 0, 16, 16 },
                                    { extra, 0, 0, 16, 16 }, { extra, 0, 0, 16, 16 }, { extra, 0, 0, 16, 16 } };
        const PartFace lip[6] = { { extra + 2, 0, 0, 16, 16 }, { extra + 2, 0, 0, 16, 16 }, { extra + 1, 0, 0, 16, 16 },
                                  { extra + 2, 0, 0, 16, 16 }, { extra + 2, 0, 0, 16, 16 }, { extra + 2, 0, 0, 16, 16 } };
        const size_t start = out.size();
        auto box = [&](float x0, float y0, float z0, float x1, float y1, float z1, const PartFace* f) {
            emitPart(snap, lx, y, lz, id, glm::vec3(x0, y0, z0) / 16.0f, glm::vec3(x1, y1, z1) / 16.0f, f, out);
        };
        const float lift = wall ? 4.0f : 0.0f;
        if (kind == 5)
        {
            const float base = wall ? 2.0f : 0.0f;
            const float back = wall ? 16.0f : 15.0f;
            box(2.0f, base, back - 12.0f, 14.0f, base + 12.0f, back, faces);
            box(3.5f, base + 3.0f, back - 22.5f, 12.5f, base + 6.75f, back - 10.5f, lip);
        }
        else if (kind == 6)
        {
            const float back = wall ? 16.0f : 12.0f;
            box(3.0f, lift, back - 8.0f, 13.0f, lift + 8.0f, back, faces);
            box(6.0f, lift, back - 9.0f, 10.0f, lift + 4.0f, back - 8.0f, snout);
        }
        else
        {
            const float back = wall ? 16.0f : 12.0f;
            box(4.0f, lift, back - 8.0f, 12.0f, lift + 8.0f, back, faces);
        }
        float fx = 0.0f, fz = -1.0f;
        if (wall)
        {
            static const float AX[4] = { 0.0f, 1.0f, 0.0f, -1.0f }, AZ[4] = { -1.0f, 0.0f, 1.0f, 0.0f };
            fx = AX[headFacing(id)];
            fz = AZ[headFacing(id)];
        }
        else
        {
            const float a = glm::radians(static_cast<float>(headRotation(id)) * 22.5f);
            fx = -std::sin(a);
            fz = std::cos(a);
        }
        const float s = fx, c = -fz;
        for (size_t i = start; i + 10 <= out.size(); i += 10)
        {
            const float dx = out[i] - static_cast<float>(lx) - 0.5f, dz = out[i + 2] - static_cast<float>(lz) - 0.5f;
            out[i] = static_cast<float>(lx) + 0.5f + dx * c - dz * s;
            out[i + 2] = static_cast<float>(lz) + 0.5f + dx * s + dz * c;
        }
    }

    void emitButton(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const size_t start = out.size();
        const float depth = buttonPowered(id) ? 1.0f / 16.0f : 2.0f / 16.0f;
        emitBox(snap, lx, y, lz, id, glm::vec3(5.0f / 16.0f, 0.0f, 6.0f / 16.0f), glm::vec3(11.0f / 16.0f, depth, 10.0f / 16.0f), out);
        reorient(out, start, lx, y, lz, buttonFace(id), buttonFacing(id));
    }

    void emitPlate(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const float height = platePowered(id) ? 0.5f / 16.0f : 1.0f / 16.0f;
        emitBox(snap, lx, y, lz, id, glm::vec3(1.0f / 16.0f, 0.0f, 1.0f / 16.0f), glm::vec3(15.0f / 16.0f, height, 15.0f / 16.0f), out);
    }

    void emitWallTorch(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        static const int DX[4] = { 0, 1, 0, -1 };
        static const int DZ[4] = { -1, 0, 1, 0 };
        const int facing = wallTorchFacing(id);
        const glm::vec3 away(static_cast<float>(DX[facing]), 0.0f, static_cast<float>(DZ[facing]));
        const TileUV uv = tileUV(blockInfo(id).tileSide);
        const float sky = static_cast<float>(snap.skyLight(lx, y, lz)) / 15.0f;
        const float block = std::max(static_cast<float>(snap.blockLight(lx, y, lz)), static_cast<float>(lightEmission(id))) / 15.0f;
        const glm::vec3 base(static_cast<float>(lx), static_cast<float>(y), static_cast<float>(lz));
        auto place = [&](float px, float py, float pz) {
            const float lean = 0.3125f - 0.25f * py;
            return base + glm::vec3(px, py + 0.1875f, pz) - away * lean;
        };
        const float inset = 0.146f;
        const float quads[2][4] = { { inset, inset, 1.0f - inset, 1.0f - inset }, { inset, 1.0f - inset, 1.0f - inset, inset } };
        for (const auto& q : quads)
            for (int side = 0; side < 2; ++side)
            {
                const float ax = side == 0 ? q[0] : q[2], az = side == 0 ? q[1] : q[3];
                const float bx = side == 0 ? q[2] : q[0], bz = side == 0 ? q[3] : q[1];
                const glm::vec3 p0 = place(ax, 0.0f, az), p1 = place(bx, 0.0f, bz);
                const glm::vec3 p2 = place(bx, 1.0f, bz), p3 = place(ax, 1.0f, az);
                pushVertex(out, p0.x, p0.y, p0.z, uv.u0, uv.vBottom, 0.95f, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, p1.x, p1.y, p1.z, uv.u1, uv.vBottom, 0.95f, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, p2.x, p2.y, p2.z, uv.u1, uv.vTop, 0.95f, sky, block, tagOf(CROSS_FACE, id, true));
                pushVertex(out, p0.x, p0.y, p0.z, uv.u0, uv.vBottom, 0.95f, sky, block, tagOf(CROSS_FACE, id));
                pushVertex(out, p2.x, p2.y, p2.z, uv.u1, uv.vTop, 0.95f, sky, block, tagOf(CROSS_FACE, id, true));
                pushVertex(out, p3.x, p3.y, p3.z, uv.u0, uv.vTop, 0.95f, sky, block, tagOf(CROSS_FACE, id, true));
            }
    }

    void emitLever(const MeshSnapshot& snap, int lx, int y, int lz, BlockId id, std::vector<float>& out)
    {
        const size_t start = out.size();
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
                 HANDLE_UV, leverOn(id) ? 45.0f : -45.0f, glm::vec3(8.0f, 1.0f, 8.0f),
                 sky, block, id, true);
        if (isFamilyLever(id)) reorient(out, start, lx, y, lz, leverFace(id), leverFacing(id));
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
    out.signs.clear();
    out.opaqueVertices.reserve(8192);
    t_tint.grassBase = BiomeColours::fromMap(false, 0.8f, 0.4f);
    t_tint.foliageBase = BiomeColours::fromMap(true, 0.8f, 0.4f);
    t_tint.waterBase = 0x3F76E4u;

    for (int y = 0; y < Chunk::SY; ++y)
    {
        for (int lz = 0; lz < Chunk::SZ; ++lz)
        {
            for (int lx = 0; lx < Chunk::SX; ++lx)
            {
                const BlockId id = snapshot.block(lx, y, lz);
                const size_t column = static_cast<size_t>((lz + MeshSnapshot::PAD) * MeshSnapshot::PX + (lx + MeshSnapshot::PAD));
                t_tint.grass = snapshot.grass[column];
                t_tint.foliage = snapshot.foliage[column];
                t_tint.water = snapshot.water[column];
                if (isSign(id))
                {
                    out.signs.push_back(glm::ivec3(snapshot.pos.x * Chunk::SX + lx, y, snapshot.pos.z * Chunk::SZ + lz));
                    continue;
                }
                if (!isVisible(id)) continue;

                if (isLeverBlock(id))
                {
                    emitLever(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isHead(id))
                {
                    emitHead(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (id == Blocks::Bamboo)
                {
                    emitBamboo(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (id == Blocks::Cactus)
                {
                    emitCactus(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isAnvil(id) || id == Blocks::EnchantingTable || id == Blocks::EndPortalFrame ||
                         id == Blocks::EndPortalFrameFilled || isHopper(id) || isComposter(id) || id == Blocks::BrewingStand)
                {
                    emitWorkBlock(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isAnyCauldron(id))
                {
                    emitCauldron(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isLantern(id))
                {
                    emitLantern(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isAnyCampfire(id))
                {
                    emitCampfire(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isFamilyButton(id))
                {
                    emitButton(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isPlate(id))
                {
                    emitPlate(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isWallTorch(id))
                {
                    emitWallTorch(snapshot, lx, y, lz, id, out.opaqueVertices);
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
                else if (isTopSlab(id))
                {
                    emitBox(snapshot, lx, y, lz, id, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f), out.opaqueVertices);
                }
                else if (isFenceGate(id))
                {
                    emitGate(snapshot, lx, y, lz, id, out.opaqueVertices);
                }
                else if (isWall(id))
                {
                    emitWall(snapshot, lx, y, lz, id, out.opaqueVertices);
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
