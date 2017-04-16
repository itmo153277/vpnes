/**
 * @file
 *
 * Defines basic device classess
 */
/*
 NES Emulator
 Copyright (C) 2012-2017  Ivanov Viktor

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

 */

#ifndef VPNES_INCLUDE_CORE_DEVICE_HPP_
#define VPNES_INCLUDE_CORE_DEVICE_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <unordered_map>
#include <set>
#include <vpnes/vpnes.hpp>

namespace vpnes {

namespace core {

/**
 * Base device class
 */
class CDevice {
protected:
	/**
	 * Default constructor
	 * Put as protected for converting into meta-class
	 */
	CDevice() = default;
public:
	/**
	 * Default destructor
	 */
	virtual ~CDevice() = default;
};

/**
 * Clock ticks type
 */
typedef std::size_t clock_t;

/**
 * Default clocked device
 */
class CClockedDevice: public CDevice {
protected:
	/**
	 * Current clock value
	 */
	clock_t m_Clock;

	/**
	 * Executes till reaching current clock value
	 */
	virtual void execute() = 0;
	/**
	 * Synchronizes the clock
	 *
	 * @param ticks New clock value
	 */
	virtual void sync(clock_t ticks) {
	}
public:
	/**
	 * Constructs the object
	 */
	CClockedDevice() :
			m_Clock() {
	}
	/**
	 * Destroys the object
	 */
	~CClockedDevice() = default;

	/**
	 * Advances the clock
	 *
	 * @param ticks End time
	 */
	virtual void simulate(clock_t ticks) {
		setClock(ticks);
		execute();
	}

	/**
	 * Gets current clock value
	 *
	 * @return Current clock value
	 */
	clock_t getClock() const {
		return m_Clock;
	}
	/**
	 * Sets the end point for clock
	 *
	 * @param ticks Amount of ticks
	 */
	void setClock(clock_t ticks) {
		if (ticks < m_Clock) {
			sync(ticks);
			m_Clock = ticks;
		}
	}
	/**
	 * Resets the clock by ticks amount
	 *
	 * @param ticks Amount of ticks
	 */
	virtual void resetClock(clock_t ticks) {
		m_Clock -= ticks;
	}
};

/**
 * Event ID type
 */
typedef std::size_t eventId_t;

/**
 * Base device event class
 */
class CDeviceEvent {
protected:
	/**
	 * Time when firing
	 */
	clock_t m_Time;
	/**
	 * Event name
	 */
	const char *m_Name;
	/**
	 * Event id
	 */
	eventId_t m_Id;
	/**
	 * Flag enabling the event
	 */
	bool m_Enabled;
	/**
	 * Default constructor
	 * Put as protected for converting into meta-class
	 *
	 * @param time Firing time
	 * @param name Event name
	 */
	CDeviceEvent(const char *name, eventId_t id, clock_t time, bool enabled) :
			m_Time(time), m_Name(name), m_Id(id), m_Enabled(enabled) {
	}
	/**
	 * Triggered after the event was changed
	 */
	virtual void updateBack() = 0;
public:
	/**
	 * Default destructor
	 */
	virtual ~CDeviceEvent() = default;
	/**
	 * Fires the event
	 */
	virtual void fire() = 0;
	/**
	 * Synchronizes the clock
	 *
	 * @param ticks Amount of ticks
	 */
	void sync(clock_t ticks) {
		m_Time -= ticks;
	}
	/**
	 * Gets time when event will fire
	 *
	 * @return Time when firing
	 */
	clock_t getFireTime() const {
		return m_Time;
	}
	/**
	 * Updates fire time
	 *
	 * @param time Time when fired
	 */
	void setFireTime(clock_t time) {
		m_Time = time;
		updateBack();
	}
	/**
	 * Checks if event is enabled
	 *
	 * @return True if enabled
	 */
	bool isEnabled() const {
		return m_Enabled;
	}
	/**
	 * Enables or disables the event
	 *
	 * @param enabled True to enable, false to disable
	 */
	void setEnabled(bool enabled) {
		m_Enabled = enabled;
		updateBack();
	}
	/**
	 * Gets event id
	 *
	 * @return Event id
	 */
	eventId_t getId() const {
		return m_Id;
	}
};

/**
 * Event-based device
 */
class CEventDevice: public CClockedDevice {
private:
	/**
	 * Saved event data
	 */
	struct SEventData {
		/**
		 * Link to the event
		 */
		CDeviceEvent *m_Event;
		/**
		 * Saved time
		 */
		std::size_t m_Time;
		/**
		 * Saved flag if enabled
		 */
		bool m_Enabled;
		/**
		 * Constructs the object
		 *
		 * @param event Event to add
		 */
		SEventData(CDeviceEvent *event) {
			m_Event = event;
			update();
		}

		/**
		 * Updates the local data
		 */
		void update() {
			m_Time = m_Event->getFireTime();
			m_Enabled = m_Event->isEnabled();
		}
		/**
		 * Special routine for comparison of event data pointers
		 *
		 * @param left Left operand
		 * @param right Right operand
		 * @return *Left < *Right
		 */
		static bool compare(SEventData * const left, SEventData * const right) {
			if (!right->m_Enabled) {
				return true;
			} else if (!left->m_Enabled) {
				return false;
			}
			return left->m_Time < right->m_Time;
		}
	};
	/**
	 * Event data mapped to event ID
	 */
	typedef std::unordered_map<eventId_t, SEventData> EventMap;
	/**
	 * Event queue
	 */
	typedef std::set<SEventData *,
			bool (*)(SEventData * const, SEventData * const)> EventQueue;
	/**
	 * Event data mapped to event ID
	 */
	EventMap m_EventData;
	/**
	 * Event queue
	 */
	EventQueue m_EventQueue;
protected:
	/**
	 * Local end time
	 */
	clock_t m_LocalTime;

	/**
	 * Updates clock value
	 */
	void updateClock() {
		clock_t newClock = m_Clock;
		EventQueue::const_reverse_iterator iter = m_EventQueue.crbegin();

		if (iter == m_EventQueue.rend()) {
			newClock = m_LocalTime;
		} else {
			newClock = (*iter)->m_Time;
			if (newClock > m_LocalTime) {
				newClock = m_LocalTime;
			}
		}
		setClock(newClock);
	}
	/**
	 * Fires events that are available to
	 */
	void fireEvents() {
		for (;;) {
			EventQueue::const_reverse_iterator iter = m_EventQueue.crbegin();
			if (iter == m_EventQueue.rend() || (*iter)->m_Time > m_Clock) {
				break;
			}
			(*iter)->m_Event->fire();
		}
	}
public:
	/**
	 * Constructs the object
	 */
	CEventDevice() :
			m_EventData(), m_EventQueue(SEventData::compare), m_LocalTime() {
	}
	/**
	 * Destroys the object
	 */
	~CEventDevice() = default;

	/**
	 * Advances the clock
	 *
	 * @param ticks End time
	 */
	void simulate(clock_t ticks) {
		m_LocalTime = ticks;
		do {
			updateClock();
			execute();
			fireEvents();
		} while (m_Clock < m_LocalTime);
	}
	/**
	 * Update trigger for event
	 *
	 * @param eventId Event id
	 */
	void updateBack(eventId_t eventId) {
		SEventData &eventData = m_EventData.find(eventId)->second;
		if (eventData.m_Enabled) {
			m_EventQueue.erase(&eventData);
		}
		eventData.update();
		if (eventData.m_Enabled) {
			m_EventQueue.insert(&eventData);
		}
		updateClock();
	}
	/**
	 * Resets the clock
	 *
	 * @param ticks Amount of ticks
	 */
	void resetClock(clock_t ticks) {
		CClockedDevice::resetClock(ticks);
		for (auto &event : m_EventData) {
			SEventData &eventData = event.second;
			eventData.m_Event->sync(ticks);
			eventData.m_Time = eventData.m_Event->getFireTime();
		}
	}
	/**
	 * Registers new event
	 *
	 * @param event New event
	 */
	void registerEvent(CDeviceEvent &event) {
		auto newData = m_EventData.emplace(event.getId(), &event);
		if (newData.second) {
			SEventData &eventData = newData.first->second;
			if (eventData.m_Enabled) {
				m_EventQueue.insert(&eventData);
				updateClock();
			}
		}
	}
};

}

}

#endif /* VPNES_INCLUDE_CORE_DEVICE_HPP_ */
