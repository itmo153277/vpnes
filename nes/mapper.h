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

#include "ines.h"

namespace vpnes {

namespace mapper {

/* Получить маску */
inline int GetMask(int Size) {
	int i, j;
	bool less = false;

	i = Size << 1;
	for (;;) {
		j = i - 1;
		i &= j;
		if (i) {
			less = true;
		} else
			break;
	}
	if (!less)
		j >>= 1;
	return j;
}

}

/* Static SolderPad */
struct StaticSolderPad {
	ines::SolderPad Mirroring; /* Текущий переключатель */
	uint8 *Screen1; /* Экран 1 */
	uint8 *Screen2; /* Экран 2 */
	uint8 *RAM; /* Дополнительная память */

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
			case ines::FourScreen:
				switch (Address & 0x0c00) {
					case 0x0000:
						return Screen1[Address & 0x03ff];
					case 0x0400:
						return Screen2[Address & 0x03ff];
					case 0x0800:
					case 0x0c00:
						return RAM[Address & 0x07ff];
				}
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
				break;
			case ines::FourScreen:
				switch (Address & 0x0c00) {
					case 0x0000:
						Screen1[Address & 0x03ff] = Src;
						break;
					case 0x0400:
						Screen2[Address & 0x03ff] = Src;
						break;
					case 0x0800:
					case 0x0c00:
						RAM[Address & 0x07ff] = Src;
						break;
				}
		}
	}
};

/* SolderPad через чип (управляется маппером) */
template <class _Bus>
struct DynamicSolderPad: public StaticSolderPad {
	/* Указатель на шину */
	_Bus* Bus;

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return Bus->GetROM()->NTRead(Address);
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		Bus->GetROM()->NTWrite(Address, Src);
	}
};

}

#endif
