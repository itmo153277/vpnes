#!/bin/sh

BUILDNUM=0
while read line ; do
	BUILDNUM=$line
done < win32-builds/BUILD
rm -rf release-win32-config
mkdir release-win32-config
cd release-win32-config
LDFLAGS="-static-libgcc" CFLAGS="-O3 -Wall -fomit-frame-pointer" CXXFLAGS="$CFLAGS -static-libstdc++" ../../vpnes-0.4/configure --enable-interactive --with-configfile=local --with-ft-prefix=/usr/local/freetype --with-sdl-prefix=/usr/local/sdl-release --with-sdl_ttf-prefix=/usr/local/sdl-ttf --disable-sdltest --disable-freetypetest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
echo "#define BUILDNUM \"$BUILDNUM\"" >> config.h
if [ $# -gt 0 ] ; then
	echo "#define SVNREV \"$1\"" >> config.h
fi
cp -v /usr/local/freetype/bin/libfreetype-6.dll /usr/local/sdl-release/bin/SDL.dll .
make || exit 1
