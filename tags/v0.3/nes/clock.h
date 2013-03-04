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

#ifndef __CLOCK_H_
#define __CLOCK_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "manager.h"
#include "bus.h"

namespace vpnes {

/* Генератор */
template <class _Bus, class _Oscillator>
class CStdClock {
private:
	/* Шина */
	_Bus *Bus;
	/* Текущий такт */
	int PreCycles;
public:
	inline explicit CStdClock(_Bus *pBus) {
		Bus = pBus;
	}
	inline ~CStdClock() {}

	/* Обработать кадр */
	inline double ProccessFrame() {

		do {
			PreCycles = 0;
			Bus->GetCPU()->Execute();
			PreCycles = Bus->GetCPU()->GetCycles();
			Bus->GetAPU()->Clock(PreCycles);
			Bus->GetPPU()->Clock(PreCycles * _Bus::CPUClass::ClockDivider);
		} while (!Bus->GetPPU()->IsFrameReady());
		return Bus->GetPPU()->GetFrameCycles() * GetFix();
	}

	/* Точный сдвиг */
	inline void Clock(int Cycles) {
		PreCycles += Cycles;
	}
	/* Пауза */
	inline void Pause(int Cycle) {
		PreCycles = Cycle;
	}

	/* Получить точный сдвиг */
	inline int GetPreCycles() const { return PreCycles *
		_Bus::CPUClass::ClockDivider; }
	inline int GetPreCPUCycles() const { return PreCycles; }
	/* Получить точный коэф */
	static inline const double GetFix() { return _Oscillator::GetFreq(); }
};

}

#endif
