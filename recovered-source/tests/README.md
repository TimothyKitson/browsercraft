# Tests

Small, dependency-free checks built with Emscripten and run under node, so
they need nothing the web build does not already need.

```bash
source /path/to/emsdk/emsdk_env.sh
em++ -std=c++17 -sUSE_SDL=2 -fexceptions -sENVIRONMENT=node -sNODERAWFS=1 \
     -Isrc -Ithird_party -Ibuild-web/_deps/glm-src \
     tests/keybind_test.cpp src/Core/Keybinds.cpp -o /tmp/keybind_test.js
node /tmp/keybind_test.js
```

`keybind_test.cpp` covers the keymap: the defaults, a file overriding some
bindings while absent ones keep theirs, malformed lines being skipped, a
save/load round trip across all 21 actions, a missing file leaving defaults
alone, and reset. All 13 assertions pass.
