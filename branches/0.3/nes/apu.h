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

#include <cstring>
#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace APUID {

typedef APUGroup<1>::ID::NoBatteryID InternalDataID;
typedef APUGroup<2>::ID::NoBatteryID CycleDataID;
typedef APUGroup<3>::ID::NoBatteryID StateID;

}

/* APU */
template <class _Bus>
class CAPU: public CDevice {
private:
	/* Шина */
	_Bus *Bus;

	/* Внутренние данные */
	struct SInternalData {
		int StrobeCounter_A;
		int StrobeCounter_B;
		int WasteCycles;
	} InternalData;

	/* Данные о тактах */
	struct SCycleData {
		int CyclesLeft;
		int Step;
	} CycleData;

	/* Состояние */
	struct SState {
		bool DMCInterrupt; /* Флаг прерывания DMC */
		bool FrameInterrupt; /* Флаг прерывания счетчика кадров */
		bool NoiseCounter; /* Флаг счетчика для канала шума */
		bool TriangleCounter; /* Флаг счетчика для треугольного канала */
		bool Pulse2Counter; /* Флаг счетчика для квадратного канала 2 */
		bool Pulse1Counter; /* Флаг счетчика для квадратного канала 1 */
		bool DMCRemain; /* Флаг опустошения буфера DMC */
		bool InterruptInhibit; /* Подавление IRQ */
		enum SeqMode {
			Mode_4step,
			Mode_5step
		} Mode; /* Режим */
		inline void Write_4015(uint8 Src) { if (!(Src & 0x10)) DMCRemain = false;
			NoiseCounter = Src & 0x08; TriangleCounter = Src & 0x04; Pulse2Counter = Src &
			0x02; Pulse1Counter = Src & 0x01; DMCInterrupt = false;
		}
		inline uint8 Read_4015() { uint8 Res = 0; if (DMCInterrupt) Res |= 0x80;
			if (FrameInterrupt) Res |= 0x40; if (DMCRemain) Res |= 0x10; if (NoiseCounter)
			Res |= 0x08; if (TriangleCounter) Res |= 0x04; if (Pulse2Counter) Res |= 0x02;
			if (Pulse1Counter) Res |= 0x01; FrameInterrupt = false; return Res;
		}
		inline void Write_4017(uint8 Src) { Mode = (Src & 0x80) ? Mode_5step : Mode_4step;
			InterruptInhibit = (Src & 0x40); if (InterruptInhibit) FrameInterrupt = false;
		}
	} State;
	
	/* Буфер */
	VPNES_IBUF ibuf;
public:
	inline explicit CAPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<APUID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
		Bus->GetManager()->template SetPointer<APUID::CycleDataID>(\
			&CycleData, sizeof(CycleData));
		Bus->GetManager()->template SetPointer<APUID::StateID>(\
			&State, sizeof(State));
	}
	inline ~CAPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		CycleData.CyclesLeft += Cycles;
		while (CycleData.CyclesLeft > 6 * 3728) {
			CycleData.CyclesLeft -= 6 * 3728;
			switch (State.Mode) {
				case SState::Mode_4step:
					switch (CycleData.Step) {
						case 0:
							//Envelope
							//Linear counter
						case 1:
							//Envelope
							//Linear counter
							//Length counter
							//Sweep
						case 2:
							//Envelope
							//Linear counter
							break;
						case 3:
							//Envelope
							//Linear counter
							//Length counter
							//Sweep
							if (!State.InterruptInhibit) {
								State.FrameInterrupt = true;
								Bus->GetCPU()->GetIRQPin() = true;
							}
							break;
					}
					CycleData.Step++;
					if (CycleData.Step == 4)
						CycleData.Step = 0;
					break;
				case SState::Mode_5step:
					switch (CycleData.Step) {
						case 0:
							//Envelope
							//Linear counter
						case 1:
							//Envelope
							//Linear counter
							//Length counter
							//Sweep
						case 2:
							//Envelope
							//Linear counter
						case 4:
							//Envelope
							//Linear counter
							//Length counter
							//Sweep
							break;
					}
					CycleData.Step++;
					if (CycleData.Step == 5)
						CycleData.Step = 0;
					break;
			}
		}
	}

	/* Сброс */
	inline void Reset() {
		memset(&InternalData, 0, sizeof(InternalData));
		memset(&CycleData, 0, sizeof(CycleData));
		memset(&State, 0, sizeof(State));
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;

		switch (Address) {
			case 0x4015: /* Состояние APU */
				Res = State.Read_4015();
				Bus->GetCPU()->GetIRQPin() = State.FrameInterrupt;
				return Res;
			case 0x4016: /* Данные контроллера 1 */
				if (InternalData.StrobeCounter_A < 8)
					return 0x40 | ibuf[InternalData.StrobeCounter_A++];
				break;
			case 0x4017: /* Данные контроллера 2 */
				break;
		}
		return 0x40;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		switch (Address) {
			/* Прямоугольный канал 1 */
			case 0x4000:
			case 0x4001:
			case 0x4002:
			case 0x4003:
			/* Прямоугольный канал 2 */
			case 0x4004:
			case 0x4005:
			case 0x4006:
			case 0x4007:
			/* Пилообразный канал */
			case 0x4008:
			case 0x400a:
			case 0x400b:
			/* Шум */
			case 0x400c:
			case 0x400e:
			case 0x400f:
			/* ДМ-канал */
			case 0x4010:
			case 0x4011:
			case 0x4012:
			case 0x4013:
			/* Другое */
			case 0x4014: /* DMA */
				break;
			case 0x4015: /* Упраление каналами */
				State.Write_4015(Src);
				break;
			case 0x4016: /* Стробирование контроллеров */
				InternalData.StrobeCounter_A = 0;
				InternalData.StrobeCounter_B = 0;
				break;
			case 0x4017: /* Счетчик кадров */
				State.Write_4017(Src);
				break;
		}
	}

	/* CPU IDLE */
	inline int WasteCycles() {
		int Res = InternalData.WasteCycles;

		InternalData.WasteCycles = 0;
		return Res;
	}
	/* Буфер */
	inline VPNES_IBUF &GetBuf() { return ibuf; }
};

}

#endif
