CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDLIBS  ?= -lm
LDFLAGS ?= -Wl,-z,noexecstack

PKG_GTK  := $(shell pkg-config --cflags --libs gtk+-3.0 glib-2.0)
PKG_GLIB := $(shell pkg-config --cflags --libs glib-2.0)

# Main source file (baseline)
SRC := wbasic.c

ICON_PNG := icon.png
ICON_OBJ := wbasic_icon_png.o

.PHONY: all unified gtk headless cli termux clean fresh

all: unified

# Build icon object (PNG embedded into executable)
# Note: adding alloc+readonly and forcing noexecstack eliminates the linker warning on most systems.
$(ICON_OBJ): $(ICON_PNG)
	ld -r -b binary "$(ICON_PNG)" -o "$(ICON_OBJ)"
	objcopy --add-section .note.GNU-stack=/dev/null \
	        --set-section-flags .note.GNU-stack=contents,alloc,readonly \
	        "$(ICON_OBJ)"

# Unified build (single binary: GTK by default, --cli/--headless switches at runtime)
unified: wbasic

# GTK build (alias)
gtk: wbasic

wbasic: $(SRC) $(ICON_OBJ)
	clear && $(CC) $(CFLAGS) $(SRC) $(ICON_OBJ) -o $@ $(PKG_GTK) $(LDFLAGS) $(LDLIBS)

# Headless / CLI-only build (no GTK libs required)
# We intentionally do NOT link the icon object here to avoid GNU-stack warnings and needless baggage.
headless: wbasic_cli
cli: headless

wbasic_cli: $(SRC)
	clear && $(CC) $(CFLAGS) -DWBASIC_NO_UI $(SRC) -o $@ $(PKG_GLIB) $(LDFLAGS) $(LDLIBS)

# Termux build (ARM64-friendly clang + glib only, no GTK/UI)
termux: CC=clang
termux: CFLAGS+=-DWBASIC_NO_UI
termux: wbasic_termux

wbasic_termux: $(SRC)
	clear && $(CC) $(CFLAGS) $(SRC) -o wbasic $(PKG_GLIB) $(LDFLAGS) $(LDLIBS)

fresh:
	@clear
	@$(MAKE) clean all

clean:
	rm -f wbasic wbasic_cli $(ICON_OBJ)
