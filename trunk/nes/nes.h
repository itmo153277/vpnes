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

namespace vpnes {

/* Независимый интерфейс */
class CBasicNES {
public:
	inline explicit CBasicNES() {}
	inline virtual ~CBasicNES() {}

	/* Запуск */
	virtual int PowerOn() = 0;
	/* Reset */
	virtual int Reset() = 0;

	/* *NOTE* */
	/* Предполагается, что работа приставки будет осуществляться */
	/* с помощью функции PowerOn(), поскольку виртуальные функции */
	/* не будут интерпретироваться как inline-функции. Для осуществления */
	/* обратной связи вероятно будет введен callback-параметр, на данной */
	/* стадии он не определен. */
};

/* Стандартный NES (NTSC) */
template <class _Bus>
class CNES: public CBasicNES {
public:
	typedef _Bus BusClass;
	typedef typename BusClass::CPUClass CPUClass;
	typedef typename BusClass::APUClass APUClass;
	typedef typename BusClass::PPUClass PPUClass;
	typedef CClock<BusClass> ClockClass;

private:
	/* Шина */
	BusClass Bus;
	/* Тактовый генератор */
	ClockClass Clock;
	/* CPU */
	CPUClass CPU;
	/* APU */
	APUClass APU;
	/* PPU */
	PPUClass PPU;
public:
	inline explicit CNES(): Bus(), Clock(&Bus), CPU(&Bus), APU(), PPU(&Bus) {
		Bus.GetDeviceList()[BusClass::CPU] = &CPU;
		Bus.GetDeviceList()[BusClass::APU] = &APU;
		Bus.GetDeviceList()[BusClass::PPU] = &PPU;
	}
	inline ~CNES() {
		/* ROM добавляется маппером */
		delete Bus.GetDeviceList()[BusClass::ROM];
	}

	/* Запуск */
	inline int PowerOn() {
		for (;;)
			Clock.Clock();
		return 0;
	}
	/* Reset */
	inline int Reset() { return 0; }

	/* Доступ к шине */
	inline BusClass &GetBus() { return Bus; }
};

}

#endif
