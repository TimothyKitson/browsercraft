# Block texture atlas

Drop a file named `blocks.png` in this folder to replace the built-in
placeholder colors with real textures. If this file doesn't exist, the
engine generates a flat-colored atlas in memory instead (see
`Texture::createProceduralAtlas` in `src/Renderer/Texture.cpp`) so the
project always runs with zero required art assets.

## Layout your atlas must follow

- A 4x4 grid of square tiles (16 tiles total).
- Each tile should be the same size -- 16x16 pixels is the classic
  Minecraft-style size, so the whole image is 64x64. (You can use a bigger
  tile size like 32x32 or 64x64 for higher-res textures; just keep tiles
  square and keep the grid 4x4.)
- Tile index 0 is the **top-left** tile. Indices increase left-to-right,
  then wrap to the next row down -- tile 4 is the first tile of the second
  row, and so on.

Tile indices are assigned to blocks in `src/World/Block.cpp`
(`TileIndex` namespace + `getBlockFaceTiles`). Current assignment:

| Index | Used for |
|------:|----------|
| 0 | unused (treated as fully transparent) |
| 1 | grass top |
| 2 | grass side |
| 3 | dirt |
| 4 | stone |
| 5 | sand |
| 6 | wood (log) side |
| 7 | wood (log) top |
| 8 | leaves |
| 9 | bedrock |
| 10-15 | unused -- free slots for new blocks |

Do not use Mojang's actual Minecraft textures if you plan to share or
distribute this project -- they're copyrighted. Draw your own, generate
some procedurally, or use a texture pack explicitly licensed for reuse.
