include ../Makefile.inc

LIBS = $(shell $(PKG_CONFIG) --libs zlib)
CFLAGS += $(shell $(PKG_CONFIG) --cflags zlib) -I../libpackdude
LDFLAGS += -L../libpackdude

all: dudepack

dudepack: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f dudepack $(OBJECTS)
