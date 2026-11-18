# Runtime skins

- `wide/` and `slim/`: named player skins used by the character picker
- `custom.png`: the optional custom player skin
- `mob/`: model-layout entity sheets installed by `tools/install-mob-textures.ps1`
- `shield.png`: optional shield texture

The web build always stages player skins. Mob sheets are staged only when
`-MobTextures` (PowerShell) or `--mob-textures` (shell) is passed, because
many commonly installed mob sheets are Mojang artwork and are not licensed
for public redistribution.

Only commit skins you own or have explicit permission to redistribute.
