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

typedef CPUGroup<1>::ID::NoBatteryID StateID;
typedef CPUGroup<2>::ID::NoBatteryID RegistersID;
typedef CPUGroup<3>::ID::NoBatteryID RAMID;
typedef CPUGroup<4>::ID::NoBatteryID InternalDataID;

}

/* CPU */
template <class _Bus>
class CCPU: public CDevice {
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

	/* Таблица опкодов */
	static const SOpcode Opcodes[1];

	/* Такты */
	int Cycles;

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
	
	/* Встроенная память */
	uint8 *RAM;

	/* Внутренние данные */
	struct SInternalData {
		bool Halt; /* Зависание */
		bool NMI; /* Не маскируемое прерывание */
		bool IRQ; /* Прерывание */
	} InternalData;

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
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 Address) {
			return ReadByte(CPU);
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 Address, uint8 Src) {
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
			uint8 Res;

			Address = GetAddr(CPU);
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res); /* WTF?! 0_o */
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* ZP */
	struct ZeroPage {
		inline static uint8 ReadByte(CCPU &CPU) {
			/* 3 лишних такта... забить */
			return CPU.RAM[CPU.ReadMemory(CPU.Registers.pc - 1)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			/* Собственно и тут тоже... */
			CPU.RAM[CPU.ReadMemory(CPU.Registers.pc - 1)] = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint8 Res;

			Address = CPU.ReadMemory(CPU.Registers.pc - 1);
			Res = CPU.RAM[Address];
			//CPU.Bus->GetClock()->Clock(9); /* Неизвестное поведение */
			/* Опять-таки, а какая разница? */
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 Address, uint8 Src) {
			CPU.RAM[Address] = Src;
		}
	};

	/* PC + значение */
	struct Relative {
		inline static uint16 GetAddr(CCPU &CPU) {
			//CPU.Bus->GetClock()->Clock(??); /* Неизвестное поведение */
			return CPU.Registers.pc + (sint8) CPU.ReadMemory(CPU.Registers.pc - 1);
		}
	};

	/* Адрес + X */
	struct AbsoluteX {
		inline static uint16 GetAddr(CCPU &CPU, uint16 &Address) {
			Address = CPU.ReadMemory(CPU.Registers.pc - 2) + CPU.Registers.x;
			return (CPU.ReadMemory(CPU.Registers.pc - 1) << 8) + Address;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			if (Page & 0x0100) { /* Перепрыгнули страницу */
				CPU.ReadMemory(Address - 0x0100); /* Промазал Ha-Ha */
				CPU.Cycles++;
			}
			return CPU.ReadMemory(Address);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100)); /* WTF?! */
			CPU.WriteMemory(Address, Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint16 Page;
			uint8 Res;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x100));
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res); /* WTF?! */
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* Адрес + Y */
	struct AbsoluteY {
		inline static uint16 GetAddr(CCPU &CPU, uint16 &Address) {
			Address = CPU.ReadMemory(CPU.Registers.pc - 2) + CPU.Registers.y;
			return (CPU.ReadMemory(CPU.Registers.pc - 1) << 8) + Address;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			if (Page & 0x0100) {
				CPU.ReadMemory(Address - 0x0100);
				CPU.Cycles++;
			}
			return CPU.ReadMemory(Address);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100));
			CPU.WriteMemory(Address, Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint16 Page;
			uint8 Res;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x100));
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res);
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* ZP + X */
	struct ZeroPageX {
		inline static uint8 GetAddr(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 1) + CPU.Registers.x;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[GetAddr(CPU)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[GetAddr(CPU)] = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			Address = GetAddr(CPU);
			return CPU.RAM[Address];
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.RAM[Address] = Src;
		}
	};

	/* ZP + Y */
	struct ZeroPageY {
		inline static uint8 GetAddr(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 1) + CPU.Registers.y;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[GetAddr(CPU)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[GetAddr(CPU)] = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			Address = GetAddr(CPU);
			return CPU.RAM[Address];
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.RAM[Address] = Src;
		}
	};

	/* Адрес + indirect */
	struct AbsoluteInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint16 Address;

			Address = CPU.ReadMemory(CPU.Registers.pc - 2) |
				(CPU.ReadMemory(CPU.Registers.pc - 1) << 8);
			return CPU.ReadMemory(Address) | (CPU.ReadMemory(\
				(Address & 0xff00) | ((Address + 1) & 0x00ff)) << 8);
		}
	};

	/* ZP + X + indirect */
	struct ZeroPageInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint8 AddressOffset;

			AddressOffset = CPU.ReadMemory(CPU.Registers.pc - 1) + CPU.Registers.x;
			CPU.Bus->GetClock()->Clock(9);
			return CPU.RAM[AddressOffset] | (CPU.RAM[(uint8) (AddressOffset + 1)] << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.WriteMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint8 Res;

			Address = GetAddr(CPU);
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res);
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* ZP + indirect + Y */
	struct ZeroPageIndY {
		inline static uint16 GetAddr(CCPU &CPU, uint16 &Address) {
			uint8 AddressOffset;

			AddressOffset = CPU.ReadMemory(CPU.Registers.pc - 1);
			Address = CPU.RAM[AddressOffset] + CPU.Registers.y;
			return (CPU.RAM[(uint8) (AddressOffset + 1)] << 8) + Address;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			if (Page & 0x0100) {
				CPU.ReadMemory(Address - 0x0100);
				CPU.Cycles++;
			}
			return CPU.ReadMemory(Address);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100));
			CPU.WriteMemory(Address, Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint16 Page;
			uint8 Res;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x100));
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res);
			return Res;
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
		RAM = (uint8 *) Bus->GetManager()->template GetPointer<CPUID::RAMID>(\
			0x0800 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<CPUID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
	}
	inline ~CCPU() {}

	/* Обработать такты */
	inline int Execute();

	/* Сброс */
	inline void Reset() {
		Registers.s -= 3;
		State.State |= 0x04; /* Interrupt */
		memset(&InternalData, 0, sizeof(InternalData));
		Registers.pc = Bus->ReadCPUMemory(0xfffc) | (Bus->ReadCPUMemory(0xfffd) << 8);
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		return RAM[Address & 0x07ff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		RAM[Address & 0x07ff] = Src;
	}
private:
	/* Команды CPU */

	/* Неизвестная команда */
	void OpIllegal();
};

template <class _Bus>
int CCPU<_Bus>::Execute() {
	uint8 opcode;

	if (InternalData.Halt) /* Зависли */
		return 3;
	/* Текущий опкод */
	opcode = ReadMemory(Registers.pc);
	Registers.pc += Opcodes[0].Length;
	Cycles = Opcodes[0].Cycles;
	/* Выполняем */
	(this->*Opcodes[0].Handler)();
	return Cycles * 3;
}

/* Команды */

/* Неизвестный опкод */
template <class _Bus>
void CCPU<_Bus>::OpIllegal() {
	VPNES_CPUHALT HaltData;

	/* Зависание */
	InternalData.Halt = true;
	HaltData.PC = Registers.pc;
	HaltData.A = Registers.a;
	HaltData.X = Registers.x;
	HaltData.Y = Registers.y;
	HaltData.S = Registers.s;
	HaltData.State = State.State;
	Bus->GetCallback()(VPNES_CALLBACK_CPUHALT, (void *) &HaltData);
}

/* Таблица опкодов */
template <class _Bus>
const typename CCPU<_Bus>::SOpcode CCPU<_Bus>::Opcodes[1] = {
	{1, 1, &CCPU<_Bus>::OpIllegal}
};

}

#endif
