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
	virtual void SyncResume() = 0;
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
		wsSoftReset,
		wsHardReset,
		wsSaveState,
		wsLoadState
	};
private:
	/* Поверхность окна */
	SDL_Surface *Screen;
	/* Текущее состояние окна */
	WindowState CurState;
	/* Имя файла */
	char FileName[VPNES_MAX_PATH];
#if defined(VPNES_USE_TTF)
	/* Текущее собщение */
	char *WindowText;
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
public:
	CWindow(char *DefaultFileName, CAudio *Audio, CInput *Input);
	~CWindow();

	/* Выполнить обработку сообщений */
	WindowState ProcessMessages();
	/* Поверхность окна */
	inline SDL_Surface * const &GetSurface() const { return Screen; }
	/* Имя файла */
	inline const char *GetFileName() const { return FileName; }
	/* Текущее состояние окна */
	inline const WindowState &GetWindowState() const { return CurState; }
#if defined(VPNES_USE_TTF)
	/* Сообщение */
	inline char *&GetWindowText() { return WindowText; }
#endif
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
