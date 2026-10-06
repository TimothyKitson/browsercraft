#pragma once
#include <string>

// The block texture atlas: a grid of ATLAS_TILES_PER_ROW x ATLAS_TILES_PER_ROW
// square tiles, each TILE_PIXEL_SIZE pixels wide, packed into one texture.
// Block/getUV() maps a tile index to UV coordinates inside this atlas.
class Texture
{
public:
    static constexpr int ATLAS_TILES_PER_ROW = 4;
    static constexpr int TILE_PIXEL_SIZE = 16;

    // Tries to load assets/textures/blocks.png (a real atlas you supply,
    // laid out exactly as described above). If the file isn't there, it
    // generates a flat-colored placeholder atlas in memory instead, so the
    // engine runs with zero external assets required.
    explicit Texture(const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(unsigned int unit = 0) const;

private:
    unsigned int m_id = 0;

    void createFromFile(const std::string& path);
    void createProceduralAtlas();
    void upload(const unsigned char* pixels, int width, int height);
};
