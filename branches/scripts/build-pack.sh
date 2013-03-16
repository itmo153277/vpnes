#!/bin/sh

mkdir ./vpnes-0.4
if test "x$1" = "xia32" ; then
	cd release-ia32-config
	cp -v vpnes.exe ../ia32/package/* ../../../msvc/bin/SDL.dll ../../../msvc/bin/libfreetype-6.dll ../vpnes-0.4/
else
	cd release-win32-config
	cp -v vpnes.exe ../win32/* /usr/local/freetype/bin/libfreetype-6.dll /usr/local/sdl-release/bin/SDL.dll ../vpnes-0.4/
fi
cd ..
touch vpnes-0.4/*
cmd /c start make-arch.bat
