#!/bin/sh
BUILDNUM=0
while read line ; do
	BUILDNUM=$line
done < win32-builds/BUILD
COMPFLAGS="/MT /FP:precise /Dsnprintf=_snprintf /DNO_STDIO_REDIRECT /Wall /O3 /Oy /Og"
rm -rf release-ia32-config
mkdir release-ia32-config
cd release-ia32-config
LD="cccl" LIBS="user32.lib comdlg32.lib shell32.lib" CC="cccl" CXX="cccl" RANLIB="true" CFLAGS="$COMPFLAGS" CXXFLAGS="$COMPFLAGS" ../../vpnes-0.3/configure --enable-interactive --with-sdl-prefix=/home/Viktor/msvc --with-ft-prefix=/home/Viktor/msvc --with-sdl_ttf-prefix=/home/Viktor/msvc --disable-sdltest --disable-freetypetest --host=i686-w64-mingw32 --build=i686-w64-mingw32 || exit 1
sed -i -e "s/\$(COMPILE) -c/\$(COMPILE) -c -o \$@/" -e "s@\$(libvpnes_gui_a_AR) @xilib $COMPFLAGS /out:@" gui/Makefile
sed -i -e "s@\$(libvpnes_a_AR) @xilib $COMPFLAGS /out:@" nes/Makefile
sed -i -e "s/\.rc\.o:/\.rc\.obj:/" -e "s/-lSDL_ttf//" Makefile
echo "#pragma comment(linker, \"\\\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\\\"\")" >> config.h
echo "#define BUILDNUM \"$BUILDNUM\"" >> config.h
if [ $# -gt 0 ] ; then
	echo "#define SVNREV \"$1-ia32\"" >> config.h
fi
#echo "#define bool int" >> config.h
#echo "#define inline __forceinline" >> config.h
# possible loss of data
echo "#pragma warning(disable:4244)" >> config.h
# was never referenced
echo "#pragma warning(disable:869)" >> config.h
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
cp -v vpnes.exe ../ia32/package/* ../../../msvc/bin/SDL.dll ../../../msvc/bin/libfreetype-6.dll ../vpnes-0.3/
cd ..
touch vpnes-0.3/*
cmd /c start make-arch.bat
