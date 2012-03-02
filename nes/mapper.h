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

#include <istream>
#include "device.h"
#include "clock.h"
#include "nes.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"

/* Открыть картридж */
vpnes::CBasicNES *OpenROM(std::istream &ROM, vpnes::clock::CallbackFunc CallBack);

namespace vpnes {

/* Реализация 0-маппера */
template <class _Bus>
class CNROM: public CDevice<_Bus> {
private:
	/* Преобразование PRG */
	bool PRGMirror;
	/* FamilyBasic */
	bool FamilyBasic;
	/* PRG */
	uint8 *PRG;
	/* CHR */
	uint8 *CHR;
	/* SRAM */
	uint8 *SRAM;
public:
	inline explicit CNROM(_Bus *pBus, std::istream &ROM) {
		uint8 psize, csize, flags;

		ROM.seekg(4, std::ios_base::beg);
		ROM.read((char *) &psize, sizeof(uint8));
		ROM.read((char *) &csize, sizeof(uint8));
		ROM.read((char *) &flags, sizeof(uint8));
		PRGMirror = psize < 2;
		FamilyBasic = flags & 0x02;
		if (flags & 0x01)
			pBus->GetMirrorMask() = 0x27ff;
		PRG = new uint8[psize * 0x4000];
		CHR = new uint8[csize * 0x2000];
		ROM.seekg(16, std::ios_base::beg);
		ROM.read((char *) PRG, psize * 0x4000);
		ROM.read((char *) CHR, csize * 0x2000);
		if (FamilyBasic)
			SRAM = new uint8[0x2000];
	}
	inline ~CNROM() {
		delete [] PRG;
		delete [] CHR;
		if (FamilyBasic)
			delete [] SRAM;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0;
		if (Address < 0x8000) { /* SRAM */
			if (FamilyBasic)
				return SRAM[Address & 0x5fff];
			else
				return 0x00;
		}
		if (!PRGMirror)
			return PRG[Address & 0x7fff];
		return PRG[Address & 0x3fff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (!FamilyBasic)
			return;
		if ((Address >= 0x6000) && (Address <= 0x7fff))
			SRAM[Address & 0x5fff] = Src;
	}
};

/* Махинации с классом */
struct NROM_rebind {
	template <class _Bus>
	struct rebind {
		typedef CNROM<_Bus> rebinded;
	};
};

/* Стандартный NES на NROM */
typedef CNES< CBus<CPU_rebind, APU_rebind, PPU_rebind, NROM_rebind> > CNES_NROM;

}

#endif
