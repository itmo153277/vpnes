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

#ifndef __WINDOW_H_
#define __WINDOW_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <SDL.h>
#include "audio.h"
#include "input.h"
#if defined(VPNES_CONFIGFILE)
#include "configfile.h"
#endif

#ifdef _WIN32
/* Minimal version */
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#endif

namespace vpnes_gui {

#ifndef VPNES_MAX_PATH
#ifdef MAX_PATH
#define VPNES_MAX_PATH MAX_PATH
#else
#define VPNES_MAX_PATH 256
#endif
#endif

#if !defined(VPNES_DISABLE_SYNC)
/* Интерфейс для синхронизации */
struct CSyncManager {
public:
	/* Остановить синхронизацию */
	virtual void SyncStop() = 0;
	/* Продолжить синхронизацию */
	virtual Uint32 SyncResume() = 0;
	/* Сбросить синхронизацию */
	virtual void SyncReset() = 0;
	/* Синхронизировать время */
	virtual void Sync(double PlayRate) = 0;
};
#endif

/* Главное окно */
class CWindow: CInputConfigProcessor {
public:
	/* Возможные ответы обработчика окна */
	enum WindowState {
		wsNormal,
		wsQuit,
		wsUpdateBuffer,
		wsPause,
		wsSoftReset,
		wsHardReset,
		wsSaveState,
		wsLoadState
	};
private:
	/* События */
	enum WindowActions {
		waNone,
		waSoftReset,
		waHardReset,
		waSaveState,
		waLoadState,
		waChangeSlot,
		waPause,
		waStep,
		waResume,
		waSpeed
	};

	/* Поверхность окна */
	SDL_Surface *Screen;
	/* Размеры */
	int _Width;
	int _Height;
	/* Текущее состояние окна */
	WindowState CurState;
	/* Имя файла */
	char FileName[VPNES_MAX_PATH];
#ifdef _WIN32
	/* Экземпляр */
	HINSTANCE Instance;
	/* Дексриптор */
	HWND Handle;
	/* Иконка */
	HICON Icon;
#endif
#if defined(VPNES_USE_TTF)
	/* Буфер для сообщений */
	char sbuf[20];
	/* Текущее собщение */
	const char *WindowText;
	/* Обновить сообщение */
	bool UpdateText;
#endif
#if !defined(VPNES_DISABLE_SYNC)
	/* Обраточик для синхронизации */
	CSyncManager *pSyncManager;
#endif
#if defined(VPNES_CONFIGFILE)
	/* Обработчик конфигурации */
	CConfig *pConfig;
#endif
	/* Обработчик звука */
	CAudio *pAudio;
	/* Обработчик ввода */
	CInput *pInput;
	/* Таймер указателя мыши */
	Uint32 MouseTimer;
	/* Текущее состояние мыши */
	bool MouseState;
	/* Флаг паузы */
	enum {
		psPlay,
		psPaused,
		psStep
	} PauseState;
	/* Скорость эмуляции */
	double PlayRate;
#if defined(VPNES_INTERACTIVE)
	/* Информация о файле */
	const char *InfoText;
	/* Режим отладки */
	bool DebugMode;
	/* Флаг открытия файла */
	bool OpenFile;
	/* Флаг готовности файла */
	bool FileReady;
	/* Буфер для имени файла */
	char FileNameBuf[VPNES_MAX_PATH];
#ifdef _WIN32
	/* Дескриптор меню */
	HMENU Menu;
	/* Обработчик сообщений */
	WNDPROC OldWndProc;

	/* Обработчик сообщений */
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	/* Диалоговое окно «О программе» */
	static BOOL CALLBACK AboutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

	/* Смена состояния */
	void UpdateState(bool Run);
	/* Обработчик сообщений */
	bool InteractiveDispatch(SDL_SysWMmsg *Msg);
#endif

	/* Обработка событий */
	void ProcessAction(WindowActions Action);
	/* Инициализация буфера */
	void InitializeScreen();
public:
	CWindow(const char *DefaultFileName, CAudio *Audio, CInput *Input);
	~CWindow();

	/* Выполнить обработку сообщений */
	WindowState ProcessMessages();
	/* Изменить размер */
	void UpdateSizes(int Width, int Height);
	/* Поверхность окна */
	inline SDL_Surface * const &GetSurface() const { return Screen; }
#if defined(VPNES_INTERACTIVE)
	/* Имя файла */
	const char *GetFileName();
	/* Флаг открытия нового файла */
	inline const bool &CanOpenFile() const { return OpenFile; }
	/* Информация о файле */
	inline const char *&GetInfoText() { return InfoText; }
#else
	/* Имя файла */
	inline const char *GetFileName() { return FileName; }
#endif
	/* Текущее состояние окна */
	inline const WindowState &GetWindowState() const { return CurState; }
#if defined(VPNES_USE_TTF)
	/* Сообщение */
	inline const char *GetText() const { return WindowText; }
	/* Обновить текст */
	inline bool &GetUpdateTextFlag() { return UpdateText; }
	/* Установить сообщение */
	inline void SetText(const char *Text) { WindowText = Text; UpdateText = true; }
#endif
	/* Очистить окно */
	void ClearWindow();
#ifdef _WIN32
	/* Экземпляр */
	inline const HINSTANCE &GetInstance() const { return Instance; }
	/* Дескриптор */
	inline const HWND &GetWindowHandle() const { return Handle; }
#endif
	/* Вывести сообщение об ошибке */
	void PrintErrorMsg(const char *Msg);
#if !defined(VPNES_DISABLE_SYNC)
	/* Добавить обработчик синхронизации */
	inline void AppendSyncManager(CSyncManager *SyncManager) {
		pSyncManager = SyncManager;
	}
#endif
#if defined(VPNES_CONFIGFILE)
	/* Добавить обработчик конфигурации */
	inline void AppendConfig(CConfig *Config) {
		pConfig = Config;
	}
#endif
};

}

#endif
