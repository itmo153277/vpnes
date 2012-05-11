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
#include "manager.h"
#include "bus.h"
#include "ines.h"

namespace vpnes {

/* Static SolderPad */
struct StaticSolderPad {
	ines::SolderPad Mirroring; /* Текущий переключатель */
	uint8 *Screen1; /* Экран 1 */
	uint8 *Screen2; /* Экран 2 */

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		switch (Mirroring) {
			case ines::Horizontal:
				if (Address & 0x0800)
					return Screen2[Address & 0x7ff];
				else
					return Screen1[Address & 0x7ff];
			case ines::Vertical:
				if (Address & 0x0b00)
					return Screen2[Address & 0x7ff];
				else
					return Screen1[Address & 0x7ff];
			case ines::SingleScreen_1:
				return Screen1[Address & 0x07ff];
			case ines::SingleScreen_2:
				return Screen2[Address & 0x07ff];
		}
		return 0x00;
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		switch (Mirroring) {
			case ines::Horizontal:
				if (Address & 0x0800)
					Screen2[Address & 0x7ff] = Src;
				else
					Screen1[Address & 0x7ff] = Src;
				break;
			case ines::Vertical:
				if (Address & 0x0b00)
					Screen2[Address & 0x7ff] = Src;
				else
					Screen1[Address & 0x7ff] = Src;
				break;
			case ines::SingleScreen_1:
				Screen1[Address & 0x07ff] = Src;
				break;
			case ines::SingleScreen_2:
				Screen2[Address & 0x07ff] = Src;
		}
	}
};

namespace ROMID {

typedef MapperGroup<'N', 1>::ID::StaticID PRGID;
typedef MapperGroup<'N', 2>::ID::StaticID CHRID;
typedef MapperGroup<'N', 3>::ID::NoBatteryID CHRRAMID;
typedef MapperGroup<'N', 4>::ID::NoBatteryID RAMID;
typedef MapperGroup<'N', 5>::ID::BatteryID BatteryID;
typedef MapperGroup<'N', 6>::ID::NoBatteryID SolderPadID;

}

/* NROM */
template <class _Bus>
class CNROM: public CDevice {
public:
	typedef StaticSolderPad SolderPad;
protected:
	_Bus *Bus;
	/* PRG */
	uint8 *PRGLow;
	uint8 *PRGHi;
	/* CHR */
	uint8 *CHR;
	/* RAM */
	uint8 *RAM;
	/* ROM Data */
	const ines::NES_ROM_Data *ROM;
public:
	inline explicit CNROM(_Bus *pBus, const ines::NES_ROM_Data *Data) {
		Bus = pBus;
		ROM = Data;
		Bus->GetManager()->template SetPointer<ROMID::PRGID>(\
			ROM->PRG, ROM->Header.PRGSize * sizeof(uint8));
		PRGLow = ROM->PRG;
		PRGHi = ROM->PRG + (ROM->Header.PRGSize - 0x4000);
		if (ROM->CHR != NULL) {
			Bus->GetManager()->template SetPointer<ROMID::CHRID>(\
				ROM->CHR, ROM->Header.CHRSize * sizeof(uint8));
			CHR = ROM->CHR;
		} else
			CHR = (uint8 *) Bus->GetManager()->\
				template GetPointer<ROMID::CHRRAMID>(0x2000 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<ROMID::SolderPadID>(\
			Bus->GetSolderPad(), sizeof(SolderPad));
		Bus->GetSolderPad()->Mirroring = ROM->Header.Mirroring;
		if (ROM->Header.RAMSize == 0)
			RAM = NULL;
		else {
			RAM = new uint8[ROM->Header.RAMSize];
			if (ROM->Trainer != NULL)
				memcpy(RAM + 0x1000, ROM->Trainer, 0x0200 * sizeof(uint8));
			if (ROM->Header.HaveBattery) {
				Bus->GetManager()->template SetPointer<ROMID::BatteryID>(\
					RAM + (ROM->Header.RAMSize - 0x2000), 0x2000 * sizeof(uint8));
				if (ROM->Header.RAMSize > 0x2000)
					Bus->GetManager()->template SetPointer<ROMID::RAMID>(\
						RAM, (ROM->Header.RAMSize - 0x2000) * sizeof(uint8));
			} else
				Bus->GetManager()->template SetPointer<ROMID::RAMID>(\
					RAM, ROM->Header.RAMSize * sizeof(uint8));
		}
	}
	inline ~CNROM() {
		delete [] RAM;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0x00;
		if (Address < 0x8000) { /* SRAM */
			if (RAM != NULL)
				return RAM[Address & 0x1fff];
			else	if ((ROM->Trainer != NULL) && (Address >= 0x7000)
				&& (Address < 0x7200))
				return ROM->Trainer[Address & 0x01ff];
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
		if (ROM->CHR == NULL)
			CHR[Address] = Src;
	}
};

}

#endif
