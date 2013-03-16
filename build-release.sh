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
make || exit 1 
make dist || exit 1
echo Packing exe...
i686-w64-mingw32-strip --strip-debug vpnes.exe
i686-w64-mingw32-strip --strip-unneeded vpnes.exe
/cygdrive/d/pack/raw/RAD\ Studio/Builder/upx -9 --best --ultra-brute vpnes.exe &> /dev/null || exit 1
/cygdrive/d/pack/raw/RAD\ Studio/Builder/rebuildpe --hard vpnes.exe &> /dev/null || exit 1
echo ============
echo Build [ OK ]
echo ============
mv -v vpnes-0.4.tar.gz ..
mkdir ../vpnes-0.4
cp -v vpnes.exe ../win32/* /usr/local/freetype/bin/libfreetype-6.dll /usr/local/sdl-release/bin/SDL.dll ../vpnes-0.4/
cd ..
touch vpnes-0.4/*
cmd /c start make-arch.bat
