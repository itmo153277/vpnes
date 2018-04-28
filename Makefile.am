ACLOCAL_AMFLAGS=-I m4
AUTOMAKE_OPTIONS = subdir-objects

CORE_SOURCES = \
	src/core/mappers/nrom.cpp \
	src/core/config.cpp \
	src/core/cpu.cpp \
	src/core/ines.cpp
GUI_SOURCES = \
	src/gui/gui.cpp \
	src/gui/config.cpp
UNITTEST_SOURCES = \
	src/tests/unittests/example-test.cpp \
	src/tests/unittests/init.cpp
TESTER_SOURCES = \
	src/tests/end2end/end2end.cpp \
	src/gui/config.cpp
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
	include/vpnes/core/debugger.hpp \
	include/vpnes/core/device.hpp \
	include/vpnes/core/factory.hpp \
	include/vpnes/core/frontend.hpp \
	include/vpnes/core/ines.hpp \
	include/vpnes/core/mboard.hpp \
	include/vpnes/core/nes.hpp \
	include/vpnes/core/ppu_compile.hpp \
	include/vpnes/core/ppu.hpp

BLARGG_TESTS = \
	tests/blargg/cpu/instr/01-basics.nes \
	tests/blargg/cpu/instr/02-implied.nes \
	tests/blargg/cpu/instr/03-immediate.nes \
	tests/blargg/cpu/instr/04-zero_page.nes \
	tests/blargg/cpu/instr/05-zp_xy.nes \
	tests/blargg/cpu/instr/06-absolute.nes \
	tests/blargg/cpu/instr/07-abs_xy.nes \
	tests/blargg/cpu/instr/08-ind_x.nes \
	tests/blargg/cpu/instr/09-ind_y.nes \
	tests/blargg/cpu/instr/10-branches.nes \
	tests/blargg/cpu/instr/11-stack.nes \
	tests/blargg/cpu/instr/12-jmp_jsr.nes \
	tests/blargg/cpu/instr/13-rts.nes \
	tests/blargg/cpu/instr/14-rti.nes \
	tests/blargg/cpu/instr/15-brk.nes \
	tests/blargg/cpu/instr/16-special.nes

BLARGG_DIST = \
	$(BLARGG_TESTS) \
	tests/blargg/LICENSE.txt \
	tests/cpu/instr/README.txt

if UNITTESTS_ENABLED
UNITTESTS=unittests
else
UNITTESTS=
endif
TEST_EXTENSIONS = .nes
TESTS = $(UNITTESTS) $(BLARGG_TESTS)
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	 $(top_srcdir)/auxdir/tap-driver.sh --ignore-exit
NES_LOG_COMPILER = $(top_builddir)/tester_blargg$(EXEEXT)

bin_PROGRAMS = vpnes
check_PROGRAMS = $(UNITTESTS) tester_blargg
noinst_LIBRARIES = libcore.a

libcore_a_SOURCES = $(CORE_SOURCES)
vpnes_SOURCES = \
	main.cpp \
	$(GUI_SOURCES)
unittests_SOURCES =	$(UNITTEST_SOURCES)
tester_blargg_SOURCES = $(TESTER_SOURCES)

AM_CPPFLAGS = -I$(top_srcdir)/include

vpnes_CPPFLAGS = $(AM_CPPFLAGS) $(SDL_CFLAGS)
vpnes_LDADD = $(SDL_LIBS) libcore.a

unittests_CPPFLAGS = $(AM_CPPFLAGS) $(BOOST_CPPFLAGS) $(BOOST_UNIT_TEST_FRAMEWORK_CPPFLAGS)
unittests_LDADD = $(BOOST_LDFLAGS) $(BOOST_UNIT_TEST_FRAMEWORK_LIB) libcore.a

tester_blargg_LDADD = libcore.a

@DX_RULES@
EXTRA_DIST = \
	autogen.sh \
	vpnes_msvc.sln \
	vpnes_msvc.vcxproj \
	vpnes_msvc.vcxproj.filters \
	config.msvc.h \
	README.md \
	$(DX_CONFIG) \
	$(BLARGG_DIST)
MOSTLYCLEANFILES = $(DX_CLEANFILES)
if MAKE_MAN
dist_man3_MANS = $(top_builddir)/doc/man/man3/*.3
$(dist_man3_MANS): doxygen-doc
endif
