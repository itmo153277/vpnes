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

#ifndef __NES_H_
#define __NES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "device.h"
#include "clock.h"
#include "cpu.h"
#include "ppu.h"

namespace vpnes {

/* Стандартный NES (NTSC) */
template <class _Bus>
class CNES {
public:
	typedef _Bus BusClass;
	typedef CCPU<BusClass> CPUClass;
	typedef CPPU<BusClass> PPUClass;
	typedef CClock<BusClass, CPUClass, PPUClass> ClockClass;
private:
	/* Шина */
	BusClass Bus;
	/* Тактовый генератор */
	ClockClass Clock;
	/* CPU */
	CPUClass CPU;
	/* PPU */
	PPUClass PPU;
public:
	inline explicit CNES(): Bus(), Clock(&Bus), CPU(&Bus), PPU(&Bus) {
		Bus.GetDeviceList()[BusClass::CPU] = &CPU;
		Bus.GetDeviceList()[BusClass::PPU] = &PPU;
	}
	inline ~CNES() {}

	/* Запуск */
	inline int PowerUp() { return 0; }
	/* Reset */
	inline int Reset() { return 0; }

	/* Доступ к тактовому генератору */
	inline ClockClass &GetClock() { return Clock; }
};

}

#endif
