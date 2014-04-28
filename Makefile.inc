CC ?= cc
CFLAGS ?= -Os 
CFLAGS += -Wall -pedantic
PKG_CONFIG ?= pkg-config
LDFLAGS ?= -s
INSTALL = install -v

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)
