# Wally's Basic (WBASIC)

WBASIC is a GW-BASIC–style interpreter with a GTK3 user interface, focused on
faithful classic BASIC behavior in a C codebase.  V1.0 currently implements
text only output.  GW-Basic graphics are planned for V2.0

## Version
**1.0** — January 29, 2026

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
