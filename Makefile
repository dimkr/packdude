CC ?= cc
CFLAGS ?= -Os
LDFLAGS ?=
PKG_CONFIG ?= pkg-config
DESTDIR ?=
BIN_DIR ?= /bin
DOC_DIR ?= /usr/share/doc
VAR_DIR ?= /var
ARCH ?= $(shell uname -m)

CFLAGS += -Wall -pedantic -DVAR_DIR=\"$(VAR_DIR)\" -DARCH=\"$(ARCH)\" \
          $(shell $(PKG_CONFIG) --cflags libcurl libarchive sqlite3)
LDFLAGS += $(shell $(PKG_CONFIG) --libs libcurl libarchive sqlite3)
INSTALL = install -v

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)

all: packdude dudepack

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

dudepack: dudepack.o miniz.o
	$(CC) -o $@ $^ $(LDFLAGS)

packdude: packdude.o manager.o database.o fetch.o repo.o log.o stack.o \
          package.o archive.o miniz.o
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	$(INSTALL) -D -m 755 packdude $(DESTDIR)/$(BIN_DIR)/packdude
	$(INSTALL) -m 755 dudepack $(DESTDIR)/$(BIN_DIR)/dudepack
	$(INSTALL) -D -d -m 755 $(DESTDIR)/$(VAR_DIR)/packdude/archive
	$(INSTALL) -D -m 644 README $(DESTDIR)/$(DOC_DIR)/packdude/README
	$(INSTALL) -m 644 AUTHORS $(DESTDIR)/$(DOC_DIR)/packdude/AUTHORS
	$(INSTALL) -m 644 COPYING $(DESTDIR)/$(DOC_DIR)/packdude/COPYING

clean:
	rm -f packdude dudepack $(OBJECTS)