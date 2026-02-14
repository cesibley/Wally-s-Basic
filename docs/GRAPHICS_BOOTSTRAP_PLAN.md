# WBASIC Graphics Roadmap (Current Status + Forward Plan)

## Goal
Drive WBASIC graphics toward practical GW-BASIC compatibility while keeping the runtime stable across GTK and CLI/headless targets.

## Current Implementation Status
Implemented and in active use:
- `SCREEN 0`, `SCREEN 1`, `SCREEN 2` mode switching.
- Graphics primitives: `PSET`, `POINT`, `LINE`, `CIRCLE`, `PAINT`.
- `CLS` behavior in graphics modes.
- Text-over-graphics overlay in `SCREEN 1` and `SCREEN 2`.

Supporting compatibility work already landed:
- `SCREEN` optional argument parsing (`SCREEN mode[,arg][,arg][,arg]`, including omitted optional args).
- `COLOR` compatibility improvements (text-mode foreground attribute range support).
- `LINE` optional variant handling (`B`/`BF`, omitted color forms).

## Important Compatibility Notes (Documented Behavior)
1. **`SCREEN 2` color behavior in WBASIC is currently expanded.**
   - WBASIC uses a 16-color graphics index pipeline for `SCREEN 1` and `SCREEN 2`.
   - This is intentionally retained for now because existing WBASIC programs/tests rely on it.
   - Treat this as a **documented superset** of strict historical 2-color semantics.
2. **CLI/headless target is intentionally non-graphics at runtime.**
   - Graphics syntax remains parser-compatible.
   - Graphics execution paths return explicit runtime errors in CLI/headless mode.
3. **Overlay is implemented; remaining work is parity polish.**
   - Focus areas: cursor edge cases, `PRINT`/`LOCATE` interactions, and attribute behavior consistency.

## Milestone Status
Completed:
- **A**: Core parser/dispatcher and graphics state model.
- **B**: GTK graphics render path.
- **C**: Core mode/pixel/clear testing.
- **D**: `LINE` primitive.
- **E**: graphics compatibility hardening for parser/runtime edge cases.
- **F**: `SCREEN 2` support and baseline validation.
- **G**: first major enhancements (`CIRCLE`, `PAINT`, text-over-graphics overlay).

## Forward Plan

### Milestone H — Compatibility Completion Pass (in progress)
1. **Behavior matrix documentation**
   - Create/maintain a command-level matrix for `SCREEN`, `COLOR`, `PSET`, `POINT`, `LINE`, `CIRCLE`, `PAINT`.
   - For each command: mark GW-BASIC parity, WBASIC superset behavior, or intentional divergence.
2. **Regression expansion**
   - Add mixed-sequence torture tests (mode switches + draw + text + color changes).
   - Add negative/omitted-argument parser cases and line-style corner cases.
   - Keep `LINE INPUT` isolation regression coverage strong.
3. **Target-specific expectations**
   - Keep GTK behavior visually stable.
   - Keep CLI/headless errors deterministic and documented.

### Milestone I — Palette/Mode Policy Decision
Choose and lock one project policy for `SCREEN 2` colors:
- **Policy A (strict parity):** enforce 2-color semantics in `SCREEN 2`.
- **Policy B (documented superset):** keep current 16-color behavior in `SCREEN 2`.

Current recommendation: **Policy B** (documented superset), unless a strict-compatibility break is explicitly desired.

## Testing Strategy (ongoing)
- Continue torture/regression tests under `tests/test cases/` for parser/runtime compatibility.
- Add targeted tests for documented divergences so behavior remains intentional and stable.
- Keep GTK smoke/manual checks for overlay and rendering regressions.

## Scope Still Deferred
- `DRAW`, graphics `GET`/`PUT`, `VIEW`, `WINDOW`, `PMAP`.
- Broader mode families and full GW-BASIC mode/palette matrix beyond current compatibility target.
