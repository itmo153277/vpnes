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

#ifndef __MMC5_H_
#define __MMC5_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../types.h"

#include "../ines.h"
#include "../mapper.h"
#include "nrom.h"

namespace vpnes {

namespace MMC5ID {

//typedef MapperGroup<'E'>::Name<1>::ID::NoBatteryID ;

}

/* Реализация маппера 5 */
template <class _Bus>
class CMMC5: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::CHR;
	using CNROM<_Bus>::ROM;
public:
	
private:
	/* Внутренник данные */
	struct SInternalData {
		/* Защита памяти */
		uint8 EnableWrite;
		/* Режим переключения банков PRG */
		enum {
			PRGSwitch_One, /* One 32k bank */
			PRGSwitch_Two, /* Two 16k banks */
			PRGSwitch_Three, /* One 16k bank 0x8000-0xbfff and two 8k banks */
			PRGSwitch_Four /* Four 8k banks */
		} PRGSwitch;
		/* Режим переключения банков CHR */
		enum {
			CHRSwitch_8k,
			CHRSwitch_4k,
			CHRSwitch_2k,
			CHRSwitch_1k
		} CHRSwitch;
	} InternalData;
public:
	inline explicit CMMC5(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
	}
};

}

#endif
