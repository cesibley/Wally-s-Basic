# WBASIC Graphics Bootstrap Plan (Minimal First Implementation)

## Goal
Add an initial graphics capability with low risk to the existing interpreter by introducing:
1. a single graphics mode command,
2. a pixel plot primitive,
3. an optional readback function,
4. clear compatibility behavior when graphics are unavailable.

## Recommended v1 Feature Slice

### 1) `SCREEN 1` and `SCREEN 0`
- `SCREEN 1` enters a basic graphics mode backed by an in-memory pixel buffer.
- `SCREEN 0` returns to current text mode behavior.
- Keep text mode fully intact so existing programs and tests remain stable.

### 2) `PSET (x,y)[,color]`
- Plot one pixel at `(x,y)` in current foreground color (or explicit `color`).
- Clamp/ignore out-of-range coordinates in a GW-BASIC-compatible way (decide once and document).

### 3) `POINT(x,y)`
- Return color index at pixel `(x,y)`.
- This enables validation tests and simple graphics algorithms quickly.

### 4) `CLS` behavior
- In graphics mode: clear pixel buffer to background color.
- In text mode: preserve current text-screen clear behavior.

## Suggested Internal Architecture

### A) Add a mode flag and graphics state to `App`
- `video_mode` enum: text vs graphics.
- `gfx_width`, `gfx_height`.
- `uint8_t *gfx_pixels` color-index buffer.
- `gfx_palette[16]` mapped to RGB for GTK rendering.

### B) Keep interpreter and renderer loosely coupled
- Statement handlers should mutate model state only.
- GTK/UI code should render from the model on idle/invalidation.
- For CLI/headless builds, keep a no-op renderer but retain state for tests.

### C) Rendering strategy for first cut
- Use a `GtkDrawingArea` for graphics mode.
- Draw pixels by converting the color-index buffer to RGB in `draw` callback.
- Start with nearest-neighbor scaling to fit widget size.

## Why this order
- `SCREEN` + `PSET` gives immediate visible progress.
- `POINT` makes deterministic tests possible without screenshot-only validation.
- Avoids complexity of line/circle/fill algorithms initially.

## Compatibility Notes (Deferred)
Defer these until v2+:
- `LINE`, `CIRCLE`, `PAINT`, `DRAW`, `GET`/`PUT` (graphics variants), `VIEW`, `WINDOW`, `PMAP`.
- Multiple graphics resolutions/pages.
- Full GW-BASIC palette and mode matrix.

## Text + Graphics Overlay (Answer to common question)
- **Not in v1.** This bootstrap plan assumes mutually exclusive modes:
  - `SCREEN 0` => text-mode renderer/path,
  - `SCREEN 1` => graphics renderer/path.
- If text-on-graphics is desired later, implement it as a v2/v3 feature using one of:
  1. a software text layer composited over the graphics pixel buffer, or
  2. a dual-pane/overlay widget strategy in GTK with shared cursor semantics.
- Recommendation: keep v1 mode-exclusive to reduce parser/runtime risk and avoid breaking existing text behavior.

## If you want GW-BASIC-style text over graphics, start with these screens
- For classic baseline behavior, prioritize overlay text support in:
  - `SCREEN 1` (320x200, 4-color class),
  - `SCREEN 2` (640x200, 2-color class).
- Practical WBASIC rollout:
  1. Keep v1 as-is (`SCREEN 1` graphics-only).
  2. Add overlay text in `SCREEN 1` first (simpler column geometry).
  3. Add `SCREEN 2` with matching `PRINT`/`LOCATE` semantics once stable.
- Defer broader mode families (`SCREEN 7+`-style variants) until parser/runtime behavior is locked.

## Proposed Milestones
1. **Milestone A**: parser/dispatcher support for `SCREEN`, `PSET`, `POINT` + data model in `App`.
2. **Milestone B**: GTK drawing-area render path for graphics mode.
3. **Milestone C**: tests for mode switching, pixel set/read, and `CLS` in graphics mode.
4. **Milestone D**: optional first shape primitive (`LINE`).

## What is next after Milestone D?
With `SCREEN`/`PSET`/`POINT`/`CLS` and `LINE` now available, the next highest-value work is to stabilize behavior and then expand mode coverage.

### Milestone E (recommended now): harden graphics compatibility
1. **Parser/runtime compatibility pass for `LINE` variants**
   - Support the commonly used optional forms incrementally (e.g. omitted color defaulting).
   - Keep `LINE INPUT` behavior isolated and regression-tested.
2. **Edge-case test expansion**
   - Negative/out-of-bounds coordinates for `PSET`, `POINT`, and `LINE`.
   - Degenerate line case (`x1=x2` and `y1=y2`).
   - Repeated `SCREEN 0 -> SCREEN 1 -> SCREEN 0` transitions to validate allocation/free paths.
3. **CLI/headless confidence checks**
   - Confirm graphics state mutation remains testable without GTK rendering.

### Milestone F: add `SCREEN 2` (640x200, 2-color)
1. Add mode/state configuration for a second graphics resolution.
2. Reuse existing drawing-area infrastructure with dynamic dimensions.
3. Add focused tests for mode dimensions, `PSET`/`POINT`, and `CLS` behavior in `SCREEN 2`.

#### Milestone F validation target
- Add a dedicated `SCREEN 2` torture test covering:
  - 640x200 boundary `PSET`/`POINT` checks,
  - out-of-range `POINT` returns `-1`,
  - `CLS` clears `SCREEN 2` pixels,
  - `SCREEN 2 -> SCREEN 0` transition remains stable.

### Milestone G: choose first user-visible enhancement
Pick one path and ship it fully tested before broadening scope:
- **Path A**: `CIRCLE` primitive.
- **Path B**: `PAINT` flood fill.
- **Path C**: text-over-graphics overlay in `SCREEN 1`, then `SCREEN 2`.

Recommendation: complete **Milestone E** first, then **Milestone F**, then decide between `CIRCLE` and overlay based on user program demand.

#### Milestone E validation target
- Add an edge-case torture test covering:
  - `PSET` default color behavior when current text color is unset/default,
  - degenerate `LINE` (`(x,y)-(x,y)`),
  - partially out-of-bounds `LINE` clipping behavior,
  - repeated `SCREEN 0` <-> `SCREEN 1` transitions.

## Testing Approach
- Add unit-style parser/execution checks in headless mode for:
  - `SCREEN 1` sets graphics mode,
  - `PSET` changes expected pixel,
  - `POINT` returns expected index,
  - `CLS` clears graphics buffer.
- Add one GTK smoke test/manual demo BASIC program that plots a pattern.

## Minimal BASIC demo target
```basic
10 SCREEN 1
20 FOR Y = 0 TO 99
30 FOR X = 0 TO 159
40 C = (X + Y) MOD 16
50 PSET (X,Y), C
60 NEXT X
70 NEXT Y
80 END
```
