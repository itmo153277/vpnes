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
#include "device.h"

namespace vpnes {

/* Стандартный тактовой генератор */
template <class _Bus>
class CClock {
private:
	/* Указатель на шину */
	_Bus *Bus;
	/* Callback */
	CallbackFunc CallBack;
	/* Всего тактов */
	int AllClocks;
public:
	inline explicit CClock(_Bus *pBus, CallbackFunc pCallBack): Bus(pBus),
		CallBack(pCallBack), AllClocks(0) { }
	inline ~CClock() {}

	/* Выполнить такт */
	inline int Clock() {
		double Tim;
		int Clocks;

		/* Выполняем команду CPU */
		Clocks = static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->Clock();
		/* APU */
		static_cast<typename _Bus::APUClass *>(Bus->GetDeviceList()[_Bus::APU])->Clock(Clocks);
		/* PPU */
		static_cast<typename _Bus::PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->Clock(Clocks * 3);
		AllClocks += Clocks;
		if (static_cast<typename _Bus::PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->IsFrameReady()) {
			static_cast<typename _Bus::PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->IsFrameReady() = false;
			/* Довыполнить рендеринг */
			static_cast<typename _Bus::PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->Clock(0);
			Tim = 528.0 * AllClocks / 945000.0;
			AllClocks = 0;
			/* Рисуем */
			return CallBack(Tim);
		}
		return 0;
	}
};

}

#endif
