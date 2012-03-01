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

#include <istream>
#include "device.h"
#include "nes.h"

/* Открыть картридж */
vpnes::CBasicNES *OpenROM(std::istream &);

namespace vpnes {

/* Реализация 0-маппера */
template <class _Bus>
class CNROM: public CDevice<_Bus>, public CPPUDevice {
public:
	inline explicit CNROM() {}
	inline ~CNROM() {}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) { return 0x00; }
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) { return 0x00; }
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {}
};

/* Махинации с классом */
struct NROM_rebind {
	template <class _Bus>
	struct rebind {
		typedef CNROM<_Bus> rebinded;
	};
};

/* Стандартный NES на NROM */
typedef CStdNES<NROM_rebind> CStdNES_NROM;

}

#endif
