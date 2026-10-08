#include "PlayerSkin.h"
#include "Core/GLFunctions.h"
#include "World/Noise.h"   // hashCoords / hashToFloat
#include <stb_image.h>
#include <algorithm>
#include <cstdio>
#include <iterator>

namespace
{
    struct Rgba { uint8_t r, g, b, a; };

    uint8_t scaleChannel(uint8_t value, float factor)
    {
        return static_cast<uint8_t>(std::clamp(value * factor, 0.0f, 255.0f));
    }

    Rgba shade(Rgba c, float factor)
    {
        return Rgba{ scaleChannel(c.r, factor), scaleChannel(c.g, factor),
                     scaleChannel(c.b, factor), c.a };
    }

    // A library of original characters, the way Eaglercraft offers a set
    // to pick from instead of a Mojang account. None of these are
    // Mojang's colours; they just have to look like they belong in the
    // same game.
    struct Palette
    {
        const char* name;
        Rgba skin, hair, shirt, trousers, shoes, eye, mouth;
    };

    const Palette PALETTES[] = {
        { "TEAL",
          { 226, 180, 142, 255 }, {  82,  54,  34, 255 }, {  56, 148, 156, 255 },
          {  58,  70, 118, 255 }, {  66,  66,  72, 255 }, {  54,  84, 132, 255 },
          { 150,  94,  84, 255 } },

        { "EMBER",
          { 240, 200, 162, 255 }, { 188,  92,  40, 255 }, {  94, 152,  76, 255 },
          { 110,  82,  58, 255 }, {  72,  60,  52, 255 }, {  82, 126,  84, 255 },
          { 158, 104,  92, 255 } },

        { "ASH",
          { 150, 106,  76, 255 }, {  32,  28,  26, 255 }, {  92,  96, 104, 255 },
          {  48,  50,  56, 255 }, {  34,  34,  38, 255 }, {  70,  52,  40, 255 },
          { 118,  72,  64, 255 } },

        { "ROSE",
          { 246, 214, 190, 255 }, { 208, 110, 140, 255 }, { 232, 232, 236, 255 },
          {  96,  54,  70, 255 }, {  60,  52,  56, 255 }, { 120,  70,  96, 255 },
          { 170, 108, 108, 255 } },

        { "PINE",
          { 198, 148, 108, 255 }, {  58,  42,  30, 255 }, {  48, 104,  72, 255 },
          {  74,  60,  44, 255 }, {  48,  40,  34, 255 }, {  56,  92,  66, 255 },
          { 142,  92,  82, 255 } },

        { "DUSK",
          { 122,  84,  60, 255 }, {  26,  24,  32, 255 }, { 104,  72, 148, 255 },
          {  44,  40,  62, 255 }, {  32,  30,  40, 255 }, {  96,  72, 140, 255 },
          { 112,  70,  64, 255 } },

        { "SAND",
          { 236, 198, 156, 255 }, { 206, 178,  96, 255 }, { 190,  72,  64, 255 },
          { 108,  86,  62, 255 }, {  66,  54,  44, 255 }, {  96, 120, 150, 255 },
          { 156, 100,  90, 255 } },

        { "FROST",
          { 214, 182, 158, 255 }, { 216, 220, 228, 255 }, {  92, 140, 190, 255 },
          {  42,  54,  82, 255 }, {  52,  56,  66, 255 }, { 120, 162, 196, 255 },
          { 148,  98,  92, 255 } },
    };

    constexpr int PALETTE_COUNT = static_cast<int>(std::size(PALETTES));

    class Canvas
    {
    public:
        Canvas(std::vector<uint8_t>& pixels, int width, int height)
            : m_pixels(pixels), m_width(width), m_height(height) {}

        void set(int x, int y, Rgba c)
        {
            if (x < 0 || y < 0 || x >= m_width || y >= m_height) return;
            const size_t i = (static_cast<size_t>(y) * m_width + x) * 4;
            m_pixels[i] = c.r; m_pixels[i + 1] = c.g; m_pixels[i + 2] = c.b; m_pixels[i + 3] = c.a;
        }

        void rect(int x, int y, int w, int h, Rgba c)
        {
            for (int j = 0; j < h; ++j)
                for (int i = 0; i < w; ++i)
                    set(x + i, y + j, c);
        }

        // Minecraft skins are flat colour with a light dither over the
        // top. The old version used a much darker speckle at twice this
        // density, which read as dirt rather than as fabric.
        void cloth(int x, int y, int w, int h, Rgba base, uint32_t salt)
        {
            const Rgba darker = shade(base, 0.95f);
            const Rgba lighter = shade(base, 1.04f);
            for (int j = 0; j < h; ++j)
                for (int i = 0; i < w; ++i)
                {
                    const float n = hashToFloat(hashCoords(x + i, y + j, 0, salt));
                    set(x + i, y + j, n < 0.07f ? darker : (n > 0.96f ? lighter : base));
                }
        }

    private:
        std::vector<uint8_t>& m_pixels;
        int m_width, m_height;
    };
}

PlayerSkin::~PlayerSkin()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

int PlayerSkin::variantCount() { return PALETTE_COUNT; }

const char* PlayerSkin::variantName(int variant)
{
    if (variant == CUSTOM_VARIANT) return "CUSTOM";
    if (variant < 0 || variant >= PALETTE_COUNT) return "?";
    return PALETTES[variant].name;
}

bool PlayerSkin::customAvailable()
{
    // Only the header is needed to know whether it is worth offering.
    int w = 0, h = 0, channels = 0;
    if (!stbi_info("assets/skins/custom.png", &w, &h, &channels)) return false;
    return w == 64 && (h == 32 || h == 64);
}

void PlayerSkin::build(Style style, int variant)
{
    m_style = style;
    m_variant = variant;
    m_fromFile = false;

    // Your own skin wins when it is the one selected. Both the old 64x32
    // and the modern 64x64 layouts work: the parts a flat doll needs sit
    // at the same coordinates in each.
    if (variant == CUSTOM_VARIANT && loadFile("assets/skins/custom.png"))
    {
        upload();
        return;
    }

    paint(variant == CUSTOM_VARIANT ? 0 : variant);
    upload();
}

bool PlayerSkin::loadFile(const std::string& path)
{
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    if (!data)
    {
        m_width = 64;
        m_height = 32;
        return false;
    }

    // Anything that is not a skin-shaped image would map the body parts
    // onto nonsense, so it is rejected rather than drawn.
    if (m_width != 64 || (m_height != 32 && m_height != 64))
    {
        std::printf("Skin %s ignored: expected 64x32 or 64x64, got %dx%d\n",
                    path.c_str(), m_width, m_height);
        stbi_image_free(data);
        m_width = 64;
        m_height = 32;
        return false;
    }

    m_pixels.assign(data, data + static_cast<size_t>(m_width) * m_height * 4);
    stbi_image_free(data);
    m_fromFile = true;
    std::printf("Skin: loaded %s (%dx%d)\n", path.c_str(), m_width, m_height);
    return true;
}

void PlayerSkin::paint(int variant)
{
    const Palette& p = PALETTES[std::clamp(variant, 0, PALETTE_COUNT - 1)];
    const int arm = armWidth();

    m_width = 64;
    m_height = 32;
    m_pixels.assign(static_cast<size_t>(m_width) * m_height * 4, 0);
    Canvas canvas(m_pixels, m_width, m_height);

    const Rgba white{ 240, 240, 240, 255 };
    const Rgba skinDark = shade(p.skin, 0.86f);
    const Rgba hairLight = shade(p.hair, 1.18f);
    const Rgba hairDark = shade(p.hair, 0.78f);

    // ---------------------------------------------------------- head ---
    canvas.cloth(8, 0, 8, 8, p.hair, 11u);                  // top
    canvas.cloth(16, 0, 8, 8, skinDark, 12u);               // under the jaw

    canvas.cloth(0, 8, 8, 8, p.skin, 13u);                  // right
    canvas.cloth(8, 8, 8, 8, p.skin, 14u);                  // front
    canvas.cloth(16, 8, 8, 8, p.skin, 15u);                 // left
    canvas.cloth(24, 8, 8, 8, p.hair, 16u);                 // back

    // Hair: three solid rows, then a fringe that is deeper at the
    // temples than over the eyes, which is what stops it reading as a
    // hat. The old version had a flat edge all the way across.
    for (int face = 0; face < 3; ++face)
        canvas.rect(face * 8, 8, 8, 3, p.hair);
    canvas.rect(24, 8, 8, 6, p.hair);

    const int fringe[8] = { 5, 4, 3, 4, 4, 3, 4, 5 };
    for (int x = 0; x < 8; ++x)
        for (int y = 3; y < fringe[x]; ++y)
            canvas.set(8 + x, 8 + y, (y == fringe[x] - 1) ? hairDark : p.hair);

    // A lit top edge, so the hair has a direction to it.
    for (int face = 0; face < 3; ++face)
        for (int x = 0; x < 8; ++x)
            if ((x % 3) != 2) canvas.set(face * 8 + x, 8, hairLight);

    // Sideburns, longer at the back.
    canvas.rect(0, 11, 1, 4, p.hair);
    canvas.rect(6, 11, 2, 2, p.hair);
    canvas.rect(16, 11, 2, 2, p.hair);
    canvas.rect(23, 11, 1, 4, p.hair);

    // Face. Brow shadow above each eye makes them read as set into the
    // head rather than painted on.
    canvas.set(9, 11, skinDark);  canvas.set(10, 11, skinDark);
    canvas.set(13, 11, skinDark); canvas.set(14, 11, skinDark);
    canvas.set(9, 12, white);     canvas.set(10, 12, p.eye);
    canvas.set(13, 12, p.eye);    canvas.set(14, 12, white);

    canvas.set(11, 13, skinDark);        // nose
    canvas.set(12, 13, shade(p.skin, 0.94f));
    canvas.set(10, 14, shade(p.skin, 0.95f));   // cheeks
    canvas.set(13, 14, shade(p.skin, 0.95f));
    canvas.set(11, 15, p.mouth);
    canvas.set(12, 15, p.mouth);

    // ---------------------------------------------------------- body ---
    canvas.cloth(20, 16, 8, 4, p.shirt, 21u);               // shoulders
    canvas.cloth(28, 16, 8, 4, p.trousers, 22u);            // underside
    canvas.cloth(16, 20, 4, 12, shade(p.shirt, 0.88f), 23u);// right side
    canvas.cloth(20, 20, 8, 12, p.shirt, 24u);              // front
    canvas.cloth(28, 20, 4, 12, p.shirt, 25u);              // left side
    canvas.cloth(32, 20, 8, 12, shade(p.shirt, 0.93f), 26u);// back

    // A lit collar and the trousers showing below the hem.
    canvas.rect(20, 20, 8, 1, shade(p.shirt, 1.14f));
    canvas.rect(16, 29, 24, 3, p.trousers);
    canvas.rect(16, 29, 24, 1, shade(p.trousers, 0.80f));

    // ----------------------------------------------------------- arm ---
    canvas.cloth(44, 16, arm, 4, p.shirt, 31u);                     // top
    canvas.cloth(44 + arm, 16, arm, 4, skinDark, 32u);              // hand underside
    for (int face = 0; face < 4; ++face)
        canvas.cloth(40 + arm * face, 20, arm, 12, p.skin, 33u + face);

    // A short sleeve over the top four rows, a darker cuff where it
    // ends, and a slightly darker hand so the wrist reads.
    for (int face = 0; face < 4; ++face)
    {
        const int fx = 40 + arm * face;
        canvas.cloth(fx, 20, arm, 4, p.shirt, 51u + face);
        canvas.rect(fx, 23, arm, 1, shade(p.shirt, 0.86f));   // sleeve hem
    }

    // ----------------------------------------------------------- leg ---
    canvas.cloth(4, 16, 4, 4, p.trousers, 41u);             // top
    canvas.cloth(8, 16, 4, 4, shade(p.shoes, 0.85f), 42u);  // sole
    for (int face = 0; face < 4; ++face)
    {
        const int fx = face * 4;
        canvas.cloth(fx, 20, 4, 12, p.trousers, 43u + face);

        // The shoe, with a lighter line where it meets the leg --
        // without it the boot just looks like darker trouser.
        canvas.rect(fx, 29, 4, 3, p.shoes);
        canvas.rect(fx, 29, 4, 1, shade(p.shoes, 1.35f));
    }
}

void PlayerSkin::upload()
{
    if (!m_texture) glGenTextures(1, &m_texture);

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, m_pixels.data());

    // Nearest everywhere: a 64-pixel skin blown up to doll size has to
    // stay crisp, and smoothing would bleed the hair into the face.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
