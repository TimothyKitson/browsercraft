#pragma once
#include "Items.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Banners
{
    constexpr int MAX_LAYERS = 6;
    constexpr int CLOTH_W = 20;
    constexpr int CLOTH_H = 40;
    constexpr int SHEET = 64;

    enum Pattern : uint8_t
    {
        Base,
        SquareBottomLeft, SquareBottomRight, SquareTopLeft, SquareTopRight,
        StripeBottom, StripeTop, StripeLeft, StripeRight, StripeCenter, StripeMiddle,
        StripeDownright, StripeDownleft, SmallStripes, Cross, StraightCross,
        TriangleBottom, TriangleTop, TrianglesBottom, TrianglesTop,
        DiagonalLeft, DiagonalUpRight, DiagonalUpLeft, DiagonalRight,
        Circle, Rhombus, HalfVertical, HalfHorizontal, HalfVerticalRight, HalfHorizontalBottom,
        Border, CurlyBorder, Gradient, GradientUp, Bricks,
        Globe, Creeper, Skull, Flower, Piglin, Flow, Guster, Mojang,
        PatternCount
    };

    constexpr int FREE_FIRST = SquareBottomLeft;
    constexpr int FREE_LAST = Bricks;

    struct Layer
    {
        uint8_t pattern = 0;
        uint8_t colour = 0;
        bool operator==(const Layer& other) const { return pattern == other.pattern && colour == other.colour; }
    };
    using Design = std::vector<Layer>;

    const char* textureName(int pattern);
    std::string bannerTexture(int pattern);
    std::string shieldTexture(int pattern);
    const char* patternName(int pattern);
    const char* colourName(int colour);
    unsigned int dyeRgb(int colour);

    int patternOfItem(StackId item);
    StackId itemOfPattern(int pattern);
    bool isPatternItem(StackId item);
    std::vector<int> loomChoices(StackId patternItem);
    bool addLayer(const Design& current, int pattern, int colour, Design& out);

    float coverage(int pattern, int x, int y);
    Design ominous();

    std::string key(int baseColour, const Design& design);

    class Library
    {
    public:
        int idOf(const Design& design);
        const Design* get(int id) const;
        Design design(int id) const;
        int size() const { return static_cast<int>(m_designs.size()); }
        void clear() { m_designs.clear(); }
        const std::vector<Design>& all() const { return m_designs; }
        void add(const Design& design) { m_designs.push_back(design); }

    private:
        std::vector<Design> m_designs;
    };

    struct Sheet
    {
        int width = 0;
        int height = 0;
        std::vector<uint8_t> rgba;
        bool loaded() const { return !rgba.empty(); }
    };

    struct Sheets
    {
        Sheet frame;
        std::vector<Sheet> patterns = std::vector<Sheet>(PatternCount);
    };

    void compose(int baseColour, const Design& design, const Sheets& sheets, std::vector<uint8_t>& out);

    void setActive(Library* library);
    Library* active();
}
