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

#include "../types.h"

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
	/* Установить указатель на буфер */
	virtual void SetBuf(uint32 *Buf) = 0;
	/* Установить указатель на палитру */
	virtual void SetPal(uint32 *Pal) = 0;
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
	inline explicit CNES(clock::CallbackFunc CallBack): Bus(),
		Clock(&Bus, CallBack), CPU(&Bus), APU(&Bus), PPU(&Bus) {
		Bus.GetDeviceList()[BusClass::CPU] = &CPU;
		Bus.GetDeviceList()[BusClass::APU] = &APU;
		Bus.GetDeviceList()[BusClass::PPU] = &PPU;
	}
	inline ~CNES() {
		/* ROM добавляется маппером */
		delete Bus.GetDeviceList()[BusClass::ROM];
	}

	/* Установить указатель на буфер */
	inline void SetBuf(uint32 *Buf) {
		PPU.GetBuf() = Buf;
	}
	/* Установить указатель на палитру */
	inline void SetPal(uint32 *Pal) {
		PPU.GetPalette() = Pal;
	}
	/* Запуск */
	inline int PowerOn() {
		Reset();
		for (;;) {
			if (Clock.Clock() < 0)
				break;
		}
		return 0;
	}
	/* Reset */
	inline int Reset() {
		CPU.Reset();
		APU.Reset();
		PPU.Reset();
		return 0;
	}

	/* Доступ к шине */
	inline BusClass &GetBus() { return Bus; }
};

}

#endif
