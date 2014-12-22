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

#if defined(VPNES_USE_TTF)
#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#endif

#include <exception>
#include <cwchar>

namespace vpnes_gui {

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
		waSyncInc,
		waSyncDec,
		waChangeSlot,
		waWAVRecordStart,
		waWAVRecordStop,
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
	VPNES_PATH FileName[VPNES_MAX_PATH];
	/* WAV файл */
	VPNES_PATH WAVFile[VPNES_MAX_PATH];
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
	wchar_t sbuf[20];
	/* Текущее собщение */
	std::u16string WindowText;
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
	const wchar_t *InfoText;
	/* Режим отладки */
	bool DebugMode;
	/* Флаг открытия файла */
	bool OpenFile;
	/* Флаг готовности файла */
	bool FileReady;
	/* Буфер для имени файла */
	VPNES_PATH FileNameBuf[VPNES_MAX_PATH];
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
	/* Сбросить состояние указателя мыши */
	void ResetMouse();
public:
	CWindow(VPNES_PATH *DefaultFileName, CAudio *Audio, CInput *Input);
	~CWindow();

	/* Выполнить обработку сообщений */
	WindowState ProcessMessages();
	/* Изменить размер */
	void UpdateSizes(int Width, int Height);
	/* Поверхность окна */
	inline SDL_Surface * const &GetSurface() const { return Screen; }
#if defined(VPNES_INTERACTIVE)
	/* Имя файла */
	VPNES_PATH *GetFileName();
	/* Флаг открытия нового файла */
	inline const bool &CanOpenFile() const { return OpenFile; }
	/* Информация о файле */
	inline const wchar_t *&GetInfoText() { return InfoText; }
#else
	/* Имя файла */
	inline VPNES_PATH *GetFileName() { return FileName; }
#endif
	/* Текущее состояние окна */
	inline const WindowState &GetWindowState() const { return CurState; }
#if defined(VPNES_USE_TTF)
	/* Сообщение */
	inline const char16_t *GetText() const { return WindowText.c_str(); }
	/* Обновить текст */
	inline bool &GetUpdateTextFlag() { return UpdateText; }
	/* Установить сообщение */
	void SetText(const wchar_t *Text) {
#ifdef HAVE_ICONV

		iconv_t cd = iconv_open("UCS-2", "WCHAR_T");

		if (cd < 0) {
#endif
			std::u16string OutStr(Text, Text + wcslen(Text));

			WindowText = OutStr;
#ifdef HAVE_ICONV
		} else {
			char16_t *OutStr = NULL;

			try {
				ICONV_CONST char *InBuf = (ICONV_CONST char *)
					reinterpret_cast<const char *>(Text);
				size_t InSize = (wcslen(Text) + 1) * sizeof(wchar_t);
				OutStr = new char16_t[InSize];
				ICONV_CONST char *OutBuf = (ICONV_CONST char *)
					reinterpret_cast<const char *>(OutStr);
				size_t OutSize = InSize * sizeof(char16_t);

				iconv(cd, &InBuf, &InSize, &OutBuf, &OutSize);
				WindowText = OutStr;
				delete [] OutStr;
				iconv_close(cd);
			} catch (std::exception &e) {
				delete [] OutStr;
				iconv_close(cd);
				throw e;
			}
		}
#endif
		UpdateText = true;
	}
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
	/* Скорость вывода */
	inline double GetPlayRate() const { return PlayRate; }
};

}

#endif
