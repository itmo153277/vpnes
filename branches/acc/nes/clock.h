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

namespace clock {
	typedef int (*CallbackFunc)(double);
}

/* Стандартный тактовой генератор */
template <class _Bus>
class CClock {
public:
	typedef typename _Bus::CPUClass CPUClass;
	typedef typename _Bus::PPUClass PPUClass;

private:
	/* Указатель на шину */
	_Bus *Bus;
	/* Текущие такты */
	int Clocks;
	/* Callback */
	clock::CallbackFunc CallBack;
	/* Всего тактов */
	int AllClocks;
public:
	inline explicit CClock(_Bus *pBus, clock::CallbackFunc pCallBack): Bus(pBus), Clocks(0),
		CallBack(pCallBack), AllClocks(0) { }
	inline ~CClock() {}

	/* Выполнить такт */
	inline int Clock() {
		double Tim;

		Clocks = std::min(static_cast<CClockedDevice<_Bus> *>(Bus->GetDeviceList()[_Bus::CPU])->GetClocks(),
			static_cast<CClockedDevice<_Bus> *>(Bus->GetDeviceList()[_Bus::PPU])->GetClocks());
		static_cast<PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->Clock(Clocks);
		static_cast<CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->Clock(Clocks);
		AllClocks += Clocks;
		if (static_cast<PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->IsFrameReady()) {
			static_cast<PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->IsFrameReady() = false;
			Tim = 176.0 * AllClocks / 945000.0;
			AllClocks = 0;
			return CallBack(Tim);
		}
		return 0;
	}
};

}

#endif
