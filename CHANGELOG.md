# CHANGELOG

## [V1.11] – Extensive Testing, Compatibility GAP and Bug Squish

- Testing and Verification Added
  - Added tests directory with "Torture" tests, and run_all.sh to test for GW-BASIC
    compatibility, as for refression testing.
  
- IF / Expression / Boolean Semantics
  - Fixed single-line IF parsing rejecting valid expressions with logical operators.
  - Unified block IF and single-line IF to use the same full boolean expression parser.
  - Corrected NOT semantics to match GW-BASIC (NOT x = -(x+1)).
  - Fixed boolean operator precedence (NOT > AND > OR).
  - Corrected bitwise XOR implementation.
  - Ensured relational operators return -1 (true) or 0 (false).

- Expression Evaluation
  - Fixed unary minus vs exponent precedence (-2^2 = -4, (-2)^2 = 4).
  - Corrected integer division operator (\) semantics and sign behavior.
  - Fixed MOD operator sign rules to match GW-BASIC.
  - Ensured mixed numeric/string expression errors report correct ERR/ERL.

- DATA / READ / RESTORE
  - Fixed DATA stream ordering to be static and independent of control flow.
  - Ensured DATA inside skipped IF, GOTO, GOSUB, FOR, and unreachable code is still counted.
  - Fixed DATA in loops being counted once, not per iteration.
  - Corrected RESTORE <line> to reposition DATA pointer correctly.
  - Fixed blank numeric DATA fields yielding 0.
  - Corrected doubled-quote unescaping in DATA strings.
  - Fixed Out of DATA error reporting (ERR=4) and ERL accuracy.

- INPUT / LINE INPUT / WRITE
  - Fixed OPEN to accept filename expressions (variables), not just string literals.
  - Corrected INPUT # handling of blank fields.
  - Fixed LINE INPUT # raw fidelity vs PRINT #.
  - Corrected WRITE/INPUT round-trip quoting and numeric formatting.
  - Ensured WRITE always emits a newline.
  - Preserved GW-BASIC distinction between PRINT # zones and WRITE # literal commas.

- Strings
  - Fixed string relational operators (< > = <= >= <>) behavior.
  - Fixed MID$ assignment bounds checking and error reporting.
  - Prevented MID$ assignment to string literals.
  - Corrected UCASE$ / LCASE$ behavior in expressions and assignments.
  - Fixed string comparisons to follow GW-BASIC collation rules.

- Arrays / OPTION BASE
  - Fixed auto-dimension default lower bounds.
  - Corrected typed array handling.
  - Enforced OPTION BASE placement rules (must appear before array creation).
  - Ensured OPTION BASE errors match GW-BASIC behavior.

- FOR / NEXT
  - Fixed NEXT wrong-variable error ERL reporting.
  - Corrected NEXT var-list handling (NEXT J,I).
  - Ensured STEP 0 throws correct error.
  - Fixed loop variable modification behavior.
  - Corrected GOTO escape behavior from FOR loops.

- WHILE / WEND
  - Fixed condition re-evaluation timing (true N times plus one false).
  - Corrected nested WHILE/WEND matching.
  - Ensured WEND without WHILE errors correctly.
  - Fixed statement chaining (:) inside WHILE bodies.

- GOTO / GOSUB / RETURN
  - Fixed GOSUB stack handling in deeply nested flows.
  - Corrected RETURN without GOSUB error code (ERR=3).
  - Fixed GOTO escaping from inside GOSUB without corrupting return stack.
  - Ensured deterministic mixed GOTO/GOSUB flow paths.

- DEF FN
  - Fixed precedence and boolean semantics inside DEF FN bodies.
  - Ensured undefined FN usage raises correct error.
  - Fixed type mismatch errors for string vs numeric FN calls.
  - Corrected ERR/ERL reporting for DEF FN errors.

- Error Handling
  - Fixed ON ERROR GOTO replacement semantics (new handler overrides old).
  - Corrected error handling inside GOSUB while preserving RETURN stack.
  - Fixed RESUME, RESUME NEXT, and RESUME <line> flow behavior.
  - Corrected ERR/ERL values for divide-by-zero and type mismatch.

- Random Access File I/O
  - Implemented GW-BASIC-correct OPEN "R" with mandatory LEN.
  - Fixed FIELD / LSET / RSET fixed-width padding.
  - Corrected PUT/GET record positioning.
  - Fixed LOF(#) and SEEK # behavior.
  - Ensured random access I/O does not break sequential I/O afterward.

- ERASE / SWAP
  - Fixed ERASE to fully reset arrays for re-DIM.
  - Corrected SWAP behavior for scalars, strings, and array elements.
  - Fixed SWAP ambiguity when scalar and array share a name.
  - Corrected SWAP type mismatch error code.

- TIMER / ON TIMER
  - Implemented ON TIMER event handling.
  - Added TIMER ON / OFF / STOP semantics.
  - Prevented timer handler re-entrancy.
  - Ensured timer events coexist safely with RUN/STOP.

- SYSTEM / END / STOP
  - Implemented SYSTEM to behave identically to END.
  - Ensured STOP halts execution and returns to OK prompt.
  - Fixed termination semantics consistency.

- Parser / Syntax Robustness
  - Fixed colon statement splitting to ignore colons inside quotes.
  - Fixed REM and apostrophe comments ignoring subsequent colons.
  - Ensured keywords inside strings are never parsed.
  - Fixed trailing colon creating empty statements.
  - Corrected THEN/ELSE chaining and newline parsing.
  - Fixed DATA parsing with embedded colons and commas inside quoted fields.

- Numeric Edge Cases
  - Fixed INT, FIX, and SGN edge semantics.
  - Corrected integer division (\) behavior.
  - Fixed bitwise AND, OR, XOR truth tables.
  - Ensured floating-point tolerance comparisons behave correctly.
  - Fixed division-by-zero trapping and ERL accuracy.

- DATA Placement + Flow (Deep)
  - Ensured DATA after END remains readable.
  - Fixed DATA ordering through GOTO, GOSUB, FOR, and RETURN.
  - Ensured RESTORE inside loops replays DATA correctly.
  - Fixed unreachable DATA still contributing to the DATA stream.

- Fixed keyboard LINE INPUT so LINE INPUT A$ correctly reads from the console instead of being 
  misparsed as file input

- Implemented UCASE$ and LCASE$ as core string functions (GW-BASIC compatible).

- Fixed single-line IF condition parsing when the condition begins with parentheses (e.g., IF (A*2)+B=20 THEN ...).


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
