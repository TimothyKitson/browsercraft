#include "Maps.h"
#include <algorithm>
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

const char* Maps::markTexture(int mark)
{
    static const char* NAMES[MarkCount] = {
        "player", "frame", "red_marker", "blue_marker", "target_x", "target_point", "player_off_map", "player_off_limits",
        "woodland_mansion", "ocean_monument",
        "white_banner", "orange_banner", "magenta_banner", "light_blue_banner", "yellow_banner", "lime_banner", "pink_banner",
        "gray_banner", "light_gray_banner", "cyan_banner", "purple_banner", "blue_banner", "brown_banner", "green_banner",
        "red_banner", "black_banner",
        "red_x", "desert_village", "plains_village", "savanna_village", "snowy_village", "taiga_village",
        "jungle_temple", "swamp_hut", "trial_chambers",
    };
    return NAMES[mark >= 0 && mark < MarkCount ? mark : 0];
}

std::string Maps::markPath(int mark)
{
    return "../map/decorations/" + std::string(markTexture(mark));
}

const char* Maps::explorerName(int kind)
{
    switch (kind)
    {
        case OceanExplorer: return "Ocean Explorer Map";
        case WoodlandExplorer: return "Woodland Explorer Map";
        case TrialExplorer: return "Trial Explorer Map";
        default: return "Map";
    }
}

int Maps::explorerMark(int kind)
{
    switch (kind)
    {
        case OceanExplorer: return Monument;
        case WoodlandExplorer: return Mansion;
        case TrialExplorer: return TrialChambers;
        default: return TargetPoint;
    }
}

int Maps::rotationFor(float yaw)
{
    const float minecraft = yaw - 90.0f;
    const int step = static_cast<int>(std::floor((minecraft + 11.25f) / 22.5f));
    return ((step % 16) + 16) % 16;
}

Maps::Placed Maps::place(const Data& map, float worldX, float worldZ, int mark, int rotation)
{
    Placed out;
    const float step = static_cast<float>(1 << map.scale);
    out.x = (worldX - static_cast<float>(map.centreX)) / step + SIZE * 0.5f;
    out.z = (worldZ - static_cast<float>(map.centreZ)) / step + SIZE * 0.5f;
    out.mark = mark;
    out.rotation = rotation;
    out.shown = out.x >= 0.0f && out.x <= SIZE && out.z >= 0.0f && out.z <= SIZE;
    return out;
}

Maps::Placed Maps::placePlayer(const Data& map, float worldX, float worldZ, float yaw, bool sameDimension)
{
    Placed out = place(map, worldX, worldZ, Player, rotationFor(yaw));
    if (!sameDimension)
    {
        out.shown = false;
        return out;
    }
    if (out.shown) return out;
    const float fromX = out.x - SIZE * 0.5f, fromZ = out.z - SIZE * 0.5f;
    out.x = std::clamp(out.x, 0.0f, static_cast<float>(SIZE));
    out.z = std::clamp(out.z, 0.0f, static_cast<float>(SIZE));
    out.rotation = 0;
    if (std::fabs(fromX) < 320.0f && std::fabs(fromZ) < 320.0f)
    {
        out.mark = PlayerOffMap;
        out.shown = true;
    }
    else
    {
        out.mark = PlayerOffLimits;
        out.shown = map.explorer != NotExplorer;
    }
    return out;
}

bool Maps::toggleBanner(Data& map, int worldX, int worldZ, int colour)
{
    for (size_t i = 0; i < map.decorations.size(); ++i)
    {
        const Decoration& d = map.decorations[i];
        if (d.mark >= BannerFirst && d.mark <= BannerLast && d.worldX == worldX && d.worldZ == worldZ)
        {
            map.decorations.erase(map.decorations.begin() + static_cast<long>(i));
            return false;
        }
    }
    if (!place(map, static_cast<float>(worldX), static_cast<float>(worldZ), 0, 0).shown) return false;
    map.decorations.push_back(Decoration{ static_cast<uint8_t>(BannerFirst + (colour & 15)), worldX, worldZ, 8 });
    return true;
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
    const int32_t extra[2] = { data.explorer, static_cast<int32_t>(data.decorations.size()) };
    ok = ok && std::fwrite(extra, sizeof(extra), 1, file) == 1;
    for (const Decoration& d : data.decorations)
    {
        const int32_t row[4] = { d.mark, d.worldX, d.worldZ, d.rotation };
        ok = ok && std::fwrite(row, sizeof(row), 1, file) == 1;
    }
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
    data.explorer = 0;
    data.decorations.clear();
    int32_t extra[2] = {};
    if (ok && std::fread(extra, sizeof(extra), 1, file) == 1)
    {
        data.explorer = extra[0];
        for (int32_t i = 0; i < extra[1] && i < 4096; ++i)
        {
            int32_t row[4] = {};
            if (std::fread(row, sizeof(row), 1, file) != 1) break;
            data.decorations.push_back(Decoration{ static_cast<uint8_t>(row[0]), row[1], row[2], static_cast<uint8_t>(row[3]) });
        }
    }
    std::fclose(file);
    if (!ok) return false;
    data.centreX = header[0];
    data.centreZ = header[1];
    data.scale = header[2];
    data.dimension = header[3];
    data.locked = header[4] != 0;
    return true;
}
