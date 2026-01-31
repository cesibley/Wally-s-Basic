# CHANGELOG

## Updates

- **SPEED command range clarified and tightened**
  - SPEED is a WBASIC-specific command.
  - Valid range is now **1 (slowest) to 100 (fastest)**.
  - Values outside this range are **clamped**, not rejected:
    - Values < 1 are treated as SPEED 1.
    - Values > 100 are treated as SPEED 100.
  - Documentation updated in the reference manual to reflect this behavior.

---

## [V1.05] – GW-BASIC Compatibility & Core Semantics

### Added
- Auto-dimensioning of arrays (GW-BASIC compatible):
  - Arrays are implicitly DIM’d on first subscripted use.
  - Default bounds are 0 TO 10 per dimension.
  - Dimension count inferred from first use.
  - Applies to both numeric and string arrays.
  - Works through READ/DATA.
  - Works through MID$ l-value assignment on array elements.
  - DIM after implicit creation correctly raises an error.

- RUN statement implemented:
  - RUN is now a true statement (not parsed as an assignment).
  - Legal in statement chains (e.g. `CLS:RUN`).
  - RUN fully resets program state:
    - Numeric variables reset to 0.
    - String variables reset to "".
    - Arrays cleared and deallocated.
    - FOR / WHILE / GOSUB stacks cleared.
    - DEFxxx functions reset.
    - OPTION BASE reset and unlocked.
    - DATA pool rebuilt and READ pointer reset.

### Improved
- Legacy programs relying on `CLS:RUN` now execute correctly.
- STARTREK.bas added as a reference compatibility demo.

---

## [V1.04] – Skipped

---

## [V1.03] – GW-BASIC Compatibility Fixes

### Fixed
- Relational expressions now return GW-BASIC truth values (-1 for TRUE, 0 for FALSE) in numeric expressions.
- IF conditions now accept numeric expressions as truth tests.
- Scalar variables and arrays with the same name (e.g. S and S()) can coexist, matching GW-BASIC behavior.

---

## [V1.02] – Export + Demo Updates

### Fixed
- Export now uses baseline (CLI/Preferences) speed, not the last runtime SPEED value.

### Added
- Added demos/bench.bas.

---

## [V1.01] – Headless Rendering Fixes

### Fixed
- LOCATE no longer resets color in headless mode.
- Correct mid-line COLOR behavior.
- Removed unnecessary ANSI resets during color application.

### Improved
- Proper newline handling to prevent color banding.
- Automatic color reassert after structural resets.

### Preserved
- CLS remains a hard screen reset.

---

## [V1.00] – Initial Stable Release
- Core GW-BASIC compatible language.
- GTK interpreter and headless exporter.
