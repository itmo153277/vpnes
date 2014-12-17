/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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

#include "video.h"

#include "../types.h"

#ifdef _WIN32
#include "win32-res/win32-res.h"
#endif

#include <cstdio>

namespace vpnes_gui {

/* CVideo */

CVideo::CVideo(CWindow *Window) {
	pWindow = Window;
	InternalSurface = NULL;
	Pal = new Uint32[64 * 8];
	UpdatePalette();
#if defined(VPNES_USE_TTF)
	Font = NULL;
	if (TTF_Init() < 0)
		throw CGenericException("TTF initialization failure");
#ifdef _WIN32
	void *FontData = NULL;
	SDL_RWops *FontRW = NULL;

	/* Используем ресурсы для получения шрифта */
	ResourceInfo = ::FindResource(pWindow->GetInstance(), MAKEINTRESOURCE(IDR_MAINFONT),
		RT_RCDATA);
	ResourceHandle = ::LoadResource(pWindow->GetInstance(), ResourceInfo);
	FontData = ::LockResource(ResourceHandle);
	FontRW = ::SDL_RWFromConstMem(FontData, ::SizeofResource(pWindow->GetInstance(),
		ResourceInfo));
	if (FontRW != NULL)
		Font = ::TTF_OpenFontRW(FontRW, -1, 22);
#else
#ifndef VPNES_TTF_PATH
#define VPNES_TTF_PATH "text.otf"
#endif
	Font = ::TTF_OpenFont(VPNES_TTF_PATH, 22);
#endif
	if (Font == NULL)
		throw CGenericException("Couldn't open a font file");
	TextSurface = NULL;
#endif
}

CVideo::~CVideo() {
#if defined(VPNES_USE_TTF)
	if (Font != NULL)
		::TTF_CloseFont(Font);
#ifdef _WIN32
	UnlockResource(ResourceHandle);
	::FreeResource(ResourceHandle);
#endif
	if (TextSurface != NULL)
		::SDL_FreeSurface(TextSurface);
	::TTF_Quit();
#endif
	if (InternalSurface != NULL)
		::SDL_FreeSurface(InternalSurface);
	delete [] Pal;
}

/* Обновить палитру */
void CVideo::UpdatePalette() {
	const Uint8 NES_Palette[64][3] = {
		{ 88,  88,  88}, {  0,  35, 140}, {  0,  19, 155}, { 45,   5, 133},
		{ 93,   0,  82}, {122,   0,  23}, {122,   8,   0}, { 95,  24,   0},
		{ 53,  42,   0}, {  9,  57,   0}, {  0,  63,   0}, {  0,  60,  34},
		{  0,  50,  93}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
		{161, 161, 161}, {  0,  83, 238}, { 21,  60, 254}, { 96,  40, 228},
		{169,  29, 152}, {212,  30,  65}, {210,  44,   0}, {170,  68,   0},
		{108,  94,   0}, { 45, 115,   0}, {  0, 125,   6}, {  0, 120,  82},
		{  0, 105, 169}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
		{255, 255, 255}, { 31, 165, 254}, { 94, 137, 254}, {181, 114, 254},
		{254, 101, 246}, {254, 103, 144}, {254, 119,  60}, {254, 147,   8},
		{196, 178,   0}, {121, 202,  16}, { 58, 213,  74}, { 17, 209, 164},
		{  6, 191, 254}, { 66,  66,  66}, {  0,   0,   0}, {  0,   0,   0},
		{255, 255, 255}, {160, 217, 254}, {189, 204, 254}, {225, 194, 254},
		{254, 188, 251}, {254, 189, 208}, {254, 197, 169}, {254, 209, 142},
		{233, 222, 134}, {199, 233, 146}, {168, 238, 176}, {149, 236, 217},
		{145, 228, 254}, {172, 172, 172}, {  0,   0,   0}, {  0,   0,   0}
	};
	int i, j;

#define C_R(comp) ((comp) & 1)
#define C_G(comp) (((comp) & 2) >> 1)
#define C_B(comp) (((comp) & 4) >> 2)
	for (i = 0; i < 64; i++)
		for (j = 0; j < 8; j++)
			Pal[i + 64 * j] = ::SDL_MapRGB(pWindow->GetSurface()->format,
				(int) (NES_Palette[i][0] * (6 - C_G(j) - C_B(j)) / 6.0),
				(int) (NES_Palette[i][1] * (6 - C_R(j) - C_B(j)) / 6.0),
				(int) (NES_Palette[i][2] * (6 - C_R(j) - C_G(j)) / 6.0));
#undef C_R
#undef C_G
#undef C_B
}

/* Обновить поверхность */
SDL_Surface *CVideo::UpdateSurface() {
	SDL_Surface *NewSurface, *Screen;

	Screen = pWindow->GetSurface();
	NewSurface = ::SDL_CreateRGBSurface(SDL_SWSURFACE, _Width, _Height, 32,
		Screen->format->Rmask, Screen->format->Gmask, Screen->format->Bmask,
		Screen->format->Amask);
	return NewSurface;
}

/* Обновить размер */
void CVideo::UpdateSizes(int Width, int Height) {
#if defined(VPNES_USE_TTF)
	if (TextSurface != NULL) {
		::SDL_FreeSurface(TextSurface);
		TextSurface = NULL;
	}
#if defined(VPNES_DISABLE_SYNC)
	TextTimer = ::SDL_GetTicks();
#endif
#endif
	if (InternalSurface != NULL)
		::SDL_FreeSurface(InternalSurface);
	_Width = Width;
	_Height = Height;
	InternalSurface = UpdateSurface();
	Buf = reinterpret_cast<uint32 *>(InternalSurface->pixels);
}

/* Обновить кадр */
bool CVideo::UpdateFrame(double FrameTime) {
	SDL_Surface *Screen, *NewSurface;
	CWindow::WindowState RetState;

#if !defined(VPNES_DISABLE_SYNC)
	LastFrameTime = FrameTime;
#endif
	do {
		for (;;) {
			RetState = pWindow->ProcessMessages();
			if (RetState != CWindow::wsUpdateBuffer)
				break;
			/* Обновляем параметры экрана, если это необходимо */
			UpdatePalette();
			NewSurface = ::SDL_ConvertSurface(InternalSurface, pWindow->GetSurface()->format,
				SDL_SWSURFACE);
			::SDL_FreeSurface(InternalSurface);
			InternalSurface = NewSurface;
			Buf = reinterpret_cast<uint32 *>(InternalSurface->pixels);
		}
		Screen = pWindow->GetSurface();
#if defined(VPNES_USE_TTF)
#if defined(VPNES_DISABLE_SYNC)
		if ((TextSurface != NULL) && ((::SDL_GetTicks() - TextTimer) >= 4000)) {
			::SDL_FreeSurface(TextSurface);
			TextSurface = NULL;
		}
#endif
		if (pWindow->GetUpdateTextFlag()) {
			SDL_Surface *TempSurface;
			SDL_Rect TextInRect = {7, 7};
			SDL_Rect BorderRect = {1, 1};
			const SDL_Color TextColour = {224, 224, 224};
			const SDL_Color BorderColour = {56, 56, 56};
			const SDL_Color BGColour = {40, 40, 40};

			if (TextSurface != NULL)
				::SDL_FreeSurface(TextSurface);
			TempSurface = ::TTF_RenderUNICODE_Shaded(Font,
				reinterpret_cast<const Uint16 *>(pWindow->GetText()), TextColour, BGColour);
			TextSurface = ::SDL_CreateRGBSurface(SDL_HWSURFACE,
				TempSurface->w + TextInRect.x * 2,
				TempSurface->h + TextInRect.y * 2,
				Screen->format->BitsPerPixel, Screen->format->Rmask,
				Screen->format->Gmask, Screen->format->Bmask, Screen->format->Amask);
			::SDL_FillRect(TextSurface, NULL, ::SDL_MapRGB(TextSurface->format,
				BorderColour.r, BorderColour.g, BorderColour.b));
			BorderRect.w = TextSurface->w - BorderRect.x * 2;
			BorderRect.h = TextSurface->h - BorderRect.y * 2;
			::SDL_FillRect(TextSurface, &BorderRect, ::SDL_MapRGB(TextSurface->format,
				BGColour.r, BGColour.g, BGColour.b));
			::SDL_BlitSurface(TempSurface, NULL, TextSurface, &TextInRect);
			::SDL_FreeSurface(TempSurface);
			pWindow->GetUpdateTextFlag() = false;
#if defined(VPNES_DISABLE_SYNC)
			TextTimer = ::SDL_GetTicks();
#endif
		}
#endif
#if !defined(VPNES_DISABLE_SYNC) && !defined(VPNES_DISABLE_FSKIP)
		if (!SkipFrame) {
#endif
		/* Обновляем экран */
		::SDL_SoftStretch(InternalSurface, NULL, Screen, NULL);
#if defined(VPNES_USE_TTF)
		if (TextSurface != NULL) {
			SDL_Rect TextRect = {10, 10};

			::SDL_BlitSurface(TextSurface, NULL, Screen, &TextRect);
		}
#endif
		::SDL_Flip(Screen);
#if !defined(VPNES_DISABLE_SYNC) && !defined(VPNES_DISABLE_FSKIP)
		}
#endif
	} while (RetState == CWindow::wsPause);
	return pWindow->GetWindowState() == CWindow::wsNormal;
}

#if !defined(VPNES_DISABLE_SYNC)

/* Остановить синхронизацию */
void CVideo::SyncStop() {
	if (PausedSync)
		return;
	StopTime = ::SDL_GetTicks();
	PausedSync = true;
}

/* Продолжить синхронизацию */
Uint32 CVideo::SyncResume() {
	Uint32 TimeDiff;

	if (!PausedSync)
		return 0;
	SyncReset();
	TimeDiff = FrameStart - StopTime;
#if defined(VPNES_USE_TTF)
	TextTimer += TimeDiff;
#endif
	return TimeDiff;
}

/* Сбросить синхронизацию */
void CVideo::SyncReset() {
	FrameStart = ::SDL_GetTicks();
	Delta = 0.0;
	FrameTimeCheck = FrameStart;
	Jitter = 0;
#if !defined(VPNES_DISABLE_FSKIP)
	SkipFrame = false;
#endif
	PausedSync = false;
}

/* Синхронизировать время */
void CVideo::Sync(double PlayRate) {
	Sint32 FrameTime;
	Sint32 DelayTime;

	/* Синхронизация */
	Delta += LastFrameTime / PlayRate;
	FrameTime = (Sint32) Delta;
	DelayTime = FrameTime - (::SDL_GetTicks() - FrameStart);
	Delta -= FrameTime;
#if !defined(VPNES_DISABLE_FSKIP)
	if (!SkipFrame) {
#endif
	DelayTime -= Jitter / 2;
	if (DelayTime > 0)
		::SDL_Delay(DelayTime);
#if !defined(VPNES_DISABLE_FSKIP)
	}
#endif
	FrameStart = ::SDL_GetTicks();
	Jitter += FrameStart - FrameTimeCheck - FrameTime;
#if !defined(VPNES_DISABLE_FSKIP)
	if (SkipFrame) {
		SkippedTime += FrameStart - FrameTimeCheck;
		if (SkippedTime > 50) {
			Jitter = 0;
			/* log */
		}
		FramesSkipped++;
		if (Jitter < FrameTime) {
			SkipFrame = false;
			/* log */
		}
	} else if (Jitter > (FrameTime * 2)) {
		SkipFrame = true;
		SkippedTime = 0;
		FramesSkipped = 0;
		/* log */
	}
#else
	if (Jitter > 50)
		Jitter = 0; /* Сброс */
#endif
	FrameTimeCheck = FrameStart;
#if defined(VPNES_USE_TTF)
	if ((TextSurface != NULL) && ((FrameStart - TextTimer) >= 4000)) {
		::SDL_FreeSurface(TextSurface);
		TextSurface = NULL;
	}
	if (pWindow->GetUpdateTextFlag()) {
		TextTimer = FrameStart;
	}
#endif
}

#endif

}
