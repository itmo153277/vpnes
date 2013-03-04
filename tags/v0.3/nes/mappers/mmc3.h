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

#ifndef __MMC3_H_
#define __MMC3_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../types.h"

#include "../manager.h"
#include "../ines.h"
#include "../mapper.h"
#include "nrom.h"

namespace vpnes {

namespace MMC3ID {

typedef MapperGroup<'T'>::Name<1>::ID::NoBatteryID InternalDataID;
typedef MapperGroup<'T'>::Name<2>::ID::NoBatteryID IRQCircuitID;

}

/* Реализация маппера 4 */
template <class _Bus>
class CMMC3: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::ROM;
	using CNROM<_Bus>::CHR;
public:
	typedef FourScreenSolderPad SolderPad;
private:
	struct SInternalData: public ManagerID<MMC3ID::InternalDataID> {
		int PRGBanks[2]; /* PRG */
		int CHRBanks[8]; /* CHR */
		/* Переключение PRG */
		enum {
			PRGSwitch_Low = 0,
			PRGSwitch_Middle = 2
		} PRGSwitch;
		/* Переключение CHR */
		enum {
			CHRSwitch_24 = 0,
			CHRSwitch_42 = 4
		} CHRSwitch;
		/* Выбор банка */
		uint8 MuxAddr;
		/* RAM Enabled */
		int EnableRAM;
		/* RAM Write */
		int EnableWrite;
	} InternalData;

	struct SIRQCircuit: public ManagerID<MMC3ID::IRQCircuitID> {
		/* Счетчик сканлайнов */
		uint8 IRQCounter;
		/* Обновление счетчика */
		uint8 IRQLatch;
		/* Генерировать IRQ */
		bool IRQEnable;
		/* Игнорировать следующее обращение */
		bool IgnoreAccess;
		inline void Process(uint16 Address, _Bus *pBus) {
			if (Address & 0x1000) {
				if (IgnoreAccess)
					return;
				IgnoreAccess = true;
				if (IRQCounter == 0)
					IRQCounter = IRQLatch;
				else
					IRQCounter--;
				if ((IRQCounter == 0) && IRQEnable) {
					pBus->GetCPU()->GenerateIRQ(pBus->GetPPU()->GetCycles() *
						_Bus::PPUClass::ClockDivider);
				}
			} else
				IgnoreAccess = false;
		}
	} IRQCircuit;

	/* Ограничитель PRG */
	int PRGMask;
	/* Ограничитель CHR */
	uint8 CHRMask;
public:
	inline explicit CMMC3(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		int i;

		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		memset(&InternalData, 0, sizeof(InternalData));
		InternalData.PRGBanks[1] = 0x2000;
		for (i = 0; i < 8; i++)
			InternalData.CHRBanks[i] = i << 10;
		InternalData.EnableRAM = false;
		InternalData.EnableWrite = true;
		if (ROM->Header.Mirroring & 0x08) { /* 4-screen */
			Bus->GetManager()->template FreeMemory<NROMID::RAMID>();
			Bus->GetManager()->template FreeMemory<NROMID::BatteryID>();
			Bus->GetSolderPad()->RAM = (uint8 *) Bus->GetManager()->\
				template GetPointer<ManagerID<NROMID::RAMID> >(0x1000 * sizeof(uint8));
			memset(Bus->GetSolderPad()->RAM, 0x00, 0x1000 * sizeof(uint8));
		}
		Bus->GetManager()->template SetPointer<SIRQCircuit>(&IRQCircuit);
		memset(&IRQCircuit, 0, sizeof(IRQCircuit));
		PRGMask = mapper::GetMask(ROM->Header.PRGSize);
		CHRMask = mapper::GetMask(ROM->Header.CHRSize) >> 10;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		int PRGAddr;

		if (Address < 0x8000) {
			if (InternalData.EnableRAM)
				return CNROM<_Bus>::ReadAddress(Address);
			else
				return 0x40;
		}
		switch (Address & 0x6000) {
			case 0x2000:
				PRGAddr = InternalData.PRGBanks[1];
				break;
			case 0x0000:
			case 0x4000:
				if (((Address & 0x6000) >> 13) == InternalData.PRGSwitch)
					PRGAddr = InternalData.PRGBanks[0];
				else
					PRGAddr = ROM->Header.PRGSize - 0x4000;
				break;
			case 0x6000:
				PRGAddr = ROM->Header.PRGSize - 0x2000;
				break;
		}
		PRGAddr |= Address & 0x1fff;
		return ROM->PRG[PRGAddr & PRGMask];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000) {
			if (InternalData.EnableRAM && InternalData.EnableWrite)
				CNROM<_Bus>::WriteAddress(Address, Src);
		} else if (Address < 0xa000) {
			if (Address & 0x0001) { /* Bank Data */
				if (InternalData.MuxAddr > 5) /* PRG select */
					InternalData.PRGBanks[InternalData.MuxAddr & 0x01] = Src << 13;
				else {
					Bus->GetPPU()->PreRender();
					Src &= CHRMask;
					if (InternalData.MuxAddr < 2) {
						InternalData.CHRBanks[InternalData.MuxAddr << 1] =
							(Src & 0xfe) << 10;
						InternalData.CHRBanks[\
							(InternalData.MuxAddr << 1) | 0x01] =
							(Src | 0x01) << 10;
					} else
						InternalData.CHRBanks[InternalData.MuxAddr + 2] =
							Src << 10;
				}
			} else { /* Bank Select */
				Bus->GetPPU()->PreRender();
				if (Src & 0x80)
					InternalData.CHRSwitch = SInternalData::CHRSwitch_42;
				else
					InternalData.CHRSwitch = SInternalData::CHRSwitch_24;
				if (Src & 0x40)
					InternalData.PRGSwitch = SInternalData::PRGSwitch_Middle;
				else
					InternalData.PRGSwitch = SInternalData::PRGSwitch_Low;
				InternalData.MuxAddr = Src & 0x07;
			}
		} else if (Address < 0xc000) {
			if (~ROM->Header.Mirroring & 0x08) {
				if (Address & 0x0001) { /* PRG RAM chip */
					InternalData.EnableRAM = Src & 0x80;
					InternalData.EnableWrite = ~Src & 0x40;
				} else { /* Mirroring */
					Bus->GetPPU()->PreRender();
					if (Src & 0x01)
						Bus->GetSolderPad()->Mirroring = ines::Horizontal;
					else
						Bus->GetSolderPad()->Mirroring = ines::Vertical;
				}
			}
		} else {
			Bus->GetPPU()->PreRender();
			if (Address < 0xe000) {
				if (Address & 0x0001) { /* IRQ Clear */
					IRQCircuit.IRQCounter = 0;
				} else { /* IRQ Latch */
					IRQCircuit.IRQLatch = Src;
				}
			} else if (Address & 0x0001) { /* IRQ Enable */
				IRQCircuit.IRQEnable = true;
			} else { /* IRQ Disable */
				IRQCircuit.IRQEnable = false;
				Bus->GetCPU()->ClearIRQ();
			}
		}
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return CHR[InternalData.CHRBanks[((Address & 0x1c00) >> 10) ^
			InternalData.CHRSwitch] | (Address & 0x03ff)];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (ROM->CHR == NULL)
			CHR[InternalData.CHRBanks[((Address & 0x1c00) >> 10) ^
				 InternalData.CHRSwitch] | (Address & 0x03ff)] = Src;
	}

	/* Обновление адреса PPU */
	inline void UpdatePPUAddress(uint16 Address) {
		if (Address < 0x2000)
			IRQCircuit.Process(Address, Bus);
	}
};

}

#endif
