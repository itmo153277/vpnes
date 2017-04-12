ACLOCAL_AMFLAGS=-I m4
AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = vpnes
vpnes_SOURCES = \
	main.cpp \
	src/gui/gui.cpp \
	src/gui/config.cpp \
	src/core/config.cpp \
	src/core/ines.cpp
vpnes_CPPFLAGS = -I$(top_srcdir)/include $(AM_CPP_FLAGS)
noinst_HEADERS = \
	include/vpnes/vpnes.hpp \
	include/vpnes/gui/config.hpp \
	include/vpnes/gui/gui.hpp \
	include/vpnes/core/config.hpp \
	include/vpnes/core/ines.hpp \
	include/vpnes/core/nes.hpp

@DX_RULES@
EXTRA_DIST = \
	autogen.sh \
	vpnes_msvc.sln \
	vpnes_msvc.vcxproj \
	vpnes_msvc.vcxproj.filters \
	config.msvc.h \
	README.md \
	$(DX_CONFIG)
MOSTLYCLEANFILES = $(DX_CLEANFILES)
if MAKE_MAN
dist_man3_MANS = $(top_builddir)/doc/man/man3/*.3
$(dist_man3_MANS): doxygen-doc
endif
