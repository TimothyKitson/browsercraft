#include "Banners.h"
#include <algorithm>
#include <cmath>

namespace
{
    struct Info
    {
        const char* texture;
        const char* name;
    };

    const Info INFO[Banners::PatternCount] = {
        { "base", "Field" },
        { "square_bottom_left", "Base Dexter Canton" },
        { "square_bottom_right", "Base Sinister Canton" },
        { "square_top_left", "Chief Dexter Canton" },
        { "square_top_right", "Chief Sinister Canton" },
        { "stripe_bottom", "Base" },
        { "stripe_top", "Chief" },
        { "stripe_left", "Pale Dexter" },
        { "stripe_right", "Pale Sinister" },
        { "stripe_center", "Pale" },
        { "stripe_middle", "Fess" },
        { "stripe_downright", "Bend" },
        { "stripe_downleft", "Bend Sinister" },
        { "small_stripes", "Paly" },
        { "cross", "Saltire" },
        { "straight_cross", "Cross" },
        { "triangle_bottom", "Chevron" },
        { "triangle_top", "Inverted Chevron" },
        { "triangles_bottom", "Base Indented" },
        { "triangles_top", "Chief Indented" },
        { "diagonal_left", "Per Bend Sinister" },
        { "diagonal_up_right", "Per Bend Inverted" },
        { "diagonal_up_left", "Per Bend Sinister Inverted" },
        { "diagonal_right", "Per Bend" },
        { "circle", "Roundel" },
        { "rhombus", "Lozenge" },
        { "half_vertical", "Per Pale" },
        { "half_horizontal", "Per Fess" },
        { "half_vertical_right", "Per Pale Inverted" },
        { "half_horizontal_bottom", "Per Fess Inverted" },
        { "border", "Bordure" },
        { "curly_border", "Bordure Indented" },
        { "gradient", "Gradient" },
        { "gradient_up", "Base Gradient" },
        { "bricks", "Field Masoned" },
        { "globe", "Globe" },
        { "creeper", "Creeper Charge" },
        { "skull", "Skull Charge" },
        { "flower", "Flower Charge" },
        { "piglin", "Snout" },
        { "flow", "Flow" },
        { "guster", "Guster" },
    };

    std::string packPath(bool shield, int pattern)
    {
        const char* name = pattern >= 0 && pattern < Banners::PatternCount ? INFO[pattern].texture : "base";
        if (shield) return "assets/textures/entity/shield/" + std::string(name) + ".png";
        return "assets/textures/entity/banner/" + std::string(name) + ".png";
    }

    Banners::Library* g_active = nullptr;

    const char* COLOUR_NAMES[16] = {
        "White", "Orange", "Magenta", "Light Blue", "Yellow", "Lime", "Pink", "Gray",
        "Light Gray", "Cyan", "Purple", "Blue", "Brown", "Green", "Red", "Black",
    };

    const unsigned int DYE_RGB[16] = {
        0xF9FFFEu, 0xF9801Du, 0xC74EBDu, 0x3AB3DAu, 0xFED83Du, 0x80C71Fu, 0xF38BAAu, 0x474F52u,
        0x9D9D97u, 0x169C9Cu, 0x8932B8u, 0x3C44AAu, 0x835432u, 0x5E7C16u, 0xB02E26u, 0x1D1D21u,
    };

    struct ItemPattern
    {
        StackId item;
        int pattern;
    };

    const ItemPattern ITEM_PATTERNS[] = {
        { Items::FlowerBannerPattern, Banners::Flower },
        { Items::SkullBannerPattern, Banners::Skull },
        { Items::GlobeBannerPattern, Banners::Globe },
        { Items::PiglinBannerPattern, Banners::Piglin },
        { Items::FlowBannerPattern, Banners::Flow },
        { Items::GusterBannerPattern, Banners::Guster },
    };

    float ring(float d, float inner, float outer) { return d >= inner && d < outer ? 1.0f : 0.0f; }

    float clothNoise(int x, int y)
    {
        uint32_t h = static_cast<uint32_t>(x * 73856093) ^ static_cast<uint32_t>(y * 19349663);
        h ^= h >> 13;
        h *= 0x5bd1e995u;
        h ^= h >> 15;
        return static_cast<float>(h & 255u) / 255.0f;
    }

    float sheetCoverage(int pattern, int sx, int sy, float& light)
    {
        light = 1.0f;
        int x = -1, y = -1;
        if (sy >= 1 && sy < 41)
        {
            if (sx >= 1 && sx < 21) { x = sx - 1; y = sy - 1; }
            else if (sx >= 22 && sx < 42) { x = 19 - (sx - 22); y = sy - 1; }
            else if (sx == 0) { x = 0; y = sy - 1; light = 0.85f; }
            else if (sx == 21) { x = 19; y = sy - 1; light = 0.85f; }
        }
        else if (sy == 0)
        {
            if (sx >= 1 && sx < 21) { x = sx - 1; y = 0; light = 0.9f; }
            else if (sx >= 21 && sx < 41) { x = sx - 21; y = 39; light = 0.75f; }
        }
        if (x < 0) return 0.0f;
        if (pattern == Banners::Base) light *= 0.9f + 0.08f * clothNoise(x, y);
        return Banners::coverage(pattern, x, y);
    }

    void frameFallback(int sx, int sy, uint8_t* px)
    {
        const bool pole = sx >= 44 && sx < 52 && sy < 44;
        const bool bar = sy >= 42 && sy < 46 && sx < 44;
        const bool cloth = sy < 41 && sx < 42;
        if (pole || bar)
        {
            const float n = clothNoise(sx * 3, sy);
            const int grain = (sx + sy / 3) % 3 == 0 ? -10 : 0;
            px[0] = static_cast<uint8_t>(std::clamp(112 + grain + static_cast<int>(n * 14.0f), 0, 255));
            px[1] = static_cast<uint8_t>(std::clamp(88 + grain + static_cast<int>(n * 10.0f), 0, 255));
            px[2] = static_cast<uint8_t>(std::clamp(54 + grain + static_cast<int>(n * 8.0f), 0, 255));
            px[3] = 255;
        }
        else if (cloth)
        {
            px[0] = px[1] = px[2] = 226;
            px[3] = 255;
        }
        else
        {
            px[0] = px[1] = px[2] = px[3] = 0;
        }
    }
}

std::string Banners::bannerTexture(int pattern)
{
    return packPath(false, pattern);
}

std::string Banners::shieldTexture(int pattern)
{
    return packPath(true, pattern);
}

void Banners::setActive(Library* library)
{
    g_active = library;
}

Banners::Library* Banners::active()
{
    return g_active;
}

const char* Banners::textureName(int pattern)
{
    return pattern >= 0 && pattern < PatternCount ? INFO[pattern].texture : "base";
}

const char* Banners::patternName(int pattern)
{
    return pattern >= 0 && pattern < PatternCount ? INFO[pattern].name : "";
}

const char* Banners::colourName(int colour)
{
    return COLOUR_NAMES[std::clamp(colour, 0, 15)];
}

unsigned int Banners::dyeRgb(int colour)
{
    return DYE_RGB[std::clamp(colour, 0, 15)];
}

int Banners::patternOfItem(StackId item)
{
    for (const ItemPattern& entry : ITEM_PATTERNS)
        if (entry.item == item) return entry.pattern;
    return -1;
}

StackId Banners::itemOfPattern(int pattern)
{
    for (const ItemPattern& entry : ITEM_PATTERNS)
        if (entry.pattern == pattern) return entry.item;
    return Blocks::Air;
}

bool Banners::isPatternItem(StackId item) { return patternOfItem(item) >= 0; }

std::vector<int> Banners::loomChoices(StackId patternItem)
{
    std::vector<int> out;
    const int special = patternOfItem(patternItem);
    if (special >= 0)
    {
        out.push_back(special);
        return out;
    }
    for (int p = FREE_FIRST; p <= FREE_LAST; ++p) out.push_back(p);
    return out;
}

bool Banners::addLayer(const Design& current, int pattern, int colour, Design& out)
{
    if (static_cast<int>(current.size()) >= MAX_LAYERS) return false;
    if (pattern <= Base || pattern >= PatternCount || colour < 0 || colour > 15) return false;
    out = current;
    out.push_back(Layer{ static_cast<uint8_t>(pattern), static_cast<uint8_t>(colour) });
    return true;
}

float Banners::coverage(int pattern, int x, int y)
{
    if (x < 0 || x >= CLOTH_W || y < 0 || y >= CLOTH_H) return 0.0f;
    const float u = (static_cast<float>(x) + 0.5f) / CLOTH_W;
    const float v = (static_cast<float>(y) + 0.5f) / CLOTH_H;
    const float cx = static_cast<float>(x) - 9.5f;
    const float cy = static_cast<float>(y) - 19.5f;
    const float d = std::sqrt(cx * cx + cy * cy);
    auto is = [](bool b) { return b ? 1.0f : 0.0f; };

    switch (pattern)
    {
        case Base: return 1.0f;
        case SquareBottomLeft: return is(x < 10 && y >= 26);
        case SquareBottomRight: return is(x >= 10 && y >= 26);
        case SquareTopLeft: return is(x < 10 && y < 14);
        case SquareTopRight: return is(x >= 10 && y < 14);
        case StripeBottom: return is(y >= 27);
        case StripeTop: return is(y < 13);
        case StripeLeft: return is(x < 7);
        case StripeRight: return is(x >= 13);
        case StripeCenter: return is(x >= 7 && x < 13);
        case StripeMiddle: return is(y >= 17 && y < 23);
        case StripeDownright: return is(std::fabs(u - v) < 0.17f);
        case StripeDownleft: return is(std::fabs((1.0f - u) - v) < 0.17f);
        case SmallStripes: return is(x % 5 >= 1 && x % 5 <= 3);
        case Cross: return is(std::fabs(u - v) < 0.12f || std::fabs((1.0f - u) - v) < 0.12f);
        case StraightCross: return is((x >= 8 && x < 12) || (y >= 17 && y < 22));
        case TriangleBottom: return is(static_cast<float>(39 - y) < 18.0f - std::fabs(cx) * 1.8f);
        case TriangleTop: return is(static_cast<float>(y) < 18.0f - std::fabs(cx) * 1.8f);
        case TrianglesBottom:
        {
            const float t = std::fabs(static_cast<float>((x + 2) % 5) - 2.0f);
            return is(static_cast<float>(39 - y) < 5.0f - t * 1.5f);
        }
        case TrianglesTop:
        {
            const float t = std::fabs(static_cast<float>((x + 2) % 5) - 2.0f);
            return is(static_cast<float>(y) < 5.0f - t * 1.5f);
        }
        case DiagonalLeft: return is(u + v < 1.0f);
        case DiagonalUpRight: return is(u + v >= 1.0f);
        case DiagonalUpLeft: return is(v >= u);
        case DiagonalRight: return is(v < u);
        case Circle: return is(d < 5.2f);
        case Rhombus: return is(std::fabs(cx) / 7.0f + std::fabs(cy) / 11.0f < 1.0f);
        case HalfVertical: return is(x < 10);
        case HalfHorizontal: return is(y < 20);
        case HalfVerticalRight: return is(x >= 10);
        case HalfHorizontalBottom: return is(y >= 20);
        case Border: return is(x < 2 || x >= 18 || y < 2 || y >= 38);
        case CurlyBorder:
        {
            const int edge = std::min(std::min(x, 19 - x), std::min(y, 39 - y));
            if (edge < 1) return 1.0f;
            if (edge == 1) return is(((x + y) & 1) == 0);
            if (edge == 2) return is(((x + y) & 3) == 0);
            return 0.0f;
        }
        case Gradient: return std::clamp(1.35f - v * 1.7f, 0.0f, 1.0f);
        case GradientUp: return std::clamp(v * 1.7f - 0.35f, 0.0f, 1.0f);
        case Bricks:
        {
            if (y % 5 == 4) return 0.0f;
            const int offset = (y / 5) % 2 == 0 ? 0 : 3;
            return is((x + offset) % 6 != 5);
        }
        case Globe:
        {
            if (d >= 6.5f) return 0.0f;
            if (d >= 5.5f) return 1.0f;
            const float land = std::sin(cx * 0.9f) + std::cos(cy * 0.7f + cx * 0.3f);
            return is(land > 0.6f || std::fabs(cy) < 0.6f);
        }
        case Creeper:
        {
            const bool eyes = (y >= 12 && y < 16) && ((x >= 5 && x < 9) || (x >= 11 && x < 15));
            const bool nose = (y >= 16 && y < 20) && (x >= 8 && x < 12);
            const bool jaw = (y >= 18 && y < 23) && ((x >= 6 && x < 8) || (x >= 12 && x < 14));
            return is(eyes || nose || jaw);
        }
        case Skull:
        {
            const float hx = cx, hy = static_cast<float>(y) - 16.0f;
            const bool head = (hx * hx) / 30.0f + (hy * hy) / 26.0f < 1.0f;
            const bool socket = (std::fabs(hx + 2.2f) < 1.3f || std::fabs(hx - 2.2f) < 1.3f) && std::fabs(hy) < 1.3f;
            const bool jaw = y >= 20 && y < 23 && std::fabs(hx) < 3.5f && (x % 2 == 0 || y == 20);
            const float by = static_cast<float>(y) - 28.0f;
            const bool bones = std::fabs(by) < 4.5f && (std::fabs(cx - by * 1.4f) < 1.1f || std::fabs(cx + by * 1.4f) < 1.1f);
            return is(((head && !socket) || jaw) || bones);
        }
        case Flower:
        {
            if (d < 1.6f) return 1.0f;
            if (d < 2.6f) return 0.0f;
            const float angle = std::atan2(cy, cx);
            return is(d < 6.5f && std::cos(angle * 8.0f) > -0.15f);
        }
        case Piglin:
        {
            const bool snout = std::fabs(cx) < 5.0f && std::fabs(cy) < 3.5f && !(std::fabs(cx) > 4.0f && std::fabs(cy) > 2.5f);
            const bool nostril = std::fabs(cy) < 1.2f && (std::fabs(cx + 2.0f) < 1.0f || std::fabs(cx - 2.0f) < 1.0f);
            return is(snout && !nostril);
        }
        case Flow:
        {
            if (d > 8.0f) return 0.0f;
            const float angle = std::atan2(cy, cx) + 3.14159265f;
            const float turn = std::fmod(d + angle * 1.1f, 3.4f);
            return is(turn < 1.3f || d < 1.0f);
        }
        case Guster:
        {
            const float angle = std::atan2(cy, cx);
            const bool swirl = ring(d, 2.0f, 3.2f) * (std::sin(angle * 3.0f) > -0.4f ? 1.0f : 0.0f) > 0.0f;
            const bool outer = ring(d, 5.0f, 6.3f) * (std::sin(angle * 2.0f + 1.0f) > 0.0f ? 1.0f : 0.0f) > 0.0f;
            const bool eyes = std::fabs(cy + 0.5f) < 0.8f && std::fabs(std::fabs(cx) - 1.5f) < 0.7f;
            return is(swirl || outer || eyes);
        }
        default: return 0.0f;
    }
}

std::string Banners::key(int baseColour, const Design& design)
{
    std::string out;
    out.push_back(static_cast<char>('a' + std::clamp(baseColour, 0, 15)));
    for (const Layer& layer : design)
    {
        out.push_back(static_cast<char>('A' + layer.pattern));
        out.push_back(static_cast<char>('a' + layer.colour));
    }
    return out;
}

int Banners::Library::idOf(const Design& design)
{
    if (design.empty()) return 0;
    for (size_t i = 0; i < m_designs.size(); ++i)
        if (m_designs[i] == design) return static_cast<int>(i) + 1;
    if (m_designs.size() >= 65000u) return 0;
    m_designs.push_back(design);
    return static_cast<int>(m_designs.size());
}

const Banners::Design* Banners::Library::get(int id) const
{
    if (id <= 0 || id > static_cast<int>(m_designs.size())) return nullptr;
    return &m_designs[static_cast<size_t>(id - 1)];
}

Banners::Design Banners::Library::design(int id) const
{
    const Design* found = get(id);
    return found ? *found : Design{};
}

void Banners::compose(int baseColour, const Design& design, const Sheets& sheets, std::vector<uint8_t>& out)
{
    out.assign(static_cast<size_t>(SHEET * SHEET * 4), 0);
    for (int sy = 0; sy < SHEET; ++sy)
        for (int sx = 0; sx < SHEET; ++sx)
        {
            uint8_t* px = &out[static_cast<size_t>((sy * SHEET + sx) * 4)];
            const Sheet& frame = sheets.frame;
            if (frame.loaded() && frame.width >= SHEET && frame.height >= SHEET)
            {
                const uint8_t* src = &frame.rgba[static_cast<size_t>((sy * frame.width + sx) * 4)];
                px[0] = src[0]; px[1] = src[1]; px[2] = src[2]; px[3] = src[3];
            }
            else
                frameFallback(sx, sy, px);
        }

    Design layers;
    layers.push_back(Layer{ Base, static_cast<uint8_t>(std::clamp(baseColour, 0, 15)) });
    layers.insert(layers.end(), design.begin(), design.end());

    for (const Layer& layer : layers)
    {
        if (layer.pattern >= PatternCount) continue;
        const Sheet& mask = sheets.patterns[layer.pattern];
        const bool real = mask.loaded() && mask.width >= SHEET && mask.height >= SHEET;
        const unsigned int dye = dyeRgb(layer.colour);
        const float dr = static_cast<float>((dye >> 16) & 255u);
        const float dg = static_cast<float>((dye >> 8) & 255u);
        const float db = static_cast<float>(dye & 255u);
        for (int sy = 0; sy < SHEET; ++sy)
            for (int sx = 0; sx < SHEET; ++sx)
            {
                float alpha = 0.0f, light = 1.0f;
                if (real)
                {
                    const uint8_t* src = &mask.rgba[static_cast<size_t>((sy * mask.width + sx) * 4)];
                    alpha = static_cast<float>(src[3]) / 255.0f;
                    light = static_cast<float>(src[0]) / 255.0f;
                }
                else
                    alpha = sheetCoverage(layer.pattern, sx, sy, light);
                if (alpha <= 0.0f) continue;
                uint8_t* px = &out[static_cast<size_t>((sy * SHEET + sx) * 4)];
                px[0] = static_cast<uint8_t>(std::clamp(px[0] * (1.0f - alpha) + dr * light * alpha, 0.0f, 255.0f));
                px[1] = static_cast<uint8_t>(std::clamp(px[1] * (1.0f - alpha) + dg * light * alpha, 0.0f, 255.0f));
                px[2] = static_cast<uint8_t>(std::clamp(px[2] * (1.0f - alpha) + db * light * alpha, 0.0f, 255.0f));
                px[3] = 255;
            }
    }
}
