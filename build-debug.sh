#!/bin/sh

rm -rf debug-win32-config/*
cd debug-win32-config
cp /usr/local/sdl-debug/bin/SDL.dll .
LDFLAGS="-static-libgcc" CFLAGS="-g -O0 -Wall" CXXFLAGS="-g -O0 -Wall -static-libstdc++" ../../vpnes-0.3/configure --enable-interactive --with-sdl-prefix=/usr/local/sdl-debug --disable-sdltest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
make
