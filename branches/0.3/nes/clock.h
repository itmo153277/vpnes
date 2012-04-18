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

#include <algorithm>
#include "manager.h"
#include "bus.h"

namespace vpnes {

typedef MiscGroup<1>::ID ClocksWaitID;

/* Генератор (NTSC) */
template <class _Bus>
class CClock {
private:
	/* Шина */
	_Bus *Bus;
	/* Осталось прождать тактов */
	int *ClocksWait;
public:
	inline explicit CClock(_Bus *pBus) {
		Bus = pBus;
		ClocksWait = (int *) Bus->GetManager()->template GetPointer<ClocksWaitID>(sizeof(int));
		*ClocksWait = 0;
	}
	inline ~CClock() {}

	/* Обработать кадр */
	inline double ProccessFrame() {
		int ClocksDone = 0;

		do {
			ClocksDone += *ClocksWait;
			*ClocksWait = std::min(Bus->GetCPU()->DoClocks(*ClocksWait),
			                       std::min(Bus->GetAPU()->DoClocks(*ClocksWait),
			                                Bus->GetPPU()->DoClocks(*ClocksWait)));
		} while (!Bus->GetPPU()->IsFrameReady());
		return (176.0 * ClocksDone / 945000.0);
	}
};

struct Clock_rebind {
	template <class _Bus>
	struct rebind {
		typedef CClock<_Bus> rebinded;
	};
};

}

#endif
