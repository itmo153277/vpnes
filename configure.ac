AC_PREREQ([2.68])

AC_INIT([vpnes], [0.3], [viktprog@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AM_CONFIG_HEADER([config.h])

AC_COPYRIGHT([Copyright (c) 2012 Ivanov Viktor])
AC_REVISION([rev10])

AC_CANONICAL_HOST

host_os_windows=no
case ${host_os} in
	*cygwin* | *mingw* )
		host_os_windows=yes
		;;
esac
AM_CONDITIONAL([WINDOWS], [test "x$host_os_windows" = "xyes"])

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

AM_PATH_SDL([1.2.14], [], [PKG_CHECK_MODULES([SDL], [SDL >= 1.2.14], [],
	[AC_MSG_ERROR([SDL >= 1.2.14 required])])])
LIBS="$SDL_LIBS $LIBS"
CFLAGS="$SDL_CFLAGS $CFLAGS"
CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"

AC_ARG_WITH(sdl_ttf-prefix, AS_HELP_STRING([--with-sdl_ttf-prefix=PFX], 
	[prefix where SDL_ttf is installed (optional)]),
	[sdl_ttf_prefix="$withval"], [sdl_ttf_prefix=""])

sdl_ttf_use="no"

if test -n "$sdl_ttf_prefix" ; then
	SDL_ttf_LIBS="-L$sdl_ttf_prefix/lib -lSDL_ttf"
	SDL_ttf_CFLAGS="-I$sdl_ttf_prefix/include/SDL"
	sdl_ttf_use="yes"
else
	PKG_CHECK_MODULES([SDL_ttf], [SDL_ttf], [sdl_ttf_use="yes"])
fi

if test "x$sdl_ttf_use" = "xyes" ; then
	AC_CHECK_FT2([2.4.0], [], [PKG_CHECK_MODULES([FT2], [FreeType >= 15.0.0], [],
	[sdl_ttf_use="no"])])
	LIBS="$FT2_LIBS $LIBS"
	CFLAGS="$FT2_CFLAGS $CFLAGS"
	CXXFLAGS="$FT2_CFLAGS $CXXFLAGS"
fi

if test "x$sdl_ttf_use" = "xyes" ; then
	AC_DEFINE([VPNES_USE_TTF], 1, [Define to 1 to enable TTF support])
	LIBS="$SDL_ttf_LIBS $LIBS"
	CFLAGS="$SDL_ttf_CFLAGS $CFLAGS"
	CXXFLAGS="$SDL_ttf_CFLAGS $CXXFLAGS"
fi

#Headers

if test "x$host_os_windows" = "xyes" ; then
	LIBS="-luser32 $LIBS"
	AC_CHECK_HEADER([windows.h], [], [AC_MSG_ERROR([required headers not found])])
fi

#Compilator characterics

#Functions

vpnes_configfile=yes
AC_ARG_WITH(configfile, AS_HELP_STRING([--with-configfile=ARG],
	[use configfile (global/local or full path) [default=local]]), [vpnes_configfile=$withval])
if test "x$vpnes_configfile" = "xyes" ; then
	vpnes_configfile=local
fi
AM_CONDITIONAL([VPNES_CONFIGFILE], test "x$vpnes_configfile" != "xno")
if test "x$vpnes_configfile" != "xno" ; then
	AC_DEFINE_UNQUOTED([VPNES_CONFIGFILE], ["$vpnes_configfile"],
		[Define to local/global or full path to config file to use configurations])
fi
AC_ARG_ENABLE(interactive, AS_HELP_STRING([--enable-interactive], [interactive mode for GUI]),
              [vpnes_interactive=$enableval])
AM_CONDITIONAL([VPNES_INTERACTIVE], test "x$vpnes_interactive" = "xyes")
if test "x$vpnes_interactive" = "xyes" ; then
	AC_DEFINE([VPNES_INTERACTIVE], 1, [Define to 1 to enable interactive mode for GUI])
	if test "x$host_os_windows" = "xyes" ; then
		LIBS="-lcomctl32 $LIBS"
		AC_CHECK_HEADERS([commdlg.h commctrl.h shellapi.h], [],
			[AC_MSG_ERROR([required headers not found])], [#include <windows.h>])
	fi
fi
AC_ARG_ENABLE(showcurframe, AS_HELP_STRING([--enable-showcurframe], 
	[show frame number in window title]),
	[if test "x$enableval" = "xyes" ; then
		AC_DEFINE([VPNES_SHOW_CURFRAME], 1, [Define to 1 to show frame number in window title])
	fi])
AC_ARG_ENABLE(showfps, AS_HELP_STRING([--enable-showfps], [show fps in window title]),
	[if test "x$enableval" = "xyes" ; then
		AC_DEFINE([VPNES_SHOW_FPS], 1, [Define to 1 to show fps in window title])
	fi])
AC_ARG_ENABLE(sync, AS_HELP_STRING([--disable-sync], [disable frame synchronization]),
	[if test "x$enableval" = "xno" ; then
		AC_DEFINE([VPNES_DISABLE_SYNC], 1, [Define to 1 to disable frame synchronization])
	fi])
AC_ARG_ENABLE(fskip, AS_HELP_STRING([--disable-fskip], [disable frame skipping]),
	[if test "x$enableval" = "xno" ; then
		AC_DEFINE([VPNES_DISABLE_FSKIP], 1, [Define to 1 to disable frame skip])
	fi])

AC_CONFIG_FILES([Makefile
                 gui/Makefile
                 nes/Makefile])
AC_OUTPUT
