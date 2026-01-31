# Wally's Basic (WBASIC)

WBASIC is a GW-BASIC–style interpreter with a GTK3 user interface, focused on
faithful classic BASIC behavior and a clean, portable C codebase.  Currently text
only.  Graphic command are in planning.  

## Version
**1.05** — January 30, 2026

## Features
- GW-BASIC–style syntax and semantics
- Line-numbered BASIC programs
- IF / THEN / ELSE, FOR/NEXT, WHILE/WEND, DO/LOOP
- ON ERROR / RESUME / RESUME NEXT
- DEFINT / DEFSNG / DEFDBL / DEFSTR
- DATA / READ / RESTORE
- CLEAR, STOP, END, SYSTEM
- GTK3 graphical interface
- Headless (no-UI) build option

## Limitations and differences
- Text mode only
- Files may ve "exported" to a Linux executable.  Run speeds increased by ~10x

## Building
GTK build:
```sh
clear && gcc wbasic.c -o wbasic $(pkg-config --cflags --libs gtk+-3.0) -lm
```

Headless build (requires GLib):
```sh
clear && gcc -DWBASIC_NO_UI wbasic.c -o wbasic \
  $(pkg-config --cflags --libs glib-2.0) -lm
```

## Running
```sh
./wbasic
```

## Demos
See the `demo/` directory for example BASIC programs, including a full V1.0 smoke test.

## License
MIT License
