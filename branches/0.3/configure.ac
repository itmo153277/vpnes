AC_PREREQ([2.68])

AC_INIT([vpnes], [0.3], [viktprog@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AM_CONFIG_HEADER([config.h])

AC_COPYRIGHT([Copyright (c) 2012 Ivanov Viktor])
AC_REVISION([rev5])

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

if test "x$host_os_windows" = "xyes" ; then
	AC_CHECK_TOOL([RC], [windres], [AC_MSG_ERROR([cannot build resource files])])
fi

#Libraries

AM_PATH_SDL([1.2.14], [], [AC_MSG_ERROR([SDL >= 1.2.14 required])])
LIBS="$SDL_LIBS $LIBS"
CFLAGS="$SDL_CFLAGS $CFLAGS"
CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"

#Headers

#Compilator characterics

#Functions

AC_ARG_ENABLE(interactive, [  --enable-interactive    interactive mode],
              [vpnes_interactive=yes])
AM_CONDITIONAL([VPNES_INTERACTIVE], test "x$vpnes_interactive" = "xyes")
if test "x$vpnes_interactive" = "xyes" ; then
	AC_DEFINE([VPNES_INTERACTIVE], 1, [Interactive mode])
fi
AC_ARG_ENABLE(showcurframe, [  --enable-showcurframe   show current frame number in window title],
              [AC_DEFINE([VPNES_SHOW_CURFRAME], 1, [Show current frame number in window title])])
AC_ARG_ENABLE(showfps, [  --enable-showfps        show fps in window title],
              [AC_DEFINE([VPNES_SHOW_FPS], 1, [Show fps in window title])])
AC_ARG_ENABLE(sync, [  --disable-sync          disable frame synchronization],
              [AC_DEFINE([VPNES_DISABLE_SYNC], 1, [Disable frame synchronization])])
AC_ARG_ENABLE(sync, [  --disable-fskip         disable frame skipping],
              [AC_DEFINE([VPNES_DISABLE_FSKIP], 1, [Disable frame skip])])

AC_CONFIG_FILES([Makefile
                 gui/Makefile
                 nes/Makefile])
AC_OUTPUT
