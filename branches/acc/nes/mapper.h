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
				return SRAM[Address & 0x1fff];
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

/* Реализация маппера 1 */
template <class _Bus>
class CMMC1: public CDevice<_Bus> {
	using CDevice<_Bus>::Bus;
private:
	/* Battery */
	bool HaveBattery;
	/* RAM Enabled */
	bool RAMEnabled;
	/* CHR как RAM */
	bool UseRAM;
	/* PRG */
	uint8 *PRGLow;
	uint8 *PRGHi;
	/* PRG Banks */
	uint8 *PRGBanks;
	/* CHR */
	uint8 *CHRLow;
	uint8 *CHRHi;
	/* CHR Banks */
	uint8 *CHRBanks;
	/* RAM */
	uint8 *RAM;
	uint8 *SRAMLow;
	uint8 *SRAMHi;
	/* Mode */
	enum SxROM_Mode {
		ModeNormal,
		Mode_512,
		Mode_256_RAM
	} Mode;
	/* PRG bank mode */
	enum MMC1_PRGBank_Mode {
		NoFixed,
		FixedFirst,
		FixedSecond
	} PRGBank_Mode;
	/* CHR bank mode */
	enum MMC1_CHRBank_Mode {
		SwitchBoth,
		SwitchSeparate
	} CHRBank_Mode;
	/* Shift register */
	uint8 ShiftReg;
	/* Shift iteration */
	int ShiftIter;
public:
	inline explicit CMMC1(_Bus *pBus, std::istream &ROM) {
		uint8 psize, csize, rsize, flags;

		Bus = pBus;
		ROM.seekg(4, std::ios_base::beg);
		ROM.read((char *) &psize, sizeof(uint8));
		ROM.read((char *) &csize, sizeof(uint8));
		ROM.read((char *) &flags, sizeof(uint8));
		ROM.seekg(8, std::ios_base::beg);
		ROM.read((char *) &rsize, sizeof(uint8));
		HaveBattery = flags & 0x02;
		RAMEnabled = true;
		if (rsize == 0)
			rsize = 1;
		if (flags & 0x08) {
			Bus->GetMirrorMask() = 0x23ff;
			Bus->GetPPUPage() = (flags & 0x01) << 10;
		} else if (flags & 0x01)
			pBus->GetMirrorMask() = 0x27ff;
		else
			pBus->GetMirrorMask() = 0x2bff;
		UseRAM = csize == 0;
		if (UseRAM)
			csize = rsize;
		PRGBanks = new uint8[psize * 0x4000];
		CHRBanks = new uint8[csize * 0x2000];
		ROM.seekg(16, std::ios_base::beg);
		ROM.read((char *) PRGBanks, psize * 0x4000);
		if (!UseRAM)
			ROM.read((char *) CHRBanks, csize * 0x2000);
		if (HaveBattery)
			RAM = new uint8[rsize * 0x2000];
		if (psize == 32)
			Mode = Mode_512;
		else {
			if (psize < 32 && UseRAM)
				Mode = Mode_256_RAM;
			else
				Mode = ModeNormal;
		}
		CHRLow = CHRBanks + (csize - 1) * 0x2000;
		CHRHi = CHRBanks + (csize - 1) * 0x2000 + 0x1000;
		PRGLow = PRGBanks + (psize - 2) * 0x4000;
		PRGHi = PRGBanks + (psize - 1) * 0x4000;
		if (HaveBattery) {
			SRAMLow = RAM + (rsize - 1) * 0x2000;
			SRAMHi = RAM + (rsize - 1) * 0x2000 + 0x1000;
		}
		ShiftReg = 0;
		ShiftIter = 0;
		PRGBank_Mode = FixedSecond;
		CHRBank_Mode = SwitchBoth;
	}
	inline ~CMMC1() {
		delete [] PRGBanks;
		delete [] CHRBanks;
		if (HaveBattery)
			delete [] RAM;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0x00;
		if (Address < 0x8000) { /* RAM */
			if (HaveBattery && RAMEnabled) {
				if (Address < 0x7000)
					return SRAMLow[Address & 0x0fff];
				else
					return SRAMHi[Address & 0x0fff];
			} else
				return 0x00;
		}
		if (Address < 0xc000)
			return PRGLow[Address & 0x3fff];
		return PRGHi[Address & 0x3fff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x6000) /* Registers */
			return;
		if (Address < 0x8000) { /* PRG RAM */
			if (HaveBattery && RAMEnabled) {
				if (Address < 0x7000)
					SRAMLow[Address & 0x0fff] = Src;
				else
					SRAMHi[Address & 0x0fff] = Src;
			}
			return;
		}
		if (Src & 0x80) { /* Reset bit */
			ShiftReg = 0;
			ShiftIter = 0;
			PRGBank_Mode = FixedSecond;
			return;
		}
		ShiftReg |= (Src & 0x01) << ShiftIter;
		if (++ShiftIter != 5)
			return;
		if (Address < 0xa000) { /* Control */
			switch (ShiftReg & 0x03) { /* Mirroring */
				case 0x00:
				case 0x01:
					/* Single screen */
					Bus->GetMirrorMask() = 0x23ff;
					Bus->GetPPUPage() = (ShiftReg & 0x01) << 10;
					break;
				case 0x02:
					/* Vertical */
					Bus->GetMirrorMask() = 0x27ff;
					Bus->GetPPUPage() = 0;
					break;
				case 0x03:
					/* Horizontal */
					Bus->GetMirrorMask() = 0x2bff;
					Bus->GetPPUPage() = 0;
					break;
			}
			switch (ShiftReg & 0x0c) { /* PRG Bank Mode */
				case 0x00:
				case 0x04:
					PRGBank_Mode = NoFixed;
					break;
				case 0x08:
					PRGBank_Mode = FixedFirst;
					break;
				case 0x0c:
					PRGBank_Mode = FixedSecond;
			}
			if (ShiftReg & 0x10)
				CHRBank_Mode = SwitchSeparate;
			else
				CHRBank_Mode = SwitchBoth;
		} else if (Address < 0xc000) { /* CHR 0 */
			switch (Mode) {
				case ModeNormal:
					if (UseRAM && HaveBattery) {
						if (CHRBank_Mode == SwitchBoth) {
							SRAMLow = RAM + ((ShiftReg & 0x1e) << 12);
							SRAMHi = RAM + ((ShiftReg | 0x01) << 12);
						} else
							SRAMLow = RAM + (ShiftReg << 12);
					} else {
						if (CHRBank_Mode == SwitchBoth) {
							CHRLow = CHRBanks + ((ShiftReg & 0x1e) << 12);
							CHRHi = CHRBanks + ((ShiftReg | 0x01) << 12);
						} else
							CHRLow = CHRBanks + (ShiftReg << 12);
					}
					break;
				case Mode_512:
					break;
				case Mode_256_RAM:
					break;
			}
		} else if (Address < 0xe000) { /* CHR 1 */
			switch (Mode) {
				case ModeNormal:
					if (CHRBank_Mode == SwitchSeparate) {
						if (UseRAM && HaveBattery)
							SRAMHi = RAM + (ShiftReg << 12);
						else
							CHRHi = CHRBanks + (ShiftReg << 12);
					}
					break;
				case Mode_512:
					break;
				case Mode_256_RAM:
					break;
			}
		} else { /* PRG */
			RAMEnabled = ~ShiftReg & 0x10;
			switch (PRGBank_Mode) {
				case NoFixed:
					PRGLow = PRGBanks + ((ShiftReg & 0x0e) << 14);
					PRGHi = PRGBanks + ((ShiftReg | 0x01) << 14);
					break;
				case FixedFirst:
					PRGHi = PRGBanks + (ShiftReg << 14);
					break;
				case FixedSecond:
					PRGLow = PRGBanks + (ShiftReg << 14);
			}
		}
		ShiftIter = 0;
		ShiftReg = 0;
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		if (Address < 0x1000)
			return CHRLow[Address];
		return CHRHi[Address & 0x0fff];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (!UseRAM)
			return;
		if (Address < 0x1000)
			CHRLow[Address] = Src;
		CHRHi[Address & 0x0fff] = Src;
	}
};

/* Махинации с классом */
struct MMC1_rebind {
	template <class _Bus>
	struct rebind {
		typedef CMMC1<_Bus> rebinded;
	};
};

/* Стандартный NES на MMC1 */
typedef CNES< CBus<CPU_rebind, APU_rebind, PPU_rebind, MMC1_rebind> > CNES_MMC1;

}

#endif
