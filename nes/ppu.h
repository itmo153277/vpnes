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

#ifndef __PPU_H_
#define __PPU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "manager.h"
#include "frontend.h"
#include "clock.h"
#include "bus.h"
#include "cpu.h"

namespace vpnes {

namespace PPUID {

}

/* PPU */
template <class _Bus, class _Settings>
class CPPU {
public:
	enum { ClockDivider = _Settings::PPU_Divider };
private:
	/* Шина */
	_Bus *Bus;

	/* Флаг готовности кадра */
	bool FrameReady;
	/* Время отрисовки кадра */
	int FrameTime;
public:
	CPPU(_Bus *pBus) {
		Bus = pBus;
	}
	inline ~CPPU() {}

	/* Сброс */
	inline void Reset() {}

	/* Сброс часов шины */
	inline void ResetInternalClock(int Time) {
	}
	/* Чтение памяти */
	inline uint8 ReadByte(uint16 Address) {
		return 0x40;
	}
	/* Запись памяти */
	inline void WriteByte(uint16 Address, uint8 Src) {
	}

	/* Флаг готовности кадра */
	inline bool IsFrameReady() { return FrameReady; }
	/* Время отрисовки кадра */
	inline int GetFrameTime() {
		FrameReady = false;
		return FrameTime;
	}
	/* Частота PPU */
	static inline const double GetFreq() { return _Settings::GetFreq(); }
};

}

#endif
