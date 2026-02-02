CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDLIBS  ?= -lm

PKG_GTK  := $(shell pkg-config --cflags --libs gtk+-3.0 glib-2.0)
PKG_GLIB := $(shell pkg-config --cflags --libs glib-2.0)

SRC := wbasic.c

ICON_PNG := icon.png
ICON_OBJ := wbasic_icon_png.o

.PHONY: all gtk headless clean fresh

all: gtk

# Build icon object (PNG embedded into executable)
$(ICON_OBJ): $(ICON_PNG)
	ld -r -b binary "$(ICON_PNG)" -o "$(ICON_OBJ)"
	objcopy --add-section .note.GNU-stack=/dev/null \
	        --set-section-flags .note.GNU-stack=contents,readonly \
	        "$(ICON_OBJ)"


gtk: wbasic
wbasic: $(SRC) $(ICON_OBJ)
	clear && $(CC) $(CFLAGS) $(SRC) $(ICON_OBJ) -o $@ $(PKG_GTK) $(LDLIBS)
headless: wbasic_cli
wbasic_cli: $(SRC) $(ICON_OBJ)
	clear && $(CC) $(CFLAGS) -DWBASIC_NO_UI $(SRC) $(ICON_OBJ) -o $@ $(PKG_GLIB) $(LDLIBS)
fresh:
	@clear
	@$(MAKE) clean all

clean:
	rm -f wbasic wbasic_cli $(ICON_OBJ)