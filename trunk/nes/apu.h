/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2013  Ivanov Viktor

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

#include <algorithm>
#include <cstring>
#include "aputables.h"
#include "manager.h"
#include "frontend.h"
#include "bus.h"

namespace vpnes {

namespace APUID {

typedef APUGroup<1>::ID::NoBatteryID InternalDataID;
typedef APUGroup<2>::ID::NoBatteryID CycleDataID;
typedef APUGroup<3>::ID::NoBatteryID ChannelsID;

}

/* APU */
template <class _Bus, class _Settings>
class CAPU: public CDevice {
public:
	/* Таблицы */
	typedef typename _Settings::Tables Tables;
	/* Делитель частоты */
	enum { ClockDivider = _Bus::CPUClass::ClockDivider };
private:
	/* Шина */
	_Bus *Bus;
	/* Предобработано тактов */
	int PreprocessedCycles;

	/* Прерывания */
	enum {
		FrameIRQ = 0x10,
		DMCIRQ = 0x20
	};

	/* Внутренние данные */
	struct SInternalData: public ManagerID<APUID::InternalDataID> {
		uint8 bMode; /* Начальный режим */
		int DMA;
		int Odd;
	} InternalData;

	/* Данные о тактах */
	struct SCycleData: public ManagerID<APUID::CycleDataID> {
		int Step;
		int CurCycle;
		int NextCycle;
		int LastCycle;
		int CyclesLeft;
		int ResetCycle;
		int DebugTimer;
		int LastCur;
	} CycleData;

	struct SChannels: public ManagerID<APUID::ChannelsID> {
		int NextCycle; /* Следующее действие */
		int CurCycle; /* Текущий такт */
		int UpdCycle; /* Число тактов с данным выходом */
		double LastOutput; /* Текущий выход */

		/* Дописать буфер */
		inline void FlushBuffer(_Bus *pBus) {
			pBus->GetFrontend()->GetAudioFrontend()->PushSample(LastOutput, UpdCycle *
				_Bus::ClockClass::GetFix() * ClockDivider);
			UpdCycle = 0;
		}

		bool FrameInterrupt; /* Флаг прерывания счетчика кадров */
		int InterruptInhibit; /* Подавление IRQ */
		enum SeqMode {
			Mode_4step = 4,
			Mode_5step = 5
		} Mode; /* Режим */

		/* Прямоугольный канал */
		struct SSquareChannel {
			int Output; /* Выход на ЦАП */
			int UseCounter; /* Использовать счетчик */
			bool Start; /* Начать */
			int DutyCycle; /* Цикл */
			uint8 DutyMode; /* Режим */
			int LengthCounterDisable; /* Отключить счетчик */
			int EnvelopeConstant; /* Константный выход */
			uint8 EnvelopePeriod; /* Период последовательности */
			int SweepEnabled; /* Свип включен */
			uint8 SweepPeriod; /* Период */
			int SweepNegative; /* Отрицательный */
			uint8 SweepShiftCount; /* Число сдвигов */
			uint16 Timer; /* Период */
			uint16 TimerTarget; /* Обновление периода */
			bool Valid; /* Новый период правильный */
			int TimerCounter; /* Счетчик */
			int LengthCounter; /* Счетчик */
			int EnvelopeCounter; /* Счетчик огибающей */
			int EnvelopeDivider; /* Делитель огибающей */
			int SweepDivider; /* Делитель частоты свипа */
			bool SweepReload; /* Сбросить свип */
			inline void Write_1(uint8 Src) {
				DutyMode = Src >> 6; LengthCounterDisable = (Src & 0x20);
				EnvelopeConstant = (Src & 0x10); EnvelopePeriod = (Src & 0x0f);
				if (EnvelopeConstant)
					Output = EnvelopePeriod;
				else
					Output = EnvelopeCounter;
			}
			inline void Write_2(uint8 Src) {
				SweepEnabled = (Src & 0x80); SweepPeriod = (Src >> 4) & 0x07;
				SweepNegative = (Src & 0x08); SweepShiftCount = (Src & 0x07);
				SweepReload = true;
			}
			inline void Write_3(uint8 Src) {
				if (Timer < 8) {
					DutyCycle = (DutyCycle + (TimerCounter / ((Timer + 1) << 1))) & 7;
					TimerCounter %= ((Timer + 1) << 1);
				}
				Timer = (Timer & 0x0700) | Src;
			}
			inline void Write_4(uint8 Src) {
				if (Timer < 8)
					TimerCounter %= ((Timer + 1) << 1);
				Timer = (Timer & 0x00ff) | ((Src & 0x07) << 8);
				Start = true;
				DutyCycle = 0;
				if (UseCounter)
					LengthCounter = Tables::LengthCounterTable[Src >> 3];
			}
			/* Генерация формы */
			inline void Envelope() {
				if (Start) {
					EnvelopeCounter = 15;
					EnvelopeDivider = 0;
					if (!EnvelopeConstant)
						Output = EnvelopeCounter;
					Start = false;
				} else {
					EnvelopeDivider++;
					if (EnvelopeDivider > EnvelopePeriod) {
						if (EnvelopeCounter == 0) {
							if (LengthCounterDisable)
								EnvelopeCounter = 15;
						} else
							EnvelopeCounter--;
						if (!EnvelopeConstant)
							Output = EnvelopeCounter;
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
				SweepDivider++;
				if (SweepDivider > SweepPeriod) {
					if (SweepEnabled && Valid && (SweepShiftCount > 0))
						Timer = TimerTarget & 0x07ff;
					SweepDivider = 0;
				}
				if (SweepReload) { /* Сброс _после_ обновления */
					SweepDivider = 0;
					SweepReload = false;
				}
			}
			/* Таймер */
			inline bool Do_Timer(int Cycles) {
				TimerCounter += Cycles;
				if (Timer < 8)
					return false;
				if (TimerCounter >= ((Timer + 1) << 1)) {
					TimerCounter = 0;
					DutyCycle++;
					DutyCycle &= 7;
					return true;
				}
				return false;
			}
			/* Возможность вывода */
			inline bool CanOutput() {
				return (LengthCounter > 0) && Valid &&
					Tables::DutyTable[(DutyMode << 3) + DutyCycle];
			}
			/* Обновить такты */
			inline void UpdateCycles(int &Cycle) {
				int RestCycles;

				if (Timer < 8)
					return;
				RestCycles = ((Timer + 1) << 1) - TimerCounter;
				if (RestCycles <= 0)
					RestCycles = 1;
				if (RestCycles < Cycle)
					Cycle = RestCycles;
			}
		};

		/* Прямоугольный канал 1 */
		struct SSquareChannel1: public SSquareChannel {
			using SSquareChannel::SweepShiftCount;
			using SSquareChannel::SweepNegative;
			using SSquareChannel::Timer;
			using SSquareChannel::TimerTarget;
			using SSquareChannel::Valid;
			using SSquareChannel::UpdateCycles;

			/* Вычислить свип */
			inline void CalculateSweep(int &Cycle) {
				uint16 Res;

				Res = Timer >> SweepShiftCount;
				if (SweepNegative)
					Res = ~Res;
				TimerTarget = Timer + Res;
				Valid = (Timer > 7) && (SweepNegative || (TimerTarget < 0x0800));
				UpdateCycles(Cycle);
			}
		} SquareChannel1;

		/* Прямоугольный канал 2 */
		struct SSquareChannel2: public SSquareChannel {
			using SSquareChannel::SweepShiftCount;
			using SSquareChannel::SweepNegative;
			using SSquareChannel::Timer;
			using SSquareChannel::TimerTarget;
			using SSquareChannel::Valid;
			using SSquareChannel::UpdateCycles;

			/* Вычислить свип */
			inline void CalculateSweep(int &Cycle) {
				uint16 Res;

				Res = Timer >> SweepShiftCount;
				if (SweepNegative)
					Res = -Res;
				TimerTarget = Timer + Res;
				Valid = (Timer > 7) && (SweepNegative || (TimerTarget < 0x0800));
				UpdateCycles(Cycle);
			}
		} SquareChannel2;

		/* Пилообразный канал */
		struct STriangleChannel {
			int Output; /* Выход на ЦАП */
			int UseCounter; /* Использовать счетчик */
			int HaltFlag; /* Флаг сброса */
			int ControlFlag; /* Флаг управления счетчиком */
			uint8 LinearCounterReload; /* Период счетчика */
			uint16 Timer; /* Период */
			int TimerCounter; /* Счетчик */
			int LengthCounter; /* Счетчик длины */
			int LinearCounter; /* Счетчик */
			int Sequencer; /* Секвенсер */
			inline void Write_1(uint8 Src) {
				ControlFlag = Src & 0x80; LinearCounterReload = Src & 0x7f;
			}
			inline void Write_2(uint8 Src, int &Cycle) {
				Timer = (Timer & 0x0700) | Src;
				if (Timer < 2)
					TimerCounter = 0;
				UpdateCycles(Cycle);
			}
			inline void Write_3(uint8 Src, int &Cycle) {
				Timer = (Timer & 0x00ff) | ((Src & 0x07) << 8);
				if (Timer < 2)
					TimerCounter = 0;
				if (UseCounter)
					LengthCounter = Tables::LengthCounterTable[Src >> 3];
				HaltFlag = true;
				UpdateCycles(Cycle);
			}
			/* Счетчик длины */
			inline void Do_LinearCounter() {
				if (HaltFlag) {
					LinearCounter = LinearCounterReload;
				} else if (LinearCounter != 0) {
					LinearCounter--;
				}
				if (!ControlFlag)
					HaltFlag = false;
			}
			/* Счетчик длины */
			inline void Do_LengthCounter() {
				if ((!ControlFlag) && (LengthCounter != 0)) {
					LengthCounter--;
				}
			}
			/* Таймер */
			inline bool Do_Timer(int Cycles) {
				TimerCounter += Cycles;
				if (Timer > 0) {
					if (TimerCounter > Timer) {
						TimerCounter = 0;
						if ((LinearCounter > 0) && (LengthCounter > 0)) {
							Output = Tables::SeqTable[Sequencer++];
							Sequencer &= 31;
							return true;
						}
					}
				} else {
					if ((LinearCounter > 0) && (LengthCounter > 0))
						Sequencer = (Sequencer + TimerCounter) & 31;
					TimerCounter = 0;
					/* Не обновляем выход */
				}
				return false;
			}
			/* На высокой частоте используем приближенное значение */
			inline bool HighFreq() {
				return (Timer < 2) && (LinearCounter > 0) && (LengthCounter > 0);
			}
			/* Обновить такты */
			inline void UpdateCycles(int &Cycle) {
				int RestCycles;

				if (Timer > 1) {
					RestCycles = Timer + 1 - TimerCounter;
					if (RestCycles <= 0)
						RestCycles = 1;
					if (RestCycles < Cycle)
						Cycle = RestCycles;
				}
			}
		} TriangleChannel;

		/* Канал шума */
		struct SNoiseChannel {
			int Output; /* Выход */
			int UseCounter; /* Использовать счетчик */
			bool Start; /* Начать */
			int LengthCounterDisable; /* Отключить счетчик */
			int EnvelopeConstant; /* Константный выход */
			uint8 EnvelopePeriod; /* Период последовательности */
			uint16 Timer; /* Период */
			int Shift; /* Сдвиг */
			int TimerCounter; /* Счетчик */
			int LengthCounter; /* Счетчик */
			int EnvelopeCounter; /* Счетчик огибающей */
			int EnvelopeDivider; /* Делитель огибающей */
			uint16 Random; /* Биты рандома */
			inline void Write_1(uint8 Src) {
				LengthCounterDisable = (Src & 0x20);
				EnvelopeConstant = (Src & 0x10);
				EnvelopePeriod = (Src & 0x0f);
				if (EnvelopeConstant)
					Output = EnvelopePeriod;
				else
					Output = EnvelopeCounter;
			}
			inline void Write_2(uint8 Src, int &Cycle) {
				Timer = Tables::NoiseTable[Src & 0x0f];
				Shift = ((Src & 0x80) ? 8 : 13);
				UpdateCycles(Cycle);
			}
			inline void Write_3(uint8 Src) {
				Start = true;
				if (UseCounter)
					LengthCounter = Tables::LengthCounterTable[Src >> 3];
			}
			/* Генерация формы */
			inline void Envelope() {
				if (Start) {
					EnvelopeCounter = 15;
					EnvelopeDivider = 0;
					if (!EnvelopeConstant)
						Output = EnvelopeCounter;
					Start = false;
				} else {
					EnvelopeDivider++;
					if (EnvelopeDivider > EnvelopePeriod) {
						if (EnvelopeCounter == 0) {
							if (LengthCounterDisable)
								EnvelopeCounter = 15;
						} else
							EnvelopeCounter--;
						if (!EnvelopeConstant)
							Output = EnvelopeCounter;
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
			/* Таймер */
			inline bool Do_Timer(int Cycles) {
				TimerCounter += Cycles;
				if (TimerCounter >= Timer) {
					TimerCounter = 0;
					Random = (((Random >> 14) ^ (Random >> Shift)) & 0x01) |
						(Random << 1);
					return true;
				}
				return false;
			}
			/* Возможность вывода */
			inline bool CanOutput() {
				return (LengthCounter > 0) && !(Random & 0x4000);
			}
			/* Обновить такты */
			inline void UpdateCycles(int &Cycle) {
				int RestCycles;

				RestCycles = Timer - TimerCounter;
				if (RestCycles <= 0)
					RestCycles = 1;
				if (RestCycles < Cycle)
					Cycle = RestCycles;
			}
		} NoiseChannel;

		/* ДМ-канал */
		struct SDMChannel {
			uint8 Output; /* Выход на ЦАП */
			bool InterruptFlag; /* Флаг прерывания */
			int InterruptEnabled; /* Прерывание включено */
			int LoopFlag; /* Флаг цикличности */
			uint16 Timer; /* Период */
			uint16 LastAddress; /* Адрес */
			uint16 LastCounter; /* Длина */
			uint16 Address; /* Адрес */
			uint16 LengthCounter; /* Длина */
			bool NotEmpty; /* Буфер не пуст */
			uint8 SampleBuffer; /* Буфер */
			uint8 ShiftReg; /* Регистр сдвига */
			int ShiftCounter; /* Счетчик сдвигов */
			bool SilenceFlag; /* Флаг тишины */
			int TimerCounter; /* Счетчик таймера */
			inline void Write_1(uint8 Src, int &Cycle) {
				InterruptEnabled = Src & 0x80;
				if (!InterruptEnabled)
					InterruptFlag = false;
				LoopFlag = Src & 0x40;
				Timer = Tables::DMTable[Src & 0x0f];
				UpdateCycles(Cycle);
			}
			inline void Write_2(uint8 Src) {
				Output = Src & 0x7f;
			}
			inline void Write_3(uint8 Src) {
				Address = 0xc000 | (Src << 6);
				LastAddress = Address;
			}
			inline void Write_4(uint8 Src) {
				LastCounter = (Src << 4) | 0x0001;
				if (LengthCounter == 0)
					LengthCounter = LastCounter;
			}
			/* Таймер */
			inline bool Do_Timer(int Cycles) {
				TimerCounter += Cycles;
				if (TimerCounter >= Timer) {
					TimerCounter = 0;
					if (ShiftCounter == 0) {
						ShiftCounter = 8;
						if (NotEmpty) {
							SilenceFlag = false;
							NotEmpty = false;
							ShiftReg = SampleBuffer;
						} else
							SilenceFlag = true;
					}
					if (!SilenceFlag) {
						if (ShiftReg & 0x01) {
							if (Output < 0x7e)
								Output += 2;
						} else {
							if (Output > 1)
								Output -= 2;
						}
					}
					ShiftReg >>= 1;
					ShiftCounter--;
					return !SilenceFlag;
				}
				return false;
			}
			/* Обновить такты */
			inline void UpdateCycles(int &Cycle) {
				int RestCycles;

				RestCycles = Timer - TimerCounter;
				if (RestCycles <= 0)
					RestCycles = 1;
				if (RestCycles < Cycle)
					Cycle = RestCycles;
			}
		} DMChannel;

		/* Обновить выход */
		inline void Update(_Bus *pBus) {
			CAudioFrontend::SOutput Output; /* Вход DAC */
			double NewOutput; /* Выход DAC */

			if (SquareChannel1.CanOutput())
				Output.Square1 = SquareChannel1.Output;
			else
				Output.Square1 = 0;
			if (SquareChannel2.CanOutput())
				Output.Square2 = SquareChannel2.Output;
			else
				Output.Square2 = 0;
			if (NoiseChannel.CanOutput())
				Output.Noise = NoiseChannel.Output;
			else
				Output.Noise = 0;
			Output.DMC = DMChannel.Output;
			if (TriangleChannel.HighFreq())
				Output.Triangle = -1;
			else 
				Output.Triangle = TriangleChannel.Output;
			NewOutput = pBus->GetFrontend()->GetAudioFrontend()->\
				template UpdateDAC<typename _Bus::ROMClass>(pBus->GetROM(), &Output);
			if (NewOutput != LastOutput) {
				FlushBuffer(pBus);
				LastOutput = NewOutput;
			}
		}

		/* Генерировать форму каналов */
		inline void Envelope() {
			SquareChannel1.Envelope();
			SquareChannel2.Envelope();
			NoiseChannel.Envelope();
		}
		/* Обновить счетчик длины */
		inline void LengthCounter() {
			SquareChannel1.Do_LengthCounter();
			SquareChannel2.Do_LengthCounter();
			TriangleChannel.Do_LengthCounter();
			NoiseChannel.Do_LengthCounter();
		}
		/* Свип */
		inline void Sweep() {
			SquareChannel1.Sweep();
			SquareChannel1.CalculateSweep(NextCycle);
			SquareChannel2.Sweep();
			SquareChannel2.CalculateSweep(NextCycle);
		}
		/* Четный такт */
		inline void EvenClock() {
			Envelope();
			TriangleChannel.Do_LinearCounter();
		}
		/* Нечетный такт */
		inline void OddClock() {
			EvenClock();
			LengthCounter();
			Sweep();
		}
		/* Таймер */
		inline void Do_Timer(int Cycles, _Bus *pBus) {
			UpdCycle += Cycles;
			if (SquareChannel1.Do_Timer(Cycles) |
				SquareChannel2.Do_Timer(Cycles) |
				TriangleChannel.Do_Timer(Cycles) |
				NoiseChannel.Do_Timer(Cycles) |
				DMChannel.Do_Timer(Cycles) |
				pBus->GetROM()->Do_Timer(Cycles)) {
				Update(pBus);
			}
			NextCycle -= Cycles;
			if (NextCycle <= 0) {
				NextCycle = 0x0800;
				SquareChannel1.UpdateCycles(NextCycle);
				SquareChannel2.UpdateCycles(NextCycle);
				TriangleChannel.UpdateCycles(NextCycle);
				NoiseChannel.UpdateCycles(NextCycle);
				DMChannel.UpdateCycles(NextCycle);
				pBus->GetROM()->UpdateAPUCycles(NextCycle);
				if (NextCycle == 0x0800)
					NextCycle = 0;
			}
		}
		/* Выполнить таймиер */
		inline void Timer(int Cycles, _Bus *pBus) {
			Do_Timer(Cycles - CurCycle, pBus);
			CurCycle = Cycles;
		}

		inline void Write_4015(uint8 Src) {
			if (!(Src & 0x10))
				DMChannel.LengthCounter = 0;
			else if (DMChannel.LengthCounter == 0)
				DMChannel.LengthCounter = DMChannel.LastCounter;
			NoiseChannel.UseCounter = Src & 0x08;
			TriangleChannel.UseCounter = Src & 0x04;
			SquareChannel2.UseCounter = Src & 0x02;
			SquareChannel1.UseCounter = Src & 0x01;
			DMChannel.InterruptFlag = false;
			if (!SquareChannel1.UseCounter)
				SquareChannel1.LengthCounter = 0;
			if (!SquareChannel2.UseCounter)
				SquareChannel2.LengthCounter = 0;
			if (!TriangleChannel.UseCounter)
				TriangleChannel.LengthCounter = 0;
			if (!NoiseChannel.UseCounter)
				NoiseChannel.LengthCounter = 0;
		}
		inline uint8 Read_4015() {
			uint8 Res = 0;

			if (DMChannel.InterruptFlag) Res |= 0x80;
			if (FrameInterrupt) Res |= 0x40;
			if (DMChannel.LengthCounter > 0) Res |= 0x10;
			if (NoiseChannel.LengthCounter > 0) Res |= 0x08;
			if (TriangleChannel.LengthCounter > 0) Res |= 0x04;
			if (SquareChannel2.LengthCounter > 0) Res |= 0x02;
			if (SquareChannel1.LengthCounter > 0) Res |= 0x01;
			return Res;
		}
		inline void Write_4017(uint8 Src) {
			Mode = (Src & 0x80) ? Mode_5step : Mode_4step;
			InterruptInhibit = (Src & 0x40);
			if (InterruptInhibit)
				FrameInterrupt = false;
		}
	} Channels;

	/* Обработка */
	inline void Process(int Cycles);
public:
	inline explicit CAPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		InternalData.bMode = 0x00;
		Bus->GetManager()->template SetPointer<SCycleData>(&CycleData);
		Bus->GetManager()->template SetPointer<SChannels>(&Channels);
		Channels.TriangleChannel.Sequencer = 0;
		Channels.TriangleChannel.ControlFlag = false;
	}
	inline ~CAPU() {}

	/* Предобработать такты */
	inline void Preprocess() {
		/* Обрабатываем текущие такты */
		Process(Bus->GetClock()->GetPreCPUCycles() - PreprocessedCycles);
		PreprocessedCycles = Bus->GetClock()->GetPreCPUCycles();
		/* Обновляем состояние каналов */
		Channels.Timer(CycleData.CyclesLeft, Bus);
	}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		/* Обрабатываем необработанные такты */
		Process(Cycles - PreprocessedCycles);
		PreprocessedCycles = 0;
		CycleData.LastCycle = CycleData.CyclesLeft;
	}

	/* Сброс */
	inline void Reset() {
		int oldout;
		int oldcontr;

		oldout = Channels.TriangleChannel.Sequencer;
		oldcontr = Channels.TriangleChannel.ControlFlag;
		memset(&Channels, 0, sizeof(Channels));
		Channels.TriangleChannel.Sequencer = oldout;
		Channels.TriangleChannel.Output = Tables::SeqTable[oldout];
		Channels.TriangleChannel.ControlFlag = oldcontr;
		Channels.NoiseChannel.Timer = Tables::NoiseTable[0];
		Channels.NoiseChannel.Random = 1;
		Channels.NoiseChannel.Shift = 13;
		Channels.DMChannel.Timer = Tables::DMTable[0];
		Channels.Write_4017(InternalData.bMode);
		Channels.Update(Bus);
		memset(&InternalData, 0, sizeof(InternalData));
		memset(&CycleData, 0, sizeof(CycleData));
		CycleData.NextCycle = Tables::StepCycles[0];
		CycleData.CurCycle = Tables::StepCycles[0];
		CycleData.ResetCycle = -3;
		PreprocessedCycles = 0;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;

		switch (Address) {
			case 0x4015: /* Состояние APU */
				/* Отрабатываем прошедшие такты */
				Preprocess();
				Res = Channels.Read_4015();
				/* Если мы не попали на установку флага, сбрасываем его */
				if ((Channels.Mode != SChannels::Mode_4step) ||
					(CycleData.CyclesLeft <= Tables::StepCycles[\
					SChannels::Mode_4step - 1])) {
					Channels.FrameInterrupt = false;
					Bus->GetCPU()->ClearIRQ(FrameIRQ);
				}
				return Res;
			case 0x4016: /* Данные контроллера 1 */
				return Bus->GetFrontend()->GetInput1Frontend()->ReadState();
			case 0x4017: /* Данные контроллера 2 */
				return Bus->GetFrontend()->GetInput2Frontend()->ReadState();
				break;
		}
		return 0x40;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		if (Address == 0x4016) { /* Стробирование контроллеров */
			Bus->GetFrontend()->GetInput1Frontend()->InputSignal(Src);
			Bus->GetFrontend()->GetInput2Frontend()->InputSignal(Src);
			return;
		} else /* Отрабатываем прошедшие такты */
			Preprocess();
		switch (Address) {
			/* Прямоугольный канал 1 */
			case 0x4000: /* Вид и огибающая */
				Channels.SquareChannel1.Write_1(Src);
				break;
			case 0x4001: /* Свип */
				Channels.SquareChannel1.Write_2(Src);
				Channels.SquareChannel1.CalculateSweep(Channels.NextCycle);
				UpdateCycle();
				break;
			case 0x4002: /* Период */
				Channels.SquareChannel1.Write_3(Src);
				Channels.SquareChannel1.CalculateSweep(Channels.NextCycle);
				UpdateCycle();
				break;
			case 0x4003: /* Счетчик, период */
				Channels.SquareChannel1.Write_4(Src);
				Channels.SquareChannel1.CalculateSweep(Channels.NextCycle);
				UpdateCycle();
				break;
			/* Прямоугольный канал 2 */
			case 0x4004:
				Channels.SquareChannel2.Write_1(Src);
				break;
			case 0x4005:
				Channels.SquareChannel2.Write_2(Src);
				Channels.SquareChannel2.CalculateSweep(Channels.NextCycle);
				UpdateCycle();
				break;
			case 0x4006:
				Channels.SquareChannel2.Write_3(Src);
				Channels.SquareChannel2.CalculateSweep(Channels.NextCycle);
				UpdateCycle();
				break;
			case 0x4007:
				Channels.SquareChannel2.Write_4(Src);
				Channels.SquareChannel2.CalculateSweep(Channels.NextCycle);
				UpdateCycle();
				break;
			/* Пилообразный канал */
			case 0x4008:
				Channels.TriangleChannel.Write_1(Src);
				break;
			case 0x400a:
				Channels.TriangleChannel.Write_2(Src, Channels.NextCycle);
				UpdateCycle();
				break;
			case 0x400b:
				Channels.TriangleChannel.Write_3(Src, Channels.NextCycle);
				UpdateCycle();
				break;
			/* Шум */
			case 0x400c:
				Channels.NoiseChannel.Write_1(Src);
				break;
			case 0x400e:
				Channels.NoiseChannel.Write_2(Src, Channels.NextCycle);
				UpdateCycle();
				break;
			case 0x400f:
				Channels.NoiseChannel.Write_3(Src);
				break;
			/* ДМ-канал */
			case 0x4010:
				Channels.DMChannel.Write_1(Src, Channels.NextCycle);
				UpdateCycle();
				if (!Channels.DMChannel.InterruptFlag)
					Bus->GetCPU()->ClearIRQ(DMCIRQ);
				break;
			case 0x4011:
				Channels.DMChannel.Write_2(Src);
				break;
			case 0x4012:
				Channels.DMChannel.Write_3(Src);
				break;
			case 0x4013:
				Channels.DMChannel.Write_4(Src);
				break;
			/* Другое */
			case 0x4014: /* DMA */
				/* DMA */
				Bus->GetCPU()->Pause(512 + (InternalData.Odd & 1));
				InternalData.DMA = 1;
				Preprocess();
				InternalData.DMA = 0;
				Bus->GetPPU()->EnableDMA(Src);
				Bus->GetPPU()->DoDMA();
				return;
			case 0x4015: /* Управление каналами */
				Channels.Write_4015(Src);
				if (!Channels.DMChannel.InterruptFlag)
					Bus->GetCPU()->ClearIRQ(DMCIRQ);
				break;
			case 0x4017: /* Счетчик кадров */
				InternalData.bMode = Src;
				/* Запись в 4017 */
				Channels.Write_4017(Src);
				if (!Channels.FrameInterrupt)
					Bus->GetCPU()->ClearIRQ(FrameIRQ);
				if (Channels.Mode == SChannels::Mode_5step)
					Channels.OddClock();
				else
					Channels.EvenClock();
				CycleData.ResetCycle = CycleData.CyclesLeft + 2 + (InternalData.Odd & 1);
				UpdateCycle();
				CycleData.DebugTimer = 0;
				break;
		}
		Channels.Update(Bus);
	}

	/* Дописать буфер */
	inline void FlushBuffer() {
		Channels.Timer(CycleData.CyclesLeft, Bus);
		Channels.FlushBuffer(Bus);
	}
	/* Текущий такт */
	inline int GetCycles() {
		return CycleData.CurCycle - CycleData.LastCycle + 1;
	}
	/* Обновить текущий такт */
	inline void UpdateCycle() {
		if ((Channels.NextCycle > 0) &&
			(Channels.CurCycle + Channels.NextCycle < CycleData.NextCycle))
			CycleData.CurCycle = Channels.CurCycle + Channels.NextCycle;
		else
			CycleData.CurCycle = CycleData.NextCycle;
		if ((CycleData.ResetCycle >= 0) && (CycleData.CurCycle > CycleData.ResetCycle))
			CycleData.CurCycle = CycleData.ResetCycle;
	}
};

/* Обработка */
template <class _Bus, class _Settings>
void CAPU<_Bus, _Settings>::Process(int Cycles) {
	InternalData.Odd ^= Cycles;
	CycleData.CyclesLeft += Cycles;
	while (CycleData.CyclesLeft > CycleData.CurCycle) {
		CycleData.DebugTimer += CycleData.CurCycle - CycleData.LastCur;
		CycleData.LastCur = CycleData.CurCycle;
		Channels.Timer(CycleData.CurCycle, Bus);
		if (!Channels.DMChannel.NotEmpty && (Channels.DMChannel.LengthCounter > 0)) {
			if (Channels.DMChannel.Address < 0x8000)
				Channels.DMChannel.Address |= 0x8000;
			Channels.DMChannel.SampleBuffer = Bus->ReadCPUMemory(\
				Channels.DMChannel.Address);
			Channels.DMChannel.Address++;
			Channels.DMChannel.LengthCounter--;
			if (Channels.DMChannel.LengthCounter == 0) {
				if (Channels.DMChannel.LoopFlag) {
					Channels.DMChannel.Address =
						Channels.DMChannel.LastAddress;
					Channels.DMChannel.LengthCounter =
						Channels.DMChannel.LastCounter;
				} else if (Channels.DMChannel.InterruptEnabled) {
					Channels.DMChannel.InterruptFlag = true;
					Bus->GetCPU()->GenerateIRQ(GetCycles() * ClockDivider, DMCIRQ);
				}
			}
			Channels.DMChannel.NotEmpty = true;
		}
		if (CycleData.CurCycle == Tables::StepCycles[CycleData.Step]) {
			CycleData.DebugTimer = 0;
			/* Секвенсер */
			switch (Channels.Mode) {
				case SChannels::Mode_4step:
					switch (CycleData.Step) {
						case 0:
						case 2:
							Channels.EvenClock();
							break;
						case 1:
						case 3:
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
			Channels.Update(Bus);
			CycleData.Step++;
			if (CycleData.Step != Channels.Mode)
				CycleData.NextCycle = Tables::StepCycles[CycleData.Step];
			else
				CycleData.NextCycle = Tables::StepCycles[Channels.Mode - 1] + 2;
		}
		if ((CycleData.CurCycle >= Tables::StepCycles[SChannels::Mode_4step - 1]) &&
			(Channels.Mode == SChannels::Mode_4step) && !Channels.InterruptInhibit) {
			Channels.FrameInterrupt = true;
			Bus->GetCPU()->GenerateIRQ(GetCycles() * ClockDivider, FrameIRQ);
			CycleData.NextCycle = std::min(\
				Tables::StepCycles[SChannels::Mode_4step - 1] + 2,
				CycleData.CyclesLeft);
		}
		if (((CycleData.Step == Channels.Mode) &&
			(CycleData.CurCycle == (Tables::StepCycles[Channels.Mode - 1] + 2))) ||
			(CycleData.CurCycle == CycleData.ResetCycle)) {
			CycleData.CyclesLeft -= CycleData.CurCycle;
			CycleData.LastCycle -= CycleData.CurCycle;
			CycleData.LastCur -= CycleData.CurCycle;
			Channels.CurCycle -= CycleData.CurCycle;
			CycleData.Step = 0;
			CycleData.NextCycle = Tables::StepCycles[0];
			if (CycleData.CurCycle == CycleData.ResetCycle)
				CycleData.ResetCycle = -1;
		}
		UpdateCycle();
	}

}

}

#endif
