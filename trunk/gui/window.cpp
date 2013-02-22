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
#if defined(VPNES_INTERACTIVE)
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#endif
#include "win32-res/win32-res.h"
#endif

#include <cstdio>
#include <cstring>
#include <iostream>

namespace vpnes_gui {

/* CWindow */

CWindow::CWindow(const char *DefaultFileName, CAudio *Audio, CInput *Input) {
	FileName[VPNES_MAX_PATH - 1] = '\0';
#if !defined(VPNES_INTERACTIVE)
	if (DefaultFileName == NULL)
		throw CGenericException("Nothing to run");
	strncpy(FileName, DefaultFileName, VPNES_MAX_PATH - 1);
#else
	FileNameBuf[VPNES_MAX_PATH - 1] = '\0';
	DebugMode = false;
	if (DefaultFileName != NULL) {
		if (!std::clog.fail())
			DebugMode = true;
		FileReady = true;
		strncpy(FileNameBuf, DefaultFileName, VPNES_MAX_PATH - 1);
	} else {
		FileReady = false;
		FileNameBuf[0] = '\0';
	}
#endif
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
#ifdef _WIN32
	SDL_SysWMinfo WMInfo;

	/* Изменяем параметры окна SDL */
	SDL_VERSION(&WMInfo.version)
	::SDL_GetWMInfo(&WMInfo);
	Handle = WMInfo.window;
	Instance = ::GetModuleHandle(NULL);
	Icon = ::LoadIcon(Instance, MAKEINTRESOURCE(IDI_MAINICON));
	::SetClassLong(Handle, GCL_HICON, (LONG) Icon);
#if defined(VPNES_INTERACTIVE)
	if (!DebugMode) {
		INITCOMMONCONTROLSEX icc;

		/* Инициализация контролов */
		memset(&icc, 0, sizeof(INITCOMMONCONTROLSEX));
		icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icc.dwICC = ICC_WIN95_CLASSES;
		::InitCommonControlsEx(&icc);
		/* Меню */
		Menu = ::LoadMenu(Instance, MAKEINTRESOURCE(IDM_MAINMENU));
		::SetMenu(Handle, Menu);
		::SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
		/* Использовать Drag'n'Drop */
		::DragAcceptFiles(Handle, 1);
		/* Перехватчик сообщений */
		OldWndProc = (WNDPROC) ::SetWindowLongPtr(Handle, GWL_WNDPROC, (LONG_PTR) WndProc);
		/* Ассоциируем указатель и дескриптор */
		::SetWindowLongPtr(Handle, GWL_USERDATA, (LONG_PTR) this);
	}
#endif
#endif
	::SDL_WM_SetCaption("VPNES " VERSION, NULL);
	Screen = ::SDL_SetVideoMode(512, 448, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (Screen == NULL)
		throw CGenericException("Couldn't set up video mode");
}

CWindow::~CWindow() {
#ifdef _WIN32
#if defined(VPNES_INTERACTIVE)
	if (!DebugMode) {
		::DestroyMenu(Menu);
		/* Удаляем обработчик */
		::SetWindowLongPtr(Handle, GWL_WNDPROC, (LONG_PTR) OldWndProc);
	}
#endif
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
			pAudio->StopDevice();
			PauseState = psPaused;
#if defined(VPNES_INTERACTIVE)
			UpdateState(true);
#endif
			break;
		case waStep:
#if defined(VPNES_USE_TTF)
			SetText("Step");
#endif
			pAudio->StopDevice();
			PauseState = psStep;
#if defined(VPNES_INTERACTIVE)
			UpdateState(true);
#endif
			break;
		case waResume:
#if defined(VPNES_USE_TTF)
			SetText("Resume");
#endif
			pAudio->ResumeDevice();
			PauseState = psPlay;
#if defined(VPNES_INTERACTIVE)
			UpdateState(true);
#endif
			break;
		case waSpeed:
			PlayRate = PlayRate * 2;
			if (PlayRate > 4)
				PlayRate = 0.5;
#if defined(VPNES_USE_TTF)
			snprintf(sbuf, 20, "Speed: x%.1lf", PlayRate);
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
#if defined(VPNES_INTERACTIVE)
			case SDL_SYSWMEVENT:
				Quit = InteractiveDispatch(Event.syswm.msg);
				break;
#endif
			case SDL_MOUSEMOTION:
				::SDL_ShowCursor(SDL_ENABLE);
				MouseTimer = ::SDL_GetTicks();
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
		if (Quit)
			break;
	}
	if (PauseState == psPlay)
		pAudio->ResumeDevice();
#if !defined(VPNES_DISABLE_SYNC)
	MouseTimer += pSyncManager->SyncResume();
	pSyncManager->Sync(PlayRate);
#endif
	if (Quit) {
		/* Освобождаем очередь */
		while (::SDL_PollEvent(&Event));
		return CurState;
	}
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
	if (MouseState && ((Uint32) (::SDL_GetTicks() - MouseTimer) > 3000)) {
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
	MouseState = true;
	MouseTimer = ::SDL_GetTicks();
#if defined(VPNES_INTERACTIVE)
	UpdateState(true);
#endif
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

/* Вывести сообщение об ошибке */
void CWindow::PrintErrorMsg(const char *Msg) {
	pAudio->StopDevice();
#ifdef _WIN32
	MessageBox(Handle, Msg, "Error", MB_ICONHAND);
#else
	std::clog << Msg << std::endl;
#endif
#if defined(VPNES_INTERACTIVE)
	if (!DebugMode)
		OpenFile = true;
#endif
}

#if defined(VPNES_INTERACTIVE)

#ifdef _WIN32
/* Обработчик сообщений */
LRESULT CALLBACK CWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	SDL_SysWMmsg Msg;
	SDL_Event Event;
	CWindow *Window;

	switch (msg) {
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			return ::DefWindowProc(hwnd, msg, wParam, lParam);
		default:
			Msg.msg = msg;
			Msg.wParam = wParam;
			Msg.lParam = lParam;
			Window = (CWindow *) ::GetWindowLongPtr(hwnd, GWL_USERDATA);
			if (Window->InteractiveDispatch(&Msg)) {
				Event.type = SDL_USEREVENT;
				SDL_PushEvent(&Event);
			}
			return ::CallWindowProc(Window->OldWndProc, hwnd, msg, wParam, lParam);
	}
}

/* О программе */
BOOL CALLBACK CWindow::AboutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_INITDIALOG:
			SetDlgItemText(hwnd, IDC_STATICINFO, (const char *) lParam);
			return TRUE;
		case WM_COMMAND:
			if (wParam == IDCANCEL) {
				EndDialog(hwnd, wParam);
				return TRUE;
			}
			break;
	}
	return FALSE;
}
#endif

/* Обработчик сообщений */
bool CWindow::InteractiveDispatch(SDL_SysWMmsg *Msg) {
#ifdef _WIN32
	OPENFILENAME ofn;
	HDROP hDrop;

	switch (Msg->msg) {
		case WM_DROPFILES:
			hDrop = (HDROP) Msg->wParam;
			::DragQueryFile(hDrop, 0, FileNameBuf, VPNES_MAX_PATH);
			::DragFinish(hDrop);
			OpenFile = true;
			FileReady = true;
			CurState = wsQuit;
			return true;
		case WM_ENTERSIZEMOVE:
		case WM_ENTERMENULOOP:
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_SIZING:
		case WM_MOVING:
#if !defined(VPNES_DISABLE_SYNC)
			pSyncManager->SyncStop();
#endif
			pAudio->StopDevice();
			break;
		case WM_COMMAND:
			switch (Msg->wParam) {
				case ID_FILE_OPEN:
#if !defined(VPNES_DISABLE_SYNC)
					pSyncManager->SyncStop();
#endif
					pAudio->StopDevice();
					memset(&ofn, 0, sizeof(OPENFILENAME));
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = Handle;
					ofn.hInstance = Instance;
					ofn.lpstrFilter = "iNES 1.0 Files (*.nes)\0*.nes\0"
						"All Files (*.*)\0*.*\0";
					ofn.lpstrFile = FileNameBuf;
					ofn.nMaxFile = VPNES_MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES |
						OFN_HIDEREADONLY;
					ofn.lpstrDefExt = "nes";
					if (::GetOpenFileName(&ofn)) {
						OpenFile = true;
						FileReady = true;
						CurState = wsQuit;
						return true;
					}
					break;
				case ID_FILE_CLOSE:
					OpenFile = true;
					FileReady = false;
					CurState = wsQuit;
					return true;
				case ID_FILE_EXIT:
					OpenFile = false;
					CurState = wsQuit;
					return true;
				case ID_CPU_SOFTWARERESET:
					ProcessAction(waSoftReset);
					return true;
				case ID_CPU_HARDWARERESET:
					ProcessAction(waHardReset);
					return true;
				case ID_CPU_PAUSE:
					ProcessAction(waPause);
					return true;
				case ID_CPU_RESUME:
					ProcessAction(waResume);
					return true;
				case ID_CPU_STEP:
					ProcessAction(waStep);
					return true;
				case ID_CPU_SAVESTATE:
					ProcessAction(waSaveState);
					return true;
				case ID_CPU_LOADSTATE:
					ProcessAction(waLoadState);
					return true;
				case ID_CPU_ABOUT:
#if !defined(VPNES_DISABLE_SYNC)
					pSyncManager->SyncStop();
#endif
					pAudio->StopDevice();
					::DialogBoxParam(Instance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), Handle,
						(DLGPROC) AboutProc, (LPARAM) InfoText);
					break;
			}
			break;
	}
#endif
	return false;
}

/* Получить имя файла */
const char *CWindow::GetFileName() {
	if (!FileReady) {
		const char DefaultInfoText[] = "No ROM";
		SDL_Event Event;

		UpdateState(false);
		/* Курсор мыши должен отображаться всегда */
		::SDL_ShowCursor(SDL_ENABLE);
		CurState = wsNormal;
		InfoText = DefaultInfoText;
		do {
			::SDL_WaitEvent(&Event);
			switch (Event.type) {
				case SDL_QUIT:
					CurState = wsQuit;
					break;
				case SDL_SYSWMEVENT:
					InteractiveDispatch(Event.syswm.msg);
					break;
			}
		} while (CurState != wsQuit);
		/* Освобождаем очередь */
		while (::SDL_PollEvent(&Event));
		if (!FileReady)
			return NULL;
	}
	FileReady = false;
	OpenFile = false;
	strncpy(FileName, FileNameBuf, VPNES_MAX_PATH - 1);
	FileNameBuf[0] = '\0';
	return FileName;
}

/* Смена состояния */
void CWindow::UpdateState(bool Run) {
#ifdef _WIN32
	if (Run) {
		::EnableMenuItem(Menu, ID_FILE_CLOSE, MF_ENABLED);
		::EnableMenuItem(Menu, ID_FILE_SETTINGS, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_SOFTWARERESET, MF_ENABLED);
		::EnableMenuItem(Menu, ID_CPU_HARDWARERESET, MF_ENABLED);
		if (PauseState == psPlay) {
			::EnableMenuItem(Menu, ID_CPU_PAUSE, MF_ENABLED);
			::EnableMenuItem(Menu, ID_CPU_RESUME, MF_GRAYED);
		} else {
			::EnableMenuItem(Menu, ID_CPU_PAUSE, MF_GRAYED);
			::EnableMenuItem(Menu, ID_CPU_RESUME, MF_ENABLED);
		}
		::EnableMenuItem(Menu, ID_CPU_STEP, MF_ENABLED);
		::EnableMenuItem(Menu, ID_CPU_SAVESTATE, MF_ENABLED);
		::EnableMenuItem(Menu, ID_CPU_LOADSTATE, MF_ENABLED);
	} else {
		::EnableMenuItem(Menu, ID_FILE_CLOSE, MF_GRAYED);
		::EnableMenuItem(Menu, ID_FILE_SETTINGS, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_SOFTWARERESET, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_HARDWARERESET, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_PAUSE, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_RESUME, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_STEP, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_SAVESTATE, MF_GRAYED);
		::EnableMenuItem(Menu, ID_CPU_LOADSTATE, MF_GRAYED);
	}
#endif
}

#endif

}
