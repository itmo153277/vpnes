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

	/* Функция обработки из события */
	inline void EventCall(int CallTime) {
		int i;

		Process(CallTime);
		CycleData.Async = (CycleData.Async + CycleData.InternalClock) % ClockDivider;
		CycleData.NextEvent = EventTime[0];
		for (i = 0; i < MAX_EVENTS; i++)
			if (EventTime[i] > 0) {
				EventTime[i] -= CycleData.InternalClock;
				if (EventTime[i] < CycleData.NextEvent)
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
	CycleData.InternalClock = Time;
}

}

#endif
