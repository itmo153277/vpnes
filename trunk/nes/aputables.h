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

#ifndef __APUTABLES_H_
#define __APUTABLES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace vpnes {

namespace apu {

/* Таблица выхода прямоугольных каналов */
extern const double SquareTable[31];
/* Таблица выхода остальных каналов */
extern const double TNDTable[203];

/* Таблицы для NTSC */
struct NTSC_Tables {
	/* Число тактов на каждом шаге */
	static const int StepCycles[6];
	/* Перекрываемые кнопки контроллера */
	static const int ButtonsRemap[4];
	/* Таблица счетчика */
	static const int LengthCounterTable[32];
	/* Таблица формы */
	static const bool DutyTable[32];
	/* Таблица пилообразной формы */
	static const int SeqTable[32];
	/* Таблица длин для канала шума */
	static const int NoiseTable[16];
	/* Таблица длин для ДМ-канала */
	static const int DMTable[16];
};

/* Таблицы для PAL */
struct PAL_Tables: public NTSC_Tables {
	/* Число тактов на каждом шаге */
	static const int StepCycles[6];
	/* Таблица длин для канала шума */
	static const int NoiseTable[16];
	/* Таблица длин для ДМ-канала */
	static const int DMTable[16];
};

}

}

#endif
