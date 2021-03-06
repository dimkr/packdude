CC ?= cc
CFLAGS ?= -O2 -pipe
LDFLAGS ?= -s
PKG_CONFIG ?= pkg-config
DESTDIR ?=
BIN_DIR ?= /bin
MAN_DIR ?= /usr/share/man
DOC_DIR ?= /usr/share/doc
VAR_DIR ?= /var
ARCH ?= $(shell uname -m)

PACKAGE = packdude
VERSION = 1
CFLAGS += -std=gnu99 -Wall -pedantic \
          -DNDEBUG \
          -DVAR_DIR=\"$(VAR_DIR)\" \
          -DARCH=\"$(ARCH)\" \
          -DPACKAGE=\"$(PACKAGE)\" \
          -DVERSION=$(VERSION) \
          $(shell $(PKG_CONFIG) --cflags libcurl libarchive sqlite3 zlib)

INSTALL = install -v
LIBCURL_LIBS = $(shell $(PKG_CONFIG) --libs libcurl)
LIBARCHIVE_LIBS = $(shell $(PKG_CONFIG) --libs libarchive)
SQLITE_LIBS = $(shell $(PKG_CONFIG) --libs sqlite3)
ZLIB_LIBS = $(shell $(PKG_CONFIG) --libs zlib)

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)

all: packdude dudepack dudeunpack repodude

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

dudepack: dudepack.o log.o
	$(CC) -o $@ $^ $(LDFLAGS) $(ZLIB_LIBS)

dudeunpack: dudeunpack.o package.o archive.o log.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBARCHIVE_LIBS) $(ZLIB_LIBS)

repodude: repodude.c database.o log.o
	$(CC) -o $@ $^ $(LDFLAGS) $(SQLITE_LIBS)

packdude: packdude.o manager.o database.o fetch.o repo.o log.o stack.o \
          package_ops.o package.o archive.o
	$(CC) -o $@ $^ $(LDFLAGS) \
	               $(LIBCURL_LIBS) \
	               $(LIBARCHIVE_LIBS) \
	               $(SQLITE_LIBS) \
	               $(ZLIB_LIBS)

doc: $(SRCS) $(HEADERS) doxygen.conf
	doxygen doxygen.conf

install: all
	$(INSTALL) -D -m 755 packdude $(DESTDIR)/$(BIN_DIR)/packdude
	$(INSTALL) -m 755 dudepack $(DESTDIR)/$(BIN_DIR)/dudepack
	$(INSTALL) -m 755 dudeunpack $(DESTDIR)/$(BIN_DIR)/dudeunpack
	$(INSTALL) -m 755 repodude $(DESTDIR)/$(BIN_DIR)/repodude
	$(INSTALL) -D -d -m 755 $(DESTDIR)/$(VAR_DIR)/packdude
	$(INSTALL) -D -m 644 packdude.8 $(DESTDIR)/$(MAN_DIR)/man8/packdude.8
	$(INSTALL) -D -m 644 dudepack.1 $(DESTDIR)/$(MAN_DIR)/man1/dudepack.1
	$(INSTALL) -m 644 dudeunpack.1 $(DESTDIR)/$(MAN_DIR)/man1/dudeunpack.1
	$(INSTALL) -m 644 repodude.1 $(DESTDIR)/$(MAN_DIR)/man1/repodude.1
	$(INSTALL) -D -m 644 README $(DESTDIR)/$(DOC_DIR)/packdude/README
	$(INSTALL) -m 644 AUTHORS $(DESTDIR)/$(DOC_DIR)/packdude/AUTHORS
	$(INSTALL) -m 644 COPYING $(DESTDIR)/$(DOC_DIR)/packdude/COPYING

clean:
	rm -f repodude packdude dudepack dudeunpack $(OBJECTS)