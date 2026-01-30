# CHANGELOG

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
