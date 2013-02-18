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

#include <cstdio>
#include <cstring>

namespace vpnes_gui {

/* CWindow */

CWindow::CWindow(const char *DefaultFileName, CAudio *Audio, CInput *Input) {
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
	::SDL_WM_SetCaption("VPNES " VERSION, NULL);
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
void CWindow::ProcessAction(WindowActions Action) {
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
#if defined(VPNES_USE_TTF)
			SetText("Pause");
#endif
			pAudio->StopAudio();
			PauseState = psPaused;
			break;
		case waStep:
#if defined(VPNES_USE_TTF)
			SetText("Step");
#endif
			pAudio->StopAudio();
			PauseState = psStep;
			break;
		case waResume:
#if defined(VPNES_USE_TTF)
			SetText("Resume");
#endif
			pAudio->ResumeAudio();
			PauseState = psPlay;
			break;
		case waSpeed:
			PlayRate = PlayRate * 2;
			if (PlayRate > 4)
				PlayRate = 0.5;
#if defined(VPNES_USE_TTF)
			sprintf(sbuf, "Speed: x%.1f", PlayRate);
			SetText(sbuf);
#endif
			pAudio->ChangeSpeed(PlayRate);
			break;
		default:
			break;
	}
}

/* Выполнить обработку сообщений */
CWindow::WindowState CWindow::ProcessMessages() {
	SDL_Event Event;
	bool Quit = false;
	WindowActions Action;
	
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
		Action = waNone;
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
				switch (Event.key.keysym.sym) {
					case SDLK_F1:
						Action = waSoftReset;
						break;
					case SDLK_F2:
						Action = waHardReset;
						break;
					case SDLK_F5:
						Action = waSaveState;
						break;
					case SDLK_F7:
						Action = waLoadState;
						break;
					case SDLK_PAUSE:
						Action = waPause;
						break;
					case SDLK_SPACE:
						Action = waStep;
						break;
					case SDLK_TAB:
						Action = waSpeed;
						break;
					default:
						break;
				}
//					Action = GetKeyboardAction(&Event.key.keysym);
				if ((Action == waPause) && (PauseState == psPaused))
					Action = waResume;
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
		if (Action != waNone) {
			ProcessAction(Action);
			if (CurState != wsNormal)
				Quit = true;
		}
		if (Quit) {
			/* Освобождаем очередь */
			while (::SDL_PollEvent(&Event));
			return CurState;
		}
	}
#if !defined(VPNES_DISABLE_SYNC)
	pSyncManager->Sync(PlayRate);
#endif
#if defined(VPNES_SHOW_FPS)
	static int cur_frame = 0;
	char buf[20];
	static Uint32 fpst = 0;
	Sint32 passed;
	double fps;

	/* FPS */
	passed = ::SDL_GetTicks() - fpst;
	if (passed > 1000) {
		fps = cur_frame * 1000.0 / passed;
		snprintf(buf, 20, "%.3lf", fps);
		::SDL_WM_SetCaption(buf, NULL);
		fpst = ::SDL_GetTicks();
		cur_frame = 0;
	}
	cur_frame++;
#endif
	if (MouseState && ((Uint32) (::SDL_GetTicks() - MouseTimer) > 5000)) {
		::SDL_ShowCursor(SDL_DISABLE);
		MouseState = false;
	}
	if (PauseState == psPaused)
		return wsPause;
	if (PauseState == psStep)
		PauseState = psPaused;
	return CurState;
}

/* Изменить размер */
void CWindow::UpdateSizes(int Width, int Height) {
	_Width = Width;
	_Height = Height;
	PlayRate = 1.0;
	PauseState = psPlay;
	InitializeScreen();
}

/* Инициализация буфера */
void CWindow::InitializeScreen() {
	Screen = SDL_SetVideoMode(_Width * 2, _Height * 2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (Screen == NULL)
		throw CGenericException("Couldn't reinitialize video");
#if !defined(VPNES_DISABLE_SYNC)
	pSyncManager->SyncReset();
#endif
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
