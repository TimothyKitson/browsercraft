# Runtime textures

The game loads these directories directly at runtime:

- `block/`: world block atlas textures, animation metadata, and optional LabPBR maps
- `item/`: inventory and dropped-item icons
- `gui/`: HUD sprites and GUI images
- `entity/`: entity, block-model, particle, environment, and other vanilla texture sheets
- `colormap/`, `particle/`, `environment/`, and related categories: renderer support assets

Install a Java resource pack with `tools/install-texture-pack.ps1`. The installer
copies all supported texture categories instead of a short list of selected
block/item names. The web build stages these same directories into the
Emscripten data package.

Only add art to this public repository when you own it or have explicit
permission to redistribute it. Attribution alone is not a licence.
