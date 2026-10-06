# VoxelEngine

A from-scratch 3D voxel engine in C++ and OpenGL -- a Minecraft-1.12-style
"look around, generated terrain, break/place blocks" foundation you build on
yourself. No engine framework, no vendored black-box libraries you can't
read: every system here (windowing, the OpenGL loader, shaders, meshing,
world generation, physics) is something you can open and understand in one
sitting.

This is **not** a finished game. It's a working starting point plus a clear
roadmap (below) for the next pieces. Build what you can, and when you get
stuck on a piece, send it back and we'll work through it together.

## What's here right now

- A window + OpenGL 3.3 core context (SDL2), with a hand-written minimal GL
  function loader instead of a vendored GLAD/GLEW file (`src/Core/GLFunctions.*`)
  -- so you can see exactly which GL functions are used and add more
  yourself when you need them.
- A shader class that loads/compiles/links GLSL from disk with real error
  messages (`src/Renderer/Shader.*`).
- A chunked voxel world: 16x64x16 block chunks, each with its own mesh
  (`src/World/Chunk.*`, `src/World/World.*`).
- Procedural terrain using hand-written value noise (no external noise
  library) -- rolling hills, stone/dirt/grass layering, bedrock floor
  (`src/World/Noise.*`, `src/World/WorldGen.*`).
- A texture atlas system that falls back to a generated placeholder so the
  game runs with zero art assets, or loads your own `assets/textures/blocks.png`
  (`src/Renderer/Texture.*`, see `assets/textures/README.md`).
- A real player: gravity, jumping, walking, AABB collision against the
  voxel grid, plus a toggleable noclip flight mode for exploring/debugging
  (`src/Player/Player.*`).
- Break/place block interaction via a raycast from the camera
  (`World::raycast` in `src/World/World.cpp`).

## Controls

| Input | Action |
|---|---|
| Mouse | Look around |
| W / A / S / D | Walk |
| Space | Jump (walking) / fly up (flying) |
| Left Ctrl | Fly down (flying only) |
| F | Toggle flying (noclip) |
| Left click | Break the block you're looking at |
| Right click | Place the selected block |
| 1-5 | Select block to place (stone/dirt/grass/sand/wood) |
| Escape | Quit |

## Project layout

```
VoxelEngine/
  CMakeLists.txt          Build config -- fetches SDL2, GLM, stb at configure time
  assets/
    shaders/              GLSL source, loaded from disk at runtime
    textures/              Drop blocks.png here (see textures/README.md)
  src/
    main.cpp              Entry point
    Core/                 Window, input, OpenGL function loading, the Application/main loop
    Renderer/             Shader, Camera, Mesh (VAO/VBO wrapper), Texture
    World/                Block types, Chunk (voxel storage + meshing), World (chunk map, raycast), noise-based WorldGen
    Player/               Gravity, jumping, AABB-vs-voxel collision
```

Read `src/Core/Application.cpp` first -- it wires every system together and
is the shortest path to understanding how a frame happens: poll input ->
update player physics -> handle block break/place -> render all chunks.

## Building

### 1. Install prerequisites

- **CMake** 3.20+ (if you have Visual Studio 2022 installed with the "Desktop
  development with C++" workload, you likely already have CMake -- check with
  `cmake --version` in a regular terminal).
- **A C++ compiler**: MSVC (via Visual Studio or the standalone Build Tools,
  "Desktop development with C++" workload) is what this was tested with. MinGW
  works too with minor adjustments (see Troubleshooting).
- **Git** (CMake uses it to fetch SDL2/GLM/stb automatically -- you don't
  need to install those yourself).
- An internet connection the first time you configure (to fetch those
  dependencies). After that, they're cached in `build/_deps` and configuring
  again is instant.

### 2. Open in VS Code

Install the recommended extensions when prompted (CMake Tools + C/C++), or
manually: `ms-vscode.cmake-tools` and `ms-vscode.cpptools`.

With CMake Tools installed, opening this folder should prompt you to select
a "kit" (compiler) -- choose your Visual Studio / MSVC kit. Then:

- **Configure**: Command Palette -> "CMake: Configure" (or it runs
  automatically on open). First run downloads SDL2/GLM/stb -- expect this to
  take a few minutes.
- **Build**: Command Palette -> "CMake: Build", or the Build button in the
  status bar.
- **Run**: Command Palette -> "CMake: Run Without Debugging", or press the
  Run/Debug button. `launch.json` is already set up for F5 debugging with
  MSVC (`cppvsdbg`).

### 3. Or from the command line

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

(Run this from a "Developer Command Prompt for VS" / "x64 Native Tools
Command Prompt" so `cl.exe` is on PATH, or let CMake Tools' kit selection
handle that for you in VS Code.) The built executable and a copy of
`assets/` end up in `build/`.

## Troubleshooting

- **"cmake: command not found"**: install it standalone (`winget install
  Kitware.CMake`), or use the copy bundled with Visual Studio at
  `...\VC\Tools\...` / under `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`.
- **Using MinGW instead of MSVC**: change `.vscode/launch.json`'s `"type"`
  from `cppvsdbg` to `cppdbg` and add `"miDebuggerPath"` pointing at your
  `gdb.exe`. CMake Tools' kit selection should otherwise handle the rest.
- **First configure is slow**: that's SDL2's full source being cloned and
  about to be compiled as part of your build -- normal, and only happens
  once (cached in `build/_deps`).
- **Window opens but shows a black/garbage screen or crashes immediately**:
  check the terminal output -- `Window::Window` prints your GPU's OpenGL
  version. If it's below 3.3, your GPU/driver doesn't support the engine's
  required OpenGL version.

## Roadmap -- what to build next

Roughly in order of how naturally each builds on the last:

1. **Fix chunk-boundary face culling.** Right now `Chunk::generateMesh()`
   only checks neighbors *inside* the same chunk, so every chunk edge
   renders faces that are actually hidden underground by the chunk next
   door. Pass neighboring chunks' edge data into mesh generation to fix it
   -- a big win for vertex count/performance once you have many chunks.
2. **Infinite terrain / chunk streaming.** `World` currently generates a
   fixed square of chunks once at startup (`WORLD_RADIUS_CHUNKS` in
   `Application.cpp`). Track which chunk the player is standing in each
   frame, and load new chunks (generate + mesh) as they come into range,
   unload ones that leave it.
2. **Block selection outline.** `World::raycast` tells you which block
   you're looking at -- render a wireframe cube around it so breaking/placing
   feels precise (right now it's "trust me, that's the block").
3. **Inventory / hotbar.** Right now the "selected block" is just a variable
   toggled by number keys with no on-screen feedback. Render actual hotbar UI
   (will need basic 2D/text rendering -- a good excuse to add an orthographic
   render pass).
4. **Trees and more terrain variety.** `WorldGen::generate` currently makes
   one landform shape everywhere. Add tree placement (another noise sample
   deciding where trunks go), then biomes (vary surface block / tree density
   by a second, larger-scale noise sample).
5. **Caves.** Subtract a 3D noise field from the solid terrain instead of
   only using a 2D height map.
6. **Water.** A block type that doesn't fully occlude neighbors (needs the
   transparency handling in `isBlockTransparent` extended) plus a sea-level
   fill pass in `WorldGen`.
7. **Greedy meshing.** `Chunk::generateMesh()` emits one quad per visible
   face. Merging adjacent same-type, same-tile faces into larger quads
   (the classic "greedy meshing" algorithm) dramatically cuts vertex counts
   on flat areas.
8. **Save/load.** Serialize chunk block data to disk so your world persists
   between runs.
9. **Lighting.** Currently just fixed per-face brightness constants. A real
   lighting system (even simple flood-fill block-light propagation, like
   Minecraft's) is a substantial but very rewarding next system.
10. **Multiplayer.** The "VoidSMP"-scale version of this -- a client/server
    split, which is a whole project on its own.

Each of these is self-contained enough to tackle independently -- pick
whichever sounds most interesting, give it a shot, and send me what you've
got.
