#include "Core/Application.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

// Usage: VoxelEngine [width height] [--autoshot <seconds>]
//   --autoshot waits for the world to load, saves a screenshot, then exits.
int main(int argc, char** argv)
{
    int width = 1280;
    int height = 720;
    float autoShotDelay = 0.0f;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--autoshot") == 0 && i + 1 < argc)
        {
            autoShotDelay = static_cast<float>(std::atof(argv[++i]));
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

    try
    {
        Application app(width, height);
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
