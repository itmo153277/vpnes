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

#ifndef __MMC1_H_
#define __MMC1_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../types.h"

#include "../manager.h"
#include "../ines.h"
#include "../mapper.h"
#include "nrom.h"

namespace vpnes {

namespace MMC1ID {

typedef MapperGroup<'S'>::Name<1>::ID::NoBatteryID InternalDataID;

}

/* Реализация маппера 1 */
template <class _Bus>
class CMMC1: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::ROM;
	using CNROM<_Bus>::CHR;
	using CNROM<_Bus>::RAM;
private:
	struct SInternalData: public ManagerID<MMC1ID::InternalDataID> {
		/* PRW R/~W pin */
		bool PRGRW;
		/* Модификация MMC1 */
		enum {
			MMC1_Normal,
			MMC1_SNROM,
			MMC1_SOROM,
			MMC1_SUROM,
			MMC1_SXROM
		} Mode;
		/* Банки */
		int CHRBanks[2];
		int PRGBank;
		int PRGPage;
		int SRAMPage;
		/* Регистр сдвига */
		uint8 ShiftReg;
		/* Счетчик сдвига */
		int ShiftCounter;
		/* RAM Enabled */
		int EnableRAM;
		/* RAM Write */
		int EnableWrite;
		/* Режим переключения CHR */
		enum {
			CHRSwitch_4k = 0,
			CHRSwitch_8k = 1
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

	/* Ограничитель PRG */
	int PRGMask;
	/* Ограничитель CHR */
	int CHRMask;
public:
	inline explicit CMMC1(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		memset(&InternalData, 0, sizeof(InternalData));
		InternalData.EnableRAM = true;
		InternalData.EnableWrite = true;
		InternalData.CHRSwitch = SInternalData::CHRSwitch_4k;
		InternalData.PRGSwitch = SInternalData::PRGSwitch_Low;
		InternalData.CHRBanks[1] = 0x1000;
		if (ROM->Header.PRGSize > 0x40000) { /* SUROM / SXROM */
			if (ROM->Header.RAMSize > 0x2000) { /* SXROM */
				InternalData.Mode = SInternalData::MMC1_SXROM;
				Bus->GetManager()->template FreeMemory<NROMID::RAMID>();
				Bus->GetManager()->template FreeMemory<NROMID::BatteryID>();
				Bus->GetManager()->template SetPointer<ManagerID<NROMID::BatteryID> >(\
					RAM, ROM->Header.RAMSize * sizeof(uint8));
			} else
				InternalData.Mode = SInternalData::MMC1_SUROM;
		} else if (ROM->Header.RAMSize > 0x2000) { /* SOROM */
			InternalData.Mode = SInternalData::MMC1_SOROM;
		} else if (ROM->Header.CHRSize == 0x2000) { /* SNROM */
			InternalData.Mode = SInternalData::MMC1_SNROM; /* Tricky */
		} else
			InternalData.Mode = SInternalData::MMC1_Normal;
		PRGMask = mapper::GetMask(ROM->Header.PRGSize);
		CHRMask = mapper::GetMask(ROM->Header.CHRSize);
	}

	/* Обновление адреса на шине CPU */
	inline void UpdateCPUBus(uint16 Address) {
		InternalData.PRGRW = true; /* Детектим чтение */
	}
	inline void UpdateCPUBus(uint16 Address, uint8 Src) {
	}
	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		int PRGAddr;

		if (Address < 0x8000) {
			if (RAM != NULL) {
				if (!InternalData.EnableRAM)
					return 0x40;
				return RAM[InternalData.SRAMPage | (Address & 0x1fff)];
			} else if ((ROM->Trainer != NULL) && (Address >= 0x7000)
				&& (Address < 0x7200))
				return ROM->Trainer[Address & 0x01ff];
			return 0x40;
		}
		PRGAddr = InternalData.PRGPage;
		switch (InternalData.PRGSwitch) {
			case SInternalData::PRGSwitch_Both:
				PRGAddr |= (InternalData.PRGBank & 0x38000) | (Address & 0x7fff);
				break;
			case SInternalData::PRGSwitch_Low:
				if (Address < 0xc000)
					PRGAddr |= InternalData.PRGBank;
				else
					PRGAddr |= 0x3c000;
				PRGAddr |= (Address & 0x3fff);
				break;
			case SInternalData::PRGSwitch_Hi:
				if (Address >= 0xc000)
					PRGAddr |= InternalData.PRGBank;
				PRGAddr |= (Address & 0x3fff);
				break;
		}
		return ROM->PRG[PRGAddr & PRGMask];
	}
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000) {
			if (InternalData.EnableRAM && InternalData.EnableWrite && (RAM != NULL))
				RAM[InternalData.SRAMPage | (Address & 0x1fff)] = Src;
		} else {
			/* Игнорируем запись, если не было изменения PRG R/$W */
			if (!InternalData.PRGRW)
				return;
			InternalData.PRGRW = false;
			if (Src & 0x80) { /* Сброс */
				InternalData.Reset();
				return;
			}
			InternalData.ShiftReg |= (Src & 0x01) << InternalData.ShiftCounter;
			InternalData.ShiftCounter++;
			if (InternalData.ShiftCounter == 5) {
				Bus->GetPPU()->PreRenderBeforeCERise();
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
				} else if (Address < 0xc000) { /* CHR Bank 1 */
					switch (InternalData.Mode) {
						case SInternalData::MMC1_Normal:
							InternalData.CHRBanks[0] = InternalData.ShiftReg << 12;
							break;
						case SInternalData::MMC1_SNROM:
							if (InternalData.CHRSwitch ==
								SInternalData::CHRSwitch_4k)
								InternalData.CHRBanks[0] =
									(InternalData.ShiftReg & 0x01) << 12;
							InternalData.EnableWrite =
								~InternalData.ShiftReg & 0x10;
							break;
						case SInternalData::MMC1_SOROM:
							if (InternalData.CHRSwitch ==
								SInternalData::CHRSwitch_4k) {
								InternalData.CHRBanks[0] =
									(InternalData.ShiftReg & 0x01) << 12;
								InternalData.SRAMPage =
									(InternalData.ShiftReg & 0x10) << 9;
							} else
								InternalData.SRAMPage =
									(InternalData.ShiftReg & 0x08) << 10;
							break;
						case SInternalData::MMC1_SUROM:
							if (InternalData.CHRSwitch ==
								SInternalData::CHRSwitch_4k)
								InternalData.CHRBanks[0] =
									(InternalData.ShiftReg & 0x01) << 12;
							InternalData.PRGPage =
								(InternalData.ShiftReg & 0x10) << 14;
							break;
						case SInternalData::MMC1_SXROM:
							if (InternalData.CHRSwitch ==
								SInternalData::CHRSwitch_4k)
								InternalData.CHRBanks[0] =
									(InternalData.ShiftReg & 0x01) << 12;
							InternalData.SRAMPage =
								(InternalData.ShiftReg & 0x0c) << 11;
							InternalData.PRGPage =
								(InternalData.ShiftReg & 0x10) << 14;
							break;
					}
				} else if (Address < 0xe000) { /* CHR Bank 2 */
					if (InternalData.CHRSwitch == SInternalData::CHRSwitch_4k) {
						switch (InternalData.Mode) {
							case SInternalData::MMC1_Normal:
								InternalData.CHRBanks[1] =
									InternalData.ShiftReg << 12;
								break;
							case SInternalData::MMC1_SNROM:
								InternalData.CHRBanks[1] =
									(InternalData.ShiftReg & 0x01) << 12;
								InternalData.EnableWrite =
									~InternalData.ShiftReg & 0x10;
								break;
							case SInternalData::MMC1_SOROM:
								InternalData.CHRBanks[1] =
									(InternalData.ShiftReg & 0x01) << 12;
								InternalData.SRAMPage =
									(InternalData.ShiftReg & 0x10) << 9;
								break;
							case SInternalData::MMC1_SUROM:
								InternalData.CHRBanks[1] =
									(InternalData.ShiftReg & 0x01) << 12;
								InternalData.PRGPage =
									(InternalData.ShiftReg & 0x10) << 14;
								break;
							case SInternalData::MMC1_SXROM:
								InternalData.CHRBanks[1] =
									(InternalData.ShiftReg & 0x01) << 12;
								InternalData.SRAMPage =
									(InternalData.ShiftReg & 0x0c) << 11;
								InternalData.PRGPage =
									(InternalData.ShiftReg & 0x10) << 14;
								break;
						}
					}
				} else { /* PRG Bank */
					InternalData.PRGBank = (InternalData.ShiftReg & 0x0f) << 14;
					InternalData.EnableRAM = ~InternalData.ShiftReg & 0x10;
				}
				InternalData.ShiftReg = 0;
			}
		}
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		if (Address < 0x1000)
			return CHR[(InternalData.CHRBanks[0] | Address) &
				~(InternalData.CHRSwitch << 12) & CHRMask];
		else
			return CHR[(InternalData.CHRBanks[InternalData.CHRSwitch ^ 1] |
				(InternalData.CHRSwitch << 12) | (Address & 0x0fff)) & CHRMask];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (ROM->CHR == NULL) {
			if (Address < 0x1000)
				CHR[(InternalData.CHRBanks[0] | Address) &
					~(InternalData.CHRSwitch << 12) & CHRMask] = Src;
			else
				CHR[(InternalData.CHRBanks[InternalData.CHRSwitch ^ 1] |
					(InternalData.CHRSwitch << 12) | (Address & 0x0fff)) &
					CHRMask] = Src;
		}
	}
};

}

#endif
