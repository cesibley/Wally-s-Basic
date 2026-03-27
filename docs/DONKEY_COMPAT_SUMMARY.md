# DONKEY.BAS Compatibility Work Summary (WBASIC core changes only)

This is a concise record of the key compatibility changes made during this debugging cycle to get `DONKEY.BAS` running/rendering correctly in WBASIC.

## 1) Runtime compatibility required by DONKEY
- Added `DEF SEG`, `PEEK`, and `POKE` support with a memory shim (`mem_seg`, `mem_poke_overrides`) in `wbasic.c`.
- Added BIOS-like default byte behavior for uninitialized reads (including equipment-byte compatibility paths used by legacy probes).
- Added no-op compatibility stubs for `SOUND` and `PLAY` so probe calls don’t trigger fatal errors.

## 2) Parser/legacy syntax tolerance
- Accepted swapped relational operators `=>` and `=<`.
- Added tolerant `DRAW` parsing for legacy/unclosed quoted forms used by old listings.

## 3) Graphics command behavior fixes
- `PAINT` updated to accept:
  - `PAINT (x,y)`
  - `PAINT (x,y),color`
  - `PAINT (x,y),color,border`
- Added standalone `PRESET (x,y)[,color]` support.
- Added/adjusted `PUT` action handling (`PSET`, `PRESET`, `OR`, `AND`, `XOR`) and default behavior compatibility for DONKEY flows.

## 4) GET/PUT sprite buffer compatibility
- Reworked GET/PUT packed buffer handling for GW-era layouts.
- Added mode-aware packing (1/2/4/8bpp behavior) and row/header handling compatibility.
- Fixed bit-order/packing edge cases and width handling that caused sprite spill/corruption.
- Added compatibility logic for small handcrafted legacy buffers used by DONKEY road marker animation.

## 5) DRAW and shape rendering alignment
- Corrected DRAW scaling semantics toward GW behavior (`S` handling, movement semantics, `M`/`BM` behavior).
- Iteratively corrected line/move interpretation to match DONKEY sprite construction.

## 6) CGA palette/color behavior
- Added SCREEN 1 color/palette semantics (`COLOR background[,palette]`) and palette state handling.
- Improved SCREEN 1 palette mapping and color-index clamping so car/donkey/fill colors are closer to GW-BASIC output.

## 7) Text/source compatibility
- Added DOS text normalization on load (`CRLF`/`CR` handling, stop at `0x1A` EOF).
- Added DOS encoding on save/export (`CRLF` + trailing `0x1A`).

## 8) 40-column and CP437 display fixes
- Added CP437-to-Unicode conversion for screen text so box characters are not dropped by UTF-8 rendering.
- Added 40-column text presentation on the gfx surface for better retro aspect.
- Added explicit CP437 box-line drawing fallback to reduce broken border segments on splash/title screens.

## 9) Runtime robustness/diagnostics
- Added trapped runtime error diagnostics (`ERL`, `ERR`, message) to identify remaining incompatibilities quickly.
- Fixed control-flow/runtime edge cases (e.g., loop-stack cleanup on jump paths) that caused false exits like `FOR stack overflow`.

---

Scope note: this document intentionally lists only compatibility changes implemented in `wbasic.c`.
