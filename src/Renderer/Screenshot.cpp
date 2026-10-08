#include "Screenshot.h"
#include "Core/GLFunctions.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <vector>
#include <filesystem>
#include <ctime>
#include <cstdio>

std::string saveScreenshot(int width, int height, const std::string& directory)
{
    if (width <= 0 || height <= 0) return {};

    std::error_code ec;
    std::filesystem::create_directories(directory, ec);

    // RGBA rather than RGB: WebGL2 only guarantees RGBA readback.
    constexpr int CHANNELS = 4;
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * CHANNELS);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Drop the alpha channel. Alpha-tested terrain writes the texture's own
    // alpha into the framebuffer, which the screen ignores but a PNG would
    // honour -- leaves and grass would come out semi-transparent.
    const size_t rowBytes = static_cast<size_t>(width) * 3;
    std::vector<unsigned char> flipped(rowBytes * height);
    for (int y = 0; y < height; ++y)
    {
        // OpenGL hands back rows bottom-up; PNG wants them top-down.
        const unsigned char* source = pixels.data() + static_cast<size_t>(height - 1 - y) * width * CHANNELS;
        unsigned char* destination = flipped.data() + static_cast<size_t>(y) * rowBytes;
        for (int x = 0; x < width; ++x)
        {
            destination[x * 3 + 0] = source[x * CHANNELS + 0];
            destination[x * 3 + 1] = source[x * CHANNELS + 1];
            destination[x * 3 + 2] = source[x * CHANNELS + 2];
        }
    }

    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    char name[64];
    std::strftime(name, sizeof(name), "%Y-%m-%d_%H-%M-%S.png", &localTime);
    const std::string path = directory + "/" + name;

    if (!stbi_write_png(path.c_str(), width, height, 3, flipped.data(), static_cast<int>(rowBytes)))
        return {};

    return path;
}
