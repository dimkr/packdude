VAR_DIR ?= /var

ARCH ?= $(shell uname -m)

CC ?= cc
CFLAGS ?= -Os 
CFLAGS += -Wall -pedantic -DARCH=\"$(ARCH)\" -DVAR_DIR=\"$(VAR_DIR)\"
PKG_CONFIG ?= pkg-config
LDFLAGS ?= -s
INSTALL = install -v

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)
