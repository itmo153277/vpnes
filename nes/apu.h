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
#include "tables.h"
#include "manager.h"
#include "clock.h"
#include "bus.h"

namespace vpnes {

namespace APUID {

typedef APUGroup<1>::ID::NoBatteryID CycleDataID;
typedef APUGroup<2>::ID::NoBatteryID EventDataID;
typedef APUGroup<3>::ID::NoBatteryID EventTimeID;
typedef APUGroup<4>::ID::NoBatteryID ChannelsID;

}

/* APU */
template <class _Bus, class _Settings>
class CAPU {
public:
	/* Делитель частоты */
	enum { ClockDivider = _Bus::CPUClass::ClockDivider };
	/* Таблицы APU */
	typedef typename _Settings::APUTables Tables;
private:
	/* Шина */
	_Bus *Bus;

	/* Данные о тактах */
	struct SCycleData: public ManagerID<APUID::CycleDataID> {
		int OddClock;
		int InternalClock;
		int Async;
		int NextEvent;
		int IRQOffset;
		int FrameCycle;
		int Step;
	} CycleData;

	/* События */
	enum {
		EVENT_APU_TICK = 0,
		EVENT_APU_FRAME_IRQ,
		EVENT_APU_DMC_IRQ,
		EVENT_APU_DMC_DMA,
		MAX_EVENTS
	};
	/* Данные о событиях */
	SEventData LocalEvents[MAX_EVENTS];
	/* Указатели на события */
	SEvent *EventChain[MAX_EVENTS];
	/* Периоды событий */
	int EventTime[MAX_EVENTS];

	/* Длина буфера APU */
	enum { MAX_BUF = 1024 };

	/* APU буфер */
	struct SAPUUnit {
		struct SSample {
			int Sample;
			int Length;
		} Buffer[MAX_BUF];
		int Pos;
		int PlayPos;
		int InternalClock;

		/* Сбросить часы */
		inline void ResetClock(int Time) {
			InternalClock -= Time;
		}
		/* Проиграть семпл */
		inline int PlaySample(int Length) {
			int Out;

			Out = Buffer[PlayPos].Sample;
			Buffer[PlayPos].Length -= Length;
			if (Buffer[PlayPos].Length == 0) {
				PlayPos++;
				if (PlayPos == MAX_BUF) {
					PlayPos = 0;
					Pos = 0;
				}
			}
			return Out;
		}
		/* Минимальная длина */
		inline void MinimizeLength(int &Length) {
			if (Buffer[PlayPos].Length < Length)
				Length = Buffer[PlayPos].Length;
		}
	};

	/* Прямоугольный сигнал */
	struct SPulseChannel: public SAPUUnit {
		using SAPUUnit::Pos;
		using SAPUUnit::Buffer;
		using SAPUUnit::InternalClock;
		int DutyCycle; /* Секвенсер */
		uint8 DutyMode; /* Режим секвенсера */
		bool EnvelopeStart; /* Сброс счетчика */
		int EnvelopeLoop; /* Повторение */
		int ConstantVolume; /* Постоянная громкость */
		int EnvelopePeriod; /* Период изменений громкости */
		int EnvelopeCounter; /* Счетчик выхода */
		int SweepEnabled; /* Свип вылючен */
		int SweepPeriod; /* Период свипа */
		int NegateFlag; /* Отрицаиельное изменение */
		int ShiftCount; /* Число сдвигов */
		bool SweepReload; /* Флаг сброса свипа */
		int TimerPeriod; /* Период */
		int SweepedTimer; /* Рассчитанный период */
		bool TimerValid; /* Периоды корректные */
		bool UseCounter; /* Подавление изменения счетчикка длины */
		int LengthCounter; /* Счетчик длины */
		int Timer; /* Таймер */
		int EnvelopeTimer; /* Счетчик для энвелопа */
		int SweepTimer; /* Счетчик для свипа */

		/* Запись 0x4000 / 0x4004 */
		inline void Write_1(uint8 Src) {
			DutyMode = Src >> 6;
			EnvelopeLoop = Src & 0x20;
			ConstantVolume = Src & 0x10;
			EnvelopePeriod = Src & 0x04;
		}
		/* Запись 0x4001 / 0x4005 */
		inline void Write_2(uint8 Src) {
			SweepEnabled = Src & 0x80;
			SweepPeriod = (Src >> 4) & 0x07;
			NegateFlag = Src & 0x08;
			ShiftCount = Src & 0x07;
			SweepReload = true;
		}
		/* Запись 0x4002 / 0x4006 */
		inline void Write_3(uint8 Src) {
			TimerPeriod = (TimerPeriod & 0x0700) | Src;
		}
		/* Запись 0x4003 / 0x4007 */
		inline void Write_4(uint8 Src) {
			TimerPeriod = (TimerPeriod & 0x00ff) | ((Src & 0x07) << 8);
			EnvelopeStart = true;
			DutyCycle = 0;
			if (UseCounter)
				LengthCounter = Tables::LengthCounterTable[Src >> 3];
		}

		/* Генерация формы */
		inline void Envelope() {
			if (EnvelopeStart) {
				EnvelopeCounter = 15;
				EnvelopeTimer = EnvelopePeriod;
				EnvelopeStart = false;
			} else {
				if (EnvelopeTimer <= 0) {
					EnvelopeTimer = EnvelopePeriod;
					if (EnvelopeCounter <= 0) {
						if (EnvelopeLoop)
							EnvelopeCounter = 15;
					} else
						EnvelopeCounter--;
				} else
					EnvelopeTimer--;
			}
		}
		/* Счетчик длины */
		inline void ClockLengthCounter() {
			if ((!EnvelopeLoop) && (LengthCounter > 0))
				LengthCounter--;
		}
		/* Свип */
		inline void Sweep() {
			if (SweepTimer <= 0) {
				if (SweepEnabled && TimerValid && (ShiftCount > 0))
					TimerPeriod = SweepedTimer & 0x07ff;
				SweepTimer = SweepPeriod;
			} else
				SweepTimer--;
			if (SweepReload) {
				SweepReload = false;
				SweepTimer = SweepPeriod;
			}
		}
		/* Сброс */
		inline void Reset() {
		}
		/* Выход */
		inline int GetOutput() {
			if (Tables::DutyTable[(DutyMode << 3) + DutyCycle]) {
				if (ConstantVolume)
					return EnvelopePeriod;
				else
					return EnvelopeCounter;
			} else
				return 0;
		}
		/* Заполнить буфер */
		int FillBuffer(int Count) {
			int Len = (Count + (Count & 1)) >> 1;
			int RealCount = 0;

			if (Len == 0)
				return 0;
			if (!TimerValid || (LengthCounter <= 0)) {
				Buffer[Pos].Sample = 0;
				Buffer[Pos++].Length = Len << 1;
				DutyCycle = (DutyCycle + (TimerPeriod - Timer + Count) /
					(TimerPeriod + 1)) & 7;
				Timer = TimerPeriod - ((Len + TimerPeriod - Timer) % (TimerPeriod + 1));
				return Len << 1;
			}
			if (Len < (Timer + 1)) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = Len << 1;
				Timer -= Len;
				return Len << 1;
			}
			if (Timer != TimerPeriod) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = (Timer + 1) << 1;
				Len -= Timer + 1;
				RealCount += (Timer + 1) << 1;
				Timer = TimerPeriod;
				DutyCycle++;
				DutyCycle &= 7;
			}
			while ((Pos < MAX_BUF) && (Len >= (TimerPeriod + 1))) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = (TimerPeriod + 1) << 1;
				Len -= TimerPeriod + 1;
				DutyCycle++;
				DutyCycle &= 7;
				RealCount += (TimerPeriod + 1) << 1;
			}
			if ((Pos < MAX_BUF) && (Len > 0)) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = Len << 1;
				Timer = TimerPeriod - Len;
				RealCount += Len << 1;
			}
			return RealCount;
		}

		/* Обновить буфер */
		inline int UpdateBuffer(int Time) {
			if (Time <= InternalClock)
				return Time;
			if (Pos == MAX_BUF)
				return InternalClock;
			InternalClock += FillBuffer(Time - InternalClock);
			return std::min(Time, InternalClock);
		}
		inline void UpdateBuffer(CAPU &APU, int Time) {
			while (InternalClock < Time) {
				if (Pos == MAX_BUF)
					APU.Channels.FlushBuffer(APU, InternalClock);
				InternalClock += FillBuffer(Time - InternalClock);
			}
		}
	};

	/* Прямоугольный канал 1 */
	struct SPulseChannel1: public SPulseChannel {
		using SPulseChannel::ShiftCount;
		using SPulseChannel::NegateFlag;
		using SPulseChannel::TimerPeriod;
		using SPulseChannel::SweepedTimer;
		using SPulseChannel::TimerValid;

		/* Обсчитать свип */
		void CalculateSweep() {
			uint16 Res;

			Res = TimerPeriod >> ShiftCount;
			if (NegateFlag)
				Res = ~Res;
			SweepedTimer = TimerPeriod + Res;
			TimerValid = (TimerPeriod > 7) && (NegateFlag || (SweepedTimer < 0x800));
		}
	};

	/* Прямоугольный канал 2 */
	struct SPulseChannel2: public SPulseChannel {
		using SPulseChannel::ShiftCount;
		using SPulseChannel::NegateFlag;
		using SPulseChannel::TimerPeriod;
		using SPulseChannel::SweepedTimer;
		using SPulseChannel::TimerValid;

		/* Обсчитать свип */
		void CalculateSweep() {
			uint16 Res;

			Res = TimerPeriod >> ShiftCount;
			if (NegateFlag)
				Res = -Res;
			SweepedTimer = TimerPeriod + Res;
			TimerValid = (TimerPeriod > 7) && (NegateFlag || (SweepedTimer < 0x800));
		}
	};

	/* Треугольный канал */
	struct STriangleChannel: public SAPUUnit {
		using SAPUUnit::Pos;
		using SAPUUnit::Buffer;
		using SAPUUnit::InternalClock;
		int SequencePos; /* Номер в последовательности */
		int ControlFlag; /* Флаг управления счетчиком */
		int CounterReloadValue; /* Значение для сброса счетчика */
		bool CounterReload; /* Флаг сброса счетчика */
		int LinearCounter; /* Линейный счетчик */
		int TimerPeriod; /* Период */
		int Timer; /* Счетчик */
		bool UseCounter; /* Флаг подавления счетчика длины */
		int LengthCounter; /* Счетчик длины */

		/* Запись 0x4008 */
		inline void Write_1(uint8 Src) {
			ControlFlag = Src & 0x80;
			CounterReload = Src & 0x7f;
		}
		/* Запись 0x400a */
		inline void Write_2(uint8 Src) {
			TimerPeriod = (TimerPeriod & 0x0700) | Src;
		}
		/* Запсиь 0x400b */
		inline void Write_3(uint8 Src) {
			TimerPeriod = (TimerPeriod & 0x00ff) | ((Src & 0x07) << 8);
			CounterReload = true;
			if (UseCounter)
				LengthCounter = Tables::LengthCounterTable[Src >> 3];
		}

		/* Линейный счетчик */
		inline void ClockLinearCounter() {
			if (CounterReload)
				LinearCounter = CounterReloadValue;
			else if (LinearCounter > 0)
				LinearCounter--;
			if (!ControlFlag)
				CounterReload = false;
		}
		/* Счетчик длины */
		inline void ClockLengthCounter() {
			if ((!ControlFlag) && (LengthCounter > 0))
				LengthCounter--;
		}
		/* Сброс */
		inline void Reset() {
		}
		/* Заполнить буфер */
		int FillBuffer(int Count) {
			int RealCount = 0;
			int Len = Count;

			if (Count == 0)
				return Count;
			if ((LinearCounter <= 0) || (LengthCounter <= 0) || (TimerPeriod < 2)) {
				if (TimerPeriod < 2) {
					Buffer[Pos].Sample = -1;
					SequencePos = (SequencePos + (TimerPeriod - Timer + Count) /
						(TimerPeriod + 1)) & 31;
				} else
					Buffer[Pos].Sample = Tables::SeqTable[SequencePos];
				Buffer[Pos++].Length = Count;
				Timer = TimerPeriod - ((Count + TimerPeriod - Timer) % (TimerPeriod + 1));
				return Count;
			}
			if (Len < (Timer + 1)) {
				Buffer[Pos].Sample = Tables::SeqTable[SequencePos];
				Buffer[Pos++].Length = Len;
				Timer -= Len;
				return Count;
			}
			if (Timer != TimerPeriod) {
				Buffer[Pos].Sample = Tables::SeqTable[SequencePos];
				Buffer[Pos++].Length = Timer + 1;
				Len -= Timer + 1;
				RealCount += Timer + 1;
				Timer = TimerPeriod;
				SequencePos++;
				SequencePos &= 31;
			}
			while ((Pos < MAX_BUF) && (Len >= (TimerPeriod + 1))) {
				Buffer[Pos].Sample = Tables::SeqTable[SequencePos];
				Buffer[Pos++].Length = TimerPeriod + 1;
				Len -= TimerPeriod + 1;
				SequencePos++;
				SequencePos &= 31;
				RealCount += TimerPeriod + 1;
			}
			if ((Pos < MAX_BUF) && (Len > 0)) {
				Buffer[Pos].Sample = Tables::SeqTable[SequencePos];
				Buffer[Pos++].Length = Len;
				Timer = TimerPeriod - Len;
				RealCount += Len;
			}
			return RealCount;
		}

		/* Обновить буфер */
		inline int UpdateBuffer(int Time) {
			if (Time <= InternalClock)
				return Time;
			InternalClock += FillBuffer(Time - InternalClock);
			return InternalClock;
		}
		inline void UpdateBuffer(CAPU &APU, int Time) {
			while (InternalClock < Time) {
				InternalClock += FillBuffer(Time - InternalClock);
				if (Pos == MAX_BUF)
					APU.Channels.FlushBuffer(APU, InternalClock);
			}
		}
	};

	/* Канал шума */
	struct SNoiseChannel: public SAPUUnit {
		using SAPUUnit::Pos;
		using SAPUUnit::Buffer;
		using SAPUUnit::InternalClock;
		bool EnvelopeStart; /* Сброс счетчика */
		int EnvelopeLoop; /* Повторение */
		int ConstantVolume; /* Постоянная громкость */
		int EnvelopePeriod; /* Период изменений громкости */
		int EnvelopeCounter; /* Счетчик выхода */
		int ShiftCount; /* Бит обратной связи */
		int TimerPeriod; /* Период */
		bool UseCounter; /* Подавление изменения счетчикка длины */
		int LengthCounter; /* Счетчик длины */
		int Timer; /* Таймер */
		int EnvelopeTimer; /* Счетчик для энвелопа */
		uint16 Random; /* Биты рандома */

		/* Запсиь в 0x400c */
		inline void Write_1(uint8 Src) {
			EnvelopeLoop = Src & 0x20;
			ConstantVolume = Src & 0x10;
			EnvelopePeriod = Src & 0x04;
		}
		/* Запись в 0x400e */
		inline void Write_2(uint8 Src) {
			TimerPeriod = Tables::NoiseTable[Src & 0x0f];
			ShiftCount = (Src & 0x80) ? 8 : 13;
		}
		/* Запись в 0x400f */
		inline void Write_3(uint8 Src) {
			EnvelopeStart = true;
			if (UseCounter)
				LengthCounter = Tables::LengthCounterTable[Src >> 3];
		}

		/* Генерация формы */
		inline void Envelope() {
			if (EnvelopeStart) {
				EnvelopeCounter = 15;
				EnvelopeTimer = EnvelopePeriod;
				EnvelopeStart = false;
			} else {
				if (EnvelopeTimer <= 0) {
					EnvelopeTimer = EnvelopePeriod;
					if (EnvelopeCounter <= 0) {
						if (EnvelopeLoop)
							EnvelopeCounter = 15;
					} else
						EnvelopeCounter--;
				} else
					EnvelopeTimer--;
			}
		}
		/* Счетчик длины */
		inline void ClockLengthCounter() {
			if ((!EnvelopeLoop) && (LengthCounter > 0))
				LengthCounter--;
		}
		/* Сброс */
		inline void Reset() {
			Random = 0x0001;
			ShiftCount = 13;
		}
		/* Выход */
		inline int GetOutput() {
			if (!(Random & 0x4000)) {
				if (ConstantVolume)
					return EnvelopePeriod;
				else
					return EnvelopeCounter;
			} else
				return 0;
		}
		/* Заполнить буфер */
		int FillBuffer(int Count) {
			int RealCount = 0;
			int Len = Count;

			if (Count == 0)
				return Count;
			if (LengthCounter <= 0) {
				int ChCount = (TimerPeriod - Timer + Count) /
					(TimerPeriod + 1);

				for (; ChCount > 0; ChCount--)
					Random = (((Random >> 14) ^ (Random >> ShiftCount)) & 0x01) |
						(Random << 1);
				Buffer[Pos].Sample = 0;
				Buffer[Pos++].Length = Count;
				Timer = TimerPeriod - ((Count + TimerPeriod - Timer) % (TimerPeriod + 1));
				return Count;
			}
			if (Len < (Timer + 1)) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = Len;
				Timer -= Len;
				return Count;
			}
			if (Timer != TimerPeriod) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = Timer + 1;
				Len -= Timer + 1;
				RealCount += Timer + 1;
				Timer = TimerPeriod;
				Random = (((Random >> 14) ^ (Random >> ShiftCount)) & 0x01) | (Random << 1);
			}
			while ((Pos < MAX_BUF) && (Len >= (TimerPeriod + 1))) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = TimerPeriod + 1;
				Len -= TimerPeriod + 1;
				Random = (((Random >> 14) ^ (Random >> ShiftCount)) & 0x01) | (Random << 1);
				RealCount += TimerPeriod + 1;
			}
			if ((Pos < MAX_BUF) && (Len > 0)) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = Len;
				Timer = TimerPeriod - Len;
				RealCount += Len;
			}
			return RealCount;
		}

		/* Обновить буфер */
		inline int UpdateBuffer(int Time) {
			if (Time <= InternalClock)
				return Time;
			InternalClock += FillBuffer(Time - InternalClock);
			return InternalClock;
		}
		inline void UpdateBuffer(CAPU &APU, int Time) {
			while (InternalClock < Time) {
				InternalClock += FillBuffer(Time - InternalClock);
				if (Pos == MAX_BUF)
					APU.Channels.FlushBuffer(APU, InternalClock);
			}
		}
	};

	/* Все каналы */
	struct SChannels: public ManagerID<APUID::CycleDataID> {
		/* Все каналы */
		SPulseChannel1 PulseChannel1;
		SPulseChannel2 PulseChannel2;
		STriangleChannel TriangleChannel;
		SNoiseChannel NoiseChannel;
//		SDMChannel DMChannel;
		int PlayTime; /* Текущее время проигрывания */
		int InternalClock; /* Текущее время */
		bool FrameInterrupt; /* Прерывание счетчика кадров */
		int InterruptInhibit; /* Подавление IRQ */
		enum SeqMode {
			Mode_4step = 4,
			Mode_5step = 5
		} Mode; /* Режим последовательностей */
		uint8 Buf4017; /* Сохраненный регистр 0x4017 */

		/* Запись 0x4015 */
		inline void Write4015(uint8 Src) {
			PulseChannel1.UseCounter = Src & 0x01;
			PulseChannel2.UseCounter = Src & 0x02;
			TriangleChannel.UseCounter = Src & 0x04;
			NoiseChannel.UseCounter = Src & 0x08;
//			if (!(Src & 0x10))
//				DMChannel.LengthCounter = 0;
//			else if (DMChannel.LengthCounter == 0)
//				DMChannel.LengthCounter = DMChannel.ReloadCounter;
			if (!PulseChannel1.UseCounter)
				PulseChannel1.LengthCounter = 0;
			if (!PulseChannel2.UseCounter)
				PulseChannel2.LengthCounter = 0;
			if (!TriangleChannel.UseCounter)
				TriangleChannel.LengthCounter = 0;
			if (!NoiseChannel.UseCounter)
				NoiseChannel.LengthCounter = 0;
		}
		/* Чтение 0x4015 */
		inline uint8 Read4015() {
			uint8 Res = 0;

			if (PulseChannel1.LengthCounter > 0)
				Res |= 0x01;
			if (PulseChannel2.LengthCounter > 0)
				Res |= 0x02;
			if (TriangleChannel.LengthCounter > 0)
				Res |= 0x04;
			if (NoiseChannel.LengthCounter > 0)
				Res |= 0x08;
//			if (DMChannel.LengthCounter > 0)
//				Res |= 0x10;
			if (FrameInterrupt)
				Res |= 0x40;
//			if (DMChannel.InterruptFlag)
//				Res |= 0x80;
			return Res;
		}
		/* Запись 0x4017 */
		inline void Write4017(uint8 Src) {
			Buf4017 = Src;
			Mode = (Src & 0x80) ? Mode_5step : Mode_4step;
			InterruptInhibit = (Src & 0x40);
			if (InterruptInhibit)
				FrameInterrupt = false;
		}

		/* Вывести звук */
		void MuxChannels(CAPU &APU, int Clocks) {
#if !defined(VPNES_PRCISE_MUX)
			int Length, PulseOut, TOut, NDOut = 0;
			double Sample;

			while (PlayTime < Clocks) {
				Length = Clocks - PlayTime;
				PulseChannel1.MinimizeLength(Length);
				PulseChannel2.MinimizeLength(Length);
				TriangleChannel.MinimizeLength(Length);
				NoiseChannel.MinimizeLength(Length);
//				DMChannel.MinimizeLength(Length);
				PlayTime += Length;
				PulseOut = PulseChannel1.PlaySample(Length) +
					PulseChannel2.PlaySample(Length);
				TOut = TriangleChannel.PlaySample(Length);
				NDOut = NoiseChannel.PlaySample(Length) * 2;// +
//					DMChannel.PlaySample(Length);
				if (TOut < 0)
					Sample = (apu::TNDTable[NDOut + 21] + apu::TNDTable[NDOut + 24]) / 2;
				else
					Sample = apu::TNDTable[NDOut + TOut];
				Sample += apu::SquareTable[PulseOut];
				APU.Bus->GetFrontend()->GetAudioFrontend()->PushSample(Sample, Length *
					ClockDivider * _Settings::GetFreq());
			}
#else
#endif
		}
		/* Обновить буфер */
		void FlushBuffer(CAPU &APU, int Clocks) {
			int RealTime;

			do {
				RealTime = PulseChannel1.UpdateBuffer(Clocks);
				RealTime = PulseChannel2.UpdateBuffer(RealTime);
				RealTime = TriangleChannel.UpdateBuffer(RealTime);
				RealTime = NoiseChannel.UpdateBuffer(RealTime);
//				RealTime = DMChannel.UpdateBuffer(RealTime);
				MuxChannels(APU, RealTime);
			} while (RealTime < Clocks);
		}
		/* Сбросить время */
		inline void ResetClock() {
			PulseChannel1.ResetClock(InternalClock);
			PulseChannel2.ResetClock(InternalClock);
			TriangleChannel.ResetClock(InternalClock);
			NoiseChannel.ResetClock(InternalClock);
//			DMChannel.ResetClock(InternalClock);
			PlayTime = 0;
			InternalClock = 0;
		}
		/* Обновить буфер до конца */
		inline void FlushAll(CAPU &APU) {
			FlushBuffer(APU, InternalClock);
		}
		/* Сброс */
		void Reset() {
			PulseChannel1.Reset();
			PulseChannel2.Reset();
			TriangleChannel.Reset();
			NoiseChannel.Reset();
//			DMChannel.Reset();
			ResetClock();
			Write4015(0);
			Write4017(Buf4017);
		}
		/* Общий вызов */
		inline void Envelope() {
			PulseChannel1.Envelope();
			PulseChannel2.Envelope();
			NoiseChannel.Envelope();
		}
		inline void LengthCounter() {
			PulseChannel1.ClockLengthCounter();
			PulseChannel2.ClockLengthCounter();
			TriangleChannel.ClockLengthCounter();
			NoiseChannel.ClockLengthCounter();
		}
		inline void Sweep() {
			PulseChannel1.Sweep();
			PulseChannel1.CalculateSweep();
			PulseChannel2.Sweep();
			PulseChannel2.CalculateSweep();
		}
		/* Четные такты */
		inline void EvenClock() {
			Envelope();
			TriangleChannel.ClockLinearCounter();
		}
		/* Нечетные такты */
		inline void OddClock() {
			EvenClock();
			LengthCounter();
			Sweep();
		}
		/* Обновить все буферы */
		inline void UpdateAllBuffers(CAPU &APU) {
			PulseChannel1.UpdateBuffer(APU, InternalClock);
			PulseChannel2.UpdateBuffer(APU, InternalClock);
			TriangleChannel.UpdateBuffer(APU, InternalClock);
			NoiseChannel.UpdateBuffer(APU, InternalClock);
//			DMChannel.UpdateBuffer(APU, InternalClock);
		}
	} Channels;

	/* Функция обработки из события */
	inline void EventCall(int CallTime) {
		int i;

		Process(CallTime);
		/* Async is always 0?? */
		CycleData.Async = (CycleData.Async + CallTime) % ClockDivider;
		for (i = 0; i < MAX_EVENTS; i++)
			if (EventTime[i] >= 0) {
				/* Знак не изменится, если событие не было выполнено */
				EventTime[i] -= CallTime;
			}
		CycleData.NextEvent -= CallTime;
		CycleData.InternalClock = -CycleData.Async;
		CycleData.IRQOffset -= CycleData.Async + CallTime;
		CycleData.FrameCycle += CallTime;
	}
	/* Функция обработки из CPU */
	inline void CPUCall() {
		Process(Bus->GetInternalClock());
	}

	/* Функция обработки */
	void Process(int Time);

	/* Простой рендер */
	inline int SimpleRender(int Time) {
		int FullClocks = Time / ClockDivider;
		Channels.InternalClock += FullClocks;
		return FullClocks * ClockDivider;
	}

	/* Обработка события */
	inline void ProcessEvent() {
		int i;

		for (i = 0; i < MAX_EVENTS; i++) {
			if (CycleData.InternalClock == EventTime[i])
				switch (i) {
					case EVENT_APU_TICK:
						Channels.UpdateAllBuffers(*this);
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
						CycleData.Step++;
						if (CycleData.Step == Channels.Mode)
							CycleData.Step = 0;
						Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
							LocalEvents[EVENT_APU_TICK].Time +
							Tables::StepCycles[CycleData.Step]);
						EventTime[EVENT_APU_TICK] += Tables::StepCycles[CycleData.Step];
						break;
					case EVENT_APU_FRAME_IRQ:
					case EVENT_APU_DMC_IRQ:
					case EVENT_APU_DMC_DMA:
						EventTime[i] = -1;
						break;
				}
		}
		CycleData.NextEvent = -1;
		for (i = 0; i < MAX_EVENTS; i++) {
			if ((EventTime[i] >= 0) && ((CycleData.NextEvent < 0) ||
				(CycleData.NextEvent) > EventTime[i]))
				CycleData.NextEvent = EventTime[i];
		}
	}
public:
	CAPU(_Bus *pBus) {
		SEvent *NewEvent;

		Bus = pBus;
		Bus->GetManager()->template SetPointer<SCycleData>(&CycleData);
		Bus->GetManager()->template SetPointer<ManagerID<APUID::EventDataID> >(LocalEvents,
			sizeof(SEventData) * MAX_EVENTS);
		memset(LocalEvents, 0, sizeof(SEventData) * MAX_EVENTS);
		Bus->GetManager()->template SetPointer<ManagerID<APUID::EventTimeID> >(EventTime,
			sizeof(int) * MAX_EVENTS);
		memset(EventTime, 0, sizeof(int) * MAX_EVENTS);
#define MAKE_EVENT(alias) \
		do { \
			NewEvent = new SEvent; \
			NewEvent->Data = &LocalEvents[alias]; \
			NewEvent->Name = #alias; \
			NewEvent->Execute = [this] { EventCall(EventTime[alias]); }; \
			EventChain[alias] = NewEvent; \
			Bus->GetClock()->RegisterEvent(NewEvent); \
		} while(0)
		MAKE_EVENT(EVENT_APU_TICK);
		MAKE_EVENT(EVENT_APU_FRAME_IRQ);
		MAKE_EVENT(EVENT_APU_DMC_IRQ);
		MAKE_EVENT(EVENT_APU_DMC_DMA);
#undef MAKE_EVENT
		Bus->GetManager()->template SetPointer<SChannels>(&Channels);
		memset(&Channels, 0, sizeof(Channels));
	}
	inline ~CAPU() {
	}

	/* Заполнить буфер */
	inline void FlushBuffer(int Time) {
		EventCall(Time - CycleData.FrameCycle);
		Channels.FlushAll(*this);
		Channels.ResetClock();
		CycleData.FrameCycle -= Time;
	}
	/* Сброс */
	inline void Reset() {
		int i;

		memset(&CycleData, 0, sizeof(CycleData));
		for (i = 0; i < MAX_EVENTS; i++)
			Bus->GetClock()->DisableEvent(EventChain[i]);
		Channels.Reset();
		EventTime[EVENT_APU_TICK] = Tables::StepCycles[0];
		Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK], Tables::StepCycles[0]);
		Bus->GetClock()->EnableEvent(EventChain[EVENT_APU_TICK]);
	}
	/* Сброс часов шины */
	inline void ResetInternalClock(int Time) {
		CycleData.IRQOffset += Time;
	}
	/* Выполнить DMA */
	inline int Execute() {
		return ClockDivider;
	}
	/* Прочитать из регистра */
	inline uint8 ReadByte(uint16 Address) {
		return 0x40;
	}
	/* Записать в регистр */
	inline void WriteByte(uint16 Address, uint8 Src) {
	}
};

/* Обработчик тактов */
template <class _Bus, class _Settings>
void CAPU<_Bus, _Settings>::Process(int Time) {
	int RunTime, LeftTime = Time + CycleData.Async;
	bool ExecuteEvent;

	while (LeftTime >= ClockDivider) {
		RunTime = LeftTime;
		if ((CycleData.InternalClock + RunTime) >= CycleData.NextEvent) {
			RunTime = CycleData.NextEvent - CycleData.InternalClock;
			ExecuteEvent = true;
		} else
			ExecuteEvent = false;
		RunTime = SimpleRender(RunTime);
		CycleData.InternalClock += RunTime;
		if (ExecuteEvent)
			ProcessEvent();
		LeftTime -= RunTime;
	}
}

}

#endif
