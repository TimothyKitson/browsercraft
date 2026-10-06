# Sounds

Drop your own `.ogg` files in here, run the packer, and the game picks them up.

```bash
python3 tools/pack-sounds.py --check   # show what would change
python3 tools/pack-sounds.py           # rebuild index.data + index.js
```

## Why a packer and not just a folder

Browsercraft ships as a prebuilt WebAssembly bundle. Every asset lives inside
`index.data` as a flat blob, and `index.js` holds a manifest of byte offsets
into it. Replacing a sound shifts every offset after it, so the two files have
to be rewritten together. That is all `tools/pack-sounds.py` does.

Editing `index.data` by hand will corrupt the bundle. The packer is reversible:
restore a file in here, run it again, and you are back to the previous build.

## Format

| Property | Value |
|---|---|
| Container | Ogg Vorbis (`.ogg`) |
| Channels | Mono — the engine positions sounds in 3D, stereo files break panning |
| Sample rate | 44100 Hz |
| Length | 0.2s–0.5s for steps and digs |
| Peak | Normalize to about -3 dBFS so volumes match across materials |

Convert anything to the right format with:

```bash
ffmpeg -i input.wav -ac 1 -ar 44100 -c:a libvorbis -q:a 5 output.ogg
```

## The complete list

These 75 paths are the only ones the engine loads. The numbered variants are
chosen at random each time the sound plays, and **the counts are compiled into
`index.wasm`** — adding `stone5.ogg` to a group that ends at 4 does nothing, and
the packer will tell you it was ignored.

### `dig/` — breaking a block (4 variants each)

`cloth1-4` `grass1-4` `gravel1-4` `sand1-4` `stone1-4` `wood1-4`

Cloth covers wool and beds, grass covers dirt and leaves, stone covers ore and
metal, wood covers planks and logs.

### `step/` — footsteps (6 variants each)

`cloth1-6` `grass1-6` `gravel1-6` `sand1-6` `snow1-6` `stone1-6` `wood1-6`

Quieter and shorter than the matching `dig/` sound — these fire constantly while
walking, so anything with a sharp transient gets fatiguing fast.

### `damage/` — taking damage

| File | Plays when |
|---|---|
| `hit1.ogg` `hit2.ogg` `hit3.ogg` | The player is hurt (3 variants) |
| `fallsmall.ogg` | Landing hard enough to take fall damage |

### `random/` — everything else

| File | Plays when |
|---|---|
| `glass1.ogg` `glass2.ogg` `glass3.ogg` | Glass breaks (3 variants) |
| `click.ogg` | Button and UI click |
| `pop.ogg` | An item is picked up |

## A note on sourcing

Whatever you put in here ships to every player on browsercraft.net, so it needs
to be audio you have the right to distribute — your own recordings, or something
under a license that permits redistribution (CC0 and CC-BY both do). Audio
extracted from another game does not qualify, however the files are named.
Freesound's CC0 pool and OpenGameArt are the usual sources if you want a
shortcut.
