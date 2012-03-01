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

#include <algorithm>
#include "device.h"

namespace vpnes {

/* Стандартное утройство, работающие по тактам */
template <class _Bus>
class CClockedDevice: public CDevice<_Bus> {
protected:
	/* Текущие такты */
	int Clocks;
public:
	inline explicit CClockedDevice():Clocks(0) {}
	inline ~CClockedDevice() {}

	/* Получить такты */
	inline const int &GetClocks() const { return Clocks; }
	/* Выполнить действие */
	inline void Clock(int DoClocks) {}
};

/* Стандартный тактовой генератор */
template <class _Bus>
class CClock {
public:
	typedef class _Bus::CPUClass CPUClass;
	typedef class _Bus::PPUClass PPUClass;

private:
	/* Указатель на шину */
	_Bus *Bus;
	/* Текущие такты */
	int Clocks;
public:
	inline explicit CClock(_Bus *pBus): Bus(pBus), Clocks(0) {}
	inline ~CClock() {}

	/* Выполнить такт */
	inline void Clock() {
		Clocks = std::min(static_cast<CClockedDevice<_Bus> *>(Bus->GetDeviceList()[_Bus::CPU])->GetClocks(),
			static_cast<CClockedDevice<_Bus> *>(Bus->GetDeviceList()[_Bus::PPU])->GetClocks());
		static_cast<CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->Clock(Clocks);
		static_cast<PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->Clock(Clocks);
	}
};

}

#endif
