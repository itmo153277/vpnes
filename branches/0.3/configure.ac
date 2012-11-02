AC_PREREQ([2.68])

AC_INIT([vpnes], [0.3], [viktprog@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AM_CONFIG_HEADER([config.h])

AC_COPYRIGHT([Copyright (c) 2012 Ivanov Viktor])
AC_REVISION([rev7])

AC_CANONICAL_HOST

host_os_windows=no
case ${host_os} in
	*mingw32*)
		host_os_windows=yes
		;;
	*cygwin*)
		host_os_windows=yes
		;;
esac
AM_CONDITIONAL([WINDOWS], test "x$host_os_windows" = "xyes")

#Programs

AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX

AC_ARG_VAR([RCC], [resource compiler command])
AC_ARG_VAR([RCFLAGS], [resource compiler flags])
if test "x$host_os_windows" = "xyes" ; then
	AC_CHECK_TOOL([RCC], [windres], [AC_MSG_ERROR([cannot build resource files])])
fi

#Libraries

AM_PATH_SDL([1.2.14], [], [PKG_CHECK_MODULES([SDL], [SDL >= 1.2.14], []
	[AC_MSG_ERROR([SDL >= 1.2.14 required])])])
LIBS="$SDL_LIBS $LIBS"
CFLAGS="$SDL_CFLAGS $CFLAGS"
CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"

AC_ARG_WITH(sdl_ttf-prefix,[  --with-sdl_ttf-prefix=PFX   
                          prefix where SDL_ttf is installed (optional)],
	sdl_ttf_prefix="$withval", sdl_ttf_prefix="")

sdl_ttf_use="no"

if test -n "$sdl_ttf_prefix" ; then
	SDL_ttf_LIBS="-L$sdl_ttf_prefix/lib -lSDL_ttf"
	SDL_ttf_CFLAGS="-I$sdl_ttf_prefix/include/SDL"
	sdl_ttf_use="yes"
else
	PKG_CHECK_MODULES([SDL_ttf], [SDL_ttf], [sdl_ttf_use="yes"])
fi

if test "x$sdl_ttf_use" = "xyes" ; then
	AC_CHECK_FT2([2.4.0], [], [sdl_ttf_use="no"])
	LIBS="$FT2_LIBS $LIBS"
	CFLAGS="$FT2_CFLAGS $CFLAGS"
	CXXFLAGS="$FT2_CFLAGS $CXXFLAGS"
fi

if test "x$sdl_ttf_use" = "xyes" ; then
	AC_DEFINE([VPNES_USE_TTF], 1, [Enable TTF support])
	LIBS="$SDL_ttf_LIBS $LIBS"
	CFLAGS="$SDL_ttf_CFLAGS $CFLAGS"
	CXXFLAGS="$SDL_ttf_CFLAGS $CXXFLAGS"
fi

#Headers

#Compilator characterics

#Functions

AC_ARG_ENABLE(interactive, AS_HELP_STRING([--enable-interactive], [interactive mode]),
              [vpnes_interactive=yes])
AM_CONDITIONAL([VPNES_INTERACTIVE], test "x$vpnes_interactive" = "xyes")
if test "x$vpnes_interactive" = "xyes" ; then
	AC_DEFINE([VPNES_INTERACTIVE], 1, [Interactive mode])
fi
AC_ARG_ENABLE(showcurframe, AS_HELP_STRING([--enable-showcurframe], 
	[show current frame number in window title]),
	AC_DEFINE([VPNES_SHOW_CURFRAME], 1, [Show current frame number in window title])])
AC_ARG_ENABLE(showfps, AS_HELP_STRING([--enable-showfps], [show fps in window title]),
              [AC_DEFINE([VPNES_SHOW_FPS], 1, [Show fps in window title])])
AC_ARG_ENABLE(sync, AS_HELP_STRING([--disable-sync], [disable frame synchronization]),
              [AC_DEFINE([VPNES_DISABLE_SYNC], 1, [Disable frame synchronization])])
AC_ARG_ENABLE(fskip, AS_HELP_STRING([--disable-fskip], [disable frame skipping]),
              [AC_DEFINE([VPNES_DISABLE_FSKIP], 1, [Disable frame skip])])

AC_CONFIG_FILES([Makefile
                 gui/Makefile
                 nes/Makefile])
AC_OUTPUT
