dnl VPNES 0.5 configure
AC_PREREQ([2.68])
AC_INIT([vpnes], [0.5], [viktprog@gmail.com])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([auxdir])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)], [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([vpnes], [], [doc])
AC_CONFIG_HEADERS([config.h])

AC_COPYRIGHT([Copyright (c) 2012-2018 Ivanov Viktor])

AC_CANONICAL_HOST

host_os_windows=no
case ${host_os} in
	*cygwin* | *mingw* | *msys* )
		host_os_windows=yes
		;;
esac
AM_CONDITIONAL([WINDOWS], [test "x$host_os_windows" = "xyes"])

AM_CONDITIONAL([MAKE_MAN], [test "x$DX_FLAG_man" = "x1"])

AC_LANG([C++])

AX_CXX_COMPILE_STDCXX_17([], [mandatory])

can_use_sdl2="yes"
PKG_CHECK_MODULES([SDL], [sdl2 >= 2.0.5 SDL2_gfx], [], [can_use_sdl2="no"])
if test "x$can_use_sdl2" = "xno" ; then
	AC_MSG_WARN([pkg-config failed: $SDL_PKG_ERRORS])
	dnl Retrying with direct approach
	AM_PATH_SDL2([2.0.5], [can_use_sdl2="yes"])
	if test "x$can_use_sdl2" = "xyes" ; then
		SDL_LIBS="$SDL_LIBS -lSDL2_gfx"
		LIBS_SAVE=$LIBS
		CPPFLAGS_SAVE=$CPPFLAGS
		LIBS="$SDL_LIBS $LIBS"
		CPPFLAGS="$SDL_CFLAGS $CPPFLAGS"
		AC_CHECK_HEADERS([SDL2_gfxPrimitives.h SDL2_framerate.h], [], [can_use_sdl2="no"], [])
		LIBS=$LIBS_SAVE
		CPPFLAGS=$CPPFLAGS_SAVE
	fi
fi
if test "x$can_use_sdl2" = "xno" ; then
	AC_MSG_ERROR([could not find compatible SDL2])
else
	AC_DEFINE([USE_SDL], 1, [Define to 1 to enable SDL])
	CPPFLAGS="$SDL_CFLAGS $CPPFLAGS"
	LIBS="$SDL_LIBS $LIBS"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
