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
	/* CHR как RAM */
	bool UseRAM;
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
		else
			pBus->GetMirrorMask() = 0x2bff;
		UseRAM = csize == 0;
		if (UseRAM)
			csize = 1;
		PRG = new uint8[psize * 0x4000];
		CHR = new uint8[csize * 0x2000];
		ROM.seekg(16, std::ios_base::beg);
		ROM.read((char *) PRG, psize * 0x4000);
		if (!UseRAM)
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

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return CHR[Address];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (UseRAM)
			CHR[Address] = Src;
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

/* Реализация маппера 2 */
template <class _Bus>
class CUxROM: public CDevice<_Bus> {
private:
	/* PRG */
	uint8 *PRGLow;
	uint8 *PRGHi;
	/* Banks */
	uint8 *PRGBanks;
	/* CHR */
	uint8 *CHR;
	/* SwitchMask */
	uint8 SwitchMask;
public:
	inline explicit CUxROM(_Bus *pBus, std::istream &ROM) {
		uint8 psize, csize, flags;

		ROM.seekg(4, std::ios_base::beg);
		ROM.read((char *) &psize, sizeof(uint8));
		ROM.read((char *) &csize, sizeof(uint8));
		ROM.read((char *) &flags, sizeof(uint8));
		if (flags & 0x01)
			pBus->GetMirrorMask() = 0x27ff;
		else
			pBus->GetMirrorMask() = 0x2bff;
		PRGBanks = new uint8[psize * 0x4000];
		CHR = new uint8[0x2000];
		ROM.seekg(16, std::ios_base::beg);
		ROM.read((char *) PRGBanks, psize * 0x4000);
		PRGHi = PRGBanks + ((psize - 1) << 14);
		PRGLow = PRGBanks;
		if (psize <= 8)
			SwitchMask = 0x07;
		else
			SwitchMask = 0x0f;
	}
	inline ~CUxROM() {
		delete [] PRGBanks;
		delete [] CHR;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0x00;
		if (Address < 0x8000) /* SRAM */
			return 0x00;
		if (Address < 0xc000)
			return PRGLow[Address & 0x3fff];
		return PRGHi[Address & 0x3fff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address >= 0x8000)
			PRGLow = PRGBanks + ((Src & SwitchMask) << 14);
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return CHR[Address];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		CHR[Address] = Src;
	}
};

/* Махинации с классом */
struct UxROM_rebind {
	template <class _Bus>
	struct rebind {
		typedef CUxROM<_Bus> rebinded;
	};
};

/* Стандартный NES на UxROM */
typedef CNES< CBus<CPU_rebind, APU_rebind, PPU_rebind, UxROM_rebind> > CNES_UxROM;

/* Реализация маппера 7 */
template <class _Bus>
class CAxROM: public CDevice<_Bus> {
	using CDevice<_Bus>::Bus;
private:
	/* PRG */
	uint8 *PRG;
	/* Banks */
	uint8 *PRGBanks;
	/* CHR */
	uint8 *CHR;
	/* SwitchMask */
	uint8 SwitchMask;
public:
	inline explicit CAxROM(_Bus *pBus, std::istream &ROM) {
		uint8 psize;

		Bus = pBus;
		ROM.seekg(4, std::ios_base::beg);
		ROM.read((char *) &psize, sizeof(uint8));
		PRGBanks = new uint8[psize * 0x4000];
		CHR = new uint8[0x2000];
		ROM.seekg(16, std::ios_base::beg);
		ROM.read((char *) PRGBanks, psize * 0x4000);
		PRG = PRGBanks;
		Bus->GetMirrorMask() = 0x23ff;
	}
	inline ~CAxROM() {
		delete [] PRGBanks;
		delete [] CHR;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0x00;
		if (Address < 0x8000) /* SRAM */
			return 0x00;
		return PRG[Address & 0x7fff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address >= 0x8000) {
			PRG = PRGBanks + ((Src & 0x07) << 15);
			if (Src & 0x10)
				Bus->GetPPUPage() = 0x0400;
			else
				Bus->GetPPUPage() = 0x0000;
		}
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return CHR[Address];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		CHR[Address] = Src;
	}
};

/* Махинации с классом */
struct AxROM_rebind {
	template <class _Bus>
	struct rebind {
		typedef CAxROM<_Bus> rebinded;
	};
};

/* Стандартный NES на AxROM */
typedef CNES< CBus<CPU_rebind, APU_rebind, PPU_rebind, AxROM_rebind> > CNES_AxROM;

}

#endif
