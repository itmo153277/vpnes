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

#ifndef __INES_H_
#define __INES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <istream>

namespace vpnes {

namespace ines {

/* iNES Header */
#pragma pack(push, 1)
typedef struct iNES_Header {
	char Signature[4]; /* NES */
	uint8 PRGSize;     /* Размер PRG ROM в 16 KB */
	uint8 CHRSize;     /* Размер CHR ROM в 8 KB */
	uint8 Flags;       /* Флаги */
	uint8 Flags_ex;    /* Флаги */
	uint8 RAMSize;     /* Размер PRG RAM в 8 KB */
	uint8 TV_system;   /* Система */
	uint8 Flags_unofficial; /* Недокументировано */
	uint8 Reserved;
	uint32 BadROM;     /* Должно быть 0 */
} iNES_Header;
#pragma pack(pop)

/* SolderPad */
enum SolderPad {
	Horizontal = 0x00,
	Vertical = 0x01,
	SingleScreen_1 = 0x08,
	SingleScreen_2 = 0x09
};

/* NES_Type */
enum NES_Type {
	NES_Auto = 0x00,
	NES_NTSC = 0x01,
	NES_PAL = 0x02,
	NES_Dendy = 0x03
};

/* Header Data */
struct NES_Header_Data {
	int PRGSize; /* Размер PRG ROM */
	int CHRSize; /* Размер CHR ROM */
	int RAMSize; /* Размер PRG RAM */
	int Mapper;  /* Номер маппера */
	SolderPad Mirroring; /* Отображение */
	bool HaveBattery; /* Сохранять память */
	int TVSystem; /* Система */
};

/* NES ROM Data */
struct NES_ROM_Data {
	NES_Header_Data Header; /* Заголовок */
	uint8 *PRG;     /* PRG ROM */
	uint8 *CHR;     /* CHR ROM */
	uint8 *Trainer; /* Trainer */
};

}

/* Прочитать образ */
int ReadROM(std::istream &ROM, ines::NES_ROM_Data *Data);
/* Освободить память */
inline void FreeROMData(ines::NES_ROM_Data *Data) {
	delete [] Data->PRG;
	delete [] Data->CHR;
	delete [] Data->Trainer;
}

}

#endif
