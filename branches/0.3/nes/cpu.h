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

}

/* CPU */
template <class _Bus>
class CCPU: public CDevice {
private:
	/* Шина */
	_Bus *Bus;

	/* Обращения к памяти */
	inline uint8 ReadMemory(uint16 Address) {
		uint8 Res;

		Res = Bus->ReadCPUMemory(Address);
		Bus->GetClock()->Clock(3);
		return Res;
	}
	inline void WriteMemory(uint16 Address, uint8 Src) {
		Bus->WriteCPUMemory(Address, Src);
		Bus->GetClock()->Clock(3);
	}

public:
	inline explicit CCPU(_Bus *pBus) {
		Bus = pBus;
	}
	inline ~CCPU() {}

	/* Обработать такты */
	inline int Execute() {
		return 3;
	}

	/* Сброс */
	inline void Reset() {
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
