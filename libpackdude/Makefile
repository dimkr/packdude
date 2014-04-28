include ../Makefile.inc

CFLAGS += $(shell $(PKG_CONFIG) --cflags zlib libarchive libcurl)

all: libpackdude.a

libpackdude.a: $(OBJECTS)
	ar rcs libpackdude.a $(OBJECTS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f libpackdude.a $(OBJECTS)
