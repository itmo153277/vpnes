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

#ifndef __VIDEO_H_
#define __VIDEO_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#if defined(VPNES_USE_TTF)
#include <SDL_ttf.h>
#endif
#include "../nes/frontend.h"
#include "window.h"
#if !defined(VPNES_CONFIGFILE)
#include "configfile.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace vpnes_gui {

/* Зависимости */
class CVideoDependencies:
	public vpnes::CVideoFrontend
#if !defined(VPNES_DISABLE_SYNC)
	, public CSyncManager
#endif
#if defined(VPNES_CONFIGFILE)
	, public CConfigProcessor
#endif
	{};

/* Обработчик видео */
class CVideo: public CVideoDependencies {
private:
	/* Указатель на главное окно */
	CWindow *pWindow;
	/* Внутренняя поверхность */
	SDL_Surface *InternalSurface;
	/* Размеры */
	int _Width;
	int _Height;
#if defined(VPNES_USE_TTF)
	/* Используемый шрифт */
	TTF_Font *Font;
	/* Поверхность с текстом */
	SDL_Surface *TextSurface;
	/* Таймер текста */
	Uint32 TextTimer;
#ifdef _WIN32
	/* Шрифт в ресурсах */ 
	HGLOBAL ResourceHandle;
	HRSRC ResourceInfo;
#endif
#endif
#if !defined(VPNES_DISABLE_SYNC)
	/* Отсчет времени кадра */
	Uint32 FrameStart;
	/* Контроль времени */
	Uint32 FrameTimeCheck;
	/* Накапливаемая ошибка */
	double Delta;
	/* Смещение */
	Uint32 Jitter;
	/* Время остановки */
	Uint32 StopTime;
#if !defined(VPNES_DISABLE_FSKIP)
	/* Пропускаем фрейм */
	bool SkipFrame;
	/* Всего пропущенно */
	int FramesSkipped;
	/* Пропущено времени */
	Uint32 SkippedTime;
#endif
#endif

	/* Обновить палитру */
	void UpdatePalette();
	/* Обновить поверхность */
	SDL_Surface *UpdateSurface();
public:
	CVideo(CWindow *Window);
	~CVideo();

	/* Обновить размер */
	void UpdateSizes(int Width, int Height);
	/* Обновляем кадр */
	bool UpdateFrame(double FrameTime);
#if !defined(VPNES_DISABLE_SYNC)
	/* Остановить синхронизацию */
	void SyncStop();
	/* Продолжить синхронизацию */
	Uint32 SyncResume();
	/* Сбросить синхронизацию */
	void SyncReset();
	/* Синхронизировать время */
	void Sync(double PlayRate);
#endif
};

}

#endif
