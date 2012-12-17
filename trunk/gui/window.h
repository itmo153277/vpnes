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

#ifndef __WINDOW_H_
#define __WINDOW_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
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

#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VERSION
#define VERSION "0.3"
#endif

/* Window States */
#define VPNES_QUIT       0x00000000
#define VPNES_UPDATEBUF  0x00000001
#define VPNES_SAVESTATE  0x00000002
#define VPNES_LOADSTATE  0x00000004
#define VPNES_RESET      0x00000008
#define VPNES_PAUSE      0x00000010
#define VPNES_RESUME     0x00000020
#define VPNES_STEP       0x00000040
#define VPNES_RATE       0x00000080

/* Обратная связь с главным циклом */
extern int WindowState;
/* Номер слота сохранения */
extern int SaveState;
#ifdef _WIN32
/* Дескриптор окна */
extern HWND WindowHandle;
/* Программа */
extern HINSTANCE Instance;
#endif

/* Инициализация лога */
int InitLog(void);
/* Инициализация SDL */
int InitMainWindow(int Width, int Height);
/* Очистить окно */
void ClearWindow(void);
/* Установить режим */
int SetMode(int Width, int Height, double FrameLength);
/* Выход */
void AppClose(void);
void AppQuit(void);
/* Управление эмуляцией */
void Pause(void);
void StopRender(void);
void ResumeRender(void);
/* Callback-функция */
int WindowCallback(uint32, void *);

#ifdef __cplusplus
}
#endif

#endif
