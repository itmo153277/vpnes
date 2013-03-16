#!/bin/sh

rm -rf profile-win32-config/*
cd profile-win32-config
cp /usr/local/sdl-debug/bin/SDL.dll /usr/local/freetype/bin/libfreetype-6.dll .
LDFLAGS="-static-libgcc" CFLAGS="-g -O0 -Wall" CXXFLAGS="$CFLAGS -static-libstdc++" ../../vpnes-0.4/configure --disable-interactive --with-ft-prefix=/usr/local/freetype --with-sdl-prefix=/usr/local/sdl-debug --with-sdl_ttf-prefix=/usr/local/sdl-ttf --disable-sdltest --disable-freetypetest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
sed -e "s/-g -O0/-pg -O2/" -i Makefile gui/Makefile nes/Makefile
make
