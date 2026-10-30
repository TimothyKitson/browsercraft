#include "MobSkin.h"
#include "BoxMesh.h"
#include "Core/GLFunctions.h"
#include "World/Noise.h"
#include <algorithm>
#include <cstdio>
#include <stb_image.h>

namespace
{
    struct Rgba { uint8_t r, g, b, a; };

    Rgba unpack(uint32_t colour)
    {
        return Rgba{ static_cast<uint8_t>(colour & 0xFF),
                     static_cast<uint8_t>((colour >> 8) & 0xFF),
                     static_cast<uint8_t>((colour >> 16) & 0xFF),
                     static_cast<uint8_t>((colour >> 24) & 0xFF) };
    }

    uint8_t scaleChannel(uint8_t value, float factor)
    {
        return static_cast<uint8_t>(std::clamp(value * factor, 0.0f, 255.0f));
    }

    Rgba shade(Rgba c, float factor)
    {
        return Rgba{ scaleChannel(c.r, factor), scaleChannel(c.g, factor),
                     scaleChannel(c.b, factor), c.a };
    }

    // A pack texture, sampled with wrap so a face wider than one tile
    // repeats the grain instead of running off the edge.
    struct Source
    {
        std::vector<uint8_t> pixels;
        int width = 0;
        int height = 0;

        Rgba at(int x, int y) const
        {
            const int sx = ((x % width) + width) % width;
            const int sy = ((y % height) + height) % height;
            const size_t i = (static_cast<size_t>(sy) * width + sx) * 4;
            return Rgba{ pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3] };
        }
    };

    bool loadBlockTexture(const char* name, Source& out)
    {
        const std::string path = std::string("assets/textures/block/") + name + ".png";

        int channels = 0;
        unsigned char* data = stbi_load(path.c_str(), &out.width, &out.height, &channels, 4);
        if (!data) return false;

        out.pixels.assign(data, data + static_cast<size_t>(out.width) * out.height * 4);
        stbi_image_free(data);
        return out.width > 0 && out.height > 0;
    }

    class Canvas
    {
    public:
        Canvas(std::vector<uint8_t>& pixels, int w, int h)
            : m_pixels(pixels), m_width(w), m_height(h) {}

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

        // A one-pixel darker rim, so one box reads as separate from
        // the next.
        void rim(int x, int y, int w, int h, Rgba c)
        {
            if (w < 3 || h < 3) return;
            for (int i = 0; i < w; ++i) { set(x + i, y, c); set(x + i, y + h - 1, c); }
            for (int j = 0; j < h; ++j) { set(x, y + j, c); set(x + w - 1, y + j, c); }
        }

        // Irregular blotches, on a coarse grid so they read as patches
        // rather than as noise: a cow, a panda, a llama.
        void blotches(int x, int y, int w, int h, Rgba c, uint32_t salt)
        {
            for (int j = 0; j < h; ++j)
                for (int i = 0; i < w; ++i)
                {
                    const float n = hashToFloat(hashCoords((x + i) / 3, (y + j) / 3, 0, salt));
                    const float edge = hashToFloat(hashCoords(x + i, y + j, 1, salt));
                    if (n > 0.58f || (n > 0.50f && edge > 0.6f)) set(x + i, y + j, c);
                }
        }

        // Bands across the patch: a bee, and nothing else so far.
        void bands(int x, int y, int w, int h, Rgba c)
        {
            for (int j = 0; j < h; ++j)
                if (((y + j) / 2) % 2 == 1)
                    for (int i = 0; i < w; ++i) set(x + i, y + j, c);
        }

        // A pale underside over the bottom third, faded in so it does
        // not read as a painted line.
        void underside(int x, int y, int w, int h, Rgba c)
        {
            const int from = y + (h * 2) / 3;
            for (int j = from; j < y + h; ++j)
                for (int i = 0; i < w; ++i)
                    set(x + i, j, (j == from) ? mix(c, 0.5f) : c);
        }

        // A fine grain of the second colour: scales, or whatever a
        // slime is made of.
        void grain(int x, int y, int w, int h, Rgba c, uint32_t salt)
        {
            for (int j = 0; j < h; ++j)
                for (int i = 0; i < w; ++i)
                    if (hashToFloat(hashCoords(x + i, y + j, 2, salt)) > 0.78f)
                        set(x + i, y + j, c);
        }

        // Flat colour under a light speckle, the same treatment the
        // player's cloth gets: at this scale it reads as hide rather
        // than as a flat fill.
        void hide(int x, int y, int w, int h, Rgba base, uint32_t salt)
        {
            const Rgba darker = shade(base, 0.93f);
            const Rgba lighter = shade(base, 1.05f);
            for (int j = 0; j < h; ++j)
                for (int i = 0; i < w; ++i)
                {
                    const float n = hashToFloat(hashCoords(x + i, y + j, 0, salt));
                    set(x + i, y + j, n < 0.09f ? darker : (n > 0.95f ? lighter : base));
                }
        }

    private:
        // Halfway between what is already there and the new colour,
        // which is what makes a belly fade in rather than start.
        Rgba mix(Rgba c, float amount) const
        {
            return Rgba{ scaleChannel(c.r, 1.0f - 0.18f * amount),
                         scaleChannel(c.g, 1.0f - 0.18f * amount),
                         scaleChannel(c.b, 1.0f - 0.18f * amount), c.a };
        }

        std::vector<uint8_t>& m_pixels;
        int m_width, m_height;
    };

}

MobSkin::~MobSkin()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
    if (m_overlayTexture) glDeleteTextures(1, &m_overlayTexture);
}

std::string MobSkin::texturePath(const char* name)
{
    return std::string("assets/skins/mob/") + name + ".png";
}

bool MobSkin::fits(const std::vector<MobBox>& boxes, int sheetWidth, int sheetHeight)
{
    for (const MobBox& box : boxes)
    {
        const int w = 2 * (box.size.x + box.size.z);
        const int h = box.size.y + box.size.z;
        if (box.u < 0 || box.v < 0) return false;
        if (box.u + w > sheetWidth || box.v + h > sheetHeight) return false;
    }
    return true;
}

void MobSkin::layout(MobId id)
{
    const MobType& type = mobType(id);
    m_boxes = type.model;
    m_width = type.sheetWidth;
    m_height = type.sheetHeight;
}

void MobSkin::build(MobId id)
{
    layout(id);

    const MobType& type = mobType(id);

    if (*type.texture && loadFile(texturePath(type.texture), m_pixels, m_width, m_height))
    {
        m_fromFile = true;
        m_pixelWidth = m_width;
        m_pixelHeight = m_height;
    }
    else
    {
        m_width = type.sheetWidth;
        m_height = type.sheetHeight;
        m_pixelWidth = m_width;
        m_pixelHeight = m_height;

        // A thing built out of a block wears that block's texture out of
        // the pack. Only if the pack is there: without it, the painting.
        m_clad = clad(type);
        if (!m_clad) paint(type);
    }

    // Said out loud, because "are the real textures actually being used"
    // is otherwise a question you can only answer by squinting at a
    // screenshot.
    std::printf("Mob %-10s %s\n", type.name,
                m_fromFile ? "installed sheet"
                           : (m_clad ? "clad in a pack block texture"
                                     : "painted in code (no sheet installed)"));

    m_texture = upload(m_pixels, m_pixelWidth, m_pixelHeight, m_texture);

    // The overlay only ever comes from a file. Painting a sheep's wool
    // would mean painting the same hide twice, so without the art the
    // overlay boxes simply are not drawn.
    std::vector<uint8_t> overlay;
    int ow = 0, oh = 0;
    if (*type.overlay && loadFile(texturePath(type.overlay), overlay, ow, oh) &&
        ow == m_width && oh == m_height)
    {
        m_overlayTexture = upload(overlay, ow, oh, m_overlayTexture);
    }
    else
    {
        m_boxes.erase(std::remove_if(m_boxes.begin(), m_boxes.end(),
                                     [](const MobBox& b) { return b.layer != 0; }),
                      m_boxes.end());
    }
}

bool MobSkin::loadFile(const std::string& path, std::vector<uint8_t>& pixels, int& w, int& h)
{
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data) return false;

    // Anything that is not a mob sheet would map the boxes onto nonsense,
    // so it is turned away rather than drawn.
    // A boat is laid out on a sheet twice as wide as an animal's; a
    // breeze and a bat are on the smallest sheet in the game.
    const bool smallSheet = (w == 32 && h == 32);
    const bool animalSheet = (w == 64 && (h == 32 || h == 64));
    const bool wideSheet = (w == 128 && (h == 64 || h == 128));
    if (!smallSheet && !animalSheet && !wideSheet)
    {
        std::printf("Mob texture %s ignored: expected 32x32, 64x32, 64x64 or 128x64, "
                    "got %dx%d\n",
                    path.c_str(), w, h);
        stbi_image_free(data);
        return false;
    }

    pixels.assign(data, data + static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);
    return true;
}

bool MobSkin::clad(const MobType& type)
{
    if (!*type.blockTexture) return false;

    Source source;
    if (!loadBlockTexture(type.blockTexture, source) &&
        !(*type.blockTextureAlt && loadBlockTexture(type.blockTextureAlt, source)))
        return false;

    // One block is sixteen model units, so this many picture pixels to
    // the unit. A sixteen-pixel texture gives one and an HD pack more,
    // which is how the hull ends up finer than the sheet it is laid out
    // on without a single texture coordinate changing.
    const int scale = std::max(1, std::min(8, source.width / 16));

    m_pixelWidth = m_width * scale;
    m_pixelHeight = m_height * scale;
    m_pixels.assign(static_cast<size_t>(m_pixelWidth) * m_pixelHeight * 4, 0);

    for (const MobBox& box : m_boxes)
    {
        if (box.layer != 0) continue;

        for (int face = 0; face < 6; ++face)
        {
            int x = 0, y = 0, w = 0, h = 0;
            BoxMesh::facePatch(face, box.u, box.v, box.size.x, box.size.y, box.size.z, x, y, w, h);

            // The same top-lit, bottom-shadowed treatment the flat
            // painting gets, so a hull still reads as a solid thing
            // before any world lighting touches it.
            float tone = 1.0f;
            if (face == BoxMesh::Top) tone = 1.08f;
            else if (face == BoxMesh::Bottom) tone = 0.78f;
            else if (face == BoxMesh::Left || face == BoxMesh::Right) tone = 0.90f;

            for (int j = 0; j < h * scale; ++j)
                for (int i = 0; i < w * scale; ++i)
                {
                    const Rgba c = shade(source.at(i, j), tone);
                    const int px = x * scale + i;
                    const int py = y * scale + j;
                    if (px < 0 || py < 0 || px >= m_pixelWidth || py >= m_pixelHeight) continue;
                    const size_t at = (static_cast<size_t>(py) * m_pixelWidth + px) * 4;
                    m_pixels[at] = c.r; m_pixels[at + 1] = c.g;
                    m_pixels[at + 2] = c.b; m_pixels[at + 3] = c.a;
                }
        }
    }

    return true;
}

void MobSkin::paint(const MobType& type)
{
    m_pixelWidth = m_width;
    m_pixelHeight = m_height;
    m_pixels.assign(static_cast<size_t>(m_width) * m_height * 4, 0);
    Canvas canvas(m_pixels, m_width, m_height);

    const Rgba body = unpack(type.bodyColour);
    const Rgba head = unpack(type.headColour);
    const Rgba eye = unpack(type.eyeColour);

    // The second colour. Nought means work one out from the body,
    // which is what most animals want: a blotch is the body darkened
    // and a belly is the body lightened, and neither needs naming.
    const bool paleMark = type.pattern == MobType::Pattern::Belly;
    const Rgba mark = type.markColour ? unpack(type.markColour)
                                      : shade(body, paleMark ? 1.34f : 0.66f);

    uint32_t salt = 7u;

    for (const MobBox& box : m_boxes)
    {
        if (box.layer != 0) continue;

        const bool isHead = box.part == Part::Head;
        const bool isBody = box.part == Part::Body;

        // Limbs sit a shade under the body and wings a shade under
        // them. Nothing in nature is one flat colour from nose to
        // foot, and without this every animal read as a single
        // extruded shape rather than a thing with parts.
        float partTone = 1.0f;
        if (!isHead && !isBody) partTone = 0.92f;
        if (box.part == Part::WingLeft || box.part == Part::WingRight) partTone = 0.86f;

        const Rgba base = shade(isHead ? head : body, partTone);

        // Every face of the box, so it is covered from any angle.
        for (int face = 0; face < 6; ++face)
        {
            int x = 0, y = 0, w = 0, h = 0;
            BoxMesh::facePatch(face, box.u, box.v, box.size.x, box.size.y, box.size.z, x, y, w, h);

            // The underside sits in shadow and the top catches the light,
            // which stops a flat-coloured animal reading as a cardboard
            // cut-out before any lighting is applied.
            float tone = 1.0f;
            if (face == BoxMesh::Top) tone = 1.06f;
            else if (face == BoxMesh::Bottom) tone = 0.82f;

            canvas.hide(x, y, w, h, shade(base, tone), salt++);

            // And the markings, which is most of what tells one
            // animal from another at this size.
            const bool side = face != BoxMesh::Top && face != BoxMesh::Bottom;
            switch (type.pattern)
            {
                case MobType::Pattern::Patches:
                    canvas.blotches(x, y, w, h, mark, salt++);
                    break;
                case MobType::Pattern::Stripes:
                    if (side) canvas.bands(x, y, w, h, mark);
                    break;
                case MobType::Pattern::Belly:
                    if (side && (isBody || isHead)) canvas.underside(x, y, w, h, mark);
                    break;
                case MobType::Pattern::Speckled:
                    canvas.grain(x, y, w, h, mark, salt++);
                    break;
                case MobType::Pattern::Plain:
                    break;
            }

            // A one-pixel darker rim round every patch. It is what
            // makes a leg read as a leg rather than as part of the
            // body it hangs off, and it costs four lines.
            canvas.rim(x, y, w, h, shade(base, 0.80f));
        }

        // Mojang's layout stacks the snout, horns and wattle on the head
        // patches, so only the biggest box of a head gets a face painted
        // on it -- otherwise a pig would have eyes on its nose.
        if (!isHead || box.size.x < 4 || box.size.y < 4) continue;

        // The face. Eyes sit a third of the way down the front patch and
        // a quarter in from each side, which lands them sensibly on
        // everything from a chicken's four pixels to a cow's eight.
        int fx = 0, fy = 0, fw = 0, fh = 0;
        BoxMesh::facePatch(BoxMesh::Front, box.u, box.v,
                           box.size.x, box.size.y, box.size.z, fx, fy, fw, fh);

        const int eyeY = fy + std::max(1, fh / 3);
        const int inset = std::max(1, fw / 5);
        const int eyeW = std::max(1, fw / 6);
        const int eyeH = std::max(1, fh / 6);

        // White round a dark pupil where there is room for both. One
        // flat rectangle of eye colour is a dot; a dot with a white
        // round it is an eye, and that is the difference between a
        // box and an animal looking at you.
        const Rgba white{ 236, 238, 240, 255 };
        const bool roomForWhite = eyeW >= 2 && eyeH >= 2;

        for (int which = 0; which < 2; ++which)
        {
            const int ex = which == 0 ? fx + inset : fx + fw - inset - eyeW;
            if (roomForWhite)
            {
                canvas.rect(ex, eyeY, eyeW, eyeH, white);
                canvas.rect(ex + (which == 0 ? eyeW - 1 : 0), eyeY, 1, eyeH, eye);
            }
            else
            {
                canvas.rect(ex, eyeY, eyeW, eyeH, eye);
            }
        }

        // A creeper's face is the one everybody knows: two eyes and a
        // mouth that drops into a pair of legs.
        if (type.id == MobId::Creeper)
        {
            const int mouthX = fx + fw / 2 - 1;
            canvas.rect(fx + inset, eyeY, eyeW, eyeH, eye);
            canvas.rect(fx + fw - inset - eyeW, eyeY, eyeW, eyeH, eye);
            canvas.rect(mouthX, eyeY + eyeH, 2, std::max(1, fh / 4), eye);
            canvas.rect(mouthX - 1, eyeY + eyeH + std::max(1, fh / 4), 1, std::max(1, fh / 5), eye);
            canvas.rect(mouthX + 2, eyeY + eyeH + std::max(1, fh / 4), 1, std::max(1, fh / 5), eye);
        }
    }
}

unsigned int MobSkin::upload(const std::vector<uint8_t>& pixels, int w, int h, unsigned int into)
{
    if (!into) glGenTextures(1, &into);

    glBindTexture(GL_TEXTURE_2D, into);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return into;
}
