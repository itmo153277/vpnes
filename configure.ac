AC_PREREQ([2.68])

AC_INIT([vpnes], [0.1], [viktprog@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AM_CONFIG_HEADER([config.h])

AC_COPYRIGHT([Copyright (c) 2012 Ivanov Viktor])
AC_REVISION([rev2])

#Programs

AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX

#Libraries

AM_PATH_SDL([1.2.14], [], [AC_MSG_ERROR([SDL >= 1.2.14 required])])
LIBS="$SDL_LIBS $LIBS"
CFLAGS="$SDL_CFLAGS $CFLAGS"
CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"

AC_ARG_ENABLE(asmstretch, [  --disable-asmstretch    Do not use assembler for stretching SDL surfaces],
		    [AC_DEFINE([SDL_STRETCH_DISABLE_ASM], 1, [Disable using asm code for stretching])])
AC_ARG_ENABLE(interprstr, [  --enable-interprstr     Use interpreted code for stretching instead of native],
		    [AC_DEFINE([SDL_STRETCH_USE_INTERPRETED_CODE], 1, [Use interpreted code instead of native])])

#Headers

#Compilator characterics

#Functions

AC_CONFIG_FILES([Makefile
                 nes/Makefile
                 sdlstretch/Makefile])
AC_OUTPUT
