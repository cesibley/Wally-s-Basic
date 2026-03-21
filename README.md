# Wally's Basic (WBASIC)

WBASIC is a GW-BASIC–style interpreter with a GTK3 user interface, focused on
faithful classic BASIC behavior and a portable C codebase.  Currently text
only.  Graphic commands are in planning.

The project is primarily for Linux and requires GTK and Glib.  It does compile and run under Windows with MSYS2, but it is not as fully tested.

## Version
**1.20** — February 13, 2026

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

## Limitations and differences
- Text mode only.  Graphics modes in planning
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
