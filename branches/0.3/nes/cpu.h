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

#ifndef __CPU_H_
#define __CPU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace CPUID {

typedef CPUGroup<1>::ID::NoBatteryID ClocksLeftID;

}

/* CPU */
template <class _Bus>
class CCPU: public CDevice {
private:
	/* Шина */
	_Bus *Bus;
	/* Осталось до завершения выполнения команды */
	int *ClocksLeft;
public:
	inline explicit CCPU(_Bus *pBus) {
		Bus = pBus;
		ClocksLeft = (int *) Bus->GetManager()->\
			template GetPointer<CPUID::ClocksLeftID>(sizeof(int));
	}
	inline ~CCPU() {}

	/* Обработать такты */
	inline int DoClocks(int Clocks) {
		if ((*ClocksLeft -= Clocks) == 0)
			*ClocksLeft = 3;
		return *ClocksLeft;
	}

	/* Сброс */
	inline void Reset() {
		*ClocksLeft = 0;
	}
};

struct CPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CCPU<_Bus> rebinded;
	};
};

}

#endif
