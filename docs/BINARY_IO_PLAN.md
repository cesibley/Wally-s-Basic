# WBASIC Binary / Random-Access File I/O — Phase 0 (Scope & Rules)

Date: 2026-02-04

This document captures the **agreed scope decisions** for implementing GW-BASIC style binary and random-access I/O
in WBASIC, before any functional coding begins.

## Goals

Primary goal (GW-BASIC compatible):
- Implement **Random-access files** and the classic record-buffer workflow used for binary-ish data:
  - `OPEN ... FOR RANDOM AS #n LEN = <reclen>`
  - `FIELD #n, <len> AS <var$>, ...`
  - `LSET` / `RSET`
  - `GET #n, rec` / `PUT #n, rec`
  - `SEEK` / `LOF`
  - `INPUT$`
  - `MKI$ / MKS$ / MKD$` and `CVI / CVS / CVD`
  - optional: `LOCK / UNLOCK`

Secondary (optional, later):
- Consider adding QBasic-style `OPEN ... FOR BINARY` convenience mode **after** GW-BASIC random files are complete.

## Compatibility rules (locked)

### Byte semantics
- Strings are treated as **byte containers** for binary I/O features:
  - No encoding conversions are applied.
  - Operations like `INPUT$`, `FIELD`, `MKI$` etc. operate on raw bytes.
- Binary packing/unpacking uses **little-endian** byte order (GW/QB convention on common platforms).

### Numeric packing
- `MKI$` packs a 16-bit signed integer into 2 bytes.
- `MKS$` packs an IEEE-754 single into 4 bytes.
- `MKD$` packs an IEEE-754 double into 8 bytes.
- `CVI/CVS/CVD` reverse the above.
- These functions accept/return **byte strings** (may contain NUL bytes).

### Random file model
- `OPEN ... FOR RANDOM ... LEN = N` creates a handle with record length `N` bytes.
- `GET #n, rec` and `PUT #n, rec` address records using **1-based** record numbers.
- Record addressing maps to byte offsets as:
  - `offset = (rec - 1) * reclen`
- `FIELD` attaches a record buffer of size `reclen` and maps slices into string variables.
  - `LSET/RSET` writes into those slices (space padded / truncated) in GW-BASIC style.

### Cross-platform constraint
- All language functionality is implemented in the shared **core**; platform wrappers are only for physical I/O where needed.
  (GTK and headless builds must both benefit automatically.)

## Out of scope for Phase 0
- No functional code changes required beyond documentation and release notes.
- No new syntax implemented in this phase.

## Open questions (to resolve during implementation)
- Exact behavior when `GET` reads beyond EOF:
  - Should the record be filled with `CHR$(0)` bytes, spaces, or leave existing buffer contents?
- Exact `SEEK` semantics in sequential vs random modes:
  - For RANDOM files, `SEEK #n, rec` should be record-based.
  - For sequential/binary stream convenience, consider byte-based seek.
- Do we require `LEN=` for `OPEN RANDOM`, or default to 128 (GW-BASIC default)?

## Acceptance tests (planned)
A minimal regression program will:
- Create a random file with LEN=16
- FIELD the record as numeric and string slices
- Use MKI$/MKD$ to pack values
- PUT and GET records
- Validate round-trip equality
