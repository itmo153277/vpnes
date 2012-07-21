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
					return Screen2[Address & 0x3ff];
				else
					return Screen1[Address & 0x3ff];
			case ines::Vertical:
				if (Address & 0x0400)
					return Screen2[Address & 0x3ff];
				else
					return Screen1[Address & 0x3ff];
			case ines::SingleScreen_1:
				return Screen1[Address & 0x03ff];
			case ines::SingleScreen_2:
				return Screen2[Address & 0x03ff];
		}
		return 0x40;
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		switch (Mirroring) {
			case ines::Horizontal:
				if (Address & 0x0800)
					Screen2[Address & 0x3ff] = Src;
				else
					Screen1[Address & 0x3ff] = Src;
				break;
			case ines::Vertical:
				if (Address & 0x0400)
					Screen2[Address & 0x3ff] = Src;
				else
					Screen1[Address & 0x3ff] = Src;
				break;
			case ines::SingleScreen_1:
				Screen1[Address & 0x03ff] = Src;
				break;
			case ines::SingleScreen_2:
				Screen2[Address & 0x03ff] = Src;
		}
	}
};

namespace NROMID {

typedef MapperGroup<'N'>::Name<1>::ID::StaticID PRGID;
typedef MapperGroup<'N'>::Name<2>::ID::StaticID CHRID;
typedef MapperGroup<'N'>::Name<3>::ID::NoBatteryID CHRRAMID;
typedef MapperGroup<'N'>::Name<4>::ID::NoBatteryID RAMID;
typedef MapperGroup<'N'>::Name<5>::ID::BatteryID BatteryID;

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
		Bus->GetManager()->template SetPointer<NROMID::PRGID>(\
			ROM->PRG, ROM->Header.PRGSize * sizeof(uint8));
		PRGLow = ROM->PRG;
		PRGHi = ROM->PRG + (ROM->Header.PRGSize - 0x4000);
		if (ROM->CHR != NULL) {
			Bus->GetManager()->template SetPointer<NROMID::CHRID>(\
				ROM->CHR, ROM->Header.CHRSize * sizeof(uint8));
			CHR = ROM->CHR;
		} else
			CHR = (uint8 *) Bus->GetManager()->\
				template GetPointer<NROMID::CHRRAMID>(0x2000 * sizeof(uint8));
		Bus->GetSolderPad()->Mirroring = ROM->Header.Mirroring;
		if (ROM->Header.RAMSize == 0)
			RAM = NULL;
		else {
			RAM = new uint8[ROM->Header.RAMSize];
			if (ROM->Trainer != NULL)
				memcpy(RAM + 0x1000, ROM->Trainer, 0x0200 * sizeof(uint8));
			if (ROM->Header.HaveBattery) {
				Bus->GetManager()->template SetPointer<NROMID::BatteryID>(\
					RAM + (ROM->Header.RAMSize - 0x2000), 0x2000 * sizeof(uint8));
				if (ROM->Header.RAMSize > 0x2000)
					Bus->GetManager()->template SetPointer<NROMID::RAMID>(\
						RAM, (ROM->Header.RAMSize - 0x2000) * sizeof(uint8));
			} else
				Bus->GetManager()->template SetPointer<NROMID::RAMID>(\
					RAM, ROM->Header.RAMSize * sizeof(uint8));
		}
	}
	inline ~CNROM() {
		delete [] RAM;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0x40;
		if (Address < 0x8000) { /* SRAM */
			if (RAM != NULL)
				return RAM[Address & 0x1fff];
			else	if ((ROM->Trainer != NULL) && (Address >= 0x7000)
				&& (Address < 0x7200))
				return ROM->Trainer[Address & 0x01ff];
			return 0x40;
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

	/* Обновить указатели */
	inline void UpdateBanks() {
	}
};

namespace MMC1ID {
typedef MapperGroup<'S'>::Name<1>::ID::NoBatteryID InternalDataID;
}

/* Реализация маппера 1 */
template <class _Bus>
class CMMC1: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::ROM;
	using CNROM<_Bus>::CHR;
	using CNROM<_Bus>::PRGLow;
	using CNROM<_Bus>::PRGHi;
	using CNROM<_Bus>::RAM;
private:
	/* CHR */
	uint8 *CHRLow;
	uint8 *CHRHi;
	uint8 *SRAM;
	struct SInternalData {
		/* Модификация MMC1 */
		enum {
			MMC1_Normal,
			MMC1_SNROM,
			MMC1_SOROM,
			MMC1_SUROM,
			MMC1_SXROM
		} Mode;
		/* Банки */
		int CHRLowBank;
		int CHRHiBank;
		int PRGLowBank;
		int PRGHiBank;
		int PRGPage;
		int SRAMPage;
		/* Регистр сдвига */
		uint8 ShiftReg;
		/* Счетчик сдвига */
		int ShiftCounter;
		/* RAM Enabled */
		bool EnableRAM;
		/* RAM Enabled (CHR High Line) */
		bool EnableRAM1;
		/* Ignore Write */
		bool IgnoreWrite;
		/* Режим переключения CHR */
		enum {
			CHRSwitch_4k,
			CHRSwitch_8k
		} CHRSwitch;
		/* Режим переключения PRG */
		enum {
			PRGSwitch_Both,
			PRGSwitch_Low,
			PRGSwitch_Hi
		} PRGSwitch;
		/* Сброс */
		inline void Reset() {
			ShiftReg = 0;
			ShiftCounter = 0;
			PRGSwitch = PRGSwitch_Low;
		}
	} InternalData;
public:
	inline explicit CMMC1(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		Bus->GetManager()->template SetPointer<MMC1ID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
		InternalData.EnableRAM = true;
		InternalData.EnableRAM1 = true;
		InternalData.IgnoreWrite = false;
		InternalData.Reset();
		InternalData.CHRSwitch = SInternalData::CHRSwitch_4k;
		InternalData.CHRLowBank = 0;
		InternalData.CHRHiBank = 1;
		InternalData.PRGLowBank = 0;
		InternalData.PRGHiBank = -1;
		InternalData.PRGPage = 0;
		InternalData.SRAMPage = 0;
		if (ROM->Header.PRGSize > 0x40000) { /* SUROM / SXROM */
			InternalData.PRGHiBank = 0x0f;
			if (ROM->Header.RAMSize > 0x2000) { /* SXROM */
				InternalData.Mode = SInternalData::MMC1_SXROM;
				/* TODO: нужно изменить в менеджере battry-backed память */
			} else
				InternalData.Mode = SInternalData::MMC1_SOROM;
		} else if (ROM->Header.RAMSize > 0x2000) { /* SUROM */
			InternalData.Mode = SInternalData::MMC1_SUROM;
		} else if (ROM->CHR == NULL) { /* SNROM */
			InternalData.Mode = SInternalData::MMC1_SNROM;
		} else
			InternalData.Mode = SInternalData::MMC1_Normal;
		UpdateBanks();
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		InternalData.IgnoreWrite = false;
		if (Address < 0x7000 && (SRAM != NULL))
			return SRAM[Address & 0x0fff];
		else
			return CNROM<_Bus>::ReadAddress(Address);
	}
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000) {
			if (InternalData.EnableRAM && InternalData.EnableRAM1) {
				if (Address < 0x7000 && (SRAM != NULL))
					SRAM[Address & 0x0fff] = Src;
				else
					CNROM<_Bus>::WriteAddress(Address, Src);
			}
		} else {
			if (InternalData.IgnoreWrite) /* Игнорируем близкие запросы */
				return;
			InternalData.IgnoreWrite = true;
			if (Src & 0x80) { /* Сброс */
				InternalData.Reset();
				return;
			}
			InternalData.ShiftReg |= (Src & 0x01) << InternalData.ShiftCounter;
			InternalData.ShiftCounter++;
			if (InternalData.ShiftCounter == 5) {
				InternalData.ShiftCounter = 0;
				if (Address < 0xa000) { /* Control */
					if (InternalData.ShiftReg & 0x10)
						InternalData.CHRSwitch = SInternalData::CHRSwitch_4k;
					else
						InternalData.CHRSwitch = SInternalData::CHRSwitch_8k;
					switch (InternalData.ShiftReg & 0x0c) {
						case 0x00:
						case 0x04:
							InternalData.PRGSwitch = SInternalData::PRGSwitch_Both;
							break;
						case 0x08:
							InternalData.PRGSwitch = SInternalData::PRGSwitch_Hi;
							break;
						case 0x0c:
							InternalData.PRGSwitch = SInternalData::PRGSwitch_Low;
							break;
					}
					switch (InternalData.ShiftReg & 0x03) {
						case 0x00:
							Bus->GetSolderPad()->Mirroring = ines::SingleScreen_1;
							break;
						case 0x01:
							Bus->GetSolderPad()->Mirroring = ines::SingleScreen_2;
							break;
						case 0x02:
							Bus->GetSolderPad()->Mirroring = ines::Vertical;
							break;
						case 0x03:
							Bus->GetSolderPad()->Mirroring = ines::Horizontal;
							break;
					}
				} else {
					if (Address < 0xc000) { /* CHR Bank 1 */
						switch (InternalData.Mode) {
							case SInternalData::MMC1_Normal:
								if (InternalData.CHRSwitch ==
									SInternalData::CHRSwitch_4k) {
									InternalData.CHRLowBank = InternalData.ShiftReg;
								} else {
									InternalData.CHRLowBank =
										InternalData.ShiftReg & 0x1e;
									InternalData.CHRHiBank =
										InternalData.ShiftReg | 0x01;
								}
								break;
							case SInternalData::MMC1_SNROM:
								if (InternalData.CHRSwitch ==
									SInternalData::CHRSwitch_4k)
									InternalData.CHRLowBank =
										InternalData.ShiftReg & 0x01;
								InternalData.EnableRAM1 =
									~InternalData.ShiftReg & 0x10;
								break;
							case SInternalData::MMC1_SOROM:
								if (InternalData.CHRSwitch ==
									SInternalData::CHRSwitch_4k) {
									InternalData.CHRLowBank =
										InternalData.ShiftReg & 0x01;
									InternalData.SRAMPage =
										(InternalData.ShiftReg & 0x10) << 9;
								} else
									InternalData.SRAMPage =
										(InternalData.ShiftReg & 0x08) << 10;
								break;
							case SInternalData::MMC1_SUROM:
								if (InternalData.CHRSwitch ==
									SInternalData::CHRSwitch_4k)
									InternalData.CHRLowBank =
										InternalData.ShiftReg & 0x01;
								InternalData.PRGPage = InternalData.ShiftReg & 0x10;
								break;
							case SInternalData::MMC1_SXROM:
								if (InternalData.CHRSwitch ==
									SInternalData::CHRSwitch_4k)
									InternalData.CHRLowBank =
										InternalData.ShiftReg & 0x01;
								InternalData.SRAMPage =
									(InternalData.ShiftReg & 0x0c) << 11;
								InternalData.PRGPage = InternalData.ShiftReg & 0x10;
								break;
						}
					} else if (Address < 0xe000) { /* CHR Bank 2 */
						if (InternalData.CHRSwitch == SInternalData::CHRSwitch_4k) {
							switch (InternalData.Mode) {
								case SInternalData::MMC1_Normal:
									InternalData.CHRHiBank = InternalData.ShiftReg;
									break;
								case SInternalData::MMC1_SNROM:
									InternalData.CHRHiBank =
										InternalData.ShiftReg & 0x01;
									InternalData.EnableRAM1 =
										~InternalData.ShiftReg & 0x10;
									break;
								case SInternalData::MMC1_SOROM:
									InternalData.CHRLowBank =
										InternalData.ShiftReg & 0x01;
									InternalData.SRAMPage =
										(InternalData.ShiftReg & 0x10) << 9;
									break;
								case SInternalData::MMC1_SUROM:
									InternalData.CHRHiBank =
										InternalData.ShiftReg & 0x01;
									InternalData.PRGPage =
										InternalData.ShiftReg & 0x10;
									break;
								case SInternalData::MMC1_SXROM:
									InternalData.CHRHiBank =
										InternalData.ShiftReg & 0x01;
									InternalData.SRAMPage =
										(InternalData.ShiftReg & 0x0c) << 11;
									InternalData.PRGPage =
										InternalData.ShiftReg & 0x10;
									break;
							}
						}
					} else { /* PRG Bank */
						switch (InternalData.PRGSwitch) {
							case SInternalData::PRGSwitch_Both:
								InternalData.PRGLowBank =
									InternalData.ShiftReg & 0x0e;
								InternalData.PRGHiBank = InternalData.PRGLowBank + 1;
								break;
							case SInternalData::PRGSwitch_Low:
								InternalData.PRGLowBank =
									InternalData.ShiftReg & 0x0f;
								break;
							case SInternalData::PRGSwitch_Hi:
								InternalData.PRGHiBank =
									InternalData.ShiftReg & 0x0f;
								break;
						}
						InternalData.EnableRAM = ~InternalData.ShiftReg & 0x10;
					}
					UpdateBanks();
				}
				InternalData.ShiftReg = 0;
			}
		}
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		if (Address < 0x1000)
			return CHRLow[Address];
		else
			return CHRHi[Address & 0x0fff];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (ROM->CHR == NULL) {
			if (Address < 0x1000)
				CHRLow[Address] = Src;
			else
				CHRHi[Address & 0x0fff] = Src;
		}
	}

	/* Обновить указатели */
	inline void UpdateBanks() {
		CHRLow = CHR + (InternalData.CHRLowBank << 12);
		CHRHi = CHR + (InternalData.CHRHiBank << 12);
		PRGLow = ROM->PRG + ((InternalData.PRGLowBank | InternalData.PRGPage) << 14);
		if (InternalData.PRGHiBank < 0)
			PRGHi = ROM->PRG + (ROM->Header.PRGSize - 0x4000);
		else
			PRGHi = ROM->PRG + ((InternalData.PRGHiBank | InternalData.PRGPage) << 14);
		SRAM = RAM + InternalData.SRAMPage;
	}
};

/* Реализация маппера 2 */
template <class _Bus>
class CUxROM: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::ROM;
	using CNROM<_Bus>::PRGLow;
private:
	/* Смена PRG блоков */
	uint8 SwitchMask;
public:
	inline explicit CUxROM(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		if (ROM->Header.PRGSize < 0x20000)
			SwitchMask = 0x07;
		else
			SwitchMask = 0x0f;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000)
			CNROM<_Bus>::WriteAddress(Address, Src);
		else
			PRGLow = ROM->PRG + ((Src & SwitchMask) << 14);
	}
};

/* Реализация маппера 7 */
template <class _Bus>
class CAxROM: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::ROM;
	using CNROM<_Bus>::PRGLow;
	using CNROM<_Bus>::PRGHi;
public:
	inline explicit CAxROM(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		PRGHi = PRGLow + 0x4000;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000)
			CNROM<_Bus>::WriteAddress(Address, Src);
		else {
			PRGLow = ROM->PRG + ((Src & 0x07) << 15);
			PRGHi = PRGLow + 0x4000;
			Bus->GetPPU()->PreRender();
			if (Src & 0x10)
				Bus->GetSolderPad()->Mirroring = ines::SingleScreen_2;
			else
				Bus->GetSolderPad()->Mirroring = ines::SingleScreen_1;
		}
	}
};

}

#endif
