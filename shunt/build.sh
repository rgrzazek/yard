#!/usr/bin/env bash
# Build the core to a standalone wasm (no JS glue) and drop it
# where Spring serves it. The loader is static/shunt/shunt.js; the page is
# templates/shunt.html. View at /shunt under ./gradlew bootRun.
#
#   --no-entry            no main(); the browser drives it
#   -sSTANDALONE_WASM     self-contained module, no emscripten runtime
#   -sEXPORTED_FUNCTIONS  the boundary the browser is allowed to call
#
# Debugging C? Swap to a glue build for printf + bounds checks:
#   emcc shunt.c -O0 -sASSERTIONS=2 -sEXPORTED_RUNTIME_METHODS=HEAPU8 -o shunt.js
set -e
cd "$(dirname "$0")"
out=../src/main/resources/static/shunt/shunt.wasm
emcc shunt.c -O3 --no-entry -sSTANDALONE_WASM \
  -sEXPORTED_FUNCTIONS=_fb_width,_fb_height,_framebuffer,_sprite_ptr,_set_sprite_size,_init,_tick,_click,_clear_tiles \
  -o "$out"
echo "Built $out ($(wc -c < "$out" | tr -d ' ') bytes)"
