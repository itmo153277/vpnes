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

#ifndef __CLOCK_H_
#define __CLOCK_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>

namespace vpnes {

/* Данные события */
struct SEventData {
	bool Enabled; /* Активно */
	int Time; /* Время события */
};

/* Событие */
struct SEvent {
	SEvent *Previous; /* Предыдущее событие */
	SEvent *Next; /* Следующее событие */
	SEventData *Data; /* Данные события */
	/* Выполнить */
	virtual void Execute() = 0;

	inline SEvent() {}
	inline virtual ~SEvent() {}
};

/* Часы */
class CClock {
public:
	typedef std::vector<SEvent *> EventCollection;
private:
	/* Коллекция событий */
	EventCollection Events;
	/* Последнее активное событие */
	SEvent *Last;
	/* Первое активное событие */
	SEvent *First;
	/* Флаг остановки */
	bool Terminated;
	/* Текущее время */
	int Time;
	/* Ближайшее событие */
	int NextEventTime;
public:
	CClock();
	virtual ~CClock();

	/* Зарегестрировать событие */
	void RegisterEvent(SEvent *Event);
	/* Активировать событие */
	void EnableEvent(SEvent *Event);
	/* Деактивировать событие */
	void DisableEvent(SEvent *Event);
	/* Установить время */
	inline void SetEventTime(SEvent *Event, int Time) {
		Event->Data->Time = Time;
		if (NextEventTime > Time)
			NextEventTime = Time;
	}
	/* Обновить список */
	void UpdateList();
	/* Запустить часы */
	void Start();
	/* Сбросить часы */
	void Reset(int ResetTime);

	inline void Terminate() { Terminated = true; }
	inline const bool &IsTerminated() const { return Terminated; }
	inline const int &GetTime() const { return Time; }
};

}

#endif
