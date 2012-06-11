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

#ifndef __TYPES_H_
#define __TYPES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Основные типы данных */
typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short int uint16;
typedef signed short int sint16;
typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned long long uint64;
typedef signed long long sint64;

/* Callback */
typedef int (*VPNES_CALLBACK)(uint32, void *);

/* Callback Events */
#define VPNES_CALLBACK_FRAME       0x00000001
#define VPNES_CALLBACK_CPUHALT     0x00000002
#define VPNES_CALLBACK_PCM         0x00000004
#define VPNES_CALLBACK_INPUT       0x00000008
#define VPNES_CALLBACK_VIDEO       0x00000010

/* Video */

/* Данные видеобуфера */
typedef struct VPNES_VBUF {
	uint32 *Buf;
	uint32 *Pal;
	uint32 RMask;
	uint32 GMask;
	uint32 BMask;
	uint32 AMask;
} VPNES_VBUF;

typedef VPNES_VBUF * VPNES_VIDEO;

/* Input */

#define VPNES_INPUT_UP        0x00000000
#define VPNES_INPUT_DOWN      0x00000001
#define VPNES_INPUT_LEFT      0x00000002
#define VPNES_INPUT_RIGHT     0x00000003
#define VPNES_INPUT_SELECT    0x00000004
#define VPNES_INPUT_START     0x00000005
#define VPNES_INPUT_A         0x00000006
#define VPNES_INPUT_B         0x00000007

/* Буфер для ввода */
typedef int * VPNES_IBUF;

typedef VPNES_IBUF VPNES_INPUT;

/* PCM */

/* dunno lol */

/* CPUHALT */

/* Дампы можно получить из менеджера памяти */
typedef struct VPNES_CPUHALT {
	uint16 PC;
	uint8 A;
	uint8 X;
	uint8 Y;
	uint8 S;
	uint8 State;
} VPNES_CPUHALT;

/* Frame */

typedef double VPNES_FRAME;
/* Вернув отрицательное значение можно выйти из цикла эмуляции */
/* Если необходимо производить манипуляции с объектом, вход из */
/* цикла обязателен */

#ifdef __cplusplus
}
#endif

#endif
