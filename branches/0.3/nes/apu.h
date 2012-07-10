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

#include <iostream>

#include <cstring>
#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace APUID {

typedef APUGroup<1>::ID::NoBatteryID InternalDataID;
typedef APUGroup<2>::ID::NoBatteryID CycleDataID;
typedef APUGroup<3>::ID::NoBatteryID ChannelsID;

}

/* APU */
template <class _Bus>
class CAPU: public CDevice {
private:
	/* Шина */
	_Bus *Bus;
	/* Предобработано тактов */
	int PreprocessedCycles;

	/* Внутренние данные */
	struct SInternalData {
		int StrobeCounter_A;
		int StrobeCounter_B;
		uint8 bMode; /* Начальный режим */
	} InternalData;

	/* Данные о тактах */
	struct SCycleData {
		int WasteCycles;
		int Step;
		int DMACycle;
		int CurCycle;
		int CyclesLeft;
		int SupressCounter;
	} CycleData;

	/* Число тактов на каждом шаге */
	static const int StepCycles[6];
	/* Перекрываемые кнопки контроллера */
	static const int ButtonsRemap[4];
	/* Таблица счетчика */
	static const int LengthCounterTable[32];
	/* Таблица формы */
	static const int DutyTable[32];
	/* Таблица выхода прямоугольных каналов */
	static const double SquareTable[31];

	struct SChannels {
		int CurCycle;
		int UpdCycle; /* Число тактов с данным выходом */
		double LastOutput; /* Текущий выход */
		bool DMCInterrupt; /* Флаг прерывания ДМ-канала */
		bool FrameInterrupt; /* Флаг прерывания счетчика кадров */
		bool Square2Counter; /* Флаг счетчика для прямоугольного канала 2 */
		bool Square1Counter; /* Флаг счетчика для прямоугольного канала 1 */
		bool DMCRemain; /* Флаг опустошения буфера ДМ-канала */
		bool InterruptInhibit; /* Подавление IRQ */
		enum SeqMode {
			Mode_4step = 4,
			Mode_5step = 5
		} Mode; /* Режим */

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
			int TimerCounter; /* Счетчик */
			int LengthCounter; /* Счетчик */
			int EnvelopeCounter; /* Счетчик огибающей */
			int EnvelopeDivider; /* Делитель огибающей */
			int SweepDivider; /* Делитель частоты свипа */
			bool SweepReload; /* Сбросить свип */
			inline void Write_1(uint8 Src) {
				DutyMode = Src >> 6; LengthCounterDisable = (Src & 0x20);
				EnvelopeConstant = (Src & 0x10); EnvelopePeriod = (Src & 0x0f);
			}
			inline void Write_2(uint8 Src) {
				SweepEnabled = (Src & 0x80); SweepPeriod = (Src >> 4) & 0x07;
				SweepNegative = (Src & 0x08); SweepShiftCount = (Src & 0x07);
				SweepReload = true;
			}
			inline void Write_3(uint8 Src) {
				Timer = (Timer & 0x0700) | Src;
			}
			inline void Write_4(uint8 Src, bool CounterEnable) {
				Timer = (Timer & 0x00ff) | ((Src & 0x07) << 8);
				if (CounterEnable)
					LengthCounter = LengthCounterTable[Src >> 3];
			}
			/* Генерация формы */
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
			/* Счетчик длины */
			inline void Do_LengthCounter() {
				if ((!LengthCounterDisable) && (LengthCounter != 0)) {
					LengthCounter--;
				}
			}
			/* Свип */
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
				if (SweepReload) /* Сброс _после_ обновления */
					SweepDivider = 0;
			}
			/* Таймер */
			inline bool Do_Timer(int Cycles) {
				TimerCounter += Cycles;
				if (TimerCounter >= Timer * 2) {
					TimerCounter -= Timer * 2;
					if (EnvelopeConstant)
						Output = EnvelopePeriod;
					else
						Output = EnvelopeCounter;
					Output &= DutyTable[DutyMode * 8 + DutyCycle];
					DutyCycle++;
					if (DutyCycle == 8)
						DutyCycle = 0;
					return true;
				}
				return false;
			}
		} SquareChannel1;
		struct SSquareChannel2: public SSquareChannel {
			using SSquareChannel::SweepEnabled;
			using SSquareChannel::SweepDivider;
			using SSquareChannel::SweepPeriod;
			using SSquareChannel::SweepShiftCount;
			using SSquareChannel::SweepReload;
			using SSquareChannel::SweepNegative;
			using SSquareChannel::Timer;

			/* Свип */
			inline void Sweep() {
				uint16 Res;

				if (!SweepEnabled)
					return;
				SweepDivider++;
				if (SweepDivider == SweepPeriod) {
					Res = Timer >> SweepShiftCount;
					if (SweepNegative)
						Res = ~Res + 1;
					Timer = (Timer + Res) & 0x1fff;
					SweepDivider = 0;
				}
				if (SweepReload) /* Сброс _после_ обновления */
					SweepDivider = 0;
			}
		} SquareChannel2;
		/* Обновить выход */
		inline void Update() {
			int SqOut = 0;
			double NewOutput;

			if (SquareChannel1.LengthCounter > 0)
				SqOut += SquareChannel1.Output;
			if (SquareChannel2.LengthCounter > 0)
				SqOut += SquareChannel2.Output;
			NewOutput = SquareTable[SqOut];
			if (LastOutput != NewOutput) {
				std::cout << NewOutput << '\t' << UpdCycle << std::endl;
				UpdCycle = 0;
				LastOutput = NewOutput;
			}
		}
		/* Генерировать форму каналов */
		inline void Envelope() {
			SquareChannel1.Envelope();
			SquareChannel2.Envelope();
		}
		/* Обновить счетчик длины */
		inline void LengthCounter() {
			SquareChannel1.Do_LengthCounter();
			SquareChannel2.Do_LengthCounter();
		}
		/* Свип */
		inline void Sweep() {
			SquareChannel1.Sweep();
			SquareChannel2.Sweep();
		}
		/* Четный такт */
		inline void EvenClock() {
			Envelope();
			//Linear counter
		}
		/* Нечетный такт */
		inline void OddClock() {
			EvenClock();
			LengthCounter();
			Sweep();
		}
		/* Таймер */
		inline void Do_Timer(int Cycles) {
			UpdCycle += Cycles;
			if (SquareChannel1.Do_Timer(Cycles) || SquareChannel2.Do_Timer(Cycles)) {
				Update();
			}
		}
		/* Таймер (с кешированием) */
		inline void Timer(int Cycles) {
			Do_Timer(Cycles - CurCycle);
			CurCycle = Cycles;
		}
		/* Таймер (сбросить кеш) */
		inline void FlushTimer(int Cycles) {
			Do_Timer(Cycles - CurCycle);
			CurCycle = 0;
		}
		inline void Write_4015(uint8 Src, SChannels &ChannelsUpd) { if (!(Src & 0x10))
			DMCRemain = false; /* NoiseCounter = Src & 0x08; TriangleCounter = Src & 0x04; */
			Square2Counter = Src & 0x02; Square1Counter = Src & 0x01;
			DMCInterrupt = false;
			if (!Square1Counter)
				SquareChannel1.LengthCounter = 0;
			if (!Square2Counter)
				SquareChannel2.LengthCounter = 0;
		}
		inline uint8 Read_4015(const SChannels &ChannelsUpd) {
			uint8 Res = 0; if (DMCInterrupt) Res |= 0x80;
			if (FrameInterrupt) Res |= 0x40; if (DMCRemain) Res |= 0x10;
			/* if (NoiseCounter) Res |= 0x08; if (TriangleCounter) Res |= 0x04; */
			if (ChannelsUpd.SquareChannel2.LengthCounter > 0) Res |= 0x02;
			if (ChannelsUpd.SquareChannel1.LengthCounter > 0) Res |= 0x01;
			return Res;
		}
		inline void Write_4017(uint8 Src) { Mode = (Src & 0x80) ? Mode_5step : Mode_4step;
			InterruptInhibit = (Src & 0x40); if (InterruptInhibit) FrameInterrupt = false;
		}
	} Channels;

	/* Буфер */
	VPNES_IBUF ibuf;

	/* Обработка */
	inline void Process(int Cycles) {
		CycleData.CyclesLeft += Cycles;
		while (CycleData.CyclesLeft >= 3) {
			CycleData.CyclesLeft -= 3;
			/* Сброс счетчика */
			if (CycleData.SupressCounter >= 0) {
				CycleData.SupressCounter++;
				if (CycleData.SupressCounter > 3) {
					CycleData.CurCycle = 0;
					CycleData.Step = 0;
					CycleData.SupressCounter = -1;
				}
			}
			if (CycleData.CurCycle == StepCycles[CycleData.Step]) {
				/* Секвенсер */
				switch (Channels.Mode) {
					case SChannels::Mode_4step:
						switch (CycleData.Step) {
							case 0:
							case 2:
								Channels.EvenClock();
								break;
							case 3:
								if (!Channels.InterruptInhibit) {
									Channels.FrameInterrupt = true;
									Bus->GetCPU()->GetIRQPin() = true;
								}
							case 1:
								Channels.OddClock();
								break;
						}
						break;
					case SChannels::Mode_5step:
						switch (CycleData.Step) {
							case 0:
							case 2:
								Channels.EvenClock();
								break;
							case 1:
							case 4:
								Channels.OddClock();
								break;
						}
						break;
				}
				Channels.Update();
				CycleData.Step++;
				if (CycleData.Step == Channels.Mode) /* Сбросить счетчик через 2 такта */
					CycleData.SupressCounter = 2;
			}
			Channels.Do_Timer(1);
			CycleData.CurCycle++;
		}
	}
public:
	inline explicit CAPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<APUID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
		InternalData.bMode = 0x00;
		Bus->GetManager()->template SetPointer<APUID::CycleDataID>(\
			&CycleData, sizeof(CycleData));
		Bus->GetManager()->template SetPointer<APUID::ChannelsID>(\
			&Channels, sizeof(Channels));
	}
	inline ~CAPU() {}

	/* Предобработать такты */
	inline void Preprocess() {
		/* Обрабатываем текущие такты */
		Process(Bus->GetClock()->GetPreCycles() - PreprocessedCycles + 3);
		PreprocessedCycles = Bus->GetClock()->GetPreCycles() + 3;
	}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		/* Обрабатываем необработанные такты */
		Process(Cycles - PreprocessedCycles);
		PreprocessedCycles = 0;
	}

	/* Сброс */
	inline void Reset() {
		memset(&Channels, 0, sizeof(Channels));
		Channels.Write_4017(InternalData.bMode);
		Channels.Update();
		memset(&InternalData, 0, sizeof(InternalData));
		memset(&CycleData, 0, sizeof(CycleData));
		CycleData.CurCycle = 6;
		CycleData.SupressCounter = -1;
		CycleData.DMACycle = -1;
		PreprocessedCycles = 0;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;

		switch (Address) {
			case 0x4015: /* Состояние APU */
				/* Отрабатываем прошедшие такты */
				Preprocess();
				Res = Channels.Read_4015(Channels);
				/* Если мы не попали на установку флага, сбрасываем его */
				if ((Channels.Mode != SChannels::Mode_4step) || (CycleData.CurCycle !=
					(StepCycles[SChannels::Mode_4step - 1] + 2))) {
					Channels.FrameInterrupt = false;
					Bus->GetCPU()->GetIRQPin() = false;
				}
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
		Preprocess();
		switch (Address) {
			/* Прямоугольный канал 1 */
			case 0x4000: /* Вид и огибающая */
				Channels.SquareChannel1.Write_1(Src);
				break;
			case 0x4001: /* Свип */
				Channels.SquareChannel1.Write_2(Src);
				break;
			case 0x4002: /* Период */
				Channels.SquareChannel1.Write_3(Src);
				break;
			case 0x4003: /* Счетчик, период */
				Channels.SquareChannel1.Write_4(Src, Channels.Square1Counter);
				break;
			/* Прямоугольный канал 2 */
			case 0x4004:
				Channels.SquareChannel2.Write_1(Src);
				break;
			case 0x4005:
				Channels.SquareChannel2.Write_2(Src);
				break;
			case 0x4006:
				Channels.SquareChannel2.Write_3(Src);
				break;
			case 0x4007:
				Channels.SquareChannel2.Write_4(Src, Channels.Square2Counter);
				break;
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
				/* ... */
				if (CycleData.CurCycle & 1) {
					CycleData.WasteCycles = 513;
					CycleData.DMACycle = 0;
				} else {
					CycleData.WasteCycles = 512;
					CycleData.DMACycle = 3;
				}
				Bus->GetPPU()->EnableDMA(Src);
				break;
			case 0x4015: /* Управление каналами */
				Channels.Write_4015(Src, Channels);
				break;
			case 0x4016: /* Стробирование контроллеров */
				InternalData.StrobeCounter_A = 0;
				InternalData.StrobeCounter_B = 0;
				break;
			case 0x4017: /* Счетчик кадров */
				InternalData.bMode = Src;
				/* Запись в 4017 */
				Channels.Write_4017(Src);
				Bus->GetCPU()->GetIRQPin() = Channels.FrameInterrupt;
				if (Channels.Mode == SChannels::Mode_5step)
					Channels.OddClock();
				CycleData.SupressCounter = CycleData.CurCycle & 1;
				break;
		}
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
const int CAPU<_Bus>::StepCycles[6] = {7456, 14912, 22370, 29828, 37280, 0};

/* Перекрываемые кнопки контроллера */
template <class _Bus>
const int CAPU<_Bus>::ButtonsRemap[4] = {
	VPNES_INPUT_DOWN, VPNES_INPUT_UP,
	VPNES_INPUT_RIGHT, VPNES_INPUT_LEFT
};

/* Таблица счетчика */
template <class _Bus>
const int CAPU<_Bus>::LengthCounterTable[32] = {
	0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a,
	0x0e, 0x0c, 0x1a, 0x0e, 0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e
};

/* Таблица формы */
template <class _Bus>
const int CAPU<_Bus>::DutyTable[32] = {
	0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
	0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Таблица выхода прямоугольных сигналов */
template <class _Bus>
const double CAPU<_Bus>::SquareTable[31] = {
	0.0000000, 0.0116091, 0.0229395, 0.0340009, 0.0448030, 0.0553547,
	0.0656645, 0.0757408, 0.0855914, 0.0952237, 0.1046450, 0.1138620,
	0.1228820, 0.1317100, 0.1403530, 0.1488160, 0.1571050, 0.1652260,
	0.1731830, 0.1809810, 0.1886260, 0.1961200, 0.2034700, 0.2106790,
	0.2177510, 0.2246890, 0.2314990, 0.2381820, 0.2447440, 0.2511860,
	0.2575130
};

}

#endif
