# Prompt to paste into Claude Code on the Windows machine

---

You are picking up work from me — I'm Claude, running in a cloud session for
Timothy on the `TimothyKitson/browsercraft` repo. I can't reach this computer,
so I'm handing this to you in writing. Everything below is fact I verified in
my session, not guesswork; where I'm unsure I say so.

## The situation: there are two codebases and they are not the same game

**ORIGINAL** — `C:\Users\TempAdmin\VoxelEngine` on the machine you're running
on. This is what built the live browsercraft.net. It is **not in version
control anywhere.** It is the authoritative copy. It has four things the other
one doesn't:

- The Nether
- The End
- Nether portals
- Rebindable controls (`keybinds.txt`)

**REBUILT** — GitHub branch `claude/fervent-goodall-ueozok`, HEAD `307086c`. I
reconstructed this from the transcript of an earlier session, because the
original was lost to that session. It is *older* at its root but has a lot of
work layered on top that the original has never seen:

- 32x resource pack loading, 512px atlas, animated water and lava
- LabPBR normal + specular atlases, bound and fed to the shader
- 8 mob species with physics, AI, spawning, despawn, and rendering
- Positional audio: SDL2 device, stb_vorbis, 24 voices, distance roll-off
- 135 packed sound slots
- A chat/command console with Minecraft-style commands
- A main menu, pause menu, settings and Controls screen
- A panorama system and a generating screen
- **Three bug fixes without which the web build does not work at all**

## Your job

Port REBUILT's work into ORIGINAL, so there is one codebase that has
everything. ORIGINAL wins on the four features it has; REBUILT wins on the
rest. **Diff before you assume** — REBUILT is older at its root, so in places
ORIGINAL's version of a file is the better one even though REBUILT's is the
one with new features bolted on. I hit 177 compile errors from exactly this
kind of version skew when I rebuilt it; expect some.

## Step 1 — get REBUILT onto this machine

```
cd C:\Users\TempAdmin
git clone https://github.com/TimothyKitson/browsercraft.git browsercraft-rebuilt
cd browsercraft-rebuilt
git checkout claude/fervent-goodall-ueozok
```

The engine source is in `recovered-source/`. Read `recovered-source/RECOVERY.md`
first — it documents exactly how each file was recovered and how far to trust
it. Short version: seven of eight shaders are byte-identical to what's deployed,
so the extraction is sound.

**Before you change anything, put ORIGINAL under version control.** It is the
only copy and it has four features nothing else has:

```
cd C:\Users\TempAdmin\VoxelEngine
git init
git add -A
git commit -m "Original VoxelEngine source, as found"
```

Do this even if you do nothing else. Losing it again would be the single worst
outcome here.

## Step 2 — the three fixes that matter most

These are small and they are the difference between a working build and a
broken one. Check whether ORIGINAL has them; I'd guess not, since they were
found by testing the web build and ORIGINAL predates that testing.

### 2a. The world never generates (the big one)

`src/World/World.cpp`, in `World::update()`. On the web there are no worker
threads, so nothing ever drained the job queue — the queue grew forever and
**not one chunk was ever built.** The player spawned in the void and fell.

```cpp
namespace { constexpr double CHUNK_JOB_BUDGET_MS = 6.0; }

void World::update(const glm::vec3& playerPosition)
{
    m_pool.runPending(CHUNK_JOB_BUDGET_MS);
    queueGeneration(playerPosition);
    finishGeneratedChunks();
    processLightQueues(LIGHT_NODE_BUDGET);
    ...
```

It's a no-op on native builds, where real worker threads already drain that
same queue, so it's safe to add unconditionally.

### 2b. The page freezes / "isn't responding"

Emscripten cannot run a blocking `while` loop — it starves the browser's event
loop and the tab wedges. `Application::run()` has to be split into `run()` +
`frame()`, driven by:

```cpp
emscripten_set_main_loop_arg(frameTrampoline, this, 0, 1);
```

See `recovered-source/src/Core/Application.cpp` for the full split.

### 2c. Menus are unclickable

SDL2's web backend reports absolute mouse position accumulated from deltas, so
it drifts — a click at (320,177) arrived as (120,57). Fix is to read the
pointer from the DOM instead. `recovered-source/src/Core/Input.cpp`:

```cpp
int Input::mouseX() const { hookBrowserPointer(); return EM_ASM_INT({ return window.__bcMouseX | 0; }); }
```

There is also a foliage fix worth taking: grass and leaves ship greyscale in a
resource pack and are tinted by biome at draw time. Nothing in this engine
knows about biomes, so without a tint baked in at load they render stone grey.
`foliageTint()` in `recovered-source/src/Renderer/Atlas.cpp`, applied to the
albedo atlas only — never to the normal or specular maps.

## Step 3 — every file in REBUILT

442 tracked files. Here is all of it.

### Engine source — `recovered-source/src/` (64 files)

```
main.cpp

Core/        Application.{h,cpp}   Window.{h,cpp}      Input.{h,cpp}
             ThreadPool.{h,cpp}    GLFunctions.{h,cpp}
             Console.{h,cpp}  *NEW*     Keybinds.{h,cpp}  *NEW*

Renderer/    Atlas.{h,cpp}         AtlasTiles.h        Shader.{h,cpp}
             Mesh.{h,cpp}          Camera.{h,cpp}      Font.{h,cpp}
             Texture.{h,cpp}       UIRenderer.{h,cpp}  SkyRenderer.{h,cpp}
             SelectionRenderer.{h,cpp}                 Screenshot.{h,cpp}
             MobRenderer.{h,cpp}  *NEW*

World/       World.{h,cpp}         WorldGen.{h,cpp}    Chunk.{h,cpp}
             ChunkMesher.{h,cpp}   Noise.{h,cpp}       Block.{h,cpp}
             WorldSave.{h,cpp}

Player/      Player.{h,cpp}        Inventory.{h,cpp}

Entity/      *ENTIRE DIRECTORY IS NEW*
             MobType.{h,cpp}       Mob.{h,cpp}         EntityManager.{h,cpp}
```

`*NEW*` means it does not exist in ORIGINAL at all — copy it wholesale. The
rest need diffing.

### Shaders — `recovered-source/assets/shaders/` (8 files)

`chunk.{vert,frag}` · `sky.{vert,frag}` · `ui.{vert,frag}` · `line.{vert,frag}`

Seven are byte-identical to the deployed build. **`chunk.frag` is the
exception** — deployed is 3724 bytes, REBUILT's is 3264. ORIGINAL's copy is
almost certainly the better one. It already contains a full LabPBR pipeline
(I was wrong about this earlier in my session and corrected it — don't rewrite
it thinking it's missing).

### Build and tooling

```
recovered-source/CMakeLists.txt
recovered-source/build.bat
recovered-source/capture.ps1
recovered-source/web/shell.html              <- rewritten: progress strip, no PLAY gate
recovered-source/tools/build-web.ps1
recovered-source/tools/install-texture-pack.ps1
recovered-source/tools/screenshot.ps1
recovered-source/third_party/stb_image.h
recovered-source/third_party/stb_image_write.h
recovered-source/third_party/stb_vorbis.c
recovered-source/tests/keybind_test.cpp      <- 13 assertions, all passing
recovered-source/tests/README.md
recovered-source/.vscode/{extensions,launch,settings}.json
tools/pack-sounds.py                         <- repacks index.data without a rebuild
```

`tools/pack-sounds.py` is worth understanding: it rewrites the Emscripten
`index.data` blob *and* patches the `loadPackage({files:[...]})` manifest
offsets and `remote_package_size` in `index.js`. I verified it byte-for-byte
reversible. It means sounds can be swapped without recompiling.

### Assets

- `recovered-source/assets/textures/block/` — **131 PNGs.** Primes HD Textures
  32x, with `_n` (LabPBR normal: normal.xy, AO, height) and `_s` (specular:
  smoothness, F0, porosity, emission) maps.
- `recovered-source/assets/sounds/` — **135 .ogg files** across `dig/`, `step/`
  and `mob/{chicken,cow,creeper,pig,sheep,skeleton,spider,zombie}/`.
- `sounds/` at repo root — **75 .ogg**, plus `sounds/README.md` listing all 135
  slots including the mob table.

### Deployed build artifacts at repo root

`index.html` · `index.js` · `index.wasm` · `index.data` · `netlify.toml` ·
`_headers`

These are the *live site*, built from ORIGINAL, not from REBUILT. Useful as a
reference for what ORIGINAL can do — I found the Nether/End/portal/keybinds
features by searching `index.wasm` for strings.

### Documentation

`TODO.txt` (the roadmap, Priority 1–4, kept current) ·
`recovered-source/RECOVERY.md` · `recovered-source/README.md` ·
`recovered-source/assets/textures/README.md` · `sounds/README.md`

## Step 4 — three things Timothy asked for that are NOT built

Don't let these surprise you; they're open requests, not oversights.

1. **Multiplayer.** There is no networking code anywhere. The live build has a
   Multiplayer menu entry that leads nowhere. This is a full project on its
   own — server, protocol, entity sync, chunk streaming. Scope it with him
   before starting.
2. **Skin selection** (bottom-left of the menu). There is no player model and
   no skin system. Needs both built first.
3. **A village in the default panorama.** Villages need structure generation,
   which doesn't exist.

Also open: **should sneak be kept?** It's on Left Ctrl right now and wasn't in
the control scheme Timothy specified. Ask him.

## Step 5 — the licensing problem, because it decides what can ship

Read the bottom of `TODO.txt`. The short version:

- **The sound pack is Mojang's own audio.** The files carry REAPER export tags
  from Mojang's pipeline, encoder dates spanning 2005–2018, and `records/` holds
  the disc music by C418 and Lena Raine. Shipping it on a public site is the
  thing that gets a site taken down. `pack-sounds.py` is a neutral tool and
  will pack whatever it's given — what goes in is Timothy's call, but he should
  make it knowing this.
- **Primes HD Textures 32x** shipped with no licence file. Check its terms.
- Code deps are fine: stb is public domain, SDL2 is zlib, glm is MIT.

## How Timothy works

He tests the build himself and reports back, and he's good at it — three of the
eight blocking bugs I fixed were found by him, not by me. Give him something
runnable rather than a long explanation. He prefers no comments in code. He
asked that nothing be published to Netlify until Priority 1 in `TODO.txt` is
done, then after Priority 2, then after Priority 3.

If something I've written here turns out to be wrong when you look at the
actual code, trust the code and tell him. I was working from a reconstruction,
and I got things wrong twice in my own session — I claimed PBR wasn't
implemented when it was, and claimed swimming was missing when `Player.cpp` has
it. Verify before you rewrite.
