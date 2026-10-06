#include "Texture.h"
#include "Core/GLFunctions.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>
#include <cstdio>
#include <cstdint>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_NEAREST 0x2600
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif

Texture::Texture(const std::string& path)
{
    createFromFile(path);
}

Texture::~Texture()
{
    if (m_id) glDeleteTextures(1, &m_id);
}

void Texture::createFromFile(const std::string& path)
{
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);

    if (data)
    {
        std::printf("Texture: loaded %s (%dx%d)\n", path.c_str(), w, h);
        upload(data, w, h);
        stbi_image_free(data);
    }
    else
    {
        std::printf("Texture: %s not found, generating placeholder atlas instead\n", path.c_str());
        createProceduralAtlas();
    }
}

// A flat color per tile index. Keep this in sync with the tile indices
// documented in src/World/Block.cpp (getBlockFaceTiles).
void Texture::createProceduralAtlas()
{
    const int tilesPerRow = ATLAS_TILES_PER_ROW;
    const int tileSize = TILE_PIXEL_SIZE;
    const int atlasSize = tilesPerRow * tileSize;

    struct RGB { uint8_t r, g, b; };
    static const RGB palette[16] = {
        {0, 0, 0},       // 0  unused (air)
        {95, 159, 53},   // 1  grass top
        {121, 85, 58},   // 2  grass side (dirt w/ green fringe, simplified to brown-green here)
        {134, 96, 67},   // 3  dirt
        {127, 127, 127}, // 4  stone
        {219, 211, 160}, // 5  sand
        {103, 78, 53},   // 6  wood side (log bark)
        {166, 136, 94},  // 7  wood top (log rings)
        {63, 109, 48},   // 8  leaves
        {40, 40, 40},    // 9  bedrock
        {255, 0, 255}, {255, 0, 255}, {255, 0, 255}, {255, 0, 255}, {255, 0, 255}, {255, 0, 255}, // unused -> magenta
    };

    std::vector<uint8_t> pixels(atlasSize * atlasSize * 4);

    for (int tile = 0; tile < tilesPerRow * tilesPerRow; ++tile)
    {
        const RGB& color = palette[tile % 16];
        int tileX = (tile % tilesPerRow) * tileSize;
        int tileY = (tile / tilesPerRow) * tileSize;

        for (int y = 0; y < tileSize; ++y)
        {
            for (int x = 0; x < tileSize; ++x)
            {
                // Cheap pseudo-random speckle so flat colors don't look
                // like plastic. Replace this whole function with a real
                // texture load once you have actual art.
                int hash = (tileX + x) * 374761393 + (tileY + y) * 668265263;
                hash = (hash ^ (hash >> 13)) * 1274126177;
                int jitter = (hash & 0xF) - 8; // -8..7

                auto clamp255 = [](int v) { return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v)); };

                int px = (tileY + y) * atlasSize + (tileX + x);
                pixels[px * 4 + 0] = clamp255(color.r + jitter);
                pixels[px * 4 + 1] = clamp255(color.g + jitter);
                pixels[px * 4 + 2] = clamp255(color.b + jitter);
                pixels[px * 4 + 3] = (tile == 0) ? 0 : 255; // tile 0 (air) fully transparent
            }
        }
    }

    upload(pixels.data(), atlasSize, atlasSize);
}

void Texture::upload(const unsigned char* pixels, int width, int height)
{
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Nearest filtering = crisp blocky pixels, like Minecraft's default.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
