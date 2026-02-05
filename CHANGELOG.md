# CHANGELOG

## [V1.11] – UCASE$/LCASE$ (Phase 1)

- Fixed keyboard LINE INPUT so LINE INPUT A$ correctly reads from the console instead of being 
  misparsed as file input
- Implemented UCASE$ and LCASE$ as core string functions (GW-BASIC compatible).


## [V1.10] – WRITE / INPUT Round-Trip & CSV Semantics

- Fixed Block IF boolean condition parsing

- Added WRITE / WRITE #n for machine-readable output (CSV-style) designed to round-trip with INPUT / INPUT #.
- INPUT / INPUT # now accept doubled quotes (`""`) inside quoted fields (to match GW-BASIC WRITE semantics).
- Added demo coverage for WRITE/INPUT edge cases: empty string, embedded commas in strings, leading/trailing spaces, 
  and exponent/scientific numeric formats.


## [V1.07]
- Changes to eliminate warning during compile. GTK and CLI now compile cleanly.
- Restored the cli alias target to build the headless binary and marked it phony to match README 
  expectations.
- Update existing GTK color tags so preference-based default colors are refreshed when a program 
  runs, ensuring output uses current preference colors.
- Removed the obsolete export speed checkbox from the Preferences dialog and tightened the layout
  by collapsing the extra row spacing around the splash option.
- Fixed issues with Windows exe not supporting dark mode
- Fixed non visible cursor in Windows exe

## [V1.06] – Windows Executable, Embedded Icon, SPEED cleanup
- Embedded Executable Icon
  - icon.png is now embedded into the executable at link time (via ld -r -b binary wbasic_icon_png.o).
  - Eliminated wbasic_icon_png.inc.
  - Makefile now auto-generates wbasic_icon_png.o and links it into both GTK and headless builds.
  - Added an objcopy step to add the note.GNU-stack section (silences the linker warning).

- Windows/MSYS2 portability improvements**
  - Updated code base to support Windows build and execution
  - Added MSYS2/MinGW build and runtime guidance, including PATH setup tips and drive navigation.
  - Added a Windows batch helper to set MSYS2 MinGW PATH entries for a Command Prompt session.
  - Improved Win32 compatibility in headless/export builds (Sleep delay path, stdint include.
  - Guarded POSIX-only headers and headless TTY handling to prevent Windows build errors.

- SPEED command range clarified and tightened
  - SPEED is a WBASIC-specific command.
  - Valid range is now 1 (slowest) to 100 (fastest).
  - Values outside this range are clamped, not rejected:
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
