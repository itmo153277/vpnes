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

#include <cstring>
#include "device.h"

namespace vpnes {

/* APU */
template <class _Bus>
class CAPU: public CDevice<_Bus> {
	using CDevice<_Bus>::Bus;
private:
	/* Режим */
	enum SeqMode {
		Mode_4step,
		Mode_5step
	};

	/* Состояние */
	struct SState {
		bool DMCInterrupt; /* Флаг прерывания DMC */
		bool FrameInterrupt; /* Флан прерывания счетчика кадров */
		bool NoiseCounter; /* Флаг счетчика для канала шума */
		bool TriangleCounter; /* Флаг счетчика для треугольного канала */
		bool Pulse2Counter; /* Флаг счетчика для квадратного канала 2 */
		bool Pulse1Counter; /* Флаг счетчика для квадратного канала 1 */
		bool DMCRemain; /* Флаг опустошения буфера DMC */
		bool InterruptInhibit; /* Подавление IRQ */
		SeqMode Mode; /* Режим */
		int CounterReset; /* Сброс счетчика */
		inline void Write_4015(uint8 Src) { if (!(Src & 0x10)) DMCRemain = false; NoiseCounter = Src & 0x08;
			TriangleCounter = Src & 0x04; Pulse2Counter = Src & 0x02; Pulse1Counter = Src & 0x01;
			DMCInterrupt = false; }
		inline uint8 Read_4015() { uint8 Res = 0; if (DMCInterrupt) Res |= 0x80; if (FrameInterrupt) Res |= 0x40;
			if (DMCRemain) Res |= 0x10; if (NoiseCounter) Res |= 0x08; if (TriangleCounter) Res |= 0x04;
			if (Pulse2Counter) Res |= 0x02; if (Pulse1Counter) Res |= 0x01; FrameInterrupt = false; return Res; }
		inline void Write_4017(uint8 Src) { Mode = (Src & 0x80) ? Mode_5step : Mode_4step;
			InterruptInhibit = (Src & 0x40); if (InterruptInhibit) FrameInterrupt = false; CounterReset = -1; }
	} State;

	/* Счетчик для игрока 1 */
	uint8 b;
	/* Флаг генерации прерывания */
	bool GenerateIRQ;
	/* Текущий такт */
	int CurClock;
	/* Осталось тактов */
	int ClocksLeft;

public:
	inline explicit CAPU(_Bus *pBus) {
		Bus = pBus;
	}
	inline ~CAPU() {}

	inline void Clock(int DoClocks) {
		bool ResetWrap = ClocksLeft; /* Сдвиг счетчика */

		ClocksLeft += DoClocks;
		while (true) {
			if ((State.CounterReset > 0) && (ClocksLeft > 0)) { /* Сброс счетчика */
				CurClock = 14915;
				if (!ResetWrap)
					ClocksLeft--;
				State.CounterReset = 0;
			}
			if (CurClock >= 14914) {
				if (!State.InterruptInhibit)
					State.FrameInterrupt = true;
			}
			if (CurClock == 14915)
				CurClock = 0;
			if (GenerateIRQ) { /* Вызываем прерывание */
				RaiseIRQ();
			}
			if (ClocksLeft < 2)
				break;
			ClocksLeft -= 2;
			CurClock++;
			if (State.CounterReset < 0)
				State.CounterReset = 1;
		}
	}

	/* Сброс */
	inline void Reset() {
		memset(&State, 0x00, sizeof(SState));
		CurClock = 0;
		ClocksLeft = 0;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		switch (Address) {
			case 0x4015: /* State */
				return State.Read_4015();
			case 0x4016: /* Player 1 Joystick */
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
				break;
		}
		return 0x00;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		switch (Address) {
			case 0x4014: /* OAM DMA */
				static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->GetDMA() = Src;
				break;
			case 0x4015: /* State */
				State.Write_4015(Src);
				break;
			case 0x4016: /* Player 1 Joystick */
				b = 0;
				break;
			case 0x4017: /* Player 2 Joysrick / APU counter mode */
				State.Write_4017(Src);
				break;
		}
	}

	/* Спровацировать IRQ */
	inline void ForceIRQ() { GenerateIRQ = true; }
	/* Выполнить IRQ */
	inline void RaiseIRQ() {
		if (State.FrameInterrupt)
			static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->GetIRQPin() = true;
		GenerateIRQ = false;
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
