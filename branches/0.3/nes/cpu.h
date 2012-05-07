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

typedef typename CPUGroup<1>::ID::NoBatteryID StateID;
typedef typename CPUGroup<2>::ID::NoBatteryID RegistersID;

}

/* CPU */
template <class _Bus>
class CCPU: public CDevice {
private:
	/* Шина */
	_Bus *Bus;

	/* Дополнительные такты */
	int AddCycles;

	/* Регистр состояния */
	struct SState {
		uint8 State;
		inline bool Negative() { return State & 0x80; }
		inline bool Overflow() { return State & 0x40; }
		inline bool Break() { return State & 0x10; }
		inline bool Decimal() { return State & 0x08; }
		inline bool Interrupt() { return State & 0x04; }
		inline bool Zero() { return State & 0x02; }
		inline bool Carry() { return State & 0x01; }
		inline void SetState(uint8 NewState) { State = NewState | 0x20; }
		inline void NFlag(uint8 Src) { State = (State & 0x7f) | (Src & 0x80); }
		inline void VFlag(bool Flag) { if (Flag) State |= 0x40; else State &= 0xbf; }
		inline void ZFlag(uint8 Src) { if (Src == 0) State |= 0x02; else State &= 0xfd; }
		inline void CFlag(bool Flag) { if (Flag) State |= 0x01; else State &= 0xfe; }
	} State;

	/* Регистры */
	struct SRegisters {
		uint8 a; /* Аккумулятор */
		uint8 x; /* X */
		uint8 y; /* Y */
		uint8 s; /* S */
		uint16 pc; /* PC */
	} Registers;

	/* Обращения к памяти */
	inline uint8 ReadMemory(uint16 Address) {
		uint8 Res;

		Res = Bus->ReadCPUMemory(Address);
		Bus->GetClock()->Clock(3);
		return Res;
	}
	inline void WriteMemory(uint16 Address, uint8 Src) {
		Bus->WriteCPUMemory(Address, Src);
		Bus->GetClock()->Clock(3);
	}

	/* Адресация */

	/* Аккумулятор */
	struct Accumulator {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Registers.a;
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Registers.a = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			return ReadByte(CPU);
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			WriteByte(CPU, Src);
		}
	};

	/* Без параметра */
	struct Implied {
	};

	/* В параметре значение */
	struct Immediate {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 1);
		}
	};

	/* В параметре адрес */
	struct Absolute {
		inline static uint16 GetAddr(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 2) |
				(CPU.ReadMemory(CPU.Registers.pc - 1) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.WriteMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			Address = GetAddr(CPU);
			return CPU.ReadMemory(CPU.AddrCache);
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

public:
	inline explicit CCPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<CPUID::StateID>(\
			&State, sizeof(State));
		memset(&State, 0, sizeof(State));
		Bus->GetManager()->template SetPointer<CPUID::RegistersID>(\
			&Registers, sizeof(Registers));
		memset(&Registers, 0, sizeof(Registers));
	}
	inline ~CCPU() {}

	/* Обработать такты */
	inline int Execute() {
		return 3;
	}

	/* Сброс */
	inline void Reset() {
	}
};

struct CPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CCPU<_Bus> rebinded;
	};
};

}

#endif
