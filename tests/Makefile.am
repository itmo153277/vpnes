if CHECK
TESTS = nesclock
else
TESTS =
endif
check_PROGRAMS = $(TESTS)
nesclock_CXXFLAGS = @CHECK_CFLAGS@
nesclock_LDADD = @CHECK_LIBS@
nesclock_SOURCES = nes-clock.cpp $(top_builddir)/nes/clock.cpp $(top_builddir)/nes/clock.h