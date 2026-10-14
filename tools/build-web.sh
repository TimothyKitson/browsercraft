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
SERVE=0
PORT=8080

while [ $# -gt 0 ]; do
    case "$1" in
        --emsdk)        EMSDK="$2"; shift 2 ;;
        --textures)     TEXTURE_DIR="$2"; shift 2 ;;
        --mob-textures) MOB_TEXTURES=1; shift ;;
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

# Stage the assets that get baked into the download.
stage="$root/build-web/stage/assets"
rm -rf "$stage"
mkdir -p "$stage"
cp -r "$root/assets/shaders" "$stage/shaders"

if [ -z "$TEXTURE_DIR" ]; then
    if   [ -d "$root/assets/textures/block32" ]; then TEXTURE_DIR="$root/assets/textures/block32"
    elif [ -d "$root/assets/textures/block" ];   then TEXTURE_DIR="$root/assets/textures/block"
    fi
fi

if [ -n "$TEXTURE_DIR" ] && [ -d "$TEXTURE_DIR" ]; then
    mkdir -p "$stage/textures"
    cp -r "$TEXTURE_DIR" "$stage/textures/block"
    echo "Staging textures from $TEXTURE_DIR ($(du -sh "$stage/textures/block" | cut -f1))"

    # The published build is the one that actually redistributes the art,
    # so the credit has to go with it or the title screen credits nobody.
    if [ -f "$root/assets/textures/CREDIT.txt" ]; then
        cp "$root/assets/textures/CREDIT.txt" "$stage/textures/CREDIT.txt"
        echo "Crediting: $(head -n 1 "$root/assets/textures/CREDIT.txt")"
    else
        echo "WARNING: no assets/textures/CREDIT.txt - the published build will credit nobody." >&2
        echo "WARNING: most packs are free to redistribute only if their author is credited." >&2
    fi
else
    echo "No texture pack staged - the web build will use procedural textures."
fi

# Mob sheets are a separate decision from block textures. A block pack is
# usually somebody's own work, passed on under their own terms; the
# vanilla entity textures most people install here are Mojang's, and
# putting them on a public site is republishing Mojang's art. So they
# stay out of the download unless you say otherwise, and the game falls
# back to the hides it paints for itself.
mobDir="$root/assets/skins/mob"
if [ "$MOB_TEXTURES" = "1" ] && [ -d "$mobDir" ]; then
    mkdir -p "$stage/skins"
    cp -r "$mobDir" "$stage/skins/mob"
    [ -f "$root/assets/skins/shield.png" ] && cp "$root/assets/skins/shield.png" "$stage/skins/shield.png"
    echo "Staging mob textures ($(du -sh "$stage/skins" | cut -f1))"
    echo "WARNING: publishing mob textures. Make sure they are yours to pass on - the vanilla ones are not." >&2
elif [ -d "$mobDir" ]; then
    echo "Mob textures left out of the download (pass --mob-textures to include them)."
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
