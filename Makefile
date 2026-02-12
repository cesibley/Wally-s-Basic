CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDLIBS  ?= -lm
LDFLAGS ?=

# GNU ld on Linux understands -z,noexecstack; MinGW/Windows linkers do not.
# Detect MinGW toolchains by compiler target triplet as they may run from non-Windows hosts.
CC_TARGET := $(shell $(CC) -dumpmachine 2>/dev/null)
ifneq (,$(findstring mingw,$(CC)))
  NOEXECSTACK_LDFLAG :=
else ifneq (,$(findstring mingw,$(CC_TARGET)))
  NOEXECSTACK_LDFLAG :=
else ifeq ($(OS),Windows_NT)
  NOEXECSTACK_LDFLAG :=
else
  NOEXECSTACK_LDFLAG := -Wl,-z,noexecstack
endif

PKG_GTK  := $(shell pkg-config --cflags --libs gtk+-3.0 glib-2.0)
PKG_GLIB := $(shell pkg-config --cflags --libs glib-2.0)

# Main source file (baseline)
SRC := wbasic.c

ICON_PNG := icon.png
ICON_OBJ := wbasic_icon_png.o

.PHONY: all unified gtk headless cli clean fresh

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
	clear && $(CC) $(CFLAGS) $(SRC) $(ICON_OBJ) -o $@ $(PKG_GTK) $(LDFLAGS) $(NOEXECSTACK_LDFLAG) $(LDLIBS)

# Headless / CLI-only build (no GTK libs required)
# We intentionally do NOT link the icon object here to avoid GNU-stack warnings and needless baggage.
headless: wbasic_cli
cli: headless

wbasic_cli: $(SRC)
	clear && $(CC) $(CFLAGS) -DWBASIC_NO_UI $(SRC) -o $@ $(PKG_GLIB) $(LDFLAGS) $(NOEXECSTACK_LDFLAG) $(LDLIBS)

fresh:
	@clear
	@$(MAKE) clean all

clean:
	rm -f wbasic wbasic_cli $(ICON_OBJ)
