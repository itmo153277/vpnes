#!/bin/sh

rm -rf profile-win32-config/*
cd profile-win32-config
cp /usr/local/sdl-debug/bin/SDL.dll .
LDFLAGS="-static-libgcc" CFLAGS="-g -O3 -Wall" CXXFLAGS="-g -O3 -Wall -static-libstdc++" ../../vpnes-0.3/configure --enable-showfps --with-sdl-prefix=/usr/local/sdl-debug --disable-sdltest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
sed -e "s/-g -O3/-pg -O3/" -i Makefile gui/Makefile nes/Makefile
make
