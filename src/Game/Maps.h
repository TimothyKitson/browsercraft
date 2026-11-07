#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Maps
{
    constexpr int SIZE = 128;
    constexpr int MAX_SCALE = 4;

    struct Data
    {
        int centreX = 0;
        int centreZ = 0;
        int scale = 0;
        int dimension = 0;
        bool locked = false;
        std::vector<uint8_t> rgba = std::vector<uint8_t>(SIZE * SIZE * 4, 0);
    };

    int span(int scale);
    void centreFor(int x, int z, int scale, int& centreX, int& centreZ);
    unsigned int shaded(unsigned int colour, int step);

    bool save(const std::string& directory, int id, const Data& data);
    bool load(const std::string& directory, int id, Data& data);
}
