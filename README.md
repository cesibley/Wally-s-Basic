# Wally's Basic (WBASIC)

WBASIC is a GW-BASIC–style interpreter with a GTK3 user interface, focused on
faithful classic BASIC behavior and a portable C codebase, including
text and graphics command support.

The project is primarily for Linux and requires GTK and Glib.  It does compile and run under Windows with MSYS2, but it is not as fully tested.

## Version
**V2.0** — March 21, 2026

## Features
- Nearly all GW-BASIC Structures, Commands and Funtions are implemented and tested
  - GW-BASIC–style syntax and semantics
  - Line-numbered BASIC programs
  - IF / THEN / ELSE, FOR/NEXT, WHILE/WEND, DO/LOOP
  - ON ERROR / RESUME / RESUME NEXT
  - DEFINT / DEFSNG / DEFDBL / DEFSTR
  - DATA / READ / RESTORE
  - CLEAR, STOP, END, SYSTEM
  - Etc.
- GTK3 graphical interface
- Headless (no-UI) build option
- Graphics commands currently implemented in `wbasic.c`
  - `SCREEN` modes `0, 1, 2, 3, 7, 8, 9, 10, 11, 12, 13`
  - `PSET`
  - `LINE` including plain lines plus `B` and `BF` box modes
  - `CIRCLE`
  - `PAINT` with optional border color
  - `DRAW` string graphics with movement, angle, color, scale, blank, and no-update controls
  - `GET` and `PUT` for graphics array capture/blitting, with `PSET`, `OR`, `AND`, and `XOR` actions on `PUT`
  - `POINT(x,y)` function for reading a pixel color
  - `CLS` clears graphics screens, and `COLOR` supplies the current drawing color used by commands that omit an explicit color

## Limitations and differences
- Graphics are available only in the GTK UI build; CLI/headless mode reports graphics as unavailable
- `SCREEN` accepts the common optional arguments for compatibility, but currently ignores `colorburst`, `apage`, and `vpage`
- `LINE` parses style arguments for compatibility, but patterned line styles are not yet applied
- `CIRCLE` currently supports center, radius, and optional color; aspect, start/end angles, and other extended GW-BASIC options are not yet implemented
- `PAINT` implements fill color and optional border color
- `GET`/`PUT` use numeric BASIC arrays as a simple width/height + pixel buffer format
- Files may be "exported" to a Linux executable.  Run speeds increased by ~10x
- SPEED directive added to slow down screen prints to throttle older programs

## Building
GTK build:
```sh
make gtk
```

Headless build (requires GLib):
```sh
make cli
```

## Windows (MSYS2)
See `docs/windows/README.md` for MSYS2/MinGW setup and build steps.

## Running
```sh
./wbasic
```

## Demos
See the `demo/` directory for example BASIC programs

## License
MIT License


## Documentation
- `docs/GWBASIC_COMPAT_AUDIT.md` — GW-BASIC compatibility audit and behavioral notes
