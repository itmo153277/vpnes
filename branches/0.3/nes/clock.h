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
			Bus->GetAPU()->Clock(Bus->GetCPU()->GetCycles());
			Bus->GetPPU()->Clock(Bus->GetCPU()->GetCycles() *
				_Bus::CPUClass::ClockDivider);
		} while (!Bus->GetPPU()->IsFrameReady());
		return Bus->GetPPU()->GetFrameCycles() * GetFix();
	}

	/* Точный сдвиг */
	inline void Clock(int Cycles) {
		PreCycles += Cycles;
	}

	/* Получить точный сдвиг */
	inline int GetPreCycles() const { return PreCycles *
		_Bus::CPUClass::ClockDivider; }
	inline int GetPreCPUCycles() const { return PreCycles; }
	/* Получить точный коэф */
	inline double GetFix() const { return _Oscillator::GetFreq(); }
};

/* Стандартный генератор */
template <class _Oscillator>
struct StdClock {
	template <class _Bus>
	class CClock: public CStdClock<_Bus, _Oscillator> {
	public:
		inline explicit CClock(_Bus *pBus): CStdClock<_Bus, _Oscillator>(pBus) {}
		inline ~CClock() {}
	};
};

}

#endif
