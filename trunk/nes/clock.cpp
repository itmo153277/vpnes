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

#include "clock.h"
#include <cstddef>

using namespace vpnes;

/* CClock */

/* Конструктор */
CClock::CClock() {
	Last = NULL;
	First = NULL;
	Time = 0;
	NextEventTime = 0;
}

/* Деструктор */
CClock::~CClock() {
	EventCollection::iterator EventIter;

	for (EventIter = Events.begin(); EventIter != Events.end(); EventIter++)
		delete *EventIter;
}

/* Регистрация */
void CClock::RegisterEvent(SEvent *Event) {
	Event->Previous = NULL;
	Event->Next = NULL;
	Event->Data->Enabled = false;
	Events.push_back(Event);
}

/* Активировать событие */
void CClock::EnableEvent(SEvent *Event) {
	if (!Event->Data->Enabled) {
		Event->Previous = Last;
		Event->Next = NULL;
		if (Last != NULL)
			Last->Next = Event;
		else
			First = Event;
		Last = Event;
		Event->Data->Enabled = true;
	}
}

/* Деактивировать событие */
void CClock::DisableEvent(SEvent *Event) {
	if (Event->Data->Enabled) {
		if (SafeNext == Event)
			SafeNext = Event->Next;
		if (Event->Next != NULL) {
			Event->Next->Previous = Event->Previous;
			if (Event->Previous != NULL)
				Event->Previous->Next = Event->Next;
			else
				First = Event->Next;
		} else {
			Last = Event->Previous;
			if (Last != NULL)
				Last->Next = NULL;
			else
				First = NULL;
		}
		Event->Data->Enabled = false;
	}
}

/* Обновить список */
void CClock::UpdateList() {
	EventCollection::iterator EventIter;

	First = NULL;
	Last = NULL;
	for (EventIter = Events.begin(); EventIter != Events.end(); EventIter++)
		if ((*EventIter)->Data->Enabled) { /* Обновить регистрацию */
			(*EventIter)->Data->Enabled = false;
			EnableEvent(*EventIter);
		}
}

/* Запустить часы */
void CClock::Start() {
	SEvent *CurEvent = NULL;

	Terminated = false;
	for (;;) {
		if (CurEvent == NULL) {
			if (Terminated)
				break;
			CurEvent = First;
			Time = NextEventTime;
		}
		SafeNext = CurEvent->Next;
		if (CurEvent->Data->Time > Time) {
			if ((NextEventTime > CurEvent->Data->Time) || (NextEventTime == Time))
				NextEventTime = CurEvent->Data->Time;
		} else
			CurEvent->Execute();
		CurEvent = SafeNext;
	}
}

/* Сбросить часы */
void CClock::Reset(int ResetTime) {
	SEvent *CurEvent = NULL;

	while (CurEvent != NULL) {
		CurEvent->Data->Time -= ResetTime;
		CurEvent = CurEvent->Next;
	}
	NextEventTime -= ResetTime;
	Time -= ResetTime;
}
