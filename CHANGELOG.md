# CHANGELOG

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
