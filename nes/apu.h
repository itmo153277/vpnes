/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2013  Ivanov Viktor

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

#ifndef __APU_H_
#define __APU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bus.h"

namespace vpnes {

/* APU */
template <class _Bus, class _Settings>
class CAPU {
public:
	/* Делитель частоты */
	enum { ClockDivider = _Bus::CPUClass::ClockDivider };
private:
	/* Шина */
	_Bus *Bus;
public:
	CAPU(_Bus *pBus) {
		Bus = pBus;
	}
	inline ~CAPU() {}

	/* Заполнить буфер */
	inline void FlushBuffer() {}
	/* Сброс */
	inline void Reset() {}

	/* Выполнить DMA */
	inline int Execute() {
		return _Settings::CPU_Divider;
	}
	/* Прочитать из регистра */
	inline uint8 ReadByte(uint16 Address) {
		return 0x40;
	}
	/* Записать в регистр */
	inline void WriteByte(uint16 Address, uint8 Src) {
	}
};

}

#endif
