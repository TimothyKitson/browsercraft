#include "Screenshot.h"
#include "Core/GLFunctions.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <vector>
#include <filesystem>
#include <ctime>
#include <cstdio>
#include <algorithm>

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

bool saveWorldIcon(int width, int height, const std::string& path)
{
    if (width <= 0 || height <= 0) return false;
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const int side = std::min(width, height);
    const int left = (width - side) / 2;
    const int bottom = (height - side) / 2;
    constexpr int SIZE = 64;
    std::vector<unsigned char> icon(static_cast<size_t>(SIZE) * SIZE * 3);
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
        {
            const int x0 = left + x * side / SIZE, x1 = std::max(x0 + 1, left + (x + 1) * side / SIZE);
            const int y0 = bottom + (SIZE - 1 - y) * side / SIZE, y1 = std::max(y0 + 1, bottom + (SIZE - y) * side / SIZE);
            unsigned long sum[3] = { 0, 0, 0 };
            unsigned long count = 0;
            for (int sy = y0; sy < y1; ++sy)
                for (int sx = x0; sx < x1; ++sx)
                {
                    const unsigned char* p = pixels.data() + (static_cast<size_t>(sy) * width + sx) * 4;
                    for (int c = 0; c < 3; ++c) sum[c] += p[c];
                    ++count;
                }
            for (int c = 0; c < 3; ++c)
                icon[(static_cast<size_t>(y) * SIZE + x) * 3 + c] = static_cast<unsigned char>(sum[c] / std::max(1ul, count));
        }
    return stbi_write_png(path.c_str(), SIZE, SIZE, 3, icon.data(), SIZE * 3) != 0;
}
