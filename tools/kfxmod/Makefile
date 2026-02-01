# Makefile for kfxmod tool

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../src
LDFLAGS = -lz

TARGET = kfxmod
SOURCES = tool_modpack.c ../../src/config_modpack.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/

.PHONY: all clean install
