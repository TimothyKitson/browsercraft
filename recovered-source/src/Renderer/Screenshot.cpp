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

    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL hands back rows bottom-up; PNG wants them top-down.
    const size_t rowBytes = static_cast<size_t>(width) * 3;
    std::vector<unsigned char> flipped(pixels.size());
    for (int y = 0; y < height; ++y)
    {
        const unsigned char* source = pixels.data() + static_cast<size_t>(height - 1 - y) * rowBytes;
        std::copy(source, source + rowBytes, flipped.begin() + static_cast<size_t>(y) * rowBytes);
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
