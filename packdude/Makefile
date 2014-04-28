include ../Makefile.inc

LIBS = $(shell $(PKG_CONFIG) --libs zlib libarchive) -lpackdude
CFLAGS += $(shell $(PKG_CONFIG) --cflags zlib libarchive) -I../libpackdude
LDFLAGS += -L../libpackdude

all: packdude

packdude: $(OBJECTS) ../libpackdude/libpackdude.a
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f packdude $(OBJECTS)
