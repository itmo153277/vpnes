/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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

#ifndef __CPU_H_
#define __CPU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <cstring>
#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace CPUID {

typedef CPUGroup<1>::ID::NoBatteryID RAMID;
typedef CPUGroup<2>::ID::NoBatteryID InternalDataID;

}

/* CPU */
template <class _Bus, class _Settings>
class CCPU {
public:
	/* Делитель частоты */
	enum { ClockDivier = _Settings::CPU_Divider };
private:
	/* Шина */
	_Bus *Bus;
	
	/* Обработчик инструкции */
	typedef void (CCPU::*OpHandler)();

	/* Описание опкода */
	struct SOpcode {
		int Cycles; /* Номиальное число тактов */
		int Length; /* Длина команды */
		OpHandler Handler; /* Обработчик */
	};

	/* Внутренние данные */
	struct SInternalData: public ManagerID<CPUID::InternalDataID> {
		enum {
			Reset = 0,
			Halt,
			Run
		} State; /* Текущее состояние */
		int AddCycles; /* Дополнительные циклы */ 
	} InternalData;

	/* Внутренние часы */
	int InternalClock;

	/* Встроенная память */
	uint8 *RAM;

	/* Обращения к памяти */
	inline uint8 ReadMemory(uint16 Address) {
		Bus->IncrementClock(ClockDivier);
		return Bus->ReadCPUMemory(Address);
	}
	inline void WriteMemory(uint16 Address, uint8 Src) {
		Bus->IncrementClock(ClockDivier);
		Bus->WriteCPUMemory(Address, Src);
	}

	/* Декодер операции */
	SOpcode *GetNextOpcode();
public:
	CCPU(_Bus *pBus) {
		Bus = pBus;
		RAM = static_cast<uint8 *>(Bus->GetManager()->\
			template GetPointer<ManagerID<CPUID::RAMID> >(0x800 * sizeof(uint8)));
		memset(RAM, 0xff, 0x0800 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		memset(&InternalData, 0, sizeof(InternalData));
	}
	inline ~CCPU() {}

	/* Обработать такты */
	void Execute();

	/* Сброс */
	inline void Reset() {
		InternalData.State = SInternalData::Reset;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		return RAM[Address & 0x07ff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		RAM[Address & 0x07ff] = Src;
	}
};

/* Обработчик тактов */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::Execute() {
	SOpcode *NextOpcode;
	int ClockCounter;

	if (InternalData.Satet == SInternalData::Halt)
		return;
	Bus->ResetInternalClock();
	ClockCounter = 0;
	while (ClockCounter < Bus->GetClock()->GetWaitClocks()) {
		if (InternalData.AddCycles > 0) {
			Bus->IncrementClock(InternalData.AddCycles);
			InternalClock = InternalData.AddCycles;
			InternalData.AddCycles = 0;
		} else
			InternalClock = 0;
		NextOpcode = GetNextOpcode();
		InternalClock += NextOpcode->Cycles;
		NextOpcode->Execute();
		ClockCounter += InternalClock;
		Bus->Synchronize(ClockCounter);
	}
}

/* Декодер операции */
template <class _Bus, class _Settings>
typename CCPU<_Bus, _Settings>::SOpcode *CCPU<_Bus, _Settings>::GetNextOpcode() {
	return NULL;
}

}

#endif
