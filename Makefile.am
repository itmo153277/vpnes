ACLOCAL_AMFLAGS=-I m4
AUTOMAKE_OPTIONS = subdir-objects


bin_PROGRAMS = vpnes
check_PROGRAMS = unittests

if UNITTESTS_ENABLED
UNITTESTS=unittests
else
UNITTESTS=
endif
TESTS = $(UNITTESTS)

CORE_SOURCES = \
    src/core/mappers/nrom.cpp \
	src/core/config.cpp \
	src/core/cpu.cpp \
	src/core/ines.cpp
GUI_SOURCES = \
	src/gui/gui.cpp \
	src/gui/config.cpp
UNITTEST_SOURCES = \
	src/tests/example-test.cpp
noinst_HEADERS = \
	include/vpnes/vpnes.hpp \
	include/vpnes/gui/config.hpp \
	include/vpnes/gui/gui.hpp \
	include/vpnes/core/mappers/helper.hpp \
	include/vpnes/core/mappers/nrom.hpp \
	include/vpnes/core/apu.hpp \
	include/vpnes/core/bus.hpp \
	include/vpnes/core/config.hpp \
	include/vpnes/core/cpu.hpp \
	include/vpnes/core/cpu_compile.hpp \
	include/vpnes/core/device.hpp \
	include/vpnes/core/factory.hpp \
	include/vpnes/core/frontend.hpp \
	include/vpnes/core/ines.hpp \
	include/vpnes/core/mboard.hpp \
	include/vpnes/core/nes.hpp \
	include/vpnes/core/ppu_compile.hpp \
	include/vpnes/core/ppu.hpp

vpnes_SOURCES = \
	main.cpp \
	$(CORE_SOURCES) \
	$(GUI_SOURCES)
unittests_SOURCES = \
	$(UNITTEST_SOURCES) \
	$(CORE_SOURCES)

AM_CPPFLAGS = -I$(top_srcdir)/include

vpnes_CPPFLAGS = $(AM_CPPFLAGS) $(SDL_CFLAGS)
vpnes_LDADD = $(SDL_LIBS)

unittests_CPPFLAGS = $(AM_CPPFLAGS) $(BOOST_CPPFLAGS)
unittests_LDADD = $(BOOST_LDFLAGS) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

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
