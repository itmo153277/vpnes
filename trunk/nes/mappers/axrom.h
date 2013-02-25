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

#ifndef __AXROM_H_
#define __AXROM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../types.h"

#include "../ines.h"
#include "nrom.h"

namespace vpnes {

/* Реализация маппера 7 */
template <class _Bus>
class CAxROM: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::ROM;
	using CNROM<_Bus>::PRGData;
public:
	inline explicit CAxROM(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		PRGData.PRGHi = PRGData.PRGLow + 0x4000;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address < 0x8000)
			CNROM<_Bus>::WriteAddress(Address, Src);
		else {
			PRGData.PRGLow = ((Src & 0x07) << 15);
			PRGData.PRGHi = PRGData.PRGLow + 0x4000;
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
