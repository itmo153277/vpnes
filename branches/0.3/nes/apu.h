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
typedef APUGroup<3>::ID::NoBatteryID ChannelsID;

}

/* APU */
template <class _Bus, class _Tables>
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
		int Step;
		int CurCycle;
		int CyclesLeft;
		int WaitPass;
		int SupressCounter;
	} CycleData;

	/* Аудио буфер */
	VPNES_ABUF *abuf;

	struct SChannels {
		int CurCycle;
		int UpdCycle; /* Число тактов с данным выходом */
		double LastOutput; /* Текущий выход */
		double Avr; /* Среднее */
		double Fixed; /* Коэф */
		double TimeDiff; /* Различие времени */
		double Time; /* Время */
		double Sum; /* Сумма */

		/* Вывод семпла */
		inline void OutputSample(double Sample, VPNES_ABUF *Buf) {
			double Res = Avr + Sample;

			Avr -= Res / 128 / Buf->Freq;
			Buf->PCM[Buf->Pos] = (sint16) (Res * 37267.0);
			Buf->Pos++;
			if (Buf->Pos == Buf->Size) {
				Buf->Pos = 0;
				Buf->Callback(VPNES_PCM_UPDATE, Buf);
			}
		}
		/* Дописать буфер */
		inline void FlushBuffer(VPNES_ABUF *Buf) {
			int i, num;

			Time += UpdCycle * Fixed * Buf->Freq;
			num = (int) Time;
			if (num > 0) {
				Time -= num;
				Sum += LastOutput * (1.0 - TimeDiff);
				OutputSample(Sum, Buf);
				for (i = 1; i < num; i++)
					OutputSample(LastOutput, Buf);
				Sum = LastOutput * Time;
			} else
				Sum += LastOutput * (Time - TimeDiff);
			TimeDiff = Time;
			UpdCycle = 0;
		}

		bool FrameInterrupt; /* Флаг прерывания счетчика кадров */
		bool InterruptInhibit; /* Подавление IRQ */
		enum SeqMode {
			Mode_4step = 4,
			Mode_5step = 5
		} Mode; /* Режим */

		/* Прямоугольный канал */
		struct SSquareChannel {
			int Output; /* Выход на ЦАП */
			bool UseCounter; /* Использовать счетчик */
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
				Timer = (Timer & 0x0700) | Src;
			}
			inline void Write_4(uint8 Src) {
				Timer = (Timer & 0x00ff) | ((Src & 0x07) << 8);
				Start = true;
				if (UseCounter)
					LengthCounter = _Tables::LengthCounterTable[Src >> 3];
			}
			/* Генерация формы */
			inline void Envelope() {
				if (EnvelopeConstant)
					return;
				if (Start) {
					EnvelopeCounter = 15;
					EnvelopeDivider = 0;
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
				if ((TimerCounter >= Timer * 2) && !(TimerCounter & 1)) {
					TimerCounter = 0;
					DutyCycle++;
					if (DutyCycle == 8)
						DutyCycle = 0;
					return true;
				}
				return false;
			}
			inline bool CanOutput() {
				return (LengthCounter > 0) && Valid &&
					_Tables::DutyTable[(DutyMode << 3) + DutyCycle];
			}
		};

		/* Прямоугольный канал 1 */
		struct SSquareChannel1: public SSquareChannel {
			using SSquareChannel::SweepShiftCount;
			using SSquareChannel::SweepNegative;
			using SSquareChannel::Timer;
			using SSquareChannel::TimerTarget;
			using SSquareChannel::Valid;

			/* Вычислить свип */
			inline void CalculateSweep() {
				uint16 Res;

				Res = Timer >> SweepShiftCount;
				if (SweepNegative)
					Res = ~Res;
				TimerTarget = Timer + Res;
				Valid = (Timer > 7) && (SweepNegative || (TimerTarget < 0x0800));
			}
		} SquareChannel1;

		/* Прямоугольный канал 2 */
		struct SSquareChannel2: public SSquareChannel {
			using SSquareChannel::SweepShiftCount;
			using SSquareChannel::SweepNegative;
			using SSquareChannel::Timer;
			using SSquareChannel::TimerTarget;
			using SSquareChannel::Valid;

			/* Вычислить свип */
			inline void CalculateSweep() {
				uint16 Res;

				Res = Timer >> SweepShiftCount;
				if (SweepNegative)
					Res = -Res;
				TimerTarget = Timer + Res;
				Valid = (Timer > 7) && (SweepNegative || (TimerTarget < 0x0800));
			}
		} SquareChannel2;

		/* Пилообразный канал */
		struct STriangleChannel {
			int Output; /* Выход на ЦАП */
			bool UseCounter; /* Использовать счетчик */
			bool HaltFlag; /* Флаг сброса */
			bool ControlFlag; /* Флаг управления счетчиком */
			uint8 LinearCounterReload; /* Период счетчика */
			uint16 Timer; /* Период */
			int TimerCounter; /* Счетчик */
			int LengthCounter; /* Счетчик длины */
			int LinearCounter; /* Счетчик */
			int Sequencer; /* Секвенсер */
			inline void Write_1(uint8 Src) {
				ControlFlag = Src & 0x80; LinearCounterReload = Src & 0x7f;
			}
			inline void Write_2(uint8 Src) {
				Timer = (Timer & 0x0700) | Src;
			}
			inline void Write_3(uint8 Src) {
				Timer = (Timer & 0x00ff) | ((Src & 0x07) << 8);
				if (UseCounter)
					LengthCounter = _Tables::LengthCounterTable[Src >> 3];
				HaltFlag = true;
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
				if (TimerCounter >= Timer) {
					TimerCounter = 0;
					if ((LinearCounter > 0) && (LengthCounter > 0)) {
						Output = _Tables::SeqTable[Sequencer++];
						if (Sequencer > 31)
							Sequencer = 0;
						return true;
					}
				}
				return false;
			}
		} TriangleChannel;

		/* Канал шума */
		struct SNoiseChannel {
			int Output; /* Выход */
			bool UseCounter; /* Использовать счетчик */
			bool Start; /* Начать */
			bool LengthCounterDisable; /* Отключить счетчик */
			bool EnvelopeConstant; /* Константный выход */
			uint8 EnvelopePeriod; /* Период последовательности */
			uint16 Timer; /* Период */
			int Shift; /* Сдвиг */
			int TimerCounter; /* Счетчик */
			int LengthCounter; /* Счетчик */
			int EnvelopeCounter; /* Счетчик огибающей */
			int EnvelopeDivider; /* Делитель огибающей */
			uint16 Random; /* Биты рандома */
			inline void Write_1(uint8 Src) {
				LengthCounterDisable = (Src & 0x20); EnvelopeConstant = (Src & 0x10);
				EnvelopePeriod = (Src & 0x0f);
				if (EnvelopeConstant)
					Output = EnvelopePeriod;
				else
					Output = EnvelopeCounter;
			}
			inline void Write_2(uint8 Src) {
				Timer = _Tables::NoiseTable[Src & 0x0f];
				Shift = ((Src & 0x80) ? 8 : 13);
			}
			inline void Write_3(uint8 Src) {
				Start = true;
				if (UseCounter)
					LengthCounter = _Tables::LengthCounterTable[Src >> 3];
			}
			/* Генерация формы */
			inline void Envelope() {
				if (EnvelopeConstant)
					return;
				if (Start) {
					EnvelopeCounter = 15;
					EnvelopeDivider = 0;
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
			inline bool CanOutput() {
				return (LengthCounter > 0) && !(Random & 0x4000);
			}
		} NoiseChannel;

		/* ДМ-канал */
		struct SDMChannel {
			uint8 Output; /* Выход на ЦАП */
			bool InterruptFlag; /* Флаг прерывания */
			bool InterruptEnabled; /* Прерывание включено */
			bool LoopFlag; /* Флаг цикличности */
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
			inline void Write_1(uint8 Src) {
				InterruptEnabled = Src & 0x80;
				if (!InterruptEnabled)
					InterruptFlag = false;
				LoopFlag = Src & 0x40;
				Timer = _Tables::DMTable[Src & 0x0f];
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
		} DMChannel;

		/* Обновить выход */
		inline void Update(VPNES_ABUF *Buf) {
			int SqOut = 0, TNDOut = 0;
			double NewOutput;

			if (SquareChannel1.CanOutput())
				SqOut += SquareChannel1.Output;
			if (SquareChannel2.CanOutput())
				SqOut += SquareChannel2.Output;
			TNDOut += TriangleChannel.Output * 3;
			if (NoiseChannel.CanOutput())
				TNDOut += NoiseChannel.Output * 2;
			TNDOut += DMChannel.Output;
			NewOutput = _Tables::SquareTable[SqOut] + _Tables::TNDTable[TNDOut];
			if (LastOutput != NewOutput) {
				FlushBuffer(Buf);
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
			SquareChannel1.CalculateSweep();
			SquareChannel2.Sweep();
			SquareChannel2.CalculateSweep();
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
		inline void Do_Timer(int Cycles, VPNES_ABUF *Buf) {
			UpdCycle += Cycles;
			if (SquareChannel1.Do_Timer(Cycles) |
				SquareChannel2.Do_Timer(Cycles) |
				TriangleChannel.Do_Timer(Cycles) |
				NoiseChannel.Do_Timer(Cycles) |
				DMChannel.Do_Timer(Cycles)) {
				Update(Buf);
			}
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
		inline void Write_4017(uint8 Src) { Mode = (Src & 0x80) ? Mode_5step : Mode_4step;
			InterruptInhibit = (Src & 0x40); if (InterruptInhibit) FrameInterrupt = false;
		}
	} Channels;

	/* Буфер */
	VPNES_IBUF ibuf;

	/* Обработка */
	inline void Process(int Cycles) {

		CycleData.CyclesLeft += Cycles;
		while (CycleData.CyclesLeft > 0) {
			CycleData.CyclesLeft--;
			/* Сброс счетчика */
			if (CycleData.SupressCounter >= 0) {
				CycleData.SupressCounter++;
				if (CycleData.SupressCounter > 3) {
					CycleData.CurCycle = 0;
					CycleData.Step = 0;
					CycleData.SupressCounter = -1;
				}
			}
			if (CycleData.CurCycle == _Tables::StepCycles[CycleData.Step]) {
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
//									Bus->GetCPU()->GetIRQPin() = true;
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
				Channels.Update(abuf);
				CycleData.Step++;
				if (CycleData.Step == Channels.Mode) /* Сбросить счетчик через 2 такта */
					CycleData.SupressCounter = 2;
			}
			Channels.Do_Timer(1, abuf);
			if (!Channels.DMChannel.NotEmpty && (Channels.DMChannel.LengthCounter > 0) &&
				(CycleData.WaitPass == 0)) {
				CycleData.WaitPass = -1;
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
//						Bus->GetCPU()->GetIRQPin() = true;
					}
				}
				Channels.DMChannel.NotEmpty = true;
				CycleData.WaitPass = 0;
			}
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
		memset(&Channels.TriangleChannel, 0, sizeof(Channels.TriangleChannel));
		Channels.TriangleChannel.Output = _Tables::SeqTable[0];
	}
	inline ~CAPU() {}

	/* Предобработать такты */
	inline void Preprocess() {
		/* Обрабатываем текущие такты */
		Process(Bus->GetClock()->GetPreCPUCycles() - PreprocessedCycles);
		PreprocessedCycles = Bus->GetClock()->GetPreCPUCycles();
	}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		/* Обрабатываем необработанные такты */
		Process(Cycles - PreprocessedCycles);
		PreprocessedCycles = 0;
	}

	/* Сброс */
	inline void Reset() {
		typename SChannels::STriangleChannel temp;

		memcpy(&temp, &Channels.TriangleChannel,
			sizeof(typename SChannels::STriangleChannel));
		memset(&Channels, 0, sizeof(Channels));
		Channels.Fixed = Bus->GetClock()->GetFix() * _Bus::CPUClass::ClockDivider;
		Channels.Write_4017(InternalData.bMode);
		Channels.NoiseChannel.Random = 1;
		Channels.NoiseChannel.Shift = 13;
		memcpy(&Channels.TriangleChannel, &temp,
			sizeof(typename SChannels::STriangleChannel));
		Channels.Update(abuf);
		memset(&InternalData, 0, sizeof(InternalData));
		memset(&CycleData, 0, sizeof(CycleData));
		CycleData.CurCycle = -1;
		CycleData.SupressCounter = -1;
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
				if ((Channels.Mode != SChannels::Mode_4step) || (CycleData.CurCycle !=
					(_Tables::StepCycles[SChannels::Mode_4step - 1] + 2))) {
					Channels.FrameInterrupt = false;
//					Bus->GetCPU()->GetIRQPin() = Channels.DMChannel.InterruptFlag;
				}
				return Res;
			case 0x4016: /* Данные контроллера 1 */
				if (InternalData.StrobeCounter_A < 4)
					return 0x40 | ibuf[InternalData.StrobeCounter_A++];
				if (InternalData.StrobeCounter_A < 8) {
					Res = 0x40 | (ibuf[InternalData.StrobeCounter_A] &
						~ibuf[_Tables::ButtonsRemap[\
						InternalData.StrobeCounter_A & 0x03]]);
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
		if (Address == 0x4016) { /* Стробирование контроллеров */
			InternalData.StrobeCounter_A = 0;
			InternalData.StrobeCounter_B = 0;
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
				Channels.SquareChannel1.CalculateSweep();
				break;
			case 0x4002: /* Период */
				Channels.SquareChannel1.Write_3(Src);
				Channels.SquareChannel1.CalculateSweep();
				break;
			case 0x4003: /* Счетчик, период */
				Channels.SquareChannel1.Write_4(Src);
				Channels.SquareChannel1.CalculateSweep();
				break;
			/* Прямоугольный канал 2 */
			case 0x4004:
				Channels.SquareChannel2.Write_1(Src);
				break;
			case 0x4005:
				Channels.SquareChannel2.Write_2(Src);
				Channels.SquareChannel2.CalculateSweep();
				break;
			case 0x4006:
				Channels.SquareChannel2.Write_3(Src);
				Channels.SquareChannel2.CalculateSweep();
				break;
			case 0x4007:
				Channels.SquareChannel2.Write_4(Src);
				Channels.SquareChannel2.CalculateSweep();
				break;
			/* Пилообразный канал */
			case 0x4008:
				Channels.TriangleChannel.Write_1(Src);
				break;
			case 0x400a:
				Channels.TriangleChannel.Write_2(Src);
				break;
			case 0x400b:
				Channels.TriangleChannel.Write_3(Src);
				break;
			/* Шум */
			case 0x400c:
				Channels.NoiseChannel.Write_1(Src);
				break;
			case 0x400e:
				Channels.NoiseChannel.Write_2(Src);
				break;
			case 0x400f:
				Channels.NoiseChannel.Write_3(Src);
				break;
			/* ДМ-канал */
			case 0x4010:
				Channels.DMChannel.Write_1(Src);
//				if (!Channels.DMChannel.InterruptFlag)
//					Bus->GetCPU()->GetIRQPin() = Channels.FrameInterrupt;
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
				return;
			case 0x4015: /* Управление каналами */
				Channels.Write_4015(Src);
//				if (!Channels.DMChannel.InterruptFlag)
//					Bus->GetCPU()->GetIRQPin() = Channels.FrameInterrupt;
				break;
			case 0x4017: /* Счетчик кадров */
				InternalData.bMode = Src;
				/* Запись в 4017 */
				Channels.Write_4017(Src);
//				if (!Channels.FrameInterrupt)
//					Bus->GetCPU()->GetIRQPin() = Channels.DMChannel.InterruptFlag;
				if (Channels.Mode == SChannels::Mode_5step)
					Channels.OddClock();
				else
					Channels.EvenClock();
				CycleData.SupressCounter = CycleData.CurCycle & 1;
				break;
		}
		Channels.Update(abuf);
	}

	/* Буфер */
	inline VPNES_ABUF *&GetABuf() { return abuf; }
	inline VPNES_IBUF &GetIBuf() { return ibuf; }
	/* Дописать буфер */
	inline void FlushBuffer() {
		Channels.FlushBuffer(abuf);
	}
};

namespace apu {

/* Таблицы для NTSC */
struct NTSC_Tables {
	/* Число тактов на каждом шаге */
	static const int StepCycles[6];
	/* Перекрываемые кнопки контроллера */
	static const int ButtonsRemap[4];
	/* Таблица счетчика */
	static const int LengthCounterTable[32];
	/* Таблица формы */
	static const bool DutyTable[32];
	/* Таблица пилообразной формы */
	static const int SeqTable[32];
	/* Таблица длин для канала шума */
	static const int NoiseTable[16];
	/* Таблица длин для ДМ-канала */
	static const int DMTable[16];
	/* Таблица выхода прямоугольных каналов */
	static const double SquareTable[31];
	/* Таблица выхода остальных каналов */
	static const double TNDTable[203];
};

/* Число тактов для каждого шага */
const int NTSC_Tables::StepCycles[6] = {7456, 14912, 22370, 29828, 37280, 0};

/* Перекрываемые кнопки контроллера */
const int NTSC_Tables::ButtonsRemap[4] = {
	VPNES_INPUT_DOWN, VPNES_INPUT_UP,
	VPNES_INPUT_RIGHT, VPNES_INPUT_LEFT
};

/* Таблица счетчика */
const int NTSC_Tables::LengthCounterTable[32] = {
	0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a,
	0x0e, 0x0c, 0x1a, 0x0e, 0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e
};

/* Таблица формы */
const bool NTSC_Tables::DutyTable[32] = {
	false, true,  false, false, false, false, false, false, false, true,  true,  false,
	false, false, false, false, false, true,  true,  true,  true,  false, false, false,
	true,  false, false, true,  true,  true,  true,  true
};

/* Таблица пилообразной формы */
const int NTSC_Tables::SeqTable[32] = {
	15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

/* Таблица длин для канала шума */
const int NTSC_Tables::NoiseTable[16] = {
	0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0060, 0x0080, 0x00a0,
	0x00ca, 0x00fe, 0x017c, 0x01fc, 0x02fa, 0x03f8, 0x07f2, 0x0fe4
};

/* Таблица длин для ДМ-канала */
const int NTSC_Tables::DMTable[16] = {
	428, 380, 340, 320, 286, 254, 226, 214,
	190, 160, 142, 128, 106,  84,  72,  54
};

/* Таблица выхода прямоугольных сигналов */
const double NTSC_Tables::SquareTable[31] = {
	0.0000000, 0.0116091, 0.0229395, 0.0340009, 0.0448030, 0.0553547,
	0.0656645, 0.0757408, 0.0855914, 0.0952237, 0.1046450, 0.1138620,
	0.1228820, 0.1317100, 0.1403530, 0.1488160, 0.1571050, 0.1652260,
	0.1731830, 0.1809810, 0.1886260, 0.1961200, 0.2034700, 0.2106790,
	0.2177510, 0.2246890, 0.2314990, 0.2381820, 0.2447440, 0.2511860,
	0.2575130
};

/* Таблица выхода остальных каналов */
const double NTSC_Tables::TNDTable[203] = {
	0.00000000, 0.00669982, 0.01334500, 0.01993630, 0.02647420, 0.03295940,
	0.03939270, 0.04577450, 0.05210550, 0.05838640, 0.06461760, 0.07079990,
	0.07693370, 0.08301960, 0.08905830, 0.09505010, 0.10099600, 0.10689600,
	0.11275100, 0.11856100, 0.12432700, 0.13004900, 0.13572800, 0.14136500,
	0.14695900, 0.15251200, 0.15802400, 0.16349400, 0.16892500, 0.17431500,
	0.17966600, 0.18497800, 0.19025200, 0.19548700, 0.20068400, 0.20584500,
	0.21096800, 0.21605400, 0.22110500, 0.22612000, 0.23109900, 0.23604300,
	0.24095300, 0.24582800, 0.25066900, 0.25547700, 0.26025200, 0.26499300,
	0.26970200, 0.27437900, 0.27902400, 0.28363800, 0.28822000, 0.29277100,
	0.29729200, 0.30178200, 0.30624200, 0.31067300, 0.31507400, 0.31944600,
	0.32378900, 0.32810400, 0.33239000, 0.33664900, 0.34087900, 0.34508300,
	0.34925900, 0.35340800, 0.35753000, 0.36162600, 0.36569600, 0.36974000,
	0.37375900, 0.37775200, 0.38172000, 0.38566200, 0.38958100, 0.39347400,
	0.39734400, 0.40118900, 0.40501100, 0.40880900, 0.41258400, 0.41633500,
	0.42006400, 0.42377000, 0.42745400, 0.43111500, 0.43475400, 0.43837100,
	0.44196600, 0.44554000, 0.44909300, 0.45262500, 0.45613500, 0.45962500,
	0.46309400, 0.46654300, 0.46997200, 0.47338000, 0.47676900, 0.48013800,
	0.48348800, 0.48681800, 0.49012900, 0.49342100, 0.49669400, 0.49994800,
	0.50318400, 0.50640200, 0.50960100, 0.51278200, 0.51594600, 0.51909100,
	0.52221900, 0.52533000, 0.52842300, 0.53149900, 0.53455800, 0.53760100,
	0.54062600, 0.54363500, 0.54662700, 0.54960300, 0.55256300, 0.55550700,
	0.55843400, 0.56134600, 0.56424300, 0.56712300, 0.56998800, 0.57283800,
	0.57567300, 0.57849300, 0.58129800, 0.58408800, 0.58686300, 0.58962300,
	0.59237000, 0.59510100, 0.59781900, 0.60052200, 0.60321200, 0.60588700,
	0.60854900, 0.61119700, 0.61383100, 0.61645200, 0.61905900, 0.62165300,
	0.62423400, 0.62680200, 0.62935700, 0.63189900, 0.63442800, 0.63694400,
	0.63944800, 0.64193900, 0.64441800, 0.64688500, 0.64933900, 0.65178100,
	0.65421200, 0.65663000, 0.65903600, 0.66143100, 0.66381300, 0.66618500,
	0.66854400, 0.67089300, 0.67322900, 0.67555500, 0.67786900, 0.68017300,
	0.68246500, 0.68474600, 0.68701700, 0.68927600, 0.69152500, 0.69376300,
	0.69599100, 0.69820800, 0.70041500, 0.70261100, 0.70479700, 0.70697300,
	0.70913900, 0.71129400, 0.71344000, 0.71557600, 0.71770200, 0.71981800,
	0.72192400, 0.72402100, 0.72610800, 0.72818600, 0.73025400, 0.73231300,
	0.73436200, 0.73640200, 0.73843300, 0.74045500, 0.74246800
};

}

/* Стандартный АПУ */
template <class _Tables>
struct StdAPU {
	template <class _Bus>
	class APU: public CAPU<_Bus, _Tables> {
	public:
		inline explicit APU(_Bus *pBus): CAPU<_Bus, _Tables>(pBus) {}
		inline ~APU() {}
	};
};

}

#endif
