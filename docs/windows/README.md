# Windows build (MSYS2 / MinGW)

This project is already GCC + pkg-config + GTK/GLib based, so the most direct
Windows path is MSYS2 with the MinGW toolchain (UCRT64 or MINGW64). These steps
leave the existing Linux build flow untouched.

## 1) Install MSYS2
Download and install MSYS2 from https://www.msys2.org/. Follow their update
instructions after install.

## 2) Open the MinGW shell
Use **MSYS2 UCRT64** (recommended) or **MSYS2 MINGW64** from the Start Menu.

## 3) Install dependencies
In the MSYS2 shell:

### UCRT64
```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-pkg-config \
  mingw-w64-ucrt-x86_64-gtk3 \
  mingw-w64-ucrt-x86_64-glib2 \
  make
```

### MINGW64 (alternative)
```sh
pacman -S --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-gtk3 \
  mingw-w64-x86_64-glib2 \
  make
```

## 4) Build
From the repo root:

### GTK UI build
```sh
make gtk
```

### Headless build (no GTK)
```sh
make headless
```

## 5) Run
```sh
./wbasic
```

## MSYS2 drive navigation
- In MSYS2 shells, Windows drives are mounted under `/c`, `/d`, etc.
  Example: `cd /d/projects/my-repo`
- If you are using a standard Windows Command Prompt (outside MSYS2),
  use `D:` to switch drives, then `cd` to change directories.

## Notes
- If you only need the CLI, use the headless build to avoid GTK.
- The MinGW toolchain uses the same Makefile and GCC flow as Linux, so no
  separate Windows-specific build system is required.
- If you see `libcairo-2.dll`/`libcairo.dll` missing at runtime, make sure
  you are launching the program from the MSYS2 MinGW shell so it can find
  the GTK runtime DLLs, or add the MSYS2 MinGW `bin` directory to your PATH:
  - UCRT64: `C:\\msys64\\ucrt64\\bin`
  - MINGW64: `C:\\msys64\\mingw64\\bin`
- You can also run `tools\\windows\\msys2_env.bat` to set PATH for the current
  Command Prompt session (edit the MSYS2 root inside the script if needed).

