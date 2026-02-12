# WBASIC vs GW-BASIC Compatibility Audit (Non-Graphics / Non-Drawing / Non-Sound)

## Scope
This audit compares WBASIC's currently implemented feature set against commonly published GW-BASIC references (GW-BASIC User's Guide / command quick-reference lists) while explicitly **excluding**:
- graphics modes and graphics statements/functions,
- drawing commands,
- sound/music commands.

## Method used
1. Inspect WBASIC's statement dispatcher and direct-command handlers in `wbasic.c`.
2. Inspect WBASIC's numeric and string intrinsic function handling in `wbasic.c`.
3. Compare that implemented set to the standard GW-BASIC non-graphics/non-sound command/function families.

## Implemented WBASIC baseline (detected in source)
### Statements / commands present
`AUTO, BEEP, CLEAR, CLOSE, CLS, COLOR, DATA, DEF, DEFDBL, DEFINT, DEFSNG, DEFSTR, DIM, DO/LOOP, ELSE/ELSEIF/ENDIF, END, ERASE, EXIT DO, FIELD, FOR/NEXT, GET, GOSUB, GOTO, IF, INPUT, KEY, LET, LINE INPUT, LIST, LOAD, LOCATE, LSET, NEW, ON, OPEN, OPTION BASE, PRINT, PUT, RANDOMIZE, READ, REDIM, REM, RENUM, RESTORE, RESUME, RETURN, RSET, RUN, SAVE, SEEK, SPEED (WBASIC extension), STOP, SWAP, SYSTEM, TIMER, WEND/WHILE, WIDTH, WRITE`.

### Functions present
- Numeric: `ABS, ASC, ATN, CINT, COS, CVD, CVI, CVS, EOF, ERL, ERR, EXP, FIX, INSTR, INT, LBOUND, LEN, LOF, LOG, PI, RND, SEEK, SGN, SIN, SQR, TAN, TIMER, UBOUND, VAL`.
- String: `CHR$, DATE$, HEX$, LEFT$, LTRIM$, MID$, MKD$, MKI$, MKS$, OCT$, RIGHT$, RTRIM$, SPACE$, SPC, STR$, STRING$, TAB, TIME$, TRIM$, UCASE$, LCASE$, INPUT$, INKEY$`.

## Compatibility gaps found (items not implemented)

> Notes:
> - These are gaps relative to commonly published GW-BASIC references for text/runtime/program/file/control features.
> - Graphics/drawing/sound items are intentionally omitted from this gap list.

### 1) Program editing / environment commands
- Missing: `CONT` (continue after `STOP`/break).
- Missing: `DELETE` (line-range delete command).
- Missing: `EDIT` (line editor command).
- Missing: `LLIST` / `LPRINT` (printer listing/output family).
- Missing: `TRON` / `TROFF` (trace on/off).
-
### 2) Disk and OS/file-management command family
- Missing: `FILES`.
- Missing: `KILL`.
- Missing: `NAME` (rename file).
- Missing: `CHAIN`, `MERGE`.
- Missing (DOS-shell related): `CHDIR`, `MKDIR`, `RMDIR`, `SHELL`.

WBASIC supports core sequential/random file I/O (`OPEN/CLOSE/INPUT#/PRINT#/WRITE#/GET/PUT/FIELD/LSET/RSET/SEEK/LOF/EOF`), but not the broader GW-BASIC DOS command surface.

### 3) Error/trap and event model gaps
- Partially present: `ON ERROR ...`, `RESUME`, `ON KEY`, `ON TIMER`, `TIMER ON/OFF/STOP` are implemented.
- Missing GW-BASIC event families commonly documented in references:
  - `ON PEN`,
  - `ON PLAY`,
  - `ON STRIG`,
  - `ON COM(n)`.

### 4) Memory/machine-level and low-level I/O compatibility gaps
- Missing: `DEF SEG`, `POKE`, `PEEK`, `WAIT`.
- Missing: `INP`, `OUT`.
- Missing binary memory transfer commands: `BLOAD`, `BSAVE`.
- Missing external-call hooks: `CALL`, `USR`.

### 5) Non-graphics intrinsic function gaps (common GW-BASIC references)
The following non-graphics functions are typically listed for GW-BASIC but are not implemented in WBASIC's current function parser:
- `FRE`
- `POS`
- `CSRLIN`
- `LPOS`
- `ENVIRON$`, `ENVIRON`
- `VARPTR`, `VARPTR$`
- `IOCTL$`

### 6) DATA-type conversion / formatting family coverage
WBASIC includes `MKI$/MKS$/MKD$` and `CVI/CVS/CVD`, but does **not** currently expose the full related family often listed in GW-BASIC references:
- Missing: `CVI$`-adjacent byte-string helper variants tied to memory-pointer APIs (because pointer APIs themselves are not present),
- Missing: machine-memory address operators (`VARPTR*`) that usually pair with those workflows.

## Practical compatibility impact summary
- **High impact on old utility/admin programs**: file-management and DOS command gaps (`FILES`, `KILL`, `NAME`, `SHELL`, directory commands).
- **High impact on hardware/port-oriented legacy software**: missing memory/port access (`PEEK/POKE/INP/OUT/WAIT/DEF SEG/BLOAD/BSAVE/CALL/USR`).
- **Medium impact on debugging workflows**: no `TRON/TROFF`, no `CONT`.
- **Low-to-medium impact on pure educational BASIC code**: most core control flow, arrays, strings, numeric math, DATA/READ, and file record I/O are present.

## Recommended implementation order (if GW-BASIC parity is the goal)
1. `CONT`, `DELETE`, `TRON/TROFF`.
2. `FILES`, `KILL`, `NAME`, `CHDIR/MKDIR/RMDIR`, `SHELL`.
3. `FRE`, `POS`, `CSRLIN`, `LPOS`.
4. `DEF SEG`, `PEEK/POKE`, `INP/OUT`, `WAIT`.
5. `BLOAD/BSAVE`, then optional `CALL/USR` compatibility strategy.

