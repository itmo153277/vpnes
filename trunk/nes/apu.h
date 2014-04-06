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

#include "manager.h"
#include "clock.h"
#include "bus.h"

namespace vpnes {

namespace APUID {

typedef APUGroup<1>::ID::NoBatteryID CycleDataID;
typedef APUGroup<2>::ID::NoBatteryID EventDataID;
typedef APUGroup<3>::ID::NoBatteryID EventTimeID;

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
		enum BufCode {
			BufOK = 0,
			BufFull
		};

		/* Обновить буфер */
		inline BufCode UpdateBuffer(int Time) {
			if (Time == InternalClock)
				return BufOK;
			if (Pos == MAX_BUF)
				Pos = 0;
			InternalClock += FillBuffer(Time - InternalClock);
			if (Pos == MAX_BUF)
				return BufFull;
			else
				return BufOK;
		}
		/* Сбросить часы */
		inline void ResetClock() {
			InternalClock = 0;
		}
		/* Заполнить буфер */
		virtual int FillBuffer(int Count) = 0;
		/* Проиграть семпл */
		inline int PlaySample(int Length) {
			int Out;

			Out = Buffer[PlayPos].Sample;
			Buffer[PlayPos].Length -= Length;
			if (Buffer[PlayPos].Length == 0) {
				PlayPos++;
				if (PlayPos == MAX_BUF)
					PlayPos = 0;
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
	struct SPulseChannel: SAPUUnit {
		using SAPUUnit::Pos;
		using SAPUUnit::Buffer;
		using SAPUUnit::MAX_BUF;
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
		int TimerDiff; /* Сдвиг таймера */

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
		void FillBuffer(int Count) {
			int Len = (Count - TimerDiff + 1) >> 1;
			int RealCount = Count - (Len << 1);

			TimerDiff ^= Count & 1;
			if (Len == 0)
				return Count;
			if (!TimerValid || (LengthCounter <= 0)) {
				Buffer[Pos].Sample = 0;
				Buffer[Pos++].Length = Len << 1;
				DutyCycle = (DutyCycle + (Timer + Len) / (TimerPeriod + 1)) & 7;
				Timer = TimerPeriod - ((Len + TimerPeriod - Timer) % (TimerPeriod + 1));
				return Count;
			}
			if (Len < (Timer + 1)) {
				Buffer[Pos].Sample = GetOutput();
				Buffer[Pos++].Length = Len << 1;
				Timer -= Len;
				return Count;
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
	};

	/* Прямоугольный канал 1 */
	struct SPulseChannel1: public SPulseChannel {
		using SPulseChannel::ShiftCount;
		using SPulseChannel::NegateFlag;
		using SPulseChannel::TimerPeriod;
		using SPulseChannel::SweepedTimer;
		using SPulseChannel::Valid;

		/* Обсчитать свип */
		void CalculateSweep() {
			uint16 Res;

			Res = TimerPeriod >> ShiftCount;
			if (NegateFlag)
				Res = ~Res;
			SweepedTimer = TimerPeriod + Res;
			Valid = (TimerPeriod > 7) && (NegateFlag || (SweepedTimer < 0x800));
		}
	};

	/* Прямоугольный канал 2 */
	struct SPulseChannel2: public SPulseChannel {
		using SPulseChannel::ShiftCount;
		using SPulseChannel::NegateFlag;
		using SPulseChannel::TimerPeriod;
		using SPulseChannel::SweepedTimer;
		using SPulseChannel::Valid;

		/* Обсчитать свип */
		void CalculateSweep() {
			uint16 Res;

			Res = TimerPeriod >> ShiftCount;
			if (NegateFlag)
				Res = -Res;
			SweepedTimer = TimerPeriod + Res;
			Valid = (TimerPeriod > 7) && (NegateFlag || (SweepedTimer < 0x800));
		}
	};

	/* Треугольный канал */
	struct STriangleChannel: SAPUUnit {
		using SAPUUnit::Pos;
		using SAPUUnit::Buffer;
		using SAPUUnit::MAX_BUF;
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
		/* Заполнить буфер */
		void FillBuffer(int Count) {
			int RealCount = 0;
			int Len = Count;

			if (Count == 0)
				return Count;
			if ((LinearCounter <= 0) || (LengthCounter <= 0) || (TimerPeriod < 2)) {
				if (TimerPeriod < 2) {
					Buffer[Pos].Sample = -1;
					SequencePos = (SequencePos + (Timer + Count) / (TimerPeriod + 1)) & 31;
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
	};

	/* Обновить буфер */
	void UpdateBuffer(SAPUUnit *APUUnit, int Clocks) {
		typename SAPUUnit::BufCode BufRet;
		for (;;) {
			BufRet = APUUnit->UpdateBuffer(Clocks);
			if (BufRet == SAPUUnit::BufOK)
				break;
			FlushBuffer();
		}
	}

	/* Функция обработки из события */
	inline void EventCall(int CallTime) {
		int i;

		Process(CallTime);
		/* Async is always 0?? */
		CycleData.Async = (CycleData.Async + CallTime) % ClockDivider;
		CycleData.NextEvent = EventTime[0];
		for (i = 0; i < MAX_EVENTS; i++)
			if (EventTime[i] >= 0) {
				/* Знак не изменится, если событие не было выполнено */
				EventTime[i] -= CallTime;
				if ((EventTime[i] < CycleData.NextEvent) && (EventTime[i] >= 0))
					CycleData.NextEvent = EventTime[i];
			}
		CycleData.InternalClock = -CycleData.Async;
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
		return FullClocks * ClockDivider;
	}

	/* Обработка события */
	inline void ProcessEvent() {
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
	}
	inline ~CAPU() {
	}

	/* Заполнить буфер */
	inline void FlushBuffer() {
	}
	/* Сброс */
	inline void Reset() {
		int i;

		memset(&CycleData, 0, sizeof(CycleData));
		for (i = 0; i < MAX_EVENTS; i++)
			Bus->GetClock()->DisableEvent(EventChain[i]);
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
		if ((CycleData.InternalClock + RunTime) > CycleData.NextEvent) {
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
