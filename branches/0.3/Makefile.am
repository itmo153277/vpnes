SUBDIRS = gui nes
bin_PROGRAMS = vpnes
vpnes_SOURCES = main.cpp
noinst_HEADERS = types.h
vpnes_LDADD = gui/libvpnes-gui.a nes/libvpnes.a

if WINDOWS

RCCOMPILE = $(RCC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_RCFLAGS) $(RCFLAGS)

vpnes_SOURCES += gui/win32-res/win32-res.rc

.rc.o:
	$(RCCOMPILE) $< $@

endif
