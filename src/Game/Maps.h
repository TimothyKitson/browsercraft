#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Maps
{
    constexpr int SIZE = 128;
    constexpr int MAX_SCALE = 4;

    enum Mark : uint8_t
    {
        Player, Frame, RedMarker, BlueMarker, TargetX, TargetPoint, PlayerOffMap, PlayerOffLimits,
        Mansion, Monument, BannerFirst, BannerLast = BannerFirst + 15, RedX,
        DesertVillage, PlainsVillage, SavannaVillage, SnowyVillage, TaigaVillage,
        JungleTemple, SwampHut, TrialChambers, MarkCount
    };
    const char* markTexture(int mark);
    std::string markPath(int mark);

    enum Explorer : uint8_t { NotExplorer, OceanExplorer, WoodlandExplorer, TrialExplorer, TreasureExplorer, ExplorerCount };
    const char* explorerName(int kind);
    int explorerMark(int kind);

    struct Decoration
    {
        uint8_t mark = 0;
        int worldX = 0;
        int worldZ = 0;
        uint8_t rotation = 8;
    };

    struct Data
    {
        int centreX = 0;
        int centreZ = 0;
        int scale = 0;
        int dimension = 0;
        bool locked = false;
        int explorer = 0;
        std::vector<uint8_t> rgba = std::vector<uint8_t>(SIZE * SIZE * 4, 0);
        std::vector<Decoration> decorations;
    };

    struct Placed
    {
        float x = 0.0f;
        float z = 0.0f;
        int mark = 0;
        int rotation = 0;
        bool shown = false;
    };
    Placed place(const Data& map, float worldX, float worldZ, int mark, int rotation);
    Placed placePlayer(const Data& map, float worldX, float worldZ, float yaw, bool sameDimension);
    int rotationFor(float yaw);
    bool toggleBanner(Data& map, int worldX, int worldZ, int colour);

    int span(int scale);
    void centreFor(int x, int z, int scale, int& centreX, int& centreZ);
    unsigned int shaded(unsigned int colour, int step);

    bool save(const std::string& directory, int id, const Data& data);
    bool load(const std::string& directory, int id, Data& data);
}
