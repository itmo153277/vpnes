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

#ifndef __CLOCK_H_
#define __CLOCK_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace MiscID {


}

/* Генератор (NTSC) */
template <class _Bus>
class CClock {
private:
	/* Шина */
	_Bus *Bus;
	/* Текущий такт */
	int PreCycles;
public:
	inline explicit CClock(_Bus *pBus) {
		Bus = pBus;
		PreCycles = 0;
	}
	inline ~CClock() {}

	/* Обработать кадр */
	inline double ProccessFrame() {
		int CyclesDone;

		do {
			CyclesDone = Bus->GetCPU()->Execute();
			PreCycles = 0;
			Bus->GetAPU()->Clock(CyclesDone);
			Bus->GetPPU()->Clock(CyclesDone);
		} while (!Bus->GetPPU()->IsFrameReady());
		return Bus->GetPPU()->GetFrameCycles() * GetFix();
	}

	/* Точный сдвиг */
	inline void Clock(int Cycles) {
		PreCycles += Cycles;
	}

	/* Получить точный сдвиг */
	inline const int &GetPreCycles() const { return PreCycles; }
	/* Получить точный коэф */
	inline double GetFix() const { return 176.0 / 945000.0; }
};

}

#endif
