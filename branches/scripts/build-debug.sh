#!/bin/sh

rm -rf debug-win32-config/*
cd debug-win32-config
cp /usr/local/sdl-debug/bin/SDL.dll /usr/local/freetype/bin/libfreetype-6.dll .
LDFLAGS="-static-libgcc" LIBS="-lcomctl32" CFLAGS="-g -O0 -Wall" CXXFLAGS="-g -O0 -Wall -static-libstdc++" ../../vpnes-0.3/configure --enable-interactive --with-ft-prefix=/usr/local/freetype --with-sdl-prefix=/usr/local/sdl-debug --with-sdl_ttf-prefix=/usr/local/sdl-ttf --disable-sdltest --disable-freetypetest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
make
