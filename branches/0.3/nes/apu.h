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
typedef APUGroup<4>::ID::NoBatteryID ChannelsID;

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
		uint8 bMode; /* Начальный режим */
		bool SupressCounter; /* Сбросить счетчик */
		bool CachedInterrupt;
	} InternalData;

	/* Данные о тактах */
	struct SCycleData {
		int CyclesLeft;
		int WasteCycles;
		int Step;
		int DMACycle;
		int SupressCycles;
	} CycleData;

	/* Число тактов на каждом шаге */
	static const int StepCycles[5];
	/* Перекрываемые кнопки контроллера */
	static const int ButtonsRemap[4];
	/* Таблица счетчика */
	static const int LengthCounterTable[32];

	struct SChannels {
		int CurCycle;

		/* Прямоугольный канал */
		struct SSquareChannel {
			int Output; /* Выход на ЦАП */
			bool Start; /* Начать */
			int DutyCycle; /* Цикл */
			uint8 DutyMode; /* Режим */
			bool LengthCounterDisable; /* Отключить счетчик */
			bool EnvelopeConstant; /* Константный выход */
			uint8 EnvelopePeriod; /* Период последовательности */
			bool SweepEnabled; /* Свип включен */
			uint8 SweepPeriod; /* Период */
			bool SweepNegative; /* Отрицательный */
			uint8 SweepShiftCount; /* Число сдвигов */
			uint16 Timer; /* Период */
			int LengthCounter; /* Счетчик */
			int EnvelopeCounter; /* Счетчик огибающей */
			int EnvelopeDivider; /* Делитель огибающей */
			int SweepDivider; /* Делитель частоты свипа */
			bool SweepReload; /* Сбросить свип */
			inline void Write_4000(uint8 Src) {
				DutyMode = Src >> 6; LengthCounterDisable = (Src & 0x20);
				EnvelopeConstant = (Src & 0x10); EnvelopePeriod = (Src & 0x0f);
			}
			inline void Write_4001(uint8 Src) {
				SweepEnabled = (Src & 0x80); SweepPeriod = (Src >> 4) & 0x07;
				SweepNegative = (Src & 0x08); SweepShiftCount = (Src & 0x07);
				SweepReload = true;
			}
			inline void Write_4002(uint8 Src) {
				Timer = (Timer & 0x0700) | Src;
			}
			inline void Write_4003(uint8 Src) {
				Timer = (Timer & 0x00ff) | ((Src & 0x07) << 8);
				LengthCounter = LengthCounterTable[Src >> 3];
			}
			inline void Envelope() {
				if (EnvelopeConstant)
					return;
				if (Start) {
					EnvelopeCounter = 15;
					EnvelopeDivider = 0;
				} else {
					EnvelopeDivider++;
					if (EnvelopeDivider == EnvelopePeriod) {
						if (EnvelopeCounter == 0) {
							if (LengthCounterDisable)
								EnvelopeCounter = 15;
						} else
							EnvelopeCounter--;
						EnvelopeDivider = 0;
					}
				}
			}
			inline void Do_LengthCounter() {
				if ((!LengthCounterDisable) && (LengthCounter != 0))
					LengthCounter--;
			}
			inline void Sweep() {
				uint16 Res;

				if (!SweepEnabled)
					return;
				SweepDivider++;
				if (SweepDivider == SweepPeriod) {
					Res = Timer >> SweepShiftCount;
					if (SweepNegative)
						Res = ~Res;
					Timer = (Timer + Res) & 0x1fff;
					SweepDivider = 0;
				}
				if (SweepReload)
					SweepDivider = 0;
			}
			inline void Do_Timer(int Cycles) {
			}
		} SquareChannel1;
		inline void Envelope() {
			SquareChannel1.Envelope();
		}
		inline void LengthCounter() {
			SquareChannel1.Do_LengthCounter();
		}
		inline void Sweep() {
			SquareChannel1.Sweep();
		}
		inline void Do_Timer(int Cycles) {
			SquareChannel1.Do_Timer(Cycles);
		}
		inline void Timer(int Cycles) {
			Do_Timer(Cycles - CurCycle);
			CurCycle = Cycles;
		}
		inline void FlushTimer(int Cycles) {
			Do_Timer(Cycles - CurCycle);
			CurCycle = 0;
		}
	} Channels;

	/* Состояние */
	struct SState {
		bool DMCInterrupt; /* Флаг прерывания ДМ-канала */
		bool FrameInterrupt; /* Флаг прерывания счетчика кадров */
		bool NoiseCounter; /* Флаг счетчика для канала шума */
		bool TriangleCounter; /* Флаг счетчика для пилообразного канала */
		bool Square2Counter; /* Флаг счетчика для прямоугольного канала 2 */
		bool Square1Counter; /* Флаг счетчика для прямоугольного канала 1 */
		bool DMCRemain; /* Флаг опустошения буфера ДМ-канала */
		bool InterruptInhibit; /* Подавление IRQ */
		enum SeqMode {
			Mode_4step,
			Mode_5step
		} Mode; /* Режим */
		inline void UpdateCounters(SChannels &ChannelsUpd) {
			if (!Square1Counter)
				ChannelsUpd.SquareChannel1.LengthCounter = 0;
		}
		inline void Write_4015(uint8 Src, SChannels &ChannelsUpd) { if (!(Src & 0x10))
			DMCRemain = false; NoiseCounter = Src & 0x08; TriangleCounter = Src & 0x04;
			Square2Counter = Src & 0x02; Square1Counter = Src & 0x01;
			DMCInterrupt = false;
			UpdateCounters(ChannelsUpd);
		}
		inline uint8 Read_4015(const SChannels &ChannelsUpd) {
			uint8 Res = 0; if (DMCInterrupt) Res |= 0x80;
			if (FrameInterrupt) Res |= 0x40; if (DMCRemain) Res |= 0x10;
			if (NoiseCounter) Res |= 0x08; if (TriangleCounter) Res |= 0x04;
			if (Square2Counter) Res |= 0x02;
			if (ChannelsUpd.SquareChannel1.LengthCounter > 0) Res |= 0x01;
			return Res;
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
		InternalData.bMode = 0x00;
		Bus->GetManager()->template SetPointer<APUID::CycleDataID>(\
			&CycleData, sizeof(CycleData));
		Bus->GetManager()->template SetPointer<APUID::StateID>(\
			&State, sizeof(State));
		Bus->GetManager()->template SetPointer<APUID::ChannelsID>(\
			&Channels, sizeof(Channels));
	}
	inline ~CAPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		CycleData.CyclesLeft += Cycles;
		/* Сброс счетчика */
		if (InternalData.SupressCounter) {
			CycleData.SupressCycles += Cycles;
			if (CycleData.SupressCycles >= 0)
				/* Обрабатываем только 2 или 3 такта */
				CycleData.CyclesLeft -= CycleData.SupressCycles;
		}
		for (;;) {
			while (CycleData.CyclesLeft >= 3 * StepCycles[CycleData.Step]) {
				Channels.FlushTimer(3 * StepCycles[CycleData.Step]);
				CycleData.CyclesLeft -= 3 * StepCycles[CycleData.Step];
				switch (State.Mode) {
					case SState::Mode_4step:
						switch (CycleData.Step) {
							case 0:
								Channels.Envelope();
								//Linear counter
							case 1:
								Channels.Envelope();
								//Linear counter
								Channels.LengthCounter();
								Channels.Sweep();
							case 2:
								Channels.Envelope();
								//Linear counter
								break;
							case 3:
								Channels.Envelope();
								//Linear counter
								Channels.LengthCounter();
								Channels.Sweep();
								if (!State.InterruptInhibit) {
									InternalData.CachedInterrupt =
										State.FrameInterrupt;
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
								Channels.Envelope();
								//Linear counter
							case 1:
								Channels.Envelope();
								//Linear counter
								Channels.LengthCounter();
								Channels.Sweep();
							case 2:
								Channels.Envelope();
								//Linear counter
							case 4:
								Channels.Envelope();
								//Linear counter
								Channels.LengthCounter();
								Channels.Sweep();
								break;
						}
						CycleData.Step++;
						if (CycleData.Step == 5)
							CycleData.Step = 0;
						break;
				}
			}
			Channels.Timer(CycleData.CyclesLeft);
			/* Необходимо обработать остальные такты */
			if (InternalData.SupressCounter &&
				(CycleData.SupressCycles >= 0)) {
				/* Сброс счетчика */
				CycleData.CyclesLeft = CycleData.SupressCycles;
				CycleData.Step = 0;
				Channels.CurCycle = 0;
				InternalData.SupressCounter = false;
			} else
				break;
		}
	}

	/* Сброс */
	inline void Reset() {
		memset(&State, 0, sizeof(State));
		State.Write_4017(InternalData.bMode);
		memset(&InternalData, 0, sizeof(InternalData));
		memset(&CycleData, 0, sizeof(CycleData));
		CycleData.DMACycle = -1;
		CycleData.CyclesLeft = -4;
		memset(&Channels, 0, sizeof(Channels));
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;

		switch (Address) {
			case 0x4015: /* Состояние APU */
				/* Отрабатываем прошедшие такты */
				Clock(Bus->GetClock()->GetPreCycles());
				Res = State.Read_4015(Channels);
				/* Если мы не попали на установку флага, сбрасываем его */
				if ((CycleData.CyclesLeft >= 6) ||
					(CycleData.Step != 0) || (State.Mode != SState::Mode_4step)) {
					State.FrameInterrupt = false;
					Bus->GetCPU()->GetIRQPin() = false;
				}
				/* Не обрабатывать такты снова */
				CycleData.CyclesLeft -= Bus->GetClock()->GetPreCycles();
				return Res;
			case 0x4016: /* Данные контроллера 1 */
				if (InternalData.StrobeCounter_A < 4)
					return 0x40 | ibuf[InternalData.StrobeCounter_A++];
				if (InternalData.StrobeCounter_A < 8) {
					Res = 0x40 | (ibuf[InternalData.StrobeCounter_A] &
						~ibuf[ButtonsRemap[InternalData.StrobeCounter_A & 0x03]]);
					InternalData.StrobeCounter_A++;
					return Res;
				}
				break;
			case 0x4017: /* Данные контроллера 2 */
				break;
		}
		return 0x40;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		/* Отрабатываем прошедшие такты */
		Clock(Bus->GetClock()->GetPreCycles());
		switch (Address) {
			/* Прямоугольный канал 1 */
			case 0x4000: /* Вид и огибающая */
				Channels.SquareChannel1.Write_4000(Src);
				break;
			case 0x4001: /* Свип */
				Channels.SquareChannel1.Write_4001(Src);
				break;
			case 0x4002: /* Период */
				Channels.SquareChannel1.Write_4002(Src);
				break;
			case 0x4003: /* Счетчик, период */
				Channels.SquareChannel1.Write_4003(Src);
				State.UpdateCounters(Channels);
				break;
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
				break;
			/* Другое */
			case 0x4014: /* DMA */
				if (((CycleData.CyclesLeft +
					Bus->GetClock()->GetPreCycles()) % 6) >= 3) {
					CycleData.WasteCycles = 513;
					CycleData.DMACycle = 0;
				} else {
					CycleData.WasteCycles = 512;
					CycleData.DMACycle = 3;
				}
				Bus->GetPPU()->EnableDMA(Src);
				break;
			case 0x4015: /* Управление каналами */
				State.Write_4015(Src, Channels);
				break;
			case 0x4016: /* Стробирование контроллеров */
				InternalData.StrobeCounter_A = 0;
				InternalData.StrobeCounter_B = 0;
				break;
			case 0x4017: /* Счетчик кадров */
				InternalData.bMode = Src;
				/* Запись в 4017 */
				State.Write_4017(Src);
				Bus->GetCPU()->GetIRQPin() = State.FrameInterrupt;
				InternalData.SupressCounter = true;
				if (State.Mode == SState::Mode_5step) {
					Channels.Envelope();
					//Linear counter
					Channels.LengthCounter();
					Channels.Sweep();
				}
				/* +1 лишний такт... */
				/* TODO: Исправить синхронизацию */
				CycleData.SupressCycles = -Bus->GetClock()->GetPreCycles() -
					(((CycleData.CyclesLeft % 6) < 3) ? 12 : 9);
				break;
		}
		/* Не обрабатывать такты снова */
		CycleData.CyclesLeft -= Bus->GetClock()->GetPreCycles();
	}

	/* CPU IDLE */
	inline int WasteCycles() {
		int Res = CycleData.WasteCycles;

		CycleData.WasteCycles = 0;
		return Res;
	}
	/* Текущий такт DMA */
	inline int GetDMACycle(int Cycles) {
		if (CycleData.DMACycle >= 0) {
			CycleData.DMACycle += Cycles;
			if (CycleData.DMACycle >= 1539)
				CycleData.DMACycle = -1;
			else
				return CycleData.DMACycle / 3;
		}
		return -1;
	}
	/* Буфер */
	inline VPNES_IBUF &GetBuf() { return ibuf; }
};

/* Число тактов для каждого шага */
template <class _Bus>
const int CAPU<_Bus>::StepCycles[5] = {7456, 7456, 7458, 7458, 7452};

/* Перекрываемые кнопки контроллера */
template <class _Bus>
const int CAPU<_Bus>::ButtonsRemap[4] = {VPNES_INPUT_DOWN, VPNES_INPUT_UP,
	VPNES_INPUT_RIGHT, VPNES_INPUT_LEFT};

/* Таблица счетчика */
template <class _Bus>
const int CAPU<_Bus>::LengthCounterTable[32] = {
	0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a,
	0x0e, 0x0c, 0x1a, 0x0e, 0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e};

}

#endif
