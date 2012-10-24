/****************************************************************************\

	NES Emulator
	Copyright (C) 2012  Ivanov Viktor

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

\****************************************************************************/

#include "interactive.h"
#include "gui.h"
#include "window.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include "win32-res/win32-res.h"
#endif

#ifdef _WIN32
char FileName[MAX_PATH];
int opennew = 0;
int quit = 0;
#endif

/* Обработчик сообщений */
int InteractiveDispatcher(SDL_SysWMmsg *Msg) {
#ifdef _WIN32
	OPENFILENAME ofn;

	if (Msg->msg == WM_ENTERMENULOOP) {
		SDL_PauseAudio(-1);
		return 0;
	} else if (Msg->msg == WM_EXITMENULOOP) {
		SDL_PauseAudio(0);
		return 0;
	} else if (Msg->msg != WM_COMMAND)
		return 0;
	switch (Msg->wParam) {
		case ID_FILE_OPEN:
			SDL_PauseAudio(-1);
			memset(&ofn, 0, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = WindowHandle;
			ofn.hInstance = Instance;
			ofn.lpstrFilter = "iNES 1.0 Files (*.nes)\0*.nes\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = FileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
				OFN_ALLOWMULTISELECT;
			ofn.lpstrDefExt = "nes";
			if (GetOpenFileName(&ofn)) {
				SDL_PauseAudio(0);
				WindowState = VPNES_QUIT;
				opennew = -1;
				return -1;
			}
			SDL_PauseAudio(0);
			break;
		case ID_FILE_CLOSE:
			WindowState = VPNES_QUIT;
			quit = 0;
			return -1;
		case ID_FILE_EXIT:
			WindowState = VPNES_QUIT;
			return -1;
		case ID_CPU_SOFTWARERESET:
			WindowState = VPNES_RESET;
			return -1;
		case ID_CPU_HARDWARERESET:
			WindowState = VPNES_QUIT;
			opennew = -1;
			return -1;
		case ID_CPU_SAVESTATE:
			WindowState = VPNES_SAVESTATE;
			return -1;
		case ID_CPU_LOADSTATE:
			WindowState = VPNES_LOADSTATE;
			return -1;
	}
#endif
	return 0;
}

/* Запуск GUI */
int InteractiveGUI() {
	int ret = 0;
	SDL_Event event;

	FileName[0] = '\0';
	do {
		quit = -1;
		WindowState = -1;
		EnableMenuItem(Menu, ID_FILE_CLOSE, MF_GRAYED);
		EnableMenuItem(Menu, ID_FILE_SETTINGS, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_SOFTWARERESET, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_HARDWARERESET, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_SAVESTATE, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_LOADSTATE, MF_GRAYED);
		while (WindowState != VPNES_QUIT) {
			SDL_WaitEvent(&event);
			switch (event.type) {
				case SDL_QUIT:
					WindowState = VPNES_QUIT;
					break;
				case SDL_SYSWMEVENT:
					InteractiveDispatcher(event.syswm.msg);
					break;
			}
		}
		EnableMenuItem(Menu, ID_FILE_CLOSE, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_SOFTWARERESET, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_HARDWARERESET, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_SAVESTATE, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_LOADSTATE, MF_ENABLED);
		while (opennew) {
			opennew = 0;
			ret = StartGUI(FileName);
		}
		ClearWindow();
	} while (!quit);
	return ret;
}
