noinst_LIBRARIES = libvpnes-gui.a
libvpnes_gui_a_SOURCES = gui.cpp window.c
noinst_HEADERS = gui.h interactive.h window.h win32-res/win32-res.h
dist_noinst_DATA = interactive.c win32-res/icon.ico \
                   win32-res/win32-res.rc

if VPNES_INTERACTIVE
libvpnes_gui_a_SOURCES += interactive.c
endif
