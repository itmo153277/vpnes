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

#ifndef __APU_H_
#define __APU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <SDL.h>

#include "device.h"

namespace vpnes {

/* APU */
template <class _Bus>
class CAPU: public CDevice<_Bus> {
	using CDevice<_Bus>::Bus;
private:
	uint8 b;
public:
	inline explicit CAPU(_Bus *pBus) { Bus = pBus; b = 0; }
	inline ~CAPU() {}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		switch (Address) {
			case 0x4016:
				b++;
				switch (b) {
				case 1:
					return SDL_GetKeyState(NULL)[SDLK_c];
				case 2:
					return SDL_GetKeyState(NULL)[SDLK_x];
				case 3:
					return SDL_GetKeyState(NULL)[SDLK_a];
				case 4:
					return SDL_GetKeyState(NULL)[SDLK_s];
				case 5:
					return SDL_GetKeyState(NULL)[SDLK_UP];
				case 6:
					return SDL_GetKeyState(NULL)[SDLK_DOWN];
				case 7:
					return SDL_GetKeyState(NULL)[SDLK_LEFT];
				case 8:
					return SDL_GetKeyState(NULL)[SDLK_RIGHT];
				}
		}
		return 0x00;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		switch (Address) {
			case 0x4014: /* OAM DMA */
				static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->GetDMA() = Src;
				static_cast<typename _Bus::PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->SetDMA(Src);
				break;
			case 0x4016:
				b = 0;
				break;
		}
	}
};

/* Махинации с классом */
struct APU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CAPU<_Bus> rebinded;
	};
};

}

#endif
