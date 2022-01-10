#!/bin/sh


CFLAGS="-Wall $(pkg-config --cflags sdl2)"
LIBS="$(pkg-config --libs sdl2)"

cc $CFLAGS $LIBS src/main.c -o game
