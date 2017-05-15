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

AC_COPYRIGHT([Copyright (c) 2012-2017 Ivanov Viktor])

AC_CANONICAL_HOST

AM_CONDITIONAL([MAKE_MAN], [test "x$DX_FLAG_man" = "x1"])

AC_PROG_CXX

AX_CXX_COMPILE_STDCXX_11([], [mandatory])

can_use_sdl2="yes"
AM_PATH_SDL2([2.0.0], [], [can_use_sdl2="no"])
SDL_LIBS="$SDL_LIBS -lSDL2_gfx"
if test "x$can_use_sdl2" = "xyes" ; then
	AC_LANG_PUSH([C])
	LIBS_SAVE=$LIBS
	CFLAGS_SAVE=$CFLAGS
	CPPFLAGS_SAVE=$CPPFLAGS
	LIBS="$SDL_LIBS $LIBS"
	CPPFLAGS="$SDL_CFLAGS $CPPFLAGS"
	AC_CHECK_HEADERS([SDL2_gfxPrimitives.h SDL2_framerate.h], [], [can_use_sdl2="no"], [])
	LIBS=$LIBS_SAVE
	CFLAGS=$CFLAGS_SAVE
	CPPFLAGS=$CPPFLAGS_SAVE
	AC_LANG_POP
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

