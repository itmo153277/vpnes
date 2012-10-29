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
#include <malloc.h>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include "win32-res/win32-res.h"
#endif

int DisableInteractive = -1;
#ifdef _WIN32
HMENU Menu = INVALID_HANDLE_VALUE;
WNDPROC OldWndProc = NULL;
char FileName[MAX_PATH];
int opennew = 0;
int quit = 0;
#endif

/* Обработчик сообщений */
int InteractiveDispatcher(SDL_SysWMmsg *Msg) {
#ifdef _WIN32
	OPENFILENAME ofn;
	HDROP hDrop;

	switch (Msg->msg) {
		case WM_DROPFILES:
			hDrop = (HDROP) Msg->wParam;
			DragQueryFile(hDrop, 0, FileName, MAX_PATH);
			DragFinish(hDrop);
			WindowState = VPNES_QUIT;
			opennew = -1;
			return -1;
		case WM_ENTERSIZEMOVE:
		case WM_ENTERMENULOOP:
			Pause();
			break;
		case WM_EXITSIZEMOVE:
		case WM_EXITMENULOOP:
			Resume();
			break;
		case WM_COMMAND:
			switch (Msg->wParam) {
				case ID_FILE_OPEN:
					Pause();
					memset(&ofn, 0, sizeof(OPENFILENAME));
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = WindowHandle;
					ofn.hInstance = Instance;
					ofn.lpstrFilter = "iNES 1.0 Files (*.nes)\0*.nes\0"
						"All Files (*.*)\0*.*\0";
					ofn.lpstrFile = FileName;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
						OFN_ALLOWMULTISELECT;
					ofn.lpstrDefExt = "nes";
					if (GetOpenFileName(&ofn)) {
						WindowState = VPNES_QUIT;
						opennew = -1;
						return -1;
					}
					Resume();
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
			break;
	}
#endif
	return 0;
}

#ifdef _WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	SDL_SysWMmsg Msg;
	SDL_Event Event;

	switch (msg) {
		case WM_SYSKEYUP:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_SYSKEYDOWN:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		default:
			Msg.msg = msg;
			Msg.wParam = wParam;
			Msg.lParam = lParam;
			if (InteractiveDispatcher(&Msg) < 0) {
				Event.type = SDL_USEREVENT;
				SDL_PushEvent(&Event);
			}
			return CallWindowProc(OldWndProc, hwnd, msg, wParam, lParam);
	}
}
#endif

/* Инициализация интерактивного режима */
void InitInteractive(void) {
	if (DisableInteractive)
		return;
#ifdef _WIN32
	/* Меню */
	Menu = LoadMenu(Instance, MAKEINTRESOURCE(IDR_MAINMENU));
	SetMenu(WindowHandle, Menu);
	SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
	/* Использовать Drag'n'Drop */
	DragAcceptFiles(WindowHandle, 1);
	/* Перехватчик сообщений */
	OldWndProc = (WNDPROC) SetWindowLong(WindowHandle, GWL_WNDPROC, (LONG) WndProc);
#endif
	return;
}

/* Завершение интерактивного режима */
void DestroyInteractive(void) {
	if (DisableInteractive)
		return;
#ifdef _WIN32
	DestroyMenu(Menu);
#endif
}

/* Запуск GUI */
int InteractiveGUI() {
	int ret = 0;
	SDL_Event event;

	FileName[0] = '\0';
	do {
		quit = -1;
		ret = 0;
		WindowState = -1;
#ifdef _WIN32
		EnableMenuItem(Menu, ID_FILE_CLOSE, MF_GRAYED);
		EnableMenuItem(Menu, ID_FILE_SETTINGS, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_SOFTWARERESET, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_HARDWARERESET, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_SAVESTATE, MF_GRAYED);
		EnableMenuItem(Menu, ID_CPU_LOADSTATE, MF_GRAYED);
#endif
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
#ifdef _WIN32
		EnableMenuItem(Menu, ID_FILE_CLOSE, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_SOFTWARERESET, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_HARDWARERESET, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_SAVESTATE, MF_ENABLED);
		EnableMenuItem(Menu, ID_CPU_LOADSTATE, MF_ENABLED);
#endif
		while (opennew) {
			opennew = 0;
			ret = StartGUI(FileName);
		}
		ClearWindow();
		if (ret < 0)
			quit = 0;
	} while (!quit);
	return ret;
}
