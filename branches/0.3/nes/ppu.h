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

#ifndef __PPU_H_
#define __PPU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "manager.h"
#include "bus.h"

namespace vpnes {

/* PPU */
template <class _Bus>
class CPPU: public CDevice {
private:
	bool FrameReady;
	int clocks;
public:
	inline explicit CPPU(_Bus *pBus, uint32 *Buf, const uint32 *Pal) {}
	inline ~CPPU() {}

	/* Обработать такты */
	inline int DoClocks(int Clocks) {
		if (FrameReady)
			FrameReady = false;
		clocks += Clocks;
		if (clocks >= 341 * 262) {
			clocks -= 341 * 262;
			FrameReady = true;
		}
		return 1;
	}

	/* Сброс */
	inline void Reset() {
		FrameReady = false;
		clocks = 0;
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return 0x00;
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
	}

	/* Флаг окончания рендеринга фрейма */
	inline const bool &IsFrameReady() const { return FrameReady; }
};

struct PPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CPPU<_Bus> rebinded;
	};
};

}

#endif
