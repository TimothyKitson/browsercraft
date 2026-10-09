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
    }
    else
    {
        m_width = type.sheetWidth;
        m_height = type.sheetHeight;
        paint(type);
    }

    m_texture = upload(m_pixels, m_width, m_height, m_texture);

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
    if (w != 64 || (h != 32 && h != 64))
    {
        std::printf("Mob texture %s ignored: expected 64x32 or 64x64, got %dx%d\n",
                    path.c_str(), w, h);
        stbi_image_free(data);
        return false;
    }

    pixels.assign(data, data + static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);
    return true;
}

void MobSkin::paint(const MobType& type)
{
    m_pixels.assign(static_cast<size_t>(m_width) * m_height * 4, 0);
    Canvas canvas(m_pixels, m_width, m_height);

    const Rgba body = unpack(type.bodyColour);
    const Rgba head = unpack(type.headColour);
    const Rgba eye = unpack(type.eyeColour);

    uint32_t salt = 7u;

    for (const MobBox& box : m_boxes)
    {
        if (box.layer != 0) continue;

        const bool isHead = box.part == Part::Head;
        const Rgba base = isHead ? head : body;

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

        canvas.rect(fx + inset, eyeY, eyeW, eyeH, eye);
        canvas.rect(fx + fw - inset - eyeW, eyeY, eyeW, eyeH, eye);

        // A creeper's face is the one everybody knows: two eyes and a
        // mouth that drops into a pair of legs.
        if (type.id == MobId::Creeper)
        {
            const int mouthX = fx + fw / 2 - 1;
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
