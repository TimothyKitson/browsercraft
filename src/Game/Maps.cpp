#include "Maps.h"
#include <cmath>
#include <cstdio>
#include <filesystem>

int Maps::span(int scale)
{
    return SIZE << (scale < 0 ? 0 : scale > MAX_SCALE ? MAX_SCALE : scale);
}

void Maps::centreFor(int x, int z, int scale, int& centreX, int& centreZ)
{
    const int width = span(scale);
    centreX = static_cast<int>(std::floor((x + 64.0) / width)) * width + width / 2 - 64;
    centreZ = static_cast<int>(std::floor((z + 64.0) / width)) * width + width / 2 - 64;
}

unsigned int Maps::shaded(unsigned int colour, int step)
{
    static const int FACTOR[3] = { 180, 220, 255 };
    const int f = FACTOR[step < 0 ? 0 : step > 2 ? 2 : step];
    const unsigned int r = ((colour >> 16) & 255u) * static_cast<unsigned int>(f) / 255u;
    const unsigned int g = ((colour >> 8) & 255u) * static_cast<unsigned int>(f) / 255u;
    const unsigned int b = (colour & 255u) * static_cast<unsigned int>(f) / 255u;
    return (r << 16) | (g << 8) | b;
}

bool Maps::save(const std::string& directory, int id, const Data& data)
{
    std::error_code ec;
    std::filesystem::create_directories(directory + "/maps", ec);
    const std::string path = directory + "/maps/map_" + std::to_string(id) + ".bin";
    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const int32_t header[5] = { data.centreX, data.centreZ, data.scale, data.dimension, data.locked ? 1 : 0 };
    bool ok = std::fwrite(header, sizeof(header), 1, file) == 1;
    ok = ok && std::fwrite(data.rgba.data(), 1, data.rgba.size(), file) == data.rgba.size();
    std::fclose(file);
    return ok;
}

bool Maps::load(const std::string& directory, int id, Data& data)
{
    const std::string path = directory + "/maps/map_" + std::to_string(id) + ".bin";
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    int32_t header[5] = {};
    bool ok = std::fread(header, sizeof(header), 1, file) == 1;
    data.rgba.assign(SIZE * SIZE * 4, 0);
    ok = ok && std::fread(data.rgba.data(), 1, data.rgba.size(), file) == data.rgba.size();
    std::fclose(file);
    if (!ok) return false;
    data.centreX = header[0];
    data.centreZ = header[1];
    data.scale = header[2];
    data.dimension = header[3];
    data.locked = header[4] != 0;
    return true;
}
