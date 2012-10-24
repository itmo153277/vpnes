#!/bin/sh

BUILDNUM=0
while read line ; do
	BUILDNUM=$line
done < win32-builds/BUILD
if [ $# -gt 0 ] ; then
	REVSTR="-DSVNREV=\"\\\"$1\\\"\""
else
	REVSTR=""
fi
rm -rf release-win32-config
mkdir release-win32-config
cd release-win32-config
LDFLAGS="-static-libgcc" CPPFLAGS="-DBUILDNUM=$BUILDNUM $REVSTR" CFLAGS="-O3 -Wall -fomit-frame-pointer" CXXFLAGS="-O3 -Wall -fomit-frame-pointer -static-libstdc++" ../../vpnes-0.3/configure --enable-interactive --with-sdl-prefix=/usr/local/sdl-release --disable-sdltest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
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
mv -v vpnes-0.3.tar.gz ..
mkdir ../vpnes-0.3
cp -v vpnes.exe ../win32/* /usr/local/sdl-release/bin/SDL.dll ../vpnes-0.3/
cd ..
touch vpnes-0.3/*
cmd /c start make-arch.bat
