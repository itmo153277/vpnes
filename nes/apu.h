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
#include "frontend.h"
#include "clock.h"
#include "bus.h"
#include "cpu.h"

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
		int InternalClock;
		int Async;
		int NextEvent;
		int IRQOffset;
		int FrameCycle;
		int LastFrameIRQCycle;
		int Step;
		int OddCycle;
	} CycleData;

	/* События */
	enum {
		EVENT_APU_FRAME_IRQ = 0,
		EVENT_APU_DMC_DMA,
		EVENT_APU_TICK,
		EVENT_APU_RESET,
		EVENT_APU_TICK2,
		MAX_EVENTS
	};
	enum {
		LAST_REG_EVENT = EVENT_APU_RESET
	};
	/* Данные о событиях */
	SEventData LocalEvents[LAST_REG_EVENT + 1];
	/* Указатели на события */
	SEvent *EventChain[LAST_REG_EVENT + 1];
	/* Периоды событий */
	int EventTime[MAX_EVENTS];
	/* Время записи 0x4017 */
	int WriteCommit;

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
		bool IgnoreCounter; /* Игнорировать изменение счетчика */
		int LengthCounter; /* Счетчик длины */
		int Timer; /* Таймер */
		int EnvelopeTimer; /* Счетчик для энвелопа */
		int SweepTimer; /* Счетчик для свипа */

		/* Запись 0x4000 / 0x4004 */
		inline void Write_1(uint8 Src) {
			DutyMode = Src >> 6;
			EnvelopeLoop = Src & 0x20;
			ConstantVolume = Src & 0x10;
			EnvelopePeriod = Src & 0x0f;
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
			if (UseCounter && !IgnoreCounter)
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
		inline void ClockLengthCounter(bool EndPoint) {
			if ((!EnvelopeLoop) && (LengthCounter > 0)) {
				LengthCounter--;
				IgnoreCounter = EndPoint;
			}
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
				if (Pos == MAX_BUF)
					return RealCount;
			}
			while (Len >= (TimerPeriod + 1)) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = (TimerPeriod + 1) << 1;
				DutyCycle++;
				DutyCycle &= 7;
				RealCount += (TimerPeriod + 1) << 1;
				if (Pos == MAX_BUF)
					return RealCount;
				Len -= TimerPeriod + 1;
			}
			if (Len > 0) {
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
		bool IgnoreCounter; /* Игнорировать изменение счетчика */
		int LengthCounter; /* Счетчик длины */

		/* Запись 0x4008 */
		inline void Write_1(uint8 Src) {
			ControlFlag = Src & 0x80;
			CounterReloadValue = Src & 0x7f;
		}
		/* Запись 0x400a */
		inline void Write_2(uint8 Src) {
			TimerPeriod = (TimerPeriod & 0x0700) | Src;
		}
		/* Запсиь 0x400b */
		inline void Write_3(uint8 Src) {
			TimerPeriod = (TimerPeriod & 0x00ff) | ((Src & 0x07) << 8);
			CounterReload = true;
			if (UseCounter && !IgnoreCounter)
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
		inline void ClockLengthCounter(bool EndPoint) {
			if ((!ControlFlag) && (LengthCounter > 0)) {
				LengthCounter--;
				IgnoreCounter = EndPoint;
			}
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
				if ((TimerPeriod < 2) && (LengthCounter > 0) && (LinearCounter > 0)) {
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
				if (Pos == MAX_BUF)
					return RealCount;
			}
			while (Len >= (TimerPeriod + 1)) {
				Buffer[Pos].Sample = Tables::SeqTable[SequencePos];
				Buffer[Pos++].Length = TimerPeriod + 1;
				SequencePos++;
				SequencePos &= 31;
				RealCount += TimerPeriod + 1;
				if (Pos == MAX_BUF)
					return RealCount;
				Len -= TimerPeriod + 1;
			}
			if (Len > 0) {
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
			if (Pos == MAX_BUF)
				return InternalClock;
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
		bool IgnoreCounter; /* Игнорировать изменение счетчика */
		int LengthCounter; /* Счетчик длины */
		int Timer; /* Таймер */
		int EnvelopeTimer; /* Счетчик для энвелопа */
		uint16 Random; /* Биты рандома */

		/* Запсиь в 0x400c */
		inline void Write_1(uint8 Src) {
			EnvelopeLoop = Src & 0x20;
			ConstantVolume = Src & 0x10;
			EnvelopePeriod = Src & 0x0f;
		}
		/* Запись в 0x400e */
		inline void Write_2(uint8 Src) {
			TimerPeriod = Tables::NoiseTable[Src & 0x0f];
			ShiftCount = (Src & 0x80) ? 8 : 13;
		}
		/* Запись в 0x400f */
		inline void Write_3(uint8 Src) {
			EnvelopeStart = true;
			if (UseCounter && !IgnoreCounter)
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
		inline void ClockLengthCounter(bool EndPoint) {
			if ((!EnvelopeLoop) && (LengthCounter > 0)) {
				LengthCounter--;
				IgnoreCounter = EndPoint;
			}
		}
		/* Сброс */
		inline void Reset() {
			Random = 0x0001;
			ShiftCount = 13;
			TimerPeriod = Tables::NoiseTable[0];
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
				if (Pos == MAX_BUF)
					return RealCount;
			}
			while (Len >= (TimerPeriod + 1)) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = TimerPeriod + 1;
				Random = (((Random >> 14) ^ (Random >> ShiftCount)) & 0x01) | (Random << 1);
				RealCount += TimerPeriod + 1;
				if (Pos == MAX_BUF)
					return RealCount;
				Len -= TimerPeriod + 1;
			}
			if (Len > 0) {
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
			if (Pos == MAX_BUF)
				return InternalClock;
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

	/* ДМ-канал */
	struct SDMChannel: public SAPUUnit {
		using SAPUUnit::Pos;
		using SAPUUnit::Buffer;
		using SAPUUnit::InternalClock;
		int Sample; /* Выход */
		int IRQEnabled; /* Генерировать IRQ */
		bool IRQFlag; /* Флаг IRQ */
		int LoopFlag; /* Флаг повтора */
		int TimerPeriod; /* Период */
		uint16 Address; /* Адрес для семплов */
		uint16 CurAddress; /* Текущий адрес */
		int Length; /* Количество семплов */
		int SamplesLeft; /* Оставшееся количество */
		int BitsRemain; /* Осталось битов */
		bool NotEmpty; /* В буфере есть данные */
		uint8 ShiftReg; /* Регистр сдвига */
		uint8 SampleBuffer; /* Буфер */
		bool SilenceFlag; /* Ничего не выводить */
		int Timer; /* Таймер */

		/* Запись в 0x4010 */
		inline void Write_1(uint8 Src) {
			IRQEnabled = Src & 0x80;
			if (!IRQEnabled)
				IRQFlag = false;
			LoopFlag = Src & 0x40;
			TimerPeriod = Tables::DMTable[Src & 0x0f];
		}
		/* Запись в 0x4011 */
		inline void Write_2(uint8 Src) {
			Sample = Src & 0x7f;
		}
		/* Запись в 0x4012 */
		inline void Write_3(uint8 Src) {
			Address = 0xc000 | (Src << 6);
			CurAddress = Address;
		}
		/* Запись в 0x4013 */
		inline void Write_4(uint8 Src) {
			Length = (Src << 4) | 0x0001;
			if (SamplesLeft == 0)
				SamplesLeft = Length;
		}

		/* Сброс */
		inline void Reset() {
			TimerPeriod = Tables::DMTable[0];
			NotEmpty = false;
			SilenceFlag = true;
		}
		/* Обработка такта */
		inline void ProcessClock() {
			if (BitsRemain == 0) {
				BitsRemain = 8;
				if (NotEmpty) {
					SilenceFlag = false;
					NotEmpty = false;
					ShiftReg = SampleBuffer;
				} else
					SilenceFlag = true;
			}
			if (!SilenceFlag) {
				if (ShiftReg & 0x01) {
					if (Sample < 0x7e)
						Sample += 2;
				} else {
					if (Sample > 1)
						Sample -= 2;
				}
				ShiftReg >>= 1;
			}
			BitsRemain--;
		}
		/* Заполнить буфер */
		int FillBuffer(int Count) {
			int RealCount = 0;
			int Len = Count;

			if (Count == 0)
				return Count;
			if (SilenceFlag && !NotEmpty) {
				Buffer[Pos].Sample = Sample;
				Buffer[Pos++].Length = Count;
				BitsRemain = 7 - ((7 - BitsRemain + (TimerPeriod - Timer - 1 + Count) /
					(TimerPeriod + 1)) & 7);
				Timer = TimerPeriod - ((Count + TimerPeriod - Timer) % (TimerPeriod + 1));
				return Count;
			}
			if (Len < Timer) {
				Buffer[Pos].Sample = Sample;
				Buffer[Pos++].Length = Len;
				Timer -= Len;
				return Count;
			}
			if (Timer > 0) {
				Buffer[Pos].Sample = Sample;
				Buffer[Pos++].Length = Timer;
				Len -= Timer;
				RealCount += Timer;
				Timer = 0;
				ProcessClock();
				if (Pos == MAX_BUF)
					return RealCount;
			}
			for (;;) {
				if (SilenceFlag && !NotEmpty && Len > 0) {
					Buffer[Pos].Sample = Sample;
					Buffer[Pos++].Length = Len;
					BitsRemain = 7 - ((7 - BitsRemain + (TimerPeriod + Count - 1) /
						(TimerPeriod + 1)) & 7);
					Timer = TimerPeriod  - (Len + TimerPeriod) % (TimerPeriod + 1);
					return Count;
				}
				if (Len < (TimerPeriod + 1))
					break;
				Buffer[Pos].Sample = Sample;
				Buffer[Pos++].Length = TimerPeriod + 1;
				ProcessClock();
				RealCount += TimerPeriod + 1;
				if (Pos == MAX_BUF)
					return RealCount;
				Len -= TimerPeriod + 1;
			}
			if (Len > 0) {
				Buffer[Pos].Sample = Sample;
				Buffer[Pos++].Length = Len;
				Timer = TimerPeriod + 1 - Len;
				RealCount += Len;
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
		SDMChannel DMChannel;
		int PlayTime; /* Текущее время проигрывания */
		int InternalClock; /* Текущее время */
		bool FrameInterrupt; /* Прерывание счетчика кадров */
		int InterruptInhibit; /* Подавление IRQ */
		enum SeqMode {
			Mode_4step = 4,
			Mode_5step = 5
		} Mode; /* Режим последовательностей */
		uint8 Buf4017; /* Сохраненный регистр 0x4017 */
		bool Double4017; /* Флаги отслеживаний записи в 0x4017 */

		/* Запись 0x4015 */
		inline void Write4015(uint8 Src) {
			PulseChannel1.UseCounter = Src & 0x01;
			PulseChannel2.UseCounter = Src & 0x02;
			TriangleChannel.UseCounter = Src & 0x04;
			NoiseChannel.UseCounter = Src & 0x08;
			if (!(Src & 0x10))
				DMChannel.SamplesLeft = 0;
			else if (DMChannel.SamplesLeft == 0)
				DMChannel.SamplesLeft = DMChannel.Length;
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
			if (DMChannel.SamplesLeft > 0)
				Res |= 0x10;
			if (FrameInterrupt)
				Res |= 0x40;
			if (DMChannel.IRQFlag)
				Res |= 0x80;
			return Res;
		}
		/* Запись 0x4017 */
		inline void Write4017(uint8 Src) {
			Mode = (Src & 0x80) ? Mode_5step : Mode_4step;
			InterruptInhibit = (Src & 0x40);
			if (InterruptInhibit)
				FrameInterrupt = false;
		}

		/* Вывести звук */
		void MuxChannels(CAPU &APU, int Clocks) {
#if !defined(VPNES_PRECISE_MUX)
			int Length, PulseOut, TOut, NDOut = 0;
			double Sample;

			while (PlayTime < Clocks) {
				Length = Clocks - PlayTime;
				PulseChannel1.MinimizeLength(Length);
				PulseChannel2.MinimizeLength(Length);
				TriangleChannel.MinimizeLength(Length);
				NoiseChannel.MinimizeLength(Length);
				DMChannel.MinimizeLength(Length);
				PlayTime += Length;
				PulseOut = PulseChannel1.PlaySample(Length) +
					PulseChannel2.PlaySample(Length);
				TOut = TriangleChannel.PlaySample(Length);
				NDOut = NoiseChannel.PlaySample(Length) * 2 +
					DMChannel.PlaySample(Length);
				if (TOut < 0)
					Sample = (apu::TNDTable[NDOut + 21] + apu::TNDTable[NDOut + 24]) / 2.0;
				else
					Sample = apu::TNDTable[NDOut + TOut * 3];
				Sample += apu::SquareTable[PulseOut];
				APU.Bus->GetFrontend()->GetAudioFrontend()->PushSample(Sample, Length *
					ClockDivider * _Settings::GetFreq());
			}
#else
			int Length, PulseOut, TOut, NOut, DOut;
			double Sample;

			while (PlayTime < Clocks) {
				Length = Clocks - PlayTime;
				PulseChannel1.MinimizeLength(Length);
				PulseChannel2.MinimizeLength(Length);
				TriangleChannel.MinimizeLength(Length);
				NoiseChannel.MinimizeLength(Length);
				DMChannel.MinimizeLength(Length);
				PlayTime += Length;
				PulseOut = PulseChannel1.PlaySample(Length) +
					PulseChannel2.PlaySample(Length);
				TOut = TriangleChannel.PlaySample(Length);
				NOut = NoiseChannel.PlaySample(Length);
				DOut = DMChannel.PlaySample(Length);
				if (TOut < 0)
					Sample = 95.88 / (8128.0 / PulseOut + 100) +
						159.79 / (1 / (7.5 / 8227.0 + NOut / 12241.0 +
						DOut / 22638.0) + 100);
				else
					Sample = 95.88 / (8128.0 / PulseOut + 100) +
						159.79 / (1 / (TOut / 8227.0 + NOut / 12241.0 +
						DOut / 22638.0) + 100);
				APU.Bus->GetFrontend()->GetAudioFrontend()->PushSample(Sample, Length *
					ClockDivider * _Settings::GetFreq());
			}
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
				RealTime = DMChannel.UpdateBuffer(RealTime);
				MuxChannels(APU, RealTime);
			} while (RealTime < Clocks);
		}
		/* Сбросить время */
		inline void ResetClock() {
			PulseChannel1.ResetClock(InternalClock);
			PulseChannel2.ResetClock(InternalClock);
			TriangleChannel.ResetClock(InternalClock);
			NoiseChannel.ResetClock(InternalClock);
			DMChannel.ResetClock(InternalClock);
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
			DMChannel.Reset();
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
		inline void LengthCounter(bool EndPoint) {
			PulseChannel1.ClockLengthCounter(EndPoint);
			PulseChannel2.ClockLengthCounter(EndPoint);
			TriangleChannel.ClockLengthCounter(EndPoint);
			NoiseChannel.ClockLengthCounter(EndPoint);
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
		inline void OddClock(bool EndPoint) {
			EvenClock();
			LengthCounter(EndPoint);
			Sweep();
		}
		/* Обновить все буферы */
		inline void UpdateAllBuffers(CAPU &APU) {
			PulseChannel1.UpdateBuffer(APU, InternalClock);
			PulseChannel2.UpdateBuffer(APU, InternalClock);
			TriangleChannel.UpdateBuffer(APU, InternalClock);
			NoiseChannel.UpdateBuffer(APU, InternalClock);
			DMChannel.UpdateBuffer(APU, InternalClock);
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
		CycleData.InternalClock -= CallTime;
		CycleData.IRQOffset -= CallTime;
		CycleData.FrameCycle += CallTime;
		if (CycleData.LastFrameIRQCycle >= 0)
			CycleData.LastFrameIRQCycle -= CallTime;
	}
	/* Функция обработки из CPU */
	inline void CPUCall(int Offset = 0) {
		WriteCommit = Bus->GetInternalClock() + CycleData.IRQOffset + Offset;
		Process(WriteCommit);
		WriteCommit = -1;
	}

	/* Функция обработки */
	void Process(int Time);

	/* Простой рендер */
	inline int SimpleRender(int Time) {
		int FullClocks = Time / ClockDivider;
		if ((Time % ClockDivider) != 0)
			FullClocks++;
		Channels.InternalClock += FullClocks;
		CycleData.OddCycle ^= FullClocks;
		return FullClocks * ClockDivider;
	}

	/* Обработка события */
	inline void ProcessEvent() {
		int i;

		for (i = 0; i < MAX_EVENTS; i++) {
			if (CycleData.InternalClock == EventTime[i])
				switch (i) {
					case EVENT_APU_FRAME_IRQ:
						if (CycleData.InternalClock == WriteCommit)
							Channels.Write4017(Channels.Buf4017);
						if (Channels.Mode == SChannels::Mode_4step) {
							CycleData.LastFrameIRQCycle = CycleData.InternalClock +
								2 * ClockDivider;
							if (!Channels.InterruptInhibit) {
								Channels.FrameInterrupt = true;
								Bus->GetCPU()->GenerateIRQ(CycleData.InternalClock -
									CycleData.IRQOffset - Bus->GetCPU()->GetIRQOffset() +
									ClockDivider, _Bus::CPUClass::FrameIRQ);
							}
						}
						Bus->GetClock()->DisableEvent(EventChain[EVENT_APU_FRAME_IRQ]);
						EventTime[EVENT_APU_FRAME_IRQ] = -1;
						break;
					case EVENT_APU_DMC_DMA:
						EventTime[i] = -1;
						break;
					case EVENT_APU_TICK:
						Channels.UpdateAllBuffers(*this);
						if (CycleData.InternalClock != EventTime[EVENT_APU_TICK2])
							switch (CycleData.Step) {
								case 0:
								case 2:
									Channels.EvenClock();
									break;
								case 1:
								case 3:
									Channels.OddClock(CycleData.InternalClock == WriteCommit);
									break;
							}
						CycleData.Step++;
						if (CycleData.Step == 4) {
							EventTime[EVENT_APU_FRAME_IRQ] = EventTime[EVENT_APU_TICK] +
								Tables::StepCycles[0] + Tables::StepCycles[1] +
								Tables::StepCycles[2] + Tables::StepCycles[3];
							Bus->GetClock()->EnableEvent(EventChain[EVENT_APU_FRAME_IRQ]);
							Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_FRAME_IRQ],
								LocalEvents[EVENT_APU_TICK].Time - EventTime[EVENT_APU_TICK] +
								EventTime[EVENT_APU_FRAME_IRQ]);
							Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
								LocalEvents[EVENT_APU_TICK].Time + Tables::StepCycles[0] +
								ClockDivider);
							EventTime[EVENT_APU_TICK] += Tables::StepCycles[0] + ClockDivider;
							CycleData.Step = 0;
						} else {
							if ((Channels.Mode == SChannels::Mode_5step) &&
								(CycleData.Step == 3)) {
								Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
									LocalEvents[EVENT_APU_TICK].Time + Tables::StepCycles[3] +
									Tables::StepCycles[4]);
								EventTime[EVENT_APU_TICK] += Tables::StepCycles[3] +
									Tables::StepCycles[4];
							} else {
								Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
									LocalEvents[EVENT_APU_TICK].Time +
									Tables::StepCycles[CycleData.Step]);
								EventTime[EVENT_APU_TICK] +=
									Tables::StepCycles[CycleData.Step];
							}
						}
						break;
					case EVENT_APU_TICK2:
						Channels.UpdateAllBuffers(*this);
						Channels.OddClock(CycleData.InternalClock == WriteCommit);
						EventTime[EVENT_APU_TICK2] = -1;
						break;
					case EVENT_APU_RESET:
						Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
							LocalEvents[EVENT_APU_TICK].Time - EventTime[EVENT_APU_TICK] +
							EventTime[EVENT_APU_RESET] + Tables::StepCycles[0]);
						EventTime[EVENT_APU_TICK] = EventTime[EVENT_APU_RESET] +
							Tables::StepCycles[0];
						EventTime[EVENT_APU_FRAME_IRQ] = EventTime[EVENT_APU_TICK] +
							Tables::StepCycles[1] + Tables::StepCycles[2] +
							Tables::StepCycles[3] - ClockDivider;
						Bus->GetClock()->EnableEvent(EventChain[EVENT_APU_FRAME_IRQ]);
						Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_FRAME_IRQ],
							LocalEvents[EVENT_APU_RESET].Time - EventTime[EVENT_APU_RESET] +
							EventTime[EVENT_APU_FRAME_IRQ]);
						CycleData.Step = 0;
						if (Channels.Double4017) {
							EventTime[EVENT_APU_RESET] += 2 * ClockDivider;
							if (Channels.Mode == SChannels::Mode_5step)
								EventTime[EVENT_APU_TICK2] = EventTime[EVENT_APU_RESET] -
									ClockDivider;
							Channels.Double4017 = false;
							Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_RESET],
								LocalEvents[EVENT_APU_RESET].Time + 2 * ClockDivider);
						} else {
							Bus->GetClock()->DisableEvent(EventChain[EVENT_APU_RESET]);
							EventTime[EVENT_APU_RESET] = -1;
						}
						break;
					default:
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
			sizeof(SEventData) * (LAST_REG_EVENT + 1));
		memset(LocalEvents, 0, sizeof(SEventData) * (LAST_REG_EVENT + 1));
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
		MAKE_EVENT(EVENT_APU_FRAME_IRQ);
		MAKE_EVENT(EVENT_APU_DMC_DMA);
		MAKE_EVENT(EVENT_APU_RESET);
		MAKE_EVENT(EVENT_APU_TICK);
#undef MAKE_EVENT
		Bus->GetManager()->template SetPointer<SChannels>(&Channels);
		memset(&Channels, 0, sizeof(Channels));
		WriteCommit = -1;
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
		for (i = 0; i < (LAST_REG_EVENT + 1); i++)
			Bus->GetClock()->DisableEvent(EventChain[i]);
		Channels.Reset();
		EventTime[EVENT_APU_FRAME_IRQ] = Tables::StepCycles[0] + Tables::StepCycles[1] +
			Tables::StepCycles[2] + Tables::StepCycles[3] - ClockDivider;
		Bus->GetClock()->EnableEvent(EventChain[EVENT_APU_FRAME_IRQ]);
		Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_FRAME_IRQ],
			Bus->GetClock()->GetTime() + EventTime[EVENT_APU_FRAME_IRQ]);
		EventTime[EVENT_APU_TICK] = Tables::StepCycles[0];
		Bus->GetClock()->EnableEvent(EventChain[EVENT_APU_TICK]);
		Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK], Bus->GetClock()->GetTime() +
			EventTime[EVENT_APU_TICK]);
		EventTime[EVENT_APU_TICK2] = -1;
		EventTime[EVENT_APU_RESET] = -1;
	}
	/* Сброс часов шины */
	inline void ResetInternalClock(int Time) {
		CycleData.IRQOffset += Time;
	}
	/* Выполнить DMA */
	inline int Execute(uint16 Address) {
		Bus->IncrementClock(ClockDivider);
		return ClockDivider;
	}
	/* Прочитать из регистра */
	inline uint8 ReadByte(uint16 Address) {
		uint8 Res;

		switch (Address) {
			case 0x4015:
				CPUCall(-ClockDivider);
				Res = Channels.Read4015();
				if ((CycleData.InternalClock + ClockDivider) > CycleData.LastFrameIRQCycle) {
					Channels.FrameInterrupt = false;
					Bus->GetCPU()->ClearIRQ(_Bus::CPUClass::FrameIRQ);
				}
				return Res;
			case 0x4016:
				return Bus->GetFrontend()->GetInput1Frontend()->ReadState();
			case 0x4017:
				return Bus->GetFrontend()->GetInput2Frontend()->ReadState();
		}
		return 0x40;
	}
	/* Записать в регистр */
	inline void WriteByte(uint16 Address, uint8 Src) {
		int t, x;

		switch (Address) {
			case 0x4000:
				CPUCall();
				Channels.PulseChannel1.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel1.Write_1(Src);
				break;
			case 0x4001:
				CPUCall();
				Channels.PulseChannel1.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel1.Write_2(Src);
				Channels.PulseChannel1.CalculateSweep();
				break;
			case 0x4002:
				CPUCall();
				Channels.PulseChannel1.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel1.Write_3(Src);
				Channels.PulseChannel1.CalculateSweep();
				break;
			case 0x4003:
				Channels.PulseChannel1.IgnoreCounter = false;
				CPUCall();
				Channels.PulseChannel1.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel1.Write_4(Src);
				Channels.PulseChannel1.CalculateSweep();
				break;
			case 0x4004:
				CPUCall();
				Channels.PulseChannel2.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel2.Write_1(Src);
				break;
			case 0x4005:
				CPUCall();
				Channels.PulseChannel2.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel2.Write_2(Src);
				Channels.PulseChannel2.CalculateSweep();
				break;
			case 0x4006:
				CPUCall();
				Channels.PulseChannel2.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel2.Write_3(Src);
				Channels.PulseChannel2.CalculateSweep();
				break;
			case 0x4007:
				Channels.PulseChannel2.IgnoreCounter = false;
				CPUCall();
				Channels.PulseChannel2.UpdateBuffer(*this, Channels.InternalClock);
				Channels.PulseChannel2.Write_4(Src);
				Channels.PulseChannel2.CalculateSweep();
				break;
			case 0x4008:
				CPUCall();
				Channels.TriangleChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.TriangleChannel.Write_1(Src);
				break;
			case 0x400a:
				CPUCall();
				Channels.TriangleChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.TriangleChannel.Write_2(Src);
				break;
			case 0x400b:
				Channels.TriangleChannel.IgnoreCounter = false;
				CPUCall();
				Channels.TriangleChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.TriangleChannel.Write_3(Src);
				break;
			case 0x400c:
				CPUCall();
				Channels.NoiseChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.NoiseChannel.Write_1(Src);
				break;
			case 0x400e:
				CPUCall();
				Channels.NoiseChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.NoiseChannel.Write_2(Src);
				break;
			case 0x400f:
				Channels.NoiseChannel.IgnoreCounter = false;
				CPUCall();
				Channels.NoiseChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.NoiseChannel.Write_3(Src);
				break;
			case 0x4010:
				CPUCall();
				Channels.DMChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.DMChannel.Write_1(Src);
				if (!Channels.DMChannel.IRQFlag)
					Bus->GetCPU()->ClearIRQ(_Bus::CPUClass::DMCIRQ);
				break;
			case 0x4011:
				CPUCall();
				Channels.DMChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.DMChannel.Write_2(Src);
				break;
			case 0x4012:
				CPUCall();
				Channels.DMChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.DMChannel.Write_3(Src);
			case 0x4013:
				CPUCall();
				Channels.DMChannel.UpdateBuffer(*this, Channels.InternalClock);
				Channels.DMChannel.Write_4(Src);
//			case 0x4014:
//				CPUCall();
//				...
//				break;
			case 0x4015:
				CPUCall();
				Channels.UpdateAllBuffers(*this);
				Channels.Write4015(Src);
				if (!Channels.DMChannel.IRQFlag)
					Bus->GetCPU()->ClearIRQ(_Bus::CPUClass::DMCIRQ);
				break;
			case 0x4016:
				Bus->GetFrontend()->GetInput1Frontend()->InputSignal(Src);
				Bus->GetFrontend()->GetInput2Frontend()->InputSignal(Src);
				break;
			case 0x4017:
				t = Channels.Mode;
				Channels.Buf4017 = Src;
				CPUCall();
				Channels.Write4017(Src);
				if (CycleData.OddCycle & 1)
					x = 3 * ClockDivider;
				else
					x = 2 * ClockDivider;
				if ((EventTime[EVENT_APU_RESET] >= 0) && ((CycleData.InternalClock + x) >
					EventTime[EVENT_APU_RESET]))
					Channels.Double4017 = true;
				else if (EventTime[EVENT_APU_RESET] < 0) {
					EventTime[EVENT_APU_RESET] = CycleData.InternalClock + x;
					if (Channels.Mode == SChannels::Mode_5step)
						EventTime[EVENT_APU_TICK2] = EventTime[EVENT_APU_RESET] - ClockDivider;
					Bus->GetClock()->EnableEvent(EventChain[EVENT_APU_RESET]);
					Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_RESET],
						LocalEvents[EVENT_APU_TICK].Time - EventTime[EVENT_APU_TICK] +
						EventTime[EVENT_APU_RESET]);
				}
				if ((CycleData.Step == 3) && (CycleData.InternalClock <=
					(EventTime[EVENT_APU_TICK] - ClockDivider))) {
					if ((t == SChannels::Mode_4step) &&
						(Channels.Mode == SChannels::Mode_5step)) {
						EventTime[EVENT_APU_TICK] += Tables::StepCycles[4];
						Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
							LocalEvents[EVENT_APU_TICK].Time + Tables::StepCycles[4]);
					} else if ((t == SChannels::Mode_5step) && (Channels.Mode ==
						SChannels::Mode_4step)) {
						if (CycleData.InternalClock <= (EventTime[EVENT_APU_TICK] -
							Tables::StepCycles[4] - ClockDivider)) {
							EventTime[EVENT_APU_TICK] -= Tables::StepCycles[4];
							Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
								LocalEvents[EVENT_APU_TICK].Time - Tables::StepCycles[4]);
						} else {
							/* Игнорируем последнюю фазу. По срабатыванию должно */
							/* произойти Step = 0, но не произойдет из-за RESET */
							EventTime[EVENT_APU_TICK] += Tables::StepCycles[5];
							Bus->GetClock()->SetEventTime(EventChain[EVENT_APU_TICK],
								LocalEvents[EVENT_APU_TICK].Time + Tables::StepCycles[5]);
						}
					}
				}
				if ((EventTime[EVENT_APU_TICK2] > 0) && (EventTime[EVENT_APU_TICK2] <
					CycleData.NextEvent))
					CycleData.NextEvent = EventTime[EVENT_APU_TICK2];
				else if (EventTime[EVENT_APU_RESET] < CycleData.NextEvent)
					CycleData.NextEvent = EventTime[EVENT_APU_RESET];
				if (!Channels.FrameInterrupt) {
					if (!Channels.InterruptInhibit &&
						(Channels.Mode == SChannels::Mode_4step) &&
						CycleData.InternalClock <= CycleData.LastFrameIRQCycle) {
						Channels.FrameInterrupt = true;
						Bus->GetCPU()->GenerateIRQ(CycleData.InternalClock -
							CycleData.IRQOffset - Bus->GetCPU()->GetIRQOffset() +
							ClockDivider, _Bus::CPUClass::FrameIRQ);
					} else
						Bus->GetCPU()->ClearIRQ(_Bus::CPUClass::FrameIRQ);
				}
				break;
		}
	}
};

/* Обработчик тактов */
template <class _Bus, class _Settings>
void CAPU<_Bus, _Settings>::Process(int Time) {
	int RunTime, LeftTime = Time - CycleData.InternalClock;
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
