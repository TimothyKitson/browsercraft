# Recovered VoxelEngine source

This is the C++ source for Browsercraft, recovered from the transcript of the
Claude Code session **"Custom game engine setup"** (session
`017FHKueHc8u64k89Eg8nmis`, 2026-10-03, 00:52–05:15 UTC). It originally lived at
`C:\Users\TempAdmin\VoxelEngine` on a Windows machine.

It was reassembled by walking that session's transcript backwards in twelve
pages and extracting every `Write` body, `Edit` replacement, and `Read` result,
keeping the longest version seen of each file. Nothing here was retyped or
guessed — every byte came out of a recorded tool call.

## How much to trust it

Good evidence that the extraction is accurate: the deployed `index.data` packs
the eight shaders, and **seven of the eight are byte-identical** to the copies
here.

| Shader | Deployed | Recovered | |
|---|---|---|---|
| `chunk.vert` | 2129 | 2129 | identical |
| `line.frag` | 102 | 102 | identical |
| `line.vert` | 176 | 176 | identical |
| `sky.frag` | 3442 | 3442 | identical |
| `sky.vert` | 145 | 145 | identical |
| `ui.frag` | 226 | 226 | identical |
| `ui.vert` | 294 | 294 | identical |
| `chunk.frag` | 3724 | 3264 | **differs** |

All 26 `.cpp` files end on a closing brace and every header opens with
`#pragma once`, so nothing is obviously truncated.

## What it is NOT

**This snapshot is older than the build on browsercraft.net.** `chunk.frag`
differing is the first clue; the feature gap confirms it. Searching the
deployed `index.wasm` for strings that have no counterpart in this source:

| Feature | In deployed `index.wasm` | In this source |
|---|---|---|
| Nether | yes | **no** |
| End Stone | yes | **no** |
| Nether Portal | yes | **no** |
| `keybinds.txt` / rebindable controls | yes | **no** |

So roughly three days of work — dimensions, portals, and the rebindable
controls system — happened after this snapshot and is not captured here.
Building this tree would produce an earlier Browsercraft, not the current one.

Treat it as a recovered backup and a reliable guide to how the engine is
structured, not as the exact source of the live build. The authoritative copy
is still `C:\Users\TempAdmin\VoxelEngine` on that Windows machine.

## Layout

```
CMakeLists.txt          build.bat         tools/build-web.ps1
src/Core/               Application, Window, Input, ThreadPool, GLFunctions
src/Renderer/           Atlas, Shader, Mesh, Camera, Font, Texture,
                        UIRenderer, SkyRenderer, SelectionRenderer, Screenshot
src/World/              World, WorldGen, Chunk, ChunkMesher, Noise, Block,
                        WorldSave
src/Player/             Player, Inventory
assets/shaders/         8 GLSL files
```

Not recovered: the texture assets (`assets/textures/`) and anything else never
opened during that session. Block textures appear to be generated procedurally
in `src/Renderer/Atlas.cpp` rather than loaded from image files, so the gap may
be smaller than it looks.

## Notes on the tree bugs

`src/World/WorldGen.cpp` holds the tree code — `buildTree()` at line 288 and the
placement loop in `decorate()` at line 336.

Two hypotheses for the "trees grow in rows" report were tested against this
code and **both were ruled out**:

1. *A weak coordinate hash.* `hashCoords()` in `src/World/Noise.cpp` was run
   over a 256×256 area at the Forest rate of 0.055. Per-line tree counts came
   out at 1.00× and 0.96× the standard deviation expected from independent
   random placement, with no empty lines. The hash is statistically clean.
2. *Striped biomes.* `temperatureAt()` and `humidityAt()` sample fbm noise at
   0.0016 and 0.0019, giving biome features hundreds of blocks across. Biomes
   are large blobs, not stripes.

What does stand out is density: `decorate()` tests every single column at
`treeChance = 0.055` in Forest with no minimum spacing between trees. That puts
a trunk in roughly one column in eighteen — trees about four blocks apart —
while the canopy has radius 2. Neighbouring canopies therefore overlap
constantly and merge into a lattice, which is a plausible source of the
"rows" impression. Minecraft instead picks a tree count per chunk and enforces
spacing.

This analysis is against *this* snapshot. The deployed build is newer, so
confirm the code still looks like this before acting on it.
