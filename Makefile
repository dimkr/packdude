CC ?= cc
CFLAGS ?= -O2 -pipe
LDFLAGS ?=
PKG_CONFIG ?= pkg-config
DESTDIR ?=
BIN_DIR ?= /bin
MAN_DIR ?= /usr/share/man
DOC_DIR ?= /usr/share/doc
VAR_DIR ?= /var
ARCH ?= $(shell uname -m)

CFLAGS += -std=gnu99 -Wall -pedantic -DVAR_DIR=\"$(VAR_DIR)\" -DARCH=\"$(ARCH)\" \
          $(shell $(PKG_CONFIG) --cflags libcurl libarchive sqlite3)
LDFLAGS += $(shell $(PKG_CONFIG) --libs libcurl libarchive sqlite3)
INSTALL = install -v

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)

all: packdude dudepack

miniz.o: miniz.c
	$(shell $(CC) -c -o $@ $< $(CFLAGS) > /dev/null 2>&1)

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

dudepack: dudepack.o comp.o miniz.o log.o
	$(CC) -o $@ $^ $(LDFLAGS)

packdude: packdude.o manager.o database.o fetch.o repo.o log.o stack.o \
          package.o archive.o comp.o miniz.o
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	$(INSTALL) -D -m 755 packdude $(DESTDIR)/$(BIN_DIR)/packdude
	$(INSTALL) -m 755 dudepack $(DESTDIR)/$(BIN_DIR)/dudepack
	$(INSTALL) -D -d -m 755 $(DESTDIR)/$(VAR_DIR)/packdude/archive
	$(INSTALL) -D -m 644 packdude.8 $(DESTDIR)/$(MAN_DIR)/man8/packdude.8
	$(INSTALL) -D -m 644 dudepack.1 $(DESTDIR)/$(MAN_DIR)/man1/dudepack.1
	$(INSTALL) -D -m 644 README $(DESTDIR)/$(DOC_DIR)/packdude/README
	$(INSTALL) -m 644 AUTHORS $(DESTDIR)/$(DOC_DIR)/packdude/AUTHORS
	$(INSTALL) -m 644 COPYING $(DESTDIR)/$(DOC_DIR)/packdude/COPYING

clean:
	rm -f packdude dudepack $(OBJECTS)