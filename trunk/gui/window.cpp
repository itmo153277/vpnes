/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2013  Ivanov Viktor

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

#include "window.h"

#include <SDL_syswm.h>

#ifdef _WIN32
#include "win32-res/win32-res.h"
#endif

#include <cstring>

namespace vpnes_gui {

/* CWindow */

CWindow::CWindow(char *DefaultFileName, CAudio *Audio, CInput *Input) {
	if (DefaultFileName == NULL)
		throw CGenericException("Nothing to run");
	strcpy(FileName, DefaultFileName);
	pAudio = Audio;
	pInput = Input;
#if !defined(VPNES_DISABLE_SYNC)
	pSyncManager = NULL;
#endif
#if defined(VPNES_CONFIGFILE)
	pConfig = NULL;
#endif
	CurState = wsNormal;
	PauseState = 0;
#if defined(VPNES_USE_TTF)
	WindowText = NULL;
	UpdateText = false;
#endif
	MouseState = true;
	MouseTimer = ::SDL_GetTicks();
#ifdef _WIN32
	SDL_SysWMinfo WMInfo;

	/* Изменяем параметры окна SDL */
	SDL_VERSION(&WMInfo.version)
	::SDL_GetWMInfo(&WMInfo);
	Handle = WMInfo.window;
	Instance = ::GetModuleHandle(NULL);
	Icon = ::LoadIcon(Instance, MAKEINTRESOURCE(IDI_MAINICON));
	::SetClassLong(Handle, GCL_HICON, (LONG) Icon);
#endif
	::SDL_WM_SetCaption("VPNES" VERSION, NULL);
	Screen = ::SDL_SetVideoMode(512, 448, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (Screen == NULL)
		throw CGenericException("Couldn't set up video mode");
}

CWindow::~CWindow() {
#ifdef _WIN32
	::DestroyIcon(Icon);
#endif
}

/* Обработка событий */
void CWindow::ProcessAction(int Action) {
	switch (Action) {
		case waSoftReset:
			CurState = wsSoftReset;
			break;
		case waHardReset:
			CurState = wsHardReset;
			break;
		case waSaveState:
			CurState = wsSaveState;
			break;
		case waLoadState:
			CurState = wsLoadState;
			break;
		case waChangeSlot:
			break;
		case waPause:
			SetText("Pause");
			PauseState = -1;
			break;
		case waStep:
			SetText("Step");
			PauseState = -2;
			break;
		case waResume:
			SetText("Resume");
			PauseState = 0;
			break;
		case waSpeed:
			PlayRate = PlayRate * 2;
			if (PlayRate > 4)
				PlayRate = 0.5;
			break;
	}
}

/* Выполнить обработку сообщений */
CWindow::WindowState CWindow::ProcessMessages() {
	SDL_Event Event;
	bool Quit = false;
	int Action;
	
	do {
		CurState = wsNormal;
		while (::SDL_PollEvent(&Event)) {
			if (CurState == wsUpdateBuffer) {
				/* Возвращаем событие в очередь */
				SDL_PushEvent(&Event);
				/* Реинициализация */
				InitializeScreen();
				CurState = wsNormal;
				/* Принуждаем к реинициализации CVideo */
				return wsUpdateBuffer;
			}
			Action = 0;
			switch(Event.type) {
				case SDL_QUIT:
					CurState = wsQuit;
				case SDL_USEREVENT:
					Quit = true;
					break;
				case SDL_MOUSEMOTION:
					::SDL_ShowCursor(SDL_ENABLE);
					MouseTimer = SDL_GetTicks();
					MouseState = true;
					break;
				case SDL_KEYDOWN:
					pInput->ProcessKeyboard(&Event.key);
					break;
				case SDL_KEYUP:
					pInput->ProcessKeyboard(&Event.key);
					Action = GetKeyboardAction(&Event.key.keysym);
					if ((Action == (waPause - 1)) && (PauseState < 0))
						Action = waResume + 1;
					break;
				case SDL_JOYAXISMOTION:
					pInput->ProcessGamepadAxis(&Event.jaxis);
					break;
				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
					pInput->ProcessGamepadButton(&Event.jbutton);
					break;
				case SDL_JOYHATMOTION:
					pInput->ProcessGamepadHat(&Event.jhat);
					break;
			}
			if (Action > 0) {
				ProcessAction(Action - 1);
				if (CurState != wsNormal)
					Quit = true;
			}
			if (Quit) {
				/* Освобождаем очередь */
				while (SDL_PollEvent(&Event));
				break;
			}
		}
		pSyncManager->Sync(PlayRate);
	} while (PauseState < 0);
	if (PauseState > 0)
		PauseState = -1;
	return CurState;
}

/* Изменить размер */
void CWindow::UpdateSizes(int Width, int Height) {
	_Width = Width;
	_Height = Height;
	PlayRate = 1.0;
	InitializeScreen();
}

/* Инициализация буфера */
void CWindow::InitializeScreen() {
	Screen = SDL_SetVideoMode(_Width * 2, _Height * 2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (Screen == NULL)
		throw CGenericException("Couldn't reinitialize video");
	pSyncManager->SyncReset();
}

/* Очистить окно */
void CWindow::ClearWindow() {
	if (SDL_MUSTLOCK(Screen))
		::SDL_LockSurface(Screen);
	::SDL_FillRect(Screen, NULL, Screen->format->colorkey);
	if (SDL_MUSTLOCK(Screen))
		::SDL_UnlockSurface(Screen);
	::SDL_Flip(Screen);
}

}
