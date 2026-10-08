#include "Core/Application.h"
#include "World/WorldGen.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

namespace
{
    // Dev tool: sample the height field over a wide area and report what
    // the generator is actually producing. Far more useful for tuning
    // terrain than squinting at screenshots, and needs no window.
    // Measures how clumpy the sea floor is. White noise gives runs of
    // about 1.5 blocks -- the salt-and-pepper look; real patches run ten
    // or more. Cheap way to prove the change did what it claims.
    int printSeabedInfo(uint32_t seed)
    {
        const WorldGen generator(seed);

        long long gravel = 0, sand = 0, other = 0;
        long long runs = 0, runBlocks = 0;

        for (int cz = -6; cz <= 6; ++cz)
            for (int cx = -6; cx <= 6; ++cx)
            {
                Chunk chunk(ChunkPos{ cx, cz });
                generator.generate(chunk);

                for (int z = 0; z < Chunk::SZ; ++z)
                {
                    bool inRun = false;
                    for (int x = 0; x < Chunk::SX; ++x)
                    {
                        // Topmost solid block of a column that is under water.
                        int surface = -1;
                        bool submerged = false;
                        for (int y = Chunk::SY - 1; y > 0; --y)
                        {
                            const BlockId b = chunk.getBlock(x, y, z);
                            if (b == Blocks::Water) submerged = true;
                            if (b != Blocks::Air && b != Blocks::Water) { surface = y; break; }
                        }
                        if (surface < 0 || !submerged) { inRun = false; continue; }

                        const BlockId top = chunk.getBlock(x, surface, z);
                        if (top == Blocks::Gravel) { ++gravel; ++runBlocks; if (!inRun) { ++runs; inRun = true; } }
                        else { if (top == Blocks::Sand) ++sand; else ++other; inRun = false; }
                    }
                }
            }

        const long long floorTotal = gravel + sand + other;
        std::printf("seabed sample: %lld blocks\n", floorTotal);
        if (floorTotal == 0) { std::printf("  no ocean floor in range\n"); return 0; }

        std::printf("  sand   %5.1f%%\n", 100.0 * sand / floorTotal);
        std::printf("  gravel %5.1f%%\n", 100.0 * gravel / floorTotal);
        std::printf("  other  %5.1f%%\n", 100.0 * other / floorTotal);
        std::printf("  mean gravel run: %.1f blocks (white noise would be ~1.5)\n",
                    runs ? static_cast<double>(runBlocks) / runs : 0.0);
        return 0;
    }

    // Scores seeds for use as the title-screen panorama: dramatic
    // terrain nearby, some water in view, and -- the hard requirement --
    // nothing within ten blocks of where the camera will sit.
    int scoutPanoramaSeeds(int count)
    {
        struct Candidate
        {
            uint32_t seed; int cx, cz, cameraY;
            float score, relief, water;
        };
        std::vector<Candidate> best;

        // A handful of spots per seed, so a good seed is not thrown away
        // because the origin happens to sit in the sea.
        const int OFFSETS[][2] = { { 0, 0 }, { 300, 120 }, { -260, 340 },
                                   { 520, -180 }, { -420, -500 } };

        for (int i = 0; i < count; ++i)
        {
            const uint32_t seed = 1000u + static_cast<uint32_t>(i) * 7919u;
            const WorldGen generator(seed);

            for (const auto& offset : OFFSETS)
            {
                const int cx = offset[0];
                const int cz = offset[1];

                int highest = 0;
                for (int dz = -16; dz <= 16; dz += 4)
                    for (int dx = -16; dx <= 16; dx += 4)
                        highest = std::max(highest, generator.surfaceHeight(cx + dx, cz + dz));
                const int cameraY = highest + 14;

                // Hard requirement: ten clear blocks around the camera,
                // with seven more allowed for a tree the height map does
                // not know about.
                bool clear = true;
                for (int dz = -10; dz <= 10 && clear; ++dz)
                    for (int dx = -10; dx <= 10 && clear; ++dx)
                    {
                        if (dx * dx + dz * dz > 100) continue;
                        if (generator.surfaceHeight(cx + dx, cz + dz) + 7 > cameraY - 10)
                            clear = false;
                    }
                if (!clear) continue;

                int minHeight = 1000, maxHeight = -1000, water = 0, samples = 0;
                for (int dz = -70; dz <= 70; dz += 3)
                    for (int dx = -70; dx <= 70; dx += 3)
                    {
                        const int h = generator.surfaceHeight(cx + dx, cz + dz);
                        minHeight = std::min(minHeight, h);
                        maxHeight = std::max(maxHeight, h);
                        if (h <= WorldGen::SEA_LEVEL) ++water;
                        ++samples;
                    }

                const float relief = static_cast<float>(maxHeight - minHeight);
                const float waterFraction = static_cast<float>(water) / samples;

                // Mostly land with a bit of coast. All ocean is a flat
                // blue rectangle and all land tends to be a flat green one.
                if (waterFraction > 0.35f) continue;
                const float coastBonus = (waterFraction > 0.04f) ? 14.0f : 0.0f;

                best.push_back({ seed, cx, cz, cameraY, relief + coastBonus,
                                 relief, waterFraction });
            }
        }

        std::sort(best.begin(), best.end(),
                  [](const Candidate& a, const Candidate& b) { return a.score > b.score; });

        std::printf("scouted %d seeds, %d viewpoints usable\n",
                    count, static_cast<int>(best.size()));
        for (size_t i = 0; i < best.size() && i < 10; ++i)
            std::printf("  seed %-10u at %5d,%-5d  camera Y %-4d relief %-4.0f water %3.0f%%\n",
                        best[i].seed, best[i].cx, best[i].cz, best[i].cameraY,
                        best[i].relief, best[i].water * 100.0f);
        return 0;
    }

    int printWorldInfo(uint32_t seed)
    {
        const WorldGen generator(seed);

        {
            // Where a new game actually starts, and whether that spot is
            // dry land -- spawning in the sea is the bug this catches.
            int sx = 0, sy = 0, sz = 0;
            generator.findSpawn(sx, sy, sz);
            const int surface = generator.surfaceHeight(sx, sz);
            std::printf("spawn: %d %d %d  surface %d  biome %s  %s\n",
                        sx, sy, sz, surface, biomeName(generator.biomeAt(sx, sz)),
                        surface > WorldGen::SEA_LEVEL ? "(dry land)" : "(UNDERWATER!)");
        }

        constexpr int RANGE = 1200;
        constexpr int STEP = 6;

        std::map<std::string, int> biomeCounts;
        std::map<int, int> heightBands; // 10-block bands
        int samples = 0;
        int minHeight = 1000, maxHeight = -1000;
        long long heightSum = 0;
        int aboveSea = 0;

        for (int x = -RANGE; x <= RANGE; x += STEP)
        {
            for (int z = -RANGE; z <= RANGE; z += STEP)
            {
                const int height = generator.surfaceHeight(x, z);
                biomeCounts[biomeName(generator.biomeAt(x, z))]++;
                heightBands[(height / 10) * 10]++;
                minHeight = std::min(minHeight, height);
                maxHeight = std::max(maxHeight, height);
                heightSum += height;
                if (height > WorldGen::SEA_LEVEL) ++aboveSea;
                ++samples;
            }
        }

        std::printf("seed %u, %d samples over %d x %d blocks\n", seed, samples, RANGE * 2, RANGE * 2);
        std::printf("height  min %d  max %d  mean %.1f  (sea level %d)\n",
                    minHeight, maxHeight, static_cast<double>(heightSum) / samples, WorldGen::SEA_LEVEL);
        std::printf("land    %.1f%% above sea level\n", 100.0 * aboveSea / samples);

        std::printf("\nheight distribution:\n");
        for (const auto& [band, count] : heightBands)
        {
            const int bar = count * 60 / samples;
            std::printf("  %3d-%3d %5.1f%% %s\n", band, band + 9, 100.0 * count / samples,
                        std::string(bar, '#').c_str());
        }

        std::printf("\nbiomes:\n");
        for (const auto& [name, count] : biomeCounts)
            std::printf("  %-10s %5.1f%%\n", name.c_str(), 100.0 * count / samples);

        // What range do the raw noise fields actually cover? fBm averages
        // octaves together, so it stays far inside [-1,1] and terrain
        // amplitudes have to be calibrated against the real spread.
        const Noise probe(seed + 1);
        double absSum = 0.0, ridgeSum = 0.0;
        float lo = 1.0f, hi = -1.0f, ridgeLo = 1.0f, ridgeHi = 0.0f;
        int n = 0;
        for (int x = -600; x <= 600; x += 3)
            for (int z = -600; z <= 600; z += 3)
            {
                const float v = probe.fbm2D(x * 0.00085f, z * 0.00085f, 5);
                const float r = probe.ridged2D(x * 0.004f, z * 0.004f, 5);
                absSum += std::fabs(v);
                ridgeSum += r;
                lo = std::min(lo, v); hi = std::max(hi, v);
                ridgeLo = std::min(ridgeLo, r); ridgeHi = std::max(ridgeHi, r);
                ++n;
            }
        std::printf("\nraw noise: fbm5 range [%.3f, %.3f] mean|v| %.3f\n", lo, hi, absSum / n);
        std::printf("           ridged5 range [%.3f, %.3f] mean %.3f\n", ridgeLo, ridgeHi, ridgeSum / n);

        return 0;
    }
}

int runSelfTest();   // SelfTest.cpp: headless inventory and crafting checks

// Usage: VoxelEngine [width height] [--autoshot <seconds>] [--nopbr]
//   --autoshot waits for the world to load, saves a screenshot, then exits.
//   --nopbr    starts with resource-pack PBR shading off (toggle in-game with P).
int main(int argc, char** argv)
{
    int width = 1280;
    int height = 720;
    float autoShotDelay = 0.0f;
    bool pbr = true;
    const char* startupScreen = nullptr;
    uint32_t seed = 0;
    bool hasSeed = false;
    bool worldInfo = false;
    bool selfTest = false;
    bool seabedInfo = false;
    int scoutSeeds = 0;
    const char* panorama = nullptr;
    const char* dimensionName = nullptr;
    bool hostOnStart = false;
    bool buildTest = false;
    const char* joinAddress = nullptr;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--autoshot") == 0 && i + 1 < argc)
        {
            autoShotDelay = static_cast<float>(std::atof(argv[++i]));
        }
        else if (std::strcmp(argv[i], "--nopbr") == 0)
        {
            pbr = false;
        }
        else if (std::strcmp(argv[i], "--screen") == 0 && i + 1 < argc)
        {
            startupScreen = argv[++i];
        }
        else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
        {
            seed = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            hasSeed = true;
        }
        else if (std::strcmp(argv[i], "--worldinfo") == 0)
        {
            worldInfo = true;
        }
        else if (std::strcmp(argv[i], "--dim") == 0 && i + 1 < argc)
        {
            dimensionName = argv[++i];
        }
        else if (std::strcmp(argv[i], "--panorama") == 0 && i + 1 < argc)
        {
            panorama = argv[++i];   // seed,x,y,z
        }
        else if (std::strcmp(argv[i], "--scout") == 0 && i + 1 < argc)
        {
            scoutSeeds = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--seabedinfo") == 0)
        {
            seabedInfo = true;
        }
        else if (std::strcmp(argv[i], "--selftest") == 0)
        {
            selfTest = true;
        }
        else if (std::strcmp(argv[i], "--buildtest") == 0)
        {
            buildTest = true;
        }
        else if (std::strcmp(argv[i], "--host") == 0)
        {
            hostOnStart = true;
        }
        else if (std::strcmp(argv[i], "--join") == 0 && i + 1 < argc)
        {
            joinAddress = argv[++i];
        }
        else if (argv[i][0] != '-' && i + 1 < argc && argv[i + 1][0] != '-')
        {
            const int requestedWidth = std::atoi(argv[i]);
            const int requestedHeight = std::atoi(argv[i + 1]);
            if (requestedWidth >= 320 && requestedHeight >= 240)
            {
                width = requestedWidth;
                height = requestedHeight;
                ++i;
            }
        }
    }

    if (selfTest) return runSelfTest();
    if (worldInfo) return printWorldInfo(hasSeed ? seed : 12345u);
    if (seabedInfo) return printSeabedInfo(hasSeed ? seed : 12345u);
    if (scoutSeeds > 0) return scoutPanoramaSeeds(scoutSeeds);

    try
    {
        Application app(width, height);
        app.setPbrEnabled(pbr);
        if (startupScreen) app.setStartupScreen(startupScreen);
        if (hasSeed) app.setSeedOverride(seed);
        if (buildTest) app.setBuildTest();
        if (panorama)
        {
            unsigned int panoramaSeed = 0;
            float px = 0.0f, py = 0.0f, pz = 0.0f;
            if (std::sscanf(panorama, "%u,%f,%f,%f", &panoramaSeed, &px, &py, &pz) == 4)
                app.setPanoramaOverride(panoramaSeed, px, py, pz);
        }
        if (dimensionName)
        {
            if (std::strcmp(dimensionName, "nether") == 0)
                app.setStartupDimension(Dimension::Nether);
            else if (std::strcmp(dimensionName, "end") == 0)
                app.setStartupDimension(Dimension::End);
        }
        if (hostOnStart) app.setHostOnStart();
        else if (joinAddress) app.setJoinOnStart(joinAddress);
        if (autoShotDelay > 0.0f) app.setAutoScreenshot(autoShotDelay);
        app.run();
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "Fatal error: %s\n", e.what());
        return 1;
    }

    return 0;
}
