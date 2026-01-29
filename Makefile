CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDLIBS  ?= -lm

PKG_GTK  := $(shell pkg-config --cflags --libs gtk+-3.0 glib-2.0)
PKG_GLIB := $(shell pkg-config --cflags --libs glib-2.0)

SRC := wbasic.c

.PHONY: all gtk headless clean fresh

all: gtk

gtk: wbasic
wbasic: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(PKG_GTK) $(LDLIBS)

headless: wbasic_cli
wbasic_cli: $(SRC)
	$(CC) $(CFLAGS) -DWBASIC_NO_UI $(SRC) -o $@ $(PKG_GLIB) $(LDLIBS)

fresh:
	@clear
	@$(MAKE) clean all

clean:
	rm -f wbasic wbasic_cli
