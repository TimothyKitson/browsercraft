#!/usr/bin/env bash
#
# Builds the browser (WebAssembly) version into build-web/, on Linux and
# macOS. The PowerShell script beside this one does the same job on
# Windows; they stage the same assets and pass the same flags, so a page
# built either way is the same page.
#
#   tools/build-web.sh
#   tools/build-web.sh --emsdk ~/emsdk --serve
#
# The web build deliberately ships a lower-resolution texture set: every
# texture is baked into the page download, so a 256x pack would mean tens
# of megabytes before anyone sees a single block.
set -euo pipefail

EMSDK="${EMSDK:-$HOME/emsdk}"
TEXTURE_DIR=""
MOB_TEXTURES=0
SOUND_FILES=0
SERVE=0
PORT=8080

while [ $# -gt 0 ]; do
    case "$1" in
        --emsdk)        EMSDK="$2"; shift 2 ;;
        --textures)     TEXTURE_DIR="$2"; shift 2 ;;
        --mob-textures) MOB_TEXTURES=1; shift ;;
        --sound-files) SOUND_FILES=1; shift ;;
        --serve)        SERVE=1; shift ;;
        --port)         PORT="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
done

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

if [ ! -f "$EMSDK/emsdk_env.sh" ]; then
    echo "Emscripten not found at $EMSDK." >&2
    echo "Clone https://github.com/emscripten-core/emsdk and run:" >&2
    echo "  ./emsdk install latest && ./emsdk activate latest" >&2
    exit 1
fi

# The resource pack is stored at the repository root through Git LFS.
# Import it before staging so a clean web build does not fall back to
# procedural block textures. The PowerShell installer is cross-platform
# when PowerShell 7 (pwsh) is installed.
pack_archive="$root/textures.zip.zip"
if [ -f "$pack_archive" ]; then
    if ! command -v pwsh >/dev/null 2>&1; then
        echo "Found textures.zip.zip, but PowerShell 7 (pwsh) is required to import it." >&2
        echo "Install PowerShell 7, then rerun this build." >&2
        exit 1
    fi

    # A Git LFS pointer is a small text file, not the actual ZIP archive.
    if ! python3 - "$pack_archive" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
with p.open("rb") as f:
    if f.read(4)[:2] != b"PK":
        raise SystemExit(1)
PY
    then
        echo "textures.zip.zip is only a Git LFS pointer. Run 'git lfs pull' in the repository, then build again." >&2
        exit 1
    fi

    pack_signature="$(stat -c '%s:%Y' "$pack_archive" 2>/dev/null || stat -f '%z:%m' "$pack_archive")"
    pack_stamp="$root/.cache/textures-zip-installed.txt"
    installed_signature=""
    [ -f "$pack_stamp" ] && installed_signature="$(cat "$pack_stamp")"

    if [ "$installed_signature" != "$pack_signature" ]; then
        echo "Found textures.zip.zip in the repository. Importing its supported textures and skins..."
        pwsh -NoProfile -ExecutionPolicy Bypass -File "$root/tools/install-texture-pack.ps1" -Zip "$pack_archive"
        block_dir="$root/assets/textures/block"
        block_count=0
        [ -d "$block_dir" ] && block_count="$(find "$block_dir" -type f -iname '*.png' | wc -l | tr -d ' ')"
        if [ "$block_count" -eq 0 ]; then
            echo "The repository archive did not install any block textures." >&2
            exit 1
        fi
        mkdir -p "$(dirname "$pack_stamp")"
        printf '%s' "$pack_signature" > "$pack_stamp"
    else
        echo "Repository texture pack is already imported."
    fi
else
    echo "WARNING: textures.zip.zip was not found at the repository root. The web build may use fallback textures." >&2
fi

# Copy directory contents into the matching runtime asset path. Keeping the
# source directory's contents (rather than nesting its basename) matters:
# Atlas and the entity renderers open exact assets/textures/... paths.
copy_dir_contents() {
    local source="$1" destination="$2"
    [ -d "$source" ] || return 1
    mkdir -p "$destination"
    cp -a "$source/." "$destination/"
}

stage="$root/build-web/stage/assets"
rm -rf "$stage"
mkdir -p "$stage"
cp -r "$root/assets/shaders" "$stage/shaders"

if [ -z "$TEXTURE_DIR" ]; then
    if [ -d "$root/assets/textures/block" ]; then TEXTURE_DIR="$root/assets/textures/block"; fi
fi

if [ -n "$TEXTURE_DIR" ] && [ -d "$TEXTURE_DIR" ]; then
    block_dest="$stage/textures/block"
    copy_dir_contents "$TEXTURE_DIR" "$block_dest"
    block_count="$(find "$block_dest" -type f | wc -l | tr -d ' ')"
    echo "Staging $block_count block texture files from $TEXTURE_DIR ($(du -sh "$block_dest" | cut -f1))"

    # The published build redistributes the art, so preserve its credit.
    if [ -f "$root/assets/textures/CREDIT.txt" ]; then
        cp "$root/assets/textures/CREDIT.txt" "$stage/textures/CREDIT.txt"
        echo "Crediting: $(head -n 1 "$root/assets/textures/CREDIT.txt")"
    else
        echo "WARNING: no assets/textures/CREDIT.txt. Check the pack licence before publishing its textures." >&2
    fi
else
    echo "No block texture pack staged - the web build will use procedural block textures."
fi

# Copy every other texture category the renderer can address (items,
# GUI, entities, colormaps, particles, environment, etc). block32 is a
# legacy staging directory and is not used as the default.
textures_root="$root/assets/textures"
if [ -d "$textures_root" ]; then
    for source in "$textures_root"/*/; do
        [ -d "$source" ] || continue
        category="$(basename "$source")"
        [ "$category" = "block" ] && continue
        [ "$category" = "block32" ] && continue
        destination="$stage/textures/$category"
        if copy_dir_contents "$source" "$destination"; then
            count="$(find "$destination" -type f | wc -l | tr -d ' ')"
            echo "Staging $count $category texture files"
        fi
    done
fi

# Player skins are user content, separate from the vanilla mob sheets.
skin_root="$root/assets/skins"
skin_stage="$stage/skins"
player_skin_count=0
for style in wide slim; do
    source="$skin_root/$style"
    destination="$skin_stage/$style"
    if copy_dir_contents "$source" "$destination"; then
        count="$(find "$destination" -type f | wc -l | tr -d ' ')"
        player_skin_count=$((player_skin_count + count))
    fi
done
if [ -f "$skin_root/custom.png" ]; then
    mkdir -p "$skin_stage"
    cp "$skin_root/custom.png" "$skin_stage/custom.png"
    player_skin_count=$((player_skin_count + 1))
fi
echo "Staging $player_skin_count player skin files"

# Vanilla entity sheets are copyrighted separately from player skins.
# Keep them opt-in for public builds; pass --mob-textures only when the
# installed pack may legally be redistributed. Its credit file travels too.
mob_dir="$skin_root/mob"
if [ "$MOB_TEXTURES" = "1" ] && [ -d "$mob_dir" ]; then
    mob_stage="$skin_stage/mob"
    copy_dir_contents "$mob_dir" "$mob_stage"
    mob_count="$(find "$mob_stage" -type f | wc -l | tr -d ' ')"
    mob_size="$(du -sh "$mob_stage" | cut -f1)"
    if [ -f "$skin_root/shield.png" ]; then
        cp "$skin_root/shield.png" "$skin_stage/shield.png"
    fi
    echo "Staging $mob_count mob texture files ($mob_size)"
    if [ -f "$mob_dir/CREDIT.txt" ]; then
        echo "Mob textures credited: $(head -n 1 "$mob_dir/CREDIT.txt")"
    else
        echo "WARNING: no assets/skins/mob/CREDIT.txt - check the mob pack's redistribution terms." >&2
    fi
elif [ -d "$mob_dir" ]; then
    echo "Mob textures left out of the download (pass --mob-textures only when redistribution is permitted)."
fi

# Audio is optional because the engine synthesizes replacements. Keep
# third-party/vanilla sound packs opt-in for public builds.
sound_dir="$root/assets/sounds"
if [ "$SOUND_FILES" = "1" ] && [ -d "$sound_dir" ]; then
    sound_stage="$stage/sounds"
    copy_dir_contents "$sound_dir" "$sound_stage"
    sound_count="$(find "$sound_stage" -type f | wc -l | tr -d ' ')"
    echo "Staging $sound_count sound files"
    if [ -f "$sound_dir/CREDIT.txt" ]; then
        cp "$sound_dir/CREDIT.txt" "$sound_stage/CREDIT.txt"
    else
        echo "WARNING: no assets/sounds/CREDIT.txt - check the sound pack's redistribution terms." >&2
    fi
elif [ -d "$sound_dir" ]; then
    echo "Sound files left out of the download (pass --sound-files only when redistribution is permitted)."
fi

# shellcheck disable=SC1091
source "$EMSDK/emsdk_env.sh" >/dev/null 2>&1

# Emscripten fetches its own SDL2 port from a GitHub archive the first
# time it links. Behind a proxy that refuses those, clone the matching
# tag and point the port at it instead -- the version is whatever
# upstream/emscripten/tools/ports/sdl2.py asks for:
#
#   git clone --depth 1 --branch release-2.32.10 \
#       https://github.com/libsdl-org/SDL /tmp/SDL-src
#   EMCC_LOCAL_PORTS="sdl2=/tmp/SDL-src" tools/build-web.sh
#
# Nothing to do here when the download works, which is the usual case.

generator=()
command -v ninja >/dev/null 2>&1 && generator=(-G Ninja)

emcmake cmake -S . -B build-web "${generator[@]}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DWEB_ASSET_DIR="$stage"
cmake --build build-web -j "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

echo
echo "Built build-web/index.html"
du -h build-web/index.wasm build-web/index.data 2>/dev/null || true

if [ "$SERVE" = "1" ]; then
    echo "Serving http://localhost:$PORT/ (Ctrl-C to stop)"
    cd build-web && exec python3 -m http.server "$PORT"
fi
