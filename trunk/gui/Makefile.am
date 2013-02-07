noinst_LIBRARIES = libvpnes-gui.a
libvpnes_gui_a_SOURCES = audio.cpp config.cpp gui.cpp input.cpp video.cpp \
                         window.cpp
noinst_HEADERS = audio.h config.h gui.h input.h video.h window.h \
                 win32-res/win32-res.h
dist_noinst_DATA = text.otf win32-res/icon.ico win32-res/win32-res.rc

if VPNES_USEFONT

if WINDOWS
FONT = 
FONTFLAGS = 
else
FONT = text.otf
FONTFLAGS = -DVPNES_TTF_PATH='"$(datadir)/text.otf"'
endif

data_DATA = $(FONT)
AM_CPPFLAGS = $(FONTFLAGS)

endif
