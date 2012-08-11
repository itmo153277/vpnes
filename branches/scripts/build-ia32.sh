#!/bin/sh

BUILDNUM=0
while read line ; do
	BUILDNUM=$line
done < win32-builds/BUILD
if [ $# -gt 0 ] ; then
	REVSTR="/DSVNREV=\\\"$1-ia32\\\""
else
	REVSTR="/DSVNREV=\\\"ia32\\\""
fi
COMPFLAGS="/MT /Dsnprintf=_snprintf /DNO_STDIO_REDIRECT /DBUILDNUM=$BUILDNUM $REVSTR /Wall /O3 /Og /Qipo"
rm -rf release-ia32-config
mkdir release-ia32-config
cd release-ia32-config
LD="cccl" CC="cccl" CXX="cccl" RANLIB="true" CFLAGS="$COMPFLAGS" CXXFLAGS="$COMPFLAGS" ../../vpnes-0.3/configure --enable-showfps --with-sdl-prefix=/home/Viktor/vpnes/maintain/ia32 --disable-sdltest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
sed -i -e "s/\$(COMPILE) -c/\$(COMPILE) -c -o \$@/" Makefile
sed -i -e "s@\$(libvpnes_a_AR) @xilib $COMPFLAGS /out:@" nes/Makefile
make || exit 1 
make dist || exit 1
echo Packing exe...
/cygdrive/d/pack/raw/RAD\ Studio/Builder/upx -9 --best --ultra-brute vpnes.exe &> /dev/null || exit 1
/cygdrive/d/pack/raw/RAD\ Studio/Builder/rebuildpe --hard vpnes.exe &> /dev/null || exit 1
echo ============
echo Build [ OK ]
echo ============
mv -v vpnes-0.3.tar.gz ..
mkdir ../vpnes-0.3
cp -v vpnes.exe ../ia32/package/* ../vpnes-0.3/
cd ..
touch vpnes-0.3/*
cmd /c start make-arch.bat
