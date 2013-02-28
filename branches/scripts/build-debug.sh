#!/bin/sh

rm -rf debug-win32-config/*
cd debug-win32-config
cp /usr/local/sdl-release/bin/SDL.dll /usr/local/freetype/bin/libfreetype-6.dll .
LDFLAGS="-static-libgcc" CFLAGS="-g -O0 -Wall" CXXFLAGS="-g -O0 -Wall -static-libstdc++" ../../vpnes-0.3/configure --enable-interactive --enable-showfps --with-configfile=local --with-ft-prefix=/usr/local/freetype --with-sdl-prefix=/usr/local/sdl-release --with-sdl_ttf-prefix=/usr/local/sdl-ttf --disable-sdltest --disable-freetypetest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
make
