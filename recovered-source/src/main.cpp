#include "Core/Application.h"
#include <cstdio>
#include <stdexcept>

// Application sizes itself from the window (the canvas, on the web build),
// so there is nothing to parse here.
int main()
{
    try
    {
        Application app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "Fatal error: %s\n", e.what());
        return 1;
    }

    return 0;
}
