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

#ifndef __MAPPER_H_
#define __MAPPER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <cstring>
#include "ines.h"
#include "device.h"
#include "clock.h"
#include "nes.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"

namespace vpnes {

/* Реализация 0-маппера */
template <class _Bus>
class CNROM: public CDevice<_Bus> {
	using CDevice<_Bus>::Bus;
protected:
	/* PRG */
	uint8 *PRGLow;
	uint8 *PRGHi;
	/* CHR */
	uint8 *CHR;
	/* RAM */
	uint8 *RAM;
	/* ROM Data */
	ines::NES_ROM_Data ROM;
public:
	inline explicit CNROM(_Bus *pBus, ines::NES_ROM_Data *Data) {
		Bus = pBus;
		ROM = *Data;
		PRGLow = ROM.PRG;
		PRGHi = ROM.PRG + (ROM.Header.PRGSize - 0x4000);
		if (ROM.CHR != NULL)
			CHR = ROM.CHR;
		else
			CHR = new uint8[0x2000];
		switch (ROM.Header.Mirroring) {
			case ines::Horizontal:
			case ines::SingleScreen_1:
				Bus->GetMirrorMask() = 0x2bff;
				break;
			case ines::Vertical:
			case ines::SingleScreen_2:
				Bus->GetMirrorMask() = 0x27ff;
				break;
		}
		if (ROM.Header.RAMSize == 0)
			RAM = NULL;
		else {
			RAM = new uint8[ROM.Header.RAMSize];
			if (ROM.Trainer != NULL)
				memcpy(RAM + 0x1000, ROM.Trainer, 0x0200 * sizeof(uint8));
		}
	}
	inline ~CNROM() {
		delete [] ROM.PRG;
		if (ROM.CHR != NULL)
			delete [] ROM.CHR;
		else
			delete [] CHR;
		if (RAM != NULL)
			delete [] RAM;
		delete [] ROM.Trainer;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0x00;
		if (Address < 0x8000) { /* SRAM */
			if (RAM != NULL)
				return RAM[Address & 0x1fff];
			else	if ((ROM.Trainer != NULL) && (Address >= 0x7000)
				&& (Address < 0x7200))
				return ROM.Trainer[Address & 0x01ff];
			return 0x00;
		}
		if (Address < 0xc000)
			return PRGLow[Address & 0x3fff];
		return PRGHi[Address & 0x3fff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (RAM == NULL)
			return;
		if ((Address >= 0x6000) && (Address <= 0x7fff))
			RAM[Address & 0x1fff] = Src;
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return CHR[Address];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (ROM.CHR == NULL)
			CHR[Address] = Src;
	}
};

struct NROM_rebind {
	template <class _Bus>
	struct rebind {
		typedef CNROM<_Bus> rebinded;
	};
};

/* Стандартный NES на NROM */
typedef CNES< CBus<CPU_rebind, APU_rebind, PPU_rebind, NROM_rebind> > CNES_NROM;

/* Реализация маппера 2 */
template <class _Bus>
class CUxROM: public CNROM<_Bus> {
	using CNROM<_Bus>::ROM;
	using CNROM<_Bus>::PRGLow;
private:
	/* Маска для смены PRG банков */
	uint8 SwitchMask;
public:
	inline explicit CUxROM(_Bus *pBus, ines::NES_ROM_Data *Data): CNROM<_Bus>(pBus, Data) {
		if (ROM.Header.PRGSize < 0x20000)
			SwitchMask = 0x07;
		else
			SwitchMask = 0x0f;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000)
			CNROM<_Bus>::WriteAddress(Address, Src);
		else
			PRGLow = ROM.PRG + ((Src & SwitchMask) << 14);
	}
};

struct UxROM_rebind {
	template <class _Bus>
	struct rebind {
		typedef CUxROM<_Bus> rebinded;
	};
};

/* Стандартный NES на UxROM */
typedef CNES< CBus<CPU_rebind, APU_rebind, PPU_rebind, UxROM_rebind> > CNES_UxROM;

}

#endif
