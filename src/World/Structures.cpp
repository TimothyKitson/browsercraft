#include "Structures.h"
#include "Noise.h"
#include <cmath>

int Structures::squareOf(int chunk, int spacing)
{
    return (chunk >= 0) ? (chunk / spacing)
                        : -(((-chunk) + spacing - 1) / spacing);
}

Structures::Spot Structures::inSquare(const Grid& grid, int squareX, int squareZ,
                                      uint32_t seed)
{
    // Somewhere in the first (spacing - separation) chunks of the
    // square. Everything after that is the gap that guarantees two
    // neighbours cannot touch.
    const int room = grid.spacing - grid.separation;

    const float alongX = hashToFloat(hashCoords(squareX, 1, squareZ, seed + grid.salt));
    const float alongZ = hashToFloat(hashCoords(squareX, 2, squareZ, seed + grid.salt));

    Spot spot;
    spot.exists = true;
    spot.chunkX = squareX * grid.spacing + static_cast<int>(alongX * static_cast<float>(room));
    spot.chunkZ = squareZ * grid.spacing + static_cast<int>(alongZ * static_cast<float>(room));
    return spot;
}

Structures::Spot Structures::nearest(const Grid& grid, int blockX, int blockZ,
                                     uint32_t seed, int squares)
{
    const int chunkX = (blockX >= 0) ? blockX / CHUNK : -(((-blockX) + CHUNK - 1) / CHUNK);
    const int chunkZ = (blockZ >= 0) ? blockZ / CHUNK : -(((-blockZ) + CHUNK - 1) / CHUNK);

    const int centreX = squareOf(chunkX, grid.spacing);
    const int centreZ = squareOf(chunkZ, grid.spacing);

    Spot best;
    long long bestDistance = 0;

    for (int sz = centreZ - squares; sz <= centreZ + squares; ++sz)
        for (int sx = centreX - squares; sx <= centreX + squares; ++sx)
        {
            const Spot spot = inSquare(grid, sx, sz, seed);
            const long long dx = static_cast<long long>(spot.chunkX) - chunkX;
            const long long dz = static_cast<long long>(spot.chunkZ) - chunkZ;
            const long long distance = dx * dx + dz * dz;

            if (!best.exists || distance < bestDistance)
            {
                best = spot;
                bestDistance = distance;
            }
        }

    return best;
}

bool Structures::spreadAt(int chunkX, int chunkZ, uint32_t seed, uint32_t salt, float chance)
{
    return hashToFloat(hashCoords(chunkX, 3, chunkZ, seed + salt)) < chance;
}

int Structures::strongholdCount()
{
    int total = 0;
    for (int ring = 0; ring < STRONGHOLD_RINGS; ++ring) total += STRONGHOLD_IN_RING[ring];
    return total;
}

void Structures::strongholdAt(int index, uint32_t seed, int& blockX, int& blockZ)
{
    int ring = 0;
    int within = index;
    while (ring < STRONGHOLD_RINGS - 1 && within >= STRONGHOLD_IN_RING[ring])
    {
        within -= STRONGHOLD_IN_RING[ring];
        ++ring;
    }

    const int count = STRONGHOLD_IN_RING[ring];
    const float radius = static_cast<float>(STRONGHOLD_RING_BLOCKS[ring]);

    // Evenly round the circle, turned by a seeded amount so two worlds
    // do not have their strongholds in the same compass directions,
    // and nudged so they are not exactly on the spokes.
    const float turn = hashToFloat(hashCoords(ring, 7, 0, seed + 55501)) * 6.2831853f;
    const float wobble =
        (hashToFloat(hashCoords(ring, 8, within, seed + 55502)) - 0.5f) * 0.45f;
    const float angle = turn + wobble +
                        (static_cast<float>(within) / static_cast<float>(count)) * 6.2831853f;

    const float spread =
        0.85f + hashToFloat(hashCoords(ring, 9, within, seed + 55503)) * 0.30f;

    blockX = static_cast<int>(std::cos(angle) * radius * spread);
    blockZ = static_cast<int>(std::sin(angle) * radius * spread);
}

void Structures::nearestStronghold(int blockX, int blockZ, uint32_t seed,
                                   int& outX, int& outZ)
{
    long long best = 0;
    bool found = false;
    outX = outZ = 0;

    for (int i = 0; i < strongholdCount(); ++i)
    {
        int x = 0, z = 0;
        strongholdAt(i, seed, x, z);

        const long long dx = static_cast<long long>(x) - blockX;
        const long long dz = static_cast<long long>(z) - blockZ;
        const long long distance = dx * dx + dz * dz;

        if (!found || distance < best)
        {
            best = distance;
            found = true;
            outX = x;
            outZ = z;
        }
    }
}
