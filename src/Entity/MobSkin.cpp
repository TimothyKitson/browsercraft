#include "MobSkin.h"
#include "BoxMesh.h"
#include "Core/GLFunctions.h"
#include "World/Noise.h"
#include <algorithm>

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
        Canvas(std::vector<uint8_t>& pixels, int size) : m_pixels(pixels), m_size(size) {}

        void set(int x, int y, Rgba c)
        {
            if (x < 0 || y < 0 || x >= m_size || y >= m_size) return;
            const size_t i = (static_cast<size_t>(y) * m_size + x) * 4;
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
        int m_size;
    };

    // How wide and tall a box's unwrapped strip is.
    void stripSize(const glm::ivec3& size, int& w, int& h)
    {
        w = 2 * (size.x + size.z);
        h = size.y + size.z;
    }
}

MobSkin::~MobSkin()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

// Shelf packing, tallest strip first. Boxes of the same size and part
// share one patch -- four identical legs cost one, which is what keeps a
// cow inside a 64-pixel sheet.
bool MobSkin::pack(std::vector<MobBox>& boxes, int sheet)
{
    std::vector<int> order(boxes.size());
    for (size_t i = 0; i < boxes.size(); ++i) order[i] = static_cast<int>(i);

    std::sort(order.begin(), order.end(), [&](int a, int b) {
        int aw = 0, ah = 0, bw = 0, bh = 0;
        stripSize(boxes[a].size, aw, ah);
        stripSize(boxes[b].size, bw, bh);
        if (ah != bh) return ah > bh;
        return aw > bw;
    });

    std::vector<bool> placed(boxes.size(), false);
    int cursorX = 0, cursorY = 0, shelfHeight = 0;

    for (int index : order)
    {
        MobBox& box = boxes[index];

        int twin = -1;
        for (size_t i = 0; i < boxes.size(); ++i)
            if (placed[i] && boxes[i].size == box.size && boxes[i].part == box.part)
            {
                twin = static_cast<int>(i);
                break;
            }

        if (twin >= 0)
        {
            box.u = boxes[twin].u;
            box.v = boxes[twin].v;
            placed[index] = true;
            continue;
        }

        int w = 0, h = 0;
        stripSize(box.size, w, h);
        if (w > sheet || h > sheet) return false;

        if (cursorX + w > sheet)
        {
            cursorX = 0;
            cursorY += shelfHeight;
            shelfHeight = 0;
        }
        if (cursorY + h > sheet) return false;

        box.u = cursorX;
        box.v = cursorY;
        cursorX += w;
        shelfHeight = std::max(shelfHeight, h);
        placed[index] = true;
    }

    return true;
}

void MobSkin::layout(MobId id)
{
    m_boxes = mobType(id).model;
    pack(m_boxes, SHEET);
}

void MobSkin::build(MobId id)
{
    layout(id);
    paint(mobType(id));
    upload();
}

void MobSkin::paint(const MobType& type)
{
    m_pixels.assign(static_cast<size_t>(SHEET) * SHEET * 4, 0);
    Canvas canvas(m_pixels, SHEET);

    const Rgba body = unpack(type.bodyColour);
    const Rgba head = unpack(type.headColour);
    const Rgba eye = unpack(type.eyeColour);

    uint32_t salt = 7u;

    for (const MobBox& box : m_boxes)
    {
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

        if (!isHead) continue;

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

        // A beak, for the one species that has one.
        if (type.id == MobId::Chicken)
            canvas.rect(fx + fw / 2 - 1, eyeY + eyeH, 2, 1, shade(unpack(0xFF28A8E8u), 1.0f));
    }
}

void MobSkin::upload()
{
    if (!m_texture) glGenTextures(1, &m_texture);

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SHEET, SHEET, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, m_pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
