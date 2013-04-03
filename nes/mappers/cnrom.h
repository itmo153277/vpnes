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

#ifndef __CNROM_H_
#define __CNROM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../types.h"

#include "../ines.h"
#include "../mapper.h"
#include "nrom.h"

namespace vpnes {

namespace CNROMID {

typedef MapperGroup<'C'>::Name<1>::ID::NoBatteryID CHRBankID;

}

/* Реализация маппера 3 */
template <class _Bus>
class CCNROM: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::CHR;
	using CNROM<_Bus>::ROM;
private:
	/* Смена CHR блоков */
	uint8 SwitchMask;
	/* Текущий банк */
	int CHRBank;
public:
	inline explicit CCNROM(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		Bus->GetManager()->template SetPointer<ManagerID<CNROMID::CHRBankID> >(&CHRBank,
			sizeof(int));
		SwitchMask = mapper::GetMask(ROM->Header.CHRSize) >> 13;
		CHRBank = SwitchMask;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000)
			CNROM<_Bus>::WriteAddress(Address, Src);
		else {
			Bus->GetPPU()->PreRenderBeforeCERise();
			CHRBank = ((Src & SwitchMask) << 13);
		}
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return CHR[Address | CHRBank];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (ROM->CHR == NULL)
			CHR[Address | CHRBank] = Src;
	}
};

}

#endif
